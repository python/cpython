/* struct module -- pack values into and (out of) strings */

/* New version supporting byte order, alignment and size options,
   character strings, and unsigned numbers */

#include "Python.h"
#include <ctype.h>

PyDoc_STRVAR(struct__doc__,
"Functions to convert between Python values and C structs.\n\
Python strings are used to hold the data representing the C struct\n\
and also as format strings to describe the layout of data in the C struct.\n\
\n\
The optional first format char indicates byte order, size and alignment:\n\
 @: native order, size & alignment (default)\n\
 =: native order, std. size & alignment\n\
 <: little-endian, std. size & alignment\n\
 >: big-endian, std. size & alignment\n\
 !: same as >\n\
\n\
The remaining chars indicate types of args and must match exactly;\n\
these can be preceded by a decimal repeat count:\n\
 x: pad byte (no data); c:char; b:signed byte; B:unsigned byte;\n\
 h:short; H:unsigned short; i:int; I:unsigned int;\n\
 l:long; L:unsigned long; f:float; d:double.\n\
Special cases (preceding decimal count indicates length):\n\
 s:string (array of char); p: pascal string (with count byte).\n\
Special case (only available in native format):\n\
 P:an integer type that is wide enough to hold a pointer.\n\
Special case (not in native mode unless 'long long' in platform C):\n\
 q:long long; Q:unsigned long long\n\
Whitespace between formats is ignored.\n\
\n\
The variable struct.error is an exception raised on errors.");


/* Exception */

static PyObject *StructError;


/* Define various structs to figure out the alignments of types */


typedef struct { char c; short x; } st_short;
typedef struct { char c; int x; } st_int;
typedef struct { char c; long x; } st_long;
typedef struct { char c; float x; } st_float;
typedef struct { char c; double x; } st_double;
typedef struct { char c; void *x; } st_void_p;

#define SHORT_ALIGN (sizeof(st_short) - sizeof(short))
#define INT_ALIGN (sizeof(st_int) - sizeof(int))
#define LONG_ALIGN (sizeof(st_long) - sizeof(long))
#define FLOAT_ALIGN (sizeof(st_float) - sizeof(float))
#define DOUBLE_ALIGN (sizeof(st_double) - sizeof(double))
#define VOID_P_ALIGN (sizeof(st_void_p) - sizeof(void *))

/* We can't support q and Q in native mode unless the compiler does;
   in std mode, they're 8 bytes on all platforms. */
#ifdef HAVE_LONG_LONG
typedef struct { char c; PY_LONG_LONG x; } s_long_long;
#define LONG_LONG_ALIGN (sizeof(s_long_long) - sizeof(PY_LONG_LONG))
#endif

#define STRINGIFY(x)    #x

#ifdef __powerc
#pragma options align=reset
#endif

/* Helper to get a PyLongObject by hook or by crook.  Caller should decref. */

static PyObject *
get_pylong(PyObject *v)
{
	PyNumberMethods *m;

	assert(v != NULL);
	if (PyInt_Check(v))
		return PyLong_FromLong(PyInt_AS_LONG(v));
	if (PyLong_Check(v)) {
		Py_INCREF(v);
		return v;
	}
	m = v->ob_type->tp_as_number;
	if (m != NULL && m->nb_long != NULL) {
		v = m->nb_long(v);
		if (v == NULL)
			return NULL;
		if (PyLong_Check(v))
			return v;
		Py_DECREF(v);
	}
	PyErr_SetString(StructError,
			"cannot convert argument to long");
	return NULL;
}

/* Helper routine to get a Python integer and raise the appropriate error
   if it isn't one */

static int
get_long(PyObject *v, long *p)
{
	long x = PyInt_AsLong(v);
	if (x == -1 && PyErr_Occurred()) {
		if (PyErr_ExceptionMatches(PyExc_TypeError))
			PyErr_SetString(StructError,
					"required argument is not an integer");
		return -1;
	}
	*p = x;
	return 0;
}


/* Same, but handling unsigned long */

static int
get_ulong(PyObject *v, unsigned long *p)
{
	if (PyLong_Check(v)) {
		unsigned long x = PyLong_AsUnsignedLong(v);
		if (x == (unsigned long)(-1) && PyErr_Occurred())
			return -1;
		*p = x;
		return 0;
	}
	else {
		return get_long(v, (long *)p);
	}
}

