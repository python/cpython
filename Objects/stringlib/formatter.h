/* implements the string, long, and float formatters.  that is,
   string.__format__, etc. */

/* Before including this, you must include either:
   stringlib/unicodedefs.h
   stringlib/stringdefs.h

   Also, you should define the names:
   FORMAT_STRING
   FORMAT_LONG
   FORMAT_FLOAT
   to be whatever you want the public names of these functions to
   be.  These are the only non-static functions defined here.
*/

#define ALLOW_PARENS_FOR_SIGN 0

/*
    get_integer consumes 0 or more decimal digit characters from an
    input string, updates *result with the corresponding positive
    integer, and returns the number of digits consumed.

    returns -1 on error.
*/
static int
get_integer(STRINGLIB_CHAR **ptr, STRINGLIB_CHAR *end,
                  Py_ssize_t *result)
{
    Py_ssize_t accumulator, digitval, oldaccumulator;
    int numdigits;
    accumulator = numdigits = 0;
    for (;;(*ptr)++, numdigits++) {
        if (*ptr >= end)
            break;
        digitval = STRINGLIB_TODECIMAL(**ptr);
        if (digitval < 0)
            break;
        /*
           This trick was copied from old Unicode format code.  It's cute,
           but would really suck on an old machine with a slow divide
           implementation.  Fortunately, in the normal case we do not
           expect too many digits.
        */
        oldaccumulator = accumulator;
        accumulator *= 10;
        if ((accumulator+10)/10 != oldaccumulator+1) {
            PyErr_Format(PyExc_ValueError,
                         "Too many decimal digits in format string");
            return -1;
        }
        accumulator += digitval;
    }
    *result = accumulator;
    return numdigits;
}

/************************************************************************/
/*********** standard format specifier parsing **************************/
/************************************************************************/

/* returns true if this character is a specifier alignment token */
Py_LOCAL_INLINE(int)
is_alignment_token(STRINGLIB_CHAR c)
{
    switch (c) {
    case '<': case '>': case '=': case '^':
        return 1;
    default:
        return 0;
    }
}

/* returns true if this character is a sign element */
Py_LOCAL_INLINE(int)
is_sign_element(STRINGLIB_CHAR c)
{
    switch (c) {
    case ' ': case '+': case '-':
#if ALLOW_PARENS_FOR_SIGN
    case '(':
#endif
        return 1;
    default:
        return 0;
    }
}


typedef struct {
    STRINGLIB_CHAR fill_char;
    STRINGLIB_CHAR align;
    STRINGLIB_CHAR sign;
    Py_ssize_t width;
    Py_ssize_t precision;
    STRINGLIB_CHAR type;
} InternalFormatSpec;

