/*************************************************
*       Perl-Compatible Regular Expressions      *
*************************************************/

/* Copyright (c) 1997 University of Cambridge */

/* Have to include stdlib.h in order to ensure that size_t is defined;
it is needed in there for malloc. */

#ifndef PCRE_H
#define PCRE_H

#include <stdlib.h>
#ifdef FOR_PYTHON
#include "Python.h"
#endif

/* Options */

#define PCRE_CASELESS     0x01
#define PCRE_EXTENDED     0x02
#define PCRE_ANCHORED     0x04
#define PCRE_MULTILINE    0x08
#define PCRE_DOTALL       0x10

/* Exec-time error codes */

#define PCRE_ERROR_NOMATCH        (-1)
#define PCRE_ERROR_BADREF         (-2)
#define PCRE_ERROR_NULL           (-3)
#define PCRE_ERROR_BADOPTION      (-4)
#define PCRE_ERROR_BADMAGIC       (-5)
#define PCRE_ERROR_UNKNOWN_NODE   (-6)

/* Types */

typedef void pcre;
typedef void pcre_extra;

/* Store get and free functions. These can be set to alternative malloc/free
functions if required. */

extern void *(*pcre_malloc)(size_t);
extern void  (*pcre_free)(void *);

/* Functions */

#ifdef FOR_PYTHON
extern pcre *pcre_compile(char *, int, char **, int *, PyObject *);
#else
extern pcre *pcre_compile(char *, int, char **, int *);
#endif
extern int pcre_exec(pcre *, pcre_extra *, char *, int, int, int *, int);
extern int pcre_info(pcre *, int *, int *);
extern pcre_extra *pcre_study(pcre *, int, char **);
extern char *pcre_version(void);

#endif /* ifndef PCRE_H */
/* End of pcre.h */
