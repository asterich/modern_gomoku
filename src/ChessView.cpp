module;

export module chess_view;

import std;

import point;
import strings;

namespace {
std::string cell_marker(int x, int y) {
    const bool left_edge = x == 1;
    const bool right_edge = x == board_rows;
    const bool bottom_edge = y == 1;
    const bool top_edge = y == board_cols;

    if ((left_edge || right_edge) && (bottom_edge || top_edge)) {
        if (left_edge && bottom_edge) {
            return "┗ ";
        }
        if (left_edge && top_edge) {
            return "┏ ";
        }
        if (right_edge && bottom_edge) {
            return "┛ ";
        }
        if (right_edge && top_edge) {
            return "┓ ";
        }
    }
    if (left_edge || right_edge) {
        if (left_edge) {
            return "┣ ";
        } else {
            return "┫ ";
        }
    }
    if (bottom_edge || top_edge) {
        if (bottom_edge) {
            return "┷ ";
        } else {
            return "┯ ";
        }
    }
    return "┼ ";
}
}  // namespace

export namespace chess_view {

void show_board(const int (&board)[16][16], point current) {
    for (int y : std::views::iota(1, board_cols + 1) | std::views::reverse) {
        std::cout << std::setw(2) << y;
        for (int x : std::views::iota(1, board_rows + 1)) {
            if (board[x][y] == 0) {
                std::cout << cell_marker(x, y);
            } else if (board[x][y] == white_piece) {
                const bool is_current = current.x == x && current.y == y;
                std::cout << (is_current ? "△ " : "○ ");
            } else if (board[x][y] == black_piece) {
                const bool is_current = current.x == x && current.y == y;
                std::cout << (is_current ? "▲ " : "● ");
            }
        }
        std::cout << '\n';
    }
    std::cout << " ";
    for (int x : std::views::iota(1, board_rows + 1)) {
        std::cout << std::setw(2) << static_cast<char>('a' + x - 1);
    }
    std::cout << '\n';
}

std::string marker_for_cell(int x, int y) {
    return cell_marker(x, y);
}

}  // namespace chess_view
