#include "solver_agent_greed.h"

namespace 
{
	// Hill climb iteration in O(nm); for each agent, move it to the coalition that maximizes delta value.
	bool LocalSearch_AgentGreed
	(
		instance_solution& SolutionToImprove,
		coalitional_values_generator* pProblem,
		std::vector<unsigned int>& AgentOrder
	)
	{
		auto AgentAssignmentIndices = SolutionToImprove.get_coalition_indices_of_agents(pProblem->get_n_agents());
		
		int nBestAgentIndex = 0;
		int nBestCoalitionIndexFrom = AgentAssignmentIndices[0];
		int nBestCoalitionIndexTo = nBestCoalitionIndexFrom;
		coalition::value_t vBestDiff = 0;

		for (const uint32_t nAgentIndex : AgentOrder)
		{
			int nCoalitionIndexFrom = AgentAssignmentIndices[nAgentIndex];
			
			// Remove agent from initial coalition.
			coalition::value_t vValueOfOldCoalitionWithAgent = 
				SolutionToImprove.calc_value_of_coalition(pProblem, nCoalitionIndexFrom);
			SolutionToImprove.remove_agent_from_coalition(nAgentIndex, nCoalitionIndexFrom);
			coalition::value_t vValueOfOldCoalitionWithoutAgent = 
				SolutionToImprove.calc_value_of_coalition(pProblem, nCoalitionIndexFrom);
			coalition::value_t vDiffOld = vValueOfOldCoalitionWithoutAgent - vValueOfOldCoalitionWithAgent;

			for (uint32_t j = 0; j < pProblem->get_n_tasks(); ++j)
			{
				if (nCoalitionIndexFrom == j) continue; // Don't try to assign yourself to the coalition you are already in.

				// Add agent and calc value.
				coalition::value_t vValueOfNewCoalitionWithoutAgent = 
					SolutionToImprove.calc_value_of_coalition(pProblem, j);
				SolutionToImprove.add_agent_to_coalition(nAgentIndex, j);
				coalition::value_t vValueOfNewCoalitionWithAgent = 
					SolutionToImprove.calc_value_of_coalition(pProblem, j);
				SolutionToImprove.remove_agent_from_coalition(nAgentIndex, j);

				// Compute what we gained (or lost).
				coalition::value_t vDiffNew = vValueOfNewCoalitionWithAgent - vValueOfNewCoalitionWithoutAgent;
				coalition::value_t vDiffTot = vDiffNew + vDiffOld;

				const bool betterSwap = vDiffTot > vBestDiff;
				if (betterSwap)
				{
					// Better assignment.
					vBestDiff = vDiffTot;
					nBestCoalitionIndexFrom = nCoalitionIndexFrom;
					nBestCoalitionIndexTo = j;
					nBestAgentIndex = nAgentIndex;
				}
			}

			SolutionToImprove.add_agent_to_coalition(nAgentIndex, nCoalitionIndexFrom);
		}

		const bool bFoundImprovement = nBestCoalitionIndexTo != nBestCoalitionIndexFrom;
		if (bFoundImprovement)
		{
			// Update feasible solution since we found a better solution.
			SolutionToImprove.value += vBestDiff; // Should always be positive here.
			SolutionToImprove.remove_agent_from_coalition(nBestAgentIndex, nBestCoalitionIndexFrom);
			SolutionToImprove.add_agent_to_coalition(nBestAgentIndex, nBestCoalitionIndexTo);
			return true;
		}
		return false;
	}

	// Clears a coalition in O(mm); for each alternative, clear coalition, 
	// and re-distribute agents greedily.
	// Commit to best choice.
	bool LocalSearch_ClearCoalition
	(
		instance_solution& SolutionToImprove,
		coalitional_values_generator* pProblem,
		std::vector<unsigned int>& AgentOrder
	)
	{
		return false;
	}

