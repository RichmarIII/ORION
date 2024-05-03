#include "MemoryPlugin.hpp"
#include "RecallKnowledgeFunctionTool.hpp"
#include "RememberKnowledgeFunctionTool.hpp"
#include "UpdateKnowledgeFunctionTool.hpp"

using namespace ORION;

void MemoryPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<RememberKnowledgeFunctionTool>());
    m_Tools.push_back(std::make_unique<RecallKnowledgeFunctionTool>());
    m_Tools.push_back(std::make_unique<UpdateKnowledgeFunctionTool>());
}

void MemoryPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> MemoryPlugin::GetTools() const
{
    std::vector<FunctionTool*> Tools;
    for (const auto& Tool : m_Tools)
    {
        Tools.push_back(Tool.get());
    }
    return Tools;
}