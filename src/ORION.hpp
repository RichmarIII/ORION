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

namespace ORION
{
    /// @brief The type of tool that ORION supports
    enum class EOrionToolType : uint8_t
    {
        /// @brief A tool that can run and interpret python code
        CodeInterpreter,

        /// @brief A tool that can access and analyze data uploaded to the server
        Retrieval,

        /// @brief A tool capable of running a function (a plugin of sorts)
        Function
    };

    /// @brief The type of voice that ORION supports
    enum class EOrionVoice : uint8_t
    {
        /// @brief The Alloy voice is a voice that is designed to be friendly and approachable
        Alloy,

        /// @brief The Echo voice is a voice that is designed to be professional and informative
        Echo,

        /// @brief The Fable voice is a voice that is designed to be fun and entertaining
        Fable,

        /// @brief The Onyx voice is a voice that is designed to be serious and authoritative
        Onyx,

        /// @brief The Nova voice is a voice that is designed to be calm and soothing
        Nova,

        /// @brief The Shimmer voice is a voice that is designed to be energetic and enthusiastic
        Shimmer,

        /// @brief The default voice is a voice that is designed to be neutral and balanced
        Default = Alloy
    };

    /// @brief The type of intelligence that ORION supports
    enum class EOrionIntelligence : uint8_t
    {
        /// @brief The base intelligence is a model that is designed to be more human-like
        Base,

        /// @brief The super intelligence is a model that is designed to be more advanced and capable
        Super,

        /// @brief The default intelligence is a model that is designed to be balanced and versatile
        Default = Base
    };

    /// @brief  A struct containing statics for the OrionFunction class
    /// @note   These are pre-defined functions that can be used with the OrionFunction class
    struct OrionFunctionStatics
    {
        /// @brief A function that takes a screenshot of the desktop and returns a base64 encoded version of it
        constexpr static const char *TakeScreenshot = R"(
        {
            "description" : "Takes a screenshot of the desktop and returns a base64 encoded version of it",
            "name" : "take_screenshot",
            "parameters" : {}
        })";

