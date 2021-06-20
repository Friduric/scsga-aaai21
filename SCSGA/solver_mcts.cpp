#include "solver_mcts.h"

#define PRINT_NEURAL_NETWORK_TIME_RATIO false

void solver_mcts::print_node(const uint32_t parent_node, const uint32_t child_task,
	const uint32_t current_depth, const std::string arm)
{
	std::cout << std::setw(current_depth * INDENTATION_IN_DEBUG_PRINT) << arm
		<< nodes[parent_node].get_child_node(child_task) << " "
		<< nodes[parent_node].selection_policy.get_n_simulations(child_task)
		<< std::endl;
}

void solver_mcts::print_sub_tree(const uint32_t root, const uint32_t current_depth, const std::string arm)
{
	MCTS_Node& node{ nodes[root] };
	for (int64_t i{ n_actions - 1 }; i >= n_actions / 2; --i)
	{
		if (node.has_expanded_child_node(i))
		{
			print_sub_tree(node.get_child_node(i), current_depth + 1, "/");
		}
		else
		{
			print_node(root, i, current_depth + 1, "/");
		}
	}
	print_node(node.parent_node, node.action_from_parent_node, current_depth, arm);
	for (int64_t i{ n_actions / 2 - 1 }; i >= 0; --i)
	{
		if (node.has_expanded_child_node(i))
		{
			print_sub_tree(node.get_child_node(i), current_depth + 1, "\\");
		}
		else
		{
			print_node(root, i, current_depth + 1, "\\");
		}
	}
}

uint32_t solver_mcts::add_new_child_node(const uint32_t parent_node, const uint32_t action)
{
	nodes.emplace_back
	(
		parent_node,
		action,
		n_actions,
		exploration_weight,
		variance_weight,
		estimation_weight
	);
	nodes[parent_node].child_nodes[action] = n_nodes++;
	return nodes.size() - 1;
}

coalition::value_t solver_mcts::brute_force(const uint32_t current_agent)
{
	if (current_agent >= n_agents)
	{
		++evaluated_solutions;
		
		temp_solution.recalculate_value(coalitional_values);

		if (temp_solution.get_value() > best_solution.get_value())
		{
			best_solution = temp_solution;

			if (_RunHillClimbToPolish)
			{
				solver_agent_greed::HillClimb
				(
					best_solution,
					coalitional_values,
					agent_order,
					generator,
					timer,
					false,
					25
				);
			}
		}
		return temp_solution.get_value();
	}
	
	// Try to assign this agent to all coalitions.
	coalition::value_t v_best_solution_value = coalition::NEG_INF;
	for (uint32_t n_task_index = 0; n_task_index < n_actions; ++n_task_index)
	{
		add_agent_to_temp_solution(agent_order[current_agent], n_task_index);
		v_best_solution_value = std::max(v_best_solution_value, brute_force(current_agent + 1));
		remove_agent_from_temp_solution(agent_order[current_agent], n_task_index);
	}
	return v_best_solution_value;
}

coalition::value_t solver_mcts::greedy_rollout(const uint32_t current_agent)
{
	// Brute-force.
	if (current_agent >= n_agents - brute_force_depth)
	{
		return brute_force(current_agent);
	}

	if (USE_GREEDY_ROULETTE)
	{
		if (rand() % 10000 < GREEDY_ROULETTE_CHANCE)
		{
			// Do random rollout.
			const uint32_t random_task_index{ uniform_action_distribution(generator) };
			add_agent_to_temp_solution(agent_order[current_agent], random_task_index);
			coalition::value_t rollout_value{ greedy_rollout(current_agent + 1) };
			remove_agent_from_temp_solution(agent_order[current_agent], random_task_index);
			return rollout_value;
		}
	}

	// Greedy rollout.
	uint32_t nBestTaskIndex = 0;
	coalition::value_t vBestTaskValue = std::numeric_limits<coalition::value_t>::lowest();
	for (uint32_t nTaskIndex = 0; nTaskIndex < coalitional_values->get_n_tasks(); ++nTaskIndex)
	{
		coalition::value_t vValueBeforeAssignment =
			coalitional_values->get_value_of(temp_solution.get_coalition(nTaskIndex), nTaskIndex);
		add_agent_to_temp_solution(agent_order[current_agent], nTaskIndex);
		coalition::value_t vValueAfterAssignment =
			coalitional_values->get_value_of(temp_solution.get_coalition(nTaskIndex), nTaskIndex);
		remove_agent_from_temp_solution(agent_order[current_agent], nTaskIndex);

		if (vValueAfterAssignment - vValueBeforeAssignment > vBestTaskValue)
		{
			nBestTaskIndex = nTaskIndex;
			vBestTaskValue = vValueAfterAssignment - vValueBeforeAssignment;
		}
	}
	add_agent_to_temp_solution(agent_order[current_agent], nBestTaskIndex);
	coalition::value_t value = greedy_rollout(current_agent + 1);
	remove_agent_from_temp_solution(agent_order[current_agent], nBestTaskIndex);
	return value;
}

