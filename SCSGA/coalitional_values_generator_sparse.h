#pragma once

#include "coalitional_values_generator.h"
#include <string>

class coalitional_values_generator_sparse : public coalitional_values_generator
{
public:
	coalitional_values_generator_sparse();
	bool use_uniform = false;
	coalition::value_t generate_new_value(const coalition::coalition_t& coalition, const uint32_t task) override;

protected:
	std::string get_file_name() const override;
	void reset(uint32_t n_agents, uint32_t n_tasks, int seed = 0) override;
	
private:
	const float probability_to_draw_good_value = 0.01f;

	std::default_random_engine generator;
	std::normal_distribution<coalition::value_t> sparse_generator;
	std::uniform_real_distribution<coalition::value_t> sparse_uniform_generator;
	std::uniform_real_distribution<coalition::value_t> uniform_01_generator;
};

