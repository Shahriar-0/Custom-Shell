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

// Runs one pipeline. Multi-stage pipelines (cmd1 | cmd2) are Phase 3; a
// single command is the common case today.
int runPipeline(const parser::Pipeline& pipeline, bool background) {
    if (pipeline.isMultiStage()) {
        reportNotImplemented("pipes",
                             std::to_string(pipeline.commands.size()) +
                                 "-command pipeline");
        return kNotImplemented;
    }
    return runCommand(pipeline.commands.front(), background);
}

} // namespace

int execute(const parser::CommandLine& line) {
    variables::lastExitStatus = 0;

    // `incoming` is the connector that led INTO the current link — i.e.
    // the previous link's connectorToNext — not the current link's own
    // connectorToNext, which describes where it leads NEXT. Checking a
    // link's own outgoing connector to decide whether to run it is the
    // classic off-by-one here: it would ask "should the thing after this
    // succeed" while it's actually deciding whether THIS thing runs.
    parser::Connector incoming = parser::Connector::None;

    for (const auto& link : line.pipelines) {
        switch (incoming) {
        case parser::Connector::And:
            if (variables::lastExitStatus != 0) {
                incoming = link.connectorToNext;
                continue; // short-circuit: previous command failed
            }
            break;
        case parser::Connector::Or:
            if (variables::lastExitStatus == 0) {
                incoming = link.connectorToNext;
                continue; // short-circuit: previous command succeeded
            }
            break;
        case parser::Connector::Sequence:
        case parser::Connector::None:
            break;
        }
        variables::lastExitStatus = runPipeline(link.pipeline, line.background);
        incoming = link.connectorToNext;
    }

    return variables::lastExitStatus;
}

} // namespace executor
