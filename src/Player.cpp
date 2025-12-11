module;

export module player;

import std;

import ai;
import chess_info;
import point;
import strings;
import tool;

export namespace player {

enum class piece_side { black, white };

[[nodiscard]] constexpr int piece_value_for_side(piece_side side) noexcept {
    return side == piece_side::black ? black_piece : white_piece;
}

class player_base {
public:
    player_base(piece_side side, bool is_human, std::string label)
        : side_{side}, piece_{piece_value_for_side(side)}, is_human_{is_human}, label_{std::move(label)} {}

    virtual ~player_base() = default;

    [[nodiscard]] piece_side side() const noexcept { return side_; }
    [[nodiscard]] int piece_value() const noexcept { return piece_; }
    [[nodiscard]] bool is_human() const noexcept { return is_human_; }
    [[nodiscard]] const std::string &label() const noexcept { return label_; }

    [[nodiscard]] virtual std::optional<point> next_move(const chess_info &state) = 0;

protected:
    piece_side side_;
    int piece_;
    bool is_human_;
    std::string label_;
};

class human_player final : public player_base {
public:
    human_player(piece_side side, std::string label)
        : player_base(side, true, std::move(label)) {}

    [[nodiscard]] std::optional<point> next_move(const chess_info &) override {
        while (true) {
            std::cout << label() << " move (e.g., h8 or q to quit): ";
            std::string input;
            if (!(std::cin >> input)) {
                std::cin.clear();
                return std::nullopt;
            }
            if (input == "q" || input == "Q") {
                return std::nullopt;
            }
            if (input.size() < 2U) {
                std::cout << "Invalid input format.\n";
                continue;
            }
            const char row_char = input.front();
            const std::string column_str = input.substr(1);
            point candidate{tool::parse_row(row_char), tool::parse_col(column_str)};
            if (!candidate.is_valid()) {
                std::cout << "Invalid coordinate.\n";
                continue;
            }
            return candidate;
        }
    }
};

class ai_player final : public player_base {
public:
    ai_player(piece_side side, std::string label, int depth)
        : player_base(side, false, std::move(label)), engine_(depth) {}

    [[nodiscard]] std::optional<point> next_move(const chess_info &state) override {
        if (state.round == 0) {
            return point{8, 8};
        }
        point choice = engine_.get_best_point(state);
        if (!choice.is_valid()) {
            return std::nullopt;
        }
        return choice;
    }

private:
    ai::engine engine_{};
};

}  // namespace player
