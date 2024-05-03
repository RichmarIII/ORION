#include "AutonomyPlugin.hpp"
#include "CreateAutonomousActionPlanFunctionTool.hpp"

using namespace ORION;

void AutonomyPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<CreateAutonomousActionPlanFunctionTool>());
}

void AutonomyPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> AutonomyPlugin::GetTools() const
{
    return { m_Tools.back().get() };
}