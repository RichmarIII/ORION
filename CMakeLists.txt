cmake_minimum_required(VERSION 3.8)

# Set the cmake toolchain file to use
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

project(ORION VERSION 1.0 DESCRIPTION "Operational Reasoning and Intelligence Optimized Nexus")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_CLANG_TIDY
  clang-tidy;
  -format-style='file';
  -header-filter=${CMAKE_CURRENT_SOURCE_DIR};
)

# Include directory for header files
include_directories(include)

# List all the asset files to watch for changes. changing any of these files will trigger the post-build copy assets command
# which will copy all the assets to the executable directory
# use file(GLOB_RECURSE ASSETS "${CMAKE_SOURCE_DIR}/assets/*") to include all files in the assets directory
file(GLOB_RECURSE ASSETS
    "${CMAKE_SOURCE_DIR}/assets/*"
)

# The generated .cpp file to trigger post-build copy assets command.
# A build (and therefore a post-build) is triggered only when source files change, not when assets change.
# This is a workaround to trigger a post-build command when assets change.
set(ASSETS_CPP_FILE "${CMAKE_SOURCE_DIR}/src/generated/Assets.cpp")

# This command generates an empty .cpp file when any of the assets change (DEPENDS ${ASSETS}).
# And because a .cpp file is changed, a post-build command is triggered.
add_custom_command(
    OUTPUT ${ASSETS_CPP_FILE}
    COMMAND ${CMAKE_COMMAND} -E touch ${ASSETS_CPP_FILE}
    DEPENDS ${ASSETS}
    COMMENT "Assets changed: Generating an empty .cpp file to trigger post-build copy assets command..."
)

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
    src/MimeTypes.cpp

    ${ASSETS_CPP_FILE} # This is the generated .cpp file to trigger post-build copy assets command
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
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:ORION>/assets

    COMMENT "Copying asset files to the executable directory..."
)