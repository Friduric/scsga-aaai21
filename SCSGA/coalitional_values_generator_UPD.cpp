#include "coalitional_values_generator_UPD.h"
#include <cassert>

coalitional_values_generator_UPD::coalitional_values_generator_UPD() :
	upd_generator(coalition::value_t(0.0), coalition::value_t(1.0))
{

}

void coalitional_values_generator_UPD::reset(uint32_t n_agents, uint32_t n_tasks, int seed)
{
	if (seed >= 0)
		generator.seed(seed); // Used to generate samples.
}

coalition::value_t coalitional_values_generator_UPD::generate_new_value(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	return upd_generator(generator);
}

std::string coalitional_values_generator_UPD::get_file_name() const
{
	return "UPD_" + std::to_string(seed) + "_" + std::to_string(n_agents) + "_" + std::to_string(n_tasks) + ".problem";
}