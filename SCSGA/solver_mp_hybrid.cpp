#include "solver_mp_hybrid.h"

namespace MPHybrid
{

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
			)
			{
				if (CurrentPartition.size() > nSizeFilter)
				{
					return;
				}
				if (nSize == 0)
				{
					Results.push_back(CurrentPartition);
					return;
				}
				for (int i = std::min<int>(nSize, nMax); i >= 1; --i)
				{
					CurrentPartition.push_back(i);
					assert(i >= 0);
					GenerateAllIntegerPartitions(nSize - i, i, nSizeFilter, Results, CurrentPartition);
					CurrentPartition.pop_back();
				}
			}
		}

		// Generates all possible integer partitions of a given number.
		std::vector<std::vector<uint32_t>> GenerateAllPossibleIntegerPartitions
		(
			const int nNumber,
			const int nSizeFilter
		)
		{
			std::vector<std::vector<uint32_t>> Subsets;

			// Generate all possible subsets of coalition structures.
			std::vector<uint32_t> EmptyPartition;
			EmptyPartition.reserve(nSizeFilter);
			GenerateAllIntegerPartitions(nNumber, nNumber, nSizeFilter, Subsets, EmptyPartition);
			assert(Subsets.size() >= 0);
			for (auto& Subset : Subsets)
			{
				while (Subset.size() < nSizeFilter)
				{
					Subset.push_back(0);
				}
				assert(Subset.size() == nSizeFilter);
			}
			return Subsets;
		}
	}

	// ------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------
	// Coalition formation generator.
	// ------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------

	CHybridSearcher::CHybridSearcher
	(
		coalitional_values_generator* problem,
		const size_t nAgents,
		const size_t nTasks,
		const std::vector<std::vector<float>>& UtilityValues
	)
		: _pProblem(problem), _nAgents(nAgents), _nTasks(nTasks), _UtilityValues(UtilityValues) { }

	inline bool CHybridSearcher::HasTimeElapsed()
	{
		if (!_bHasFoundSolution)
			return false;

		if (_vTimeLimit >= 0)
		{
			std::chrono::high_resolution_clock::time_point TimeNowPoint = std::chrono::high_resolution_clock::now();
			auto vElapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(TimeNowPoint - _TimeStartPoint).count();
			if (vElapsedTime >= _vTimeLimit)
			{
				return true;
			}
		}
		return false;
	}

	inline bool CHybridSearcher::IsBetterThanCurrentBest(const float vValue, const bool bCheckMaximumLowestBound) const
	{
		if (!_bHasFoundSolution)
			return true;

		if (vValue - _vOptimalityTolerance < _vBestSolutionValue)
			return false;

		if (bCheckMaximumLowestBound && vValue - _vLowestBoundTolerance < _vMaximumLowestBound)
			return false;

		return true;
	}

		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------
		// Partition data calculator.
		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------

	void CHybridSearcher::CalculatePartitionData()
	{
		const uint32_t nNumberOfPossibleCoalitions = (1 << _nAgents);

		// --------------------------------------------------------------------
		// Generate and calculate coalition values for all possible coalitions.	
		// --------------------------------------------------------------------

		_CardinalValuesCount.assign(_nAgents + 1, 0u);
		_CardinalValuesLowerBound.assign(_nAgents + 1, 0.0f);
		_CardinalValuesUpperBound.assign(_nAgents + 1, std::numeric_limits<float>::lowest());

		_TaskCardinalValuesLowerBound.assign(_nTasks, std::vector<float>(_nAgents + 1, 0.0f));
		_TaskCardinalValuesUpperBound.assign(_nTasks, std::vector<float>(_nAgents + 1, std::numeric_limits<float>::lowest()));
		_TaskCardinalValuesCount.assign(_nTasks, std::vector<uint32_t>(_nAgents + 1, 0u));

		_CoalitionsOfSize.assign(_nAgents + 1, std::vector<uint32_t>());

		for (uint32_t nCoalitionMask = 0u; nCoalitionMask < nNumberOfPossibleCoalitions; ++nCoalitionMask)
		{
			// Calculate starting bounds.
			const uint32_t nCoalitionMembers = utility::bits::bit_count_32bit(nCoalitionMask);

			_CoalitionsOfSize[nCoalitionMembers].push_back(nCoalitionMask);

			float& CurrentCardinalValuesLowerBound = _CardinalValuesLowerBound[nCoalitionMembers];
			float& CurrentCardinalValuesUpperBound = _CardinalValuesUpperBound[nCoalitionMembers];
			uint32_t& CurrentCardinalValueCount = _CardinalValuesCount[nCoalitionMembers];

			for (uint32_t nTaskIndex = 0; nTaskIndex < _nTasks; ++nTaskIndex)
			{
				const auto Value = _UtilityValues[nTaskIndex][nCoalitionMask];

				float& CurrentTaskCardinalValuesUpperBound = _TaskCardinalValuesUpperBound[nTaskIndex][nCoalitionMembers];
				float& CurrentTaskCardinalValuesLowerBound = _TaskCardinalValuesLowerBound[nTaskIndex][nCoalitionMembers];
				uint32_t& CurrentTaskCardinalValuesCount = _TaskCardinalValuesCount[nTaskIndex][nCoalitionMembers];

				// Update upper bound (max) for cardinal value.
				if (Value > CurrentCardinalValuesUpperBound)
					CurrentCardinalValuesUpperBound = Value;

				// Update upper bound (max) for cardinal-task value.
				if (Value > CurrentTaskCardinalValuesUpperBound)
					CurrentTaskCardinalValuesUpperBound = Value;

				// Update lower bound (avg) for cardinal value.
				CurrentCardinalValuesLowerBound = CurrentCardinalValuesLowerBound * CurrentCardinalValueCount + Value;
				CurrentCardinalValuesLowerBound /= static_cast<float>(CurrentCardinalValueCount + 1.0f);

				// Update lower bound (avg) for cardinal-task value;
				CurrentTaskCardinalValuesLowerBound = CurrentTaskCardinalValuesLowerBound * CurrentTaskCardinalValuesCount + Value;
				CurrentTaskCardinalValuesLowerBound /= static_cast<float>(CurrentTaskCardinalValuesCount + 1.0f);

				// Update counts.
				++CurrentCardinalValueCount;
				++CurrentTaskCardinalValuesCount;
			}
		}

		// -------------------------------------------------------
		// Calculate partition lower and upper bounds.
		// -------------------------------------------------------

		const size_t nPartitions = _Partitions.size();
		_PartitionsLowerBound.assign(nPartitions, 0.0f);
		_PartitionsUpperBound.assign(nPartitions, 0.0f);
		for (uint32_t i = 0; i < nPartitions; ++i)
		{
			for (const uint32_t nCardinality : _Partitions[i])
			{
				_PartitionsLowerBound[i] += _CardinalValuesLowerBound[nCardinality];
				_PartitionsUpperBound[i] += _CardinalValuesUpperBound[nCardinality];
			}
			if (_PartitionsLowerBound[i] > _vMaximumLowestBound)
			{
				_vMaximumLowestBound = _PartitionsLowerBound[i];
			}
		}
	}

	// ------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------
	// Search space partitioner.
	// ------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------

	void CHybridSearcher::Partition()
	{
		_Partitions = Utility::GenerateAllPossibleIntegerPartitions(static_cast<int>(_nAgents), static_cast<int>(_nTasks));
	}

		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------
		// Partition searcher (searches a single partition).
		// ------------------------------------------------------------------------------
		// ------------------------------------------------------------------------------

	void CHybridSearcher::StartPartitionSearch
	(
		const std::vector<uint32_t>& CurrentPartition,
		const float vUpperBound
	)
	{
		_CurrentPartition = &CurrentPartition;

		_MemoizationMap.clear();
		const uint32_t nAllAgentBits = (~0u) >> (32u - _nAgents);

		_bPartitionSearchFoundBetterSolution = false;

		float vWorth = __SearchPartition(vUpperBound, 0.f, nAllAgentBits, uint32_t(_nTasks - 1));

		// Reconstruct solution found by __SearchPartition.
		if (_bPartitionSearchFoundBetterSolution && IsBetterThanCurrentBest(vWorth, false))
		{
			_vBestSolutionValue = vWorth;
			_vMaximumLowestBound = std::max(_vBestSolutionValue, _vMaximumLowestBound);
			uint32_t nUnassignedAgentsMask = nAllAgentBits;
			for (int nTaskIndex = int(_nTasks) - 1; nTaskIndex >= 1; --nTaskIndex)
			{
				uint64_t nMemoizationKey = CalculateMemoizationHashKey(nUnassignedAgentsMask, nTaskIndex);
				auto pMemoizationPointer = _MemoizationMap.find(nMemoizationKey);
				if (pMemoizationPointer == _MemoizationMap.end())
				{
					_BestSolution[nTaskIndex].set_value(0u); // We get here for tasks that have 0 agents assigned to them.
				}
				else
				{
					_BestSolution[nTaskIndex].set_value(pMemoizationPointer->second._nBestCoalition);
					nUnassignedAgentsMask &= (~_BestSolution[nTaskIndex].get_agent_mask());
				}
			}
			_BestSolution[0].set_value(nUnassignedAgentsMask);
			_bHasFoundSolution = true;
		}
	}

	inline const uint64_t CHybridSearcher::CalculateMemoizationHashKey(const uint32_t nUnassignedAgentsMask, const uint32_t nTaskIndex)
	{
		return uint64_t(nUnassignedAgentsMask) | (uint64_t(nTaskIndex) << 32ULL);
	}

	float CHybridSearcher::__SearchPartition
	(
		const float vUpperBoundRemaining,
		const float vCurrentValue,
		const uint32_t nUnassignedAgentsMask,
		const uint32_t nTaskIndex
	)
	{
		if (nTaskIndex == 0)
		{
			// Assign all agents to the only remaining task.
			_bPartitionSearchFoundBetterSolution = true;
			if (_UtilityValues[nTaskIndex][nUnassignedAgentsMask] + vCurrentValue > _vMaximumLowestBound)
			{
				_vMaximumLowestBound = _UtilityValues[nTaskIndex][nUnassignedAgentsMask] + vCurrentValue;
			}
			return _UtilityValues[nTaskIndex][nUnassignedAgentsMask];
		}

		if (!IsBetterThanCurrentBest(vCurrentValue + vUpperBoundRemaining))
		{
			// This does NOT generate an optimal solution, since vCurrentValue can be different when evaluating the 
			// search tree different paths.
			return NEG_INF;
		}

		const uint64_t nMemoizationKey = CalculateMemoizationHashKey(nUnassignedAgentsMask, nTaskIndex);

		auto pMemoizationPointer = _MemoizationMap.find(nMemoizationKey);
		if (pMemoizationPointer != _MemoizationMap.end())
		{
			// Already evaluated this branch.
			return pMemoizationPointer->second._vUtilityValue;
		}

		const uint32_t nCoalitionSize = (*_CurrentPartition)[nTaskIndex];

		uint32_t nBestMask = 0u;
		float vBestValue = NEG_INF;

		for (const uint32_t nCoalitionMask : _CoalitionsOfSize[nCoalitionSize])
		{
			if (HasTimeElapsed())
			{
				return NEG_INF;
			}

			if ((nUnassignedAgentsMask & nCoalitionMask) == nCoalitionMask)
			{
				// These two values are only used for branch-and-bound techniques.
				const float vValue = vCurrentValue + _UtilityValues[nTaskIndex][nCoalitionMask];
				const float vNewUpperBoundRemaining = vUpperBoundRemaining - _TaskCardinalValuesUpperBound[nTaskIndex][nCoalitionSize];

				float vWorth = __SearchPartition(vNewUpperBoundRemaining, vValue, nUnassignedAgentsMask & (~nCoalitionMask), nTaskIndex - 1);
				if (vWorth > NEG_INF) // Prevent storing DP-solution when there is no end-path.
				{
					vWorth += _UtilityValues[nTaskIndex][nCoalitionMask];
					if (vWorth > vBestValue)
					{
						vBestValue = vWorth;
						nBestMask = nCoalitionMask;
					}
				}
			}
		}

		if (vBestValue > NEG_INF) // Only store DP solution if we found an end-path.
		{
			_MemoizationMap.emplace(nMemoizationKey, SMemoizationEntry(vBestValue, nBestMask));
			if (vCurrentValue + vBestValue > _vMaximumLowestBound)
			{
				_vMaximumLowestBound = vCurrentValue + vBestValue;
			}
		}

		return vBestValue;
	}

	// ------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------
	// Permutation searcher (searches all permutations).
	// ------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------

	void CHybridSearcher::SearchCurrentPermutations()
	{
		std::sort(PermutationOrder.begin(), PermutationOrder.end(), [&](const uint32_t lhs, const uint32_t rhs)
			{
				if (std::abs(Permutations[lhs]._vUpperBound - Permutations[rhs]._vUpperBound) < _vOptimalityTolerance)
				{
					return Permutations[lhs]._vLowerBound > Permutations[rhs]._vLowerBound;
				}
				return Permutations[lhs]._vUpperBound > Permutations[rhs]._vUpperBound;
			});

		for (const uint32_t PermutationIndex : PermutationOrder)
		{
			if (HasTimeElapsed())
			{
				return;
			}

			if (IsBetterThanCurrentBest(Permutations[PermutationIndex]._vUpperBound))
			{
				StartPartitionSearch(Permutations[PermutationIndex]._Permutation, Permutations[PermutationIndex]._vUpperBound);
			}
			else
			{
				break; // We can discard the rest (since the search queue is sorted on upper bound first).
			}
		}
	}

	// ------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------
	// Finds the optimal collaboration structure.
	// ------------------------------------------------------------------------------
	// ------------------------------------------------------------------------------

	instance_solution CHybridSearcher::FindOptimalCoalitionStructure
	(
		const instance_solution* pInitialSolution,
		const bool bTryUseAGIAsLowerBound,
		const float vTimeLimit,
		const uint32_t nPermutationsPerBlock
	)
	{
		instance_solution Result; // Result.

		// ----------------------------------------------------------------------------
		// Construct optimal solution quickly if there's only one task (or 1 agent).
		// ----------------------------------------------------------------------------

		{
			if (_nAgents <= 1 || _nTasks <= 1)
			{
				return solver_brute_force().solve(_pProblem);
			}
		}

		_TimeStartPoint = std::chrono::high_resolution_clock::now();
		_vTimeLimit = vTimeLimit;

		// ----------------------------------------------------------------------------
		// Find an initial solution to use as baseline.
		// ----------------------------------------------------------------------------

		_vBestSolutionValue = std::numeric_limits<float>::lowest();
		_bHasFoundSolution = false;

		if (pInitialSolution == nullptr)
		{
			_vBestSolutionValue = std::numeric_limits<float>::lowest();
			_bHasFoundSolution = false;
			_BestSolution.resize(_nTasks, coalition::coalition_t(_nAgents));
		}
		else
		{
			_bHasFoundSolution = true;
			_vBestSolutionValue = pInitialSolution->value;
			_BestSolution = pInitialSolution->ordered_coalition_structure;
		}

		// ----------------------------------------------------------------------------
		// Partition search space and calculate bounds.
		// ----------------------------------------------------------------------------

		Partition();
		CalculatePartitionData();

		// ----------------------------------------------------------------------------
		// Try to improve partition's lower bound using a greedy solution.
		// ----------------------------------------------------------------------------

		solver_agent_greed GreedySolver;
		GreedySolver._RunHillClimbToPolish = true;
		if (bTryUseAGIAsLowerBound)
		{
			auto GreedyGlobalSolution = GreedySolver.solve(_pProblem);
			if (IsBetterThanCurrentBest(GreedyGlobalSolution.value, false))
			{
				// Update initially found solution so that it corresponds to the best greedy one.
				_bHasFoundSolution = true;
				_BestSolution = GreedyGlobalSolution.ordered_coalition_structure;
				_vBestSolutionValue = GreedyGlobalSolution.value;
				_vMaximumLowestBound = std::max(_vBestSolutionValue, _vMaximumLowestBound);;
			}

			for (uint32_t nPartitionIndex = 0; nPartitionIndex < _Partitions.size(); ++nPartitionIndex)
			{
				if (!IsBetterThanCurrentBest(_PartitionsUpperBound[nPartitionIndex]))
				{
					continue;
				}
				auto GreedySolution = GreedySolver.solve(_pProblem, &_Partitions[nPartitionIndex]);
				if (IsBetterThanCurrentBest(GreedySolution.value, false))
				{
					// Update initially found solution so that it corresponds to the best greedy one.
					_bHasFoundSolution = true;
					_BestSolution = GreedySolution.ordered_coalition_structure;
					_vBestSolutionValue = GreedySolution.value;
					_vMaximumLowestBound = std::max(_vBestSolutionValue, _vMaximumLowestBound);;
				}
			}
		}

		// ----------------------------------------------------------------------------
		// Decide on the order for which we expand partitions.
		// ----------------------------------------------------------------------------

		std::vector<uint32_t> ExpansionOrder;
		ExpansionOrder.reserve(_Partitions.size());
		for (uint32_t nPartitionIndex = 0; nPartitionIndex < _Partitions.size(); ++nPartitionIndex)
		{
			if (IsBetterThanCurrentBest(_PartitionsUpperBound[nPartitionIndex]))
			{
				ExpansionOrder.push_back(nPartitionIndex);
			}
		}

		// ----------------------------------------------------------------------------
		// Decide on ExpansionOrderFunction.
		// ----------------------------------------------------------------------------

		const auto ExpansionOrderFunction = [&](const uint32_t lhs, const uint32_t rhs)
		{
			if (std::abs(_PartitionsUpperBound[lhs] - _PartitionsUpperBound[rhs]) < _vOptimalityTolerance)
				return _PartitionsLowerBound[lhs] > _PartitionsLowerBound[rhs];
			return _PartitionsUpperBound[lhs] > _PartitionsUpperBound[rhs];
		};

		std::sort(ExpansionOrder.begin(), ExpansionOrder.end(), ExpansionOrderFunction);

		// ----------------------------------------------------------------------------
		// Initialize search.
		// ----------------------------------------------------------------------------

		PermutationOrder.clear();
		Permutations.clear();

		PermutationOrder.reserve(nPermutationsPerBlock);
		Permutations.reserve(nPermutationsPerBlock);

		// ----------------------------------------------------------------------------
		// Start search (branch-and-bound).
		// ----------------------------------------------------------------------------
		for (uint32_t i = 0; i < ExpansionOrder.size() && !HasTimeElapsed(); ++i)
		{
			const int nPartitionIndex = ExpansionOrder[i];
			auto& CurrentPartition = _Partitions[nPartitionIndex];
			const auto vSubspaceUpperBound = _PartitionsUpperBound[nPartitionIndex];

			// Can this subspace improve best solution value?
			if (!IsBetterThanCurrentBest(vSubspaceUpperBound))
			{
				break;
			}

			// Non-increasing order for next_permutation.
			std::sort(CurrentPartition.begin(), CurrentPartition.end());

			do
			{
				// Calculate tighter (better) bounds.
				float vPermutationUpperBound = 0.0f, vPermutationLowerBound = 0.0f;
				for (uint32_t nTaskIndex = 0; nTaskIndex < CurrentPartition.size(); ++nTaskIndex)
				{
					const uint32_t P = CurrentPartition[nTaskIndex];
					vPermutationLowerBound += _TaskCardinalValuesLowerBound[nTaskIndex][P];
					vPermutationUpperBound += _TaskCardinalValuesUpperBound[nTaskIndex][P];
				}

				// Update maximum lowest bound if possible.
				if (vPermutationLowerBound > _vMaximumLowestBound)
				{
					_vMaximumLowestBound = vPermutationLowerBound;
				}

				if (IsBetterThanCurrentBest(vPermutationUpperBound))
				{
					if (bTryUseAGIAsLowerBound)
					{
						auto GreedySolution = GreedySolver.solve(_pProblem, &CurrentPartition);
						if (IsBetterThanCurrentBest(GreedySolution.value, false))
						{
							_BestSolution = GreedySolution.ordered_coalition_structure;
							_vBestSolutionValue = GreedySolution.value;
							_vMaximumLowestBound = std::max(_vMaximumLowestBound, _vBestSolutionValue);
						}
					}

					// Add permutation to search queue.
					PermutationOrder.push_back(uint32_t(PermutationOrder.size()));
					Permutations.emplace_back(CurrentPartition, vPermutationLowerBound, vPermutationUpperBound);

					if (Permutations.size() >= nPermutationsPerBlock)
					{
						SearchCurrentPermutations();
						Permutations.clear();
						PermutationOrder.clear();
					}
				}
			} while (std::next_permutation(CurrentPartition.begin(), CurrentPartition.end()) && !HasTimeElapsed());
		}

		if (Permutations.size() > 0 && !HasTimeElapsed())
		{
			SearchCurrentPermutations();
		}

		// ----------------------------------------------------------------------------
		// Construct best solution.
		// ----------------------------------------------------------------------------

		assert(_bHasFoundSolution);
		assert(_BestSolution.size() == _nTasks);

		Result.ordered_coalition_structure = _BestSolution;
		Result.value = _vBestSolutionValue;

		return Result;
	}
}

instance_solution solver_mp_hybrid::solve(coalitional_values_generator* problem, const instance_solution* initial_solution)
{
	MPHybrid::CHybridSearcher generator(problem, problem->get_n_agents(), problem->get_n_tasks(), problem->get_data());
	return generator.FindOptimalCoalitionStructure(initial_solution, true, vTimeLimit);
}

instance_solution solver_mp_hybrid::solve(coalitional_values_generator* problem)
{
	assert(problem->get_n_agents() <= 32);
	return solve(problem, nullptr);
}