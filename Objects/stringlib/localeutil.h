/* stringlib: locale related helpers implementation */

#ifndef STRINGLIB_LOCALEUTIL_H
#define STRINGLIB_LOCALEUTIL_H

#include <locale.h>

/**
 * _Py_InsertThousandsGrouping:
 * @buffer: A pointer to the start of a string.
 * @n_buffer: The length of the string.
 * @n_digits: The number of digits in the string, in which we want
 *            to put the grouping chars.
 * @buf_size: The maximum size of the buffer pointed to by buffer.
 * @count: If non-NULL, points to a variable that will receive the
 *         number of characters we need to insert (and no formatting
 *         will actually occur).
 * @append_zero_char: If non-zero, put a trailing zero at the end of
 *         of the resulting string, if and only if we modified the
 *         string.
 *
 * Inserts thousand grouping characters (as defined in the current
 *  locale) into the string between buffer and buffer+n_digits.  If
 *  count is non-NULL, don't do any formatting, just count the number
 *  of characters to insert.  This is used by the caller to
 *  appropriately resize the buffer, if needed.  If count is non-NULL,
 *  buffer can be NULL (it is not dereferenced at all in that case).
 *
 * Return value: 0 on error, else 1.  Note that no error can occur if
 *  count is non-NULL.
 *
 * This name won't be used, the includer of this file should define
 *  it to be the actual function name, based on unicode or string.
 **/
int
_Py_InsertThousandsGrouping(STRINGLIB_CHAR *buffer,
			    Py_ssize_t n_buffer,
			    Py_ssize_t n_digits,
			    Py_ssize_t buf_size,
			    Py_ssize_t *count,
			    int append_zero_char)
{
	struct lconv *locale_data = localeconv();
	const char *grouping = locale_data->grouping;
	const char *thousands_sep = locale_data->thousands_sep;
	Py_ssize_t thousands_sep_len = strlen(thousands_sep);
	STRINGLIB_CHAR *pend = NULL; /* current end of buffer */
	STRINGLIB_CHAR *pmax = NULL; /* max of buffer */
	char current_grouping;
	Py_ssize_t remaining = n_digits; /* Number of chars remaining to
					    be looked at */

	/* Initialize the character count, if we're just counting. */
	if (count)
		*count = 0;
	else {
		/* We're not just counting, we're modifying buffer */
		pend = buffer + n_buffer;
		pmax = buffer + buf_size;
	}

	/* Starting at the end and working right-to-left, keep track of
	   what grouping needs to be added and insert that. */
	current_grouping = *grouping++;

	/* If the first character is 0, perform no grouping at all. */
	if (current_grouping == 0)
		return 1;

	while (remaining > current_grouping) {
		/* Always leave buffer and pend valid at the end of this
		   loop, since we might leave with a return statement. */

		remaining -= current_grouping;
		if (count) {
			/* We're only counting, not touching the memory. */
			*count += thousands_sep_len;
		}
		else {
			/* Do the formatting. */

			STRINGLIB_CHAR *plast = buffer + remaining;

			/* Is there room to insert thousands_sep_len chars? */
			if (pmax - pend < thousands_sep_len)
				/* No room. */
				return 0;

			/* Move the rest of the string down. */
			memmove(plast + thousands_sep_len,
				plast,
				(pend - plast) * sizeof(STRINGLIB_CHAR));
			/* Copy the thousands_sep chars into the buffer. */
#if STRINGLIB_IS_UNICODE
			/* Convert from the char's of the thousands_sep from
			   the locale into unicode. */
			{
				Py_ssize_t i;
				for (i = 0; i < thousands_sep_len; ++i)
					plast[i] = thousands_sep[i];
			}
#else
			/* No conversion, just memcpy the thousands_sep. */
			memcpy(plast, thousands_sep, thousands_sep_len);
#endif
		}

		/* Adjust end pointer. */
		pend += thousands_sep_len;

		/* Move to the next grouping character, unless we're
		   repeating (which is designated by a grouping of 0). */
		if (*grouping != 0) {
			current_grouping = *grouping++;
			if (current_grouping == CHAR_MAX)
				/* We're done. */
				break;
		}
	}
	if (append_zero_char) {
		/* Append a zero character to mark the end of the string,
		   if there's room. */
		if (pend - (buffer + remaining) < 1)
			/* No room, error. */
			return 0;
		*pend = 0;
	}
	return 1;
}
#endif /* STRINGLIB_LOCALEUTIL_H */
