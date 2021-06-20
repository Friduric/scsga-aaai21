#pragma once

#include "solver.h"

#include "utility.h"

#include <iostream> // For testing.
#include <cassert>
#include <algorithm>

class solver_task_greed : public solver
{
public:
	instance_solution solve(coalitional_values_generator* pProblem, std::vector<uint32_t> const* coalition_size_bounds);
	instance_solution solve(coalitional_values_generator* problem) override;
};