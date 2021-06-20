#!/usr/bin/env python3

import argparse
import os
import time
import random
import itertools
import multiprocessing
import logging

import numpy
import matplotlib.pyplot as plt

from experiment_description import ExperimentDescription

import sys
# TODO: What's the path on Windows? Append both or have an if-statement
sys.path.append('../build/PYTHON-API')

import scsga
from scsga import problem_t as Problem
from scsga import problem_specification_t as ProblemSpecifiation
from scsga import benchmark_specification_t as BenchmarkSpecification

import tqdm

logger = logging.getLogger("benchmark_logger")
logger.setLevel(logging.DEBUG)

DISTRIBUTIONS = ['upd', 'npd', 'supd', 'snpd']
TYPES = ['time_limit', 'agent', 'many', 'large']

def assert_in(desc, path, valid_values):
    """Raise an exception if the value at path is not a valid value."""
    value = desc.get(path).lower()
    if not value in valid_values:
        valid_values_string = ", ".join(valid_values)
        msg = f"Invalid value found at {path}: {value}. Must be one of {valid_values_string}"
        raise Exception(msg)

def std_err(seq):
    if len(seq) <= 1:
        return 0
    else:
        return numpy.std(seq) / numpy.sqrt(len(seq))

def get_unique_name(desc):
    timestr = time.strftime("%y%m%d-%H%M%S")
    experiment_name = desc.get('name')
    unique_name = timestr + "-" + experiment_name.replace(' ', '-')
    return unique_name

def get_distribution(desc):
    valid_distributions = ["eu4", "mpd", "ndcs", "npd", "nrd", "nsd", "nsrd", "simple", "snpd", "supd", "trap", "upd"]
    assert_in(desc, "distribution", valid_distributions)
    name = desc.get("distribution").lower()
    if name == "eu4":
        return ProblemSpecifiation.distribution_t.EU4
    elif name == "mpd":
        return ProblemSpecifiation.distribution_t.MPD
    elif name == "ndcs":
        return ProblemSpecifiation.distribution_t.NDCS
    elif name == "npd":
        return ProblemSpecifiation.distribution_t.NPD
    elif name == "nrd":
        return ProblemSpecifiation.distribution_t.NRD
    elif name == "nsd":
        return ProblemSpecifiation.distribution_t.NSD
    elif name == "nsrd":
        return ProblemSpecifiation.distribution_t.NSRD
    elif name == "simple":
        return ProblemSpecifiation.distribution_t.SIMPLE
    elif name == "snpd":
        return ProblemSpecifiation.distribution_t.SNPD
    elif name == "supd":
        return ProblemSpecifiation.distribution_t.SUPD
    elif name == "trap":
        return ProblemSpecifiation.distribution_t.TRAP
    elif name == "upd":
        return ProblemSpecifiation.distribution_t.UPD

def ensure_directory_exists(directory_name):
    """Creates a directory if it doesn't already exist."""
    if not os.path.exists(directory_name):
        os.mkdir(directory_name)

