#include "redirect.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace redirect {

namespace {

constexpr int kFirstSaveFd = 10;

#ifdef _WIN32

// The UCRT aborts the process on an invalid fd instead of returning EBADF.
// Probing whether an fd is open is what this file does, so make it an error.
void initPlatform() {
    static const bool done = [] {
        _set_invalid_parameter_handler(
            [](const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) {});
        return true;
    }();
    (void)done;
}

long long maxFd() { return 2048; }

void sysClose(int fd) { _close(fd); }

int sysDup2(int from, int to) { return _dup2(from, to) == 0 ? to : -1; }

bool isOpen(int fd) {
    intptr_t handle = _get_osfhandle(fd);
    return handle != -1 && handle != -2;
}

// The CRT can only duplicate to the lowest free number, so duplicate until
// the result is high enough and close the ones in between.
int dupAbove(int fd, int minFd) {
    std::vector<int> spare;
    int result;
    while ((result = _dup(fd)) >= 0 && result < minFd) {
        spare.push_back(result);
    }
    int err = errno;
    for (int s : spare) {
        _close(s);
    }
    errno = err;
    return result;
}

int errnoFromWin32(DWORD error) {
    switch (error) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        return ENOENT;
    case ERROR_ACCESS_DENIED:
    case ERROR_SHARING_VIOLATION:
    case ERROR_LOCK_VIOLATION:
        return EACCES;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
        return EEXIST;
    case ERROR_DIRECTORY:
        return ENOTDIR;
    case ERROR_TOO_MANY_OPEN_FILES:
        return EMFILE;
    case ERROR_INVALID_NAME:
    case ERROR_BAD_PATHNAME:
    case ERROR_FILENAME_EXCED_RANGE:
        return EINVAL;
    default:
        return EIO;
    }
}

// Uses CreateFileW instead of _wopen so that ">>" can hand the child a
// handle that already sits at the end of the file. A child only inherits the
// OS handle, and the CRT's _O_APPEND is just a seek the CRT does itself
// before its own writes. The handle keeps full write access, not only
// FILE_APPEND_DATA, because some programs (the MSYS runtime, apparently)
// treat a handle without FILE_WRITE_DATA as read-only.
int openFile(const std::string& path, parser::RedirType type) {
    const std::string name = (path == "/dev/null") ? "NUL" : path;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, nullptr, 0);
    if (wlen <= 0) {
        errno = EINVAL;
        return -1;
    }
    std::wstring wide(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, wide.data(), wlen);

    DWORD access = GENERIC_WRITE;
    DWORD disposition = CREATE_ALWAYS;
    int crtFlags = _O_BINARY;
    switch (type) {
    case parser::RedirType::In:
        access = GENERIC_READ;
        disposition = OPEN_EXISTING;
        crtFlags |= _O_RDONLY;
        break;
    case parser::RedirType::Append:
        disposition = OPEN_ALWAYS;
        crtFlags |= _O_APPEND;
        break;
    default:
        break;
    }

    HANDLE handle = CreateFileW(wide.c_str(), access,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr, disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (type == parser::RedirType::Append &&
        !SetFilePointerEx(handle, LARGE_INTEGER{}, nullptr, FILE_END)) {
        errno = errnoFromWin32(GetLastError());
        CloseHandle(handle);
        return -1;
    }
    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), crtFlags);
    if (fd < 0) {
        int err = errno;
        CloseHandle(handle);
        errno = err;
    }
    return fd;
}

#else

void initPlatform() {}

long long maxFd() {
    long limit = ::sysconf(_SC_OPEN_MAX);
    return limit > 0 ? limit : 1024;
}

void sysClose(int fd) { ::close(fd); }

int sysDup2(int from, int to) {
    int r;
    do {
        r = ::dup2(from, to);
    } while (r < 0 && errno == EINTR);
    return r;
}

bool isOpen(int fd) { return ::fcntl(fd, F_GETFD) != -1; }

// Close-on-exec so a saved copy can't leak into a program we exec.
int dupAbove(int fd, int minFd) { return ::fcntl(fd, F_DUPFD_CLOEXEC, minFd); }

int openFile(const std::string& path, parser::RedirType type) {
    int flags = O_RDONLY;
    switch (type) {
    case parser::RedirType::In:
        break;
    case parser::RedirType::Append:
        flags = O_WRONLY | O_CREAT | O_APPEND;
        break;
    default:
        flags = O_WRONLY | O_CREAT | O_TRUNC;
        break;
    }
    int fd;
    do {
        fd = ::open(path.c_str(), flags, 0666);
    } while (fd < 0 && errno == EINTR);
    return fd;
}

