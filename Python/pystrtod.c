/* -*- Mode: C; c-file-style: "python" -*- */

#include <Python.h>
#include "pycore_dtoa.h"
#include <locale.h>

/* Case-insensitive string match used for nan and inf detection; t should be
   lower-case.  Returns 1 for a successful match, 0 otherwise. */

static int
case_insensitive_match(const char *s, const char *t)
{
    while(*t && Py_TOLOWER(*s) == *t) {
        s++;
        t++;
    }
    return *t ? 0 : 1;
}

/* _Py_parse_inf_or_nan: Attempt to parse a string of the form "nan", "inf" or
   "infinity", with an optional leading sign of "+" or "-".  On success,
   return the NaN or Infinity as a double and set *endptr to point just beyond
   the successfully parsed portion of the string.  On failure, return -1.0 and
   set *endptr to point to the start of the string. */

#ifndef PY_NO_SHORT_FLOAT_REPR

double
_Py_parse_inf_or_nan(const char *p, char **endptr)
{
    double retval;
    const char *s;
    int negate = 0;

    s = p;
    if (*s == '-') {
        negate = 1;
        s++;
    }
    else if (*s == '+') {
        s++;
    }
    if (case_insensitive_match(s, "inf")) {
        s += 3;
        if (case_insensitive_match(s, "inity"))
            s += 5;
        retval = _Py_dg_infinity(negate);
    }
    else if (case_insensitive_match(s, "nan")) {
        s += 3;
        retval = _Py_dg_stdnan(negate);
    }
    else {
        s = p;
        retval = -1.0;
    }
    *endptr = (char *)s;
    return retval;
}

#else

double
_Py_parse_inf_or_nan(const char *p, char **endptr)
{
    double retval;
    const char *s;
    int negate = 0;

    s = p;
    if (*s == '-') {
        negate = 1;
        s++;
    }
    else if (*s == '+') {
        s++;
    }
    if (case_insensitive_match(s, "inf")) {
        s += 3;
        if (case_insensitive_match(s, "inity"))
            s += 5;
        retval = negate ? -Py_HUGE_VAL : Py_HUGE_VAL;
    }
#ifdef Py_NAN
    else if (case_insensitive_match(s, "nan")) {
        s += 3;
        retval = negate ? -Py_NAN : Py_NAN;
    }
#endif
    else {
        s = p;
        retval = -1.0;
    }
    *endptr = (char *)s;
    return retval;
}

#endif

/**
 * _PyOS_ascii_strtod:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  if non-%NULL, it returns the character after
 *           the last character used in the conversion.
 *
 * Converts a string to a #gdouble value.
 * This function behaves like the standard strtod() function
 * does in the C locale. It does this without actually
 * changing the current locale, since that would not be
 * thread-safe.
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtod() function.
 *
 * If the correct value would cause overflow, plus or minus %HUGE_VAL
 * is returned (according to the sign of the value), and %ERANGE is
 * stored in %errno. If the correct value would cause underflow,
 * zero is returned and %ERANGE is stored in %errno.
 * If memory allocation fails, %ENOMEM is stored in %errno.
 *
 * This function resets %errno before calling strtod() so that
 * you can reliably detect overflow and underflow.
 *
 * Return value: the #gdouble value.
 **/

#ifndef PY_NO_SHORT_FLOAT_REPR

static double
_PyOS_ascii_strtod(const char *nptr, char **endptr)
{
    double result;
    _Py_SET_53BIT_PRECISION_HEADER;

    assert(nptr != NULL);
    /* Set errno to zero, so that we can distinguish zero results
       and underflows */
    errno = 0;

    _Py_SET_53BIT_PRECISION_START;
    result = _Py_dg_strtod(nptr, endptr);
    _Py_SET_53BIT_PRECISION_END;

    if (*endptr == nptr)
        /* string might represent an inf or nan */
        result = _Py_parse_inf_or_nan(nptr, endptr);

    return result;

}

#else

/*
   Use system strtod;  since strtod is locale aware, we may
   have to first fix the decimal separator.

   Note that unlike _Py_dg_strtod, the system strtod may not always give
   correctly rounded results.
*/