	// Merge in O(mm); for each alternative, merge coalitions that maximizes delta value.
	bool LocalSearch_MergeCoalitions
	(
		instance_solution& SolutionToImprove,
		coalitional_values_generator* pProblem,
		std::vector<unsigned int>& AgentOrder
	)
	{
		return false;
	}

	// Double hill climb in O(nnm); for each agent-agent pair, put them in alternative that maximizes the value.
	bool LocalSearch_DoubleAgentGreed
	(
		instance_solution& SolutionToImprove,
		coalitional_values_generator* pProblem,
		std::vector<unsigned int>& AgentOrder
	)
	{
		return false;
	}

	// Swap in O(mm); for each alternative, try to swap the coalitions that maximizes delta value.
	bool LocalSearch_SwapCoalitions
	(
		instance_solution& SolutionToImprove,
		coalitional_values_generator* pProblem,
		std::vector<unsigned int>& AgentOrder
	)
	{
		return false;
	}
}

void solver_agent_greed::LocalSearch2Opt
(
	instance_solution& SolutionToImprove,
	coalitional_values_generator* pProblem,
	std::vector<unsigned int>& AgentOrder,
	std::default_random_engine& Generator,
	utility::date_and_time::timer& timer,
	const int _nMaximumNumberOfIterations
)
{
	int nIterations = 0;
	bool bFoundImprovement;
	do
	{
		// WARNING: THIS IS WORK IN PROGRESS!

		bFoundImprovement = false;

		// *******************************
		// Deterministic methods.
		// *******************************
		bFoundImprovement |=
			LocalSearch_AgentGreed(SolutionToImprove, pProblem, AgentOrder);

		bFoundImprovement |=
			LocalSearch_ClearCoalition(SolutionToImprove, pProblem, AgentOrder);

		bFoundImprovement |=
			LocalSearch_MergeCoalitions(SolutionToImprove, pProblem, AgentOrder);

		bFoundImprovement |=
			LocalSearch_DoubleAgentGreed(SolutionToImprove, pProblem, AgentOrder);

		bFoundImprovement |=
			LocalSearch_SwapCoalitions(SolutionToImprove, pProblem, AgentOrder);

		// *******************************
		// Stochastic methods.
		// *******************************
	} while (bFoundImprovement && nIterations < _nMaximumNumberOfIterations);
}

