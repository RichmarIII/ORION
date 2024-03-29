#pragma once

#include <cpprest/http_listener.h>
#include <vector>
#include <memory>
#include "Orion.hpp"
#include "User.hpp"
#include <regex>

namespace ORION
{
    class Orion;

    /// @brief  A web server that can be used to host the Orion web interface and provide a REST API for Orion.
    /// @note   All requests must include the X-User-Id header with the id of the Orion instance you want to interact with.
    class OrionWebServer
    {
    public:
        /// @brief  This struct contains all of the string constants used in string templating throughout the Orion Web Server.
        ///         It helps prevent accidental typos and makes it easier to find and change string constants.
        ///
        ///         The string constants defined in this struct are used as templates for constructing file paths and URLs
        ///         within the Orion Web Server. They are designed to be easily customizable and allow for dynamic
        ///         substitution of values, such as the Orion instance ID.
        ///
        ///         For example, the `ORION_AUDIO_DIR_TEMPLATE` constant is used to define the directory path
        ///         where the web server will look for generated audio files specific to an Orion instance.
        ///         The `{orion_id}` placeholder within this constant will be replaced with the actual ID of the Orion instance
        ///         during runtime.
        ///
        ///         Additionally, other constants such as `STATIC_IMAGE_DIR`, `STATIC_STYLE_DIR`, `STATIC_SCRIPT_DIR`,
        ///         `STATIC_HTML_DIR`, `STATIC_FONT_DIR`, and `STATIC_AUDIO_DIR` define the directory paths for
        ///         various types of static assets (images, stylesheets, scripts, HTML files, fonts, and audio files)
        ///         that are not specific to any particular Orion instance.
        struct AssetDirectories
        {
#define ASSETS_DIR "assets"                            // The directory where the web server will look for all assets
#define ORION_ID_PLACEHOLDER "{orion_id}"              // The placeholder for the Orion id in template strings
#define AUDIO_DIR_TEMPLATE "{audio_dir}"               // The placeholder for the audio directory in template strings
#define DATABASE_DIR "database"                        // The directory where the web server will look for database files
#define USERS_DATABASE_FILE_NAME "users.db"            // The users database file
#define OPENAI_API_KEY_FILE_NAME ".openai_api_key.txt" // The file containing the OpenAI API key

            /// @brief  The root directory where the web server will look for static assets not specific to an Orion instance
            static constexpr const char* STATIC_ASSETS_DIR = ASSETS_DIR;

            /// @brief  The directory where the web server will look for static image files not specific to an Orion instance
            static constexpr const char* STATIC_IMAGES_DIR = ASSETS_DIR "/images";

            /// @brief  The directory where the web server will look for static style files not specific to an Orion instance
            static constexpr const char* STATIC_STYLES_DIR = ASSETS_DIR "/styles";

            /// @brief  The directory where the web server will look for static script files not specific to an Orion instance
            static constexpr const char* STATIC_SCRIPTS_DIR = ASSETS_DIR "/scripts";

            /// @brief  The directory where the web server will look for static html files not specific to an Orion instance
            static constexpr const char* STATIC_HTML_DIR = ASSETS_DIR "/html";

            /// @brief  The directory where the web server will look for static audio files not specific to an Orion instance
            static constexpr const char* STATIC_AUDIO_DIR = ASSETS_DIR "/audio";

            /// @brief  The directory where the web server will look for static database files
            static constexpr const char* STATIC_DATABASE_DIR = DATABASE_DIR;

            static constexpr const char* DATABASE_FILE = ASSETS_DIR "/" DATABASE_DIR "/" USERS_DATABASE_FILE_NAME;

            static constexpr const char* OPENAI_API_KEY_FILE = OPENAI_API_KEY_FILE_NAME;

            /// @brief  The directory where the web server will look for generated audio files specific to an Orion instance
            /// @note   The {orion_id} placeholder will be replaced with the id of the Orion instance
            static constexpr const char* ORION_AUDIO_DIR_TEMPLATE = AUDIO_DIR_TEMPLATE "/" ORION_ID_PLACEHOLDER "/speech";

