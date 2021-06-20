#include "coalitional_values_generator_NRD.h"
#include "utility.h"
#include <cassert>

coalitional_values_generator_NRD::coalitional_values_generator_NRD() :
	nrd_generator(coalition::value_t(0), coalition::value_t(0.1)), AgentToAgentToTaskSkillLevel{}
{

}

void coalitional_values_generator_NRD::reset
(
	uint32_t n_agents,
	uint32_t n_tasks,
	int seed
)
{
	if (seed >= 0)
		generator.seed(seed); // Used to generate samples.

	// Generate "relational utility" for each agent-to-task assignment.
	AgentToAgentToTaskSkillLevel = std::vector < std::vector<std::vector<coalition::value_t>>>
		(n_agents, std::vector < std::vector<coalition::value_t>>(n_agents, std::vector<coalition::value_t >(n_tasks, 0.0)));
	for (uint32_t n_agent_index_i = 0; n_agent_index_i < n_agents; ++n_agent_index_i)
		for (uint32_t n_agent_index_j = n_agent_index_i + 1; n_agent_index_j < n_agents; ++n_agent_index_j)
			for (uint32_t n_task_index = 0; n_task_index < n_tasks; ++n_task_index)
			{
				auto& relational_utility_ij = AgentToAgentToTaskSkillLevel[n_agent_index_i][n_agent_index_j][n_task_index];
				auto& relational_utility_ji = AgentToAgentToTaskSkillLevel[n_agent_index_j][n_agent_index_i][n_task_index];
				relational_utility_ij = relational_utility_ji = nrd_generator(generator);
			}

		// TODO See if this can be integrated with the new coalitional values generation.
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

coalition::value_t coalitional_values_generator_NRD::get_value_of(
	const coalition::coalition_t& coalition,
	const uint32_t n_task
)
{
	if (n_agents <= 32)
	{
		return coalitional_values_generator::get_value_of(coalition, n_task);
	}
	else
	{
		return generate_new_value(coalition, n_task);
	}
}

coalition::value_t coalitional_values_generator_NRD::generate_new_value(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	coalition::value_t value{};
	const std::vector<uint32_t> agents{ coalition.get_all_agents() };
	auto it_1 = agents.cbegin();
	while (it_1 != agents.cend())
	{
		auto it_2 = it_1 + 1;
		while (it_2 != agents.cend())
		{
			value += AgentToAgentToTaskSkillLevel[*it_1][*it_2][task];
			++it_2;
		}
		++it_1;
	}
	return value;
}

std::string coalitional_values_generator_NRD::get_file_name() const
{
	return "NRD_" + std::to_string(seed) + "_" + std::to_string(n_agents) + "_" + std::to_string(n_tasks) + ".problem";
}