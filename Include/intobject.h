
/* Integer object interface */

/*
PyIntObject represents a (long) integer.  This is an immutable object;
an integer cannot change its value after creation.

There are functions to create new integer objects, to test an object
for integer-ness, and to get the integer value.  The latter functions
returns -1 and sets errno to EBADF if the object is not an PyIntObject.
None of the functions should be applied to nil objects.

The type PyIntObject is (unfortunately) exposed here so we can declare
_Py_TrueStruct and _Py_ZeroStruct in boolobject.h; don't use this.
*/

#ifndef Py_INTOBJECT_H
#define Py_INTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/*
typedef struct {
    PyObject_HEAD
    long ob_ival;
} PyIntObject;

PyAPI_DATA(PyTypeObject) PyInt_Type;
*/

#define PyInt_CheckExact(op) (PyLong_CheckExact(op) && _PyLong_FitsInLong(op))

#ifdef 0
#    define PyInt_Check(op) PyLong_Check(op)
#    define PyInt_FromString PyLong_FromString
#    define PyInt_FromUnicode PyLong_FromUnicode
#    define PyInt_FromLong PyLong_FromLong
#    define PyInt_FromSize_t PyLong_FromSize_t
#    define PyInt_FromSsize_t PyLong_FromSsize_t
#    define PyInt_AsLong PyLong_AsLong
#    define PyInt_AsSsize_t PyLong_AsSsize_t
#    define PyInt_AsUnsignedLongMask PyLong_AsUnsignedLongMask
#    define PyInt_AsUnsignedLongLongMask PyLong_AsUnsignedLongLongMask
#    define PyInt_AS_LONG PyLong_AS_LONG
#endif

PyAPI_FUNC(long) PyInt_GetMax(void);

/* These aren't really part of the Int object, but they're handy; the protos
 * are necessary for systems that need the magic of PyAPI_FUNC.
 */
PyAPI_FUNC(unsigned long) PyOS_strtoul(char *, char **, int);
PyAPI_FUNC(long) PyOS_strtol(char *, char **, int);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTOBJECT_H */
