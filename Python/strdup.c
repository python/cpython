/* strdup() replacement (from stdwin, if you must know) */

#include "config.h"
#include "myproto.h"
#include "mymalloc.h"

#include <string.h>

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
