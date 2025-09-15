#ifndef Py_INTERNAL_SETOBJECT_H
#define Py_INTERNAL_SETOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Export for '_abc' shared extension
PyAPI_FUNC(int) _PySet_NextEntry(
    PyObject *set,
    Py_ssize_t *pos,
    PyObject **key,
    Py_hash_t *hash);

// Export for '_pickle' shared extension
PyAPI_FUNC(int) _PySet_NextEntryRef(
    PyObject *set,
    Py_ssize_t *pos,
    PyObject **key,
    Py_hash_t *hash);

// Export for '_pickle' shared extension
PyAPI_FUNC(int) _PySet_Update(PyObject *set, PyObject *iterable);

// Export for the gdb plugin's (python-gdb.py) benefit
PyAPI_DATA(PyObject *) _PySet_Dummy;

PyAPI_FUNC(int) _PySet_Contains(PySetObject *so, PyObject *key);

// Clears the set without acquiring locks. Used by _PyCode_Fini.
extern void _PySet_ClearInternal(PySetObject *so);

PyAPI_FUNC(int) _PySet_AddTakeRef(PySetObject *so, PyObject *key);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_SETOBJECT_H
