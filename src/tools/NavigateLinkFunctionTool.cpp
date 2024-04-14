#include "tools/NavigateLinkFunctionTool.hpp"
#include "MimeTypes.hpp"
#include "Orion.hpp"

#include <regex>

using namespace ORION;

using FunctionResultStatics = FunctionTool::Statics::FunctionResults;

std::string NavigateLinkFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    std::cout << "NavigateLinkFunctionTool::Execute: " << Parameters.at(U("link")).as_string() << std::endl;

    // Check if link is a file
    if (MimeTypes::GetMimeType(Parameters.at(U("link")).as_string()) != "application/octet-stream")
    {
        // Link is a file with a known mime type, instruct orion that it should analyze the file or download it
        web::json::value NavigateLinkResult = web::json::value::object();
        NavigateLinkResult[FunctionResultStatics::NAME_RESULT.data()] =
            web::json::value::string(U("The link is a file with a known mime type. Not a web page."));
        NavigateLinkResult[FunctionResultStatics::NAME_ORION_INSTRUCTIONS.data()] =
            web::json::value::string(U("concider analyzing with python (code_interpreter) and generate a summary."));

        // Serialize the result to a string
        auto Result = NavigateLinkResult.serialize();

        // Return the result
        return Result;
    }

    // Create the http_client to send the request
    web::http::client::http_client NavigateLinkClient(Parameters.at(U("link")).as_string());

    // Send the request
    const web::http::http_request  NAVIGATE_LINK_REQUEST(web::http::methods::GET);
    const web::http::http_response NAVIGATE_LINK_RESPONSE = NavigateLinkClient.request(NAVIGATE_LINK_REQUEST).get();

    // content of the link must be html
    if (const auto CONTENT_TYPE = NAVIGATE_LINK_RESPONSE.headers().content_type(); CONTENT_TYPE.find(U("text/html")) == std::string::npos)
    {
        web::json::value ErrorObject                           = web::json::value::object();
        ErrorObject[FunctionResultStatics::NAME_RESULT.data()] = web::json::value::string(U("The content of the link is not html"));
        ErrorObject[FunctionResultStatics::NAME_ORION_INSTRUCTIONS.data()] =
            web::json::value::string(U("The content of the link is not html. Try a different link."));

        std::cout << std::endl << "NavigateLinkFunctionTool::Execute: " << ErrorObject.serialize() << std::endl;

        return ErrorObject.serialize();
    }

    // Parse the response
    const auto RESPONSE_CONTENT = NAVIGATE_LINK_RESPONSE.extract_string().get();

    // We only want the content, none of the html markup or other stuff.
    // So we will strip out everything but the content.
    // We will use a regular expression to do this. Replace the markup with an empty string so we are left with just the content.

    const std::regex BODY_REGEX("<body[^>]*>(.*?)</body>", std::regex::icase | std::regex::ECMAScript);

    // Find and keep the body content
    if (std::smatch BodyMatch; std::regex_search(RESPONSE_CONTENT, BodyMatch, BODY_REGEX))
    {
        const std::string BODY_CONTENT = BodyMatch[1];

        // Remove all the html tags
        const std::regex HTML_TAG_REGEX("<[^>]*>", std::regex::icase | std::regex::ECMAScript);

        // Replace all the html tags with an empty string
        const std::string BODY_CONTENT_NO_TAGS = std::regex_replace(BODY_CONTENT, HTML_TAG_REGEX, "");

        std::string FinalResult = BODY_CONTENT_NO_TAGS;

        // Maku sure the result is not too long
        if (constexpr size_t MAX_RESULT_SIZE = 512 * 1024; FinalResult.size() >= MAX_RESULT_SIZE)
        {
            // If the result is too long, truncate it
            FinalResult = FinalResult.substr(0, MAX_RESULT_SIZE);

            // TODO: Maybe integrage vector search to only keep the most relevant content
        }

        // Create the result
        web::json::value NavigateLinkResult                                       = web::json::value::object();
        NavigateLinkResult[FunctionResultStatics::NAME_RESULT.data()]             = web::json::value::string(FinalResult);
        NavigateLinkResult[FunctionResultStatics::NAME_ORION_INSTRUCTIONS.data()] = web::json::value::string(
            U("Analyze the content of the link. If the user's question was not answered, The web_search function should be used on the returned "
              "content to complete the search until the answer is found."
              "If the user's question was answered, make sure to cite the source of the information with a link to the page you got the information "
              "from. a user-clickable (href) link MUST be provided, simply stating the source is not enough."));

        std::cout << std::endl << "NavigateLinkFunctionTool::Execute: " << NavigateLinkResult.serialize() << std::endl;

        return NavigateLinkResult.serialize();
    }

    {
        web::json::value NavigateLinkResult = web::json::value::object();
        NavigateLinkResult[FunctionResultStatics::NAME_ORION_INSTRUCTIONS.data()] =
            web::json::value::string(U("There was no content in the body of the web page. This is unexpected. Try a different link or the web_search "
                                       "function should be used again, but with a slightly different "
                                       "query.  Preferably a different link if possible."));

        NavigateLinkResult[FunctionResultStatics::NAME_USER_QUERY.data()] = Parameters.at(FunctionResultStatics::NAME_USER_QUERY.data());

        std::cout << std::endl << "NavigateLinkFunctionTool::Execute: " << NavigateLinkResult.serialize() << std::endl;

        return NavigateLinkResult.serialize();
    }
}