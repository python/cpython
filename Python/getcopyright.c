/* Return the copyright string.  This is updated manually. */

#include "Python.h"

static char cprt[] = 
"Copyright (c) 2000 BeOpen.com.\n\
All Rights Reserved.\n\
\n\
Copyright (c) 1995-2000 Corporation for National Research Initiatives.\n\
All Rights Reserved.\n\
\n\
Copyright (c) 1991-1995 Stichting Mathematisch Centrum, Amsterdam.\n\
All Rights Reserved.";

const char *
Py_GetCopyright(void)
{
	return cprt;
}