void solver_agent_greed::HillClimb
(
	instance_solution& SolutionToImprove, // The solution which we want to improve.
	coalitional_values_generator* pProblem, // The problem that we want to solve.
	std::vector<unsigned int>& AgentOrder,
	std::default_random_engine& Generator,
	utility::date_and_time::timer& timer,
	const bool _ShuffleAgentsEachHillClimbIteration,
	const int _HillClimbSwapLimit // -1 means no limit; keep on swapping for as long as we can improve the solution.
)
{
	auto AgentAssignmentIndices = SolutionToImprove.get_coalition_indices_of_agents(pProblem->get_n_agents());

	int nIterations = 0;
	bool bFoundGoodSwap;
	do
	{
		if (_ShuffleAgentsEachHillClimbIteration)
		{
			std::shuffle(AgentOrder.begin(), AgentOrder.end(), Generator);
		}

		bFoundGoodSwap = false;

		for (const uint32_t nAgentIndex : AgentOrder)
		{
			int nBestCoalitionIndex = AgentAssignmentIndices[nAgentIndex];
			coalition::value_t vBestAssignmentImprovementValue = 0; // Not moving an agent doesn't change its value at all.

			// Remove agent from initial coalition.
			coalition::value_t vValueOfOldCoalitionWithAgent = SolutionToImprove.calc_value_of_coalition(pProblem, nBestCoalitionIndex);
			SolutionToImprove.remove_agent_from_coalition(nAgentIndex, nBestCoalitionIndex);
			coalition::value_t vValueOfOldCoalitionWithoutAgent = SolutionToImprove.calc_value_of_coalition(pProblem, nBestCoalitionIndex);
			coalition::value_t vDiffOld = vValueOfOldCoalitionWithoutAgent - vValueOfOldCoalitionWithAgent;

			for (uint32_t j = 0; j < pProblem->get_n_tasks(); ++j)
			{
				if (AgentAssignmentIndices[nAgentIndex] == j) continue; // Don't try to assign yourself to the coalition you are already in.

				coalition::value_t vValueOfNewCoalitionWithoutAgent = SolutionToImprove.calc_value_of_coalition(pProblem, j);
				SolutionToImprove.add_agent_to_coalition(nAgentIndex, j);
				coalition::value_t vValueOfNewCoalitionWithAgent = SolutionToImprove.calc_value_of_coalition(pProblem, j);
				SolutionToImprove.remove_agent_from_coalition(nAgentIndex, j);

				coalition::value_t vDiffNew = vValueOfNewCoalitionWithAgent - vValueOfNewCoalitionWithoutAgent;
				coalition::value_t vDiffTot = vDiffNew + vDiffOld;

				const bool betterSwap = vDiffTot > vBestAssignmentImprovementValue;
				if (betterSwap)
				{
					vBestAssignmentImprovementValue = vDiffTot;
					nBestCoalitionIndex = j;
				}
			}

			SolutionToImprove.add_agent_to_coalition(nAgentIndex, nBestCoalitionIndex);

			if (AgentAssignmentIndices[nAgentIndex] != nBestCoalitionIndex)
			{
				const uint32_t nPreviousCoalitionIndex = AgentAssignmentIndices[nAgentIndex];
				AgentAssignmentIndices[nAgentIndex] = nBestCoalitionIndex;
				bFoundGoodSwap = true;
				SolutionToImprove.value += vBestAssignmentImprovementValue; // Should always be positive here.
			}
		}
	} while (bFoundGoodSwap && nIterations++ != _HillClimbSwapLimit);
}

void solver_agent_greed::greedily_assign_agents
(
	coalitional_values_generator* problem,
	instance_solution& solution,
	utility::date_and_time::timer& timer,
	std::vector<uint32_t> agents_to_assign
)
{
	// --------------------------------------------------------------------------------------------------
	// Torch code used for guiding the neural network.
	// --------------------------------------------------------------------------------------------------
	const auto add_agent_to_solution = [&](uint32_t _nAgentIndex, uint32_t _nTaskIndex)
	{
		solution.add_agent_to_coalition(_nAgentIndex, _nTaskIndex);
	};

	const auto remove_agent_from_solution = [&](uint32_t _nAgentIndex, uint32_t _nTaskIndex)
	{
		solution.remove_agent_from_coalition(_nAgentIndex, _nTaskIndex);
	};

	if (agents_to_assign.empty())
	{
		agents_to_assign.resize(problem->get_n_agents());
		std::iota(agents_to_assign.begin(), agents_to_assign.end(), 0);
	}

	// --------------------------------------------------------------------------------------------------
	// Greedily assign agents. 
	// --------------------------------------------------------------------------------------------------
	for (const uint32_t nAgentIndex : agents_to_assign)
	{
		uint32_t nBestTaskIndex = 0;
		float vBestTaskValue = std::numeric_limits<float>::lowest();
		for (uint32_t nTaskIndex = 0; nTaskIndex < problem->get_n_tasks(); ++nTaskIndex)
		{
			if (_coalition_size_bounds != nullptr)
			{
				const uint32_t nAgentsInCoalition = solution.n_agents_in_coalition(nTaskIndex);
				const uint32_t nMaxAgentsInCoalition = (*_coalition_size_bounds)[nTaskIndex];
				if (nAgentsInCoalition >= nMaxAgentsInCoalition)
				{
					continue;
				}
			}

			// Greedily assign agent.
			coalition::value_t vValueBeforeAssignment = problem->get_value_of(solution.get_coalition(nTaskIndex), nTaskIndex);
			add_agent_to_solution(nAgentIndex, nTaskIndex);
			coalition::value_t vValueAfterAssignment = problem->get_value_of(solution.get_coalition(nTaskIndex), nTaskIndex);
			remove_agent_from_solution(nAgentIndex, nTaskIndex);

			if (vValueAfterAssignment - vValueBeforeAssignment > vBestTaskValue)
			{
				nBestTaskIndex = nTaskIndex;
				vBestTaskValue = vValueAfterAssignment - vValueBeforeAssignment;
			}
		}
		add_agent_to_solution(nAgentIndex, nBestTaskIndex);
	}

	// Evaluate solution.
	solution.recalculate_value(problem);

	if (_RunHillClimbToPolish)
	{
		HillClimb(solution, problem, agents_to_assign, _generator, timer, false, 1000);
	}
}

