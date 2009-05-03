/* -*- Mode: C; c-file-style: "python" -*- */

#include <Python.h>
#include <locale.h>

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

	/* Set errno to zero, so that we can distinguish zero results
	   and underflows */
	errno = 0;

	/* We process any leading whitespace and the optional sign manually,
	   then pass the remainder to the system strtod.  This ensures that
	   the result of an underflow has the correct sign. (bug #1725)  */

	p = nptr;
	/* Skip leading space */
	while (Py_ISSPACE(*p))
		p++;

	/* Process leading sign, if present */
	if (*p == '-') {
		negate = 1;
		p++;
	}
	else if (*p == '+') {
		p++;
	}

	/* Parse infinities and nans */
	if (*p == 'i' || *p == 'I') {
		if (PyOS_strnicmp(p, "inf", 3) == 0) {
			val = Py_HUGE_VAL;
			if (PyOS_strnicmp(p+3, "inity", 5) == 0)
				fail_pos = (char *)p+8;
			else
				fail_pos = (char *)p+3;
			goto got_val;
		}
		else
			goto invalid_string;
	}
#ifdef Py_NAN
	if (*p == 'n' || *p == 'N') {
		if (PyOS_strnicmp(p, "nan", 3) == 0) {
			val = Py_NAN;
			fail_pos = (char *)p+3;
			goto got_val;
		}
		else
			goto invalid_string;
	}
#endif

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
		goto invalid_string;

  got_val:
	if (negate && fail_pos != nptr)
		val = -val;

	if (endptr)
		*endptr = fail_pos;

	return val;

  invalid_string:
	if (endptr)
		*endptr = (char*)nptr;
	errno = EINVAL;
	return -1.0;
}

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


Py_LOCAL_INLINE(void)
ensure_sign(char* buffer, size_t buf_size)
{
	size_t len;

	if (buffer[0] == '-')
		/* Already have a sign. */
		return;

	/* Include the trailing 0 byte. */
	len = strlen(buffer)+1;
	if (len >= buf_size+1)
		/* No room for the sign, don't do anything. */
		return;

	memmove(buffer+1, buffer, len);
	buffer[0] = '+';
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
	char* chars_to_insert, *digits_start;

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
/* DEPRECATED, will be deleted in 2.8 and 3.2 */
PyAPI_FUNC(char *)
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

	if (PyErr_WarnEx(PyExc_DeprecationWarning,
			 "PyOS_ascii_formatd is deprecated, "
			 "use PyOS_double_to_string instead", 1) < 0)
		return NULL;

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
		buffer = ensure_decimal_point(buffer, buf_size, -1);

	return buffer;
}

/* Precisions used by repr() and str(), respectively.

   The repr() precision (17 significant decimal digits) is the minimal number
   that is guaranteed to have enough precision so that if the number is read
   back in the exact same binary value is recreated.  This is true for IEEE
   floating point by design, and also happens to work for all other modern
   hardware.

   The str() precision (12 significant decimal digits) is chosen so that in
   most cases, the rounding noise created by various operations is suppressed,
   while giving plenty of precision for practical use.

*/

PyAPI_FUNC(void)
_PyOS_double_to_string(char *buf, size_t buf_len, double val,
		    char format_code, int precision,
		    int flags, int *ptype)
{
	char format[32];
	int t;
	int upper = 0;

	if (buf_len < 1) {
		assert(0);
		/* There's no way to signal this error. Just return. */
		return;
	}
	buf[0] = 0;

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
		if (precision != 0)
			return;
		precision = 17;
		format_code = 'g';
		break;
	case 's':          /* str format */
		/* Supplied precision is unused, must be 0. */
		if (precision != 0)
			return;
		precision = 12;
		format_code = 'g';
		break;
	default:
		assert(0);
		return;
	}

	/* Check for buf too small to fit "-inf". Other buffer too small
	   conditions are dealt with when converting or formatting finite
	   numbers. */
	if (buf_len < 5) {
		assert(0);
		return;
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

		/* Build the format string. */
		PyOS_snprintf(format, sizeof(format), "%%%s.%i%c",
			      (flags & Py_DTSF_ALT ? "#" : ""), precision,
			      format_code);

		/* Have PyOS_snprintf do the hard work. */
		PyOS_snprintf(buf, buf_len, format, val);

		/* Do various fixups on the return string */

		/* Get the current locale, and find the decimal point string.
		   Convert that string back to a dot. */
		change_decimal_from_locale_to_dot(buf);

		/* If an exponent exists, ensure that the exponent is at least
		   MIN_EXPONENT_DIGITS digits, providing the buffer is large
		   enough for the extra zeros.  Also, if there are more than
		   MIN_EXPONENT_DIGITS, remove as many zeros as possible until
		   we get back to MIN_EXPONENT_DIGITS */
		ensure_minimum_exponent_length(buf, buf_len);

		/* Possibly make sure we have at least one character after the
		   decimal point (and make sure we have a decimal point). */
		if (flags & Py_DTSF_ADD_DOT_0)
			buf = ensure_decimal_point(buf, buf_len, precision);
	}

	/* Add the sign if asked and the result isn't negative. */
	if (flags & Py_DTSF_SIGN && buf[0] != '-')
		ensure_sign(buf, buf_len);

	if (upper) {
		/* Convert to upper case. */
		char *p;
		for (p = buf; *p; p++)
			*p = Py_TOUPPER(*p);
	}

	if (ptype)
		*ptype = t;
}


PyAPI_FUNC(char *) PyOS_double_to_string(double val,
                                         char format_code,
                                         int precision,
                                         int flags,
                                         int *ptype)
{
	char buf[128];
	Py_ssize_t len;
	char *result;

	_PyOS_double_to_string(buf, sizeof(buf), val, format_code, precision,
			       flags, ptype);
	len = strlen(buf);
	if (len == 0) {
		PyErr_BadInternalCall();
		return NULL;
	}

	/* Add 1 for the trailing 0 byte. */
	result = PyMem_Malloc(len + 1);
	if (result == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	strcpy(result, buf);

	return result;
}
