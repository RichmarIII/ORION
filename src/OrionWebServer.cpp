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

void OrionWebServer::Start(int port)
{
    // Create a listener
    m_Listener = web::http::experimental::listener::http_listener(U("http://localhost:") + std::to_string(port));

    // Handle requests
    m_Listener.support(web::http::methods::POST, std::bind(&OrionWebServer::HandleRequest, this, std::placeholders::_1));
    m_Listener.support(web::http::methods::GET, std::bind(&OrionWebServer::HandleRequest, this, std::placeholders::_1));

    // Start the listener
    m_Listener.open().wait();

    // Set the running flag
    m_bRunning = true;
}

void OrionWebServer::Stop()
{
    // Close the listener
    m_bRunning = false;
    m_ConditionVariable.notify_one();
}

void OrionWebServer::Wait()
{
    // Aquire the lock
    std::unique_lock<std::mutex> Lock(m_Mutex);

    // Wait for the condition variable
    m_ConditionVariable.wait(Lock, [this] { return !m_bRunning; });

    // Close the listener
    m_Listener.close().wait();
}

void OrionWebServer::HandleRequest(web::http::http_request request)
{
    // Get the request path
    auto Path = request.request_uri().path();

    // Dispatch the request to the appropriate handler
    if (Path == U("/send_message"))
    {
        HandleSendMessageEndpoint(request);
    }
    else if (Path == U("/"))
    {
        HandleRootEndpoint(request);
    }
    else if (Path == U("/markdown"))
    {
        HandleMarkdownEndpoint(request);
    }
    else if (Path == U("/chat_history"))
    {
        HandleChatHistoryEndpoint(request);
    }
    else if (Path == U("/speak"))
    {
        HandleSpeakEndpoint(request);
    }
    else if (Path == U("/create_orion"))
    {
        HandleCreateOrionEndpoint(request);
    }
    // Check if /audio is in the request path
    else if (Path.find(U("/audio")) != std::string::npos)
    {
        HandleAudioFileEndpoint(request);
    }
    else
    {
        HandleStaticFileEndpoint(request);
    }
}

void OrionWebServer::HandleSendMessageEndpoint(web::http::http_request request)
{
    // Check for the X-Orion-Id header
    if (!request.headers().has(U("X-Orion-Id")))
    {
        request.reply(web::http::status_codes::BadRequest, U("The X-Orion-Id header is required."));
        return;
    }

    // Get the Orion instance id from the header
    auto OrionID = request.headers().find(U("X-Orion-Id"))->second;

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(), m_OrionInstances.end(),
                                [OrionID](const std::unique_ptr<Orion>& orion) { return orion->GetCurrentAssistantID() == OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        request.reply(web::http::status_codes::BadRequest, U("The Orion instance with the given id was not found."));
        return;
    }

    // Check if markdown is requested
    bool bMarkDown = request.request_uri().query().find(U("markdown=true")) != std::string::npos;

    // Get the message from the request body
    request.extract_string()
        .then([](pplx::task<std::string> task) { return task.get(); })
        .then(
            [OrionIt, bMarkDown, request](std::string message)
            {
                (*OrionIt)
                    ->SendMessageAsync(message)
                    .then([](pplx::task<std::string> task) { return task.get(); })
                    .then(
                        [bMarkDown, request](std::string response)
                        {
                            // Convert the message to markdown if requested
                            if (bMarkDown)
                            {
                                auto szMarkdown = cmark_markdown_to_html(response.c_str(), response.length(), CMARK_OPT_UNSAFE);
                                response        = szMarkdown;
                                free(szMarkdown);
                            }

                            // Send the response
                            request.reply(web::http::status_codes::OK, response);
                        });
            });
}

void OrionWebServer::HandleRootEndpoint(web::http::http_request request)
{
    // Find the index.html file
    std::ifstream IndexFile("templates/index.html");
    if (!IndexFile.is_open())
    {
        request.reply(web::http::status_codes::NotFound, U("The index.html file was not found."));
        return;
    }

    // Read the contents of the index.html file
    std::string IndexContents((std::istreambuf_iterator<char>(IndexFile)), std::istreambuf_iterator<char>());

    // Send the contents of the index.html file
    request.reply(web::http::status_codes::OK, IndexContents, U("text/html"));
}

