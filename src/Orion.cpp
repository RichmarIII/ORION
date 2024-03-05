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
#include <cpprest/http_msg.h>

using namespace ORION;

Orion::Orion(const std::string& ID, std::vector<std::unique_ptr<IOrionTool>>&& Tools, const EOrionIntelligence INTELLIGENCE, const EOrionVoice VOICE,
             const char* pName, const char* pInstructions, const char* pDescription)
    : m_Tools(std::move(Tools)),
      m_CurrentIntelligence(INTELLIGENCE),
      m_CurrentVoice(VOICE),
      m_Name(pName),
      m_Instructions(pInstructions),
      m_Description(pDescription)
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
pplx::task<std::string> Orion::SendMessageAsync(const std::string& Message)
{
    // Create a message in the openai thread
    web::http::http_request CreateMessageRequest(web::http::methods::POST);
    CreateMessageRequest.set_request_uri(U("threads/" + m_CurrentThreadID + "/messages"));
    CreateMessageRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    CreateMessageRequest.headers().add("OpenAI-Beta", "assistants=v1");
    CreateMessageRequest.headers().add("Content-Type", "application/json");

    web::json::value CreateMessageBody = web::json::value::object();
    CreateMessageBody["content"]       = web::json::value::string(Message);
    CreateMessageBody["role"]          = web::json::value::string("user");
    CreateMessageRequest.set_body(CreateMessageBody);

    return m_OpenAIClient->request(CreateMessageRequest)
        .then(
            [this](web::http::http_response CreateMessageResponse)
            {
                if (CreateMessageResponse.status_code() != web::http::status_codes::OK)
                {
                    std::cerr << "Failed to create a new message" << std::endl;
                    std::cout << CreateMessageResponse.to_string() << std::endl;
                    return pplx::task_from_result(std::string("Failed to create a new message: " + CreateMessageResponse.to_string()));
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
                        [this](web::http::http_response CreateRunResponse)
                        {
                            if (CreateRunResponse.status_code() != web::http::status_codes::OK)
                            {
                                std::cerr << "Failed to run the assistant" << std::endl;
                                std::cout << CreateRunResponse.to_string() << std::endl;
                                return pplx::task_from_result(std::string("Failed to run the assistant: " + CreateRunResponse.to_string()));
                            }

                            std::string RunID = CreateRunResponse.extract_json().get().at("id").as_string();

                            // Get the response from the openai assistant
                            while (true)
                            {
                                auto CheckRunStatusRequest = web::http::http_request(web::http::methods::GET);
                                CheckRunStatusRequest.set_request_uri(U("threads/" + m_CurrentThreadID + "/runs/" + RunID));
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

                                web::json::value Json = CheckRunStatusResponse.extract_json().get();

                                if (Json.at("status").as_string() == "completed")
                                {
                                    break;
                                }
                                else if (Json.at("status").as_string() == "failed")
                                {
                                    return pplx::task_from_result(std::string("The request has failed"));
                                }
                                else if (Json.at("status").as_string() == "expired")
                                {
                                    return pplx::task_from_result(std::string("The request has expired"));
                                }
                                else if (Json.at("status").as_string() == "cancelled")
                                {
                                    return pplx::task_from_result(std::string("The request has been cancelled"));
                                }
                                else if (Json.at("status").as_string() == "queued")
                                {
                                    // Do nothing
                                }
                                else if (Json.at("status").as_string() == "requires_action")
                                {
                                    // Check if the thread wants to run a tool
                                    web::json::value ToolCallResults = web::json::value::array();
                                    for (const auto& ToolCall : Json.at("required_action").at("submit_tool_outputs").at("tool_calls").as_array())
                                    {
                                        if (ToolCall.at("type").as_string() == "function")
                                        {
                                            // Get tool from available tools by name
                                            auto Tool = std::find_if(m_Tools.begin(), m_Tools.end(),
                                                                     [&](const auto& Item)
                                                                     { return Item->GetName() == ToolCall.at("function").at("name").as_string(); });

                                            if (Tool != m_Tools.end())
                                            {
                                                // Call the tool
                                                auto pFunctionTool = static_cast<FunctionTool*>(Tool->get());
                                                auto Arguments     = ToolCall.at("function").at("arguments").as_string();
                                                auto Output        = pFunctionTool->Execute(*this, web::json::value::parse(Arguments));

                                                // Append the tool call results
                                                web::json::value ToolCallResult         = web::json::value::object();
                                                ToolCallResult["tool_call_id"]          = ToolCall.at("id");
                                                ToolCallResult["output"]                = web::json::value::string(Output);
                                                ToolCallResults[ToolCallResults.size()] = ToolCallResult;
                                            }
                                        }
                                    }

                                    // Submit the tool call results
                                    auto SubmitToolOutputsRequest = web::http::http_request(web::http::methods::POST);
                                    SubmitToolOutputsRequest.set_request_uri(
                                        U("threads/" + m_CurrentThreadID + "/runs/" + RunID + "/submit_tool_outputs"));
                                    SubmitToolOutputsRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
                                    SubmitToolOutputsRequest.headers().add("OpenAI-Beta", "assistants=v1");
                                    SubmitToolOutputsRequest.headers().add("Content-Type", "application/json");

                                    auto ToolOutputs            = web::json::value::object();
                                    ToolOutputs["tool_outputs"] = ToolCallResults;
                                    SubmitToolOutputsRequest.set_body(ToolOutputs);

                                    auto SubmitToolOutputsResponse = m_OpenAIClient->request(SubmitToolOutputsRequest).get();

                                    if (SubmitToolOutputsResponse.status_code() != web::http::status_codes::OK)
                                    {
                                        std::cerr << "Failed to submit the tool call results" << std::endl;
                                        std::cout << SubmitToolOutputsResponse.to_string() << std::endl;
                                        return pplx::task_from_result(
                                            std::string("Failed to submit the tool call results: " + SubmitToolOutputsResponse.to_string()));
                                    }
                                }
                                else if (Json.at("status").as_string() == "in_progress")
                                {
                                    // Do nothing
                                }
                                else
                                {
                                    std::cerr << "Unknown status: " << Json.at("status").as_string() << std::endl;
                                    std::cout << CheckRunStatusResponse.to_string() << std::endl;
                                    return pplx::task_from_result(std::string("Unknown status: " + Json.at("status").as_string()));
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
                            web::uri_builder ListMessagesRequestURIBuilder;
                            ListMessagesRequestURIBuilder.append_path("threads/" + m_CurrentThreadID + "/messages");
                            ListMessagesRequestURIBuilder.append_query("limit", 5);
                            ListMessagesRequest.set_request_uri(ListMessagesRequestURIBuilder.to_string());

                            auto ListMessagesResponse = m_OpenAIClient->request(ListMessagesRequest).get();

                            if (ListMessagesResponse.status_code() != web::http::status_codes::OK)
                            {
                                std::cerr << "Failed to get the response from the assistant" << std::endl;
                                return pplx::task_from_result(
                                    std::string("Failed to get the response from the assistant: " + ListMessagesResponse.to_string()));
                            }

                            web::json::value ThreadMessages = ListMessagesResponse.extract_json().get();

                            std::vector<std::string> NewMessages;
                            for (const auto& ThreadMessage : ThreadMessages.at("data").as_array())
                            {
                                if (ThreadMessage.at("role").as_string() == "assistant")
                                {
                                    for (const auto& Content : ThreadMessage.at("content").as_array())
                                    {
                                        if (Content.at("type").as_string() == "text")
                                        {
                                            NewMessages.push_back(Content.at("text").at("value").as_string());
                                        }
                                    }
                                }
                                else if (ThreadMessage.at("role").as_string() == "user")
                                {
                                    break;
                                }
                            }

                            // Combine the messages
                            std::string CombinedMessage;
                            for (const auto& Message : NewMessages)
                            {
                                CombinedMessage += Message + " \n";
                            }

                            return pplx::task_from_result(CombinedMessage);
                        });
            });
}

void Orion::CreateAssistant()
{
    // Check if an assistant already exists on the server
    web::http::http_request ListAssistantsRequest(web::http::methods::GET);
    ListAssistantsRequest.set_request_uri(U("assistants"));
    ListAssistantsRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    ListAssistantsRequest.headers().add("OpenAI-Beta", "assistants=v1");

    web::http::http_response ListAssistantsResponse = m_OpenAIClient->request(ListAssistantsRequest).get();

    bool DoesAssistantExist = false;
    if (ListAssistantsResponse.status_code() == web::http::status_codes::OK)
    {
        web::json::value Json = ListAssistantsResponse.extract_json().get();
        for (const auto& Assistant : Json.at("data").as_array())
        {
            if (Assistant.at("id").as_string() == m_CurrentAssistantID)
            {
                DoesAssistantExist = true;
                break;
            }
        }
    }
    else
    {
        std::cerr << "Failed to get the list of assistants" << std::endl;
        return;
    }

    if (DoesAssistantExist)
    {
        // Update the assistant
        web::http::http_request UpdateAssistantRequest(web::http::methods::POST);
        UpdateAssistantRequest.set_request_uri(U("assistants/" + m_CurrentAssistantID));
        UpdateAssistantRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
        UpdateAssistantRequest.headers().add("OpenAI-Beta", "assistants=v1");
        UpdateAssistantRequest.headers().add("Content-Type", "application/json");

        web::json::value UpdateAssistantRequestBody = web::json::value::object();
        UpdateAssistantRequestBody["instructions"]  = web::json::value::string(m_Instructions);
        UpdateAssistantRequestBody["description"]   = web::json::value::string(m_Description);
        UpdateAssistantRequestBody["model"] =
            web::json::value::string(m_CurrentIntelligence == EOrionIntelligence::Base ? "gpt-3.5-turbo" : "gpt-4-turbo-preview");

        if (!m_Tools.empty())
        {
            web::json::value Tools = web::json::value::array();
            for (const auto& Tool : m_Tools)
            {
                printf("Adding tool: %s\n", Tool->GetName().c_str());
                Tools[Tools.size()] = web::json::value::parse(Tool->ToJson());
            }
            UpdateAssistantRequestBody["tools"] = Tools;
        }

        UpdateAssistantRequest.set_body(UpdateAssistantRequestBody);

        web::http::http_response UpdateAssistantResponse = m_OpenAIClient->request(UpdateAssistantRequest).get();

        if (UpdateAssistantResponse.status_code() != web::http::status_codes::OK)
        {
            std::cerr << "Failed to update the assistant" << std::endl;

            // Print the response
            std::cout << UpdateAssistantResponse.to_string() << std::endl;
        }
    }
    else
    {
        // Create a new assistant
        web::http::http_request CreateAssistantRequest(web::http::methods::POST);
        CreateAssistantRequest.set_request_uri(U("assistants"));
        CreateAssistantRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
        CreateAssistantRequest.headers().add("OpenAI-Beta", "assistants=v1");
        CreateAssistantRequest.headers().add("Content-Type", "application/json");

        web::json::value CreateAssistantRequestBody = web::json::value::object();
        CreateAssistantRequestBody["name"]          = web::json::value::string(m_Name);
        CreateAssistantRequestBody["instructions"]  = web::json::value::string(m_Instructions);
        CreateAssistantRequestBody["description"]   = web::json::value::string(m_Description);
        CreateAssistantRequestBody["model"] =
            web::json::value::string(m_CurrentIntelligence == EOrionIntelligence::Base ? "gpt-3.5-turbo" : "gpt-4-turbo-preview");

        if (!m_Tools.empty())
        {
            web::json::value Tools = web::json::value::array();
            for (const auto& Tool : m_Tools)
            {
                printf("Adding tool: %s\n", Tool->GetName().c_str());
                Tools[Tools.size()] = web::json::value::parse(Tool->ToJson());
            }
            CreateAssistantRequestBody["tools"] = Tools;
        }

        CreateAssistantRequest.set_body(CreateAssistantRequestBody);

        web::http::http_response CreateAssistantResponse = m_OpenAIClient->request(CreateAssistantRequest).get();

        if (CreateAssistantResponse.status_code() == web::http::status_codes::OK)
        {
            web::json::value Json = CreateAssistantResponse.extract_json().get();
            m_CurrentAssistantID  = Json.at("id").as_string();
        }
        else
        {
            std::cerr << "Failed to create a new assistant" << std::endl;

            // Print the response
            std::cout << CreateAssistantResponse.to_string() << std::endl;
        }
    }
}

void Orion::CreateThread()
{
    // Create a new thread
    web::http::http_request CreateThreadRequest(web::http::methods::POST);
    CreateThreadRequest.set_request_uri(U("threads"));
    CreateThreadRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    CreateThreadRequest.headers().add("OpenAI-Beta", "assistants=v1");
    CreateThreadRequest.headers().add("Content-Type", "application/json");
    CreateThreadRequest.set_body(web::json::value::object());

    web::http::http_response CreateThreadResponse = m_OpenAIClient->request(CreateThreadRequest).get();

    if (CreateThreadResponse.status_code() == web::http::status_codes::OK)
    {
        web::json::value ResponseDataJson = CreateThreadResponse.extract_json().get();
        m_CurrentThreadID                 = ResponseDataJson.at("id").as_string();
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
        if (auto pKey = std::getenv("OPENAI_API_KEY"); pKey)
        {
            m_OpenAIAPIKey = pKey;
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
        if (auto pKey = std::getenv("OPENWEATHER_API_KEY"); pKey)
        {
            m_OpenWeatherAPIKey = pKey;
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

    // Load the Home Assistant API key from the environment or a file
    {
        if (auto pKey = std::getenv("HASS_API_KEY"); pKey)
        {
            m_HASSAPIKey = pKey;
        }
        else
        {
            // Load the Home Assistant API key from a file
            std::ifstream APIKey {".hass_api_key.txt"};
            m_HASSAPIKey = std::string {std::istreambuf_iterator<char>(APIKey), std::istreambuf_iterator<char>()};
        }
        if (m_HASSAPIKey.empty())
        {
            std::cerr << "Home Assistant API key not found" << std::endl;
            return;
        }
    }
}

void Orion::SetNewVoice(const EOrionVoice VOICE)
{
    m_CurrentVoice = VOICE;
}

void Orion::SetNewIntelligence(const EOrionIntelligence INTELLIGENCE)
{
    m_CurrentIntelligence = INTELLIGENCE;
}

pplx::task<void> Orion::SpeakAsync(const std::string& Message, const ETTSAudioFormat AUDIO_FORMAT)
{
    // Split the message into multiple messages if it's too long
    return SplitMessageAsync(Message).then(
        [this, AUDIO_FORMAT](pplx::task<std::vector<std::string>> SplitMessageTask)
        {
            const auto SPLIT_MESSAGES = SplitMessageTask.get();

            // Each orion instance has it's own folder for audio files to avoid conflicts. Append the assistant id to the audio folder.
            // Create the audio directory if it doesn't exist
            std::error_code ErrorCode;
            if (!std::filesystem::exists("audio/" + m_CurrentAssistantID, ErrorCode))
            {
                std::filesystem::create_directory("audio/" + m_CurrentAssistantID, ErrorCode);
                if (ErrorCode)
                {
                    std::cerr << "Failed to create the audio directory" << std::endl;
                    return;
                }
            }

            // Delete all the audio files in the directory
            for (const auto& Entry : std::filesystem::directory_iterator("audio/" + m_CurrentAssistantID))
            {
                std::filesystem::remove(Entry.path(), ErrorCode);
                if (ErrorCode)
                {
                    std::cerr << "Failed to delete the audio file" << std::endl;
                    return;
                }
            }

            // Create a task for each message
            std::vector<pplx::task<void>> Tasks;
            uint8_t                       Index = 0;
            for (const auto& Message : SPLIT_MESSAGES)
            {
                Tasks.push_back(SpeakSingleAsync(Message, Index, AUDIO_FORMAT));
                Index++;
            }

            // Only wait for the first task to complete.  By the time the first task is done, the rest of the tasks should be done as well or
            // enough generated audio should be available to play without buffering.
            Tasks.begin()->wait();
        });
}

web::json::value Orion::ListSmartDevices(const std::string& Domain)
{
    try
    {
        // Create a new http_client to communicate with the home-assistant api
        web::http::client::http_client HomeAssistantClient(U("http://homeassistant.local:8123/api/"));

        // Create a new http_request to get the smart devices
        web::http::http_request ListStatesRequest(web::http::methods::GET);
        ListStatesRequest.set_request_uri(U("states"));
        ListStatesRequest.headers().add("Authorization", "Bearer " + m_HASSAPIKey);
        ListStatesRequest.headers().add("Content-Type", "application/json");

        const auto DOMAIN_WITH_DOT = Domain + ".";

        // Send the request and get the response
        return HomeAssistantClient.request(ListStatesRequest)
            .then(
                [DOMAIN_WITH_DOT](web::http::http_response ListStatesResponse)
                {
                    if (ListStatesResponse.status_code() == web::http::status_codes::OK)
                    {
                        return ListStatesResponse.extract_json().then(
                            [DOMAIN_WITH_DOT](pplx::task<web::json::value> ExtractJsonTask)
                            {
                                return ExtractJsonTask.then(
                                    [DOMAIN_WITH_DOT](web::json::value ListStatesResponseDataJson)
                                    {
                                        web::json::value JSmartDevices = web::json::value::array();
                                        for (const auto& Device : ListStatesResponseDataJson.as_array())
                                        {
                                            const auto DEVICE_NAME = Device.at("entity_id").as_string();
                                            if (DOMAIN_WITH_DOT == "all." || DEVICE_NAME.find(DOMAIN_WITH_DOT) != std::string::npos)
                                            {
                                                JSmartDevices[JSmartDevices.size()] = web::json::value::string(DEVICE_NAME);
                                            }
                                        }

                                        return JSmartDevices;
                                    });
                            });
                    }
                    else
                    {
                        std::cerr << "Failed to get the smart devices: " << ListStatesResponse.to_string() << std::endl;
                        std::cout << "Failed to get the smart devices: " << ListStatesResponse.to_string() << std::endl;
                        return pplx::task_from_result(web::json::value::array());
                    }
                })
            .get();
    }
    catch (const std::exception& Exception)
    {
        std::cerr << "Failed to list the smart devices: " << Exception.what() << std::endl;
        return web::json::value::array();
    }
}

web::json::value Orion::ExecSmartDeviceService(const web::json::value& Devices, const std::string& Service)
{
    try
    {
        // Loop through the devices and execute the service.  The domain is the first part of the entity_id (e.g. light.bedroom_light -> light)
        for (const auto& Device : Devices.as_array())
        {
            const auto DEVICE_NAME = Device.as_string();
            const auto DOMAIN      = DEVICE_NAME.substr(0, DEVICE_NAME.find('.'));
            const auto ENTITY_ID   = DEVICE_NAME.substr(DEVICE_NAME.find('.') + 1);

            // Create a new http_client to communicate with the home-assistant api
            web::http::client::http_client HomeAssistantClient(U("http://homeassistant.local:8123/api/"));

            // Create a new http_request to execute the smart device service
            web::http::http_request ExecuteServiceRequest(web::http::methods::POST);
            ExecuteServiceRequest.set_request_uri(U("services/" + DOMAIN + "/" + Service));
            ExecuteServiceRequest.headers().add("Authorization", "Bearer " + m_HASSAPIKey);
            ExecuteServiceRequest.headers().add("Content-Type", "application/json");

            // Add the body to the request
            web::json::value ExecuteServiceRequestBody = web::json::value::object();
            ExecuteServiceRequestBody["entity_id"]     = web::json::value::string(DEVICE_NAME);
            ExecuteServiceRequest.set_body(ExecuteServiceRequestBody);

            // Send the request and get the response
            auto ExecuteServiceResponse = HomeAssistantClient.request(ExecuteServiceRequest).get();

            if (ExecuteServiceResponse.status_code() != web::http::status_codes::OK)
            {
                std::cerr << "Failed to execute the smart device service" << std::endl;
                std::cout << ExecuteServiceResponse.to_string() << std::endl;
            }
            else
            {
                std::cout << "Executed the smart device service: " << Service << " on the device: " << DEVICE_NAME << std::endl;
            }
        }

        return web::json::value::string("Executed the smart device service: " + Service);
    }
    catch (const std::exception& Exception)
    {
        std::cerr << "Failed to execute the smart device service: " << Exception.what() << std::endl;
        return web::json::value::string("Failed to execute the smart device service: " + std::string(Exception.what()));
    }
}

pplx::task<web::json::value> Orion::GetChatHistoryAsync()
{
    // Create a new http_request to get the chat history
    web::http::http_request ListMessagesRequest(web::http::methods::GET);
    ListMessagesRequest.set_request_uri(U("threads/" + m_CurrentThreadID + "/messages"));
    ListMessagesRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    ListMessagesRequest.headers().add("OpenAI-Beta", "assistants=v1");

    // Check if the query parameter is present
    bool IsMarkdownRequested = ListMessagesRequest.request_uri().query().find(U("markdown=true")) != std::string::npos;

    // Send the request and get the response
    return m_OpenAIClient->request(ListMessagesRequest)
        .then(
            [=](web::http::http_response ListMessagesResponse)
            {
                if (ListMessagesResponse.status_code() == web::http::status_codes::OK)
                {
                    return ListMessagesResponse.extract_json()
                        .then([=](pplx::task<web::json::value> ExtractJsonTask) { return ExtractJsonTask.get(); })
                        .then(
                            [=](web::json::value ListMessagesResponseDataJson)
                            {
                                auto JData = ListMessagesResponseDataJson.at("data").as_array();

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

                                            if (IsMarkdownRequested)
                                            {
                                                auto pMarkdown = cmark_markdown_to_html(Message.c_str(), Message.length(), CMARK_OPT_DEFAULT);
                                                Message        = pMarkdown;
                                                free(pMarkdown);
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
                    std::cout << ListMessagesResponse.to_string() << std::endl;

                    return pplx::task_from_result(web::json::value::object());
                }
            });
}

pplx::task<void> Orion::SpeakSingleAsync(const std::string& Message, const uint8_t INDEX, const ETTSAudioFormat AUDIO_FORMAT)
{
    // Create a new http_request to get the speech
    web::http::http_request TextToSpeechRequest(web::http::methods::POST);
    TextToSpeechRequest.set_request_uri(U("audio/speech"));
    TextToSpeechRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    TextToSpeechRequest.headers().add("Content-Type", "application/json");

    // Add the body to the request
    web::json::value TextToSpeechRequestBody = web::json::value::object();
    TextToSpeechRequestBody["model"]         = web::json::value::string("tts-1");
    TextToSpeechRequestBody["input"]         = web::json::value::string(Message);

    // Default to mp3
    TextToSpeechRequestBody["response_format"] = web::json::value::string("mp3");
    std::string MimeType                       = "audio/mpeg";

    if (AUDIO_FORMAT == ETTSAudioFormat::Opus)
    {
        MimeType                                   = "audio/ogg";
        TextToSpeechRequestBody["response_format"] = web::json::value::string("opus");
    }
    else if (AUDIO_FORMAT == ETTSAudioFormat::AAC)
    {
        MimeType                                   = "audio/aac";
        TextToSpeechRequestBody["response_format"] = web::json::value::string("aac");
    }
    else if (AUDIO_FORMAT == ETTSAudioFormat::FLAC)
    {
        MimeType                                   = "audio/flac";
        TextToSpeechRequestBody["response_format"] = web::json::value::string("flac");
    }
    else if (AUDIO_FORMAT == ETTSAudioFormat::Wav)
    {
        MimeType                                   = "audio/wav";
        TextToSpeechRequestBody["response_format"] = web::json::value::string("wav");
    }
    else if (AUDIO_FORMAT == ETTSAudioFormat::PCM)
    {
        MimeType                                   = "audio/wav";
        TextToSpeechRequestBody["response_format"] = web::json::value::string("pcm");
    }

    // Set the voice based on the current voice
    if (m_CurrentVoice == EOrionVoice::Alloy)
    {
        TextToSpeechRequestBody["voice"] = web::json::value::string("alloy");
    }
    else if (m_CurrentVoice == EOrionVoice::Echo)
    {
        TextToSpeechRequestBody["voice"] = web::json::value::string("echo");
    }
    else if (m_CurrentVoice == EOrionVoice::Fable)
    {
        TextToSpeechRequestBody["voice"] = web::json::value::string("fable");
    }
    else if (m_CurrentVoice == EOrionVoice::Nova)
    {
        TextToSpeechRequestBody["voice"] = web::json::value::string("nova");
    }
    else if (m_CurrentVoice == EOrionVoice::Onyx)
    {
        TextToSpeechRequestBody["voice"] = web::json::value::string("onyx");
    }
    else if (m_CurrentVoice == EOrionVoice::Shimmer)
    {
        TextToSpeechRequestBody["voice"] = web::json::value::string("shimmer");
    }

    const auto EXTENSION = "." + TextToSpeechRequestBody["response_format"].as_string();

    TextToSpeechRequest.set_body(TextToSpeechRequestBody);

    // Generate audio file name from index
    std::string FileName = "audio/" + m_CurrentAssistantID + "/speech_" + std::to_string(INDEX) + EXTENSION;

    // Send the request and get the response
    return m_OpenAIClient->request(TextToSpeechRequest)
        .then(
            [FileName, MimeType](web::http::http_response TextToSpeechResponse)
            {
                if (TextToSpeechResponse.status_code() == web::http::status_codes::OK)
                {
                    // Stream the response to a file using concurrency::streams::fstream to avoid loading the entire response into memory
                    // Then return the filename once the file has started streaming

                    // Create a file stream
                    return concurrency::streams::fstream::open_ostream(FileName).then(
                        [TextToSpeechResponse, FileName, MimeType](concurrency::streams::ostream AudioFileStream)
                        {
                            // Stream the response to the file
                            TextToSpeechResponse.body().read_to_end(AudioFileStream.streambuf()).wait();

                            return pplx::task_from_result();
                        });
                }
                else
                {
                    std::cerr << "Failed to get the speech" << std::endl;
                    std::cout << TextToSpeechResponse.to_string() << std::endl;
                }

                return pplx::task_from_result();
            });
}

pplx::task<std::vector<std::string>> Orion::SplitMessageAsync(const std::string& Message)
{
    // Split the message into multiple messages if it's too long. But only on periods or newlines.
    // This is to avoid splitting words in half. A message can be longer than x amount of characters if it contains no periods or newlines.
    // It must NOT be plit mid-word or mid-sentence.

    return pplx::create_task(
        [Message]
        {
            std::vector<std::string> Messages;
            constexpr char           NEWLINE_CHAR          = '\n';
            constexpr char           PERIOD_CHAR           = '.';
            constexpr size_t         MAX_LENGTH_SOFT_LIMIT = 256;
            constexpr size_t         MAX_LENGTH_HARD_LIMIT = 4096;
            constexpr size_t         DISTANCE_THRESHOLD    = 25;

            size_t MessageStart   = 0;                 // Start index of the current message
            size_t LastSplitIndex = std::string::npos; // Initialize to indicate no split index found yet
            size_t Index          = 0;                 // Current index in the message

            while (Index < Message.length())
            {
                bool DidReachHardLimit = false;

                // Look for next split point or end of message
                while (Index < Message.length() && !DidReachHardLimit)
                {
                    if (Message[Index] == PERIOD_CHAR || Message[Index] == NEWLINE_CHAR)
                    {
                        LastSplitIndex = Index;
                    }
                    ++Index;

                    // Check conditions for splitting
                    if ((Index - MessageStart >= MAX_LENGTH_SOFT_LIMIT &&
                         (LastSplitIndex != std::string::npos && Index - LastSplitIndex <= DISTANCE_THRESHOLD)) ||
                        Index - MessageStart >= MAX_LENGTH_HARD_LIMIT)
                    {
                        DidReachHardLimit = true; // Force split if hard limit reached
                        // Adjust SplitIndex to last known good split if within threshold, otherwise split at current index
                        size_t SplitIndex =
                            (LastSplitIndex != std::string::npos && Index - LastSplitIndex <= DISTANCE_THRESHOLD) ? LastSplitIndex : Index - 1;
                        Messages.push_back(Message.substr(MessageStart, SplitIndex - MessageStart + 1));
                        MessageStart   = SplitIndex + 1;
                        Index          = MessageStart;
                        LastSplitIndex = std::string::npos; // Reset last split index
                    }
                }
            }

            // Handle any remaining part of the message
            if (MessageStart < Message.length())
            {
                Messages.push_back(Message.substr(MessageStart));
            }

            return Messages;
        });
}