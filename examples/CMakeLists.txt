set(EXAMPLES
    async_pubsub2.cpp
    async_pubsub.cpp
    async_set_get2.cpp
    async_set_get.cpp
    sync_set_get.cpp
    benchmark.cpp
)

foreach(EXAMPLE ${EXAMPLES})
  get_filename_component(EXECUTABLE ${EXAMPLE} NAME_WE)
  add_executable(${EXECUTABLE} ${EXAMPLE})
  target_link_libraries(${EXECUTABLE}
      RedisClient
      Boost::date_time)
endforeach()

target_link_libraries(benchmark
    RedisClient
    Boost::program_options
)
