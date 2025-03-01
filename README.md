# Build
1. Build kernel
```
make kernel
```
2. Build UART bootloader
```
make bootloader
```
3. Generate the config file
```
make config-file
```
4. Move the bootloader image (bootloader.img), ramdisk (initramfs.cpio) and the config file (config.txt) into SD card
5. Insert the SD card into Rpi3, then power it on
6. Send the kernel image to UART bootloader
```
python3 send_kernel.py <tty-dev-path>
```
