#ifndef REDIRECT_HPP
#define REDIRECT_HPP

#include <string>
#include <vector>

#include "parser/ast.hpp"

namespace redirect {

// Applies redirections to this process's fd table and puts it back when the
// scope ends. Redirections apply left to right, so `>f 2>&1` and `2>&1 >f`
// differ, and `n>&m` copies fd m as it is at that moment.
class Scope {
public:
    Scope() = default;
    ~Scope();

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

    // On failure prints "myshell: <what>: <reason>" to stderr (as redirected
    // so far, like bash), restores the table and returns false.
    bool apply(const std::vector<parser::ExpandedRedirection>& redirections);

    void restore();

private:
    struct Saved {
        int fd;
        int copy; // duplicate of the original fd, or -1 if it was closed
    };

    // Both return an error message, empty on success.
    std::string applyOne(const parser::ExpandedRedirection& redirection);
    std::string save(int fd);

    std::vector<Saved> saved_;
    int saveBase_ = 10; // saved copies are numbered from here up
};

} // namespace redirect

#endif // REDIRECT_HPP
