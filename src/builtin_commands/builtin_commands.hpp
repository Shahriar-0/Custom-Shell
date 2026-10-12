#ifndef BUILTIN_COMMANDS_HPP
#define BUILTIN_COMMANDS_HPP

#include <functional>
#include <string>
#include <string_view>
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

// Every builtin receives the expanded command: argv-style strings in
// `cmd.args`.
using CommandFunction = std::function<int(const parser::ExpandedCommand&)>;

extern const std::unordered_map<std::string, CommandFunction> shell_builtin_cmds;

bool shellBuiltinCommandExists(const std::string& command);

int echo(const parser::ExpandedCommand& cmd);
int shellExit(const parser::ExpandedCommand& cmd);
int help(const parser::ExpandedCommand& cmd);
int clear(const parser::ExpandedCommand& cmd);
int type(const parser::ExpandedCommand& cmd);
int pwd(const parser::ExpandedCommand& cmd);
int cd(const parser::ExpandedCommand& cmd);

} // namespace shell_builtin_commands

#endif
