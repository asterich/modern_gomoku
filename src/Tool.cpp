module;

export module tool;

import std;

export namespace tool {

int parse_row(char c) {
    if (c == '\0') {
        return -1;
    }
    if (std::isalpha(static_cast<unsigned char>(c))) {
        return std::toupper(static_cast<unsigned char>(c)) - 'A' + 1;
    }
    return -1;
}

int parse_col(const std::string &s) {
    if (s.empty()) {
        return -1;
    }
    int num = 0;
    for (char ch : s) {
        if (ch >= '0' && ch <= '9') {
            num = num * 10 + (ch - '0');
        } else {
            return -1;
        }
    }
    return (num >= 1 && num <= 15) ? num : -1;
}

}  // namespace tool