/*
  ptr points to the start of the format_spec, end points just past its end.
  fills in format with the parsed information.
  returns 1 on success, 0 on failure.
  if failure, sets the exception
*/
static int
parse_internal_render_format_spec(PyObject *format_spec,
                                  InternalFormatSpec *format,
                                  char default_type)
{
    STRINGLIB_CHAR *ptr = STRINGLIB_STR(format_spec);
    STRINGLIB_CHAR *end = ptr + STRINGLIB_LEN(format_spec);

    /* end-ptr is used throughout this code to specify the length of
       the input string */

    Py_ssize_t specified_width;

    format->fill_char = '\0';
    format->align = '\0';
    format->sign = '\0';
    format->width = -1;
    format->precision = -1;
    format->type = default_type;

    /* If the second char is an alignment token,
       then parse the fill char */
    if (end-ptr >= 2 && is_alignment_token(ptr[1])) {
        format->align = ptr[1];
        format->fill_char = ptr[0];
        ptr += 2;
    }
    else if (end-ptr >= 1 && is_alignment_token(ptr[0])) {
        format->align = ptr[0];
        ptr++;
    }

    /* Parse the various sign options */
    if (end-ptr >= 1 && is_sign_element(ptr[0])) {
        format->sign = ptr[0];
        ptr++;
#if ALLOW_PARENS_FOR_SIGN
        if (end-ptr >= 1 && ptr[0] == ')') {
            ptr++;
        }
#endif
    }

    /* The special case for 0-padding (backwards compat) */
    if (format->fill_char == '\0' && end-ptr >= 1 && ptr[0] == '0') {
        format->fill_char = '0';
        if (format->align == '\0') {
            format->align = '=';
        }
        ptr++;
    }

    /* XXX add error checking */
    specified_width = get_integer(&ptr, end, &format->width);

    /* if specified_width is 0, we didn't consume any characters for
       the width. in that case, reset the width to -1, because
       get_integer() will have set it to zero */
    if (specified_width == 0) {
        format->width = -1;
    }

    /* Parse field precision */
    if (end-ptr && ptr[0] == '.') {
        ptr++;

        /* XXX add error checking */
        specified_width = get_integer(&ptr, end, &format->precision);

        /* not having a precision after a dot is an error */
        if (specified_width == 0) {
            PyErr_Format(PyExc_ValueError,
                         "Format specifier missing precision");
            return 0;
        }

    }

    /* Finally, parse the type field */

    if (end-ptr > 1) {
        /* invalid conversion spec */
        PyErr_Format(PyExc_ValueError, "Invalid conversion specification");
        return 0;
    }

    if (end-ptr == 1) {
        format->type = ptr[0];
        ptr++;
    }

    return 1;
}


/************************************************************************/
/*********** common routines for numeric formatting *********************/
/************************************************************************/

/* describes the layout for an integer, see the comment in
   _calc_integer_widths() for details */
typedef struct {
    Py_ssize_t n_lpadding;
    Py_ssize_t n_spadding;
    Py_ssize_t n_rpadding;
    char lsign;
    Py_ssize_t n_lsign;
    char rsign;
    Py_ssize_t n_rsign;
    Py_ssize_t n_total; /* just a convenience, it's derivable from the
                           other fields */
} NumberFieldWidths;

/* not all fields of format are used.  for example, precision is
   unused.  should this take discrete params in order to be more clear
   about what it does?  or is passing a single format parameter easier
   and more efficient enough to justify a little obfuscation? */
static void
calc_number_widths(NumberFieldWidths *r, STRINGLIB_CHAR actual_sign,
                   Py_ssize_t n_digits, const InternalFormatSpec *format)
{
    r->n_lpadding = 0;
    r->n_spadding = 0;
    r->n_rpadding = 0;
    r->lsign = '\0';
    r->n_lsign = 0;
    r->rsign = '\0';
    r->n_rsign = 0;

    /* the output will look like:
       |                                                           |
       | <lpadding> <lsign> <spadding> <digits> <rsign> <rpadding> |
       |                                                           |

       lsign and rsign are computed from format->sign and the actual
       sign of the number

       digits is already known

       the total width is either given, or computed from the
       actual digits

       only one of lpadding, spadding, and rpadding can be non-zero,
       and it's calculated from the width and other fields
    */

    /* compute the various parts we're going to write */
    if (format->sign == '+') {
        /* always put a + or - */
        r->n_lsign = 1;
        r->lsign = (actual_sign == '-' ? '-' : '+');
    }
#if ALLOW_PARENS_FOR_SIGN
    else if (format->sign == '(') {
        if (actual_sign == '-') {
            r->n_lsign = 1;
            r->lsign = '(';
            r->n_rsign = 1;
            r->rsign = ')';
        }
    }
#endif
    else if (format->sign == ' ') {
        r->n_lsign = 1;
        r->lsign = (actual_sign == '-' ? '-' : ' ');
    }
    else {
        /* non specified, or the default (-) */
        if (actual_sign == '-') {
            r->n_lsign = 1;
            r->lsign = '-';
        }
    }

    /* now the number of padding characters */
    if (format->width == -1) {
        /* no padding at all, nothing to do */
    }
    else {
        /* see if any padding is needed */
        if (r->n_lsign + n_digits + r->n_rsign >= format->width) {
            /* no padding needed, we're already bigger than the
               requested width */
        }
        else {
            /* determine which of left, space, or right padding is
               needed */
            Py_ssize_t padding = format->width - (r->n_lsign + n_digits + r->n_rsign);
            if (format->align == '<')
                r->n_rpadding = padding;
            else if (format->align == '>')
                r->n_lpadding = padding;
            else if (format->align == '^') {
                r->n_lpadding = padding / 2;
                r->n_rpadding = padding - r->n_lpadding;
            }
            else if (format->align == '=')
                r->n_spadding = padding;
            else
                r->n_lpadding = padding;
        }
    }
    r->n_total = r->n_lpadding + r->n_lsign + r->n_spadding +
        n_digits + r->n_rsign + r->n_rpadding;
}

