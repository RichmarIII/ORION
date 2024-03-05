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

#include <cmark.h>

#include <cpprest/filestream.h>

using namespace ORION;

void OrionWebServer::Start(int Port)
{
    // Create a listener
    m_Listener = web::http::experimental::listener::http_listener(U("http://0.0.0.0:") + std::to_string(Port));

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
    else if (Path == U("/create_orion"))
    {
        HandleCreateOrionEndpoint(Request);
    }
    // Check if /audio is in the request path
    else if (Path.find(U("/audio")) != std::string::npos)
    {
        HandleAudioFileEndpoint(Request);
    }
    else
    {
        HandleStaticFileEndpoint(Request);
    }
}

void OrionWebServer::HandleSendMessageEndpoint(web::http::http_request Request)
{
    // Check for the X-Orion-Id header
    if (!Request.headers().has(U("X-Orion-Id")))
    {
        Request.reply(web::http::status_codes::BadRequest, U("The X-Orion-Id header is required."));
        return;
    }

    // Get the Orion instance id from the header
    auto OrionID = Request.headers().find(U("X-Orion-Id"))->second;

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(), m_OrionInstances.end(),
                                [OrionID](const std::unique_ptr<Orion>& Orion) { return Orion->GetCurrentAssistantID() == OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        Request.reply(web::http::status_codes::BadRequest, U("The Orion instance with the given id was not found."));
        return;
    }

    // Check if markdown is requested
    const bool IS_MARKDOWN_REQUESTED = Request.request_uri().query().find(U("markdown=true")) != std::string::npos;

    // Get the message from the request body
    Request.extract_string()
        .then([](pplx::task<std::string> ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [OrionIt, IS_MARKDOWN_REQUESTED, Request](std::string RequestMessage)
            {
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

                            // Send the response
                            Request.reply(web::http::status_codes::OK, SendMessageResponse);
                        });
            });
}

void OrionWebServer::HandleRootEndpoint(web::http::http_request Request)
{
    // Find the index.html file
    std::ifstream IndexFile("templates/index.html");
    if (!IndexFile.is_open())
    {
        Request.reply(web::http::status_codes::NotFound, U("The index.html file was not found."));
        return;
    }

    // Read the contents of the index.html file
    std::string IndexContents((std::istreambuf_iterator<char>(IndexFile)), std::istreambuf_iterator<char>());

    // Send the contents of the index.html file
    Request.reply(web::http::status_codes::OK, IndexContents, U("text/html"));
}

void OrionWebServer::HandleStaticFileEndpoint(web::http::http_request Request)
{
    // Get the file name from the request path
    auto FileName = Request.request_uri().path();

    // Remove the leading slash
    FileName = FileName.substr(1);

    // Get the file extension
    const auto LAST_DOT_INDEX = FileName.find_last_of('.');
    if (LAST_DOT_INDEX == std::string::npos)
    {
        Request.reply(web::http::status_codes::BadRequest, U("No file extension was found."));
        return;
    }

    // Get the file extension
    const auto EXTENSION = FileName.substr(LAST_DOT_INDEX);

    // Calculate the content type
    utility::string_t ContentType;
    if (EXTENSION == U(".html"))
        ContentType = U("text/html");
    else if (EXTENSION == U(".css"))
        ContentType = U("text/css");
    else if (EXTENSION == U(".js"))
        ContentType = U("application/javascript");
    else if (EXTENSION == U(".png"))
        ContentType = U("image/png");
    else if (EXTENSION == U(".jpg") || EXTENSION == U(".jpeg"))
        ContentType = U("image/jpeg");
    else if (EXTENSION == U(".gif"))
        ContentType = U("image/gif");
    else if (EXTENSION == U(".svg"))
        ContentType = U("image/svg+xml");
    else if (EXTENSION == U(".ico"))
        ContentType = U("image/x-icon");
    else if (EXTENSION == U(".mp4"))
        ContentType = U("video/mp4");
    else if (EXTENSION == U(".webm"))
        ContentType = U("video/webm");
    else if (EXTENSION == U(".mp3"))
        ContentType = U("audio/mpeg");
    else if (EXTENSION == U(".wav"))
        ContentType = U("audio/wav");
    else if (EXTENSION == U(".ogg"))
        ContentType = U("audio/ogg");
    else if (EXTENSION == U(".opus"))
        ContentType = U("audio/ogg");
    else if (EXTENSION == U(".flac"))
        ContentType = U("audio/flac");
    else if (EXTENSION == U(".aac"))
        ContentType = U("audio/aac");
    else
        ContentType = U("application/octet-stream"); // Default to binary stream for unknown file types

    // Prepend the static directory to the file name
    FileName = "static/" + FileName;

    // Stream the file to the response
    concurrency::streams::fstream::open_istream(FileName)
        .then(
            [Request, ContentType](concurrency::streams::istream StaticFileInputStream)
            {
                // Send the response
                Request.reply(web::http::status_codes::OK, StaticFileInputStream, ContentType);
            })
        .then(
            [FileName, Request](pplx::task<void> OpenStreamTask)
            {
                try
                {
                    OpenStreamTask.get();
                }
                catch (std::exception& Exception)
                {
                    std::cerr << Exception.what() << std::endl;
                    std::cout << Exception.what() << std::endl;

                    Request.reply(web::http::status_codes::NotFound,
                                  U("The file ") + FileName + U(" was not found: ") + std::string(Exception.what()));
                }
            });
}

