#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include "parser/ast.hpp"

namespace executor {

// Executes a parsed pipeline line. Returns the exit status of the last
// command run (bash convention) and updates variables::lastExitStatus.
//
// Phase 1 scope:
//   - single-command nodes: builtins and externals, fully working
//   - multi-command pipelines: reported as not-yet-implemented (status 2)
//   - && / || / ; short-circuit logic: fully working
//   - redirections & backgrounding: attached by the parser, reported as
//     not-yet-implemented (status 2)
int execute(const parser::Pipeline& pipeline);

} // namespace executor

#endif // EXECUTOR_HPP
