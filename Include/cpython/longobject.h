#ifndef Py_CPYTHON_LONGOBJECT_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(int) _PyLong_UnsignedShort_Converter(PyObject *, void *);
PyAPI_FUNC(int) _PyLong_UnsignedInt_Converter(PyObject *, void *);
PyAPI_FUNC(int) _PyLong_UnsignedLong_Converter(PyObject *, void *);
PyAPI_FUNC(int) _PyLong_UnsignedLongLong_Converter(PyObject *, void *);
PyAPI_FUNC(int) _PyLong_Size_t_Converter(PyObject *, void *);

PyAPI_FUNC(PyObject*) PyLong_FromUnicodeObject(PyObject *u, int base);

/* _PyLong_Sign.  Return 0 if v is 0, -1 if v < 0, +1 if v > 0.
   v must not be NULL, and must be a normalized long.
   There are no error cases.
*/
PyAPI_FUNC(int) _PyLong_Sign(PyObject *v);

/* _PyLong_NumBits.  Return the number of bits needed to represent the
   absolute value of a long.  For example, this returns 1 for 1 and -1, 2
   for 2 and -2, and 2 for 3 and -3.  It returns 0 for 0.
   v must not be NULL, and must be a normalized long.
   (size_t)-1 is returned and OverflowError set if the true result doesn't
   fit in a size_t.
*/
PyAPI_FUNC(size_t) _PyLong_NumBits(PyObject *v);

PyAPI_FUNC(int) PyUnstable_Long_IsCompact(const PyLongObject* op);
PyAPI_FUNC(Py_ssize_t) PyUnstable_Long_CompactValue(const PyLongObject* op);

