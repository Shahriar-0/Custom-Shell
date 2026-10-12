#include "executables.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <string>
#include <vector>

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

#ifdef _WIN32
// PATHEXT lists suffixes in resolution order (".COM;.EXE;.BAT;.CMD" by
// default). Windows stores it uppercase; we lowercase on the way in so
// resolved paths come out looking like "cmd.exe" instead of "cmd.EXE" —
// cosmetic only, since the filesystem itself is case-insensitive either way.
std::vector<std::string> pathExtensions() {
    auto toLower = [](std::string s) {
        for (char& c : s) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    };
    if (auto it = variables::ENVs.find("PATHEXT"); it != variables::ENVs.end() && !it->second.empty()) {
        std::vector<std::string> exts = utils::split(it->second, ';');
        for (auto& ext : exts) {
            ext = toLower(ext);
        }
        return exts;
    }
    return {".com", ".exe", ".bat", ".cmd"};
}
#endif

// All filenames worth trying for a given base path: the bare name, plus
// PATHEXT-suffixed variants on Windows. POSIX has no notion of an implicit
// executable extension, so it's just the base path itself.
std::vector<std::string> candidatePaths(const std::string& base) {
#ifdef _WIN32
    std::vector<std::string> candidates{base};
    for (const auto& ext : pathExtensions()) {
        candidates.push_back(base + ext);
    }
    return candidates;
#else
    return {base};
#endif
}

// Resolves a command name to a runnable path. Names containing a path
// separator are treated as direct paths (relative or absolute) and never
// searched on PATH; anything else is looked up in each PATH entry in order,
// first hit wins.
std::optional<std::string> getExecutablePath(const std::string& command) {
    auto tryDirect = [](const std::string& path) -> std::optional<std::string> {
        for (const auto& candidate : candidatePaths(path)) {
            if (isExecutable(candidate)) {
                return candidate;
            }
        }
        return std::nullopt;
    };

    if (command.find('/') != std::string::npos || command.find('\\') != std::string::npos) {
        // A literal '~' passes through unexpanded here — only cd and
        // friends expand home today; executables take the path as written.
        return tryDirect(command);
    }

    for (const auto& path : variables::PATHs) {
        std::string candidate = (std::filesystem::path(path) / command).string();
        if (auto hit = tryDirect(candidate); hit.has_value()) {
            return hit;
        }
    }
    return std::nullopt;
}

bool commandExists(const std::string& command) {
    return getExecutablePath(command).has_value();
}

#ifdef _WIN32

namespace {

// Quotes one argument for a Windows command line using the rules the MSVC /
// MinGW CRT uses to split it back apart: wrap in "..." when the argument is
// empty or contains whitespace or a quote; backslashes are literal unless
// they precede a quote (or the closing quote), in which case each one is
// doubled and any quote itself is escaped with one more backslash.
std::string quoteWindowsArg(const std::string& arg) {
    if (!arg.empty() && arg.find_first_of(" \t\n\v\"") == std::string::npos) {
        return arg;
    }
    std::string out = "\"";
    size_t backslashes = 0;
    for (char c : arg) {
        if (c == '\\') {
            ++backslashes;
            continue;
        }
        if (c == '"') {
            out.append(backslashes * 2 + 1, '\\');
            out.push_back('"');
        }
        else {
            out.append(backslashes, '\\');
            out.push_back(c);
        }
        backslashes = 0;
    }
    out.append(backslashes * 2, '\\'); // they now precede the closing quote
    out.push_back('"');
    return out;
}

} // namespace

// Windows has no fork/exec pair, so the whole command line is assembled as
// one string and handed to CreateProcessW. The program is the *resolved*
// path (so `foo` finds foo.cmd / foo.bat the same way `type` reports it,
// instead of CreateProcess's own .exe-only guessing), and every piece is
// quoted so the child's CRT splits the line back into exactly these args.
int run(const parser::ExpandedCommand& cmd) {
    auto resolved = getExecutablePath(cmd.program);
    if (!resolved.has_value()) {
        std::cerr << cmd.program << ": command not found\n";
        return 127;
    }
    std::string program = *resolved;
    std::replace(program.begin(), program.end(), '/', '\\');

    std::string cmdLineStr = quoteWindowsArg(program);
    for (const auto& arg : cmd.args) {
        cmdLineStr += ' ';
        cmdLineStr += quoteWindowsArg(arg);
    }

    int wlen = MultiByteToWideChar(CP_UTF8, 0, cmdLineStr.c_str(), -1, nullptr, 0);
    std::wstring wcmdLine(wlen > 0 ? wlen : 1, L'\0');
    if (wlen > 0) {
        MultiByteToWideChar(CP_UTF8, 0, cmdLineStr.c_str(), -1, wcmdLine.data(), wlen);
    }

    STARTUPINFOW si{};
    si.cb = sizeof(STARTUPINFOW);
    PROCESS_INFORMATION pi{};

    if (!CreateProcessW(nullptr, wcmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        std::cerr << std::format("{}: CreateProcess failed ({}).\n", cmd.program, GetLastError());
        return 126;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exitCode);
}

#else

// POSIX split: fork, then execvp in the child. The exec only returns if it
// failed; the child reports that and leaves with _exit (not exit: this is a
// forked copy of the shell, and exit() would run the parent's atexit
// handlers and static destructors a second time, flushing buffers twice).
// The parent waits and decodes the status the way bash does.
int run(const parser::ExpandedCommand& cmd) {
    std::vector<char*> cArgs;
    cArgs.push_back(const_cast<char*>(cmd.program.c_str()));
    for (const auto& arg : cmd.args) {
        cArgs.push_back(const_cast<char*>(arg.c_str()));
    }
    cArgs.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        execvp(cmd.program.c_str(), cArgs.data());
        int err = errno;
        std::cerr << std::format("{}: {}\n", cmd.program, strerror(err));
        _exit(err == ENOENT ? 127 : 126);
    }
    if (pid > 0) {
        int status = 0;
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                std::cerr << std::format("waitpid failed: {}\n", strerror(errno));
                return 1;
            }
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);
        }
        return 1;
    }
    std::cerr << std::format("fork failed: {}\n", strerror(errno));
    return 126;
}
#endif

} // namespace executables
