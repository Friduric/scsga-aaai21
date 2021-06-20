#pragma once

#include "solver.h"
#include <iostream> // For testing.
#include <random>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <chrono>

class solver_genetic : public solver
{
	std::uniform_int_distribution<uint32_t> _UniformTaskIndexGenerator;
	std::uniform_int_distribution<uint32_t> _UniformPoolIndexGenerator;
	std::uniform_int_distribution<uint32_t> _UniformMutaIndexGenerator;
	std::default_random_engine _RandomGenerator;

	std::vector<instance_solution> _PopulationPool; // Contains the current population pool.
	instance_solution _BestSolution; // The best solution found so far.

	coalitional_values_generator* _pProblem = nullptr;
public:
	enum class DISCARD_STRATEGY	
	{ 
		RANDOM, 
		KEEP_ALL_CHILDREN_REMOVE_WORST_PARENTS 
	};
	enum class MUTATION_STRATEGY
	{ 
		RANDOM
	};
	enum class SELECTION_STRATEGY
	{ 
		BEST 
	};

	// --------------------------------------------------------------------------------------
	// Settings.
	// --------------------------------------------------------------------------------------
	uint32_t _nParentsPerMutation = 50;	// The number of parents per mutative-phase.
	uint32_t _nNewChildrenPerMutation = 100; // The number of new children per mutative-phase.
	uint32_t _nParentsPerChild = 3;	// The number of parents needed to breed a new solution.
	uint32_t _nPopulationPoolSize = 500; // The total number of solutions in the population at all times.
	uint64_t _nMaxMutations = 118200000000ULL;

	// Dictates which new children to remove ("abort"/discard).
	DISCARD_STRATEGY	_ChildRemovalStrategy		= DISCARD_STRATEGY::RANDOM;

	// Dictates how to mutate and breed new children.
	MUTATION_STRATEGY	_MutationStrategy			= MUTATION_STRATEGY::RANDOM;

	// Dictates which parents to select for breeding.
	SELECTION_STRATEGY	_ParentSelectionStrategy	= SELECTION_STRATEGY::BEST;

	unsigned _nSeed = 0;

private:
	// --------------------------------------------------------------------------------------
	// Genetic algorithm.
	// --------------------------------------------------------------------------------------
	
	std::vector<uint32_t> ShuffledCoalitionIndices;

	// This method breeds a selected vector of solutions.
	std::vector<instance_solution> RecombineAndBreed(std::vector<uint32_t>& ParentIndices);

	std::vector<uint32_t> SelectPromisingParentsFromPopulationPool();

	void MutatePopulationPool();

	void InitializePopulationPool();

public:
	instance_solution solve(coalitional_values_generator* problem) override;
};

