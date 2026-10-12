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

// Turns a parsed Command into plain strings. For now this just drops the
// per-character quote info.
// TODO: real expansion ($VAR, ${VAR}, $?, $$, ~) driven by Word::quoteKinds.
// Runs right before each command so earlier commands on the line are visible.
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

// Runs one simple command (no pipes). Redirections aren't applied yet, so
// refuse them loudly rather than pretend they worked.
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

// Runs one pipeline. Only single commands for now; cmd1 | cmd2 is stubbed.
int runPipeline(const parser::Pipeline& pipeline, bool background) {
    if (pipeline.isMultiStage()) {
        reportNotImplemented("pipes",
                             std::to_string(pipeline.commands.size()) +
                                 "-command pipeline");
        return kNotImplemented;
    }
    return runCommand(expand(pipeline.commands.front()), background);
}

// Runs one AND-OR list left to right with short-circuiting.
//
// `incoming` is the previous link's connectorToNext. Whether a pipeline runs
// depends on the connector before it, not its own trailing one.
//
// A skipped pipeline leaves lastExitStatus alone, so in `a && b || c` with
// `a` failing, `||` still sees a's failure.
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
    // Don't reset lastExitStatus: it's $?, and carries over between lines.
    for (const auto& list : line.lists) {
        runList(list);
    }
    return variables::lastExitStatus;
}

} // namespace executor
