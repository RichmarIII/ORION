#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can get the weather for a location
    class GetWeatherFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that gets the weather for a location
            constexpr static auto GET_WEATHER = R"(
            {
                "description" : "Gets the weather for a location",
                "name" : "get_weather",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "location" : {
                            "type" : "string",
                            "description" : "The location to get the weather for, for example: Manchester, NH, USA."
                        },
                        "unit" : {
                            "type" : "string",
                            "enum" : [ "metric", "imperial" ],
                            "description" : "The unit to get the weather in. Can be one of the following: metric, imperial"
                        }
                    },
                    "required" : ["location"]
                }
            })";
        };

        inline GetWeatherFunctionTool()
            : FunctionTool(Statics::GET_WEATHER)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
