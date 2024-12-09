#ifndef UTILS_HPP
#define UTILS_HPP

#include <sstream>
#include <string>
#include <vector>

namespace utils {

// Helper functions for string manipulation
std::string trim(const std::string& str);
std::vector<std::string> split(const std::string& s, char delimiter);

}  // namespace utils

#endif
