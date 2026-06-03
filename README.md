# HypoV [![Build](https://github.com/akoskovacs/HypoV/actions/workflows/build.yml/badge.svg)](https://github.com/akoskovacs/HypoV/actions/workflows/build.yml)

64-bit **hypervisor** for funzies.

The project boots via GRUB multiboot into a 32-bit stub that sets up paging and long mode, then decompresses and loads a self-contained 64-bit hypervisor core (`hvcore.elf64`) at a fixed physical address. The 64-bit core has its own GDT, IDT, and interrupt handling.

## Dependencies

### Linux (Debian/Ubuntu)

```sh
sudo apt-get install -y build-essential gcc-i686-linux-gnu binutils-i686-linux-gnu \
    yasm grub-common grub-pc-bin xorriso mtools ruby
```

### macOS (Homebrew)

```sh
brew install i686-elf-gcc i686-elf-binutils i686-elf-grub \
    x86_64-elf-gcc x86_64-elf-binutils yasm xorriso
```

## Building

### Configure

```sh
make menuconfig
```

The default configuration is stored in `.config`. To reset to defaults:

```sh
make defconfig
```

### Compile

**macOS:**
```sh
make
```

**Linux:**
```sh
make CROSS_COMPILE=i686-linux-gnu- GRUB_MKRESCUE=grub-mkrescue
```

### Generate a bootable ISO

**macOS:**
```sh
make iso
```

**Linux:**
```sh
make CROSS_COMPILE=i686-linux-gnu- GRUB_MKRESCUE=grub-mkrescue iso
```

## Running

### QEMU (from ISO)

```sh
qemu-system-x86_64 -boot d -cdrom hypov.iso -m 512 -serial stdio
```

After boot, press **F1** then **L** to load and execute the 64-bit hypervisor core.

### QEMU (shortcut)

```sh
make qemuiso
```

## Project structure

| Path | Description |
|---|---|
| `boot/` | Multiboot entry point and 32-bit chainloader |
| `sys/` | 32-bit kernel: CPU init, memory/paging, ELF loader, debug console |
| `sys/core/` | 64-bit hypervisor core (hvcore): GDT, IDT, interrupt handling |
| `lib/lib32/` | Support library compiled for 32-bit |
| `lib/lib64/` | Support library compiled for 64-bit |
| `lib/drivers/` | VGA character display, serial debug output |
| `scripts/` | Build system scripts and kconfig |
| `etc/grub/` | GRUB configuration for ISO generation |
