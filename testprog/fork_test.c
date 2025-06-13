#include <stddef.h>
#include <stdarg.h>

#define SYS_uartwrite 2

void printf(const char *fmt, ...);
int getpid();
int fork();
void exit();

int main() {
    printf("\nFork Test, pid %d\n", getpid());
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        ++cnt;

        if ((ret = fork()) != 0) {
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        }
        else {
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
                for (int i = 0; i < 1000000; i++)
                    asm volatile("nop");
                ++cnt;
            }
        }
        exit();
    }
    else {
        printf("parent here, pid %d, child %d\n", getpid(), ret);
    }

    exit();
    __builtin_unreachable(); // prevent "no return" warning
}

// System call wrapper for uartwrite
size_t uartwrite(const char *buf, size_t size) {
    register const char *x0 asm("x0") = buf;
    register size_t x1 asm("x1") = size;
    register int x8 asm("x8") = SYS_uartwrite;
    asm volatile("svc 0"
                 : "+r"(x0)
                 : "r"(x1), "r"(x8)
                 : "memory");
    return (size_t)x0;
}

void printchar(char c) {
    uartwrite(&c, 1);
}

void printstr(const char *s) {
    while (*s) {
        printchar(*s++);
    }
}

void printdec(int val) {
    char buf[20];
    int i = 0;
    if (val < 0) {
        printchar('-');
        val = -val;
    }
    if (val == 0) {
        printchar('0');
        return;
    }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i--) {
        printchar(buf[i]);
    }
}

void printhex(unsigned int val) {
    const char *hex = "0123456789abcdef";
    char buf[8];
    int i = 0;
    if (val == 0) {
        printchar('0');
        return;
    }
    while (val > 0) {
        buf[i++] = hex[val & 0xf];
        val >>= 4;
    }
    printstr("0x");
    while (i--) {
        printchar(buf[i]);
    }
}

void printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (; *fmt; fmt++) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 'd') {
                int d = va_arg(args, int);
                printdec(d);
            } else if (*fmt == 'x') {
                unsigned int x = va_arg(args, unsigned int);
                printhex(x);
            } else if (*fmt == 's') {
                char *s = va_arg(args, char *);
                printstr(s);
            } else if (*fmt == '%') {
                printchar('%');
            } else {
                // Unknown format, just print it raw
                printchar('%');
                printchar(*fmt);
            }
        } else {
            printchar(*fmt);
        }
    }

    va_end(args);
}

int getpid() {
    register int ret __asm__("x0");
    register int syscall_id __asm__("x8") = 0;

    __asm__ volatile (
        "svc #0"
        : "=r"(ret)
        : "r"(syscall_id)
        : "memory"
    );
    return ret;
}

int fork() {
    register int ret __asm__("x0");
    register int syscall_id __asm__("x8") = 4;

    __asm__ volatile (
        "svc #0"
        : "=r"(ret)
        : "r"(syscall_id)
        : "memory"
    );
    return ret;
}

void exit() {
    register int syscall_id __asm__("x8") = 5;

    __asm__ volatile (
        "svc #0"
        :
        : "r"(syscall_id)
        : "memory"
    );
    __builtin_unreachable(); // prevent "no return" warning
}