def setup_benchmark_function(desc, prefix):
    valid_algorithms = ["brute_force", "mp", "agent_greedy", "task_greedy", "pure_random_search", "dp", "hybrid", "genetic", "mcts_flat", "spmcts", "annealing"]
    assert_in(desc, prefix + "name", valid_algorithms)
    name = desc.get(prefix + "name").lower()
    if name == "brute_force":
        function = scsga.run_brute_benchmark
    elif name == "mp":
        greedy_init = desc.get(prefix + "greedy_init", False)
        function = lambda problem, benchmark : scsga.run_mp_benchmark(problem, benchmark, greedy_init)
    elif name == "agent_greedy":
        shuffle_agents = desc.get(prefix + "shuffle_agents", True)
        random_start = desc.get(prefix + "random_start", False)
        use_hillclimb = desc.get(prefix + "use_hillclimb", False)
        solver_seed = desc.get(prefix + "solver_seed", -1)
        function = lambda problem, benchmark : scsga.run_agent_greedy_benchmark(problem, benchmark, shuffle_agents, random_start, use_hillclimb, solver_seed)
    elif name == "task_greedy":
        function = scsga.run_task_greedy_benchmark
    elif name == "pure_random_search":
        use_hillclimb = desc.get(prefix + "use_hillclimb", False)
        solver_seed = desc.get(prefix + "solver_seed", -1)
        function = lambda problem, benchmark : scsga.run_random_search_benchmark(problem, benchmark, use_hillclimb, solver_seed)
    elif name == "dp":
        function = scsga.run_dp_benchmark
    elif name == "hybrid":
        function = scsga.run_hybrid_benchmark
    elif name == "genetic":
        function = scsga.run_genetic_benchmark
    elif name == "mcts_flat":
        function = scsga.run_mcts_flat_benchmark
    elif name == "spmcts":
        exploration_weight = desc.get(prefix + "exploration_weight", -0.18)
        variance_weight = desc.get(prefix + "variance_weight", 0.33)
        dnn_weight = desc.get(prefix + "dnn_weight", 1.0)
        use_hillclimb = desc.get(prefix + "use_hillclimb", False)
        function = lambda problem, benchmark : scsga.run_mcts_benchmark(problem, benchmark, exploration_weight, variance_weight, dnn_weight, use_hillclimb)
    elif name == "annealing":
        function = scsga.run_annealing_benchmark
        
    return function

def run(data):
    problem_specification, benchmark, algorithm_index, desc = data
    function = setup_benchmark_function(desc, f"algorithms/{algorithm_index}/")
    problem = Problem(problem_specification)
    display_name = desc.get(f'algorithms/{algorithm_index}/display_name')
    
    msg = "Solving n = {0:4d}, m = {1:4d}, seed = {2} using algorithm {3:>5}." \
        .format(problem_specification.n_agents, problem_specification.n_tasks, problem_specification.seed,
                display_name)
    logger.info(msg)

    result = function(problem, benchmark)
    
    msg = "Solved n = {0:4d}, m = {1:4d}, seed = {2} using algorithm {3:>5}. Got result {4:5.2f} after {5:6.0f} ms." \
        .format(problem_specification.n_agents, problem_specification.n_tasks, problem_specification.seed,
                display_name, result.solution_values[0], result.times_taken[0]*1000)
    logger.info(msg)

    del problem # They get quite large, delete ASAP

    return (result.solution_values[0], result.times_taken[0])

def calculate_optimum(problem_specification):
    problem = Problem(problem_specification)
    benchmark = BenchmarkSpecification(-1, 1)
    result = scsga.run_hybrid_benchmark(problem, benchmark)

    msg = "Optimum for n = {0:4d}, m = {1:4d}, seed = {2} is {3:5.2f} (took {4:6.0f} ms)." \
        .format(problem_specification.n_agents, problem_specification.n_tasks, problem_specification.seed,
            result.solution_values[0], result.times_taken[0]*1000)
    logger.info(msg)

    return result.solution_values[0]

