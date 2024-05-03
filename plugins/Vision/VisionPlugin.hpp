#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(VisionPlugin,
                   "Vision",
                   "Allows Orion to visually perceive the world around it and describe what it sees. Also allows for taking screenshots.",
                   "**Plugin:**\n"
                   "Vision:\n"
                   "Provides Tools:\n"
                   "  - take_screenshot\n"
                   "    - description: Takes a screenshot of the desktop and returns a base64 encoded version of it\n",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
