#pragma once

#include "solver.h"
#include "solver_agent_greed.h"
#include <iostream> // For testing.
#include <random>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <chrono>

class solver_annealing : public solver
{
	std::uniform_int_distribution<uint32_t> TaskIndexUniformGenerator;
	std::uniform_int_distribution<uint32_t> AgentIndexUniformGenerator;
	std::uniform_real_distribution<float> AcceptanceUniformGenerator;
	std::default_random_engine generator;

public:
	unsigned seed = 0;
	uint32_t nMaxSwapTries = 3u;
	uint64_t nIterations = 118200000000ULL;
	bool _RunHillClimbToPolish = false;
	instance_solution solve(coalitional_values_generator* problem) override;
};