/* implements the string, long, and float formatters.  that is,
   string.__format__, etc. */

#include <locale.h>

/* Before including this, you must include either:
   stringlib/unicodedefs.h
   stringlib/stringdefs.h

   Also, you should define the names:
   FORMAT_STRING
   FORMAT_LONG
   FORMAT_FLOAT
   FORMAT_COMPLEX
   to be whatever you want the public names of these functions to
   be.  These are the only non-static functions defined here.
*/

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

static void
invalid_comma_type(STRINGLIB_CHAR presentation_type)
{
#if STRINGLIB_IS_UNICODE
    /* See comment in unknown_presentation_type */
    if (presentation_type > 32 && presentation_type < 128)
#endif
        PyErr_Format(PyExc_ValueError,
                     "Cannot specify ',' with '%c'.",
                     (char)presentation_type);
#if STRINGLIB_IS_UNICODE
    else
        PyErr_Format(PyExc_ValueError,
                     "Cannot specify ',' with '\\x%x'.",
                     (unsigned int)presentation_type);
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
    int thousands_separators;
    Py_ssize_t precision;
    STRINGLIB_CHAR type;
} InternalFormatSpec;


#if 0
/* Occassionally useful for debugging. Should normally be commented out. */
static void
DEBUG_PRINT_FORMAT_SPEC(InternalFormatSpec *format)
{
    printf("internal format spec: fill_char %d\n", format->fill_char);
    printf("internal format spec: align %d\n", format->align);
    printf("internal format spec: alternate %d\n", format->alternate);
    printf("internal format spec: sign %d\n", format->sign);
    printf("internal format spec: width %zd\n", format->width);
    printf("internal format spec: thousands_separators %d\n",
           format->thousands_separators);
    printf("internal format spec: precision %zd\n", format->precision);
    printf("internal format spec: type %c\n", format->type);
    printf("\n");
}
#endif


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
                                  char default_type,
                                  char default_align)
{
    STRINGLIB_CHAR *ptr = format_spec;
    STRINGLIB_CHAR *end = format_spec + format_spec_len;

    /* end-ptr is used throughout this code to specify the length of
       the input string */

    Py_ssize_t consumed;
    int align_specified = 0;

    format->fill_char = '\0';
    format->align = default_align;
    format->alternate = 0;
    format->sign = '\0';
    format->width = -1;
    format->thousands_separators = 0;
    format->precision = -1;
    format->type = default_type;

    /* If the second char is an alignment token,
       then parse the fill char */
    if (end-ptr >= 2 && is_alignment_token(ptr[1])) {
        format->align = ptr[1];
        format->fill_char = ptr[0];
        align_specified = 1;
        ptr += 2;
    }
    else if (end-ptr >= 1 && is_alignment_token(ptr[0])) {
        format->align = ptr[0];
        align_specified = 1;
        ++ptr;
    }

    /* Parse the various sign options */
    if (end-ptr >= 1 && is_sign_element(ptr[0])) {
        format->sign = ptr[0];
        ++ptr;
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
        if (!align_specified) {
            format->align = '=';
        }
        ++ptr;
    }

    consumed = get_integer(&ptr, end, &format->width);
    if (consumed == -1)
        /* Overflow error. Exception already set. */
        return 0;

    /* If consumed is 0, we didn't consume any characters for the
       width. In that case, reset the width to -1, because
       get_integer() will have set it to zero. -1 is how we record
       that the width wasn't specified. */
    if (consumed == 0)
        format->width = -1;

    /* Comma signifies add thousands separators */
    if (end-ptr && ptr[0] == ',') {
        format->thousands_separators = 1;
        ++ptr;
    }

    /* Parse field precision */
    if (end-ptr && ptr[0] == '.') {
        ++ptr;

        consumed = get_integer(&ptr, end, &format->precision);
        if (consumed == -1)
            /* Overflow error. Exception already set. */
            return 0;

        /* Not having a precision after a dot is an error. */
        if (consumed == 0) {
            PyErr_Format(PyExc_ValueError,
                         "Format specifier missing precision");
            return 0;
        }

    }

    /* Finally, parse the type field. */

    if (end-ptr > 1) {
        /* More than one char remain, invalid conversion spec. */
        PyErr_Format(PyExc_ValueError, "Invalid conversion specification");
        return 0;
    }

    if (end-ptr == 1) {
        format->type = ptr[0];
        ++ptr;
    }

    /* Do as much validating as we can, just by looking at the format
       specifier.  Do not take into account what type of formatting
       we're doing (int, float, string). */

    if (format->thousands_separators) {
        switch (format->type) {
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'E':
        case 'G':
        case '%':
        case 'F':
        case '\0':
            /* These are allowed. See PEP 378.*/
            break;
        default:
            invalid_comma_type(format->type);
            return 0;
        }
    }

    return 1;
}

