
/* PD implementation of strerror() for systems that don't have it.
   Author: Guido van Rossum, CWI Amsterdam, Oct. 1990, <guido@cwi.nl>. */

#include <stdio.h>
#include "Python.h"

extern int sys_nerr;
extern char *sys_errlist[];

char *
strerror(int err)
{
	static char buf[20];
	if (err >= 0 && err < sys_nerr)
		return sys_errlist[err];
	PyOS_snprintf(buf, sizeof(buf), "Unknown errno %d", err);
	return buf;
}

#ifdef macintosh
int sys_nerr = 0;
char *sys_errlist[1] = 0;
#endif
