#include "Python.h"

/* snprintf() and vsnprintf() wrappers.

   We wrap vsnprintf() to guarantee consistent behavior in corner cases:
   C99 behavior is to return the number of characters that would have been
   written had the buffer not been too small, and to set the last byte of the
   buffer to \0.
   Unlike C99, our wrappers do not support passing a null buffer.

   The wrappers ensure that str[size-1] is always \0 upon return.

   PyOS_snprintf and PyOS_vsnprintf never write more than size bytes
   (including the trailing '\0') into str.

   Return value (rv):

    When 0 <= rv < size, the output conversion was unexceptional, and
    rv characters were written to str (excluding a trailing \0 byte at
    str[rv]).

    When rv >= size, output conversion was truncated, and a buffer of
    size rv+1 would have been needed to avoid truncation.  str[size-1]
    is \0 in this case.

    When rv < 0, "something bad happened".  str[size-1] is \0 in this
    case too, but the rest of str is unreliable.  It could be that
    an error in format codes was detected by libc.

   CAUTION:  Unlike C99, str != NULL and size > 0 are required.
   Also, size must be smaller than INT_MAX.
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
    assert(str != NULL);
    assert(size > 0);
    assert(size <= (INT_MAX - 1));
    assert(format != NULL);

    int len;  /* # bytes written, excluding \0 */
    /* We take a size_t as input but return an int.  Sanity check
     * our input so that it won't cause an overflow in the
     * vsnprintf return value.  */
    if (size > INT_MAX - 1) {
        len = -666;
        goto Done;
    }

    len = vsnprintf(str, size, format, va);

Done:
    if (size > 0) {
        str[size-1] = '\0';
    }
    return len;
}
