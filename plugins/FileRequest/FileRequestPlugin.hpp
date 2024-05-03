#pragma once

#include "Plugin.hpp"

namespace ORION
{
    DECLARE_PLUGIN(FileRequestPlugin,
                   "FileRequest",
                   "Allows Orion to request files from the user.",
                   "**Plugin:**\n"
                   "FileRequest:\n"
                   "Provides Tools:\n"
                   "  - request_file_upload_from_user\n"
                   "    - description: Requests the user to upload a file from the USER'S computer/device so that it can be used by the assistant in the future.\n",
                   "Richard Marcoux III",
                   "1.0.0")
} // namespace ORION
