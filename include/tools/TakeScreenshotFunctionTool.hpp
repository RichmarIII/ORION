#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can take a screenshot of the desktop
    class TakeScreenshotFunctionTool : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that takes a screenshot of the desktop and returns a base64 encoded version of it
            constexpr static const char* TakeScreenshot = R"(
            {
                "description" : "Takes a screenshot of the desktop and returns a base64 encoded version of it",
                "name" : "take_screenshot",
                "parameters" : {}
            })";
        };

        inline TakeScreenshotFunctionTool() : FunctionTool(Statics::TakeScreenshot)
        {
        }

        virtual std::string Execute(class Orion& orion, const web::json::value& parameters) override;
    };
} // namespace ORION
