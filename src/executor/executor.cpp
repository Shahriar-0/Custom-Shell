#include "executor.hpp"

#include <iostream>
#include <string>

#include "builtin_commands/builtin_commands.hpp"
#include "executables/executables.hpp"
#include "variables/variables.hpp"

namespace executor {

namespace {

constexpr int kNotImplemented = 2;
constexpr int kNotFound = 127;

void reportNotImplemented(const std::string& feature, const std::string& detail) {
    std::cerr << feature << ": not implemented yet (" << detail << ")\n";
}

const char* redirOperator(parser::RedirType type) {
    switch (type) {
    case parser::RedirType::In:
        return "<";
    case parser::RedirType::Append:
        return ">>";
    case parser::RedirType::Out:
        return ">";
    }
    return "?";
}

// Resolves a parsed Command to plain strings. Today that only means dropping
// the per-character quote provenance; TODO: replaces the body with real
// expansion ($VAR, ${VAR}, $?, $$, tilde, ...) driven by Word::quoteKinds.
// Called once per command, immediately before it runs, so variable state
// left by earlier commands on the same line is visible.
parser::ExpandedCommand expand(const parser::Command& cmd) {
    parser::ExpandedCommand out;
    out.program = cmd.program.text;
    out.args.reserve(cmd.args.size());
    for (const auto& arg : cmd.args) {
        out.args.push_back(arg.text);
    }
    out.redirections.reserve(cmd.redirections.size());
    for (const auto& redir : cmd.redirections) {
        out.redirections.push_back({redir.fd, redir.type, redir.target.text});
    }
    return out;
}

// Runs one simple command (no pipes). Redirections are parsed but not yet
// applied — that is Phase 2's job; we refuse to silently ignore them so the
// user never thinks a redirect succeeded when it didn't.
int runCommand(const parser::ExpandedCommand& cmd, bool background) {
    if (!cmd.redirections.empty()) {
        const auto& redir = cmd.redirections.front();
        reportNotImplemented("redirection",
                             "fd " + std::to_string(redir.fd) + " " +
                                 redirOperator(redir.type) + " '" + redir.filename +
                                 "' was parsed but not applied");
        return kNotImplemented;
    }
    if (background) {
        reportNotImplemented("backgrounding",
                             "'" + cmd.program + "' will run in the foreground");
        // fall through and run in foreground for now
    }

    if (auto it = shell_builtin_commands::shell_builtin_cmds.find(cmd.program);
        it != shell_builtin_commands::shell_builtin_cmds.end()) {
        return it->second(cmd);
    }
    if (executables::commandExists(cmd.program)) {
        return executables::run(cmd);
    }
    std::cerr << cmd.program << ": command not found\n";
    return kNotFound;
}

// Runs one pipeline. Multi-stage pipelines (cmd1 | cmd2) are Phase 3; a
// single command is the common case today.
int runPipeline(const parser::Pipeline& pipeline, bool background) {
    if (pipeline.isMultiStage()) {
        reportNotImplemented("pipes",
                             std::to_string(pipeline.commands.size()) +
                                 "-command pipeline");
        return kNotImplemented;
    }
    return runCommand(expand(pipeline.commands.front()), background);
}

// Walks one AND-OR list left to right with short-circuiting.
//
// `incoming` is the connector that led INTO the current link — i.e. the
// previous link's connectorToNext — not the current link's own
// connectorToNext, which describes where it leads NEXT. Checking a link's
// own outgoing connector to decide whether to run it is the classic
// off-by-one here: it would ask "should the thing after this succeed" while
// it's actually deciding whether THIS thing runs.
//
// A skipped pipeline leaves lastExitStatus alone, so in `a && b || c` with
// `a` failing, `b` is skipped and `||` still sees `a`'s failure.
void runList(const parser::AndOrList& list) {
    parser::Connector incoming = parser::Connector::None;

    for (const auto& link : list.pipelines) {
        const bool skip =
            (incoming == parser::Connector::And && variables::lastExitStatus != 0) ||
            (incoming == parser::Connector::Or && variables::lastExitStatus == 0);
        if (!skip) {
            variables::lastExitStatus = runPipeline(link.pipeline, list.background);
        }
        incoming = link.connectorToNext;
    }
}

} // namespace

int execute(const parser::CommandLine& line) {
    // lastExitStatus is deliberately NOT reset here: it is the shell's `$?`,
    // and must keep the previous line's value until a command actually runs
    // (the first pipeline of a line always runs, so it is overwritten
    // immediately in the normal case).
    for (const auto& list : line.lists) {
        runList(list);
    }
    return variables::lastExitStatus;
}

} // namespace executor
