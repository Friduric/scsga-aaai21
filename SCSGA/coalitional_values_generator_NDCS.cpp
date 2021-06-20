#include "coalitional_values_generator_NDCS.h"
#include "utility.h"
#include <cassert>

void coalitional_values_generator_NDCS::reset
(
	uint32_t n_agents, 
	uint32_t n_tasks, 
	int seed
)
{
	if (seed >= 0)
		generator.seed(seed); // Used to generate samples.

	// Each coalition size should have its own normal distribution.
	ndcs_generators = std::vector<std::normal_distribution<coalition::value_t>>(uint64_t(n_agents) + 1ULL);
	for (uint64_t size = 0; size < uint64_t(n_agents) + 1ULL; ++size)
	{
		ndcs_generators.push_back
		(
			std::normal_distribution<coalition::value_t>
			(
				coalition::value_t(size),
				coalition::value_t(size == 0 ? 0.000000001f : sqrtf(float(size)))
			)
		);
	}
}

coalition::value_t coalitional_values_generator_NDCS::generate_new_value(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	return ndcs_generators[coalition.count_agents_in_coalition()](generator);
}

std::string coalitional_values_generator_NDCS::get_file_name() const
{
	return "NDCS" + std::to_string(seed) + "_" + std::to_string(n_agents) + "_" + std::to_string(n_tasks) + ".problem";
}