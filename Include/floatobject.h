
/* Float object interface */

/*
PyFloatObject represents a (double precision) floating point number.
*/

#ifndef Py_FLOATOBJECT_H
#define Py_FLOATOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_HEAD
    double ob_fval;
} PyFloatObject;

extern DL_IMPORT(PyTypeObject) PyFloat_Type;

#define PyFloat_Check(op) ((op)->ob_type == &PyFloat_Type)

extern DL_IMPORT(PyObject *) PyFloat_FromString(PyObject*, char**);
extern DL_IMPORT(PyObject *) PyFloat_FromDouble(double);
extern DL_IMPORT(double) PyFloat_AsDouble(PyObject *);

/* Macro, trading safety for speed */
#define PyFloat_AS_DOUBLE(op) (((PyFloatObject *)(op))->ob_fval)

#ifdef __cplusplus
}
#endif
#endif /* !Py_FLOATOBJECT_H */
