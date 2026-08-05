#include "utils.hpp"

namespace utils {

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) {
        return ""; // all whitespace or empty
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

std::string join(const std::vector<std::string>& strings, char delimiter) {
    std::string result;
    for (size_t i = 0; i < strings.size(); i++) {
        result += strings[i];
        if (i + 1 < strings.size()) {
            result += delimiter;
        }
    }
    return result;
}

std::string escape(const std::string& str) {
    (void)str;
    return "";
}

std::string unescape(const std::string& str) {
    (void)str;
    return "";
}

std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return str; // avoid infinite loop on empty 'from'
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

std::string remove(const std::string& str, const std::string& token) {
    return replace(str, token, "");
}

std::string removeQuotes(const std::string& str) {
    if (str.empty()) {
        return str;
    }
    std::string res = str;
    char front = res.front();
    if (res.size() >= 2 && (front == '"' || front == '\'') && res.back() == front) {
        res = res.substr(1, res.size() - 2); // only strip if quotes match
    }
    return res;
}

// Quote-aware, escape-aware tokenizer. No shell expansion (no globbing,
// no command substitution, no env expansion) — that belongs in the parser layer later.
std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> result;
    std::string token;
    bool insideQuotes = false;
    bool escaped = false;
    char quoteChar = '\0';

    for (size_t i = 0; i < str.size(); ++i) {
        char currentChar = str[i];

        if (!insideQuotes && !escaped && currentChar == '\\') {
            escaped = true;
            continue;
        }

        if (!escaped && (currentChar == '"' || currentChar == '\'')) {
            if (insideQuotes && currentChar == quoteChar) {
                insideQuotes = false;
            }
            else if (!insideQuotes) {
                insideQuotes = true;
                quoteChar = currentChar;
            }
            else {
                token += currentChar;
            }
        }
        else if (!escaped && !insideQuotes && str.compare(i, delimiter.size(), delimiter) == 0) {
            if (!token.empty()) {
                result.push_back(token);
                token.clear();
            }
            i += delimiter.size() - 1;
        }
        else {
            token += currentChar;
        }

        escaped = false;
    }

    if (!token.empty()) {
        result.push_back(token);
    }
    return result;
}

// Minimal literal glob: '*' matches any suffix within one directory level.
// Not a real glob engine (no '?', no '[]', no recursive '**') — placeholder until parser work.
std::vector<std::string> glob(const std::string& pattern) {
    std::vector<std::string> result;
    std::filesystem::path patternPath(pattern);
    std::filesystem::path dir = patternPath.has_parent_path() ? patternPath.parent_path() : ".";
    std::string filePattern = patternPath.filename().string();

    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        return result;
    }

    size_t starPos = filePattern.find('*');
    std::string prefix = starPos == std::string::npos ? filePattern : filePattern.substr(0, starPos);
    std::string suffix = starPos == std::string::npos ? "" : filePattern.substr(starPos + 1);

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        std::string name = entry.path().filename().string();
        if (name.size() >= prefix.size() + suffix.size() &&
            name.compare(0, prefix.size(), prefix) == 0 &&
            name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) {
            result.push_back(entry.path().string());
        }
    }
    return result;
}

bool fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool isRelativePath(const std::string& path) {
    return !isAbsolutePath(path) && !isHomePath(path);
}

bool isAbsolutePath(const std::string& path) {
    return !path.empty() && path.front() == '/';
}

bool isHomePath(const std::string& path) {
    return !path.empty() && path.front() == '~';
}

} // namespace utils