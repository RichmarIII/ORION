# Generate ssl certificates
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/cert.pem ${CMAKE_CURRENT_SOURCE_DIR}/key.pem
        COMMAND openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes -subj "/C=US/ST=CA/L=San Francisco/O=Global Security/OU=IT Department/CN=localhost"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating ssl certificates..."
)

# Add custom target to generate ssl certificates
add_custom_target(ssl_certificates DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cert.pem ${CMAKE_CURRENT_SOURCE_DIR}/key.pem)

# Explicitly list your source files
set(SOURCES
        src/Orion.cpp
        src/OrionWebServer.cpp
        src/MimeTypes.cpp
        src/GUID.cpp
        src/Process.cpp
        src/Plugin.cpp
)

# Explicitly list your header files
set(HEADERS
        include/GUID.hpp
        include/IOrionTool.hpp
        include/MimeTypes.hpp
        include/Orion.hpp
        include/OrionWebServer.hpp
        include/User.hpp
        include/Knowledge.hpp
        include/tools/CodeInterpreterTool.hpp
        include/tools/FunctionTool.hpp
        include/tools/RetrievalTool.hpp
        include/MiscConstants.hpp
        include/ETTSAudioFormat.hpp
        include/Process.hpp
        include/Plugin.hpp
)

# Define the library
add_library(Orion SHARED ${HEADERS} ${SOURCES})

include(FetchContent)

message(STATUS "Github Token: " ${GITHUB_TOKEN})

if (NOT DEFINED GITHUB_TOKEN)
    FetchContent_Declare(boost GIT_REPOSITORY git@github.com:boostorg/boost.git EXCLUDE_FROM_ALL)
    FetchContent_Declare(myhtml GIT_REPOSITORY git@github.com:lexborisov/myhtml.git EXCLUDE_FROM_ALL)
    FetchContent_Declare(cpprestsdk GIT_REPOSITORY git@github.com:microsoft/cpprestsdk.git EXCLUDE_FROM_ALL)
    FetchContent_Declare(cmark GIT_REPOSITORY git@github.com:commonmark/cmark.git EXCLUDE_FROM_ALL)
    FetchContent_Declare(
            sqlite3
            URL https://www.sqlite.org/src/zip/sqlite.zip?r=release
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE EXCLUDE_FROM_ALL
    )
    FetchContent_Declare(sqlite_modern_cpp GIT_REPOSITORY git@github.com:SqliteModernCpp/sqlite_modern_cpp.git EXCLUDE_FROM_ALL)