#endif

// Target of `>&` / `<&`: a descriptor number, or "-" to close.
bool parseDupTarget(const std::string& text, int& fd, bool& closing) {
    closing = (text == "-");
    fd = -1;
    if (closing) {
        return true;
    }
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), fd);
    return !text.empty() && ec == std::errc{} && ptr == text.data() + text.size() && fd >= 0;
}

bool isDup(parser::RedirType type) {
    return type == parser::RedirType::DupOut || type == parser::RedirType::DupIn;
}

void flushStreams() {
    std::cout.flush();
    std::cerr.flush();
    std::fflush(nullptr);
}

// A failed write to a redirected stream leaves it in an error state that
// would otherwise silence every later prompt.
void clearStreamErrors() {
    std::cout.clear();
    std::cerr.clear();
    std::clearerr(stdout);
    std::clearerr(stderr);
}

} // namespace

Scope::~Scope() {
    restore();
}

std::string Scope::save(int fd) {
    for (const auto& s : saved_) {
        if (s.fd == fd) {
            return {};
        }
    }
    int copy = dupAbove(fd, saveBase_);
    if (copy >= 0) {
        saved_.push_back({fd, copy});
    }
    else if (errno == EBADF) {
        saved_.push_back({fd, -1});
    }
    else {
        return std::to_string(fd) + ": " + std::strerror(errno);
    }
    return {};
}

std::string Scope::applyOne(const parser::ExpandedRedirection& r) {
    if (isDup(r.type)) {
        int source = -1;
        bool closing = false;
        if (!parseDupTarget(r.filename, source, closing)) {
            return r.filename + ": file descriptor number or '-' expected";
        }
        if (!closing && !isOpen(source)) {
            return std::to_string(source) + ": " + std::strerror(EBADF);
        }
        if (std::string err = save(r.fd); !err.empty()) {
            return err;
        }
        if (closing) {
            if (isOpen(r.fd)) {
                sysClose(r.fd);
            }
        }
        else if (source != r.fd && sysDup2(source, r.fd) < 0) {
            return std::to_string(r.fd) + ": " + std::strerror(errno);
        }
        return {};
    }

    // Save first: if r.fd is closed, open() may return that very number.
    if (std::string err = save(r.fd); !err.empty()) {
        return err;
    }
    int opened = openFile(r.filename, r.type);
    if (opened < 0) {
        return r.filename + ": " + std::strerror(errno);
    }
    if (opened != r.fd) {
        int err = sysDup2(opened, r.fd) < 0 ? errno : 0;
        sysClose(opened);
        if (err != 0) {
            return std::to_string(r.fd) + ": " + std::strerror(err);
        }
    }
    return {};
}

bool Scope::apply(const std::vector<parser::ExpandedRedirection>& redirections) {
    initPlatform();
    if (redirections.empty()) {
        return true;
    }

    // Number the saved copies above every fd the command mentions, so its own
    // `10>f` or `3>&10` can't clobber a backup.
    long long highest = kFirstSaveFd - 1;
    for (const auto& r : redirections) {
        highest = std::max<long long>(highest, r.fd);
        int source = -1;
        bool closing = false;
        if (isDup(r.type) && parseDupTarget(r.filename, source, closing) && !closing) {
            highest = std::max<long long>(highest, source);
        }
    }
    if (highest + 1 >= maxFd()) {
        std::cerr << "myshell: " << highest << ": " << std::strerror(EBADF) << "\n";
        return false;
    }
    saveBase_ = static_cast<int>(highest + 1);

    flushStreams();
    for (const auto& r : redirections) {
        if (std::string err = applyOne(r); !err.empty()) {
            std::cerr << "myshell: " << err << "\n";
            restore();
            return false;
        }
    }
    return true;
}

void Scope::restore() {
    if (saved_.empty()) {
        return;
    }
    flushStreams();
    for (auto it = saved_.rbegin(); it != saved_.rend(); ++it) {
        if (it->copy >= 0) {
            sysDup2(it->copy, it->fd);
            sysClose(it->copy);
        }
        else if (isOpen(it->fd)) {
            sysClose(it->fd);
        }
    }
    saved_.clear();
    clearStreamErrors();
}

} // namespace redirect
