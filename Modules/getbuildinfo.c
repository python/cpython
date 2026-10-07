/* Functions to get information about the Python build */

#include "Python.h"

#include "getbuildinfo.h"

static const char copyright[] =
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
    return copyright;
}

const char *
Py_GetCompiler(void)
{
    return COMPILER;
}

const char *
Py_GetPlatform(void)
{
    return PLATFORM;
}

const char *
_Py_gitversion(void)
{
    return GIT_VERSION;
}

const char *
_Py_gitidentifier(void)
{
    return GIT_IDENTIFIER;
}

const char *
Py_GetBuildInfo(void)
{
    return BUILD_INFO;
}

// Export the Python hex version as a constant.
const unsigned long Py_Version = PY_VERSION_HEX;

// Keep the 'version' variable for backward compatibility.
// Some debuggers inspect directly the variable.
static const char *version = VERSION;

const char *
Py_GetVersion(void)
{
    return version;
}


// Check if Python was built with NDEBUG macro defined or not. Implement the
// function in Modules/getbuildinfo.c so it's built with the same compiler
// flags than the Python core C code.
//
// Export the function for '_testlimitedcapi' shared extension.
PyAPI_FUNC(int)
_Py_GetBuiltWithAssert(void)
{
#ifdef NDEBUG
    return 0;
#else
    return 1;
#endif
}
