/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Convert a possibly signed character to a nonnegative int */
/* XXX This assumes characters are 8 bits wide */
#ifdef __CHAR_UNSIGNED__
#define Py_CHARMASK(c)		(c)
#else
#define Py_CHARMASK(c)		((c) & 0xff)
#endif

#include "rename2.h"

/* strtol and strtoul, renamed to avoid conflicts */

/*
**	strtoul
**		This is a general purpose routine for converting
**		an ascii string to an integer in an arbitrary base.
**		Leading white space is ignored.  If 'base' is zero
**		it looks for a leading 0, 0x or 0X to tell which
**		base.  If these are absent it defaults to 10.
**		Base must be 0 or between 2 and 36 (inclusive).
**		If 'ptr' is non-NULL it will contain a pointer to
**		the end of the scan.
**		Errors due to bad pointers will probably result in
**		exceptions - we don't check for them.
*/

#include <ctype.h>
#include <errno.h>

unsigned long
mystrtoul(str, ptr, base)
register char *	str;
char **		ptr;
int		base;
{
    register unsigned long	result;	/* return value of the function */
    register int		c;	/* current input character */
    register unsigned long	temp;	/* used in overflow testing */
    int				ovf;	/* true if overflow occurred */

    result = 0;
    ovf = 0;

/* catch silly bases */
    if (base != 0 && (base < 2 || base > 36))
    {
	if (ptr)
	    *ptr = str;
	return 0;
    }

/* skip leading white space */
    while (*str && isspace(Py_CHARMASK(*str)))
	str++;

/* check for leading 0 or 0x for auto-base or base 16 */
    switch (base)
    {
    case 0:		/* look for leading 0, 0x or 0X */
	if (*str == '0')
	{
	    str++;
	    if (*str == 'x' || *str == 'X')
	    {
		str++;
		base = 16;
	    }
	    else
		base = 8;
	}
	else
	    base = 10;
	break;

    case 16:	/* skip leading 0x or 0X */
	if (*str == '0' && (*(str+1) == 'x' || *(str+1) == 'X'))
	    str += 2;
	break;
    }

/* do the conversion */
    while (c = Py_CHARMASK(*str))
    {
	if (isdigit(c) && c - '0' < base)
	    c -= '0';
	else
	{
	    if (isupper(c))
		c = tolower(c);
	    if (c >= 'a' && c <= 'z')
		c -= 'a' - 10;
	    else	/* non-"digit" character */
		break;
	    if (c >= base)	/* non-"digit" character */
		break;
	}
	temp = result;
	result = result * base + c;
#ifndef MPW
	if ((result - c) / base != temp)	/* overflow */
	    ovf = 1;
#endif
	str++;
    }

/* set pointer to point to the last character scanned */
    if (ptr)
	*ptr = str;
    if (ovf)
    {
	result = ~0;
	errno = ERANGE;
    }
    return result;
}

long
mystrtol(str, ptr, base)
char *	str;
char ** ptr;
int	base;
{
	long result;
	char sign;
	
	while (*str && isspace(Py_CHARMASK(*str)))
		str++;
	
	sign = *str;
	if (sign == '+' || sign == '-')
		str++;
	
	result = (long) mystrtoul(str, ptr, base);
	
	/* Signal overflow if the result appears negative,
	   except for the largest negative integer */
	if (result < 0 && !(sign == '-' && result == -result)) {
		errno = ERANGE;
		result = 0x7fffffff;
	}
	
	if (sign == '-')
		result = -result;
	
	return result;
}
