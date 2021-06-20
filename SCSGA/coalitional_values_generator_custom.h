#pragma once

#include "coalitional_values_generator.h"

#include <string>

class coalitional_values_generator_custom : public coalitional_values_generator
{
public:
	coalitional_values_generator_custom(const uint32_t n_agents, const uint32_t n_tasks);

	void reset(const uint32_t n_agents, const uint32_t n_tasks, int seed = 0) override;
	coalition::value_t generate_new_value(const coalition::coalition_t& coalition, const uint32_t task) override;
	void generate_coalitional_values(const uint32_t n_agents, const uint32_t n_tasks, int seed = 0) override;

	void set_value_of(const uint32_t coalition_agent_mask, const uint32_t n_task, const coalition::value_t v_value);

protected:
	std::string get_file_name() const override;
};