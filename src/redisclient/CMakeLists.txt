set(hdrs config.h
         redisasyncclient.h
         redisbuffer.h
         redisparser.h
         redissyncclient.h
         redisvalue.h
         version.h
         impl/redisclientimpl.h)
set(srcs impl/redisasyncclient.cpp
         impl/redisclientimpl.cpp
         impl/redisparser.cpp
         impl/redissyncclient.cpp
         impl/redisvalue.cpp)

RedisClientLibrary(redisclient SOURCES ${srcs} HEADERS ${hdrs} WITH_INSTALL)
target_link_libraries(redisclient
  PRIVATE
    Boost::system
    Boost::date_time
)
target_include_directories(redisclient PUBLIC ${BOOST_INCLUDE_DIRS})
