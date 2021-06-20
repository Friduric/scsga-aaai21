#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "coalitional_values_generator.h"
#include "coalitional_values_generator_custom.h"
#include "coalitional_values_generator_file.h"
#include "coalitional_values_generator_MPD.h"
#include "coalitional_values_generator_NDCS.h"
#include "coalitional_values_generator_NPD.h"
#include "coalitional_values_generator_NRD.h"
#include "coalitional_values_generator_NSD.h"
#include "coalitional_values_generator_NSRD.h"
#include "coalitional_values_generator_simple.h"
#include "coalitional_values_generator_sparse.h"
#include "coalitional_values_generator_trap.h"
#include "coalitional_values_generator_UPD.h"

struct problem_specification_t {
	
	enum distribution_t {
		SIMPLE, UPD, NPD, NDCS, NSD, NRD, MPD, NSRD, SUPD, SNPD, trap, EU4
	};

	uint32_t n_agents, n_tasks;
	int seed;
	distribution_t distribution;
	std::string distribution_file_name{ "" };
};

std::string to_string(const problem_specification_t::distribution_t distribution_type);

class problem_t {
public:
	problem_specification_t specification;
	coalitional_values_generator* generator{ nullptr };
	
	problem_t(const problem_specification_t& specification);
	~problem_t();

	problem_t(problem_t&& o);
	problem_t& operator=(problem_t&& o);

	problem_t(problem_t& o) = delete;
	problem_t& operator=(problem_t& o) = delete;

	void allocate();
};