#include "executables.hpp"

namespace executables {

bool isExecutable(const std::string& path) {
    return utils::fileExists(path) &&
           ((std::filesystem::status(path).permissions() & std::filesystem::perms::owner_exec) !=
            std::filesystem::perms::none);
}

bool isExecutable(const std::filesystem::path& path) { return isExecutable(path.string()); }

bool commandExists(const std::string& command) {
    if (variables::PATHs.size() > 0) {
        std::string command_path;
        for (const auto& path : variables::PATHs) {
            command_path = path + "/" + command;
            if (isExecutable(command_path)) {
                return true;
            }
        }
    }
    return false;
}

std::optional<std::string> getExecutablePath(const std::string& command) {
    if (variables::PATHs.empty()) {
        return std::nullopt;
    }
    for (const auto& path : variables::PATHs) {
        std::string command_path = path + "/" + command;
        if (isExecutable(command_path)) {
            return command_path;
        }
    }
    return std::nullopt;
}

int run(const std::string& command, const std::string& args) {
    // This is another solution which is not right but it is working
    // std::string runningCommand = command + " " + args;
    // return std::system(runningCommand.c_str());

#ifdef _WIN32
    STARTUPINFOW si = {0};
    si.cb = sizeof(STARTUPINFOW);
    PROCESS_INFORMATION pi;

    // Convert command and args to a single wstring
    std::string cmdLineStr = command + " " + args;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, cmdLineStr.c_str(), -1, nullptr, 0);
    std::wstring wcmdLine(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, cmdLineStr.c_str(), -1, &wcmdLine[0], wlen);

    if (!CreateProcessW(nullptr,      // Application name
                        &wcmdLine[0], // Command line
                        nullptr,      // Process attributes
                        nullptr,      // Thread attributes
                        FALSE,        // Inherit handles
                        0,            // Creation flags
                        nullptr,      // Environment
                        nullptr,      // Current directory
                        &si,          // Startup info
                        &pi)) {       // Process info
        std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
        return -1;
    }

    // Wait for the child process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;

#else
    std::vector<std::string> arguments = utils::split(args, variables::COMMAND_DELIMITER);

    std::vector<char*> cArgs;
    cArgs.push_back(const_cast<char*>(command.c_str()));
    for (const std::string& arg : arguments) {
        cArgs.push_back(const_cast<char*>(arg.c_str()));
    }
    cArgs.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execvp(command.c_str(), cArgs.data());
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
    else {
        // Fork failed
        perror("fork failed");
        return -1;
    }

    return 0;
#endif
}

} // namespace executables