static double
_PyOS_ascii_strtod(const char *nptr, char **endptr)
{
    char *fail_pos;
    double val;
    struct lconv *locale_data;
    const char *decimal_point;
    size_t decimal_point_len;
    const char *p, *decimal_point_pos;
    const char *end = NULL; /* Silence gcc */
    const char *digits_pos = NULL;
    int negate = 0;

    assert(nptr != NULL);

    fail_pos = NULL;

    locale_data = localeconv();
    decimal_point = locale_data->decimal_point;
    decimal_point_len = strlen(decimal_point);

    assert(decimal_point_len != 0);

    decimal_point_pos = NULL;

    /* Parse infinities and nans */
    val = _Py_parse_inf_or_nan(nptr, endptr);
    if (*endptr != nptr)
        return val;

    /* Set errno to zero, so that we can distinguish zero results
       and underflows */
    errno = 0;

    /* We process the optional sign manually, then pass the remainder to
       the system strtod.  This ensures that the result of an underflow
       has the correct sign. (bug #1725)  */
    p = nptr;
    /* Process leading sign, if present */
    if (*p == '-') {
        negate = 1;
        p++;
    }
    else if (*p == '+') {
        p++;
    }

    /* Some platform strtods accept hex floats; Python shouldn't (at the
       moment), so we check explicitly for strings starting with '0x'. */
    if (*p == '0' && (*(p+1) == 'x' || *(p+1) == 'X'))
        goto invalid_string;

    /* Check that what's left begins with a digit or decimal point */
    if (!Py_ISDIGIT(*p) && *p != '.')
        goto invalid_string;

    digits_pos = p;
    if (decimal_point[0] != '.' ||
        decimal_point[1] != 0)
    {
        /* Look for a '.' in the input; if present, it'll need to be
           swapped for the current locale's decimal point before we
           call strtod.  On the other hand, if we find the current
           locale's decimal point then the input is invalid. */
        while (Py_ISDIGIT(*p))
            p++;

        if (*p == '.')
        {
            decimal_point_pos = p++;

            /* locate end of number */
            while (Py_ISDIGIT(*p))
                p++;

            if (*p == 'e' || *p == 'E')
                p++;
            if (*p == '+' || *p == '-')
                p++;
            while (Py_ISDIGIT(*p))
                p++;
            end = p;
        }
        else if (strncmp(p, decimal_point, decimal_point_len) == 0)
            /* Python bug #1417699 */
            goto invalid_string;
        /* For the other cases, we need not convert the decimal
           point */
    }

    if (decimal_point_pos) {
        char *copy, *c;
        /* Create a copy of the input, with the '.' converted to the
           locale-specific decimal point */
        copy = (char *)PyMem_Malloc(end - digits_pos +
                                    1 + decimal_point_len);
        if (copy == NULL) {
            *endptr = (char *)nptr;
            errno = ENOMEM;
            return val;
        }

        c = copy;
        memcpy(c, digits_pos, decimal_point_pos - digits_pos);
        c += decimal_point_pos - digits_pos;
        memcpy(c, decimal_point, decimal_point_len);
        c += decimal_point_len;
        memcpy(c, decimal_point_pos + 1,
               end - (decimal_point_pos + 1));
        c += end - (decimal_point_pos + 1);
        *c = 0;

        val = strtod(copy, &fail_pos);

        if (fail_pos)
        {
            if (fail_pos > decimal_point_pos)
                fail_pos = (char *)digits_pos +
                    (fail_pos - copy) -
                    (decimal_point_len - 1);
            else
                fail_pos = (char *)digits_pos +
                    (fail_pos - copy);
        }

        PyMem_Free(copy);

    }
    else {
        val = strtod(digits_pos, &fail_pos);
    }

    if (fail_pos == digits_pos)
        goto invalid_string;

    if (negate && fail_pos != nptr)
        val = -val;
    *endptr = fail_pos;

    return val;

  invalid_string:
    *endptr = (char*)nptr;
    errno = EINVAL;
    return -1.0;
}

#endif

/* PyOS_string_to_double converts a null-terminated byte string s (interpreted
   as a string of ASCII characters) to a float.  The string should not have
   leading or trailing whitespace.  The conversion is independent of the
   current locale.

   If endptr is NULL, try to convert the whole string.  Raise ValueError and
   return -1.0 if the string is not a valid representation of a floating-point
   number.

   If endptr is non-NULL, try to convert as much of the string as possible.
   If no initial segment of the string is the valid representation of a
   floating-point number then *endptr is set to point to the beginning of the
   string, -1.0 is returned and again ValueError is raised.

   On overflow (e.g., when trying to convert '1e500' on an IEEE 754 machine),
   if overflow_exception is NULL then +-Py_HUGE_VAL is returned, and no Python
   exception is raised.  Otherwise, overflow_exception should point to
   a Python exception, this exception will be raised, -1.0 will be returned,
   and *endptr will point just past the end of the converted value.

   If any other failure occurs (for example lack of memory), -1.0 is returned
   and the appropriate Python exception will have been set.
*/

double
PyOS_string_to_double(const char *s,
                      char **endptr,
                      PyObject *overflow_exception)
{
    double x, result=-1.0;
    char *fail_pos;

    errno = 0;
    x = _PyOS_ascii_strtod(s, &fail_pos);

    if (errno == ENOMEM) {
        PyErr_NoMemory();
        fail_pos = (char *)s;
    }
    else if (!endptr && (fail_pos == s || *fail_pos != '\0'))
        PyErr_Format(PyExc_ValueError,
                      "could not convert string to float: "
                      "'%.200s'", s);
    else if (fail_pos == s)
        PyErr_Format(PyExc_ValueError,
                      "could not convert string to float: "
                      "'%.200s'", s);
    else if (errno == ERANGE && fabs(x) >= 1.0 && overflow_exception)
        PyErr_Format(overflow_exception,
                      "value too large to convert to float: "
                      "'%.200s'", s);
    else
        result = x;

    if (endptr != NULL)
        *endptr = fail_pos;
    return result;
}

