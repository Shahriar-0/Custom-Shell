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

# Some tests only make sense on Windows (PATHEXT, handle passing) or only off it.
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) is_windows=1 ;;
    *) is_windows=0 ;;
esac

cmake --build build || exit 1
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
# Absolute, since the redirection tests cd into a scratch dir.
BIN="$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN")"

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

# check <n> <expected> <actual> [out|err|rc]
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

# checknot <n> <substring> [out|err]
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

# pass_if <n> <command...>: passes when the command succeeds.
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

run 'echo a    b\n'
check "runs of spaces collapse"        "a b" "$OUT"

run 'echo "q\\"x"\n'
check "escaped quote inside double quotes" 'q"x' "$OUT"

run 'echo "a\\\\b"\n'
check "escaped backslash inside double quotes" 'a\b' "$OUT"

run 'echo a#b\n'
check "# inside a word is not a comment" "a#b" "$OUT"

run 'echo a #c\n'
checknot "# after whitespace starts a comment" "#c"

run 'echo "#x"\n'
check "quoted # is literal" "#x" "$OUT"

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
syntax_error_case "'>&' with no target" 'echo >&\n'
syntax_error_case "'<&' with no target" 'echo <&\n'
syntax_error_case "'&>' with no target" 'echo &>\n'

# operators carry their column, too
run 'echo a | | b\n'
check "syntax error on an operator reports its column" "(column 10)" "$ERR" err

# error must not kill the session: bad line, then a good one still runs
run 'bad |||\necho still-alive\n'
check "REPL continues after error"  "still-alive" "$OUT"

# ---------- parsed-but-not-implemented stubs ----------

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

# ---------- redirection ----------

# Runs in a scratch dir (the shell inherits our cwd), cleaned up at the end.
scratch=$(mktemp -d)
cd "$scratch" || exit 1

# file_is <name> <path> <printf format>: byte-exact content check.
file_is() {
    local name="$1" path="$2" fmt="$3" got
    if printf "$fmt" | cmp -s - "$path" 2>/dev/null; then
        pass=$((pass+1))
        printf 'ok   %s\n' "$name"
    else
        fail=$((fail+1))
        got=$(cat "$path" 2>&1; printf x)
        printf 'FAIL %s\n     wanted %q, got %q\n' "$name" "$(printf "$fmt")" "${got%x}"
    fi
}

# file_has <n> <path> <substring>
file_has() {
    pass_if "$1" grep -qF -- "$3" "$2"
}

# --- builtins ---

run 'echo hello > o1.txt\n'
file_is "> writes builtin output to the file" o1.txt 'hello\n'
check "> leaves stdout empty" "" "$OUT"
check "> leaves stderr empty" "" "$ERR" err

run 'echo longer-text > o2.txt\necho x > o2.txt\n'
file_is "> truncates" o2.txt 'x\n'

run 'echo a > o3.txt\necho b >> o3.txt\n'
file_is ">> appends" o3.txt 'a\nb\n'

run 'echo c >> o4.txt\n'
file_is ">> creates a missing file" o4.txt 'c\n'

run 'cd /definitely/not/a/real/dir 2> e1.txt\n'
file_has "2> captures a builtin's stderr" e1.txt 'cd:'
check "2> leaves the terminal's stderr empty" "" "$ERR" err

run 'cd /definitely/not/a/real/dir 2> e2.txt\ncd /another/missing 2>> e2.txt\n'
file_has "2>> appends stderr" e2.txt '/another/missing'

run 'echo x > w1.txt > w2.txt\n'
file_is "last redirection of an fd wins (loser still created)" w1.txt ''
file_is "last redirection of an fd wins (winner)" w2.txt 'x\n'

run 'echo hi > "my file.txt"\n'
file_is "quoted filename with a space" "my file.txt" 'hi\n'

run '> l1.txt echo lead\n'
file_is "redirection before the command name" l1.txt 'lead\n'

run 'echo a > m1.txt b\n'
file_is "redirection between arguments" m1.txt 'a b\n'

run 'echo a > sz.txt\n'
pass_if "files are byte-exact, no CRLF" test "$(wc -c < sz.txt | tr -d ' ')" = 2

run 'echo hidden-b > r1.txt\necho shown-b\n'
check "output returns to the terminal afterwards" "shown-b" "$OUT"
checknot "redirected output doesn't leak to the terminal" "hidden-b"

