#pragma once

#include "IOrionTool.hpp"

#include <cpprest/json.h> // web::json::value

namespace ORION
{
    /// @brief  A tool capable of running a function (a plugin of sorts)
    class FunctionTool : public IOrionTool
    {
    public:
        /// @brief Construct a new Function Tool object
        /// @param function The json string representing the function
        /// @note The function must be a valid JSON string
        /// @see OrionFunctionStatics   For examples of valid functions
        inline FunctionTool(const std::string& FUNCTION) : m_Function(FUNCTION)
        {
        }

        inline EOrionToolType GetType() const override
        {
            return EOrionToolType::Function;
        }

        inline std::string ToJson() const override
        {
            return R"({"type":"function", "function":)" + U(m_Function) + "}";
        }

        inline std::string GetName() const override
        {
            return web::json::value::parse(U(m_Function)).at("name").as_string();
        }

        /// @brief  Execute the function
        /// @param  orion The Orion instance
        /// @param  Parameters The parameters to pass to the function
        /// @return The result of the function as a json string
        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) = 0;

    private:
        std::string m_Function;
    };
} // namespace ORION
