#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(MemoryPlugin,
                   "Memory",
                   "Allows Orion to remember things. Gives Orion a long-term memory.",
                   "**Plugin:**\n"
                   "Memory:\n"
                   "Provides Tools:\n"
                   "  - remember_knowledge\n"
                   "    - description: Remember a piece of knowledge.\n"
                   "  - recall_knowledge\n"
                   "    - description: Recall a piece of knowledge.\n"
                   "  - update_knowledge\n"
                   "    - description: Update a piece of knowledge.\n",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
