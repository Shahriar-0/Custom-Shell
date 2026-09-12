#!/usr/bin/env bash
# Phase 1 integration tests: pipes lines into the built shell.exe and checks
# stdout/stderr/exit-status against expectations.
#
# Usage:  tests/run_tests.sh            (uses build/shell.exe)
#         SHELL_BIN=build/shell_static.exe tests/run_tests.sh
#
# Requires the ucrt64 runtime DLLs on PATH (any MSYS shell has them) when
# testing a Windows build; on Linux/macOS the block below is a no-op.

set -u

# The exe links ucrt64's runtime DLLs (libstdc++-6 etc.). This harness's bash
# only has mingw64 on PATH, so add ucrt64 — without it the process dies with
# 0xC0000139 (STATUS_ENTRYPOINT_NOT_FOUND) and every test would "fail".
case ":$PATH:" in
    *":/c/msys64/ucrt64/bin:"*) ;;
    *) [ -d /c/msys64/ucrt64/bin ] && export PATH="/c/msys64/ucrt64/bin:$PATH" ;;
esac

BIN="${SHELL_BIN:-build/shell.exe}"
if [ ! -x "$BIN" ]; then
    echo "shell binary not found at $BIN — build first (cmake --build build)" >&2
    exit 1
fi

pass=0
fail=0

# run <script> -> sets OUT, ERR, RC for the last line executed
run() {
    local tmp
    tmp=$(mktemp)
    printf '%b' "$1" | "$BIN" >"$tmp.out" 2>"$tmp.err"
    RC=$?
    # strip prompts so assertions only see command output
    OUT=$(sed 's/\$ //g' "$tmp.out")
    ERR=$(cat "$tmp.err")
    PROMPTS=$(grep -o '\$ ' "$tmp.out" | wc -l || true)
    rm -f "$tmp" "$tmp.out" "$tmp.err"
}

# check <name> <expected> <actual-value> <where>
#   where: out (default) | err  -> matched against $OUT / $ERR; $3 is IGNORED
#          rc                   -> $3 is the real exit status, exact match vs $2
# NOTE: for out/err the match is always against the captured globals. To
# compare a transformed value (e.g. normalized paths), assign it to OUT/ERR
# first — passing it as $3 silently does nothing.
# An empty <expected> means "nothing at all" and is matched exactly, not as
# a (trivially-true-for-any-string) empty substring.
check() {
    local name="$1" want="$2" actual="$3" where="${4:-out}" ok=0
    case "$where" in
        out) actual="$OUT" ;;
        err) actual="$ERR" ;;
        rc)  : ;;
    esac
    if [[ "$where" == "rc" ]]; then
        [[ "$actual" -eq "$want" ]] && ok=1 || ok=0
    elif [[ -z "$want" ]]; then
        [[ -z "$actual" ]] && ok=1 || ok=0
    else
        [[ "$actual" == *"$want"* ]] && ok=1 || ok=0
    fi
    if [ "$ok" = 1 ]; then
        pass=$((pass+1))
        printf 'ok   %s\n' "$name"
    else
        fail=$((fail+1))
        printf 'FAIL %s\n     wanted [%s] in %s: %s\n' "$name" "$want" "$where" "$actual"
    fi
}

# checknot <name> <forbidden-substring> [where]
# Asserts <forbidden-substring> is absent from the given stream (default
# out). Use this — not check with an empty "want" — whenever the point is
# "this specific thing must not have run/appeared" alongside other output.
checknot() {
    local name="$1" forbidden="$2" where="${3:-out}" actual=""
    case "$where" in
        out) actual="$OUT" ;;
        err) actual="$ERR" ;;
    esac
    if [[ "$actual" != *"$forbidden"* ]]; then
        pass=$((pass+1))
        printf 'ok   %s\n' "$name"
    else
        fail=$((fail+1))
        printf 'FAIL %s\n     did not want [%s] in %s: %s\n' "$name" "$forbidden" "$where" "$actual"
    fi
}

