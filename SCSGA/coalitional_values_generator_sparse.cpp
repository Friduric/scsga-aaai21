#include "coalitional_values_generator_sparse.h"
#include <cassert>

coalitional_values_generator_sparse::coalitional_values_generator_sparse() :
	sparse_generator(coalition::value_t(1.0), coalition::value_t(0.1)),
	uniform_01_generator(coalition::value_t(0.0), coalition::value_t(1.0)),
	sparse_uniform_generator(coalition::value_t(0.0), coalition::value_t(1.0))
{

}

void coalitional_values_generator_sparse::reset(uint32_t n_agents, uint32_t n_tasks, int seed)
{
	if (seed >= 0)
	{
		generator.seed(seed); // Used to generate samples.
	}
}

coalition::value_t coalitional_values_generator_sparse::generate_new_value(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	if (uniform_01_generator(generator) < probability_to_draw_good_value)
	{
		if (use_uniform)
		{
			return sparse_uniform_generator(generator);
		}
		return sparse_generator(generator);
	}
	else
	{
		if (use_uniform)
		{
			return 0.1f * sparse_uniform_generator(generator);
		}
		return 0.1f * sparse_generator(generator);
	}
}

std::string coalitional_values_generator_sparse::get_file_name() const
{
	return "sparse_" + std::to_string(seed) + "_" + std::to_string(n_agents) + "_" + std::to_string(n_tasks) + ".problem";
}