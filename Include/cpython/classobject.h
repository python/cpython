/* Former class object interface -- now only bound methods are here  */

/* Revealing some structures (not for general use) */

#ifndef Py_LIMITED_API
#ifndef Py_CLASSOBJECT_H
#define Py_CLASSOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_HEAD
    PyObject *im_func;   /* The callable object implementing the method */
    PyObject *im_self;   /* The instance it is bound to */
    PyObject *im_weakreflist; /* List of weak references */
    vectorcallfunc vectorcall;
} PyMethodObject;

PyAPI_DATA(PyTypeObject) PyMethod_Type;

#define PyMethod_Check(op) Py_IS_TYPE((op), &PyMethod_Type)

PyAPI_FUNC(PyObject *) PyMethod_New(PyObject *, PyObject *);

PyAPI_FUNC(PyObject *) PyMethod_Function(PyObject *);
PyAPI_FUNC(PyObject *) PyMethod_Self(PyObject *);

#define _PyMethod_CAST(meth) \
    (assert(PyMethod_Check(meth)), _Py_CAST(PyMethodObject*, meth))

/* Static inline functions for direct access to these values.
   Type checks are *not* done, so use with care. */
static inline PyObject* PyMethod_GET_FUNCTION(PyObject *meth) {
    return _PyMethod_CAST(meth)->im_func;
}
#define PyMethod_GET_FUNCTION(meth) PyMethod_GET_FUNCTION(_PyObject_CAST(meth))

static inline PyObject* PyMethod_GET_SELF(PyObject *meth) {
    return _PyMethod_CAST(meth)->im_self;
}
#define PyMethod_GET_SELF(meth) PyMethod_GET_SELF(_PyObject_CAST(meth))

typedef struct {
    PyObject_HEAD
    PyObject *func;
} PyInstanceMethodObject;

PyAPI_DATA(PyTypeObject) PyInstanceMethod_Type;

#define PyInstanceMethod_Check(op) Py_IS_TYPE((op), &PyInstanceMethod_Type)

PyAPI_FUNC(PyObject *) PyInstanceMethod_New(PyObject *);
PyAPI_FUNC(PyObject *) PyInstanceMethod_Function(PyObject *);

#define _PyInstanceMethod_CAST(meth) \
    (assert(PyInstanceMethod_Check(meth)), \
     _Py_CAST(PyInstanceMethodObject*, meth))

/* Static inline function for direct access to these values.
   Type checks are *not* done, so use with care. */
static inline PyObject* PyInstanceMethod_GET_FUNCTION(PyObject *meth) {
    return _PyInstanceMethod_CAST(meth)->func;
}
#define PyInstanceMethod_GET_FUNCTION(meth) PyInstanceMethod_GET_FUNCTION(_PyObject_CAST(meth))

#ifdef __cplusplus
}
#endif
#endif   // !Py_CLASSOBJECT_H
#endif   // !Py_LIMITED_API
