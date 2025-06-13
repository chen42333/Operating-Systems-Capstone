#!/bin/bash

TMP_DIR="./.tmp"
TMP_VOL="TMP_VOL"

if [ $(basename ${1} .dmg) = ${1} ]; then
    echo "Invalid image extension name (must be .dmg)"
    exit 1
fi

mkdir ${TMP_DIR}
for i in rpi3_fw/* config.txt; do
    cp ${i} ${TMP_DIR}/
done;

hdiutil create -size 64m -fs HFS+ -volname ${TMP_VOL} -type UDIF -layout MBRSPUD -attach "$(PWD)/$(basename ${1} .dmg)"
MOUNT_POINT=$(hdiutil info | grep ${TMP_VOL} | head -n 1 | awk '{print $NF}')
DEVICE_NODE=$(diskutil info ${MOUNT_POINT} | grep "Device Node" | awk '{print $NF}')
diskutil eraseVolume FAT32 ${TMP_VOL} ${DEVICE_NODE}
MOUNT_POINT=$(hdiutil info | grep ${TMP_VOL} | head -n 1 | awk '{print $NF}')
rsync -av --delete ${TMP_DIR}/ ${MOUNT_POINT}/
hdiutil detach ${MOUNT_POINT}

rm -rf ${TMP_DIR}