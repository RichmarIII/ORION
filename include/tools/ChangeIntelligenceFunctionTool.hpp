#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can change the intelligence of the assistant
    class ChangeIntelligenceFunctionTool : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that changes (or lists) the current intelligence of the assistant
            constexpr static const char* ChangeIntelligence = R"(
            {
                "description" : "Changes (or lists) the current intelligence of the assistant. This will change the model that the assistant uses to respond to messages",
                "name" : "change_intelligence",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "intelligence" : {
                            "type" : "string",
                            "enum" : [ "base", "super" ],
                            "description" : "The intelligence to change to."
                        },
                        "list" : {
                            "type" : "boolean",
                            "enum" : [ true, false ],
                            "description" : "Whether to list the current intelligence instead of changing the intelligence."
                        }
                    },
                    "required" : ["intelligence", "list"]
                }
            })";
        };

        inline ChangeIntelligenceFunctionTool() : FunctionTool(Statics::ChangeIntelligence)
        {
        }

        virtual std::string Execute(class Orion& orion, const web::json::value& parameters) override;
    };
} // namespace ORION