            template <typename... Args>
            static std::string ResolveTemplate(const std::string& TemplateString, Args&&... Values)
            {
                std::regex        RegexPattern("\\{.*?\\}");
                std::stringstream ResultStream;
                std::string       Argument;
                std::stringstream ArgumentsStream;
                ((ArgumentsStream << Values << ' '), ...);
                std::istringstream ArgumentsIn(ArgumentsStream.str());

                size_t Start = 0;
                for (std::sregex_iterator Iter = std::sregex_iterator(TemplateString.begin(), TemplateString.end(), RegexPattern);
                     Iter != std::sregex_iterator(); ++Iter)
                {
                    std::smatch Match = *Iter;
                    if (std::getline(ArgumentsIn, Argument, ' '))
                    {
                        ResultStream << TemplateString.substr(Start, Match.position() - Start) << Argument;
                        Start = Match.position() + Match.length();
                    }
                }

                ResultStream << TemplateString.substr(Start);
                return ResultStream.str();
            }

            static inline std::string ResolveOrionAudioDir(const std::string& OrionId)
            {
                return ResolveTemplate(ORION_AUDIO_DIR_TEMPLATE, STATIC_AUDIO_DIR, OrionId);
            }

            static inline std::string ResolveOpenAIKeyFile()
            {
                return OPENAI_API_KEY_FILE;
            }

            /// @brief  Resolves the base asset directory for a given file extension
            /// @param  Extension The file extension
            /// @return The base asset directory (absolute path to the directory where the web server will look for assets of the given extension)
            static std::string ResolveBaseAssetDirectory(const std::string& Extension);

#undef ASSETS_DIR
#undef DATABASE_DIR
#undef USERS_DATABASE_FILE_NAME
#undef OPENAI_API_KEY_FILE_NAME
        };

        /// @brief  Destructor (virtual for inheritance)
        virtual ~OrionWebServer() = default;

        /// @brief  Start the web server
        /// @param  Port The port to listen on
        void Start(int Port);

        /// @brief  Stop the web server
        void Stop();

        /// @brief  Wait for the web server to stop
        void Wait();

    protected:
        /// @brief  Dispatches a request to the appropriate handler based on the request method and path
        /// @param  Request The HTTP request
        void HandleRequest(web::http::http_request Request);

        /// @brief  The /send_message endpoint is used to send a message to Orion. optionally converting it to markdown via the ?markdown=true query
        /// parameter
        /// @param  Request The HTTP request
        /// @example curl -X POST -d {"message": "Hello, Orion!"} http://localhost:5000/send_message
        /// @example Response: "Hello, user!"
        /// @example curl -X POST -d {"message": "Hello, Orion!"} http://localhost:5000/send_message?markdown=true
        /// @example Response: {"message": "<p>Hello, user!</p>"}
        void HandleSendMessageEndpoint(web::http::http_request Request);

        /// @brief  The / endpoint is used to serve the Orion web interface
        /// @param  Request The HTTP request
        /// @example curl -X GET http://localhost:5000/
        /// @example Response: The contents of the index.html file
        void HandleRootEndpoint(web::http::http_request Request);

        /// @brief  The /<file> endpoint is used to serve asset files (images, stylesheets, scripts, etc.)
        ///         This is a catch-all endpoint that will serve any file in the assets directory.
        /// @param  Request The HTTP request
        /// @example curl -X GET http://localhost:5000/image.png
        /// @example Response: The contents of the image.png file served from the assets directory
        void HandleAssetFileEndpoint(web::http::http_request Request);

        /// @brief  The /speech/<index> endpoint is used to serve speech asset files (audio files).
        ///         Defaults to mp3 format if no format is specified.
        /// @param Request The HTTP request
        /// @example curl -X GET http://localhost:5000/speech/{index}?format=mp3
        /// @example Response: The contents of the audio file served from the speech directory
        void HandleSpeechAssetFileEndpoint(web::http::http_request Request);

        /// @brief  The /markdown endpoint is used to convert a message to markdown
        /// @param  Request The HTTP request
        /// @example curl -X POST -d {"message": "Hello, Orion!"} http://localhost:5000/markdown
        /// @example Response: {"message": "<p>Hello, Orion!</p>"}
        void HandleMarkdownEndpoint(web::http::http_request Request);

