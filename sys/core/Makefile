subdir-ccflags-y := -fPIC
subdir-ccflags-y += -fvisibility=protected
subdir-ccflags-y += -m64
subdir-ldflags-y := -Wl$(comma)-melf_x86_64
nasmflags-y := -felf64

always := hvcore.lds
hvobj-y += head.o
hvobj-y += hvisor.o
