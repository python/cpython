
/* Lowest-level memory allocation interface */

#ifndef Py_PYMEM_H
#define Py_PYMEM_H

#include "pyport.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Core memory allocator
 * =====================
 */

/* To make sure the interpreter is user-malloc friendly, all memory
   APIs are implemented on top of this one.

   The PyCore_* macros can be defined to make the interpreter use a
   custom allocator. Note that they are for internal use only. Both
   the core and extension modules should use the PyMem_* API. */

#ifndef PyCore_MALLOC
#undef PyCore_REALLOC
#undef PyCore_FREE
#define PyCore_MALLOC(n)      malloc(n)
#define PyCore_REALLOC(p, n)  realloc((p), (n))
#define PyCore_FREE(p)        free(p)
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

/* Functions */

/* Function wrappers around PyCore_MALLOC and friends; useful if you
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

/* Macros */
#define PyMem_MALLOC(n)         PyCore_MALLOC(n)
#define PyMem_REALLOC(p, n)     PyCore_REALLOC((void *)(p), (n))
#define PyMem_FREE(p)           PyCore_FREE((void *)(p))

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

#ifdef __cplusplus
}
#endif

#endif /* !Py_PYMEM_H */
