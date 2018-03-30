#include <string.h>
#include <basic.h>

/* Some of these functions may borrowed from Linux */

#define abs(n) ((n < 0) ? -(n) : (n))

size_t strlen(const char *str)
{
    const char *ostr = str;
    if (str == NULL) {
        return 0;
    }

    while (*++str)
        ;

    return str - ostr;
}

char *strcpy(char *dest, const char *src)
{
    char *ret = dest;
    while ((*dest++ = *src++) != '\0')
        ;

    return ret;
}

char *strncpy(char *dest, const char *src, size_t count)
{
    char *tmp = dest;

    while (count) {
        if ((*tmp = *src) != 0)
            src++;
        tmp++;
        count--;
    }
    return dest;
}

int strcmp(const char *cs, const char *ct)
{
    unsigned char c1, c2;

    while (1) {
        c1 = *cs++;
        c2 = *ct++;
        if (c1 != c2)
            return c1 < c2 ? -1 : 1;
        if (!c1)
            break;
    }
    return 0;
}

char hexdigit(int n)
{
    char x;
    switch(n)
    {
        case 10:
            x = 'A';
            break;
        case 11:
            x = 'B';
            break;
        case 12:
            x = 'C';
            break;
        case 13:
            x = 'D';
            break;
        case 14:
            x = 'E';
            break;
        case 15:
            x = 'F';
            break;
        default:
            x = n + 0x30;
            break;
    }
    return x;
}

void itoa(int num, int base, char *buffer)
{
    unsigned int r = 0;
    int i = 0;
    char tmp[32];
    if(num < 0)
        *buffer++ = '-';
    num = abs(num);

    if(num < base){
        *buffer++ = hexdigit(num);
        *buffer = '\0';
        return;
    }

    if(num >= base)
        while(num)
        {
            r = num % base;
            num /= base;
            tmp[i++] = hexdigit(r);	
        }
    i--;
    for( ; i>=0 ; i--)
        *buffer++ = tmp[i];
    *buffer = '\0';
}

void uitoa(unsigned int num, unsigned int base, char *buffer)
{
    unsigned int r=0;
    int i=0;
    char tmp[32];

    if(num < base){
        *buffer++ = hexdigit(num);
        *buffer = '\0';
        return;
    }

    if(num >= base)
        while(num)
        {
            r = num % base;
            num /= base;
            tmp[i++] = hexdigit(r);	
        }
    i--;
    for( ; i>=0 ; i--)
        *buffer++ = tmp[i];
    *buffer = '\0';
}

void bzero(void *target, size_t n)
{
    char *s = (char *)target;
    while (n--) {
        *s = '\0';
    }
}

void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *ds 	  = dest;
    const uint8_t *ts = src;

    while (n--) {
        *ds++ = *ts++;
    }
    return dest;
}

bool memeq(const void *a, const void *b, size_t size)
{
    const uint8_t *x = a;
    const uint8_t *y = b;
    size_t i;

    for (i = 0; i < size; ++i) {
        if (x[i] != y[i]) {
            return false;
        }
    }

    return true;
}

void memzero(void *buf, size_t size)
{
    uint8_t *b = buf;
    uint8_t *e = b + size;

    while (b != e) {
        *b++ = '\0';
    }
}

void *memmove(void *dest, const void *src, size_t size)
{
    uint8_t *d = dest;
    const uint8_t *s = src;
    size_t i;

    if (d < s) {
        for (i = 0; i < size; ++i) {
            d[i] = s[i];
        }
    } else if (d > s) {
        i = size;
        while (i-- > 0) {
            d[i] = s[i];
        }
    }

    return dest;
}

/**
 * memcmp - Compare two areas of memory
 * @cs: One area of memory
 * @ct: Another area of memory
 * @count: The size of the area.
 */
int memcmp(const void *cs, const void *ct, size_t count)
{
    const unsigned char *su1, *su2;
    int res = 0;

    for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--) {
        if ((res = *su1 - *su2) != 0) {
            break;
        }
    }
    return res;
}

/**
 * memset - Fill a region of memory with the given value
 * @s: Pointer to the start of the area.
 * @c: The byte to fill the area with
 * @count: The size of the area.
 *
 * Do not use memset() to access IO space, use memset_io() instead.
 */
void *memset(void *s, int c, size_t count)
{
    char *xs = s;

    while (count--) {
        *xs++ = c;
    }
    return s;
}

