#include "tools/GetWeatherFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string GetWeatherFunctionTool::Execute(Orion& orion, const web::json::value& parameters)
{
    // Default units is "imperial"
    std::string units = "imperial";
    if (parameters.has_field("units"))
    {
        units = parameters.at("units").as_string();
    }

    // Default location is "New York, US"
    std::string Location = "New York, US";
    if (parameters.has_field("location"))
    {
        Location = parameters.at("location").as_string();
    }

    // Create a new http_client to send the request
    web::http::client::http_client client(U("https://api.openweathermap.org/data/2.5/"));

    // Create a new http_request to get the weather
    web::http::http_request request(web::http::methods::GET);
    request.headers().add("Content-Type", "application/json");
    request.headers().add("Accept", "application/json");
    request.headers().add("User-Agent", "ORION");

    // Add the query parameters to the request
    web::uri_builder builder;
    builder.append_path("weather");
    builder.append_query("q", Location);
    builder.append_query("appid", orion.GetOpenWeatherAPIKey());
    builder.append_query("units", units);
    request.set_request_uri(builder.to_string());

    // Send the request and get the response
    web::http::http_response response = client.request(request).get();

    if (response.status_code() == web::http::status_codes::OK)
    {
        web::json::value json = response.extract_json().get();
        return json.serialize();
    }
    else
    {
        std::cerr << "Failed to get the weather" << std::endl;
        std::cout << response.to_string() << std::endl;
        return std::string(R"({"message": "Failed to get the weather. )" + response.to_string() + R"("})");
    }
}