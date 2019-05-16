
/* Return the full version string. */

#include "Python.h"

#include "patchlevel.h"

const char *
Py_GetVersion(void)
{
    static char version[250];  // Static is okay here (buffer, non-threaded?).
    PyOS_snprintf(version, sizeof(version), "%.80s (%.80s) %.80s",
                  PY_VERSION, Py_GetBuildInfo(), Py_GetCompiler());
    return version;
}
