/* The PyMem_ family:  low-level memory allocation interfaces.
   See objimpl.h for the PyObject_ memory family.
*/

#ifndef Py_PYMEM_H
#define Py_PYMEM_H

#include "pyport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BEWARE:

   Each interface exports both functions and macros.  Extension modules should
   use the functions, to ensure binary compatibility across Python versions.
   Because the Python implementation is free to change internal details, and
   the macros may (or may not) expose details for speed, if you do use the
   macros you must recompile your extensions with each Python release.

   Never mix calls to PyMem_ with calls to the platform malloc/realloc/
   calloc/free.  For example, on Windows different DLLs may end up using
   different heaps, and if you use PyMem_Malloc you'll get the memory from the
   heap used by the Python DLL; it could be a disaster if you free()'ed that
   directly in your own extension.  Using PyMem_Free instead ensures Python
   can return the memory to the proper heap.  As another example, in
   PYMALLOC_DEBUG mode, Python wraps all calls to all PyMem_ and PyObject_
   memory functions in special debugging wrappers that add additional
   debugging info to dynamic memory blocks.  The system routines have no idea
   what to do with that stuff, and the Python wrappers have no idea what to do
   with raw blocks obtained directly by the system routines then.
*/

/*
 * Raw memory interface
 * ====================
 */

/* Functions

   Functions supplying platform-independent semantics for malloc/realloc/
   free.  These functions make sure that allocating 0 bytes returns a distinct
   non-NULL pointer (whenever possible -- if we're flat out of memory, NULL
   may be returned), even if the platform malloc and realloc don't.
   Returned pointers must be checked for NULL explicitly.  No action is
   performed on failure (no exception is set, no warning is printed, etc).
*/

PyAPI_FUNC(void *) PyMem_Malloc(size_t);
PyAPI_FUNC(void *) PyMem_Realloc(void *, size_t);
PyAPI_FUNC(void) PyMem_Free(void *);

/* Starting from Python 1.6, the wrappers Py_{Malloc,Realloc,Free} are
   no longer supported. They used to call PyErr_NoMemory() on failure. */

/* Macros. */
#ifdef PYMALLOC_DEBUG
/* Redirect all memory operations to Python's debugging allocator. */
#define PyMem_MALLOC		PyObject_MALLOC
#define PyMem_REALLOC		PyObject_REALLOC

#else	/* ! PYMALLOC_DEBUG */

/* PyMem_MALLOC(0) means malloc(1). Some systems would return NULL
   for malloc(0), which would be treated as an error. Some platforms
   would return a pointer with no memory behind it, which would break
   pymalloc. To solve these problems, allocate an extra byte. */
#define PyMem_MALLOC(n)         malloc((n) ? (n) : 1)
#define PyMem_REALLOC(p, n)     realloc((p), (n) ? (n) : 1)

#endif	/* PYMALLOC_DEBUG */

/* In order to avoid breaking old code mixing PyObject_{New, NEW} with
   PyMem_{Del, DEL} and PyMem_{Free, FREE}, the PyMem "release memory"
   functions have to be redirected to the object deallocator. */
#define PyMem_FREE           	PyObject_FREE

/*
 * Type-oriented memory interface
 * ==============================
 *
 * These are carried along for historical reasons.  There's rarely a good
 * reason to use them anymore (you can just as easily do the multiply and
 * cast yourself).
 */

#define PyMem_New(type, n) \
	( (type *) PyMem_Malloc((n) * sizeof(type)) )
#define PyMem_NEW(type, n) \
	( (type *) PyMem_MALLOC((n) * sizeof(type)) )

#define PyMem_Resize(p, type, n) \
	( (p) = (type *) PyMem_Realloc((p), (n) * sizeof(type)) )
#define PyMem_RESIZE(p, type, n) \
	( (p) = (type *) PyMem_REALLOC((p), (n) * sizeof(type)) )

/* In order to avoid breaking old code mixing PyObject_{New, NEW} with
   PyMem_{Del, DEL} and PyMem_{Free, FREE}, the PyMem "release memory"
   functions have to be redirected to the object deallocator. */
#define PyMem_Del		PyObject_Free
#define PyMem_DEL		PyObject_FREE

#ifdef __cplusplus
}
#endif

#endif /* !Py_PYMEM_H */
