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
                "description" : "Creates an autonomous plan of action for the assistant to follow. The plan steps are a detailed list of steps that the assistant determined that it should follow in order to complete users request. Returns the detailed steps that the assistant should then execute autonomously. Created by the assistant.",
                "name" : "create_autonomous_action_plan",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "plan_steps" : {
                            "type" : "string",
                            "description" : "The detailed steps that the assistant should follow in order to complete the plan. Created by the assistant."
                        }
                    },
                    "required" : [ "plan_steps" ]
                }
            })";
        };

        inline
        CreateAutonomousActionPlanFunctionTool()
            : FunctionTool(Statics::CREATE_AUTONOMOUS_ACTION_PLAN)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
