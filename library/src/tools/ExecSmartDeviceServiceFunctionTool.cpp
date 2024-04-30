#include "tools/ExecSmartDeviceServiceFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string ExecSmartDeviceServiceFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    try
    {
        // Wants to list the smart devices
        const auto        JDEVICES = Parameters.at("devices");
        const auto        SERVICE  = Parameters.at("service").as_string();
        const std::string RESULT   = Orion.ExecSmartDeviceService(JDEVICES, SERVICE).serialize();
        return RESULT;
    }
    catch (const std::exception& Exception)
    {
        std::cerr << "Failed to execute the smart device service function: " << Exception.what() << std::endl;
        return std::string(R"({"message": "Failed to execute the smart device service function: )") + Exception.what() + "\"}";
    }
}