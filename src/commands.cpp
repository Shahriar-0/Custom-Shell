#include "commands.hpp"
#include <iostream>

namespace commands {

// Define the command map
std::unordered_map<std::string, CommandFunction> cmd = {
    { EXIT, &shellExit },
    { ECHO, &echo }
};

// Function implementations
int echo(const std::string& args) {
    std::cout << args << std::endl;
    return 0;
}

int shellExit(const std::string& args) {
    int status = 0;
    if (!args.empty()) {
        status = std::stoi(args);
    }
    std::exit(status);
    return 0;
}

} // namespace commands
