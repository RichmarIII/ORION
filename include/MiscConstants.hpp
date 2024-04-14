#pragma once

#include <string_view>

namespace ORION
{
    struct MiscConstants final
    {

        /**
         * @brief The name used for indexing a json object containing the instructions for Orion to follow after execution of a function tool
         */
        static constexpr std::string_view NAME_ORION_INSTRUCTIONS = "orion_instructions";
    };
}