# norm_path <path> -> path with backslashes turned into forward slashes and
# lowercased. Needed only for the pwd comparison below: on Windows, MSYS2
# bash's own `pwd` is POSIX-style (/d/foo/bar) while shell.exe is a native
# binary using std::filesystem, which prints Windows-style paths
# (D:\foo\bar) — same directory, different spelling. `pwd -W` (an MSYS2/Git
# Bash extension) gives bash's own Windows-style form so the two are at
# least comparable, and normalizing away slash direction and drive-letter
# case handles the rest. On real POSIX systems `pwd -W` doesn't exist, the
# `|| pwd` fallback kicks in, and both sides are already identical POSIX
# paths — normalization is then a harmless no-op.
norm_path() {
    printf '%s' "$1" | tr '\\' '/' | tr '[:upper:]' '[:lower:]'
}

# ---------- basic execution ----------

run 'echo hello world\n'
check "echo args"            "hello world" "$OUT"

# Compare against the test runner's own cwd rather than a hardcoded folder
# name — the shell inherits this process's cwd, but the repo can be checked
# out anywhere, on any OS. `pwd -W` gives bash's Windows-style form when
# running under MSYS2/Git Bash (falls back to plain `pwd` elsewhere); both
# sides are normalized so backslash-vs-forward-slash and drive-letter case
# don't cause a false failure.
expected_cwd="$(norm_path "$(pwd -W 2>/dev/null || pwd)")"
run 'pwd\n'
# check() matches $OUT verbatim, so normalize the capture in place first — a
# normalized value passed as $3 would be ignored (see check's NOTE above).
OUT="$(norm_path "$OUT")"
check "pwd prints a path"    "$expected_cwd" "$OUT"

run 'type echo\n'
check "type finds builtin"   "shell builtin" "$OUT"

run 'nosuchcmd_xyz\n'
check "unknown command msg"  "command not found" "$ERR" err
check "unknown command rc"   "127" "$RC" rc

# ---------- quoting ----------

run 'echo "a | b" c\n'
check "double-quoted pipe is literal"  "a | b c" "$OUT"

run "echo 'a && b' end\n"
check "single quotes are literal"      "a && b end" "$OUT"

run 'echo esc\\|pipe\n'
check "backslash escapes operator"     "esc|pipe" "$OUT"

run 'echo "mix ""ed"x\n'
check "adjacent quoted segments join"  "mix edx" "$OUT"

run 'echo tab\tsep\n'
check "tab separates words"            "tab sep" "$OUT"

# ---------- connectors ----------

run 'echo one ; echo two\n'
check "semicolon runs both"    "one" "$OUT"
check "semicolon runs both (2)" "two" "$OUT"

# nosuchcmd_xyz is guaranteed missing (127, a real failure); pwd is a
# builtin, guaranteed present on every platform and always succeeding (0).
# Deliberately not using `true`/`false`: those are external binaries that
# some platforms don't ship on PATH at all and others (e.g. Linux
# coreutils) do — relying on that made these tests' outcome depend on the
# OS running them rather than on the shell's own logic.
run 'nosuchcmd_xyz || echo fallback\n'
check "|| runs after failure"   "fallback" "$OUT"

run 'pwd || echo fallback\n'
checknot "|| skips after success"  "fallback"

run 'nosuchcmd_xyz && echo skipped\n'
check "&& skips after failure"  "" "$OUT"

run 'pwd && echo yes\n'
check "&& runs after success"   "yes" "$OUT"

# Regression: the connector that decides whether a pipeline runs is the
# connector coming INTO it (the previous pipeline's), never its own
# outgoing connector. Getting this backwards makes a shell skip the very
# first command whenever it's followed by '||' (it checks "succeeded so
# far" before anything has run), and lets '&&'-guarded commands run when
# they should have been skipped.
run 'pwd || echo should-not-run\n'
checknot "first command isn't skipped by its own trailing '||'" "should-not-run"

# cd fails (rc 1) -> the && branch must be skipped; the ; branch always
# runs regardless. The checknot below is what actually catches a shell
# that runs both branches unconditionally — the original version of this
# test only checked that "ran" appeared and never checked that "skipped"
# didn't, so it passed even when the && branch incorrectly ran too.
run 'cd /definitely/not/a/real/dir && echo skipped ; echo ran\n'
check    "connector uses real status: ; always runs"    "ran" "$OUT"
checknot "connector uses real status: && branch skipped" "skipped"

# Three-link chain false && X || Y: X must be skipped (previous failed);
# Y must run, because the last *actually executed* command's status (the
# failure) is what || sees — a skipped command never updates the status.
run 'nosuchcmd_xyz && echo skipped2 || echo shown\n'
check    "&&-then-|| chain: || branch runs"    "shown" "$OUT"
checknot "&&-then-|| chain: && branch skipped" "skipped2"

