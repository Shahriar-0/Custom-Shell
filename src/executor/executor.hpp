#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include "parser/ast.hpp"

namespace executor {

// Executes a parsed command line, one AND-OR list at a time. Returns the exit
// status of the last pipeline actually run (bash convention: pipelines
// skipped by && / || short-circuiting don't update the status) and keeps
// variables::lastExitStatus up to date after every pipeline, so it always
// holds the status of the most recent command — across lines too.
//
// Each Command is turned into an ExpandedCommand (plain strings) right
// before it runs.
int execute(const parser::CommandLine& line);

} // namespace executor

#endif // EXECUTOR_HPP
