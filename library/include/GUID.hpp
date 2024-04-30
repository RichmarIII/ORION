#pragma once
#include <string>

namespace ORION
{
    /// @brief The GUID class.
    struct GUID final
    {
        /// @brief The GUID of the object. Should be set on instantiation.
        /*const*/ std::string GUIDString;

        static struct GUID Generate();

        explicit inline operator std::string() const
        {
            return GUIDString;
        }

        explicit inline operator bool() const
        {
            return !GUIDString.empty();
        }
    };
} // namespace ORION