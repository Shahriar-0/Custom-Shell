# Custom Shell

A custom POSIX-style shell in C++23 (CMake build → `build/shell.exe`), portable across Windows and POSIX via `#ifdef`. Currently: lexer → recursive-descent parser → executor, with quoting, `;`/`&&`/`||` chaining, I/O redirection, a few builtins, and an integration suite in `tests/`.

## Build & run

```sh
cmake -B build            # existing cache: Unix Makefiles, ucrt64 g++
cmake --build build
printf 'echo hi\nexit\n' | ./build/shell.exe
```

<!-- > **Windows note:** the exe links ucrt64 runtime DLLs, so `C:\msys64\ucrt64\bin` must be on `PATH` at runtime or it dies with `0xC0000139`. Any MSYS shell has it; from PowerShell/cmd prepend it first. In MSYS bash that's `export PATH="/c/msys64/ucrt64/bin:$PATH"` (POSIX form — the `C:...` form doesn't resolve in bash). -->

Tests (no WSL needed — the `.ps1` uses MSYS2 bash under the hood):

```sh
bash tests/run_tests.sh       # POSIX / MSYS2
.\tests\run_tests.ps1         # PowerShell
```

## What works today

- **REPL** (`src/main.cpp`): lex → parse → execute per line. A line with a syntax error never partially runs — it prints `myshell: syntax error: <msg> (column N)`, sets `$? = 2`, and the REPL continues. EOF (Ctrl+D) exits cleanly with the last status.
- **Builtins**: `exit` (bare `exit` uses the last status; `exit <code>` must be a whole integer, else status 2; extra args refused, shell stays alive), `echo`, `help`, `clear`, `type` (resolves every arg), `pwd`, `cd` (absolute / relative / `~`, bare `cd` → `$HOME`; extra args refused). Unknown commands → `command not found`, exit `127`.
- **Externals**: PATH search plus arg passing, verified by a real run in the suite; on Windows, bare names resolve via PATHEXT probing (`hostname` finds `hostname.exe`) and the resolved path is what gets launched, with CRT-correct argument quoting. POSIX statuses follow bash: `126`/`127` for exec failures, `128+N` for a child killed by signal N.
- **Quoting** (lexer, `src/parser/lexer.cpp`): single quotes fully literal, double quotes group (backslash escapes only `\"`, `\\`, `$`, backtick), backslash escapes operators outside quotes, adjacent segments join, empty quotes (`""`, `''`) are real empty arguments, `#` comments, tab separates words. Per-char quote info is kept on `Word` through the AST for future `$VAR` expansion.
- **Chaining**: `;` / `&` separate AND-OR lists (a trailing `;` is fine); inside a list `&&` / `||` short-circuit, driven by the *incoming* connector and real exit statuses (regression-tested, incl. `false && X || Y` chains). `$?` (`variables::lastExitStatus`) is never reset between lines.
- **Redirection** (`src/redirect/`), for builtins and externals alike:
  - Forms: `>`, `>>`, `<` with an optional fd (`2>f`, `2>>f`, `0<f`; only an unquoted, all-digit word touching the operator counts as an fd), `>&N`, `<&N`, `N>&M`, and `N>&-` / `<&-` to close, plus `&>f` and `&>>f` for stdout and stderr together. They can appear anywhere in a command, including before the program name. A command still needs a program (`> f` alone is a syntax error).
  - Applied left to right on the real fd table, as in bash: `>f 2>&1` and `2>&1 >f` differ, the last redirection of an fd wins (earlier files are still created or truncated), `n>&m` copies fd m as it is at that moment, and everything is restored afterwards. `/dev/null` works on Windows too.
  - A failed redirection prints `myshell: <name>: <reason>` to the stderr as redirected so far, skips the command and sets `$? = 1` (so `&&` skips and `||` runs). "command not found" goes through the redirections too, so `nosuch 2>log` captures it. A builtin whose write fails (`echo hi >&-`) reports `write error`, returns `1`, and the shell keeps working.
  - Externals are redirected in the forked child on POSIX. On Windows there is no fork, so the shell redirects its own fds and spawns the child with exactly those three handles; only fds 0-2 can reach an external there (`3>f cmd` is refused, builtins can use any fd). Files are opened in binary mode, so output is byte-exact, and `>>` appends at the OS level so a child's writes land at the end.
  - Not supported: `>&word` meaning "both streams to file word" (use `&>word`; a non-numeric target is an error), `<>`, here-docs, `>|`, and commands made only of redirections.
- **Parsed but explicitly stubbed**: multi-stage `|` pipelines fail loudly with `not implemented yet` and `$? = 2` instead of silently misbehaving. `&` backgrounding is parsed per AND-OR list (`a ; b &` backgrounds only `b`); for now it prints a notice and runs in the foreground with the real status. `( )` are rejected as reserved syntax.
- **Exit codes**: `127` not found, `126` found but could not start, `128+N` killed by signal N (POSIX), `1` failed redirection, `2` syntax/usage/not-implemented, `0` success; tracked in `variables::lastExitStatus` (the future `$?`).
