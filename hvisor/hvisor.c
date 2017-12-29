static char hv_exe_warning[] = "HypoV cannot be used from operating systems.\n";

long scall_linux_write(long fd, const void *data, unsigned long size)
{
    long retcode = 0;
    __asm__ __volatile__(
         /* "movq $1, %rax\n\t"
            "movq %0, %%rdi\n\t"
            "movq %1, %%rsi\n\t"
            "movq %2, %%rdx\n\t" */
            "syscall"
            : "=a"(retcode)
            : "a"(1), "D"(fd), "S"(data), "d"(size));
            // : "rdi", "rsi", "rdx");
    return retcode;

}

void scall_linux_exit(long n)
{
    __asm__ __volatile__(
         /* "movq $60, %%rax\n\t"
            "movq %0, %%rdi\n\t" */
            "syscall"
            : // No output
            : "a"(60), "D"(n));
}

void hv_entry_64(void *arg)
{
    /* No arguments, we must be executed from an OS, hopefully Linux :D */
    if (arg == 0x0) {
        scall_linux_write(1, hv_exe_warning, sizeof(hv_exe_warning));
        scall_linux_exit(0xff);
        return;
    } 

    while (1) {
        ;
    }
}
