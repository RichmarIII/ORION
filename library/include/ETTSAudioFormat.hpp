#pragma once

#include <cstdint>

namespace ORION
{
    /// @brief The type of audio format that TTS supports
    enum class ETTSAudioFormat : uint8_t
    {
        /// @brief The MP3 audio format. For digital audio compression, preferred by most audio players. file extension: .mp3
        MP3,

        /// @brief The PCM audio format. Similar to Wav but containing the raw samples in 24kHz (16-bit signed, low-endian), without the header.
        /// file extension: .pcm
        PCM,

        /// @brief The Opus audio format. For internet streaming and communication, low latency.
        /// file extension: .opus
        Opus,

        /// @brief The FLAC audio format. For lossless audio compression. favored by audio enthusiasts for archiving. file extension: .flac
        FLAC,

        /// @brief The AAC audio format. For digital audio compression, preferred by YouTube, Android, iOS. file extension: .aac
        AAC,

        /// @brief The Wav audio format. Uncompressed Wav audio, suitable for low-latency applications to avoid decoding overhead. file extension:
        /// .wav
        Wav,

        /// @brief The default audio format
        Default = MP3
    };
}