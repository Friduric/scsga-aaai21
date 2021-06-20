#include "solver_genetic.h"

std::vector<instance_solution> solver_genetic::RecombineAndBreed(std::vector<uint32_t>& ParentIndices)
{
	// Initialize new children.
	std::vector<instance_solution> NewChildren(_nNewChildrenPerMutation);
	for (auto& Child : NewChildren)
	{
		Child.ordered_coalition_structure.assign(_pProblem->get_n_tasks(), coalition::coalition_t{ 0u });
	}

	// Mutate (generate solutions).
	switch (_MutationStrategy)
	{
	case MUTATION_STRATEGY::RANDOM:
		std::vector<uint32_t> CurrentParentIndices(_nParentsPerChild);
		for (uint32_t nChildIndex = 0; nChildIndex < NewChildren.size(); ++nChildIndex)
		{
			auto& NewChild = NewChildren[nChildIndex]; // Use this reference to build a new solution.

			// Select parents to breed from.
			for (uint32_t nParentIndex = 0; nParentIndex < CurrentParentIndices.size(); ++nParentIndex)
			{
				const uint32_t nRandomParentIndex = _UniformMutaIndexGenerator(_RandomGenerator);
				CurrentParentIndices[nParentIndex] = nRandomParentIndex;
			}

			// Use parents to breed.
			std::shuffle(ShuffledCoalitionIndices.begin(), ShuffledCoalitionIndices.end(), _RandomGenerator);
			uint32_t nAllAgentBits = (~0u) >> (32u - _pProblem->get_n_agents());
			for (const uint32_t nCoalitionIndex : ShuffledCoalitionIndices)
			{
				// Select a random parent to choose coalition from. 
				const uint32_t nSkewedRandomParentIndex = _UniformMutaIndexGenerator(_RandomGenerator);
				const uint32_t nRealParentIndex = ParentIndices[nSkewedRandomParentIndex];

				// Retrieve the parent's coalition.
				const uint32_t nParentCoalition = _PopulationPool[nRealParentIndex].ordered_coalition_structure[nCoalitionIndex].get_agent_mask();

				// Update the child's coalition.
				NewChild.ordered_coalition_structure[nCoalitionIndex].set_value(nParentCoalition & nAllAgentBits); // Prevent "agent dupes".
				nAllAgentBits &= ~(nParentCoalition);
			}

			// Go through all coalitions, and assign the unassigned agents.
			std::shuffle(ShuffledCoalitionIndices.begin(), ShuffledCoalitionIndices.end(), _RandomGenerator);
			for (uint32_t nCoalitionIndex : ShuffledCoalitionIndices)
			{
				if (nAllAgentBits == 0u) break;

				std::shuffle(ParentIndices.begin(), ParentIndices.end(), _RandomGenerator);
				uint32_t nChildCoalition = NewChild.ordered_coalition_structure[nCoalitionIndex].get_agent_mask();
				for (const uint32_t ParentIndex : ParentIndices)
				{
					uint32_t nParentCoalition = _PopulationPool[ParentIndex].ordered_coalition_structure[nCoalitionIndex].get_agent_mask();
					NewChild.ordered_coalition_structure[nCoalitionIndex].set_value(nChildCoalition | (nParentCoalition & nAllAgentBits));
					nChildCoalition |= (nParentCoalition & nAllAgentBits);
				}
			}
		}
	}
	return NewChildren;
}

std::vector<uint32_t> solver_genetic::SelectPromisingParentsFromPopulationPool()
{
	std::vector<uint32_t> ParentIndices(_nParentsPerMutation);
	switch (_ParentSelectionStrategy)
	{
	case SELECTION_STRATEGY::BEST:
		const auto ValueBasedOrderFunction = [&](const instance_solution& lhs, const instance_solution& rhs)
		{
			return lhs.value > rhs.value;
		};
		sort(_PopulationPool.begin(), _PopulationPool.end(), ValueBasedOrderFunction);
		for (uint32_t nParentIndex = 0; nParentIndex < _nParentsPerMutation; ++nParentIndex)
		{
			ParentIndices[nParentIndex] = nParentIndex;
		}
		break;
	}
	return ParentIndices;
}

