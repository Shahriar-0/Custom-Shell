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

}  // namespace executables