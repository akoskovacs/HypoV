/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Generic printf() functions                                 |
 * +------------------------------------------------------------+
*/

#include <print.h>
#include <string.h>

int hv_vsnprintf(char *dest, size_t size, const char *fmt, va_list ap)
{
   char *tmp;
   size_t asize = 0;
   size_t nsize = 0;
   unsigned long unum = 0;
   char buffer[CONFIG_PRINTF_BUFFER_SIZE];
   long num;

   while (*fmt != '\0') {
       if (*fmt == '%') {
           fmt++;
           switch (*fmt) {
               case 's':
                   tmp = va_arg(ap, char *);
                   if (tmp == NULL)
                       tmp = "(null)";
                   nsize = strlen(tmp);
                   strncpy(dest+asize, tmp, size-asize);
                   asize += nsize;
               break;

               case 'u':
                   unum = va_arg(ap, unsigned long);
                   uitoa(unum, 10, buffer);
                   nsize = strlen(buffer);
                   strncpy(dest+asize, buffer, size-asize);
                   asize += nsize;
               break;

               case 'd': case 'i':
                   num = va_arg(ap, long);
                   itoa(num, 10, buffer);
                   nsize = strlen(buffer);
                   strncpy(dest+asize, buffer, size-asize);
                   asize += nsize;
               break;

               case 'x': case 'p': case 'X':
                    unum = (*fmt == 'x') ? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, void *);
                    if (*fmt != 'X') {
                        strncpy(dest+asize, "0x", size-asize);
                        asize += 2;
                    }
                    uitoa(unum, 16, buffer);
                    nsize = strlen(buffer);
                    strncpy(dest+asize, buffer, size-asize);
                    asize += nsize;
               break;

               case 'c':
                    dest[asize++] = va_arg(ap, int);
               break;

               case '%':
                    strcpy(dest+asize++, "%");
               break;
           } // end of switch
           if (asize >= size)
               return asize;
       } else {
           if (asize + 1 < size)
               dest[asize++] = *fmt;
       }
       fmt++;
   } // end of while
   dest[asize] = '\0';
   return asize;
}

/* Very, bad idea... */
#define KPRINT_TEMPLATE() \
    int size;  \
    va_list ap; \
    va_start(ap, fmt); \
    char buf[CONFIG_PRINTF_BUFFER_SIZE]; \
    size = hv_vsnprintf(buf, CONFIG_PRINTF_BUFFER_SIZE, fmt, ap); \
    va_end(ap)

int hv_snprintf(char *dest, size_t size, const char *fmt, ...)
{
    va_list ap;
    int ret;
    va_start(ap, fmt);
    ret = hv_vsnprintf(dest, size, fmt, ap);
    va_end(ap);
    return ret;
}

int hv_printf(struct CharacterDisplay *disp, const char *fmt, ...)
{
    KPRINT_TEMPLATE();
    hv_disp_puts(disp, buf);
    return size;
}

int hv_printf_xy(struct CharacterDisplay *disp, int x, int y, const char *fmt, ...)
{
    KPRINT_TEMPLATE();
    hv_disp_puts_xy(disp, x, y, buf);
    return size;
}
