#include "solver_mp_AGI.h"

instance_solution solver_mp_AGI::solve(coalitional_values_generator* problem)
{
	auto initial_solution = solver_agent_greed().solve(problem);
	MPAnytime::CAnytimeSearcher generator(problem, problem->get_n_agents(), problem->get_n_tasks(), problem->get_data());
	return generator.FindOptimalCoalitionStructure(&initial_solution, bUseAGIForSubspaceLowerBounds, vTimeLimit);
}