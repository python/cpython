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

  /* This routines isn't a proper arena allocation routine.  It takes
     a PyObject* and records it so that it can be DECREFed when the
     arena is freed.
   */
  PyAPI_FUNC(int) PyArena_AddPyObject(PyArena *, PyObject *);

#ifdef __cplusplus
}
#endif

#endif /* !Py_PYARENA_H */
