#include "WeatherPlugin.hpp"
#include "GetWeatherFunctionTool.hpp"

using namespace ORION;

void WeatherPlugin::Load(const Orion& InOrion)
{
    m_Tools.push_back(std::make_unique<GetWeatherFunctionTool>());
}

void WeatherPlugin::Unload()
{
    m_Tools.clear();
}

std::vector<FunctionTool*> WeatherPlugin::GetTools() const
{
    return { m_Tools.back().get() };
}