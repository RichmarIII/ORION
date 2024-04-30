#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can search the web for a query
    class WebSearchFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that searches the web for a query
            constexpr static auto WEB_SEARCH = R"(
            {
                "description" : "Searches the web for a query. Needs to be run for up to date information not available to the ai. return json object that describes the search results and next possible actions. final_action contains the action to perform when information is ready to be displayed to the user. next_action contains the suggested next action to perform if user question is not answered adequately",
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

        inline WebSearchFunctionTool()
            : FunctionTool(Statics::WEB_SEARCH)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
