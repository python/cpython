#ifndef Py_LONGOBJECT_H
#define Py_LONGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


/* Long (arbitrary precision) integer object interface */

typedef struct _longobject PyLongObject; /* Revealed in longintrepr.h */

extern DL_IMPORT(PyTypeObject) PyLong_Type;

#define PyLong_Check(op) ((op)->ob_type == &PyLong_Type)

extern DL_IMPORT(PyObject *) PyLong_FromLong(long);
extern DL_IMPORT(PyObject *) PyLong_FromUnsignedLong(unsigned long);
extern DL_IMPORT(PyObject *) PyLong_FromDouble(double);
extern DL_IMPORT(long) PyLong_AsLong(PyObject *);
extern DL_IMPORT(unsigned long) PyLong_AsUnsignedLong(PyObject *);
extern DL_IMPORT(double) PyLong_AsDouble(PyObject *);
extern DL_IMPORT(PyObject *) PyLong_FromVoidPtr(void *);
extern DL_IMPORT(void *) PyLong_AsVoidPtr(PyObject *);

#ifdef HAVE_LONG_LONG

/* Hopefully this is portable... */
#ifndef ULONG_MAX
#define ULONG_MAX 4294967295U
#endif
#ifndef LONGLONG_MAX
#define LONGLONG_MAX 9223372036854775807LL
#endif
#ifndef ULONGLONG_MAX
#define ULONGLONG_MAX 0xffffffffffffffffULL
#endif

extern DL_IMPORT(PyObject *) PyLong_FromLongLong(LONG_LONG);
extern DL_IMPORT(PyObject *) PyLong_FromUnsignedLongLong(unsigned LONG_LONG);
extern DL_IMPORT(LONG_LONG) PyLong_AsLongLong(PyObject *);
extern DL_IMPORT(unsigned LONG_LONG) PyLong_AsUnsignedLongLong(PyObject *);
#endif /* HAVE_LONG_LONG */

DL_IMPORT(PyObject *) PyLong_FromString(char *, char **, int);
DL_IMPORT(PyObject *) PyLong_FromUnicode(Py_UNICODE*, int, int);

/* _PyLong_FromByteArray:  View the n unsigned bytes as a binary integer in
   base 256, and return a Python long with the same numeric value.
   If n is 0, the integer is 0.  Else:
   If little_endian is 1/true, bytes[n-1] is the MSB and bytes[0] the LSB;
   else (little_endian is 0/false) bytes[0] is the MSB and bytes[n-1] the
   LSB.
   If is_signed is 0/false, view the bytes as a non-negative integer.
   If is_signed is 1/true, view the bytes as a 2's-complement integer,
   non-negative if bit 0x80 of the MSB is clear, negative if set.
   Error returns:
   + Return NULL with the appropriate exception set if there's not
     enough memory to create the Python long.
*/
extern DL_IMPORT(PyObject *) _PyLong_FromByteArray(
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
     case, but bytes holds the least-signficant n bytes of the true value.
*/
extern DL_IMPORT(int) _PyLong_AsByteArray(PyLongObject* v,
	unsigned char* bytes, size_t n,
	int little_endian, int is_signed);

#ifdef __cplusplus
}
#endif
#endif /* !Py_LONGOBJECT_H */
