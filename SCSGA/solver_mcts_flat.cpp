#include "solver_mcts_flat.h"

coalition::value_t solver_mcts_flat::rollout(coalitional_values_generator* coalitional_values, instance_solution& solution, uint32_t current_agent)
{
	if (current_agent >= coalitional_values->get_n_agents())
	{
		return solution.recalculate_value(coalitional_values);
	}
	else
	{
		const uint32_t random_task_index{ uniform_distribution(generator) };
		solution.add_agent_to_coalition(current_agent, random_task_index);
		coalition::value_t rollout_value{ rollout(coalitional_values, solution, current_agent + 1) };
		solution.remove_agent_from_coalition(current_agent, random_task_index);
		return rollout_value;
	}
}

instance_solution solver_mcts_flat::do_one_mcts_pass(coalitional_values_generator* coalitional_values,
	const uint32_t simulations_per_level,
	SelectionPolicy& selection_policy)
{
	instance_solution solution;
	solution.reset(coalitional_values->get_n_tasks(), coalitional_values->get_n_agents());

	for (uint32_t current_agent{ 0 }; current_agent < coalitional_values->get_n_agents(); ++current_agent)
	{
		selection_policy.reset(coalitional_values->get_n_tasks());
		std::vector<coalition::value_t> total_rollout_value(coalitional_values->get_n_agents(), std::numeric_limits<coalition::value_t>().lowest());
		for (uint32_t simulation_step{ 0 }; simulation_step < simulations_per_level; ++simulation_step)
		{
			const uint32_t rollout_task_index{ selection_policy.get_next_action() };
			solution.add_agent_to_coalition(current_agent, rollout_task_index);
			const coalition::value_t rollout_result{ rollout(coalitional_values, solution, current_agent + 1) };
			selection_policy.add_rollout_result(rollout_task_index, rollout_result);
			solution.remove_agent_from_coalition(current_agent, rollout_task_index);
		}
		const uint32_t best_action{ selection_policy.get_best_action() };
		solution.add_agent_to_coalition(current_agent, best_action);
	}

	solution.recalculate_value(coalitional_values);
	return solution;
}

inline bool solver_mcts_flat::time_limit_reached(const std::chrono::high_resolution_clock::time_point& start) const
{
	const std::chrono::high_resolution_clock::time_point current_time{ std::chrono::high_resolution_clock::now() };
	const auto passed_time{ std::chrono::duration_cast<std::chrono::duration<double>>(current_time - start).count() };
	return passed_time >= vTimeLimit;
}

instance_solution solver_mcts_flat::solve(coalitional_values_generator* coalitional_values)
{
	uniform_distribution = std::uniform_int_distribution<uint32_t>(0, coalitional_values->get_n_tasks() - 1);
	if (seed > 0)
	{
		generator.seed(seed);
	}

	SPMCTSSelectionPolicy selection_policy{ coalitional_values->get_n_tasks(), exploration_weight, variance_weight, estimation_weight };
	instance_solution best_solution;
	if (vTimeLimit < 0) {
		best_solution = do_one_mcts_pass(coalitional_values, SIMULATIONS_PER_ACTION_NO_TIME_LIMIT, selection_policy);
	}
	else
	{
		uint32_t passes{ 1 };
		std::chrono::high_resolution_clock::time_point start_time{ std::chrono::high_resolution_clock::now() };
		best_solution = do_one_mcts_pass(coalitional_values, coalitional_values->get_n_tasks() * SIMULATIONS_PER_ACTION_TIME_LIMIT, selection_policy);
		while (!time_limit_reached(start_time))
		{
			instance_solution current_solution{ do_one_mcts_pass(coalitional_values, coalitional_values->get_n_tasks() * SIMULATIONS_PER_ACTION_TIME_LIMIT, selection_policy) };
			if (current_solution.value > best_solution.value)
			{
				best_solution = current_solution;
			}
			++passes;
		}
		std::cout << "Did " << passes << " passes." << std::endl;
	}

	return best_solution;
}