#include "utils.hpp"

#include <algorithm>
#include <cstdlib>

namespace utils {

std::vector<std::string> split(std::string_view str, char delimiter) {
    std::vector<std::string> result;
    size_t start = 0;
    while (start <= str.size()) {
        size_t end = str.find(delimiter, start);
        if (end == std::string_view::npos) {
            end = str.size();
        }
        if (end > start) {
            result.emplace_back(str.substr(start, end - start));
        }
        start = end + 1;
    }
    return result;
}

std::string join(const std::vector<std::string>& strings, std::string_view delimiter) {
    std::string result;
    for (size_t i = 0; i < strings.size(); i++) {
        result += strings[i];
        if (i + 1 < strings.size()) {
            result += delimiter;
        }
    }
    return result;
}

// Expands a leading '~' to the user's home directory (USERPROFILE on
// Windows, HOME elsewhere). Non-home paths pass through unchanged.
std::string expandHome(std::string_view pattern) {
    if (!isHomePath(pattern)) {
        return std::string(pattern);
    }
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    if (home == nullptr) {
        return std::string(pattern);
    }
    return std::string(home) + std::string(pattern.substr(1));
}

bool fileExists(const std::filesystem::path& path) {
    return std::filesystem::exists(path);
}

bool isAbsolutePath(std::string_view path) {
    return !path.empty() && path.front() == '/';
}

bool isHomePath(std::string_view path) {
    return !path.empty() && path.front() == '~';
}

bool isRelativePath(std::string_view path) {
    return !isAbsolutePath(path) && !isHomePath(path);
}

} // namespace utils
