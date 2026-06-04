/*
 * hypv_check — HypoV guest proof tool
 *
 * Executes VMMCALL with the HypoV signature. If running under HypoV,
 * the hypervisor responds with a magic value and the VM exit count.
 *
 * Build inside Alpine:
 *   apk add gcc
 *   gcc -static -o hypv_check hypv_check.c
 *   ./hypv_check
 */
#include <stdio.h>
#include <stdint.h>

#define HV_SIGNATURE  0x48594F56UL  /* "HYOV" */
#define HV_MAGIC      0xDEADBEEFUL

int main(void)
{
    uint64_t rax = HV_SIGNATURE;
    uint64_t rbx = 0;
    uint64_t rcx = 0;

    /* "0" ties the input rax to output operand 0 (rax), works on GCC and clang */
    __asm__ __volatile__(
        "vmmcall"
        : "=a"(rax), "=b"(rbx), "=c"(rcx)
        : "0"(rax)
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
