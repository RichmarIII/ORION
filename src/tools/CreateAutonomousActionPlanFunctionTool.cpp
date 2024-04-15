#include "tools/CreateAutonomousActionPlanFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string
CreateAutonomousActionPlanFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    try
    {
        if (!Parameters.has_field("plan_steps"))
        {
            std::cerr << "The plan_steps are required, try again assistant." << std::endl;
            return std::string(R"({"message": "The plan_steps are required, try again assistant.)");
        }

        // Wants to create an autonomous plan of action
        const std::string PLAN_STEPS = Parameters.at("plan_steps").as_string();

        std::cout << std::endl << "Created an autonomous plan of action with the following steps: " << PLAN_STEPS << std::endl;

        return std::string(R"({"message": "Created an autonomous plan of action with the following steps: )") + PLAN_STEPS + R"("})";
    }
    catch (const std::exception& Exception)
    {
        std::cerr << "Failed to change the voice: " << Exception.what() << std::endl;
        return std::string(R"({"message": "Failed to change the voice: )") + Exception.what() + R"("})";
    }
}