#ifdef HAVE_LONG_LONG

/* Same, but handling native long long. */

static int
get_longlong(PyObject *v, PY_LONG_LONG *p)
{
	PY_LONG_LONG x;

	v = get_pylong(v);
	if (v == NULL)
		return -1;
	assert(PyLong_Check(v));
	x = PyLong_AsLongLong(v);
	Py_DECREF(v);
	if (x == (PY_LONG_LONG)-1 && PyErr_Occurred())
		return -1;
	*p = x;
	return 0;
}

/* Same, but handling native unsigned long long. */

static int
get_ulonglong(PyObject *v, unsigned PY_LONG_LONG *p)
{
	unsigned PY_LONG_LONG x;

	v = get_pylong(v);
	if (v == NULL)
		return -1;
	assert(PyLong_Check(v));
	x = PyLong_AsUnsignedLongLong(v);
	Py_DECREF(v);
	if (x == (unsigned PY_LONG_LONG)-1 && PyErr_Occurred())
		return -1;
	*p = x;
	return 0;
}

#endif

/* Floating point helpers */

static PyObject *
unpack_float(const char *p,  /* start of 4-byte string */
             int le)	     /* true for little-endian, false for big-endian */
{
	double x;

	x = _PyFloat_Unpack4((unsigned char *)p, le);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	return PyFloat_FromDouble(x);
}

static PyObject *
unpack_double(const char *p,  /* start of 8-byte string */
              int le)         /* true for little-endian, false for big-endian */
{
	double x;

	x = _PyFloat_Unpack8((unsigned char *)p, le);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	return PyFloat_FromDouble(x);
}


/* The translation function for each format character is table driven */

typedef struct _formatdef {
	char format;
	int size;
	int alignment;
	PyObject* (*unpack)(const char *,
			    const struct _formatdef *);
	int (*pack)(char *, PyObject *,
		    const struct _formatdef *);
} formatdef;

/* A large number of small routines follow, with names of the form

	[bln][up]_TYPE

   [bln] distiguishes among big-endian, little-endian and native.
   [pu] distiguishes between pack (to struct) and unpack (from struct).
   TYPE is one of char, byte, ubyte, etc.
*/

/* Native mode routines. ****************************************************/
/* NOTE:
   In all n[up]_<type> routines handling types larger than 1 byte, there is
   *no* guarantee that the p pointer is properly aligned for each type,
   therefore memcpy is called.  An intermediate variable is used to
   compensate for big-endian architectures.
   Normally both the intermediate variable and the memcpy call will be
   skipped by C optimisation in little-endian architectures (gcc >= 2.91
   does this). */

static PyObject *
nu_char(const char *p, const formatdef *f)
{
	return PyString_FromStringAndSize(p, 1);
}

static PyObject *
nu_byte(const char *p, const formatdef *f)
{
	return PyInt_FromLong((long) *(signed char *)p);
}

static PyObject *
nu_ubyte(const char *p, const formatdef *f)
{
	return PyInt_FromLong((long) *(unsigned char *)p);
}

static PyObject *
nu_short(const char *p, const formatdef *f)
{
	short x;
	memcpy((char *)&x, p, sizeof x);
	return PyInt_FromLong((long)x);
}

static PyObject *
nu_ushort(const char *p, const formatdef *f)
{
	unsigned short x;
	memcpy((char *)&x, p, sizeof x);
	return PyInt_FromLong((long)x);
}

static PyObject *
nu_int(const char *p, const formatdef *f)
{
	int x;
	memcpy((char *)&x, p, sizeof x);
	return PyInt_FromLong((long)x);
}

static PyObject *
nu_uint(const char *p, const formatdef *f)
{
	unsigned int x;
	memcpy((char *)&x, p, sizeof x);
	return PyLong_FromUnsignedLong((unsigned long)x);
}

static PyObject *
nu_long(const char *p, const formatdef *f)
{
	long x;
	memcpy((char *)&x, p, sizeof x);
	return PyInt_FromLong(x);
}

static PyObject *
nu_ulong(const char *p, const formatdef *f)
{
	unsigned long x;
	memcpy((char *)&x, p, sizeof x);
	return PyLong_FromUnsignedLong(x);
}

