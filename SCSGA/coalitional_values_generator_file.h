#pragma once

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "coalitional_values_generator.h"

class coalitional_values_generator_file : public coalitional_values_generator {
public:
    coalitional_values_generator_file(const std::string& file_name);

    coalition::value_t generate_new_value(const coalition::coalition_t& coalition, const uint32_t n_task) override;
    void generate_coalitional_values(unsigned int n_agents, unsigned int n_tasks, int seed = 0) override;

protected:
    std::string get_file_name() const override;
    void reset(const uint32_t n_agents, const uint32_t n_tasks, const int seed = 0) override;

private:
    std::string file_name;
};