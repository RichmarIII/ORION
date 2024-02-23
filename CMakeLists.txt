cmake_minimum_required(VERSION 3.8)

project(ORION VERSION 1.0 DESCRIPTION "Operational Reasoning and Intelligence Optimized Nexus")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directory for header files
include_directories(include)

# Explicitly list your source files
set(SOURCES
    src/main.cpp
    src/ORION.cpp
    # Add all source files here
)

# Define the executable based on the explicitly listed source files
add_executable(ORION ${SOURCES})

# If you need to link against libraries, do so here
find_package(cpprestsdk REQUIRED)
target_link_libraries(ORION PRIVATE cpprestsdk::cpprest)