/* Native mode doesn't support q or Q unless the platform C supports
   long long (or, on Windows, __int64). */

#ifdef HAVE_LONG_LONG

static PyObject *
nu_longlong(const char *p, const formatdef *f)
{
	PY_LONG_LONG x;
	memcpy((char *)&x, p, sizeof x);
	return PyLong_FromLongLong(x);
}

static PyObject *
nu_ulonglong(const char *p, const formatdef *f)
{
	unsigned PY_LONG_LONG x;
	memcpy((char *)&x, p, sizeof x);
	return PyLong_FromUnsignedLongLong(x);
}

#endif

static PyObject *
nu_float(const char *p, const formatdef *f)
{
	float x;
	memcpy((char *)&x, p, sizeof x);
	return PyFloat_FromDouble((double)x);
}

static PyObject *
nu_double(const char *p, const formatdef *f)
{
	double x;
	memcpy((char *)&x, p, sizeof x);
	return PyFloat_FromDouble(x);
}

static PyObject *
nu_void_p(const char *p, const formatdef *f)
{
	void *x;
	memcpy((char *)&x, p, sizeof x);
	return PyLong_FromVoidPtr(x);
}

static int
np_byte(char *p, PyObject *v, const formatdef *f)
{
	long x;
	if (get_long(v, &x) < 0)
		return -1;
	if (x < -128 || x > 127){
		PyErr_SetString(StructError,
				"byte format requires -128<=number<=127");
		return -1;
	}
	*p = (char)x;
	return 0;
}

static int
np_ubyte(char *p, PyObject *v, const formatdef *f)
{
	long x;
	if (get_long(v, &x) < 0)
		return -1;
	if (x < 0 || x > 255){
		PyErr_SetString(StructError,
				"ubyte format requires 0<=number<=255");
		return -1;
	}
	*p = (char)x;
	return 0;
}

static int
np_char(char *p, PyObject *v, const formatdef *f)
{
	if (!PyString_Check(v) || PyString_Size(v) != 1) {
		PyErr_SetString(StructError,
				"char format require string of length 1");
		return -1;
	}
	*p = *PyString_AsString(v);
	return 0;
}

static int
np_short(char *p, PyObject *v, const formatdef *f)
{
	long x;
	short y;
	if (get_long(v, &x) < 0)
		return -1;
	if (x < SHRT_MIN || x > SHRT_MAX){
		PyErr_SetString(StructError,
				"short format requires " STRINGIFY(SHRT_MIN)
				"<=number<=" STRINGIFY(SHRT_MAX));
		return -1;
	}
	y = (short)x;
	memcpy(p, (char *)&y, sizeof y);
	return 0;
}

static int
np_ushort(char *p, PyObject *v, const formatdef *f)
{
	long x;
	unsigned short y;
	if (get_long(v, &x) < 0)
		return -1;
	if (x < 0 || x > USHRT_MAX){
		PyErr_SetString(StructError,
				"short format requires 0<=number<=" STRINGIFY(USHRT_MAX));
		return -1;
	}
	y = (unsigned short)x;
	memcpy(p, (char *)&y, sizeof y);
	return 0;
}

static int
np_int(char *p, PyObject *v, const formatdef *f)
{
	long x;
	int y;
	if (get_long(v, &x) < 0)
		return -1;
	y = (int)x;
	memcpy(p, (char *)&y, sizeof y);
	return 0;
}

static int
np_uint(char *p, PyObject *v, const formatdef *f)
{
	unsigned long x;
	unsigned int y;
	if (get_ulong(v, &x) < 0)
		return -1;
	y = (unsigned int)x;
	memcpy(p, (char *)&y, sizeof y);
	return 0;
}

static int
np_long(char *p, PyObject *v, const formatdef *f)
{
	long x;
	if (get_long(v, &x) < 0)
		return -1;
	memcpy(p, (char *)&x, sizeof x);
	return 0;
}

static int
np_ulong(char *p, PyObject *v, const formatdef *f)
{
	unsigned long x;
	if (get_ulong(v, &x) < 0)
		return -1;
	memcpy(p, (char *)&x, sizeof x);
	return 0;
}

