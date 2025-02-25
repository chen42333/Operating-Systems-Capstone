CC = clang
CFLAGS = -mcpu=cortex-a53 --target=aarch64-rpi3-elf
LINKER = ld.lld
LINKFLAGS = -m aarch64elf
LINKERSCRIPT = linker.ld
OBJCPY = llvm-objcopy
OBJCPYFLAGS = --output-target=aarch64-rpi3-elf -O binary

OBJS = kernel8.o shell.o
PROGS = kernel8.img kernel8.elf

EXEC = qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

.PHONY = clean all

all: kernel8.img
test: kernel8.img
	$(EXEC)
kernel8.img: kernel8.elf
	$(OBJCPY) $(OBJCPYFLAGS) $< $@
kernel8.elf: $(OBJS)
	$(LINKER) $(LINKFLAGS) -T $(LINKERSCRIPT) -o $@ $^
kernel8.o: kernel8.S
	$(CC) $(CFLAGS) -c $< -o $@
shell.o: shell.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJS) $(PROGS)