/* Remove underscores that follow the underscore placement rule from
   the string and then call the `innerfunc` function on the result.
   It should return a new object or NULL on exception.

   `what` is used for the error message emitted when underscores are detected
   that don't follow the rule. `arg` is an opaque pointer passed to the inner
   function.

   This is used to implement underscore-agnostic conversion for floats
   and complex numbers.
*/
PyObject *
_Py_string_to_number_with_underscores(
    const char *s, Py_ssize_t orig_len, const char *what, PyObject *obj, void *arg,
    PyObject *(*innerfunc)(const char *, Py_ssize_t, void *))
{
    char prev;
    const char *p, *last;
    char *dup, *end;
    PyObject *result;

    assert(s[orig_len] == '\0');

    if (strchr(s, '_') == NULL) {
        return innerfunc(s, orig_len, arg);
    }

    dup = PyMem_Malloc(orig_len + 1);
    if (dup == NULL) {
        return PyErr_NoMemory();
    }
    end = dup;
    prev = '\0';
    last = s + orig_len;
    for (p = s; *p; p++) {
        if (*p == '_') {
            /* Underscores are only allowed after digits. */
            if (!(prev >= '0' && prev <= '9')) {
                goto error;
            }
        }
        else {
            *end++ = *p;
            /* Underscores are only allowed before digits. */
            if (prev == '_' && !(*p >= '0' && *p <= '9')) {
                goto error;
            }
        }
        prev = *p;
    }
    /* Underscores are not allowed at the end. */
    if (prev == '_') {
        goto error;
    }
    /* No embedded NULs allowed. */
    if (p != last) {
        goto error;
    }
    *end = '\0';
    result = innerfunc(dup, end - dup, arg);
    PyMem_Free(dup);
    return result;

  error:
    PyMem_Free(dup);
    PyErr_Format(PyExc_ValueError,
                 "could not convert string to %s: "
                 "%R", what, obj);
    return NULL;
}

#ifdef PY_NO_SHORT_FLOAT_REPR

/* Given a string that may have a decimal point in the current
   locale, change it back to a dot.  Since the string cannot get
   longer, no need for a maximum buffer size parameter. */
Py_LOCAL_INLINE(void)
change_decimal_from_locale_to_dot(char* buffer)
{
    struct lconv *locale_data = localeconv();
    const char *decimal_point = locale_data->decimal_point;

    if (decimal_point[0] != '.' || decimal_point[1] != 0) {
        size_t decimal_point_len = strlen(decimal_point);

        if (*buffer == '+' || *buffer == '-')
            buffer++;
        while (Py_ISDIGIT(*buffer))
            buffer++;
        if (strncmp(buffer, decimal_point, decimal_point_len) == 0) {
            *buffer = '.';
            buffer++;
            if (decimal_point_len > 1) {
                /* buffer needs to get smaller */
                size_t rest_len = strlen(buffer +
                                     (decimal_point_len - 1));
                memmove(buffer,
                    buffer + (decimal_point_len - 1),
                    rest_len);
                buffer[rest_len] = 0;
            }
        }
    }
}


/* From the C99 standard, section 7.19.6:
The exponent always contains at least two digits, and only as many more digits
as necessary to represent the exponent.
*/
#define MIN_EXPONENT_DIGITS 2

/* Ensure that any exponent, if present, is at least MIN_EXPONENT_DIGITS
   in length. */
Py_LOCAL_INLINE(void)
ensure_minimum_exponent_length(char* buffer, size_t buf_size)
{
    char *p = strpbrk(buffer, "eE");
    if (p && (*(p + 1) == '-' || *(p + 1) == '+')) {
        char *start = p + 2;
        int exponent_digit_cnt = 0;
        int leading_zero_cnt = 0;
        int in_leading_zeros = 1;
        int significant_digit_cnt;

        /* Skip over the exponent and the sign. */
        p += 2;

        /* Find the end of the exponent, keeping track of leading
           zeros. */
        while (*p && Py_ISDIGIT(*p)) {
            if (in_leading_zeros && *p == '0')
                ++leading_zero_cnt;
            if (*p != '0')
                in_leading_zeros = 0;
            ++p;
            ++exponent_digit_cnt;
        }

        significant_digit_cnt = exponent_digit_cnt - leading_zero_cnt;
        if (exponent_digit_cnt == MIN_EXPONENT_DIGITS) {
            /* If there are 2 exactly digits, we're done,
               regardless of what they contain */
        }
        else if (exponent_digit_cnt > MIN_EXPONENT_DIGITS) {
            int extra_zeros_cnt;

            /* There are more than 2 digits in the exponent.  See
               if we can delete some of the leading zeros */
            if (significant_digit_cnt < MIN_EXPONENT_DIGITS)
                significant_digit_cnt = MIN_EXPONENT_DIGITS;
            extra_zeros_cnt = exponent_digit_cnt -
                significant_digit_cnt;

            /* Delete extra_zeros_cnt worth of characters from the
               front of the exponent */
            assert(extra_zeros_cnt >= 0);

            /* Add one to significant_digit_cnt to copy the
               trailing 0 byte, thus setting the length */
            memmove(start,
                start + extra_zeros_cnt,
                significant_digit_cnt + 1);
        }
        else {
            /* If there are fewer than 2 digits, add zeros
               until there are 2, if there's enough room */
            int zeros = MIN_EXPONENT_DIGITS - exponent_digit_cnt;
            if (start + zeros + exponent_digit_cnt + 1
                  < buffer + buf_size) {
                memmove(start + zeros, start,
                    exponent_digit_cnt + 1);
                memset(start, '0', zeros);
            }
        }
    }
}

