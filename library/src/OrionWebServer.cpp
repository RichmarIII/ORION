#include "OrionWebServer.hpp"
#include "GUID.hpp"
#include "MimeTypes.hpp"
#include "Orion.hpp"
#include "User.hpp"
#include "tools/ChangeIntelligenceFunctionTool.hpp"
#include "tools/ChangeVoiceFunctionTool.hpp"
#include "tools/CodeInterpreterTool.hpp"
#include "tools/CreateAutonomousActionPlanFunctionTool.hpp"
#include "tools/DownloadHTTPFileFunctionTool.hpp"
#include "tools/ExecSmartDeviceServiceFunctionTool.hpp"
#include "tools/GetWeatherFunctionTool.hpp"
#include "tools/ListSmartDevicesFunctionTool.hpp"
#include "tools/NavigateLinkFunctionTool.hpp"
#include "tools/RecallKnowledgeFunctionTool.hpp"
#include "tools/RememberKnowledgeFunctionTool.hpp"
#include "tools/RequestFileUploadFromUserFunctionTool.hpp"
#include "tools/RetrievalTool.hpp"
#include "tools/SearchFilesystemFunctionTool.hpp"
#include "tools/TakeScreenshotFunctionTool.hpp"
#include "tools/UpdateKnowledgeFunctionTool.hpp"
#include "tools/WebSearchFunctionTool.hpp"

#include <cmark.h>

#include <cpprest/filestream.h>
#include <cpprest/producerconsumerstream.h>

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

void OrionWebServer::Start(const int PORT)
{
    web::http::experimental::listener::http_listener_config ListenerConfig;
    ListenerConfig.set_ssl_context_callback(
        [this](boost::asio::ssl::context& Ctx)
        {
            Ctx.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3 |
                            boost::asio::ssl::context::single_dh_use);
            Ctx.set_password_callback([](std::size_t MaxLength, boost::asio::ssl::context::password_purpose Purpose) { return "test"; });
            Ctx.use_certificate_chain_file("cert.pem");
            Ctx.use_private_key_file("key.pem", boost::asio::ssl::context::pem);
        });

    // Create a listener
    m_Listener = web::http::experimental::listener::http_listener(U("https://0.0.0.0:") + std::to_string(PORT), ListenerConfig);

    // Handle requests
    m_Listener.support(web::http::methods::POST, std::bind(&OrionWebServer::HandleRequest, this, std::placeholders::_1));
    m_Listener.support(web::http::methods::GET, std::bind(&OrionWebServer::HandleRequest, this, std::placeholders::_1));

    // Start the listener
    m_IsRunning = m_Listener.open().wait() == pplx::task_status::completed ? true : false;

    // Create orion event thread
    m_OrionEventThread = std::thread(std::bind(&OrionWebServer::OrionEventThreadHandler, this));
}

void OrionWebServer::Stop()
{
    // Close the listener
    m_IsRunning = false;
    m_ConditionVariable.notify_one();
    m_OrionEventQueueConditionVariable.notify_one();
    m_OrionEventThread.join();
}

void OrionWebServer::Wait()
{
    // Aquire the lock
    std::unique_lock<std::mutex> Lock(m_Mutex);

    // Wait for the condition variable
    m_ConditionVariable.wait(Lock, [this] { return !m_IsRunning; });

    // Close the listener
    if (m_Listener.close().wait() != pplx::task_status::completed)
    {
        std::cout << __FUNCTION__ << ":" << __LINE__ << ": Failed to close http listener!";
    }
}

