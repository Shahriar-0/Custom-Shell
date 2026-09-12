# Custom Shell

A custom POSIX-style shell in C++23 (CMake build → `build/shell.exe`), portable across Windows and POSIX via `#ifdef`. Currently: a real lexer → recursive-descent parser → executor pipeline with quoting, `;`/`&&`/`||` chaining, and a builtin set — plus a 61-assertion integration suite pinning it all.

## Build & run

```sh
cmake -B build            # existing cache: Unix Makefiles, ucrt64 g++
cmake --build build
printf 'echo hi\nexit\n' | ./build/shell.exe
```

> **Windows note:** the exe links ucrt64 runtime DLLs, so `C:\msys64\ucrt64\bin` must be on `PATH` at runtime or it dies with `0xC0000139`. Any MSYS shell has it; from PowerShell/cmd prepend it first. In MSYS bash that's `export PATH="/c/msys64/ucrt64/bin:$PATH"` (POSIX form — the `C:...` form doesn't resolve in bash). See `NOTES.md` for details.

Tests (no WSL needed — the `.ps1` uses MSYS2 bash under the hood):

```sh
bash tests/run_tests.sh       # POSIX / MSYS2
.\tests\run_tests.ps1         # PowerShell
```

61 assertions covering quoting, connectors, comments, syntax-error recovery,
stubs, and `exit`/EOF semantics — all passing via either runner.

## What works today

- **REPL** (`src/main.cpp`): lex → parse → execute per line. A line with a syntax error never partially runs — it prints `myshell: syntax error: <msg> (column N)`, sets `$? = 2`, and the REPL continues. EOF (Ctrl+D) exits cleanly with the last status.
- **Builtins**: `exit` (incl. `exit <code>`), `echo`, `help`, `clear`, `type`, `pwd`, `cd` (absolute / relative / `~`, bare `cd` → `$HOME`). Unknown commands → `command not found`, exit `127`.
- **Quoting** (lexer, `src/parser/lexer.cpp`): single quotes fully literal, double quotes group (backslash escapes only `\"`, `\\`, `$`, backtick/newline), backslash escapes operators outside quotes, adjacent segments join, `#` comments, tab separates words. Per-char quote provenance is recorded for future `$VAR` expansion.
- **Chaining**: `;` sequencing plus `&&` / `||` short-circuit driven by the *incoming* connector and real exit statuses (regression-tested, incl. `false && X || Y` chains).
- **Parsed but explicitly stubbed** (fail loudly with `not implemented yet`, `$? = 2`, instead of silently misbehaving): `>` / `>>` / `<` redirection, multi-stage `|` pipelines, trailing-`&` backgrounding. `( )` are rejected as reserved syntax.
- **Exit codes**: `127` not found, `2` syntax/usage/not-implemented, `0` success; tracked in `variables::lastExitStatus` (the future `$?`).

## Roadmap

Canonical plan: `.hermes/ROADMAP.md` (ordered by dependency, with a CodeCrafters 76-stage mapping in Appendix A). `docs/shell-foundations.md` holds the design notes; `.hermes/DECISIONS.md` logs past decisions.

| Phase | Status |
|---|---|
| 0 — Stabilization | ✅ done |
| 1 — `Command`/`Pipeline` AST + lexer/parser/executor | ✅ done |
| 1b — Quoting | ✅ done |
| 12 (part) — `&&`/`\|\|`/`;` short-circuit | ✅ done (landed early) |
| 2 — I/O redirection (`>`, `>>`, `<`, per-fd) | ⬅️ next |
| 3 — Pipes | not started |
| 4 — Variable expansion (`$VAR`, `${VAR}`, `$?`, `$$`) | not started |
| 5 — Exit status formalization | not started |
| 6 — Job control (`&`, `jobs`, `fg`, `bg`, `kill`) | not started |
| 7 — History | not started |
| 8 — Line editing / completion | not started (parallel track) |
| 9 — Aliases | not started |
| 10 — Command substitution | not started |
| 11 — Scripting (`if`/`for`/`while`, functions) | not started |
| 12 (rest) — `{ }` grouping, `exec`, process substitution | not started |
| 13 — Misc builtins (`ls`, `mkdir`, `rmdir`, `set -x/-v`) | not started |

## Layout

```
src/main.cpp            REPL: lex → parse → execute
src/parser/             lexer, recursive-descent parser, AST (Command/Pipeline/CommandLine)
src/executor/           walks CommandLine: connectors + single-command dispatch
src/builtin_commands/   exit echo help clear type pwd cd
src/executables/        PATH lookup + external execution
src/variables/          ENVs, PATHs, lastExitStatus
src/utils/              legacy splitter (unused by main), expandHome, glob helpers
tests/run_tests.sh / .ps1   61-assertion integration suite (pipes stdin, checks stdout/stderr/exit)
docs/shell-foundations.md   design reference (Brennan lsh + Crafting Interpreters)
.hermes/ROADMAP.md      canonical roadmap   .hermes/DECISIONS.md  decision log
```

One module per folder, each its own static lib (`-Wall -Wextra`). Known toolchain quirk: MSYS2 MinGW GCC ships no `libasan`/`libubsan`, so sanitizers are POSIX-only in `CMakeLists.txt` (Windows Debug is plain `-g`).
