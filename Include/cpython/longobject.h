#ifndef Py_CPYTHON_LONGOBJECT_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(PyObject*) PyLong_FromUnicodeObject(PyObject *u, int base);

/* PyLong_CopyBits: Copy the integer value to a native buffer.
   n_bytes is the number of bytes available in the buffer. Pass 0 to request
   the required size for the value.
   endianness is -1 for native endian, 0 for big endian or 1 for little.

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
PyAPI_FUNC(int) PyLong_CopyBits(PyObject* v, void* buffer, size_t n_bytes,
    int endianness);

/* PyLong_FromByteArray: Create an integer value containing the number from
   a native buffer.
   n is the number of bytes to read from the buffer.
   Uses the current build's default endianness, and assumes the value was
   sign extended to 'n' bytes.
   PyLong_FromUnsignedByteArray assumes the value was zero extended, and
   even if the MSB is set the resulting int will be positive.

   Returns the int object, or NULL with an exception set. */
PyAPI_FUNC(PyObject) PyLong_FromByteArray(void* buffer, size_t n);
PyAPI_FUNC(PyObject) PyLong_FromUnsignedByteArray(void* buffer, size_t n);

PyAPI_FUNC(int) PyUnstable_Long_IsCompact(const PyLongObject* op);
PyAPI_FUNC(Py_ssize_t) PyUnstable_Long_CompactValue(const PyLongObject* op);

// _PyLong_Sign.  Return 0 if v is 0, -1 if v < 0, +1 if v > 0.
// v must not be NULL, and must be a normalized long.
// There are no error cases.
PyAPI_FUNC(int) _PyLong_Sign(PyObject *v);

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
