
/* Return the full version string. */

#include "Python.h"

#include "patchlevel.h"

static int initialized = 0;
static char version[258];

void _Py_InitVersion(void)
{
    if (initialized) {
        return;
    }
    initialized = 1;
#ifdef Py_NOGIL
    const char *gil = " [NOGIL]";  // 8 characters
#else
    const char *gil = "";
#endif
    PyOS_snprintf(version, sizeof(version), "%.80s (%.80s) %.80s%s",
                  PY_VERSION, Py_GetBuildInfo(), Py_GetCompiler(), gil);
}

const char *
Py_GetVersion(void)
{
    _Py_InitVersion();
    return version;
}

// Export the Python hex version as a constant.
const unsigned long Py_Version = PY_VERSION_HEX;
