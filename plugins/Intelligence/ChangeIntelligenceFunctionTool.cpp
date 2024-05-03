#include "ChangeIntelligenceFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

using FunctionResults = FunctionTool::Statics::FunctionResults;

std::string ChangeIntelligenceFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    constexpr std::string_view NAME_INTELLIGENCE_BASE     = "base";
    constexpr std::string_view NAME_INTELLIGENCE_ADVANCED = "advanced";
    constexpr std::string_view NAME_INTELLIGENCES         = "intelligences";

    try
    {
        if (const auto SHOULD_CHANGE_INTELLIGENCE = !Parameters.at("list").as_bool(); !SHOULD_CHANGE_INTELLIGENCE)
        {
            // Wants to list the available intelligences
            auto JFunctionResult = web::json::value::object();
            auto JIntelligences  = web::json::value::object();

            JIntelligences[NAME_INTELLIGENCES.data()]            = web::json::value::array(2);
            JIntelligences[NAME_INTELLIGENCES.data()][0]         = web::json::value::string(NAME_INTELLIGENCE_BASE.data());
            JIntelligences[NAME_INTELLIGENCES.data()][1]         = web::json::value::string(NAME_INTELLIGENCE_ADVANCED.data());
            JFunctionResult[FunctionResults::NAME_RESULT.data()] = JIntelligences;

            return JFunctionResult.serialize();
        }
        else
        {
            // Wants to change the intelligence
            if (const std::string INTELLIGENCE = Parameters.at("intelligence").as_string(); INTELLIGENCE == NAME_INTELLIGENCE_BASE)
            {
                Orion.SetNewIntelligence(EOrionIntelligence::Base);

                auto JFunctionResult                                 = web::json::value::object();
                JFunctionResult[FunctionResults::NAME_RESULT.data()] = web::json::value::string("Changed intelligence to base");

                return JFunctionResult.serialize();
            }
            else if (INTELLIGENCE == NAME_INTELLIGENCE_ADVANCED)
            {
                Orion.SetNewIntelligence(EOrionIntelligence::Advanced);

                auto JFunctionResult                                 = web::json::value::object();
                JFunctionResult[FunctionResults::NAME_RESULT.data()] = web::json::value::string("Changed intelligence to advanced");

                return JFunctionResult.serialize();
            }
            else
            {
                std::cout << __func__ << ": Unknown intelligence: " << INTELLIGENCE << std::endl;

                auto JFunctionResult                                 = web::json::value::object();
                JFunctionResult[FunctionResults::NAME_RESULT.data()] = web::json::value::string("Unknown intelligence: " + INTELLIGENCE);

                return JFunctionResult.serialize();
            }
        }
    }
    catch (const std::exception& Exception)
    {
        std::cout << __func__ << ": Failed to change the intelligence: " << Exception.what() << std::endl;

        auto JFunctionResult                                 = web::json::value::object();
        JFunctionResult[FunctionResults::NAME_RESULT.data()] = web::json::value::string("Failed to change the intelligence: " + std::string(Exception.what()));

        return JFunctionResult.serialize();
    }
}
