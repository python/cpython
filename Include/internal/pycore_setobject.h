#ifndef Py_INTERNAL_SETOBJECT_H
#define Py_INTERNAL_SETOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Export for 'pickle' shared extension
PyAPI_FUNC(int) _PySet_NextEntry(
    PyObject *set,
    Py_ssize_t *pos,
    PyObject **key,
    Py_hash_t *hash);

// Export for 'pickle' shared extension
PyAPI_FUNC(int) _PySet_Update(PyObject *set, PyObject *iterable);

// Export _PySet_Dummy for the gdb plugin's benefit
PyAPI_DATA(PyObject *) _PySet_Dummy;

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_SETOBJECT_H