/* Remove trailing zeros after the decimal point from a numeric string; also
   remove the decimal point if all digits following it are zero.  The numeric
   string must end in '\0', and should not have any leading or trailing
   whitespace.  Assumes that the decimal point is '.'. */
Py_LOCAL_INLINE(void)
remove_trailing_zeros(char *buffer)
{
    char *old_fraction_end, *new_fraction_end, *end, *p;

    p = buffer;
    if (*p == '-' || *p == '+')
        /* Skip leading sign, if present */
        ++p;
    while (Py_ISDIGIT(*p))
        ++p;

    /* if there's no decimal point there's nothing to do */
    if (*p++ != '.')
        return;

    /* scan any digits after the point */
    while (Py_ISDIGIT(*p))
        ++p;
    old_fraction_end = p;

    /* scan up to ending '\0' */
    while (*p != '\0')
        p++;
    /* +1 to make sure that we move the null byte as well */
    end = p+1;

    /* scan back from fraction_end, looking for removable zeros */
    p = old_fraction_end;
    while (*(p-1) == '0')
        --p;
    /* and remove point if we've got that far */
    if (*(p-1) == '.')
        --p;
    new_fraction_end = p;

    memmove(new_fraction_end, old_fraction_end, end-old_fraction_end);
}

/* Ensure that buffer has a decimal point in it.  The decimal point will not
   be in the current locale, it will always be '.'. Don't add a decimal point
   if an exponent is present.  Also, convert to exponential notation where
   adding a '.0' would produce too many significant digits (see issue 5864).

   Returns a pointer to the fixed buffer, or NULL on failure.
*/
Py_LOCAL_INLINE(char *)
ensure_decimal_point(char* buffer, size_t buf_size, int precision)
{
    int digit_count, insert_count = 0, convert_to_exp = 0;
    const char *chars_to_insert;
    char *digits_start;

    /* search for the first non-digit character */
    char *p = buffer;
    if (*p == '-' || *p == '+')
        /* Skip leading sign, if present.  I think this could only
           ever be '-', but it can't hurt to check for both. */
        ++p;
    digits_start = p;
    while (*p && Py_ISDIGIT(*p))
        ++p;
    digit_count = Py_SAFE_DOWNCAST(p - digits_start, Py_ssize_t, int);

    if (*p == '.') {
        if (Py_ISDIGIT(*(p+1))) {
            /* Nothing to do, we already have a decimal
               point and a digit after it */
        }
        else {
            /* We have a decimal point, but no following
               digit.  Insert a zero after the decimal. */
            /* can't ever get here via PyOS_double_to_string */
            assert(precision == -1);
            ++p;
            chars_to_insert = "0";
            insert_count = 1;
        }
    }
    else if (!(*p == 'e' || *p == 'E')) {
        /* Don't add ".0" if we have an exponent. */
        if (digit_count == precision) {
            /* issue 5864: don't add a trailing .0 in the case
               where the '%g'-formatted result already has as many
               significant digits as were requested.  Switch to
               exponential notation instead. */
            convert_to_exp = 1;
            /* no exponent, no point, and we shouldn't land here
               for infs and nans, so we must be at the end of the
               string. */
            assert(*p == '\0');
        }
        else {
            assert(precision == -1 || digit_count < precision);
            chars_to_insert = ".0";
            insert_count = 2;
        }
    }
    if (insert_count) {
        size_t buf_len = strlen(buffer);
        if (buf_len + insert_count + 1 >= buf_size) {
            /* If there is not enough room in the buffer
               for the additional text, just skip it.  It's
               not worth generating an error over. */
        }
        else {
            memmove(p + insert_count, p,
                buffer + strlen(buffer) - p + 1);
            memcpy(p, chars_to_insert, insert_count);
        }
    }
    if (convert_to_exp) {
        int written;
        size_t buf_avail;
        p = digits_start;
        /* insert decimal point */
        assert(digit_count >= 1);
        memmove(p+2, p+1, digit_count); /* safe, but overwrites nul */
        p[1] = '.';
        p += digit_count+1;
        assert(p <= buf_size+buffer);
        buf_avail = buf_size+buffer-p;
        if (buf_avail == 0)
            return NULL;
        /* Add exponent.  It's okay to use lower case 'e': we only
           arrive here as a result of using the empty format code or
           repr/str builtins and those never want an upper case 'E' */
        written = PyOS_snprintf(p, buf_avail, "e%+.02d", digit_count-1);
        if (!(0 <= written &&
              written < Py_SAFE_DOWNCAST(buf_avail, size_t, int)))
            /* output truncated, or something else bad happened */
            return NULL;
        remove_trailing_zeros(buffer);
    }
    return buffer;
}

