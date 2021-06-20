# Simultaneous Coalition Structure Generation and Assignment

This repository contains implementations, test cases, utilities and benchmarks for simultaneous coalition structure generation and assignment (SCSGA) algorithms. It contains all code use to derive the results in *Anytime Heuristic and Monte Carlo Methods for Large-Scale Simultaneous Coalition Structure Generation and Assignment* by Präntare et. al. 2021. The repository is provided by the article authors.

A short overview of the code in this repository:

- **SCSGA**: The main algorithm library
  - Files with prefix **coalitional_values_generator** contains code for generating problem distributions.
  - Files with prefix **solver** implements different algorithms to solve SCSGA problems.
- **SCSGA-Benchmark**: A custom-built benchmark suite for SCSGA algorithms.
  - C++ functions to benchmark different solvers are located in benchmark.\[h/cpp\].
  - To simplify running consecutive benchmarks, a Python script is also provided in benchmark.py (see below).
- **PYTHON-API**: Bindings to enable calling benchmark functions from Python.

# Running instructions

The following steps should be completed to reproduce the results from the article. It is written for and tested on Ubuntu 20.04. First, install system dependencies.
```
sudo apt update
sudo apt install cmake git gcc g++ python3 python3-venv
```

Clone the repository and initialize the pybind11 submodule.
```
git clone TODO
git submodule update --init --recursive
```

Setup the build system and compile the C++ code.
```
cd scsga
mkdir build
cd build
cmake ..
make -j 8
cd ..
```

Create a virtual Python environment and install dependencies.
```
python3 -m venv env
source env/bin/activate 
# To later leave the virtual environment, use the command deactivate
pip3 install -r requirements.txt
```

Run the benchmark script.
```
cd SCSGA-Benchmark
benchmark.py
```

This executes the benchmark pipeline for all benchmarks shown in the article (defined using JSON files in the specifications directory). Note that this will take many CPU-days to complete. Use the --n_threads argument to run the benchmarks in parallel. The benchmark time limit is implemented using wall clock time, so don't use more threads than the number of physical cores on your machine. Also, keep in mind that the peak memory usage is approx. 2.5 MB per thread used. The --log_to_console flag might also be of interest to display subresults.

Once the benchmark suite has completed, the results are presented in three forms in the results directory. The raw results are saved to json files, including the settings for each benchmark type. Second, graphs are produced comparing the performance of each algorithm on the different benchmarks. Finally, all results are collected to a LaTeX file (results/graph.tex). This file required the pgfplots library to compile.

# Contact

Feel free to contact us with code or research related questions. You can find our contact information using the search function at the [Linköping University - IDA website](https://www.ida.liu.se/department/contact/search_person.en.shtml).
