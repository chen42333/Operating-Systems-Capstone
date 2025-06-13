#!/bin/bash

DEV_PATH=$1
MNT_POINT=$2

diskutil unmountDisk force $DEV_PATH
diskutil eraseDisk FAT32 OSC MBRFormat $DEV_PATH
diskutil unmountDisk force $DEV_PATH
sudo dd if=./nycuos.img of=$DEV_PATH
if [ -z "$MNT_POINT" ]; then
    diskutil mountDisk $DEV_PATH
else
    diskutil mountDisk -mountPoint $MNT_POINT $DEV_PATH
fi