void OrionWebServer::HandleStaticFileEndpoint(web::http::http_request request)
{
    // Get the file name from the request path
    auto FileName = request.request_uri().path();

    // Remove the leading slash
    FileName = FileName.substr(1);

    // Get the file extension
    const auto LastDotIndex = FileName.find_last_of('.');
    if (LastDotIndex == std::string::npos)
    {
        request.reply(web::http::status_codes::BadRequest, U("No file extension was found."));
        return;
    }

    // Get the file extension
    const auto Extension = FileName.substr(LastDotIndex);

    // Calculate the content type
    utility::string_t ContentType;
    if (Extension == U(".html"))
        ContentType = U("text/html");
    else if (Extension == U(".css"))
        ContentType = U("text/css");
    else if (Extension == U(".js"))
        ContentType = U("application/javascript");
    else if (Extension == U(".png"))
        ContentType = U("image/png");
    else if (Extension == U(".jpg") || Extension == U(".jpeg"))
        ContentType = U("image/jpeg");
    else if (Extension == U(".gif"))
        ContentType = U("image/gif");
    else if (Extension == U(".svg"))
        ContentType = U("image/svg+xml");
    else if (Extension == U(".ico"))
        ContentType = U("image/x-icon");
    else if (Extension == U(".mp4"))
        ContentType = U("video/mp4");
    else if (Extension == U(".webm"))
        ContentType = U("video/webm");
    else if (Extension == U(".mp3"))
        ContentType = U("audio/mpeg");
    else if (Extension == U(".wav"))
        ContentType = U("audio/wav");
    else if (Extension == U(".ogg"))
        ContentType = U("audio/ogg");
    else if (Extension == U(".opus"))
        ContentType = U("audio/ogg");
    else if (Extension == U(".flac"))
        ContentType = U("audio/flac");
    else if (Extension == U(".aac"))
        ContentType = U("audio/aac");
    else
        ContentType = U("application/octet-stream"); // Default to binary stream for unknown file types

    // Prepend the static directory to the file name
    FileName = "static/" + FileName;

    // Stream the file to the response
    concurrency::streams::fstream::open_istream(FileName)
        .then(
            [request, ContentType](concurrency::streams::istream is)
            {
                // Send the response
                request.reply(web::http::status_codes::OK, is, ContentType);
            })
        .then(
            [FileName, request](pplx::task<void> t)
            {
                try
                {
                    t.get();
                }
                catch (std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                    std::cout << e.what() << std::endl;

                    request.reply(web::http::status_codes::NotFound, U("The file ") + FileName + U(" was not found."));
                }
            });
}

void OrionWebServer::HandleAudioFileEndpoint(web::http::http_request request)
{
    // Get the file name from the request path
    auto FileName = request.request_uri().path();

    // Remove the leading slash
    FileName = FileName.substr(1);

    // Calculate the content type
    auto ContentType = U("audio/mpeg");
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
            [request, ContentType](concurrency::streams::istream is)
            {
                // Send the response.  Make sure to specify the content type
                request.reply(web::http::status_codes::OK, is, ContentType);
            })
        .then(
            [FileName, request](pplx::task<void> t)
            {
                try
                {
                    t.get();
                }
                catch (std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                    std::cout << e.what() << std::endl;

                    request.reply(web::http::status_codes::NotFound, U("The file ") + FileName + U(" was not found."));
                }
            });
}

void OrionWebServer::HandleMarkdownEndpoint(web::http::http_request request)
{
    // Get the message from the request body
    request.extract_string()
        .then([this, request](pplx::task<std::string> task) { return task.get(); })
        .then(
            [this, request](std::string message)
            {
                // Convert the message to markdown
                auto        szMarkdown = cmark_markdown_to_html(message.c_str(), message.length(), CMARK_OPT_UNSAFE);
                std::string Markdown   = szMarkdown;
                free(szMarkdown);

                // Send the response
                request.reply(web::http::status_codes::OK, Markdown)
                    .then(
                        [this](pplx::task<void> t)
                        {
                            try
                            {
                                t.get();
                            }
                            catch (std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                std::cout << e.what() << std::endl;
                            }
                        });
            });
}

