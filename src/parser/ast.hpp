#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>

namespace parser {

enum class RedirType {
    In,     // <
    Out,    // >
    Append, // >>
};

struct Redirection {
    RedirType type;
    std::string filename;
};

// One simple command: a program name, its arguments, and any redirections
// attached to it (parsed starting Phase 1; applied starting Phase 2).
struct Command {
    std::string program;                   // first word
    std::vector<std::string> args;         // remaining words
    std::vector<Redirection> redirections; // parse order preserved
};

// One or more commands connected by '|': `cmd1 | cmd2 | cmd3`. Named to
// match the POSIX grammar's use of "pipeline" for exactly this — a single
// command is just a one-stage pipeline.
struct Pipeline {
    std::vector<Command> commands;

    bool isMultiStage() const { return commands.size() > 1; }
};

// How one pipeline connects to the pipeline that follows it on the same
// line. Lives on the link between two pipelines, not on the pipeline
// itself — a pipeline doesn't "have" a connector, the gap between two
// pipelines does.
enum class Connector {
    None,     // no pipeline follows (end of line)
    Sequence, // ;
    And,      // &&  (next pipeline runs only if this one succeeded)
    Or,       // ||  (next pipeline runs only if this one failed)
};

struct PipelineLink {
    Pipeline pipeline;
    Connector connectorToNext = Connector::None;
};

// One full input line (POSIX calls this a "complete_command"): a sequence
// of pipelines connected by ';', '&&', '||', optionally backgrounded with
// a trailing '&'.
struct CommandLine {
    std::vector<PipelineLink> pipelines;
    bool background = false; // trailing &
};

} // namespace parser

#endif // AST_HPP