/* see FORMATBUFLEN in unicodeobject.c */
#define FLOAT_FORMATBUFLEN 120

/**
 * _PyOS_ascii_formatd:
 * @buffer: A buffer to place the resulting string in
 * @buf_size: The length of the buffer.
 * @format: The printf()-style format to use for the
 *          code to use for converting.
 * @d: The #gdouble to convert
 * @precision: The precision to use when formatting.
 *
 * Converts a #gdouble to a string, using the '.' as
 * decimal point. To format the number you pass in
 * a printf()-style format string. Allowed conversion
 * specifiers are 'e', 'E', 'f', 'F', 'g', 'G', and 'Z'.
 *
 * 'Z' is the same as 'g', except it always has a decimal and
 *     at least one digit after the decimal.
 *
 * Return value: The pointer to the buffer with the converted string.
 * On failure returns NULL but does not set any Python exception.
 **/
static char *
_PyOS_ascii_formatd(char       *buffer,
                   size_t      buf_size,
                   const char *format,
                   double      d,
                   int         precision)
{
    char format_char;
    size_t format_len = strlen(format);

    /* Issue 2264: code 'Z' requires copying the format.  'Z' is 'g', but
       also with at least one character past the decimal. */
    char tmp_format[FLOAT_FORMATBUFLEN];

    /* The last character in the format string must be the format char */
    format_char = format[format_len - 1];

    if (format[0] != '%')
        return NULL;

    /* I'm not sure why this test is here.  It's ensuring that the format
       string after the first character doesn't have a single quote, a
       lowercase l, or a percent. This is the reverse of the commented-out
       test about 10 lines ago. */
    if (strpbrk(format + 1, "'l%"))
        return NULL;

    /* Also curious about this function is that it accepts format strings
       like "%xg", which are invalid for floats.  In general, the
       interface to this function is not very good, but changing it is
       difficult because it's a public API. */

    if (!(format_char == 'e' || format_char == 'E' ||
          format_char == 'f' || format_char == 'F' ||
          format_char == 'g' || format_char == 'G' ||
          format_char == 'Z'))
        return NULL;

    /* Map 'Z' format_char to 'g', by copying the format string and
       replacing the final char with a 'g' */
    if (format_char == 'Z') {
        if (format_len + 1 >= sizeof(tmp_format)) {
            /* The format won't fit in our copy.  Error out.  In
               practice, this will never happen and will be
               detected by returning NULL */
            return NULL;
        }
        strcpy(tmp_format, format);
        tmp_format[format_len - 1] = 'g';
        format = tmp_format;
    }


    /* Have PyOS_snprintf do the hard work */
    PyOS_snprintf(buffer, buf_size, format, d);

    /* Do various fixups on the return string */

    /* Get the current locale, and find the decimal point string.
       Convert that string back to a dot. */
    change_decimal_from_locale_to_dot(buffer);

    /* If an exponent exists, ensure that the exponent is at least
       MIN_EXPONENT_DIGITS digits, providing the buffer is large enough
       for the extra zeros.  Also, if there are more than
       MIN_EXPONENT_DIGITS, remove as many zeros as possible until we get
       back to MIN_EXPONENT_DIGITS */
    ensure_minimum_exponent_length(buffer, buf_size);

    /* If format_char is 'Z', make sure we have at least one character
       after the decimal point (and make sure we have a decimal point);
       also switch to exponential notation in some edge cases where the
       extra character would produce more significant digits that we
       really want. */
    if (format_char == 'Z')
        buffer = ensure_decimal_point(buffer, buf_size, precision);

    return buffer;
}

/* The fallback code to use if _Py_dg_dtoa is not available. */

