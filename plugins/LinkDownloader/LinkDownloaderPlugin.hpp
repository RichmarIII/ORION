#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(LinkDownloaderPlugin,
                   "LinkDownloader",
                   "Allows Orion to download files from the internet",
                   "**Plugin:**\n"
                   "LinkDownloader:\n"
                   "Provides Tools:\n"
                   "  - download_http_file\n"
                   "    - description: Downloads a file from the internet",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
