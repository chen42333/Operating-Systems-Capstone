#include "process.h"
#include "mem.h"
#include "syscall.h"
#include "signal.h"
#include "vmem.h"
#include "dev.h"

pid_t last_pid = 0;
struct list ready_queue, dead_queue, wait_queue[_LAST];
struct pcb_t *pcb_table[MAX_PROC];
void (*default_sig_handler[_NSIG])() =  {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, exit, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

void ps() {
    for (int i = 0; i < MAX_PROC; i++)
    {
        struct pcb_t *pcb = pcb_table[i];

        if (pcb)
        {
            switch (pcb->state)
            {
                case RUN:
                case READY:
                    printf("%d(R) ", i);
                    break;
                case WAIT:
                    printf("%d(S) ", i);
                    break;
                case DEAD:
                    printf("%d(Z) ", i);
                    break;
                default:
                    printf("%d(?) ", i);
                    break;
            }
        }
    }
    printf("\r\n");
}

static int init_uart_fd(struct file *fd_table[], uint16_t *fd_bitmap) {
    struct file *f;

    if (vfs_open(DEV_UART, O_RDWR, &f) < 0)
        return -1;

    fd_table[STDIN] = f;
    fd_table[STDOUT] = f;
    fd_table[STDERR] = f;
    *fd_bitmap = (1ULL << STDIN) | (1ULL << STDOUT) | (1ULL << STDERR);

    return 0;
}

void init_pcb() {
    struct pcb_t *pcb;

    pcb = calloc(1, sizeof(struct pcb_t));
    pcb->pid = 0;
    pcb->pc = idle;
    pcb->state = RUN;
    pcb->el = 1;
    pcb->pstate = EL1H_W_DAIF;
    asm volatile("mrs %0, ttbr1_el1" : "=r"(pcb->ttbr));
    pcb->stack[1] = (uint8_t*)_estack;
    pcb->sp = (uintptr_t)_estack;
    asm volatile ("mov %0, x29" : "=r"(pcb->fp));
    pcb->lr = (uintptr_t)exit;
    pcb->cur_dir = rootfs->root;
    memset(pcb->fd_table, 0, sizeof(pcb->fd_table));
    pcb->fd_bitmap = 0;
    init_uart_fd(pcb->fd_table, &pcb->fd_bitmap);
    for (int i = 0; i < _NSIG; i++)
        pcb->sig_handler[i] = SIG_DFL;

    pcb_table[0] = pcb;
    set_current(pcb);
}

void free_init_pcb() {
    free((void*)pcb_table[0]);
}

pid_t thread_create(void (*func)(void *args), void *args) {
    struct pcb_t *pcb;
    pid_t pid;

    disable_int();

    for (pid = (last_pid + 1) % MAX_PROC; pcb_table[pid] != NULL; pid++)
    {
        if (pid == last_pid)
        {
            enable_int();
            err("Cannot create more threads\r\n");
            return -1;
        }
    }

    pcb = calloc(1, sizeof(struct pcb_t));
    pcb->pid = pid;
    pcb->pc = func;
    pcb->args = args;
    pcb->state = READY;
    pcb->el = 1;
    pcb->pstate = EL1H_W_DAIF;
    asm volatile("mrs %0, ttbr1_el1" : "=r"(pcb->ttbr));
    pcb->stack[1] = malloc(STACK_EL1_SIZE) + STACK_EL1_SIZE;
    pcb->sp = (uint64_t)pcb->stack[1];
    pcb->fp = (uint64_t)pcb->stack[1];
    pcb->lr = (uintptr_t)exit;
    pcb->cur_dir = rootfs->root;
    pcb->fd_bitmap = 0;
    memset(pcb->fd_table, 0, sizeof(pcb->fd_table)); // STDIN, STDOUT, STDERR will only be initialized at INIT
    for (int i = 0; i < _NSIG; i++)
        pcb->sig_handler[i] = SIG_DFL;

    pcb_table[pid] = pcb;
    last_pid = pid;

    list_push(&ready_queue, pcb);

    enable_int();

    return pid;
}

void switch_to_next(struct pcb_t *prev) {
    struct pcb_t *next = list_pop(&ready_queue);

    disable_int();

    prev->pc = &&out;

    next->state = RUN;
    set_current(next);

    if (next->el == 0)
        asm volatile ("msr sp_el0, %0" :: "r"(next->sp_el0));
    
    if (list_empty(&next->signal_queue))
    {
        enable_int();
        switch_to(prev->reg, next->reg, next->pc, next->pstate, next->args, next->ttbr);
    } else
    {
        int *signo_ptr = list_pop(&next->signal_queue);
        int signo = *signo_ptr;
        void (*handler)() = next->sig_handler[signo];
        void *frame_ptr, *stack_ptr;

        free(signo_ptr);
        asm volatile ("mov %0, fp": "=r"(frame_ptr));
        asm volatile ("mov %0, sp": "=r"(stack_ptr));
        memcpy(next->reg_backup, next->reg, sizeof(next->reg));
        next->sp_el0_backup = next->sp_el0;

        if (handler == SIG_DFL)
        {
            handler = default_sig_handler[signo];
            save_regs(prev->reg, frame_ptr, &&out, stack_ptr);
            next->lr = (uint64_t)sigreturn;

            enable_int();

            load_regs(next->reg, handler, EL1H_W_DAIF, NULL, next->ttbr);

        } else if (handler == SIG_IGN) {
            enable_int();
            switch_to(prev->reg, next->reg, next->pc, next->pstate, next->args, next->ttbr);
        } else {
            save_regs(prev->reg, frame_ptr, &&out, stack_ptr);
            next->lr = (uint64_t)_sigreturn;

            enable_int();
            
            load_regs(next->reg, handler, EL0_W_DAIF, NULL, next->ttbr);
        }
    }

out:
    ; // the following will do the restoration of fp and lr
}

void schedule() {
    struct pcb_t *prev;

    if (list_empty(&ready_queue))
        return;

    disable_int();

    prev = get_current();
    prev->state = READY;
    list_push(&ready_queue, prev);

    // It is at EL1 currently, so it doesn't need to save/restore it additionally if prev->el == 1
    if (prev->el == 0)
        asm volatile("mrs %0, sp_el0" : "=r"(prev->sp_el0));
    prev->pstate = EL1H_W_DAIF;

    enable_int();

    switch_to_next(prev);
}

void _exit() {
    register int syscall_id __asm__("x8") = EXIT;
    asm volatile ("svc #0":: "r"(syscall_id): "memory");
}

void add_section(struct pcb_t *pcb, sec type, void *base, size_t len) {
    struct section *s;

    s = malloc(sizeof(struct section));
    s->type = type;
    s->base = base;
    s->size = len;
    list_push(&pcb->sections, s);
}

void free_sections(struct list *sections, void *ttbr) {
    struct section *s = list_pop(sections);

    while (s)
    {
        void *base = v2p_trans(s->base, ttbr);
        for (size_t i = 0; i < s->size; i += PAGE_SIZE)
            deref_page(base + i);
        
        s = list_pop(sections);
    }
}

bool in_section(void *ptr, void *addr) {
    struct section *s = ptr;

    if (addr >= s->base && addr < s->base + s->size)
        return true;
    
    return false;
}

static void kill_zombies() {
    struct pcb_t *pcb = list_pop(&dead_queue);

    while (pcb)
    {
        free_sections(&pcb->sections, pcb->ttbr);
        if (pcb->el == 0)
            free_page_table(pcb->ttbr, PGD);
        free(pcb->stack[1] - STACK_EL1_SIZE);
        pcb_table[pcb->pid] = NULL;
        free(pcb);
        pcb = list_pop(&dead_queue);
    }
}

void idle() {
    while (true)
    {
        kill_zombies();
        schedule();
    }
}

void wait(event e, size_t data) {
    struct pcb_t *pcb = get_current();

    disable_int();

    pcb->wait_data = data;
    pcb->state = WAIT;
    pcb->wait_q = &wait_queue[e];
    list_push(&wait_queue[e], pcb);

    // It is at EL1 currently, so it doesn't need to save/restore it additionally if prev->el == 1
    if (pcb->el == 0)
        asm volatile("mrs %0, sp_el0" : "=r"(pcb->sp_el0));
    pcb->pstate = EL1H_W_DAIF;

    enable_int();

    if (list_empty(&ready_queue))
        return;

    switch_to_next(pcb);
}

void wait_to_ready(void *ptr) {
    struct pcb_t *pcb = (struct pcb_t*)ptr;
    
    disable_int();

    pcb->wait_data = 0;
    pcb->state = READY;
    pcb->wait_q = NULL;
    list_push(&ready_queue, pcb);

    enable_int();
}