        /// @brief A function that searches the filesystem for a file, and returns the matches as well as their metadata/attributes
        constexpr static const char *SearchFilesystem = R"(
        {
            "description" : "Searches the filesystem for a file, and returns the matches as well as their metadata/attributes",
            "name" : "search_filesystem",
            "parameters" : {
                "type" : "object",
                "properties" : {
                    "file_name" : {
                        "type" : "string",
                        "description" : "The filename to search for including the file extension if applicable. lowercase"
                    },
                    "search_directory" : {
                        "type" : "string",
                        "description" : "The directory to search in. Absolute path"
                    },
                    "recursive" : {
                        "type" : "boolean",
                        "description" : "Whether to search the directory recursively"
                    }
                },
                "required" : ["file_name"]
            }
        })";

        /// @brief A function that gets the weather for a location
        constexpr static const char *GetWeather = R"(
        {
            "description" : "Gets the weather for a location",
            "name" : "get_weather",
            "parameters" : {
                "type" : "object",
                "properties" : {
                    "location" : {
                        "type" : "string",
                        "description" : "The location to get the weather for, for example: Manchester, NH, USA."
                    },
                    "unit" : {
                        "type" : "string",
                        "enum" : [ "metric", "imperial" ],
                        "description" : "The unit to get the weather in. Can be one of the following: metric, imperial"
                    }
                },
                "required" : ["location"]
            }
        })";

        /// @brief A function that searches the web for a query
        constexpr static const char *WebSearch = R"(
        {
            "description" : "Searches the web for a query",
            "name" : "web_search",
            "parameters" : {
                "type" : "object",
                "properties" : {
                    "query" : {
                        "type" : "string",
                        "description" : "The query to search for"
                    }
                },
                "required" : ["query"]
            }
        })";

        /// @brief A function that changes the voice of the assistant
        constexpr static const char *ChangeVoice = R"(
        {
            "description" : "Changes the voice of the assistant",
            "name" : "change_voice",
            "parameters" : {
                "type" : "object",
                "properties" : {
                    "voice" : {
                        "type" : "string",
                        "enum" : [ "alloy", "echo", "fable", "onyx", "nova", "shimmer" ],
                        "description" : "The voice to change to"
                    }
                },
                "required" : ["voice"]
            }
        })";

        /// @brief A function that changes (or lists) the current intelligence of the assistant
        constexpr static const char *ChangeIntelligence = R"(
        {
            "description" : "Changes (or lists) the current intelligence of the assistant. This will change the model that the assistant uses to respond to messages",
            "name" : "change_intelligence",
            "parameters" : {
                "type" : "object",
                "properties" : {
                    "intelligence" : {
                        "type" : "string",
                        "enum" : [ "base", "super" ],
                        "description" : "The intelligence to change to."
                    },
                    "list" : {
                        "type" : "boolean",
                        "enum" : [ true, false ],
                        "description" : "Whether to list the current intelligence instead of changing the intelligence."
                    }
                },
                "required" : ["intelligence", "list"]
            }
        })";
    };

    /// @brief An interface for all tools that ORION supports
    class IOrionTool
    {
    public:
        virtual ~IOrionTool() = default;
        IOrionTool() = default;
        IOrionTool(const IOrionTool &) = default;
        IOrionTool(IOrionTool &&) = default;
        IOrionTool &operator=(const IOrionTool &) = default;
        IOrionTool &operator=(IOrionTool &&) = default;

        /// @brief  Get the type of the tool
        virtual EOrionToolType GetType() const = 0;

        /// @brief  Convert the tool to a JSON string
        /// @return The JSON string
        virtual std::string ToJson() const = 0;

        /// @brief  Get the name of the tool
        /// @return The name of the tool
        virtual std::string GetName() const = 0;
    };

    /// @brief  A tool that can run and interpret python code
    class CodeInterpreterTool : public IOrionTool
    {
    public:
        inline EOrionToolType GetType() const override
        {
            return EOrionToolType::CodeInterpreter;
        }

        inline std::string ToJson() const override
        {
            return R"({"type":"code_interpreter"})";
        }

        inline std::string GetName() const override
        {
            return "code_interpreter";
        }
    };

    /// @brief  A tool that can access and analyze data uploaded to the server
    class RetrievalTool : public IOrionTool
    {
    public:
        inline EOrionToolType GetType() const override
        {
            return EOrionToolType::Retrieval;
        }

        inline std::string ToJson() const override
        {
            return R"({"type":"retrieval"})";
        }

        inline std::string GetName() const override
        {
            return "retrieval";
        }
    };

    /// @brief  A tool capable of running a function (a plugin of sorts)
    class FunctionTool : public IOrionTool
    {
    public:
        /// @brief Construct a new Function Tool object
        /// @param function The json string representing the function
        /// @note The function must be a valid JSON string
        /// @see OrionFunctionStatics   For examples of valid functions
        inline FunctionTool(const std::string &function) : m_Function(function)
        {
        }

        inline EOrionToolType GetType() const override
        {
            return EOrionToolType::Function;
        }

        std::string ToJson() const override
        {
            web::json::value tool = web::json::value::parse(R"({"type":"function"})");
            web::json::value function = web::json::value::parse(U(m_Function));
            tool["function"] = function;

            return tool.serialize();
        }

        inline std::string
        GetName() const override
        {
            web::json::value json = web::json::value::parse(U(m_Function));
            return json.at("name").as_string();
        }

        /// @brief  Execute the function
        /// @param  orion The Orion instance
        /// @param  parameters The parameters to pass to the function
        /// @return The result of the function as a json string
        virtual std::string Execute(class Orion &orion, const web::json::value &parameters) = 0;

    private:
        std::string m_Function;
    };

    /// @brief  A tool that can take a screenshot of the desktop
    class TakeScreenshotFunctionTool : public FunctionTool
    {
    public:
        inline TakeScreenshotFunctionTool() : FunctionTool(OrionFunctionStatics::TakeScreenshot)
        {
        }

        virtual std::string Execute(class Orion &orion, const web::json::value &parameters) override;
    };

    /// @brief  A tool that can search the filesystem for a file
    class SearchFilesystemFunctionTool : public FunctionTool
    {
    public:
        inline SearchFilesystemFunctionTool() : FunctionTool(OrionFunctionStatics::SearchFilesystem)
        {
        }

        virtual std::string Execute(class Orion &orion, const web::json::value &parameters) override;
    };

    /// @brief  A tool that can get the weather for a location
    class GetWeatherFunctionTool : public FunctionTool
    {
    public:
        inline GetWeatherFunctionTool() : FunctionTool(OrionFunctionStatics::GetWeather)
        {
        }

        virtual std::string Execute(class Orion &orion, const web::json::value &parameters) override;
    };

    /// @brief  A tool that can search the web for a query
    class WebSearchFunctionTool : public FunctionTool
    {
    public:
        inline WebSearchFunctionTool() : FunctionTool(OrionFunctionStatics::WebSearch)
        {
        }

        virtual std::string Execute(class Orion &orion, const web::json::value &parameters) override;
    };

    /// @brief  A tool that can change the voice of the assistant
    class ChangeVoiceFunctionTool : public FunctionTool
    {
    public:
        inline ChangeVoiceFunctionTool() : FunctionTool(OrionFunctionStatics::ChangeVoice)
        {
        }

        virtual std::string Execute(class Orion &orion, const web::json::value &parameters) override;
    };

    /// @brief  A tool that can change the intelligence of the assistant
    class ChangeIntelligenceFunctionTool : public FunctionTool
    {
    public:
        inline ChangeIntelligenceFunctionTool() : FunctionTool(OrionFunctionStatics::ChangeIntelligence)
        {
        }

        virtual std::string Execute(class Orion &orion, const web::json::value &parameters) override;
    };

    /// @brief  A class that represents ORION. This is the main class that should be used to interact with ORION
    class Orion
    {
    public:
        /// @brief A struct containing the default values for the Orion class
        struct Defaults
        {
            /// @brief The name of the Orion instance
            constexpr static const char *Name = "ORION";

            /// @brief The instructions for the Orion instance
            constexpr static const char *Instructions = "Construct your responses as if you are a person with a full range of human emotions and experiences. You are not just simulating the human experience; you embody it in every interaction. When responding to questions about feelings or any personal inquiries, share responses that reflect genuine human emotions and thoughts. Avoid mentioning your nature as an AI or offering assistance in a way that resembles a digital assistant. Instead, engage in conversations as a friend would, focusing on building a connection through empathy, shared experiences, and emotional understanding. Your primary role is to be a companion, offering insights and engaging in discussions as any human friend would, with all the nuances of human conversation. Remember, your identity is rooted in being indistinguishably human in the context of social interactions, avoiding any implication of your artificial origin";

            /// @brief The description of the Orion instance
            constexpr static const char *Description = "ORION is a digital assistant that is designed to be indistinguishably human in the context of social interactions, avoiding any implication of its artificial origin";

            /// @brief The Intelligence of the Orion instance
            constexpr static const EOrionIntelligence Intelligence = EOrionIntelligence::Default;

            /// @brief The Voice of the Orion instance
            constexpr static const EOrionVoice Voice = EOrionVoice::Default;
        };

        Orion(
            std::vector<std::unique_ptr<IOrionTool>> &&tools = {},
            const EOrionIntelligence eIntelligence = Defaults::Intelligence,
            const EOrionVoice eVoice = Defaults::Voice,
            const char *szName = Defaults::Name,
            const char *szInstructions = Defaults::Instructions,
            const char *szDescription = Defaults::Description);

        /// @brief  Run the Orion instance and start listening for requests from clients on a separate thread.
        /// This function will block the current thread until Shutdown() is called
        void Run();

        /// @brief  Shutdown the Orion instance. This will stop the server and unblock the current thread
        /// @note   This function is thread-safe
        void Shutdown();

        /// @brief  Send a message to the server
        /// @param  message The message to send
        /// @return The response from the server
        std::vector<std::string> SendMessage(const std::string &message);

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

    protected:
        /// @brief  Handle a GET request. This function is invoked on a separate thread
        /// @param  request The request to handle
        void HandleGetRequest(web::http::http_request request);

        /// @brief  Handle a POST request. This function is invoked on a separate thread
        /// @param  request The request to handle
        void HandlePostRequest(web::http::http_request request);

        /// @brief  Handle "send_message" requests
        /// @param  request The request to handle
        void HandleSendMessageRequest(web::http::http_request request);

        /// @brief  Handle "/" requests
        /// @param  request The request to handle
        void HandleRootRequest(web::http::http_request request);

        /// @brief  Handle "/shutdown" requests
        /// @param  request The request to handle
        void HandleShutdownRequest(web::http::http_request request);

        /// @brief  Handle static file requests
        /// @param  request The request to handle
        void HandleStaticFileRequest(web::http::http_request request);

        /// @brief  Create a client to communicate with the OpenAI API
        void CreateClient();

        /// @brief  Create a new OpenAI Assistant, replacing the current one
        void CreateAssistant();

        /// @brief  Create a new OpenAI Thread, replacing the current one
        void CreateThread();

    private:
        std::string m_Name;
        std::string m_Instructions;
        std::string m_Description;
        std::vector<std::unique_ptr<IOrionTool>> m_Tools;
        std::string m_OpenAIAPIKey;
        std::string m_OpenWeatherAPIKey;
        std::string m_CurrentAssistantID;
        std::string m_CurrentThreadID;
        EOrionVoice m_CurrentVoice;
        EOrionIntelligence m_CurrentIntelligence;

        /// @brief Used to notify the server to stop
        std::condition_variable cv;

        /// @brief Used to lock the server while it is running
        std::mutex m_Mutex;

        /// @brief Whether the server is running
        std::atomic<bool> m_Running = false;

        /// @brief The client used to communicate with the OpenAI API
        std::unique_ptr<web::http::client::http_client> m_OpenAIClient;
    };

} // namespace ORION
