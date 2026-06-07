/*
 * hyp_check — HypoV guest proof tool
 *
 * Designed to run as Linux /init inside the HypoV guest initramfs.
 * Detects Intel vs AMD and uses vmcall/vmmcall accordingly.
 * If running under HypoV the hypervisor responds with a magic value.
 */
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mount.h>
#include <unistd.h>

#define HV_SIGNATURE 0x48594F56UL /* "HYOV" */
#define HV_MAGIC     0xDEADBEEFUL

/* When running as PID 1 (Linux /init), the kernel can't open /dev/console
 * until devtmpfs is mounted.  Mount it ourselves so printf reaches ttyS0. */
static void setup_console(void)
{
    mount("devtmpfs", "/dev", "devtmpfs", 0, "");
    int fd = open("/dev/console", O_RDWR);
    if (fd < 0)
        return;
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > 2)
        close(fd);
}

static int is_intel(void)
{
    uint32_t ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    return ebx == 0x756e6547; /* "Genu" = GenuineIntel */
}

int main(void)
{
    pid_t pid = getpid();
    if (pid == 1) {
        setup_console();
    }

    uint64_t rbx = 0, rcx = 0;

    if (is_intel()) {
        __asm__ __volatile__(
            "vmcall" /* Intel VT-x */
            : "=b"(rbx), "=c"(rcx)
            : "a"((uint64_t)HV_SIGNATURE)
            : "memory");
    } else {
        __asm__ __volatile__(
            "vmmcall" /* AMD SVM */
            : "=b"(rbx), "=c"(rcx)
            : "a"((uint64_t)HV_SIGNATURE)
            : "memory");
    }

    if (rbx == HV_MAGIC) {
        printf("Running under HypoV  (magic=0x%llX, exit_count=%llu)\n",
               (unsigned long long)rbx, (unsigned long long)rcx);
        return 0;
    } else {
        printf("Not running under HypoV\n");
        return 1;
    }
}
