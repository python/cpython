/* strdup() replacement (from stdwin, if you must know) */

#include "config.h"
#include <string.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
extern ANY *malloc Py_PROTO((size_t));
#endif

char *
strdup(str)
	const char *str;
{
	if (str != NULL) {
		register char *copy = malloc(strlen(str) + 1);
		if (copy != NULL)
			return strcpy(copy, str);
	}
	return NULL;
}
