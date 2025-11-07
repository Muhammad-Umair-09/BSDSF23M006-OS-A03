# MyShell: Feature-wise Implementation Details

## 🧩 Overview
This document provides a detailed description of all **eight implemented features** in the custom shell `MyShell`, as part of the OS Assignment. Each feature builds upon the previous one to extend shell functionality, job control, process management, and now variable handling.

---

## ⚙️ **Feature 1: Basic Shell Execution**
### Description
Implements the core functionality of the shell: reading, parsing, and executing commands.

### Key Functionalities
- Reads command input using a loop.
- Parses arguments using tokenization (`strtok`).
- Executes external commands using `fork()` and `execvp()`.
- Waits for command completion using `waitpid()`.

---

## 🧱 **Feature 2: Makefile Integration**
### Description
Automates the compilation and linking process using a parent and child Makefile system.

### Key Functionalities
- **Parent Makefile:** Defines build rules and dependencies.
- **Child Makefile:** Handles compilation for specific directories (e.g., `src`, `obj`).
- Supports `make`, `make run`, and `make clean` commands.

---

## 🧮 **Feature 3: Internal Commands**
### Description
Implements built-in commands within the shell process.

### Internal Commands
- `cd <dir>` → Change directory.
- `exit` → Exit the shell.
- `pwd` → Print working directory.
- `help` → Display available commands.

---

## 🏃 **Feature 4: Background Execution**
### Description
Allows users to execute processes in the background using `&` at the end of a command.

### Example
```bash
sleep 10 &
```
Output:
```
[BG] Started job with PID 2341: sleep
```

---

## 📋 **Feature 5: Job Control (jobs, fg, bg)**
### Description
Implements job tracking, resuming, and foreground/background switching.

### Example
```bash
sleep 100 &
jobs
fg %1
bg %1
```

---

## 🧠 **Feature 6: Signal Handling**
### Description
Handles signals like `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z) for both foreground and background jobs.

### Example Behavior
```bash
Umair> sleep 1000
^Z
[1]+  Stopped  sleep 1000
```

---

## 🧰 **Feature 7: Robust Job Management & Synchronization**
### Description
Improves job consistency and prevents incorrect job recording (e.g., `make run` showing multiple times).

### Example Output
```bash
Umair> sleep 100 &
[1] Started job with PID 4203: sleep
Umair> jobs
[1]+  Running  sleep
Umair> fg %1
sleep 100
Umair> jobs
[1]+  Done  sleep
```

---

## 💡 **Feature 8: Shell Variables and Expansion**
### Description
Introduces variable assignment, expansion, and the `set` command—allowing users to store and reuse values directly inside the shell.

### Key Functionalities
- **Variable Assignment:**  
  Supports setting variables like:
  ```bash
  Umair> msg="hello, world"
  ```
  The shell stores `msg` in a hash map or array structure.
  
- **Variable Expansion:**  
  Expands `$variable` to its value during command parsing:
  ```bash
  Umair> echo $msg
  hello, world
  ```
  
- **`set` Command:**  
  Displays all currently defined shell variables and their values.
  ```bash
  Umair> set
  msg="hello, world"
  PATH="/usr/bin"
  ```

- **Memory Handling:**  
  Ensures variable values are dynamically allocated and freed safely on exit.

### Implementation Highlights
- Added `var_table` structure to store key-value pairs.
- Integrated expansion logic in the command parsing stage.
- Modified `execute.c` and `main.c` to handle assignments before `fork()`.
- Supported quotes and spaces inside variable values.

### Example Usage
```bash
Umair> user="Umair"
Umair> greet="Hello, $user!"
Umair> echo $greet
Hello, Umair!
```

---

## ✅ **Summary**

| Feature | Description | Key System Calls / Components |
|----------|--------------|-------------------------------|
| 1 | Basic command execution | `fork`, `execvp`, `waitpid` |
| 2 | Makefile automation | `make` |
| 3 | Internal shell commands | `chdir`, `getcwd` |
| 4 | Background jobs | `setpgid` |
| 5 | Job control (`jobs`, `fg`, `bg`) | `waitpid`, `kill` |
| 6 | Signal handling | `SIGINT`, `SIGTSTP` |
| 7 | Job management robustness | `update_job_status`, `WUNTRACED` |
| 8 | Shell variables & expansion | `set`, `getenv`, custom var table |

---

### 🧑‍💻 Author
**Muhammad Umair (BSDSF23M006)**  
PUCIT – Operating Systems Assignment 3
