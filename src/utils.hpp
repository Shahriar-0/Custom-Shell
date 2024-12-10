#ifndef UTILS_HPP
#define UTILS_HPP

#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

namespace utils {

// Helper functions for string manipulation
std::string trim(const std::string& str);
std::vector<std::string> split(const std::string& s, char delimiter);
std::string join(const std::vector<std::string>& strings, char delimiter);
std::string escape(const std::string& str);
std::string unescape(const std::string& str);
std::string replace(const std::string& str, const std::string& from, const std::string& to);
std::string remove(const std::string& str, const std::string& remove);

// Helper functions for command execution
bool fileExists(const std::string& path);

// Helper function for changing directory
bool isRelativePath(const std::string& path);
bool isAbsolutePath(const std::string& path);
bool isHomePath(const std::string& path);

}  // namespace utils

#endif
