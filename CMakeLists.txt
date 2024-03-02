cmake_minimum_required(VERSION 3.8)

# Set the cmake toolchain file to use
set(CMAKE_TOOLCHAIN_FILE "/mnt/Apps/Dev/Repositories/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(ORION VERSION 1.0 DESCRIPTION "Operational Reasoning and Intelligence Optimized Nexus")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directory for header files
include_directories(include)

# Explicitly list your source files
set(SOURCES
    src/main.cpp
    src/Orion.cpp
    src/OrionWebServer.cpp
    src/tools/TakeScreenshotFunctionTool.cpp
    src/tools/SearchFilesystemFunctionTool.cpp
    src/tools/GetWeatherFunctionTool.cpp
    src/tools/WebSearchFunctionTool.cpp
    src/tools/ChangeVoiceFunctionTool.cpp
    src/tools/ChangeIntelligenceFunctionTool.cpp
    src/tools/ListSmartDevicesFunctionTool.cpp
    src/tools/ExecSmartDeviceServiceFunctionTool.cpp
    # Add all source files here
)

# Define the executable based on the explicitly listed source files
add_executable(ORION ${SOURCES})

# If you need to link against libraries, do so here
find_package(cpprestsdk CONFIG REQUIRED)
find_package(cmark CONFIG REQUIRED)

target_link_libraries(ORION PRIVATE cpprestsdk::cpprest cpprestsdk::cpprestsdk_zlib_internal cpprestsdk::cpprestsdk_boost_internal cpprestsdk::cpprestsdk_openssl_internal cmark::cmark_static)


# Copy the files to the executable directory recursively
add_custom_command(TARGET ORION POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/.openai_api_key.txt
    $<TARGET_FILE_DIR:ORION>/.openai_api_key.txt

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/.openweather_api_key.txt
    $<TARGET_FILE_DIR:ORION>/.openweather_api_key.txt

    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/.hass_api_key.txt
    $<TARGET_FILE_DIR:ORION>/.hass_api_key.txt

    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/templates
    $<TARGET_FILE_DIR:ORION>/templates

    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/static
    $<TARGET_FILE_DIR:ORION>/static
)