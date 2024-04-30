#include "tools/RememberKnowledgeFunctionTool.hpp"
#include "GUID.hpp"
#include "Knowledge.hpp"
#include "Orion.hpp"
#include "OrionWebServer.hpp"

#include <filesystem>

using namespace ORION;

std::string RememberKnowledgeFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    try
    {
        // Get the knowledge type
        const auto KNOWLEDGE_TYPE = Parameters.at(U("knowledge_type")).as_string();

        // Get the knowledge
        const auto KNOWLEDGE = Parameters.at(U("knowledge")).as_string();

        // Get the knowledge subject
        const auto KNOWLEDGE_SUBJECT_ARRAY = Parameters.at(U("knowledge_subject_and_tags")).as_array();

        const std::string KNOWLEDGE_SUBJECT = [&]()
        {
            std::string KnowledgeSubject;
            for (const auto& Tag : KNOWLEDGE_SUBJECT_ARRAY)
            {
                KnowledgeSubject += Tag.as_string() + " ";
            }
            return KnowledgeSubject;
        }();

        // Generate a unique ID for the knowledge
        const auto KNOWLEDGE_ID = static_cast<std::string>(GUID::Generate());

        std::string DatabaseFileName;

        if (KNOWLEDGE_TYPE == U("user_personal_info"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::USER_PERSONAL_INFO;
        }
        else if (KNOWLEDGE_TYPE == U("user_interests"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::USER_INTERESTS;
        }
        else if (KNOWLEDGE_TYPE == U("family_personal_info"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::FAMILY_PERSONAL_INFO;
        }
        else if (KNOWLEDGE_TYPE == U("family_interests"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::FAMILY_INTERESTS;
        }
        else if (KNOWLEDGE_TYPE == U("user_preferences"))
        {
            DatabaseFileName = KnowledgeFileNameStatics::USER_PREFERENCES;
        }
        else if (KNOWLEDGE_TYPE == U("unknown"))
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

        {
            // First we need to check if the knowledge already exists in the database (we don't want to store duplicate knowledge)
            // We also want to expand the knowledge if it already exists or remove it if it is no longer valid
            // We also want to update the knowledge if it has changed

            // A vector to store the matching knowledge fragments
            std::vector<std::pair<web::json::value, double>> MatchingKnowledgeFragments;

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

                    // Check if the knowledge subject matches
                    const auto SIMILARITY = Orion.GetSemanticSimilarity(KNOWLEDGE_SUBJECT, JKnowledgeFragment.at(U("knowledge_subject_and_tags")).as_string());
                    if (SIMILARITY >= 0.8)
                    {
                        MatchingKnowledgeFragments.push_back({ JKnowledgeFragment, SIMILARITY });
                    }
                }
            }

            // Sort the matching knowledge fragments by similarity (highest to lowest)
            std::sort(MatchingKnowledgeFragments.begin(), MatchingKnowledgeFragments.end(), [](const auto& A, const auto& B) { return A.second > B.second; });

            if (!MatchingKnowledgeFragments.empty())
            {
                // Notify the ai that similar knowledge already exists and that it can should me removed and incorporated into a new, larger knowledge
                // fragment
                web::json::value MatchingKnowledgeFragmentIds = web::json::value::array();
                web::json::value JResult                      = web::json::value::object();
                for (const auto& [JKnowledgeFragment, SIMILARITY] : MatchingKnowledgeFragments)
                {
                    MatchingKnowledgeFragmentIds[MatchingKnowledgeFragmentIds.size()] = JKnowledgeFragment.at(U("knowledge_id"));
                }
                JResult[U("matching_knowledge_fragment_ids")] = MatchingKnowledgeFragmentIds;
                JResult[U("instructions_for_orion")] =
                    web::json::value::string(U("Similar knowledge already exists. You MUST remove the existing knowledge and incorporate it into a new, larger, more cohesive "
                                               "knowledge fragment taking into account the updated information and existing information. This MUST be done by calling the "
                                               "update_knowledge function. matching_knowledge_fragment_ids contains the ids of the matching knowledge fragments that must be "
                                               "removed. pay special attention to the parameters of the update_knowledge function."));
                JResult[U("next_action")] = web::json::value::string(U("update_knowledge"));

                return JResult.serialize();
            }
        }

        // Get the current date and time
        const auto CurrentDateTime        = std::chrono::system_clock::now();
        const auto CurrentDateTimeInTimeT = std::chrono::system_clock::to_time_t(CurrentDateTime);
        const auto CurrentDateTimeString  = std::ctime(&CurrentDateTimeInTimeT);

        // Construct the knowledge object
        web::json::value Knowledge                 = web::json::value::object();
        Knowledge[U("knowledge_subject_and_tags")] = web::json::value::string(KNOWLEDGE_SUBJECT);
        Knowledge[U("knowledge")]                  = web::json::value::string(KNOWLEDGE);
        Knowledge[U("date_time_knowledge_stored")] = web::json::value::string(CurrentDateTimeString);
        Knowledge[U("knowledge_id")]               = web::json::value::string(KNOWLEDGE_ID);

        // Write the knowledge to the database
        {
            // Load the knowledge database for writing
            auto DatabaseFileStream = std::ofstream(DatabaseFilePath, std::ios::app);

            // Write the knowledge to the database
            DatabaseFileStream << Knowledge.serialize() << std::endl;
        }

        return U("Knowledge stored successfully.");
    }
    catch (const std::exception& Exception)
    {
        return U("Failed to store knowledge: " + std::string(Exception.what()));
    }
}