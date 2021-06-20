#include "problem.h"

std::string to_string(const problem_specification_t::distribution_t distribution_type)
{
	switch (distribution_type)
	{
	case problem_specification_t::distribution_t::SIMPLE:
		return "simple";
	case problem_specification_t::distribution_t::UPD:
		return "UPD";
	case problem_specification_t::distribution_t::NPD:		
		return "NPD";
	case problem_specification_t::distribution_t::NDCS:
		return "NDCS";
	case problem_specification_t::distribution_t::NSD:		
		return "NSD";
	case problem_specification_t::distribution_t::NRD:		
		return "NRD";
	case problem_specification_t::distribution_t::MPD:		
		return "MPD";
	case problem_specification_t::distribution_t::NSRD:		
		return "NSRD";
	case problem_specification_t::distribution_t::SUPD:		
		return "SUPD";
	case problem_specification_t::distribution_t::SNPD:		
		return "SNPD";
	case problem_specification_t::distribution_t::trap:		
		return "trap";
	case problem_specification_t::distribution_t::EU4:
		return "EU4";
	default:
		throw std::invalid_argument("Invalid distribution type: " + distribution_type);
	}
}

problem_t::problem_t(const problem_specification_t& specification)
	: specification{ specification }
{
	switch (specification.distribution)
	{
	case problem_specification_t::distribution_t::SIMPLE:
		generator = new coalitional_values_generator_simple();
		break;
	case problem_specification_t::distribution_t::UPD:
		generator = new coalitional_values_generator_UPD();
		break;
	case problem_specification_t::distribution_t::NPD:
		generator = new coalitional_values_generator_NPD();
		break;
	case problem_specification_t::distribution_t::NDCS:
		generator = new coalitional_values_generator_NDCS();
		break;
	case problem_specification_t::distribution_t::NSD:
		generator = new coalitional_values_generator_NSD();
		break;
	case problem_specification_t::distribution_t::NRD:
		generator = new coalitional_values_generator_NRD();
		break;
	case problem_specification_t::distribution_t::MPD:
		generator = new coalitional_values_generator_MPD();
		break;
	case problem_specification_t::distribution_t::NSRD:
		generator = new coalitional_values_generator_NSRD();
		break;
	case problem_specification_t::distribution_t::SUPD:
		generator = new coalitional_values_generator_sparse();
		dynamic_cast<coalitional_values_generator_sparse*>(generator)->use_uniform = true;
		break;
	case problem_specification_t::distribution_t::SNPD:
		generator = new coalitional_values_generator_sparse();
		dynamic_cast<coalitional_values_generator_sparse*>(generator)->use_uniform = false;
		break;
	case problem_specification_t::distribution_t::trap:
		generator = new coalitional_values_generator_trap();
		break;
	case problem_specification_t::distribution_t::EU4:
		generator = new coalitional_values_generator_file(specification.distribution_file_name);
		break;
	}
}

problem_t::~problem_t()
{
	delete generator;
}

problem_t::problem_t(problem_t&& o)
{
	std::swap(generator, o.generator);
}

problem_t& problem_t::operator=(problem_t&& o)
{
	std::swap(generator, o.generator);
	return *this;
}

void problem_t::allocate()
{
	generator->generate_coalitional_values(specification.n_agents, specification.n_tasks, specification.seed);
}