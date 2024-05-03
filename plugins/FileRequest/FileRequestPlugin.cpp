#include "FileRequestPlugin.hpp"
#include "RequestFileUploadFromUserFunctionTool.hpp"

using namespace ORION;

void FileRequestPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<RequestFileUploadFromUserFunctionTool>());
}

void FileRequestPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> FileRequestPlugin::GetTools() const
{
    return { m_Tools.back().get() };
}