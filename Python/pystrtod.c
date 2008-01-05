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


/**
 * PyOS_ascii_formatd:
 * @buffer: A buffer to place the resulting string in
 * @buf_len: The length of the buffer.
 * @format: The printf()-style format to use for the
 *          code to use for converting. 
 * @d: The #gdouble to convert
 *
 * Converts a #gdouble to a string, using the '.' as
 * decimal point. To format the number you pass in
 * a printf()-style format string. Allowed conversion
 * specifiers are 'e', 'E', 'f', 'F', 'g' and 'G'. 
 * 
 * Return value: The pointer to the buffer with the converted string.
 **/
char *
PyOS_ascii_formatd(char       *buffer, 
		   size_t      buf_len, 
		   const char *format, 
		   double      d)
{
	struct lconv *locale_data;
	const char *decimal_point;
	size_t decimal_point_len, rest_len;
	char *p;
	char format_char;

/* 	g_return_val_if_fail (buffer != NULL, NULL); */
/* 	g_return_val_if_fail (format[0] == '%', NULL); */
/* 	g_return_val_if_fail (strpbrk (format + 1, "'l%") == NULL, NULL); */

	format_char = format[strlen(format) - 1];

/* 	g_return_val_if_fail (format_char == 'e' || format_char == 'E' || */
/* 			      format_char == 'f' || format_char == 'F' || */
/* 			      format_char == 'g' || format_char == 'G', */
/* 			      NULL); */

	if (format[0] != '%')
		return NULL;

	if (strpbrk(format + 1, "'l%"))
		return NULL;

	if (!(format_char == 'e' || format_char == 'E' || 
	      format_char == 'f' || format_char == 'F' || 
	      format_char == 'g' || format_char == 'G'))
		return NULL;


	PyOS_snprintf(buffer, buf_len, format, d);

	locale_data = localeconv();
	decimal_point = locale_data->decimal_point;
	decimal_point_len = strlen(decimal_point);

	assert(decimal_point_len != 0);

	if (decimal_point[0] != '.' || 
	    decimal_point[1] != 0)
	{
		p = buffer;

		if (*p == '+' || *p == '-')
			p++;

		while (isdigit((unsigned char)*p))
			p++;

		if (strncmp(p, decimal_point, decimal_point_len) == 0)
		{
			*p = '.';
			p++;
			if (decimal_point_len > 1) {
				rest_len = strlen(p + (decimal_point_len - 1));
				memmove(p, p + (decimal_point_len - 1), 
					rest_len);
				p[rest_len] = 0;
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
