#include "Orion.hpp"
#include "tools/ChangeIntelligenceFunctionTool.hpp"
#include "tools/ChangeVoiceFunctionTool.hpp"
#include "tools/CodeInterpreterTool.hpp"
#include "tools/GetWeatherFunctionTool.hpp"
#include "tools/RetrievalTool.hpp"
#include "tools/SearchFilesystemFunctionTool.hpp"
#include "tools/TakeScreenshotFunctionTool.hpp"
#include "tools/WebSearchFunctionTool.hpp"

using namespace ORION;

int main(int argc, char* argv[])
{
    // Declare Tools
    std::vector<std::unique_ptr<IOrionTool>> Tools {};
    Tools.reserve(10);

    Tools.push_back(std::make_unique<TakeScreenshotFunctionTool>());
    Tools.push_back(std::make_unique<SearchFilesystemFunctionTool>());
    Tools.push_back(std::make_unique<GetWeatherFunctionTool>());
    Tools.push_back(std::make_unique<WebSearchFunctionTool>());
    Tools.push_back(std::make_unique<ChangeVoiceFunctionTool>());
    Tools.push_back(std::make_unique<ChangeIntelligenceFunctionTool>());
    Tools.push_back(std::make_unique<RetrievalTool>());
    Tools.push_back(std::make_unique<CodeInterpreterTool>());

    // Create an instance of Orion
    Orion Orion {std::move(Tools)};

    Orion.Run();

    return 0;
}