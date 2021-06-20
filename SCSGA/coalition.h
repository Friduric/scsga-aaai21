#pragma once

#define LARGE_COALITIONS

#include <cassert>
#include <unordered_set>
#include <algorithm>
#include <vector>

namespace coalition {

#ifdef LARGE_COALITIONS
	class coalition_t
	{
	public:
		coalition_t();
		explicit coalition_t(const uint32_t n_agents);

		void reset(const uint32_t n_agents);
		void add_agent(const uint32_t agent);
		void remove_agent(const uint32_t agent);

		bool is_agent_in_coalition(const uint32_t agent) const;
		std::vector<uint32_t> get_all_agents() const;
		uint32_t count_agents_in_coalition() const;
		uint32_t get_n_agents() const;

		uint32_t get_agent_mask() const; // Requires n_agents <= 32
		void set_value(const std::vector<uint32_t> agent_bitset);
		void set_value(const uint32_t agent_mask); // Requires n_agents <= 32

		bool operator==(const coalition_t& other) const;

	private:
		uint32_t n_agents;
		std::vector<uint32_t> agent_bitset;
	};
#else
	class coalition_t
	{
	public:
		coalition_t();
		explicit coalition_t(const uint32_t n_agents);

		void reset(const uint32_t n_agents);
		void add_agent(const uint32_t agent);
		void remove_agent(const uint32_t agent);

		bool is_agent_in_coalition(const uint32_t agent) const;
		const std::unordered_set<uint32_t>& get_assigned_agents() const;
		uint32_t count_agents_in_coalition() const;
		uint32_t get_n_agents() const;

		uint32_t get_agent_mask() const; // Requires n_agents <= 32
		void set_value(const std::vector<uint32_t> agent_bitset);
		void set_value(const uint32_t agent_mask); // Requires n_agents <= 32

	private:
		uint32_t n_agents;
		uint32_t assigned_agents_mask;
	};
#endif

	using value_t = float;
	using ordered_coalition_structure_t = std::vector<coalition_t>;
	constexpr value_t NEG_INF = std::numeric_limits<value_t>::lowest();
	constexpr value_t POS_INF = std::numeric_limits<value_t>::max();
	constexpr uint32_t MAX_AGENTS = 32;
}

namespace std {
	template<> struct hash<coalition::coalition_t>
	{
		std::size_t operator()(const coalition::coalition_t& coalition) const noexcept
		{
			std::size_t h{ 5381 };
			std::vector<uint32_t> agents_in_coalition(coalition.get_all_agents());
			for (const uint32_t& agent : agents_in_coalition)
			{
				h = (h << 5) + h + agent;
			}
			return h;
		}
	};
}
