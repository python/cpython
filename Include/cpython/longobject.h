#ifndef Py_CPYTHON_LONGOBJECT_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(PyObject*) PyLong_FromUnicodeObject(PyObject *u, int base);

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

PyAPI_FUNC(int) PyUnstable_Long_IsCompact(const PyLongObject* op);
PyAPI_FUNC(Py_ssize_t) PyUnstable_Long_CompactValue(const PyLongObject* op);

// _PyLong_Sign.  Return 0 if v is 0, -1 if v < 0, +1 if v > 0.
// v must not be NULL, and must be a normalized long.
// There are no error cases.
PyAPI_FUNC(int) _PyLong_Sign(PyObject *v);

/* _PyLong_NumBits.  Return the number of bits needed to represent the
   absolute value of a long.  For example, this returns 1 for 1 and -1, 2
   for 2 and -2, and 2 for 3 and -3.  It returns 0 for 0.
   v must not be NULL, and must be a normalized long.
   (size_t)-1 is returned and OverflowError set if the true result doesn't
   fit in a size_t.
*/
PyAPI_FUNC(size_t) _PyLong_NumBits(PyObject *v);

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
