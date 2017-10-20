set(hdrs config.h
         pipeline.h
         redisasyncclient.h
         redisbuffer.h
         redisparser.h
         redissyncclient.h
         redisvalue.h
         version.h
         impl/redisclientimpl.h)
set(srcs impl/pipeline.cpp
         impl/redisasyncclient.cpp
         impl/redisclientimpl.cpp
         impl/redisparser.cpp
         impl/redissyncclient.cpp
         impl/redisvalue.cpp)

if (HEADER_ONLY)
  RedisClientHeaderLibrary(RedisClient HEADERS ${hdrs} ${srcs} WITH_INSTALL)
  target_compile_definitions(RedisClient
    INTERFACE
      REDIS_CLIENT_HEADER_ONLY
  )
  target_include_directories(RedisClient
    INTERFACE
      ${Boost_INCLUDE_DIRS}
  )
  target_link_libraries(RedisClient
    INTERFACE
      ${Boost_SYSTEM_LIBRARY}
  )
else()
  RedisClientLibrary(RedisClient SOURCES ${srcs} HEADERS ${hdrs} WITH_INSTALL)
  target_compile_definitions(RedisClient
    PUBLIC
      REDIS_CLIENT_SEPARATED_COMPILATION
  )
  target_include_directories(RedisClient
    PUBLIC
      ${Boost_INCLUDE_DIRS}
  )
  target_link_libraries(RedisClient
    PUBLIC
      ${Boost_SYSTEM_LIBRARY}
  )
endif()

