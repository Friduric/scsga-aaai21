#include "coalitional_values_generator_NSD.h"
#include "utility.h"
#include <cassert>

coalitional_values_generator_NSD::coalitional_values_generator_NSD() :
	nsd_generator(coalition::value_t(0.0), coalition::value_t(0.1)), TaskToAgentSkillLevel{}
{

}

void coalitional_values_generator_NSD::reset
(
	uint32_t n_agents,
	uint32_t n_tasks,
	int seed
)
{
	if (seed >= 0)
		generator.seed(seed); // Used to generate samples.

	// Generate "skill-level" for each agent-to-task assignment.
	TaskToAgentSkillLevel = std::vector<std::vector<coalition::value_t>>(n_tasks, std::vector<coalition::value_t>(n_agents));
	for (uint32_t n_agent_index = 0; n_agent_index < n_agents; ++n_agent_index) {
		for (uint32_t n_task_index = 0; n_task_index < n_tasks; ++n_task_index) {
			TaskToAgentSkillLevel[n_task_index][n_agent_index] = nsd_generator(generator);
		}
	}

		// TODO Try to integrate this with the new coalitional values generation.
		// Generate a vector for each coalition mask that represents the agents in that coalition mask 
		// (decreases time complexity of generating values)
		/*
		std::vector<std::vector<uint32_t>> CoalitionToAgentIndices(n_number_of_possible_coalitions, std::vector<uint32_t>());
		for (uint32_t n_coalition_mask = 0ULL; n_coalition_mask < n_number_of_possible_coalitions; ++n_coalition_mask)
			for (uint32_t n_agent_index = 0; n_agent_index < 32; ++n_agent_index)
				if (((1 << n_agent_index) & n_coalition_mask) != 0) // Is agent in coalition?
					CoalitionToAgentIndices[n_coalition_mask].push_back(n_agent_index);
		*/
}

coalition::value_t coalitional_values_generator_NSD::get_value_of(const coalition::coalition_t& coalition, const uint32_t n_task)
{
	coalition::value_t value{};
	if (n_agents <= 32)
	{
		value = coalitional_values_generator::get_value_of(coalition, n_task);
	}
	else
	{
		value = generate_new_value(coalition, n_task);
	}
	return value;
}

coalition::value_t coalitional_values_generator_NSD::generate_new_value(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	coalition::value_t value{};
	const std::vector<uint32_t> agents{ coalition.get_all_agents() };
	for (const uint32_t& agent : agents)
	{
		value += TaskToAgentSkillLevel[task][agent];
	}
	return value;
}

std::string coalitional_values_generator_NSD::get_file_name() const
{
	return "NSD_" + std::to_string(seed) + "_" + std::to_string(n_agents) + "_" + std::to_string(n_tasks) + ".problem";
}