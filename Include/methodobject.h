
/* Method object interface */

#ifndef Py_METHODOBJECT_H
#define Py_METHODOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


/* This is about the classes 'builtin_function_or_method'
   and 'method_descriptor', not Python methods in user-defined
   classes.  See classobject.h for the latter. */

PyAPI_DATA(PyTypeObject) PyCFunction_Type;
PyAPI_DATA(PyTypeObject) PyMethodDescr_Type;

#define PyCFunction_Check(op) (Py_TYPE(op) == &PyCFunction_Type)

typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
typedef PyObject *(*_PyCFunctionFast) (PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *,
                                             PyObject *);
typedef PyObject *(*_PyCFunctionFastWithKeywords) (PyObject *,
                                                   PyObject *const *, Py_ssize_t,
                                                   PyObject *);
typedef PyObject *(*PyNoArgsFunction)(PyObject *);

PyAPI_FUNC(PyCFunction) PyCFunction_GetFunction(PyObject *);
PyAPI_FUNC(PyObject *) PyCFunction_GetSelf(PyObject *);
PyAPI_FUNC(int) PyCFunction_GetFlags(PyObject *);  /* deprecated and useless */

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#ifndef Py_LIMITED_API
#define PyCFunction_GET_FUNCTION(func) ( \
        (PyCFunction)((PyCFunctionObject *)func)->m_ccall->cc_func)
#define PyCFunction_GET_SELF(func) ( \
        (((PyCFunctionObject *)func)->m_self == Py_None) ? \
        NULL : ((PyCFunctionObject *)func)->m_self)
#define PyCFunction_GET_FLAGS PyCFunction_GetFlags
#endif
PyAPI_FUNC(PyObject *) PyCFunction_Call(PyObject *, PyObject *, PyObject *);

struct PyMethodDef {
    const char  *ml_name;   /* The name of the built-in function/method */
    PyCFunction ml_meth;    /* The C function that implements it */
    int         ml_flags;   /* Combination of METH_xxx flags, which mostly
                               describe the args expected by the C func */
    const char  *ml_doc;    /* The __doc__ attribute, or NULL */
};
typedef struct PyMethodDef PyMethodDef;

#define PyCFunction_New(ML, SELF) PyCFunction_NewEx((ML), (SELF), NULL)
PyAPI_FUNC(PyObject *) PyCFunction_NewEx(PyMethodDef *, PyObject *,
                                         PyObject *);

/* Flag passed to newmethodobject */
/* #define METH_OLDARGS  0x0000   -- unsupported now */
#define METH_VARARGS  0x0001
#define METH_KEYWORDS 0x0002
/* METH_NOARGS and METH_O must not be combined with the flags above. */
#define METH_NOARGS   0x0004
#define METH_O        0x0008

/* METH_CLASS and METH_STATIC are a little different; these control
   the construction of methods for a class.  These cannot be used for
   functions in modules. */
#define METH_CLASS    0x0010
#define METH_STATIC   0x0020

/* METH_COEXIST allows a method to be entered even though a slot has
   already filled the entry.  When defined, the flag allows a separate
   method, "__contains__" for example, to coexist with a defined
   slot like sq_contains. */

#define METH_COEXIST   0x0040

#ifndef Py_LIMITED_API
#define METH_FASTCALL  0x0080

/* All flags influencing the signature of the C function */
#define METH_SIGNATURE (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O | METH_KEYWORDS)
#endif

/* This bit is preserved for Stackless Python */
#ifdef STACKLESS
#define METH_STACKLESS 0x0100
#else
#define METH_STACKLESS 0x0000
#endif

PyAPI_FUNC(PyObject *) PyDescr_NewMethod(PyTypeObject *, PyMethodDef *);

PyAPI_FUNC(PyObject *) PyCFunction_ClsNew(
    PyTypeObject *cls,
    const PyMethodDef *ml,
    PyObject *self,
    PyObject *module,
    PyObject *parent);


#ifndef Py_LIMITED_API
#  define Py_CPYTHON_CCALL_H
#  include  "cpython/ccall.h"
#  undef Py_CPYTHON_CCALL_H


typedef struct {
    PyObject_HEAD
    PyCCallDef  *m_ccall;
    PyObject    *m_self;         /* Passed as 'self' arg to the C function */
    PyCCallDef   _ccalldef;      /* Storage for m_ccall */
    PyObject    *m_name;         /* __name__; str object (not NULL) */
    PyObject    *m_module;       /* __module__; can be anything */
    const char  *m_doc;          /* __doc__ and __text_signature__ */
    PyObject    *m_weakreflist;  /* List of weak references */
} PyCFunctionObject;

PyAPI_FUNC(PyObject *) _PyCFunction_NewBoundMethod(PyCFunctionObject *func, PyObject *self);
#endif

PyAPI_FUNC(int) PyCFunction_ClearFreeList(void);

#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _PyCFunction_DebugMallocStats(FILE *out);
PyAPI_FUNC(void) _PyMethod_DebugMallocStats(FILE *out);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_METHODOBJECT_H */
