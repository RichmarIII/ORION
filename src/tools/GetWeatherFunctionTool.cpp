#include "tools/GetWeatherFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string GetWeatherFunctionTool::Execute(Orion& Orion, const web::json::value& PARAMETERS)
{
    // Default units is "imperial"
    std::string Units = "imperial";
    if (PARAMETERS.has_field("units"))
    {
        Units = PARAMETERS.at("units").as_string();
    }

    // Default location is "New York, US"
    std::string Location = "New York, US";
    if (PARAMETERS.has_field("location"))
    {
        Location = PARAMETERS.at("location").as_string();
    }

    // Create a new http_client to send the request
    web::http::client::http_client OpenWeatherMapClient(U("https://api.openweathermap.org/data/2.5/"));

    // Create a new http_request to get the weather
    web::http::http_request GetWeatherRequest(web::http::methods::GET);
    GetWeatherRequest.headers().add("Content-Type", "application/json");
    GetWeatherRequest.headers().add("Accept", "application/json");
    GetWeatherRequest.headers().add("User-Agent", "ORION");

    // Add the query parameters to the request
    web::uri_builder GetWeatherRequestURIBuilder;
    GetWeatherRequestURIBuilder.append_path("weather");
    GetWeatherRequestURIBuilder.append_query("q", Location);
    GetWeatherRequestURIBuilder.append_query("appid", Orion.GetOpenWeatherAPIKey());
    GetWeatherRequestURIBuilder.append_query("units", Units);
    GetWeatherRequest.set_request_uri(GetWeatherRequestURIBuilder.to_string());

    // Send the request and get the response
    web::http::http_response GetWeatherResponse = OpenWeatherMapClient.request(GetWeatherRequest).get();

    if (GetWeatherResponse.status_code() == web::http::status_codes::OK)
    {
        web::json::value ResponseDataJson = GetWeatherResponse.extract_json().get();
        return ResponseDataJson.serialize();
    }
    else
    {
        std::cerr << "Failed to get the weather" << std::endl;
        std::cout << GetWeatherResponse.to_string() << std::endl;
        return std::string(R"({"message": "Failed to get the weather. )" + GetWeatherResponse.to_string() + R"("})");
    }
}