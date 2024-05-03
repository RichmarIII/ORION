#include "Plugin.hpp"

#include <dlfcn.h>
#include <filesystem>

using namespace ORION;

PluginModule::PluginModule()
    : m_Handle(nullptr, nullptr),
      m_Plugin(nullptr)

{
}

PluginModule::~PluginModule()
{
    Unload(/*INNER_UNLOAD=*/true);
}

bool PluginModule::Load(const std::string& InPath, const Orion& InOrion, const bool INNER_LOAD)
{
    if (m_Handle)
    {
        std::cout << "Plugin already loaded." << std::endl;
        return true;
    }

    if (!std::filesystem::exists(InPath))
    {
        std::cout << "Plugin not found: " << InPath << std::endl;
        return false;
    }

    m_Handle = std::unique_ptr<void, void (*)(void*)>(dlopen(InPath.c_str(), RTLD_LAZY), reinterpret_cast<void (*)(void*)>(dlclose));
    if (!m_Handle)
    {
        std::cout << "Failed to load plugin: " << dlerror() << std::endl;
        return false;
    }

    using CreatePluginFunc                          = IPlugin* (*) ();
    const auto                        CREATE_PLUGIN = reinterpret_cast<CreatePluginFunc>(dlsym(m_Handle.get(), "CreatePlugin"));
    if (!CREATE_PLUGIN)
    {
        std::cout << "Failed to load plugin: " << dlerror() << std::endl;
        return false;
    }

    m_Plugin = std::unique_ptr<IPlugin>(CREATE_PLUGIN());
    if (!m_Plugin)
    {
        std::cout << "Failed to load plugin: " << dlerror() << std::endl;
        return false;
    }

    if (INNER_LOAD)
    {
        m_Plugin->Load(InOrion);
    }
    return true;
}

void PluginModule::Unload(const bool INNER_UNLOAD)
{
    if (!m_Handle)
    {
        std::cout << "Plugin not loaded." << std::endl;
        return;
    }

    if (!m_Plugin)
    {
        std::cout << "Plugin not loaded." << std::endl;
        return;
    }

    if (INNER_UNLOAD)
    {
        m_Plugin->Unload();
    }
    m_Plugin.reset();
    m_Handle.reset();
}
