/* Return the copyright string.  This is updated manually. */

#include "Python.h"

static char cprt[] = 
"Copyright 1991-1995 Stichting Mathematisch Centrum, Amsterdam\n\
Copyright 1995-2000 Corporation for National Research Initiatives (CNRI)";

const char *
Py_GetCopyright(void)
{
	return cprt;
}
