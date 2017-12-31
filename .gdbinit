# Connect to the QEMU remote debugger `make qemudbg'
# To automatically load this file, the local ~/.gdbinit
# has to contain:
#   set auto-load local-gdbinit
#   add-auto-load-safe-path <YOUR_PARENT_DIR>/HypoV/.gdbinit

file hypov.bin
target remote localhost:1234
