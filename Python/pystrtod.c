/* -*- Mode: C; c-file-style: "python" -*- */

#include <Python.h>
#include <locale.h>

/* ascii character tests (as opposed to locale tests) */
#define ISSPACE(c)  ((c) == ' ' || (c) == '\f' || (c) == '\n' || \
                     (c) == '\r' || (c) == '\t' || (c) == '\v')
#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')
#define ISXDIGIT(c) (ISDIGIT(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))


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
		/* For the other cases, we need not convert the decimal point */
	}

	/* Set errno to zero, so that we can distinguish zero results
	   and underflows */
	errno = 0;

	if (decimal_point_pos)
	{
		char *copy, *c;

		/* We need to convert the '.' to the locale specific decimal point */
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
		memcpy(c, decimal_point_pos + 1, end - (decimal_point_pos + 1));
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


/* From the C99 standard, section 7.19.6:
The exponent always contains at least two digits, and only as many more digits
as necessary to represent the exponent.
*/
#define MIN_EXPONENT_DIGITS 2

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
 * specifiers are 'e', 'E', 'f', 'F', 'g', 'G', and 'n'.
 * 
 * 'n' is the same as 'g', except it uses the current locale.
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
	char *p;
	char format_char;
	size_t format_len = strlen(format);

	/* For type 'n', we need to make a copy of the format string, because
	   we're going to modify 'n' -> 'g', and format is const char*, so we
	   can't modify it directly.  FLOAT_FORMATBUFLEN should be longer than
	   we ever need this to be.  There's an upcoming check to ensure it's
	   big enough. */
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
	      format_char == 'n' || format_char == 'Z'))
		return NULL;

	/* Map 'n' or 'Z' format_char to 'g', by copying the format string and
	   replacing the final char with a 'g' */
	if (format_char == 'n' || format_char == 'Z') {
		if (format_len + 1 >= sizeof(tmp_format)) {
			/* The format won't fit in our copy.  Error out.  In
			   practice, this will never happen and will be detected
			   by returning NULL */
			return NULL;
		}
		strcpy(tmp_format, format);
		tmp_format[format_len - 1] = 'g';
		format = tmp_format;
	}


	/* Have PyOS_snprintf do the hard work */
	PyOS_snprintf(buffer, buf_size, format, d);

	/* Get the current local, and find the decimal point character (or
	   string?).  Convert that string back to a dot.  Do not do this if
	   using the 'n' (number) format code. */
	if (format_char != 'n') {
		struct lconv *locale_data = localeconv();
		const char *decimal_point = locale_data->decimal_point;
		size_t decimal_point_len = strlen(decimal_point);
		size_t rest_len;

		assert(decimal_point_len != 0);

		if (decimal_point[0] != '.' || decimal_point[1] != 0) {
			p = buffer;

			if (*p == '+' || *p == '-')
				p++;

			while (isdigit(Py_CHARMASK(*p)))
				p++;

			if (strncmp(p, decimal_point, decimal_point_len) == 0) {
				*p = '.';
				p++;
				if (decimal_point_len > 1) {
					rest_len = strlen(p +
						      (decimal_point_len - 1));
					memmove(p, p + (decimal_point_len - 1),
						rest_len);
					p[rest_len] = 0;
				}
			}
		}
	}

	/* If an exponent exists, ensure that the exponent is at least
	   MIN_EXPONENT_DIGITS digits, providing the buffer is large enough
	   for the extra zeros.  Also, if there are more than
	   MIN_EXPONENT_DIGITS, remove as many zeros as possible until we get
	   back to MIN_EXPONENT_DIGITS */
	p = strpbrk(buffer, "eE");
	if (p && (*(p + 1) == '-' || *(p + 1) == '+')) {
		char *start = p + 2;
		int exponent_digit_cnt = 0;
		int leading_zero_cnt = 0;
		int in_leading_zeros = 1;
		int significant_digit_cnt;

		p += 2;
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
			extra_zeros_cnt = exponent_digit_cnt - significant_digit_cnt;

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

	/* If format_char is 'Z', make sure we have at least one character
	   after the decimal point (and make sure we have a decimal point). */
	if (format_char == 'Z') {
		int insert_count = 0;
		char* chars_to_insert;

		/* search for the first non-digit character */
		p = buffer;
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
		else {
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

	return buffer;
}

double
PyOS_ascii_atof(const char *nptr)
{
	return PyOS_ascii_strtod(nptr, NULL);
}
