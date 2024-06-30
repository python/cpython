/* Generator object interface */

#ifndef Py_LIMITED_API
#ifndef Py_GENOBJECT_H
#define Py_GENOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/* --- Generators --------------------------------------------------------- */

typedef struct _PyGenObject PyGenObject;

PyAPI_DATA(PyTypeObject) PyGen_Type;

#define PyGen_Check(op) PyObject_TypeCheck((op), &PyGen_Type)
#define PyGen_CheckExact(op) Py_IS_TYPE((op), &PyGen_Type)

PyAPI_FUNC(PyObject *) PyGen_New(PyFrameObject *);
PyAPI_FUNC(PyObject *) PyGen_NewWithQualName(PyFrameObject *,
    PyObject *name, PyObject *qualname);
PyAPI_FUNC(PyCodeObject *) PyGen_GetCode(PyGenObject *gen);


/* --- PyCoroObject ------------------------------------------------------- */

typedef struct _PyCoroObject PyCoroObject;

PyAPI_DATA(PyTypeObject) PyCoro_Type;

#define PyCoro_CheckExact(op) Py_IS_TYPE((op), &PyCoro_Type)
PyAPI_FUNC(PyObject *) PyCoro_New(PyFrameObject *,
    PyObject *name, PyObject *qualname);


/* --- Asynchronous Generators -------------------------------------------- */

typedef struct _PyAsyncGenObject PyAsyncGenObject;

PyAPI_DATA(PyTypeObject) PyAsyncGen_Type;
PyAPI_DATA(PyTypeObject) _PyAsyncGenASend_Type;

PyAPI_FUNC(PyObject *) PyAsyncGen_New(PyFrameObject *,
    PyObject *name, PyObject *qualname);

#define PyAsyncGen_CheckExact(op) Py_IS_TYPE((op), &PyAsyncGen_Type)

#define PyAsyncGenASend_CheckExact(op) Py_IS_TYPE((op), &_PyAsyncGenASend_Type)

#undef _PyGenObject_HEAD

#ifdef __cplusplus
}
#endif
#endif /* !Py_GENOBJECT_H */
#endif /* Py_LIMITED_API */