else ()
    FetchContent_Declare(boost GIT_REPOSITORY https://${GITHUB_TOKEN}@github.com/boostorg/boost.git EXCLUDE_FROM_ALL)
    FetchContent_Declare(myhtml GIT_REPOSITORY https://${GITHUB_TOKEN}@github.com/lexborisov/myhtml.git EXCLUDE_FROM_ALL)
    FetchContent_Declare(cpprestsdk GIT_REPOSITORY https://${GITHUB_TOKEN}@github.com/microsoft/cpprestsdk.git EXCLUDE_FROM_ALL)
    FetchContent_Declare(cmark GIT_REPOSITORY https://${GITHUB_TOKEN}@github.com/commonmark/cmark.git EXCLUDE_FROM_ALL)
    FetchContent_Declare(
            sqlite3
            URL https://www.sqlite.org/src/zip/sqlite.zip?r=release
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE EXCLUDE_FROM_ALL
    )
    FetchContent_Declare(sqlite_modern_cpp GIT_REPOSITORY https://${GITHUB_TOKEN}@github.com/SqliteModernCpp/sqlite_modern_cpp.git EXCLUDE_FROM_ALL)
endif ()


FetchContent_GetProperties(sqlite3)
if (NOT sqlite3_POPULATED)
    FetchContent_Populate(sqlite3)
    add_custom_command(
            OUTPUT ${sqlite3_BINARY_DIR}/install/lib/libsqlite3.so
            COMMAND cd ${sqlite3_BINARY_DIR}
            COMMAND ${sqlite3_SOURCE_DIR}/configure --prefix=${sqlite3_BINARY_DIR}/install
            COMMAND make
            COMMAND make sqlite3.c
            COMMAND make install
            WORKING_DIRECTORY ${sqlite3_SOURCE_DIR}
            COMMENT "Building sqlite3..."
    )
endif ()

add_custom_target(sqlite3_build ALL DEPENDS ${sqlite3_BINARY_DIR}/install/lib/libsqlite3.so)
add_dependencies(Orion sqlite3_build ssl_certificates)

FetchContent_MakeAvailable(
        boost
        myhtml
        cpprestsdk
        cmark
        sqlite_modern_cpp
)

get_target_property(MYHTML_SOURCE_DIR myhtml SOURCE_DIR)
set(MYHTML_INCLUDE_DIR ${MYHTML_SOURCE_DIR}/include)
get_target_property(CPPRESTSDK_SOURCE_DIR cpprest SOURCE_DIR)
set(CPPRESTSDK_INCLUDE_DIR ${CPPRESTSDK_SOURCE_DIR}/../include)
#disable warnings as errors for cpprestsdk
target_compile_options(cpprest PRIVATE -Wno-error)
get_target_property(CMARK_SOURCE_DIR cmark SOURCE_DIR)
set(CMARK_INCLUDE_DIR ${CMARK_SOURCE_DIR}/src)
set(SQLITE3_INCLUDE_DIR ${sqlite3_BINARY_DIR}/install/include)
set(SQLITE3_LIBRARY ${sqlite3_BINARY_DIR}/install/lib/libsqlite3.so)
set(SQLITE_MODERN_CPP_SOURCE_DIR ${sqlite_modern_cpp_SOURCE_DIR})
set(SQLITE_MODERN_CPP_INCLUDE_DIR ${SQLITE_MODERN_CPP_SOURCE_DIR}/hdr)


# Include directories for header files
target_include_directories(Orion PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CPPRESTSDK_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${MYHTML_INCLUDE_DIR}>

        $<INSTALL_INTERFACE:include/Orion>
        $<INSTALL_INTERFACE:include/Orion/third_party/cpprestsdk>
        $<INSTALL_INTERFACE:include/Orion/third_party/myhtml>

)
target_include_directories(Orion PRIVATE
        $<BUILD_INTERFACE:${CMARK_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${SQLITE3_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${SQLITE_MODERN_CPP_INCLUDE_DIR}>
)

target_link_libraries(Orion PUBLIC
        cpprest
        myhtml
        crypto
)

target_link_libraries(Orion PRIVATE
        cmark
        ${SQLITE3_LIBRARY}
)

# Set the targets Plugin Directory (where the plugins will be installed). This should be accessible by the plugins
set_property(GLOBAL PROPERTY PLUGIN_DIR ${CMAKE_INSTALL_PREFIX}/share/Orion/plugins)

# Install the dependencies

# Install the library
install(TARGETS
        Orion
        cpprest
        cpprestsdk_openssl_internal
        cpprestsdk_websocketpp_internal
        cpprestsdk_boost_internal
        myhtml
        cmark

        EXPORT OrionTargets
        LIBRARY DESTINATION lib/Orion
        ARCHIVE DESTINATION lib/Orion
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include/Orion
        PUBLIC_HEADER DESTINATION include/Orion
        PRIVATE_HEADER DESTINATION include/Orion
)

# Install the header files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include/Orion)
install(DIRECTORY ${CPPRESTSDK_INCLUDE_DIR}/ DESTINATION include/Orion/third_party/cpprestsdk)
install(DIRECTORY ${MYHTML_INCLUDE_DIR}/ DESTINATION include/Orion/third_party/myhtml)

# Install the ssl certificates
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cert.pem ${CMAKE_CURRENT_SOURCE_DIR}/key.pem DESTINATION bin)

# Install the assets
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets DESTINATION share/Orion)

# Install the api keys
install(FILES
        ${CMAKE_SOURCE_DIR}/.openai_api_key.txt
        ${CMAKE_SOURCE_DIR}/.openweather_api_key.txt
        ${CMAKE_SOURCE_DIR}/.hass_api_key.txt
        ${CMAKE_SOURCE_DIR}/.google_api_key.txt
        ${CMAKE_SOURCE_DIR}/.google_cse_id.txt
        DESTINATION bin
)