coalition::value_t solver_mcts::random_rollout(uint32_t current_agent)
{
	// Brute-force rollout.
	if (current_agent >= n_agents - brute_force_depth)
	{
		return brute_force(current_agent);
	}

	// Random rollout.
	const uint32_t random_task_index{ uniform_action_distribution(generator) };
	add_agent_to_temp_solution(agent_order[current_agent], random_task_index);
	coalition::value_t rollout_value{ random_rollout(current_agent + 1) };
	remove_agent_from_temp_solution(agent_order[current_agent], random_task_index);
	return rollout_value;
}

coalition::value_t solver_mcts::tree_policy(const uint32_t current_node_index, const uint32_t current_agent)
{
	if (current_agent >= n_agents - brute_force_depth)
	{
		return brute_force(current_agent);
	}

	auto& current_selection_policy = nodes[current_node_index].selection_policy;
	const uint32_t action{ current_selection_policy.get_next_action(best_solution.get_value()) };
	if (nodes[current_node_index].should_expand_child_node(action))
	{
		// Add new node.
		const uint32_t child_node_index = add_new_child_node(current_node_index, action);
		add_agent_to_temp_solution(agent_order[current_agent], action);
		remove_agent_from_temp_solution(agent_order[current_agent], action);
	}

	add_agent_to_temp_solution(agent_order[current_agent], action);
	coalition::value_t action_value{ std::numeric_limits<coalition::value_t>::lowest() };
	if (nodes[current_node_index].has_expanded_child_node(action))
	{
		action_value = tree_policy(nodes[current_node_index].get_child_node(action), current_agent + 1);
	}
	else
	{
		if (USE_GREEDY_ROLLOUT)
		{
			action_value = std::max(action_value, greedy_rollout(current_agent + 1));
		}
		if (USE_RANDOM_ROLLOUT)
		{
			action_value = std::max(action_value, random_rollout(current_agent + 1));
		}
	}
	nodes[current_node_index].selection_policy.add_rollout_result(action, action_value);
	remove_agent_from_temp_solution(agent_order[current_agent], action);
	return action_value;
}

void solver_mcts::do_one_mcts_pass()
{

	temp_solution.reset(n_actions, n_agents);

	for (uint32_t current_agent{}; current_agent < n_agents - brute_force_depth; ++current_agent)
	{
		n_nodes = 1;
		nodes.resize(0, MCTS_Node(0, 0, n_actions, exploration_weight, variance_weight, estimation_weight));
		nodes.push_back(MCTS_Node(0, 0, n_actions, exploration_weight, variance_weight, estimation_weight));
		uint32_t current_node{ 0 };

		for (uint32_t simulation{ 0 }; simulation < SIMULATIONS_BEFORE_RESTART_AT_NEXT_DEPTH; ++simulation)
		{
			tree_policy(current_node, current_agent);
			if (timer.countdown_reached()) break;
		}

#ifdef MCTS_DEBUG
		print_sub_tree(current_node, 0);
		std::cout << "Press ENTER to continue..." << std::flush;
		std::string s;
		std::getline(std::cin, s);
#endif

		const uint32_t best_task{ nodes[current_node].selection_policy.get_best_action() };
		add_agent_to_temp_solution(agent_order[current_agent], best_task);
		current_node = nodes[current_node].child_nodes[best_task];
	}
}

instance_solution solver_mcts::solve(coalitional_values_generator* _coalitional_values)
{
	coalitional_values = _coalitional_values;

	n_actions = coalitional_values->get_n_tasks();
	n_agents = coalitional_values->get_n_agents();
	evaluated_solutions = 0;

	brute_force_depth = SIMULATION_BRUTE_FORCE_DEPTH < n_agents ? SIMULATION_BRUTE_FORCE_DEPTH : n_agents;

	uniform_action_distribution = std::uniform_int_distribution<uint32_t>(0, n_actions - 1);
	if (seed > 0)
	{
		generator.seed(seed);
	}

	best_solution.reset(n_actions, n_agents);
	// Initialize best solution to a random solution.
	for (uint32_t nAgentIndex = 0; nAgentIndex < n_agents; ++nAgentIndex)
	{
		best_solution.add_agent_to_coalition(nAgentIndex, uniform_action_distribution(generator));
	}
	best_solution.recalculate_value(coalitional_values);

	agent_order.resize(n_agents);
	for (uint32_t i = 0; i < n_agents; ++i)
	{
		agent_order[i] = i;
	}

	timer.start_countdown(vTimeLimit);

	if (vTimeLimit < 0) {
		do_one_mcts_pass();
	}
	else
	{
		uint32_t passes{ 0 };
		do
		{
			if (SHUFFLE_AGENTS_EVERY_PASS)
			{
				std::shuffle(agent_order.begin(), agent_order.end(), generator);
			}
			do_one_mcts_pass();
			++passes;
			//std::cout << "One pass. " << " Best value is now: " << best_solution.value << std::endl;
		} while (!timer.countdown_reached());
		//std::cout << "Did " << passes << " passes." << std::endl;
	}
	//std::cout << "MCTS evaluated solutions: " << evaluated_solutions << std::endl;

	return best_solution;
}
