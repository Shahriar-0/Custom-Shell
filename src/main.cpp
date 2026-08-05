#include <iostream>
#include <string>

#include "builtin_commands/builtin_commands.hpp"
#include "utils/utils.hpp"
#include "variables/variables.hpp"

int main() {
    // Flush after every std::cout / std:cerr
    // So that the output is immediately visible
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

#ifdef _WIN32
    const char delimiter = ';';
#else
    const char delimiter = ':';
#endif

    const char* env = std::getenv("PATH");
    if (env != nullptr) {
        variables::PATHs = utils::split(env, delimiter);
    }

    while (true) {
        std::cout << "$ ";

        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << std::endl; // EOF (Ctrl+D) — exit cleanly instead of looping forever
            break;
        }

        size_t spacePos = input.find(' ');
        std::string command = (spacePos == std::string::npos) ? input : input.substr(0, spacePos);
        std::string args = (spacePos == std::string::npos) ? "" : input.substr(spacePos + 1);

        command = utils::trim(command);
        if (command.empty()) {
            continue;
        }

        if (shell_builtin_commands::shell_builtin_cmds.find(command) !=
            shell_builtin_commands::shell_builtin_cmds.end()) {
            shell_builtin_commands::shell_builtin_cmds[command](args);
        }
        else if (executables::commandExists(command)) {
            executables::run(command, args);
        }
        else {
            std::cerr << input << ": command not found" << std::endl;
        }
    }
}
