
/* Return the full version string. */

#include "Python.h"

#include "patchlevel.h"

static int initialized = 0;
static char version[300];

void _Py_InitVersion(void)
{
    if (initialized) {
        return;
    }
    initialized = 1;
#ifdef Py_GIL_DISABLED
    const char *buildinfo_format = "%.80s experimental free-threading build (%.80s) %.80s";
#else
    const char *buildinfo_format = "%.80s (%.80s) %.80s";
#endif
    // The format string is defined above and is observably safe.
    // Ignore warnings related to non-literal format strings.
    #if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    #endif
    PyOS_snprintf(version, sizeof(version), buildinfo_format,
                  PY_VERSION, Py_GetBuildInfo(), Py_GetCompiler());
    #if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC diagnostic pop
    #endif
}

const char *
Py_GetVersion(void)
{
    _Py_InitVersion();
    return version;
}

// Export the Python hex version as a constant.
const unsigned long Py_Version = PY_VERSION_HEX;
