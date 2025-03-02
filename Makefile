CC = clang
CFLAGS = -mcpu=cortex-a53 --target=aarch64-rpi3-elf \
	-g -Wall -MMD -MP -nostdlib -ffreestanding \
	-mno-unaligned-access -mgeneral-regs-only
LD = ld.lld
LDFLAGS = -m aarch64elf
OBJCPY = llvm-objcopy
OBJCPYFLAGS = --output-target=aarch64-rpi3-elf -O binary
QEMU = qemu-system-aarch64
QEMUFLAGS = -M raspi3b -kernel $(TARGET_FILE) -display none -serial null -initrd $(RAMDISK)

KERNEL_TARGET = kernel8.img
BOOTLOADER_TARGET = bootloader.img

TARGET = kernel
TARGET_FILE = $(KERNEL_TARGET)

ifeq ($(MAKECMDGOALS),bootloader)
	TARGET = bootloader
	TARGET_FILE = $(BOOTLOADER_TARGET)
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
RAMDISK_DIR = ./rootfs
RAMDISK = initramfs.cpio
RAMDISK_FILES = $(shell find $(RAMDISK_DIR))
JUNK = $(shell find . -type f \( -name "*.o" -o -name "*.d" \))
JUNK += $(PROGS)
JUNK += $(RAMDISK)

CFLAGS += -Iinclude/$(TARGET) -Iinclude/$(LIB)

.PHONY = clean all kernel bootloader test debug test-pty test-asm test-int

.PRECIOUS: %.elf

all: $(TARGET_FILE) $(RAMDISK)

kernel: $(TARGET_FILE) $(RAMDISK)

bootloader: $(TARGET_FILE)

test: $(TARGET_FILE) $(RAMDISK)
	$(QEMU) $(QEMUFLAGS) -serial stdio

debug: $(TARGET_FILE) $(RAMDISK)
	$(QEMU) $(QEMUFLAGS) -serial stdio -S -s

test-pty: $(TARGET_FILE) $(RAMDISK)
	$(QEMU) $(QEMUFLAGS) -serial pty
	
test-asm: $(TARGET_FILE) $(RAMDISK)
	$(QEMU) $(QEMUFLAGS) -serial stdio -d in_asm

test-int: $(TARGET_FILE) $(RAMDISK)
	$(QEMU) $(QEMUFLAGS) -serial stdio -d int

$(RAMDISK): $(RAMDISK_FILES)
	cd $(RAMDISK_DIR) && find . | cpio -o -H newc > ../$@

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