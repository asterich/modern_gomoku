module;
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <shared_mutex>
#include <optional>
#include <random>

export module ai;

import std;

import chess_info;
import pattern;
import point;
import rule;
import strings;

namespace {


constexpr int search_infinity = 0x0f3f3f3f;

constexpr std::array<pattern_entry, 18> score_table_black{ {
    { "11111", 50000 },
    { "011110", 50000 },
    { "011100", 1440 },
    { "001110", 1440 },
    { "011010", 1440 },
    { "010110", 1440 },
    { "11110", 7200 },
    { "01111", 7200 },
    { "11011", 3600 },
    { "10111", 3600 },
    { "11101", 3600 },
    { "01112" , 720 },
    { "21110" , 720 },
    { "001100", 120 },
    { "001010", 120 },
    { "010100", 120 },
    { "000100", 20 },
    { "001000", 20 }
} };

constexpr std::array<pattern_entry, 18> score_table_white{ {
    { "22222", 50000 },
    { "022220", 50000 },
    { "022200", 1440 },
    { "002220", 1440 },
    { "022020", 1440 },
    { "020220", 1440 },
    { "22220", 7200 },
    { "02222", 7200 },
    { "22022", 3600 },
    { "20222", 3600 },
    { "22202", 3600 },
    { "02221" , 720 },
    { "12220", 720 },
    { "002200", 120 },
    { "002020", 120 },
    { "020200", 120 },
    { "000200", 20 },
    { "002000", 20 }
} };

constexpr std::array<point, 4> evaluation_directions{
    point{-1, 0}, point{0, -1}, point{-1, -1}, point{-1, 1}
};

[[nodiscard]] std::shared_ptr<spdlog::logger> ai_logger() {
    static std::shared_ptr<spdlog::logger> logger = [] {
        if (auto existing = spdlog::get("ai_logger")) {
            return existing;
        }
        try {
            auto created = spdlog::basic_logger_mt("ai_logger", "ai.log", true);
            created->set_level(spdlog::level::info);
            created->flush_on(spdlog::level::info);
            return created;
        } catch (const spdlog::spdlog_ex &ex) {
            spdlog::default_logger()->error("Failed to create ai logger: {}", ex.what());
            return spdlog::default_logger();
        }
    }();
    return logger;
}

uint64_t zobrist_table[16][16][2];
bool zobrist_initialized = false;

void init_zobrist() {
    if (zobrist_initialized) return;
    std::mt19937_64 rng(12345);
    for (int i : std::views::iota(1, 16)) {
        for (int j : std::views::iota(1, 16)) {
            zobrist_table[i][j][0] = rng();
            zobrist_table[i][j][1] = rng();
        }
    }
    zobrist_initialized = true;
}

int evaluate_line(char* line, size_t len, const std::array<pattern_entry, 18>& table) {
    int score = 0;
    for (const auto& entry : table) {
        char* it = line;
        char* end = line + len;
        while ((it = std::search(it, end, entry.s.begin(), entry.s.end())) != end) {
            score += entry.score;
            std::fill(it, it + entry.s.length(), '3');
        }
    }
    return score;
}

char to_char(int i) {
    return static_cast<char>(i + '0');
}

}  // namespace

export namespace ai {

class engine {
public:
    engine(int depth = 3) : max_depth(depth) {}

