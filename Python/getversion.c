
/* Return the full version string. */

#include "Python.h"
#include "pycore_initconfig.h"    // _PyStatus_NO_MEMORY()


static int initialized = 0;
// Use the smallest possible buffer: it's only needed if malloc() fails *and*
// Py_GetVersion() is called before Py_Initialize(). The buffer should be big
// enough to store "3.16.0a0 " string.
static char static_version[20];
static char *heap_version = NULL;


PyStatus
_Py_GetVersion_Init(void)
{
    if (initialized) {
        return _PyStatus_OK();
    }

#ifdef Py_GIL_DISABLED
    const char *format = "%s free-threading build (%s) %s";
    size_t format_len = strlen(" free-threading build () ");
#else
    const char *format = "%s (%s) %s";
    size_t format_len = strlen(" () ");
#endif
    const char *version_str = PY_VERSION;
    const char *buildinfo = Py_GetBuildInfo();
    const char *compiler = Py_GetCompiler();
    // +1 for the trailing NUL byte
    size_t len = (format_len + strlen(version_str) + strlen(buildinfo)
                  + strlen(compiler) + 1);

    // Always format the static version
    PyOS_snprintf(static_version, sizeof(static_version), format,
                  version_str, buildinfo, compiler);

    heap_version = malloc(len);
    if (heap_version == NULL) {
        // If malloc() failed, don't set initialized to 1, so next
        // Py_GetVersion() will try again to allocate memory.
        return _PyStatus_NO_MEMORY();
    }

    PyOS_snprintf(heap_version, len, format,
                  version_str, buildinfo, compiler);
    initialized = 1;
    return _PyStatus_OK();
}

void
_Py_GetVersion_Fini(void)
{
    if (heap_version) {
        free(heap_version);
        heap_version = NULL;
    }
}

const char *
Py_GetVersion(void)
{
    PyStatus status = _Py_GetVersion_Init();
    // Ignore error: Py_GetVersion() API cannot report error
    (void)status;

    if (heap_version) {
        return heap_version;
    }
    else {
        return static_version;
    }
}

// Export the Python hex version as a constant.
const unsigned long Py_Version = PY_VERSION_HEX;
