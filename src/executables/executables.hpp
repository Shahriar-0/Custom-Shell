#ifndef EXECUTABLES_HPP
#define EXECUTABLES_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace executables {

bool isExecutable(const std::filesystem::path& path);
bool commandExists(const std::string& command);
std::optional<std::string> getExecutablePath(const std::string& command);

// Runs `command` with `args` (already tokenized). Returns the child's exit
// status, or -1 if the process could not be started.
int run(const std::string& command, const std::vector<std::string>& args);

} // namespace executables

#endif
