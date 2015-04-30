cmake_minimum_required(VERSION 2.6)

PROJECT(RedisClient)

FIND_PACKAGE(Boost COMPONENTS unit_test_framework system REQUIRED)

SET(CMAKE_CXX_FLAGS "-W -Wall -Wextra -Wshadow -fPIC -pthread")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG ")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

SET(SOURCES
    src/redisclient/impl/redisvalue.cpp
    src/redisclient/impl/redissyncclient.cpp
    src/redisclient/impl/redisparser.cpp
    src/redisclient/impl/redisclientimpl.cpp
    src/redisclient/impl/redisasyncclient.cpp
)

SET(HEADERS
    src/redisclient/config.h
    src/redisclient/version.h
    src/redisclient/redisvalue.h
    src/redisclient/redissyncclient.h
    src/redisclient/redisparser.h
    src/redisclient/redisclient.h
    src/redisclient/redisbuffer.h
    src/redisclient/redisasyncclient.h
    src/redisclient/impl/redisclientimpl.h
)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
)

ADD_EXECUTABLE(parsertest tests/parsertest.cpp ${SOURCES} ${HEADERS})
ADD_LIBRARY(redisclient STATIC ${HEADERS} ${SOURCES})

TARGET_LINK_LIBRARIES(parsertest
    ${Boost_LIBRARIES}
)

TARGET_LINK_LIBRARIES(redisclient
    ${Boost_LIBRARIES}
)

INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/src DESTINATION include)

ADD_CUSTOM_TARGET(test)
ADD_CUSTOM_COMMAND(TARGET test 
    POST_BUILD
    COMMAND ./parsertest
    COMMENT "Run tests"
    VERBATIM
)

ADD_SUBDIRECTORY(examples)
