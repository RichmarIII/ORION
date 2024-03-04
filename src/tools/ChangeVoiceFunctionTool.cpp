#include "tools/ChangeVoiceFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string ChangeVoiceFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    try
    {
        // Wants to change the voice
        std::string Voice = Parameters.at("voice").as_string();
        if (Voice == "alloy")
        {
            Orion.SetNewVoice(EOrionVoice::Alloy);
            return std::string(R"({"message": "Changed voice to alloy"})");
        }
        else if (Voice == "echo")
        {
            Orion.SetNewVoice(EOrionVoice::Echo);
            return std::string(R"({"message": "Changed voice to echo"})");
        }
        else if (Voice == "fable")
        {
            Orion.SetNewVoice(EOrionVoice::Fable);
            return std::string(R"({"message": "Changed voice to fable"})");
        }
        else if (Voice == "nova")
        {
            Orion.SetNewVoice(EOrionVoice::Nova);
            return std::string(R"({"message": "Changed voice to nova"})");
        }
        else if (Voice == "onyx")
        {
            Orion.SetNewVoice(EOrionVoice::Onyx);
            return std::string(R"({"message": "Changed voice to onyx"})");
        }
        else if (Voice == "shimmer")
        {
            Orion.SetNewVoice(EOrionVoice::Shimmer);
            return std::string(R"({"message": "Changed voice to shimmer"})");
        }
        else
        {
            std::cerr << "Unknown voice: " << Voice << std::endl;
            return std::string(R"({"message": "Unknown voice"})");
        }
    }
    catch (const std::exception& Exception)
    {
        std::cerr << "Failed to change the voice: " << Exception.what() << std::endl;
        return std::string(R"({"message": "Failed to change the voice: )") + Exception.what() + R"("})";
    }
}