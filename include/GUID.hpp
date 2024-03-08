#pragma once
#include <string>

namespace ORION
{
    /// @brief The GUID class.
    struct GUID
    {
        /// @brief The GUID of the object. Should be set on instantiation.
        /*const*/ std::string GUID;

        static struct GUID Generate();

        operator std::string() const
        {
            return GUID;
        }

        inline operator bool() const
        {
            return !GUID.empty();
        }
    };
} // namespace ORION