#include "solver_task_greed.h"

instance_solution solver_task_greed::solve(coalitional_values_generator* pProblem, std::vector<uint32_t> const* coalition_size_bounds)
{
	assert(pProblem->get_n_agents() < 32);

	// Initialize result.
	instance_solution solution;
	solution.reset(pProblem->get_n_tasks(), pProblem->get_n_agents());
	solution.recalculate_value(pProblem);

	// Initialize unassigned agents.
	uint32_t nUnassignedAgentsMask = 0ULL;
	for (uint32_t i = 0ULL; i < pProblem->get_n_agents(); ++i)
	{
		nUnassignedAgentsMask |= (1 << i);
	}

	// Initialize unassigned tasks.
	std::vector<uint32_t> UnassignedTasks;
	UnassignedTasks.reserve(pProblem->get_n_tasks());
	for (uint32_t nTask = 0; nTask < pProblem->get_n_tasks(); ++nTask)
	{
		UnassignedTasks.push_back(nTask);
	}

	// Generate a solution greedily (potentially suboptimally).
	while (nUnassignedAgentsMask > 0 && UnassignedTasks.size() > 0)
	{
		if (UnassignedTasks.size() == 1)
		{
			const auto nTask = UnassignedTasks[0];
			solution.set_coalition(nTask, nUnassignedAgentsMask);
			solution.value -= pProblem->get_value_of(0, nTask);
			solution.value += pProblem->get_value_of(nUnassignedAgentsMask, nTask);
			return solution;
		}

		// Keep track of best possible coalition-to-task assignment this "round".
		uint32_t nBestCoalitionMask = 0;
		uint32_t nBestTask = UnassignedTasks[0];
		uint32_t nBestTaskIndex = 0;
		float vBestValue = std::numeric_limits<float>().lowest();

		// Helper variables.
		const uint32_t nNumberOfUnassignedAgents = utility::bits::calc_number_of_bits_set_32bit(nUnassignedAgentsMask);
		const uint32_t nNumberOfPossibleCoalitions = (1 << nNumberOfUnassignedAgents);

		// Try all possible coalitions that can be created with the unassigned agents.
		for (uint32_t nSkewedCoalitionMask = 0; nSkewedCoalitionMask < nNumberOfPossibleCoalitions; ++nSkewedCoalitionMask)
		{
			const uint32_t nRealCoalitionMask = utility::bits::calc_parallel_bits_deposit_32bit(nSkewedCoalitionMask, nUnassignedAgentsMask);
			const uint32_t nCoalitionAgentCount = utility::bits::calc_number_of_bits_set_32bit(nRealCoalitionMask);

			for (uint32_t nTaskIndex = 0; nTaskIndex < UnassignedTasks.size(); ++nTaskIndex)
			{
				uint32_t nTask = UnassignedTasks[nTaskIndex];
				if (coalition_size_bounds != nullptr)
				{
					if ((*coalition_size_bounds)[nTask] != nCoalitionAgentCount) continue;
				}

				float vPreviousValue = pProblem->get_value_of(0, nTask);
				float vNewValue = pProblem->get_value_of(nRealCoalitionMask, nTask);

				if (vNewValue - vPreviousValue > vBestValue)
				{
					nBestTaskIndex = nTaskIndex;
					nBestTask = nTask;
					nBestCoalitionMask = nRealCoalitionMask;
					vBestValue = vNewValue - vPreviousValue;
				}
			}
		}

		nUnassignedAgentsMask &= (~nBestCoalitionMask);

		solution.value -= pProblem->get_value_of(0, nBestTask);
		solution.value += pProblem->get_value_of(nBestCoalitionMask, nBestTask);

		solution.set_coalition(nBestTask, nBestCoalitionMask);

		std::swap(UnassignedTasks[nBestTaskIndex], UnassignedTasks[UnassignedTasks.size() - 1]);
		UnassignedTasks.pop_back();
	}

	return solution;
}

instance_solution solver_task_greed::solve(coalitional_values_generator* problem)
{
	return solve(problem, nullptr);
}