    [[nodiscard]] point get_best_point(chess_info state) {
        if (state.round == 0) return {8, 8};

        init_zobrist();
        uint64_t current_hash = 0;
        for (int i : std::views::iota(1, 16)) {
            for (int j : std::views::iota(1, 16)) {
                if (state.pieces[i][j] == 1) current_hash ^= zobrist_table[i][j][0];
                else if (state.pieces[i][j] == 2) current_hash ^= zobrist_table[i][j][1];
            }
        }

        bool maximizing = (state.turn == 0); 
        point best_move{-1, -1};
        int best_val = maximizing ? -search_infinity : search_infinity;
        
        auto moves = get_moves(state);
        if (moves.empty()) return {8, 8};

        std::vector<point> winning_candidates;

        // check for immediate win (five or live four)
        for (const auto& move : moves) {
            chess_info next_state = state;
            int piece = (state.turn == 0) ? 1 : 2;
            next_state.pieces[move.x][move.y] = piece;

            if (state.turn == 0) {
                if (!rule::is_win(next_state.pieces, move)) {
                    if (rule::is_double_three(next_state.pieces, move) ||
                        rule::is_double_four(next_state.pieces, move) ||
                        rule::is_long_chain(next_state.pieces, move, 1)) {
                        continue;
                    }
                }
            }

            if (rule::is_win(next_state.pieces, move)) {
                return move; // win
            }

            int score = evaluate(next_state);
            if (state.turn == 0) { 
                if (score >= 40000) winning_candidates.push_back(move);
            } else { 
                if (score <= -40000) winning_candidates.push_back(move);
            }
        }

        // immediate loss block
        for (const auto& move : moves) {
            chess_info next_state = state;
            int piece = (state.turn == 0) ? 2 : 1; 
            next_state.pieces[move.x][move.y] = piece;
            
            if (state.turn == 1) { 
                 if (!rule::is_win(next_state.pieces, move)) {
                    if (rule::is_double_three(next_state.pieces, move) ||
                        rule::is_double_four(next_state.pieces, move) ||
                        rule::is_long_chain(next_state.pieces, move, 1)) {
                        continue; 
                    }
                }
            }

            if (rule::is_win(next_state.pieces, move)) {
                return move; // block it
            }
        }

        // prioritize winning candidates
        if (!winning_candidates.empty()) {
            moves = winning_candidates;
        }

        std::vector<int> scores(moves.size());
        unsigned int n_threads = std::thread::hardware_concurrency();
        if (n_threads == 0) n_threads = 1;
        exec::static_thread_pool pool(n_threads);
        auto scheduler = pool.get_scheduler();

        auto bulk_sender = stdexec::just()
            | stdexec::continues_on(scheduler)
            | stdexec::bulk(moves.size(), [&](size_t i) {
                const auto& move = moves[i];
                chess_info next_state = state;
                int piece = (state.turn == 0) ? 1 : 2;
                next_state.pieces[move.x][move.y] = piece;

                if (state.turn == 0) {
                    if (!rule::is_win(next_state.pieces, move)) {
                        if (rule::is_double_three(next_state.pieces, move) ||
                            rule::is_double_four(next_state.pieces, move) ||
                            rule::is_long_chain(next_state.pieces, move, 1)) {
                            scores[i] = maximizing ? -search_infinity : search_infinity;
                            return;
                        }
                    }
                }

                next_state.turn = 1 - state.turn;
                next_state.round++;
                next_state.current_point = move;

                uint64_t next_hash = current_hash ^ zobrist_table[move.x][move.y][piece == 1 ? 0 : 1];
                scores[i] = minimax(next_state, max_depth, -search_infinity, search_infinity, !maximizing, next_hash);
            });

        stdexec::sync_wait(std::move(bulk_sender));

        for (auto [val, move] : std::views::zip(scores, moves)) {
            if (maximizing) {
                if (val > best_val) {
                    best_val = val;
                    best_move = move;
                }
                if (val >= 40000) break; 
            } else {
                if (val < best_val) {
                    best_val = val;
                    best_move = move;
                }
                if (val <= -40000) break;
            }
        }
        return best_move;
    }

private:
    int max_depth;

    struct TTEntry {
        int value;
        int flag;
        int depth;
        point best_move;
    };

    std::array<std::array<int, 16>, 16> history_table{};

    template<typename Key, typename Value>
    class ShardedMap {
        static constexpr size_t num_shards = 64;
        struct Shard {
            std::shared_mutex mutex;
            std::unordered_map<Key, Value> map;
        };
        std::array<Shard, num_shards> shards;
        
        Shard& get_shard(const Key& key) {
            return shards[key % num_shards];
        }

    public:
        std::optional<Value> find(const Key& key) {
            auto& shard = get_shard(key);
            std::shared_lock lock(shard.mutex);
            auto it = shard.map.find(key);
            if (it != shard.map.end()) return it->second;
            return std::nullopt;
        }
        