# ---------- comments & blank input ----------

run '# a comment line\necho visible\n'
check "comment ignored"       "visible" "$OUT"

run '\n   \n\t\n'
check "blank lines produce no output"  "" "$OUT"
check "blank lines produce no errors"  "" "$ERR" err

# ---------- syntax errors: report, set rc=2, keep REPL alive ----------

syntax_error_case() {
    local name="$1" script="$2"
    run "$script"
    check "$name: message"   "syntax error" "$ERR" err
    check "$name: status 2"  "2" "$RC" rc
    # REPL survived: a fresh prompt was printed after the error line.
    # check() only does exact rc matches, so fold ">= 2 prompts" into 2/0.
    check "$name: survives"  "2" "$(( ${PROMPTS:-0} >= 2 ? 2 : 0 ))" rc
}

syntax_error_case "leading pipe"        '| cmd\n'
syntax_error_case "leading &&"          '&& cmd\n'
syntax_error_case "trailing pipe"       'cmd |\n'
syntax_error_case "trailing &&"         'cmd &&\n'
syntax_error_case "missing redirect target" 'echo >\n'
syntax_error_case "empty pipeline slot" 'a | | b\n'
syntax_error_case "stray paren"         'echo (x)\n'
syntax_error_case "unclosed double quote" 'echo "open\n'
syntax_error_case "unclosed single quote" "echo 'open\n"
syntax_error_case "mid-line ampersand"  'echo a & echo b\n'

# error must not kill the session: bad line, then a good one still runs
run 'bad |||\necho still-alive\n'
check "REPL continues after error"  "still-alive" "$OUT"

# ---------- parsed-but-not-implemented stubs ----------

run 'echo x > f.txt\n'
check "redirection reports stub"  "redirection: not implemented yet" "$ERR" err

run 'echo p | wc -l\n'
check "pipe reports stub"         "pipes: not implemented yet" "$ERR" err

run 'echo hi &\n'
check "background reports stub"   "not implemented yet" "$ERR" err

# ---------- builtin argument edges (bash behavior) ----------

run 'cd /tmp extra-arg\n'
check "cd rejects extra args"  "too many arguments" "$ERR" err
check "cd extra args rc"       "1" "$RC" rc

run 'type echo nosuchcmd_xyz\n'
check "type resolves each arg"   "shell builtin" "$OUT"
check "type reports the missing" "not found" "$ERR" err
check "type multi-arg rc"        "1" "$RC" rc

# bash refuses to exit on extra args: message, status 1, shell stays alive
run 'exit 1 2\necho still-here\n'
check "exit refuses extra args"  "too many arguments" "$ERR" err
check "exit extra args survives" "still-here" "$OUT"

# ---------- real external execution ----------

# `true` is the closest thing to a universal external; skip where absent
# rather than failing on platforms that don't ship it.
if command -v true >/dev/null 2>&1; then
    run 'true\n'
    check "external runs silent"  "" "$OUT"
    check "external no errors"    "" "$ERR" err
    check "external rc 0"         "0" "$RC" rc
else
    printf 'skip: no `true` on PATH\n'
fi

# Windows-only: bare names resolve via PATHEXT probing. Lookup only, never
# executed — a bare `cmd` with no args would hang waiting on stdin.
if command -v cmd.exe >/dev/null 2>&1; then
    run 'type cmd\n'
    check "bare Windows name resolves" "cmd.exe" "$OUT"
fi
if command -v hostname >/dev/null 2>&1; then
    run 'hostname\n'
    checknot "extensionless lookup works" "command not found" err
fi

# ---------- exit builtin ----------

printf 'exit 3\n' | "$BIN" >/dev/null 2>&1
rc=$?
check "exit propagates status"  "3" "$rc" rc

printf 'exit\n' | "$BIN" >/dev/null 2>&1
rc=$?
check "bare exit is success"    "0" "$rc" rc

# EOF (Ctrl+D) exits cleanly with last status
printf 'echo bye\n' | "$BIN" >/dev/null 2>&1
rc=$?
check "EOF exits with last status" "0" "$rc" rc

# ---------- cleanup any file the redirect test may have created ----------
rm -f f.txt

echo
echo "passed: $pass  failed: $fail"
[ "$fail" -eq 0 ]
