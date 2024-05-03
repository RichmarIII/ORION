#include "LinkReaderPlugin.hpp"
#include "NavigateLinkFunctionTool.hpp"

using namespace ORION;

void LinkReaderPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<NavigateLinkFunctionTool>());
}

void LinkReaderPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> LinkReaderPlugin::GetTools() const
{
    return { m_Tools.back().get() };
}