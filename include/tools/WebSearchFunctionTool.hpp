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
            constexpr static const char* WebSearch = R"(
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

        inline WebSearchFunctionTool() : FunctionTool(Statics::WebSearch)
        {
        }

        virtual std::string Execute(class Orion& orion, const web::json::value& parameters) override;
    };
} // namespace ORION
