#include "coalitional_values_generator_NSRD.h"
#include "utility.h"
#include <cassert>

coalitional_values_generator_NSRD::coalitional_values_generator_NSRD() :
	nrd_generator(coalition::value_t(1.0), coalition::value_t(0.1)),
	nsd_generator(coalition::value_t(1.0), coalition::value_t(0.1))
{

}

void coalitional_values_generator_NSRD::reset
(
	uint32_t n_agents,
	uint32_t n_tasks,
	int seed
)
{
	if (seed >= 0)
		generator.seed(seed); // Used to generate samples.

	// Generate "skill-level" for each agent-to-task assignment.
	AgentToTaskSkillLevel = std::vector<std::vector<coalition::value_t>>(n_agents, std::vector<coalition::value_t>(n_tasks));
	for (uint32_t n_agent_index = 0; n_agent_index < n_agents; ++n_agent_index)
		for (uint32_t n_task_index = 0; n_task_index < n_tasks; ++n_task_index)
			AgentToTaskSkillLevel[n_agent_index][n_task_index] = nsd_generator(generator);

	// Generate "relational utility" for each agent-to-task assignment.
	AgentToAgentToTaskSkillLevel =
		std::vector<std::vector<std::vector<coalition::value_t>>>(n_agents, std::vector<std::vector<coalition::value_t>>(n_agents, std::vector<coalition::value_t>(n_tasks)));
	for (uint32_t n_agent_index_i = 0; n_agent_index_i < n_agents; ++n_agent_index_i)
		for (uint32_t n_agent_index_j = n_agent_index_i + 1; n_agent_index_j < n_agents; ++n_agent_index_j)
			for (uint32_t n_task_index = 0; n_task_index < n_tasks; ++n_task_index)
			{
				auto& relational_utility_ij = AgentToAgentToTaskSkillLevel[n_agent_index_i][n_agent_index_j][n_task_index];
				auto& relational_utility_ji = AgentToAgentToTaskSkillLevel[n_agent_index_j][n_agent_index_i][n_task_index];
				relational_utility_ij = relational_utility_ji = nrd_generator(generator);
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

coalition::value_t coalitional_values_generator_NSRD::get_value_of(
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
		return generate_new_value(coalition, task);
	}
}

coalition::value_t coalitional_values_generator_NSRD::generate_new_value(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	coalition::value_t coalitional_value = 0.0f;
	coalition::value_t skill_value = 0.0f;
	uint32_t skill_value_samples = 0;
	coalition::value_t relation_value = 0.0f;
	uint32_t relation_value_samples = 0;
	std::vector<uint32_t> agents_in_coalition{ coalition.get_all_agents() };
	for (uint32_t i = 0; i < agents_in_coalition.size(); ++i)
	{
		uint32_t n_agent_index_i = agents_in_coalition[i];
		skill_value += AgentToTaskSkillLevel[n_agent_index_i][task];
		++skill_value_samples;
		for (uint32_t j = i + 1; j < agents_in_coalition.size(); ++j)
		{
			uint32_t n_agent_index_j = agents_in_coalition[j];
			relation_value += AgentToAgentToTaskSkillLevel[n_agent_index_i][n_agent_index_j][task];
			++relation_value_samples;
		}
	}

	if (skill_value_samples > 0)
		skill_value /= float(skill_value_samples);

	if (relation_value_samples > 0)
		relation_value /= float(relation_value_samples);

	coalitional_value = skill_value + relation_value;
	return coalitional_value;
}

std::string coalitional_values_generator_NSRD::get_file_name() const
{
	return "NSRD_" + std::to_string(seed) + "_" + std::to_string(n_agents) + "_" + std::to_string(n_tasks) + ".problem";
}