        /// @brief  The /chat_history endpoint is used to retrieve the chat history.
        /// Optionally converting it to markdown via the ?markdown=true query parameter
        /// @param  Request The HTTP request
        /// @example curl -X GET http://localhost:5000/chat_history
        /// @example Response: [{ "role": "user", "message": "Hello, Orion!" }, { "role": "orion", "message": "Hello, user!" }]
        /// @example curl -X GET http://localhost:5000/chat_history?markdown=true
        /// @example Response: [{ "role": "user", "message": "<p>Hello, Orion!</p>" }, { "role": "orion", "message": "<p>Hello, user!</p>" }]
        void HandleChatHistoryEndpoint(web::http::http_request Request);

        /// @brief  The /speak endpoint is used to make Orion speak a message.
        /// Supported audio formats: mp3, opus, aac, flac, wav, and pcm
        /// Audio is segmented into multiple files if the message is too long.
        /// @param  Request The HTTP request
        /// @example curl -X POST -d "Hello, Orion!" http://localhost:5000/speak
        /// @example curl -X POST -d "Hello, Orion!" http://localhost:5000/speak?format=opus
        /// @example curl -X POST -d "Hello, Orion!" http://localhost:5000/speak?format=wav
        /// @note The audio file is saved to the your orion instance's speech directory. @see AssetDirectories::ResolveOrionAudioDir
        void HandleSpeakEndpoint(web::http::http_request Request);

        /// @brief  The /login endpoint is used to log in to an Orion instance.
        /// The user is returned as a JSON object with an user_id property. This id must be used in the X-User-Id header for all requests.
        /// The user can log in with an existing user id instead of username and password.
        /// @param  Request The HTTP request
        /// @example curl -X -d "{"username": "user", "password": "password"}" http://localhost:5000/login
        /// @example Response: { "user_id": "1234" }
        /// @example curl -X -d "{"user_id": "1234"}" http://localhost:5000/login
        /// @example Response: { "user_id": "1234" }
        /// @note   The user id should be stored and used in the X-User-Id header for all requests.
        void HandleLoginEndpoint(web::http::http_request Request);

        /// @brief  The /register endpoint is used to register a new user to an Orion instance.
        /// The user is returned as a JSON object with an id property. This id must be used in the X-User-Id header for all requests.
        /// @param  Request The HTTP request
        /// @example curl -X -d "{"username": "user", "password": "password"}" http://localhost:5000/register
        /// @example Response: { "id": "1234" }
        /// @note   The user id should be stored and used in the X-User-Id header for all requests.
        void HandleRegisterEndpoint(web::http::http_request Request);

        /// @brief  The /stt endpoint is used to convert speech to text.
        /// The audio file is sent in the body of the request.
        /// @param  Request The HTTP request
        /// @example curl -X POST -d @audio_file http://localhost:5000/stt
        /// @example Response: {"message": "Hello, Orion!"}
        /// example curl -X POST -d @audio_file http://localhost:5000/stt?markdown=true
        /// @example Response: {"message": "<p>Hello, Orion!</p>"}
        void HandleSpeechToTextEndpoint(web::http::http_request Request);

        /// @brief  Instantiates a new Orion instance and returns the new instance. If an Orion instance with the given id already exists on
        /// the server A new local Orion instance is created from the data of the existing server instance. If an Orion instance with the given id
        /// exists locally, the existing instance is returned. If an Orion instance with the given id does not exist, a new instance is created on the
        /// server.
        /// @param  ExistingOrionInstanceID The id of an existing Orion server instance to instantiate locally. If an Orion instance with this id
        /// does not exist, a new instance is created on the server, and a new local Orion instance is returned representing the new server instance.
        /// @return The Orion instance.
        const Orion& InstantiateOrionInstance(const std::string& ExistingOrionInstanceID = "");

        /// @brief  The orion instances that were created this session
        std::vector<std::unique_ptr<Orion>> m_OrionInstances;

        /// @brief  The listener for the web server
        web::http::experimental::listener::http_listener m_Listener;

        /// @brief  Whether the web server is running
        std::atomic<bool> m_IsRunning = false;

        /// @brief  The condition variable for the web server. Signaled when the web server is stopped
        std::condition_variable m_ConditionVariable;

        /// @brief  The mutex for the web server
        std::mutex m_Mutex;

        /// @brief  The Users that have logged in this session
        std::vector<User> m_LoggedInUsers;
    };
} // namespace ORION