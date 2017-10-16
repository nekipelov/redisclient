# Gets the install path for libraries
function(GetInstallLibDir output)
  get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
  if ("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
  else()
    set(LIBSUFFIX "")
  endif()
  set(${output} lib${LIBSUFFIX} PARENT_SCOPE)
endfunction()

# Function for installing headers
#
# Arguments:
#   HEADERS The headers to install
function(RedisClientInstallHeaders)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs HEADERS)
  cmake_parse_arguments(RedisClientInstallHeaders
      "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Install the headers (and sources because of the header-only mode)
  file(RELATIVE_PATH module_path ${CMAKE_SOURCE_DIR}/src
       ${CMAKE_CURRENT_SOURCE_DIR})
  foreach (HDR ${RedisClientInstallHeaders_HEADERS})
    get_filename_component(path_within_module ${HDR} DIRECTORY)
    if (path_within_module STREQUAL "")
      install(FILES ${HDR}
              COMPONENT Development
              DESTINATION include/${module_path})
    else()
      install(FILES ${HDR}
              COMPONENT Development
              DESTINATION include/${module_path}/${path_within_module})
    endif()
  endforeach()
endfunction()

# Function for adding a RedisClient library to the build process
#
# Arguments:
#   LIB_NAME Name for the library
#   SOURCES  Named parameter for the source files that go into the library
#   HEADERS  Named parameter for the headers that go into the library
#   WITH_INSTALL Whether this is a library that should be installed
function(RedisClientLibrary LIB_NAME)
  set(options WITH_INSTALL)
  set(oneValueArgs)
  set(multiValueArgs SOURCES HEADERS)
  cmake_parse_arguments(RedisClientLibrary "${options}" "${oneValueArgs}"
                                           "${multiValueArgs}" ${ARGN})

  if (NOT DEFINED LIB_NAME)
    message(FATAL_ERROR "RedisClientLibrary(): LIB_NAME not defined")
  endif()

  add_library(${LIB_NAME} ${RedisClientLibrary_SOURCES}
                          ${RedisClientLibrary_HEADERS})
  target_include_directories(${LIB_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
  )

  if (BUILD_SHARED_LIBS)
    set_target_properties(${LIB_NAME} PROPERTIES
                          VERSION ${VERSION}
                          SOVERSION ${VERSION_MAJOR}.${VERSION_MINOR})
    target_link_libraries(${LIB_NAME} PRIVATE ${CMAKE_DL_LIBS})
  endif()

  # Define install for the library
  if (RedisClientLibrary_WITH_INSTALL)
    # Install the library
    GetInstallLibDir(INSTALL_LIB_DIR)
    install(TARGETS ${LIB_NAME}
            EXPORT ${CMAKE_PROJECT_NAME}-targets 
            COMPONENT Libraries
            DESTINATION ${INSTALL_LIB_DIR}
            LIBRARY NAMELINK_SKIP)
    if (BUILD_SHARED_LIBS)
      install(TARGETS ${LIB_NAME}
              EXPORT ${CMAKE_PROJECT_NAME}-targets LIBRARY
              DESTINATION ${INSTALL_LIB_DIR}
              COMPONENT Development
              NAMELINK_ONLY)
    endif()

    # Install the headers (and sources because of the header-only mode)
    RedisClientInstallHeaders(HEADERS ${RedisClientLibrary_HEADERS})
  endif()
endfunction()

# Function for adding a header-only RedisClient library to the build process
#
# Arguments:
#   LIB_NAME Name for the library
#   SOURCES  Named parameter for the source files that go into the library
#   HEADERS  Named parameter for the headers that go into the library
#   WITH_INSTALL Whether this is a library that should be installed
function(RedisClientHeaderLibrary LIB_NAME)
  set(options WITH_INSTALL)
  set(oneValueArgs)
  set(multiValueArgs HEADERS)
  cmake_parse_arguments(RedisClientHeaderLibrary "${options}" "${oneValueArgs}"
                                                 "${multiValueArgs}" ${ARGN})

  if (NOT DEFINED LIB_NAME)
    message(FATAL_ERROR "RedisClientHeaderLibrary(): LIB_NAME not defined")
  endif()

  add_library(${LIB_NAME} INTERFACE)
  target_include_directories(${LIB_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
  )

  if (RedisClientHeaderLibrary_WITH_INSTALL)
    install(TARGETS ${LIB_NAME}
            EXPORT ${CMAKE_PROJECT_NAME}-targets
            COMPONENT Libraries)
    RedisClientInstallHeaders(HEADERS ${RedisClientHeaderLibrary_HEADERS})
  endif()
endfunction()

# Adds a test for the RedisClientLibrary
#
# Arguments:
#  TEST_NAME     Name of the test executable
#  SOURCES       List of source files to compile into the test
#  DEPENDENCIES  Dependencies (other than RedisClient and the boost unit testing
#                framework) to link into the test 
function(RedisClientTest TEST_NAME)
  set(options)
  set(oneValueArgs SHEET)
  set(multiValueArgs SOURCES DEPENDENCIES)
  cmake_parse_arguments(RedisClientTest "${options}" "${oneValueArgs}"
                                        "${multiValueArgs}" ${ARGN})

  add_executable(${TEST_NAME} ${RedisClientTest_SOURCES})

  target_include_directories(${TEST_NAME} PUBLIC ${BOOST_INCLUDE_DIRS})
  target_link_libraries(${TEST_NAME} RedisClient ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()
