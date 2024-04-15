#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can upload a file for the assistant to use in the future (code_interpreter, retrieval, etc.)
    class UploadFileToAssistantFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief // A function that uploads a file to the assistant
            constexpr static auto UPLOAD_FILE_TO_ASSISTANT = R"(
            {
                "description" : "Uploads a local file to the assistant so it can be used by the assistant. eg: code_interpreter, retrieval, etc.",
                "name" : "upload_file_to_assistant",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "file_path" : {
                            "type" : "string",
                            "description" : "The absolute path to the file to upload. must include a file extension."
                        }
                    },
                    "required" : ["file_path"]
                }
            })";
        };

        inline
        UploadFileToAssistantFunctionTool()
            : FunctionTool(Statics::UPLOAD_FILE_TO_ASSISTANT)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
