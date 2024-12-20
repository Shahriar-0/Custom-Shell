# Custom Shell

This is a custom shell implementation in C++ that simulates the behavior of a shell like `bash` or `zsh`.

## How to use

To use the custom shell, simply compile the source code and run the resulting executable.

## Features

- [x] Create a simple shell
  - [ ] Basic shell loop
  - [x] Parse user input
  - [x] Execute commands
  - [ ] Handle errors
- [ ] Add navigation
  - [x] `cd` (Change directory)
  - [x] `pwd` (Print working directory)
  - [ ] `ls` (List directory contents)
    - [ ] Handle flags (e.g., `-l`, `-a`)
    - [ ] Handle file sorting
    - [ ] Format output (columns, colors)
  - [ ] `mkdir` (Make directory)
  - [ ] `rmdir` (Remove directory)
- [ ] Add quotes
  - [x] Single quotes (`'`)
  - [x] Double quotes (`"`)
  - [x] Escape characters inside quotes
  - [x] Support for escaping special characters
- [ ] Variable interpolation
  - [ ] `$variable` syntax
  - [ ] Handle special variables like `$?`, `$$`, `$PATH`
  - [ ] Support for environment variables
  - [ ] Handle whitespace around variables
- [ ] IO Redirection
  - [ ] Output redirection (`>`, `>>`)
    - [ ] Overwrite vs append
  - [ ] Input redirection (`<`)
  - [ ] Pipe (`|`)
    - [ ] Multiple pipes
    - [ ] Handle errors from piped commands
- [ ] History
  - [ ] Store command history in a file (e.g., `.bash_history`)
  - [ ] Access history with `history` command
  - [ ] Re-run previous commands with `!number`
  - [ ] Clear history
  - [ ] Search through history (`Ctrl + R`)
- [ ] Job Control
  - [ ] Background processes (`&`)
  - [ ] List background jobs (`jobs`)
  - [ ] Bring jobs to the foreground (`fg`)
  - [ ] Send jobs to the background (`bg`)
  - [ ] Kill processes (`kill`)
  - [ ] Stop processes (`Ctrl + Z`)
  - [ ] Handle job states (running, stopped, etc.)
- [ ] Auto Completion
  - [ ] File path completion (e.g., `Tab` to complete filenames)
  - [ ] Command name completion
  - [ ] History-based completion (e.g., `!command` to complete previous commands)
  - [ ] Handle directory navigation (e.g., `cd` completion)
- [ ] Cursor Positioning
  - [ ] Moving the cursor with arrow keys (up, down, left, right)
  - [ ] Edit mode (backspace, delete, etc.)
  - [ ] Handle cursor positioning with `Ctrl + A` (beginning of line), `Ctrl + E` (end of line)
  - [ ] Handle `Ctrl + U`, `Ctrl + K` (delete line, delete to end of line)
  - [ ] Handle `Ctrl + W` (delete word)
  - [ ] Handle `Ctrl + L` (clear screen)
- [ ] Signal Handling
  - [ ] Handle `SIGINT` (Ctrl + C)
  - [ ] Handle `SIGTSTP` (Ctrl + Z)
  - [ ] Handle `SIGQUIT` (Ctrl + \)
  - [ ] Handle `SIGCHLD` (child process state change)
- [ ] Background Process Management
  - [ ] Wait for background jobs to finish
  - [ ] Capture and display background process output
- [ ] Custom Aliases
  - [ ] Support for creating custom commands (e.g., `alias ll='ls -l'`)
  - [ ] Handle removing aliases (e.g., `unalias`)
- [ ] Command Substitution
  - [ ] `$()` for command substitution
  - [ ] Backticks `` `command` `` for command substitution
- [ ] Error Handling
  - [ ] Return status codes for each command
  - [ ] Display error messages for failed commands
  - [ ] Handle unknown commands gracefully
  - [ ] Use `set -e` to exit on error (optional)
- [ ] Scripting Features
  - [ ] Support for writing shell scripts
  - [ ] Execute shell scripts (`./script.sh`)
  - [ ] Looping constructs (`for`, `while`)
  - [ ] Conditional statements (`if`, `else`, `elif`)
  - [ ] Functions
  - [ ] User-defined functions
- [ ] Advanced Features
  - [ ] Input/output redirection with append (`>>`, `<`)
  - [ ] Conditional execution (`&&`, `||`)
  - [ ] Grouping commands (`{ command1; command2; }`)
  - [ ] Process substitution (`<(command)` and `>(command)`)
  - [ ] `exec` for replacing shell with a command
- [ ] Support for External Commands
  - [ ] External command execution (e.g., `grep`, `echo`)
  - [ ] Search for external commands in `$PATH`
  - [ ] Handling command arguments
- [ ] Debugging Features
  - [ ] Debug mode for scripts
  - [ ] Trace execution (`set -x`)
  - [ ] Print variable values (`set -v`)
