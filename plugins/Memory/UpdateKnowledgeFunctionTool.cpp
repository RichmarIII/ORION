#include "UpdateKnowledgeFunctionTool.hpp"
#include "GUID.hpp"
#include "Knowledge.hpp"
#include "Orion.hpp"
#include "OrionWebServer.hpp"

#include <filesystem>

using namespace ORION;

std::string UpdateKnowledgeFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    try
    {
        // Get the existing knowledge type
        const auto EXISTING_KNOWLEDGE_TYPE = Parameters.at(U("existing_knowledge_type")).as_string();

        // Get the new knowledge
        const auto NEW_KNOWLEDGE = Parameters.at(U("new_knowledge")).as_string();

        // Get the new knowledge subject
        const auto NEW_KNOWLEDGE_SUBJECT_ARRAY = Parameters.at(U("new_knowledge_subject_and_tags")).as_array();

        const std::string NEW_KNOWLEDGE_SUBJECT = [&]()
        {
            std::string KnowledgeSubject;
            for (const auto& Tag : NEW_KNOWLEDGE_SUBJECT_ARRAY)
            {
                KnowledgeSubject += Tag.as_string() + " ";
            }
            return KnowledgeSubject;
        }();

        // Get the existing knowledge ids
        if (!Parameters.has_field(U("existing_knowledge_ids")))
        {
            return U("The existing knowledge ids parameter is missing. call recall_knowledge to get the existing knowledge ids. then call "
                     "update_knowledge with the existing knowledge ids to update the knowledge.");
        }

        const auto EXISTING_KNOWLEDGE_IDS = Parameters.at(U("existing_knowledge_ids")).as_array();

        // Generate a unique ID for the knowledge
        const auto KNOWLEDGE_ID = static_cast<std::string>(GUID::Generate());

        std::string DatabaseFileName;

        if (EXISTING_KNOWLEDGE_TYPE == U("user_personal_info"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::USER_PERSONAL_INFO;
        }
        else if (EXISTING_KNOWLEDGE_TYPE == U("user_interests"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::USER_INTERESTS;
        }
        else if (EXISTING_KNOWLEDGE_TYPE == U("family_personal_info"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::FAMILY_PERSONAL_INFO;
        }
        else if (EXISTING_KNOWLEDGE_TYPE == U("family_interests"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::FAMILY_INTERESTS;
        }
        else if (EXISTING_KNOWLEDGE_TYPE == U("user_preferences"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::USER_PREFERENCES;
        }
        else if (EXISTING_KNOWLEDGE_TYPE == U("unknown"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::UNKNOWN;
        }
        else
        {
            return U("Invalid knowledge type.");
        }

        // Get the user ID to store the knowledge in the correct directory
        const auto USER_ID = Orion.GetUserID();

        // Get the directory to store the knowledge
        const auto APP_RELATIVE_KNOWLEDGE_DIR = OrionWebServer::AssetDirectories::ResolveUserKnowledgeDir(USER_ID);

        // Get the path to the database file
        const auto DatabaseFilePath = std::filesystem::path(APP_RELATIVE_KNOWLEDGE_DIR) / DatabaseFileName;

        // Create the database directories if they don't exist
        std::filesystem::create_directories(DatabaseFilePath.parent_path());

        // A vector to store the updated knowledge fragments
        std::vector<web::json::value> KnowledgeFragmentsAfterRemoval;

        // First we need to delete the existing knowledge fragments that are being updated

        {
            // Load the knowledge database for reading
            auto DatabaseFileStream = std::ifstream(DatabaseFilePath, std::ios::in);

            // Read the knowledge database line by line
            std::string Line;
            while (std::getline(DatabaseFileStream, Line))
            {
                if (Line.empty())
                {
                    continue;
                }

                // Parse the knowledge line
                const auto JKnowledgeFragment = web::json::value::parse(Line);

                // Check if the knowledge fragment is being updated
                if (std::find(EXISTING_KNOWLEDGE_IDS.begin(), EXISTING_KNOWLEDGE_IDS.end(), JKnowledgeFragment.at(U("knowledge_id"))) == EXISTING_KNOWLEDGE_IDS.end())
                {
                    // Add the knowledge fragment to the vector
                    KnowledgeFragmentsAfterRemoval.push_back(JKnowledgeFragment);
                }
            }
        }

        // Get the current date and time
        const auto CurrentDateTime        = std::chrono::system_clock::now();
        const auto CurrentDateTimeInTimeT = std::chrono::system_clock::to_time_t(CurrentDateTime);
        const auto CurrentDateTimeString  = std::ctime(&CurrentDateTimeInTimeT);

        // Construct the knowledge object
        web::json::value Knowledge                 = web::json::value::object();
        Knowledge[U("knowledge_subject_and_tags")] = web::json::value::string(NEW_KNOWLEDGE_SUBJECT);
        Knowledge[U("knowledge")]                  = web::json::value::string(NEW_KNOWLEDGE);
        Knowledge[U("date_time_knowledge_stored")] = web::json::value::string(CurrentDateTimeString);
        Knowledge[U("knowledge_id")]               = web::json::value::string(KNOWLEDGE_ID);

        // Add the knowledge to the vector
        KnowledgeFragmentsAfterRemoval.push_back(Knowledge);

        // Write the updated knowledge fragments to the database
        {
            // Load the knowledge database for writing
            auto DatabaseFileStream = std::ofstream(DatabaseFilePath, std::ios::out);

            // Write the knowledge fragments to the database
            for (const auto& JKnowledgeFragment : KnowledgeFragmentsAfterRemoval)
            {
                DatabaseFileStream << JKnowledgeFragment.serialize() << std::endl;
            }
        }

        return U("Knowledge stored successfully.");
    }
    catch (const std::exception& Exception)
    {
        return U("Failed to store knowledge: " + std::string(Exception.what()));
    }
}