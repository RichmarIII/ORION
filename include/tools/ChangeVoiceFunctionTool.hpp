#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can change the voice of the assistant
    class ChangeVoiceFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that changes the voice of the assistant
            constexpr static auto CHANGE_VOICE = R"(
            {
                "description" : "Changes the voice of the assistant. If no voice is provided, it will list the available voices.",
                "name" : "change_or_list_voice",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "voice" : {
                            "type" : "string",
                            "enum" : [ "alloy", "echo", "fable", "onyx", "nova", "shimmer" ],
                            "description" : "The voice to change to"
                        }
                    }
                }
            })";
        };

        inline ChangeVoiceFunctionTool() : FunctionTool(Statics::CHANGE_VOICE)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
