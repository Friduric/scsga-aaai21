#include "benchmark.h"

/*
	Performs some basic checks that the solution is valid. Throws BenchmarkError
	if these checks failed.
*/
void assert_valid(
	instance_solution& solution,
	problem_t& problem
)
{
	if (solution.ordered_coalition_structure.size() != problem.specification.n_tasks)
	{
		// Error.
		const std::string msg{
			"Solution coalition structure was of incorrect size! Return: "
			+ std::to_string(solution.ordered_coalition_structure.size()) +
			", expected: " + std::to_string(problem.specification.n_tasks)
		};
		throw BenchmarkError(msg);
	}

	// Check if solution's value is valid.
	auto solution_value = solution.value;
	auto real_solution_value = solution.recalculate_value(problem.generator);
	if (fabs(solution_value - real_solution_value) > 0.0001f)
	{
		// Error.
		const std::string msg{
			"Solution value was incorrect! Return: " + std::to_string(solution_value)
			+ ", correct: " + std::to_string(real_solution_value)
		};
		throw BenchmarkError(msg);
	}

	// Check if solution contains all agents at exactly 1 place.
	std::vector<int> agent_count(problem.specification.n_agents, 0);
	for (uint32_t n_agent_index = 0; n_agent_index < problem.specification.n_agents; ++n_agent_index)
	{
		for (uint32_t n_coalition_index = 0; n_coalition_index < problem.specification.n_tasks; ++n_coalition_index)
		{
			if (solution.is_agent_in_coalition(n_agent_index, n_coalition_index))
			{
				agent_count[n_agent_index]++;
			}
		}
	}
	for (uint32_t n_agent_index = 0; n_agent_index < problem.specification.n_agents; ++n_agent_index)
	{
		if (agent_count[n_agent_index] != 1)
		{
			const std::string msg{
				"Solution was incorrect: It was not an ordered coalition structure since agent "
				+ std::to_string(n_agent_index) + " is member of "
				+ std::to_string(agent_count[n_agent_index]) + " coalitions."
			};
			throw BenchmarkError(msg);
		}
	}
}

/*
	Utility function that runs benchmarks using a prepared solver.

	problem 			A specification of the problem to benchmark on. All benchmarks
						are performed on the same problem, i.e. no re-seeding is performed
						between benchmark iterations. As such, successive iterations of
						deterministic algorithms will yield the same result.
	benchmark			A benchmark specification.
	solver				A solver that has been configured with all parameters
						required except time limit, which is set depending on
						the benchmark specification. The same solver is used for all
						benchmarks, i.e. no re-seeding or similar is performed before
						or between benchmarks. The solver is not deallocated by this
						function.

	returns				A vector of benchmark results.
	throws				BenchmarkError if an error occurred during benchmark.
*/
benchmark_result_t run_benchmark_with_solver(
	problem_t& problem,
	const benchmark_specification_t& benchmark,
	solver* solver
)
{
	benchmark_result_t result{};
	problem.allocate();

	using clock_t = std::chrono::high_resolution_clock;
	using time_t = clock_t::time_point;

	std::vector<coalition::value_t> solution_values;

	solver->set_time_limit(benchmark.time_limit_sec);

	for (uint32_t iteration{}; iteration < benchmark.iterations; ++iteration)
	{
		utility::date_and_time::timer timer{};
		timer.start();
		instance_solution solution = solver->solve(problem.generator);
		const float elapsed_time{ timer.stop() };

		solution.recalculate_value(problem.generator);
		assert_valid(solution, problem);

		result.times_taken.push_back(elapsed_time);
		result.solution_values.push_back(solution.value);
	}

	result.calculate_statistics();

	double total_time = 0.0;
	for (auto f : result.times_taken)
	{
		total_time += f;
	}

	double total_value = 0.0;
	for (auto f : solution_values)
	{
		total_value += f;
	}

//	std::cout << "-------------------------------------------" << std::endl;
//	std::cout << "TIME   | average: " << result.time_mean << ", total: " << total_time << ", standard error: " << result.time_standard_error << std::endl;
//	std::cout << "VALUE  | average: " << result.value_mean << ", total: " << total_value << ", standard error: " << result.value_standard_error << std::endl;
//	std::cout << std::endl;

	return result;
}

