all: hypov.elf64

hypov.elf64:
	gcc hvisor.c -Wall -Wundef -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -std=c99 -nostdinc -fno-builtin -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -ffreestanding -nostartfiles -nodefaultlibs -nostdlib -shared -m64 -fPIC -static -fvisibility=protected -Wl,-ehv_entry_64 -o hvisor.elf64 