void OrionWebServer::HandleChatHistoryEndpoint(web::http::http_request request)
{
    // Check for the X-Orion-Id header
    if (!request.headers().has(U("X-Orion-Id")))
    {
        request.reply(web::http::status_codes::BadRequest, U("The X-Orion-Id header is required."));
        return;
    }

    // Get the Orion instance id from the header
    auto OrionID = request.headers().find(U("X-Orion-Id"))->second;

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(), m_OrionInstances.end(),
                                [OrionID](const std::unique_ptr<Orion>& orion) { return orion->GetCurrentAssistantID() == OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        request.reply(web::http::status_codes::BadRequest, U("The Orion instance with the given id was not found."));
        return;
    }

    // Get the chat history
    (*OrionIt)->GetChatHistoryAsync().then(
        [this, &request](web::json::value chatHistory)
        {
            // Check if the query parameter is present
            bool bMarkDown = request.request_uri().query().find(U("markdown=true")) != std::string::npos;

            // Convert the chat history to markdown if the query parameter is present
            if (bMarkDown)
            {
                for (auto& JMessage : chatHistory.as_array())
                {
                    auto Message           = JMessage.at(U("message")).as_string();
                    auto szMarkdown        = cmark_markdown_to_html(Message.c_str(), Message.length(), CMARK_OPT_UNSAFE);
                    JMessage[U("message")] = web::json::value::string(szMarkdown);
                    free(szMarkdown);
                }
            }

            // Send the response
            request.reply(web::http::status_codes::OK, chatHistory);
        });
}

void OrionWebServer::HandleSpeakEndpoint(web::http::http_request request)
{
    // Check for the X-Orion-Id header
    if (!request.headers().has(U("X-Orion-Id")))
    {
        request.reply(web::http::status_codes::BadRequest, U("The X-Orion-Id header is required."));
        return;
    }

    // Get the Orion instance id from the header
    auto OrionID = request.headers().find(U("X-Orion-Id"))->second;

    // Find the Orion instance with the given id
    auto OrionIt = std::find_if(m_OrionInstances.begin(), m_OrionInstances.end(),
                                [OrionID](const std::unique_ptr<Orion>& orion) { return orion->GetCurrentAssistantID() == OrionID; });

    // Check if the Orion instance was found
    if (OrionIt == m_OrionInstances.end())
    {
        request.reply(web::http::status_codes::BadRequest, U("The Orion instance with the given id was not found."));
        return;
    }

    // Get the message from the request body
    request.extract_string()
        .then([this, OrionIt, request](pplx::task<std::string> task) { return task.get(); })
        .then(
            [this, OrionIt, request](std::string message)
            {
                // Get the audio format from the query parameter
                ETTSAudioFormat eAudioFormat = ETTSAudioFormat::Mp3;
                if (request.request_uri().query().find(U("format=opus")) != std::string::npos)
                {
                    eAudioFormat = ETTSAudioFormat::Opus;
                }
                else if (request.request_uri().query().find(U("format=flac")) != std::string::npos)
                {
                    eAudioFormat = ETTSAudioFormat::Flac;
                }
                else if (request.request_uri().query().find(U("format=aac")) != std::string::npos)
                {
                    eAudioFormat = ETTSAudioFormat::AAC;
                }
                else if (request.request_uri().query().find(U("format=pcm")) != std::string::npos)
                {
                    eAudioFormat = ETTSAudioFormat::PCM;
                }
                else if (request.request_uri().query().find(U("format=wav")) != std::string::npos)
                {
                    eAudioFormat = ETTSAudioFormat::Wav;
                }

                // Make Orion speak the message
                (*OrionIt)
                    ->SpeakAsync(message, eAudioFormat)
                    .then([this, request](pplx::task<web::json::value> task) { return task.get(); })
                    .then(
                        [request](web::json::value audioFiles)
                        {
                            // Send the response
                            request.reply(web::http::status_codes::OK, audioFiles);
                        });
            });
}

void OrionWebServer::HandleCreateOrionEndpoint(web::http::http_request request)
{
    // Get the Orion instance id from the query parameter "id"
    auto Query   = request.request_uri().query();
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
    request.reply(web::http::status_codes::OK, web::json::value::parse(U("{ \"id\": \"") + OrionID + U("\" }")));
}