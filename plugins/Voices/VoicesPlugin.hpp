#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(VoicesPlugin,
                   "Voices",
                   "Allows Orion to switch between different voices when using text-to-speech. You can still change voices without this plugin using the api.",
                   "**Plugin:**\n"
                   "Voices:\n"
                   "Provides Tools:\n"
                   "  - change_or_list_voice\n"
                   "    - description: Changes the voice used for text-to-speech. Or lists all available voices if no voice is provided.",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
