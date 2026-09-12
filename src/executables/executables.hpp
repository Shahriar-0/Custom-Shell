#ifndef EXECUTABLES_HPP
#define EXECUTABLES_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "parser/ast.hpp"

namespace executables {

bool isExecutable(const std::filesystem::path& path);
bool commandExists(const std::string& command);
std::optional<std::string> getExecutablePath(const std::string& command);

// Runs a parsed command (program + already-tokenized args). Redirections
// travel on `cmd` for Phase 2; the executor applies them before calling.
// Returns the child's exit status, or -1 if the process could not start.
int run(const parser::Command& cmd);

} // namespace executables

#endif
