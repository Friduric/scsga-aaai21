#include "selection_policy.h"

FlatMCTSSelectionPolicy::FlatMCTSSelectionPolicy(const uint32_t n_actions)
	: n_actions{ n_actions }, next_action{}, total_rollout_value(n_actions, 0)
{  }

void FlatMCTSSelectionPolicy::reset(const uint32_t _n_actions)
{
	n_actions = _n_actions;
	total_rollout_value = std::vector<coalition::value_t>(_n_actions, 0);
}

void FlatMCTSSelectionPolicy::add_rollout_result(const uint32_t action, const coalition::value_t rollout_value)
{
	total_rollout_value[action] += rollout_value;
}

uint32_t FlatMCTSSelectionPolicy::get_next_action()
{
	if (next_action == n_actions)
	{
		next_action = 0;
	}
	return next_action++;
}

uint32_t FlatMCTSSelectionPolicy::get_best_action()
{
	uint32_t best_action_index{ 0 };
	coalition::value_t best_action_value{ total_rollout_value[0] };
	for (uint32_t i{ 1 }; i < n_actions; ++i)
	{
		if (total_rollout_value[i] > best_action_value)
		{
			best_action_index = i;
			best_action_value = total_rollout_value[i];
		}
	}
	return best_action_index;
}

SPMCTSSelectionPolicy::SPMCTSSelectionPolicy(
	const uint32_t n_actions,
	const float exploration_weight,
	const float variance_weight,
	const float estimation_weight
) :
	n_actions{ n_actions },
	exploration_weight{ exploration_weight },
	variance_weight{ variance_weight },
	estimation_weight{ estimation_weight },
	average_squared_result(n_actions, 0),
	average_result(n_actions, 0),
	n_used(n_actions, 0),
	total_used{ 0 },
	estimated_optimal_value(n_actions, SAFE_MAX),
	estimated_optimal_value_confidence(n_actions, 0.0f)
{

}

void SPMCTSSelectionPolicy::reset(const uint32_t n_actions)
{
	average_squared_result.assign(n_actions, 0);
	average_result.assign(n_actions, 0);
	n_used.assign(n_actions, 0);
	estimated_optimal_value.assign(n_actions, 99999999.0f);
	estimated_optimal_value_confidence.assign(n_actions, 0);
	total_used = 0;
}

void SPMCTSSelectionPolicy::update_estimated_optimal_value
(
	const uint32_t action,
	const coalition::value_t estimated_value,
	const coalition::value_t confidence
)
{
	estimated_optimal_value[action] = estimated_value;
	estimated_optimal_value_confidence[action] = confidence;
}

void SPMCTSSelectionPolicy::add_rollout_result(const uint32_t action, const coalition::value_t result)
{
	const coalition::value_t contribution{ coalition::value_t(result / (n_used[action] + 1.0)) };
	average_result[action] *= coalition::value_t(n_used[action] / (n_used[action] + 1.0));
	average_result[action] += contribution;

	const coalition::value_t squared_contribution{ result * contribution };
	average_squared_result[action] *= coalition::value_t(n_used[action] / (n_used[action] + 1.0));
	average_squared_result[action] += squared_contribution;

	++n_used[action];
	++total_used;
}

uint32_t SPMCTSSelectionPolicy::get_next_action(const coalition::value_t value_of_best_solution_found)
{
	uint32_t best_candidate{ 0 };
	coalition::value_t best_score{ get_action_score(0, value_of_best_solution_found) };
	for (uint32_t i{ 1 }; i < n_actions; ++i)
	{
		const coalition::value_t current_score{ get_action_score(i, value_of_best_solution_found) };
		if (current_score > best_score)
		{
			best_score = current_score;
			best_candidate = i;
		}
	}
	return best_candidate;
}

uint32_t SPMCTSSelectionPolicy::get_next_action()
{
	return get_next_action(1.414f);
}

uint32_t SPMCTSSelectionPolicy::get_best_action()
{
	uint32_t best_action_index{ 0 };
	uint32_t best_action_value{ n_used[0] };
	for (uint32_t i{ 1 }; i < n_actions; ++i)
	{
		if (n_used[i] > best_action_value)
		{
			best_action_index = i;
			best_action_value = n_used[i];
		}
	}
	return best_action_index;
}

uint32_t SPMCTSSelectionPolicy::get_n_simulations(const uint32_t action)
{
	return n_used[action];
}

coalition::value_t SPMCTSSelectionPolicy::get_action_score
(
	const uint32_t action,
	const coalition::value_t value_of_best_solution_found
)
const
{
	if (n_used[action] == 0)
	{
		if (estimated_optimal_value_confidence[action] < CONFIDENCE_CUTOFF)
		{
			return SAFE_MAX;
		}
		else
		{
			return estimated_optimal_value[action] * estimated_optimal_value[action];
		}
	}

	const coalition::value_t normalized_average{ average_result[action] / value_of_best_solution_found };

	// Exploration. 
	const coalition::value_t exploration_term =
		exploration_weight * sqrt(log(total_used) / n_used[action]);

	// Variance.
	const coalition::value_t normalized_variance =
		average_squared_result[action] / (value_of_best_solution_found * value_of_best_solution_found) -
		normalized_average * normalized_average;

	const coalition::value_t variance_term =
		sqrt(normalized_variance + variance_weight / n_used[action]);

	// Estimated.
	const coalition::value_t estimation_term =
		estimation_weight * estimated_optimal_value_confidence[action] * estimated_optimal_value[action] /
		value_of_best_solution_found; // *coalition::value_t(sqrt(log(total_used) / n_used[action]));

	if (false)
	{
		std::cout
			<< "  | Runs:      " << n_used[action]
			<< "  | E (estim): " << estimation_term
			<< "  | A (avera): " << normalized_average
			<< "  | C (explo): " << exploration_term
			<< "  | D (varia): " << variance_term << std::endl;
	}

	return
		estimation_term +
		normalized_average +
		exploration_term +
		variance_term;
}