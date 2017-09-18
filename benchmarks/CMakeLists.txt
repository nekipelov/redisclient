
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

SET(BENCHMARKS
    redis-parser-benchmark.cpp
)

FOREACH(FILE ${REDISCLIENT_SOURCES})
    SET_SOURCE_FILES_PROPERTIES(${FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
ENDFOREACH()

FOREACH(BENCHMARK ${BENCHMARKS})
    GET_FILENAME_COMPONENT(EXECUTABLE ${BENCHMARK} NAME_WE)
    ADD_EXECUTABLE(${EXECUTABLE} ${BENCHMARK} ${REDISCLIENT_SOURCES})
    TARGET_LINK_LIBRARIES(${EXECUTABLE}
        ${Boost_SYSTEM_LIBRARY}
#        ${Boost_DATE_TIME_LIBRARY}
#        ${Boost_TIMER_LIBRARY}
        -lbenchmark
        -lprofiler
    )
ENDFOREACH(BENCHMARK)

#TARGET_LINK_LIBRARIES(benchmark
#    ${Boost_PROGRAM_OPTIONS_LIBRARY}
#    ${Boost_DATE_TIME_LIBRARY}
#)