/* Calculate the padding needed. */
static void
calc_padding(Py_ssize_t nchars, Py_ssize_t width, STRINGLIB_CHAR align,
             Py_ssize_t *n_lpadding, Py_ssize_t *n_rpadding,
             Py_ssize_t *n_total)
{
    if (width >= 0) {
        if (nchars > width)
            *n_total = nchars;
        else
            *n_total = width;
    }
    else {
        /* not specified, use all of the chars and no more */
        *n_total = nchars;
    }

    /* Figure out how much leading space we need, based on the
       aligning */
    if (align == '>')
        *n_lpadding = *n_total - nchars;
    else if (align == '^')
        *n_lpadding = (*n_total - nchars) / 2;
    else if (align == '<' || align == '=')
        *n_lpadding = 0;
    else {
        /* We should never have an unspecified alignment. */
        *n_lpadding = 0;
        assert(0);
    }

    *n_rpadding = *n_total - nchars - *n_lpadding;
}

/* Do the padding, and return a pointer to where the caller-supplied
   content goes. */
static STRINGLIB_CHAR *
fill_padding(STRINGLIB_CHAR *p, Py_ssize_t nchars, STRINGLIB_CHAR fill_char,
             Py_ssize_t n_lpadding, Py_ssize_t n_rpadding)
{
    /* Pad on left. */
    if (n_lpadding)
        STRINGLIB_FILL(p, fill_char, n_lpadding);

    /* Pad on right. */
    if (n_rpadding)
        STRINGLIB_FILL(p + nchars + n_lpadding, fill_char, n_rpadding);

    /* Pointer to the user content. */
    return p + n_lpadding;
}

#if defined FORMAT_FLOAT || defined FORMAT_LONG || defined FORMAT_COMPLEX
/************************************************************************/
/*********** common routines for numeric formatting *********************/
/************************************************************************/

/* Locale type codes. */
#define LT_CURRENT_LOCALE 0
#define LT_DEFAULT_LOCALE 1
#define LT_NO_LOCALE 2

/* Locale info needed for formatting integers and the part of floats
   before and including the decimal. Note that locales only support
   8-bit chars, not unicode. */
typedef struct {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
} LocaleInfo;

/* describes the layout for an integer, see the comment in
   calc_number_widths() for details */
typedef struct {
    Py_ssize_t n_lpadding;
    Py_ssize_t n_prefix;
    Py_ssize_t n_spadding;
    Py_ssize_t n_rpadding;
    char sign;
    Py_ssize_t n_sign;      /* number of digits needed for sign (0/1) */
    Py_ssize_t n_grouped_digits; /* Space taken up by the digits, including
                                    any grouping chars. */
    Py_ssize_t n_decimal;   /* 0 if only an integer */
    Py_ssize_t n_remainder; /* Digits in decimal and/or exponent part,
                               excluding the decimal itself, if
                               present. */

    /* These 2 are not the widths of fields, but are needed by
       STRINGLIB_GROUPING. */
    Py_ssize_t n_digits;    /* The number of digits before a decimal
                               or exponent. */
    Py_ssize_t n_min_width; /* The min_width we used when we computed
                               the n_grouped_digits width. */
} NumberFieldWidths;