char * PyOS_double_to_string(double val,
                                         char format_code,
                                         int precision,
                                         int flags,
                                         int *type)
{
    char format[32];
    Py_ssize_t bufsize;
    char *buf;
    int t, exp;
    int upper = 0;

    /* Validate format_code, and map upper and lower case */
    switch (format_code) {
    case 'e':          /* exponent */
    case 'f':          /* fixed */
    case 'g':          /* general */
        break;
    case 'E':
        upper = 1;
        format_code = 'e';
        break;
    case 'F':
        upper = 1;
        format_code = 'f';
        break;
    case 'G':
        upper = 1;
        format_code = 'g';
        break;
    case 'r':          /* repr format */
        /* Supplied precision is unused, must be 0. */
        if (precision != 0) {
            PyErr_BadInternalCall();
            return NULL;
        }
        /* The repr() precision (17 significant decimal digits) is the
           minimal number that is guaranteed to have enough precision
           so that if the number is read back in the exact same binary
           value is recreated.  This is true for IEEE floating point
           by design, and also happens to work for all other modern
           hardware. */
        precision = 17;
        format_code = 'g';
        break;
    default:
        PyErr_BadInternalCall();
        return NULL;
    }

    /* Here's a quick-and-dirty calculation to figure out how big a buffer
       we need.  In general, for a finite float we need:

         1 byte for each digit of the decimal significand, and

         1 for a possible sign
         1 for a possible decimal point
         2 for a possible [eE][+-]
         1 for each digit of the exponent;  if we allow 19 digits
           total then we're safe up to exponents of 2**63.
         1 for the trailing nul byte

       This gives a total of 24 + the number of digits in the significand,
       and the number of digits in the significand is:

         for 'g' format: at most precision, except possibly
           when precision == 0, when it's 1.
         for 'e' format: precision+1
         for 'f' format: precision digits after the point, at least 1
           before.  To figure out how many digits appear before the point
           we have to examine the size of the number.  If fabs(val) < 1.0
           then there will be only one digit before the point.  If
           fabs(val) >= 1.0, then there are at most

         1+floor(log10(ceiling(fabs(val))))

           digits before the point (where the 'ceiling' allows for the
           possibility that the rounding rounds the integer part of val
           up).  A safe upper bound for the above quantity is
           1+floor(exp/3), where exp is the unique integer such that 0.5
           <= fabs(val)/2**exp < 1.0.  This exp can be obtained from
           frexp.

       So we allow room for precision+1 digits for all formats, plus an
       extra floor(exp/3) digits for 'f' format.

    */

    if (Py_IS_NAN(val) || Py_IS_INFINITY(val))
        /* 3 for 'inf'/'nan', 1 for sign, 1 for '\0' */
        bufsize = 5;
    else {
        bufsize = 25 + precision;
        if (format_code == 'f' && fabs(val) >= 1.0) {
            frexp(val, &exp);
            bufsize += exp/3;
        }
    }

    buf = PyMem_Malloc(bufsize);
    if (buf == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    /* Handle nan and inf. */
    if (Py_IS_NAN(val)) {
        strcpy(buf, "nan");
        t = Py_DTST_NAN;
    } else if (Py_IS_INFINITY(val)) {
        if (copysign(1., val) == 1.)
            strcpy(buf, "inf");
        else
            strcpy(buf, "-inf");
        t = Py_DTST_INFINITE;
    } else {
        t = Py_DTST_FINITE;
        if (flags & Py_DTSF_ADD_DOT_0)
            format_code = 'Z';

        PyOS_snprintf(format, sizeof(format), "%%%s.%i%c",
                      (flags & Py_DTSF_ALT ? "#" : ""), precision,
                      format_code);
        _PyOS_ascii_formatd(buf, bufsize, format, val, precision);
    }

    /* Add sign when requested.  It's convenient (esp. when formatting
     complex numbers) to include a sign even for inf and nan. */
    if (flags & Py_DTSF_SIGN && buf[0] != '-') {
        size_t len = strlen(buf);
        /* the bufsize calculations above should ensure that we've got
           space to add a sign */
        assert((size_t)bufsize >= len+2);
        memmove(buf+1, buf, len+1);
        buf[0] = '+';
    }
    if (upper) {
        /* Convert to upper case. */
        char *p1;
        for (p1 = buf; *p1; p1++)
            *p1 = Py_TOUPPER(*p1);
    }

    if (type)
        *type = t;
    return buf;
}

#else

/* _Py_dg_dtoa is available. */

/* I'm using a lookup table here so that I don't have to invent a non-locale
   specific way to convert to uppercase */
#define OFS_INF 0
#define OFS_NAN 1
#define OFS_E 2

/* The lengths of these are known to the code below, so don't change them */
static const char * const lc_float_strings[] = {
    "inf",
    "nan",
    "e",
};
static const char * const uc_float_strings[] = {
    "INF",
    "NAN",
    "E",
};


/* Convert a double d to a string, and return a PyMem_Malloc'd block of
   memory contain the resulting string.

   Arguments:
     d is the double to be converted
     format_code is one of 'e', 'f', 'g', 'r'.  'e', 'f' and 'g'
       correspond to '%e', '%f' and '%g';  'r' corresponds to repr.
     mode is one of '0', '2' or '3', and is completely determined by
       format_code: 'e' and 'g' use mode 2; 'f' mode 3, 'r' mode 0.
     precision is the desired precision
     always_add_sign is nonzero if a '+' sign should be included for positive
       numbers
     add_dot_0_if_integer is nonzero if integers in non-exponential form
       should have ".0" added.  Only applies to format codes 'r' and 'g'.
     use_alt_formatting is nonzero if alternative formatting should be
       used.  Only applies to format codes 'e', 'f' and 'g'.  For code 'g',
       at most one of use_alt_formatting and add_dot_0_if_integer should
       be nonzero.
     type, if non-NULL, will be set to one of these constants to identify
       the type of the 'd' argument:
     Py_DTST_FINITE
     Py_DTST_INFINITE
     Py_DTST_NAN

   Returns a PyMem_Malloc'd block of memory containing the resulting string,
    or NULL on error. If NULL is returned, the Python error has been set.
 */

static char *
format_float_short(double d, char format_code,
                   int mode, int precision,
                   int always_add_sign, int add_dot_0_if_integer,
                   int use_alt_formatting, const char * const *float_strings,
                   int *type)
{
    char *buf = NULL;
    char *p = NULL;
    Py_ssize_t bufsize = 0;
    char *digits, *digits_end;
    int decpt_as_int, sign, exp_len, exp = 0, use_exp = 0;
    Py_ssize_t decpt, digits_len, vdigits_start, vdigits_end;
    _Py_SET_53BIT_PRECISION_HEADER;

    /* _Py_dg_dtoa returns a digit string (no decimal point or exponent).
       Must be matched by a call to _Py_dg_freedtoa. */
    _Py_SET_53BIT_PRECISION_START;
    digits = _Py_dg_dtoa(d, mode, precision, &decpt_as_int, &sign,
                         &digits_end);
    _Py_SET_53BIT_PRECISION_END;

    decpt = (Py_ssize_t)decpt_as_int;
    if (digits == NULL) {
        /* The only failure mode is no memory. */
        PyErr_NoMemory();
        goto exit;
    }
    assert(digits_end != NULL && digits_end >= digits);
    digits_len = digits_end - digits;

    if (digits_len && !Py_ISDIGIT(digits[0])) {
        /* Infinities and nans here; adapt Gay's output,
           so convert Infinity to inf and NaN to nan, and
           ignore sign of nan. Then return. */

        /* ignore the actual sign of a nan */
        if (digits[0] == 'n' || digits[0] == 'N')
            sign = 0;

        /* We only need 5 bytes to hold the result "+inf\0" . */
        bufsize = 5; /* Used later in an assert. */
        buf = (char *)PyMem_Malloc(bufsize);
        if (buf == NULL) {
            PyErr_NoMemory();
            goto exit;
        }
        p = buf;

        if (sign == 1) {
            *p++ = '-';
        }
        else if (always_add_sign) {
            *p++ = '+';
        }
        if (digits[0] == 'i' || digits[0] == 'I') {
            strncpy(p, float_strings[OFS_INF], 3);
            p += 3;

            if (type)
                *type = Py_DTST_INFINITE;
        }
        else if (digits[0] == 'n' || digits[0] == 'N') {
            strncpy(p, float_strings[OFS_NAN], 3);
            p += 3;

            if (type)
                *type = Py_DTST_NAN;
        }
        else {
            /* shouldn't get here: Gay's code should always return
               something starting with a digit, an 'I',  or 'N' */
            Py_UNREACHABLE();
        }
        goto exit;
    }

    /* The result must be finite (not inf or nan). */
    if (type)
        *type = Py_DTST_FINITE;


    /* We got digits back, format them.  We may need to pad 'digits'
       either on the left or right (or both) with extra zeros, so in
       general the resulting string has the form

         [<sign>]<zeros><digits><zeros>[<exponent>]

       where either of the <zeros> pieces could be empty, and there's a
       decimal point that could appear either in <digits> or in the
       leading or trailing <zeros>.

       Imagine an infinite 'virtual' string vdigits, consisting of the
       string 'digits' (starting at index 0) padded on both the left and
       right with infinite strings of zeros.  We want to output a slice

         vdigits[vdigits_start : vdigits_end]

       of this virtual string.  Thus if vdigits_start < 0 then we'll end
       up producing some leading zeros; if vdigits_end > digits_len there
       will be trailing zeros in the output.  The next section of code
       determines whether to use an exponent or not, figures out the
       position 'decpt' of the decimal point, and computes 'vdigits_start'
       and 'vdigits_end'. */
    vdigits_end = digits_len;
    switch (format_code) {
    case 'e':
        use_exp = 1;
        vdigits_end = precision;
        break;
    case 'f':
        vdigits_end = decpt + precision;
        break;
    case 'g':
        if (decpt <= -4 || decpt >
            (add_dot_0_if_integer ? precision-1 : precision))
            use_exp = 1;
        if (use_alt_formatting)
            vdigits_end = precision;
        break;
    case 'r':
        /* convert to exponential format at 1e16.  We used to convert
           at 1e17, but that gives odd-looking results for some values
           when a 16-digit 'shortest' repr is padded with bogus zeros.
           For example, repr(2e16+8) would give 20000000000000010.0;
           the true value is 20000000000000008.0. */
        if (decpt <= -4 || decpt > 16)
            use_exp = 1;
        break;
    default:
        PyErr_BadInternalCall();
        goto exit;
    }

    /* if using an exponent, reset decimal point position to 1 and adjust
       exponent accordingly.*/
    if (use_exp) {
        exp = (int)decpt - 1;
        decpt = 1;
    }
    /* ensure vdigits_start < decpt <= vdigits_end, or vdigits_start <
       decpt < vdigits_end if add_dot_0_if_integer and no exponent */
    vdigits_start = decpt <= 0 ? decpt-1 : 0;
    if (!use_exp && add_dot_0_if_integer)
        vdigits_end = vdigits_end > decpt ? vdigits_end : decpt + 1;
    else
        vdigits_end = vdigits_end > decpt ? vdigits_end : decpt;

    /* double check inequalities */
    assert(vdigits_start <= 0 &&
           0 <= digits_len &&
           digits_len <= vdigits_end);
    /* decimal point should be in (vdigits_start, vdigits_end] */
    assert(vdigits_start < decpt && decpt <= vdigits_end);

    /* Compute an upper bound how much memory we need. This might be a few
       chars too long, but no big deal. */
    bufsize =
        /* sign, decimal point and trailing 0 byte */
        3 +

        /* total digit count (including zero padding on both sides) */
        (vdigits_end - vdigits_start) +

        /* exponent "e+100", max 3 numerical digits */
        (use_exp ? 5 : 0);

    /* Now allocate the memory and initialize p to point to the start of
       it. */
    buf = (char *)PyMem_Malloc(bufsize);
    if (buf == NULL) {
        PyErr_NoMemory();
        goto exit;
    }
    p = buf;

    /* Add a negative sign if negative, and a plus sign if non-negative
       and always_add_sign is true. */
    if (sign == 1)
        *p++ = '-';
    else if (always_add_sign)
        *p++ = '+';

    /* note that exactly one of the three 'if' conditions is true,
       so we include exactly one decimal point */
    /* Zero padding on left of digit string */
    if (decpt <= 0) {
        memset(p, '0', decpt-vdigits_start);
        p += decpt - vdigits_start;
        *p++ = '.';
        memset(p, '0', 0-decpt);
        p += 0-decpt;
    }
    else {
        memset(p, '0', 0-vdigits_start);
        p += 0 - vdigits_start;
    }

    /* Digits, with included decimal point */
    if (0 < decpt && decpt <= digits_len) {
        strncpy(p, digits, decpt-0);
        p += decpt-0;
        *p++ = '.';
        strncpy(p, digits+decpt, digits_len-decpt);
        p += digits_len-decpt;
    }
    else {
        strncpy(p, digits, digits_len);
        p += digits_len;
    }

    /* And zeros on the right */
    if (digits_len < decpt) {
        memset(p, '0', decpt-digits_len);
        p += decpt-digits_len;
        *p++ = '.';
        memset(p, '0', vdigits_end-decpt);
        p += vdigits_end-decpt;
    }
    else {
        memset(p, '0', vdigits_end-digits_len);
        p += vdigits_end-digits_len;
    }

    /* Delete a trailing decimal pt unless using alternative formatting. */
    if (p[-1] == '.' && !use_alt_formatting)
        p--;

    /* Now that we've done zero padding, add an exponent if needed. */
    if (use_exp) {
        *p++ = float_strings[OFS_E][0];
        exp_len = sprintf(p, "%+.02d", exp);
        p += exp_len;
    }
  exit:
    if (buf) {
        *p = '\0';
        /* It's too late if this fails, as we've already stepped on
           memory that isn't ours. But it's an okay debugging test. */
        assert(p-buf < bufsize);
    }
    if (digits)
        _Py_dg_freedtoa(digits);

    return buf;
}


char * PyOS_double_to_string(double val,
                                         char format_code,
                                         int precision,
                                         int flags,
                                         int *type)
{
    const char * const *float_strings = lc_float_strings;
    int mode;

    /* Validate format_code, and map upper and lower case. Compute the
       mode and make any adjustments as needed. */
    switch (format_code) {
    /* exponent */
    case 'E':
        float_strings = uc_float_strings;
        format_code = 'e';
        /* Fall through. */
    case 'e':
        mode = 2;
        precision++;
        break;

    /* fixed */
    case 'F':
        float_strings = uc_float_strings;
        format_code = 'f';
        /* Fall through. */
    case 'f':
        mode = 3;
        break;

    /* general */
    case 'G':
        float_strings = uc_float_strings;
        format_code = 'g';
        /* Fall through. */
    case 'g':
        mode = 2;
        /* precision 0 makes no sense for 'g' format; interpret as 1 */
        if (precision == 0)
            precision = 1;
        break;

    /* repr format */
    case 'r':
        mode = 0;
        /* Supplied precision is unused, must be 0. */
        if (precision != 0) {
            PyErr_BadInternalCall();
            return NULL;
        }
        break;

    default:
        PyErr_BadInternalCall();
        return NULL;
    }

    return format_float_short(val, format_code, mode, precision,
                              flags & Py_DTSF_SIGN,
                              flags & Py_DTSF_ADD_DOT_0,
                              flags & Py_DTSF_ALT,
                              float_strings, type);
}
#endif /* ifdef PY_NO_SHORT_FLOAT_REPR */
