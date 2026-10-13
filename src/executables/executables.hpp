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

// Runs an expanded command and waits for it, applying cmd.redirections to
// the child. On Windows only fds 0-2 can be redirected.
//
// Returns the child's exit status, using bash's conventions: 1 if a
// redirection failed (the program is not run), 127 if the program could not
// be found, 126 if it was found but could not be started, 128 + N if a POSIX
// child was killed by signal N.
int run(const parser::ExpandedCommand& cmd);

} // namespace executables

#endif
