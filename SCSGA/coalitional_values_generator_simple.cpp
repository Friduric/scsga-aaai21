#include "coalitional_values_generator_simple.h"
#include "utility.h"
#include <cassert>

coalitional_values_generator_simple::coalitional_values_generator_simple(coalition::value_t _size_factor) :
	size_factor(_size_factor)
{

}

void coalitional_values_generator_simple::reset(uint32_t n_agents, uint32_t n_tasks, int seed)
{
	
}

coalition::value_t coalitional_values_generator_simple::get_value_of(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	if (n_agents <= 32)
	{
		return coalitional_values_generator::get_value_of(coalition, task);
	}
	else
	{
		return coalition.count_agents_in_coalition() * size_factor;
	}
}

coalition::value_t coalitional_values_generator_simple::generate_new_value(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	return coalition.count_agents_in_coalition() * size_factor;
}

std::string coalitional_values_generator_simple::get_file_name() const
{
	return "simple_" + std::to_string(seed) + "_" + std::to_string(n_agents) + "_" + std::to_string(n_tasks) + ".problem";
}