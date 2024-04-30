cmake_minimum_required(VERSION 3.18)

project(ORION VERSION 1.0 DESCRIPTION "Operational Reasoning and Intelligence Optimized Nexus")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#set(CMAKE_CXX_CLANG_TIDY
#  clang-tidy;
#  -format-style='file';
#  -header-filter=${CMAKE_CURRENT_SOURCE_DIR};
#)

# List all the asset files to watch for changes. changing any of these files will trigger the post-build copy assets command
# which will copy all the assets to the executable directory
# use file(GLOB_RECURSE ASSETS "${CMAKE_SOURCE_DIR}/assets/*") to include all files in the assets directory
file(GLOB_RECURSE ASSETS
        "${CMAKE_SOURCE_DIR}/assets/*"
        ${CMAKE_SOURCE_DIR}/.openai_api_key.txt
        ${CMAKE_SOURCE_DIR}/.openweather_api_key.txt
        ${CMAKE_SOURCE_DIR}/.hass_api_key.txt
        ${CMAKE_SOURCE_DIR}/.google_api_key.txt
        ${CMAKE_SOURCE_DIR}/.google_cse_id.txt
        ${CMAKE_SOURCE_DIR}/cert.pem
        ${CMAKE_SOURCE_DIR}/key.pem
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

# Generate ssl certificates
add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/cert.pem ${CMAKE_SOURCE_DIR}/key.pem
        COMMAND openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes -subj "/C=US/ST=CA/L=San Francisco/O=Global Security/OU=IT Department/CN=localhost"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating ssl certificates..."
)

# Add custom target to generate ssl certificates
add_custom_target(ssl_certificates ALL DEPENDS ${CMAKE_SOURCE_DIR}/cert.pem ${CMAKE_SOURCE_DIR}/key.pem)

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
        src/tools/NavigateLinkFunctionTool.cpp
        src/tools/DownloadHTTPFileFunctionTool.cpp
        src/tools/CreateAutonomousActionPlanFunctionTool.cpp
        src/tools/RequestFileUploadFromUserFunctionTool.cpp
        src/tools/RememberKnowledgeFunctionTool.cpp
        src/tools/RecallKnowledgeFunctionTool.cpp
        src/tools/UpdateKnowledgeFunctionTool.cpp
        src/MimeTypes.cpp
        src/GUID.cpp

        ${ASSETS_CPP_FILE} # This is the generated .cpp file to trigger post-build copy assets command
        src/Process.cpp
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
        include/tools/ChangeIntelligenceFunctionTool.hpp
        include/tools/ChangeVoiceFunctionTool.hpp
        include/tools/CodeInterpreterTool.hpp
        include/tools/DownloadHTTPFileFunctionTool.hpp
        include/tools/ExecSmartDeviceServiceFunctionTool.hpp
        include/tools/FunctionTool.hpp
        include/tools/GetWeatherFunctionTool.hpp
        include/tools/ListSmartDevicesFunctionTool.hpp
        include/tools/NavigateLinkFunctionTool.hpp
        include/tools/RetrievalTool.hpp
        include/tools/SearchFilesystemFunctionTool.hpp
        include/tools/TakeScreenshotFunctionTool.hpp
        include/tools/WebSearchFunctionTool.hpp
        include/tools/CreateAutonomousActionPlanFunctionTool.hpp
        include/tools/RequestFileUploadFromUserFunctionTool.hpp
        include/tools/RememberKnowledgeFunctionTool.hpp
        include/tools/RecallKnowledgeFunctionTool.hpp
        include/tools/UpdateKnowledgeFunctionTool.hpp
        include/MiscConstants.hpp
        include/ETTSAudioFormat.hpp
        include/Process.hpp
)

# Define the executable based on the explicitly listed source files
add_executable(ORION ${HEADERS} ${SOURCES})

# If you need to link against libraries, do so here

include(FetchContent)
FetchContent_Declare(myhtml GIT_REPOSITORY git@github.com:lexborisov/myhtml.git)
FetchContent_Declare(cpprestsdk GIT_REPOSITORY git@github.com:microsoft/cpprestsdk.git)
FetchContent_Declare(cmark GIT_REPOSITORY git@github.com:commonmark/cmark.git)
FetchContent_Declare(
        sqlite3
        URL https://www.sqlite.org/src/zip/sqlite.zip?r=release
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(sqlite_modern_cpp GIT_REPOSITORY git@github.com:SqliteModernCpp/sqlite_modern_cpp.git)

FetchContent_GetProperties(sqlite3)
if (NOT sqlite3_POPULATED)
    FetchContent_Populate(sqlite3)
    message(STATUS "sqlite3_SOURCE_DIR: ${sqlite3_SOURCE_DIR}")
    message(STATUS "sqlite3_BINARY_DIR: ${sqlite3_BINARY_DIR}")
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
add_dependencies(ORION sqlite3_build ssl_certificates)

FetchContent_MakeAvailable(
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
target_include_directories(ORION PRIVATE
        include
        ${MYHTML_INCLUDE_DIR}
        ${CPPRESTSDK_INCLUDE_DIR}
        ${CMARK_INCLUDE_DIR}
        ${SQLITE3_INCLUDE_DIR}
        ${SQLITE_MODERN_CPP_INCLUDE_DIR}
)

target_link_libraries(ORION PRIVATE
        myhtml
        cpprest
        cmark
        ${SQLITE3_LIBRARY}
)

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

        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/.google_api_key.txt
        $<TARGET_FILE_DIR:ORION>/.google_api_key.txt

        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/.google_cse_id.txt
        $<TARGET_FILE_DIR:ORION>/.google_cse_id.txt

        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/cert.pem
        $<TARGET_FILE_DIR:ORION>/cert.pem

        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/key.pem
        $<TARGET_FILE_DIR:ORION>/key.pem

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:ORION>/assets

        COMMENT "Copying asset files to the executable directory..."
)