#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can download a file from an HTTP link
    class DownloadHTTPFileFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief // A function that downloads a file from an HTTP link
            constexpr static auto DOWNLOAD_HTTP_FILE = R"(
            {
                "description" : "Downloads a file from an HTTP link. MUST be a valid HTTP link (contains host, path, and file extension) eg. https://example.com/file.txt.",
                "name" : "download_http_file",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "link" : {
                            "type" : "string",
                            "description" : "The HTTP link to download the file from."
                        }
                    },
                    "required" : ["link"]
                }
            })";
        };

        inline
        DownloadHTTPFileFunctionTool()
            : FunctionTool(Statics::DOWNLOAD_HTTP_FILE)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
