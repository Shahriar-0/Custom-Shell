#ifndef UTILS_HPP
#define UTILS_HPP

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace utils {

// String manipulation
std::string trim(std::string_view str);
std::vector<std::string> split(std::string_view str, char delimiter);
std::string join(const std::vector<std::string>& strings, std::string_view delimiter);
std::string replace(std::string_view str, std::string_view from, std::string_view to);
std::string removeAll(std::string_view str, std::string_view token);
std::string removeQuotes(std::string_view str);

// Quote/escape-aware tokenizer for shell input. Not a general-purpose
// string splitter: encodes shell-specific grammar (matching quotes,
// backslash escapes, word boundaries) that no library gets right for us.
std::vector<std::string> tokenize(std::string_view str, char delimiter = ' ');

// Filesystem glob, for now it's hand-rolled, but could be replaced with
// a more robust implementation later.
std::vector<std::string> glob(const std::string& pattern);

bool fileExists(const std::filesystem::path& path);
bool isRelativePath(std::string_view path);
bool isAbsolutePath(std::string_view path);
bool isHomePath(std::string_view path);

// Expands a leading '~' to the user's home directory (USERPROFILE on
// Windows, HOME elsewhere). Non-home paths pass through unchanged.
std::string expandHome(std::string_view pattern);

} // namespace utils

#endif
