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