/* Given a number of the form:
   digits[remainder]
   where ptr points to the start and end points to the end, find where
    the integer part ends. This could be a decimal, an exponent, both,
    or neither.
   If a decimal point is present, set *has_decimal and increment
    remainder beyond it.
   Results are undefined (but shouldn't crash) for improperly
    formatted strings.
*/
static void
parse_number(STRINGLIB_CHAR *ptr, Py_ssize_t len,
             Py_ssize_t *n_remainder, int *has_decimal)
{
    STRINGLIB_CHAR *end = ptr + len;
    STRINGLIB_CHAR *remainder;

    while (ptr<end && isdigit(*ptr))
        ++ptr;
    remainder = ptr;

    /* Does remainder start with a decimal point? */
    *has_decimal = ptr<end && *remainder == '.';

    /* Skip the decimal point. */
    if (*has_decimal)
        remainder++;

    *n_remainder = end - remainder;
}

/* not all fields of format are used.  for example, precision is
   unused.  should this take discrete params in order to be more clear
   about what it does?  or is passing a single format parameter easier
   and more efficient enough to justify a little obfuscation? */
static Py_ssize_t
calc_number_widths(NumberFieldWidths *spec, Py_ssize_t n_prefix,
                   STRINGLIB_CHAR sign_char, STRINGLIB_CHAR *number,
                   Py_ssize_t n_number, Py_ssize_t n_remainder,
                   int has_decimal, const LocaleInfo *locale,
                   const InternalFormatSpec *format)
{
    Py_ssize_t n_non_digit_non_padding;
    Py_ssize_t n_padding;

    spec->n_digits = n_number - n_remainder - (has_decimal?1:0);
    spec->n_lpadding = 0;
    spec->n_prefix = n_prefix;
    spec->n_decimal = has_decimal ? strlen(locale->decimal_point) : 0;
    spec->n_remainder = n_remainder;
    spec->n_spadding = 0;
    spec->n_rpadding = 0;
    spec->sign = '\0';
    spec->n_sign = 0;

    /* the output will look like:
       |                                                                                         |
       | <lpadding> <sign> <prefix> <spadding> <grouped_digits> <decimal> <remainder> <rpadding> |
       |                                                                                         |

       sign is computed from format->sign and the actual
       sign of the number

       prefix is given (it's for the '0x' prefix)

       digits is already known

       the total width is either given, or computed from the
       actual digits

       only one of lpadding, spadding, and rpadding can be non-zero,
       and it's calculated from the width and other fields
    */

    /* compute the various parts we're going to write */
    switch (format->sign) {
    case '+':
        /* always put a + or - */
        spec->n_sign = 1;
        spec->sign = (sign_char == '-' ? '-' : '+');
        break;
    case ' ':
        spec->n_sign = 1;
        spec->sign = (sign_char == '-' ? '-' : ' ');
        break;
    default:
        /* Not specified, or the default (-) */
        if (sign_char == '-') {
            spec->n_sign = 1;
            spec->sign = '-';
        }
    }

    /* The number of chars used for non-digits and non-padding. */
    n_non_digit_non_padding = spec->n_sign + spec->n_prefix + spec->n_decimal +
        spec->n_remainder;

    /* min_width can go negative, that's okay. format->width == -1 means
       we don't care. */
    if (format->fill_char == '0' && format->align == '=')
        spec->n_min_width = format->width - n_non_digit_non_padding;
    else
        spec->n_min_width = 0;

    if (spec->n_digits == 0)
        /* This case only occurs when using 'c' formatting, we need
           to special case it because the grouping code always wants
           to have at least one character. */
        spec->n_grouped_digits = 0;
    else
        spec->n_grouped_digits = STRINGLIB_GROUPING(NULL, 0, NULL,
                                                    spec->n_digits,
                                                    spec->n_min_width,
                                                    locale->grouping,
                                                    locale->thousands_sep);

    /* Given the desired width and the total of digit and non-digit
       space we consume, see if we need any padding. format->width can
       be negative (meaning no padding), but this code still works in
       that case. */
    n_padding = format->width -
                        (n_non_digit_non_padding + spec->n_grouped_digits);
    if (n_padding > 0) {
        /* Some padding is needed. Determine if it's left, space, or right. */
        switch (format->align) {
        case '<':
            spec->n_rpadding = n_padding;
            break;
        case '^':
            spec->n_lpadding = n_padding / 2;
            spec->n_rpadding = n_padding - spec->n_lpadding;
            break;
        case '=':
            spec->n_spadding = n_padding;
            break;
        case '>':
            spec->n_lpadding = n_padding;
            break;
        default:
            /* Shouldn't get here, but treat it as '>' */
            spec->n_lpadding = n_padding;
            assert(0);
            break;
        }
    }
    return spec->n_lpadding + spec->n_sign + spec->n_prefix +
        spec->n_spadding + spec->n_grouped_digits + spec->n_decimal +
        spec->n_remainder + spec->n_rpadding;
}

