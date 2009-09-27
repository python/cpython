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

/* Raises an exception about an unknown presentation type for this
 * type. */

static void
unknown_presentation_type(STRINGLIB_CHAR presentation_type,
                          const char* type_name)
{
#if STRINGLIB_IS_UNICODE
    /* If STRINGLIB_CHAR is Py_UNICODE, %c might be out-of-range,
       hence the two cases. If it is char, gcc complains that the
       condition below is always true, hence the ifdef. */
    if (presentation_type > 32 && presentation_type < 128)
#endif
        PyErr_Format(PyExc_ValueError,
                     "Unknown format code '%c' "
                     "for object of type '%.200s'",
                     (char)presentation_type,
                     type_name);
#if STRINGLIB_IS_UNICODE
    else
        PyErr_Format(PyExc_ValueError,
                     "Unknown format code '\\x%x' "
                     "for object of type '%.200s'",
                     (unsigned int)presentation_type,
                     type_name);
#endif
}

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
    int alternate;
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
parse_internal_render_format_spec(STRINGLIB_CHAR *format_spec,
				  Py_ssize_t format_spec_len,
                                  InternalFormatSpec *format,
                                  char default_type)
{
    STRINGLIB_CHAR *ptr = format_spec;
    STRINGLIB_CHAR *end = format_spec + format_spec_len;

    /* end-ptr is used throughout this code to specify the length of
       the input string */

    Py_ssize_t specified_width;

    format->fill_char = '\0';
    format->align = '\0';
    format->alternate = 0;
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
        ++ptr;
    }

    /* Parse the various sign options */
    if (end-ptr >= 1 && is_sign_element(ptr[0])) {
        format->sign = ptr[0];
        ++ptr;
#if ALLOW_PARENS_FOR_SIGN
        if (end-ptr >= 1 && ptr[0] == ')') {
            ++ptr;
        }
#endif
    }

    /* If the next character is #, we're in alternate mode.  This only
       applies to integers. */
    if (end-ptr >= 1 && ptr[0] == '#') {
	format->alternate = 1;
	++ptr;
    }

    /* The special case for 0-padding (backwards compat) */
    if (format->fill_char == '\0' && end-ptr >= 1 && ptr[0] == '0') {
        format->fill_char = '0';
        if (format->align == '\0') {
            format->align = '=';
        }
        ++ptr;
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
        ++ptr;

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
        ++ptr;
    }

    return 1;
}

#if defined FORMAT_FLOAT || defined FORMAT_LONG
/************************************************************************/
/*********** common routines for numeric formatting *********************/
/************************************************************************/

/* describes the layout for an integer, see the comment in
   calc_number_widths() for details */
