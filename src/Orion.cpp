#include "Orion.hpp"
#include "OrionWebServer.hpp"
#include "tools/FunctionTool.hpp"
#include "MimeTypes.hpp"

// Include cpprestsdk headers
#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>
#include <cpprest/ws_client.h>
#include <cpprest/ws_msg.h>
#include <cpprest/producerconsumerstream.h>
#include <cpprest/containerstream.h>
#include <cpprest/interopstream.h>
#include <cpprest/http_msg.h>

// Include cmark headers
#include <cmark.h>

// Include standard headers
#include <chrono>
#include <cpprest/http_msg.h>
#include <filesystem>
#include <thread>

using namespace ORION;

Orion::Orion(const std::string& ID, std::vector<std::unique_ptr<IOrionTool>>&& Tools, const EOrionIntelligence INTELLIGENCE, const EOrionVoice VOICE,
             const char* pName, const char* pInstructions, const char* pDescription)
    : m_Name(pName),
      m_Instructions(pInstructions),
      m_Description(pDescription),
      m_Tools(std::move(Tools)),
      m_CurrentVoice(VOICE),
      m_CurrentIntelligence(INTELLIGENCE)
{
    m_Instructions = "Your name is " + m_Name + ". " + m_Instructions;

    // Generate a guid for the Orion instance
    if (!ID.empty())
    {
        m_CurrentAssistantID = ID;
    }
}

bool ORION::Orion::Initialize(OrionWebServer& WebServer, const web::http::http_request& Request)
{
    m_pOrionWebServer     = &WebServer;
    m_pOrionClientContext = &Request;

    CreateClient();
    CreateAssistant();
    CreateThread();

    return true;
}

std::string Orion::GetUserID() const
{
    if (!m_pOrionWebServer)
    {
        return "";
    }

    return m_pOrionWebServer->GetUserID(m_CurrentAssistantID);
}

double Orion::GetSemanticSimilarity(const std::string& Content, const std::string& Query) const
{
    // Create a new http_request to create an embedding for the content
    web::http::http_request VectorSearchRequest(web::http::methods::POST);
    VectorSearchRequest.set_request_uri(U("embeddings"));
    VectorSearchRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    VectorSearchRequest.headers().add("OpenAI-Beta", "assistants=v1");
    VectorSearchRequest.headers().add("Content-Type", "application/json");

    // Create json array for the input
    web::json::value JInputArray    = web::json::value::array();
    JInputArray[JInputArray.size()] = web::json::value::string(Content);
    JInputArray[JInputArray.size()] = web::json::value::string(Query);

    web::json::value VectorSearchRequestBody = web::json::value::object();
    VectorSearchRequestBody["input"]         = JInputArray;
    VectorSearchRequestBody["model"]         = web::json::value::string("text-embedding-3-small");

    VectorSearchRequest.set_body(VectorSearchRequestBody);

    // Send the request and get the response
    auto VectorSearchResponse = m_OpenAIClient->request(VectorSearchRequest).get();

    if (VectorSearchResponse.status_code() != web::http::status_codes::OK)
    {
        std::cerr << "Failed to create an embedding for the content" << std::endl;
        std::cout << VectorSearchResponse.to_string() << std::endl;
        return 0.0;
    }

    // Get the embedding
    auto VectorSearchResponseJson = VectorSearchResponse.extract_json().get();
    auto JEmbeddingArray          = VectorSearchResponseJson.at("data").as_array();

    // Calculate the similarity between the content and the query
    const auto EMBEDDING_CONTENT = JEmbeddingArray[0].at("embedding").as_array();
    const auto EMBEDDING_QUERY   = JEmbeddingArray[1].at("embedding").as_array();

    double DotProduct  = 0.0;
    double NormContent = 0.0;
    double NormQuery   = 0.0;

    for (size_t i = 0; i < EMBEDDING_CONTENT.size(); ++i)
    {
        DotProduct += EMBEDDING_CONTENT.at(i).as_double() * EMBEDDING_QUERY.at(i).as_double();
        NormContent += EMBEDDING_CONTENT.at(i).as_double() * EMBEDDING_CONTENT.at(i).as_double();
        NormQuery += EMBEDDING_QUERY.at(i).as_double() * EMBEDDING_QUERY.at(i).as_double();
    }

    return DotProduct / (std::sqrt(NormContent) * std::sqrt(NormQuery));
}

