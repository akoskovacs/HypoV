subdir-ccflags-y := -m32 -march=i586
#subdir-ldflags-y := -Wl$(comma)-melf_i386 
LDFLAGS := -melf_i386 
yasmflags-y := -felf32

obj-y += drivers/
obj-y += init.o
obj-y += char_display.o
obj-y += debug_console.o
obj-y += system.o
obj-y += memory.o
obj-y += cpu.o
obj-y += cpu_ll.o
obj-y += loader.o