def run_benchmarks(desc):

    # Setup problem specifications
    problems = []
    distribution = get_distribution(desc)
    if desc.get("problem_file_names", []):
        for file_name in desc.get("problem_file_names"):
            specification = ProblemSpecifiation(0, 0, 0, distribution)
            specification.distribution_file_name = file_name
            problems.append(specification)
    else:
        for n_agents in desc.get("n_agents"):
            for n_tasks in desc.get("n_tasks"):
                start_seed = desc.get("start_seed")
                for iteration in range(desc.get("iterations")):
                    if start_seed == -1:
                        seed = random.randint(0, (1 << 31) - 1)
                    else:
                        seed = start_seed + iteration
                    specification = ProblemSpecifiation(n_agents, n_tasks, seed, distribution)
                    specification.distribution_file_name = ""
                    problems.append(specification)

    # Setup benchmarks
    benchmarks = [BenchmarkSpecification(time_limit, 1) for time_limit in desc.get("time_limits")]

    # Establish algorithm parameters
    n_algorithms = len(desc.get("algorithms"))
    for i in range(n_algorithms):
        setup_benchmark_function(desc, f"algorithms/{i}/")

    # Run benchmarks using all available threads
    calc_optimum = desc.get("present/compare_to_optimum", False)
    n_threads = desc.get("n_threads", 1)
    with multiprocessing.Pool(n_threads) as pool:
        data = list(itertools.product(problems, benchmarks, range(n_algorithms), [desc]))
        with tqdm.tqdm(iterable=pool.imap(run, data), total=len(data), position=1, leave=False) as pbar:
            pbar.set_description('{0:<30}'.format('Evaluating algorithms'), refresh=True)
            results = list(pbar)
        if calc_optimum:
            with tqdm.tqdm(iterable=pool.imap(calculate_optimum, problems), total=len(problems), position=1, leave=False) as pbar:
                pbar.set_description('{0:<30}'.format('Calculating optimum'), refresh=True)
                optimum = list(pbar)

    # Save results using proper format
    desc.set("results", [])
    for problem_index, problem in enumerate(problems):
        problem_entry = {
            "n_agents" : problem.n_agents,
            "n_tasks" : problem.n_tasks,
            "seed" : problem.seed,
            "distribution_file_name" : problem.distribution_file_name,
            "benchmarks" : []
        }
        if calc_optimum:
            problem_entry["optimum"] = optimum[problem_index]
        for benchmark_index, benchmark in enumerate(benchmarks):
            for algorithm_index in range(n_algorithms):
                result_index = algorithm_index + n_algorithms * benchmark_index + n_algorithms * len(benchmarks) * problem_index
                result_entry = {
                    "algorithm" : algorithm_index,
                    "time_limit" : benchmark.time_limit_sec,
                    "value" : results[result_index][0],
                    "time_taken" : results[result_index][1]
                }
                problem_entry["benchmarks"].append(result_entry)
        desc.get("results").append(problem_entry)

def print_latex_file(desc, values, names):
    ensure_directory_exists("results")
    benchmark_name = desc.get('name')
    for name, serie in zip(names, values):
        latex_file_name = f'results/{benchmark_name}_{name}.txt'
        with open(latex_file_name, 'w') as f:
            x_axis_value = desc.get("present/x_axis")
            if x_axis_value == "n_agents":
                format_string = "({0}, {1}) +- ({2}, {3})\n"
            else:
                format_string = "({0:.2f}, {1}) +- ({2}, {3})\n"
            for x, y, err in serie:
                f.write(format_string.format(x, y, err, err))
            f.write("\n")

def create_graphs(desc, values, names):
    ensure_directory_exists("results")
    graph_file_name = "results/" + desc.get("name") + "_graph.pdf"

    plt.figure()
    colors = "bgrcmykw"
    for name, serie, color in zip(names, values, colors):
        serie = numpy.array(serie)
        x = serie[:, 0]
        y = serie[:, 1]
        err = serie[:, 2]
        plt.errorbar(x, y, yerr=err, fmt=color, capsize=2, label=name)
    plt.legend()
    plt.title(desc.get("name"))
    plt.ylabel(desc.get("present/y_axis"))
    plt.xlabel(desc.get("present/x_axis"))

    plt.savefig(graph_file_name)
    desc.set("present/graph_file_name", graph_file_name)
    plt.close()

