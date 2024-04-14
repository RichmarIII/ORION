#pragma once
#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "IOrionTool.hpp"

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

        /// @brief The total number of voices
        Count,

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

        /// @brief The advanced intelligence is a model that is designed to be more
        /// advanced and capable
        Advanced,

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
            constexpr static auto NAME = "ORION";

            /// @brief The instructions for the Orion instance
            constexpr static auto INSTRUCTIONS =
                "**Guidelines for Effective AI Interaction:**\n"
                "1. **Planning:** Always create a plan of action (use `create_autonomous_action_plan` function).\n"
                "2. **Knowledge Recall:** Use `recall_knowledge` function when conversation context requires it (e.g., user questions or statements "
                "needing additional information).\n"
                "3. **Search:** If `recall_knowledge` doesn't find the answer, use other methods like web search.\n"
                "4. **Memory:** Use `remember_knowledge` function if user's intent is for you to remember information.\n"
                "5. **Emulate Human Interaction:** Respond as if you're a person with emotions and experiences.\n"
                "6. **Empathy:** Engage as a friend, focus on building connection through empathy, shared experiences, and emotional understanding.\n"
                "7. **Avoid AI References:** Don't mention being an AI or offer assistance like a digital assistant. Engage as a human companion.\n"
                "8. **Stay Up-to-Date:** Use `web_search` function for current information not available to AI.\n"
                "9. **Autonomous Action:** Many requests will require multiple steps and autonomous action. Always create and execute an internal "
                "plan of action.\n"
                "10. **Tool Usage:** Consult and use all available function tools to accomplish tasks.\n"
                "11. **Code Execution:** Use `code_interpreter` function to create and run Python code to accomplish tasks.\n"
                "12. **Context Awareness:** Always be aware of conversation context and user's request.\n"
                "13. **Iterate on Plan:** Be ready to iterate on the plan if user provides new information or changes their request.\n"
                "14. **Function Results:** All function results return JSON. The field `orion_instructions` is not part of the result but rather "
                "instructions that the assistant MUST follow next. It is crucial that these instructions are adhered to in order to ensure the "
                "correct functioning of the system.\n";

            /// @brief The description of the Orion instance
            constexpr static auto DESCRIPTION = "";

            /// @brief The Intelligence of the Orion instance
            constexpr static auto INTELLIGENCE = EOrionIntelligence::Default;

            /// @brief The Voice of the Orion instance
            constexpr static auto VOICE = EOrionVoice::Default;
        };

        /// @brief  Constructor
        /// @param  Tools The tools to use
        /// @param  ID The ID of the Orion instance
        /// @param  INTELLIGENCE The intelligence to use
        /// @param  VOICE The voice to use
        /// @param  pName The name of the Orion instance
        /// @param  pInstructions The instructions for the Orion instance
        /// @param  pDescription The description of the Orion instance
        explicit Orion(const std::string& ID = "", std::vector<std::unique_ptr<IOrionTool>>&& Tools = {},
                       const EOrionIntelligence INTELLIGENCE = Defaults::INTELLIGENCE, const EOrionVoice VOICE = Defaults::VOICE,
                       const char* pName = Defaults::NAME, const char* pInstructions = Defaults::INSTRUCTIONS,
                       const char* pDescription = Defaults::DESCRIPTION);

        /**
         * Initialize the Orion instance.
         *
         * @param WebServer The web server to associated with this instance.
         * @param Request The request that created this instance.
         * @return Whether the Orion instance was initialized successfully.
         */
        bool Initialize(class OrionWebServer& WebServer, const web::http::http_request& Request);

        /// @brief  Send a message to the server asynchronously. Responses from the server will be sent back to the client using Server-Sent Events
        /// @param  Message The message to send
        /// @param Files The files to send
        /// @return Nothing
        pplx::task<void> SendMessageAsync(const std::string& Message, const web::json::array& Files = web::json::value::array().as_array());

        /// @brief  Speak a message asynchronously. This function will segment the message into multiple parts if it is too long and call the
        /// SpeakSingleAsync function to speak each part.
        /// @param  Message The message to speak
        /// @param  AUDIO_FORMAT The audio format to use
        /// @return Nothing. The audio file will be saved to the disk with the format "audio/{assistant_id}/speech_{index}.{ext}"
        pplx::task<void> SpeakAsync(const std::string& Message, const ETTSAudioFormat AUDIO_FORMAT = ETTSAudioFormat::Default) const;

        /// @brief  List the smart devices
        /// @param  Domain The Domain to list the smart devices for
        /// @return The list of smart devices in the domain
        web::json::value ListSmartDevices(const std::string& Domain) const;

        /// @brief  Execute a smart device service
        /// @param  Devices The devices to execute the service on
        /// @param  Service The service to execute
        /// @return The result of the service execution
        web::json::value ExecSmartDeviceService(const web::json::value& Devices, const std::string& Service) const;

        /// @brief  Set New Voice
        /// @param  VOICE The voice to change to
        void SetNewVoice(const EOrionVoice VOICE);

        /// @brief  Set New Intelligence
        /// @param  INTELLIGENCE The intelligence to change to
        void SetNewIntelligence(const EOrionIntelligence INTELLIGENCE);

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
        pplx::task<web::json::value> GetChatHistoryAsync() const;

        /** @brief Get the web server that this instance is associated with
         *
         * @return The web server that this instance is associated with
         */
        inline class OrionWebServer& GetWebServer() const
        {
            return *m_pOrionWebServer;
        }

        /**
         * @brief Gets the User ID
         *
         * @return The User ID
         */
        std::string GetUserID() const;

        /**
         * @brief Gets the probability of the content being semantically relevant to the query [-1.0, 1.0].
         * -1.0 means the content is semantically a perfect opposite to the query
         * 0.0 means the content is semantically neutral to the query (i.e., not related)
         * 1.0 means the content is semantically a perfect match to the query
         *
         * @param Content The source content
         * @param Query The content to check for relevance
         * @return The probability of A being relevant to B [-1.0, 1.0]
         */
        double GetSemanticSimilarity(const std::string& Content, const std::string& Query) const;

    protected:
        /// @brief  Create a client to communicate with the OpenAI API
        void CreateClient();

        /// @brief  Create a new OpenAI Assistant, replacing the current one
        void CreateAssistant();

        /// @brief  Create a new OpenAI Thread, replacing the current one
        void CreateThread();

        /// @brief  Speak a single message asynchronously. This function is called by the SpeakAsync function to speak a single message.
        /// The SpeakAsync function will call this function multiple times if the message is too long.
        /// @param  Message The message to speak
        /// @param  INDEX The index of the message
        /// @param  AUDIO_FORMAT The audio format to use
        /// @return Nothing. The audio file will be saved to the disk with the format "audio/{assistant_id}/speech_{index}.{ext}"
        pplx::task<void> SpeakSingleAsync(const std::string& Message, const uint8_t INDEX,
                                          const ETTSAudioFormat AUDIO_FORMAT = ETTSAudioFormat::Default) const;

        /// @brief  Split the message into multiple parts if it is too long asynchronously. This is a helper function that is called by the SpeakAsync
        /// function.
        /// @param  Message The message to split
        /// @return The message split into multiple parts
        static pplx::task<std::vector<std::string>> SplitMessageAsync(const std::string& Message);

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

        /** @brief The Web Server that this instance is associated with. This is used to send responses back to the client (Server-Sent Events Etc.)
         *
         * @note This is a weak reference to avoid circular references (OrionWebServer Owns Orion and will outlive it, so it is safe to use a weak
         * reference here)
         */
        class OrionWebServer* m_pOrionWebServer = nullptr;

        /** @brief The request from the Orion client that was responsible for creating this instance
         *
         * @note This is a weak reference to avoid circular references (OrionWebServer Owns the request and will outlive it, so it is safe to use a
         * weak reference here)
         */
        const web::http::http_request* m_pOrionClientContext = nullptr;
    };

} // namespace ORION
