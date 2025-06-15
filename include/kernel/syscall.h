#ifndef __SYSCALL_H
#define __SYSCALL_H

#include "process.h"

#define TRAP_FRAME_REGS 34 // 31 general + 1 dummy + elr_el1 + spsr_el1
#define x(x) regs[x]
#define arg(x) regs[x]
#define RET regs[0]
#define nr_syscall regs[8]
#define ELR_EL1 regs[32]
#define SPSR_EL1 regs[33]

typedef enum syscall {
    GET_PID = 0,
    UART_READ = 1,
    UART_WRITE = 2,
    EXEC = 3,
    FORK = 4,
    EXIT = 5,
    MBOX_CALL = 6,
    KILL = 7,
    SIGNAL = 8, 
    SIGNAL_KILL = 9,
    MMAP = 10,
    OPEN = 11,
    CLOSE = 12,
    WRITE = 13,
    READ = 14,
    MKDIR = 15,
    MOUNT = 16,
    CHDIR = 17,
    LSEEK = 18,
    IOCTL = 19,
    SYNC = 20,
    SIGRET = 21
} syscall;

struct trap_frame {
    size_t regs[TRAP_FRAME_REGS];
};

extern void exec_prog(void *addr, void *stack, void *ttbr);

void syscall_entry(struct trap_frame *frame);
pid_t getpid();
int exec(const char *name, char *const argv[]);
int fork();
void exit();
void kill(pid_t pid);

#endif
