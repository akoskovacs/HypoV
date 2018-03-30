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

void bzero(void *target, size_t size);
void *memcpy(void *dest, const void *src, size_t n);
bool memeq(const void *a, const void *b, size_t size);
void memzero(void *buf, size_t size);
void *memmove(void *dest, const void *src, size_t size);
int memcmp(const void *cs, const void *ct, size_t count);
void *memset(void *s, int c, size_t count);

#endif // STRING_H
