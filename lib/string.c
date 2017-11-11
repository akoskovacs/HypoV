#include <string.h>
#include <basic.h>

size_t strlen(const char *str)
{
    size_t n = 0;
    const char *ostr = str;
    if (str == NULL) {
        return n;
    }

    while (*++str)
        ;

    return str - ostr;
}