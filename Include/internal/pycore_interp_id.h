/* Interpreter ID Object */

#ifndef Py_INTERNAL_INTERPRETERIDOBJECT_H
#define Py_INTERNAL_INTERPRETERIDOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Export for '_xxsubinterpreters' shared extension
PyAPI_DATA(PyTypeObject) _PyInterpreterID_Type;

// Export for '_xxsubinterpreters' shared extension
PyAPI_FUNC(PyObject*) _PyInterpreterID_New(int64_t);

// Export for '_xxinterpchannels' shared extension
PyAPI_FUNC(PyObject*) _PyInterpreterState_GetIDObject(PyInterpreterState *);

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(PyInterpreterState*) _PyInterpreterID_LookUp(PyObject *);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_INTERPRETERIDOBJECT_H
