#include "executor.hpp"

#include <iostream>

#include "builtin_commands/builtin_commands.hpp"
#include "executables/executables.hpp"
#include "variables/variables.hpp"

namespace executor {

namespace {

constexpr int kNotImplemented = 2;

void reportNotImplemented(const std::string& feature, const std::string& detail) {
    std::cerr << feature << ": not implemented yet (" << detail << ")\n";
}

// Runs one simple command (no pipes). Redirections are parsed but not yet
// applied — that is Phase 2's job; we refuse to silently ignore them so the
// user never thinks a redirect succeeded when it didn't.
int runCommand(const parser::Command& cmd, bool background) {
    if (!cmd.redirections.empty()) {
        reportNotImplemented("redirection", "'" + cmd.redirections.front().filename +
                                                 "' was parsed but not applied");
        return kNotImplemented;
    }
    if (background) {
        reportNotImplemented("backgrounding",
                             "'" + cmd.program + "' will run in the foreground");
        // fall through and run in foreground for now
    }

    const auto& args = cmd.args;

    if (auto it = shell_builtin_commands::shell_builtin_cmds.find(cmd.program);
        it != shell_builtin_commands::shell_builtin_cmds.end()) {
        return it->second(args);
    }
    if (executables::commandExists(cmd.program)) {
        return executables::run(cmd.program, args);
    }
    std::cerr << cmd.program << ": command not found\n";
    return 127;
}

// Runs one pipeline segment. Multi-command pipelines are Phase 3; a single
// command is the common case today.
int runPipelineNode(const parser::PipelineNode& node, bool background) {
    if (node.isPipeline()) {
        reportNotImplemented("pipes",
                             std::to_string(node.commands.size()) +
                                 "-command pipeline");
        return kNotImplemented;
    }
    return runCommand(node.commands.front(), background);
}

} // namespace

int execute(const parser::Pipeline& pipeline) {
    variables::lastExitStatus = 0;

    for (const auto& node : pipeline.nodes) {
        switch (node.connectorToNext) {
            case parser::Connector::And:
                if (variables::lastExitStatus != 0) {
                    continue; // short-circuit: skip until next non-&& link
                }
                break;
            case parser::Connector::Or:
                if (variables::lastExitStatus == 0) {
                    continue; // short-circuit: skip until next non-|| link
                }
                break;
            case parser::Connector::Sequence:
            case parser::Connector::None:
                break;
        }
        variables::lastExitStatus = runPipelineNode(node, pipeline.background);
    }

    return variables::lastExitStatus;
}

} // namespace executor
