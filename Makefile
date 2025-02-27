CC = clang
CFLAGS = -mcpu=cortex-a53 --target=aarch64-rpi3-elf \
	-g -Wall -MMD -MP -nostdlib -ffreestanding \
	-mno-unaligned-access -mgeneral-regs-only
LD = ld.lld
LDFLAGS = -m aarch64elf
LDFILE = linker.ld
OBJCPY = llvm-objcopy
OBJCPYFLAGS = --output-target=aarch64-rpi3-elf -O binary

SRCS = kernel8.S shell.c uart.c mailbox.c
OBJS = $(SRCS:.c=.o) $(SRCS:.S=.o)
DEPS = $(OBJS:.o=.d)
PROGS = kernel8.img kernel8.elf

QEMU = qemu-system-aarch64
QEMUFLAGS = -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

.PHONY = clean all test debug asm int

all: kernel8.img

test: kernel8.img
	$(QEMU) $(QEMUFLAGS)

debug: kernel8.img
	$(QEMU) $(QEMUFLAGS) -S -s
	
asm: kernel8.img
	$(QEMU) $(QEMUFLAGS) -d in_asm

int: kernel8.img
	$(QEMU) $(QEMUFLAGS) -d int

kernel8.img: kernel8.elf
	$(OBJCPY) $(OBJCPYFLAGS) $< $@

kernel8.elf: $(OBJS)
	$(LD) $(LDFLAGS) -T $(LDFILE) -o $@ $^

-include $(DEPS)

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
   
clean:
	rm -f $(OBJS) $(PROGS)