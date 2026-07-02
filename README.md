# Terminal-RI: A Custom C POSIX CLI Interpreter

A lightweight, Unix-like command-line interpreter written completely from scratch in C. This project interfaces directly with low-level kernel routines via the POSIX standard API to execute processes, orchestrate basic file descriptor redirections, construct cross-command process pipes, and catch runtime keyboard signal events dynamically.

---

## 🚀 Key Functional Architecture

### 1. Process Lifecycle Control
* Automatically spawns execution scopes via isolating process branches using the standard kernel sub-call `fork()`.
* Maps input arrays onto system execution search contexts securely with `execvp()`.
* Controls structural child execution blocking inside master runtime threads using synchronous parent wrapper hooks (`wait()` / `waitpid()`).

### 2. Stream Pipeline & Redirection Management
* **Multi-Command Chaining:** Decouples sequence strings into consecutive executable scopes instantly using structural syntax processing via continuous command block splitting token rules.
* **Inter-Process Communications:** Leverages low-level pipeline setups (`pipe()`) to link sequential file descriptors seamlessly (`dup2()`), passing standard text out pipes straight into the input targets of successive application structures.

### 3. Asynchronous Signal Catching
* Replaces standard signal hooks cleanly by utilizing robust struct-based `sigaction` traps.
* Intercepts `SIGINT` (Ctrl+C) actions gracefully to destroy child sub-processes using direct `kill()` system operations while protecting the master engine cycle from termination.

### 4. Efficient History Tracking Stack
* Implements a fixed-capacity ring array layer containing `HISTORY_SIZE = 80` allocation limits.
* Employs memory management algorithms using continuous primitive pointer shifts to automatically drop the oldest commands when matching storage capacity constraints.

---
