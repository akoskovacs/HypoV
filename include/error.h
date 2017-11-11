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
    HV_ETOOLONG     // Coordinates are out of scope
};

#endif // HV_ERROR_H