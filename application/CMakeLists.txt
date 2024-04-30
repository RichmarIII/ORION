# Explicitly list your source files
set(SOURCES src/main.cpp)

# Explicitly list your header files
set(HEADERS)

# Define the executable based on the explicitly listed source files
add_executable(OrionServer ${HEADERS} ${SOURCES})

# If you need to link against libraries, do so here
target_link_libraries(OrionServer PRIVATE Orion)

install(TARGETS OrionServer
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        INCLUDES DESTINATION include
)