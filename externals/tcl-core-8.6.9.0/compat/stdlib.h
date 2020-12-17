/*
 * stdlib.h --
 *
 *	Declares facilities exported by the "stdlib" portion of the C library.
 *	This file isn't complete in the ANSI-C sense; it only declares things
 *	that are needed by Tcl. This file is needed even on many systems with
 *	their own stdlib.h (e.g. SunOS) because not all stdlib.h files declare
 *	all the procedures needed here (such as strtod).
 *
 * Copyright (c) 1991 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _STDLIB
#define _STDLIB

extern void		abort(void);
extern double		atof(const char *string);
extern int		atoi(const char *string);
extern long		atol(const char *string);
extern char *		calloc(unsigned int numElements, unsigned int size);
extern void		exit(int status);
extern int		free(char *blockPtr);
extern char *		getenv(const char *name);
extern char *		malloc(unsigned int numBytes);
extern void		qsort(void *base, int n, int size, int (*compar)(
			    const void *element1, const void *element2));
extern char *		realloc(char *ptr, unsigned int numBytes);
extern long		strtol(const char *string, char **endPtr, int base);
extern unsigned long	strtoul(const char *string, char **endPtr, int base);

#endif /* _STDLIB */
