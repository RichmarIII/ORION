#include "Orion.hpp"
#include "tools/FunctionTool.hpp"

// Include cpprestsdk headers
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>
#include <cpprest/ws_client.h>

// Include cmark headers
#include <cmark.h>

// Include standard headers
#include <chrono>
#include <thread>
#include <filesystem>

using namespace ORION;

Orion::Orion(std::vector<std::unique_ptr<IOrionTool>>&& tools, const EOrionIntelligence eIntelligence, const EOrionVoice eVoice, const char* szName,
             const char* szInstructions, const char* szDescription)
    : m_Tools(std::move(tools)),
      m_CurrentIntelligence(eIntelligence),
      m_CurrentVoice(eVoice),
      m_Name(szName),
      m_Instructions(szInstructions),
      m_Description(szDescription)
{
    m_Instructions = "Your name is " + m_Name + ". " + m_Instructions;
}

void Orion::Run()
{
    // Create a new OpenAI client
    CreateClient();

    // Create assistant
    CreateAssistant();

    // Create a thread
    CreateThread();

    // Create a web server and start listening for requests using cpprestsdk

    web::http::experimental::listener::http_listener listener("http://localhost:5000");

    // Add a listener for GET requests
    listener.support(web::http::methods::GET, std::bind(&Orion::HandleGetRequest, this, std::placeholders::_1));

    // Add a listener for POST requests
    listener.support(web::http::methods::POST, std::bind(&Orion::HandlePostRequest, this, std::placeholders::_1));

    // Start listening for requests
    listener.open().then([&]() { std::cout << "Listening for requests at http://localhost:5000" << std::endl; });

    // Set the running flag to true
    m_Running = true;

    // Wait for the condition variable to stop the server
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        cv.wait(lock, [&]() { return !m_Running; });
    }

    // Close the listener
    listener.close().then([&]() { std::cout << "Stopped listening for requests" << std::endl; });
}

void Orion::Shutdown()
{
    // Set the running flag to false
    m_Running = false;

    // Notify the condition variable
    cv.notify_one();
}

std::vector<std::string> Orion::SendMessage(const std::string& message)
{
    // Create a message in the openai thread
    web::http::http_request request(web::http::methods::POST);
    request.set_request_uri(U("threads/" + m_CurrentThreadID + "/messages"));
    request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    request.headers().add("OpenAI-Beta", "assistants=v1");
    request.headers().add("Content-Type", "application/json");

    web::json::value body = web::json::value::object();
    body["content"]       = web::json::value::string(message);
    body["role"]          = web::json::value::string("user");
    request.set_body(body);

    web::http::http_response response = m_OpenAIClient->request(request).get();

    if (response.status_code() != web::http::status_codes::OK)
    {
        std::cerr << "Failed to create a new message" << std::endl;
        std::cout << response.to_string() << std::endl;
        return {};
    }

    // run the assistant
    request = web::http::http_request(web::http::methods::POST);
    request.set_request_uri(U("threads/" + m_CurrentThreadID + "/runs"));
    request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    request.headers().add("OpenAI-Beta", "assistants=v1");
    request.headers().add("Content-Type", "application/json");

    body                 = web::json::value::object();
    body["assistant_id"] = web::json::value::string(m_CurrentAssistantID);

    // Set the model to the current model
    body["model"] = web::json::value::string(m_CurrentIntelligence == EOrionIntelligence::Base ? "gpt-3.5-turbo" : "gpt-4-turbo-preview");
    request.set_body(body);

    response = m_OpenAIClient->request(request).get();

    if (response.status_code() != web::http::status_codes::OK)
    {
        std::cerr << "Failed to run the assistant" << std::endl;
        std::cout << response.to_string() << std::endl;
        return {};
    }

    std::string runID = response.extract_json().get().at("id").as_string();

    // Get the response from the openai assistant
    while (true)
    {
        request = web::http::http_request(web::http::methods::GET);
        request.set_request_uri(U("threads/" + m_CurrentThreadID + "/runs/" + runID));
        request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
        request.headers().add("OpenAI-Beta", "assistants=v1");

        response = m_OpenAIClient->request(request).get();

        if (response.status_code() != web::http::status_codes::OK)
        {
            std::cerr << "Failed to get the response from the assistant" << std::endl;
            std::cout << response.to_string() << std::endl;
            return {};
        }

        web::json::value json = response.extract_json().get();

        if (json.at("status").as_string() == "completed")
        {
            break;
        }
        else if (json.at("status").as_string() == "failed")
        {
            return {"An error occurred while processing the request"};
        }
        else if (json.at("status").as_string() == "expired")
        {
            return {"The request has expired"};
        }
        else if (json.at("status").as_string() == "cancelled")
        {
            return {"The request has been cancelled"};
        }
        else if (json.at("status").as_string() == "queued")
        {
            // Do nothing
        }
        else if (json.at("status").as_string() == "requires_action")
        {
            // Check if the thread wants to run a tool
            web::json::value toolCallResults = web::json::value::array();
            for (const auto& toolCall : json.at("required_action").at("submit_tool_outputs").at("tool_calls").as_array())
            {
                if (toolCall.at("type").as_string() == "function")
                {
                    // Get tool from available tools by name
                    auto tool = std::find_if(m_Tools.begin(), m_Tools.end(),
                                             [&](const auto& t) { return t->GetName() == toolCall.at("function").at("name").as_string(); });

                    if (tool != m_Tools.end())
                    {
                        // Call the tool
                        auto pFunctionTool = static_cast<FunctionTool*>(tool->get());
                        auto arguments     = toolCall.at("function").at("arguments").as_string();
                        auto output        = pFunctionTool->Execute(*this, web::json::value::parse(arguments));

                        // Append the tool call results
                        web::json::value toolCallResult         = web::json::value::object();
                        toolCallResult["tool_call_id"]          = toolCall.at("id");
                        toolCallResult["output"]                = web::json::value::string(output);
                        toolCallResults[toolCallResults.size()] = toolCallResult;
                    }
                }
            }

            // Submit the tool call results
            request = web::http::http_request(web::http::methods::POST);
            request.set_request_uri(U("threads/" + m_CurrentThreadID + "/runs/" + runID + "/submit_tool_outputs"));
            request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
            request.headers().add("OpenAI-Beta", "assistants=v1");
            request.headers().add("Content-Type", "application/json");

            auto tool_outputs            = web::json::value::object();
            tool_outputs["tool_outputs"] = toolCallResults;
            request.set_body(tool_outputs);

            response = m_OpenAIClient->request(request).get();

            if (response.status_code() != web::http::status_codes::OK)
            {
                std::cerr << "Failed to submit the tool call results" << std::endl;
                std::cout << response.to_string() << std::endl;
                return {};
            }
        }
        else if (json.at("status").as_string() == "in_progress")
        {
            // Do nothing
        }
        else
        {
            std::cerr << "Unknown status: " << json.at("status").as_string() << std::endl;
            std::cout << response.to_string() << std::endl;
            return {};
        }

        // Sleep for 1 second before checking the status again
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Get new messages from the assistant
    request = web::http::http_request(web::http::methods::GET);
    request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    request.headers().add("OpenAI-Beta", "assistants=v1");
    request.headers().add("Content-Type", "application/json");

    // Set the request URI
    web::uri_builder builder;
    builder.append_path("threads/" + m_CurrentThreadID + "/messages");
    builder.append_query("limit", 5);
    request.set_request_uri(builder.to_string());

    response = m_OpenAIClient->request(request).get();

    if (response.status_code() != web::http::status_codes::OK)
    {
        std::cerr << "Failed to get the response from the assistant" << std::endl;
        return {};
    }

    web::json::value threadMessages = response.extract_json().get();

    std::vector<std::string> NewMessages;
    for (const auto& threadMessage : threadMessages.at("data").as_array())
    {
        if (threadMessage.at("role").as_string() == "assistant")
        {
            for (const auto& content : threadMessage.at("content").as_array())
            {
                if (content.at("type").as_string() == "text")
                {
                    NewMessages.push_back(content.at("text").at("value").as_string());
                }
            }
        }
        else if (threadMessage.at("role").as_string() == "user")
        {
            break;
        }
    }

    // Combine the messages
    std::string CombinedMessages;
    for (const auto& message : NewMessages)
    {
        CombinedMessages += message;
    }

    // Speak the message
    SpeakAsync(CombinedMessages);

    // Use cmark to convert markdown to HTML
    for (auto& message : NewMessages)
    {
        auto szmessage = cmark_markdown_to_html(message.c_str(), message.size(), CMARK_OPT_DEFAULT);
        message        = szmessage;
        free(szmessage);
    }

    return NewMessages;
}

void Orion::CreateAssistant()
{
    // Check if an assistant already exists on the server
    web::http::http_request request(web::http::methods::GET);
    request.set_request_uri(U("assistants"));
    request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    request.headers().add("OpenAI-Beta", "assistants=v1");

    web::http::http_response response = m_OpenAIClient->request(request).get();

    if (response.status_code() == web::http::status_codes::OK)
    {
        web::json::value json = response.extract_json().get();
        for (const auto& assistant : json.at("data").as_array())
        {
            std::cout << assistant.at("name").serialize();
            if (assistant.at("name").as_string() == m_Name)
            {
                m_CurrentAssistantID = assistant.at("id").as_string();
                break;
            }
        }
    }
    else
    {
        std::cerr << "Failed to get the list of assistants" << std::endl;
        return;
    }

    if (!m_CurrentAssistantID.empty())
    {
        // Update the assistant
        web::http::http_request request(web::http::methods::POST);
        request.set_request_uri(U("assistants/" + m_CurrentAssistantID));
        request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
        request.headers().add("OpenAI-Beta", "assistants=v1");
        request.headers().add("Content-Type", "application/json");

        web::json::value body = web::json::value::object();
        body["instructions"]  = web::json::value::string(m_Instructions);
        body["description"]   = web::json::value::string(m_Description);
        body["model"]         = web::json::value::string(m_CurrentIntelligence == EOrionIntelligence::Base ? "gpt-3.5-turbo" : "gpt-4-turbo-preview");

        if (!m_Tools.empty())
        {
            web::json::value tools = web::json::value::array();
            for (const auto& tool : m_Tools)
            {
                printf("Adding tool: %s\n", tool->GetName().c_str());
                tools[tools.size()] = web::json::value::parse(tool->ToJson());
            }
            body["tools"] = tools;
        }

        request.set_body(body);

        web::http::http_response response = m_OpenAIClient->request(request).get();

        if (response.status_code() != web::http::status_codes::OK)
        {
            std::cerr << "Failed to update the assistant" << std::endl;

            // Print the response
            std::cout << response.to_string() << std::endl;
        }
    }
    else
    {
        // Create a new assistant
        web::http::http_request request(web::http::methods::POST);
        request.set_request_uri(U("assistants"));
        request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
        request.headers().add("OpenAI-Beta", "assistants=v1");
        request.headers().add("Content-Type", "application/json");

        web::json::value body = web::json::value::object();
        body["name"]          = web::json::value::string(m_Name);
        body["instructions"]  = web::json::value::string(m_Instructions);
        body["description"]   = web::json::value::string(m_Description);
        body["model"]         = web::json::value::string(m_CurrentIntelligence == EOrionIntelligence::Base ? "gpt-3.5-turbo" : "gpt-4-turbo-preview");

        if (!m_Tools.empty())
        {
            web::json::value tools = web::json::value::array();
            for (const auto& tool : m_Tools)
            {
                printf("Adding tool: %s\n", tool->GetName().c_str());
                tools[tools.size()] = web::json::value::parse(tool->ToJson());
            }
            body["tools"] = tools;
        }

        request.set_body(body);

        web::http::http_response response = m_OpenAIClient->request(request).get();

        if (response.status_code() == web::http::status_codes::OK)
        {
            web::json::value json = response.extract_json().get();
            m_CurrentAssistantID  = json.at("id").as_string();
        }
        else
        {
            std::cerr << "Failed to create a new assistant" << std::endl;

            // Print the response
            std::cout << response.to_string() << std::endl;
        }
    }
}

void Orion::CreateThread()
{
    // Create a new thread
    web::http::http_request request(web::http::methods::POST);
    request.set_request_uri(U("threads"));
    request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    request.headers().add("OpenAI-Beta", "assistants=v1");
    request.headers().add("Content-Type", "application/json");
    request.set_body(web::json::value::object());

    web::http::http_response response = m_OpenAIClient->request(request).get();

    if (response.status_code() == web::http::status_codes::OK)
    {
        web::json::value json = response.extract_json().get();
        m_CurrentThreadID     = json.at("id").as_string();
    }
    else
    {
        std::cerr << "Failed to create a new thread" << std::endl;
    }
}

void Orion::HandleGetRequest(web::http::http_request request)
{
    if (request.request_uri().path() == "/")
    {
        // Handle the root request
        HandleRootRequest(request);
    }
    else if (request.request_uri().path() == "/quit")
    {
        // Handle the shutdown request
        HandleShutdownRequest(request);
    }
    else
    {
        HandleStaticFileRequest(request);
    }
}

void Orion::HandlePostRequest(web::http::http_request request)
{
    // Check if endpoint is /send_message
    if (request.request_uri().path() == "/send_message")
    {
        // Handle the send message request
        HandleSendMessageRequest(request);
    }
    // Check if endpoint is /message_to_markdown
    else if (request.request_uri().path() == "/message_to_markdown")
    {
        // Handle the message to markdown request
        HandleMessageToMarkdownRequest(request);
    }
    else
    {
        // Send a 404 Not Found error to the client
        request.reply(web::http::status_codes::NotFound, "The requested resource was not found\n");
    }
}

void Orion::CreateClient()
{
    // Create a client to communicate with the OpenAI API
    m_OpenAIClient = std::make_unique<web::http::client::http_client>(U("https://api.openai.com/v1/"));

    // Load the OpenAI API key from the environment or a file
    {
        if (auto Key = std::getenv("OPENAI_API_KEY"); Key)
        {
            m_OpenAIAPIKey = Key;
        }
        else
        {
            // Load the OpenAI API key from a file
            std::ifstream APIKey {".openai_api_key.txt"};
            m_OpenAIAPIKey = std::string {std::istreambuf_iterator<char>(APIKey), std::istreambuf_iterator<char>()};
        }
        if (m_OpenAIAPIKey.empty())
        {
            std::cerr << "OpenAI API key not found" << std::endl;
            return;
        }
    }

    // Load the OpenWeather API key from the environment or a file
    {
        if (auto Key = std::getenv("OPENWEATHER_API_KEY"); Key)
        {
            m_OpenWeatherAPIKey = Key;
        }
        else
        {
            // Load the OpenWeather API key from a file
            std::ifstream APIKey {".openweather_api_key.txt"};
            m_OpenWeatherAPIKey = std::string {std::istreambuf_iterator<char>(APIKey), std::istreambuf_iterator<char>()};
        }
        if (m_OpenWeatherAPIKey.empty())
        {
            std::cerr << "OpenWeather API key not found" << std::endl;
            return;
        }
    }
}

void Orion::SetNewVoice(const EOrionVoice voice)
{
    m_CurrentVoice = voice;
}

void Orion::SetNewIntelligence(const EOrionIntelligence intelligence)
{
    m_CurrentIntelligence = intelligence;
}

void Orion::HandleSendMessageRequest(web::http::http_request request)
{
    // Get the message from the request
    request.extract_json().then(
        [=](pplx::task<web::json::value> task)
        {
            try
            {
                web::json::value json    = task.get();
                std::string      message = json.at("message").as_string();

                // Send the message to the assistant
                std::vector<std::string> NewMessages = SendMessage(message);

                if (NewMessages.empty())
                {
                    request.reply(web::http::status_codes::BadRequest, "Failed to send the message to the assistant\n");
                    return;
                }

                // Create a json array of the new messages
                web::json::value jsonMessages = web::json::value::array(NewMessages.size());
                for (size_t i = 0; i < NewMessages.size(); i++)
                {
                    jsonMessages[i] = web::json::value::string(NewMessages[i]);
                }

                // Create a new json object to send to the client
                web::json::value jsonResponse = web::json::value::object();
                jsonResponse["messages"]      = jsonMessages;

                // Send the new messages to the client
                request.reply(web::http::status_codes::OK, jsonResponse);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Failed to extract the message from the request: " << e.what() << std::endl;
                // Send a 400 Bad Request error to the client
                auto json        = web::json::value::object();
                auto messages    = web::json::value::array(1);
                messages[0]      = web::json::value::string("Failed to extract the message from the request");
                json["messages"] = messages;
                request.reply(web::http::status_codes::BadRequest, json);
            }
        });
}

void Orion::HandleRootRequest(web::http::http_request request)
{
    // Render the templates/index.html template and send it to the client
    std::ifstream indexFile {"templates/index.html"};
    std::string   indexHTML {std::istreambuf_iterator<char>(indexFile), std::istreambuf_iterator<char>()};
    request.reply(web::http::status_codes::OK, indexHTML, "text/html");
}

void Orion::HandleShutdownRequest(web::http::http_request request)
{
    // Send a message to the client
    request.reply(web::http::status_codes::OK, "ORION is shutting down\n");

    // Shutdown the Orion instance
    Shutdown();
}

void Orion::HandleStaticFileRequest(web::http::http_request request)
{
    // All static files are in the /static directory
    std::string path = "static" + request.request_uri().path();

    // Check if the file exists
    if (std::filesystem::exists(path))
    {
        // Open the file
        std::ifstream file {path};

        // Get the file extension
        std::string extension = path.substr(path.find_last_of('.') + 1);

        // Set the content type based on the file extension
        std::string contentType;
        if (extension == "html")
        {
            contentType = "text/html";
        }
        else if (extension == "css")
        {
            contentType = "text/css";
        }
        else if (extension == "js")
        {
            contentType = "application/javascript";
        }
        else if (extension == "png")
        {
            contentType = "image/png";
        }
        else if (extension == "jpg" || extension == "jpeg")
        {
            contentType = "image/jpeg";
        }
        else if (extension == "gif")
        {
            contentType = "image/gif";
        }
        else if (extension == "svg")
        {
            contentType = "image/svg+xml";
        }
        else if (extension == "ico")
        {
            contentType = "image/x-icon";
        }
        else
        {
            contentType = "text/plain";
        }

        auto fileContents = std::string {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

        // Send the file to the client
        request.reply(web::http::status_codes::OK, fileContents, contentType);
    }
    else
    {
        // Send a 404 Not Found error to the client
        request.reply(web::http::status_codes::NotFound, "The requested resource was not found\n");
    }
}

void Orion::HandleMessageToMarkdownRequest(web::http::http_request request)
{
    // Get the message from the request
    request.extract_json().then(
        [=](pplx::task<web::json::value> task)
        {
            try
            {
                web::json::value json    = task.get();
                std::string      message = json.at("message").as_string();

                // Use cmark to convert the message to markdown
                auto szmessage = cmark_markdown_to_html(message.c_str(), message.size(), CMARK_OPT_DEFAULT);
                message        = szmessage;
                free(szmessage);

                // Create a new json object to send to the client
                web::json::value jsonResponse = web::json::value::object();
                jsonResponse["message"]       = web::json::value::string(message);

                // Send the new message to the client
                request.reply(web::http::status_codes::OK, jsonResponse);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Failed to convert message to markdown: " << e.what() << std::endl;

                // Send a 400 Bad Request error to the client
                request.reply(web::http::status_codes::BadRequest,
                              R"({"message": "Failed to convert message to markdown: )" + std::string(e.what()) + R"("})");
            }
        });
}

pplx::task<void> Orion::SpeakAsync(const std::string& message)
{
    // Create a new http_request to get the speech
    web::http::http_request request(web::http::methods::POST);
    request.set_request_uri(U("audio/speech"));
    request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    request.headers().add("Content-Type", "application/json");

    // Add the body to the request
    web::json::value body = web::json::value::object();
    body["model"]         = web::json::value::string("tts-1");
    body["input"]         = web::json::value::string(message);

    // Set the voice based on the current voice
    if (m_CurrentVoice == EOrionVoice::Alloy)
    {
        body["voice"] = web::json::value::string("alloy");
    }
    else if (m_CurrentVoice == EOrionVoice::Echo)
    {
        body["voice"] = web::json::value::string("echo");
    }
    else if (m_CurrentVoice == EOrionVoice::Fable)
    {
        body["voice"] = web::json::value::string("fable");
    }
    else if (m_CurrentVoice == EOrionVoice::Nova)
    {
        body["voice"] = web::json::value::string("nova");
    }
    else if (m_CurrentVoice == EOrionVoice::Onyx)
    {
        body["voice"] = web::json::value::string("onyx");
    }
    else if (m_CurrentVoice == EOrionVoice::Shimmer)
    {
        body["voice"] = web::json::value::string("shimmer");
    }

    request.set_body(body);

    // Send the request and get the response
    return m_OpenAIClient->request(request).then(
        [=](web::http::http_response response)
        {
            if (response.status_code() == web::http::status_codes::OK)
            {
                return concurrency::streams::fstream::open_ostream("speech.mp3", std::ios::out | std::ios::binary | std::ios::trunc)
                    .then([=](concurrency::streams::ostream fileStream) { return response.body().read_to_end(fileStream.streambuf()); })
                    .then(
                        [=](size_t)
                        {
                            std::cout << "File downloaded to speech.mp3" << std::endl;

                // Play the speech

#if defined(__unix__) || defined(__unix) || defined(__APPLE__)
                            // TODO: Not cross-platform
                            std::string command = "mpg123 speech.mp3";
                            system(command.c_str());
#endif
                        });
            }
            else
            {
                std::cerr << "Failed to get the speech" << std::endl;
                std::cout << response.to_string() << std::endl;

                return pplx::task_from_result();
            }
        });
}
