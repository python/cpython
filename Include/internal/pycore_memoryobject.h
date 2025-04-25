#ifndef Py_INTERNAL_MEMORYOBJECT_H
#define Py_INTERNAL_MEMORYOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

struct _memoryobject_state {
    PyTypeObject *XIBufferViewType;
};

extern PyStatus _PyMemoryView_InitTypes(PyInterpreterState *);
extern void _PyMemoryView_FiniTypes(PyInterpreterState *);

// exported for _interpreters module
PyAPI_FUNC(PyTypeObject *) _PyMemoryView_GetXIBuffewViewType(void);


extern PyTypeObject _PyManagedBuffer_Type;

PyObject *
_PyMemoryView_FromBufferProc(PyObject *v, int flags,
                             getbufferproc bufferproc);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_MEMORYOBJECT_H */
