#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(IntelligencePlugin,
                   "Intelligence",
                   "Allows switching between base and advanced intelligence in a natural manner.  You can still change intelligence using the api without this plugin",
                   "**Plugin:**\n"
                   "Intelligence:\n"
                   "Provides Tools:\n"
                   "  - change_intelligence\n"
                   "    - description: This tool can be used to change (or list) the intelligence mode(s)\n"
                   "    - usage: NEVER USE THIS TOOL WITHOUT EXPLICIT USER PERMISSION **EVERY TIME**",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
