#pragma once

#include <cstdint> // for uint8_t
#include <string>  // for std::string

namespace ORION
{
    /// @brief The type of tool that ORION supports
    enum class EOrionToolType : uint8_t
    {
        /// @brief A tool that can run and interpret python code
        CodeInterpreter,

        /// @brief A tool that can access and analyze data uploaded to the server
        Retrieval,

        /// @brief A tool capable of running a function (a plugin of sorts)
        Function
    };

    /// @brief An interface for all tools that ORION supports
    struct IOrionTool
    {
        virtual ~IOrionTool()                    = default;
        IOrionTool()                             = default;
        IOrionTool(const IOrionTool&)            = default;
        IOrionTool(IOrionTool&&)                 = default;
        IOrionTool& operator=(const IOrionTool&) = default;
        IOrionTool& operator=(IOrionTool&&)      = default;

        /// @brief  Get the type of the tool
        virtual EOrionToolType GetType() const = 0;

        /// @brief  Convert the tool to a JSON string
        /// @return The JSON string
        virtual std::string ToJson() const = 0;

        /// @brief  Get the name of the tool
        /// @return The name of the tool
        virtual std::string GetName() const = 0;
    };
} // namespace ORION
