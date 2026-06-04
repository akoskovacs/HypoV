/*
 * hyp_check — HypoV guest proof tool
 *
 * Executes VMMCALL with the HypoV signature. If running under HypoV,
 * the hypervisor responds with a magic value and the VM exit count.
 *
 * Build on Linux:  gcc -static -o hyp_check hyp_check.c && ./hyp_check
 * Build on macOS:  make proof.iso  (uses Docker automatically)
 */
#include <stdio.h>
#include <stdint.h>

#define HV_SIGNATURE  0x48594F56UL  /* "HYOV" */
#define HV_MAGIC      0xDEADBEEFUL

int main(void)
{
    uint64_t rbx = 0, rcx = 0;

    /* "a" = rax (input: signature), "=b" = rbx output, "=c" = rcx output */
    __asm__ __volatile__(
        "vmmcall"
        : "=b"(rbx), "=c"(rcx)
        : "a"((uint64_t)HV_SIGNATURE)
        : "memory"
    );

    if (rbx == HV_MAGIC) {
        printf("Running under HypoV  (magic=0x%llX, exit_count=%llu)\n",
               (unsigned long long)rbx, (unsigned long long)rcx);
        return 0;
    } else {
        printf("Not running under HypoV\n");
        return 1;
    }
}
