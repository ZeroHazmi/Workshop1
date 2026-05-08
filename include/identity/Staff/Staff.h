#pragma once
#include <string>
#include <string_view>
#include <expected>

namespace identity::staff {
    // Create a staff profile for a newly registered user
    std::expected<int, std::string> createStaffProfile(
        int userId,
        int shopId,
        std::string_view staffName,
        std::string_view position,
        std::string_view phone
    );
}
