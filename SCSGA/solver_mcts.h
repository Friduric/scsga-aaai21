#pragma once

#include "solver.h"
#include "utility.h"
#include "selection_policy.h"

#include <algorithm> // Shuffle.
#include <vector>
#include <iomanip>

#include "solver_agent_greed.h"

// #define MCTS_DEBUG // Uncomment to enable debug printouts.

class solver_mcts : public solver
{
private:
	// Basic settings.
	static const uint32_t SIMULATIONS_BEFORE_EXPANSION{ 1 };
	static const uint32_t SIMULATIONS_BEFORE_RESTART_AT_NEXT_DEPTH{ 2000 };
	static const uint32_t SIMULATION_BRUTE_FORCE_DEPTH{ 0 };

	static const bool SHUFFLE_AGENTS_EVERY_PASS{ true };

	// Rollout settings.
	static const bool USE_GREEDY_ROLLOUT{ false };
	static const bool USE_GREEDY_ROULETTE{ false }; // If this is true, some greedy evaluations turn random.
	static const int GREEDY_ROULETTE_CHANCE = 0; // "Greedy roulette probability" with 10 = 0.10%. 
	static const bool USE_RANDOM_ROLLOUT{ true };

	// Printouts.
	static const uint32_t INDENTATION_IN_DEBUG_PRINT{ 8 };

public:
	// Settings (not constants, but based on external input data).
	bool _RunHillClimbToPolish = false;

private:
	struct MCTS_Node
	{
		uint32_t parent_node;
		uint32_t action_from_parent_node;
		SPMCTSSelectionPolicy selection_policy;
		std::vector<uint32_t> child_nodes;

		MCTS_Node
		(
			const uint32_t parent_node,
			const uint32_t action_from_parent_node,
			const uint32_t n_tasks,
			const double exploration_weight,
			const double variance_weight,
			const double estimation_weight
		) :
			parent_node{ parent_node },
			action_from_parent_node{ action_from_parent_node },
			selection_policy(n_tasks, exploration_weight, variance_weight, estimation_weight),
			child_nodes(n_tasks, 0)
		{

		}

		bool has_expanded_child_node(const uint32_t action)
		{
			return child_nodes[action] > 0;
		}

		bool should_expand_child_node(const uint32_t action)
		{
			return selection_policy.get_n_simulations(action) == SIMULATIONS_BEFORE_EXPANSION;
		}

		uint32_t get_child_node(const uint32_t action)
		{
			return child_nodes[action];
		}
	};

	uint32_t evaluated_solutions = 0;
	uint32_t n_agents;
	uint32_t n_actions;
	uint32_t n_nodes{ 0 };
	uint32_t brute_force_depth{ 1 };
	std::vector<MCTS_Node> nodes;

	// Problem definition.
	coalitional_values_generator* coalitional_values;

	// Agent book-keeping.
	std::vector<uint32_t> agent_order;
	std::vector<int> agent_assignment_indices;

	// Solutions.
	instance_solution best_solution, temp_solution;

	// Randomization.
	unsigned seed{ 0 };
	std::uniform_int_distribution<uint32_t> uniform_action_distribution;
	std::default_random_engine generator;

	// Timestamps.
	utility::date_and_time::timer timer{};

	void print_node(const uint32_t parent_node, const uint32_t child_task,
		const uint32_t current_depth, const std::string arm);

	void print_sub_tree(const uint32_t root, const uint32_t current_depth, const std::string arm = "");

	// Adds a new child node to a given node, and returns that nodes node index.
	uint32_t add_new_child_node(const uint32_t parent_node, const uint32_t action);

	coalition::value_t brute_force(const uint32_t current_agent);

	coalition::value_t greedy_rollout(const uint32_t current_agent);

	coalition::value_t random_rollout(uint32_t current_agent);

	coalition::value_t tree_policy(const uint32_t current_node_index, const uint32_t current_agent);

	void do_one_mcts_pass();

	inline void add_agent_to_temp_solution(const uint32_t agent_index, const uint32_t task_index)
	{
		temp_solution.add_agent_to_coalition(agent_index, task_index);
	}

	inline void remove_agent_from_temp_solution(const uint32_t agent_index, const uint32_t task_index)
	{
		temp_solution.remove_agent_from_coalition(agent_index, task_index);
	}

public:
	float exploration_weight{ -0.18 };
	float variance_weight{ 0.33 };
	float estimation_weight{ 1.0 };

	instance_solution solve(coalitional_values_generator* _coalitional_values);
};
