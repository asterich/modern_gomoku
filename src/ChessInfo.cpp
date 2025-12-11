module;

export module chess_info;

import point;
import strings;

export struct chess_info {
    chess_info();

    int pieces[16][16];
    int turn;
    int round{0};
    point current_point{-1, -1};
};

chess_info::chess_info() : pieces{}, turn{black_turn} {
    for (auto &row : pieces) {
        for (int &cell : row) {
            cell = 0;
        }
    }
}
