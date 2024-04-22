#pragma once

#include "IOrionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can access and analyze data uploaded to the server
    class RetrievalTool final : public IOrionTool
    {
    public:
        inline EOrionToolType GetType() const override
        {
            return EOrionToolType::Retrieval;
        }

        inline std::string ToJson() const override
        {
            return R"({"type":"retrieval"})";
        }

        inline std::string GetName() const override
        {
            return "retrieval";
        }
    };
} // namespace ORION
