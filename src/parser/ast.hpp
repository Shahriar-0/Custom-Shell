#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>

namespace parser {

// One word as written on the command line. `quoteKinds` is parallel to
// `text` (same length): one QuoteKind code (see token.hpp) per character,
// recording whether that character was bare, single-quoted, double-quoted
// or backslash-escaped. `"$X"` expands, `'$X'` does not, so it must survive
// from the lexer all the way to the executor instead of being dropped
// when the parser builds the AST.
struct Word {
    std::string text;
    std::string quoteKinds;
};

enum class RedirType {
    In,     // <
    Out,    // >
    Append, // >>
};

// One redirection. `fd` is the file descriptor being redirected: the explicit
// IO_NUMBER if one was written (`2>file`), otherwise the operator's default
// (0 for `<`, 1 for `>` and `>>`).
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

// ---------------------------------------------------------------------
// Post-expansion form
//
// What builtins and externals actually receive: plain strings, with
// quoting already resolved. The executor builds one of these from each
// Command immediately before running it — at execution time, not parse
// time, so that `declare X=1; echo $X` and `false; echo $?` see the state
// left by the preceding command.
// ---------------------------------------------------------------------

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

// One or more commands connected by '|': `cmd1 | cmd2 | cmd3`. Named to
// match the POSIX grammar's use of "pipeline" for exactly this — a single
// command is just a one-stage pipeline.
struct Pipeline {
    std::vector<Command> commands;

    bool isMultiStage() const { return commands.size() > 1; }
};

// How one pipeline connects to the pipeline that follows it inside the same
// AndOrList. Lives on the link between two pipelines, not on the pipeline
// itself — a pipeline doesn't "have" a connector, the gap between two
// pipelines does.
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