benchmark_result_t run_mcts_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark,
	const double exploration_weight,
	const double variance_weight,
	const double dnn_weight,
	const bool use_hillclimb
)
{
	solver_mcts* solver{ new solver_mcts() };
	solver->exploration_weight = exploration_weight;
	solver->variance_weight = variance_weight;
	solver->estimation_weight = dnn_weight;
	solver->_RunHillClimbToPolish = use_hillclimb;

	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_agent_greedy_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark,
	const bool run_multiple_times_with_shuffled_agents,
	const bool randomly_assign_agents_first,
	const bool use_hillclimb,
	const int solver_seed
)
{
	solver_agent_greed* solver{ new solver_agent_greed() };
	solver->_RunMultipleTimesWithShuffledAgents = run_multiple_times_with_shuffled_agents;
	solver->_RunHillClimbToPolish = use_hillclimb;
	if (solver_seed < 0) {
		srand(time(NULL));
		solver->_nSeed = rand();
	}
	else {
		solver->_nSeed = solver_seed;
	}

	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_random_search_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark,
	const bool use_hillclimb,
	const int solver_seed
)
{
	solver_pure_random_search* solver{ new solver_pure_random_search() };
	solver->_RunHillClimbToPolish = use_hillclimb;
	if (solver_seed == -1)
	{
		srand(time(NULL));
		solver->seed = rand();
	}
	else
	{
		solver->seed = solver_seed;
	}

	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_mp_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark,
	const bool greedy_init
)
{
	solver* solver{ };
	if (greedy_init)
	{
		solver = new solver_mp_AGI();
	}
	else
	{
		solver = new solver_mp_anytime();
	}
	
	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_brute_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
)
{
	solver_brute_force* solver{ new solver_brute_force() };
	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_task_greedy_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
)
{
	solver_task_greed* solver{ new solver_task_greed() };
	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_dp_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
)
{
	solver_dp* solver{ new solver_dp() };
	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_hybrid_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
)
{
	solver_mp_hybrid* solver{ new solver_mp_hybrid() };
	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_genetic_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
)
{
	solver_genetic* solver{ new solver_genetic() };
	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_mcts_flat_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark
)
{
	solver_mcts_flat* solver{ new solver_mcts_flat() };
	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_annealing_benchmark(
	problem_t& problem,
	const benchmark_specification_t benchmark,
	const bool use_hillclimb
)
{
	solver_annealing* solver{ new solver_annealing() };
	solver->_RunHillClimbToPolish = use_hillclimb;
	benchmark_result_t result{
		run_benchmark_with_solver(problem, benchmark, solver)
	};
	delete solver;
	return result;
}

benchmark_result_t run_greedy_evaluation(
	problem_t& problem,
	const std::vector<std::vector<std::pair<uint32_t, uint32_t>>> partial_agent_task_assignments,
	const float time_limit,
	const bool use_hillclimb,
	const int solver_seed
)
{
	solver_agent_greed* solver{ new solver_agent_greed() };
	problem.allocate();
	solver->_RunMultipleTimesWithShuffledAgents = true;
	solver->_RunHillClimbToPolish = use_hillclimb;
	if (solver_seed < 0) {
		srand(time(NULL));
		solver->_nSeed = rand();
	}
	else {
		solver->_nSeed = solver_seed;
	}
	solver->set_time_limit(time_limit);

	benchmark_result_t result{};
	instance_solution partial_solution{};
	utility::date_and_time::timer timer{};
	for (const auto& partial_assignment : partial_agent_task_assignments)
	{
		// Create partial solution.
		partial_solution.reset(problem.specification.n_tasks, problem.specification.n_agents);
		for (const auto& assignment : partial_assignment)
		{
			const uint32_t agent{ assignment.first };
			const uint32_t task{ assignment.second };

			//assert(agent >= 0 && agent < problem.n_agents);
			//assert(task >= 0 && task < problem.n_tasks);
			//assert(partial_solution.get_coalition_index_of_agent(agent) == -1);

			partial_solution.add_agent_to_coalition(agent, task);
		}

		// Evaluate
		timer.start();
		const coalition::value_t value{ solver->solve(problem.generator, partial_solution).value };
		const float time_taken(timer.stop());

		result.solution_values.push_back(value);
		result.times_taken.push_back(time_taken);
	}

	result.calculate_statistics();

	delete solver;

	return result;
}

benchmark_result_t run_local_evaluation(
	problem_t& problem,
	const std::vector<std::vector<std::pair<uint32_t, uint32_t>>> partial_agent_task_assignments,
	const float time_limit,
	const bool use_hillclimb,
	const int solver_seed
)
{
	solver_pure_random_search* solver{ new solver_pure_random_search() };
	problem.allocate();
	solver->set_time_limit(time_limit);
	solver->_RunHillClimbToPolish = use_hillclimb;
	if (solver_seed < 0) {
		srand(time(NULL));
		solver->seed = rand();
	}
	else {
		solver->seed = solver_seed;
	}
	solver->set_time_limit(time_limit);

	benchmark_result_t result{};
	instance_solution partial_solution{};
	utility::date_and_time::timer timer{};
	for (const auto& partial_assignment : partial_agent_task_assignments)
	{
		// Create partial solution.
		partial_solution.reset(problem.specification.n_tasks, problem.specification.n_agents);
		for (const auto& assignment : partial_assignment)
		{
			const uint32_t agent{ assignment.first };
			const uint32_t task{ assignment.second };

			//assert(agent >= 0 && agent < problem.n_agents);
			//assert(task >= 0 && task < problem.n_tasks);
			//assert(partial_solution.get_coalition_index_of_agent(agent) == -1);

			partial_solution.add_agent_to_coalition(agent, task);
		}
		std::cout << "Converting with " << partial_assignment.size() << " assigned." << std::endl;
		coalitional_values_generator* partial_problem{ partial_solution.convert_to_partial_problem(problem.generator, partial_solution) };

		// Evaluate
		timer.start();
		const coalition::value_t value{ solver->solve(partial_problem).value };
		const float time_taken(timer.stop());
		delete partial_problem;

		result.solution_values.push_back(value);
		result.times_taken.push_back(time_taken);
		
		delete partial_problem;
	}

	result.calculate_statistics();

	delete solver;

	return result;
}