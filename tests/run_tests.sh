#!/usr/bin/env bash
# Usage: tests/run_tests.sh
#        SHELL_BIN=build/shell_static.exe tests/run_tests.sh
# Defaults to build/shell.exe, or build/shell if that doesn't exist.

set -u

# On Windows the exe needs ucrt64's runtime DLLs on PATH, otherwise it dies
# with 0xC0000139 and every test fails. No-op elsewhere.
case ":$PATH:" in
    *":/c/msys64/ucrt64/bin:"*) ;;
    *) [ -d /c/msys64/ucrt64/bin ] && export PATH="/c/msys64/ucrt64/bin:$PATH" ;;
esac

cmake --build build
if [ -n "${SHELL_BIN:-}" ]; then
    BIN="$SHELL_BIN"
elif [ -x build/shell.exe ]; then
    BIN=build/shell.exe
else
    BIN=build/shell
fi
if [ ! -x "$BIN" ]; then
    echo "shell binary not found at $BIN — build first (cmake --build build)" >&2
    exit 1
fi

pass=0
fail=0

# run <script>: feeds it to the shell, sets OUT (prompts stripped), ERR, RC, PROMPTS
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

# check <name> <expected> <actual> [out|err|rc]
#   out (default) / err: substring match against $OUT / $ERR ($3 is ignored)
#   rc: exact match of $3 against <expected>
# An empty <expected> means the stream must be empty.
# To compare a transformed value, assign it to OUT/ERR first.
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

# checknot <name> <substring> [out|err]
# Asserts the substring is absent. Use it instead of an empty `check` when
# other output is expected alongside.
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

# pass_if <name> <command...>: passes when the command succeeds.
pass_if() {
    local name="$1"; shift
    if "$@"; then
        pass=$((pass+1))
        printf 'ok   %s\n' "$name"
    else
        fail=$((fail+1))
        printf 'FAIL %s\n' "$name"
    fi
}

# norm_path <path>: backslashes to slashes, lowercased. Only used for the pwd
# test: MSYS bash prints /d/foo, shell.exe prints D:\foo. `pwd -W` gives the
# Windows form; this smooths over slash direction and drive-letter case.
norm_path() {
    printf '%s' "$1" | tr '\\' '/' | tr '[:upper:]' '[:lower:]'
}

# ---------- basic execution ----------

run 'echo hello world\n'
check "echo args"            "hello world" "$OUT"

# The shell inherits our cwd, so compare against it. -P because the shell
# reports the physical path (getcwd), not $PWD.
expected_cwd="$(norm_path "$(pwd -W 2>/dev/null || pwd -P)")"
run 'pwd\n'
OUT="$(norm_path "$OUT")"
check "pwd prints a cwd"    "$expected_cwd" "$OUT"

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

# nosuchcmd_xyz always fails (127), pwd always succeeds. Not using
# true/false: they're external binaries and not on every platform's PATH.
run 'nosuchcmd_xyz || echo fallback\n'
check "|| runs after failure"   "fallback" "$OUT"

run 'pwd || echo fallback\n'
checknot "|| skips after success"  "fallback"

run 'nosuchcmd_xyz && echo skipped\n'
check "&& skips after failure"  "" "$OUT"

run 'pwd && echo yes\n'
check "&& runs after success"   "yes" "$OUT"

# Regression: whether a pipeline runs depends on the connector coming into
# it, not its own trailing one. Mixing that up skips the first command of
# `a || b`.
run 'pwd || echo should-not-run\n'
checknot "first command isn't skipped by its own trailing '||'" "should-not-run"

# cd fails, so the && branch is skipped; the ; branch still runs.
run 'cd /definitely/not/a/real/dir && echo skipped ; echo ran\n'
check    "connector uses real status: ; always runs"    "ran" "$OUT"
checknot "connector uses real status: && branch skipped" "skipped"

# a && X || Y with a failing: X is skipped and || still sees a's failure,
# because a skipped command doesn't change the status.
run 'nosuchcmd_xyz && echo skipped2 || echo shown\n'
check    "&&-then-|| chain: || branch runs"    "shown" "$OUT"
checknot "&&-then-|| chain: && branch skipped" "skipped2"

# A trailing ';' is valid (only |, && and || need a following command).
run 'echo a ;\n'
check "trailing semicolon accepted"    "a" "$OUT"
check "trailing semicolon: no error"   "" "$ERR" err

# ---------- empty quotes are real, empty arguments ----------

run 'echo a "" b\n'
check "empty double quotes keep an arg"  "a  b" "$OUT"

run "echo a '' b\n"
check "empty single quotes keep an arg"  "a  b" "$OUT"

run 'echo ""\n'
check "lone empty quotes: no syntax error" "" "$ERR" err

# ---------- $? / exit status persists between commands ----------

# Bare `exit` reports the last command's status, not a hardcoded 0.
printf 'nosuchcmd_xyz\nexit\n' | "$BIN" >/dev/null 2>&1
rc=$?
check "bare exit uses last status"   "127" "$rc" rc

