#pragma once

#include <cstdint>
#include <cmath>
#include <iostream>

#include "coalition.h"

class SelectionPolicy
{
public:
	virtual void reset(const uint32_t n_actions) = 0;
	virtual void add_rollout_result(const uint32_t action, const coalition::value_t result) = 0;
	virtual uint32_t get_next_action() = 0;
	virtual uint32_t get_best_action() = 0;
};

class FlatMCTSSelectionPolicy : public SelectionPolicy {
public:
	FlatMCTSSelectionPolicy(const uint32_t n_actions);

	void reset(const uint32_t _n_actions) override;
	void add_rollout_result(const uint32_t action, const coalition::value_t rollout_value) override;
	uint32_t get_next_action() override;
	uint32_t get_best_action() override;

private:
	uint32_t n_actions;
	uint32_t next_action;
	std::vector<coalition::value_t> total_rollout_value;
};

class SPMCTSSelectionPolicy : public SelectionPolicy
{
public:
	static constexpr coalition::value_t CONFIDENCE_CUTOFF = 0.00001f;
	static constexpr coalition::value_t SAFE_MAX = 9999999.0f;

	SPMCTSSelectionPolicy(
		const uint32_t n_actions,
		const float exploration_weight,
		const float variance_weight,
		const float estimation_weight
	);

	void reset(const uint32_t n_actions) override;
	void add_rollout_result(const uint32_t action, const coalition::value_t result) override;
	uint32_t get_next_action(const coalition::value_t value_of_best_solution_found);
	uint32_t get_next_action() override;
	uint32_t get_best_action() override;

	uint32_t get_n_simulations(const uint32_t action);

	void update_estimated_optimal_value
	(
		const uint32_t action,
		const coalition::value_t estimated_value,
		const coalition::value_t confidence
	);

	coalition::value_t get_action_score
	(
		const uint32_t action,
		const coalition::value_t value_of_best_solution_found
	) const;

private:
	uint32_t n_actions;

	std::vector<coalition::value_t> estimated_optimal_value_confidence;
	std::vector<coalition::value_t> estimated_optimal_value;
	std::vector<coalition::value_t> average_squared_result;
	std::vector<coalition::value_t> average_result;
	std::vector<uint32_t> n_used;
	uint32_t total_used;

	float exploration_weight;
	float variance_weight;
	float estimation_weight;
};