run 'echo a > r2.txt\necho b > r2.txt\necho c\n'
check "repeated redirects keep restoring" "c" "$OUT"
file_is "repeated redirects: file content" r2.txt 'b\n'

run 'echo gone > /dev/null\necho back\n'
check "/dev/null swallows output" "back" "$OUT"
checknot "/dev/null: nothing leaks" "gone"

run 'echo a > c1.txt && echo b >> c1.txt\n'
file_is "redirects inside an && chain" c1.txt 'a\nb\n'

run 'exit 3 > x1.txt\n'
check "exit with a redirect keeps its code" "3" "$RC" rc
file_is "exit's redirect target was created" x1.txt ''

run 'echo bg > bg1.txt &\n'
file_is "redirect on a backgrounded list" bg1.txt 'bg\n'

# --- failures: report, skip the command, status 1, shell stays alive ---

run 'echo xx-ran-xx > nodir/f.txt || echo fallback\n'
check "unwritable target: || sees the failure" "fallback" "$OUT"
checknot "unwritable target: command didn't run" "xx-ran-xx"
check "unwritable target: message names the file" "myshell: nodir/f.txt:" "$ERR" err

run 'echo x > nodir/f.txt && echo skipped-and\n'
checknot "failed redirect makes && skip" "skipped-and"

run 'echo x > nodir/f.txt\nexit\n'
check "failed redirect: status 1" "1" "$RC" rc

run 'echo x > fr1.txt 2> nodir/e.txt\necho still-here\n'
check "failure restores earlier redirects" "still-here" "$OUT"
file_is "failure keeps files opened before it" fr1.txt ''

# bash reports through the redirections applied so far.
run 'echo x 2> log1.txt < nodir/in.txt\n'
file_has "complaint goes to the already-redirected stderr" log1.txt 'nodir/in.txt'
check "...and not to the terminal" "" "$ERR" err

run 'echo x > .\n'
check "directory as target is refused" "myshell: .:" "$ERR" err

run 'nosuchcmd_xyz 2> nf1.txt\nexit\n'
file_has "not-found message follows 2>" nf1.txt 'command not found'
check "not-found: terminal stderr is empty" "" "$ERR" err
check "not-found: status 127" "127" "$RC" rc

run 'nosuchcmd_xyz > nf2.txt\n'
file_is "not-found still creates the > target" nf2.txt ''
check "not-found with > still complains" "command not found" "$ERR" err

run 'echo hi >&-\necho alive\n'
check "write to a closed stdout: message" "echo: write error" "$ERR" err
check "write to a closed stdout: shell recovers" "alive" "$OUT"

run 'echo hi >&-\nexit\n'
check "write to a closed stdout: status 1" "1" "$RC" rc

# --- duplication: >& <& &> &>> ---

run 'echo toerr >&2\n'
check "builtin stdout to stderr" "toerr" "$ERR" err
check "builtin stdout to stderr: stdout empty" "" "$OUT"

run 'cd /definitely/not/a/real/dir > d1.txt 2>&1\n'
file_has "builtin: > then 2>&1" d1.txt 'cd:'
check "builtin: > then 2>&1 leaves the terminal quiet" "" "$ERR" err

run 'echo hi &> d2.txt\n'
file_is "&> works for builtins" d2.txt 'hi\n'

run 'echo a &> d3.txt\necho b &>> d3.txt\n'
file_is "&>> appends" d3.txt 'a\nb\n'

run 'echo x >&9\nexit\n'
check "dup from a closed fd: message" "9: Bad file descriptor" "$ERR" err
check "dup from a closed fd: status 1" "1" "$RC" rc

run 'echo x >&abc\n'
check "dup target must be a number or '-'" "abc: file descriptor number or '-' expected" "$ERR" err

# fds apply left to right: 3 isn't open yet when >&3 runs.
run 'echo x >&3 3> h0.txt\n'
check "dup from an fd opened later fails" "3: Bad file descriptor" "$ERR" err

# Builtins run in the shell, so any fd works. 10 is where the shell starts
# numbering its own backup copies: a command using 10 must not clobber them.
run 'echo viaten 10> h1.txt >&10\n'
file_is "10> then >&10" h1.txt 'viaten\n'
run 'echo viathree 3> h2.txt >&3\n'
file_is "3> then >&3" h2.txt 'viathree\n'
run 'echo a 3> h3.txt >&3\necho b\n'
check "high fd redirect is undone" "b" "$OUT"

# --- externals (need sh and cat) ---

