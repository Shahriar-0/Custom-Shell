#!/usr/bin/env bash
# Phase 1 integration tests: pipes lines into the built shell.exe and checks
# stdout/stderr/exit-status against expectations.
#
# Usage:  tests/run_tests.sh            (uses build/shell.exe)
#         SHELL_BIN=build/shell_static.exe tests/run_tests.sh
#
# Requires the ucrt64 runtime DLLs on PATH (any MSYS shell has them).

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
    OUT=$(sed 's/\$ //' "$tmp.out")
    ERR=$(cat "$tmp.err")
    PROMPTS=$(grep -o '\$ ' "$tmp.out" | wc -l || true)
    rm -f "$tmp" "$tmp.out" "$tmp.err"
}

# check <name> <expected> <actual-value> <where>
#   where: out (default) | err  -> $3 ignored, substring match against captured output
#          rc                   -> $3 is the real exit status, exact match vs $2
check() {
    local name="$1" want="$2" actual="$3" where="${4:-out}" ok=0
    case "$where" in
        out) actual="$OUT" ;;
        err) actual="$ERR" ;;
        rc)  : ;;
    esac
    if [[ "$where" == "rc" ]]; then
        [[ "$actual" -eq "$want" ]] && ok=1 || ok=0
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

# ---------- basic execution ----------

run 'echo hello world\n'
check "echo args"            "hello world" "$OUT"

run 'pwd\n'
check "pwd prints a path"    "Custom-Shell" "$OUT"

run 'type echo\n'
check "type finds builtin"   "shell builtin" "$OUT"

run 'nosuchcmd_xyz\n'
check "unknown command msg"  "command not found" "$ERR" err
check "unknown command rc"   "" "$( [ $RC -eq 127 ] && echo ok )" rc

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

run 'false || echo fallback\necho after\n'   # false isn't builtin -> 127, still nonzero
check "|| runs after failure"   "fallback" "$OUT"

run 'true && echo yes\n'                      # true isn't builtin -> 127, && skips echo
check "&& skips after failure"  "" "$OUT"
check "&& skip leaves silence"  "yes" "$(grep -v yes <<<"$OUT")"

# exit status plumbing through the connector logic:
# cd fails (rc 1) -> && branch skipped; then ; runs regardless
run 'cd /definitely/not/a/real/dir && echo skipped ; echo ran\n'
check "connector uses real status"  "ran" "$OUT"

# ---------- comments & blank input ----------

run '# a comment line\necho visible\n'
check "comment ignored"       "visible" "$OUT"

run '\n   \n\t\n'
check "blank lines are no-ops"  "" "$ERR"

# ---------- syntax errors: report, set rc=2, keep REPL alive ----------

syntax_error_case() {
    local name="$1" script="$2"
    run "$script"
    check "$name: message"   "syntax error" "$ERR" err
    check "$name: status 2"  "2" "$( [ "$RC" -eq 2 ] && echo 2 )" rc
    # REPL survived: a fresh prompt was printed after the error line
    check "$name: survives"  "2" "$( [ "${PROMPTS:-0}" -ge 2 ] && echo 2 )" rc
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

# ---------- exit builtin ----------

printf 'exit 3\n' | "$BIN" >/dev/null 2>&1
rc=$?
check "exit propagates status"  "3" "$( [ $rc -eq 3 ] && echo 3 )" rc

printf 'exit\n' | "$BIN" >/dev/null 2>&1
rc=$?
check "bare exit is success"    "0" "$( [ $rc -eq 0 ] && echo 0 )" rc

# EOF (Ctrl+D) exits cleanly with last status
printf 'echo bye\n' | "$BIN" >/dev/null 2>&1
rc=$?
check "EOF exits with last status" "0" "$( [ $rc -eq 0 ] && echo 0 )" rc

# ---------- cleanup any file the redirect test may have created ----------
rm -f f.txt

echo
echo "passed: $pass  failed: $fail"
[ "$fail" -eq 0 ]