/* fill in the non-digit parts of a numbers's string representation,
   as determined in _calc_integer_widths().  returns the pointer to
   where the digits go. */
static STRINGLIB_CHAR *
fill_number(STRINGLIB_CHAR *p_buf, const NumberFieldWidths *spec,
            Py_ssize_t n_digits, STRINGLIB_CHAR fill_char)
{
    STRINGLIB_CHAR* p_digits;

    if (spec->n_lpadding) {
        STRINGLIB_FILL(p_buf, fill_char, spec->n_lpadding);
        p_buf += spec->n_lpadding;
    }
    if (spec->n_lsign == 1) {
        *p_buf++ = spec->lsign;
    }
    if (spec->n_spadding) {
        STRINGLIB_FILL(p_buf, fill_char, spec->n_spadding);
        p_buf += spec->n_spadding;
    }
    p_digits = p_buf;
    p_buf += n_digits;
    if (spec->n_rsign == 1) {
        *p_buf++ = spec->rsign;
    }
    if (spec->n_rpadding) {
        STRINGLIB_FILL(p_buf, fill_char, spec->n_rpadding);
        p_buf += spec->n_rpadding;
    }
    return p_digits;
}

/************************************************************************/
/*********** string formatting ******************************************/
/************************************************************************/

static PyObject *
format_string_internal(PyObject *value, const InternalFormatSpec *format)
{
    Py_ssize_t width; /* total field width */
    Py_ssize_t lpad;
    STRINGLIB_CHAR *dst;
    STRINGLIB_CHAR *src = STRINGLIB_STR(value);
    Py_ssize_t len = STRINGLIB_LEN(value);
    PyObject *result = NULL;

    /* sign is not allowed on strings */
    if (format->sign != '\0') {
        PyErr_SetString(PyExc_ValueError,
                        "Sign not allowed in string format specifier");
        goto done;
    }

    /* '=' alignment not allowed on strings */
    if (format->align == '=') {
        PyErr_SetString(PyExc_ValueError,
                        "'=' alignment not allowed "
                        "in string format specifier");
        goto done;
    }

    /* if precision is specified, output no more that format.precision
       characters */
    if (format->precision >= 0 && len >= format->precision) {
        len = format->precision;
    }

    if (format->width >= 0) {
        width = format->width;

        /* but use at least len characters */
        if (len > width) {
            width = len;
        }
    }
    else {
        /* not specified, use all of the chars and no more */
        width = len;
    }

    /* allocate the resulting string */
    result = STRINGLIB_NEW(NULL, width);
    if (result == NULL)
        goto done;

    /* now write into that space */
    dst = STRINGLIB_STR(result);

    /* figure out how much leading space we need, based on the
       aligning */
    if (format->align == '>')
        lpad = width - len;
    else if (format->align == '^')
        lpad = (width - len) / 2;
    else
        lpad = 0;

    /* if right aligning, increment the destination allow space on the
       left */
    memcpy(dst + lpad, src, len * sizeof(STRINGLIB_CHAR));

    /* do any padding */
    if (width > len) {
        STRINGLIB_CHAR fill_char = format->fill_char;
        if (fill_char == '\0') {
            /* use the default, if not specified */
            fill_char = ' ';
        }

        /* pad on left */
        if (lpad)
            STRINGLIB_FILL(dst, fill_char, lpad);

        /* pad on right */
        if (width - len - lpad)
            STRINGLIB_FILL(dst + len + lpad, fill_char, width - len - lpad);
    }

done:
    return result;
}


