
#include "Python.h"

#if defined(__sgi) && defined(WITH_THREAD) && !defined(_SGI_MP_SOURCE)
#define _SGI_MP_SOURCE
#endif

/* strtol and strtoul, renamed to avoid conflicts */


#include <ctype.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

/* Static overflow check values for bases 2 through 36.
 * smallmax[base] is the largest unsigned long i such that
 * i * base doesn't overflow unsigned long.
 */
static unsigned long smallmax[] = {
	0, /* bases 0 and 1 are invalid */
	0,
	ULONG_MAX / 2,
	ULONG_MAX / 3,
	ULONG_MAX / 4,
	ULONG_MAX / 5,
	ULONG_MAX / 6,
	ULONG_MAX / 7,
	ULONG_MAX / 8,
	ULONG_MAX / 9,
	ULONG_MAX / 10,
	ULONG_MAX / 11,
	ULONG_MAX / 12,
	ULONG_MAX / 13,
	ULONG_MAX / 14,
	ULONG_MAX / 15,
	ULONG_MAX / 16,
	ULONG_MAX / 17,
	ULONG_MAX / 18,
	ULONG_MAX / 19,
	ULONG_MAX / 20,
	ULONG_MAX / 21,
	ULONG_MAX / 22,
	ULONG_MAX / 23,
	ULONG_MAX / 24,
	ULONG_MAX / 25,
	ULONG_MAX / 26,
	ULONG_MAX / 27,
	ULONG_MAX / 28,
	ULONG_MAX / 29,
	ULONG_MAX / 30,
	ULONG_MAX / 31,
	ULONG_MAX / 32,
	ULONG_MAX / 33,
	ULONG_MAX / 34,
	ULONG_MAX / 35,
	ULONG_MAX / 36,
};

/* maximum digits that can't ever overflow for bases 2 through 36,
 * calculated by [int(math.floor(math.log(2**32, i))) for i in range(2, 37)].
 * Note that this is pessimistic if sizeof(long) > 4.
 */
#if SIZEOF_LONG == 4
static int digitlimit[] = {
	0,  0, 32, 20, 16, 13, 12, 11, 10, 10,  /*  0 -  9 */
	9,  9,  8,  8,  8,  8,  8,  7,  7,  7,  /* 10 - 19 */
	7,  7,  7,  7,  6,  6,  6,  6,  6,  6,  /* 20 - 29 */
	6,  6,  6,  6,  6,  6,  6};             /* 30 - 36 */
#elif SIZEOF_LONG == 8
/* [int(math.floor(math.log(2**64, i))) for i in range(2, 37)] */
static int digitlimit[] = {
	 0,   0, 64, 40, 32, 27, 24, 22, 21, 20,  /*  0 -  9 */
	19,  18, 17, 17, 16, 16, 16, 15, 15, 15,  /* 10 - 19 */
	14,  14, 14, 14, 13, 13, 13, 13, 13, 13,  /* 20 - 29 */
	13,  12, 12, 12, 12, 12, 12};             /* 30 - 36 */
#else
#error "Need table for SIZEOF_LONG"
#endif

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
unsigned long
PyOS_strtoul(register char *str, char **ptr, int base)
{
	register unsigned long result = 0; /* return value of the function */
	register int c;	 	/* current input character */
	register int ovlimit; 	/* required digits to overflow */

	/* skip leading white space */
	while (*str && isspace(Py_CHARMASK(*str)))
		++str;

	/* check for leading 0 or 0x for auto-base or base 16 */
	switch (base) {
	case 0:		/* look for leading 0, 0x or 0X */
		if (*str == '0') {
			++str;
			if (*str == 'x' || *str == 'X') {
				/* there must be at least one digit after 0x */
				if (_PyLong_DigitValue[Py_CHARMASK(str[1])] >= 16) {
					if (ptr)
						*ptr = str;
					return 0;
				}
				++str;
				base = 16;
			}
			else
				base = 8;
		}
		else
			base = 10;
		break;

	case 16:	/* skip leading 0x or 0X */
		if (*str == '0') {
			++str;
			if (*str == 'x' || *str == 'X') {
				/* there must be at least one digit after 0x */
				if (_PyLong_DigitValue[Py_CHARMASK(str[1])] >= 16) {
					if (ptr)
						*ptr = str;
					return 0;
				}
				++str;
			}
		}
		break;
	}

	/* catch silly bases */
	if (base < 2 || base > 36) {
		if (ptr)
			*ptr = str;
		return 0;
	}

	/* skip leading zeroes */
	while (*str == '0')
		++str;

	/* base is guaranteed to be in [2, 36] at this point */
	ovlimit = digitlimit[base];

	/* do the conversion until non-digit character encountered */
	while ((c = _PyLong_DigitValue[Py_CHARMASK(*str)]) < base) {
		if (ovlimit > 0) /* no overflow check required */
			result = result * base + c;
		else { /* requires overflow check */
			register unsigned long temp_result;

			if (ovlimit < 0) /* guaranteed overflow */
				goto overflowed;

			/* there could be an overflow */
			/* check overflow just from shifting */
			if (result > smallmax[base])
				goto overflowed;

			result *= base;

			/* check overflow from the digit's value */
			temp_result = result + c;
			if (temp_result < result)
				goto overflowed;

			result = temp_result;
		}

		++str;
		--ovlimit;
	}

	/* set pointer to point to the last character scanned */
	if (ptr)
		*ptr = str;

	return result;

overflowed:
	if (ptr) {
		/* spool through remaining digit characters */
		while (_PyLong_DigitValue[Py_CHARMASK(*str)] < base)
			++str;
		*ptr = str;
	}
	errno = ERANGE;
	return (unsigned long)-1;
}

/* Checking for overflow in PyOS_strtol is a PITA; see comments
 * about PY_ABS_LONG_MIN in longobject.c.
 */
#define PY_ABS_LONG_MIN		(0-(unsigned long)LONG_MIN)

long
PyOS_strtol(char *str, char **ptr, int base)
{
	long result;
	unsigned long uresult;
	char sign;

	while (*str && isspace(Py_CHARMASK(*str)))
		str++;

	sign = *str;
	if (sign == '+' || sign == '-')
		str++;

	uresult = PyOS_strtoul(str, ptr, base);

	if (uresult <= (unsigned long)LONG_MAX) {
		result = (long)uresult;
		if (sign == '-')
			result = -result;
	}
	else if (sign == '-' && uresult == PY_ABS_LONG_MIN) {
		result = LONG_MIN;
	}
	else {
		errno = ERANGE;
		result = LONG_MAX;
	}
	return result;
}
