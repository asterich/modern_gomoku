module;

export module game_controller;

import std;

import chess_info;
import chess_view;
import player;
import point;
import rule;
import strings;

using namespace std::chrono_literals;

namespace {

void clear_screen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

void pause_with_message(std::string_view message) {
    std::cout << message;
    std::cout.flush();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

void render_board(const std::vector<std::string> &header_lines, const chess_info &state) {
    clear_screen();
    for (const auto &line : header_lines) {
        std::cout << line << '\n';
    }
    std::cout << '\n';
    chess_view::show_board(state.pieces, state.current_point);
    std::cout << '\n';
}

std::string describe_piece(int piece_value) {
    return piece_value == black_piece ? "black" : "white";
}

std::string format_seconds(double seconds) {
    return std::format("{:.3f}", seconds);
}

}  // namespace

export namespace game {

struct game_options {
    std::vector<std::string> header_lines;
    bool enforce_center = true;
    bool show_ai_thinking = true;
};

void run_game(const game_options &options,
              player::player_base &black_player,
              player::player_base &white_player) {
    chess_info state;
    int move_count = 0;

    while (true) {
        render_board(options.header_lines, state);
        auto &current_player = (state.turn == black_turn) ? black_player : white_player;

        const auto move_start = std::chrono::steady_clock::now();
        auto move_opt = current_player.next_move(state);
        const auto move_end = std::chrono::steady_clock::now();
        const auto move_duration_s = std::chrono::duration<double>(move_end - move_start).count();

        if (!move_opt.has_value()) {
            std::cout << current_player.label() << " spent " << format_seconds(move_duration_s) << " s before quitting." << '\n';
            std::cout << current_player.label() << " quits the game." << '\n';
            pause_with_message("\nPlease press Enter to quit...");
            return;
        }

        point move = *move_opt;
        if (!move.is_valid()) {
            pause_with_message("\nInvalid coordinate. Press Enter to continue...");
            continue;
        }
        if (!move.is_empty(state.pieces)) {
            pause_with_message("\nThe chosen cell is not empty. Press Enter to continue...");
            continue;
        }
        if (options.enforce_center && move_count == 0 && current_player.piece_value() == black_piece && (move.x != 8 || move.y != 8)) {
            pause_with_message("\nThe first step of black piece needs to be in H8.\nPress Enter to continue...");
            continue;
        }

        state.pieces[move.x][move.y] = current_player.piece_value();
        state.current_point = move;
        ++move_count;

        render_board(options.header_lines, state);
        pause_with_message(std::format("{} spent {} s on the move. Press Enter to continue...", current_player.label(), format_seconds(move_duration_s)));

        const bool won = rule::is_win(state.pieces, move);
        const bool long_chain = rule::is_long_chain(state.pieces, move, current_player.piece_value());
        bool banned = false;
        std::string banned_message;

        if (current_player.piece_value() == black_piece) {
            if (rule::is_double_three(state.pieces, move)) {
                banned = true;
                banned_message = "三三禁手, 白棋赢!";
            } else if (rule::is_double_four(state.pieces, move)) {
                banned = true;
                banned_message = "四四禁手, 白棋赢!";
            } else if (long_chain) {
                banned = true;
                banned_message = "长链, 白棋赢!";
            }
        }

        if (won || (current_player.piece_value() == white_piece && long_chain)) {
            render_board(options.header_lines, state);
            std::cout << describe_piece(current_player.piece_value()) << " piece win!" << '\n';
            pause_with_message("\nPlease press Enter to quit...");
            return;
        }

        if (banned) {
            render_board(options.header_lines, state);
            std::cout << banned_message << '\n';
            pause_with_message("\nPlease press Enter to quit...");
            return;
        }

        if (move_count == board_rows * board_cols) {
            render_board(options.header_lines, state);
            std::cout << "和棋" << '\n';
            pause_with_message("\nPlease press Enter to quit...");
            return;
        }

        state.turn ^= 1;
        state.round += 1;

        if (options.show_ai_thinking) {
            auto &next_player = (state.turn == black_turn) ? black_player : white_player;
            if (!next_player.is_human()) {
                std::cout << "Waiting AI..." << '\n';
                std::cout.flush();
                std::this_thread::sleep_for(300ms);
            }
        }
    }
}

}  // namespace game
