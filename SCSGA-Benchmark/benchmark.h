#pragma once

#include <string>
#include <filesystem>
#include <exception>
#include <math.h>

#include "../SCSGA/coalitional_values_generator_simple.h"
#include "../SCSGA/coalitional_values_generator_NDCS.h"
#include "../SCSGA/coalitional_values_generator_NRD.h"
#include "../SCSGA/coalitional_values_generator_NSD.h"
#include "../SCSGA/coalitional_values_generator_NSRD.h"
#include "../SCSGA/coalitional_values_generator_NPD.h"
#include "../SCSGA/coalitional_values_generator_MPD.h"
#include "../SCSGA/coalitional_values_generator_UPD.h"
#include "../SCSGA/coalitional_values_generator_sparse.h"
#include "../SCSGA/coalitional_values_generator_trap.h"
#include "../SCSGA/coalitional_values_generator_custom.h"
#include "../SCSGA/coalitional_values_generator.h"

#include "../SCSGA/solver_brute_force.h"
#include "../SCSGA/solver_annealing.h"
#include "../SCSGA/solver_mp_anytime.h"
#include "../SCSGA/solver_agent_greed.h"
#include "../SCSGA/solver_mp_AGI.h"
#include "../SCSGA/solver_task_greed.h"
#include "../SCSGA/solver_pure_random_search.h"
#include "../SCSGA/solver_swap_random_search.h"
#include "../SCSGA/solver_dp.h"
#include "../SCSGA/solver_mp_hybrid.h"
#include "../SCSGA/solver_genetic.h"
#include "../SCSGA/solver_mcts_flat.h"
#include "../SCSGA/solver_mcts.h"
#include "../SCSGA/solver.h"

#include "../SCSGA/instance_solution.h"
#include "../SCSGA/utility.h"
#include "../SCSGA/problem.h"

struct benchmark_specification_t {
	float time_limit_sec;
	uint32_t iterations;
};

struct benchmark_result_t {
	std::vector<coalition::value_t> solution_values;
	std::vector<float> times_taken;
	coalition::value_t value_mean, value_standard_error, value_variance;
	float time_mean, time_standard_error, time_variance;

	void calculate_statistics()
	{
		value_mean = utility::statistics::calc_mean(solution_values);
		time_mean = utility::statistics::calc_mean(times_taken);
		value_standard_error = utility::statistics::calc_standard_error(solution_values);
		time_standard_error = utility::statistics::calc_standard_error(times_taken);
		value_variance = utility::statistics::calc_variance(solution_values);
		time_variance = utility::statistics::calc_variance(times_taken);
	}
};

class BenchmarkError : public std::exception
{
public:
	BenchmarkError(const std::string& message)
		: message{ message }
	{

	}

	const char* what() const noexcept override
	{
		return message.c_str();
	}

private:
	const std::string message;
};

/*
	Runs a set of benchmarks using the SP-MCTS solver.

	problem				The problem to benchmark on. All benchmarks
						are performed on the same problem, i.e. no re-seeding is performed
						between benchmark iterations.
	benchmark			A benchmark specification.
	exploration_weight	The weight for the second term in the UCT equation. For more
						information, see the c constant in section 4.3 of the IJCAI
						article.
	variance_weight		The weight for the third term in the UCT equation. For more
						information, see the d constant in section 4.3 of the IJCAI
						article.
	dnn_weight			The weight to use for the node value estimated by the DNN in
						the modified UCT equation.
	use_hillclimb		True if hillclimb should be used to polish solutions found
						by MCTS.

	returns				A vector of benchmark results.
	throws				BenchmarkError if an error occured during the benchmark.
*/
benchmark_result_t run_mcts_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark,
	const double exploration_weight = -0.18,
	const double variance_weight = 0.33,
	const double dnn_weight = 1.0,
	const bool use_hillclimb = false
);

/*
	Runs a set of benchmarks using the agent greedy solver.

	problem				The problem to benchmark on. All benchmarks
						are performed on the same problem, i.e. no re-seeding is performed
						between benchmark iterations. As such, successive iterations
						without any of the non-deterministic options will yield the
						same result.
	benchmark			A benchmark specification.
	shuffle_agents		True if the agent permutation should be shuffled between
						successive runs of the solver.
	random_start		True if random start assignment should be generated before
						trying to move each agent between tasks.
	use_hillclimb		True if hill climb should be used to polish the solutions generated
						by a purely greedy run.
	solver_seed			Seed for the solver. Set to -1 to generate a random seed based on
						current system time.

	returns				A vector of benchmark results.
	throws				BenchmarkError if an error occured during the benchmark.
*/
benchmark_result_t run_agent_greedy_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark,
	const bool shuffle_agents = true,
	const bool random_start = false,
	const bool use_hillclimb = false,
	const int solver_seed = -1
);

/*
	Runs a set of benchmarks using random search with hillclimb.

	problem				The problem to benchmark on. All benchmarks
						are performed on the same problem, i.e. no re-seeding is performed
						between benchmark iterations.
	benchmark			A benchmark specification.
	solver_seed			Seed for the solver. Set to -1 to generate a random seed based on
						current system time.

	returns				A vector of benchmark results.
	throws				BenchmarkError if an error occured during the benchmark.
*/
benchmark_result_t run_random_search_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark,
	const bool use_hillclimb = false,
	const int solver_seed = -1
);

benchmark_result_t run_mp_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark,
	const bool greedy_init = false
);
benchmark_result_t run_annealing_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark,
	const bool use_hillclimb = false
);
benchmark_result_t run_brute_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
);
benchmark_result_t run_task_greedy_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
);
benchmark_result_t run_dp_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
);
benchmark_result_t run_hybrid_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
);
benchmark_result_t run_genetic_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
);
benchmark_result_t run_mcts_flat_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
);

/*
	Evaluates a set of partial solutions using the agent greedy solver.

	problem							The problem to benchmark on. All partial
									assignments are assumed to belong to this problem.
	partial_agent_task_assignments	A set of partial assignments. Each partial assignment consists
									of a vector of agent-task pairs for a subset of the agents.
	time_limit						The time the greedy solver may use for each partial assignment.
									If -1, a single pass is performed.
	use_hillclimb					True if hill climb should be used to polish the solutions generated
									by a purely greedy run.
	solver_seed						Seed for the solver. Set to -1 to generate a random seed based on
									current system time.

	return 							A vector of benchmark results.
	throws							BenchmarkError if an error occured during the benchmark.
*/
benchmark_result_t run_greedy_evaluation(
	problem_t& problem,
	const std::vector<std::vector<std::pair<uint32_t, uint32_t>>> partial_agent_task_assignments,
	const float time_limit = -1,
	const bool use_hillclimb = true,
	const int solver_seed = -1
);

benchmark_result_t run_local_evaluation(
	problem_t& problem,
	const std::vector<std::vector<std::pair<uint32_t, uint32_t>>> partial_agent_task_assignments,
	const float time_limit = -1,
	const bool use_hillclimb = true,
	const int solver_seed = -1
);