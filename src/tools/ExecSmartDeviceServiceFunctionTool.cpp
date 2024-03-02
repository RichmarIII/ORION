#include "tools/ExecSmartDeviceServiceFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string ExecSmartDeviceServiceFunctionTool::Execute(Orion& orion, const web::json::value& parameters)
{
    try
    {
        // Wants to list the smart devices
        const auto  JDevices = parameters.at("devices");
        const auto  Service  = parameters.at("service").as_string();
        std::string Result   = orion.ExecSmartDeviceService(JDevices, Service).serialize();
        return Result;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to execute the smart device service function: " << e.what() << std::endl;
        return std::string(R"({"message": "Failed to execute the smart device service function"})");
    }
}