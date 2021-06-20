#include "coalitional_values_generator_NPD.h"

#include <cassert>
#include <fstream>
#include <string>
#include <unordered_set>
#include <iostream>

#include "coalition.h"

coalitional_values_generator_NPD::coalitional_values_generator_NPD() :
	npd_generator(coalition::value_t(1.0), coalition::value_t(0.1))
{

}

void coalitional_values_generator_NPD::reset(uint32_t n_agents, uint32_t n_tasks, int seed)
{
	if (seed >= 0)
		generator.seed(seed); // Used to generate samples.
}

coalition::value_t coalitional_values_generator_NPD::generate_new_value(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	return npd_generator(generator);
}

std::string coalitional_values_generator_NPD::get_file_name() const
{
	return "NPD_" + std::to_string(seed) + "_" + std::to_string(n_agents) + "_" + std::to_string(n_tasks) + ".problem";
}