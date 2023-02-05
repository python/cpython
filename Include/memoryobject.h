/* Memory view object. In Python this is available as "memoryview". */

#ifndef Py_MEMORYOBJECT_H
#define Py_MEMORYOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyMemoryView_Type;

#define PyMemoryView_Check(op) Py_IS_TYPE((op), &PyMemoryView_Type)

PyAPI_FUNC(PyObject *) PyMemoryView_FromObject(PyObject *base);
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03030000
PyAPI_FUNC(PyObject *) PyMemoryView_FromMemory(char *mem, Py_ssize_t size,
                                               int flags);
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030b0000
PyAPI_FUNC(PyObject *) PyMemoryView_FromBuffer(const Py_buffer *info);
#endif
PyAPI_FUNC(PyObject *) PyMemoryView_GetContiguous(PyObject *base,
                                                  int buffertype,
                                                  char order);

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_MEMORYOBJECT_H
#  include "cpython/memoryobject.h"
#  undef Py_CPYTHON_MEMORYOBJECT_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_MEMORYOBJECT_H */