typedef struct {
    Py_ssize_t n_lpadding;
    Py_ssize_t n_prefix;
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
calc_number_widths(NumberFieldWidths *spec, STRINGLIB_CHAR actual_sign,
		   Py_ssize_t n_prefix, Py_ssize_t n_digits,
		   const InternalFormatSpec *format)
{
    spec->n_lpadding = 0;
    spec->n_prefix = 0;
    spec->n_spadding = 0;
    spec->n_rpadding = 0;
    spec->lsign = '\0';
    spec->n_lsign = 0;
    spec->rsign = '\0';
    spec->n_rsign = 0;

    /* the output will look like:
       |                                                                    |
       | <lpadding> <lsign> <prefix> <spadding> <digits> <rsign> <rpadding> |
       |                                                                    |

       lsign and rsign are computed from format->sign and the actual
       sign of the number

       prefix is given (it's for the '0x' prefix)

       digits is already known

       the total width is either given, or computed from the
       actual digits

       only one of lpadding, spadding, and rpadding can be non-zero,
       and it's calculated from the width and other fields
    */

    /* compute the various parts we're going to write */
    if (format->sign == '+') {
        /* always put a + or - */
        spec->n_lsign = 1;
        spec->lsign = (actual_sign == '-' ? '-' : '+');
    }
#if ALLOW_PARENS_FOR_SIGN
    else if (format->sign == '(') {
        if (actual_sign == '-') {
            spec->n_lsign = 1;
            spec->lsign = '(';
            spec->n_rsign = 1;
            spec->rsign = ')';
        }
    }
#endif
    else if (format->sign == ' ') {
        spec->n_lsign = 1;
        spec->lsign = (actual_sign == '-' ? '-' : ' ');
    }
    else {
        /* non specified, or the default (-) */
        if (actual_sign == '-') {
            spec->n_lsign = 1;
            spec->lsign = '-';
        }
    }

    spec->n_prefix = n_prefix;

    /* now the number of padding characters */
    if (format->width == -1) {
        /* no padding at all, nothing to do */
    }
    else {
        /* see if any padding is needed */
        if (spec->n_lsign + n_digits + spec->n_rsign +
	        spec->n_prefix >= format->width) {
            /* no padding needed, we're already bigger than the
               requested width */
        }
        else {
            /* determine which of left, space, or right padding is
               needed */
            Py_ssize_t padding = format->width -
		                    (spec->n_lsign + spec->n_prefix +
				     n_digits + spec->n_rsign);
            if (format->align == '<')
                spec->n_rpadding = padding;
            else if (format->align == '>')
                spec->n_lpadding = padding;
            else if (format->align == '^') {
                spec->n_lpadding = padding / 2;
                spec->n_rpadding = padding - spec->n_lpadding;
            }
            else if (format->align == '=')
                spec->n_spadding = padding;
            else
                spec->n_lpadding = padding;
        }
    }
    spec->n_total = spec->n_lpadding + spec->n_lsign + spec->n_prefix +
	    spec->n_spadding + n_digits + spec->n_rsign + spec->n_rpadding;
}

/* fill in the non-digit parts of a numbers's string representation,
   as determined in calc_number_widths().  returns the pointer to
   where the digits go. */
static STRINGLIB_CHAR *
fill_non_digits(STRINGLIB_CHAR *p_buf, const NumberFieldWidths *spec,
		STRINGLIB_CHAR *prefix, Py_ssize_t n_digits,
		STRINGLIB_CHAR fill_char)
{
    STRINGLIB_CHAR *p_digits;

    if (spec->n_lpadding) {
        STRINGLIB_FILL(p_buf, fill_char, spec->n_lpadding);
        p_buf += spec->n_lpadding;
    }
    if (spec->n_lsign == 1) {
        *p_buf++ = spec->lsign;
    }
    if (spec->n_prefix) {
	memmove(p_buf,
		prefix,
		spec->n_prefix * sizeof(STRINGLIB_CHAR));
	p_buf += spec->n_prefix;
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
#endif /* FORMAT_FLOAT || FORMAT_LONG */

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

    /* alternate is not allowed on strings */
    if (format->alternate) {
        PyErr_SetString(PyExc_ValueError,
                        "Alternate form (#) not allowed in string format "
			"specifier");
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

#if defined FORMAT_LONG || defined FORMAT_INT
typedef PyObject*
(*IntOrLongToString)(PyObject *value, int base);

static PyObject *
format_int_or_long_internal(PyObject *value, const InternalFormatSpec *format,
			    IntOrLongToString tostring)
{
    PyObject *result = NULL;
    PyObject *tmp = NULL;
    STRINGLIB_CHAR *pnumeric_chars;
    STRINGLIB_CHAR numeric_char;
    STRINGLIB_CHAR sign = '\0';
    STRINGLIB_CHAR *p;
    Py_ssize_t n_digits;       /* count of digits need from the computed
                                  string */
    Py_ssize_t n_leading_chars;
    Py_ssize_t n_grouping_chars = 0; /* Count of additional chars to
					allocate, used for 'n'
					formatting. */
    Py_ssize_t n_prefix = 0;   /* Count of prefix chars, (e.g., '0x') */
    STRINGLIB_CHAR *prefix = NULL;
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
/* XXX: won't work for int */
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
	numeric_char = (STRINGLIB_CHAR)x;
	pnumeric_chars = &numeric_char;
        n_digits = 1;
    }
    else {
        int base;
	int leading_chars_to_skip = 0;  /* Number of characters added by
				           PyNumber_ToBase that we want to
				           skip over. */

        /* Compute the base and how many characters will be added by
           PyNumber_ToBase */
        switch (format->type) {
        case 'b':
            base = 2;
	    leading_chars_to_skip = 2; /* 0b */
            break;
        case 'o':
            base = 8;
	    leading_chars_to_skip = 2; /* 0o */
            break;
        case 'x':
        case 'X':
            base = 16;
	    leading_chars_to_skip = 2; /* 0x */
            break;
        default:  /* shouldn't be needed, but stops a compiler warning */
        case 'd':
        case 'n':
            base = 10;
            break;
        }

	/* The number of prefix chars is the same as the leading
	   chars to skip */
	if (format->alternate)
	    n_prefix = leading_chars_to_skip;

        /* Do the hard part, converting to a string in a given base */
	tmp = tostring(value, base);
        if (tmp == NULL)
            goto done;

	pnumeric_chars = STRINGLIB_STR(tmp);
        n_digits = STRINGLIB_LEN(tmp);

	prefix = pnumeric_chars;

	/* Remember not to modify what pnumeric_chars points to.  it
	   might be interned.  Only modify it after we copy it into a
	   newly allocated output buffer. */

        /* Is a sign character present in the output?  If so, remember it
           and skip it */
        sign = pnumeric_chars[0];
        if (sign == '-') {
	    ++prefix;
	    ++leading_chars_to_skip;
        }

	/* Skip over the leading chars (0x, 0b, etc.) */
	n_digits -= leading_chars_to_skip;
	pnumeric_chars += leading_chars_to_skip;
    }

    if (format->type == 'n')
	    /* Compute how many additional chars we need to allocate
	       to hold the thousands grouping. */
	    STRINGLIB_GROUPING(NULL, n_digits, n_digits,
			       0, &n_grouping_chars, 0);

    /* Calculate the widths of the various leading and trailing parts */
    calc_number_widths(&spec, sign, n_prefix, n_digits + n_grouping_chars,
		       format);

    /* Allocate a new string to hold the result */
    result = STRINGLIB_NEW(NULL, spec.n_total);
    if (!result)
	goto done;
    p = STRINGLIB_STR(result);

    /* XXX There is too much magic here regarding the internals of
       spec and the location of the prefix and digits.  It would be
       better if calc_number_widths returned a number of logical
       offsets into the buffer, and those were used.  Maybe in a
       future code cleanup. */

    /* Fill in the digit parts */
    n_leading_chars = spec.n_lpadding + spec.n_lsign +
	    spec.n_prefix + spec.n_spadding;
    memmove(p + n_leading_chars,
	    pnumeric_chars,
	    n_digits * sizeof(STRINGLIB_CHAR));

    /* If type is 'X', convert the filled in digits to uppercase */
    if (format->type == 'X') {
	Py_ssize_t t;
	for (t = 0; t < n_digits; ++t)
	    p[t + n_leading_chars] = STRINGLIB_TOUPPER(p[t + n_leading_chars]);
    }

    /* Insert the grouping, if any, after the uppercasing of the digits, so
       we can ensure that grouping chars won't be affected. */
    if (n_grouping_chars) {
	    /* We know this can't fail, since we've already
	       reserved enough space. */
	    STRINGLIB_CHAR *pstart = p + n_leading_chars;
#ifndef NDEBUG
	    int r =
#endif
		STRINGLIB_GROUPING(pstart, n_digits, n_digits,
			   spec.n_total+n_grouping_chars-n_leading_chars,
			   NULL, 0);
	    assert(r);
    }

    /* Fill in the non-digit parts (padding, sign, etc.) */
    fill_non_digits(p, &spec, prefix, n_digits + n_grouping_chars,
		    format->fill_char == '\0' ? ' ' : format->fill_char);

    /* If type is 'X', uppercase the prefix.  This has to be done after the
       prefix is filled in by fill_non_digits */
    if (format->type == 'X') {
	Py_ssize_t t;
	for (t = 0; t < n_prefix; ++t)
	    p[t + spec.n_lpadding + spec.n_lsign] =
		    STRINGLIB_TOUPPER(p[t + spec.n_lpadding + spec.n_lsign]);
    }


done:
    Py_XDECREF(tmp);
    return result;
}
#endif /* defined FORMAT_LONG || defined FORMAT_INT */

/************************************************************************/
/*********** float formatting *******************************************/
/************************************************************************/

#ifdef FORMAT_FLOAT
#if STRINGLIB_IS_UNICODE
/* taken from unicodeobject.c */
static Py_ssize_t
strtounicode(Py_UNICODE *buffer, const char *charbuffer)
{
    register Py_ssize_t i;
    Py_ssize_t len = strlen(charbuffer);
    for (i = len - 1; i >= 0; --i)
        buffer[i] = (Py_UNICODE) charbuffer[i];

    return len;
}
#endif

/* see FORMATBUFLEN in unicodeobject.c */
#define FLOAT_FORMATBUFLEN 120

/* much of this is taken from unicodeobject.c */
static PyObject *
format_float_internal(PyObject *value,
		      const InternalFormatSpec *format)
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
    STRINGLIB_CHAR type = format->type;

#if STRINGLIB_IS_UNICODE
    Py_UNICODE unicodebuf[FLOAT_FORMATBUFLEN];
#endif

    /* alternate is not allowed on floats. */
    if (format->alternate) {
        PyErr_SetString(PyExc_ValueError,
                        "Alternate form (#) not allowed in float format "
			"specifier");
        goto done;
    }

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
    if (type == 'f' && fabs(x) >= 1e50)
        type = 'g';

    /* cast "type", because if we're in unicode we need to pass a
       8-bit char.  this is safe, because we've restricted what "type"
       can be */
    PyOS_snprintf(fmt, sizeof(fmt), "%%.%" PY_FORMAT_SIZE_T "d%c", precision,
		  (char)type);

    /* do the actual formatting */
    PyOS_ascii_formatd(charbuf, sizeof(charbuf), fmt, x);

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
        ++p;
        --n_digits;
    }

    calc_number_widths(&spec, sign, 0, n_digits, format);

    /* allocate a string with enough space */
    result = STRINGLIB_NEW(NULL, spec.n_total);
    if (result == NULL)
        goto done;

    /* Fill in the non-digit parts (padding, sign, etc.) */
    fill_non_digits(STRINGLIB_STR(result), &spec, NULL, n_digits,
		    format->fill_char == '\0' ? ' ' : format->fill_char);

    /* fill in the digit parts */
    memmove(STRINGLIB_STR(result) +
	       (spec.n_lpadding + spec.n_lsign + spec.n_spadding),
            p,
            n_digits * sizeof(STRINGLIB_CHAR));

done:
    return result;
}
#endif /* FORMAT_FLOAT */

/************************************************************************/
/*********** built in formatters ****************************************/
/************************************************************************/
PyObject *
FORMAT_STRING(PyObject *obj,
	      STRINGLIB_CHAR *format_spec,
	      Py_ssize_t format_spec_len)
{
    InternalFormatSpec format;
    PyObject *result = NULL;

    /* check for the special case of zero length format spec, make
       it equivalent to str(obj) */
    if (format_spec_len == 0) {
        result = STRINGLIB_TOSTR(obj);
        goto done;
    }

    /* parse the format_spec */
    if (!parse_internal_render_format_spec(format_spec, format_spec_len,
					   &format, 's'))
        goto done;

    /* type conversion? */
    switch (format.type) {
    case 's':
        /* no type conversion needed, already a string.  do the formatting */
        result = format_string_internal(obj, &format);
        break;
    default:
        /* unknown */
        unknown_presentation_type(format.type, obj->ob_type->tp_name);
        goto done;
    }

done:
    return result;
}

#if defined FORMAT_LONG || defined FORMAT_INT
static PyObject*
format_int_or_long(PyObject* obj,
		   STRINGLIB_CHAR *format_spec,
		   Py_ssize_t format_spec_len,
		   IntOrLongToString tostring)
{
    PyObject *result = NULL;
    PyObject *tmp = NULL;
    InternalFormatSpec format;

    /* check for the special case of zero length format spec, make
       it equivalent to str(obj) */
    if (format_spec_len == 0) {
        result = STRINGLIB_TOSTR(obj);
        goto done;
    }

    /* parse the format_spec */
    if (!parse_internal_render_format_spec(format_spec,
					   format_spec_len,
					   &format, 'd'))
        goto done;

    /* type conversion? */
    switch (format.type) {
    case 'b':
    case 'c':
    case 'd':
    case 'o':
    case 'x':
    case 'X':
    case 'n':
        /* no type conversion needed, already an int (or long).  do
	   the formatting */
	    result = format_int_or_long_internal(obj, &format, tostring);
        break;

    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case '%':
        /* convert to float */
        tmp = PyNumber_Float(obj);
        if (tmp == NULL)
            goto done;
        result = format_float_internal(tmp, &format);
        break;

    default:
        /* unknown */
        unknown_presentation_type(format.type, obj->ob_type->tp_name);
        goto done;
    }

done:
    Py_XDECREF(tmp);
    return result;
}
#endif /* FORMAT_LONG || defined FORMAT_INT */

#ifdef FORMAT_LONG
/* Need to define long_format as a function that will convert a long
   to a string.  In 3.0, _PyLong_Format has the correct signature.  In
   2.x, we need to fudge a few parameters */
#if PY_VERSION_HEX >= 0x03000000
#define long_format _PyLong_Format
#else
static PyObject*
long_format(PyObject* value, int base)
{
    /* Convert to base, don't add trailing 'L', and use the new octal
       format. We already know this is a long object */
    assert(PyLong_Check(value));
    /* convert to base, don't add 'L', and use the new octal format */
    return _PyLong_Format(value, base, 0, 1);
}
#endif

PyObject *
FORMAT_LONG(PyObject *obj,
	    STRINGLIB_CHAR *format_spec,
	    Py_ssize_t format_spec_len)
{
    return format_int_or_long(obj, format_spec, format_spec_len,
			      long_format);
}
#endif /* FORMAT_LONG */

#ifdef FORMAT_INT
/* this is only used for 2.x, not 3.0 */
static PyObject*
int_format(PyObject* value, int base)
{
    /* Convert to base, and use the new octal format. We already
       know this is an int object */
    assert(PyInt_Check(value));
    return _PyInt_Format((PyIntObject*)value, base, 1);
}

PyObject *
FORMAT_INT(PyObject *obj,
	   STRINGLIB_CHAR *format_spec,
	   Py_ssize_t format_spec_len)
{
    return format_int_or_long(obj, format_spec, format_spec_len,
			      int_format);
}
#endif /* FORMAT_INT */

#ifdef FORMAT_FLOAT
PyObject *
FORMAT_FLOAT(PyObject *obj,
	     STRINGLIB_CHAR *format_spec,
	     Py_ssize_t format_spec_len)
{
    PyObject *result = NULL;
    InternalFormatSpec format;

    /* check for the special case of zero length format spec, make
       it equivalent to str(obj) */
    if (format_spec_len == 0) {
        result = STRINGLIB_TOSTR(obj);
        goto done;
    }

    /* parse the format_spec */
    if (!parse_internal_render_format_spec(format_spec,
					   format_spec_len,
					   &format, '\0'))
        goto done;

    /* type conversion? */
    switch (format.type) {
    case '\0':
	/* 'Z' means like 'g', but with at least one decimal.  See
	   PyOS_ascii_formatd */
	format.type = 'Z';
	/* Deliberate fall through to the next case statement */
    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'n':
    case '%':
        /* no conversion, already a float.  do the formatting */
        result = format_float_internal(obj, &format);
        break;

    default:
        /* unknown */
        unknown_presentation_type(format.type, obj->ob_type->tp_name);
        goto done;
    }

done:
    return result;
}
#endif /* FORMAT_FLOAT */
