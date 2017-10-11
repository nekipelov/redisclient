set(BENCHMARKS
    redis-parser-benchmark.cpp
)

foreach(BENCHMARK ${BENCHMARKS})
  get_filename_component(EXECUTABLE ${BENCHMARK} NAME_WE)
  add_executable(${EXECUTABLE} ${BENCHMARK})
  target_link_libraries(${EXECUTABLE}
    redisclient
    Boost::system
    -lbenchmark
    -lprofiler
  )
endforeach()
