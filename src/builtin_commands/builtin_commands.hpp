#ifndef BUILTIN_COMMANDS_HPP
#define BUILTIN_COMMANDS_HPP

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace shell_builtin_commands {

inline constexpr std::string_view EXIT = "exit";
inline constexpr std::string_view ECHO = "echo";
inline constexpr std::string_view HELP = "help";
inline constexpr std::string_view CLEAR = "clear";
inline constexpr std::string_view TYPE = "type";
inline constexpr std::string_view PWD = "pwd";
inline constexpr std::string_view CD = "cd";

using CommandFunction = std::function<int(const std::vector<std::string>&)>;

extern const std::unordered_map<std::string, CommandFunction> shell_builtin_cmds;

bool shellBuiltinCommandExists(const std::string& command);

int echo(const std::vector<std::string>& args);
int shellExit(const std::vector<std::string>& args);
int help(const std::vector<std::string>& args);
int clear(const std::vector<std::string>& args);
int type(const std::vector<std::string>& args);
int pwd(const std::vector<std::string>& args);
int cd(const std::vector<std::string>& args);

} // namespace shell_builtin_commands

#endif
