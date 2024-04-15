#pragma once

#include <string>

namespace ORION
{
    /// @brief The User class. Represents a logged in user.
    struct User final
    {
        /// @brief The ID of the user stored in the database. Should be set on instantiation. `User user{SOME_ID, SOME_ORION_ID}`;
        /*const*/ std::string UserID;

        /// @brief The id of the orion instance the user is logged into. Should be set on instantiation. `User user{SOME_ID, SOME_ORION_ID}`;
        /*const*/ std::string OrionID;

        explicit
        operator bool() const
        {
            return !UserID.empty() && !OrionID.empty();
        }
    };
} // namespace ORION