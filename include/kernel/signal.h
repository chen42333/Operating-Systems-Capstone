#ifndef __SIGNAL_H
#define __SIGNAL_H

#define _NSIG 64
#define SIG_DFL (void(*)())0
#define SIG_IGN (void(*)())1

#define SIGHUP    1   // Hangup detected on controlling terminal or death of controlling process
#define SIGINT    2   // Interrupt from keyboard (Ctrl+C)
#define SIGQUIT   3   // Quit from keyboard
#define SIGILL    4   // Illegal Instruction
#define SIGTRAP   5   // Trace/breakpoint trap
#define SIGABRT   6   // Abort signal from abort()
#define SIGBUS    7   // Bus error (bad memory access)
#define SIGFPE    8   // Floating point exception
#define SIGKILL   9   // Kill signal (cannot be caught or ignored)
#define SIGUSR1   10  // User-defined signal 1
#define SIGSEGV   11  // Invalid memory reference (segmentation fault)
#define SIGUSR2   12  // User-defined signal 2
#define SIGPIPE   13  // Broken pipe: write to pipe with no readers
#define SIGALRM   14  // Timer signal from alarm()
#define SIGTERM   15  // Termination signal
#define SIGCHLD   17  // Child stopped or terminated
#define SIGCONT   18  // Continue if stopped
#define SIGSTOP   19  // Stop process (cannot be caught or ignored)
#define SIGTSTP   20  // Stop typed at terminal (Ctrl+Z)
#define SIGTTIN   21  // Terminal input for background process
#define SIGTTOU   22  // Terminal output for background process

void signal(int signo, void (*handler)());
void signal_kill(int pid, int signo);

#endif