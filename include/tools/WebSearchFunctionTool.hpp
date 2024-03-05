#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can search the web for a query
    class WebSearchFunctionTool : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that searches the web for a query
            constexpr static const char* WEB_SEARCH = R"(
            {
                "description" : "Searches the web for a query",
                "name" : "web_search",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "query" : {
                            "type" : "string",
                            "description" : "The query to search for"
                        }
                    },
                    "required" : ["query"]
                }
            })";
        };

        inline WebSearchFunctionTool() : FunctionTool(Statics::WEB_SEARCH)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
