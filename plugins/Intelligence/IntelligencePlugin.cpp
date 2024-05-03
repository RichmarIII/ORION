#include "IntelligencePlugin.hpp"

#include "ChangeIntelligenceFunctionTool.hpp"

using namespace ORION;

void IntelligencePlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<ChangeIntelligenceFunctionTool>());
}

void IntelligencePlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> IntelligencePlugin::GetTools() const
{
    return { m_Tools.back().get() };
}