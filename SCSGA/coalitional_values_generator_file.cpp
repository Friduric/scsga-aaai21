#include "coalitional_values_generator_file.h"

coalitional_values_generator_file::coalitional_values_generator_file(const std::string& file_name)
    : coalitional_values_generator(), file_name{ file_name }
{
    std::ifstream file{ file_name };
    file >> n_agents >> n_tasks;
    if (!file.good())
    {
        throw std::invalid_argument("Distribution file not found: " + file_name);
    }

    for (uint32_t task{}; task < n_tasks; ++task)
    {
        for (uint32_t coalition{}; coalition < get_n_coalitions(); ++coalition)
        {
            file >> task_coalition_value[task][coalition];
        }
    }
    if (!file.good())
    {
        throw std::runtime_error("Invalid format of distribution file " + file_name);
    }
}

coalition::value_t coalitional_values_generator_file::generate_new_value(const coalition::coalition_t& coalition, const uint32_t n_task)
{
    return 0;
}

void coalitional_values_generator_file::generate_coalitional_values(unsigned int n_agents, unsigned int n_tasks, int seed)
{
    // New values cannot be generated, as they are read directly from file.
    if (n_agents != this->n_agents || n_tasks != this->n_tasks)
    {
        throw std::invalid_argument("Cannot change problem size of file generator.");
    }    
}

std::string coalitional_values_generator_file::get_file_name() const
{
    return "file_" + file_name + ".problem";
}

void coalitional_values_generator_file::reset(const uint32_t n_agents, const uint32_t n_tasks, const int seed)
{
    if (n_agents != this->n_agents || n_tasks != this->n_tasks)
    {
        throw std::invalid_argument("Cannot change problem size of file generator.");
    }
}