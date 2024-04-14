#include "tools/ChangeVoiceFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

std::string ChangeVoiceFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    try
    {
        if (!Parameters.has_field("voice"))
        {
            // Wants to list the available voices
            auto JMessage = web::json::value::object();
            auto JVoices  = web::json::value::array();

            JVoices[JVoices.size()] = web::json::value::string("alloy");
            JVoices[JVoices.size()] = web::json::value::string("echo");
            JVoices[JVoices.size()] = web::json::value::string("fable");
            JVoices[JVoices.size()] = web::json::value::string("nova");
            JVoices[JVoices.size()] = web::json::value::string("onyx");
            JVoices[JVoices.size()] = web::json::value::string("shimmer");

            JMessage["message"] = JVoices;
            return JMessage.serialize();
        }

        // Wants to change the voice
        if (const std::string VOICE = Parameters.at("voice").as_string(); VOICE == "alloy")
        {
            Orion.SetNewVoice(EOrionVoice::Alloy);
            return std::string(R"({"message": "Changed voice to alloy"})");
        }
        else if (VOICE == "echo")
        {
            Orion.SetNewVoice(EOrionVoice::Echo);
            return std::string(R"({"message": "Changed voice to echo"})");
        }
        else if (VOICE == "fable")
        {
            Orion.SetNewVoice(EOrionVoice::Fable);
            return std::string(R"({"message": "Changed voice to fable"})");
        }
        else if (VOICE == "nova")
        {
            Orion.SetNewVoice(EOrionVoice::Nova);
            return std::string(R"({"message": "Changed voice to nova"})");
        }
        else if (VOICE == "onyx")
        {
            Orion.SetNewVoice(EOrionVoice::Onyx);
            return std::string(R"({"message": "Changed voice to onyx"})");
        }
        else if (VOICE == "shimmer")
        {
            Orion.SetNewVoice(EOrionVoice::Shimmer);
            return std::string(R"({"message": "Changed voice to shimmer"})");
        }
        else
        {
            std::cerr << "Unknown voice: " << VOICE << std::endl;
            return std::string(R"({"message": "Unknown voice"})");
        }
    }
    catch (const std::exception& Exception)
    {
        std::cerr << "Failed to change the voice: " << Exception.what() << std::endl;
        return std::string(R"({"message": "Failed to change the voice: )") + Exception.what() + R"("})";
    }
}