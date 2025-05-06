#ifndef Py_LONGOBJECT_H
#define Py_LONGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


/* Long (arbitrary precision) integer object interface */

// PyLong_Type is declared by object.h

#define PyLong_Check(op) \
        PyType_FastSubclass(Py_TYPE(op), Py_TPFLAGS_LONG_SUBCLASS)
#define PyLong_CheckExact(op) Py_IS_TYPE((op), &PyLong_Type)

PyAPI_FUNC(PyObject *) PyLong_FromLong(long);
PyAPI_FUNC(PyObject *) PyLong_FromUnsignedLong(unsigned long);
PyAPI_FUNC(PyObject *) PyLong_FromSize_t(size_t);
PyAPI_FUNC(PyObject *) PyLong_FromSsize_t(Py_ssize_t);
PyAPI_FUNC(PyObject *) PyLong_FromDouble(double);

PyAPI_FUNC(long) PyLong_AsLong(PyObject *);
PyAPI_FUNC(long) PyLong_AsLongAndOverflow(PyObject *, int *);
PyAPI_FUNC(Py_ssize_t) PyLong_AsSsize_t(PyObject *);
PyAPI_FUNC(size_t) PyLong_AsSize_t(PyObject *);
PyAPI_FUNC(unsigned long) PyLong_AsUnsignedLong(PyObject *);
PyAPI_FUNC(unsigned long) PyLong_AsUnsignedLongMask(PyObject *);

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000
PyAPI_FUNC(int) PyLong_AsInt(PyObject *);
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030e0000
PyAPI_FUNC(PyObject*) PyLong_FromInt32(int32_t value);
PyAPI_FUNC(PyObject*) PyLong_FromUInt32(uint32_t value);
PyAPI_FUNC(PyObject*) PyLong_FromInt64(int64_t value);
PyAPI_FUNC(PyObject*) PyLong_FromUInt64(uint64_t value);

PyAPI_FUNC(int) PyLong_AsInt32(PyObject *obj, int32_t *value);
PyAPI_FUNC(int) PyLong_AsUInt32(PyObject *obj, uint32_t *value);
PyAPI_FUNC(int) PyLong_AsInt64(PyObject *obj, int64_t *value);
PyAPI_FUNC(int) PyLong_AsUInt64(PyObject *obj, uint64_t *value);

#define Py_ASNATIVEBYTES_DEFAULTS -1
#define Py_ASNATIVEBYTES_BIG_ENDIAN 0
#define Py_ASNATIVEBYTES_LITTLE_ENDIAN 1
#define Py_ASNATIVEBYTES_NATIVE_ENDIAN 3
#define Py_ASNATIVEBYTES_UNSIGNED_BUFFER 4
#define Py_ASNATIVEBYTES_REJECT_NEGATIVE 8
#define Py_ASNATIVEBYTES_ALLOW_INDEX 16

/* PyLong_AsNativeBytes: Copy the integer value to a native variable.
   buffer points to the first byte of the variable.
   n_bytes is the number of bytes available in the buffer. Pass 0 to request
   the required size for the value.
   flags is a bitfield of the following flags:
   * 1 - little endian
   * 2 - native endian
   * 4 - unsigned destination (e.g. don't reject copying 255 into one byte)
   * 8 - raise an exception for negative inputs
   * 16 - call __index__ on non-int types
   If flags is -1 (all bits set), native endian is used, value truncation
   behaves most like C (allows negative inputs and allow MSB set), and non-int
   objects will raise a TypeError.
   Big endian mode will write the most significant byte into the address
   directly referenced by buffer; little endian will write the least significant
   byte into that address.

   If an exception is raised, returns a negative value.
   Otherwise, returns the number of bytes that are required to store the value.
   To check that the full value is represented, ensure that the return value is
   equal or less than n_bytes.
   All n_bytes are guaranteed to be written (unless an exception occurs), and
   so ignoring a positive return value is the equivalent of a downcast in C.
   In cases where the full value could not be represented, the returned value
   may be larger than necessary - this function is not an accurate way to
   calculate the bit length of an integer object.
   */
PyAPI_FUNC(Py_ssize_t) PyLong_AsNativeBytes(PyObject* v, void* buffer,
    Py_ssize_t n_bytes, int flags);

