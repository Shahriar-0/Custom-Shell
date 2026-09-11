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

struct Command {
    std::string program;                 // first word
    std::vector<std::string> args;       // remaining words
    std::vector<Redirection> redirections; // parse order preserved; applied in Phase 2
};

// How one pipeline segment connects to the next.
enum class Connector {
    None,     // last node on the line
    Sequence, // ;
    And,      // &&  (run next only if this succeeded)
    Or,       // ||  (run next only if this failed)
};

struct PipelineNode {
    std::vector<Command> commands; // cmd1 | cmd2 | cmd3
    Connector connectorToNext = Connector::None;

    bool isPipeline() const { return commands.size() > 1; }
};

struct Pipeline { // one full input line
    std::vector<PipelineNode> nodes;
    bool background = false; // trailing &
};

} // namespace parser

#endif // AST_HPP
