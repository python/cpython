
/* Lowest-level memory allocation interface */

#ifndef Py_PYMEM_H
#define Py_PYMEM_H

#include "pyport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BEWARE:

   Each interface exports both functions and macros. Extension modules
   should normally use the functions for ensuring binary compatibility
   of the user's code across Python versions. Subsequently, if the
   Python runtime switches to its own malloc (different from standard
   malloc), no recompilation is required for the extensions.

   The macro versions are free to trade compatibility for speed, although
   there's no guarantee they're ever faster.  Extensions shouldn't use the
   macro versions, as they don't gurantee binary compatibility across
   releases.

   Do not mix calls to PyMem_xyz with calls to platform
   malloc/realloc/calloc/free. */

/*
 * Raw memory interface
 * ====================
 */

/* Functions */

/* Functions supplying platform-independent semantics for malloc/realloc/
   free; useful if you need to be sure you're using the same memory
   allocator as Python (this can be especially important on Windows, if
   you need to make sure you're using the same MS malloc/free, and out of
   the same heap, as the main Python DLL uses).
   These functions make sure that allocating 0 bytes returns a distinct
   non-NULL pointer (whenever possible -- if we're flat out of memory, NULL
   may be returned), even if the platform malloc and realloc don't.
   Returned pointers must be checked for NULL explicitly.  No action is
   performed on failure (no exception is set, no warning is printed, etc).` */

extern DL_IMPORT(void *) PyMem_Malloc(size_t);
extern DL_IMPORT(void *) PyMem_Realloc(void *, size_t);
extern DL_IMPORT(void) PyMem_Free(void *);

/* Starting from Python 1.6, the wrappers Py_{Malloc,Realloc,Free} are
   no longer supported. They used to call PyErr_NoMemory() on failure. */

/* Macros. */
#ifndef PyMem_MALLOC
#ifdef MALLOC_ZERO_RETURNS_NULL
#define PyMem_MALLOC(n)         malloc((n) ? (n) : 1)
#else
#define PyMem_MALLOC		malloc
#endif

/* Caution:  whether MALLOC_ZERO_RETURNS_NULL is #defined has nothing to
   do with whether platform realloc(non-NULL, 0) normally frees the memory
   or returns NULL.  Rather than introduce yet another config variation,
   just make a realloc to 0 bytes act as if to 1 instead. */
#define PyMem_REALLOC(p, n)     realloc((p), (n) ? (n) : 1)

#define PyMem_FREE           	free
#endif	/* PyMem_MALLOC */

/*
 * Type-oriented memory interface
 * ==============================
 *
 * These are carried along for historical reasons.  There's rarely a good
 * reason to use them anymore.
 */

/* Functions */
#define PyMem_New(type, n) \
	( (type *) PyMem_Malloc((n) * sizeof(type)) )
#define PyMem_Resize(p, type, n) \
	( (p) = (type *) PyMem_Realloc((p), (n) * sizeof(type)) )

/* In order to avoid breaking old code mixing PyObject_{New, NEW} with
   PyMem_{Del, DEL} (there was no choice about this in 1.5.2), the latter
   have to be redirected to the object allocator. */
/* XXX The parser module needs rework before this can be enabled. */
#if 0
#define PyMem_Del  PyObject_Free
#else
#define PyMem_Del  PyMem_Free
#endif

/* Macros */
#define PyMem_NEW(type, n) \
	( (type *) PyMem_MALLOC((n) * sizeof(type)) )
#define PyMem_RESIZE(p, type, n) \
	( (p) = (type *) PyMem_REALLOC((p), (n) * sizeof(type)) )

/* XXX The parser module needs rework before this can be enabled. */
#if 0
#define PyMem_DEL PyObject_FREE
#else
#define PyMem_DEL  PyMem_FREE
#endif

#ifdef __cplusplus
}
#endif

#endif /* !Py_PYMEM_H */
