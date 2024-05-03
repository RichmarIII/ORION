#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(WeatherPlugin,
                   "Weather",
                   "Allows Orion to get the weather for a given location",
                   "**Plugin:**\n"
                   "Weather:\n"
                   "Provides Tools:\n"
                   "  - get_weather\n"
                   "    - description: Get the weather for a given location",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
