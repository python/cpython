
/* Class object interface */

/* Revealing some structures (not for general use) */

#ifndef Py_CLASSOBJECT_H
#define Py_CLASSOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_HEAD
    PyObject	*cl_bases;	/* A tuple of class objects */
    PyObject	*cl_dict;	/* A dictionary */
    PyObject	*cl_name;	/* A string */
    /* The following three are functions or NULL */
    PyObject	*cl_getattr;
    PyObject	*cl_setattr;
    PyObject	*cl_delattr;
} PyClassObject;

typedef struct {
    PyObject_HEAD
    PyClassObject *in_class;	/* The class object */
    PyObject	  *in_dict;	/* A dictionary */
} PyInstanceObject;

typedef struct {
    PyObject_HEAD
    PyObject *im_func;   /* The callable object implementing the method */
    PyObject *im_self;   /* The instance it is bound to, or NULL */
    PyObject *im_class;  /* The class that defined the method */
} PyMethodObject;

extern DL_IMPORT(PyTypeObject) PyClass_Type, PyInstance_Type, PyMethod_Type;

#define PyClass_Check(op) ((op)->ob_type == &PyClass_Type)
#define PyInstance_Check(op) ((op)->ob_type == &PyInstance_Type)
#define PyMethod_Check(op) ((op)->ob_type == &PyMethod_Type)

extern DL_IMPORT(PyObject *) PyClass_New(PyObject *, PyObject *, PyObject *);
extern DL_IMPORT(PyObject *) PyInstance_New(PyObject *, PyObject *,
                                            PyObject *);
extern DL_IMPORT(PyObject *) PyMethod_New(PyObject *, PyObject *, PyObject *);

extern DL_IMPORT(PyObject *) PyMethod_Function(PyObject *);
extern DL_IMPORT(PyObject *) PyMethod_Self(PyObject *);
extern DL_IMPORT(PyObject *) PyMethod_Class(PyObject *);

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#define PyMethod_GET_FUNCTION(meth) \
        (((PyMethodObject *)meth) -> im_func)
#define PyMethod_GET_SELF(meth) \
	(((PyMethodObject *)meth) -> im_self)
#define PyMethod_GET_CLASS(meth) \
	(((PyMethodObject *)meth) -> im_class)

extern DL_IMPORT(int) PyClass_IsSubclass(PyObject *, PyObject *);

extern DL_IMPORT(PyObject *) PyInstance_DoBinOp(PyObject *, PyObject *,
                                                char *, char *,
                                                PyObject * (*)(PyObject *,
                                                               PyObject *));

extern DL_IMPORT(int)
PyInstance_HalfBinOp(PyObject *, PyObject *, char *, PyObject **,
			PyObject * (*)(PyObject *, PyObject *), int);

#ifdef __cplusplus
}
#endif
#endif /* !Py_CLASSOBJECT_H */
