#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can change the voice of the assistant
    class ChangeVoiceFunctionTool : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that changes the voice of the assistant
            constexpr static const char* CHANGE_VOICE = R"(
            {
                "description" : "Changes the voice of the assistant",
                "name" : "change_voice",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "voice" : {
                            "type" : "string",
                            "enum" : [ "alloy", "echo", "fable", "onyx", "nova", "shimmer" ],
                            "description" : "The voice to change to"
                        }
                    },
                    "required" : ["voice"]
                }
            })";
        };

        inline ChangeVoiceFunctionTool() : FunctionTool(Statics::CHANGE_VOICE)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
