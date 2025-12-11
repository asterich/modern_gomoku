module;

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

export module rule;

import std;

import point;
import strings;

namespace {

constexpr std::array<int, 4> dx_offsets{ -1, 0, -1, -1 };
constexpr std::array<int, 4> dy_offsets{ 0, -1, -1, 1 };

int count_in_direction(const int (&board)[16][16], point origin, point delta, int color) {
    origin += delta;
    int count = 0;
    while (origin.is_valid()) {
        if (board[origin.x][origin.y] == color) {
            ++count;
        } else {
            break;
        }
        origin += delta;
    }
    return count;
}

}  // namespace

export namespace rule {

[[nodiscard]] std::shared_ptr<spdlog::logger> rule_logger() {
    static std::shared_ptr<spdlog::logger> logger = [] {
        if (auto existing = spdlog::get("rule_logger")) {
            return existing;
        }
        try {
            auto created = spdlog::basic_logger_mt("rule_logger", "rule.log", true);
            created->set_level(spdlog::level::info);
            created->flush_on(spdlog::level::info);
            return created;
        } catch (const spdlog::spdlog_ex &ex) {
            spdlog::default_logger()->error("Failed to create rule logger: {}", ex.what());
            return spdlog::default_logger();
        }
    }();
    return logger;
}

std::string collect_sequence(const int (&board)[16][16], point origin, point delta, int steps) {
    std::string sequence;
    sequence.reserve(steps);
    for (int index : std::views::iota(0, steps)) {
        origin += delta;
        if (!origin.is_valid()) {
            return "N";
        }
        sequence.push_back(static_cast<char>('0' + board[origin.x][origin.y]));
    }
    return sequence;
}

bool is_win(const int (&board)[16][16], point origin) {
    const int color = board[origin.x][origin.y];
    if (count_in_direction(board, origin, point{-1, 0}, color) + count_in_direction(board, origin, point{1, 0}, color) + 1 == 5) {
        return true;
    }
    if (count_in_direction(board, origin, point{0, -1}, color) + count_in_direction(board, origin, point{0, 1}, color) + 1 == 5) {
        return true;
    }
    if (count_in_direction(board, origin, point{-1, -1}, color) + count_in_direction(board, origin, point{1, 1}, color) + 1 == 5) {
        return true;
    }
    if (count_in_direction(board, origin, point{-1, 1}, color) + count_in_direction(board, origin, point{1, -1}, color) + 1 == 5) {
        return true;
    }
    return false;
}

bool is_double_three(const int (&board)[16][16], point origin/*, const int color*/) {
    static const std::array<std::string, 3> triple_patterns{
        "01110",
        "010110",
        "011010"
    };

    int matches = 0;
    for (auto [dx, dy] : std::views::zip(dx_offsets, dy_offsets)) {
        bool found = false;
        for (const auto &pattern : triple_patterns) {
            for (std::size_t idx : std::views::iota(0u, pattern.size())) {
                if (pattern[idx] != '1') {
                    continue;
                }
                std::string before = collect_sequence(board, origin, point{dx, dy}, static_cast<int>(idx));
                std::reverse(before.begin(), before.end());
                std::string after = collect_sequence(board, origin, point{-dx, -dy}, static_cast<int>(pattern.size() - idx - 1));
                if (before != "N" && after != "N" && before + "1" + after == pattern) {
                    ++matches;
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
    }
    if (matches >= 2) {
        rule_logger()->info("double_three at ({}, {})", origin.x, origin.y);
        return true;
    }
    return false;
}

bool is_double_four(const int (&board)[16][16], point origin/*, const int color*/) {
    static const std::array<std::string, 6> quadruple_patterns{
        "011110",
        "11110",
        "01111",
        "11011",
        "10111",
        "11101"
    };

    int matches = 0;
    for (auto [dx, dy] : std::views::zip(dx_offsets, dy_offsets)) {
        bool found = false;
        for (const auto &pattern : quadruple_patterns) {
            for (std::size_t idx : std::views::iota(0u, pattern.size())) {
                if (pattern[idx] != '1') {
                    continue;
                }
                std::string before = collect_sequence(board, origin, point{dx, dy}, static_cast<int>(idx));
                std::reverse(before.begin(), before.end());
                std::string after = collect_sequence(board, origin, point{-dx, -dy}, static_cast<int>(pattern.size() - idx - 1));
                if (before != "N" && after != "N" && before + "1" + after == pattern) {
                    ++matches;
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
    }
    if (matches >= 2) {
        rule_logger()->info("double_four at ({}, {})", origin.x, origin.y);
        return true;
    }
    return false;
}

bool is_long_chain(const int (&board)[16][16], point origin, const int color) {
    if (count_in_direction(board, origin, point{-1, 0}, color) + count_in_direction(board, origin, point{1, 0}, color) + 1 > 5) {
        rule_logger()->info("long_chain at ({}, {}), color = {}", origin.x, origin.y, color);
        return true;
    }
    if (count_in_direction(board, origin, point{0, -1}, color) + count_in_direction(board, origin, point{0, 1}, color) + 1 > 5) {
        rule_logger()->info("long_chain at ({}, {}), color = {}", origin.x, origin.y, color);
        return true;
    }
    if (count_in_direction(board, origin, point{-1, -1}, color) + count_in_direction(board, origin, point{1, 1}, color) + 1 > 5) {
        rule_logger()->info("long_chain at ({}, {}), color = {}", origin.x, origin.y, color);
        return true;
    }
    if (count_in_direction(board, origin, point{-1, 1}, color) + count_in_direction(board, origin, point{1, -1}, color) + 1 > 5) {
        rule_logger()->info("long_chain at ({}, {}), color = {}", origin.x, origin.y, color);
        return true;
    }
    return false;
}

}  // namespace rule
