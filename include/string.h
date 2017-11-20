#ifndef STRING_H
#define STRING_H

#include <types.h>

size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t count);
int strcmp(const char *cs, const char *ct);
char hexdigit(int n);
void itoa(int num, int base, char *buffer);
void uitoa(unsigned int num, unsigned int base, char *buffer);

#endif // STRING_H