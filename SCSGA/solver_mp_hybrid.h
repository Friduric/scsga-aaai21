#pragma once
#include "solver.h"

#include <algorithm>
#include <functional>
#include <vector>
#include <chrono>
#include <limits>
#include <cassert>
#include <iostream> // Debugging.
#include <unordered_map>

#include "solver_brute_force.h"
#include "solver_agent_greed.h"
#include "solver_task_greed.h"

#include "utility.h"


// #define NDEBUG // If this is defined, all assert-code is prevented from running (is not needed when runnig in VS via Release).

namespace MPHybrid
{
	constexpr float NEG_INF = 0.01f * std::numeric_limits<float>::lowest();

	namespace Utility
	{

		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------
		// Utility. 
		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------

		// Generates a partitioning based on integer cardinality and sums.
		namespace
		{
			void GenerateAllIntegerPartitions
			(
				const int nSize,
				const int nMax,
				const int nSizeFilter,
				std::vector<std::vector<uint32_t>>& Results,
				std::vector<uint32_t>& CurrentPartition
			);
		}

		// Generates all possible integer partitions of a given number.
		std::vector<std::vector<uint32_t>> GenerateAllPossibleIntegerPartitions
		(
			const int nNumber,
			const int nSizeFilter
		);
	}

	// ------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------
	// Coalition formation generator.
	// ------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------

	class CHybridSearcher
	{
	private:
		std::vector<std::vector<uint32_t>> _Partitions; // Contains all partitions.
		std::vector<std::vector<uint32_t>> _CoalitionsOfSize; // _CoalitionsOfSize[k] contains all coalitions with k members. 

		std::vector<float> _PartitionsLowerBound; // Partition lower bound.
		std::vector<float> _PartitionsUpperBound; // Partition upper bound.

		const std::vector<std::vector<float>>& _UtilityValues; // Utility values for all coalitions assigned to tasks.

		std::vector<float> _CardinalValuesUpperBound; // Mask upper bound.
		std::vector<float> _CardinalValuesLowerBound; // Mask lower bound.
		std::vector<uint32_t> _CardinalValuesCount;	  // The number of values for a specific agent count (bitcount in mask).

		std::vector<std::vector<float>> _TaskCardinalValuesUpperBound;	// Mask to task upper bound.
		std::vector<std::vector<float>> _TaskCardinalValuesLowerBound;	// Mask to task lower bound.
		std::vector<std::vector<uint32_t>> _TaskCardinalValuesCount;	// The number of values for a specific agent count of a task (bitcount in mask).

		const size_t _nAgents;
		const size_t _nTasks;
		coalitional_values_generator* _pProblem;

		const float _vLowestBoundTolerance = 0.001f;	// Must be a number larger than 0.0ff.
		const float _vOptimalityTolerance = 0.001f; // A large tolerance makes it possible to cut branches earlier, but might miss optimal solution.

		float _vTimeLimit = -1.0f;

		std::chrono::high_resolution_clock::time_point _TimeStartPoint;

	public:
		// Searcher constructor.
		CHybridSearcher
		(
			coalitional_values_generator* problem,
			const size_t nAgents,
			const size_t nTasks,
			const std::vector<std::vector<float>>& UtilityValues
		);

		struct SPartitionSearchResult
		{
			std::vector<uint32_t> _CollaborationStructure;
			float _UtilityValue = 0;
		};
	private:

		inline bool HasTimeElapsed();

		inline bool IsBetterThanCurrentBest(const float vValue, const bool bCheckMaximumLowestBound = true) const;

		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------
		// Partition data calculator.
		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------

		void CalculatePartitionData();

		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------
		// Search space partitioner.
		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------

		void Partition();

		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------
		// Partition searcher (searches a single partition).
		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------

		coalition::ordered_coalition_structure_t _BestSolution; // Represents the best solution found so far. 
		bool _bPartitionSearchFoundBetterSolution = false;
		bool _bHasFoundSolution = false; // Is true if we have found a solution.
		float _vBestSolutionValue = std::numeric_limits<float>::lowest(); // The value of the best solution found so far.
		float _vMaximumLowestBound = std::numeric_limits<float>::lowest(); // The maximum lower bound for all solutions.

		std::vector<uint32_t> const * _CurrentPartition = nullptr; // Contains the original data for the current partition's permutation.

		void StartPartitionSearch
		(
			const std::vector<uint32_t>& CurrentPartition,
			const float vUpperBound
		);

		struct SMemoizationEntry
		{
			float _vUtilityValue = std::numeric_limits<float>::lowest();
			uint32_t _nBestCoalition = 0u;
			SMemoizationEntry(){}
			SMemoizationEntry(float vUtilityValue, uint32_t nBestCoalition)
			{
				_vUtilityValue = vUtilityValue;
				_nBestCoalition = nBestCoalition;
			}
		};

		std::unordered_map<uint64_t, SMemoizationEntry> _MemoizationMap;
		inline const uint64_t CalculateMemoizationHashKey(const uint32_t nUnassignedAgentsMask, const uint32_t nTaskIndex);

		float __SearchPartition
		(
			const float vUpperBoundRemaining,
			const float vCurrentValue,
			const uint32_t nUnassignedAgentsMask,
			const uint32_t nTaskIndex
		);

		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------
		// Permutation searcher (searches all permutations).
		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------

		struct SPartitionPermutation
		{
			std::vector<uint32_t> _Permutation;
			const float _vLowerBound = std::numeric_limits<float>::lowest(), _vUpperBound = std::numeric_limits<float>::max();
			SPartitionPermutation(const std::vector<uint32_t>& Permutation, const float vLowerBound, const float vUpperBound)
				: _Permutation(Permutation), _vLowerBound(vLowerBound), _vUpperBound(vUpperBound)
			{}
		};

		std::vector<SPartitionPermutation> Permutations;
		std::vector<uint32_t> PermutationOrder, SortedPermutationOrder;

		void SearchCurrentPermutations();

	public:

		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------
		// Finds the optimal collaboration structure.
		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------

		instance_solution FindOptimalCoalitionStructure
		(
			// An initial solution can be used to provide a better starting global lower bound.
			const instance_solution* pInitialSolution = nullptr,

			// If this is true, MP anytime generates a greedy solution for each subspace 
			// that is used in an attempt to improve that subspace's lower bound.
			const bool bTryUseAGIAsLowerBound = false,

			// If this is > 0.0f, a time limit is used to interrupt the algorithm's
			// search procedure.
			const float vTimeLimit = -1.0f,

			// Number of permutations that are stored per block.
			// Increasing this number => increases performance and memory usage.
			const uint32_t nPermutationsPerBlock = 100000u
		);
	};
}

class solver_mp_hybrid : public solver
{
public:

	instance_solution solve(coalitional_values_generator* problem, const instance_solution* initial_solution);
	instance_solution solve(coalitional_values_generator* problem) override;
};
