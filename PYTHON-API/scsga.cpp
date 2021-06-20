#include "scsga.h"

namespace py = pybind11;

using namespace py::literals;

PYBIND11_MODULE(scsga, m) {
    m.doc() = "Python API for MCTS training data generation and benchmarking";

    py::class_<problem_specification_t> prob_spec(m, "problem_specification_t");
    prob_spec.def(py::init<uint32_t, uint32_t, int, problem_specification_t::distribution_t>())
        .def_readwrite("n_agents", &problem_specification_t::n_agents)
        .def_readwrite("n_tasks", &problem_specification_t::n_tasks)
        .def_readwrite("seed", &problem_specification_t::seed)
        .def_readwrite("distribution", &problem_specification_t::distribution)
        .def_readwrite("distribution_file_name", &problem_specification_t::distribution_file_name)
        .def(py::pickle(
            [](const problem_specification_t& spec) {
                return py::make_tuple(spec.n_agents, spec.n_tasks, spec.seed, spec.distribution, spec.distribution_file_name);
            },
            [](py::tuple t) {
                if (t.size() != 5) {
                    throw std::runtime_error("Invalid state");
                }
                problem_specification_t spec{};
                spec.n_agents = t[0].cast<uint32_t>();
                spec.n_tasks = t[1].cast<uint32_t>();
                spec.seed = t[2].cast<int>();
                spec.distribution = t[3].cast<problem_specification_t::distribution_t>();
                spec.distribution_file_name = t[4].cast<std::string>();
                return spec;
            }
        ));
    py::enum_<problem_specification_t::distribution_t>(prob_spec, "distribution_t")
        .value("EU4", problem_specification_t::distribution_t::EU4)
        .value("MPD", problem_specification_t::distribution_t::MPD)
        .value("NDCS", problem_specification_t::distribution_t::NDCS)
        .value("NPD", problem_specification_t::distribution_t::NPD)
        .value("NRD", problem_specification_t::distribution_t::NRD)
        .value("NSD", problem_specification_t::distribution_t::NSD)
        .value("NSRD", problem_specification_t::distribution_t::NSRD)
        .value("SIMPLE", problem_specification_t::distribution_t::SIMPLE)
        .value("SNPD", problem_specification_t::distribution_t::SNPD)
        .value("SUPD", problem_specification_t::distribution_t::SUPD)
        .value("TRAP", problem_specification_t::distribution_t::trap)
        .value("UPD", problem_specification_t::distribution_t::UPD);

    py::class_<problem_t> prob(m, "problem_t");
    prob.def(py::init<problem_specification_t>());

    py::class_<benchmark_specification_t>(m, "benchmark_specification_t")
        .def(py::init<float, uint32_t>())
        .def_readwrite("time_limit_sec", &benchmark_specification_t::time_limit_sec)
        .def_readwrite("iterations", &benchmark_specification_t::iterations)
        .def(py::pickle(
            [](const benchmark_specification_t& spec) {
                return py::make_tuple(spec.time_limit_sec, spec.iterations);
            },
            [](py::tuple t) {
                if (t.size() != 2) {
                    throw std::runtime_error("Invalid state.");
                }
                benchmark_specification_t spec{};
                spec.time_limit_sec = t[0].cast<float>();
                spec.iterations = t[1].cast<uint32_t>();
                return spec;
            }
        ));

    py::class_<benchmark_result_t>(m, "benchmark_result_t")
        .def_readwrite("solution_values", &benchmark_result_t::solution_values)
        .def_readwrite("times_taken", &benchmark_result_t::times_taken)
        .def_readwrite("value_mean", &benchmark_result_t::value_mean)
        .def_readwrite("value_standard_error", &benchmark_result_t::value_standard_error)
        .def_readwrite("value_variance", &benchmark_result_t::value_variance)
        .def_readwrite("time_mean", &benchmark_result_t::time_mean)
        .def_readwrite("time_standard_error", &benchmark_result_t::time_standard_error)
        .def_readwrite("time_variance", &benchmark_result_t::time_variance);

    py::register_exception<BenchmarkError>(m, "BenchmarkError");

    m.def("run_mcts_benchmark", &run_mcts_benchmark, "problem"_a, "benchmark"_a,
        "exploration_weight"_a = -0.18, "variance_weight"_a = 0.33, "dnn_weight"_a = 1.0,
        "use_hillclimb"_a = false);
    m.def("run_agent_greedy_benchmark", &run_agent_greedy_benchmark,
        "problem"_a, "benchmark"_a,
        "shuffle_agents"_a = true, "random_start"_a = false,
        "use_hillclimb"_a = false, "solver_seed"_a = -1);
    m.def("run_random_search_benchmark", &run_random_search_benchmark,
        "problem"_a, "benchmark"_a, "use_hillclimb"_a = false, "solver_seed"_a = -1);
    m.def("run_greedy_evaluation", &run_greedy_evaluation, "problem"_a, "partial_agent_assignment"_a,
        "time_limit"_a = -1, "use_hillclimb"_a = true, "solver_seed"_a = -1);
    m.def("run_local_evaluation", &run_local_evaluation, "problem"_a, "partial_agent_assignment"_a,
        "time_limit"_a = -1, "use_hillclimb"_a = true, "solver_seed"_a = -1);
    
    m.def("run_mp_benchmark", &run_mp_benchmark, "problem"_a, "benchmark"_a, "greedy_init"_a = false);
    m.def("run_annealing_benchmark", &run_annealing_benchmark, "problem"_a, "benchmark"_a, "use_hillclimb"_a = false);
    m.def("run_brute_benchmark", &run_brute_benchmark, "problem"_a, "benchmark"_a);
    m.def("run_task_greedy_benchmark", &run_task_greedy_benchmark, "problem"_a, "benchmark"_a);
    m.def("run_dp_benchmark", &run_dp_benchmark, "problem"_a, "benchmark"_a);
    m.def("run_hybrid_benchmark", &run_hybrid_benchmark, "problem"_a, "benchmark"_a);
    m.def("run_genetic_benchmark", &run_genetic_benchmark, "problem"_a, "benchmark"_a);
    m.def("run_mcts_flat_benchmark", &run_mcts_flat_benchmark, "problem"_a, "benchmark"_a);
}