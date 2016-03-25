cmake_minimum_required(VERSION 2.6)

PROJECT(RedisClient)

FIND_PACKAGE(Boost COMPONENTS unit_test_framework system REQUIRED)


if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

SET(CMAKE_CXX_FLAGS "-W -Wall -Wextra -Wshadow -fPIC -pthread")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG ")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

SET(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/impl/redisvalue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/impl/redissyncclient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/impl/redisparser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/impl/redisclientimpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/impl/redisasyncclient.cpp
)

SET(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/version.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/redisvalue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/redissyncclient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/redisparser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/redisclient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/redisbuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/redisasyncclient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/redisclient/impl/redisclientimpl.h
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
