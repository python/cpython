#ifndef Py_MYMALLOC_H
#define Py_MYMALLOC_H
/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Lowest-level memory allocation interface */

#ifdef macintosh
#define ANY void
#endif

#ifdef __STDC__
#define ANY void
#endif

#ifdef __TURBOC__
#define ANY void
#endif

#ifdef __GNUC__
#define ANY void
#endif

#ifndef ANY
#define ANY char
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef __cplusplus
/* Move this down here since some C++ #include's don't like to be included
   inside an extern "C" */
extern "C" {
#endif

#ifdef SYMANTEC__CFM68K__
#pragma lib_export on
#endif

/* The following should never be necessary */
#ifdef NEED_TO_DECLARE_MALLOC_AND_FRIEND
extern ANY *malloc Py_PROTO((size_t));
extern ANY *calloc Py_PROTO((size_t, size_t));
extern ANY *realloc Py_PROTO((ANY *, size_t));
extern void free Py_PROTO((ANY *)); /* XXX sometimes int on Unix old systems */
#endif

#ifndef NULL
#define NULL ((ANY *)0)
#endif

#ifdef MALLOC_ZERO_RETURNS_NULL
/* XXX Always allocate one extra byte, since some malloc's return NULL
   XXX for malloc(0) or realloc(p, 0). */
#define _PyMem_EXTRA 1
#else
#define _PyMem_EXTRA 0
#endif

#define PyMem_NEW(type, n) \
	( (type *) malloc(_PyMem_EXTRA + (n) * sizeof(type)) )
#define PyMem_RESIZE(p, type, n) \
	if ((p) == NULL) \
		(p) =  (type *) malloc(_PyMem_EXTRA + (n) * sizeof(type)); \
	else \
		(p) = (type *) realloc((ANY *)(p), \
				       _PyMem_EXTRA + (n) * sizeof(type))
#define PyMem_DEL(p) free((ANY *)p)
#define PyMem_XDEL(p) if ((p) == NULL) ; else PyMem_DEL(p)


/* Two sets of function wrappers around malloc and friends; useful if
   you need to be sure that you are using the same memory allocator as
   Python.  Note that the wrappers make sure that allocating 0 bytes
   returns a non-NULL pointer, even if the underlying malloc doesn't.
   The Python interpreter continues to use PyMem_NEW etc. */

/* These wrappers around malloc call PyErr_NoMemory() on failure */
extern ANY *Py_Malloc Py_PROTO((size_t));
extern ANY *Py_Realloc Py_PROTO((ANY *, size_t));
extern void Py_Free Py_PROTO((ANY *));

/* These wrappers around malloc *don't* call anything on failure */
extern ANY *PyMem_Malloc Py_PROTO((size_t));
extern ANY *PyMem_Realloc Py_PROTO((ANY *, size_t));
extern void PyMem_Free Py_PROTO((ANY *));

#ifdef __cplusplus
}
#endif

#endif /* !Py_MYMALLOC_H */
