#include "solver_mp_anytime.h"

namespace MPAnytime
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

	CAnytimeSearcher::CAnytimeSearcher
	(
		coalitional_values_generator* problem,
		const size_t nAgents,
		const size_t nTasks,
		const std::vector<std::vector<float>>& UtilityValues
	)
		: _pProblem(problem), _nAgents(nAgents), _nTasks(nTasks), _UtilityValues(UtilityValues) { }

	inline bool CAnytimeSearcher::HasTimeElapsed()
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

	inline bool CAnytimeSearcher::IsBetterThanCurrentBest(const float vValue) const
	{
		if (!_bHasFoundSolution)
			return true;

		if (vValue - _vOptimalityTolerance < _vBestSolutionValue)
			return false;

		if (vValue - _vLowestBoundTolerance < _vMaximumLowestBound)
			return false;

		return true;
	}

	void CAnytimeSearcher::CalculatePartitionData()
	{
		const uint32_t nNumberOfPossibleCoalitions = (1 << _nAgents);

		// --------------------------------------------------------------------
		// Generate and compute coalition values for all possible coalitions.	
		// --------------------------------------------------------------------

		_CardinalValuesCount.assign(_nAgents + 1, 0u);
		_CardinalValuesLowerBound.assign(_nAgents + 1, 0.0f);
		_CardinalValuesUpperBound.assign(_nAgents + 1, std::numeric_limits<float>::lowest());

		_TaskCardinalValuesLowerBound.assign(_nTasks, std::vector<float>(_nAgents + 1, 0.0f));
		_TaskCardinalValuesUpperBound.assign(_nTasks, std::vector<float>(_nAgents + 1, std::numeric_limits<float>::lowest()));
		_TaskCardinalValuesCount.assign(_nTasks, std::vector<uint32_t>(_nAgents + 1, 0u));

		for (uint32_t nCoalitionMask = 0; nCoalitionMask < nNumberOfPossibleCoalitions; ++nCoalitionMask)
		{
			// Compute starting bounds.
			const uint32_t nCoalitionMembers = utility::bits::bit_count_32bit(nCoalitionMask);

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
		// Compute partition lower and upper bounds.
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

	void CAnytimeSearcher::Partition()
	{
		_Partitions = Utility::GenerateAllPossibleIntegerPartitions(static_cast<int>(_nAgents), static_cast<int>(_nTasks));
	}

	void CAnytimeSearcher::InitializePartitionSearch()
	{
		_CurrentSolution.reset(_nTasks, _nAgents);
		_PSPartitionToTaskIndex.resize(_nTasks);
		_PSCurrentPartition.reserve(_nTasks);
	}

	void CAnytimeSearcher::StartPartitionSearch
	(
		const std::vector<uint32_t>& CurrentPartition
	)
	{
		_PSOriginalPartition = CurrentPartition;
		_PSCurrentPartition.clear();
		float vStartValue = 0, vUpperBound = 0;

		// Evaluate all tasks that have 0 members, and only keep non-zero coalitions.
		// Also calculate upper bound for this permutation of the partition.
		uint32_t nSkewedIndex = 0;
		for (uint32_t nIndex = 0; nIndex < CurrentPartition.size(); ++nIndex)
		{
			const uint32_t nCoalitionSize = CurrentPartition[nIndex];
			if (nCoalitionSize != 0)
			{
				vUpperBound += _TaskCardinalValuesUpperBound[nIndex][nCoalitionSize];
				_PSPartitionToTaskIndex[nSkewedIndex++] = nIndex;
				_PSCurrentPartition.push_back(nCoalitionSize);
			}
			else
			{
				vStartValue += _UtilityValues[nIndex][0];
			}
		}

		if (!IsBetterThanCurrentBest(vUpperBound + vStartValue))
			return;

		__SearchPartition(vUpperBound, vStartValue);
	}

	void CAnytimeSearcher::__SearchPartition
	(
		const float UpperBoundRemaining, // The highest value we can achieve from the coalitions that are not filled yet.
		const float CurrentValue, // The value we have got from the coalitions that have already been filled.
		const uint32_t nAgentIndex
	)
	{
		if (nAgentIndex == _nAgents)
		{
			if (IsBetterThanCurrentBest(CurrentValue))
			{
				_bHasFoundSolution = true;
				_vBestSolutionValue = CurrentValue;
				_BestSolution = _CurrentSolution; // Copy new result.
				_vMaximumLowestBound = _vBestSolutionValue;
			}

			return;
		}

		// Attempt to add this agent to every task and evaluate the value it'd give.
		for (uint32_t nSkewedTaskIndex = 0; nSkewedTaskIndex < _PSCurrentPartition.size(); ++nSkewedTaskIndex)
		{
			if (HasTimeElapsed())
			{
				return;
			}

			auto& nCoalitionSize = _PSCurrentPartition[nSkewedTaskIndex]; // The number of "empty" spots left in this coalition.
			if (nCoalitionSize == 0)
			{
				continue; // Already full coalition.
			}

			const int nTaskIndex = _PSPartitionToTaskIndex[nSkewedTaskIndex];

			// In order for us to calculate the value very quickly, we can simply use the mask of this coalition
			// to retrieve its value from the pre-calculated table.

			// Remove one spot from this partition, and add the agent to the coalition mask.
			--nCoalitionSize;
			_CurrentSolution.add_agent_to_coalition(nAgentIndex, nTaskIndex);

			// Calculate what value we get from adding this agent this way.
			float vValue = CurrentValue;
			float vNewUpperBoundRemaining = UpperBoundRemaining;

			if (nCoalitionSize == 0)
			{
				// If the coalition is now full, we retrieve its value by using its mask.
				vValue += _pProblem->get_value_of(_CurrentSolution.get_coalition(nTaskIndex), nTaskIndex);

				// Also update upper bound, so we can cancel searches that are unneccessary.
				const int nSizeOfOriginalPartition = _PSOriginalPartition[nTaskIndex];
				vNewUpperBoundRemaining -= _TaskCardinalValuesUpperBound[nTaskIndex][nSizeOfOriginalPartition];
			}

			// Add next agent if this assignment can lead to an optimal solution.
			if (IsBetterThanCurrentBest(vNewUpperBoundRemaining + vValue))
			{
				__SearchPartition(vNewUpperBoundRemaining, vValue, nAgentIndex + 1);
			}

			// Reset state back to what it was.
			_CurrentSolution.remove_agent_from_coalition(nAgentIndex, nTaskIndex);
			++nCoalitionSize;
		}

		return;
	}

	void CAnytimeSearcher::SearchCurrentPermutations()
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
				StartPartitionSearch(Permutations[PermutationIndex]._Permutation);
			}
			else
			{
				break; // We can discard the rest (since the search queue is sorted on upper bound first).
			}
		}
	}

	instance_solution CAnytimeSearcher::FindOptimalCoalitionStructure
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
			if (_nTasks == 1)
			{
				Result.reset(_nTasks, _nAgents);

				for (uint32_t i = 0; i < _nAgents; ++i)
					Result.add_agent_to_coalition(i, 0);

				Result.recalculate_value(_pProblem);

				return Result;
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
		}
		else
		{
			_bHasFoundSolution = true;
			_vBestSolutionValue = pInitialSolution->value;
			_BestSolution = *pInitialSolution;
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
		if (bTryUseAGIAsLowerBound)
		{
			for (uint32_t nPartitionIndex = 0; nPartitionIndex < _Partitions.size(); ++nPartitionIndex)
			{
				if (!IsBetterThanCurrentBest(_PartitionsUpperBound[nPartitionIndex]))
				{
					continue;
				}
				auto GreedySolution = GreedySolver.solve(_pProblem, &_Partitions[nPartitionIndex]);
				if (IsBetterThanCurrentBest(GreedySolution.value))
				{
					_BestSolution = GreedySolution;
					_vBestSolutionValue = GreedySolution.value;
					_vMaximumLowestBound = _vBestSolutionValue;
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

		InitializePartitionSearch(); // Initialize search.

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

			if (!IsBetterThanCurrentBest(vSubspaceUpperBound))
			{
				break;
			}

			// Non-increasing order for next_permutation.
			std::sort(CurrentPartition.begin(), CurrentPartition.end());

			do
			{
				// Calculate a tighter (better) upper bound.
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
						if (IsBetterThanCurrentBest(GreedySolution.value))
						{
							_BestSolution = GreedySolution;
							_vBestSolutionValue = GreedySolution.value;
							_vMaximumLowestBound = _vBestSolutionValue;
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

		Result = _BestSolution;
		Result.value = _vBestSolutionValue;

		return Result;
	}
}

instance_solution solver_mp_anytime::solve(coalitional_values_generator* problem, const instance_solution* initial_solution)
{
	MPAnytime::CAnytimeSearcher generator(problem, problem->get_n_agents(), problem->get_n_tasks(), problem->get_data());
	return generator.FindOptimalCoalitionStructure(initial_solution, false, vTimeLimit);
}

instance_solution solver_mp_anytime::solve(coalitional_values_generator* problem)
{
	return solve(problem, nullptr);
}