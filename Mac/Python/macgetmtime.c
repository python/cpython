#include <stat.h>

/* Interfaced used by import.c */

long
getmtime(path)
	char *path;
{
	struct stat st;
	if (stat(path, &st) != 0)
		return -1L;
	return st.st_mtime;
}
