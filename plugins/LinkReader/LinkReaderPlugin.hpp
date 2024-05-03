#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(LinkReaderPlugin,
                   "LinkReader",
                   "Allows Orion to read the contents of a web link.",
                   "**Plugin:**\n"
                   "LinkReader:\n"
                   "Provides Tools:\n"
                   "  - navigate_link\n"
                   "    - description: Reads the contents of a web link.",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
