#ifndef UTILS_HPP
#define UTILS_HPP

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace utils {

// String manipulation
std::vector<std::string> split(std::string_view str, char delimiter);
std::string join(const std::vector<std::string>& strings, std::string_view delimiter);

bool fileExists(const std::filesystem::path& path);
bool isRelativePath(std::string_view path);
bool isAbsolutePath(std::string_view path);
bool isHomePath(std::string_view path);

// Expands a leading '~' to the user's home directory (USERPROFILE on
// Windows, HOME elsewhere). Non-home paths pass through unchanged.
std::string expandHome(std::string_view pattern);

} // namespace utils

#endif