/* Fill in the digit parts of a numbers's string representation,
   as determined in calc_number_widths().
   No error checking, since we know the buffer is the correct size. */
static void
fill_number(STRINGLIB_CHAR *buf, const NumberFieldWidths *spec,
            STRINGLIB_CHAR *digits, Py_ssize_t n_digits,
            STRINGLIB_CHAR *prefix, STRINGLIB_CHAR fill_char,
            LocaleInfo *locale, int toupper)
{
    /* Used to keep track of digits, decimal, and remainder. */
    STRINGLIB_CHAR *p = digits;

#ifndef NDEBUG
    Py_ssize_t r;
#endif

    if (spec->n_lpadding) {
        STRINGLIB_FILL(buf, fill_char, spec->n_lpadding);
        buf += spec->n_lpadding;
    }
    if (spec->n_sign == 1) {
        *buf++ = spec->sign;
    }
    if (spec->n_prefix) {
        memmove(buf,
                prefix,
                spec->n_prefix * sizeof(STRINGLIB_CHAR));
        if (toupper) {
            Py_ssize_t t;
            for (t = 0; t < spec->n_prefix; ++t)
                buf[t] = STRINGLIB_TOUPPER(buf[t]);
        }
        buf += spec->n_prefix;
    }
    if (spec->n_spadding) {
        STRINGLIB_FILL(buf, fill_char, spec->n_spadding);
        buf += spec->n_spadding;
    }

    /* Only for type 'c' special case, it has no digits. */
    if (spec->n_digits != 0) {
        /* Fill the digits with InsertThousandsGrouping. */
#ifndef NDEBUG
        r =
#endif
            STRINGLIB_GROUPING(buf, spec->n_grouped_digits, digits,
                               spec->n_digits, spec->n_min_width,
                               locale->grouping, locale->thousands_sep);
#ifndef NDEBUG
        assert(r == spec->n_grouped_digits);
#endif
        p += spec->n_digits;
    }
    if (toupper) {
        Py_ssize_t t;
        for (t = 0; t < spec->n_grouped_digits; ++t)
            buf[t] = STRINGLIB_TOUPPER(buf[t]);
    }
    buf += spec->n_grouped_digits;

    if (spec->n_decimal) {
        Py_ssize_t t;
        for (t = 0; t < spec->n_decimal; ++t)
            buf[t] = locale->decimal_point[t];
        buf += spec->n_decimal;
        p += 1;
    }

    if (spec->n_remainder) {
        memcpy(buf, p, spec->n_remainder * sizeof(STRINGLIB_CHAR));
        buf += spec->n_remainder;
        p += spec->n_remainder;
    }

    if (spec->n_rpadding) {
        STRINGLIB_FILL(buf, fill_char, spec->n_rpadding);
        buf += spec->n_rpadding;
    }
}

static char no_grouping[1] = {CHAR_MAX};

/* Find the decimal point character(s?), thousands_separator(s?), and
   grouping description, either for the current locale if type is
   LT_CURRENT_LOCALE, a hard-coded locale if LT_DEFAULT_LOCALE, or
   none if LT_NO_LOCALE. */
static void
get_locale_info(int type, LocaleInfo *locale_info)
{
    switch (type) {
    case LT_CURRENT_LOCALE: {
        struct lconv *locale_data = localeconv();
        locale_info->decimal_point = locale_data->decimal_point;
        locale_info->thousands_sep = locale_data->thousands_sep;
        locale_info->grouping = locale_data->grouping;
        break;
    }
    case LT_DEFAULT_LOCALE:
        locale_info->decimal_point = ".";
        locale_info->thousands_sep = ",";
        locale_info->grouping = "\3"; /* Group every 3 characters.  The
                                         (implicit) trailing 0 means repeat
                                         infinitely. */
        break;
    case LT_NO_LOCALE:
        locale_info->decimal_point = ".";
        locale_info->thousands_sep = "";
        locale_info->grouping = no_grouping;
        break;
    default:
        assert(0);
    }
}

