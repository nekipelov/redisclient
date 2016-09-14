cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE(Boost COMPONENTS system REQUIRED)
IF(MSVC)
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS}) # seems to be required for Boost otherwise the autolinking doesn't find some of the libs
ENDIF()

IF(MSVC)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od -DDEBUG ")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 -DNDEBUG")
ELSE()
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wextra -Wshadow -fPIC -pthread -std=c++11")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3 -DDEBUG ")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
ENDIF()

SET(REDISCLIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/impl/redisvalue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/impl/redissyncclient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/impl/redisparser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/impl/redisclientimpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/impl/redisasyncclient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/version.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/redisvalue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/redissyncclient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/redisparser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/redisbuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/redisasyncclient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/redisclient/impl/redisclientimpl.h
)

INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/
)

SET(EXAMPLES
    async_pubsub2.cpp
    async_pubsub.cpp
    async_set_get2.cpp
    async_set_get.cpp
    sync_set_get.cpp
    benchmark.cpp
)

FOREACH(FILE ${REDISCLIENT_SOURCES})
    SET_SOURCE_FILES_PROPERTIES(${FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
ENDFOREACH()

FOREACH(EXAMPLE ${EXAMPLES})
    GET_FILENAME_COMPONENT(EXECUTABLE ${EXAMPLE} NAME_WE)
    ADD_EXECUTABLE(${EXECUTABLE} ${EXAMPLE} ${REDISCLIENT_SOURCES})
    TARGET_LINK_LIBRARIES(${EXECUTABLE} ${Boost_LIBRARIES})
ENDFOREACH(EXAMPLE)
