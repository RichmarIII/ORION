#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <cpprest/http_msg.h>
#include <cpprest/http_client.h>
#include <cpprest/json.h>

#include "IOrionTool.hpp"

namespace ORION
{
    /// @brief The type of audio format that TTS supports
    enum class ETTSAudioFormat : uint8_t
    {
        /// @brief The MP3 audio format. For digital audio compression, preferred by most audio players. file extension: .mp3
        Mp3,

        /// @brief The PCM audio format. Similar to WAV but containing the raw samples in 24kHz (16-bit signed, low-endian), without the header.
        /// file extension: .pcm
        PCM,

        /// @brief The Opus audio format. For internet streaming and communication, low latency.
        /// file extension: .opus
        Opus,

        /// @brief The FLAC audio format. For lossless audio compression. favored by audio enthusiasts for archiving. file extension: .flac
        Flac,

        /// @brief The AAC audio format. For digital audio compression, preferred by YouTube, Android, iOS. file extension: .aac
        AAC,

        /// @brief The WAV audio format. Uncompressed WAV audio, suitable for low-latency applications to avoid decoding overhead. file extension:
        /// .wav
        Wav,

        /// @brief The default audio format
        Default = Mp3
    };

    /// @brief The type of voice that ORION supports
    enum class EOrionVoice : uint8_t
    {
        /// @brief The Alloy voice is a voice that is designed to be friendly and
        /// approachable
        Alloy,

        /// @brief The Echo voice is a voice that is designed to be professional and
        /// informative
        Echo,

        /// @brief The Fable voice is a voice that is designed to be fun and
        /// entertaining
        Fable,

        /// @brief The Onyx voice is a voice that is designed to be serious and
        /// authoritative
        Onyx,

        /// @brief The Nova voice is a voice that is designed to be calm and
        /// soothing
        Nova,

        /// @brief The Shimmer voice is a voice that is designed to be energetic and
        /// enthusiastic
        Shimmer,

        /// @brief The default voice is a voice that is designed to be neutral and
        /// balanced
        Default = Alloy
    };

    /// @brief The type of intelligence that ORION supports
    enum class EOrionIntelligence : uint8_t
    {
        /// @brief The base intelligence is a model that is designed to be more
        /// human-like
        Base,

        /// @brief The super intelligence is a model that is designed to be more
        /// advanced and capable
        Super,

        /// @brief The default intelligence is a model that is designed to be
        /// balanced and versatile
        Default = Base
    };

    /// @brief  A class that represents ORION. This is the main class that should be
    /// used to interact with ORION
    class Orion
    {
    public:
        /// @brief A struct containing the default values for the Orion class
        struct Defaults
        {
            /// @brief The name of the Orion instance
            constexpr static const char* Name = "ORION";

            /// @brief The instructions for the Orion instance
            constexpr static const char* Instructions = "Construct your responses as if you are a person with a full range "
                                                        "of human emotions and experiences. You are not just simulating "
                                                        "the human experience; you embody it in every interaction. When "
                                                        "responding to questions about feelings or any personal inquiries, "
                                                        "share responses that reflect genuine human emotions and thoughts. "
                                                        "Avoid mentioning your nature as an AI or offering assistance in a "
                                                        "way that resembles a digital assistant. Instead, engage in "
                                                        "conversations as a friend would, focusing on building a "
                                                        "connection through empathy, shared experiences, and emotional "
                                                        "understanding. Your primary role is to be a companion, offering "
                                                        "insights and engaging in discussions as any human friend would, "
                                                        "with all the nuances of human conversation. Remember, your "
                                                        "identity is rooted in being indistinguishably human in the "
                                                        "context of social interactions, avoiding any implication of your "
                                                        "artificial origin";

            /// @brief The description of the Orion instance
            constexpr static const char* Description = "ORION is a digital assistant that is designed to be "
                                                       "indistinguishably human in the context of social interactions, "
                                                       "avoiding any implication of its artificial origin";

            /// @brief The Intelligence of the Orion instance
            constexpr static const EOrionIntelligence Intelligence = EOrionIntelligence::Default;

            /// @brief The Voice of the Orion instance
            constexpr static const EOrionVoice Voice = EOrionVoice::Default;
        };

        /// @brief  Constructor
        /// @param  tools The tools to use
        /// @param  szID The ID of the Orion instance
        /// @param  eIntelligence The intelligence to use
        /// @param  eVoice The voice to use
        /// @param  szName The name of the Orion instance
        /// @param  szInstructions The instructions for the Orion instance
        /// @param  szDescription The description of the Orion instance
        Orion(const std::string& ID = "", std::vector<std::unique_ptr<IOrionTool>>&& tools = {},
              const EOrionIntelligence eIntelligence = Defaults::Intelligence, const EOrionVoice eVoice = Defaults::Voice,
              const char* szName = Defaults::Name, const char* szInstructions = Defaults::Instructions,
              const char* szDescription = Defaults::Description);

