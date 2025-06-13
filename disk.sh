#!/bin/bash

PATH=$1
MNT_POINT=$2

diskutil unmountDisk force $PATH
diskutil eraseDisk FAT32 OSC MBRFormat $PATH
diskutil unmountDisk force $PATH
dd if=./nycuos.img of=$PATH
if [ -z "$MNT_POINT" ]; then
    diskutil mountDisk $PATH
else
    diskutil mountDisk -mountPoint $MNT_POINT $PATH
fi