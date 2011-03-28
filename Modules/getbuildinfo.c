#include "Python.h"

#ifndef DONT_HAVE_STDIO_H
#include <stdio.h>
#endif

#ifndef DATE
#ifdef __DATE__
#define DATE __DATE__
#else
#define DATE "xx/xx/xx"
#endif
#endif

#ifndef TIME
#ifdef __TIME__
#define TIME __TIME__
#else
#define TIME "xx:xx:xx"
#endif
#endif

/* on unix, SVNVERSION is passed on the command line.
 * on Windows, the string is interpolated using
 * subwcrev.exe
 */
#ifndef SVNVERSION
#define SVNVERSION "$WCRANGE$$WCMODS?M:$"
#endif

/* XXX Only unix build process has been tested */
#ifndef HGVERSION
#define HGVERSION ""
#endif
#ifndef HGTAG
#define HGTAG ""
#endif
#ifndef HGBRANCH
#define HGBRANCH ""
#endif

const char *
Py_GetBuildInfo(void)
{
    static char buildinfo[50 + sizeof(HGVERSION) +
                          ((sizeof(HGTAG) > sizeof(HGBRANCH)) ?
                           sizeof(HGTAG) : sizeof(HGBRANCH))];
    const char *revision = _Py_hgversion();
    const char *sep = *revision ? ":" : "";
    const char *hgid = _Py_hgidentifier();
    if (!(*hgid))
        hgid = "default";
    PyOS_snprintf(buildinfo, sizeof(buildinfo),
                  "%s%s%s, %.20s, %.9s", hgid, sep, revision,
                  DATE, TIME);
    return buildinfo;
}

const char *
_Py_svnversion(void)
{
    /* the following string can be modified by subwcrev.exe */
    static const char svnversion[] = SVNVERSION;
    if (svnversion[0] != '$')
        return svnversion; /* it was interpolated, or passed on command line */
    return "Unversioned directory";
}

const char *
_Py_hgversion(void)
{
    return HGVERSION;
}

const char *
_Py_hgidentifier(void)
{
    const char *hgtag, *hgid;
    hgtag = HGTAG;
    if ((*hgtag) && strcmp(hgtag, "tip") != 0)
        hgid = hgtag;
    else
        hgid = HGBRANCH;
    return hgid;
}
