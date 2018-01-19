subdir-ccflags-y := -fPIC
subdir-ccflags-y += -fvisibility=protected
subdir-ccflags-y += -m64
subdir-ccflags-y += -mno-red-zone
subdir-ldflags-y := -Wl$(comma)-melf_x86_64
yasmflags-y := -felf64

always := hvcore.lds
hvobj-y += head.o
hvobj-y += hvisor.o
