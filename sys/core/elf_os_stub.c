/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2018                           |
 * |                                                            |
 * | Operating system ELF warning stub                          |
 * +------------------------------------------------------------+
*/

/* 
 * This code is contained in a proper ELF64 binary image. It is built
 * completily statically, without dependencies and shared libraries (including libc).
 * But, by mistake it could still be executed in a standard Linux/BSD/etc environment.
 * If it is not loaded by the HypoV loader, the code will print an error message through
 * standard Linux system call interface (we don't have libc, see above).
*/

#define SCALL_LINUX_WRITE 1
#define SCALL_LINUX_EXIT 60

static const char hv_exe_warning[] = "HypoV cannot be used from operating systems.\n";

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