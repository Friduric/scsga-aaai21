#pragma once

#include "assert.h"
#include "coalition.h"
#include "coalitional_values_generator.h"
#include "utility.h"

#include <vector>

using namespace coalition;

class instance_solution
{
public:
	coalition::value_t value;
	ordered_coalition_structure_t ordered_coalition_structure;
	instance_solution(value_t _value, ordered_coalition_structure_t _ordered_coalition_structure);
	instance_solution();
	value_t recalculate_value(coalitional_values_generator* problem);
	value_t calc_value_of_coalition(coalitional_values_generator* problem, const uint32_t coalition) const;

	coalition::value_t get_value() const {
		return value;
	}

	void reset(const uint32_t n_tasks, const uint32_t n_agents);
	void add_agent_to_coalition(const uint32_t agent, const uint32_t coalition);
	void remove_agent_from_coalition(const uint32_t agent, const uint32_t coalition);

	bool is_agent_in_coalition(const uint32_t agent, const uint32_t coalition) const;
	uint32_t n_agents_in_coalition(const uint32_t coalition) const;

	std::vector<int> get_coalition_indices_of_agents(uint32_t number_of_agents) const;
	int get_coalition_index_of_agent(const uint32_t agent_index) const;
	coalition::coalition_t get_coalition(const uint32_t coalition) const;
	uint32_t get_coalition_mask(const uint32_t coalition_index) const; // Requires n_agents <= 32
	void set_coalition(const uint32_t coalition_index, const coalition::coalition_t coalition_value);
	void set_coalition(const uint32_t coalition_index, const uint32_t coalition_agent_mask); // Requires n_agents <= 32

	coalitional_values_generator* convert_to_partial_problem
	(
		coalitional_values_generator* initial_problem,
		const instance_solution& partial_solution
	);
};