#endif /* FORMAT_FLOAT || FORMAT_LONG || FORMAT_COMPLEX */

/************************************************************************/
/*********** string formatting ******************************************/
/************************************************************************/

static PyObject *
format_string_internal(PyObject *value, const InternalFormatSpec *format)
{
    Py_ssize_t lpad;
    Py_ssize_t rpad;
    Py_ssize_t total;
    STRINGLIB_CHAR *p;
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

    calc_padding(len, format->width, format->align, &lpad, &rpad, &total);

    /* allocate the resulting string */
    result = STRINGLIB_NEW(NULL, total);
    if (result == NULL)
        goto done;

    /* Write into that space. First the padding. */
    p = fill_padding(STRINGLIB_STR(result), len,
                     format->fill_char=='\0'?' ':format->fill_char,
                     lpad, rpad);

    /* Then the source string. */
    memcpy(p, STRINGLIB_STR(value), len * sizeof(STRINGLIB_CHAR));

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
    STRINGLIB_CHAR sign_char = '\0';
    Py_ssize_t n_digits;       /* count of digits need from the computed
                                  string */
    Py_ssize_t n_remainder = 0; /* Used only for 'c' formatting, which
                                   produces non-digits */
    Py_ssize_t n_prefix = 0;   /* Count of prefix chars, (e.g., '0x') */
    Py_ssize_t n_total;
    STRINGLIB_CHAR *prefix = NULL;
    NumberFieldWidths spec;
    long x;

    /* Locale settings, either from the actual locale or
       from a hard-code pseudo-locale */
    LocaleInfo locale;

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

        /* As a sort-of hack, we tell calc_number_widths that we only
           have "remainder" characters. calc_number_widths thinks
           these are characters that don't get formatted, only copied
           into the output string. We do this for 'c' formatting,
           because the characters are likely to be non-digits. */
        n_remainder = 1;
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
        if (pnumeric_chars[0] == '-') {
            sign_char = pnumeric_chars[0];
            ++prefix;
            ++leading_chars_to_skip;
        }

        /* Skip over the leading chars (0x, 0b, etc.) */
        n_digits -= leading_chars_to_skip;
        pnumeric_chars += leading_chars_to_skip;
    }

    /* Determine the grouping, separator, and decimal point, if any. */
    get_locale_info(format->type == 'n' ? LT_CURRENT_LOCALE :
                    (format->thousands_separators ?
                     LT_DEFAULT_LOCALE :
                     LT_NO_LOCALE),
                    &locale);

    /* Calculate how much memory we'll need. */
    n_total = calc_number_widths(&spec, n_prefix, sign_char, pnumeric_chars,
                       n_digits, n_remainder, 0, &locale, format);

    /* Allocate the memory. */
    result = STRINGLIB_NEW(NULL, n_total);
    if (!result)
        goto done;

    /* Populate the memory. */
    fill_number(STRINGLIB_STR(result), &spec, pnumeric_chars, n_digits,
                prefix, format->fill_char == '\0' ? ' ' : format->fill_char,
                &locale, format->type == 'X');

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
static void
strtounicode(Py_UNICODE *buffer, const char *charbuffer, Py_ssize_t len)
{
    Py_ssize_t i;
    for (i = 0; i < len; ++i)
        buffer[i] = (Py_UNICODE)charbuffer[i];
}
#endif

