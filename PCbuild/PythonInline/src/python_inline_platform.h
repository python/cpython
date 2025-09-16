/* Python Inline Platform Utilities */
#ifndef PYTHON_INLINE_PLATFORM_H
#define PYTHON_INLINE_PLATFORM_H

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MS_WINDOWS
/* Windows-specific includes */
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

/* Platform-specific argument conversion utilities */
#ifndef MS_WINDOWS
/* Convert char** to wchar_t** for non-Windows platforms */
int python_inline_convert_args(int argc, char **argv_bytes, wchar_t ***argv_out);
void python_inline_free_converted_args(int argc, wchar_t **argv);
#endif

#ifdef __cplusplus
}
#endif

#endif /* PYTHON_INLINE_PLATFORM_H */
