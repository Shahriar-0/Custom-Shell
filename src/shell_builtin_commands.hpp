#ifndef SHELL_BUILTIN_COMMANDS_HPP
#define SHELL_BUILTIN_COMMANDS_HPP

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace shell_builtin_commands {

// Command names
constexpr char EXIT[] = "exit";
constexpr char ECHO[] = "echo";
constexpr char HELP[] = "help";
constexpr char CLEAR[] = "clear";
constexpr char TYPE[] = "type";

// Function type for commands
using CommandFunction = std::function<int(const std::string&)>;

// A map of command names to their implementations (declared, not defined here)
extern std::unordered_map<std::string, CommandFunction> shell_builtin_cmds;

// Function declarations
int echo(const std::string& args);
int shellExit(const std::string& args);
int help(const std::string& args);
int clear(const std::string& args);
int type(const std::string& args);

}  // namespace shell_builtin_commands

#endif  // COMMANDS_HPP
