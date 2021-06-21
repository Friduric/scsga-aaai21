cmake_minimum_required (VERSION 3.5)
project (scsga)

# Build with C++17 support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Allow creating filters for projects in visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
  set(BENCHMARK_LIBRARIES "-lstdc++fs")
endif()

# Add pybind11
set(PYBIND11_CPP_STANDARD -std=c++1z) 
add_subdirectory("LIB/pybind11")
add_subdirectory("PYTHON-API")

add_subdirectory(SCSGA-Benchmark)
