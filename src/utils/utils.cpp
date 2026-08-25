#include "utils.hpp"

#include <algorithm>
#include <cstdlib>

namespace utils {

std::string trim(std::string_view str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string_view::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(' ');
    return std::string(str.substr(first, last - first + 1));
}

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

std::string replace(std::string_view str, std::string_view from, std::string_view to) {
    if (from.empty()) {
        return std::string(str);
    }
    std::string result;
    size_t pos = 0;
    while (true) {
        size_t next = str.find(from, pos);
        if (next == std::string_view::npos) {
            result += str.substr(pos);
            break;
        }
        result += str.substr(pos, next - pos);
        result += to;
        pos = next + from.size();
    }
    return result;
}

std::string removeAll(std::string_view str, std::string_view token) {
    return replace(str, token, "");
}

std::string removeQuotes(std::string_view str) {
    if (str.size() >= 2) {
        char front = str.front();
        if ((front == '"' || front == '\'') && str.back() == front) {
            return std::string(str.substr(1, str.size() - 2));
        }
    }
    return std::string(str);
}

std::vector<std::string> tokenize(std::string_view str, char delimiter) {
    std::vector<std::string> result;
    std::string token;

    bool insideQuotes = false;
    bool escaped = false;
    char quoteChar = '\0';

    for (char c : str) {
        // Previous character was an escape character:
        // take this character literally.
        if (escaped) {
            token += c;
            escaped = false;
            continue;
        }

        // Start an escape sequence.
        if (!insideQuotes && c == '\\') {
            escaped = true;
            continue;
        }

        // Handle quotes.
        if (c == '"' || c == '\'') {
            if (insideQuotes) {
                // Closing quote.
                if (c == quoteChar) {
                    insideQuotes = false;
                } else {
                    token += c;
                }
            } else {
                insideQuotes = true;
                quoteChar = c;
            }
            continue;
        }

        // Delimiter outside quotes ends the token.
        if (!insideQuotes && c == delimiter) {
            if (!token.empty()) {
                result.push_back(std::move(token));
                token.clear();
            }
            continue;
        }

        token += c;
    }

    if (!token.empty()) {
        result.push_back(std::move(token));
    }

    return result;
}

namespace {

// Portable '*'/'?' wildcard match (no POSIX fnmatch dependency).
// '*' matches any run of characters, '?' matches exactly one.
bool matchesWildcard(std::string_view name, std::string_view pattern) {
    size_t n = 0, p = 0;
    size_t starIdx = std::string_view::npos, matchIdx = 0;

    while (n < name.size()) {
        // 1. Normal character or ?
        if (p < pattern.size() &&
            (pattern[p] == '?' || pattern[p] == name[n])) {
            ++n;
            ++p;
        }
        // 2. *
        else if (p < pattern.size() && pattern[p] == '*') {
            starIdx = p;
            matchIdx = n;
            ++p;
        }
        // 3. Mismatch, but we saw a previous *
        else if (starIdx != std::string_view::npos) {
            p = starIdx + 1;
            matchIdx += 1;
            n = matchIdx;
        }
        // 4. Nothing can save us
        else {
            return false;
        }
    }
    // Ignore trailing *'s
    while (p < pattern.size() && pattern[p] == '*') {
        ++p;
    }

    return p == pattern.size();
}

} // namespace

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

std::vector<std::string> glob(const std::string& pattern) {
    std::vector<std::string> result;

    std::filesystem::path patternPath(expandHome(pattern));
    std::filesystem::path dir = patternPath.has_parent_path() ? patternPath.parent_path() : ".";
    std::string filePattern = patternPath.filename().string();

    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        return result;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        std::string name = entry.path().filename().string();
        if (matchesWildcard(name, filePattern)) {
            result.push_back(entry.path().string());
        }
    }
    return result;
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