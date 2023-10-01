#ifndef Py_INITCONFIG_H
#define Py_INITCONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000

typedef struct PyInitConfig PyInitConfig;

// Create a new initialization configuration.
// It must be freed by PyInitConfig_Free().
// Return NULL on memory allocation failure.
//
// PyInitConfig_Python_New() has a Python configuration by default.
// PyInitConfig_Isolated_New() has an Isolated configuration by default.
PyAPI_FUNC(PyInitConfig*) PyInitConfig_Python_New(void);
PyAPI_FUNC(PyInitConfig*) PyInitConfig_Isolated_New(void);

// Free memory of a initialization configuration.
PyAPI_FUNC(void) PyInitConfig_Free(PyInitConfig *config);

// Set an integer configuration option.
// Return 0 on success, or return -1 on error.
PyAPI_FUNC(int) PyInitConfig_SetInt(
    PyInitConfig *config,
    const char *name,
    int64_t value);

// Set a string configuration option from a bytes string.
//
// The bytes string is decoded by Py_DecodeLocale(). Preinitialize Python if
// needed to ensure that encodings are properly configured.
//
// Return 0 on success, or return -1 on error.
PyAPI_FUNC(int) PyInitConfig_SetStr(
    PyInitConfig *config,
    const char *name,
    const char *value);

// Set a string configuration option from a wide string.
// Preinitialize Python if needed.
// Return 0 on success, or return -1 on error.
PyAPI_FUNC(int) PyInitConfig_SetWStr(
    PyInitConfig *config,
    const char *name,
    const wchar_t *value);

// Set a string list configuration option from bytes strings.
//
// The bytes strings are decoded by Py_DecodeLocale(). Preinitialize Python if
// needed to ensure that encodings are properly configured.
//
// Return 0 on success, or return -1 on error.
PyAPI_FUNC(int) PyInitConfig_SetStrList(
    PyInitConfig *config,
    const char *name,
    size_t length,
    char * const *items);

// Set a string list configuration option from a wide strings.
// Preinitialize Python if needed.
// Return 0 on success, or return -1 on error.
PyAPI_FUNC(int) PyInitConfig_SetWStrList(
    PyInitConfig *config,
    const char *name,
    size_t length,
    wchar_t * const *items);

// Initialize Python from the initialization configuration.
// Return 0 on success.
// Return -1 if Python wants to exit and on error
PyAPI_FUNC(int) Py_InitializeFromInitConfig(PyInitConfig *config);

// Return non-zero if an error was set or if an exit code was set.
// Return zero otherwise.
PyAPI_FUNC(int) PyInitConfig_Exception(PyInitConfig* config);

// Get the error message.
// Set *err_msg and return 1 if an error is set.
// Set *err_msg to NULL and return 0 otherwise.
PyAPI_FUNC(int) PyInitConfig_GetError(PyInitConfig* config, const char **err_msg);

// Get the exit code.
// Set '*exitcode' and return 1 if an exit code is set.
// Return 0 otherwise.
PyAPI_FUNC(int) PyInitConfig_GetExitCode(PyInitConfig* config, int *exitcode);

// Exit Python and free memory of a initialization configuration.
// The function does not return.
PyAPI_FUNC(void) _Py_NO_RETURN Py_ExitWithInitConfig(PyInitConfig *config);

#endif  // !Py_LIMITED_API


#ifndef Py_LIMITED_API
#  define Py_CPYTHON_INITCONFIG_H
#  include "cpython/initconfig.h"
#  undef Py_CPYTHON_INITCONFIG_H
#endif

#ifdef __cplusplus
}
#endif
#endif   // !Py_INITCONFIG_H
