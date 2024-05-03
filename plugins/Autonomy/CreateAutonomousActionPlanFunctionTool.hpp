#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can create an autonomous plan of action for the assistant to follow
    class CreateAutonomousActionPlanFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that creates an autonomous plan of action for the assistant to follow
            constexpr static auto CREATE_AUTONOMOUS_ACTION_PLAN = R"(
            {
                "description" : "Creates a plan of action for the assistant to follow autonomously with minimal or no user interaction. in order to complete users request.",
                "name" : "create_autonomous_action_plan",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "plan_steps" : {
                            "type" : "array",
                            "items": {
                                "type": "string"
                            },
                            "description" : "The detailed steps generated by the assistent that make up the plan of action to be followed by the assistant autonomously."
                        }
                    },
                    "required" : [ "plan_steps" ]
                }
            })";
        };

        inline CreateAutonomousActionPlanFunctionTool()
            : FunctionTool(Statics::CREATE_AUTONOMOUS_ACTION_PLAN)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION