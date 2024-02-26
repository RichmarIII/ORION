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

Orion::Orion(const std::string& ID, std::vector<std::unique_ptr<IOrionTool>>&& tools, const EOrionIntelligence eIntelligence,
             const EOrionVoice eVoice, const char* szName, const char* szInstructions, const char* szDescription)
    : m_Tools(std::move(tools)),
      m_CurrentIntelligence(eIntelligence),
      m_CurrentVoice(eVoice),
      m_Name(szName),
      m_Instructions(szInstructions),
      m_Description(szDescription)
{
    m_Instructions = "Your name is " + m_Name + ". " + m_Instructions;

    // Generate a guid for the Orion instance
    if (!ID.empty())
    {
        m_CurrentAssistantID = ID;
    }
}

bool ORION::Orion::Initialize()
{
    CreateClient();
    CreateAssistant();
    CreateThread();

    return true;
}

pplx::task<std::string> Orion::SendMessageAsync(const std::string& message)
{
    // Create a message in the openai thread
    web::http::http_request CreateMessageRequest(web::http::methods::POST);
    CreateMessageRequest.set_request_uri(U("threads/" + m_CurrentThreadID + "/messages"));
    CreateMessageRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    CreateMessageRequest.headers().add("OpenAI-Beta", "assistants=v1");
    CreateMessageRequest.headers().add("Content-Type", "application/json");

    web::json::value CreateMessageBody = web::json::value::object();
    CreateMessageBody["content"]       = web::json::value::string(message);
    CreateMessageBody["role"]          = web::json::value::string("user");
    CreateMessageRequest.set_body(CreateMessageBody);

    return m_OpenAIClient->request(CreateMessageRequest)
        .then(
            [this](web::http::http_response createMessageResponse)
            {
                if (createMessageResponse.status_code() != web::http::status_codes::OK)
                {
                    std::cerr << "Failed to create a new message" << std::endl;
                    std::cout << createMessageResponse.to_string() << std::endl;
                    return pplx::task_from_result(std::string("Failed to create a new message: " + createMessageResponse.to_string()));
                }

                // run the assistant
                auto CreateRunRequest = web::http::http_request(web::http::methods::POST);
                CreateRunRequest.set_request_uri(U("threads/" + m_CurrentThreadID + "/runs"));
                CreateRunRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
                CreateRunRequest.headers().add("OpenAI-Beta", "assistants=v1");
                CreateRunRequest.headers().add("Content-Type", "application/json");

                auto CreateRunBody            = web::json::value::object();
                CreateRunBody["assistant_id"] = web::json::value::string(m_CurrentAssistantID);

                // Set the model to the current model
                CreateRunBody["model"] =
                    web::json::value::string(m_CurrentIntelligence == EOrionIntelligence::Base ? "gpt-3.5-turbo" : "gpt-4-turbo-preview");
                CreateRunRequest.set_body(CreateRunBody);

                return m_OpenAIClient->request(CreateRunRequest)
                    .then(
                        [this](web::http::http_response createRunResponse)
                        {
                            if (createRunResponse.status_code() != web::http::status_codes::OK)
                            {
                                std::cerr << "Failed to run the assistant" << std::endl;
                                std::cout << createRunResponse.to_string() << std::endl;
                                return pplx::task_from_result(std::string("Failed to run the assistant: " + createRunResponse.to_string()));
                            }

                            std::string runID = createRunResponse.extract_json().get().at("id").as_string();

                            // Get the response from the openai assistant
                            while (true)
                            {
                                auto CheckRunStatusRequest = web::http::http_request(web::http::methods::GET);
                                CheckRunStatusRequest.set_request_uri(U("threads/" + m_CurrentThreadID + "/runs/" + runID));
                                CheckRunStatusRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
                                CheckRunStatusRequest.headers().add("OpenAI-Beta", "assistants=v1");

                                auto CheckRunStatusResponse = m_OpenAIClient->request(CheckRunStatusRequest).get();

                                if (CheckRunStatusResponse.status_code() != web::http::status_codes::OK)
                                {
                                    std::cerr << "Failed to get the response from the assistant" << std::endl;
                                    std::cout << CheckRunStatusResponse.to_string() << std::endl;
                                    return pplx::task_from_result(
                                        std::string("Failed to get the response from the assistant: " + CheckRunStatusResponse.to_string()));
                                }

                                web::json::value json = CheckRunStatusResponse.extract_json().get();

                                if (json.at("status").as_string() == "completed")
                                {
                                    break;
                                }
                                else if (json.at("status").as_string() == "failed")
                                {
                                    return pplx::task_from_result(std::string("The request has failed"));
                                }
                                else if (json.at("status").as_string() == "expired")
                                {
                                    return pplx::task_from_result(std::string("The request has expired"));
                                }
                                else if (json.at("status").as_string() == "cancelled")
                                {
                                    return pplx::task_from_result(std::string("The request has been cancelled"));
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
                                                                     [&](const auto& t)
                                                                     { return t->GetName() == toolCall.at("function").at("name").as_string(); });

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
                                    auto SubmitToolOutputsRequest = web::http::http_request(web::http::methods::POST);
                                    SubmitToolOutputsRequest.set_request_uri(
                                        U("threads/" + m_CurrentThreadID + "/runs/" + runID + "/submit_tool_outputs"));
                                    SubmitToolOutputsRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
                                    SubmitToolOutputsRequest.headers().add("OpenAI-Beta", "assistants=v1");
                                    SubmitToolOutputsRequest.headers().add("Content-Type", "application/json");

                                    auto tool_outputs            = web::json::value::object();
                                    tool_outputs["tool_outputs"] = toolCallResults;
                                    SubmitToolOutputsRequest.set_body(tool_outputs);

                                    auto SubmitToolOutputsResponse = m_OpenAIClient->request(SubmitToolOutputsRequest).get();

                                    if (SubmitToolOutputsResponse.status_code() != web::http::status_codes::OK)
                                    {
                                        std::cerr << "Failed to submit the tool call results" << std::endl;
                                        std::cout << SubmitToolOutputsResponse.to_string() << std::endl;
                                        return pplx::task_from_result(
                                            std::string("Failed to submit the tool call results: " + SubmitToolOutputsResponse.to_string()));
                                    }
                                }
                                else if (json.at("status").as_string() == "in_progress")
                                {
                                    // Do nothing
                                }
                                else
                                {
                                    std::cerr << "Unknown status: " << json.at("status").as_string() << std::endl;
                                    std::cout << CheckRunStatusResponse.to_string() << std::endl;
                                    return pplx::task_from_result(std::string("Unknown status: " + json.at("status").as_string()));
                                }

                                // Sleep for 1 second before checking the status again
                                std::this_thread::sleep_for(std::chrono::seconds(1));
                            }

                            // Get new messages from the assistant
                            auto ListMessagesRequest = web::http::http_request(web::http::methods::GET);
                            ListMessagesRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
                            ListMessagesRequest.headers().add("OpenAI-Beta", "assistants=v1");
                            ListMessagesRequest.headers().add("Content-Type", "application/json");

                            // Set the request URI
                            web::uri_builder builder;
                            builder.append_path("threads/" + m_CurrentThreadID + "/messages");
                            builder.append_query("limit", 5);
                            ListMessagesRequest.set_request_uri(builder.to_string());

                            auto ListMessagesResponse = m_OpenAIClient->request(ListMessagesRequest).get();

                            if (ListMessagesResponse.status_code() != web::http::status_codes::OK)
                            {
                                std::cerr << "Failed to get the response from the assistant" << std::endl;
                                return pplx::task_from_result(
                                    std::string("Failed to get the response from the assistant: " + ListMessagesResponse.to_string()));
                            }

                            web::json::value threadMessages = ListMessagesResponse.extract_json().get();

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
                            std::string CombinedMessage;
                            for (const auto& message : NewMessages)
                            {
                                CombinedMessage += message + " \n";
                            }

                            return pplx::task_from_result(CombinedMessage);
                        });
            });
}

void Orion::CreateAssistant()
{
    // Check if an assistant already exists on the server
    web::http::http_request request(web::http::methods::GET);
    request.set_request_uri(U("assistants"));
    request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    request.headers().add("OpenAI-Beta", "assistants=v1");

    web::http::http_response response = m_OpenAIClient->request(request).get();

    bool bAssistantExists = false;
    if (response.status_code() == web::http::status_codes::OK)
    {
        web::json::value json = response.extract_json().get();
        for (const auto& assistant : json.at("data").as_array())
        {
            if (assistant.at("id").as_string() == m_CurrentAssistantID)
            {
                bAssistantExists = true;
                break;
            }
        }
    }
    else
    {
        std::cerr << "Failed to get the list of assistants" << std::endl;
        return;
    }

    if (bAssistantExists)
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
                // Create the audio directory if it doesn't exist
                std::error_code ec;
                std::filesystem::create_directory("audio", ec);
                if (ec)
                {
                    std::cerr << "Failed to create the audio directory" << std::endl;
                    return pplx::task_from_result();
                }

                // Generate a unique filename for the speech
                std::string FileName = "audio/speech_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".mp3";
                return concurrency::streams::fstream::open_ostream(FileName, std::ios::out | std::ios::binary | std::ios::trunc)
                    .then([=](concurrency::streams::ostream fileStream) { return response.body().read_to_end(fileStream.streambuf()); })
                    .then(
                        [=](size_t)
                        {
            // Play the speech
#if defined(__unix__)
                            // TODO: Not cross-platform
                            std::string command = "mpg123 " + FileName;
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

pplx::task<web::json::value> Orion::GetChatHistoryAsync()
{
    // Create a new http_request to get the chat history
    web::http::http_request request(web::http::methods::GET);
    request.set_request_uri(U("threads/" + m_CurrentThreadID + "/messages"));
    request.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    request.headers().add("OpenAI-Beta", "assistants=v1");

    // Check if the query parameter is present
    bool bMarkdown = request.request_uri().query().find(U("markdown=true")) != std::string::npos;

    // Send the request and get the response
    return m_OpenAIClient->request(request).then(
        [=](web::http::http_response response)
        {
            if (response.status_code() == web::http::status_codes::OK)
            {
                return response.extract_json()
                    .then([=](pplx::task<web::json::value> task) { return task.get(); })
                    .then(
                        [=](web::json::value json)
                        {
                            auto JData = json.at("data").as_array();

                            auto JChatHistory = web::json::value::object();
                            for (const auto& Message : JData)
                            {
                                // Our assistant is always the orion role in the chat history
                                auto Role = Message.at("role").as_string();
                                Role      = Role == "assistant" ? "orion" : Role;

                                for (const auto& Content : Message.at("content").as_array())
                                {
                                    if (Content.at("type").as_string() == "text")
                                    {
                                        // Add the message to the chat history
                                        auto Message = Content.at("text").at("value").as_string();

                                        if (bMarkdown)
                                        {
                                            auto szMarkdown = cmark_markdown_to_html(Message.c_str(), Message.length(), CMARK_OPT_DEFAULT);
                                            Message         = szMarkdown;
                                            free(szMarkdown);
                                        }

                                        auto JMessage       = web::json::value::object();
                                        JMessage["message"] = web::json::value::string(Message);
                                        JMessage["role"]    = web::json::value::string("orion");

                                        // Add the message to the chat history
                                        JChatHistory[JChatHistory.size()] = JMessage;
                                    }
                                }
                            }

                            return JChatHistory;
                        });
            }
            else
            {
                std::cerr << "Failed to get the chat history" << std::endl;
                std::cout << response.to_string() << std::endl;

                return pplx::task_from_result(web::json::value::object());
            }
        });
}
