
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

   The macro versions trade compatibility for speed. They can be used
   whenever there is a performance problem, but their use implies
   recompilation of the code for each new Python release. The Python
   core uses the macros because it *is* compiled on every upgrade.
   This might not be the case with 3rd party extensions in a custom
   setup (for example, a customer does not always have access to the
   source of 3rd party deliverables). You have been warned! */

/*
 * Raw memory interface
 * ====================
 */

/* To make sure the interpreter is user-malloc friendly, all memory
   APIs are implemented on top of this one. */

/* Functions */

/* Function wrappers around PyMem_MALLOC and friends; useful if you
   need to be sure that you are using the same memory allocator as
   Python.  Note that the wrappers make sure that allocating 0 bytes
   returns a non-NULL pointer, even if the underlying malloc
   doesn't. Returned pointers must be checked for NULL explicitly.
   No action is performed on failure. */
extern DL_IMPORT(void *) PyMem_Malloc(size_t);
extern DL_IMPORT(void *) PyMem_Realloc(void *, size_t);
extern DL_IMPORT(void) PyMem_Free(void *);

/* Starting from Python 1.6, the wrappers Py_{Malloc,Realloc,Free} are
   no longer supported. They used to call PyErr_NoMemory() on failure. */

/* Macros (override these if you want to a different malloc */
#ifndef PyMem_MALLOC
#define PyMem_MALLOC(n)         malloc(n)
#define PyMem_REALLOC(p, n)     realloc((void *)(p), (n))
#define PyMem_FREE(p)           free((void *)(p))
#endif

/*
 * Type-oriented memory interface
 * ==============================
 */

/* Functions */
#define PyMem_New(type, n) \
	( (type *) PyMem_Malloc((n) * sizeof(type)) )
#define PyMem_Resize(p, type, n) \
	( (p) = (type *) PyMem_Realloc((p), (n) * sizeof(type)) )
#define PyMem_Del(p) PyMem_Free(p)

/* Macros */
#define PyMem_NEW(type, n) \
	( (type *) PyMem_MALLOC(_PyMem_EXTRA + (n) * sizeof(type)) )

/* See comment near MALLOC_ZERO_RETURNS_NULL in pyport.h. */
#define PyMem_RESIZE(p, type, n)			\
	do {						\
		size_t _sum = (n) * sizeof(type);	\
		if (!_sum)				\
			_sum = 1;			\
		(p) = (type *)((p) ?			\
			       PyMem_REALLOC(p, _sum) :	\
			       PyMem_MALLOC(_sum));	\
	} while (0)

#define PyMem_DEL(p) PyMem_FREE(p)

/* PyMem_XDEL is deprecated. To avoid the call when p is NULL,
   it is recommended to write the test explicitly in the code.
   Note that according to ANSI C, free(NULL) has no effect. */


/* pymalloc (private to the interpreter) */
#ifdef WITH_PYMALLOC
DL_IMPORT(void *) _PyMalloc_Malloc(size_t nbytes);
DL_IMPORT(void *) _PyMalloc_Realloc(void *p, size_t nbytes);
DL_IMPORT(void) _PyMalloc_Free(void *p);

#ifdef PYMALLOC_DEBUG
DL_IMPORT(void *) _PyMalloc_DebugMalloc(size_t nbytes);
DL_IMPORT(void *) _PyMalloc_DebugRealloc(void *p, size_t nbytes);
DL_IMPORT(void) _PyMalloc_DebugFree(void *p);
DL_IMPORT(void) _PyMalloc_DebugDumpAddress(const void *p);
DL_IMPORT(void) _PyMalloc_DebugCheckAddress(const void *p);
#define _PyMalloc_MALLOC _PyMalloc_DebugMalloc
#define _PyMalloc_REALLOC _PyMalloc_DebugRealloc
#define _PyMalloc_FREE _PyMalloc_DebugFree

#else	/* WITH_PYMALLOC && ! PYMALLOC_DEBUG */
#define _PyMalloc_MALLOC _PyMalloc_Malloc
#define _PyMalloc_REALLOC _PyMalloc_Realloc
#define _PyMalloc_FREE _PyMalloc_Free
#endif

#else	/* ! WITH_PYMALLOC */
#define _PyMalloc_MALLOC PyMem_MALLOC
#define _PyMalloc_REALLOC PyMem_REALLOC
#define _PyMalloc_FREE PyMem_FREE
#endif	/* WITH_PYMALLOC */


#ifdef __cplusplus
}
#endif

#endif /* !Py_PYMEM_H */
