#pragma once
#include "coalitional_values_generator.h"
class coalitional_values_generator_NSRD : public coalitional_values_generator
{
public:
	coalitional_values_generator_NSRD();

	coalition::value_t get_value_of(const coalition::coalition_t& coalition, const uint32_t task) override;
	coalition::value_t generate_new_value(const coalition::coalition_t& coalition, const uint32_t task) override;

protected:
	std::string get_file_name() const override;
	void reset(uint32_t n_agents, uint32_t n_tasks, int seed = 0) override;

private:
	std::default_random_engine generator;
	std::normal_distribution<coalition::value_t> nrd_generator, nsd_generator;
	std::vector<std::vector<coalition::value_t>> AgentToTaskSkillLevel;
	std::vector<std::vector<std::vector<coalition::value_t>>> AgentToAgentToTaskSkillLevel;
};

