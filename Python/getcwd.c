/* Quick hack to get posix.getcwd() working for pure BSD 4.3 */

#include "sys/param.h"
#include "errno.h"

extern int errno;

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

/* PS: for really old systems you must popen /bin/pwd ... */
