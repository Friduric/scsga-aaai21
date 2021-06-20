#include "solver_dp.h"

inline float solver_dp::GetUtilityValueFor(const uint32_t nTaskIndex, const uint32_t nCoalitionMask)
{
	return (*_UtilityValues).get_value_of(nCoalitionMask, nTaskIndex);
}

void solver_dp::SolveIterative()
{
	const uint32_t nNumberOfPossibleCoalitions = (1 << _nAgents);
	for (uint32_t nUnassignedAgentsMask = 0u; nUnassignedAgentsMask < nNumberOfPossibleCoalitions; ++nUnassignedAgentsMask)
	{
		// For task 0, the only option is to assign all unassigned agents.
		_MemoizationTable[0][nUnassignedAgentsMask]._vUtilityValue = GetUtilityValueFor(0, nUnassignedAgentsMask);
		_MemoizationTable[0][nUnassignedAgentsMask]._nBestCoalition = nUnassignedAgentsMask;
	}

	for (uint32_t nTaskIndex = 1u; nTaskIndex < _nTasks; ++nTaskIndex)
	{
		for (uint32_t nUnassignedAgentsMask = 0u; nUnassignedAgentsMask < nNumberOfPossibleCoalitions; ++nUnassignedAgentsMask)
		{
			// Try all other possible ways of assigning agents to this task, and improve the current best solution utility.
			const uint32_t nNumberOfUnassignedAgents = utility::bits::bit_count_32bit(nUnassignedAgentsMask);
			const uint32_t nNumberOfPossibleCoalitionsFromUnassignedAgents = (1u << nNumberOfUnassignedAgents);

			for (uint32_t nSkewedCoalitionMask = 0u; nSkewedCoalitionMask < nNumberOfPossibleCoalitionsFromUnassignedAgents; ++nSkewedCoalitionMask)
			{
				// Calculate a mask that represents the current coalition we want to try.
				const uint32_t nRealCoalitionMask = utility::bits::calc_parallel_bits_deposit_32bit(nSkewedCoalitionMask, nUnassignedAgentsMask);
				const uint32_t nNewUnassignedAgentsMask = nUnassignedAgentsMask & (~nRealCoalitionMask);

				// Calculate the utility value for assigning that mask to this task.
				const float vNewSolutionUtility =
					GetUtilityValueFor(nTaskIndex, nRealCoalitionMask) +
					_MemoizationTable[nTaskIndex - 1][nNewUnassignedAgentsMask]._vUtilityValue;

				if (vNewSolutionUtility > _MemoizationTable[nTaskIndex][nUnassignedAgentsMask]._vUtilityValue)
				{
					// Found a better assignment than before.
					_MemoizationTable[nTaskIndex][nUnassignedAgentsMask]._vUtilityValue = vNewSolutionUtility;
					_MemoizationTable[nTaskIndex][nUnassignedAgentsMask]._nBestCoalition = nRealCoalitionMask;
				}
			}
		}
	}
}

