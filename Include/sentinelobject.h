/* Sentinel object interface */

#ifndef Py_SENTINELOBJECT_H
#define Py_SENTINELOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_LIMITED_API
PyAPI_DATA(PyTypeObject) PySentinel_Type;

#define PySentinel_Check(op) Py_IS_TYPE((op), &PySentinel_Type)

PyAPI_FUNC(PyObject *) PySentinel_New(
    const char *name,
    const char *module_name);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_SENTINELOBJECT_H */