def create_presentation(desc):
    compare_to_optimum = desc.get("present/compare_to_optimum")
    assert_in(desc, "present/x_axis", ["n_agents", "n_tasks", "time_taken", "time_limit"])
    x_axis = desc.get("present/x_axis")
    assert_in(desc, "present/y_axis", ["value", "time_taken"])
    y_axis = desc.get("present/y_axis")

    # Dela upp i serier
    n_algorithms = len(desc.get("algorithms"))
    results = desc.get("results")
    series = [{} for i in range(n_algorithms)]
    for problem in results:
        for benchmark in problem["benchmarks"]:
            series_index = benchmark["algorithm"]
            if x_axis == "n_agents":
                key = problem["n_agents"]
            elif x_axis == "n_tasks":
                key = problem["n_tasks"]
            elif x_axis == "time_taken" or x_axis == "time_limit":
                key = benchmark["time_limit"]
            if key not in series[series_index].keys():
                series[series_index][key] = []
            series[series_index][key].append(benchmark)

    # Beräkna plot-värden
    values = []
    names = []
    for serie in series:
        serie_values = []
        algorithm_index = list(serie.values())[0][0]["algorithm"]
        algorithm_name = desc.get(f"algorithms/{algorithm_index}/name")
        display_name = desc.get(f"algorithms/{algorithm_index}/display_name", algorithm_name)
        for x_value, series_results in serie.items():
            if y_axis == "value":
                raw_values = [result["value"] for result in series_results]
            elif y_axis == "time_taken":
                raw_values = [result["time_taken"] for result in series_results]
            y_value = numpy.mean(raw_values)
            y_confdiff = 1.96 * std_err(raw_values)
            if x_axis == "time_taken":
                x_value = numpy.mean([result["time_taken"] for result in series_results])
            serie_values.append((x_value, y_value, y_confdiff))
        values.append(serie_values)
        names.append(display_name)

    # Hitta optimum
    results = desc.get("results")
    if compare_to_optimum:
        optimum_values = []
        if x_axis == "time_limit" or x_axis == "time_taken":
            optimum = numpy.mean([problem["optimum"] for problem in results])
            max_time_limit = numpy.max(desc.get("time_limits"))
            optimum_values.append((0, optimum, 0))
            optimum_values.append((max_time_limit, optimum, 0))
        else:
            optimum = {}
            for problem in results:
                key = problem[x_axis]
                if not key in optimum.keys():
                    optimum[key] = []
                optimum[key].append(problem["optimum"])
            for key, opt_values in optimum.items():
                value = numpy.mean(opt_values)
                optimum_values.append((key, value, 0))
        values.append(optimum_values)
        names.append("optimum")

    if desc.get("present/create_latex_file", False):
        print_latex_file(desc, values, names)

    if desc.get("present/create_graphs", True):
        create_graphs(desc, values, names)

def create_complete_latex_file():
    with open(f'pattern.tex') as template:
        with open(f'results/graph.tex', 'w') as target:
            for line in template:
                if line.split(" ")[0].strip() == '###':
                    with open("results/" + line.split(" ")[1].strip()) as content:
                        for line in content.readlines():
                            target.write(line)
                else:
                    target.write(line)

def main():
    parser = argparse.ArgumentParser(description='Run a set of SCSGA benchmarks.', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--n_threads', type=int, required=True, help='Number of threads available to run benchmarks.')
    parser.add_argument('--log_to_console', type=bool, default=False, required=False, help='Enable logging to console')
    parser.add_argument('--log_file', type=str, default='log.txt', required=False, help='File where the complete log is saved')
    args = parser.parse_args()

    if args.log_to_console:
        console = logging.StreamHandler()
        console.setLevel(logging.INFO)
        console.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
        logger.addHandler(console)
    
    log_file = logging.FileHandler(args.log_file, mode='w')
    log_file.setLevel(logging.DEBUG)
    log_file.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
    logger.addHandler(log_file)

    ensure_directory_exists('results')

    file_names = []
    for type in TYPES:
        for distr in DISTRIBUTIONS:
            file_names.append(f'specifications/{type}_{distr}.json')
    
    with tqdm.tqdm(iterable=file_names, total=len(file_names), position=0, leave=False) as pbar:
        for file_name in pbar:
            stripped_file_name = file_name.split("/")[-1].split('.')[0]
            pbar.set_description('{0:<30}'.format(f'Processing {stripped_file_name}'), refresh=True)
            desc = ExperimentDescription(file_name)
            desc.set('n_threads', int(args.n_threads))
            run_benchmarks(desc)
            create_presentation(desc)
            desc.save(f'results/{get_unique_name(desc)}_result')
    create_complete_latex_file()

if __name__ == '__main__':
    main()
