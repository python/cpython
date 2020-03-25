#include "Python.h"

/* snprintf() wrappers.  If the platform has vsnprintf, we use it, else we
   emulate it in a half-hearted way.  Even if the platform has it, we wrap
   it because platforms differ in what vsnprintf does in case the buffer
   is too small:  C99 behavior is to return the number of characters that
   would have been written had the buffer not been too small, and to set
   the last byte of the buffer to \0.  At least MS _vsnprintf returns a
   negative value instead, and fills the entire buffer with non-\0 data.

   The wrappers ensure that str[size-1] is always \0 upon return.

   PyOS_snprintf and PyOS_vsnprintf never write more than size bytes
   (including the trailing '\0') into str.

   If the platform doesn't have vsnprintf, and the buffer size needed to
   avoid truncation exceeds size by more than 512, Python aborts with a
   Py_FatalError.

   Return value (rv):

    When 0 <= rv < size, the output conversion was unexceptional, and
    rv characters were written to str (excluding a trailing \0 byte at
    str[rv]).

    When rv >= size, output conversion was truncated, and a buffer of
    size rv+1 would have been needed to avoid truncation.  str[size-1]
    is \0 in this case.

    When rv < 0, "something bad happened".  str[size-1] is \0 in this
    case too, but the rest of str is unreliable.  It could be that
    an error in format codes was detected by libc, or on platforms
    with a non-C99 vsnprintf simply that the buffer wasn't big enough
    to avoid truncation, or on platforms without any vsnprintf that
    PyMem_Malloc couldn't obtain space for a temp buffer.

   CAUTION:  Unlike C99, str != NULL and size > 0 are required.
*/

int
PyOS_snprintf(char *str, size_t size, const  char  *format, ...)
{
    int rc;
    va_list va;

    va_start(va, format);
    rc = PyOS_vsnprintf(str, size, format, va);
    va_end(va);
    return rc;
}

int
PyOS_vsnprintf(char *str, size_t size, const char  *format, va_list va)
{
    int len;  /* # bytes written, excluding \0 */
#ifdef HAVE_SNPRINTF
#define _PyOS_vsnprintf_EXTRA_SPACE 1
#else
#define _PyOS_vsnprintf_EXTRA_SPACE 512
    char *buffer;
#endif
    assert(str != NULL);
    assert(size > 0);
    assert(format != NULL);
    /* We take a size_t as input but return an int.  Sanity check
     * our input so that it won't cause an overflow in the
     * vsnprintf return value or the buffer malloc size.  */
    if (size > INT_MAX - _PyOS_vsnprintf_EXTRA_SPACE) {
        len = -666;
        goto Done;
    }

#ifdef HAVE_SNPRINTF
    len = vsnprintf(str, size, format, va);
#else
    /* Emulate it. */
    buffer = PyMem_MALLOC(size + _PyOS_vsnprintf_EXTRA_SPACE);
    if (buffer == NULL) {
        len = -666;
        goto Done;
    }

    len = vsprintf(buffer, format, va);
    if (len < 0) {
        /* ignore the error */;
    }
    else if ((size_t)len >= size + _PyOS_vsnprintf_EXTRA_SPACE) {
        _Py_FatalErrorFunc(__func__, "Buffer overflow");
    }
    else {
        const size_t to_copy = (size_t)len < size ?
                                (size_t)len : size - 1;
        assert(to_copy < size);
        memcpy(str, buffer, to_copy);
        str[to_copy] = '\0';
    }
    PyMem_FREE(buffer);
#endif
Done:
    if (size > 0)
        str[size-1] = '\0';
    return len;
#undef _PyOS_vsnprintf_EXTRA_SPACE
}
