#include "solver_annealing.h"

instance_solution solver_annealing::solve(coalitional_values_generator* problem)
{
	if (seed > 0)
		generator.seed(seed);

	instance_solution best_solution, current_solution;
	best_solution.value = std::numeric_limits<coalition::value_t>().lowest();

	TaskIndexUniformGenerator = std::uniform_int_distribution<uint32_t>(0, problem->get_n_tasks() - 1);
	AgentIndexUniformGenerator = std::uniform_int_distribution<uint32_t>(0, problem->get_n_agents() - 1);
	AcceptanceUniformGenerator = std::uniform_real_distribution<float>(0.0f, 1.0f);
	const unsigned long long nMaxIterations = std::max<unsigned long long>(1ULL, nIterations);

	utility::date_and_time::timer timer{};
	timer.start_countdown(vTimeLimit);

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

	std::vector<uint32_t> agent_order(problem->get_n_agents());
	for (uint32_t i = 0; i < problem->get_n_agents(); ++i)
	{
		agent_order[i] = i;
	}


	current_solution.recalculate_value(problem);
	best_solution = current_solution;

	unsigned nIteration = 0, solutions_evaluated = 0, solutions_improved = 0, reverts = 0, non_reverts = 0;

	float T = 10000000.0f; // large number.

	for (nIteration = 0; nIteration < nMaxIterations; ++nIteration)
	{
		float prevValue = current_solution.value;

		// Pick new random neighbour.
		const uint32_t nAgentIndex = AgentIndexUniformGenerator(generator);
		const uint32_t nRandomTaskIndex = TaskIndexUniformGenerator(generator);
		if (nRandomTaskIndex != nAgentAssignments[nAgentIndex])
		{
			float& vValue = current_solution.value;
			const uint32_t nOldTaskIndex = nAgentAssignments[nAgentIndex];
			const uint32_t nNewTaskIndex = nRandomTaskIndex;

			float oldsum =
				problem->get_value_of(current_solution.get_coalition(nOldTaskIndex), nOldTaskIndex) +
				problem->get_value_of(current_solution.get_coalition(nNewTaskIndex), nNewTaskIndex);

			vValue -= oldsum;
			current_solution.remove_agent_from_coalition(nAgentIndex, nOldTaskIndex);
			current_solution.add_agent_to_coalition(nAgentIndex, nNewTaskIndex);

			float newsum =
				problem->get_value_of(current_solution.get_coalition(nOldTaskIndex), nOldTaskIndex) +
				problem->get_value_of(current_solution.get_coalition(nNewTaskIndex), nNewTaskIndex);

			vValue += newsum;
			nAgentAssignments[nAgentIndex] = nNewTaskIndex;

			++solutions_evaluated;

			if (current_solution.value > best_solution.value) // Plus small epsilon, since we do a lot of additions / subtractions.
			{
				best_solution = current_solution;
				if (_RunHillClimbToPolish)
				{
					solver_agent_greed::HillClimb(best_solution, problem, agent_order, generator, timer, true);
				}

				// current_solution.value = best_solution.value;
				++solutions_improved;
			}
			else
			{
				// Annealing: We always accept a solution if it is better than the previous one; else, only do it with a 
				// specific probability following Kirkpatrick et al.
				if (current_solution.value < prevValue)
				{
					float p = AcceptanceUniformGenerator(generator); // Uniform real number between 0 and 1.
					// Acceptance probability function as defined by Kirkpatrick et al.
					float apf = exp((current_solution.value - prevValue) / (T * best_solution.value));
					// std::cout << current_solution.value << " " << prevValue << " " << T << " " << apf << std::endl;
					if (apf < p)
					{
						// Revert.
						vValue -= newsum;
						current_solution.add_agent_to_coalition(nAgentIndex, nOldTaskIndex);
						current_solution.remove_agent_from_coalition(nAgentIndex, nNewTaskIndex);
						vValue += oldsum;
						nAgentAssignments[nAgentIndex] = nOldTaskIndex;
						++reverts;
					}
					else
					{
						++non_reverts;
					}
				}
				else
				{
					// Accept new solution.
				}
			}
		}

		if (vTimeLimit >= 0) {
			if (timer.countdown_reached())
			{
				break;
			}
			T = float(vTimeLimit) / timer.get_time() - 1;
		}
		else {
			// Calculate temperature.
			T = float(nMaxIterations) / float(nIteration + 1) - 1;
		}
	}

	//std::cout << "Simulated annealing iterations: " << nIteration << "; solutions evaluted: " << solutions_evaluated << "; solutions found better: " << solutions_improved << std::endl;
	//std::cout << "Reverts: " << reverts << "; non-reverts: " << non_reverts << std::endl;
	best_solution.recalculate_value(problem);
	return best_solution;
}
