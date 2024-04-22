#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can change the intelligence of the assistant
    class ChangeIntelligenceFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that changes (or lists) the current intelligence of the assistant
            constexpr static auto CHANGE_INTELLIGENCE = R"(
            {
                "description" : "Changes (or lists) the current intelligence of the assistant. This will change the model that the assistant uses to respond to messages",
                "name" : "change_intelligence",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "intelligence" : {
                            "type" : "string",
                            "enum" : [ "base", "advanced" ],
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

        inline ChangeIntelligenceFunctionTool()
            : FunctionTool(Statics::CHANGE_INTELLIGENCE)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