        void insert(const Key& key, const Value& value) {
            auto& shard = get_shard(key);
            std::unique_lock lock(shard.mutex);
            shard.map[key] = value;
        }
        
        void clear() {
            for (auto& shard : shards) {
                std::unique_lock lock(shard.mutex);
                shard.map.clear();
            }
        }
    };

    ShardedMap<uint64_t, TTEntry> trans_table;

    int minimax(chess_info& board, int depth, int alpha, int beta, bool maximizing, uint64_t hash) {
        int alpha_orig = alpha;
        int beta_orig = beta;

        point hash_move = {-1, -1};
        if (auto entry = trans_table.find(hash)) {
            if (entry->depth >= depth) {
                if (entry->flag == 0) return entry->value;
                if (entry->flag == 1) alpha = std::max(alpha, entry->value);
                else if (entry->flag == 2) beta = std::min(beta, entry->value);
                if (alpha >= beta) return entry->value;
            }
            hash_move = entry->best_move;
        }

        int score = evaluate(board);
        
        if (score >= 40000) return score - (max_depth + 1 - depth); 
        if (score <= -40000) return score + (max_depth + 1 - depth); 

        if (depth == 0) {
            return score;
        }

        auto moves = get_moves(board);
        if (moves.empty()) return score;

        // Move Ordering
        std::sort(moves.begin(), moves.end(), [&](const point& a, const point& b) {
            int score_a = (a.x == hash_move.x && a.y == hash_move.y) ? 10000000 : history_table[a.x][a.y];
            int score_b = (b.x == hash_move.x && b.y == hash_move.y) ? 10000000 : history_table[b.x][b.y];
            return score_a > score_b;
        });

        int val = 0;
        point best_move_this_node = {-1, -1};

        if (maximizing) {
            int max_eval = -search_infinity;
            for (const auto& move : moves) {
                chess_info next_state = board;
                next_state.pieces[move.x][move.y] = 1; 

                if (!rule::is_win(next_state.pieces, move)) {
                    if (rule::is_double_three(next_state.pieces, move) ||
                        rule::is_double_four(next_state.pieces, move) ||
                        rule::is_long_chain(next_state.pieces, move, 1)) {
                        continue;
                    }
                }

                next_state.turn = 1; 
                next_state.round++;
                next_state.current_point = move;

                uint64_t next_hash = hash ^ zobrist_table[move.x][move.y][0];
                int eval = minimax(next_state, depth - 1, alpha, beta, false, next_hash);
                
                if (eval > max_eval) {
                    max_eval = eval;
                    best_move_this_node = move;
                }
                alpha = std::max(alpha, eval);
                if (beta <= alpha) {
                    history_table[move.x][move.y] += depth * depth;
                    break;
                }
                if (max_eval >= 40000) break;
            }
            val = max_eval;
        } else {
            int min_eval = search_infinity;
            for (const auto& move : moves) {
                chess_info next_state = board;
                next_state.pieces[move.x][move.y] = 2; 
                next_state.turn = 0; 
                next_state.round++;
                next_state.current_point = move;

                uint64_t next_hash = hash ^ zobrist_table[move.x][move.y][1];
                int eval = minimax(next_state, depth - 1, alpha, beta, true, next_hash);
                
                if (eval < min_eval) {
                    min_eval = eval;
                    best_move_this_node = move;
                }
                beta = std::min(beta, eval);
                if (beta <= alpha) {
                    history_table[move.x][move.y] += depth * depth;
                    break;
                }
                if (min_eval <= -40000) break;
            }
            val = min_eval;
        }

        TTEntry entry;
        entry.value = val;
        entry.depth = depth;
        entry.best_move = best_move_this_node;
        if (val <= alpha_orig) entry.flag = 2; // Upper bound
        else if (val >= beta_orig) entry.flag = 1; // Lower bound
        else entry.flag = 0; // Exact
        trans_table.insert(hash, entry);

        return val;
    }

