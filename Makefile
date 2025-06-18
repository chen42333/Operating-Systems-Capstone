CC = clang
CFLAGS = -mcpu=cortex-a53 --target=aarch64-rpi3-elf \
	-g -Wall -MMD -MP -nostdlib -ffreestanding \
	-mno-unaligned-access -mgeneral-regs-only -mstack-alignment=16 -fPIC
LD = ld.lld
LDFLAGS = -m aarch64elf
OBJCPY = llvm-objcopy
OBJCPYFLAGS = --output-target=aarch64-rpi3-elf -O binary
QEMU = qemu-system-aarch64
QEMUFLAGS = -M raspi3b -kernel $(TARGET_FILE) -serial null \
			-initrd $(RAMDISK) -dtb $(DEVICE_TREE) -drive if=sd,file=$(DISK_IMG),format=raw
_QEMUFLAGS = 
QEMUFLAGS += $(_QEMUFLAGS)

ifneq ($(DISPLAY),true)
	QEMUFLAGS += -display none
endif

KERNEL_TARGET = kernel8.img
BOOTLOADER_TARGET = bootloader.img

TARGET = kernel
ifeq ($(MAKECMDGOALS),bootloader)
	TARGET = bootloader
endif

TARGET_FILE = $(KERNEL_TARGET)
ifeq ($(TARGET),bootloader)
	TARGET_FILE = $(BOOTLOADER_TARGET)
endif

TEST_PROG_DIR = testprog
SRC_DIR = ./src
SRCS = $(shell find $(SRC_DIR)/$(TARGET) -type f \( -name "*.c" -o -name "*.S" \))
OBJS = $(SRCS:.c=.o)
OBJS := $(OBJS:.S=.o)
DEPS = $(OBJS:.o=.d)
LDFILE = $(SRC_DIR)/$(TARGET)/linker.ld
PROGS = *.elf *.img
PROGS += $(shell find $(TEST_PROG_DIR) -type f \( -name "*.elf" -o -name "*.img" \))
# PROGS += $(shell find $(RAMDISK_DIR) -type f \( -name "*.elf" -o -name "*.img" \))
RAMDISK_DIR = ./rootfs
RAMDISK = initramfs.cpio
RAMDISK_FILES = $(shell find $(RAMDISK_DIR) -type f ! -name "*.img" )
TEST_PROG_NAME = simple
TEST_PROG = $(TEST_PROG_DIR)/$(TEST_PROG_NAME)
TEST_PROG_SRCS = $(shell find $(TEST_PROG_DIR) -type f \( -name "*.c" -o -name "*.S" \) | grep $(TEST_PROG_NAME))
TEST_PROG_LDFILE = $(TEST_PROG_DIR)/linker.ld
DEVICE_TREE = bcm2710-rpi-3-b-plus.dtb
DISK_IMG = sd.dmg

JUNK = $(shell find . -type f \( -name "*.o" -o -name "*.d" \))
JUNK += $(shell find $(TEST_PROG_DIR) -type f \( -name "*.o" -o -name "*.d" \))
JUNK += $(PROGS) $(RAMDISK) $(DISK_IMG)

CFLAGS += -Iinclude/$(TARGET)

.PHONY = clean all kernel bootloader test debug test-pty test-asm test-int

.PRECIOUS: %.elf

kernel: $(TARGET_FILE) $(RAMDISK)

bootloader: $(TARGET_FILE)

test: $(TARGET_FILE) $(RAMDISK) $(DISK_IMG)
	$(QEMU) $(QEMUFLAGS) -serial stdio

debug: $(TARGET_FILE) $(RAMDISK) $(DISK_IMG)
	$(QEMU) $(QEMUFLAGS) -serial stdio -S -s

test-pty: $(TARGET_FILE) $(RAMDISK) $(DISK_IMG)
	$(QEMU) $(QEMUFLAGS) -serial pty
	
test-asm: $(TARGET_FILE) $(RAMDISK) $(DISK_IMG)
	$(QEMU) $(QEMUFLAGS) -serial stdio -d in_asm

test-int: $(TARGET_FILE) $(RAMDISK) $(DISK_IMG)
	$(QEMU) $(QEMUFLAGS) -serial stdio -d int

$(RAMDISK): $(RAMDISK_FILES) $(TEST_PROG).img
	cd $(RAMDISK_DIR) && find . -not -path "*/.DS_Store" -not -path "*/__MACOSX/*" | cpio -o -H newc > ../$@

$(TEST_PROG).img: $(TEST_PROG_SRCS)
	$(CC) $(CFLAGS) -c $< -o $(TEST_PROG).o
	$(LD) $(LDFLAGS) -T $(TEST_PROG_LDFILE) -o $(TEST_PROG).elf $(TEST_PROG).o
	$(OBJCPY) $(OBJCPYFLAGS) $(TEST_PROG).elf $@
	cp $@ $(RAMDISK_DIR)

$(DISK_IMG): 
	chmod +x sd_dmg.sh
	./sd_dmg.sh $(DISK_IMG)

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