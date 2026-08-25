#ifndef VARIABLES_HPP
#define VARIABLES_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace variables {

extern std::vector<std::string> PATHs;
extern std::unordered_map<std::string, std::string> ENVs;
extern int lastExitStatus;

constexpr char HOME_DIR[] = "~";
constexpr char CURRENT_DIR[] = ".";
constexpr char PARENT_DIR[] = "..";
constexpr char COMMAND_DELIMITER = ' ';

// Populates PATHs and ENVs from the current process environment.
void loadFromEnvironment();

} // namespace variables

#endif
