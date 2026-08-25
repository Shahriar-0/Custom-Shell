#include "executables.hpp"

#include <cerrno>
#include <cstring>
#include <format>
#include <iostream>

#include "utils/utils.hpp"
#include "variables/variables.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace executables {

bool isExecutable(const std::filesystem::path& path) {
    if (!utils::fileExists(path)) {
        return false;
    }
    auto status = std::filesystem::status(path);
    return std::filesystem::is_regular_file(status) &&
           (status.permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::none;
}

std::optional<std::string> getExecutablePath(const std::string& command) {
    if (utils::isAbsolutePath(command) || utils::isRelativePath(command)) {
        if (command.find('/') != std::string::npos && isExecutable(command)) {
            return command;
        }
    }
    for (const auto& path : variables::PATHs) {
        std::filesystem::path candidate = std::filesystem::path(path) / command;
        if (isExecutable(candidate)) {
            return candidate.string();
        }
    }
    return std::nullopt;
}

bool commandExists(const std::string& command) {
    return getExecutablePath(command).has_value();
}

#ifdef _WIN32

int run(const std::string& command, const std::vector<std::string>& args) {
    STARTUPINFOW si{};
    si.cb = sizeof(STARTUPINFOW);
    PROCESS_INFORMATION pi{};

    std::string cmdLineStr = command;
    for (const auto& arg : args) {
        cmdLineStr += " \"" + arg + "\"";
    }

    int wlen = MultiByteToWideChar(CP_UTF8, 0, cmdLineStr.c_str(), -1, nullptr, 0);
    std::wstring wcmdLine(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, cmdLineStr.c_str(), -1, wcmdLine.data(), wlen);

    if (!CreateProcessW(nullptr, wcmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        std::cerr << std::format("CreateProcess failed ({}).\n", GetLastError());
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exitCode);
}

#else

int run(const std::string& command, const std::vector<std::string>& args) {
    std::vector<char*> cArgs;
    cArgs.push_back(const_cast<char*>(command.c_str()));
    for (const auto& arg : args) {
        cArgs.push_back(const_cast<char*>(arg.c_str()));
    }
    cArgs.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        execvp(command.c_str(), cArgs.data());
        std::cerr << std::format("{}: {}\n", command, strerror(errno));
        std::exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        int status = 0;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
    std::cerr << std::format("fork failed: {}\n", strerror(errno));
    return -1;
}

#endif

} // namespace executables
