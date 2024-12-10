#ifndef EXECUTABLES_HPP
#define EXECUTABLES_HPP

#include <filesystem>
#include <string>
#include <optional>

#include "utils.hpp"
#include "variables.hpp"

namespace executables {

bool commandExists(const std::string& command);
bool isExecutable(const std::filesystem::path& path);
bool isExecutable(const std::string& path);

std::optional<std::string> getExecutablePath(const std::string& command);

}  // namespace executables

#endif