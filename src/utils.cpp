#include "utils.hpp"

namespace utils {

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }
    return result;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

std::string join(const std::vector<std::string>& strings, char delimiter) {
    std::string result;
    for (size_t i = 0; i < strings.size(); i++) {
        result += strings[i];
        if (i < strings.size() - 1) {
            result += delimiter;
        }
    }
    return result;
}

std::string escape(const std::string& str) {
    // TODO
    // Suppress warnings about unused variables and return
    (void)str;
    return "";
}

std::string unescape(const std::string& str) {
    // TODO
    // Suppress warnings about unused variables and return
    (void)str;
    return "";
}

std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

std::string remove(const std::string& str, const std::string& remove) {
    return replace(str, remove, "");
}

}  // namespace utils