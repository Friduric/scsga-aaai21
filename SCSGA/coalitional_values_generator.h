#pragma once

#include <random>
#include <cstring>
#include <cassert>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <iomanip>

#include "coalition.h"

namespace std
{
	template<>
	struct hash<std::pair<coalition::coalition_t, uint32_t>>
	{
		std::size_t operator()(const std::pair<coalition::coalition_t, uint32_t>& pair) const noexcept
		{
			const std::size_t coalition_hash{ std::hash<coalition::coalition_t>{}(pair.first) };
			return (coalition_hash << 5) + coalition_hash + pair.second;
		}
	};
}

class coalitional_values_generator
{
public:
	static const uint32_t MAX_AGENTS_IN_TABLE{ 32 };
	static const inline std::string DATA_DIR{ "data/" };

	virtual ~coalitional_values_generator();
	coalitional_values_generator() = default;

	uint32_t get_n_agents() const;
	uint32_t get_n_tasks() const;
	uint32_t get_n_coalitions() const;
	uint32_t get_n_value_table_size() const;

	const std::vector<std::vector<coalition::value_t>>& get_data() const;

	virtual coalition::value_t generate_new_value(const coalition::coalition_t& coalition, const uint32_t n_task) = 0;
	virtual coalition::value_t get_value_of(const coalition::coalition_t& coalition, const uint32_t n_task);
	virtual coalition::value_t get_value_of(const uint32_t coalition_agent_mask, const uint32_t n_task) const;

	// seed == -1 should REUSE previous seed, while seed != -1 should reset seed
	virtual void generate_coalitional_values(unsigned int n_agents, unsigned int n_tasks, int seed = 0);

protected:
	std::vector<std::vector<coalition::value_t>> task_coalition_value;
	std::unordered_map<std::pair<coalition::coalition_t, uint32_t>, coalition::value_t> generated_values;

	uint32_t n_agents{};
	uint32_t n_tasks{};
	int seed{};

	virtual std::string get_file_name() const = 0;
	virtual void reset(const uint32_t n_agents, const uint32_t n_tasks, const int seed = 0) = 0;

private:
	std::string file_name{};
	void save_generated_values(const std::string& file_name);
	void load_generated_values(const std::string& file_name);
};