/* PyLong_FromNativeBytes: Create an int value from a native integer
   n_bytes is the number of bytes to read from the buffer. Passing 0 will
   always produce the zero int.
   PyLong_FromUnsignedNativeBytes always produces a non-negative int.
   flags is the same as for PyLong_AsNativeBytes, but only supports selecting
   the endianness or forcing an unsigned buffer.

   Returns the int object, or NULL with an exception set. */
PyAPI_FUNC(PyObject*) PyLong_FromNativeBytes(const void* buffer, size_t n_bytes,
    int flags);
PyAPI_FUNC(PyObject*) PyLong_FromUnsignedNativeBytes(const void* buffer,
    size_t n_bytes, int flags);

#endif

PyAPI_FUNC(PyObject *) PyLong_GetInfo(void);

/* It may be useful in the future. I've added it in the PyInt -> PyLong
   cleanup to keep the extra information. [CH] */
#define PyLong_AS_LONG(op) PyLong_AsLong(op)

/* Issue #1983: pid_t can be longer than a C long on some systems */
#if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
#define _Py_PARSE_PID "i"
#define PyLong_FromPid PyLong_FromLong
# if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000
#   define PyLong_AsPid PyLong_AsInt
# elif SIZEOF_INT == SIZEOF_LONG
#   define PyLong_AsPid PyLong_AsLong
# else
static inline int
PyLong_AsPid(PyObject *obj)
{
    int overflow;
    long result = PyLong_AsLongAndOverflow(obj, &overflow);
    if (overflow || result > INT_MAX || result < INT_MIN) {
        PyErr_SetString(PyExc_OverflowError,
                        "Python int too large to convert to C int");
        return -1;
    }
    return (int)result;
}
# endif
#elif SIZEOF_PID_T == SIZEOF_LONG
#define _Py_PARSE_PID "l"
#define PyLong_FromPid PyLong_FromLong
#define PyLong_AsPid PyLong_AsLong
#elif defined(SIZEOF_LONG_LONG) && SIZEOF_PID_T == SIZEOF_LONG_LONG
#define _Py_PARSE_PID "L"
#define PyLong_FromPid PyLong_FromLongLong
#define PyLong_AsPid PyLong_AsLongLong
#else
#error "sizeof(pid_t) is neither sizeof(int), sizeof(long) or sizeof(long long)"
#endif /* SIZEOF_PID_T */

#if SIZEOF_VOID_P == SIZEOF_INT
#  define _Py_PARSE_INTPTR "i"
#  define _Py_PARSE_UINTPTR "I"
#elif SIZEOF_VOID_P == SIZEOF_LONG
#  define _Py_PARSE_INTPTR "l"
#  define _Py_PARSE_UINTPTR "k"
#elif defined(SIZEOF_LONG_LONG) && SIZEOF_VOID_P == SIZEOF_LONG_LONG
#  define _Py_PARSE_INTPTR "L"
#  define _Py_PARSE_UINTPTR "K"
#else
#  error "void* different in size from int, long and long long"
#endif /* SIZEOF_VOID_P */

PyAPI_FUNC(double) PyLong_AsDouble(PyObject *);
PyAPI_FUNC(PyObject *) PyLong_FromVoidPtr(void *);
PyAPI_FUNC(void *) PyLong_AsVoidPtr(PyObject *);

PyAPI_FUNC(PyObject *) PyLong_FromLongLong(long long);
PyAPI_FUNC(PyObject *) PyLong_FromUnsignedLongLong(unsigned long long);
PyAPI_FUNC(long long) PyLong_AsLongLong(PyObject *);
PyAPI_FUNC(unsigned long long) PyLong_AsUnsignedLongLong(PyObject *);
PyAPI_FUNC(unsigned long long) PyLong_AsUnsignedLongLongMask(PyObject *);
PyAPI_FUNC(long long) PyLong_AsLongLongAndOverflow(PyObject *, int *);

PyAPI_FUNC(PyObject *) PyLong_FromString(const char *, char **, int);

/* These aren't really part of the int object, but they're handy. The
   functions are in Python/mystrtoul.c.
 */
PyAPI_FUNC(unsigned long) PyOS_strtoul(const char *, char **, int);
PyAPI_FUNC(long) PyOS_strtol(const char *, char **, int);

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_LONGOBJECT_H
#  include "cpython/longobject.h"
#  undef Py_CPYTHON_LONGOBJECT_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_LONGOBJECT_H */
