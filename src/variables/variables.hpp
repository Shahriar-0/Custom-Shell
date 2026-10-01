#ifndef VARIABLES_HPP
#define VARIABLES_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace variables {

extern std::vector<std::string> PATHs;
extern std::unordered_map<std::string, std::string> ENVs;
extern int lastExitStatus;

// Populates PATHs and ENVs from the current process environment.
void loadFromEnvironment();

} // namespace variables

#endif
