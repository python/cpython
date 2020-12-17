/*
 * string.h --
 *
 *	Declarations of ANSI C library procedures for string handling.
 *
 * Copyright (c) 1991-1993 The Regents of the University of California.
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _STRING
#define _STRING

/*
 * The following #include is needed to define size_t. (This used to include
 * sys/stdtypes.h but that doesn't exist on older versions of SunOS, e.g.
 * 4.0.2, so I'm trying sys/types.h now.... hopefully it exists everywhere)
 */

#include <sys/types.h>

#ifdef __APPLE__
extern void *		memchr(const void *s, int c, size_t n);
#else
extern char *		memchr(const void *s, int c, size_t n);
#endif
extern int		memcmp(const void *s1, const void *s2, size_t n);
extern char *		memcpy(void *t, const void *f, size_t n);
#ifdef NO_MEMMOVE
#define memmove(d,s,n)	(bcopy((s), (d), (n)))
#else
extern char *		memmove(void *t, const void *f, size_t n);
#endif
extern char *		memset(void *s, int c, size_t n);

extern int		strcasecmp(const char *s1, const char *s2);
extern char *		strcat(char *dst, const char *src);
extern char *		strchr(const char *string, int c);
extern int		strcmp(const char *s1, const char *s2);
extern char *		strcpy(char *dst, const char *src);
extern size_t		strcspn(const char *string, const char *chars);
extern char *		strdup(const char *string);
extern char *		strerror(int error);
extern size_t		strlen(const char *string);
extern int		strncasecmp(const char *s1, const char *s2, size_t n);
extern char *		strncat(char *dst, const char *src, size_t numChars);
extern int		strncmp(const char *s1, const char *s2, size_t nChars);
extern char *		strncpy(char *dst, const char *src, size_t numChars);
extern char *		strpbrk(const char *string, const char *chars);
extern char *		strrchr(const char *string, int c);
extern size_t		strspn(const char *string, const char *chars);
extern char *		strstr(const char *string, const char *substring);
extern char *		strtok(char *s, const char *delim);

#endif /* _STRING */
