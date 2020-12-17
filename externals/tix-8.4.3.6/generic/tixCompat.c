
/*	$Id: tixCompat.c,v 1.2 2001/01/08 06:15:32 ioilam Exp $	*/

/*
 * tixCompat.c --
 *
 *	Some compatibility functions for Tix.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tixInt.h>

/*
 * strdup is not a POSIX call and is not supported on many platforms.
 */

char * tixStrDup(s)
    CONST char * s;
{
    size_t len = strlen(s)+1;
    char * new_string;

    new_string = (char*)ckalloc(len);
    strcpy(new_string, s);

    return new_string;
}


#ifdef NO_STRCASECMP

int tixStrCaseCmp _ANSI_ARGS_((CONST char * a, CONST char * b));

int tixStrCaseCmp(a, b)
    CONST char * a;
    CONST char * b;
{
    while (1) {
	if (*a== 0 && *b==0) {
	    return 0;
	}
	if (*a==0) {
	    return (1);
	}
	if (*b==0) {
	    return (-1);
	}
	if (tolower(*a)>tolower(*b)) {
	    return (-1);
	}
	if (tolower(*b)>tolower(*a)) {
	    return (1);
	}
	a++; b++;
    }
}

#endif /* NO_STRCASECMP */
