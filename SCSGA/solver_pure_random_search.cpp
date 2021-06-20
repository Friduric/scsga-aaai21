#include "solver_pure_random_search.h"

instance_solution solver_pure_random_search::solve(coalitional_values_generator* problem)
{
	if (seed > 0)
		generator.seed(seed);

	instance_solution best_solution, current_solution;
	best_solution.value = std::numeric_limits<coalition::value_t>().lowest();

	upd_generator = std::uniform_int_distribution<uint32_t>(0, problem->get_n_tasks() - 1);
	unsigned long long nMaxIterations = std::max<unsigned long long>(1ULL, nIterations);

	utility::date_and_time::timer timer{};
	timer.start_countdown(vTimeLimit);

	std::vector<unsigned int> agent_order(problem->get_n_agents());
	for (int i = 0; i < agent_order.size(); ++i)
	{
		agent_order[i] = i;
	}

	unsigned nIteration = 0;
	for (; nIteration < nMaxIterations; ++nIteration)
	{
		current_solution.reset(problem->get_n_tasks(), problem->get_n_agents());

		// Randomly assign agents. 
		for (uint32_t nAgentIndex = 0; nAgentIndex < problem->get_n_agents(); ++nAgentIndex)
		{
			uint32_t nRandomTaskIndex = upd_generator(generator);
			assert(nRandomTaskIndex >= 0 && nRandomTaskIndex < problem->get_n_tasks());
			current_solution.add_agent_to_coalition(nAgentIndex, nRandomTaskIndex);
		}

		current_solution.recalculate_value(problem);
		if (_RunHillClimbToPolish)
		{
			solver_agent_greed::HillClimb(current_solution, problem, agent_order, generator, timer);
		}

		// Evaluate solution.
		if (current_solution.value > best_solution.value)
		{
			best_solution = current_solution;
		}

		if (vTimeLimit < 0 || timer.countdown_reached()) {
			break;
		}
	}

	return best_solution;
}