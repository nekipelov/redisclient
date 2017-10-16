set(hdrs config.h
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
  target_compile_definitions(RediSclient
    PUBLIC
      REDIS_CLIENT_HEADER_ONLY
  )
else()
  RedisClientLibrary(RedisClient SOURCES ${srcs} HEADERS ${hdrs} WITH_INSTALL)
  target_compile_definitions(RedisClient
    PUBLIC
      REDIS_CLIENT_SEPARATED_COMPILATION
  )
endif()

target_link_libraries(RedisClient
  PUBLIC
    Boost::boost
    Boost::system
)
