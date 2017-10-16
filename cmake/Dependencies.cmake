# CMake script for finding dependencies. This file is included two times, once
# by the main CMake script for the project, for finding dependencies at build
# time, and once by the package's installed config script, for finding public
# dependencies. A variable, called ${CMAKE_PROJECT_NAME}Install is defined by
# the installed config script and can be used by this script to differentiate
# between the two cases

# Find Boost, users of the library only need system (for error_code), our
# examples use the other components
set(boost_components system)
if (NOT RedisClientInstall)
  set(boost_components ${boost_components}
                       program_options
                       unit_test_framework
                       date_time)
endif()

find_package(Boost
  COMPONENTS
    ${boost_components}
  REQUIRED)