void OrionWebServer::HandleRequest(const web::http::http_request& Request)
{
    // Get the request path
    // ReSharper disable once CppTooWideScopeInitStatement
    const auto PATH = Request.request_uri().path();

    // Store the current endpoint request
    m_CurrentRequest = Request;

    // Dispatch the request to the appropriate handler
    if (PATH == U("/orion/send_message"))
    {
        HandleSendMessageEndpoint(Request);
    }
    else if (PATH == U("/"))
    {
        HandleRootEndpoint(Request);
    }
    else if (PATH == U("/markdown"))
    {
        HandleMarkdownEndpoint(Request);
    }
    else if (PATH == U("/orion/chat_history"))
    {
        HandleChatHistoryEndpoint(Request);
    }
    else if (PATH == U("/orion/speak"))
    {
        HandleSpeakEndpoint(Request);
    }
    else if (PATH == U("/login"))
    {
        HandleLoginEndpoint(Request);
    }
    else if (PATH == U("/register"))
    {
        HandleRegisterEndpoint(Request);
    }
    else if (PATH == U("/orion/transcribe"))
    {
        HandleTranscribeEndpoint(Request);
    }
    // Handle orion_event endpoint
    else if (PATH == U("/orion/events"))
    {
        HandleOrionEventsEndpoint(Request);
    }
    else if (PATH.find("/orion/files/") != std::string::npos)
    {
        HandleOrionFilesEndpoint(Request);
    }
    else if (PATH.find("/assets/") != std::string::npos)
    {
        HandleAssetFileEndpoint(Request);
    }
    else
    {
        auto Response          = web::json::value::object();
        Response[U("message")] = web::json::value::string(U("The requested endpoint was not found."));
        Request.reply(web::http::status_codes::NotFound, Response);
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
    const auto USER_ITER = std::find_if(m_LoggedInUsers.begin(), m_LoggedInUsers.end(), [USER_ID](const User& User) { return User.UserID == USER_ID; });
    if (USER_ITER == m_LoggedInUsers.end())
    {
        Request.reply(web::http::status_codes::Unauthorized, U("User is not logged in."));
        return;
    }

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(),
                                m_OrionInstances.end(),
                                [USER_ITER](const std::unique_ptr<Orion>& Orion) { return Orion->GetCurrentAssistantID() == USER_ITER->OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        Request.reply(web::http::status_codes::BadRequest, U("The Orion instance with the given id was not found."));
        return;
    }

    // Check if markdown is requested
    const bool IS_MARKDOWN_REQUESTED = Request.request_uri().query().find(U("markdown=true")) != std::string::npos;

    // Extract the message and the files from the form data
    Request.extract_json()
        .then([this, OrionIt, Request, IS_MARKDOWN_REQUESTED](const pplx::task<web::json::value>& ExtractJsonTask) { return ExtractJsonTask; })
        .then(
            [this, OrionIt, Request, IS_MARKDOWN_REQUESTED](web::json::value JsonRequestBody)
            {
                // Get the message from the request body
                const auto MESSAGE = JsonRequestBody.at(U("message")).as_string();

                // Get the files from the request body
                const auto FILES = JsonRequestBody.has_field(U("files")) ? JsonRequestBody.at(U("files")).as_array() : web::json::value::array().as_array();

                // Send the message to the Orion instance
                (*OrionIt)->SendMessageAsync(MESSAGE, FILES);

                // Send the response
                Request.reply(web::http::status_codes::OK);
            });
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void OrionWebServer::HandleRootEndpoint(web::http::http_request Request)
{
    constexpr auto INDEX_HTML { U("index.html") };
    const auto     INDEX_HTML_PATH = std::filesystem::path(AssetDirectories::ResolveBaseAssetDirectory(INDEX_HTML)) / INDEX_HTML;

    // Find the index.html file
    std::ifstream IndexFile { INDEX_HTML_PATH };
    if (!IndexFile.is_open())
    {
        Request.reply(web::http::status_codes::NotFound, U("The index.html file was not found."));
        return;
    }

    // Read the contents of the index.html file
    std::string IndexContents { std::istreambuf_iterator<char>(IndexFile), std::istreambuf_iterator<char>() };

    // Send the contents of the index.html file
    const auto MIME_TYPE = MimeTypes::GetMimeType(INDEX_HTML);

    // ReSharper disable once CppExpressionWithoutSideEffects
    Request.reply(web::http::status_codes::OK, IndexContents, MIME_TYPE);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void OrionWebServer::HandleAssetFileEndpoint(web::http::http_request Request)
{
    // Get the file name from the request path and make it relative (remove the leading slash)
    const auto FILE_NAME { Request.request_uri().path() };

    if (FILE_NAME.empty())
    {
        auto Response       = web::json::value::object();
        Response["message"] = web::json::value::string(U("The file name is required."));
        Request.reply(web::http::status_codes::BadRequest, Response);
        return;
    }



    // Remove leading slash if present
    auto FileNameCorrected = FILE_NAME[0] == '/' ? FILE_NAME.substr(1) : FILE_NAME;

    // Remove the leading assets directory
    // Check if beginning of the file name is the assets directory
    constexpr std::string_view ASSETS_DIR { U("assets/") };
    FileNameCorrected = { FileNameCorrected.find(ASSETS_DIR) == 0 ? FileNameCorrected.substr(ASSETS_DIR.size()) : FileNameCorrected };

    // FIXME: This Web Server is not secure. It is vulnerable to directory traversal attacks. Fix this by checking if the file path is within the assets directory (We are doing
    // this above but it is not enough eg. /assets/../file.txt)
    // Calculate file path.
    const std::string FILE_PATH { std::filesystem::current_path() / AssetDirectories::STATIC_ASSETS_DIR / FileNameCorrected };

    // Ensure that the file path is within the assets directory. We can do this by getting the absolute path of the file
    // and checking if it starts with the absolute path of the assets directory.
    const std::filesystem::path ABSOLUTE_FILE_PATH { std::filesystem::canonical(FILE_PATH) };

    // Check if the file path is within the assets directory
    if (const std::filesystem::path ABSOLUTE_ASSETS_DIR { std::filesystem::canonical(std::filesystem::current_path() / AssetDirectories::STATIC_ASSETS_DIR) };
        ABSOLUTE_FILE_PATH.string().find(ABSOLUTE_ASSETS_DIR.string()) != 0)
    {
        auto Response       = web::json::value::object();
        Response["message"] = web::json::value::string(U("The file path is invalid."));
        Request.reply(web::http::status_codes::BadRequest, Response);
        return;
    }

    // Check if the file exists
    if (!std::filesystem::exists(FILE_PATH))
    {
        auto Response       = web::json::value::object();
        Response["message"] = web::json::value::string(U("The file was not found."));
        Request.reply(web::http::status_codes::NotFound, Response);
        return;
    }

    // Get mime type from file extension
    const std::string CONTENT_TYPE { MimeTypes::GetMimeType(FILE_PATH) };

    // Stream the file to the response
    concurrency::streams::fstream::open_istream(FILE_PATH)
        .then(
            [Request, CONTENT_TYPE](const concurrency::streams::istream& StaticFileInputStream)
            {
                // Send the response
                // ReSharper disable once CppExpressionWithoutSideEffects
                Request.reply(web::http::status_codes::OK, StaticFileInputStream, CONTENT_TYPE);
            })
        .then(
            [FILE_PATH, Request](const pplx::task<void>& OpenStreamTask)
            {
                try
                {
                    OpenStreamTask.get();
                }
                catch (std::exception& Exception)
                {
                    std::cout << U("The file ") << FILE_PATH << U(" was not found: ") << std::endl;

                    Request.reply(web::http::status_codes::NotFound, U("The file ") + FILE_PATH + U(" was not found: ") + std::string(Exception.what()));
                }
            });
}

void OrionWebServer::HandleMarkdownEndpoint(web::http::http_request Request) const
{
    // Get the message from the request body
    Request.extract_json()
        .then([this](const pplx::task<web::json::value>& ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [this, Request](web::json::value JsonRequestBody)
            {
                // Get the message from the request body
                auto RequestMessage = JsonRequestBody.at(U("message")).as_string();

                // Convert the message to markdown
                {
                    char* pMarkdown = cmark_markdown_to_html(RequestMessage.c_str(), RequestMessage.length(), CMARK_OPT_UNSAFE);
                    RequestMessage  = pMarkdown;
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
        // ReSharper disable once CppExpressionWithoutSideEffects
        Request.reply(web::http::status_codes::BadRequest, Response);
        return;
    }

    // Get the Orion instance id from the header
    const auto USER_ID = Request.headers().find(U("X-User-Id"))->second;

    // Find User in the list of logged in users
    const auto USER_ITER = std::find_if(m_LoggedInUsers.begin(), m_LoggedInUsers.end(), [USER_ID](const User& User) { return User.UserID == USER_ID; });
    if (USER_ITER == m_LoggedInUsers.end())
    {
        web::json::value Response = web::json::value::object();
        Response[U("message")]    = web::json::value::string(U("User is not logged in."));

        // ReSharper disable once CppExpressionWithoutSideEffects
        Request.reply(web::http::status_codes::Unauthorized, Response);
        return;
    }

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(),
                                m_OrionInstances.end(),
                                [USER_ITER](const std::unique_ptr<Orion>& Orion) { return Orion->GetCurrentAssistantID() == USER_ITER->OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        web::json::value Response = web::json::value::object();
        Response[U("message")]    = web::json::value::string(U("Could not find an Orion instance for the given user."));

        // ReSharper disable once CppExpressionWithoutSideEffects
        Request.reply(web::http::status_codes::BadRequest, Response);
        return;
    }

    // Get the chat history
    (*OrionIt)->GetChatHistoryAsync().then(
        [this, Request](web::json::value ChatHistory)
        {
            // Check if the query parameter is present

            // Convert the chat history to markdown if the query parameter is present
            if (const bool IS_MARKDOWN_REQUESTED = Request.request_uri().query().find(U("markdown=true")) != std::string::npos; IS_MARKDOWN_REQUESTED)
            {
                for (auto& JMessage : ChatHistory.as_array())
                {
                    auto  Message          = JMessage.at(U("message")).as_string();
                    char* pMarkdown        = cmark_markdown_to_html(Message.c_str(), Message.length(), CMARK_OPT_UNSAFE);
                    JMessage[U("message")] = web::json::value::string(pMarkdown);
                    JMessage[U("role")]    = web::json::value::string(JMessage.at(U("role")).as_string() == U("user") ? U("user") : U("orion"));
                    free(pMarkdown);
                }
            }

            // Send the response

            // ReSharper disable once CppExpressionWithoutSideEffects
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
    const auto USER_ITER = std::find_if(m_LoggedInUsers.begin(), m_LoggedInUsers.end(), [USER_ID](const User& User) { return User.UserID == USER_ID; });
    if (USER_ITER == m_LoggedInUsers.end())
    {
        Request.reply(web::http::status_codes::Unauthorized, U("User is not logged in."));
        return;
    }

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(),
                                m_OrionInstances.end(),
                                [USER_ITER](const std::unique_ptr<Orion>& Orion) { return Orion->GetCurrentAssistantID() == USER_ITER->OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        Request.reply(web::http::status_codes::BadRequest, U("The Orion instance with the given id was not found."));
        return;
    }

    // Get the message from the request body
    Request.extract_json()
        .then([this](const pplx::task<web::json::value>& ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [this, OrionIt, Request](web::json::value RequestMessageJson)
            {
                // Get the audio format from the query parameter
                auto AudioFormat = ETTSAudioFormat::MP3;
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
                        [this, Request, AudioFormat](const concurrency::streams::istream& AudioStream)
                        {
                            // Send the response
                            // ReSharper disable once CppExpressionWithoutSideEffects
                            Request.reply(web::http::status_codes::OK, AudioStream, MimeTypes::GetMimeType(AudioFormat));
                        });
            });
}

const Orion& OrionWebServer::InstantiateOrionInstance(const std::string& ExistingOrionInstanceID)
{
    // Create a new Orion instance

    if (!m_CurrentRequest)
    {
        throw std::runtime_error("The current endpoint request is null.");
    }

    // Declare Default Tools
    std::vector<std::unique_ptr<IOrionTool>> Tools {};
    Tools.reserve(10);

    Tools.push_back(std::make_unique<TakeScreenshotFunctionTool>());
    Tools.push_back(std::make_unique<SearchFilesystemFunctionTool>());
    Tools.push_back(std::make_unique<GetWeatherFunctionTool>());
    Tools.push_back(std::make_unique<WebSearchFunctionTool>());
    Tools.push_back(std::make_unique<ChangeVoiceFunctionTool>());
    Tools.push_back(std::make_unique<ChangeIntelligenceFunctionTool>());
    // Tools.push_back(std::make_unique<RetrievalTool>());
    Tools.push_back(std::make_unique<CodeInterpreterTool>());
    Tools.push_back(std::make_unique<ListSmartDevicesFunctionTool>());
    Tools.push_back(std::make_unique<ExecSmartDeviceServiceFunctionTool>());
    Tools.push_back(std::make_unique<NavigateLinkFunctionTool>());
    Tools.push_back(std::make_unique<DownloadHTTPFileFunctionTool>());
    Tools.push_back(std::make_unique<CreateAutonomousActionPlanFunctionTool>());
    Tools.push_back(std::make_unique<RequestFileUploadFromUserFunctionTool>());
    Tools.push_back(std::make_unique<RememberKnowledgeFunctionTool>());
    Tools.push_back(std::make_unique<RecallKnowledgeFunctionTool>());
    Tools.push_back(std::make_unique<UpdateKnowledgeFunctionTool>());

    // Check if the Orion instance already exists locally (Only one Orion instance is allowed per user)

    // Check if the Orion instance was found locally
    if (const auto ORION_IT = std::find_if(m_OrionInstances.begin(),
                                           m_OrionInstances.end(),
                                           [ExistingOrionInstanceID](const std::unique_ptr<Orion>& Orion) { return Orion->GetCurrentAssistantID() == ExistingOrionInstanceID; });
        ORION_IT != m_OrionInstances.end())
    {
        return **ORION_IT;
    }

    // Create the Orion instance
    auto NewOrion = std::make_unique<Orion>(ExistingOrionInstanceID, std::move(Tools));

    // Initialize the Orion instance
    NewOrion->Initialize(*this, *m_CurrentRequest);

    // Add the Orion instance to the list
    m_OrionInstances.push_back(std::move(NewOrion));

    return *m_OrionInstances.back();
}

void OrionWebServer::HandleLoginEndpoint(web::http::http_request Request)
{
    // Get the username and password from the request body
    Request.extract_json()
        .then([this](const pplx::task<web::json::value>& ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [this, Request](web::json::value JsonRequestBody)
            {
                // Get the username and password from the request body
                const auto USERNAME = JsonRequestBody.has_field(U("username")) ? JsonRequestBody.at(U("username")).as_string() : U("");
                const auto PASSWORD = JsonRequestBody.has_field(U("password")) ? JsonRequestBody.at(U("password")).as_string() : U("");
                const auto USER_ID  = JsonRequestBody.has_field(U("user_id")) ? JsonRequestBody.at(U("user_id")).as_string() : U("");

                // Get the database path
                const auto DB_PATH = std::filesystem::path(AssetDirectories::DATABASE_FILE);

                // Create directory if it does not exist
                std::filesystem::create_directories(DB_PATH.parent_path());

                // Open the database
                sqlite::database DB { DB_PATH.string() };

                User Usr {};

                // Create the users table if it does not exist with the user_id, orion_id, username, and password columns.
                DB << "CREATE TABLE IF NOT EXISTS users (user_id TEXT PRIMARY KEY, orion_id TEXT, username TEXT, password TEXT);";

                // Convert the username to lowercase
                auto UserNameLower = USERNAME;
                std::transform(UserNameLower.begin(), UserNameLower.end(), UserNameLower.begin(), ::tolower);

                // Check if the username already exists
                int UserCount = 0;
                DB << "SELECT COUNT(*) FROM users WHERE username = ? OR user_id = ?;" << UserNameLower << USER_ID >> UserCount;

                if (UserCount <= 0)
                {
                    web::json::value Response = web::json::value::object();
                    Response[U("message")]    = web::json::value::string(U("The user does not exist."));

                    // ReSharper disable once CppExpressionWithoutSideEffects
                    Request.reply(web::http::status_codes::BadRequest, Response);
                    return;
                }

                // Get the user_id and orion_id from the database if the username and password are valid OR if the user_id is valid
                DB << "SELECT user_id,orion_id FROM users WHERE (username = ? AND password = ?) OR user_id = ?;" << UserNameLower << PASSWORD << USER_ID >>
                    [&Usr](const std::string& IDArg, const std::string& OrionIDArg) {
                        Usr = { IDArg, OrionIDArg };
                    };

                // Send the response
                if (Usr)
                {
                    // Check if user is already logged in

                    if (const auto USER_ITER = std::find_if(m_LoggedInUsers.begin(), m_LoggedInUsers.end(), [Usr](const User& User) { return User.UserID == Usr.UserID; });
                        USER_ITER != m_LoggedInUsers.end())
                    {
                        web::json::value Response = web::json::value::object();
                        Response[U("user_id")]    = web::json::value::string(Usr.UserID);

                        // ReSharper disable once CppExpressionWithoutSideEffects
                        Request.reply(web::http::status_codes::OK, Response);
                        return;
                    }

                    // Instantiate the Orion instance for the user
                    InstantiateOrionInstance(Usr.OrionID);

                    m_LoggedInUsers.push_back(Usr);

                    web::json::value Response = web::json::value::object();
                    Response[U("user_id")]    = web::json::value::string(Usr.UserID);

                    // ReSharper disable once CppExpressionWithoutSideEffects
                    Request.reply(web::http::status_codes::OK, Response);
                }
                else
                {
                    web::json::value Response = web::json::value::object();
                    Response[U("message")]    = web::json::value::string(U("Invalid username or password."));

                    // ReSharper disable once CppExpressionWithoutSideEffects
                    Request.reply(web::http::status_codes::Unauthorized, Response);
                }
            });
}

void OrionWebServer::HandleRegisterEndpoint(web::http::http_request Request)
{
    // Get the username and password from the request body
    Request.extract_json()
        .then([this](const pplx::task<web::json::value>& ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [this, Request](web::json::value JsonRequestBody)
            {
                // Get the username and password from the request body
                const auto USERNAME = JsonRequestBody.at(U("username")).as_string();
                const auto PASSWORD = JsonRequestBody.at(U("password")).as_string();

                // Check if the username and password are empty
                if (USERNAME.empty() || PASSWORD.empty())
                {
                    web::json::value Response = web::json::value::object();
                    Response[U("message")]    = web::json::value::string(U("The username and password are required."));

                    // ReSharper disable once CppExpressionWithoutSideEffects
                    Request.reply(web::http::status_codes::BadRequest, Response);
                    return;
                }

                // Get the database path
                const auto DB_PATH = std::filesystem::path(AssetDirectories::DATABASE_FILE);

                // Create directory if it does not exist
                std::filesystem::create_directories(DB_PATH.parent_path());

                // Open the database
                sqlite::database DB { DB_PATH.string() };

                // Create the users table if it does not exist with the user_id, orion_id, username, and password columns.
                DB << "CREATE TABLE IF NOT EXISTS users (user_id TEXT PRIMARY KEY, orion_id TEXT, username TEXT, password TEXT);";

                // Convert the username to lowercase
                auto UserNameLower = USERNAME;
                std::transform(UserNameLower.begin(), UserNameLower.end(), UserNameLower.begin(), ::tolower);

                // Check if the username already exists
                int UserCount = 0;
                DB << "SELECT COUNT(*) FROM users WHERE username = ?;" << UserNameLower >> UserCount;

                if (UserCount > 0)
                {
                    web::json::value Response = web::json::value::object();
                    Response[U("message")]    = web::json::value::string(U("The username already exists."));

                    // ReSharper disable once CppExpressionWithoutSideEffects
                    Request.reply(web::http::status_codes::Conflict, Response);
                    return;
                }

                // Instantiate the Orion instance for the user
                const auto& ORION = InstantiateOrionInstance();

                // Get the orion id
                const auto ORION_ID = ORION.GetCurrentAssistantID();

                // Generate a guid for the user id using custom
                const auto USER_ID = static_cast<std::string>(GUID::Generate());

                if (USER_ID.empty() || ORION_ID.empty())
                {
                    web::json::value Response = web::json::value::object();
                    Response[U("message")]    = web::json::value::string(U("An internal error occurred."));

                    // ReSharper disable once CppExpressionWithoutSideEffects
                    Request.reply(web::http::status_codes::InternalError, Response);
                    return;
                }

                // Insert the user into the database
                DB << "INSERT INTO users (user_id, orion_id, username, password) VALUES (?, ?, ?, ?);" << USER_ID << ORION_ID << UserNameLower << PASSWORD;

                m_LoggedInUsers.push_back({ USER_ID, ORION_ID });

                // Send the response
                web::json::value Response = web::json::value::object();
                Response[U("user_id")]    = web::json::value::string(USER_ID);

                // ReSharper disable once CppExpressionWithoutSideEffects
                Request.reply(web::http::status_codes::OK, Response);
            });
}

void OrionWebServer::HandleTranscribeEndpoint(web::http::http_request Request) const
{
    Request.extract_vector()
        .then([this](const pplx::task<std::vector<unsigned char>>& ExtractVectorTask) { return ExtractVectorTask.get(); })
        .then(
            [this, Request](std::vector<unsigned char> AudioData)
            {
                // Get openai api key
                std::ifstream OpenAIAPIKeyFile { AssetDirectories::ResolveOpenAIKeyFile() };
                std::string   OpenAIAPIKey { std::istreambuf_iterator<char>(OpenAIAPIKeyFile), std::istreambuf_iterator<char>() };
                if (OpenAIAPIKey.empty())
                {
                    // Try to get the openai api key from the environment
                    if (const char* pAPI_KEY = std::getenv("OPENAI_API_KEY"))
                    {
                        OpenAIAPIKey = pAPI_KEY;
                    }
                    if (OpenAIAPIKey.empty())
                    {
                        auto JSpeechToTextRequestResponse          = web::json::value::object();
                        JSpeechToTextRequestResponse[U("message")] = web::json::value::string(U("The OpenAI API key was not found."));

                        // ReSharper disable once CppExpressionWithoutSideEffects
                        Request.reply(web::http::status_codes::Unauthorized, JSpeechToTextRequestResponse);
                        return;
                    }
                }

                // Stream the audio data to a file
                std::ofstream AudioFile { "audio.mp4", std::ios::binary };
                AudioFile.write(reinterpret_cast<const char*>(AudioData.data()), AudioData.size());
                AudioFile.close();

                // Convert the audio to wav format (overwrites the audio.wav file if it exists. Don't ask for confirmation)
                std::system("ffmpeg -i audio.mp4 -acodec pcm_s16le -ac 1 -ar 16000 audio.wav -y");

                // Read the wav file
                std::ifstream              WavFile { "audio.wav", std::ios::binary };
                std::vector<unsigned char> WavData { std::istreambuf_iterator<char>(WavFile), std::istreambuf_iterator<char>() };

                // Get the mime type from the request
                const auto        MIME_TYPE = MimeTypes::GetMimeType("audio.wav");
                const std::string MODEL     = "whisper-1";

                // Create a boundary for the multipart/form-data body
                const std::string BOUNDARY = "----CppRestSdkFormBoundary";

                // Create the multipart/form-data body
                std::vector<unsigned char> MultiPartFormData;

                // Helper function to append text to the vector
                auto AppendText = [&](const std::string& Text) { MultiPartFormData.insert(MultiPartFormData.end(), Text.begin(), Text.end()); };

                // Helper function to append binary data to the vector
                auto AppendBinary = [&](const std::vector<unsigned char>& Data) { MultiPartFormData.insert(MultiPartFormData.end(), Data.begin(), Data.end()); };

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
                                    .then([this, Request](const pplx::task<web::json::value>& ExtractJsonTask) { return ExtractJsonTask.get(); })
                                    .then(
                                        [this, Request](web::json::value ResponseJson)
                                        {
                                            if (ResponseJson.has_field(U("text")))
                                            {
                                                auto JSpeechToTextRequestResponse          = web::json::value::object();
                                                JSpeechToTextRequestResponse[U("message")] = ResponseJson.at(U("text"));

                                                auto Dump = ResponseJson.serialize();

                                                // Send the response
                                                // ReSharper disable once CppExpressionWithoutSideEffects
                                                Request.reply(web::http::status_codes::OK, JSpeechToTextRequestResponse);
                                            }
                                            else
                                            {
                                                auto JSpeechToTextRequestResponse          = web::json::value::object();
                                                JSpeechToTextRequestResponse[U("message")] = web::json::value::string(ResponseJson.serialize());

                                                // Send the response
                                                // ReSharper disable once CppExpressionWithoutSideEffects
                                                Request.reply(web::http::status_codes::BadRequest, JSpeechToTextRequestResponse);
                                            }
                                        });
                            }
                            else
                            {
                                Response.extract_json()
                                    .then([this, Request, Response](const pplx::task<web::json::value>& ExtractJsonTask) { return ExtractJsonTask.get(); })
                                    .then(
                                        [this, Request, Response](const web::json::value& ResponseJson)
                                        {
                                            auto JSpeechToTextRequestResponse          = web::json::value::object();
                                            JSpeechToTextRequestResponse[U("message")] = web::json::value::string(ResponseJson.serialize());

                                            // Send the response
                                            // ReSharper disable once CppExpressionWithoutSideEffects
                                            Request.reply(Response.status_code(), JSpeechToTextRequestResponse);
                                        });
                            }
                        });
            });
}

void OrionWebServer::HandleOrionEventsEndpoint(web::http::http_request Request)
{
    // Register the client for Orion events
    {
        std::lock_guard<std::mutex> OrionEventClientsLockGuard(m_OrionEventClientsMutex);

        // Create the response
        web::http::http_response Response(web::http::status_codes::OK);
        Response.headers().add(U("Content-Type"), U("text/event-stream"));
        Response.headers().add(U("Cache-Control"), U("no-cache"));
        Response.headers().add(U("Connection"), U("keep-alive"));

        concurrency::streams::producer_consumer_buffer<uint8_t> Buffer;

        // Add the client to the list of clients
        m_OrionEventClients.push_back(Buffer);

        Response.set_body(m_OrionEventClients.back().create_istream(), U("text/event-stream"));

        // Send the response
        Request.reply(Response);
    }
}

void OrionWebServer::HandleOrionFilesEndpoint(web::http::http_request Request) const
{
    // Load OpenAI API Key From environment variable or file
    std::ifstream OpenAIAPIKeyFile { AssetDirectories::ResolveOpenAIKeyFile() };
    std::string   OpenAIAPIKey { std::istreambuf_iterator<char>(OpenAIAPIKeyFile), std::istreambuf_iterator<char>() };
    if (OpenAIAPIKey.empty())
    {
        // Try to get the openai api key from the environment
        if (const char* pAPI_KEY = std::getenv("OPENAI_API_KEY"))
        {
            OpenAIAPIKey = pAPI_KEY;
        }
        if (OpenAIAPIKey.empty())
        {
            auto JFileRequestResponse          = web::json::value::object();
            JFileRequestResponse[U("message")] = web::json::value::string(U("The OpenAI API key was not found."));

            // ReSharper disable once CppExpressionWithoutSideEffects
            Request.reply(web::http::status_codes::Unauthorized, JFileRequestResponse);
            return;
        }
    }

    // Get the file_id from the request path
    const auto FILE_ID_RAW = Request.request_uri().path().substr(std::string("/orion/files/").length());
    const auto FILE_ID     = FILE_ID_RAW.substr(0, FILE_ID_RAW.find_last_of('.'));

    const auto MIME_TYPE = MimeTypes::GetMimeType(FILE_ID_RAW);

    // Create the request to get the file
    web::http::client::http_client Client(U("https://api.openai.com/v1"));
    web::http::http_request        FileRequest(web::http::methods::GET);
    FileRequest.set_request_uri(U("/files/") + FILE_ID + "/content");

    // Add the authorization header
    FileRequest.headers().add(U("Authorization"), U("Bearer " + OpenAIAPIKey));

    // Send the request
    Client.request(FileRequest)
        .then(
            [this, Request, MIME_TYPE](web::http::http_response FileRequestResponse)
            {
                // Forward the response to the client
                Request.reply(FileRequestResponse.status_code(), FileRequestResponse.body(), MIME_TYPE);
            });
}

void OrionWebServer::SendServerEvent(const OrionEventName& Event, const web::json::value& Data)
{
    // We push the message to a queue. The queue is processed in a separate thread.
    {
        std::lock_guard<std::mutex> LockGuard(m_OrionEventQueueMutex);
        m_OrionEventQueue.push({ Event, Data });
    }

    // Notify the thread to process the queue
    m_OrionEventQueueConditionVariable.notify_one();
}

void OrionWebServer::OrionEventThreadHandler()
{
    while (m_IsRunning)
    {
        std::unique_lock<std::mutex> Lock(m_OrionEventQueueMutex);

        // Wait for the condition variable to be notified
        m_OrionEventQueueConditionVariable.wait(Lock, [this] { return !m_OrionEventQueue.empty() || !m_IsRunning; });

        // Get the event from the queue
        if (!m_OrionEventQueue.empty())
        {
            const auto [Event, Data] = m_OrionEventQueue.front();
            m_OrionEventQueue.pop();

            // Unlock the mutex
            Lock.unlock();

            // Lock the OrionEventClients mutex
            std::lock_guard<std::mutex> OrionEventClientsLockGuard(m_OrionEventClientsMutex);

            // Loop through all the connected clients and send the event
            for (const auto& Client : m_OrionEventClients)
            {
                // Format the Server-Sent Event
                std::ostringstream SSEEvent;
                SSEEvent << "event: " << Event << "\n";
                SSEEvent << "data: " << Data.serialize() << "\n\n";
                SSEEvent << std::flush;

                auto ResponseStream = Client.create_ostream();
                ResponseStream.print(SSEEvent.str()).get();
                ResponseStream.flush().wait();
            }
        }
    }
}