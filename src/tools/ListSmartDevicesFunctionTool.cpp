#include "tools/ListSmartDevicesFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string
ListSmartDevicesFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    try
    {
        // Wants to list the smart devices
        const std::string DOMAIN = Parameters.at("domain").as_string();
        const std::string RESULT = Orion.ListSmartDevices(DOMAIN).serialize();
        return RESULT;
    }
    catch (const std::exception& Exception)
    {
        std::cerr << "Failed to list the smart devices: " << Exception.what() << std::endl;
        return std::string(R"({"message": "Failed to list the smart devices: )") + Exception.what() + R"("})";
    }
}