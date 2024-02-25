#include "tools/ChangeVoiceFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string ChangeVoiceFunctionTool::Execute(Orion& orion, const web::json::value& parameters)
{
    try
    {
        // Wants to change the voice
        std::string voice = parameters.at("voice").as_string();
        if (voice == "alloy")
        {
            orion.SetNewVoice(EOrionVoice::Alloy);
            return std::string(R"({"message": "Changed voice to alloy"})");
        }
        else if (voice == "echo")
        {
            orion.SetNewVoice(EOrionVoice::Echo);
            return std::string(R"({"message": "Changed voice to echo"})");
        }
        else if (voice == "fable")
        {
            orion.SetNewVoice(EOrionVoice::Fable);
            return std::string(R"({"message": "Changed voice to fable"})");
        }
        else if (voice == "nova")
        {
            orion.SetNewVoice(EOrionVoice::Nova);
            return std::string(R"({"message": "Changed voice to nova"})");
        }
        else if (voice == "onyx")
        {
            orion.SetNewVoice(EOrionVoice::Onyx);
            return std::string(R"({"message": "Changed voice to onyx"})");
        }
        else if (voice == "shimmer")
        {
            orion.SetNewVoice(EOrionVoice::Shimmer);
            return std::string(R"({"message": "Changed voice to shimmer"})");
        }
        else
        {
            std::cerr << "Unknown voice: " << voice << std::endl;
            return std::string(R"({"message": "Unknown voice"})");
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to change the voice: " << e.what() << std::endl;
        return std::string(R"({"message": "Failed to change the voice: )") + e.what() + R"("})";
    }
}