#ifdef HAVE_LONG_LONG

static int
np_longlong(char *p, PyObject *v, const formatdef *f)
{
	PY_LONG_LONG x;
	if (get_longlong(v, &x) < 0)
		return -1;
	memcpy(p, (char *)&x, sizeof x);
	return 0;
}

static int
np_ulonglong(char *p, PyObject *v, const formatdef *f)
{
	unsigned PY_LONG_LONG x;
	if (get_ulonglong(v, &x) < 0)
		return -1;
	memcpy(p, (char *)&x, sizeof x);
	return 0;
}
#endif

static int
np_float(char *p, PyObject *v, const formatdef *f)
{
	float x = (float)PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	memcpy(p, (char *)&x, sizeof x);
	return 0;
}

static int
np_double(char *p, PyObject *v, const formatdef *f)
{
	double x = PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	memcpy(p, (char *)&x, sizeof(double));
	return 0;
}

static int
np_void_p(char *p, PyObject *v, const formatdef *f)
{
	void *x;

	v = get_pylong(v);
	if (v == NULL)
		return -1;
	assert(PyLong_Check(v));
	x = PyLong_AsVoidPtr(v);
	Py_DECREF(v);
	if (x == NULL && PyErr_Occurred())
		return -1;
	memcpy(p, (char *)&x, sizeof x);
	return 0;
}

static formatdef native_table[] = {
	{'x',	sizeof(char),	0,		NULL},
	{'b',	sizeof(char),	0,		nu_byte,	np_byte},
	{'B',	sizeof(char),	0,		nu_ubyte,	np_ubyte},
	{'c',	sizeof(char),	0,		nu_char,	np_char},
	{'s',	sizeof(char),	0,		NULL},
	{'p',	sizeof(char),	0,		NULL},
	{'h',	sizeof(short),	SHORT_ALIGN,	nu_short,	np_short},
	{'H',	sizeof(short),	SHORT_ALIGN,	nu_ushort,	np_ushort},
	{'i',	sizeof(int),	INT_ALIGN,	nu_int,		np_int},
	{'I',	sizeof(int),	INT_ALIGN,	nu_uint,	np_uint},
	{'l',	sizeof(long),	LONG_ALIGN,	nu_long,	np_long},
	{'L',	sizeof(long),	LONG_ALIGN,	nu_ulong,	np_ulong},
	{'f',	sizeof(float),	FLOAT_ALIGN,	nu_float,	np_float},
	{'d',	sizeof(double),	DOUBLE_ALIGN,	nu_double,	np_double},
	{'P',	sizeof(void *),	VOID_P_ALIGN,	nu_void_p,	np_void_p},
#ifdef HAVE_LONG_LONG
	{'q',	sizeof(PY_LONG_LONG), LONG_LONG_ALIGN, nu_longlong, np_longlong},
	{'Q',	sizeof(PY_LONG_LONG), LONG_LONG_ALIGN, nu_ulonglong,np_ulonglong},
#endif
	{0}
};

/* Big-endian routines. *****************************************************/

static PyObject *
bu_int(const char *p, const formatdef *f)
{
	long x = 0;
	int i = f->size;
	do {
		x = (x<<8) | (*p++ & 0xFF);
	} while (--i > 0);
	/* Extend the sign bit. */
	if (SIZEOF_LONG > f->size)
		x |= -(x & (1L << (8*f->size - 1)));
	return PyInt_FromLong(x);
}

static PyObject *
bu_uint(const char *p, const formatdef *f)
{
	unsigned long x = 0;
	int i = f->size;
	do {
		x = (x<<8) | (*p++ & 0xFF);
	} while (--i > 0);
	if (f->size >= 4)
		return PyLong_FromUnsignedLong(x);
	else
		return PyInt_FromLong((long)x);
}

static PyObject *
bu_longlong(const char *p, const formatdef *f)
{
	return _PyLong_FromByteArray((const unsigned char *)p,
				      8,
				      0, /* little-endian */
				      1  /* signed */);
}

static PyObject *
bu_ulonglong(const char *p, const formatdef *f)
{
	return _PyLong_FromByteArray((const unsigned char *)p,
				      8,
				      0, /* little-endian */
				      0  /* signed */);
}