/************************************************************************/
/*********** long formatting ********************************************/
/************************************************************************/

static PyObject *
format_long_internal(PyObject *value, const InternalFormatSpec *format)
{
    PyObject *result = NULL;
    int total_leading_chars_to_skip = 0; /* also includes sign, if
                                            present */
    STRINGLIB_CHAR sign = '\0';
    STRINGLIB_CHAR *p;
    Py_ssize_t n_digits;       /* count of digits need from the computed
                                  string */
    Py_ssize_t len;
    Py_ssize_t tmp;
    NumberFieldWidths spec;
    long x;

    /* no precision allowed on integers */
    if (format->precision != -1) {
        PyErr_SetString(PyExc_ValueError,
                        "Precision not allowed in integer format specifier");
        goto done;
    }


    /* special case for character formatting */
    if (format->type == 'c') {
        /* error to specify a sign */
        if (format->sign != '\0') {
            PyErr_SetString(PyExc_ValueError,
                            "Sign not allowed with integer"
                            " format specifier 'c'");
            goto done;
        }

        /* taken from unicodeobject.c formatchar() */
        /* Integer input truncated to a character */
        x = PyLong_AsLong(value);
        if (x == -1 && PyErr_Occurred())
            goto done;
#ifdef Py_UNICODE_WIDE
        if (x < 0 || x > 0x10ffff) {
            PyErr_SetString(PyExc_OverflowError,
                            "%c arg not in range(0x110000) "
                            "(wide Python build)");
            goto done;
        }
#else
        if (x < 0 || x > 0xffff) {
            PyErr_SetString(PyExc_OverflowError,
                            "%c arg not in range(0x10000) "
                            "(narrow Python build)");
            goto done;
        }
#endif
        result = STRINGLIB_NEW(NULL, 1);
        if (result == NULL)
            goto done;
        p = STRINGLIB_STR(result);
        p[0] = (Py_UNICODE) x;
        n_digits = len = 1;
    }
    else {
        int base;
        int format_leading_chars_to_skip;  /* characters added by
                                              PyNumber_ToBase that we
                                              want to skip over.
                                              instead of using them,
                                              we'll compute our
                                              own. */
        /* compute the base and how many characters will be added by
           PyNumber_ToBase */
        switch (format->type) {
        case 'b':
            base = 2;
            format_leading_chars_to_skip = 2; /* 0b */
            break;
        case 'o':
            base = 8;
            format_leading_chars_to_skip = 2; /* 0o */
            break;
        case 'x':
        case 'X':
            base = 16;
            format_leading_chars_to_skip = 2; /* 0x */
            break;
        default:  /* shouldn't be needed, but stops a compiler warning */
        case 'd':
            base = 10;
            format_leading_chars_to_skip = 0;
            break;
        }

        /* do the hard part, converting to a string in a given base */
        result = PyNumber_ToBase(value, base);
        if (result == NULL)
            goto done;

        n_digits = STRINGLIB_LEN(result);
        len = n_digits;
        p = STRINGLIB_STR(result);

        /* if X, convert to uppercase */
        if (format->type == 'X')
            for (tmp = 0; tmp < len; tmp++)
                p[tmp] = STRINGLIB_TOUPPER(p[tmp]);

        /* is a sign character present in the output?  if so, remember it
           and skip it */
        sign = p[0];
        if (sign == '-') {
            total_leading_chars_to_skip += 1;
            n_digits--;
        }

        /* skip over the leading digits (0x, 0b, etc.) */
        assert(n_digits >= format_leading_chars_to_skip + 1);
        n_digits -= format_leading_chars_to_skip;
        total_leading_chars_to_skip += format_leading_chars_to_skip;
    }

    calc_number_widths(&spec, sign, n_digits, format);

    /* if the buffer is getting bigger, realloc it.  if it's getting
       smaller, don't realloc because we need to move the results
       around first.  realloc after we've done that */

    if (spec.n_total > len) {
        if (STRINGLIB_RESIZE(&result, spec.n_total) < 0)
            goto done;
        /* recalc, because string might have moved */
        p = STRINGLIB_STR(result);
    }

    /* copy the characters into position first, since we're going to
       overwrite some of that space */
    /* we need to move if the number of left padding in the output is
       different from the number of characters we need to skip */
    if ((spec.n_lpadding + spec.n_lsign + spec.n_spadding) !=
          total_leading_chars_to_skip) {
        memmove(p + (spec.n_lpadding + spec.n_lsign + spec.n_spadding),
                p + total_leading_chars_to_skip,
                n_digits * sizeof(STRINGLIB_CHAR));
    }

    /* now fill in the non-digit parts */
    fill_number(p, &spec, n_digits,
                format->fill_char == '\0' ? ' ' : format->fill_char);

    /* if we're getting smaller, realloc now */
    if (spec.n_total < len) {
        if (STRINGLIB_RESIZE(&result, spec.n_total) < 0)
            goto done;
    }

done:
    return result;
}


