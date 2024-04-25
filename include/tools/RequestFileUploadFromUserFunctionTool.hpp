#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can be used to request the user to upload a file from the USER'S computer/device so that it can be used by the assistant in the future.
    class RequestFileUploadFromUserFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief // A function that requests the user to upload a file from the USER'S computer/device so that it can be used by the assistant in the future.
            constexpr static auto REQUEST_FILE_UPLOAD_FROM_USER = R"(
            {
                "description" : "Requests the user to upload a file from the USER'S computer/device so that it can be used by the assistant in the future.",
                "name" : "request_file_upload_from_user",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "file_path" : {
                            "type" : "string",
                            "description" : "The absolute path to the file on the USER'S computer/device that the assistant wants the user to upload."
                        }
                    },
                    "required" : ["file_path"]
                }
            })";
        };

        inline RequestFileUploadFromUserFunctionTool()
            : FunctionTool(Statics::REQUEST_FILE_UPLOAD_FROM_USER)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
