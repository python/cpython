/* Two PD getcwd() implementations.
   Author: Guido van Rossum, CWI Amsterdam, Jan 1991, <guido@cwi.nl>. */

/* #define NO_GETWD /* Turn this on to popen pwd instead of calling getwd() */

#include <stdio.h>
#include <errno.h>

extern int errno;

#ifndef NO_GETWD

/* Default: Version for BSD systems -- use getwd() */

#include "sys/param.h"

extern char *getwd();

char *
getcwd(buf, size)
	char *buf;
	int size;
{
	char localbuf[MAXPATHLEN+1];
	char *ret;
	
	if (size <= 0) {
		errno = EINVAL;
		return NULL;
	}
	ret = getwd(localbuf);
	if (ret != NULL && strlen(localbuf) >= size) {
		errno = ERANGE;
		return NULL;
	}
	if (ret == NULL) {
		errno = EACCES; /* Most likely error */
		return NULL;
	}
	strncpy(buf, localbuf, size);
	return buf;
}

#else

/* NO_GETWD defined: Version for backward UNIXes -- popen /bin/pwd */

#define PWD_CMD "/bin/pwd"

char *
getcwd(buf, size)
	char *buf;
	int size;
{
	FILE *fp;
	char *p;
	int sts;
	if (size <= 0) {
		errno = EINVAL;
		return NULL;
	}
	if ((fp = popen(PWD_CMD, "r")) == NULL)
		return NULL;
	if (fgets(buf, size, fp) == NULL || (sts = pclose(fp)) != 0) {
		errno = EACCES; /* Most likely error */
		return NULL;
	}
	for (p = buf; *p != '\n'; p++) {
		if (*p == '\0') {
			errno = ERANGE;
			return NULL;
		}
	}
	*p = '\0';
	return buf;
}

#endif
