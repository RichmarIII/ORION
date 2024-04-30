#include "tools/DownloadHTTPFileFunctionTool.hpp"
#include "Orion.hpp"

#include <filesystem>

using namespace ORION;

using FunctionResultStatics = FunctionTool::Statics::FunctionResults;

std::string DownloadHTTPFileFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    std::cout << std::endl << "DownloadHTTPFileFunctionTool::Execute: " << Parameters.serialize() << std::endl;

    // Get the file name from the link
    std::string FileName = Parameters.at(U("link")).as_string();
    FileName             = FileName.substr(FileName.find_last_of("/") + 1);

    // Check for file extension
    if (FileName.find_last_of(".") == std::string::npos)
    {
        web::json::value ErrorObject                           = web::json::value::object();
        ErrorObject[FunctionResultStatics::NAME_RESULT.data()] = web::json::value::string(U("Link is not a file.  It is a page."));
        return ErrorObject.serialize();
    }

    // We need to convert the file name to a valid file name since it is coming from a URL encoded string
    FileName = web::uri::decode(FileName);

    // Create the http_client to send the request
    web::http::client::http_client DownloadFileLinkClient(Parameters.at(U("link")).as_string());

    // Download the file from the link
    const auto DOWNLOAD_FILE_LINK_RESPONSE = DownloadFileLinkClient.request(web::http::methods::GET).get();

    if (DOWNLOAD_FILE_LINK_RESPONSE.status_code() != web::http::status_codes::OK)
    {
        auto ErrorObject                                       = web::json::value::object();
        ErrorObject[FunctionResultStatics::NAME_RESULT.data()] = web::json::value::string(U("Failed to download file: ") + DOWNLOAD_FILE_LINK_RESPONSE.reason_phrase());
        return ErrorObject.serialize();
    }

    const auto DOWNLOAD_FILE_LINK_RESPONSE_VECTOR = DOWNLOAD_FILE_LINK_RESPONSE.extract_vector().get();

    const auto ASSET_RELATIVE_PATH = std::string("assets/") + "downloads/" + FileName;
    const auto DOWNLOAD_PATH       = std::filesystem::current_path() / ASSET_RELATIVE_PATH;

    if (!std::filesystem::exists(DOWNLOAD_PATH.parent_path()))
    {
        std::filesystem::create_directories(DOWNLOAD_PATH.parent_path());
    }

    // Save the file stream to the file
    std::ofstream File(DOWNLOAD_PATH, std::ios::binary);
    File.write(reinterpret_cast<const char*>(DOWNLOAD_FILE_LINK_RESPONSE_VECTOR.data()), DOWNLOAD_FILE_LINK_RESPONSE_VECTOR.size());
    File.close();

    auto Response = web::json::value::object();
    Response[FunctionResultStatics::NAME_ORION_INSTRUCTIONS.data()] =
        web::json::value::string(U("you MUST use the value of 'file_path' VERBATIM and MUST show the link to the user. eg [](file_path)"));

    // The web server will serve the file from the assets folder
    const auto WEB_SERVER_RELATIVE_PATH = "https:/" + ASSET_RELATIVE_PATH;

    auto ResultObject                                   = web::json::value::object();
    ResultObject["file_path"]                           = web::json::value::string(WEB_SERVER_RELATIVE_PATH);
    Response[FunctionResultStatics::NAME_RESULT.data()] = ResultObject;

    return Response.serialize();
}