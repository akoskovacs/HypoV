/*
 * hyp_check — HypoV guest proof tool
 *
 * Detects Intel vs AMD and uses vmcall/vmmcall accordingly.
 * If running under HypoV the hypervisor responds with a magic value.
 */
#include <stdio.h>
#include <stdint.h>

#define HV_SIGNATURE  0x48594F56UL  /* "HYOV" */
#define HV_MAGIC      0xDEADBEEFUL

static int is_intel(void)
{
    uint32_t ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    return ebx == 0x756e6547; /* "Genu" = GenuineIntel */
}

int main(void)
{
    uint64_t rbx = 0, rcx = 0;

    if (is_intel()) {
        __asm__ __volatile__(
            "vmcall"          /* Intel VT-x */
            : "=b"(rbx), "=c"(rcx)
            : "a"((uint64_t)HV_SIGNATURE)
            : "memory"
        );
    } else {
        __asm__ __volatile__(
            "vmmcall"         /* AMD SVM */
            : "=b"(rbx), "=c"(rcx)
            : "a"((uint64_t)HV_SIGNATURE)
            : "memory"
        );
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
