#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(WebSearchPlugin,
                   "WebSearch",
                   "Allows Orion to search the web for information. Not very useful without the LinkReader plugin.",
                   "**Plugin:**\n"
                   "WebSearch:\n"
                   "Provides Tools:\n"
                   "  - web_search\n"
                   "    - description: Searches the web for a query",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
