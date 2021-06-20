file(GLOB SOURCES "../SCSGA/*.cpp"
                  "../SCSGA/*.h"
                  "../SCSGA-Trainingdata/trainingdata.*"
                  "../SCSGA-Trainingdata/json.hpp"
                  "../SCSGA-Trainingdata/solution_selector*"
                  "../SCSGA-Trainingdata/base_sixty_four.*"
				  "../SCSGA-Benchmark/latex.*"
                  "../SCSGA-Benchmark/benchmark.*")

pybind11_add_module(scsga scsga.cpp scsga.h ${SOURCES}) 
target_link_libraries(scsga PRIVATE "${TORCH_LIBRARIES}" "${BENCHMARK_LIBRARIES}")

if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  add_custom_command(TARGET scsga
                     POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     ${TORCH_DLLS}
                     $<TARGET_FILE_DIR:scsga>)
ENDIF (MSVC)