/* much of this is taken from unicodeobject.c */
static PyObject *
format_float_internal(PyObject *value,
                      const InternalFormatSpec *format)
{
    char *buf = NULL;       /* buffer returned from PyOS_double_to_string */
    Py_ssize_t n_digits;
    Py_ssize_t n_remainder;
    Py_ssize_t n_total;
    int has_decimal;
    double val;
    Py_ssize_t precision = format->precision;
    Py_ssize_t default_precision = 6;
    STRINGLIB_CHAR type = format->type;
    int add_pct = 0;
    STRINGLIB_CHAR *p;
    NumberFieldWidths spec;
    int flags = 0;
    PyObject *result = NULL;
    STRINGLIB_CHAR sign_char = '\0';
    int float_type; /* Used to see if we have a nan, inf, or regular float. */

#if STRINGLIB_IS_UNICODE
    Py_UNICODE *unicode_tmp = NULL;
#endif

    /* Locale settings, either from the actual locale or
       from a hard-code pseudo-locale */
    LocaleInfo locale;

    if (format->alternate)
        flags |= Py_DTSF_ALT;

    if (type == '\0') {
        /* Omitted type specifier.  Behaves in the same way as repr(x)
           and str(x) if no precision is given, else like 'g', but with
           at least one digit after the decimal point. */
        flags |= Py_DTSF_ADD_DOT_0;
        type = 'r';
        default_precision = 0;
    }

    if (type == 'n')
        /* 'n' is the same as 'g', except for the locale used to
           format the result. We take care of that later. */
        type = 'g';

    val = PyFloat_AsDouble(value);
    if (val == -1.0 && PyErr_Occurred())
        goto done;

    if (type == '%') {
        type = 'f';
        val *= 100;
        add_pct = 1;
    }

    if (precision < 0)
        precision = default_precision;
    else if (type == 'r')
        type = 'g';

    /* Cast "type", because if we're in unicode we need to pass a
       8-bit char. This is safe, because we've restricted what "type"
       can be. */
    buf = PyOS_double_to_string(val, (char)type, precision, flags,
                                &float_type);
    if (buf == NULL)
        goto done;
    n_digits = strlen(buf);

    if (add_pct) {
        /* We know that buf has a trailing zero (since we just called
           strlen() on it), and we don't use that fact any more. So we
           can just write over the trailing zero. */
        buf[n_digits] = '%';
        n_digits += 1;
    }

    /* Since there is no unicode version of PyOS_double_to_string,
       just use the 8 bit version and then convert to unicode. */
#if STRINGLIB_IS_UNICODE
    unicode_tmp = (Py_UNICODE*)PyMem_Malloc((n_digits)*sizeof(Py_UNICODE));
    if (unicode_tmp == NULL) {
        PyErr_NoMemory();
        goto done;
    }
    strtounicode(unicode_tmp, buf, n_digits);
    p = unicode_tmp;
#else
    p = buf;
#endif

    /* Is a sign character present in the output?  If so, remember it
       and skip it */
    if (*p == '-') {
        sign_char = *p;
        ++p;
        --n_digits;
    }

    /* Determine if we have any "remainder" (after the digits, might include
       decimal or exponent or both (or neither)) */
    parse_number(p, n_digits, &n_remainder, &has_decimal);

    /* Determine the grouping, separator, and decimal point, if any. */
    get_locale_info(format->type == 'n' ? LT_CURRENT_LOCALE :
                    (format->thousands_separators ?
                     LT_DEFAULT_LOCALE :
                     LT_NO_LOCALE),
                    &locale);

    /* Calculate how much memory we'll need. */
    n_total = calc_number_widths(&spec, 0, sign_char, p, n_digits,
                                 n_remainder, has_decimal, &locale, format);

    /* Allocate the memory. */
    result = STRINGLIB_NEW(NULL, n_total);
    if (result == NULL)
        goto done;

    /* Populate the memory. */
    fill_number(STRINGLIB_STR(result), &spec, p, n_digits, NULL,
                format->fill_char == '\0' ? ' ' : format->fill_char, &locale,
                0);

done:
    PyMem_Free(buf);
#if STRINGLIB_IS_UNICODE
    PyMem_Free(unicode_tmp);
#endif
    return result;
}
#endif /* FORMAT_FLOAT */

/************************************************************************/
/*********** complex formatting *****************************************/
/************************************************************************/

#ifdef FORMAT_COMPLEX

