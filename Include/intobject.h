
/* Integer object interface */

/*
PyIntObject represents a (long) integer.  This is an immutable object;
an integer cannot change its value after creation.

There are functions to create new integer objects, to test an object
for integer-ness, and to get the integer value.  The latter functions
returns -1 and sets errno to EBADF if the object is not an PyIntObject.
None of the functions should be applied to nil objects.

The type PyIntObject is (unfortunately) exposed here so we can declare
_Py_TrueStruct and _Py_ZeroStruct below; don't use this.
*/

#ifndef Py_INTOBJECT_H
#define Py_INTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_HEAD
    long ob_ival;
} PyIntObject;

extern DL_IMPORT(PyTypeObject) PyInt_Type;

#define PyInt_Check(op) PyObject_TypeCheck(op, &PyInt_Type)
#define PyInt_CheckExact(op) ((op)->ob_type == &PyInt_Type)

extern DL_IMPORT(PyObject *) PyInt_FromString(char*, char**, int);
#ifdef Py_USING_UNICODE
extern DL_IMPORT(PyObject *) PyInt_FromUnicode(Py_UNICODE*, int, int);
#endif
extern DL_IMPORT(PyObject *) PyInt_FromLong(long);
extern DL_IMPORT(long) PyInt_AsLong(PyObject *);
extern DL_IMPORT(long) PyInt_GetMax(void);

/* Macro, trading safety for speed */
#define PyInt_AS_LONG(op) (((PyIntObject *)(op))->ob_ival)

/* These aren't really part of the Int object, but they're handy; the protos
 * are necessary for systems that need the magic of DL_IMPORT and that want
 * to have stropmodule as a dynamically loaded module instead of building it
 * into the main Python shared library/DLL.  Guido thinks I'm weird for
 * building it this way.  :-)  [cjh]
 */
extern DL_IMPORT(unsigned long) PyOS_strtoul(char *, char **, int);
extern DL_IMPORT(long) PyOS_strtol(char *, char **, int);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTOBJECT_H */