static PyObject *
bu_float(const char *p, const formatdef *f)
{
	return unpack_float(p, 0);
}

static PyObject *
bu_double(const char *p, const formatdef *f)
{
	return unpack_double(p, 0);
}

static int
bp_int(char *p, PyObject *v, const formatdef *f)
{
	long x;
	int i;
	if (get_long(v, &x) < 0)
		return -1;
	i = f->size;
	do {
		p[--i] = (char)x;
		x >>= 8;
	} while (i > 0);
	return 0;
}

static int
bp_uint(char *p, PyObject *v, const formatdef *f)
{
	unsigned long x;
	int i;
	if (get_ulong(v, &x) < 0)
		return -1;
	i = f->size;
	do {
		p[--i] = (char)x;
		x >>= 8;
	} while (i > 0);
	return 0;
}

static int
bp_longlong(char *p, PyObject *v, const formatdef *f)
{
	int res;
	v = get_pylong(v);
	if (v == NULL)
		return -1;
	res = _PyLong_AsByteArray((PyLongObject *)v,
			   	  (unsigned char *)p,
				  8,
				  0, /* little_endian */
				  1  /* signed */);
	Py_DECREF(v);
	return res;
}

static int
bp_ulonglong(char *p, PyObject *v, const formatdef *f)
{
	int res;
	v = get_pylong(v);
	if (v == NULL)
		return -1;
	res = _PyLong_AsByteArray((PyLongObject *)v,
			   	  (unsigned char *)p,
				  8,
				  0, /* little_endian */
				  0  /* signed */);
	Py_DECREF(v);
	return res;
}

static int
bp_float(char *p, PyObject *v, const formatdef *f)
{
	double x = PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	return _PyFloat_Pack4(x, (unsigned char *)p, 0);
}

static int
bp_double(char *p, PyObject *v, const formatdef *f)
{
	double x = PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	return _PyFloat_Pack8(x, (unsigned char *)p, 0);
}

static formatdef bigendian_table[] = {
	{'x',	1,		0,		NULL},
	{'b',	1,		0,		bu_int,		bp_int},
	{'B',	1,		0,		bu_uint,	bp_int},
	{'c',	1,		0,		nu_char,	np_char},
	{'s',	1,		0,		NULL},
	{'p',	1,		0,		NULL},
	{'h',	2,		0,		bu_int,		bp_int},
	{'H',	2,		0,		bu_uint,	bp_uint},
	{'i',	4,		0,		bu_int,		bp_int},
	{'I',	4,		0,		bu_uint,	bp_uint},
	{'l',	4,		0,		bu_int,		bp_int},
	{'L',	4,		0,		bu_uint,	bp_uint},
	{'q',	8,		0,		bu_longlong,	bp_longlong},
	{'Q',	8,		0,		bu_ulonglong,	bp_ulonglong},
	{'f',	4,		0,		bu_float,	bp_float},
	{'d',	8,		0,		bu_double,	bp_double},
	{0}
};

/* Little-endian routines. *****************************************************/

static PyObject *
lu_int(const char *p, const formatdef *f)
{
	long x = 0;
	int i = f->size;
	do {
		x = (x<<8) | (p[--i] & 0xFF);
	} while (i > 0);
	/* Extend the sign bit. */
	if (SIZEOF_LONG > f->size)
		x |= -(x & (1L << (8*f->size - 1)));
	return PyInt_FromLong(x);
}

static PyObject *
lu_uint(const char *p, const formatdef *f)
{
	unsigned long x = 0;
	int i = f->size;
	do {
		x = (x<<8) | (p[--i] & 0xFF);
	} while (i > 0);
	if (f->size >= 4)
		return PyLong_FromUnsignedLong(x);
	else
		return PyInt_FromLong((long)x);
}

static PyObject *
lu_longlong(const char *p, const formatdef *f)
{
	return _PyLong_FromByteArray((const unsigned char *)p,
				      8,
				      1, /* little-endian */
				      1  /* signed */);
}

static PyObject *
lu_ulonglong(const char *p, const formatdef *f)
{
	return _PyLong_FromByteArray((const unsigned char *)p,
				      8,
				      1, /* little-endian */
				      0  /* signed */);
}

