#ifndef BUILTIN_COMMANDS_HPP
#define BUILTIN_COMMANDS_HPP

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "parser/ast.hpp"

namespace shell_builtin_commands {

inline constexpr std::string_view EXIT = "exit";
inline constexpr std::string_view ECHO = "echo";
inline constexpr std::string_view HELP = "help";
inline constexpr std::string_view CLEAR = "clear";
inline constexpr std::string_view TYPE = "type";
inline constexpr std::string_view PWD = "pwd";
inline constexpr std::string_view CD = "cd";

// Every builtin receives the full parsed command: argv-style words in
// `cmd.args`, plus `cmd.redirections` for Phase 2 (applied by the executor,
// not here — builtins just read args and return a status).
using CommandFunction = std::function<int(const parser::Command&)>;

extern const std::unordered_map<std::string, CommandFunction> shell_builtin_cmds;

bool shellBuiltinCommandExists(const std::string& command);

int echo(const parser::Command& cmd);
int shellExit(const parser::Command& cmd);
int help(const parser::Command& cmd);
int clear(const parser::Command& cmd);
int type(const parser::Command& cmd);
int pwd(const parser::Command& cmd);
int cd(const parser::Command& cmd);

} // namespace shell_builtin_commands

#endif
