#include "coalitional_values_generator_custom.h"

coalitional_values_generator_custom::coalitional_values_generator_custom(
	const uint32_t n_agents,
	const uint32_t n_tasks
)
{
	assert(n_agents <= 32);
	this->n_agents = n_agents;
	this->n_tasks = n_tasks;

	const uint32_t n_coalitions{ 1U << n_agents };
	task_coalition_value.resize(n_tasks);
	for (auto& task_values : task_coalition_value) {
		task_values.resize(n_coalitions);
		std::fill(task_values.begin(), task_values.end(), 0.0);
	}
}

void coalitional_values_generator_custom::set_value_of(
	const uint32_t coalition_agent_mask, 
	const uint32_t n_task, 
	const coalition::value_t v_value
)
{
	task_coalition_value[n_task][coalition_agent_mask] = v_value;
}

void coalitional_values_generator_custom::reset(
	const uint32_t n_agents,
	const uint32_t n_tasks,
	int seed
)
{
	assert(n_agents <= 32);
	this->n_agents = n_agents;
	this->n_tasks = n_tasks;
	this->seed = seed;

	task_coalition_value.resize(n_tasks);
	for (uint32_t task{}; task < n_tasks; ++task)
	{
		task_coalition_value[task].assign(get_n_coalitions(), 0);
	}
}

void coalitional_values_generator_custom::generate_coalitional_values(const uint32_t n_agents, const uint32_t n_tasks, int seed)
{
	reset(n_agents, n_tasks, seed);
}

coalition::value_t coalitional_values_generator_custom::generate_new_value(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	return 0;
}

std::string coalitional_values_generator_custom::get_file_name() const
{
	return "custom_" + std::to_string(seed) + "_" + std::to_string(n_agents) + "_" + std::to_string(n_tasks) + ".problem";
}