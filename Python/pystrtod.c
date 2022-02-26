/* -*- Mode: C; c-file-style: "python" -*- */

#include <Python.h>
#include "pycore_dtoa.h"          // _Py_dg_strtod()
#include "pycore_pymath.h"        // _Py_SET_53BIT_PRECISION_START
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
