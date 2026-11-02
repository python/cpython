#ifdef _MSC_VER

#include <stdlib.h>

/* pycore_fileutils.h uses this function in the _Py_BEGIN/END_SUPPRESS_IPH
 * macros.
 */

static void __cdecl _silent_invalid_parameter_handler(
    wchar_t const* expression,
    wchar_t const* function,
    wchar_t const* file,
    unsigned int line,
    uintptr_t pReserved) { }

_invalid_parameter_handler _Py_silent_invalid_parameter_handler = _silent_invalid_parameter_handler;

#endif
