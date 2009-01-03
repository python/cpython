.. highlightlang:: c

.. _string-conversion:

String conversion and formatting
================================

Functions for number conversion and formatted string output.


.. cfunction:: int PyOS_snprintf(char *str, size_t size,  const char *format, ...)

   Output not more than *size* bytes to *str* according to the format string
   *format* and the extra arguments. See the Unix man page :manpage:`snprintf(2)`.


.. cfunction:: int PyOS_vsnprintf(char *str, size_t size, const char *format, va_list va)

   Output not more than *size* bytes to *str* according to the format string
   *format* and the variable argument list *va*. Unix man page
   :manpage:`vsnprintf(2)`.

:cfunc:`PyOS_snprintf` and :cfunc:`PyOS_vsnprintf` wrap the Standard C library
functions :cfunc:`snprintf` and :cfunc:`vsnprintf`. Their purpose is to
guarantee consistent behavior in corner cases, which the Standard C functions do
not.

The wrappers ensure that *str*[*size*-1] is always ``'\0'`` upon return. They
never write more than *size* bytes (including the trailing ``'\0'`` into str.
Both functions require that ``str != NULL``, ``size > 0`` and ``format !=
NULL``.

If the platform doesn't have :cfunc:`vsnprintf` and the buffer size needed to
avoid truncation exceeds *size* by more than 512 bytes, Python aborts with a
*Py_FatalError*.

The return value (*rv*) for these functions should be interpreted as follows:

* When ``0 <= rv < size``, the output conversion was successful and *rv*
  characters were written to *str* (excluding the trailing ``'\0'`` byte at
  *str*[*rv*]).

* When ``rv >= size``, the output conversion was truncated and a buffer with
  ``rv + 1`` bytes would have been needed to succeed. *str*[*size*-1] is ``'\0'``
  in this case.

* When ``rv < 0``, "something bad happened." *str*[*size*-1] is ``'\0'`` in
  this case too, but the rest of *str* is undefined. The exact cause of the error
  depends on the underlying platform.

The following functions provide locale-independent string to number conversions.


.. cfunction:: double PyOS_ascii_strtod(const char *nptr, char **endptr)

   Convert a string to a :ctype:`double`. This function behaves like the Standard C
   function :cfunc:`strtod` does in the C locale. It does this without changing the
   current locale, since that would not be thread-safe.

   :cfunc:`PyOS_ascii_strtod` should typically be used for reading configuration
   files or other non-user input that should be locale independent.

   .. versionadded:: 2.4

   See the Unix man page :manpage:`strtod(2)` for details.


.. cfunction:: char * PyOS_ascii_formatd(char *buffer, size_t buf_len, const char *format, double d)

   Convert a :ctype:`double` to a string using the ``'.'`` as the decimal
   separator. *format* is a :cfunc:`printf`\ -style format string specifying the
   number format. Allowed conversion characters are ``'e'``, ``'E'``, ``'f'``,
   ``'F'``, ``'g'`` and ``'G'``.

   The return value is a pointer to *buffer* with the converted string or NULL if
   the conversion failed.

   .. versionadded:: 2.4


.. cfunction:: double PyOS_ascii_atof(const char *nptr)

   Convert a string to a :ctype:`double` in a locale-independent way.

   .. versionadded:: 2.4

   See the Unix man page :manpage:`atof(2)` for details.


.. cfunction:: char * PyOS_stricmp(char *s1, char *s2)

   Case insensitive comparison of strings. The function works almost
   identically to :cfunc:`strcmp` except that it ignores the case.

   .. versionadded:: 2.6


.. cfunction:: char * PyOS_strnicmp(char *s1, char *s2, Py_ssize_t  size)

   Case insensitive comparison of strings. The function works almost
   identically to :cfunc:`strncmp` except that it ignores the case.

   .. versionadded:: 2.6
