#pragma once

#include <string_view>
#include <vector>

#include "tools/FunctionTool.hpp"

namespace ORION
{
    struct IPlugin
    {
        virtual ~IPlugin() = default;

        /**
         * @brief Load the plugin. This is called when the plugin is loaded by Orion.
         *
         * @param InOrion The Orion instance that is loading the plugin.
         */
        virtual void Load(const class Orion& InOrion) = 0;

        /**
         * @brief Unload the plugin. This is called when the plugin is unloaded by Orion.
         */
        virtual void Unload() = 0;

        /**
         * @brief Get the name of the plugin.
         *
         * @return std::string_view The name of the plugin.
         */
        virtual std::string_view GetName() const = 0;

        /**
         * @brief Get the description of the plugin.
         *
         * @return std::string_view The description of the plugin.
         */
        virtual std::string_view GetDescription() const = 0;

        /**
         * @brief Get the author of the plugin.
         *
         * @return std::string_view The author of the plugin.
         */
        virtual std::string_view GetAuthor() const = 0;

        /**
         * @brief Get the version of the plugin.
         *
         * @return std::string_view The version of the plugin.
         */
        virtual std::string_view GetVersion() const = 0;

        /**
         * @brief Get the tools that this plugin provides. This is a list of tools that Orion can use to interact with the plugin.
         * Tools are what Orion uses to interact with the plugin. They define the format of your tool such as name, description, and parameters.
         * They also define the function that Orion will call when the tool is executed.
         *
         * @return std::vector<struct FunctionTool*> The tools that this plugin provides. The plugin owns the tools and they should
         * not be cached unless you know what you are doing (they are not valid after the plugin is unloaded).
         */
        virtual std::vector<struct FunctionTool*> GetTools() const = 0;

        /**
         * @brief Get the instructions for a specific tool. Most tools will REALLY benefit from defining custom instructions for Orion to follow.
         * These instruction will act as the "system" prompt for Orion. This greatly enhances Orions ability to interact and understand the tool.
         * This is THE chance for the tool to define how it wants to be interacted with by Orion. The final "system" prompt for Orion will
         * be a combination of all loaded plugins and the default instructions defined by Orion.  They will not be used verbatim, but rather
         * as input for Orion to generate the final prompt.
         *
         * @return std::string_view The instructions for the tool.
         */
        virtual std::string_view GetToolInstructions() const = 0;
    };

    struct PluginModule
    {
         PluginModule();
        ~PluginModule();

        /**
         * @brief Load the plugin from the specified path.
         *
         * @param InPath The path to the plugin.
         * @param InOrion The Orion instance that is loading the plugin.
         * @param INNER_LOAD Weather to call Load on the plugin after it is created.
         * @return true If the plugin was loaded successfully.
         * @return false If the plugin failed to load.
         */
        bool Load(const std::string& InPath, const Orion& InOrion, const bool INNER_LOAD = false);

        /**
         * @brief Unload the plugin.
         *
         * @param INNER_UNLOAD Weather to call Unload on the plugin before it is destroyed.
         */
        void Unload(const bool INNER_UNLOAD = false);

        inline IPlugin* GetPlugin() const
        {
            return m_Plugin.get();
        }

    protected:
        std::unique_ptr<void, void (*)(void*)> m_Handle;
        std::unique_ptr<IPlugin>               m_Plugin;
    };

} // namespace ORION

#define DECLARE_PLUGIN(Class, Name, Description, Instructions, Author, Version) \
    class Class final : public ORION::IPlugin                                   \
    {                                                                           \
    public:                                                                     \
        ~                Class() override = default;                            \
        void             Load(const ORION::Orion& InOrion) override;            \
        void             Unload() override;                                     \
        std::string_view GetName() const override                               \
        {                                                                       \
            return Name;                                                        \
        }                                                                       \
        std::string_view GetDescription() const override                        \
        {                                                                       \
            return Description;                                                 \
        }                                                                       \
        std::string_view GetAuthor() const override                             \
        {                                                                       \
            return Author;                                                      \
        }                                                                       \
        std::string_view GetVersion() const override                            \
        {                                                                       \
            return Version;                                                     \
        }                                                                       \
        std::vector<FunctionTool*> GetTools() const override;                   \
        std::string_view           GetToolInstructions() const override         \
        {                                                                       \
            return Instructions;                                                \
        }                                                                       \
                                                                                \
    protected:                                                                  \
        std::vector<std::unique_ptr<FunctionTool>> m_Tools;                     \
    };                                                                          \
    extern "C" ORION::IPlugin* CreatePlugin()                                   \
    {                                                                           \
        return new Class();                                                     \
    }                                                                           \
    extern "C" const char* GetPluginName()                                      \
    {                                                                           \
        {                                                                       \
            return Name;                                                        \
        }                                                                       \
    }
