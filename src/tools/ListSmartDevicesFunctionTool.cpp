#include "tools/ListSmartDevicesFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string ListSmartDevicesFunctionTool::Execute(Orion& Orion, const web::json::value& PARAMETERS)
{
    try
    {
        // Wants to list the smart devices
        std::string Domain = PARAMETERS.at("domain").as_string();
        std::string Result = Orion.ListSmartDevices(Domain).serialize();
        return Result;
    }
    catch (const std::exception& Exception)
    {
        std::cerr << "Failed to list the smart devices: " << Exception.what() << std::endl;
        return std::string(R"({"message": "Failed to list the smart devices: )") + Exception.what() + R"("})";
    }
}