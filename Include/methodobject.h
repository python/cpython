
/* Method object interface */

#ifndef Py_METHODOBJECT_H
#define Py_METHODOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyCFunction_Type;

#define PyCFunction_Check(op) ((op)->ob_type == &PyCFunction_Type)

typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *,
					     PyObject *);
typedef PyObject *(*PyNoArgsFunction)(PyObject *);

PyAPI_FUNC(PyCFunction) PyCFunction_GetFunction(PyObject *);
PyAPI_FUNC(PyObject *) PyCFunction_GetSelf(PyObject *);
PyAPI_FUNC(int) PyCFunction_GetFlags(PyObject *);

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#define PyCFunction_GET_FUNCTION(func) \
        (((PyCFunctionObject *)func) -> m_ml -> ml_meth)
#define PyCFunction_GET_SELF(func) \
	(((PyCFunctionObject *)func) -> m_self)
#define PyCFunction_GET_FLAGS(func) \
	(((PyCFunctionObject *)func) -> m_ml -> ml_flags)
PyAPI_FUNC(PyObject *) PyCFunction_Call(PyObject *, PyObject *, PyObject *);

struct PyMethodDef {
    char	*ml_name;
    PyCFunction  ml_meth;
    int		 ml_flags;
    char	*ml_doc;
};
typedef struct PyMethodDef PyMethodDef;

PyAPI_FUNC(PyObject *) Py_FindMethod(PyMethodDef[], PyObject *, char *);

#define PyCFunction_New(ML, SELF) PyCFunction_NewEx((ML), (SELF), NULL)
PyAPI_FUNC(PyObject *) PyCFunction_NewEx(PyMethodDef *, PyObject *, 
					 PyObject *);

/* Flag passed to newmethodobject */
#define METH_OLDARGS  0x0000
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

typedef struct PyMethodChain {
    PyMethodDef *methods;		/* Methods of this type */
    struct PyMethodChain *link;	/* NULL or base type */
} PyMethodChain;

PyAPI_FUNC(PyObject *) Py_FindMethodInChain(PyMethodChain *, PyObject *,
                                                  char *);

typedef struct {
    PyObject_HEAD
    PyMethodDef *m_ml;
    PyObject    *m_self;
    PyObject    *m_module;
} PyCFunctionObject;

#ifdef __cplusplus
}
#endif
#endif /* !Py_METHODOBJECT_H */
