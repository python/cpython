
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

#define PyInt_Check(op) ((op)->ob_type == &PyInt_Type)

extern DL_IMPORT(PyObject *) PyInt_FromString(char*, char**, int);
extern DL_IMPORT(PyObject *) PyInt_FromUnicode(Py_UNICODE*, int, int);
extern DL_IMPORT(PyObject *) PyInt_FromLong(long);
extern DL_IMPORT(long) PyInt_AsLong(PyObject *);
extern DL_IMPORT(long) PyInt_GetMax(void);


/*
False and True are special intobjects used by Boolean expressions.
All values of type Boolean must point to either of these; but in
contexts where integers are required they are integers (valued 0 and 1).
Hope these macros don't conflict with other people's.

Don't forget to apply Py_INCREF() when returning True or False!!!
*/

extern DL_IMPORT(PyIntObject) _Py_ZeroStruct, _Py_TrueStruct; /* Don't use these directly */

#define Py_False ((PyObject *) &_Py_ZeroStruct)
#define Py_True ((PyObject *) &_Py_TrueStruct)

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
