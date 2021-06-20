#pragma once

#include "solver.h"
#include <iostream> // For testing.
#include <random>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <chrono>

class solver_swap_random_search : public solver
{
	std::uniform_int_distribution<uint32_t> TaskIndexUniformGenerator;
	std::uniform_int_distribution<uint32_t> AgentIndexUniformGenerator;
	std::default_random_engine generator;

public:
	unsigned seed = 0;
	uint32_t nMaxSwapTries = 3u;
	uint64_t nIterations = 118200000000ULL; // PRS generates roughly 4.4 million solutions per second.

	instance_solution solve(coalitional_values_generator* problem) override;
};

