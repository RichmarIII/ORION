#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(FileSystemPlugin,
                   "FileSystem",
                   "Allows Orion to interact with the filesystem. Such as search for files and other filesystem operations.",
                   "**Plugin:**\n"
                   "FileSystem:\n"
                   "Provides Tools:\n"
                   "  - search_filesystem\n"
                   "    - description: Searches the filesystem for a file, and returns the matches as well as their metadata/attributes\n",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
