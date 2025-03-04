# Operating-Systems-Capstone
## Running on RPI3
1. Build kernel
```
make kernel
```
2. Build UART bootloader
```
make bootloader
```
3. Move these file  into SD card:
- `bootloader.img`: bootloader image
- `initramfs.cpio`: ramdisk cpio archive
- `bcm2710-rpi-3-b-plus.dtb`: flattened devicetree
- `config.txt`: config file
4. Insert the SD card into RPI3, then power it on
![UART connection](UART.png)
5. Send the kernel image to UART bootloader
```
python3 send_kernel.py <tty-dev-path>
```
6. Interact with Rpi3
```
minicom -D <tty-dev-path> -b 115200 -o
``` 
## Test on QEMU
1. Build kernel
```
make kernel
```
2. Build UART bootloader
```
make bootloader
```
3. Test
```
make test TARGET=<target> _QEMUFLAGS=<additional flags>
```
- `test` can be replaced with `test-pty`, `test-asm`, `test-int`
- `<target>`: `bootloader` or `kernel`
4. Test with LLDB/GDB (optional)
```
make debug TARGET=<target> _QEMUFLAGS=<additional flags>
```
- GDB
```
gdb <target>.elf
target remote :1234
```
- LLDB
```
lldb <target>.elf
gdb-remote 1234
```