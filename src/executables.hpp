#ifndef EXECUTABLES_HPP
#define EXECUTABLES_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif


#include "utils.hpp"
#include "variables.hpp"

namespace executables {

// Search for an executable
bool commandExists(const std::string& command);
bool isExecutable(const std::filesystem::path& path);
bool isExecutable(const std::string& path);

std::optional<std::string> getExecutablePath(const std::string& command);

// Run a command
int run(const std::string& command, const std::string& args);
}  // namespace executables

#endif