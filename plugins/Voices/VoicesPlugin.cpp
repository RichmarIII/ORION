#include "VoicesPlugin.hpp"
#include "ChangeVoiceFunctionTool.hpp"

using namespace ORION;

void VoicesPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<ChangeVoiceFunctionTool>());
}

void VoicesPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> VoicesPlugin::GetTools() const
{
    return { m_Tools.back().get() };
}