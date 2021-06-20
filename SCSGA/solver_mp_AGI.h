#pragma once
#include "solver.h"

#include "solver_mp_anytime.h"
#include "solver_agent_greed.h"
#include "solver_task_greed.h"

class solver_mp_AGI : public solver
{
public:
	bool bUseAGIForSubspaceLowerBounds = true;

	instance_solution solve(coalitional_values_generator* problem) override;
};