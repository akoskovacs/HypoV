subdir-ccflags-y := -fPIC
subdir-ccflags-y += -fvisibility=protected
subdir-ccflags-y += -m64
subdir-ccflags-y += -mno-red-zone
subdir-ldflags-y := -Wl$(comma)-melf_x86_64
yasmflags-y := -felf64

always := hvcore.lds
hvobj-y += head.o
hvobj-y += hvisor.o
hvobj-y += elf_os_stub.o
hvobj-y += host_cpu.o
hvobj-y += host_cpu_ll.o
hvobj-y += host_int.o
hvobj-y += host_pic.o
hvobj-y += int_wrappers.o
hvobj-y += int_handler.o
hvobj-y += drivers/

sys/core/int_wrappers.asm: 
	$(Q)$(RUBY) scripts/intgen.rb $@
