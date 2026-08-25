#include "variables.hpp"

#include <cstdlib>

#include "utils/utils.hpp"

#ifdef _WIN32
#include <windows.h>
#else
extern char** environ;
#endif

namespace variables {

std::vector<std::string> PATHs;
std::unordered_map<std::string, std::string> ENVs;
int lastExitStatus = 0;

namespace {

void insertPair(std::string_view pair) {
    size_t eq = pair.find('=');
    if (eq == std::string_view::npos || eq == 0) {
        return; // skip malformed entries and Windows' "=C:=..." per-drive vars
    }
    ENVs.emplace(std::string(pair.substr(0, eq)), std::string(pair.substr(eq + 1)));
}

} // namespace

void loadFromEnvironment() {
#ifdef _WIN32
    LPWCH envBlock = GetEnvironmentStringsW();
    if (envBlock != nullptr) {
        for (LPWCH entry = envBlock; *entry != L'\0'; entry += wcslen(entry) + 1) {
            int len = WideCharToMultiByte(CP_UTF8, 0, entry, -1, nullptr, 0, nullptr, nullptr);
            std::string narrow(len > 0 ? len - 1 : 0, '\0');
            if (len > 0) {
                WideCharToMultiByte(CP_UTF8, 0, entry, -1, narrow.data(), len, nullptr, nullptr);
            }
            insertPair(narrow);
        }
        FreeEnvironmentStringsW(envBlock);
    }
    constexpr char delimiter = ';';
#else
    for (char** entry = environ; *entry != nullptr; ++entry) {
        insertPair(*entry);
    }
    constexpr char delimiter = ':';
#endif

    if (auto it = ENVs.find("PATH"); it != ENVs.end()) {
        PATHs = utils::split(it->second, delimiter);
    }
}

} // namespace variables