#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can search the web for a query
    class NavigateLinkFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief // A function that navigates to a link. return json object with conent of the link.
            constexpr static auto NAVIGATE_LINK = R"(
            {
                "description" : "Navigates to a link. return json object that describes the content of the link next possible actions. final_action contains the action to perform when information is ready to be displayed to the user. next_action contains the suggested next action to perform if user question is not answered adequately",
                "name" : "navigate_link",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "link" : {
                            "type" : "string",
                            "description" : "The link to navigate to"
                        },
                        "user_query" : {
                            "type" : "string",
                            "description" : "The user query that led to this link. This is used to help the ai understand the context of the link. can be empty if not available."
                        }
                    },
                    "required" : ["link", "user_query"]
                }
            })";
        };

        inline
        NavigateLinkFunctionTool()
            : FunctionTool(Statics::NAVIGATE_LINK)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
