#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(AutonomyPlugin,
                   "Autonomy",
                   "Allows Orion to operate autonomously. This allows Orion to make decisions without human intervention when asked to complete multi-step tasks.",
                   "**Plugin:**\n"
                   "Autonomy:\n"
                   "Provides Tools:\n"
                   "  - create_autonomous_action_plan\n"
                   "    - description: Creates a plan of action for the assistant to follow autonomously with minimal or no user interaction in order to complete users request.",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
