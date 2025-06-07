#!/bin/bash

OS=$(uname -s)
PATH=$1
MNT_POINT=$2

if [ "$OS" = "Darwin" ]; then
    # diskutil list
    diskutil unmountDisk force $PATH
    diskutil eraseDisk FAT32 OSC MBRFormat $PATH
    diskutil unmountDisk force $PATH
    dd if=./nycuos.img of=$PATH
    if [ -z "$MNT_POINT" ]; then
        diskutil mountDisk $PATH
    else
        diskutil mountDisk -mountPoint $MNT_POINT $PATH
    fi
elif [ "$OS" = "Linux" ]; then
    # lsblk -f
    unmount $PATH
    mkfs.fat -F 32 $PATH
    dd if=./nycuos.img of=$PATH
    if [ -z "$MNT_POINT" ]; then
        MNT_POINT=/mnt/osc
    fi
    mkdir $MNT_POINT
    mount $PATH $MNT_POINT
else
    echo "Unknown OS: $OS"
fi