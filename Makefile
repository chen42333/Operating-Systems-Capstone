CC = clang
CFLAGS = -mcpu=cortex-a53 --target=aarch64-rpi3-elf
LINKER = ld.lld
LINKFLAGS = -m aarch64elf
LINKFILE = linker.ld
OBJCPY = llvm-objcopy
OBJCPYFLAGS = --output-target=aarch64-rpi3-elf -O binary

OBJS = kernel8.o shell.o uart.o mailbox.o
PROGS = kernel8.img kernel8.elf

EXEC = qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

.PHONY = clean all test asm

all: kernel8.img

test: all $(OBJS)
	$(EXEC)

debug: all $(OBJS)
	$(EXEC) -S -s
	
asm: all $(OBJS)
	$(EXEC) -d in_asm

kernel8.img: kernel8.elf $(OBJS)
	$(OBJCPY) $(OBJCPYFLAGS) $< $@

kernel8.elf: $(OBJS)
	$(LINKER) $(LINKFLAGS) -T $(LINKFILE) -o $@ $^

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
   
clean:
	rm -f $(OBJS) $(PROGS)