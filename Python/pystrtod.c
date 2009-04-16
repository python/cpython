/* -*- Mode: C; c-file-style: "python" -*- */

#include <Python.h>
#include <locale.h>

/* ascii character tests (as opposed to locale tests) */
#define ISSPACE(c)  ((c) == ' ' || (c) == '\f' || (c) == '\n' || \
                     (c) == '\r' || (c) == '\t' || (c) == '\v')
#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')


/**
 * PyOS_ascii_strtod:
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

double
PyOS_ascii_strtod(const char *nptr, char **endptr)
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

	return result;

}

#else

/*
   Use system strtod;  since strtod is locale aware, we may
   have to first fix the decimal separator.

   Note that unlike _Py_dg_strtod, the system strtod may not always give
   correctly rounded results.
*/

double
PyOS_ascii_strtod(const char *nptr, char **endptr)
{
	char *fail_pos;
	double val = -1.0;
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

	/* We process any leading whitespace and the optional sign manually,
	   then pass the remainder to the system strtod.  This ensures that
	   the result of an underflow has the correct sign. (bug #1725)  */

	p = nptr;
	/* Skip leading space */
	while (ISSPACE(*p))
		p++;

	/* Process leading sign, if present */
	if (*p == '-') {
		negate = 1;
		p++;
	} else if (*p == '+') {
		p++;
	}

	/* What's left should begin with a digit, a decimal point, or one of
	   the letters i, I, n, N. It should not begin with 0x or 0X */
	if ((!ISDIGIT(*p) &&
	     *p != '.' && *p != 'i' && *p != 'I' && *p != 'n' && *p != 'N')
	    ||
	    (*p == '0' && (p[1] == 'x' || p[1] == 'X')))
	{
		if (endptr)
			*endptr = (char*)nptr;
		errno = EINVAL;
		return val;
	}
	digits_pos = p;

	if (decimal_point[0] != '.' || 
	    decimal_point[1] != 0)
	{
		while (ISDIGIT(*p))
			p++;

		if (*p == '.')
		{
			decimal_point_pos = p++;

			while (ISDIGIT(*p))
				p++;

			if (*p == 'e' || *p == 'E')
				p++;
			if (*p == '+' || *p == '-')
				p++;
			while (ISDIGIT(*p))
				p++;
			end = p;
		}
		else if (strncmp(p, decimal_point, decimal_point_len) == 0)
		{
			/* Python bug #1417699 */
			if (endptr)
				*endptr = (char*)nptr;
			errno = EINVAL;
			return val;
		}
		/* For the other cases, we need not convert the decimal
		   point */
	}

	/* Set errno to zero, so that we can distinguish zero results
	   and underflows */
	errno = 0;

	if (decimal_point_pos)
	{
		char *copy, *c;

		/* We need to convert the '.' to the locale specific decimal
		   point */
		copy = (char *)PyMem_MALLOC(end - digits_pos +
					    1 + decimal_point_len);
		if (copy == NULL) {
			if (endptr)
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

		PyMem_FREE(copy);

	}
	else {
		val = strtod(digits_pos, &fail_pos);
	}

	if (fail_pos == digits_pos)
		fail_pos = (char *)nptr;

	if (negate && fail_pos != nptr)
		val = -val;

	if (endptr)
		*endptr = fail_pos;

	return val;
}

#endif

double
PyOS_ascii_atof(const char *nptr)
{
	return PyOS_ascii_strtod(nptr, NULL);
}


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
		while (isdigit(Py_CHARMASK(*buffer)))
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
ensure_minumim_exponent_length(char* buffer, size_t buf_size)
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
		while (*p && isdigit(Py_CHARMASK(*p))) {
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

/* Ensure that buffer has a decimal point in it.  The decimal point will not
   be in the current locale, it will always be '.'. Don't add a decimal if an
   exponent is present. */
Py_LOCAL_INLINE(void)
ensure_decimal_point(char* buffer, size_t buf_size)
{
	int insert_count = 0;
	char* chars_to_insert;

	/* search for the first non-digit character */
	char *p = buffer;
	if (*p == '-' || *p == '+')
		/* Skip leading sign, if present.  I think this could only
		   ever be '-', but it can't hurt to check for both. */
		++p;
	while (*p && isdigit(Py_CHARMASK(*p)))
		++p;

	if (*p == '.') {
		if (isdigit(Py_CHARMASK(*(p+1)))) {
			/* Nothing to do, we already have a decimal
			   point and a digit after it */
		}
		else {
			/* We have a decimal point, but no following
			   digit.  Insert a zero after the decimal. */
			++p;
			chars_to_insert = "0";
			insert_count = 1;
		}
	}
	else if (!(*p == 'e' || *p == 'E')) {
		/* Don't add ".0" if we have an exponent. */
		chars_to_insert = ".0";
		insert_count = 2;
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
}

/* see FORMATBUFLEN in unicodeobject.c */
#define FLOAT_FORMATBUFLEN 120

/**
 * PyOS_ascii_formatd:
 * @buffer: A buffer to place the resulting string in
 * @buf_size: The length of the buffer.
 * @format: The printf()-style format to use for the
 *          code to use for converting. 
 * @d: The #gdouble to convert
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
 **/
char *
PyOS_ascii_formatd(char       *buffer, 
		   size_t      buf_size, 
		   const char *format, 
		   double      d)
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
	ensure_minumim_exponent_length(buffer, buf_size);

	/* If format_char is 'Z', make sure we have at least one character
	   after the decimal point (and make sure we have a decimal point). */
	if (format_char == 'Z')
		ensure_decimal_point(buffer, buf_size);

	return buffer;
}

#ifdef PY_NO_SHORT_FLOAT_REPR

/* The fallback code to use if _Py_dg_dtoa is not available. */

PyAPI_FUNC(char *) PyOS_double_to_string(double val,
                                         char format_code,
                                         int precision,
                                         int flags,
                                         int *type)
{
	char buf[128];
	char format[32];
	Py_ssize_t len;
	char *result;
	char *p;
	int t;
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
		precision = 17;
		format_code = 'g';
		break;
	case 's':          /* str format */
		/* Supplied precision is unused, must be 0. */
		if (precision != 0) {
			PyErr_BadInternalCall();
			return NULL;
		}
		precision = 12;
		format_code = 'g';
		break;
	default:
		PyErr_BadInternalCall();
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

		PyOS_snprintf(format, 32, "%%%s.%i%c", (flags & Py_DTSF_ALT ? "#" : ""), precision, format_code);
		PyOS_ascii_formatd(buf, sizeof(buf), format, val);
	}

	len = strlen(buf);

	/* Add 1 for the trailing 0 byte.
	   Add 1 because we might need to make room for the sign.
	   */
	result = PyMem_Malloc(len + 2);
	if (result == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	p = result;

	/* Never add sign for nan/inf, even if asked. */
	if (flags & Py_DTSF_SIGN && buf[0] != '-' && t == Py_DTST_FINITE)
		*p++ = '+';

	strcpy(p, buf);

	if (upper) {
		/* Convert to upper case. */
		char *p1;
		for (p1 = p; *p1; p1++)
			*p1 = toupper(*p1);
	}

	if (type)
		*type = t;
	return result;
}

#else

/* _Py_dg_dtoa is available. */

/* I'm using a lookup table here so that I don't have to invent a non-locale
   specific way to convert to uppercase */
#define OFS_INF 0
#define OFS_NAN 1
#define OFS_E 2

/* The lengths of these are known to the code below, so don't change them */
static char *lc_float_strings[] = {
	"inf",
	"nan",
	"e",
};
static char *uc_float_strings[] = {
	"INF",
	"NAN",
	"E",
};


/* Convert a double d to a string, and return a PyMem_Malloc'd block of
   memory contain the resulting string.

   Arguments:
     d is the double to be converted
     format_code is one of 'e', 'f', 'g', 'r' or 's'.  'e', 'f' and 'g'
       correspond to '%e', '%f' and '%g';  'r' and 's' correspond
       to repr and str.
     mode is one of '0', '2' or '3', and is completely determined by
       format_code: 'e', 'g' and 's' use mode 2; 'f' mode 3, 'r' mode 0.
     precision is the desired precision
     always_add_sign is nonzero if a '+' sign should be included for positive
       numbers
     add_dot_0_if_integer is nonzero if integers in non-exponential form
       should have ".0" added.  Only applies to format codes 'r', 's', and 'g'.
     use_alt_formatting is nonzero if alternative formatting should be
       used.  Only applies to format codes 'e', 'f' and 'g'.
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
		   int mode, Py_ssize_t precision,
		   int always_add_sign, int add_dot_0_if_integer,
		   int use_alt_formatting, char **float_strings, int *type)
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

	if (digits_len && !isdigit(digits[0])) {
		/* Infinities and nans here; adapt Gay's output,
		   so convert Infinity to inf and NaN to nan, and
		   ignore sign of nan. Then return. */

		/* We only need 5 bytes to hold the result "+inf\0" . */
		bufsize = 5; /* Used later in an assert. */
		buf = (char *)PyMem_Malloc(bufsize);
		if (buf == NULL) {
			PyErr_NoMemory();
			goto exit;
		}
		p = buf;

		if (digits[0] == 'i' || digits[0] == 'I') {
			if (sign == 1) {
				*p++ = '-';
			}
			else if (always_add_sign) {
				*p++ = '+';
			}
			strncpy(p, float_strings[OFS_INF], 3);
			p += 3;

			if (type)
				*type = Py_DTST_INFINITE;
		}
		else if (digits[0] == 'n' || digits[0] == 'N') {
			/* note that we *never* add a sign for a nan,
			   even if one has explicitly been requested */
			strncpy(p, float_strings[OFS_NAN], 3);
			p += 3;

			if (type)
				*type = Py_DTST_NAN;
		}
		else {
			/* shouldn't get here: Gay's code should always return
			   something starting with a digit, an 'I',  or 'N' */
			strncpy(p, "ERR", 3);
			p += 3;
			assert(0);
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
		if (decpt <= -4 || decpt > precision)
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
	case 's':
		/* if we're forcing a digit after the point, convert to
		   exponential format at 1e11.  If not, convert at 1e12. */
		if (decpt <= -4 || decpt >
		    (add_dot_0_if_integer ? precision-1 : precision))
			use_exp = 1;
		break;
	default:
		PyErr_BadInternalCall();
		goto exit;
	}

	/* if using an exponent, reset decimal point position to 1 and adjust
	   exponent accordingly.*/
	if (use_exp) {
		exp = decpt - 1;
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


PyAPI_FUNC(char *) PyOS_double_to_string(double val,
					 char format_code,
					 int precision,
					 int flags,
					 int *type)
{
	char **float_strings = lc_float_strings;
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

	/* str format */
	case 's':
		mode = 2;
		/* Supplied precision is unused, must be 0. */
		if (precision != 0) {
			PyErr_BadInternalCall();
			return NULL;
		}
		precision = 12;
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
