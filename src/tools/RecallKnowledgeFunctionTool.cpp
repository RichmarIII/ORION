#include "tools/RecallKnowledgeFunctionTool.hpp"
#include "Knowledge.hpp"
#include "Orion.hpp"
#include "OrionWebServer.hpp"

#include <filesystem>

using namespace ORION;

std::string RecallKnowledgeFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    try
    {
        // Get the knowledge type
        // const auto KNOWLEDGE_TYPES = Parameters.at(U("knowledge_types")).as_array();

        // Force all knowledge types for now
        web::json::array KnowledgeTypes = web::json::value::array().as_array();
        KnowledgeTypes[0]               = web::json::value::string(U("user_personal_info"));
        KnowledgeTypes[1]               = web::json::value::string(U("user_interests"));
        KnowledgeTypes[2]               = web::json::value::string(U("family_personal_info"));
        KnowledgeTypes[3]               = web::json::value::string(U("family_interests"));
        KnowledgeTypes[4]               = web::json::value::string(U("user_preferences"));
        KnowledgeTypes[5]               = web::json::value::string(U("unknown"));

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

        // Create a json array to store matching memory fragments
        web::json::value JMatchingMemoryFragmentResultsArray = web::json::value::array();

        for (const auto& KT : KnowledgeTypes)
        {
            const auto KNOWLEDGE_TYPE = KT.as_string();

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
            const auto DATABASE_FILE_PATH = std::filesystem::path(APP_RELATIVE_KNOWLEDGE_DIR) / DatabaseFileName;

            // Create the database directories if they don't exist
            std::filesystem::create_directories(DATABASE_FILE_PATH.parent_path());

            std::ifstream DatabaseFileSteam { DATABASE_FILE_PATH };

            // Database failed to open
            if (!DatabaseFileSteam.is_open())
            {
                web::json::value JSearchResults = web::json::value::object();
                JSearchResults[U("instructions_for_assistant")] =
                    web::json::value::string(U("No knowledge found. Maybe broaden your search (incorporate other knowledge_types). or use other methods to"
                                               "fill users request/statement."));

                return JSearchResults.serialize();
            }

            // Json Memory Fragment
            web::json::array JMemoryFragments = web::json::value::array().as_array();

            // Each memory fragment is separated by a newline
            while (DatabaseFileSteam && !DatabaseFileSteam.eof())
            {
                std::string MemoryFragment;
                std::getline(DatabaseFileSteam, MemoryFragment);

                if (MemoryFragment.empty())
                {
                    continue;
                }

                // Add the memory fragment to the json array
                JMemoryFragments[JMemoryFragments.size()] = web::json::value::parse(MemoryFragment);
            }

            std::vector<std::pair<web::json::value, double>> MatchingMemoryFragments;

            // For each memory fragment, check if the knowledge subject matches
            for (const auto& MemoryFragment : JMemoryFragments)
            {
                if (const auto SUBJECT_SIMILARITY = Orion.GetSemanticSimilarity(KNOWLEDGE_SUBJECT, MemoryFragment.at(U("knowledge_subject_and_tags")).as_string());
                    SUBJECT_SIMILARITY > 0.3)
                {
                    MatchingMemoryFragments.push_back({ MemoryFragment, SUBJECT_SIMILARITY });
                }
            }

            // Sort the matching memory fragments by most probable first
            std::sort(MatchingMemoryFragments.begin(), MatchingMemoryFragments.end(), [](const auto& A, const auto& B) { return A.second > B.second; });

            // If no matching memory fragments were found
            if (MatchingMemoryFragments.empty())
            {
                continue;
            }

            // Add the matching memory fragments to the json array
            for (const auto& [MemFragment, CosSimilarity] : MatchingMemoryFragments)
            {
                web::json::value FragmentResult                                                 = web::json::value::object();
                FragmentResult[U("cosine_similarity")]                                          = web::json::value::number(CosSimilarity);
                FragmentResult[U("knowledge")]                                                  = web::json::value::string(MemFragment.serialize());
                JMatchingMemoryFragmentResultsArray[JMatchingMemoryFragmentResultsArray.size()] = FragmentResult;
            }
        }

        // If no matching memory fragments were found
        if (JMatchingMemoryFragmentResultsArray.size() == 0)
        {
            web::json::value JSearchResults = web::json::value::object();
            JSearchResults[U("next_function")] =
                web::json::value::string(U("No knowledge found. call again with more knowledge_types and/or rephrase knowledge_subject_and_tags."));
            return JSearchResults.serialize();
        }

        // Return the knowledge
        web::json::value JSearchResults        = web::json::value::object();
        JSearchResults[U("recalled_memories")] = JMatchingMemoryFragmentResultsArray;
        JSearchResults[U("instructions_for_orion")] =
            web::json::value::string(U("Recalled memory. These are the most similar memories, sorted by most relevant first. Use the knowledge to help the user with their "
                                       "request/statement. If the knowledge is not"
                                       "sufficient, try to gather more information from the user to help them better. Make sure the knowledge is "
                                       "interpreted in the correct context."));

        // Log the recalled memories
        std::cout << std::endl << std::endl << "Recalled Memories: " << std::endl << JSearchResults.serialize() << std::endl << std::endl;

        return JSearchResults.serialize();
    }
    catch (const std::exception& Exception)
    {
        return U("Failed to store knowledge: " + std::string(Exception.what()));
    }
}