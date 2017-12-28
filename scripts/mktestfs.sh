# Make a FAT32 test file system and install
# the hypervisor with SYSLINUX on it

#!/bin/bash

TESTFS_NAME=testfs.img
TESTFS_LABEL=PENDRIVE
TESTFS_SIZE=64M
TMP=/tmp/fs

echo "Making FAT32 filesystem '$TESTFS_NAME'..."
dd if=/dev/zero of=$TESTFS_NAME bs=$TESTFS_SIZE count=1
mkfs.fat -n "$TESTFS_LABEL" -F32 $TESTFS_NAME
echo -n "Installing SYSLINUX bootloader..."
syslinux -i $TESTFS_NAME && echo "[OK]"
echo -n "Mounting the new filesystem at '$TMP'..."
mkdir $TMP
sudo mount $TESTFS_NAME $TMP && echo "[OK]"
echo -n "Installing SYSLINUX files..."
sudo cp etc/syslinux_pendrive/* $TMP && echo "[OK]"
echo -n "Installing HypoV..."
sudo cp hypov.bin $TMP && echo "[OK]"
echo -n "Unmounting the filesystem..."
sudo umount $TMP && echo "[DONE]"
rm -rf $TMP

echo
echo "You can test the setup, issuing: "
echo "$ make qemufs"
echo "    or"
echo "$ qemu-system-i386 -hda $TESTFS_NAME"

