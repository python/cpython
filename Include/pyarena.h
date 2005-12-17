/* An arena-like memory interface for the compiler.
 */

#ifndef Py_PYARENA_H
#define Py_PYARENA_H

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct _arena PyArena;

  /* PyArena_New() and PyArena_Free() create a new arena and free it,
     respectively.  Once an arena has been created, it can be used
     to allocate memory.  Once it is freed, all the memory it allocated
     is freed and none of its pointers are valid.

     PyArena_New() returns an arena pointer.  On error, it
     returns a negative number and sets an exception.
  */
  PyAPI_FUNC(PyArena *) PyArena_New(void);
  PyAPI_FUNC(void) PyArena_Free(PyArena *);

  PyAPI_FUNC(void *) PyArena_Malloc(PyArena *, size_t);

  /* The next two routines aren't proper arena allocation routines.
     They exist to experiment with the arena API without making wholesale
     changes to the implementation.

     The two functions register pointers with the arena id.  These
     are externally allocated pointers that will be freed when the
     arena is freed.  One takes a pointer allocated with malloc.  The
     other takes a PyObject that is DECREFed when the arena is freed.
  */
  PyAPI_FUNC(int) PyArena_AddMallocPointer(PyArena *, void *);
  PyAPI_FUNC(int) PyArena_AddPyObject(PyArena *, PyObject *);

#ifdef __cplusplus
}
#endif

#endif /* !Py_PYARENA_H */
