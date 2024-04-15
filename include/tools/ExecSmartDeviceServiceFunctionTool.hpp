#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that executes a smart device service
    class ExecSmartDeviceServiceFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that executes a smart device service in the home provided by home-assistant api
            constexpr static auto EXEC_SMART_DEVICE_SERVICE = R"(
            {
                "description" : "Executes a smart device service in the home provided by home-assistant api. only devices that are listed in the list_smart_devices response can be used",
                "name" : "exec_smart_device_service",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "devices" : {
                            "type" : "string",
                            "description" : "The list of home-assistant devices (retrieved from list_smart_devices) to execute the service on in the form of a json array of strings. For example, [\"light.name1\", \"light.name2\"]"
                        },
                        "service" : {
                            "type" : "string",
                            "description" : "The home-assistant service to execute on the devices. For example, \"turn_on\""
                        }
                    },
                    "required" : ["devices", "service"]
                }
            })";
        };

        inline
        ExecSmartDeviceServiceFunctionTool()
            : FunctionTool(Statics::EXEC_SMART_DEVICE_SERVICE)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