instance_solution solver_agent_greed::solve(coalitional_values_generator* problem, std::vector<uint32_t> const* coalition_size_bounds)
{
	_coalition_size_bounds = coalition_size_bounds;
	_generator = std::default_random_engine(_nSeed);

	instance_solution best_solution;
	best_solution.reset(problem->get_n_tasks(), problem->get_n_agents());

	utility::date_and_time::timer timer{};

	if (vTimeLimit < 0 || !_RunMultipleTimesWithShuffledAgents) // Single mode (without time limit).
	{
		timer.start_countdown(-1);
		greedily_assign_agents(problem, best_solution, timer);
	}
	else
	{
		// int nTimesRun = 0;
		bool bBestSolutionHasBeenSet = false;
		instance_solution temp_solution;

		std::vector<unsigned int> AgentOrder(problem->get_n_agents());
		std::iota(AgentOrder.begin(), AgentOrder.end(), 0);

		timer.start_countdown(vTimeLimit);

		std::uniform_int_distribution<int> agent_distribution(0, problem->get_n_agents());
		std::uniform_int_distribution<int> task_distribution(0, problem->get_n_tasks() - 1);

		int nTimesRun = 0;
		while (true)
		{
			++nTimesRun;
			temp_solution.reset(problem->get_n_tasks(), problem->get_n_agents());

			greedily_assign_agents(problem, temp_solution, timer, AgentOrder);

			if (!bBestSolutionHasBeenSet || best_solution.value < temp_solution.value)
			{
				best_solution = temp_solution;
				bBestSolutionHasBeenSet = true;
			}

			// Check if time ran out.
			if (timer.countdown_reached())
			{
				break;
			}
			std::shuffle(AgentOrder.begin(), AgentOrder.end(), _generator);
		}

		//std::cout << "AG times run: " << nTimesRun << std::endl;
	}

	return best_solution;
}

instance_solution solver_agent_greed::solve(coalitional_values_generator* problem, instance_solution partial_solution)
{
	std::vector<int> agent_indices{ partial_solution.get_coalition_indices_of_agents(problem->get_n_agents()) };
	std::vector<uint32_t> unassigned_agents{};
	for (uint32_t agent{}; agent < problem->get_n_agents(); ++agent)
	{
		if (agent_indices[agent] < 0)
		{
			unassigned_agents.push_back(agent);
		}
	}

	instance_solution best_solution{}, temp_solution{};
	bool found_solution{ false };
	utility::date_and_time::timer timer{};
	std::default_random_engine engine{};
	srand(time(NULL));
	engine.seed(rand());
	timer.start_countdown(vTimeLimit);
	do {
		temp_solution = partial_solution;
		greedily_assign_agents(problem, temp_solution, timer, unassigned_agents);
		if (!found_solution || temp_solution.value > best_solution.value)
		{
			found_solution = true;
			best_solution = temp_solution;
		}
		std::shuffle(unassigned_agents.begin(), unassigned_agents.end(), engine);
	} while (vTimeLimit >= 0 && _RunMultipleTimesWithShuffledAgents && !timer.countdown_reached());
	return best_solution;
}

instance_solution solver_agent_greed::solve(coalitional_values_generator* problem)
{
	return solve(problem, nullptr);
}