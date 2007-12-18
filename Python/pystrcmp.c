/* Cross platform case insenstive string compare functions
 */

#include "Python.h"

int
PyOS_mystrnicmp(const char *s1, const char *s2, Py_ssize_t size)
{
	if (size == 0)
		return 0;
	while ((--size > 0) && (tolower(*s1) == tolower(*s2))) {
		if (!*s1++ || !*s2++)
			break;
	}
	return tolower(*s1) - tolower(*s2);
}

int
PyOS_mystricmp(const char *s1, const char *s2)
{
	while (*s1 && (tolower(*s1++) == tolower(*s2++))) {
		;
	}
	return (tolower(*s1) - tolower(*s2));
}