static PyObject *
lu_float(const char *p, const formatdef *f)
{
	return unpack_float(p, 1);
}

static PyObject *
lu_double(const char *p, const formatdef *f)
{
	return unpack_double(p, 1);
}

static int
lp_int(char *p, PyObject *v, const formatdef *f)
{
	long x;
	int i;
	if (get_long(v, &x) < 0)
		return -1;
	i = f->size;
	do {
		*p++ = (char)x;
		x >>= 8;
	} while (--i > 0);
	return 0;
}

static int
lp_uint(char *p, PyObject *v, const formatdef *f)
{
	unsigned long x;
	int i;
	if (get_ulong(v, &x) < 0)
		return -1;
	i = f->size;
	do {
		*p++ = (char)x;
		x >>= 8;
	} while (--i > 0);
	return 0;
}

static int
lp_longlong(char *p, PyObject *v, const formatdef *f)
{
	int res;
	v = get_pylong(v);
	if (v == NULL)
		return -1;
	res = _PyLong_AsByteArray((PyLongObject*)v,
			   	  (unsigned char *)p,
				  8,
				  1, /* little_endian */
				  1  /* signed */);
	Py_DECREF(v);
	return res;
}

static int
lp_ulonglong(char *p, PyObject *v, const formatdef *f)
{
	int res;
	v = get_pylong(v);
	if (v == NULL)
		return -1;
	res = _PyLong_AsByteArray((PyLongObject*)v,
			   	  (unsigned char *)p,
				  8,
				  1, /* little_endian */
				  0  /* signed */);
	Py_DECREF(v);
	return res;
}

static int
lp_float(char *p, PyObject *v, const formatdef *f)
{
	double x = PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	return _PyFloat_Pack4(x, (unsigned char *)p, 1);
}

static int
lp_double(char *p, PyObject *v, const formatdef *f)
{
	double x = PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	return _PyFloat_Pack8(x, (unsigned char *)p, 1);
}

static formatdef lilendian_table[] = {
	{'x',	1,		0,		NULL},
	{'b',	1,		0,		lu_int,		lp_int},
	{'B',	1,		0,		lu_uint,	lp_int},
	{'c',	1,		0,		nu_char,	np_char},
	{'s',	1,		0,		NULL},
	{'p',	1,		0,		NULL},
	{'h',	2,		0,		lu_int,		lp_int},
	{'H',	2,		0,		lu_uint,	lp_uint},
	{'i',	4,		0,		lu_int,		lp_int},
	{'I',	4,		0,		lu_uint,	lp_uint},
	{'l',	4,		0,		lu_int,		lp_int},
	{'L',	4,		0,		lu_uint,	lp_uint},
	{'q',	8,		0,		lu_longlong,	lp_longlong},
	{'Q',	8,		0,		lu_ulonglong,	lp_ulonglong},
	{'f',	4,		0,		lu_float,	lp_float},
	{'d',	8,		0,		lu_double,	lp_double},
	{0}
};


static const formatdef *
whichtable(char **pfmt)
{
	const char *fmt = (*pfmt)++; /* May be backed out of later */
	switch (*fmt) {
	case '<':
		return lilendian_table;
	case '>':
	case '!': /* Network byte order is big-endian */
		return bigendian_table;
	case '=': { /* Host byte order -- different from native in aligment! */
		int n = 1;
		char *p = (char *) &n;
		if (*p == 1)
			return lilendian_table;
		else
			return bigendian_table;
	}
	default:
		--*pfmt; /* Back out of pointer increment */
		/* Fall through */
	case '@':
		return native_table;
	}
}


/* Get the table entry for a format code */

static const formatdef *
getentry(int c, const formatdef *f)
{
	for (; f->format != '\0'; f++) {
		if (f->format == c) {
			return f;
		}
	}
	PyErr_SetString(StructError, "bad char in struct format");
	return NULL;
}


/* Align a size according to a format code */

static int
align(int size, int c, const formatdef *e)
{
	if (e->format == c) {
		if (e->alignment) {
			size = ((size + e->alignment - 1)
				/ e->alignment)
				* e->alignment;
		}
	}
	return size;
}


/* calculate the size of a format string */

