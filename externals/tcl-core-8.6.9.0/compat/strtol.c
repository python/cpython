/*
 * strtol.c --
 *
 *	Source code for the "strtol" library procedure.
 *
 * Copyright (c) 1988 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 *----------------------------------------------------------------------
 *
 * strtol --
 *
 *	Convert an ASCII string into an integer.
 *
 * Results:
 *	The return value is the integer equivalent of string. If endPtr is
 *	non-NULL, then *endPtr is filled in with the character after the last
 *	one that was part of the integer. If string doesn't contain a valid
 *	integer value, then zero is returned and *endPtr is set to string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

long int
strtol(
    const char *string,		/* String of ASCII digits, possibly preceded
				 * by white space. For bases greater than 10,
				 * either lower- or upper-case digits may be
				 * used. */
    char **endPtr,		/* Where to store address of terminating
				 * character, or NULL. */
    int base)			/* Base for conversion. Must be less than 37.
				 * If 0, then the base is chosen from the
				 * leading characters of string: "0x" means
				 * hex, "0" means octal, anything else means
				 * decimal. */
{
    register const char *p;
    long result;

    /*
     * Skip any leading blanks.
     */

    p = string;
    while (isspace(UCHAR(*p))) {
	p += 1;
    }

    /*
     * Check for a sign.
     */

    if (*p == '-') {
	p += 1;
	result = -(strtoul(p, endPtr, base));
    } else {
	if (*p == '+') {
	    p += 1;
	}
	result = strtoul(p, endPtr, base);
    }
    if ((result == 0) && (endPtr != 0) && (*endPtr == p)) {
	*endPtr = (char *) string;
    }
    return result;
}
