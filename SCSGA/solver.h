#pragma once

#include <string>

#include "instance_solution.h"
#include "coalition.h"

class solver
{
public:
	enum SOLVER_TYPE 
	{ 
		BRUTE_FORCE, 
		DP, 
		MP, MP_WITH_AGENT_GREED_INIT, MP_WITH_TASK_GREED_INIT, 
		GREED_AGENT_BASED, GREED_TASK_BASED, 
		PURE_RANDOM_SEARCH, SWAP_RANDOM_SEARCH,
		HYBRID, 
		GENETIC, 
		MCTS_FLAT,
		SPMCTS,
		SIMULATED_ANNEALING
	};

	virtual instance_solution solve(coalitional_values_generator* coalitional_vales) = 0;

	void set_time_limit(const double vTimeLimit) {
		this->vTimeLimit = vTimeLimit;
	}

	static std::string convert_solver_type_to_string(SOLVER_TYPE solver_type)
	{
		switch (solver_type)
		{
		case solver::SIMULATED_ANNEALING:
			return "SA";
		case solver::BRUTE_FORCE:
			return "BF";
		case solver::DP:
			return "DP";
		case solver::MP:
			return "MP";
		case solver::MP_WITH_AGENT_GREED_INIT:
			return "MP+AG";
		case solver::MP_WITH_TASK_GREED_INIT:
			return "MP+AG";
		case solver::GREED_AGENT_BASED:
			return "AG";
		case solver::GREED_TASK_BASED:
			return "TG";
		case solver::PURE_RANDOM_SEARCH:
			return "PRS";
		case solver::HYBRID:
			return "MPH";
		case solver::GENETIC:
			return "GA";
		case solver::SWAP_RANDOM_SEARCH:
			return "SRS";
		case solver::MCTS_FLAT:
			return "MCTS-F";
		case solver::SPMCTS:
			return "SPMCTS";
		default:
			return "unknown";
		}
	}

protected:
	double vTimeLimit{ -1 };
};

