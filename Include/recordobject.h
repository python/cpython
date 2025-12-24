/* Record object interface */

#ifndef Py_RECORDOBJECT_H
#define Py_RECORDOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyRecord_Type;
PyAPI_FUNC(PyObject *) PyRecord_New(Py_ssize_t size);
#define PyRecord_Check(op) Py_IS_TYPE(op, &PyRecord_Type)

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_RECORDOBJECT_H
#  include  "cpython/recordobject.h"
#  undef Py_CPYTHON_RECORDOBJECT_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_RECORDOBJECT_H */
