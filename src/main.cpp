#include <iostream>
#include <string>

#include "builtin_commands/builtin_commands.hpp"
#include "executables/executables.hpp"
#include "utils/utils.hpp"
#include "variables/variables.hpp"

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    variables::loadFromEnvironment();

    while (true) {
        std::cout << "$ ";

        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\n";
            break; // EOF (Ctrl+D)
        }

        std::vector<std::string> tokens = utils::tokenize(input);
        if (tokens.empty()) {
            continue;
        }

        const std::string& command = tokens[0];
        std::vector<std::string> args(tokens.begin() + 1, tokens.end());

        if (auto it = shell_builtin_commands::shell_builtin_cmds.find(command);
            it != shell_builtin_commands::shell_builtin_cmds.end()) {
            variables::lastExitStatus = it->second(args);
        }
        else if (executables::commandExists(command)) {
            variables::lastExitStatus = executables::run(command, args);
        }
        else {
            std::cerr << command << ": command not found\n";
            variables::lastExitStatus = 127;
        }
    }

    return variables::lastExitStatus;
}
