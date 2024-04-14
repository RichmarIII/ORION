#pragma once

#include <string>
#include <map>

namespace ORION
{
    struct MimeTypes final
    {
        /// @brief Get the Mime Type for a given file name
        /// @param FileName File Name
        /// @return Mime Type
        static std::string GetMimeType(const std::string& FileName)
        {
            if (const auto HAS_EXTENSION = FileName.find_last_of('.') != std::string::npos; !HAS_EXTENSION)
            {
                return OCTET_STREAM;
            }
            else
            {
                const auto EXTENSION = FileName.substr(FileName.find_last_of('.') + 1);
                const auto MIME_TYPE = MIME_TYPES_MAP.find(EXTENSION);
                return MIME_TYPE != MIME_TYPES_MAP.end() ? MIME_TYPE->second : OCTET_STREAM;
            }
        }

    protected:
        /// @brief Default MIME type for unknown file extensions (to prevent unnecessary construction of temporary strings in GetMimeType)
        const static std::string OCTET_STREAM;

        /// @brief Map of file extensions to MIME types
        const static std::map<std::string, std::string> MIME_TYPES_MAP;
    };
} // namespace ORION