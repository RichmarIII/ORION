cmake_minimum_required(VERSION 3.18)

project(ORION VERSION 1.0 DESCRIPTION "Operational Reasoning and Intelligence Optimized Nexus")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/Orion")

add_subdirectory(library)
add_subdirectory(application)

# Meta target that builds both the library and the application
add_custom_target(ORION ALL COMMENT "Building ORION: Library and Application")
add_dependencies(ORION Orion OrionServer)

# Export the targets to a script
install(EXPORT OrionTargets
        FILE OrionTargets.cmake
        NAMESPACE Orion::
        DESTINATION lib/Orion/cmake
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        OrionConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/OrionConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/OrionConfig.cmake
        INSTALL_DESTINATION lib/Orion/cmake
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/OrionConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/OrionConfigVersion.cmake"
        DESTINATION lib/Orion/cmake
)