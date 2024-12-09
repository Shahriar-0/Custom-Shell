#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <string>
#include <unordered_map>
#include <functional>

namespace commands {

// Command names
constexpr char EXIT[] = "exit";
constexpr char ECHO[] = "echo";

// Function type for commands
using CommandFunction = std::function<int(const std::string&)>;

// A map of command names to their implementations (declared, not defined here)
extern std::unordered_map<std::string, CommandFunction> cmd;

// Function declarations
int echo(const std::string& args);
int shellExit(const std::string& args);

} // namespace commands

#endif // COMMANDS_HPP
