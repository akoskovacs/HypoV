# HypoV [![pipeline status](https://gitlab.com/akoskovacs/HypoV/badges/master/pipeline.svg)](https://gitlab.com/akoskovacs/HypoV/commits/master)

64-bit **hypervisor** for funzies.

## Compilation
### Installing dependencies (Debian/Ubuntu/Linux Mint/stb...)
```sh
$ sudo apt-get -y install build-essential yasm grub-common grub-pc-bin xorriso libncurses-dev ruby xz-utils
```

### Configuration via Kconfig
```sh
$ make menuconfig
```

### Compiling (hypov.bin)
```sh
$ make
```

### Generating the ISO (hypov.iso)
```sh
$ make iso
```

### Running inside QEMU
```
qemu-system-x86_64 -boot d -cdrom hypov.iso -m 512
```
