#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>

namespace parser {

// One word as written. `quoteKinds` has one QuoteKind code (token.hpp) per
// character of `text`. `"$X"` expands and `'$X'` doesn't, so this has to
// reach the executor.
struct Word {
    std::string text;
    std::string quoteKinds;
};

enum class RedirType {
    In,     // <
    Out,    // >
    Append, // >>
    DupOut, // >&  target is an fd number, or "-" to close fd
    DupIn,  // <&  same
};

// One redirection. `fd` is the descriptor being redirected: the explicit
// IO_NUMBER if one was written (`2>file`), otherwise the operator's default
// (0 for `<` and `<&`, 1 for the rest). For the dup types `target` is not
// checked here, since it may come from an expansion later. `&>f` and `&>>f`
// have no type of their own, the parser lowers them to `>f` + `2>&1`.
struct Redirection {
    int fd = 1;
    RedirType type = RedirType::Out;
    Word target;
};

// One simple command: a program name, its arguments, and any redirections
// attached to it. Redirections may appear anywhere among the words, including
// before the program; they are kept in parse order.
struct Command {
    Word program;                          // first word
    std::vector<Word> args;                // remaining words
    std::vector<Redirection> redirections; // parse order preserved
};

// Post-expansion form: plain strings, quoting resolved. Built per command
// at execution time (not parse time) so `false; echo $?` sees fresh state.

struct ExpandedRedirection {
    int fd = 1;
    RedirType type = RedirType::Out;
    std::string filename;
};

struct ExpandedCommand {
    std::string program;
    std::vector<std::string> args;
    std::vector<ExpandedRedirection> redirections;
};

// One or more commands connected by '|': `cmd1 | cmd2 | cmd3`.
struct Pipeline {
    std::vector<Command> commands;

    bool isMultiStage() const { return commands.size() > 1; }
};

// How a pipeline connects to the next one in its AndOrList.
enum class Connector {
    None, // no pipeline follows (end of the list)
    And,  // &&  (next pipeline runs only if this one succeeded)
    Or,   // ||  (next pipeline runs only if this one failed)
};

struct PipelineLink {
    Pipeline pipeline;
    Connector connectorToNext = Connector::None;
};

// POSIX "AND-OR list": pipelines joined by '&&' / '||', evaluated left to
// right with short-circuiting. The trailing '&' (if any) applies to this
// whole list and nothing else, so `a ; b &` backgrounds only `b`.
struct AndOrList {
    std::vector<PipelineLink> pipelines;
    bool background = false; // list was terminated by '&'
};

// One full input line (POSIX "complete_command"): AND-OR lists separated
// by ';' or '&'.
struct CommandLine {
    std::vector<AndOrList> lists;
};

} // namespace parser

#endif // AST_HPP