/************************************************************************/
/*********** float formatting *******************************************/
/************************************************************************/

/* taken from unicodeobject.c */
static Py_ssize_t
strtounicode(Py_UNICODE *buffer, const char *charbuffer)
{
    register Py_ssize_t i;
    Py_ssize_t len = strlen(charbuffer);
    for (i = len - 1; i >= 0; i--)
        buffer[i] = (Py_UNICODE) charbuffer[i];

    return len;
}

/* the callback function to call to do the actual float formatting.
   it matches the definition of PyOS_ascii_formatd */
typedef char*
(*DoubleSnprintfFunction)(char *buffer, size_t buf_len,
                          const char *format, double d);

/* just a wrapper to make PyOS_snprintf look like DoubleSnprintfFunction */
static char*
snprintf_double(char *buffer, size_t buf_len, const char *format, double d)
{
    PyOS_snprintf(buffer, buf_len, format, d);
    return NULL;
}

/* see FORMATBUFLEN in unicodeobject.c */
#define FLOAT_FORMATBUFLEN 120

/* much of this is taken from unicodeobject.c */
/* use type instead of format->type, so that it can be overridden by
   format_number() */
static PyObject *
_format_float(STRINGLIB_CHAR type, PyObject *value,
              const InternalFormatSpec *format,
              DoubleSnprintfFunction snprintf)
{
    /* fmt = '%.' + `prec` + `type` + '%%'
       worst case length = 2 + 10 (len of INT_MAX) + 1 + 2 = 15 (use 20)*/
    char fmt[20];

    /* taken from unicodeobject.c */
    /* Worst case length calc to ensure no buffer overrun:

       'g' formats:
         fmt = %#.<prec>g
         buf = '-' + [0-9]*prec + '.' + 'e+' + (longest exp
            for any double rep.)
         len = 1 + prec + 1 + 2 + 5 = 9 + prec

       'f' formats:
         buf = '-' + [0-9]*x + '.' + [0-9]*prec (with x < 50)
         len = 1 + 50 + 1 + prec = 52 + prec

       If prec=0 the effective precision is 1 (the leading digit is
       always given), therefore increase the length by one.

    */
    char charbuf[FLOAT_FORMATBUFLEN];
    Py_ssize_t n_digits;
    double x;
    Py_ssize_t precision = format->precision;
    PyObject *result = NULL;
    STRINGLIB_CHAR sign;
    char* trailing = "";
    STRINGLIB_CHAR *p;
    NumberFieldWidths spec;

#if STRINGLIB_IS_UNICODE
    Py_UNICODE unicodebuf[FLOAT_FORMATBUFLEN];
#endif

    /* first, do the conversion as 8-bit chars, using the platform's
       snprintf.  then, if needed, convert to unicode. */

    /* 'F' is the same as 'f', per the PEP */
    if (type == 'F')
        type = 'f';

    x = PyFloat_AsDouble(value);

    if (x == -1.0 && PyErr_Occurred())
        goto done;

    if (type == '%') {
        type = 'f';
        x *= 100;
        trailing = "%";
    }

    if (precision < 0)
        precision = 6;
    if (type == 'f' && (fabs(x) / 1e25) >= 1e25)
        type = 'g';

    /* cast "type", because if we're in unicode we need to pass a
       8-bit char.  this is safe, because we've restricted what "type"
       can be */
    PyOS_snprintf(fmt, sizeof(fmt), "%%.%" PY_FORMAT_SIZE_T "d%c", precision, (char)type);

    /* call the passed in function to do the actual formatting */
    snprintf(charbuf, sizeof(charbuf), fmt, x);

    /* adding trailing to fmt with PyOS_snprintf doesn't work, not
       sure why.  we'll just concatentate it here, no harm done.  we
       know we can't have a buffer overflow from the fmt size
       analysis */
    strcat(charbuf, trailing);

    /* rather than duplicate the code for snprintf for both unicode
       and 8 bit strings, we just use the 8 bit version and then
       convert to unicode in a separate code path.  that's probably
       the lesser of 2 evils. */
#if STRINGLIB_IS_UNICODE
    n_digits = strtounicode(unicodebuf, charbuf);
    p = unicodebuf;
#else
    /* compute the length.  I believe this is done because the return
       value from snprintf above is unreliable */
    n_digits = strlen(charbuf);
    p = charbuf;
#endif

    /* is a sign character present in the output?  if so, remember it
       and skip it */
    sign = p[0];
    if (sign == '-') {
        p++;
        n_digits--;
    }

    calc_number_widths(&spec, sign, n_digits, format);

    /* allocate a string with enough space */
    result = STRINGLIB_NEW(NULL, spec.n_total);
    if (result == NULL)
        goto done;

    /* fill in the non-digit parts */
    fill_number(STRINGLIB_STR(result), &spec, n_digits,
                format->fill_char == '\0' ? ' ' : format->fill_char);

    /* fill in the digit parts */
    memmove(STRINGLIB_STR(result) + (spec.n_lpadding + spec.n_lsign + spec.n_spadding),
            p,
            n_digits * sizeof(STRINGLIB_CHAR));

done:
    return result;
}

