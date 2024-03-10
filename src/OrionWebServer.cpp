#include "OrionWebServer.hpp"
#include "Orion.hpp"
#include "tools/ChangeIntelligenceFunctionTool.hpp"
#include "tools/ChangeVoiceFunctionTool.hpp"
#include "tools/CodeInterpreterTool.hpp"
#include "tools/GetWeatherFunctionTool.hpp"
#include "tools/RetrievalTool.hpp"
#include "tools/SearchFilesystemFunctionTool.hpp"
#include "tools/TakeScreenshotFunctionTool.hpp"
#include "tools/WebSearchFunctionTool.hpp"
#include "tools/ListSmartDevicesFunctionTool.hpp"
#include "tools/ExecSmartDeviceServiceFunctionTool.hpp"
#include "MimeTypes.hpp"
#include "User.hpp"
#include "GUID.hpp"

#include <cmark.h>

#include <cpprest/filestream.h>

#include <filesystem>

#include <sqlite_modern_cpp.h>

using namespace ORION;

std::string OrionWebServer::AssetDirectories::ResolveBaseAssetDirectory(const std::string& Extension)
{
    // Get the mime type
    const auto MIME_TYPE = MimeTypes::GetMimeType(Extension);

    // Get the current application directory
    const auto APP_DIR = std::filesystem::current_path();

    // Image files
    if (MIME_TYPE.find("image") != std::string::npos)
    {
        return APP_DIR / STATIC_IMAGES_DIR;
    }
    // Audio files
    else if (MIME_TYPE.find("audio") != std::string::npos)
    {
        return APP_DIR / STATIC_AUDIO_DIR;
    }
    // Script files
    else if (MIME_TYPE.find("javascript") != std::string::npos)
    {
        return APP_DIR / STATIC_SCRIPTS_DIR;
    }
    // Stylesheets
    else if (MIME_TYPE.find("css") != std::string::npos)
    {
        return APP_DIR / STATIC_STYLES_DIR;
    }
    // HTML files
    else if (MIME_TYPE.find("html") != std::string::npos)
    {
        return APP_DIR / STATIC_HTML_DIR;
    }
    // Other files
    else
    {
        return APP_DIR / STATIC_ASSETS_DIR;
    }
}

void OrionWebServer::Start(int Port)
{
    web::http::experimental::listener::http_listener_config ListenerConfig;
    ListenerConfig.set_ssl_context_callback(
        [this](boost::asio::ssl::context& Ctx)
        {
            Ctx.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
                            boost::asio::ssl::context::no_sslv3 | boost::asio::ssl::context::single_dh_use);
            Ctx.set_password_callback([](std::size_t MaxLength, boost::asio::ssl::context::password_purpose Purpose) { return "test"; });
            Ctx.use_certificate_chain_file("cert.pem");
            Ctx.use_private_key_file("key.pem", boost::asio::ssl::context::pem);
        });

    // Create a listener
    m_Listener = web::http::experimental::listener::http_listener(U("https://0.0.0.0:") + std::to_string(Port), ListenerConfig);

    // Handle requests
    m_Listener.support(web::http::methods::POST, std::bind(&OrionWebServer::HandleRequest, this, std::placeholders::_1));
    m_Listener.support(web::http::methods::GET, std::bind(&OrionWebServer::HandleRequest, this, std::placeholders::_1));

    // Start the listener
    m_Listener.open().wait();

    // Set the running flag
    m_IsRunning = true;
}

void OrionWebServer::Stop()
{
    // Close the listener
    m_IsRunning = false;
    m_ConditionVariable.notify_one();
}

void OrionWebServer::Wait()
{
    // Aquire the lock
    std::unique_lock<std::mutex> Lock(m_Mutex);

    // Wait for the condition variable
    m_ConditionVariable.wait(Lock, [this] { return !m_IsRunning; });

    // Close the listener
    m_Listener.close().wait();
}

