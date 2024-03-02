#include "tools/ListSmartDevicesFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string ListSmartDevicesFunctionTool::Execute(Orion& orion, const web::json::value& parameters)
{
    try
    {
        // Wants to list the smart devices
        std::string Domain = parameters.at("domain").as_string();
        std::string Result = orion.ListSmartDevices(Domain).serialize();
        return Result;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to list the smart devices: " << e.what() << std::endl;
        return std::string(R"({"message": "Failed to list the smart devices: )") + e.what() + R"("})";
    }
}