#include "instance_solution.h"

#include <unordered_set>

#include "coalitional_values_generator_custom.h"

instance_solution::instance_solution(value_t _value, ordered_coalition_structure_t _best_ordered_coalition_structure)
	: value(_value), ordered_coalition_structure(_best_ordered_coalition_structure)
{

}

instance_solution::instance_solution()
	: value(0)
{

}

value_t instance_solution::recalculate_value(coalitional_values_generator* problem)
{
	value = 0;
	for (uint32_t nTaskIndex = 0; nTaskIndex < problem->get_n_tasks(); ++nTaskIndex)
	{
		value += calc_value_of_coalition(problem, nTaskIndex);
	}
	return value;
}

value_t instance_solution::calc_value_of_coalition(coalitional_values_generator* problem, const uint32_t nCoalitionIndex) const
{
	return problem->get_value_of(ordered_coalition_structure[nCoalitionIndex], nCoalitionIndex);
}

void instance_solution::reset(const uint32_t n_tasks, const uint32_t n_agents)
{
	ordered_coalition_structure.assign(n_tasks, coalition_t(n_agents));
	value = std::numeric_limits<coalition::value_t>::lowest();
}

void instance_solution::add_agent_to_coalition(const uint32_t n_agent_index, const uint32_t n_coalition_index)
{
	ordered_coalition_structure[n_coalition_index].add_agent(n_agent_index);
}

void instance_solution::remove_agent_from_coalition(const uint32_t n_agent_index, const uint32_t n_coalition_index)
{
	ordered_coalition_structure[n_coalition_index].remove_agent(n_agent_index);
}

bool instance_solution::is_agent_in_coalition(const uint32_t agent, const uint32_t coalition) const
{
	return ordered_coalition_structure[coalition].is_agent_in_coalition(agent);
}

uint32_t instance_solution::n_agents_in_coalition(const uint32_t coalition) const
{
	return ordered_coalition_structure[coalition].count_agents_in_coalition();
}

std::vector<int> instance_solution::get_coalition_indices_of_agents(uint32_t number_of_agents) const
{
	std::vector<int> agent_indices(number_of_agents, -1);
	for (uint32_t t = 0; t < ordered_coalition_structure.size(); ++t)
	{
		for (uint32_t a = 0; a < number_of_agents; ++a)
		{
			if (is_agent_in_coalition(a, t))
			{
				agent_indices[a] = t;
			}
		}
	}
	return agent_indices;
}

int instance_solution::get_coalition_index_of_agent(const uint32_t agent_index) const
{
	for (uint32_t i = 0; i < ordered_coalition_structure.size(); ++i)
	{
		if (is_agent_in_coalition(agent_index, i))
		{
			return i;
		}
	}
	return -1;
}

coalition::coalition_t instance_solution::get_coalition(const uint32_t coalition) const
{
	return ordered_coalition_structure[coalition];
}

void instance_solution::set_coalition(const uint32_t coalition_index, const coalition::coalition_t coalition_value)
{
	ordered_coalition_structure[coalition_index] = coalition_value;
}

uint32_t instance_solution::get_coalition_mask(const uint32_t coalition_index) const
{
	/*
	The purpose of this method is to provide backwards compatibility with algorithms using a single uint32_t
	as coalition_t. As such, it is only meant to be used when n_agents in at most 32 and the coalition agent
	vectors consist of a single uint32_t.
	*/
	assert(ordered_coalition_structure[coalition_index].get_n_agents() <= 32);
	return ordered_coalition_structure[coalition_index].get_agent_mask();
}

void instance_solution::set_coalition(const uint32_t coalition_index, const uint32_t coalition_agent_mask)
{
	/*
	The purpose of this method is to provide backwards compatibility with algorithms using a single uint32_t
	as coalition_t. As such, it is only meant to be used when n_agents in at most 32 and the coalition agent
	vectors consist of a single uint32_t.
	*/
	assert(ordered_coalition_structure[coalition_index].get_n_agents() <= 32);
	ordered_coalition_structure[coalition_index].set_value(coalition_agent_mask);
}

/* Given a problem and a partial solution (where a partial solution is defined as an ordered coalition structure over some subset of the problem's agent set),
   this generates a partial problem (TODO: Need better definition/description of what a partial problem is, TODO: Add support for caching values). */
coalitional_values_generator* instance_solution::convert_to_partial_problem(coalitional_values_generator* initial_problem, const instance_solution& partial_solution)
{
	uint32_t n = initial_problem->get_n_agents();
	uint32_t m = initial_problem->get_n_tasks();

	// Find assigned agents.
	std::vector<uint32_t> assigned_agents{}, unassigned_agents{};
	for (uint32_t agent{}; agent < n; ++agent)
	{
		bool assigned{ false };
		for (uint32_t coalition{}; coalition < m && !assigned; ++coalition)
		{
			if (partial_solution.is_agent_in_coalition(agent, coalition))
			{
				assigned = true;
			}
		}
		if (assigned)
		{
			assigned_agents.push_back(agent);
		}
		else
		{
			unassigned_agents.push_back(agent);
		}
	}

	uint32_t new_n{ (uint32_t)unassigned_agents.size() };
	assert(new_n <= 32);

	// -------------------------------------------------------------
	// Generate a new problem that we "fill" with values so that
	// the optimal solution to this problem has the same value as the
	// optimal solution to the initial problem, given that the agents
	// are assigned as specificed by the partial solution.
	coalitional_values_generator_custom* partial_problem = new coalitional_values_generator_custom(new_n, m);
	for (uint32_t nTaskIndex = 0; nTaskIndex < m; ++nTaskIndex)
	{
		// !!! This only works when the number of agents <= 32 !!!

		// For all subsets (i.e., submasks) of the unassigned agents ... 
		for (uint32_t nUnassignedAgentsMask = 0; nUnassignedAgentsMask < (1u << new_n); ++nUnassignedAgentsMask)
		{
			// TODO: Implement assigning value of v({C_j} \cup {C^k}, t_j) to v({C^k}, t_j)
			// so the number of agents can be larger (the current approach is mask-based).

			coalition::coalition_t assigned_agents = partial_solution.get_coalition(nTaskIndex);
			for (uint32_t agent_in_coalition{}; agent_in_coalition < new_n; ++agent_in_coalition)
			{
				if ((1 << agent_in_coalition) & nUnassignedAgentsMask)
				{
					assert(!assigned_agents.is_agent_in_coalition(unassigned_agents[agent_in_coalition]));
					assigned_agents.add_agent(unassigned_agents[agent_in_coalition]);
				}
			}

			coalition::value_t value = initial_problem->get_value_of(assigned_agents, nTaskIndex);
			partial_problem->set_value_of(nUnassignedAgentsMask, nTaskIndex, value);
		}
	}

	return partial_problem;
}