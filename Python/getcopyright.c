/* Return the copyright string.  This is updated manually. */

#include "Python.h"

static const char cprt[] =
"\
Copyright (c) 2001 Python Software Foundation.\n\
All Rights Reserved.\n\
\n\
Copyright (c) 2000 BeOpen.com.\n\
All Rights Reserved.\n\
\n\
Copyright (c) 1995-2001 Corporation for National Research Initiatives.\n\
All Rights Reserved.\n\
\n\
Copyright (c) 1991-1995 Stichting Mathematisch Centrum, Amsterdam.\n\
All Rights Reserved.";

const char *
Py_GetCopyright(void)
{
    return cprt;
}
