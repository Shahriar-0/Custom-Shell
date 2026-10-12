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

// Runs an expanded command (program + final argument strings) and waits for
// it. Redirections travel on `cmd` for now; the executor applies them
// before calling.
//
// Returns the child's exit status. Conventions (bash's): 127 if the program
// could not be found, 126 if it was found but could not be started, and
// 128 + N if a POSIX child was killed by signal N.
int run(const parser::ExpandedCommand& cmd);

} // namespace executables

#endif
