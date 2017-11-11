#include <string.h>
#include <basic.h>

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