if command -v sh >/dev/null 2>&1 && command -v cat >/dev/null 2>&1; then
    run "sh -c 'echo ext' > e3.txt\n"
    file_is "> works for an external command" e3.txt 'ext\n'
    check "external > leaves stdout empty" "" "$OUT"

    run "sh -c 'echo oops >&2' 2> e4.txt\n"
    file_is "2> captures an external's stderr" e4.txt 'oops\n'
    check "external 2> leaves the terminal's stderr empty" "" "$ERR" err

    run "sh -c 'echo out; echo err >&2' > e5.out 2> e5.err\n"
    file_is "stdout and stderr to separate files (out)" e5.out 'out\n'
    file_is "stdout and stderr to separate files (err)" e5.err 'err\n'

    run "sh -c 'echo out; echo err >&2' 2>&1 > e6.txt\n"
    file_is "2>&1 before >: only stdout in the file" e6.txt 'out\n'
    # stderr became a copy of the old stdout, which is the terminal's here.
    check "2>&1 before >: stderr went to the old stdout" "err" "$OUT"

    run "sh -c 'echo out; echo err >&2' > e7.txt 2>&1\n"
    file_is "> then 2>&1 merges both into the file" e7.txt 'out\nerr\n'

    run "sh -c 'echo viaerr' >&2\n"
    check "external >&2" "viaerr" "$ERR" err

    run "sh -c 'echo out; echo err >&2' &> e8.txt\n"
    file_is "&> with an external" e8.txt 'out\nerr\n'

    run "sh -c 'echo x' &> e9.txt\nsh -c 'echo y' &>> e9.txt\n"
    file_is "&>> appends from an external" e9.txt 'x\ny\n'

    run "sh -c 'echo x' > e10.txt\nsh -c 'echo y' >> e10.txt\n"
    file_is ">> appends from an external" e10.txt 'x\ny\n'

    run 'echo data > in1.txt\ncat < in1.txt\n'
    check "< feeds a file to stdin" "data" "$OUT"

    run 'cat 0< in1.txt\n'
    check "0< is stdin" "data" "$OUT"

    run '< in1.txt cat\n'
    check "leading < works" "data" "$OUT"

    run 'cat < in1.txt > e11.txt\n'
    file_is "< and > together" e11.txt 'data\n'

    run 'cat < /dev/null\n'
    check "/dev/null reads as empty" "" "$OUT"

    run 'sh -c "echo hidden-x" > r3.txt\necho shown-x\n'
    check "external redirect leaves the shell alone" "shown-x" "$OUT"
    checknot "external redirect doesn't leak" "hidden-x"

    run 'cat < missing.txt\nexit\n'
    check "missing input file: message" "myshell: missing.txt:" "$ERR" err
    check "missing input file: status 1" "1" "$RC" rc

    run "sh -c 'echo xx-ext-xx' > nodir/f.txt\nexit\n"
    checknot "unwritable target: external didn't run" "xx-ext-xx"
    check "unwritable target: status 1" "1" "$RC" rc

    run 'cat <&-\nexit\n'
    pass_if "reading from a closed stdin fails" test "$RC" -ne 0

    # The shell's own backup fds must not leak into the program.
    run "sh -c 'echo x >&10' > lk.txt 2> /dev/null\nexit\n"
    pass_if "saved fds don't leak into children" test "$RC" -ne 0

    # --- running externals: arguments, status, lookup ---

    cat > showargs.sh <<'EOF'
