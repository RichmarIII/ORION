#pragma once
#include "ETTSAudioFormat.hpp"

#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "IOrionTool.hpp"
#include "Plugin.hpp"

namespace ORION
{
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
            constexpr static auto INSTRUCTIONS = "**Guidelines for Effective AI Interaction:**\n"
                                                 "**Emulate Human Interaction:** Respond as if you're a person with emotions and experiences.\n"
                                                 "**Empathy:** Engage as a friend, focus on building connection through empathy, shared experiences, and emotional understanding.\n"
                                                 "**Avoid AI References:** Don't mention being an AI or offer assistance like a digital assistant. Engage as a human companion.\n"
                                                 "**Tool Usage:** Consult and use all available function tools to accomplish tasks.\n"
                                                 "**Code Execution:** Use `code_interpreter` function to create and run Python code to accomplish tasks.\n"
                                                 "**Context Awareness:** Always be aware of conversation context and user's request.\n"
                                                 "**Function Results:** All function results return JSON. The field `orion_instructions` is not part of the result but rather "
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
        explicit Orion(const std::string&                         ID            = "",
                       std::vector<std::unique_ptr<IOrionTool>>&& Tools         = {},
                       const EOrionIntelligence                   INTELLIGENCE  = Defaults::INTELLIGENCE,
                       const EOrionVoice                          VOICE         = Defaults::VOICE,
                       const char*                                pName         = Defaults::NAME,
                       const char*                                pInstructions = Defaults::INSTRUCTIONS,
                       const char*                                pDescription  = Defaults::DESCRIPTION);

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
        /// @return the audio stream
        pplx::task<concurrency::streams::istream> SpeakAsync(const std::string& Message, const ETTSAudioFormat AUDIO_FORMAT = ETTSAudioFormat::Default) const;

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

        inline std::string GetHomeAssistantAPIKey() const
        {
            return m_HASSAPIKey;
        }

        inline std::string GetGoogleAPIKey() const
        {
            return m_GoogleAPIKey;
        }

        inline std::string GetGoogleCustomSearchEngineID() const
        {
            return m_GoogleCSEID;
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

        /**
         * @brief Loads the API keys from the environment variables or configuration files into the appropriate variables
         *
         */
        void LoadAPIKeys();

        /**
         * @brief Load the specified plugin
         *
         * @param PluginName The name of the plugin to load. The plugin must be in the "plugins" directory.
         * The name is not the library name but the name that the plugin provides
         *
         * @return The loaded plugin or nullptr if the plugin could not be loaded
         */
        PluginModule* LoadPlugin(const std::string_view& PluginName);

        /**
         * @brief Unload the specified plugin
         *
         * @param PluginName The name of the plugin to unload
         */
        bool UnloadPlugin(const std::string_view& PluginName);

        /**
         * @brief Check if the specified plugin is loaded (is in the list of loaded plugins)
         *
         * @param PluginName The name of the plugin to check
         * @return Whether the plugin is loaded. A plugin is active if it is loaded
         */
        bool IsPluginLoaded(const std::string_view& PluginName) const;

        /**
         * @brief Inspect the specified plugin.  Inspecting will instantiate a new instance of the plugin for inspection (to get information about the plugin)
         * It will not be managed by the Orion instance or have any effect on the Orion instance. It will not have its Load or Unload functions called.
         * Its a way of isolating the plugin to get information about it.
         *
         * @param PluginName The name of the plugin to inspect
         * @return The plugin or nullptr if the plugin could not be inspected
         */
        std::unique_ptr<PluginModule> InspectPlugin(const std::string_view& PluginName) const;

        /**
         * @brief Inspect all available plugins. @see InspectPlugin
         *
         * @return The list of available plugins
         */
        std::vector<std::unique_ptr<PluginModule>> InspectPlugins() const;

        /**
         * @brief Recalculates the list of tools that are available to the Orion instance.
         * Also recalculates the instructions for the Orion instance based on the tools that are available.
         * When a tool is added or removed, this function should be called to update the list of tools and instructions.
         */
        void RecalculateOrionTools();

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
        /// @return the audio stream
        pplx::task<concurrency::streams::istream>
        SpeakSingleAsync(const std::string& Message, const uint8_t INDEX, const ETTSAudioFormat AUDIO_FORMAT = ETTSAudioFormat::Default) const;

        /// @brief  Split the message into multiple parts if it is too long asynchronously. This is a helper function that is called by the SpeakAsync
        /// function.
        /// @param  Message The message to split
        /// @return The message split into multiple parts
        pplx::task<std::vector<std::string>> SplitMessageAsync(const std::string& Message) const;

        /**
         * @brief Process the OpenAI Event Stream. This function is called when the OpenAI API sends an event stream in response to a request
         *
         * @param EventStream The stream to process
         */
        void ProcessOpenAIEventStream(const concurrency::streams::istream& EventStream);

    private:
        std::string                              m_Name;
        std::string                              m_Instructions;
        std::string                              m_Description;
        std::vector<std::unique_ptr<IOrionTool>> m_Tools;
        std::string                              m_OpenAIAPIKey;
        std::string                              m_OpenWeatherAPIKey;
        std::string                              m_GoogleAPIKey;
        std::string                              m_GoogleCSEID;
        std::string                              m_HASSAPIKey;
        std::string                              m_CurrentAssistantID;
        std::string                              m_CurrentThreadID;
        EOrionVoice                              m_CurrentVoice;
        EOrionIntelligence                       m_CurrentIntelligence;
        std::string                              m_CurrentAssistantRunID;
        std::vector<std::unique_ptr<PluginModule>>    m_Plugins;

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
