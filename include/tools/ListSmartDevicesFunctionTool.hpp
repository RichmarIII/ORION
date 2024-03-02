#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can list the smart devices in the home
    class ListSmartDevicesFunctionTool : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that lists the smart devices in the home
            constexpr static const char* ListSmartDevices = R"(
            {
                "description" : "Lists the smart devices in the home provided by home-assistant api. Must always be called before calling exec_smart_device_service to get the list of available devices.",
                "name" : "list_smart_devices",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "domain" : {
                            "type" : "string",
                            "enum" : [ "all", "light", "switch", "media_player", "climate", "sensor", "binary_sensor", "cover", "fan", "input_boolean", "input_number", "input_select", "input_text", "automation", "script", "scene", "group", "zone", "person", "device_tracker", "persistent_notification", "sun", "weather", "calendar", "camera", "alarm_control_panel", "remote", "tts", "notify", "mailbox", "updater", "sensor", "device_tracker", "automation", "script", "scene", "group", "zone", "person", "device_tracker", "persistent_notification", "sun", "weather", "calendar", "camera", "alarm_control_panel", "remote", "tts", "notify", "mailbox", "updater" ],
                            "description" : "The home-assistant domain of the smart devices to list"
                        }
                    },
                    "required" : ["domain"]
                }
            })";
        };

        inline ListSmartDevicesFunctionTool() : FunctionTool(Statics::ListSmartDevices)
        {
        }

        virtual std::string Execute(class Orion& orion, const web::json::value& parameters) override;
    };
} // namespace ORION
