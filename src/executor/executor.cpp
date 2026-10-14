#include "executor.hpp"

#include <iostream>
#include <string>

#include "builtin_commands/builtin_commands.hpp"
#include "executables/executables.hpp"
#include "redirect/redirect.hpp"
#include "variables/variables.hpp"

namespace executor {

namespace {

constexpr int kNotImplemented = 2;
constexpr int kNotFound = 127;
constexpr int kRedirectionFailed = 1;

void reportNotImplemented(const std::string& feature, const std::string& detail) {
    std::cerr << feature << ": not implemented yet (" << detail << ")\n";
}

// TODO: For now this just drops per-character quote info.
// the real expansion ($VAR, ${VAR}, $?, $$, ~) driven by Word::quoteKinds.
// Turns a parsed Command into plain strings.
// Redirection targets too: one that expands to no word, or to several, is an
// "ambiguous redirect" error.
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

// Runs one simple command (no pipes). A failed redirection means the command
// doesn't run and the status is 1.
int runCommand(const parser::ExpandedCommand& cmd, bool background) {
    if (background) {
        reportNotImplemented("backgrounding",
                             "'" + cmd.program + "' will run in the foreground");
        // fall through and run in foreground for now
    }

    const auto builtin = shell_builtin_commands::shell_builtin_cmds.find(cmd.program);
    const bool isBuiltin = builtin != shell_builtin_commands::shell_builtin_cmds.end();

    // Externals redirect their own child.
    if (!isBuiltin && executables::commandExists(cmd.program)) {
        return executables::run(cmd);
    }

    // Builtins run in this process, so redirect it. The not-found message
    // goes through the redirections like any other output.
    redirect::Scope redirections;
    if (!redirections.apply(cmd.redirections)) {
        return kRedirectionFailed;
    }
    if (isBuiltin) {
        return builtin->second(cmd);
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

// `incoming` is the previous link's connectorToNext. Whether a
// pipeline runs depends on the connector before it, not its own
// trailing one. A skipped pipeline leaves lastExitStatus alone.
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
    for (const auto& list : line.lists) {
        runList(list);
    }
    return variables::lastExitStatus;
}

} // namespace executor
