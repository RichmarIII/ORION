#include "FileSystemPlugin.hpp"
#include "SearchFilesystemFunctionTool.hpp"

using namespace ORION;

void FileSystemPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<SearchFilesystemFunctionTool>());
}

void FileSystemPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> FileSystemPlugin::GetTools() const
{
    return { m_Tools.back().get() };
}