static int
calcsize(const char *fmt, const formatdef *f)
{
	const formatdef *e;
	const char *s;
	char c;
	int size,  num, itemsize, x;

	s = fmt;
	size = 0;
	while ((c = *s++) != '\0') {
		if (isspace((int)c))
			continue;
		if ('0' <= c && c <= '9') {
			num = c - '0';
			while ('0' <= (c = *s++) && c <= '9') {
				x = num*10 + (c - '0');
				if (x/10 != num) {
					PyErr_SetString(
						StructError,
						"overflow in item count");
					return -1;
				}
				num = x;
			}
			if (c == '\0')
				break;
		}
		else
			num = 1;

		e = getentry(c, f);
		if (e == NULL)
			return -1;
		itemsize = e->size;
		size = align(size, c, e);
		x = num * itemsize;
		size += x;
		if (x/itemsize != num || size < 0) {
			PyErr_SetString(StructError,
					"total struct size too long");
			return -1;
		}
	}

	return size;
}


PyDoc_STRVAR(calcsize__doc__,
"calcsize(fmt) -> int\n\
Return size of C struct described by format string fmt.\n\
See struct.__doc__ for more on format strings.");

static PyObject *
struct_calcsize(PyObject *self, PyObject *args)
{
	char *fmt;
	const formatdef *f;
	int size;

	if (!PyArg_ParseTuple(args, "s:calcsize", &fmt))
		return NULL;
	f = whichtable(&fmt);
	size = calcsize(fmt, f);
	if (size < 0)
		return NULL;
	return PyInt_FromLong((long)size);
}


PyDoc_STRVAR(pack__doc__,
"pack(fmt, v1, v2, ...) -> string\n\
Return string containing values v1, v2, ... packed according to fmt.\n\
See struct.__doc__ for more on format strings.");

static PyObject *
struct_pack(PyObject *self, PyObject *args)
{
	const formatdef *f, *e;
	PyObject *format, *result, *v;
	char *fmt;
	int size, num;
	int i, n;
	char *s, *res, *restart, *nres;
	char c;

	if (args == NULL || !PyTuple_Check(args) ||
	    (n = PyTuple_Size(args)) < 1)
	{
		PyErr_SetString(PyExc_TypeError,
			"struct.pack requires at least one argument");
		return NULL;
	}
	format = PyTuple_GetItem(args, 0);
	fmt = PyString_AsString(format);
	if (!fmt)
		return NULL;
	f = whichtable(&fmt);
	size = calcsize(fmt, f);
	if (size < 0)
		return NULL;
	result = PyString_FromStringAndSize((char *)NULL, size);
	if (result == NULL)
		return NULL;

	s = fmt;
	i = 1;
	res = restart = PyString_AsString(result);

	while ((c = *s++) != '\0') {
		if (isspace((int)c))
			continue;
		if ('0' <= c && c <= '9') {
			num = c - '0';
			while ('0' <= (c = *s++) && c <= '9')
			       num = num*10 + (c - '0');
			if (c == '\0')
				break;
		}
		else
			num = 1;

		e = getentry(c, f);
		if (e == NULL)
			goto fail;
		nres = restart + align((int)(res-restart), c, e);
		/* Fill padd bytes with zeros */
		while (res < nres)
			*res++ = '\0';
		if (num == 0 && c != 's')
			continue;
		do {
			if (c == 'x') {
				/* doesn't consume arguments */
				memset(res, '\0', num);
				res += num;
				break;
			}
			if (i >= n) {
				PyErr_SetString(StructError,
					"insufficient arguments to pack");
				goto fail;
				}
			v = PyTuple_GetItem(args, i++);
			if (v == NULL)
				goto fail;
			if (c == 's') {
				/* num is string size, not repeat count */
				int n;
				if (!PyString_Check(v)) {
					PyErr_SetString(StructError,
					  "argument for 's' must be a string");
					goto fail;
				}
				n = PyString_Size(v);
				if (n > num)
					n = num;
				if (n > 0)
					memcpy(res, PyString_AsString(v), n);
				if (n < num)
					memset(res+n, '\0', num-n);
				res += num;
				break;
			}
			else if (c == 'p') {
				/* num is string size + 1,
				   to fit in the count byte */
				int n;
				num--; /* now num is max string size */
				if (!PyString_Check(v)) {
					PyErr_SetString(StructError,
					  "argument for 'p' must be a string");
					goto fail;
				}
				n = PyString_Size(v);
				if (n > num)
					n = num;
				if (n > 0)
					memcpy(res+1, PyString_AsString(v), n);
				if (n < num)
					/* no real need, just to be nice */
					memset(res+1+n, '\0', num-n);
				if (n > 255)
					n = 255;
				*res++ = n; /* store the length byte */
				res += num;
				break;
			}
			else {
				if (e->pack(res, v, e) < 0)
					goto fail;
				res += e->size;
			}
		} while (--num > 0);
	}

	if (i < n) {
		PyErr_SetString(StructError,
				"too many arguments for pack format");
		goto fail;
	}

	return result;

 fail:
	Py_DECREF(result);
	return NULL;
}


