#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can search the filesystem for a file
    class SearchFilesystemFunctionTool : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that searches the filesystem for a file, and returns the matches as well as their metadata/attributes
            constexpr static const char* SEARCH_FILESYSTEM = R"(
            {
                "description" : "Searches the filesystem for a file, and returns the matches as well as their metadata/attributes",
                "name" : "search_filesystem",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "file_name" : {
                            "type" : "string",
                            "description" : "The filename to search for including the file extension if applicable. lowercase"
                        },
                        "search_directory" : {
                            "type" : "string",
                            "description" : "The directory to search in. Absolute path, Default is the users home directory $HOME"
                        },
                        "recursive" : {
                            "type" : "boolean",
                            "description" : "Whether to search the directory recursively"
                        }
                    },
                    "required" : ["file_name"]
                }
            })";
        };

        inline SearchFilesystemFunctionTool() : FunctionTool(Statics::SEARCH_FILESYSTEM)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
