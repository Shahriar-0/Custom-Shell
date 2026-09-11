#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include "parser/ast.hpp"

namespace executor {

// Executes a parsed command line. Returns the exit status of the last
// pipeline actually run (bash convention: pipelines skipped by && / ||
// short-circuiting don't update the status) and updates
// variables::lastExitStatus to match.
//
// Phase 1 scope:
//   - single-command pipelines: builtins and externals, fully working
//   - multi-stage pipelines (cmd1 | cmd2): reported as not-yet-implemented
//   - ; / && / || short-circuit logic: fully working
//   - redirections & backgrounding: parsed and attached, but reported as
//     not-yet-implemented rather than silently ignored
int execute(const parser::CommandLine& line);

} // namespace executor

#endif // EXECUTOR_HPP
