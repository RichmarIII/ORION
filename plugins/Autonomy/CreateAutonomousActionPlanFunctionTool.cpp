#include "CreateAutonomousActionPlanFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

using FunctionReturnResults = FunctionTool::Statics::FunctionResults;

std::string CreateAutonomousActionPlanFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    try
    {
        constexpr std::string_view NAME_PLAN_STEPS = "plan_steps";

        if (!Parameters.has_field(NAME_PLAN_STEPS.data()))
        {
            std::cout << __func__ << ": The plan_steps are required" << std::endl;

            auto Response                                                   = web::json::value::object();
            Response[FunctionReturnResults::NAME_ORION_INSTRUCTIONS.data()] = web::json::value::string("The plan_steps are required");
        }

        auto Response = web::json::value::object();
        Response[FunctionReturnResults::NAME_ORION_INSTRUCTIONS.data()] =
            web::json::value::string("The plan is complete, now follow the 'plan'.  If the conditions or context change, you may need to create a new plan.");

        auto PlanSteps    = web::json::value::object();
        PlanSteps["plan"] = Parameters.at(NAME_PLAN_STEPS.data());

        Response[FunctionReturnResults::NAME_RESULT.data()] = PlanSteps;

        return Response.serialize();
    }
    catch (const std::exception& Exception)
    {
        std::cout << "Failed to change the voice: " << Exception.what() << std::endl;

        auto ErrorObject                                       = web::json::value::object();
        ErrorObject[FunctionReturnResults::NAME_RESULT.data()] = web::json::value::string("Error: " + std::string(Exception.what()));

        return ErrorObject.serialize();
    }
}