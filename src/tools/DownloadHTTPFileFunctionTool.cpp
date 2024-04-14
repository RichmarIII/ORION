#include "tools/DownloadHTTPFileFunctionTool.hpp"
#include "Orion.hpp"

#include <filesystem>

using namespace ORION;

std::string DownloadHTTPFileFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    std::cout << std::endl << "DownloadHTTPFileFunctionTool::Execute: " << Parameters.serialize() << std::endl;

    // Get the file name from the link
    std::string FileName = Parameters.at(U("link")).as_string();
    FileName             = FileName.substr(FileName.find_last_of("/") + 1);

    // Check for file extension
    if (FileName.find_last_of(".") == std::string::npos)
    {
        web::json::value ErrorObject = web::json::value::object();
        ErrorObject[U("error")]      = web::json::value::string(U("Link is not a file.  It is a page."));
        return ErrorObject.serialize();
    }

    // Create the http_client to send the request
    web::http::client::http_client DownloadFileLinkClient(Parameters.at(U("link")).as_string());

    // Download the file from the link
    web::http::http_response DownloadFileLinkResponse = DownloadFileLinkClient.request(web::http::methods::GET).get();

    if (DownloadFileLinkResponse.status_code() != web::http::status_codes::OK)
    {
        web::json::value ErrorObject = web::json::value::object();
        ErrorObject[U("error")]      = web::json::value::string(U("Failed to download file: ") + DownloadFileLinkResponse.reason_phrase());
        return ErrorObject.serialize();
    }

    const auto DOWNLOAD_FILE_LINK_RESPONSE_VECTOR = DownloadFileLinkResponse.extract_vector().get();

    std::filesystem::path DownloadPath = std::filesystem::current_path() / "downloads" / FileName;

    if (!std::filesystem::exists(std::filesystem::current_path() / "downloads"))
    {
        std::filesystem::create_directories(std::filesystem::current_path() / "downloads");
    }

    // Save the file stream to the file
    std::ofstream File(DownloadPath, std::ios::binary);
    File.write(reinterpret_cast<const char*>(DOWNLOAD_FILE_LINK_RESPONSE_VECTOR.data()), DOWNLOAD_FILE_LINK_RESPONSE_VECTOR.size());
    File.close();

    web::json::value Response = web::json::value::object();
    Response[U("instructions_for_orion")] =
        web::json::value::string(U("you MUST display the file as a link to the user using the file:// protocol."));

    Response[U("path")] = web::json::value::string(DownloadPath.string());
    return Response.serialize();
}