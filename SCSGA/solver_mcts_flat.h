#pragma once

#include <random>
#include <chrono>

#include "solver.h"
#include "selection_policy.h"

class solver_mcts_flat : public solver {
private:

	static const uint32_t SIMULATIONS_PER_ACTION_TIME_LIMIT{ 20 }; // 2250 => ~0.3s on Intel 7700k
	static const uint32_t SIMULATIONS_PER_ACTION_NO_TIME_LIMIT{ 2250 };

	float exploration_weight{ -0.18 };
	float variance_weight{ 0.33 };
	float estimation_weight{ 1.0 };

	unsigned seed{ 0 };
	std::uniform_int_distribution<uint32_t> uniform_distribution;
	std::default_random_engine generator;

	coalition::value_t rollout(coalitional_values_generator* coalitional_values, instance_solution& solution, uint32_t current_agent);

public:
	instance_solution do_one_mcts_pass(coalitional_values_generator* coalitional_values,
		const uint32_t simulations_per_level,
		SelectionPolicy& selection_policy);

	inline bool time_limit_reached(const std::chrono::high_resolution_clock::time_point& start) const;

	instance_solution solve(coalitional_values_generator* coalitional_values) override;
};
