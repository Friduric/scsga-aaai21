#pragma once
#include "solver.h"
class solver_brute_force : public solver
{
private:
	coalitional_values_generator* problem = nullptr;
	instance_solution best_solution;

	void solve_brute_force_recursive(const uint32_t n_agent_index, instance_solution& current_solution);

public:
	instance_solution solve(coalitional_values_generator* _problem) override;
};

