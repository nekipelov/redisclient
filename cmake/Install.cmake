set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${INSTALL_LIB_DIR})

include(CMakePackageConfigHelpers)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Dependencies.cmake
     "include(CMakeFindDependencyMacro)\n"
     "find_dependency(Boost 1.50.0 COMPONENTS system)\n")

configure_package_config_file(cmake/config/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION share/${CMAKE_PROJECT_NAME}
    PATH_VARS INCLUDE_INSTALL_DIR LIB_INSTALL_DIR)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY AnyNewerVersion)

install(EXPORT ${CMAKE_PROJECT_NAME}-targets
    FILE ${CMAKE_PROJECT_NAME}Targets.cmake
    NAMESPACE ${CMAKE_PROJECT_NAME}::
    DESTINATION share/${CMAKE_PROJECT_NAME})

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Dependencies.cmake
    DESTINATION share/${CMAKE_PROJECT_NAME})

install(FILES ${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake
        RENAME ${CMAKE_PROJECT_NAME}Dependencies.cmake
        DESTINATION share/${CMAKE_PROJECT_NAME})
