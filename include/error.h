/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Global error codes                                         |
 * | When functions fail, they return the negated value of the  |
 * | actual error code, for example:                            |
 * | if (arg > MAX_VALUE) {                                     |
 * |    return -HV_BADARG;                                      |
 * | }                                                          |
 * +------------------------------------------------------------+
*/
#ifndef HV_ERROR_H
#define HV_ERROR_H

enum HV_ERROR {
    HV_ENODISP = 1, // No display, or NULL
    HV_ENOIMPL,     // Function not implemented
    HV_BADARG,      // Bad argument for the function
    HV_ENOMEM,      // Cannot allocate enough memory
    HV_ENOVALID,    // The data used is not valid
    HV_EMISALIGN,   // Pointer/address is misaligned
    HV_ENOINFO,     // System information structure (struct SystemInfo) is missing
    HV_ETOOLONG     // Coordinates are out of scope
};

/* eptr is an int *, error is an error from the enum above */
#define SET_ERROR(eptr, error) (if (error) (*(eptr)) = error)

#endif // HV_ERROR_H