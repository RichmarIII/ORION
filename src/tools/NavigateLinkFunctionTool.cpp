#include "tools/NavigateLinkFunctionTool.hpp"
#include "Orion.hpp"
#include "MimeTypes.hpp"

using namespace ORION;

std::string NavigateLinkFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    std::cout << "NavigateLinkFunctionTool::Execute: " << Parameters.at(U("link")).as_string() << std::endl;

    // Check if link is a file
    if (MimeTypes::GetMimeType(Parameters.at(U("link")).as_string()) != "application/octet-stream")
    {
        // Link is a file with a known mime type, instruct orion that it should analyze the file or download it
        web::json::value NavigateLinkResult = web::json::value::object();
        NavigateLinkResult[U("content")]    = web::json::value::string(U("The link is a file with a known mime type. Not a web page."));
        NavigateLinkResult[U("next_action")] =
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
        web::json::value ErrorObject = web::json::value::object();
        ErrorObject[U("error")]      = web::json::value::string(U("The content of the link is not html"));
        return ErrorObject.serialize();
    }

    // Parse the response
    const auto RESPONSE_CONTENT = NAVIGATE_LINK_RESPONSE.extract_string().get();

    // Create the result
    web::json::value NavigateLinkResult = web::json::value::object();
    NavigateLinkResult[U("content")]    = web::json::value::string(RESPONSE_CONTENT);

    // Serialize the result to a string
    auto Result = NavigateLinkResult.serialize();

    // We only want the content, none of the html markup or other stuff.
    // So we will strip out everything but the content.

    // Firt we get the start and end of the body tag
    if (const auto START_BODY = Result.find(U("<body")); START_BODY != std::string::npos)
    {
        // If we found the body tag, we will strip out everything but the content
        if (const auto END_BODY = Result.find(U("</body>")); END_BODY != std::string::npos)
        {
            // Get the content
            Result = Result.substr(START_BODY, END_BODY - START_BODY);
        }
    }

    // Remove all the script tags
    while (true)
    {
        // Find the start of the script tag

        // If we found the start of the script tag, we will find the end of the script tag and remove it
        if (const auto START_SCRIPT = Result.find(U("<script")); START_SCRIPT != std::string::npos)
        {
            // Find the end of the script tag

            // If we found the end of the script tag, we will remove the script tag
            if (const auto END_SCRIPT = Result.find(U("</script>"), START_SCRIPT); END_SCRIPT != std::string::npos)
            {
                // Remove the script tag
                Result = Result.substr(0, START_SCRIPT) + Result.substr(END_SCRIPT + 9);
            }
            else
            {
                // If we did not find the end of the script tag, we will break out of the loop
                break;
            }
        }
        else
        {
            // If we did not find the start of the script tag, we will break out of the loop
            break;
        }
    }

    // Remove all the style tags
    while (true)
    {
        // Find the start of the style tag

        // If we found the start of the style tag, we will find the end of the style tag and remove it
        if (const auto START_STYLE = Result.find(U("<style")); START_STYLE != std::string::npos)
        {
            // Find the end of the style tag

            // If we found the end of the style tag, we will remove the style tag
            if (const auto END_STYLE = Result.find(U("</style>"), START_STYLE); END_STYLE != std::string::npos)
            {
                // Remove the style tag
                Result = Result.substr(0, START_STYLE) + Result.substr(END_STYLE + 8);
            }
            else
            {
                // If we did not find the end of the style tag, we will break out of the loop
                break;
            }
        }
        else
        {
            // If we did not find the start of the style tag, we will break out of the loop
            break;
        }
    }

    // Remove all the noscript tags
    while (true)
    {
        // Find the start of the noscript tag

        // If we found the start of the noscript tag, we will find the end of the noscript tag and remove it
        if (const auto START_NO_SCRIPT = Result.find(U("<noscript")); START_NO_SCRIPT != std::string::npos)
        {
            // Find the end of the noscript tag

            // If we found the end of the noscript tag, we will remove the noscript tag
            if (const auto END_NO_SCRIPT = Result.find(U("</noscript>"), START_NO_SCRIPT); END_NO_SCRIPT != std::string::npos)
            {
                // Remove the noscript tag
                Result = Result.substr(0, START_NO_SCRIPT) + Result.substr(END_NO_SCRIPT + 11);
            }
            else
            {
                // If we did not find the end of the noscript tag, we will break out of the loop
                break;
            }
        }
        else
        {
            // If we did not find the start of the noscript tag, we will break out of the loop
            break;
        }
    }

    // Remove all the comments
    while (true)
    {
        // Find the start of the comment

        // If we found the start of the comment, we will find the end of the comment and remove it
        if (const auto START_COMMENT = Result.find(U("<!--")); START_COMMENT != std::string::npos)
        {
            // Find the end of the comment

            // If we found the end of the comment, we will remove the comment
            if (const auto END_COMMENT = Result.find(U("-->"), START_COMMENT); END_COMMENT != std::string::npos)
            {
                // Remove the comment
                Result = Result.substr(0, START_COMMENT) + Result.substr(END_COMMENT + 3);
            }
            else
            {
                // If we did not find the end of the comment, we will break out of the loop
                break;
            }
        }
        else
        {
            // If we did not find the start of the comment, we will break out of the loop
            break;
        }
    }

    // Remove all button tags
    while (true)
    {
        // Find the start of the button tag

        // If we found the start of the button tag, we will find the end of the button tag and remove it
        if (const auto START_BUTTON = Result.find(U("<button")); START_BUTTON != std::string::npos)
        {
            // Find the end of the button tag

            // If we found the end of the button tag, we will remove the button tag
            if (const auto END_BUTTON = Result.find(U("</button>"), START_BUTTON); END_BUTTON != std::string::npos)
            {
                // Remove the button tag
                Result = Result.substr(0, START_BUTTON) + Result.substr(END_BUTTON + 9);
            }
            else
            {
                // If we did not find the end of the button tag, we will break out of the loop
                break;
            }
        }
        else
        {
            // If we did not find the start of the button tag, we will break out of the loop
            break;
        }
    }

    // Strip all attributes from the tags
    while (true)
    {
        // Find the start of the tag

        // If we found the start of the tag, we will find the end of the tag and remove the attributes
        if (const auto START_TAG = Result.find(U("<")); START_TAG != std::string::npos)
        {
            // Find the end of the tag

            // If we found the end of the tag, we will remove the attributes
            if (const auto END_TAG = Result.find(U(">"), START_TAG); END_TAG != std::string::npos)
            {
                // Remove the attributes
                Result = Result.substr(0, START_TAG) + Result.substr(END_TAG + 1);
            }
            else
            {
                // If we did not find the end of the tag, we will break out of the loop
                break;
            }
        }
        else
        {
            // If we did not find the start of the tag, we will break out of the loop
            break;
        }
    }

    // Remove all extra whitespace and replace it with a single space
    while (true)
    {
        // Find the start of the extra whitespace

        // If we found the start of the extra whitespace, we will remove it
        if (const auto START_WHITESPACE = Result.find(U("  ")); START_WHITESPACE != std::string::npos)
        {
            // Remove the extra whitespace
            Result = Result.substr(0, START_WHITESPACE) + U(" ") + Result.substr(START_WHITESPACE + 2);
        }
        else
        {
            // If we did not find the start of the extra whitespace, we will break out of the loop
            break;
        }
    }

    // Remove all the extra newlines and replace them with a single newline
    while (true)
    {
        // Find the start of the extra newline

        // If we found the start of the extra newline, we will remove it
        if (const auto START_NEWLINE = Result.find(U("\n\n")); START_NEWLINE != std::string::npos)
        {
            // Remove the extra newline
            Result = Result.substr(0, START_NEWLINE) + U("\n") + Result.substr(START_NEWLINE + 2);
        }
        else
        {
            // If we did not find the start of the extra newline, we will break out of the loop
            break;
        }
    }

    // Maku sure the result is not too long (512kb)
    if (Result.size() >= 512 * 1024)
    {
        // If the result is too long, truncate it
        Result = Result.substr(0, 512 * 1024);
    }

    std::cout << std::endl << "NavigateLinkFunctionTool::Execute: " << Result << std::endl;

    web::json::value NavigateLinkResult2  = web::json::value::object();
    NavigateLinkResult2[U("next_action")] = web::json::value::string(
        U("The web_search function should be used on the returned content to complete the search until the answer is found."));
    NavigateLinkResult2[U("final_action")] =
        web::json::value::string(U("When Complete. Make sure to cite the source of the information with a link to the page you got the information "
                                   "from. a user-clickable (href) link MUST be provided, simply stating the source is not enough."));
    NavigateLinkResult2[U("user_query")] = Parameters.at(U("user_query"));
    NavigateLinkResult2[U("content")]    = web::json::value::string(Result);

    // Serialize the result to a string
    Result = NavigateLinkResult2.serialize();

    // Return the result
    return Result;
}