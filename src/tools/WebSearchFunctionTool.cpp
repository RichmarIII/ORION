#include "tools/WebSearchFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

using FunctionResultStatics = FunctionTool::Statics::FunctionResults;

std::string WebSearchFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    std::cout << "WebSearchFunctionTool::Execute: " << Parameters.at(U("query")).as_string() << std::endl;

    // Create the http_client to send the request.  We use googles custom search api
    web::http::client::http_client SearchClient(U("https://customsearch.googleapis.com"));
    web::http::uri_builder         SearchURIBuilder(U("/customsearch/v1"));
    SearchURIBuilder.append_query(U("key"), U("AIzaSyCqrTooKJWqeohxOFnA_4JWafNBjEl27tY"));
    SearchURIBuilder.append_query(U("q"), Parameters.at(U("query")).as_string());
    SearchURIBuilder.append_query(U("cx"), U("0051bfd3f21224b04"));

    // Send the request
    web::http::http_request SearchRequest(web::http::methods::GET);
    SearchRequest.set_request_uri(SearchURIBuilder.to_string());
    const web::http::http_response SEARCH_RESPONSE = SearchClient.request(SearchRequest).get();

    if (SEARCH_RESPONSE.status_code() != web::http::status_codes::OK)
    {
        web::json::value ErrorObject = web::json::value::object();
        ErrorObject[FunctionResultStatics::NAME_RESULT.data()] =
            web::json::value::string(U("Failed to search the web: ") + SEARCH_RESPONSE.reason_phrase());
        return ErrorObject.serialize();
    }

    // Parse the response
    web::json::value JsonResponseBody = SEARCH_RESPONSE.extract_json().get();

    // Create the result
    web::json::value SearchResult                                       = web::json::value::object();
    SearchResult[FunctionResultStatics::NAME_ORION_INSTRUCTIONS.data()] = web::json::value::string(
        U("The navigate_link function should be used on any returned links to complete the search until the answer is found. "
          "If the user's answer is found, make sure to cite the source of the information with a link to the page you got the information "
          "from. A user-clickable (href) link MUST be provided, simply stating the source is not enough"));

    // Create the results entry so we can add the items to it
    auto& SearchResultResultRef = SearchResult[FunctionResultStatics::NAME_RESULT.data()] = web::json::value::object();

    // Create the items entry so we can add the search results to it
    auto& ItemsArray = SearchResultResultRef[U("items")].as_array();

    for (auto& Item : JsonResponseBody.at(U("items")).as_array())
    {
        web::json::value ItemObject = web::json::value::object();

        ItemObject[U("title")]   = Item.has_field(U("title")) ? Item.at(U("title")) : web::json::value::string(U("No Title"));
        ItemObject[U("link")]    = Item.has_field(U("link")) ? Item.at(U("link")) : web::json::value::string(U("No Link"));
        ItemObject[U("snippet")] = Item.has_field(U("snippet")) ? Item.at(U("snippet")) : web::json::value::string(U("No Snippet"));

        ItemsArray[ItemsArray.size()] = ItemObject;
    }

    // Serialize the result to a string
    const auto RESULT = SearchResult.serialize();

    std::cout << "WebSearchFunctionTool::Execute: " << RESULT << std::endl;

    // Return the result
    return RESULT;
}