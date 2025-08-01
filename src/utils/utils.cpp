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

std::string removeQuotes(const std::string& str) {
    std::string res = str;
    if (res[0] == '"' || res[0] == '\'') {
        res = res.substr(1);
    }
    if (res[res.length() - 1] == '"' || res[res.length() - 1] == '\'') {
        res = res.substr(0, res.length() - 1);
    }
    return res;
}

std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
#ifdef _WIN32
    // Windows-specific implementation
    std::vector<std::string> result;
    std::string token;
    bool insideQuotes = false; // Track if we are inside quotes
    bool escaped = false;      // Track if the current character is escaped
    char quoteChar = '\0';     // Store which quote type we are handling

    for (size_t i = 0; i < str.size(); ++i) {
        char currentChar = str[i];

        // Handle escape character
        if (!insideQuotes && !escaped && currentChar == '\\') {
            escaped = true; // Enable escape mode
            continue;       // Skip adding the backslash to the token
        }

        // Handle quotes
        if (!escaped && (currentChar == '"' || currentChar == '\'')) {
            if (insideQuotes && currentChar == quoteChar) {
                insideQuotes = false; // Closing quote
            }
            else if (!insideQuotes) {
                insideQuotes = true; // Opening quote
                quoteChar = currentChar;
            }
            else {
                token += currentChar; // Add unmatched quote to the token
            }
        }

        // Handle delimiter
        else if (!escaped && !insideQuotes && str.substr(i, delimiter.size()) == delimiter) {
            if (!token.empty()) { // Push token only if it's non-empty
                result.push_back(token);
                token.clear();
            }
            i += delimiter.size() - 1; // Skip over the delimiter
        }

        // Handle regular characters
        else {
            token += currentChar;
        }

        // Reset escaped flag after processing the character
        escaped = false;
    }

    // Add the last token if non-empty
    if (!token.empty()) {
        result.push_back(token);
    }
    return result;

#else
    // Unix-specific implementation
    wordexp_t p;
    wordexp(str.c_str(), &p, 0);
    std::vector<std::string> result(p.we_wordv, p.we_wordv + p.we_wordc);
    wordfree(&p);
    return result;
#endif
}

std::vector<std::string> glob(const std::string& pattern) {
    std::vector<std::string> result;
    for (const auto& entry : std::filesystem::directory_iterator(pattern)) {
        result.push_back(entry.path().string());
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
    return path[0] == '/';
}

bool isHomePath(const std::string& path) {
    return path[0] == '~';
}

} // namespace utils