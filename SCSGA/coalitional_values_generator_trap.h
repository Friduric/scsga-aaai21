#pragma once

#include "coalitional_values_generator.h"
#include "utility.h"

class coalitional_values_generator_trap : public coalitional_values_generator
{
public:
	coalitional_values_generator_trap() = default;

	coalition::value_t generate_new_value(const coalition::coalition_t& coalition, const uint32_t task) override;

protected:
	std::string get_file_name() const override;
	void reset(uint32_t n_agents, uint32_t n_tasks, int seed) override;

private:
	const int MODE = 1; // 0 == old, 1 == "Läkare utan gränser" (new mode).

	std::default_random_engine generator;
	std::vector<std::normal_distribution<coalition::value_t>> trap_generators;
	std::vector<std::vector<std::normal_distribution<coalition::value_t>>> task_trap_generators;
	std::vector<float> values;
};