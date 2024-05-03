#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(HomeAssistantPlugin,
                   "HomeAssistant",
                   "Allows Orion to interact with HomeAssistant, giving it the ability to control your smart home devices.\n",
                   "**Plugin:**\n"
                   "HomeAssistant:\n"
                   "Provides Tools:\n"
                   "  - list_smart_devices\n"
                   "    - description: Lists the smart devices in the home\n"
                   "  - exec_smart_device_service\n"
                   "    - description: Executes a smart device service in the home such as turning on/off a light",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
