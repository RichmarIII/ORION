#include "HomeAssistantPlugin.hpp"
#include "ExecSmartDeviceServiceFunctionTool.hpp"
#include "ListSmartDevicesFunctionTool.hpp"

using namespace ORION;

void HomeAssistantPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<ListSmartDevicesFunctionTool>());
    m_Tools.push_back(std::make_unique<ExecSmartDeviceServiceFunctionTool>());
}

void HomeAssistantPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> HomeAssistantPlugin::GetTools() const
{
    std::vector<FunctionTool*> Tools;
    for (const auto& Tool : m_Tools)
    {
        Tools.push_back(Tool.get());
    }
    return Tools;
}