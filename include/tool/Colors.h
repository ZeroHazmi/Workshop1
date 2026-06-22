#pragma once
#include <string_view>
#include <array>

namespace tool::colors {
    // ANSI Escape Code Constants
    constexpr std::string_view RESET = "\033[0m";

    // Standard foreground colors
    constexpr std::string_view RED = "\033[31m";
    constexpr std::string_view GREEN = "\033[32m";
    constexpr std::string_view YELLOW = "\033[33m";
    constexpr std::string_view BLUE = "\033[34m";
    constexpr std::string_view MAGENTA = "\033[35m";
    constexpr std::string_view CYAN = "\033[36m";

    // Bright foreground colors
    constexpr std::string_view BRIGHT_RED = "\033[91m";
    constexpr std::string_view BRIGHT_GREEN = "\033[92m";
    constexpr std::string_view BRIGHT_YELLOW = "\033[93m";
    constexpr std::string_view BRIGHT_BLUE = "\033[94m";
    constexpr std::string_view BRIGHT_MAGENTA = "\033[95m";
    constexpr std::string_view BRIGHT_CYAN = "\033[96m";

    // Shared sequence of distinct colors for rendering analytical charts and graphs
    constexpr std::array<std::string_view, 12> GRAPH_PALETTE = {
        BRIGHT_CYAN,
        BRIGHT_MAGENTA,
        BRIGHT_YELLOW,
        BRIGHT_BLUE,
        BRIGHT_GREEN,
        BRIGHT_RED,
        CYAN,
        MAGENTA,
        YELLOW,
        BLUE,
        GREEN,
        RED
    };
}
