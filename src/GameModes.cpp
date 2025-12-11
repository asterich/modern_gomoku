module;

export module game_modes;

import std;

import ai;
import game_controller;
import player;

namespace {

enum class difficulty {
    easy,
    medium,
    hard
};

[[nodiscard]] constexpr int difficulty_to_depth(difficulty d) {
    switch (d) {
        case difficulty::easy: return 2;
        case difficulty::medium: return 4;
        case difficulty::hard: return 6;
    }
    return 6;
}

std::optional<bool> prompt_is_human(std::string_view prompt) {
    while (true) {
        std::cout << prompt;
        std::string input;
        if (!(std::cin >> input)) {
            std::cin.clear();
            return std::nullopt;
        }
        if (input == "q" || input == "Q") {
            return std::nullopt;
        }
        if (input == "1") {
            return true;
        }
        if (input == "2") {
            return false;
        }
        std::cout << "Invalid choice. Please enter 1 (Human) or 2 (AI).\n";
    }
}

std::optional<difficulty> prompt_difficulty(std::string_view prompt) {
    while (true) {
        std::cout << prompt << " (1: Easy, 2: Medium, 3: Hard): ";
        std::string input;
        if (!(std::cin >> input)) {
            std::cin.clear();
            return std::nullopt;
        }
        if (input == "q" || input == "Q") {
            return std::nullopt;
        }
        if (input == "1") return difficulty::easy;
        if (input == "2") return difficulty::medium;
        if (input == "3") return difficulty::hard;
        std::cout << "Invalid choice. Please enter 1, 2, or 3.\n";
    }
}

std::unique_ptr<player::player_base> create_player(bool is_human,
                                               player::piece_side side,
                                               std::string_view role,
                                               std::optional<difficulty> diff = std::nullopt) {
    std::string label = std::string(role) + (is_human ? " (Human)" : " (AI)");
    if (is_human) {
        return std::make_unique<player::human_player>(side, std::move(label));
    }
    int depth = difficulty_to_depth(diff.value_or(difficulty::medium));
    return std::make_unique<player::ai_player>(side, std::move(label), depth);
}

constexpr std::string_view border_line = " -------------------------------";
constexpr std::size_t inner_width = border_line.size() - 2;

std::string make_center_line(std::string_view content) {
    std::string inner(inner_width, ' ');
    const auto to_copy = std::min(content.size(), inner_width);
    const auto start = (inner_width - to_copy) / 2;
    std::copy_n(content.begin(), to_copy, inner.begin() + static_cast<std::ptrdiff_t>(start));
    std::string line;
    line.reserve(border_line.size());
    line.push_back('|');
    line.append(inner);
    line.push_back('|');
    return line;
}

std::string make_left_line(std::string_view prefix, std::string_view value) {
    std::string content = std::string(prefix) + std::string(value);
    if (content.size() > inner_width) {
        content.resize(inner_width);
    }
    std::string line;
    line.reserve(border_line.size());
    line.push_back('|');
    line.append(content);
    if (content.size() < inner_width) {
        line.append(inner_width - content.size(), ' ');
    }
    line.push_back('|');
    return line;
}

using namespace std::string_view_literals;

std::vector<std::string> build_header(std::string_view mode_title,
                                      const player::player_base &black,
                                      const player::player_base &white) {
    const auto controller_label = [](const player::player_base &p) {
        return p.is_human() ? "Human"sv : "AI"sv;
    };

    std::vector<std::string> header;
    header.reserve(7);
    header.emplace_back(border_line);
    header.emplace_back(make_center_line(mode_title));
    header.emplace_back(make_left_line(" Black controller: ", controller_label(black)));
    header.emplace_back(make_left_line(" White controller: ", controller_label(white)));
    header.emplace_back(make_left_line(" Input example: ", "H8 or h8"));
    header.emplace_back(make_left_line(" Enter q to quit", ""));
    header.emplace_back(border_line);
    return header;
}

bool configure_players(std::string_view mode_name,
                       std::unique_ptr<player::player_base> &black_player,
                       std::unique_ptr<player::player_base> &white_player) {
    std::cout << "Configure controllers for " << mode_name << " (1: Human, 2: AI, q: back).\n";

    auto black_choice = prompt_is_human("Select controller for Black: ");
    if (!black_choice.has_value()) {
        return false;
    }
    std::optional<difficulty> black_diff;
    if (!*black_choice) {
        black_diff = prompt_difficulty("Select difficulty for Black AI");
        if (!black_diff.has_value()) return false;
    }

    auto white_choice = prompt_is_human("Select controller for White: ");
    if (!white_choice.has_value()) {
        return false;
    }
    std::optional<difficulty> white_diff;
    if (!*white_choice) {
        white_diff = prompt_difficulty("Select difficulty for White AI");
        if (!white_diff.has_value()) return false;
    }

    black_player = create_player(*black_choice, player::piece_side::black, "Black", black_diff);
    white_player = create_player(*white_choice, player::piece_side::white, "White", white_diff);
    return true;
}

}  // namespace

export namespace game_modes {

void run_pvp() {
    std::unique_ptr<player::player_base> black_player;
    std::unique_ptr<player::player_base> white_player;
    if (!configure_players("PVP", black_player, white_player)) {
        return;
    }

    auto header = build_header("PVP", *black_player, *white_player);
    game::game_options options{
        .header_lines = header,
        .enforce_center = true,
        .show_ai_thinking = true
    };

    game::run_game(options, *black_player, *white_player);
}

void run_pvm() {
    std::unique_ptr<player::player_base> black_player;
    std::unique_ptr<player::player_base> white_player;
    if (!configure_players("PVM", black_player, white_player)) {
        return;
    }

    auto header = build_header("PVM", *black_player, *white_player);
    game::game_options options{
        .header_lines = header,
        .enforce_center = true,
        .show_ai_thinking = true
    };

    game::run_game(options, *black_player, *white_player);
}

}  // namespace game_modes
