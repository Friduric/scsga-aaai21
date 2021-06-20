#pragma once
#include "coalitional_values_generator.h"

#include <unordered_map>
#include <string>


class coalitional_values_generator_NPD : public coalitional_values_generator
{
public:
	coalitional_values_generator_NPD();
	~coalitional_values_generator_NPD() = default;

	coalition::value_t generate_new_value(const coalition::coalition_t& coalition, const uint32_t task) override;

protected:
	std::string get_file_name() const override;
	void reset(uint32_t n_agents, uint32_t n_tasks, int seed = 0) override;

private:
	std::default_random_engine generator;
	std::normal_distribution<coalition::value_t> npd_generator;
};

