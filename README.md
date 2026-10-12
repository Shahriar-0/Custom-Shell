# Custom Shell

A custom POSIX-style shell in C++23 (CMake build → `build/shell.exe`), portable across Windows and POSIX via `#ifdef`. Currently: a real lexer → recursive-descent parser → executor pipeline with quoting, `;`/`&&`/`||` chaining, and a builtin set — plus a 117-assertion integration suite pinning it all.

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

117 assertions covering quoting (incl. empty quotes), connectors, comments,
syntax-error recovery, redirection parsing (fd numbers, operator placement),
stubs, builtin arg edges, a real external run, Windows extension lookup, and
`exit`/EOF/`$?` semantics — all passing via either runner.

## What works today

- **REPL** (`src/main.cpp`): lex → parse → execute per line. A line with a syntax error never partially runs — it prints `myshell: syntax error: <msg> (column N)`, sets `$? = 2`, and the REPL continues. EOF (Ctrl+D) exits cleanly with the last status.
- **Builtins**: `exit` (bare `exit` uses the last status; `exit <code>` must be a whole integer, else status 2; extra args refused, shell stays alive), `echo`, `help`, `clear`, `type` (resolves every arg), `pwd`, `cd` (absolute / relative / `~`, bare `cd` → `$HOME`; extra args refused). Unknown commands → `command not found`, exit `127`.
- **Externals**: PATH search plus arg passing, verified by a real run in the suite; on Windows, bare names resolve via PATHEXT probing (`hostname` finds `hostname.exe`) and the resolved path is what gets launched, with CRT-correct argument quoting. POSIX statuses follow bash: `126`/`127` for exec failures, `128+N` for a child killed by signal N.
- **Quoting** (lexer, `src/parser/lexer.cpp`): single quotes fully literal, double quotes group (backslash escapes only `\"`, `\\`, `$`, backtick/newline), backslash escapes operators outside quotes, adjacent segments join, empty quotes (`""`, `''`) are real empty arguments, `#` comments, tab separates words. Per-char quote provenance is recorded by the lexer and carried on `Word` through the AST, ready for future `$VAR` expansion.
- **Chaining**: `;` / `&` separate AND-OR lists (a trailing `;` is fine); inside a list `&&` / `||` short-circuit, driven by the *incoming* connector and real exit statuses (regression-tested, incl. `false && X || Y` chains). `$?` (`variables::lastExitStatus`) is never reset between lines.
- **Redirection syntax**: `>`, `>>`, `<` with optional fd (`2>f`, `2>>f`, `0<f`; only an unquoted, all-digit word touching the operator counts as an fd), anywhere in a command including before the program name. A command must still have a program (`> f` alone is a syntax error).
- **Parsed but explicitly stubbed**: `>` / `>>` / `<` redirection and multi-stage `|` pipelines fail loudly with `not implemented yet` and `$? = 2` instead of silently misbehaving. `&` backgrounding is parsed per AND-OR list (`a ; b &` backgrounds only `b`); until Phase 6 it prints a notice and runs in the foreground with the real status. `( )` are rejected as reserved syntax.
- **Exit codes**: `127` not found, `2` syntax/usage/not-implemented, `0` success; tracked in `variables::lastExitStatus` (the future `$?`).

## Roadmap

Canonical plan (local, not in git): `.hermes/ROADMAP.md` (ordered by dependency, with a CodeCrafters 76-stage mapping in Appendix A). `docs/shell-foundations.md` holds the design notes; `.hermes/DECISIONS.md` logs past decisions.

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
src/parser/             lexer, recursive-descent parser, AST (Word/Command/Pipeline/AndOrList/CommandLine), SyntaxError
src/executor/           walks CommandLine: lists, connectors, expand() Command -> ExpandedCommand, single-command dispatch
src/builtin_commands/   exit echo help clear type pwd cd
src/executables/        PATH lookup + external execution
src/variables/          ENVs, PATHs, lastExitStatus
src/utils/              split/join, path predicates, expandHome
tests/run_tests.sh / .ps1   117-assertion integration suite (pipes stdin, checks stdout/stderr/exit)
docs/shell-foundations.md   design reference (Brennan lsh + Crafting Interpreters)
(local, gitignored:) .hermes/ROADMAP.md canonical roadmap, .hermes/DECISIONS.md decision log, docs/
```

One module per folder, each its own static lib (`-Wall -Wextra`). Known toolchain quirk: MSYS2 MinGW GCC ships no `libasan`/`libubsan`, so sanitizers are POSIX-only in `CMakeLists.txt` (Windows Debug is plain `-g`).
