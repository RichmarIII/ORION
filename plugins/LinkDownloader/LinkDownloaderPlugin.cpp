#include "LinkDownloaderPlugin.hpp"
#include "DownloadHTTPFileFunctionTool.hpp"

using namespace ORION;

void LinkDownloaderPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<DownloadHTTPFileFunctionTool>());
}

void LinkDownloaderPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> LinkDownloaderPlugin::GetTools() const
{
    return {m_Tools.back().get()};
}