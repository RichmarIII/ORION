#include "VisionPlugin.hpp"
#include "TakeScreenshotFunctionTool.hpp"

using namespace ORION;

void VisionPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<TakeScreenshotFunctionTool>());
}

void VisionPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> VisionPlugin::GetTools() const
{
    return { m_Tools.back().get() };
}