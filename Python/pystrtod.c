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
 * 
 * This function resets %errno before calling strtod() so that
 * you can reliably detect overflow and underflow.
 *
 * Return value: the #gdouble value.
 **/
double
PyOS_ascii_strtod(const char  *nptr, 
	    char       **endptr)
{
	char *fail_pos;
	double val;
	struct lconv *locale_data;
	const char *decimal_point;
	int decimal_point_len;
	const char *p, *decimal_point_pos;
	const char *end = NULL; /* Silence gcc */

/* 	g_return_val_if_fail (nptr != NULL, 0); */
	assert(nptr != NULL);

	fail_pos = NULL;

	locale_data = localeconv();
	decimal_point = locale_data->decimal_point;
	decimal_point_len = strlen(decimal_point);

	assert(decimal_point_len != 0);

	decimal_point_pos = NULL;
	if (decimal_point[0] != '.' || 
	    decimal_point[1] != 0)
	{
		p = nptr;
		  /* Skip leading space */
		while (ISSPACE(*p))
			p++;

		  /* Skip leading optional sign */
		if (*p == '+' || *p == '-')
			p++;

		if (p[0] == '0' && 
		    (p[1] == 'x' || p[1] == 'X'))
		{
			p += 2;
			  /* HEX - find the (optional) decimal point */

			while (ISXDIGIT(*p))
				p++;

			if (*p == '.')
			{
				decimal_point_pos = p++;

				while (ISXDIGIT(*p))
					p++;

				if (*p == 'p' || *p == 'P')
					p++;
				if (*p == '+' || *p == '-')
					p++;
				while (ISDIGIT(*p))
					p++;
				end = p;
			}
		}
		else
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
		copy = malloc(end - nptr + 1 + decimal_point_len);

		c = copy;
		memcpy(c, nptr, decimal_point_pos - nptr);
		c += decimal_point_pos - nptr;
		memcpy(c, decimal_point, decimal_point_len);
		c += decimal_point_len;
		memcpy(c, decimal_point_pos + 1, end - (decimal_point_pos + 1));
		c += end - (decimal_point_pos + 1);
		*c = 0;

		val = strtod(copy, &fail_pos);

		if (fail_pos)
		{
			if (fail_pos > decimal_point_pos)
				fail_pos = (char *)nptr + (fail_pos - copy) - (decimal_point_len - 1);
			else
				fail_pos = (char *)nptr + (fail_pos - copy);
		}

		free(copy);

	}
	else
		val = strtod(nptr, &fail_pos);

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
		   int         buf_len, 
		   const char *format, 
		   double      d)
{
	struct lconv *locale_data;
	const char *decimal_point;
	int decimal_point_len;
	char *p;
	int rest_len;
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
