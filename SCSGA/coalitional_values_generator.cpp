#include "coalitional_values_generator.h"

#include <iostream>

coalitional_values_generator::~coalitional_values_generator()
{
	if (generated_values.size() > 50000000)	std::cout << generated_values.size() << " exceeded reserved." << std::endl;
	if (n_agents > MAX_AGENTS_IN_TABLE && seed >= 0)
	{
#ifdef SAVE_PROBLEMS_TO_FILE
		save_generated_values(file_name);
#endif
	}
}

uint32_t coalitional_values_generator::get_n_agents() const
{
	return n_agents;
}

uint32_t coalitional_values_generator::get_n_tasks() const
{
	return n_tasks;
}

uint32_t coalitional_values_generator::get_n_coalitions() const
{
	assert(n_agents <= MAX_AGENTS_IN_TABLE);
	return (1 << n_agents);
}

uint32_t coalitional_values_generator::get_n_value_table_size() const
{
	return generated_values.size();
}

const std::vector<std::vector<coalition::value_t>>& coalitional_values_generator::get_data() const
{
	return task_coalition_value;
}

coalition::value_t coalitional_values_generator::get_value_of(const coalition::coalition_t& coalition, const uint32_t n_task)
{
	if (n_agents <= MAX_AGENTS_IN_TABLE)
	{
		return task_coalition_value[n_task][coalition.get_agent_mask()];
	}
	else
	{
		const auto it{ generated_values.find({coalition, n_task}) };
		if (it == generated_values.end())
		{
			coalition::value_t value{ generate_new_value(coalition, n_task) };
			generated_values[{coalition, n_task}] = value;
			return value;
		}
		else
		{
			return it->second;
		}
	}
}

coalition::value_t coalitional_values_generator::get_value_of(const uint32_t coalition_agent_mask, const uint32_t n_task) const
{
	assert(n_agents <= MAX_AGENTS_IN_TABLE);
	return task_coalition_value[n_task][coalition_agent_mask];
}

void coalitional_values_generator::generate_coalitional_values(unsigned int n_agents, unsigned int n_tasks, int seed)
{
	if (seed == -1 || n_agents != this->n_agents || n_tasks != this->n_tasks || seed != this->seed) {
		// Only regenerate if a new problem is requested.
#ifdef SAVE_PROBLEMS_TO_FILE
		if (seed >= 0 && generated_values.size() > 0)
		{
			save_generated_values(get_file_name());
		}
#endif
		this->n_agents = n_agents;
		this->n_tasks = n_tasks;
		this->seed = seed;
		generated_values.clear();
		task_coalition_value.clear();
		reset(n_agents, n_tasks, seed);
		file_name = "data/" + get_file_name();	// This result must be saved, as it is used in the base class destructor.

		if (n_agents <= MAX_AGENTS_IN_TABLE)
		{
			// Generate coalitional values.
			const uint32_t n_coalitions{ 1U << n_agents };
			coalition::coalition_t coalition(n_agents);
			task_coalition_value.resize(n_tasks);
			for (uint32_t task{}; task < n_tasks; ++task) {
				task_coalition_value[task].resize(n_coalitions);
			}
			for (uint32_t task{}; task < n_tasks; ++task)
			{
				for (uint32_t coalition_mask{}; coalition_mask < n_coalitions; ++coalition_mask)
				{
					coalition.set_value(coalition_mask);
					task_coalition_value[task][coalition_mask] = generate_new_value(coalition, task);
				}
			}
#ifdef SAVE_PROBLEMS_TO_FILE
		} else if (seed >= 0) {
			load_generated_values(get_file_name());
#endif
		} else {
			generated_values.reserve(50000000);
		}
	}
}

void coalitional_values_generator::save_generated_values(const std::string& file_name)
{
	std::filesystem::create_directory(DATA_DIR);
	std::ofstream output{ DATA_DIR + file_name, std::ios::binary };
	output << std::setprecision(10);
	for (const auto it : generated_values)
	{
		const coalition::coalition_t coalition{ it.first.first };
		const uint32_t task{ it.first.second };
		const coalition::value_t value{ it.second };
		const std::vector<uint32_t> agents_in_coalition{ coalition.get_all_agents() };
		const uint32_t n_agents_in_coalition{ static_cast<uint32_t>(agents_in_coalition.size()) };

		output << n_agents_in_coalition;
		for (const uint32_t agent : agents_in_coalition)
		{
			output << " " << agent;
		}
		output << " " << task << " " << value << "\n";
	}
	output.close();
}

void coalitional_values_generator::load_generated_values(const std::string& file_name)
{
	std::ifstream input{ DATA_DIR + file_name };
	while (true)
	{
		uint32_t n_agents_in_coalition, task;
		coalition::value_t value;
		coalition::coalition_t coalition(n_agents);

		input >> n_agents_in_coalition;
		if (!input)
		{
			break;
		}
		for (uint32_t i{}; i < n_agents_in_coalition; ++i)
		{
			uint32_t agent{ };
			input >> agent;
			coalition.add_agent(agent);
		}
		input >> task >> value;
		generated_values[{coalition, task}] = value;
		generate_new_value(coalition, task); // Discard values to restore generator state.
	}
	input.close();
}
