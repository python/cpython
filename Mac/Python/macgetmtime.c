#include "macstat.h"

/* Interfaced used by import.c */

long
getmtime(path)
	char *path;
{
	struct macstat st;
	if (macstat(path, &st) != 0)
		return -1L;
	return st.st_mtime;
}
