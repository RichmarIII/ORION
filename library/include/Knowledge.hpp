#pragma once

#include <cstdint>
#include <string>

namespace ORION
{
    /// @brief  The type of knowledge
    enum class EKnowledgeType : std::uint8_t
    {
        UserPersonalInfo,
        UserInterests,
        FamilyPersonalInfo,
        FamilyInterests,
        UserPreferences,
        Unknown
    };

    struct KnowledgeFileNameStatics
    {
        constexpr static std::string_view USER_PERSONAL_INFO   = "user_personal_info.json";
        constexpr static std::string_view USER_INTERESTS       = "user_interests.json";
        constexpr static std::string_view FAMILY_PERSONAL_INFO = "family_personal_info.json";
        constexpr static std::string_view FAMILY_INTERESTS     = "family_interests.json";
        constexpr static std::string_view USER_PREFERENCES     = "user_preferences.json";
        constexpr static std::string_view UNKNOWN              = "unknown.json";
    };

} // namespace ORION