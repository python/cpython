/*
 * memcmp.c --
 *
 *	Source code for the "memcmp" library routine.
 *
 * Copyright (c) 1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclPort.h"

/*
 * Here is the prototype just in case it is not included in tclPort.h.
 */

int		memcmp(const void *s1, const void *s2, size_t n);

/*
 *----------------------------------------------------------------------
 *
 * memcmp --
 *
 *	Compares two bytes sequences.
 *
 * Results:
 *	Compares its arguments, looking at the first n bytes (each interpreted
 *	as an unsigned char), and returns an integer less than, equal to, or
 *	greater than 0, according as s1 is less than, equal to, or greater
 *	than s2 when taken to be unsigned 8 bit numbers.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
memcmp(
    const void *s1,		/* First string. */
    const void *s2,		/* Second string. */
    size_t n)			/* Length to compare. */
{
    const unsigned char *ptr1 = (const unsigned char *) s1;
    const unsigned char *ptr2 = (const unsigned char *) s2;

    for ( ; n-- ; ptr1++, ptr2++) {
	unsigned char u1 = *ptr1, u2 = *ptr2;

	if (u1 != u2) {
	    return (u1-u2);
	}
    }
    return 0;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
