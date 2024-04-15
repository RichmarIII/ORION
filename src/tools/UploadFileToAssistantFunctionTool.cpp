#include "tools/UploadFileToAssistantFunctionTool.hpp"
#include "Orion.hpp"
#include "OrionWebServer.hpp"

#include <filesystem>

using namespace ORION;

std::string
UploadFileToAssistantFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    std::cout << std::endl << "UploadFileToAssistantFunctionTool::Execute: " << Parameters.serialize() << std::endl;

    // Get the file path from the parameters
    std::string FilePath = Parameters.has_field(U("file_path")) ? Parameters.at(U("file_path")).as_string() : "";

    if (FilePath.empty())
    {
        web::json::value ErrorObject = web::json::value::object();
        ErrorObject[U("error")]      = web::json::value::string(U("File path is empty."));
        return ErrorObject.serialize();
    }

    if (!std::filesystem::exists(FilePath) || !std::filesystem::is_regular_file(FilePath))
    {
        web::json::value ErrorObject = web::json::value::object();
        ErrorObject[U("error")]      = web::json::value::string(U("File path is not a file or doesn't exist."));
        return ErrorObject.serialize();
    }

    // Upload of the file will be queued by the client.  We will send an sse event to the client what file to upload.
    // The client will then upload the file by creating and sending a new chat message with the file attached.
    // This is done for multiple reasons:
    // 1. The file can be large and tools cannot run for a long time, they will get cancelled.
    // 2. It will go through the same process as a normal file upload, so the file will be stored in the same way.
    // 3. The client will be able to see the progress of the upload.
    // 4. The AI will be able to see the file in the chat history. Which is required for the AI to process the file durring autonomous action plans.
    //    If the file is uploaded directly to the assistant, it will have no way of knowing when the file is uploaded and ready to be processed.
    //    A message being sent to the assistant is the trigger for the assistant to process the file. And the assistant will not respond until the
    //    file is processed. For autonomous action plans, the assistant may say it needs to upload a file as part of the action plan. The only way for
    //    the assistant to know the file is fully uploaded and ready to move on with the action plan is by the client sending a message with the file
    //    attached.  That message is what triggers a response from the assistant.

    // Send an SSE event to the client to upload the file.
    web::json::value EventObject = web::json::value::object();
    EventObject[U("file_path")]  = web::json::value::string(FilePath);
    Orion.GetWebServer().SendServerEvent(U(OrionWebServer::SSEOrionEventNames::UPLOAD_FILE_REQUESTED), EventObject);

    // Tell Orion that the user is about to upload a file and to wait for the file to be uploaded.
    web::json::value ResponseObject = web::json::value::object();
    ResponseObject[U("content")]    = web::json::value::string(U("The file that is required is: " + FilePath));
    ResponseObject[U("instructions_for_orion")] =
        web::json::value::string(U("The user is about to upload a file in a chat message.  This file is REQUIRED to continue. Wait for the file to "
                                   "be uploaded before continuing with the "
                                   "autonomous action plan or next steps."));
    return ResponseObject.serialize();
}