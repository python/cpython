/* Sentinel object interface */

#ifndef Py_LIMITED_API
#ifndef _Py_SENTINELOBJECT_H
#define _Py_SENTINELOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PySentinel_Type;

#define PySentinel_CheckExact(op) Py_IS_TYPE((op), &PySentinel_Type)

/* Alias as long as subclasses are not allowed. */
#define PySentinel_Check(op) PySentinel_CheckExact(op)

PyAPI_FUNC(PyObject *) PySentinel_New(
    const char *name,
    const char *module_name);

#ifdef __cplusplus
}
#endif
#endif /* !_Py_SENTINELOBJECT_H */
#endif /* !Py_LIMITED_API */