PyDoc_STRVAR(unpack__doc__,
"unpack(fmt, string) -> (v1, v2, ...)\n\
Unpack the string, containing packed C structure data, according\n\
to fmt.  Requires len(string)==calcsize(fmt).\n\
See struct.__doc__ for more on format strings.");

static PyObject *
struct_unpack(PyObject *self, PyObject *args)
{
	const formatdef *f, *e;
	char *str, *start, *fmt, *s;
	char c;
	int len, size, num;
	PyObject *res, *v;

	if (!PyArg_ParseTuple(args, "ss#:unpack", &fmt, &start, &len))
		return NULL;
	f = whichtable(&fmt);
	size = calcsize(fmt, f);
	if (size < 0)
		return NULL;
	if (size != len) {
		PyErr_SetString(StructError,
				"unpack str size does not match format");
		return NULL;
	}
	res = PyList_New(0);
	if (res == NULL)
		return NULL;
	str = start;
	s = fmt;
	while ((c = *s++) != '\0') {
		if (isspace((int)c))
			continue;
		if ('0' <= c && c <= '9') {
			num = c - '0';
			while ('0' <= (c = *s++) && c <= '9')
			       num = num*10 + (c - '0');
			if (c == '\0')
				break;
		}
		else
			num = 1;

		e = getentry(c, f);
		if (e == NULL)
			goto fail;
		str = start + align((int)(str-start), c, e);
		if (num == 0 && c != 's')
			continue;

		do {
			if (c == 'x') {
				str += num;
				break;
			}
			if (c == 's') {
				/* num is string size, not repeat count */
				v = PyString_FromStringAndSize(str, num);
				if (v == NULL)
					goto fail;
				str += num;
				num = 0;
			}
			else if (c == 'p') {
				/* num is string buffer size,
				   not repeat count */
				int n = *(unsigned char*)str;
				/* first byte (unsigned) is string size */
				if (n >= num)
					n = num-1;
				v = PyString_FromStringAndSize(str+1, n);
				if (v == NULL)
					goto fail;
				str += num;
				num = 0;
			}
			else {
				v = e->unpack(str, e);
				if (v == NULL)
					goto fail;
				str += e->size;
			}
			if (v == NULL || PyList_Append(res, v) < 0)
				goto fail;
			Py_DECREF(v);
		} while (--num > 0);
	}

	v = PyList_AsTuple(res);
	Py_DECREF(res);
	return v;

 fail:
	Py_DECREF(res);
	return NULL;
}


/* List of functions */

static PyMethodDef struct_methods[] = {
	{"calcsize",	struct_calcsize,	METH_VARARGS, calcsize__doc__},
	{"pack",	struct_pack,		METH_VARARGS, pack__doc__},
	{"unpack",	struct_unpack,		METH_VARARGS, unpack__doc__},
	{NULL,		NULL}		/* sentinel */
};


/* Module initialization */

PyMODINIT_FUNC
initstruct(void)
{
	PyObject *m;

	/* Create the module and add the functions */
	m = Py_InitModule4("struct", struct_methods, struct__doc__,
			   (PyObject*)NULL, PYTHON_API_VERSION);

	/* Add some symbolic constants to the module */
	if (StructError == NULL) {
		StructError = PyErr_NewException("struct.error", NULL, NULL);
		if (StructError == NULL)
			return;
	}
	Py_INCREF(StructError);
	PyModule_AddObject(m, "error", StructError);
}