static PyObject *
format_float_internal(PyObject *value, const InternalFormatSpec *format)
{
    if (format->type == 'n')
        return _format_float('f', value, format, snprintf_double);
    else
        return _format_float(format->type, value, format, PyOS_ascii_formatd);
}

/************************************************************************/
/*********** built in formatters ****************************************/
/************************************************************************/

PyObject *
FORMAT_STRING(PyObject* value, PyObject* args)
{
    PyObject *format_spec;
    PyObject *tmp = NULL;
    PyObject *result = NULL;
    InternalFormatSpec format;

    if (!PyArg_ParseTuple(args, STRINGLIB_PARSE_CODE ":__format__", &format_spec))
        goto done;

    /* check for the special case of zero length format spec, make
       it equivalent to str(value) */
    if (STRINGLIB_LEN(format_spec) == 0) {
        result = STRINGLIB_TOSTR(value);
        goto done;
    }

    /* parse the format_spec */
    if (!parse_internal_render_format_spec(format_spec, &format, 's'))
        goto done;

    /* type conversion? */
    switch (format.type) {
    case 's':
        /* no type conversion needed, already a string.  do the formatting */
        result = format_string_internal(value, &format);
        break;
#if 0
    case 'b':
    case 'c':
    case 'd':
    case 'o':
    case 'x':
    case 'X':
        /* convert to integer */
        /* XXX: make a stringlib function to do this when backporting,
           since FromUnicode differs from FromString */
        tmp = PyLong_FromUnicode(STRINGLIB_STR(value), STRINGLIB_LEN(value), 0);
        if (tmp == NULL)
            goto done;
        result = format_long_internal(tmp, &format);
        break;

    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'n':
    case '%':
        /* convert to float */
        tmp = PyFloat_FromString(value);
        if (tmp == NULL)
            goto done;
        result = format_float_internal(tmp, &format);
        break;
#endif
    default:
        /* unknown */
        PyErr_Format(PyExc_ValueError, "Unknown conversion type %c",
                     format.type);
        goto done;
    }

done:
    Py_XDECREF(tmp);
    return result;
}

