#pragma once

#include <iostream> // For testing.
#include <random>
#include <stack>
#include <algorithm>
#include <numeric>

#include "solver.h"
#include "utility.h"
#include "coalitional_values_generator.h"

class solver_agent_greed : public solver
{
private:
	std::vector<uint32_t> const* _coalition_size_bounds = nullptr;
	std::default_random_engine _generator;

	void greedily_assign_agents(
		coalitional_values_generator* problem,
		instance_solution& solution,
		utility::date_and_time::timer& timer,
		std::vector<uint32_t> agents_to_assign = {}
	);
public:
	unsigned _nSeed = 0;
	bool _RunMultipleTimesWithShuffledAgents = true;
	bool _RunHillClimbToPolish = false;

	static void LocalSearch2Opt
	(
		instance_solution& SolutionToImprove, // The solution which we want to improve.
		coalitional_values_generator* pProblem, // The problem that we want to solve.
		std::vector<unsigned int>& AgentOrder,
		std::default_random_engine& Generator,
		utility::date_and_time::timer& timer, // Countdown timer initialized with desired time limit.
		const int _nMaximumNumberOfIterations
	);

	static void HillClimb
	(
		instance_solution& SolutionToImprove, // The solution which we want to improve.
		coalitional_values_generator* pProblem, // The problem that we want to solve.
		std::vector<unsigned int>& AgentOrder,
		std::default_random_engine& Generator,
		utility::date_and_time::timer& timer, // Countdown timer initialized with desired time limit.
		const bool _ShuffleAgentsEachHillClimbIteration = false,
		const int _HillClimbSwapLimit = -1 // -1 means no limit; keep on swapping for as long as we can improve the solution.
	);

	instance_solution solve(coalitional_values_generator* problem, std::vector<uint32_t> const* coalition_size_bounds);
	instance_solution solve(coalitional_values_generator* problem, instance_solution partial_solution);
	instance_solution solve(coalitional_values_generator* problem);
};

