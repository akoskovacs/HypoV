/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | The actual 64-bit hypervisor payload, embedded in the main |
 * | ELF32 image.                                               |
 * +------------------------------------------------------------+
*/
#include <drivers/video/pc_console.h>

/* 
 * This code is contained in a proper ELF64 binary image. It is built
 * completily statically, without dependencies and shared libraries (including libc).
 * But, by mistake it could still be executed in a standard Linux/BSD/etc environment.
 * If it is not loaded by the HypoV loader, the code will print an error message through
 * standard Linux system call interface (we don't have libc, see above).
*/

#define SCALL_LINUX_WRITE 1
#define SCALL_LINUX_EXIT 60

static char hv_exe_warning[] = "HypoV cannot be used from operating systems.\n";

long scall_linux_write(long fd, const void *data, unsigned long size)
{
    long retcode = 0;
    /* 
     * Using Linux x86_64 system call interface
     *   %rax = write() system call id
     *   %rdi = fd - file descriptor
     *   %rsi = data - data to write
     *   %rdx = size -  size of the data
    */
    __asm__ __volatile__( "syscall"
            : "=a"(retcode)
            : "a"(SCALL_LINUX_WRITE), "D"(fd), "S"(data), "d"(size));
    return retcode;

}

void scall_linux_exit(long ecode)
{
    /* 
     * %rax = exit() system call id
     * %rdi = ecode - exit code 
    */
    __asm__ __volatile__( "syscall"
            : // No output
            : "a"(SCALL_LINUX_EXIT), "D"(ecode));
}

void os_error_stub(void)
{
    scall_linux_write(1, hv_exe_warning, sizeof(hv_exe_warning));
    scall_linux_exit(0xff);
}

void hv_start(uint32_t arg)
{
    char message[] = "Hello, from 64bit land (hvcore.elf64) :D :D :D";
    char *hello = message;

    /* No arguments, we must be executed from an OS, hopefully Linux :D */
#ifdef CONFIG_HV_NO_OS_STUB
    if (arg == 0x0) {
        os_error_stub();
        return;
    } 
#endif

    console_font_t font = BG_COLOR_CYAN | FG_COLOR_WHITE;
    volatile console_font_t *videoram = PC_VIDEORAM_BASE_ADDRESS;
    /* Clear screen without 32bit code */
    for (int i = 0; i < CONFIG_CONSOLE_WIDTH * CONFIG_CONSOLE_HEIGHT; i++) {
       videoram[i] = 0 | (font << 8);
    }

    int i = 0;
    while (*hello) {
        videoram[i++] = *hello++ | (font << 8);
    }
    
    while (1) {
        ;
    }
}
