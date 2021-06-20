#include "solver_brute_force.h"

void solver_brute_force::solve_brute_force_recursive(const uint32_t n_agent_index, instance_solution& current_solution)
{
	if (n_agent_index == problem->get_n_agents())
	{
		// Calculate the value of this solution.
		current_solution.recalculate_value(problem);

		// Check if this solution is the best one found so far.
		if (current_solution.value > best_solution.value)
		{
			best_solution = current_solution;
		}

		return;
	}

	// Try to assign this agent to all coalitions.
	for (uint32_t n_task_index = 0; n_task_index < problem->get_n_tasks(); ++n_task_index)
	{
		current_solution.add_agent_to_coalition(n_agent_index, n_task_index);
		solve_brute_force_recursive(n_agent_index + 1, current_solution);
		current_solution.remove_agent_from_coalition(n_agent_index, n_task_index);
	}
}

instance_solution solver_brute_force::solve(coalitional_values_generator* _problem)
{
	assert(_problem->get_n_agents() <= 32);
	problem = _problem;
	best_solution.value = coalition::NEG_INF;
	instance_solution temp_solution;
	temp_solution.reset(_problem->get_n_tasks(), _problem->get_n_agents());
	solve_brute_force_recursive(0, temp_solution);
	return best_solution;
}
