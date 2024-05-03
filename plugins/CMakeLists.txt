cmake_minimum_required(VERSION 3.18)

get_property(Orion_PLUGIN_DIR GLOBAL PROPERTY PLUGIN_DIR)

# Variadic macro to create a plugin
macro(basic_plugin name)
    cmake_minimum_required(VERSION 3.28)
    project(name)

    set(CMAKE_CXX_STANDARD 17)

    set(sources ${ARGN})

    # Get the list of all source/header files from the arguments
    foreach (source ${sources})
        if (NOT source MATCHES ".*\\.cpp" AND NOT source MATCHES ".*\\.hpp")
            list(REMOVE_ITEM sources ${source})
        endif ()
    endforeach ()

    # Create the plugin
    add_library(${name} SHARED "${sources}")

    target_link_libraries(${name} PRIVATE Orion)

    install(TARGETS ${name} DESTINATION ${Orion_PLUGIN_DIR})
endmacro()

# Create variable to keep track of all plugin targets as they are created
set(PLUGIN_TARGETS)

# Add all subdirectories by searching for all directories in the current directory
file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach (child ${children})
    if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})

        if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/CMakeLists.txt)
            add_subdirectory(${child})

            # Add the plugin target to the list of all plugin targets
            list(APPEND PLUGIN_TARGETS ${child})
        endif ()
    endif ()
endforeach ()

add_custom_target(Plugins DEPENDS ${PLUGIN_TARGETS})