#include "tools/NavigateLinkFunctionTool.hpp"
#include "MimeTypes.hpp"
#include "Orion.hpp"

#include <regex>

#include <myhtml/myhtml.h>
#include <myhtml/tag_const.h>

using namespace ORION;

using FunctionResultStatics = FunctionTool::Statics::FunctionResults;

std::string NavigateLinkFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    std::cout << "NavigateLinkFunctionTool::Execute: " << Parameters.at("link").as_string() << std::endl;

    // Check if link is a file
    if (MimeTypes::GetMimeType(Parameters.at("link").as_string()) != "application/octet-stream")
    {
        // Link is a file with a known mime type, instruct orion that it should analyze the file or download it
        web::json::value NavigateLinkResult                           = web::json::value::object();
        NavigateLinkResult[FunctionResultStatics::NAME_RESULT.data()] = web::json::value::string("The link is a file with a known mime type. Not a web page.");
        NavigateLinkResult[FunctionResultStatics::NAME_ORION_INSTRUCTIONS.data()] =
            web::json::value::string("concider analyzing with python (code_interpreter) and generate a summary.");

        // Serialize the result to a string
        auto Result = NavigateLinkResult.serialize();

        // Return the result
        return Result;
    }

    // Create the http_client to send the request
    web::http::client::http_client NavigateLinkClient(Parameters.at("link").as_string());

    // Send the request
    const web::http::http_request NAVIGATE_LINK_REQUEST(web::http::methods::GET);
    const auto                    NAVIGATE_LINK_RESPONSE = NavigateLinkClient.request(NAVIGATE_LINK_REQUEST).get();

    // content of the link must be html
    if (const auto CONTENT_TYPE = NAVIGATE_LINK_RESPONSE.headers().content_type(); CONTENT_TYPE.find("text/html") == std::string::npos)
    {
        auto ErrorObject                                                   = web::json::value::object();
        ErrorObject[FunctionResultStatics::NAME_RESULT.data()]             = web::json::value::string("The content of the link is not html");
        ErrorObject[FunctionResultStatics::NAME_ORION_INSTRUCTIONS.data()] = web::json::value::string("The content of the link is not html. Try a different link.");

        std::cout << std::endl << "NavigateLinkFunctionTool::Execute: " << ErrorObject.serialize() << std::endl;

        return ErrorObject.serialize();
    }

    // Parse the response
    const auto RESPONSE_CONTENT = NAVIGATE_LINK_RESPONSE.extract_string().get();

    // We only want the content, none of the html markup or other stuff.
    // So we will strip out everything but the content.
    // We will use myhtml to parse the html and extract the content.
    {
        // In outer scope so that the myhtml object is destroyed after the tree object since the tree object depends on the myhtml object
        const auto MY_HTML = std::unique_ptr<myhtml, myhtml* (*) (myhtml*)>(myhtml_create(), myhtml_destroy);

        {
            myhtml_init(MY_HTML.get(), MyHTML_OPTIONS_DEFAULT, 1, 0);

            // tree init
            const auto HTML_TREE = std::unique_ptr<myhtml_tree, myhtml_tree* (*) (myhtml_tree*)>(myhtml_tree_create(), myhtml_tree_destroy);
            myhtml_tree_init(HTML_TREE.get(), MY_HTML.get());

            // parse html
            myhtml_parse(HTML_TREE.get(), MyENCODING_UTF_8, RESPONSE_CONTENT.data(), RESPONSE_CONTENT.size());

            // get root element
            const auto HTML_ROOT = myhtml_tree_get_document(HTML_TREE.get());

            // find the body element recursively
            myhtml_tree_node_t* pBodyElement = nullptr;

            const std::function<void(myhtml_tree_node*)> FIND_BODY_ELEMENT_FUNC = [&pBodyElement, &FIND_BODY_ELEMENT_FUNC](myhtml_tree_node_t* pNode)
            {
                myhtml_tree_node_t* pChild = myhtml_node_child(pNode);

                while (pChild != nullptr)
                {
                    if (myhtml_node_tag_id(pChild) == MyHTML_TAG_BODY)
                    {
                        pBodyElement = pChild;
                        return;
                    }

                    FIND_BODY_ELEMENT_FUNC(pChild);

                    pChild = myhtml_node_next(pChild);
                }
            };

            FIND_BODY_ELEMENT_FUNC(HTML_ROOT);

            // create a string to hold the content
            std::string Content;

            // remove all tags from the body element leaving only the text content
            if (pBodyElement != nullptr)
            {
                // iterate over the children of the body element recursively
                const std::function<void(myhtml_tree_node*)> EXTRACT_CONTENT_FUNC = [&Content, &EXTRACT_CONTENT_FUNC](myhtml_tree_node_t* pNode)
                {
                    myhtml_tree_node_t* pChild = myhtml_node_child(pNode);

                    while (pChild != nullptr)
                    {
                        // Skip wholesale (these tags should not contain any useful text content or sub tags)
                        if (myhtml_node_tag_id(pChild) == MyHTML_TAG_SCRIPT || myhtml_node_tag_id(pChild) == MyHTML_TAG_STYLE ||
                            myhtml_node_tag_id(pChild) == MyHTML_TAG_NOSCRIPT || myhtml_node_tag_id(pChild) == MyHTML_TAG_IFRAME ||
                            myhtml_node_tag_id(pChild) == MyHTML_TAG_EMBED || myhtml_node_tag_id(pChild) == MyHTML_TAG_OBJECT || myhtml_node_tag_id(pChild) == MyHTML_TAG_APPLET)
                        {
                            pChild = myhtml_node_next(pChild);
                            continue;
                        }
                        if (myhtml_node_tag_id(pChild) == MyHTML_TAG__TEXT)
                        {
                            std::string TextContent;

                            // if the parent is a link tag, append the href attribute to the string
                            if (auto pParent = myhtml_node_parent(pChild); pParent != nullptr && myhtml_node_tag_id(pParent) == MyHTML_TAG_A)
                            {
                                if (auto pHrefAttribute = myhtml_attribute_by_key(pParent, "href", strlen("href")); pHrefAttribute != nullptr)
                                {
                                    TextContent += myhtml_attribute_value(pHrefAttribute, nullptr) + std::string(" ");
                                }
                            }

                            // append the text content to the string but we don't want to append any type of whitespace (tabs, newlines, etc)
                            // except for spaces.  We want to keep the text content as a single line with spaces separating the words
                            // so we will use a regex to remove any whitespace that is not a space
                            TextContent += myhtml_node_text(pChild, nullptr);
                            TextContent = std::regex_replace(TextContent, std::regex("[^\\S ]"), "");

                            // append the text content to the string
                            Content += TextContent + " ";
                        }
                        else
                        {
                            EXTRACT_CONTENT_FUNC(pChild);
                        }

                        pChild = myhtml_node_next(pChild);
                    }
                };

                EXTRACT_CONTENT_FUNC(pBodyElement);
            }

            if (!Content.empty())
            {
                // replace all duplicate spaces with a single space
                Content = std::regex_replace(Content, std::regex(" +"), " ");

                // create the result
                auto NavigateLinkResult = web::json::value::object();
                NavigateLinkResult[FunctionResultStatics::NAME_ORION_INSTRUCTIONS.data()] =
                    web::json::value::string("Anaylize the result. The navigate_link function should be used on any returned links to complete the search until the answer is "
                                             "found. "
                                             "If the user's answer is found, make sure to cite the source of the information with a link to the page you got the information "
                                             "from. A user-clickable (href) link MUST be provided, simply stating the source is not enough");

                NavigateLinkResult[FunctionResultStatics::NAME_USER_QUERY.data()] = Parameters.at(FunctionResultStatics::NAME_USER_QUERY.data());
                NavigateLinkResult[FunctionResultStatics::NAME_RESULT.data()]     = web::json::value::string(Content);

                std::cout << std::endl << "NavigateLinkFunctionTool::Execute: " << NavigateLinkResult.serialize() << std::endl;

                return NavigateLinkResult.serialize();
            }
        }
    }

    {
        web::json::value NavigateLinkResult = web::json::value::object();
        NavigateLinkResult[FunctionResultStatics::NAME_ORION_INSTRUCTIONS.data()] =
            web::json::value::string("There was no content in the body of the web page. This is unexpected. Try a different link or the web_search "
                                     "function should be used again, but with a slightly different "
                                     "query.  Preferably a different link if possible.");

        NavigateLinkResult[FunctionResultStatics::NAME_USER_QUERY.data()] = Parameters.at(FunctionResultStatics::NAME_USER_QUERY.data());

        std::cout << std::endl << "NavigateLinkFunctionTool::Execute: " << NavigateLinkResult.serialize() << std::endl;

        return NavigateLinkResult.serialize();
    }
}