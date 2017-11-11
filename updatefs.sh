# Make a FAT32 test file HypoV update script

#!/bin/bash

TESTFS_NAME=testfs.img
TMP=/tmp/fs

echo -n "Mounting filesystem at '$TMP'..."
mkdir $TMP
sudo mount $TESTFS_NAME $TMP && echo "[OK]"
echo -n "Updating HypoV..."
sudo cp hypov.bin $TMP && echo "[OK]"
echo -n "Unmounting the filesystem..."
sudo umount $TMP && echo "[DONE]"
rm -rf $TMP

echo
echo "You can test the setup, issuing: "
echo "$ qemu-system-i386 -hda $TESTFS_NAME"

