
#include "Python.h"

/* snprintf() emulation for platforms which don't have it (yet). 
   
   Return value

       The number of characters printed (not including the trailing
       `\0' used to end output to strings) or a negative number in
       case of an error.

       PyOS_snprintf and PyOS_vsnprintf do not write more than size
       bytes (including the trailing '\0').

       If the output would have been truncated, they return the number
       of characters (excluding the trailing '\0') which would have
       been written to the final string if enough space had been
       available. This is inline with the C99 standard.

*/

#include <ctype.h>

#ifndef HAVE_SNPRINTF

static
int myvsnprintf(char *str, size_t size, const char  *format, va_list va)
{
    char *buffer = PyMem_Malloc(size + 512);
    int len;
    
    if (buffer == NULL)
	return -1;
    len = vsprintf(buffer, format, va);
    if (len < 0) {
	PyMem_Free(buffer);
	return len;
    }
    len++;
    assert(len >= 0);
    if ((size_t)len > size + 512)
	Py_FatalError("Buffer overflow in PyOS_snprintf/PyOS_vsnprintf");
    if ((size_t)len > size)
	buffer[size-1] = '\0';
    else
	size = len;
    memcpy(str, buffer, size);
    PyMem_Free(buffer);
    return len - 1;
}

int PyOS_snprintf(char *str, size_t size, const  char  *format, ...)
{
    int rc;
    va_list va;

    va_start(va, format);
    rc = myvsnprintf(str, size, format, va);
    va_end(va);
    return rc;
}

int PyOS_vsnprintf(char *str, size_t size, const char  *format, va_list va)
{
    return myvsnprintf(str, size, format, va);
}

#else

/* Make sure that a C API is included in the lib */

#ifdef PyOS_snprintf
# undef PyOS_snprintf
#endif

int PyOS_snprintf(char *str, size_t size, const  char  *format, ...)
{
    int rc;
    va_list va;

    va_start(va, format);
    rc = vsnprintf(str, size, format, va);
    va_end(va);
    return rc;
}

#ifdef PyOS_vsnprintf
# undef PyOS_vsnprintf
#endif

int PyOS_vsnprintf(char *str, size_t size, const char  *format, va_list va)
{
    return vsnprintf(str, size, format, va);
}

#endif