for a in "$@"; do printf '[%s]' "$a"; done
EOF
    run 'sh showargs.sh "a b" "" c\n'
    check "arguments keep their boundaries" "[a b][][c]" "$OUT"

    run 'sh showargs.sh a\\ b c\n'
    check "escaped space stays in one argument" "[a b][c]" "$OUT"

    run "sh showargs.sh 'say \"hi\"'\n"
    check "embedded double quotes survive" '[say "hi"]' "$OUT"

    run "sh showargs.sh 'x\\\\y\\\\'\n"
    check "backslashes survive, even a trailing one" '[x\y\]' "$OUT"

    run "sh -c 'exit 7'\nexit\n"
    check "external exit status is passed on" "7" "$RC" rc

    if [ "$is_windows" = 0 ]; then
        run "sh -c 'kill -9 \$\$'\nexit\n"
        check "external killed by a signal: 128+N" "137" "$RC" rc
    fi

    cat_src=$(command -v cat)
    cat_ext=""
    [ -f "$cat_src.exe" ] && cat_ext=.exe
    mkdir -p pdir
    cp "$cat_src$cat_ext" "pdir/mycat$cat_ext" && chmod +x "pdir/mycat$cat_ext"

    # PATH comes from the environment, so give the shell a different one.
    OUT=$(printf 'mycat < in1.txt\ntype mycat\n' | PATH="$PWD/pdir:$PATH" "$BIN" 2>&1 | sed 's/\$ //g')
    check "a command found through PATH runs" "data" "$OUT"
    check "type shows the PATH hit" "mycat is" "$OUT"

    run './pdir/mycat < in1.txt\n'
    check "relative path with a slash runs" "data" "$OUT"

    sh_abs=$(cygpath -m "$(command -v sh)" 2>/dev/null || command -v sh)
    run "$sh_abs -c 'echo viapath'\n"
    check "absolute path runs" "viapath" "$OUT"

    run './no-such-prog\n'
    check "missing path: message" "command not found" "$ERR" err
    check "missing path: status 127" "127" "$RC" rc

    run 'type sh\n'
    check "type finds an external" "sh is " "$OUT"

    if [ "$is_windows" = 1 ]; then
        # Only fds 0-2 can be handed to a Windows child.
        run "sh -c 'echo three >&3' 3> h4.txt\nexit\n"
        check "external: fd 3 is refused on Windows" "not supported" "$ERR" err
        check "external: fd 3 refusal is status 1" "1" "$RC" rc
    else
        run "sh -c 'echo three >&3' 3> h4.txt\n"
        file_is "external can write to fd 3" h4.txt 'three\n'
    fi
else
    printf 'skip: no sh/cat on PATH, external redirection tests\n'
fi

# --- cd and ~ (in the scratch dir) ---

mkdir -p cd1/inner
abs="$(pwd -W 2>/dev/null || pwd -P)"

run 'cd cd1\npwd\n'
OUT="$(norm_path "$OUT")"
check "cd into a relative dir" "/cd1" "$OUT"

run 'cd cd1/inner\ncd ..\npwd\n'
OUT="$(norm_path "$OUT")"
check "cd .. goes up" "/cd1" "$OUT"
checknot "cd .. left the inner dir" "inner"

run "cd '$abs/cd1'\npwd\n"
OUT="$(norm_path "$OUT")"
check "cd to an absolute path" "/cd1" "$OUT"

run 'cd nosuch-dir\nexit\n'
check "cd to a missing dir: message" "No such file or directory" "$ERR" err
check "cd to a missing dir: status 1" "1" "$RC" rc

# Home is HOME on POSIX and USERPROFILE on Windows; set both.
home_run() {
    printf '%b' "$1" | HOME="$(pwd -P)" USERPROFILE="$abs" "$BIN" 2>&1 | sed 's/\$ //g'
}
OUT="$(norm_path "$(home_run 'cd ~/cd1\npwd\n')")"
check "~ expands to the home dir" "/cd1" "$OUT"

OUT="$(norm_path "$(home_run 'cd cd1\ncd\npwd\n')")"
check "bare cd goes home" "$(norm_path "$abs")" "$OUT"
checknot "bare cd left the previous dir" "cd1"

cd - >/dev/null || exit 1
rm -rf "$scratch"

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

run 'exit 256\n'
check "exit status wraps to 8 bits" "0" "$RC" rc

run 'exit 257\n'
check "exit 257 wraps to 1" "1" "$RC" rc

run 'exit -1\n'
check "exit -1 is 255" "255" "$RC" rc

run 'help\n'
check "help lists echo" "echo" "$OUT"
check "help lists cd" "cd" "$OUT"

run 'clear\n'
check "clear sends the ANSI clear sequence" $'\033[2J' "$OUT"

run 'type\n'
check "type with no args prints usage" "Usage: type" "$OUT"
check "type with no args: status 1" "1" "$RC" rc

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

# Windows only: bare names resolve via PATHEXT. `type` is lookup only; running
# a bare cmd would hang on stdin.
if [ "$is_windows" = 1 ]; then
    if command -v cmd.exe >/dev/null 2>&1; then
        run 'type cmd\n'
        check "bare Windows name resolves" "cmd.exe" "$OUT"
    fi
    if command -v hostname >/dev/null 2>&1; then
        run 'hostname\n'
        checknot "extensionless lookup works" "command not found" err
    fi
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

echo
echo "passed: $pass  failed: $fail"
[ "$fail" -eq 0 ]