float solver_dp::SolveRecursive(const uint32_t nTaskIndex, const uint32_t nUnassignedAgentsMask)
{
	MemoizationEntry& _CurrentMemoizationEntry = _MemoizationTable[nTaskIndex][nUnassignedAgentsMask];
	if (_CurrentMemoizationEntry._vUtilityValue > std::numeric_limits<float>::lowest())
	{
		// We have already evaluated this function state.
		return _CurrentMemoizationEntry._vUtilityValue;
	}

	if (nTaskIndex == 0)
	{
		// Assign all agents to the only remaining task.
		_CurrentMemoizationEntry._vUtilityValue = GetUtilityValueFor(nTaskIndex, nUnassignedAgentsMask);
		_CurrentMemoizationEntry._nBestCoalition = nUnassignedAgentsMask;
		return _CurrentMemoizationEntry._vUtilityValue;
	}

	float& vBestSolutionUtility = _CurrentMemoizationEntry._vUtilityValue;
	uint32_t& nBestSolutionMembers = _CurrentMemoizationEntry._nBestCoalition;

	// Initialize the best possible utility we can achieve as assigning zero agents to this task.
	vBestSolutionUtility = GetUtilityValueFor(nTaskIndex, 0) + SolveRecursive(nTaskIndex - 1, nUnassignedAgentsMask);
	nBestSolutionMembers = 0;

#if _USE_PDEP
	// Try all other possible ways of assigning agents to this task, and improve the current best solution utility.
	const int nNumberOfUnassignedAgents = utility::bits::bit_count_32bit(nUnassignedAgentsMask);
	const uint32_t nNumberOfPossibleCoalitions = (1 << nNumberOfUnassignedAgents);

	for (uint32_t nSkewedCoalitionMask = 1; nSkewedCoalitionMask < nNumberOfPossibleCoalitions; ++nSkewedCoalitionMask)
	{
		// Calculate a mask that represents the current coalition we want to try.
		const uint32_t nRealCoalitionMask = utility::bits::calc_parallel_bits_deposit_32bit(nSkewedCoalitionMask, nUnassignedAgentsMask);
		const uint32_t nNewUnassignedAgentsMask = nUnassignedAgentsMask & (~nRealCoalitionMask);

		// Calculate the utility value for assigning that mask to this task.
		const float vNewSolutionUtility
			= GetUtilityValueFor(nTaskIndex, nRealCoalitionMask) + SolveRecursive(nTaskIndex - 1, nNewUnassignedAgentsMask);

		if (vNewSolutionUtility > vBestSolutionUtility)
		{
			// Found a better assignment than before.
			vBestSolutionUtility = vNewSolutionUtility;
			nBestSolutionMembers = nRealCoalitionMask;
		}
	}
#else
	for (uint32_t nCoalitionMask = 1; nCoalitionMask < _UtilityValues->get_n_coalitions(); ++nCoalitionMask)
	{
		const bool bIsValidCoalition = (nCoalitionMask & nUnassignedAgentsMask) == nCoalitionMask;
		if (bIsValidCoalition)
		{
			const uint32_t nNewUnassignedAgentsMask = nUnassignedAgentsMask & (~nCoalitionMask);

			// Calculate the utility value for assigning that mask to this task.
			const float vNewSolutionUtility
				= GetUtilityValueFor(nTaskIndex, nCoalitionMask) + SolveRecursive(nTaskIndex - 1, nNewUnassignedAgentsMask);

			if (vNewSolutionUtility > vBestSolutionUtility)
			{
				// Found a better assignment than before.
				vBestSolutionUtility = vNewSolutionUtility;
				nBestSolutionMembers = nCoalitionMask;
			}
		}
	}

#endif // _USE_PDEP

	return vBestSolutionUtility;
}

instance_solution solver_dp::solve(coalitional_values_generator* problem)
{
	_UtilityValues = problem, _nAgents = problem->get_n_agents(), _nTasks = problem->get_n_tasks();

	// Initialize memoization table.
	const uint32_t nNumberOfPossibleCoalitions = 1 << _nAgents;
	_MemoizationTable.assign(_nTasks, std::vector<MemoizationEntry>(nNumberOfPossibleCoalitions));

	// Make a bit mask that represents all agents being unassigned (5 agents => 00011111, 6 agents => 00111111).
	uint32_t nAllAgentBits = 0;
	for (uint32_t i = 0; i < _nAgents; ++i)
	{
		nAllAgentBits |= (1 << i);
	}

	instance_solution Solution;
#if _USE_RECV
	SolveRecursive(_nTasks - 1, nAllAgentBits);
#else 
	SolveIterative();
#endif

	// Reconstruct optimal solution in O(m).
	uint32_t nUnassignedAgentsMask = nAllAgentBits;
	for (int nTaskIndex = _nTasks - 1; nTaskIndex >= 0; --nTaskIndex)
	{
		const uint32_t nBestCoalition
			= _MemoizationTable[nTaskIndex][nUnassignedAgentsMask]._nBestCoalition;
		coalition::coalition_t current_coalition(_nAgents);
		current_coalition.set_value(nBestCoalition);
		Solution.ordered_coalition_structure.push_back(current_coalition);
		nUnassignedAgentsMask &= (~nBestCoalition);
	}
	std::reverse(Solution.ordered_coalition_structure.begin(), Solution.ordered_coalition_structure.end());
	Solution.recalculate_value(problem);

	return Solution;
}