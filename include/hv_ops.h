/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2026                           |
 * |                                                            |
 * | Hypervisor backend abstraction — common interface over     |
 * | Intel VT-x (VMX) and AMD SVM                               |
 * +------------------------------------------------------------+
*/
#ifndef HV_OPS_H
#define HV_OPS_H

struct HvOperations {
    const char *name;
    int  (*check_support)(void);  /* returns 0 if supported */
    void (*print_info)(void);     /* log capabilities to display + serial */
    int  (*enable)(void);         /* enable virtualization, returns 0 on success */
    void (*run_guest)(void);      /* enter the guest loop — does not return */
};

/* Detect and return the appropriate backend, or NULL if unsupported */
const struct HvOperations *hv_detect_backend(void);

/* Defined in vmx.c and svm.c respectively */
extern const struct HvOperations vmx_ops;
extern const struct HvOperations svm_ops;

#endif /* HV_OPS_H */
