#pragma once

#include "solver.h"
#include <iostream> // For testing.
#include <random>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <chrono>
#include "solver_agent_greed.h"

class solver_pure_random_search : public solver
{
	std::uniform_int_distribution<uint32_t> upd_generator;
	std::default_random_engine generator;
public:
	bool _RunHillClimbToPolish = false;
	unsigned seed = 0;
	uint64_t nIterations = 118200000000ULL; // PRS generates roughly 4.4 million solutions per second.

	instance_solution solve(coalitional_values_generator* problem) override;
};