PyObject *
FORMAT_LONG(PyObject* value, PyObject* args)
{
    PyObject *format_spec;
    PyObject *result = NULL;
    PyObject *tmp = NULL;
    InternalFormatSpec format;

    if (!PyArg_ParseTuple(args, STRINGLIB_PARSE_CODE ":__format__", &format_spec))
        goto done;

    /* check for the special case of zero length format spec, make
       it equivalent to str(value) */
    if (STRINGLIB_LEN(format_spec) == 0) {
        result = STRINGLIB_TOSTR(value);
        goto done;
    }

    /* parse the format_spec */
    if (!parse_internal_render_format_spec(format_spec, &format, 'd'))
        goto done;

    /* type conversion? */
    switch (format.type) {
#if 0
    case 's':
        /* convert to string/unicode */
        tmp = STRINGLIB_TOSTR(value);
        if (tmp == NULL)
            goto done;
        result = format_string_internal(tmp, &format);
        break;
#endif
    case 'b':
    case 'c':
    case 'd':
    case 'o':
    case 'x':
    case 'X':
        /* no type conversion needed, already an int.  do the formatting */
        result = format_long_internal(value, &format);
        break;

    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'n':
    case '%':
        /* convert to float */
        tmp = PyNumber_Float(value);
        if (tmp == NULL)
            goto done;
        result = format_float_internal(value, &format);
        break;

    default:
        /* unknown */
        PyErr_Format(PyExc_ValueError, "Unknown conversion type %c",
                     format.type);
        goto done;
    }

done:
    Py_XDECREF(tmp);
    return result;
}

PyObject *
FORMAT_FLOAT(PyObject *value, PyObject *args)
{
    PyObject *format_spec;
    PyObject *result = NULL;
    PyObject *tmp = NULL;
    InternalFormatSpec format;

    if (!PyArg_ParseTuple(args, STRINGLIB_PARSE_CODE ":__format__", &format_spec))
        goto done;

    /* check for the special case of zero length format spec, make
       it equivalent to str(value) */
    if (STRINGLIB_LEN(format_spec) == 0) {
        result = STRINGLIB_TOSTR(value);
        goto done;
    }

    /* parse the format_spec */
    if (!parse_internal_render_format_spec(format_spec, &format, 'g'))
        goto done;

    /* type conversion? */
    switch (format.type) {
#if 0
    case 's':
        /* convert to string/unicode */
        tmp = STRINGLIB_TOSTR(value);
        if (tmp == NULL)
            goto done;
        result = format_string_internal(tmp, &format);
        break;
#endif
    case 'b':
    case 'c':
    case 'd':
    case 'o':
    case 'x':
    case 'X':
        /* convert to integer */
        tmp = PyNumber_Long(value);
        if (tmp == NULL)
            goto done;
        result = format_long_internal(tmp, &format);
        break;

    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'n':
    case '%':
        /* no conversion, already a float.  do the formatting */
        result = format_float_internal(value, &format);
        break;

    default:
        /* unknown */
        PyErr_Format(PyExc_ValueError, "Unknown conversion type %c",
                     format.type);
        goto done;
    }

done:
    Py_XDECREF(tmp);
    return result;
}
