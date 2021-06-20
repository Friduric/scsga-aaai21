#pragma once
#include "solver.h"
#include "utility.h"

#define _USE_PDEP true
#define _USE_RECV false

class solver_dp :
	public solver
{
public:
	struct MemoizationEntry
	{
		float _vUtilityValue = std::numeric_limits<float>::lowest();
		uint32_t _nBestCoalition = 0u;
	};

	uint32_t _nAgents = 0, _nTasks = 0;
	coalitional_values_generator* _UtilityValues = nullptr; // Utiliy values (performances measures) of coalition-to-task/goal assignments.
	std::vector<std::vector<MemoizationEntry>> _MemoizationTable; // DP table used to store intermediate solutions.

	inline float GetUtilityValueFor(const uint32_t nTaskIndex, const uint32_t nCoalitionMask);

	void SolveIterative();
	float SolveRecursive(const uint32_t nTaskIndex, const uint32_t nUnassignedAgentsMask);

	instance_solution solve(coalitional_values_generator* problem);
};