pplx::task<void> Orion::SendMessageAsync(const std::string& Message, const web::json::array& Files)
{
    // Upload the files
    auto JFiles = web::json::value::array();
    for (const auto& File : Files)
    {
        // Get the file name and data
        const auto FILE_NAME = File.at("name").as_string();
        const auto FILE_DATA = utility::conversions::from_base64(File.at("data").as_string());
        const auto MIME_TYPE = MimeTypes::GetMimeType(FILE_NAME);

        // Create a boundary for the multipart/form-data body
        const std::string BOUNDARY = "----CppRestSdkFormBoundary";

        // Create the multipart/form-data body
        std::vector<unsigned char> MultiPartFormData;

        // Helper function to append text to the vector
        auto AppendText = [&](const std::string& Text) { MultiPartFormData.insert(MultiPartFormData.end(), Text.begin(), Text.end()); };

        // Helper function to append binary data to the vector
        auto AppendBinary = [&](const std::vector<unsigned char>& Data)
        { MultiPartFormData.insert(MultiPartFormData.end(), Data.begin(), Data.end()); };

        // Create the multipart/form-data body

        // Add the model part
        AppendText("--" + BOUNDARY + "\r\n");
        AppendText("Content-Disposition: form-data; name=\"purpose\"\r\n\r\n");
        AppendText(std::string("assistants") + "\r\n");

        // Add the file part
        AppendText("--" + BOUNDARY + "\r\n");
        AppendText("Content-Disposition: form-data; name=\"file\"; filename=\"" + FILE_NAME + "\"\r\n");
        AppendText("Content-Type: " + MIME_TYPE + "\r\n\r\n");
        AppendBinary(FILE_DATA);
        AppendText("\r\n");

        // End of the multipart/form-data body
        AppendText("--" + BOUNDARY + "--");

        // Create a new http_request to upload the file
        web::http::http_request UploadFileRequest(web::http::methods::POST);
        UploadFileRequest.set_request_uri(U("files"));
        UploadFileRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
        UploadFileRequest.headers().add("OpenAI-Beta", "assistants=v1");
        UploadFileRequest.headers().add("Content-Type", "multipart/form-data; boundary=" + BOUNDARY);

        // Set the body
        UploadFileRequest.set_body(MultiPartFormData);

        // Send the request and get the response
        auto UploadFileResponse = m_OpenAIClient->request(UploadFileRequest).get();

        if (UploadFileResponse.status_code() != web::http::status_codes::OK)
        {
            std::cerr << "Failed to upload the file" << std::endl;
            std::cout << UploadFileResponse.to_string() << std::endl;

            // Continue to the next file
            continue;
        }

        // Get the file id
        auto       UploadFileResponseJson = UploadFileResponse.extract_json().get();
        const auto FILE_ID                = UploadFileResponseJson.at("id").as_string();

        // Add the file to the list of files
        JFiles[JFiles.size()] = web::json::value::string(FILE_ID);
    }

    // Create a message in the openai thread
    web::http::http_request CreateMessageRequest(web::http::methods::POST);
    CreateMessageRequest.set_request_uri(U("threads/" + m_CurrentThreadID + "/messages"));
    CreateMessageRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    CreateMessageRequest.headers().add("OpenAI-Beta", "assistants=v1");
    CreateMessageRequest.headers().add("Content-Type", "application/json");

    web::json::value CreateMessageBody = web::json::value::object();
    CreateMessageBody["content"]       = web::json::value::string(Message);
    CreateMessageBody["role"]          = web::json::value::string("user");
    CreateMessageBody["file_ids"]      = JFiles;
    CreateMessageRequest.set_body(CreateMessageBody);

    return m_OpenAIClient->request(CreateMessageRequest)
        .then(
            [this](const web::http::http_response& CreateMessageResponse)
            {
                if (CreateMessageResponse.status_code() != web::http::status_codes::OK)
                {
                    std::cerr << "Failed to create a new message" << std::endl;
                    std::cout << CreateMessageResponse.to_string() << std::endl;
                    return pplx::task_from_result();
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
                CreateRunBody["stream"] = web::json::value::boolean(true);
                CreateRunRequest.set_body(CreateRunBody);

                return m_OpenAIClient->request(CreateRunRequest)
                    .then(
                        [this](web::http::http_response CreateRunResponse)
                        {
                            if (CreateRunResponse.status_code() != web::http::status_codes::OK)
                            {
                                std::cerr << "Failed to create a new run" << std::endl;
                                std::cout << CreateRunResponse.to_string() << std::endl;
                                return pplx::task_from_result();
                            }

                            // The response is a server-sent event stream that we need to parse and terminates with the "done" event
                            // Keep reading the response stream for new events
                            concurrency::streams::istream EventStream = CreateRunResponse.body();
                            std::string                   Line;
                            std::string                   EventName;
                            std::string                   EventData;
                            while (true)
                            {
                                concurrency::streams::container_buffer<std::string> LineBuff {};
                                auto                                                ReadLineTask = EventStream.read_line(LineBuff);
                                ReadLineTask.wait();
                                const auto NumCharsRead = ReadLineTask.get();
                                Line                    = LineBuff.collection();

                                if (Line.empty())
                                {
                                    // Log the event
                                    std::cout << "SSE Event name: " << EventName << std::endl;
                                    std::cout << "SSEEvent data: " << EventData << std::endl;

                                    // Process the event

                                    // Check if the event is the "done" event
                                    if (EventName.empty() || EventName == OrionWebServer::SSEOpenAIEventNames::DONE)
                                    {
                                        // Break out of the loop
                                        break;
                                    }
                                    else if (EventName == OrionWebServer::SSEOpenAIEventNames::THREAD_MESSAGE_COMPLETED)
                                    {
                                        // Format an SSE event for the SSEOrionEventNames::MESSAGE_COMPLETED event.
                                        // No data is needed for this event.
                                        m_pOrionWebServer->SendServerEvent(OrionWebServer::SSEOrionEventNames::MESSAGE_COMPLETED,
                                                                           web::json::value::object());
                                    }
                                    else if (EventName == OrionWebServer::SSEOpenAIEventNames::THREAD_MESSAGE_DELTA)
                                    {
                                        // Format an SSE event for the SSEOrionEventNames::MESSAGE_DELTA event.
                                        // The data is the message from the assistant.

                                        // First Validate the JSON
                                        web::json::value JMessage = web::json::value::parse(EventData);

                                        // Perform checks to ensure the message is valid
                                        if (!JMessage.has_field(U("delta")) || !JMessage.at(U("delta")).has_field(U("content")))
                                        {
                                            std::cout << U(__func__) << ":" << __LINE__ << U(": Unexpected message format.") << std::endl;
                                            throw std::runtime_error("Unexpected message format.");
                                        }

                                        auto JMessageContentArray = JMessage.at(U("delta")).at(U("content")).as_array();
                                        for (const auto& JContentItem : JMessageContentArray)
                                        {
                                            // Check if the content item is a text item
                                            if (JContentItem.has_field(U("text")))
                                            {
                                                auto JTextContent = JContentItem.at(U("text"));
                                                auto TextContentString =
                                                    JTextContent.has_field(U("value")) ? JTextContent.at(U("value")).as_string() : "";

                                                auto JAnnotations = JTextContent.has_field(U("annotations"))
                                                                        ? JTextContent.at(U("annotations")).as_array()
                                                                        : web::json::value::array().as_array();

                                                if (!TextContentString.empty())
                                                {
                                                    // Get the message from the assistant
                                                    web::json::value JClientSSEData = web::json::value::object();
                                                    JClientSSEData[U("message")]    = web::json::value::string(TextContentString);

                                                    // Send the message to the client
                                                    m_pOrionWebServer->SendServerEvent(OrionWebServer::SSEOrionEventNames::MESSAGE_DELTA,
                                                                                       JClientSSEData);
                                                }

                                                // Gather the annotations
                                                for (const auto& JAnnotation : JAnnotations)
                                                {
                                                    auto TextToReplace = JAnnotation.at(U("text")).as_string();
                                                    auto JFilePath     = JAnnotation.at(U("file_path"));
                                                    auto FileID        = JFilePath.at(U("file_id")).as_string();
                                                    if (!FileID.empty())
                                                    {
                                                        // Get the file from the file id
                                                        web::http::http_request GetFileRequest(web::http::methods::GET);
                                                        GetFileRequest.set_request_uri(U("files/" + FileID));
                                                        GetFileRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
                                                        GetFileRequest.headers().add("OpenAI-Beta", "assistants=v1");

                                                        auto GetFileResponse = m_OpenAIClient->request(GetFileRequest).get();

                                                        if (GetFileResponse.status_code() == web::http::status_codes::OK)
                                                        {
                                                            auto GetFileResponseJson = GetFileResponse.extract_json().get();
                                                            auto FileName            = GetFileResponseJson.at(U("filename")).as_string();

                                                            // SSE event for the annotation
                                                            web::json::value JClientSSEData      = web::json::value::object();
                                                            JClientSSEData[U("file_name")]       = web::json::value::string(FileName);
                                                            JClientSSEData[U("text_to_replace")] = web::json::value::string(TextToReplace);

                                                            // Send the message to the client

                                                            m_pOrionWebServer->SendServerEvent(
                                                                OrionWebServer::SSEOrionEventNames::MESSAGE_ANNOTATION_CREATED, JClientSSEData);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    else if (EventName == OrionWebServer::SSEOpenAIEventNames::THREAD_MESSAGE_CREATED)
                                    {
                                        // Format an SSE event for the SSEOrionEventNames::MESSAGE_CREATED event.
                                        // No data is needed for this event.

                                        // Send the message to the client
                                        m_pOrionWebServer->SendServerEvent(OrionWebServer::SSEOrionEventNames::MESSAGE_STARTED, {});
                                    }
                                    else if (EventName == OrionWebServer::SSEOpenAIEventNames::THREAD_MESSAGE_IN_PROGRESS)
                                    {
                                        // Format an SSE event for the SSEOrionEventNames::RUN_COMPLETED event.
                                        // No data is needed for this event.

                                        // Send the message to the client
                                        m_pOrionWebServer->SendServerEvent(OrionWebServer::SSEOrionEventNames::MESSAGE_IN_PROGRESS, {});
                                    }
                                    else if (EventName == OrionWebServer::SSEOpenAIEventNames::THREAD_RUN_REQUIRES_ACTION)
                                    {
                                        // Parse the data
                                        web::json::value JRun = web::json::value::parse(EventData);

                                        auto JRequiredAction = JRun.at("required_action");

                                        if (JRequiredAction.at("type").as_string() == "submit_tool_outputs")
                                        {
                                            auto JSumbitToolOutputs = JRequiredAction.at("submit_tool_outputs");
                                            auto JToolCalls         = JSumbitToolOutputs.at("tool_calls").as_array();

                                            // Create responses for each tool call
                                            auto ToolCallOutputs = web::json::value::array();

                                            for (auto& JToolCall : JToolCalls)
                                            {
                                                if (JToolCall.at("type").as_string() == "function")
                                                {
                                                    // Get the tool call
                                                    auto       JFunctionToolCall = JToolCall.at("function");
                                                    const auto TOOL_NAME         = JFunctionToolCall.at("name").as_string();
                                                    const auto TOOL_ARGS = web::json::value::parse(JFunctionToolCall.at("arguments").as_string());

                                                    auto ToolIt =
                                                        std::find_if(m_Tools.begin(), m_Tools.end(),
                                                                     [TOOL_NAME](const auto& Tool) { return Tool->GetName() == TOOL_NAME; });

                                                    if (ToolIt != m_Tools.end())
                                                    {
                                                        // Get the tool
                                                        auto pFunctionTool = static_cast<FunctionTool*>(ToolIt->get());

                                                        // Get the tool call outputs
                                                        auto JFunctionOutputs = pFunctionTool->Execute(*this, TOOL_ARGS);

                                                        // Create the tool call outputs
                                                        auto JOutput            = web::json::value::object();
                                                        JOutput["tool_call_id"] = JToolCall.at("id");
                                                        JOutput["output"]       = web::json::value::string(JFunctionOutputs);

                                                        // Add the tool call outputs to the responses
                                                        ToolCallOutputs[ToolCallOutputs.size()] = JOutput;
                                                    }
                                                    else
                                                    {
                                                        // Default to an empty output
                                                        auto JOutput            = web::json::value::object();
                                                        JOutput["tool_call_id"] = JToolCall.at("id");
                                                        JOutput["output"] = web::json::value::string("Tool doesn't exist.  Stop hallucinating.");

                                                        // Add the tool call outputs to the responses
                                                        ToolCallOutputs[ToolCallOutputs.size()] = JOutput;
                                                    }
                                                }
                                                else
                                                {
                                                    // Default to an empty output
                                                    auto JOutput            = web::json::value::object();
                                                    JOutput["tool_call_id"] = JToolCall.at("id");
                                                    JOutput["output"]       = web::json::value::string("");

                                                    // Add the tool call outputs to the responses
                                                    ToolCallOutputs[ToolCallOutputs.size()] = JOutput;
                                                }
                                            }

                                            // Submit the tool call results
                                            web::http::http_request SubmitToolOutputsRequest(web::http::methods::POST);

                                            // Set the request uri
                                            SubmitToolOutputsRequest.set_request_uri(
                                                U("threads/" + m_CurrentThreadID + "/runs/" + JRun.at("id").as_string() + "/submit_tool_outputs"));

                                            // Set the headers
                                            SubmitToolOutputsRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
                                            SubmitToolOutputsRequest.headers().add("OpenAI-Beta", "assistants=v1");
                                            SubmitToolOutputsRequest.headers().add("Content-Type", "application/json");

                                            // Create the body
                                            web::json::value SubmitToolOutputsRequestBody = web::json::value::object();
                                            SubmitToolOutputsRequestBody["tool_outputs"]  = ToolCallOutputs;
                                            SubmitToolOutputsRequestBody["stream"]        = web::json::value::boolean(true);

                                            // Set the body
                                            SubmitToolOutputsRequest.set_body(SubmitToolOutputsRequestBody);

                                            // Send the request and get the response (blocking)
                                            auto SubmitToolOutputsResponse = m_OpenAIClient->request(SubmitToolOutputsRequest).get();

                                            // FIXME: Overwriting the EventStream is a bug i think?
                                            EventStream = SubmitToolOutputsResponse.body();

                                            if (SubmitToolOutputsResponse.status_code() != web::http::status_codes::OK)
                                            {
                                                std::cerr << "Failed to submit the tool outputs" << std::endl;
                                                std::cout << SubmitToolOutputsResponse.to_string() << std::endl;
                                            }
                                        }
                                    }

                                    EventName.clear();
                                    EventData.clear();
                                }
                                else
                                {
                                    if (Line.find("event: ") == 0)
                                    {
                                        EventName = Line.substr(7);
                                    }
                                    else if (Line.find("data: ") == 0)
                                    {
                                        EventData += Line.substr(6);
                                    }
                                }
                            }

                            return pplx::task_from_result();
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

pplx::task<void> Orion::SpeakAsync(const std::string& Message, const ETTSAudioFormat AUDIO_FORMAT) const
{
    // Split the message into multiple messages if it's too long
    return SplitMessageAsync(Message).then(
        [this, AUDIO_FORMAT](const pplx::task<std::vector<std::string>>& SplitMessageTask)
        {
            const auto        SPLIT_MESSAGES {SplitMessageTask.get()};
            const std::string AUDIO_DIR {OrionWebServer::AssetDirectories::ResolveOrionAudioDir(m_CurrentAssistantID)};

            // Each orion instance has it's own folder for audio files to avoid conflicts. Append the assistant id to the audio folder.
            // Create the audio directory if it doesn't exist
            std::error_code ErrorCode;
            if (!std::filesystem::exists(AUDIO_DIR, ErrorCode))
            {
                // Create the audio directory tree
                std::filesystem::create_directories(AUDIO_DIR, ErrorCode);
                if (ErrorCode)
                {
                    std::cerr << "Failed to create the audio directory" << std::endl;
                    return;
                }
            }

            // Delete all the audio files in the directory
            for (const auto& Entry : std::filesystem::directory_iterator(AUDIO_DIR))
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
            for (const auto& Msg : SPLIT_MESSAGES)
            {
                Tasks.push_back(SpeakSingleAsync(Msg, Index, AUDIO_FORMAT));
                Index++;
            }

            // Only wait for the first task to complete.  By the time the first task is done, the rest of the tasks should be done as well or
            // enough generated audio should be available to play without buffering.
            if (Tasks.begin()->wait() != pplx::task_status::completed)
                std::cout << __FUNCTION__ << ":" << __LINE__ << ":"
                          << "Task not completed!";
        });
}

web::json::value Orion::ListSmartDevices(const std::string& Domain) const
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
                [DOMAIN_WITH_DOT](const web::http::http_response& ListStatesResponse)
                {
                    if (ListStatesResponse.status_code() == web::http::status_codes::OK)
                    {
                        return ListStatesResponse.extract_json().then(
                            [DOMAIN_WITH_DOT](const pplx::task<web::json::value>& ExtractJsonTask)
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

web::json::value Orion::ExecSmartDeviceService(const web::json::value& Devices, const std::string& Service) const
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

pplx::task<web::json::value> Orion::GetChatHistoryAsync() const
{
    // Create a new http_request to get the chat history
    web::http::http_request ListMessagesRequest(web::http::methods::GET);

    // Request uri builder
    web::uri_builder ListMessagesRequestURIBuilder;
    ListMessagesRequestURIBuilder.append_path("threads/" + m_CurrentThreadID + "/messages");
    ListMessagesRequestURIBuilder.append_query("limit", 100);
    ListMessagesRequestURIBuilder.append_query("order", "asc");
    ListMessagesRequest.set_request_uri(ListMessagesRequestURIBuilder.to_string());

    ListMessagesRequest.headers().add("Authorization", "Bearer " + m_OpenAIAPIKey);
    ListMessagesRequest.headers().add("OpenAI-Beta", "assistants=v1");

    // Check if the query parameter is present
    bool IsMarkdownRequested = ListMessagesRequest.request_uri().query().find(U("markdown=true")) != std::string::npos;

    // Send the request and get the response
    return m_OpenAIClient->request(ListMessagesRequest)
        .then(
            [=](const web::http::http_response& ListMessagesResponse)
            {
                if (ListMessagesResponse.status_code() == web::http::status_codes::OK)
                {
                    return ListMessagesResponse.extract_json()
                        .then([=](const pplx::task<web::json::value>& ExtractJsonTask) { return ExtractJsonTask.get(); })
                        .then(
                            [=](web::json::value ListMessagesResponseDataJson)
                            {
                                auto JData = ListMessagesResponseDataJson.at("data").as_array();

                                auto JChatHistory = web::json::value::array();
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
                                            auto Msg = Content.at("text").at("value").as_string();

                                            if (IsMarkdownRequested)
                                            {
                                                char* pMarkdown = cmark_markdown_to_html(Msg.c_str(), Msg.length(), CMARK_OPT_DEFAULT);
                                                Msg             = pMarkdown;
                                                free(pMarkdown);
                                            }

                                            auto JMessage       = web::json::value::object();
                                            JMessage["message"] = web::json::value::string(Msg);
                                            JMessage["role"]    = web::json::value::string(Role);

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

pplx::task<void> Orion::SpeakSingleAsync(const std::string& Message, const uint8_t INDEX, const ETTSAudioFormat AUDIO_FORMAT) const
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
        MimeType                                   = "audio/pcm";
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
    const auto        AUDIO_DIR = std::filesystem::path(OrionWebServer::AssetDirectories::ResolveOrionAudioDir(m_CurrentAssistantID));
    const std::string FILE_NAME = std::to_string(INDEX) + EXTENSION;
    const std::string FILE_PATH = (AUDIO_DIR / FILE_NAME).string();

    // Send the request and get the response
    return m_OpenAIClient->request(TextToSpeechRequest)
        .then(
            [FILE_PATH, MimeType](web::http::http_response TextToSpeechResponse)
            {
                if (TextToSpeechResponse.status_code() == web::http::status_codes::OK)
                {
                    // Stream the response to a file using concurrency::streams::fstream to avoid loading the entire response into memory
                    // Then return the filename once the file has started streaming

                    // Create a file stream
                    return concurrency::streams::fstream::open_ostream(FILE_PATH).then(
                        [TextToSpeechResponse, FILE_PATH, MimeType](const concurrency::streams::ostream& AudioFileStream)
                        {
                            // Stream the response to the file
                            if (TextToSpeechResponse.body().read_to_end(AudioFileStream.streambuf()).wait() != pplx::task_status::completed)
                                std::cout << __FUNCTION__ << ":" << __LINE__ << ": Task was not completed!";

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
            // ReSharper disable CppTooWideScopeInitStatement
            constexpr char   NEWLINE_CHAR          = '\n';
            constexpr char   PERIOD_CHAR           = '.';
            constexpr size_t MAX_LENGTH_SOFT_LIMIT = 256;
            constexpr size_t MAX_LENGTH_HARD_LIMIT = 4096;
            constexpr size_t DISTANCE_THRESHOLD    = 25;
            // ReSharper restore CppTooWideScopeInitStatement

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
                        const size_t SPLIT_INDEX =
                            (LastSplitIndex != std::string::npos && Index - LastSplitIndex <= DISTANCE_THRESHOLD) ? LastSplitIndex : Index - 1;
                        Messages.push_back(Message.substr(MessageStart, SPLIT_INDEX - MessageStart + 1));
                        MessageStart   = SPLIT_INDEX + 1;
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