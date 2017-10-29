# CMake script for RedisClient
############################################################

cmake_minimum_required(VERSION 3.1)

# Project and Versioning
project(RedisClient)
set(VERSION_MAJOR 1 CACHE STRING "Project major version number.")
set(VERSION_MINOR 0 CACHE STRING "Project minor version number.")
set(VERSION_PATCH 0 CACHE STRING "Project patch version number.")
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH VERSION)

# Include utility functions
include(cmake/Utils.cmake)

# Set the C++11 standard
set(CMAKE_CXX_STANDARD 11)

# Build options
option(ADDRESS_SANITIZER "Enable address sanitizer" OFF)
option(BENCHMARK "Build benmarks" OFF)
option(BUILD_SHARED_LIBS "Whether to build shared libraries" ON)
option(BUILD_TEST "Whether to build the unit tests" ON)
option(BUILD_EXAMPLES "Whether to build the examples" ON)
option(HEADER_ONLY "Whether to build in header-only mode" OFF)

# Default to Release mode
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Optionally add the address sanitizer
if (ADDRESS_SANITIZER)
  message(STATUS "Build with address-sanitizer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_LD_FLAGS "-fsanitize=address")
endif()

if (MSVC)
  add_definitions(-DBOOST_ALL_NO_LIB)
endif()

if (NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_ALL_DYN_LINK)
endif()

# Find all of our dependencies
include(cmake/Dependencies.cmake)

if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od -DDEBUG ")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 -DNDEBUG")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wextra -pthread")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3 -DDEBUG ")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

add_subdirectory(src)
if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
if (BUILD_TEST)
  enable_testing()
  add_subdirectory(tests)
endif()
if (BENCHMARK)
  add_subdirectory(benchmarks)
endif()

include(cmake/Install.cmake)