void solver_genetic::MutatePopulationPool()
{
	// Select promising parents to breed.
	std::vector<uint32_t> ParentIndices = SelectPromisingParentsFromPopulationPool();

	// Recombine solutions to generate new ones.
	std::vector<instance_solution> Children = RecombineAndBreed(ParentIndices);
	for (const auto& Child : Children)
	{
		if (Child.value > _BestSolution.value)
		{
			_BestSolution = Child;
		}
	}
	// Select the best new solutions and keep them (i.e., merge with population pool).
	switch (_ChildRemovalStrategy)
	{
	case DISCARD_STRATEGY::RANDOM:
		for (uint32_t nChildIndex = 0; nChildIndex < Children.size(); ++nChildIndex)
		{
			uint32_t nRandomParentIndex = _UniformPoolIndexGenerator(_RandomGenerator);
			_PopulationPool[nRandomParentIndex] = Children[nChildIndex];
		}
		break;
	}
}

void solver_genetic::InitializePopulationPool()
{
	_BestSolution.value = std::numeric_limits<coalition::value_t>().lowest();
	_PopulationPool.reserve(uint64_t(_nPopulationPoolSize) + uint64_t(_nNewChildrenPerMutation));
	_PopulationPool.resize(_nPopulationPoolSize);
	for (unsigned nIndex = 0; nIndex < _nPopulationPoolSize; ++nIndex)
	{
		auto& coalition_structure = _PopulationPool[nIndex].ordered_coalition_structure;
		coalition_structure.assign(_pProblem->get_n_tasks(), coalition::coalition_t{ 0 });

		// Randomly assign agents. 
		for (uint32_t nAgentIndex = 0; nAgentIndex < _pProblem->get_n_agents(); ++nAgentIndex)
		{
			uint32_t nRandomTaskIndex = _UniformTaskIndexGenerator(_RandomGenerator);
			assert(nRandomTaskIndex >= 0 && nRandomTaskIndex < _pProblem->get_n_tasks());
			coalition_structure[nRandomTaskIndex].add_agent(nAgentIndex);
		}

		_PopulationPool[nIndex].recalculate_value(_pProblem);

		// Keep track of the best solution found so far.
		if (_PopulationPool[nIndex].value > _BestSolution.value)
		{
			_BestSolution = _PopulationPool[nIndex];
		}
	}
}

instance_solution solver_genetic::solve(coalitional_values_generator* problem)
{
	assert(problem->get_n_agents() <= 32);
	_pProblem = problem;

	if (_nSeed > 0)
		_RandomGenerator.seed(_nSeed);

	_UniformTaskIndexGenerator = std::uniform_int_distribution<uint32_t>(0, problem->get_n_tasks() - 1);
	_UniformPoolIndexGenerator = std::uniform_int_distribution<uint32_t>(0, _nPopulationPoolSize - 1);
	_UniformMutaIndexGenerator = std::uniform_int_distribution<uint32_t>(0, _nParentsPerChild - 1);

	ShuffledCoalitionIndices.reserve(_pProblem->get_n_tasks());
	for (uint32_t nCoalitionIndex = 0; nCoalitionIndex < _pProblem->get_n_tasks(); ++nCoalitionIndex)
	{
		ShuffledCoalitionIndices.push_back(nCoalitionIndex);
	}

	InitializePopulationPool();

	unsigned long long nMaxIterations = std::max<unsigned long long>(1ULL, _nMaxMutations);

	std::chrono::high_resolution_clock::time_point t1, t2;
	if (vTimeLimit >= 0) {
		t1 = std::chrono::high_resolution_clock::now();
	}

	unsigned nIteration = 0;
	for (nIteration = 0; nIteration < nMaxIterations; ++nIteration)
	{
		MutatePopulationPool();

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
	std::cout << nIteration << std::endl;

	return _BestSolution;
}