static PyObject *
format_complex_internal(PyObject *value,
                        const InternalFormatSpec *format)
{
    double re;
    double im;
    char *re_buf = NULL;       /* buffer returned from PyOS_double_to_string */
    char *im_buf = NULL;       /* buffer returned from PyOS_double_to_string */

    InternalFormatSpec tmp_format = *format;
    Py_ssize_t n_re_digits;
    Py_ssize_t n_im_digits;
    Py_ssize_t n_re_remainder;
    Py_ssize_t n_im_remainder;
    Py_ssize_t n_re_total;
    Py_ssize_t n_im_total;
    int re_has_decimal;
    int im_has_decimal;
    Py_ssize_t precision = format->precision;
    Py_ssize_t default_precision = 6;
    STRINGLIB_CHAR type = format->type;
    STRINGLIB_CHAR *p_re;
    STRINGLIB_CHAR *p_im;
    NumberFieldWidths re_spec;
    NumberFieldWidths im_spec;
    int flags = 0;
    PyObject *result = NULL;
    STRINGLIB_CHAR *p;
    STRINGLIB_CHAR re_sign_char = '\0';
    STRINGLIB_CHAR im_sign_char = '\0';
    int re_float_type; /* Used to see if we have a nan, inf, or regular float. */
    int im_float_type;
    int add_parens = 0;
    int skip_re = 0;
    Py_ssize_t lpad;
    Py_ssize_t rpad;
    Py_ssize_t total;

#if STRINGLIB_IS_UNICODE
    Py_UNICODE *re_unicode_tmp = NULL;
    Py_UNICODE *im_unicode_tmp = NULL;
#endif

    /* Locale settings, either from the actual locale or
       from a hard-code pseudo-locale */
    LocaleInfo locale;

    /* Zero padding is not allowed. */
    if (format->fill_char == '0') {
        PyErr_SetString(PyExc_ValueError,
                        "Zero padding is not allowed in complex format "
                        "specifier");
        goto done;
    }

    /* Neither is '=' alignment . */
    if (format->align == '=') {
        PyErr_SetString(PyExc_ValueError,
                        "'=' alignment flag is not allowed in complex format "
                        "specifier");
        goto done;
    }

    re = PyComplex_RealAsDouble(value);
    if (re == -1.0 && PyErr_Occurred())
        goto done;
    im = PyComplex_ImagAsDouble(value);
    if (im == -1.0 && PyErr_Occurred())
        goto done;

    if (format->alternate)
        flags |= Py_DTSF_ALT;

    if (type == '\0') {
        /* Omitted type specifier. Should be like str(self). */
        type = 'r';
        default_precision = 0;
        if (re == 0.0 && copysign(1.0, re) == 1.0)
            skip_re = 1;
        else
            add_parens = 1;
    }

    if (type == 'n')
        /* 'n' is the same as 'g', except for the locale used to
           format the result. We take care of that later. */
        type = 'g';

    if (precision < 0)
        precision = default_precision;
    else if (type == 'r')
        type = 'g';

    /* Cast "type", because if we're in unicode we need to pass a
       8-bit char. This is safe, because we've restricted what "type"
       can be. */
    re_buf = PyOS_double_to_string(re, (char)type, precision, flags,
                                   &re_float_type);
    if (re_buf == NULL)
        goto done;
    im_buf = PyOS_double_to_string(im, (char)type, precision, flags,
                                   &im_float_type);
    if (im_buf == NULL)
        goto done;

    n_re_digits = strlen(re_buf);
    n_im_digits = strlen(im_buf);

    /* Since there is no unicode version of PyOS_double_to_string,
       just use the 8 bit version and then convert to unicode. */
#if STRINGLIB_IS_UNICODE
    re_unicode_tmp = (Py_UNICODE*)PyMem_Malloc((n_re_digits)*sizeof(Py_UNICODE));
    if (re_unicode_tmp == NULL) {
        PyErr_NoMemory();
        goto done;
    }
    strtounicode(re_unicode_tmp, re_buf, n_re_digits);
    p_re = re_unicode_tmp;

    im_unicode_tmp = (Py_UNICODE*)PyMem_Malloc((n_im_digits)*sizeof(Py_UNICODE));
    if (im_unicode_tmp == NULL) {
        PyErr_NoMemory();
        goto done;
    }
    strtounicode(im_unicode_tmp, im_buf, n_im_digits);
    p_im = im_unicode_tmp;
#else
    p_re = re_buf;
    p_im = im_buf;
#endif

    /* Is a sign character present in the output?  If so, remember it
       and skip it */
    if (*p_re == '-') {
        re_sign_char = *p_re;
        ++p_re;
        --n_re_digits;
    }
    if (*p_im == '-') {
        im_sign_char = *p_im;
        ++p_im;
        --n_im_digits;
    }

    /* Determine if we have any "remainder" (after the digits, might include
       decimal or exponent or both (or neither)) */
    parse_number(p_re, n_re_digits, &n_re_remainder, &re_has_decimal);
    parse_number(p_im, n_im_digits, &n_im_remainder, &im_has_decimal);

    /* Determine the grouping, separator, and decimal point, if any. */
    get_locale_info(format->type == 'n' ? LT_CURRENT_LOCALE :
                    (format->thousands_separators ?
                     LT_DEFAULT_LOCALE :
                     LT_NO_LOCALE),
                    &locale);

    /* Turn off any padding. We'll do it later after we've composed
       the numbers without padding. */
    tmp_format.fill_char = '\0';
    tmp_format.align = '<';
    tmp_format.width = -1;

    /* Calculate how much memory we'll need. */
    n_re_total = calc_number_widths(&re_spec, 0, re_sign_char, p_re,
                                    n_re_digits, n_re_remainder,
                                    re_has_decimal, &locale, &tmp_format);

    /* Same formatting, but always include a sign, unless the real part is
     * going to be omitted, in which case we use whatever sign convention was
     * requested by the original format. */
    if (!skip_re)
        tmp_format.sign = '+';
    n_im_total = calc_number_widths(&im_spec, 0, im_sign_char, p_im,
                                    n_im_digits, n_im_remainder,
                                    im_has_decimal, &locale, &tmp_format);

    if (skip_re)
        n_re_total = 0;

    /* Add 1 for the 'j', and optionally 2 for parens. */
    calc_padding(n_re_total + n_im_total + 1 + add_parens * 2,
                 format->width, format->align, &lpad, &rpad, &total);

    result = STRINGLIB_NEW(NULL, total);
    if (result == NULL)
        goto done;

    /* Populate the memory. First, the padding. */
    p = fill_padding(STRINGLIB_STR(result),
                     n_re_total + n_im_total + 1 + add_parens * 2,
                     format->fill_char=='\0' ? ' ' : format->fill_char,
                     lpad, rpad);

    if (add_parens)
        *p++ = '(';

    if (!skip_re) {
        fill_number(p, &re_spec, p_re, n_re_digits, NULL, 0, &locale, 0);
        p += n_re_total;
    }
    fill_number(p, &im_spec, p_im, n_im_digits, NULL, 0, &locale, 0);
    p += n_im_total;
    *p++ = 'j';

    if (add_parens)
        *p++ = ')';

done:
    PyMem_Free(re_buf);
    PyMem_Free(im_buf);
#if STRINGLIB_IS_UNICODE
    PyMem_Free(re_unicode_tmp);
    PyMem_Free(im_unicode_tmp);
#endif
    return result;
}
#endif /* FORMAT_COMPLEX */

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
                                           &format, 's', '<'))
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
                                           &format, 'd', '>'))
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
                                           &format, '\0', '>'))
        goto done;

    /* type conversion? */
    switch (format.type) {
    case '\0': /* No format code: like 'g', but with at least one decimal. */
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

#ifdef FORMAT_COMPLEX
PyObject *
FORMAT_COMPLEX(PyObject *obj,
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
                                           &format, '\0', '>'))
        goto done;

    /* type conversion? */
    switch (format.type) {
    case '\0': /* No format code: like 'g', but with at least one decimal. */
    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'n':
        /* no conversion, already a complex.  do the formatting */
        result = format_complex_internal(obj, &format);
        break;

    default:
        /* unknown */
        unknown_presentation_type(format.type, obj->ob_type->tp_name);
        goto done;
    }

done:
    return result;
}
#endif /* FORMAT_COMPLEX */