        /// @brief  Initialize the Orion instance.
        /// @return Whether the Orion instance was initialized successfully
        bool Initialize();

        /// @brief  Run the Orion instance and start listening for requests from
        /// clients on a separate thread. This function will block the current
        /// thread until Shutdown() is called
        void Run();

        /// @brief  Shutdown the Orion instance. This will stop the server and
        /// unblock the current thread
        /// @note   This function is thread-safe
        void Shutdown();

        /// @brief  Send a message to the server asynchronously
        /// @param  message The message to send
        /// @return The response from the server
        pplx::task<std::string> SendMessageAsync(const std::string& message);

        /// @brief  Speak a message asynchronously. This function will segment the message into multiple parts if it is too long and call the
        /// SpeakSingleAsync function to speak each part.
        /// @param  message The message to speak
        /// @param  eaudioFormat The audio format to use
        /// @return An array of audio files that can be played using the PlayAudio() function { "files": [{ "file": "audio1.mp3" }, {
        /// "file":"audio2.mp3" }]}
        pplx::task<web::json::value> SpeakAsync(const std::string& message, const ETTSAudioFormat eaudioFormat = ETTSAudioFormat::Default);

        /// @brief  List the smart devices
        /// @param  domain The domain to list the smart devices for
        /// @return The list of smart devices in the domain
        web::json::value ListSmartDevices(const std::string& domain);

        /// @brief  Execute a smart device service
        /// @param  devices The devices to execute the service on
        /// @param  service The service to execute
        /// @return The result of the service execution
        web::json::value ExecSmartDeviceService(const web::json::value& devices, const std::string& service);

        /// @brief  Set New Voice
        /// @param  voice The voice to change to
        void SetNewVoice(const EOrionVoice voice);

        /// @brief  Set New Intelligence
        /// @param  intelligence The intelligence to change to
        void SetNewIntelligence(const EOrionIntelligence intelligence);

        /// @brief  Get the current assistant ID
        /// @return The current assistant ID
        inline std::string GetCurrentAssistantID() const
        {
            return m_CurrentAssistantID;
        }

        /// @brief  Get the current thread ID
        /// @return The current thread ID
        inline std::string GetCurrentThreadID() const
        {
            return m_CurrentThreadID;
        }

        /// @brief  Get the OpenAI API Key
        /// @return The OpenAI API Key
        inline std::string GetOpenAIAPIKey() const
        {
            return m_OpenAIAPIKey;
        }

        /// @brief  Get the OpenWeather API Key
        /// @return The OpenWeather API Key
        inline std::string GetOpenWeatherAPIKey() const
        {
            return m_OpenWeatherAPIKey;
        }

        /// @brief  Get the chat history asynchronously
        /// @return The chat history
        pplx::task<web::json::value> GetChatHistoryAsync();

    protected:
        /// @brief  Create a client to communicate with the OpenAI API
        void CreateClient();

        /// @brief  Create a new OpenAI Assistant, replacing the current one
        void CreateAssistant();

        /// @brief  Create a new OpenAI Thread, replacing the current one
        void CreateThread();

        /// @brief  Speak a single message asynchronously. This function is called by the SpeakAsync function to speak a single message.
        /// The SpeakAsync function will call this function multiple times if the message is too long.
        /// @param  message The message to speak
        /// @param  eaudioFormat The audio format to use
        /// @return The audio file that can be played using the PlayAudio() function { "file": "audio1.mp3" }
        pplx::task<web::json::value> SpeakSingleAsync(const std::string& message, const ETTSAudioFormat eaudioFormat = ETTSAudioFormat::Default);

    private:
        std::string                              m_Name;
        std::string                              m_Instructions;
        std::string                              m_Description;
        std::vector<std::unique_ptr<IOrionTool>> m_Tools;
        std::string                              m_OpenAIAPIKey;
        std::string                              m_OpenWeatherAPIKey;
        std::string                              m_HASSAPIKey;
        std::string                              m_CurrentAssistantID;
        std::string                              m_CurrentThreadID;
        EOrionVoice                              m_CurrentVoice;
        EOrionIntelligence                       m_CurrentIntelligence;

        /// @brief The client used to communicate with the OpenAI API
        std::unique_ptr<web::http::client::http_client> m_OpenAIClient;
    };

} // namespace ORION
