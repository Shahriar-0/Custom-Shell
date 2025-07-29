#ifndef VARIABLES_HPP
#define VARIABLES_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace variables {

// Global variables (declare as extern)
extern std::vector<std::string> PATHs;
extern std::unordered_map<std::string, std::string> ENVs;

// Constants
constexpr char HOME_DIR[] = "~";
constexpr char CURRENT_DIR[] = ".";
constexpr char PARENT_DIR[] = "..";
constexpr char COMMAND_DELIMITER[] = " ";

} // namespace variables

#endif
