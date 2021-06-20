#include "solver_swap_random_search.h"

instance_solution solver_swap_random_search::solve(coalitional_values_generator* problem)
{
	if (seed > 0)
		generator.seed(seed);

	instance_solution best_solution, current_solution;
	best_solution.value = std::numeric_limits<coalition::value_t>().lowest();

	TaskIndexUniformGenerator = std::uniform_int_distribution<uint32_t>(0, problem->get_n_tasks() - 1);
	AgentIndexUniformGenerator = std::uniform_int_distribution<uint32_t>(0, problem->get_n_agents() - 1);
	const unsigned long long nMaxIterations = std::max<unsigned long long>(1ULL, nIterations);

	std::chrono::high_resolution_clock::time_point t1, t2;
	if (vTimeLimit >= 0) {
		t1 = std::chrono::high_resolution_clock::now();
	}

	std::vector<uint32_t> nAgentAssignments(problem->get_n_agents(), 0);
	current_solution = instance_solution();

	current_solution.reset(problem->get_n_tasks(), problem->get_n_agents());

	// Randomly assign agents to create an initial solution.
	for (uint32_t nAgentIndex = 0; nAgentIndex < problem->get_n_agents(); ++nAgentIndex)
	{
		uint32_t nRandomTaskIndex = TaskIndexUniformGenerator(generator);
		assert(nRandomTaskIndex >= 0 && nRandomTaskIndex < problem->get_n_tasks());
		current_solution.add_agent_to_coalition(nAgentIndex, nRandomTaskIndex);
		nAgentAssignments[nAgentIndex] = nRandomTaskIndex;
	}

	current_solution.recalculate_value(problem);
	best_solution = current_solution;

	for (unsigned nIteration = 0; nIteration < nMaxIterations; ++nIteration)
	{
		const uint32_t nAgentIndex = AgentIndexUniformGenerator(generator);
		for (unsigned nSwapTries = 0; nSwapTries < nMaxSwapTries; ++nSwapTries)
		{
			const uint32_t nRandomTaskIndex = TaskIndexUniformGenerator(generator);
			assert(nRandomTaskIndex >= 0 && nRandomTaskIndex < problem->get_n_tasks());
			if (nRandomTaskIndex != nAgentAssignments[nAgentIndex])
			{
				float& vValue = current_solution.value;
				const uint32_t nOldTaskIndex = nAgentAssignments[nAgentIndex];
				const uint32_t nNewTaskIndex = nRandomTaskIndex;

				vValue -= problem->get_value_of(current_solution.get_coalition(nOldTaskIndex), nOldTaskIndex);
				vValue -= problem->get_value_of(current_solution.get_coalition(nNewTaskIndex), nNewTaskIndex);
				current_solution.remove_agent_from_coalition(nAgentIndex, nOldTaskIndex);
				current_solution.add_agent_to_coalition(nAgentIndex, nNewTaskIndex);
				vValue += problem->get_value_of(current_solution.get_coalition(nOldTaskIndex), nOldTaskIndex);
				vValue += problem->get_value_of(current_solution.get_coalition(nNewTaskIndex), nNewTaskIndex);
				nAgentAssignments[nAgentIndex] = nNewTaskIndex;
				// current_solution.recalculate_value(problem);
				if (current_solution.value > best_solution.value)
				{
					best_solution = current_solution;
				}
				break;
			}
		}

		if (vTimeLimit >= 0)
		{
			t2 = std::chrono::high_resolution_clock::now();
			auto elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
			// std::cout << "elapsed: " << elapsed_time << std::endl;
			if (elapsed_time >= vTimeLimit)
			{
				break;
			}
		}
	}

	return best_solution;
}