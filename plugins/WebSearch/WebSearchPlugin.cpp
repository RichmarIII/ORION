#include "WebSearchPlugin.hpp"
#include "WebSearchFunctionTool.hpp"

using namespace ORION;

void WebSearchPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<WebSearchFunctionTool>());
}

void WebSearchPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> WebSearchPlugin::GetTools() const
{
    return { m_Tools.back().get() };
}