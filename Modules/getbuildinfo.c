#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_pylifecycle.h"   // _Py_gitidentifier()

#ifndef DONT_HAVE_STDIO_H
#include <stdio.h>
#endif

#ifndef DATE
#ifdef __DATE__
#define DATE __DATE__
#else
#define DATE "Jan 01 1970"
#endif
#endif

#ifndef TIME
#ifdef __TIME__
#define TIME __TIME__
#else
#define TIME "00:00:00"
#endif
#endif

/* XXX Only unix build process has been tested */
#ifndef GITVERSION
#define GITVERSION ""
#endif
#ifndef GITTAG
#define GITTAG ""
#endif
#ifndef GITBRANCH
#define GITBRANCH ""
#endif

static int initialized = 0;
static char buildinfo[50 + sizeof(GITVERSION) +
                      ((sizeof(GITTAG) > sizeof(GITBRANCH)) ?
                       sizeof(GITTAG) : sizeof(GITBRANCH))];

const char *
Py_GetBuildInfo(void)
{
    if (initialized) {
        return buildinfo;
    }
    initialized = 1;
    const char *revision = _Py_gitversion();
    const char *sep = *revision ? ":" : "";
    const char *gitid = _Py_gitidentifier();
    if (!(*gitid)) {
        gitid = "main";
    }
    PyOS_snprintf(buildinfo, sizeof(buildinfo),
                  "%s%s%s, %.20s, %.9s", gitid, sep, revision,
                  DATE, TIME);
    return buildinfo;
}

const char *
_Py_gitversion(void)
{
    return GITVERSION;
}

const char *
_Py_gitidentifier(void)
{
    const char *gittag, *gitid;
    gittag = GITTAG;
    if ((*gittag) && strcmp(gittag, "undefined") != 0)
        gitid = gittag;
    else
        gitid = GITBRANCH;
    return gitid;
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
