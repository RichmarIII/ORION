#include "tools/SearchFilesystemFunctionTool.hpp"
#include "Orion.hpp"

#include <filesystem>

using namespace ORION;

std::string SearchFilesystemFunctionTool::Execute(Orion& orion, const web::json::value& parameters)
{
    try
    {
        // Default directory is the users "Home" directory
        std::filesystem::path SearchDirectory = std::filesystem::path(std::getenv("HOME"));

        // Check if the parameters contain a directory
        if (parameters.has_field("search_directory"))
        {
            // Set the search directory to the directory in the parameters
            SearchDirectory = parameters.at("search_directory").as_string();
        }

        // Check if directory exists
        if (!std::filesystem::exists(SearchDirectory))
        {
            std::cerr << "Directory does not exist: " << SearchDirectory << std::endl;
            return std::string(R"({"message": "Directory does not exist"})");
        }

        bool Recursive = false;
        if (parameters.has_field("recursive"))
        {
            Recursive = parameters.at("recursive").as_bool();
        }

        if (parameters.has_field("file_name"))
        {
            // Get the file name from the parameters
            std::string FileName = parameters.at("file_name").as_string();

            std::vector<std::string> FileMatches;

            // Case insensitive search
            std::transform(FileName.begin(), FileName.end(), FileName.begin(), ::tolower);

            if (Recursive)
            {
                for (const auto& File : std::filesystem::recursive_directory_iterator(SearchDirectory))
                {
                    if (File.is_regular_file())
                    {
                        std::string FileNameLower = File.path().filename().string();
                        std::transform(FileNameLower.begin(), FileNameLower.end(), FileNameLower.begin(), ::tolower);
                        if (FileNameLower.find(FileName) != std::string::npos)
                        {
                            FileMatches.push_back(File.path().string());
                        }
                    }
                }
            }
            else
            {
                for (const auto& File : std::filesystem::directory_iterator(SearchDirectory))
                {
                    if (File.is_regular_file())
                    {
                        std::string FileNameLower = File.path().filename().string();
                        std::transform(FileNameLower.begin(), FileNameLower.end(), FileNameLower.begin(), ::tolower);
                        if (FileNameLower.find(FileName) != std::string::npos)
                        {
                            FileMatches.push_back(File.path().string());
                        }
                    }
                }
            }

            if (FileMatches.empty())
            {
                return std::string(R"({"message": "No files found"})");
            }

            web::json::value json = web::json::value::array(FileMatches.size());
            for (size_t i = 0; i < FileMatches.size(); i++)
            {
                json[i] = web::json::value::string(FileMatches[i]);
            }

            return json.serialize();
        }
        else
        {
            std::cerr << "No file name provided" << std::endl;
            return std::string(R"({"message": "No file name provided"})");
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return std::string(R"({"message": "Failed to search the filesystem: )") + e.what() + R"("})";
    }
}