    int evaluate(const chess_info& board) {
        int total_score = 0;
        char buffer[16];

        // Horizontal
        for (int i : std::views::iota(1, 16)) {
            int len = 0;
            for (int j : std::views::iota(1, 16)) buffer[len++] = to_char(board.pieces[i][j]);
            total_score += evaluate_line(buffer, len, score_table_black);
            
            len = 0;
            for (int j : std::views::iota(1, 16)) buffer[len++] = to_char(board.pieces[i][j]);
            total_score -= evaluate_line(buffer, len, score_table_white);
        }
        // Vertical
        for (int j : std::views::iota(1, 16)) {
            int len = 0;
            for (int i : std::views::iota(1, 16)) buffer[len++] = to_char(board.pieces[i][j]);
            total_score += evaluate_line(buffer, len, score_table_black);

            len = 0;
            for (int i : std::views::iota(1, 16)) buffer[len++] = to_char(board.pieces[i][j]);
            total_score -= evaluate_line(buffer, len, score_table_white);
        }
        // Diagonal (1,1)
        for (int k = 0; k < 15; ++k) {
            int len = 0;
            for (int x = 1 + k, y = 1; x <= 15 && y <= 15; ++x, ++y) buffer[len++] = to_char(board.pieces[x][y]);
            total_score += evaluate_line(buffer, len, score_table_black);

            len = 0;
            for (int x = 1 + k, y = 1; x <= 15 && y <= 15; ++x, ++y) buffer[len++] = to_char(board.pieces[x][y]);
            total_score -= evaluate_line(buffer, len, score_table_white);
        }
        for (int k = 1; k < 15; ++k) {
            int len = 0;
            for (int x = 1, y = 1 + k; x <= 15 && y <= 15; ++x, ++y) buffer[len++] = to_char(board.pieces[x][y]);
            total_score += evaluate_line(buffer, len, score_table_black);

            len = 0;
            for (int x = 1, y = 1 + k; x <= 15 && y <= 15; ++x, ++y) buffer[len++] = to_char(board.pieces[x][y]);
            total_score -= evaluate_line(buffer, len, score_table_white);
        }
        // Anti-diagonal (1, -1)
        for (int k = 0; k < 15; ++k) {
            int len = 0;
            for (int x = 1, y = 1 + k; x <= 15 && y >= 1; ++x, --y) buffer[len++] = to_char(board.pieces[x][y]);
            total_score += evaluate_line(buffer, len, score_table_black);

            len = 0;
            for (int x = 1, y = 1 + k; x <= 15 && y >= 1; ++x, --y) buffer[len++] = to_char(board.pieces[x][y]);
            total_score -= evaluate_line(buffer, len, score_table_white);
        }
        for (int k = 1; k < 15; ++k) {
            int len = 0;
            for (int x = 1 + k, y = 15; x <= 15 && y >= 1; ++x, --y) buffer[len++] = to_char(board.pieces[x][y]);
            total_score += evaluate_line(buffer, len, score_table_black);

            len = 0;
            for (int x = 1 + k, y = 15; x <= 15 && y >= 1; ++x, --y) buffer[len++] = to_char(board.pieces[x][y]);
            total_score -= evaluate_line(buffer, len, score_table_white);
        }
        return total_score;
    }

    std::vector<point> get_moves(const chess_info& board) {
        std::vector<point> moves;
        bool has_pieces = false;
        for (int i : std::views::iota(1, 16)) {
            for (int j : std::views::iota(1, 16)) {
                if (board.pieces[i][j] != 0) {
                    has_pieces = true;
                    break;
                }
            }
            if (has_pieces) break;
        }
        
        if (!has_pieces) {
            moves.push_back({8, 8});
            return moves;
        }

        for (int i : std::views::iota(1, 16)) {
            for (int j : std::views::iota(1, 16)) {
                if (board.pieces[i][j] == 0) {
                    bool neighbor = false;
                    for (int dx : std::views::iota(-2, 3)) {
                        for (int dy : std::views::iota(-2, 3)) {
                            int nx = i + dx;
                            int ny = j + dy;
                            if (nx >= 1 && nx <= 15 && ny >= 1 && ny <= 15) {
                                if (board.pieces[nx][ny] != 0) {
                                    neighbor = true;
                                    break;
                                }
                            }
                        }
                        if (neighbor) break;
                    }
                    if (neighbor) moves.push_back({i, j});
                }
            }
        }
        return moves;
    }
};

}  // namespace ai

