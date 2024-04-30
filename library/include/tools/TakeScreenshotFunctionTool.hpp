#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can take a screenshot of the desktop
    class TakeScreenshotFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that takes a screenshot of the desktop and returns a base64 encoded version of it
            constexpr static auto TAKE_SCREENSHOT = R"(
            {
                "description" : "Takes a screenshot of the desktop and returns a base64 encoded version of it",
                "name" : "take_screenshot",
                "parameters" : {}
            })";
        };

        inline TakeScreenshotFunctionTool()
            : FunctionTool(Statics::TAKE_SCREENSHOT)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