void OrionWebServer::HandleRequest(web::http::http_request Request)
{
    // Get the request path
    auto Path = Request.request_uri().path();

    // Dispatch the request to the appropriate handler
    if (Path == U("/send_message"))
    {
        HandleSendMessageEndpoint(Request);
    }
    else if (Path == U("/"))
    {
        HandleRootEndpoint(Request);
    }
    else if (Path == U("/markdown"))
    {
        HandleMarkdownEndpoint(Request);
    }
    else if (Path == U("/chat_history"))
    {
        HandleChatHistoryEndpoint(Request);
    }
    else if (Path == U("/speak"))
    {
        HandleSpeakEndpoint(Request);
    }
    else if (Path == U("/login"))
    {
        HandleLoginEndpoint(Request);
    }
    else if (Path == U("/register"))
    {
        HandleRegisterEndpoint(Request);
    }
    else if (Path == U("/stt"))
    {
        HandleSpeechToTextEndpoint(Request);
    }
    else if (Path.find(U("/speech/")) != std::string::npos)
    {
        HandleSpeechAssetFileEndpoint(Request);
    }
    else
    {
        HandleAssetFileEndpoint(Request);
    }
}

void OrionWebServer::HandleSendMessageEndpoint(web::http::http_request Request)
{
    // Check for the X-User-Id header
    if (!Request.headers().has(U("X-User-Id")))
    {
        Request.reply(web::http::status_codes::BadRequest, U("The X-User-Id header is required."));
        return;
    }

    // Get the Orion instance id from the header
    const auto USER_ID = Request.headers().find(U("X-User-Id"))->second;

    // Find User in the list of logged in users
    const auto USER_ITER =
        std::find_if(m_LoggedInUsers.begin(), m_LoggedInUsers.end(), [USER_ID](const User& User) { return User.UserID == USER_ID; });
    if (USER_ITER == m_LoggedInUsers.end())
    {
        Request.reply(web::http::status_codes::Unauthorized, U("User is not logged in."));
        return;
    }

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(), m_OrionInstances.end(),
                                [USER_ITER](const std::unique_ptr<Orion>& Orion) { return Orion->GetCurrentAssistantID() == USER_ITER->OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        Request.reply(web::http::status_codes::BadRequest, U("The Orion instance with the given id was not found."));
        return;
    }

    // Check if markdown is requested
    const bool IS_MARKDOWN_REQUESTED = Request.request_uri().query().find(U("markdown=true")) != std::string::npos;

    // Get the message from the request body
    Request.extract_json()
        .then([](pplx::task<web::json::value> ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [OrionIt, IS_MARKDOWN_REQUESTED, Request](web::json::value RequestMessageJson)
            {
                // Get the message from the request body
                auto RequestMessage = RequestMessageJson.at(U("message")).as_string();
                (*OrionIt)
                    ->SendMessageAsync(RequestMessage)
                    .then([](pplx::task<std::string> SendMessageTask) { return SendMessageTask.get(); })
                    .then(
                        [IS_MARKDOWN_REQUESTED, Request](std::string SendMessageResponse)
                        {
                            // Convert the message to markdown if requested
                            if (IS_MARKDOWN_REQUESTED)
                            {
                                auto pMarkdown = cmark_markdown_to_html(SendMessageResponse.c_str(), SendMessageResponse.length(), CMARK_OPT_UNSAFE);
                                SendMessageResponse = pMarkdown;
                                free(pMarkdown);
                            }

                            web::json::value SendMessageResponseJson = web::json::value::object();
                            SendMessageResponseJson[U("message")]    = web::json::value::string(SendMessageResponse);

                            // Send the response
                            Request.reply(web::http::status_codes::OK, SendMessageResponseJson);
                        });
            });
}

void OrionWebServer::HandleRootEndpoint(web::http::http_request Request)
{
    constexpr auto INDEX_HTML {U("index.html")};
    const auto     INDEX_HTML_PATH = std::filesystem::path(AssetDirectories::ResolveBaseAssetDirectory(INDEX_HTML)) / INDEX_HTML;

    // Find the index.html file
    std::ifstream IndexFile {INDEX_HTML_PATH};
    if (!IndexFile.is_open())
    {
        Request.reply(web::http::status_codes::NotFound, U("The index.html file was not found."));
        return;
    }

    // Read the contents of the index.html file
    std::string IndexContents {std::istreambuf_iterator<char>(IndexFile), std::istreambuf_iterator<char>()};

    // Send the contents of the index.html file
    const auto MIME_TYPE = MimeTypes::GetMimeType(INDEX_HTML);
    Request.reply(web::http::status_codes::OK, IndexContents, MIME_TYPE);
}

void OrionWebServer::HandleAssetFileEndpoint(web::http::http_request Request)
{
    // Get the file name from the request path and make it relative (remove the leading slash)
    const auto FILE_NAME {std::filesystem::path(Request.request_uri().path()).relative_path()};

    // Calculate file path based on the file extension
    const std::string FILE_PATH {AssetDirectories::ResolveBaseAssetDirectory(FILE_NAME) / FILE_NAME};

    // Get mime type from file extension
    const std::string CONTENT_TYPE {MimeTypes::GetMimeType(FILE_PATH)};

    // Stream the file to the response
    concurrency::streams::fstream::open_istream(FILE_PATH)
        .then(
            [Request, CONTENT_TYPE](concurrency::streams::istream StaticFileInputStream)
            {
                // Send the response
                Request.reply(web::http::status_codes::OK, StaticFileInputStream, CONTENT_TYPE);
            })
        .then(
            [FILE_PATH, Request](pplx::task<void> OpenStreamTask)
            {
                try
                {
                    OpenStreamTask.get();
                }
                catch (std::exception& Exception)
                {
                    std::cout << U("The file ") << FILE_PATH << U(" was not found: ") << std::endl;

                    Request.reply(web::http::status_codes::NotFound,
                                  U("The file ") + FILE_PATH + U(" was not found: ") + std::string(Exception.what()));
                }
            });
}

void OrionWebServer::HandleSpeechAssetFileEndpoint(web::http::http_request Request)
{
    // Get Orion ID from the request header
    const auto USER_ID {Request.headers().find(U("X-User-Id"))->second};

    if (USER_ID.empty())
    {
        Request.reply(web::http::status_codes::BadRequest, U("The X-User-Id header is required."));
        return;
    }

    // Find the Logged in user
    const auto USER_ITER =
        std::find_if(m_LoggedInUsers.begin(), m_LoggedInUsers.end(), [USER_ID](const User& User) { return User.UserID == USER_ID; });

    const auto ORION_ID = USER_ITER->OrionID;

    // get the audio format from the query parameter
    std::string AudioFormat = "mp3";
    if (Request.request_uri().query().find(U("format=opus")) != std::string::npos)
    {
        AudioFormat = "opus";
    }
    else if (Request.request_uri().query().find(U("format=flac")) != std::string::npos)
    {
        AudioFormat = "flac";
    }
    else if (Request.request_uri().query().find(U("format=aac")) != std::string::npos)
    {
        AudioFormat = "aac";
    }
    else if (Request.request_uri().query().find(U("format=pcm")) != std::string::npos)
    {
        AudioFormat = "pcm";
    }
    else if (Request.request_uri().query().find(U("format=wav")) != std::string::npos)
    {
        AudioFormat = "wav";
    }

    // Get the index from the request path
    const auto SPEECH_INDEX {Request.request_uri().path().substr(Request.request_uri().path().find_last_of('/') + 1)};

    // The speech file name
    const auto SPEECH_FILE_NAME {SPEECH_INDEX + "." + AudioFormat};

    // Speech file path
    const std::string SPEECH_FILE_PATH = std::filesystem::path(AssetDirectories::ResolveOrionAudioDir(ORION_ID)) / SPEECH_FILE_NAME;

    // Get mime type from file extension
    const auto CONTENT_TYPE {MimeTypes::GetMimeType(SPEECH_FILE_PATH)};

    // Stream the file to the response
    concurrency::streams::fstream::open_istream(SPEECH_FILE_PATH)
        .then(
            [Request, CONTENT_TYPE](concurrency::streams::istream StaticFileInputStream)
            {
                // Send the response
                Request.reply(web::http::status_codes::OK, StaticFileInputStream, CONTENT_TYPE);
            })
        .then(
            [SPEECH_FILE_PATH, Request](pplx::task<void> OpenStreamTask)
            {
                try
                {
                    OpenStreamTask.get();
                }
                catch (std::exception& Exception)
                {
                    std::cout << U("The file ") << SPEECH_FILE_PATH << U(" was not found: ") << std::endl;

                    Request.reply(web::http::status_codes::NotFound,
                                  U("The file ") + SPEECH_FILE_PATH + U(" was not found: ") + std::string(Exception.what()));
                }
            });
}

void OrionWebServer::HandleMarkdownEndpoint(web::http::http_request Request)
{
    // Get the message from the request body
    Request.extract_json()
        .then([this, Request](pplx::task<web::json::value> ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [this, Request](web::json::value JsonRequestBody)
            {
                // Get the message from the request body
                auto RequestMessage = JsonRequestBody.at(U("message")).as_string();

                // Convert the message to markdown
                {
                    auto pMarkdown = cmark_markdown_to_html(RequestMessage.c_str(), RequestMessage.length(), CMARK_OPT_UNSAFE);
                    RequestMessage = pMarkdown;
                    free(pMarkdown);
                }

                web::json::value Response = web::json::value::object();
                Response[U("message")]    = web::json::value::string(RequestMessage);

                // Send the response
                Request.reply(web::http::status_codes::OK, Response);
            });
}

void OrionWebServer::HandleChatHistoryEndpoint(web::http::http_request Request)
{
    // Check for the X-User-Id header
    if (!Request.headers().has(U("X-User-Id")))
    {
        web::json::value Response = web::json::value::object();
        Response[U("message")]    = web::json::value::string(U("The X-User-Id header is required."));
        Request.reply(web::http::status_codes::BadRequest, Response);
        return;
    }

    // Get the Orion instance id from the header
    const auto USER_ID = Request.headers().find(U("X-User-Id"))->second;

    // Find User in the list of logged in users
    const auto USER_ITER =
        std::find_if(m_LoggedInUsers.begin(), m_LoggedInUsers.end(), [USER_ID](const User& User) { return User.UserID == USER_ID; });
    if (USER_ITER == m_LoggedInUsers.end())
    {
        web::json::value Response = web::json::value::object();
        Response[U("message")]    = web::json::value::string(U("User is not logged in."));
        Request.reply(web::http::status_codes::Unauthorized, Response);
        return;
    }

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(), m_OrionInstances.end(),
                                [USER_ITER](const std::unique_ptr<Orion>& Orion) { return Orion->GetCurrentAssistantID() == USER_ITER->OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        web::json::value Response = web::json::value::object();
        Response[U("message")]    = web::json::value::string(U("Could not find an Orion instance for the given user."));
        Request.reply(web::http::status_codes::BadRequest, Response);
        return;
    }

    // Get the chat history
    (*OrionIt)->GetChatHistoryAsync().then(
        [this, Request](web::json::value ChatHistory)
        {
            // Check if the query parameter is present
            bool IsMarkdownRequested = Request.request_uri().query().find(U("markdown=true")) != std::string::npos;

            // Convert the chat history to markdown if the query parameter is present
            if (IsMarkdownRequested)
            {
                for (auto& JMessage : ChatHistory.as_array())
                {
                    auto Message           = JMessage.at(U("message")).as_string();
                    auto pMarkdown         = cmark_markdown_to_html(Message.c_str(), Message.length(), CMARK_OPT_UNSAFE);
                    JMessage[U("message")] = web::json::value::string(pMarkdown);
                    JMessage[U("role")]    = web::json::value::string(JMessage.at(U("role")).as_string() == U("user") ? U("user") : U("assistant"));
                    free(pMarkdown);
                }
            }

            // Send the response
            Request.reply(web::http::status_codes::OK, ChatHistory);
        });
}

void OrionWebServer::HandleSpeakEndpoint(web::http::http_request Request)
{
    // Check for the X-User-Id header
    if (!Request.headers().has(U("X-User-Id")))
    {
        Request.reply(web::http::status_codes::BadRequest, U("The X-User-Id header is required."));
        return;
    }

    // Get the Orion instance id from the header
    const auto USER_ID = Request.headers().find(U("X-User-Id"))->second;

    // Find User in the list of logged in users
    const auto USER_ITER =
        std::find_if(m_LoggedInUsers.begin(), m_LoggedInUsers.end(), [USER_ID](const User& User) { return User.UserID == USER_ID; });
    if (USER_ITER == m_LoggedInUsers.end())
    {
        Request.reply(web::http::status_codes::Unauthorized, U("User is not logged in."));
        return;
    }

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(), m_OrionInstances.end(),
                                [USER_ITER](const std::unique_ptr<Orion>& Orion) { return Orion->GetCurrentAssistantID() == USER_ITER->OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        Request.reply(web::http::status_codes::BadRequest, U("The Orion instance with the given id was not found."));
        return;
    }

    // Get the message from the request body
    Request.extract_json()
        .then([this, OrionIt, Request](pplx::task<web::json::value> ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [this, OrionIt, Request](web::json::value RequestMessageJson)
            {
                // Get the audio format from the query parameter
                ETTSAudioFormat AudioFormat = ETTSAudioFormat::MP3;
                if (Request.request_uri().query().find(U("format=opus")) != std::string::npos)
                {
                    AudioFormat = ETTSAudioFormat::Opus;
                }
                else if (Request.request_uri().query().find(U("format=flac")) != std::string::npos)
                {
                    AudioFormat = ETTSAudioFormat::FLAC;
                }
                else if (Request.request_uri().query().find(U("format=aac")) != std::string::npos)
                {
                    AudioFormat = ETTSAudioFormat::AAC;
                }
                else if (Request.request_uri().query().find(U("format=pcm")) != std::string::npos)
                {
                    AudioFormat = ETTSAudioFormat::PCM;
                }
                else if (Request.request_uri().query().find(U("format=wav")) != std::string::npos)
                {
                    AudioFormat = ETTSAudioFormat::Wav;
                }

                const auto REQUEST_MESSAGE = RequestMessageJson.at(U("message")).as_string();

                // Make Orion speak the message
                (*OrionIt)
                    ->SpeakAsync(REQUEST_MESSAGE, AudioFormat)
                    .then(
                        [this, Request]()
                        {
                            // Send the response
                            Request.reply(web::http::status_codes::OK);
                        });
            });
}

const Orion& OrionWebServer::InstantiateOrionInstance(const std::string& ExistingOrionInstanceID)
{
    // Create a new Orion instance

    // Declare Default Tools
    std::vector<std::unique_ptr<IOrionTool>> Tools {};
    Tools.reserve(10);

    Tools.push_back(std::make_unique<TakeScreenshotFunctionTool>());
    Tools.push_back(std::make_unique<SearchFilesystemFunctionTool>());
    Tools.push_back(std::make_unique<GetWeatherFunctionTool>());
    Tools.push_back(std::make_unique<WebSearchFunctionTool>());
    Tools.push_back(std::make_unique<ChangeVoiceFunctionTool>());
    Tools.push_back(std::make_unique<ChangeIntelligenceFunctionTool>());
    Tools.push_back(std::make_unique<RetrievalTool>());
    Tools.push_back(std::make_unique<CodeInterpreterTool>());
    Tools.push_back(std::make_unique<ListSmartDevicesFunctionTool>());
    Tools.push_back(std::make_unique<ExecSmartDeviceServiceFunctionTool>());

    // Check if the Orion instance already exists locally (Only one Orion instance is allowed per user)
    auto OrionIt = std::find_if(m_OrionInstances.begin(), m_OrionInstances.end(),
                                [ExistingOrionInstanceID](const std::unique_ptr<Orion>& Orion)
                                { return Orion->GetCurrentAssistantID() == ExistingOrionInstanceID; });

    // Check if the Orion instance was found locally
    if (OrionIt != m_OrionInstances.end())
    {
        return **OrionIt;
    }

    // Create the Orion instance
    auto NewOrion = std::make_unique<Orion>(ExistingOrionInstanceID, std::move(Tools));

    // Initialize the Orion instance
    NewOrion->Initialize();

    auto& OrionInstance = *NewOrion;

    // Add the Orion instance to the list
    m_OrionInstances.push_back(std::move(NewOrion));

    return OrionInstance;
}

void OrionWebServer::HandleLoginEndpoint(web::http::http_request Request)
{
    // Get the username and password from the request body
    Request.extract_json()
        .then([this, Request](pplx::task<web::json::value> ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [this, Request](web::json::value JsonRequestBody)
            {
                // Get the username and password from the request body
                auto Username = JsonRequestBody.has_field(U("username")) ? JsonRequestBody.at(U("username")).as_string() : U("");
                auto Password = JsonRequestBody.has_field(U("password")) ? JsonRequestBody.at(U("password")).as_string() : U("");
                auto UserID   = JsonRequestBody.has_field(U("user_id")) ? JsonRequestBody.at(U("user_id")).as_string() : U("");

                // Get the database path
                const auto DB_PATH = std::filesystem::path(AssetDirectories::DATABASE_FILE);

                // Create directory if it does not exist
                std::filesystem::create_directories(DB_PATH.parent_path());

                // Open the database
                sqlite::database DB {DB_PATH.string()};

                User Usr {};

                // Create the users table if it does not exist with the user_id, orion_id, username, and password columns.
                DB << "CREATE TABLE IF NOT EXISTS users (user_id TEXT PRIMARY KEY, orion_id TEXT, username TEXT, password TEXT);";

                // Convert the username to lowercase
                std::transform(Username.begin(), Username.end(), Username.begin(), ::tolower);

                // Check if the username already exists
                int UserCount = 0;
                DB << "SELECT COUNT(*) FROM users WHERE username = ? OR user_id = ?;" << Username << UserID >> UserCount;

                if (UserCount <= 0)
                {
                    web::json::value Response = web::json::value::object();
                    Response[U("message")]    = web::json::value::string(U("The user does not exist."));
                    Request.reply(web::http::status_codes::BadRequest, Response);
                    return;
                }

                // Get the user_id and orion_id from the database if the username and password are valid OR if the user_id is valid
                DB << "SELECT user_id,orion_id FROM users WHERE (username = ? AND password = ?) OR user_id = ?;" << Username << Password << UserID >>
                    [&Usr](std::string IDArg, std::string OrionIDArg) {
                        Usr = {IDArg, OrionIDArg};
                    };

                // Send the response
                if (Usr)
                {
                    // Check if user is already logged in
                    const auto USER_ITER =
                        std::find_if(m_LoggedInUsers.begin(), m_LoggedInUsers.end(), [Usr](const User& User) { return User.UserID == Usr.UserID; });

                    if (USER_ITER != m_LoggedInUsers.end())
                    {
                        web::json::value Response = web::json::value::object();
                        Response[U("user_id")]    = web::json::value::string(Usr.UserID);
                        Request.reply(web::http::status_codes::OK, Response);
                        return;
                    }

                    // Instantiate the Orion instance for the user
                    InstantiateOrionInstance(Usr.OrionID);

                    m_LoggedInUsers.push_back(Usr);

                    web::json::value Response = web::json::value::object();
                    Response[U("user_id")]    = web::json::value::string(Usr.UserID);
                    Request.reply(web::http::status_codes::OK, Response);
                }
                else
                {
                    web::json::value Response = web::json::value::object();
                    Response[U("message")]    = web::json::value::string(U("Invalid username or password."));
                    Request.reply(web::http::status_codes::Unauthorized, Response);
                }
            });
}

void OrionWebServer::HandleRegisterEndpoint(web::http::http_request Request)
{
    // Get the username and password from the request body
    Request.extract_json()
        .then([this, Request](pplx::task<web::json::value> ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [this, Request](web::json::value JsonRequestBody)
            {
                // Get the username and password from the request body
                auto Username = JsonRequestBody.at(U("username")).as_string();
                auto Password = JsonRequestBody.at(U("password")).as_string();

                // Check if the username and password are empty
                if (Username.empty() || Password.empty())
                {
                    web::json::value Response = web::json::value::object();
                    Response[U("message")]    = web::json::value::string(U("The username and password are required."));
                    Request.reply(web::http::status_codes::BadRequest, Response);
                    return;
                }

                // Get the database path
                const auto DB_PATH = std::filesystem::path(AssetDirectories::DATABASE_FILE);

                // Create directory if it does not exist
                std::filesystem::create_directories(DB_PATH.parent_path());

                // Open the database
                sqlite::database DB {DB_PATH.string()};

                // Create the users table if it does not exist with the user_id, orion_id, username, and password columns.
                DB << "CREATE TABLE IF NOT EXISTS users (user_id TEXT PRIMARY KEY, orion_id TEXT, username TEXT, password TEXT);";

                // Convert the username to lowercase
                std::transform(Username.begin(), Username.end(), Username.begin(), ::tolower);

                // Check if the username already exists
                int UserCount = 0;
                DB << "SELECT COUNT(*) FROM users WHERE username = ?;" << Username >> UserCount;

                if (UserCount > 0)
                {
                    web::json::value Response = web::json::value::object();
                    Response[U("message")]    = web::json::value::string(U("The username already exists."));
                    Request.reply(web::http::status_codes::Conflict, Response);
                    return;
                }

                // Instantiate the Orion instance for the user
                const auto& ORION = InstantiateOrionInstance();

                // Get the orion id
                const auto ORION_ID = ORION.GetCurrentAssistantID();

                // Generate a guid for the user id using custom
                const std::string USER_ID = GUID::Generate();

                if (USER_ID.empty() || ORION_ID.empty())
                {
                    web::json::value Response = web::json::value::object();
                    Response[U("message")]    = web::json::value::string(U("An internal error occurred."));
                    Request.reply(web::http::status_codes::InternalError, Response);
                    return;
                }

                // Insert the user into the database
                DB << "INSERT INTO users (user_id, orion_id, username, password) VALUES (?, ?, ?, ?);" << USER_ID << ORION_ID << Username << Password;

                m_LoggedInUsers.push_back({USER_ID, ORION_ID});

                // Send the response
                web::json::value Response = web::json::value::object();
                Response[U("user_id")]    = web::json::value::string(USER_ID);
                Request.reply(web::http::status_codes::OK, Response);
            });
}

void OrionWebServer::HandleSpeechToTextEndpoint(web::http::http_request Request)
{
    Request.extract_vector()
        .then([this, Request](pplx::task<std::vector<unsigned char>> ExtractVectorTask) { return ExtractVectorTask.get(); })
        .then(
            [this, Request](std::vector<unsigned char> AudioData)
            {
                // Get openai api key
                std::ifstream OpenAIAPIKeyFile {AssetDirectories::ResolveOpenAIKeyFile()};
                std::string   OpenAIAPIKey {std::istreambuf_iterator<char>(OpenAIAPIKeyFile), std::istreambuf_iterator<char>()};
                if (OpenAIAPIKey.empty())
                {
                    // Try to get the openai api key from the environment
                    const auto API_KEY = std::getenv("OPENAI_API_KEY");
                    if (API_KEY)
                    {
                        OpenAIAPIKey = API_KEY;
                    }
                    if (OpenAIAPIKey.empty())
                    {
                        auto JSpeechToTextRequestResponse          = web::json::value::object();
                        JSpeechToTextRequestResponse[U("message")] = web::json::value::string(U("The OpenAI API key was not found."));
                        Request.reply(web::http::status_codes::Unauthorized, JSpeechToTextRequestResponse);
                        return;
                    }
                }

                // Stream the audio data to a file
                std::ofstream AudioFile {"audio.mp4", std::ios::binary};
                AudioFile.write(reinterpret_cast<const char*>(AudioData.data()), AudioData.size());
                AudioFile.close();

                // Convert the audio to wav format (overwrites the audio.wav file if it exists. Don't ask for confirmation)
                std::system("ffmpeg -i audio.mp4 -acodec pcm_s16le -ac 1 -ar 16000 audio.wav -y");

                // Read the wav file
                std::ifstream              WavFile {"audio.wav", std::ios::binary};
                std::vector<unsigned char> WavData {std::istreambuf_iterator<char>(WavFile), std::istreambuf_iterator<char>()};

                // Get the mime type from the request
                const auto        MIME_TYPE = MimeTypes::GetMimeType("audio.wav");
                const std::string MODEL     = "whisper-1";

                // Create a boundary for the multipart/form-data body
                const std::string BOUNDARY = "----CppRestSdkFormBoundary";

                // Create the multipart/form-data body
                std::vector<unsigned char> MultiPartFormData;

                // Helper function to append text to the vector
                auto AppendText = [&](const std::string& text) { MultiPartFormData.insert(MultiPartFormData.end(), text.begin(), text.end()); };

                // Helper function to append binary data to the vector
                auto AppendBinary = [&](const std::vector<unsigned char>& data)
                { MultiPartFormData.insert(MultiPartFormData.end(), data.begin(), data.end()); };

                // Create the multipart/form-data body

                // Add the model part
                AppendText("--" + BOUNDARY + "\r\n");
                AppendText("Content-Disposition: form-data; name=\"model\"\r\n\r\n");
                AppendText(MODEL + "\r\n");

                // Add the file part
                AppendText("--" + BOUNDARY + "\r\n");
                AppendText("Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n");
                AppendText("Content-Type: " + MIME_TYPE + "\r\n\r\n");
                AppendBinary(WavData);
                AppendText("\r\n");

                // End of the multipart/form-data body
                AppendText("--" + BOUNDARY + "--");

                // Create the request
                web::http::client::http_client Client(U("https://api.openai.com/v1"));
                web::http::http_request        SpeechToTextRequest(web::http::methods::POST);
                SpeechToTextRequest.set_request_uri(U("/audio/transcriptions"));
                SpeechToTextRequest.headers().add(U("Authorization"), U("Bearer " + OpenAIAPIKey));
                SpeechToTextRequest.headers().add(U("Content-Type"), U("multipart/form-data; boundary=") + std::string(BOUNDARY));
                SpeechToTextRequest.set_body(MultiPartFormData);

                // Send the request
                Client.request(SpeechToTextRequest)
                    .then(
                        [this, Request](web::http::http_response Response)
                        {
                            if (Response.status_code() == web::http::status_codes::OK)
                            {
                                // Get the response body
                                Response.extract_json()
                                    .then([this, Request](pplx::task<web::json::value> ExtractJsonTask) { return ExtractJsonTask.get(); })
                                    .then(
                                        [this, Request](web::json::value ResponseJson)
                                        {
                                            if (ResponseJson.has_field(U("text")))
                                            {
                                                auto JSpeechToTextRequestResponse          = web::json::value::object();
                                                JSpeechToTextRequestResponse[U("message")] = ResponseJson.at(U("text"));

                                                auto Dump = ResponseJson.serialize();

                                                // Send the response
                                                Request.reply(web::http::status_codes::OK, JSpeechToTextRequestResponse);
                                            }
                                            else
                                            {
                                                auto JSpeechToTextRequestResponse          = web::json::value::object();
                                                JSpeechToTextRequestResponse[U("message")] = web::json::value::string(ResponseJson.serialize());

                                                // Send the response
                                                Request.reply(web::http::status_codes::BadRequest, JSpeechToTextRequestResponse);
                                            }
                                        });
                            }
                            else
                            {
                                Response.extract_json()
                                    .then([this, Request, Response](pplx::task<web::json::value> ExtractJsonTask) { return ExtractJsonTask.get(); })
                                    .then(
                                        [this, Request, Response](web::json::value ResponseJson)
                                        {
                                            auto JSpeechToTextRequestResponse          = web::json::value::object();
                                            JSpeechToTextRequestResponse[U("message")] = web::json::value::string(ResponseJson.serialize());

                                            // Send the response
                                            Request.reply(Response.status_code(), JSpeechToTextRequestResponse);
                                        });
                            }
                        });
            });
}