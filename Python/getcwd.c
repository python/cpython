
/* Two PD getcwd() implementations.
   Author: Guido van Rossum, CWI Amsterdam, Jan 1991, <guido@cwi.nl>. */

#include <stdio.h>
#include <errno.h>

#ifdef HAVE_GETWD

/* Version for BSD systems -- use getwd() */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef MAXPATHLEN
#if defined(PATH_MAX) && PATH_MAX > 1024
#define MAXPATHLEN PATH_MAX
#else
#define MAXPATHLEN 1024
#endif
#endif

extern char *getwd(char *);

char *
getcwd(char *buf, int size)
{
    char localbuf[MAXPATHLEN+1];
    char *ret;

    if (size <= 0) {
        errno = EINVAL;
        return NULL;
    }
    ret = getwd(localbuf);
    if (ret != NULL && strlen(localbuf) >= (size_t)size) {
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

#else /* !HAVE_GETWD */

/* Version for really old UNIX systems -- use pipe from pwd */

#ifndef PWD_CMD
#define PWD_CMD "/bin/pwd"
#endif

char *
getcwd(char *buf, int size)
{
    FILE *fp;
    char *p;
    if (size <= 0) {
        errno = EINVAL;
        return NULL;
    }
    if ((fp = popen(PWD_CMD, "r")) == NULL)
        return NULL;
    if (fgets(buf, size, fp) == NULL || pclose(fp) != 0) {
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

#endif /* !HAVE_GETWD */
