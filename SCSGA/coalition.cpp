#include "coalition.h"
#include "utility.h"

#include <algorithm>
#include <unordered_set>

using namespace coalition;

#ifdef LARGE_COALITIONS

coalition_t::coalition_t()
	: agent_bitset{}, n_agents{}
{

}

coalition_t::coalition_t(const uint32_t n_agents)
	: agent_bitset{}, n_agents{}
{
	reset(n_agents);
}

void coalition_t::reset(const uint32_t n_agents)
{
	this->n_agents = n_agents;
	uint32_t agent_vector_size{ n_agents >> 5 };
	if ((n_agents & 31) > 0 || n_agents == 0)
	{
		++agent_vector_size;
	}

	agent_bitset.assign(agent_vector_size, 0);
}

void coalition_t::add_agent(const uint32_t agent)
{
	// The agent is found at vector index agent // 32 and bit index agent % 32.
	const uint32_t agent_vector_index{ agent >> 5 };
	const uint32_t bit_index{ agent & 31 };
	const uint32_t agent_mask{ 1U << bit_index };

	agent_bitset[agent_vector_index] |= agent_mask;
}

void coalition_t::remove_agent(const uint32_t agent)
{
	// The agent is found at vector index agent // 32 and bit index agent % 32.
	const uint32_t agent_vector_index{ agent >> 5 };
	const uint32_t bit_index{ agent & 31 };
	const uint32_t agent_mask{ 1U << bit_index };

	agent_bitset[agent_vector_index] &= ~agent_mask;
}

bool coalition_t::is_agent_in_coalition(const uint32_t agent) const
{
	const uint32_t agent_vector_index{ agent >> 5 };
	const uint32_t bit_index{ agent & 31 };
	const uint32_t agent_mask{ 1U << bit_index };

	return (agent_bitset[agent_vector_index] & agent_mask) != 0;
}

std::vector<uint32_t> coalition_t::get_all_agents() const
{
	std::vector<uint32_t> agents;
	uint32_t agent{};
	for (const uint32_t& partial_agent_bitmask : agent_bitset)
	{
		for (uint32_t mask{ 1 }; mask; mask <<= 1)
		{
			if (partial_agent_bitmask & mask)
			{
				agents.push_back(agent);
			}
			++agent;
		}
	}
	return agents;
}

uint32_t coalition_t::count_agents_in_coalition() const
{
	uint32_t agents_in_coalition{};
	for (const uint32_t& partial_agent_mask : agent_bitset)
	{
		agents_in_coalition += utility::bits::bit_count_32bit(partial_agent_mask);
	}
	return agents_in_coalition;
}

uint32_t coalition_t::get_n_agents() const
{
	return n_agents;
}

uint32_t coalition_t::get_agent_mask() const
{
	/*
	The purpose of this method is to provide backwards compatibility with algorithms using a single uint32_t
	as coalition_t. As such, it is only meant to be used when n_agents in at most 32 and the coalition agent
	vector consists of a single uint32_t.
	*/
	assert(n_agents <= 32);
	return agent_bitset[0];
}

void coalition_t::set_value(const std::vector<uint32_t> agent_bitset)
{
	assert(this->agent_bitset.size() == agent_bitset.size());
	this->agent_bitset = agent_bitset;
}

void coalition_t::set_value(const uint32_t agent_mask)
{
	assert(n_agents <= 32);
	agent_bitset[0] = agent_mask;
}

bool coalition_t::operator==(const coalition_t& other) const
{
	return agent_bitset == other.agent_bitset;
}

#else

coalition_t::coalition_t()
	: assigned_agents_mask{}, n_agents{}
{

}

coalition_t::coalition_t(const uint32_t n_agents)
	: assigned_agents_mask{}, n_agents{}
{
	assert(n_agents <= 32);
	reset(n_agents);
}

void coalition_t::reset(const uint32_t n_agents)
{
	this->n_agents = n_agents;
	assigned_agents_mask = 0U;
}

void coalition_t::add_agent(const uint32_t agent)
{
	assigned_agents_mask |= (1 << agent);
}

void coalition_t::remove_agent(const uint32_t agent)
{
	assigned_agents_mask &= ~(1 << agent);
}

bool coalition_t::is_agent_in_coalition(const uint32_t agent) const
{
	return (assigned_agents_mask & (1 << agent)) != 0;
}

const std::unordered_set<uint32_t>& coalition_t::get_assigned_agents() const
{
	std::unordered_set<uint32_t> assigned_agents;
	for (uint32_t agent{ 0 }; agent < n_agents; ++agent)
	{
		if (assigned_agents_mask & (1 << agent))
		{
			assigned_agents.insert(agent);
		}
	}
	return assigned_agents;
}

uint32_t coalition_t::count_agents_in_coalition() const
{
	return utility::bits::bit_count_32bit(assigned_agents_mask);
}

uint32_t coalition_t::get_n_agents() const
{
	return n_agents;
}

uint32_t coalition_t::get_agent_mask() const
{
	return assigned_agents_mask;
}

void coalition_t::set_value(const std::vector<uint32_t> agent_bitset)
{
	assigned_agents_mask = agent_bitset[0];
}

void coalition_t::set_value(const uint32_t agent_mask)
{
	assigned_agents_mask = agent_mask;
}

#endif