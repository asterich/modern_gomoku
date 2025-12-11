module;

#include <format>

export module point;

import strings;

export struct point {
    int x{0};
    int y{0};

    constexpr point() = default;
    constexpr point(int x_pos, int y_pos) noexcept : x{x_pos}, y{y_pos} {}

    constexpr point &operator+=(const point &other) noexcept {
        x += other.x;
        y += other.y;
        return *this;
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return x >= 1 && x <= board_rows && y >= 1 && y <= board_cols;
    }

    [[nodiscard]] constexpr bool is_empty(const int (&piece)[16][16]) const noexcept {
        return piece[x][y] == 0;
    }

    [[nodiscard]] constexpr bool is_black(const int (&piece)[16][16]) const noexcept {
        return piece[x][y] == 1;
    }

    [[nodiscard]] constexpr bool is_white(const int (&piece)[16][16]) const noexcept {
        return piece[x][y] == 2;
    }

    [[nodiscard]] constexpr bool operator<(const point &other) const noexcept {
        if (x == other.x) {
            return y < other.y;
        }
        return x < other.x;
    }
};

export template <>
struct std::formatter<point> {
    constexpr auto parse(std::format_parse_context &context) {
        return context.begin();
    }

    template <typename FormatContext>
    auto format(const point &value, FormatContext &context) const {
        return ::std::format_to(context.out(), "(row={}, col={})", value.x, value.y);
    }
};
