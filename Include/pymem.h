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

extern DL_IMPORT(void *) PyMem_Malloc(size_t);
extern DL_IMPORT(void *) PyMem_Realloc(void *, size_t);
extern DL_IMPORT(void) PyMem_Free(void *);

/* Starting from Python 1.6, the wrappers Py_{Malloc,Realloc,Free} are
   no longer supported. They used to call PyErr_NoMemory() on failure. */

/* Macros. */
#ifdef PYMALLOC_DEBUG
/* Redirect all memory operations to Python's debugging allocator. */
#define PyMem_MALLOC		PyObject_MALLOC
#define PyMem_REALLOC		PyObject_REALLOC

#else	/* ! PYMALLOC_DEBUG */

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
