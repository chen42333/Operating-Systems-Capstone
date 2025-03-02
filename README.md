# Build
1. Build kernel
```
make kernel
```
2. Build UART bootloader
```
make bootloader
```
3. Move the bootloader image (bootloader.img), ramdisk (initramfs.cpio) and the config file (config.txt) into SD card
4. Insert the SD card into Rpi3, then power it on
5. Send the kernel image to UART bootloader
```
python3 send_kernel.py <tty-dev-path>
```
6. Interact with Rpi3
```
minicom -D <tty-dev-path> -b 115200 -o
``` 