void OrionWebServer::HandleAudioFileEndpoint(web::http::http_request Request)
{
    // Get the file name from the request path
    auto FileName = Request.request_uri().path();

    // Remove the leading slash
    FileName = FileName.substr(1);

    // Calculate the content type
    std::string ContentType = U("audio/mpeg");
    if (FileName.find(U(".opus")) != std::string::npos)
    {
        ContentType = U("audio/ogg");
    }
    else if (FileName.find(U(".flac")) != std::string::npos)
    {
        ContentType = U("audio/flac");
    }
    else if (FileName.find(U(".aac")) != std::string::npos)
    {
        ContentType = U("audio/aac");
    }
    else if (FileName.find(U(".wav")) != std::string::npos)
    {
        ContentType = U("audio/wav");
    }

    // Stream the file to the response
    concurrency::streams::fstream::open_istream(FileName)
        .then(
            [Request, ContentType](concurrency::streams::istream AudioFileInputStream)
            {
                // Send the response.  Make sure to specify the content type
                Request.reply(web::http::status_codes::OK, AudioFileInputStream, ContentType);
            })
        .then(
            [FileName, Request](pplx::task<void> OpenStreamTask)
            {
                try
                {
                    OpenStreamTask.get();
                }
                catch (std::exception& Exception)
                {
                    std::cerr << Exception.what() << std::endl;
                    std::cout << Exception.what() << std::endl;

                    Request.reply(web::http::status_codes::NotFound,
                                  U("The file ") + FileName + U(" was not found: ") + std::string(Exception.what()));
                }
            });
}

void OrionWebServer::HandleMarkdownEndpoint(web::http::http_request Request)
{
    // Get the message from the request body
    Request.extract_string()
        .then([this, Request](pplx::task<std::string> ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [this, Request](std::string RequestMessage)
            {
                // Convert the message to markdown
                auto        pMarkdown = cmark_markdown_to_html(RequestMessage.c_str(), RequestMessage.length(), CMARK_OPT_UNSAFE);
                std::string Markdown  = pMarkdown;
                free(pMarkdown);

                // Send the response
                Request.reply(web::http::status_codes::OK, Markdown)
                    .then(
                        [this](pplx::task<void> ReplyTask)
                        {
                            try
                            {
                                ReplyTask.get();
                            }
                            catch (std::exception& Exception)
                            {
                                std::cerr << Exception.what() << std::endl;
                                std::cout << Exception.what() << std::endl;
                            }
                        });
            });
}

void OrionWebServer::HandleChatHistoryEndpoint(web::http::http_request Request)
{
    // Check for the X-Orion-Id header
    if (!Request.headers().has(U("X-Orion-Id")))
    {
        Request.reply(web::http::status_codes::BadRequest, U("The X-Orion-Id header is required."));
        return;
    }

    // Get the Orion instance id from the header
    auto OrionID = Request.headers().find(U("X-Orion-Id"))->second;

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(), m_OrionInstances.end(),
                                [OrionID](const std::unique_ptr<Orion>& Orion) { return Orion->GetCurrentAssistantID() == OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        Request.reply(web::http::status_codes::BadRequest, U("The Orion instance with the given id was not found."));
        return;
    }

    // Get the chat history
    (*OrionIt)->GetChatHistoryAsync().then(
        [this, &Request](web::json::value ChatHistory)
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
                    free(pMarkdown);
                }
            }

            // Send the response
            Request.reply(web::http::status_codes::OK, ChatHistory);
        });
}

void OrionWebServer::HandleSpeakEndpoint(web::http::http_request Request)
{
    // Check for the X-Orion-Id header
    if (!Request.headers().has(U("X-Orion-Id")))
    {
        Request.reply(web::http::status_codes::BadRequest, U("The X-Orion-Id header is required."));
        return;
    }

    // Get the Orion instance id from the header
    auto OrionID = Request.headers().find(U("X-Orion-Id"))->second;

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(), m_OrionInstances.end(),
                                [OrionID](const std::unique_ptr<Orion>& Orion) { return Orion->GetCurrentAssistantID() == OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        Request.reply(web::http::status_codes::BadRequest, U("The Orion instance with the given id was not found."));
        return;
    }

    // Get the message from the request body
    Request.extract_string()
        .then([this, OrionIt, Request](pplx::task<std::string> ExtractJsonTask) { return ExtractJsonTask.get(); })
        .then(
            [this, OrionIt, Request](std::string RequestMessage)
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

                // Make Orion speak the message
                (*OrionIt)
                    ->SpeakAsync(RequestMessage, AudioFormat)
                    .then(
                        [this, Request]()
                        {
                            // Send the response
                            Request.reply(web::http::status_codes::OK);
                        });
            });
}

void OrionWebServer::HandleCreateOrionEndpoint(web::http::http_request Request)
{
    // Get the Orion instance id from the query parameter "id"
    auto Query   = Request.request_uri().query();
    auto OrionID = std::string();
    if (!Query.empty())
    {
        auto QueryParams = web::http::uri::split_query(web::http::uri::decode(Query));
        if (QueryParams.find(U("id")) != QueryParams.end())
        {
            OrionID = utility::conversions::to_utf8string(QueryParams.at(U("id")));
        }
    }

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

    // Create the Orion instance
    auto NewOrion = std::make_unique<Orion>(OrionID, std::move(Tools));

    // Initialize the Orion instance
    NewOrion->Initialize();
    OrionID = NewOrion->GetCurrentAssistantID();

    // Add the Orion instance to the list
    m_OrionInstances.push_back(std::move(NewOrion));

    // Send the response
    Request.reply(web::http::status_codes::OK, web::json::value::parse(U("{ \"id\": \"") + OrionID + U("\" }")));
}