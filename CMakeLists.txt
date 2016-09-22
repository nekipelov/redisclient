cmake_minimum_required(VERSION 2.6)

PROJECT(RedisClient)

OPTION(ADDRESS_SANITIZER "Enable address sanitizer" OFF)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

if( ADDRESS_SANITIZER )
    message(STATUS "Build with address-sanitizer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_LD_FLAGS "-fsanitize=address")
endif()

if(MSVC)
    ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB)
endif()

if(NOT Boost_USE_STATIC_LIBS)
    ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK)
endif()

FIND_PACKAGE(Boost COMPONENTS system program_options unit_test_framework date_time REQUIRED)

LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

IF(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od -DDEBUG ")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 -DNDEBUG")
ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wextra -fPIC -pthread -std=c++11")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3 -DDEBUG ")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
ENDIF()

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
ADD_EXECUTABLE(redisvaluetest tests/redisvaluetest.cpp ${SOURCES} ${HEADERS})
ADD_LIBRARY(redisclient STATIC ${HEADERS} ${SOURCES})

SET(TESTS
    parsertest
    redisvaluetest
)

FOREACH(TEST ${TESTS})
    TARGET_LINK_LIBRARIES(${TEST} ${Boost_SYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
ENDFOREACH()


INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/src DESTINATION include)

ADD_SUBDIRECTORY(examples)

INCLUDE(CTest)
ADD_TEST(ParserTest parsertest)
ADD_TEST(RedisValueTest redisvaluetest)

