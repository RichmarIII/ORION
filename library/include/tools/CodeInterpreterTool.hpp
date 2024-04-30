#pragma once

#include "IOrionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can run and interpret python code
    class CodeInterpreterTool final : public IOrionTool
    {
    public:
        inline EOrionToolType GetType() const override
        {
            return EOrionToolType::CodeInterpreter;
        }

        inline std::string ToJson() const override
        {
            return R"({"type":"code_interpreter"})";
        }

        inline std::string GetName() const override
        {
            return "code_interpreter";
        }
    };
} // namespace ORION