printf 'nosuchcmd_xyz\npwd\nexit\n' | "$BIN" >/dev/null 2>&1
rc=$?
check "bare exit after later success" "0" "$rc" rc

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
    # REPL survived: a second prompt was printed after the error.
    pass_if "$name: survives" test "${PROMPTS:-0}" -ge 2
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
syntax_error_case "double semicolon"    'echo a ;; echo b\n'
syntax_error_case "semicolon then ampersand" 'echo a ; &\n'
syntax_error_case "ampersand then semicolon" 'echo a &;\n'
syntax_error_case "ampersand then ampersand" 'echo a & &\n'
syntax_error_case "redirect with no command" '> f.txt\n'
syntax_error_case "fd number out of range" 'echo 9999999999>f.txt\n'

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

# '&' is a list separator: `a & b` is valid and applies only to `a`. Nothing
# is really backgrounded yet, so both run (in the foreground, in order).
run 'echo first & echo second\n'
check "mid-line & parses: both lists run"   "second" "$OUT"
check "mid-line & parses: stub notice"      "backgrounding: not implemented yet" "$ERR" err
checknot "mid-line & is not a syntax error" "syntax error" err

# The '&' backgrounds only the list it ends: the 'ran' list gets no notice.
run 'pwd ; echo bg &\n'
checknot "& does not touch the earlier list" "'pwd' will run" err
check    "& applies to the list it ends"     "'echo' will run in the foreground" "$ERR" err

# ---------- redirection parsing: fd numbers and operator placement ----------
# Redirections aren't applied for now, but the stub message reports the
# fd and operator the parser attached, so these pin the parse itself.

run 'echo x > f.txt\n'
check "'>' defaults to fd 1"          "fd 1 > 'f.txt'" "$ERR" err

run 'echo x >> f.txt\n'
check "'>>' defaults to fd 1"         "fd 1 >> 'f.txt'" "$ERR" err

run 'cat < in.txt\n'
check "'<' defaults to fd 0"          "fd 0 < 'in.txt'" "$ERR" err

run 'echo x 2> f.txt\n'
check "2> is fd 2 (spaced target)"    "fd 2 > 'f.txt'" "$ERR" err

run 'echo x 2>f.txt\n'
check "2> is fd 2 (attached target)"  "fd 2 > 'f.txt'" "$ERR" err

run 'echo x 2>>f.txt\n'
check "2>> is fd 2 append"            "fd 2 >> 'f.txt'" "$ERR" err

run 'cat 0<in.txt\n'
check "0< is fd 0"                    "fd 0 < 'in.txt'" "$ERR" err

# A digit word not touching the operator is a normal argument...
run 'echo 2 > f.txt\n'
check "spaced digit is an argument, not an fd" "fd 1 > 'f.txt'" "$ERR" err

# ...and so is a quoted, escaped, or embedded one.
run 'echo "2">f.txt\n'
check "quoted digit is not an fd"     "fd 1 > 'f.txt'" "$ERR" err

run 'echo \\2>f.txt\n'
check "escaped digit is not an fd"    "fd 1 > 'f.txt'" "$ERR" err

run 'echo a2>f.txt\n'
check "digits inside a word are not an fd" "fd 1 > 'f.txt'" "$ERR" err

# Redirections may come before the command name and between its arguments.
run '> f.txt echo hi\n'
check "leading redirection parses"    "fd 1 > 'f.txt'" "$ERR" err

run '2>err.txt echo hi\n'
check "leading fd redirection parses" "fd 2 > 'err.txt'" "$ERR" err

run 'echo a > f.txt b\n'
check "redirection between args parses" "fd 1 > 'f.txt'" "$ERR" err

# The stub also fires for a later pipeline in a && chain.
run 'pwd && echo x > f.txt\n'
check "redirection stub inside && chain" "redirection: not implemented yet" "$ERR" err

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

# "3abc" is an error, not 3; the shell exits with 2, like bash.
run 'exit 3abc\n'
check "exit rejects trailing junk"   "numeric argument required" "$ERR" err
check "exit non-numeric status is 2" "2" "$RC" rc

run 'exit abc\n'
check "exit rejects non-numeric"     "numeric argument required" "$ERR" err

# ---------- real external execution ----------

# Needs a real external; skip if there's no `true` on PATH.
if command -v true >/dev/null 2>&1; then
    run 'true\n'
    check "external runs silent"  "" "$OUT"
    check "external no errors"    "" "$ERR" err
    check "external rc 0"         "0" "$RC" rc
else
    printf 'skip: no `true` on PATH\n'
fi

# Windows: bare names resolve via PATHEXT. Lookup only; a bare cmd would hang on stdin.
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

# ---------- cleanup ----------
rm -f f.txt

echo
echo "passed: $pass  failed: $fail"
[ "$fail" -eq 0 ]
