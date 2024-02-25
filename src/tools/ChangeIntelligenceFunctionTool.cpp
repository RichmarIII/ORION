#include "tools/ChangeIntelligenceFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string ChangeIntelligenceFunctionTool::Execute(Orion& orion, const web::json::value& parameters)
{
    try
    {
        auto bChangeIntelligence = !parameters.at("list").as_bool();
        if (!bChangeIntelligence)
        {
            // Wants to list the available intelligences
            web::json::value json    = web::json::value::object();
            json["intelligences"]    = web::json::value::array(2);
            json["intelligences"][0] = web::json::value::string("base");
            json["intelligences"][1] = web::json::value::string("super");

            return json.serialize();
        }
        else
        {
            // Wants to change the intelligence
            std::string intelligence = parameters.at("intelligence").as_string();
            if (intelligence == "base")
            {
                orion.SetNewIntelligence(EOrionIntelligence::Base);
                return std::string(R"({"message": "Changed intelligence to base"})");
            }
            else if (intelligence == "super")
            {
                orion.SetNewIntelligence(EOrionIntelligence::Super);
                return std::string(R"({"message": "Changed intelligence to super"})");
            }
            else
            {
                std::cerr << "Unknown intelligence: " << intelligence << std::endl;
                return std::string(R"({"message": "Unknown intelligence"})");
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to change the intelligence: " << e.what() << std::endl;
        return std::string(R"({"message": "Failed to change the intelligence"})");
    }
}
