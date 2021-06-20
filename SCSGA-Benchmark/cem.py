#!/usr/bin/env python3

import argparse
import random
import multiprocessing
import os
import numpy as np
import logging

from experiment_description import ExperimentDescription

import sys
# TODO: What's the path on Windows? Append both or have an if-statement
sys.path.append('../build/PYTHON-API')

import scsga
from scsga import problem_t as Problem
from scsga import problem_specification_t as ProblemSpecifiation
from scsga import benchmark_specification_t as BenchmarkSpecification

logger = logging.getLogger("cem_logger")
console = logging.StreamHandler()
console.setLevel(logging.INFO)
logger.setLevel(logging.INFO)
console.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
logger.addHandler(console)

def assert_in(desc, path, valid_values):
    """Raise an exception if the value at path is not a valid value."""
    value = desc.get(path).lower()
    if not value in valid_values:
        valid_values_string = ", ".join(valid_values)
        msg = f"Invalid value found at {path}: {value}. Must be one of {valid_values_string}"
        raise Exception(msg)

def get_distribution(desc):
    valid_distributions = ["eu4", "mpd", "ndcs", "npd", "nrd", "nsd", "nsrd", "simple", "snpd", "supd", "trap", "upd"]
    assert_in(desc, "problem/distribution", valid_distributions)
    name = desc.get("problem/distribution").lower()
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

def evaluate(problem_spec, exploration_weight, variance_weight, evaluation_time_limit):
    problem = Problem(problem_spec)
    benchmark_specification = BenchmarkSpecification(evaluation_time_limit, 1)
    dnn_weight = 0
    use_hillclimb = False
    result = scsga.run_mcts_benchmark(problem, benchmark_specification, exploration_weight, variance_weight, dnn_weight, use_hillclimb)
    value = result.solution_values[0]
    return value

def run_cem(desc):
    
    distribution = get_distribution(desc)
    if desc.get("problem/file_names", []):
        n_agents = 0
        n_tasks = 0
    else:
        n_agents = desc.get("problem/n_agents")
        n_tasks = desc.get("problem/n_tasks")
    n_evaluations = desc.get("n_evaluations")

    # Skapa parameterdistributioner
    exploration_mean = desc.get("start_values/exploration_weight/mean")
    exploration_stdev = desc.get("start_values/exploration_weight/stdev")
    variance_mean = desc.get("start_values/variance_weight/mean")
    variance_stdev = desc.get("start_values/variance_weight/stdev")
    learning_rate = desc.get("learning_rate")
    convergence_cutoff = desc.get("convergence_cutoff")
    evaluation_time_limit = desc.get("evaluation_time_limit")

    n_iterations = desc.get("n_max_iterations")
    iteration = 0
    desc.set("results", [])
    while iteration < n_iterations and (exploration_stdev > convergence_cutoff or variance_stdev > convergence_cutoff):
        # Dra samples
        n_samples = desc.get("n_samples")
        exploration_samples = np.random.normal(exploration_mean, exploration_stdev, n_samples)
        variance_samples = np.random.normal(variance_mean, variance_stdev, n_samples)

        # Skapa problem
        specifications = []
        for i in range(n_evaluations):
            seed = random.randint(0, (1 << 31) - 1)
            specification = ProblemSpecifiation(n_agents, n_tasks, seed, distribution)
            if desc.get("problem/file_names", []):
                specification.distribution_file_name = desc.get("problem/file_names")[i]
            else:
                specification.distribution_file_name = ""
            specifications.append(specification)

        # Evaluera kombinationer
        n_threads = desc.get("n_threads")
        results = []
        for num, exp, var in zip(range(n_samples), exploration_samples, variance_samples):
            thread_input = [(spec, exp, var, evaluation_time_limit) for spec in specifications]
            with multiprocessing.Pool(n_threads) as pool:
                sample_results = pool.starmap(evaluate, thread_input)
            results.append((np.mean(sample_results), exp, var))
            logger.info("Sample {0:4d}: exploration {1:6.2f} variance {2:6.2f} value {3:6.2f}".format(num, exp, var, np.mean(sample_results)))

        # Hitta elite samples
        n_elite = desc.get("n_elite_samples")
        elite = sorted(results)[-n_elite:]
        elite_values = [e[0] for e in elite]
        exploration_elite = [e[1] for e in elite]
        variance_elite = [e[2] for e in elite]

        # Uppdatera distributioner
        exploration_mean = learning_rate * np.mean(exploration_elite) + (1 - learning_rate) * exploration_mean
        exploration_stdev = learning_rate * np.std(exploration_elite) + (1 - learning_rate) * exploration_stdev
        variance_mean = learning_rate * np.mean(variance_elite) + (1 - learning_rate) * variance_mean
        variance_stdev = learning_rate * np.std(variance_elite) + (1 - learning_rate) * variance_stdev

        elite_mean = np.mean(elite_values)
        result_info = {
            "exploration_weight" : {
                "mean" : exploration_mean,
                "stdev" : exploration_stdev
            },
            "variance_weight" : {
                "mean" : variance_mean,
                "stdev" : variance_stdev
            },
            "elite_mean" : elite_mean
        }
        desc.get("results").append(result_info)
        desc.save("cem_results/" + desc.get("name"))
        logger.info("After iteration {0}: exploration ({1:6.2f}, {2:6.2f}) variance ({3:6.2f}, {4:6.2f}) elite value mean {5:6.2f}".format(iteration, exploration_mean, exploration_stdev, variance_mean, variance_stdev, elite_mean))
        iteration += 1

def main():
    parser = argparse.ArgumentParser(description='Run a set of SCSGA benchmarks.', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('file', help='Path to JSON-file describing the benchmarks.')
    args = parser.parse_args()

    desc = ExperimentDescription(args.file)
    ensure_directory_exists("cem_results")
    run_cem(desc)

if __name__ == '__main__':
    main()
