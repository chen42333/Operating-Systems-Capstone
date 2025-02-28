CC = clang
CFLAGS = -mcpu=cortex-a53 --target=aarch64-rpi3-elf \
	-g -Wall -MMD -MP -nostdlib -ffreestanding \
	-mno-unaligned-access -mgeneral-regs-only
LD = ld.lld
LDFLAGS = -m aarch64elf
OBJCPY = llvm-objcopy
OBJCPYFLAGS = --output-target=aarch64-rpi3-elf -O binary
QEMU = qemu-system-aarch64
QEMUFLAGS = -M raspi3b -kernel $(TARGET_FILE) -display none -serial null -serial stdio

TARGET = kernel
TARGET_FILE = kernel8.img

ifeq ($(MAKECMDGOALS),bootloader)
	TARGET = bootloader
	TARGET_FILE = bootloader.img
endif

LIB = lib
SRC_DIR = ./src
SRCS = $(shell find $(SRC_DIR)/$(TARGET) -type f \( -name "*.c" -o -name "*.S" \))
SRCS += $(shell find $(SRC_DIR)/$(LIB) -type f \( -name "*.c" -o -name "*.S" \))
OBJS = $(SRCS:.c=.o)
OBJS := $(OBJS:.S=.o)
DEPS = $(OBJS:.o=.d)
LDFILE = $(SRC_DIR)/$(TARGET)/linker.ld
PROGS = *.elf *.img
JUNK = $(shell find . -type f \( -name "*.o" -o -name "*.d" \))
JUNK += $(PROGS)

CFLAGS += -Iinclude/$(TARGET) -Iinclude/$(LIB)

.PHONY = clean all test debug asm int kernel bootloader

.PRECIOUS: %.elf

all: $(TARGET_FILE)

kernel: $(TARGET_FILE)

bootloader: $(TARGET_FILE)

test: $(TARGET_FILE)
	$(QEMU) $(QEMUFLAGS)

debug: $(TARGET_FILE)
	$(QEMU) $(QEMUFLAGS) -S -s
	
asm: $(TARGET_FILE)
	$(QEMU) $(QEMUFLAGS) -d in_asm

int: $(TARGET_FILE)
	$(QEMU) $(QEMUFLAGS) -d int

%.img: %.elf
	$(OBJCPY) $(OBJCPYFLAGS) $< $@

%.elf: $(OBJS) $(LDFILE)
	$(LD) $(LDFLAGS) -T $(LDFILE) -o $@ $(OBJS)

-include $(DEPS)

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
   
clean:
	rm -f $(JUNK)