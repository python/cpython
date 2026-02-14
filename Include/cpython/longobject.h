#ifndef Py_CPYTHON_LONGOBJECT_H
#  error "this header file must not be included directly"
#endif

#define _PyLong_CAST(op) \
    (assert(PyLong_Check(op)), _Py_CAST(PyLongObject*, (op)))

PyAPI_FUNC(PyObject*) PyLong_FromUnicodeObject(PyObject *u, int base);

PyAPI_FUNC(int) PyUnstable_Long_IsCompact(const PyLongObject* op);
PyAPI_FUNC(Py_ssize_t) PyUnstable_Long_CompactValue(const PyLongObject* op);

/* PyLong_IsPositive.  Check if the integer object is positive.

   - On success, return 1 if *obj is positive, and 0 otherwise.
   - On failure, set an exception, and return -1. */
PyAPI_FUNC(int) PyLong_IsPositive(PyObject *obj);

/* PyLong_IsNegative.  Check if the integer object is negative.

   - On success, return 1 if *obj is negative, and 0 otherwise.
   - On failure, set an exception, and return -1. */
PyAPI_FUNC(int) PyLong_IsNegative(PyObject *obj);

/* PyLong_IsZero.  Check if the integer object is zero.

   - On success, return 1 if *obj is zero, and 0 if it is non-zero.
   - On failure, set an exception, and return -1. */
PyAPI_FUNC(int) PyLong_IsZero(PyObject *obj);

/* PyLong_GetSign.  Get the sign of an integer object:
   0, -1 or +1 for zero, negative or positive integer, respectively.

   - On success, set '*sign' to the integer sign, and return 0.
   - On failure, set an exception, and return -1. */
PyAPI_FUNC(int) PyLong_GetSign(PyObject *v, int *sign);

Py_DEPRECATED(3.14) PyAPI_FUNC(int) _PyLong_Sign(PyObject *v);

/* _PyLong_NumBits.  Return the number of bits needed to represent the
   absolute value of a long.  For example, this returns 1 for 1 and -1, 2
   for 2 and -2, and 2 for 3 and -3.  It returns 0 for 0.
   v must not be NULL, and must be a normalized long.
   Always successful.
*/
PyAPI_FUNC(int64_t) _PyLong_NumBits(PyObject *v);

/* _PyLong_FromByteArray:  View the n unsigned bytes as a binary integer in
   base 256, and return a Python int with the same numeric value.
   If n is 0, the integer is 0.  Else:
   If little_endian is 1/true, bytes[n-1] is the MSB and bytes[0] the LSB;
   else (little_endian is 0/false) bytes[0] is the MSB and bytes[n-1] the
   LSB.
   If is_signed is 0/false, view the bytes as a non-negative integer.
   If is_signed is 1/true, view the bytes as a 2's-complement integer,
   non-negative if bit 0x80 of the MSB is clear, negative if set.
   Error returns:
   + Return NULL with the appropriate exception set if there's not
     enough memory to create the Python int.
*/
PyAPI_FUNC(PyObject *) _PyLong_FromByteArray(
    const unsigned char* bytes, size_t n,
    int little_endian, int is_signed);

/* _PyLong_AsByteArray: Convert the least-significant 8*n bits of long
   v to a base-256 integer, stored in array bytes.  Normally return 0,
   return -1 on error.
   If little_endian is 1/true, store the MSB at bytes[n-1] and the LSB at
   bytes[0]; else (little_endian is 0/false) store the MSB at bytes[0] and
   the LSB at bytes[n-1].
   If is_signed is 0/false, it's an error if v < 0; else (v >= 0) n bytes
   are filled and there's nothing special about bit 0x80 of the MSB.
   If is_signed is 1/true, bytes is filled with the 2's-complement
   representation of v's value.  Bit 0x80 of the MSB is the sign bit.
   Error returns (-1):
   + is_signed is 0 and v < 0.  TypeError is set in this case, and bytes
     isn't altered.
   + n isn't big enough to hold the full mathematical value of v.  For
     example, if is_signed is 0 and there are more digits in the v than
     fit in n; or if is_signed is 1, v < 0, and n is just 1 bit shy of
     being large enough to hold a sign bit.  OverflowError is set in this
     case, but bytes holds the least-significant n bytes of the true value.
*/
PyAPI_FUNC(int) _PyLong_AsByteArray(PyLongObject* v,
    unsigned char* bytes, size_t n,
    int little_endian, int is_signed, int with_exceptions);

/* For use by the gcd function in mathmodule.c */
PyAPI_FUNC(PyObject *) _PyLong_GCD(PyObject *, PyObject *);
