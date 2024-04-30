#pragma once

#include "ETTSAudioFormat.hpp"

#include <map>
#include <string>

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

            const auto EXTENSION = FileName.substr(FileName.find_last_of('.') + 1);
            const auto MIME_TYPE = MIME_TYPES_MAP.find(EXTENSION);
            return MIME_TYPE != MIME_TYPES_MAP.end() ? MIME_TYPE->second : OCTET_STREAM;
        }

        static std::string GetMimeType(const ETTSAudioFormat AUDIO_FORMAT)
        {
            switch (AUDIO_FORMAT)
            {
                case ETTSAudioFormat::Opus:
                    return MIME_TYPES_MAP.at("opus");
                case ETTSAudioFormat::Wav:
                    return MIME_TYPES_MAP.at("wav");
                case ETTSAudioFormat::MP3:
                    return MIME_TYPES_MAP.at("mp3");
                case ETTSAudioFormat::AAC:
                    return MIME_TYPES_MAP.at("aac");
                case ETTSAudioFormat::PCM:
                    return MIME_TYPES_MAP.at("pcm");
                case ETTSAudioFormat::FLAC:
                    return MIME_TYPES_MAP.at("flac");
                default:
                    return OCTET_STREAM;
            }
        }

    protected:
        /// @brief Default MIME type for unknown file extensions (to prevent unnecessary construction of temporary strings in GetMimeType)
        const static std::string OCTET_STREAM;

        /// @brief Map of file extensions to MIME types
        const static std::map<std::string, std::string> MIME_TYPES_MAP;
    };
} // namespace ORION