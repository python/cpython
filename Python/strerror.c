/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* PD implementation of strerror() for systems that don't have it.
   Author: Guido van Rossum, CWI Amsterdam, Oct. 1990, <guido@cwi.nl>. */

#include <stdio.h>

extern int sys_nerr;
extern char *sys_errlist[];

char *
strerror(err)
	int err;
{
	static char buf[20];
	if (err >= 0 && err < sys_nerr)
		return sys_errlist[err];
	sprintf(buf, "Unknown errno %d", err);
	return buf;
}

#ifdef macintosh
int sys_nerr = 0;
char *sys_errlist[1] = 0;
#endif
