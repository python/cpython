
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

/* Return Python float from string PyObject.  Second argument ignored on
   input, and, if non-NULL, NULL is stored into *junk (this tried to serve a
   purpose once but can't be made to work as intended). */
extern DL_IMPORT(PyObject *) PyFloat_FromString(PyObject*, char** junk);

/* Return Python float from C double. */
extern DL_IMPORT(PyObject *) PyFloat_FromDouble(double);

/* Extract C double from Python float.  The macro version trades safety for
   speed. */
extern DL_IMPORT(double) PyFloat_AsDouble(PyObject *);
#define PyFloat_AS_DOUBLE(op) (((PyFloatObject *)(op))->ob_fval)

/* Write repr(v) into the char buffer argument, followed by null byte.  The
   buffer must be "big enough"; >= 100 is very safe.
   PyFloat_AsReprString(buf, x) strives to print enough digits so that
   PyFloat_FromString(buf) then reproduces x exactly. */
extern DL_IMPORT(void) PyFloat_AsReprString(char*, PyFloatObject *v);

/* Write str(v) into the char buffer argument, followed by null byte.  The
   buffer must be "big enough"; >= 100 is very safe.  Note that it's
   unusual to be able to get back the float you started with from
   PyFloat_AsString's result -- use PyFloat_AsReprString() if you want to
   preserve precision across conversions. */
extern DL_IMPORT(void) PyFloat_AsString(char*, PyFloatObject *v);

#ifdef __cplusplus
}
#endif
#endif /* !Py_FLOATOBJECT_H */
