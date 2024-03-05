#include "tools/ChangeIntelligenceFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string ChangeIntelligenceFunctionTool::Execute(Orion& Orion, const web::json::value& PARAMETERS)
{
    try
    {
        const auto SHOULD_CHANGE_INTELLIGENCE = !PARAMETERS.at("list").as_bool();
        if (!SHOULD_CHANGE_INTELLIGENCE)
        {
            // Wants to list the available intelligences
            web::json::value Json    = web::json::value::object();
            Json["intelligences"]    = web::json::value::array(2);
            Json["intelligences"][0] = web::json::value::string("base");
            Json["intelligences"][1] = web::json::value::string("super");

            return Json.serialize();
        }
        else
        {
            // Wants to change the intelligence
            const std::string INTELLIGENCE = PARAMETERS.at("intelligence").as_string();
            if (INTELLIGENCE == "base")
            {
                Orion.SetNewIntelligence(EOrionIntelligence::Base);
                return std::string(R"({"message": "Changed intelligence to base"})");
            }
            else if (INTELLIGENCE == "super")
            {
                Orion.SetNewIntelligence(EOrionIntelligence::Advanced);
                return std::string(R"({"message": "Changed intelligence to super"})");
            }
            else
            {
                std::cerr << "Unknown intelligence: " << INTELLIGENCE << std::endl;
                return std::string(R"({"message": "Unknown intelligence"})");
            }
        }
    }
    catch (const std::exception& Exception)
    {
        std::cerr << "Failed to change the intelligence: " << Exception.what() << std::endl;
        return std::string(R"({"message": "Failed to change the intelligence: )") + Exception.what() + "\"}";
    }
}
