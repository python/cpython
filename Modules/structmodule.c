
/* struct module -- pack values into and (out of) strings */

/* New version supporting byte order, alignment and size options,
   character strings, and unsigned numbers */

static char struct__doc__[] = "\
Functions to convert between Python values and C structs.\n\
Python strings are used to hold the data representing the C struct\n\
and also as format strings to describe the layout of data in the C struct.\n\
\n\
The optional first format char indicates byte ordering and alignment:\n\
 @: native w/native alignment(default)\n\
 =: native w/standard alignment\n\
 <: little-endian, std. alignment\n\
 >: big-endian, std. alignment\n\
 !: network, std (same as >)\n\
\n\
The remaining chars indicate types of args and must match exactly;\n\
these can be preceded by a decimal repeat count:\n\
 x: pad byte (no data); c:char; b:signed byte; B:unsigned byte;\n\
 h:short; H:unsigned short; i:int; I:unsigned int;\n\
 l:long; L:unsigned long; f:float; d:double.\n\
Special cases (preceding decimal count indicates length):\n\
 s:string (array of char); p: pascal string (w. count byte).\n\
Special case (only available in native format):\n\
 P:an integer type that is wide enough to hold a pointer.\n\
Whitespace between formats is ignored.\n\
\n\
The variable struct.error is an exception raised on errors.";

#include "Python.h"

#include <ctype.h>


/* Exception */

static PyObject *StructError;


/* Define various structs to figure out the alignments of types */

#ifdef __MWERKS__
/*
** XXXX We have a problem here. There are no unique alignment rules
** on the PowerPC mac. 
*/
#ifdef __powerc
#pragma options align=mac68k
#endif
#endif /* __MWERKS__ */

typedef struct { char c; short x; } s_short;
typedef struct { char c; int x; } s_int;
typedef struct { char c; long x; } s_long;
typedef struct { char c; float x; } s_float;
typedef struct { char c; double x; } s_double;
typedef struct { char c; void *x; } s_void_p;

#define SHORT_ALIGN (sizeof(s_short) - sizeof(short))
#define INT_ALIGN (sizeof(s_int) - sizeof(int))
#define LONG_ALIGN (sizeof(s_long) - sizeof(long))
#define FLOAT_ALIGN (sizeof(s_float) - sizeof(float))
#define DOUBLE_ALIGN (sizeof(s_double) - sizeof(double))
#define VOID_P_ALIGN (sizeof(s_void_p) - sizeof(void *))

#define STRINGIFY(x)    #x

#ifdef __powerc
#pragma options align=reset
#endif

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


/* Floating point helpers */

/* These use ANSI/IEEE Standard 754-1985 (Standard for Binary Floating
   Point Arithmetic).  See the following URL:
   http://www.psc.edu/general/software/packages/ieee/ieee.html */

/* XXX Inf/NaN are not handled quite right (but underflow is!) */

static int
pack_float(double x, /* The number to pack */
           char *p,  /* Where to pack the high order byte */
           int incr) /* 1 for big-endian; -1 for little-endian */
{
	int s;
	int e;
	double f;
	long fbits;

	if (x < 0) {
		s = 1;
		x = -x;
	}
	else
		s = 0;

	f = frexp(x, &e);

	/* Normalize f to be in the range [1.0, 2.0) */
	if (0.5 <= f && f < 1.0) {
		f *= 2.0;
		e--;
	}
	else if (f == 0.0) {
		e = 0;
	}
	else {
		PyErr_SetString(PyExc_SystemError,
				"frexp() result out of range");
		return -1;
	}

	if (e >= 128) {
		/* XXX 128 itself is reserved for Inf/NaN */
		PyErr_SetString(PyExc_OverflowError,
				"float too large to pack with f format");
		return -1;
	}
	else if (e < -126) {
		/* Gradual underflow */
		f = ldexp(f, 126 + e);
		e = 0;
	}
	else if (!(e == 0 && f == 0.0)) {
		e += 127;
		f -= 1.0; /* Get rid of leading 1 */
	}

	f *= 8388608.0; /* 2**23 */
	fbits = (long) floor(f + 0.5); /* Round */

	/* First byte */
	*p = (s<<7) | (e>>1);
	p += incr;

	/* Second byte */
	*p = (char) (((e&1)<<7) | (fbits>>16));
	p += incr;

	/* Third byte */
	*p = (fbits>>8) & 0xFF;
	p += incr;

	/* Fourth byte */
	*p = fbits&0xFF;

	/* Done */
	return 0;
}

static int
pack_double(double x, /* The number to pack */
            char *p,  /* Where to pack the high order byte */
            int incr) /* 1 for big-endian; -1 for little-endian */
{
	int s;
	int e;
	double f;
	long fhi, flo;

	if (x < 0) {
		s = 1;
		x = -x;
	}
	else
		s = 0;

	f = frexp(x, &e);

	/* Normalize f to be in the range [1.0, 2.0) */
	if (0.5 <= f && f < 1.0) {
		f *= 2.0;
		e--;
	}
	else if (f == 0.0) {
		e = 0;
	}
	else {
		PyErr_SetString(PyExc_SystemError,
				"frexp() result out of range");
		return -1;
	}

	if (e >= 1024) {
		/* XXX 1024 itself is reserved for Inf/NaN */
		PyErr_SetString(PyExc_OverflowError,
				"float too large to pack with d format");
		return -1;
	}
	else if (e < -1022) {
		/* Gradual underflow */
		f = ldexp(f, 1022 + e);
		e = 0;
	}
	else if (!(e == 0 && f == 0.0)) {
		e += 1023;
		f -= 1.0; /* Get rid of leading 1 */
	}

	/* fhi receives the high 28 bits; flo the low 24 bits (== 52 bits) */
	f *= 268435456.0; /* 2**28 */
	fhi = (long) floor(f); /* Truncate */
	f -= (double)fhi;
	f *= 16777216.0; /* 2**24 */
	flo = (long) floor(f + 0.5); /* Round */

	/* First byte */
	*p = (s<<7) | (e>>4);
	p += incr;

	/* Second byte */
	*p = (char) (((e&0xF)<<4) | (fhi>>24));
	p += incr;

	/* Third byte */
	*p = (fhi>>16) & 0xFF;
	p += incr;

	/* Fourth byte */
	*p = (fhi>>8) & 0xFF;
	p += incr;

	/* Fifth byte */
	*p = fhi & 0xFF;
	p += incr;

	/* Sixth byte */
	*p = (flo>>16) & 0xFF;
	p += incr;

	/* Seventh byte */
	*p = (flo>>8) & 0xFF;
	p += incr;

	/* Eighth byte */
	*p = flo & 0xFF;
	p += incr;

	/* Done */
	return 0;
}

static PyObject *
unpack_float(const char *p,  /* Where the high order byte is */
             int incr)       /* 1 for big-endian; -1 for little-endian */
{
	int s;
	int e;
	long f;
	double x;

	/* First byte */
	s = (*p>>7) & 1;
	e = (*p & 0x7F) << 1;
	p += incr;

	/* Second byte */
	e |= (*p>>7) & 1;
	f = (*p & 0x7F) << 16;
	p += incr;

	/* Third byte */
	f |= (*p & 0xFF) << 8;
	p += incr;

	/* Fourth byte */
	f |= *p & 0xFF;

	x = (double)f / 8388608.0;

	/* XXX This sadly ignores Inf/NaN issues */
	if (e == 0)
		e = -126;
	else {
		x += 1.0;
		e -= 127;
	}
	x = ldexp(x, e);

	if (s)
		x = -x;

	return PyFloat_FromDouble(x);
}

static PyObject *
unpack_double(const char *p,  /* Where the high order byte is */
              int incr)       /* 1 for big-endian; -1 for little-endian */
{
	int s;
	int e;
	long fhi, flo;
	double x;

	/* First byte */
	s = (*p>>7) & 1;
	e = (*p & 0x7F) << 4;
	p += incr;

	/* Second byte */
	e |= (*p>>4) & 0xF;
	fhi = (*p & 0xF) << 24;
	p += incr;

	/* Third byte */
	fhi |= (*p & 0xFF) << 16;
	p += incr;

	/* Fourth byte */
	fhi |= (*p & 0xFF) << 8;
	p += incr;

	/* Fifth byte */
	fhi |= *p & 0xFF;
	p += incr;

	/* Sixth byte */
	flo = (*p & 0xFF) << 16;
	p += incr;

	/* Seventh byte */
	flo |= (*p & 0xFF) << 8;
	p += incr;

	/* Eighth byte */
	flo |= *p & 0xFF;
	p += incr;

	x = (double)fhi + (double)flo / 16777216.0; /* 2**24 */
	x /= 268435456.0; /* 2**28 */

	/* XXX This sadly ignores Inf/NaN */
	if (e == 0)
		e = -1022;
	else {
		x += 1.0;
		e -= 1023;
	}
	x = ldexp(x, e);

	if (s)
		x = -x;

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
	return PyInt_FromLong((long) *(short *)p);
}

static PyObject *
nu_ushort(const char *p, const formatdef *f)
{
	return PyInt_FromLong((long) *(unsigned short *)p);
}

static PyObject *
nu_int(const char *p, const formatdef *f)
{
	return PyInt_FromLong((long) *(int *)p);
}

static PyObject *
nu_uint(const char *p, const formatdef *f)
{
	unsigned int x = *(unsigned int *)p;
	return PyLong_FromUnsignedLong((unsigned long)x);
}

static PyObject *
nu_long(const char *p, const formatdef *f)
{
	return PyInt_FromLong(*(long *)p);
}

static PyObject *
nu_ulong(const char *p, const formatdef *f)
{
	return PyLong_FromUnsignedLong(*(unsigned long *)p);
}

static PyObject *
nu_float(const char *p, const formatdef *f)
{
	float x;
	memcpy((char *)&x, p, sizeof(float));
	return PyFloat_FromDouble((double)x);
}

static PyObject *
nu_double(const char *p, const formatdef *f)
{
	double x;
	memcpy((char *)&x, p, sizeof(double));
	return PyFloat_FromDouble(x);
}

static PyObject *
nu_void_p(const char *p, const formatdef *f)
{
	return PyLong_FromVoidPtr(*(void **)p);
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
	if (get_long(v, &x) < 0)
		return -1;
	if (x < SHRT_MIN || x > SHRT_MAX){
		PyErr_SetString(StructError,
				"short format requires " STRINGIFY(SHRT_MIN)
                                "<=number<=" STRINGIFY(SHRT_MAX));
		return -1;
	}
	* (short *)p = (short)x;
	return 0;
}

static int
np_ushort(char *p, PyObject *v, const formatdef *f)
{
	long x;
	if (get_long(v, &x) < 0)
		return -1;
	if (x < 0 || x > USHRT_MAX){
		PyErr_SetString(StructError,
				"short format requires 0<=number<=" STRINGIFY(USHRT_MAX));
		return -1;
	}
	* (unsigned short *)p = (unsigned short)x;
	return 0;
}

static int
np_int(char *p, PyObject *v, const formatdef *f)
{
	long x;
	if (get_long(v, &x) < 0)
		return -1;
	* (int *)p = x;
	return 0;
}

static int
np_uint(char *p, PyObject *v, const formatdef *f)
{
	unsigned long x;
	if (get_ulong(v, &x) < 0)
		return -1;
	* (unsigned int *)p = x;
	return 0;
}

static int
np_long(char *p, PyObject *v, const formatdef *f)
{
	long x;
	if (get_long(v, &x) < 0)
		return -1;
	* (long *)p = x;
	return 0;
}

static int
np_ulong(char *p, PyObject *v, const formatdef *f)
{
	unsigned long x;
	if (get_ulong(v, &x) < 0)
		return -1;
	* (unsigned long *)p = x;
	return 0;
}

static int
np_float(char *p, PyObject *v, const formatdef *f)
{
	float x = (float)PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	memcpy(p, (char *)&x, sizeof(float));
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
	void *x = PyLong_AsVoidPtr(v);
	if (x == NULL && PyErr_Occurred()) {
		/* ### hrm. PyLong_AsVoidPtr raises SystemError */
		if (PyErr_ExceptionMatches(PyExc_TypeError))
			PyErr_SetString(StructError,
					"required argument is not an integer");
		return -1;
	}
	*(void **)p = x;
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
	{0}
};

static PyObject *
bu_int(const char *p, const formatdef *f)
{
	long x = 0;
	int i = f->size;
	do {
		x = (x<<8) | (*p++ & 0xFF);
	} while (--i > 0);
	i = 8*(sizeof(long) - f->size);
	if (i) {
		x <<= i;
		x >>= i;
	}
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
bu_float(const char *p, const formatdef *f)
{
	return unpack_float(p, 1);
}

static PyObject *
bu_double(const char *p, const formatdef *f)
{
	return unpack_double(p, 1);
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
bp_float(char *p, PyObject *v, const formatdef *f)
{
	double x = PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	return pack_float(x, p, 1);
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
	return pack_double(x, p, 1);
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
	{'f',	4,		0,		bu_float,	bp_float},
	{'d',	8,		0,		bu_double,	bp_double},
	{0}
};

static PyObject *
lu_int(const char *p, const formatdef *f)
{
	long x = 0;
	int i = f->size;
	do {
		x = (x<<8) | (p[--i] & 0xFF);
	} while (i > 0);
	i = 8*(sizeof(long) - f->size);
	if (i) {
		x <<= i;
		x >>= i;
	}
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
lu_float(const char *p, const formatdef *f)
{
	return unpack_float(p+3, -1);
}

static PyObject *
lu_double(const char *p, const formatdef *f)
{
	return unpack_double(p+7, -1);
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
lp_float(char *p, PyObject *v, const formatdef *f)
{
	double x = PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	return pack_float(x, p+3, -1);
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
	return pack_double(x, p+7, -1);
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


static char calcsize__doc__[] = "\
calcsize(fmt) -> int\n\
Return size of C struct described by format string fmt.\n\
See struct.__doc__ for more on format strings.";

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


static char pack__doc__[] = "\
pack(fmt, v1, v2, ...) -> string\n\
Return string containing values v1, v2, ... packed according to fmt.\n\
See struct.__doc__ for more on format strings.";

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
	if (!PyArg_Parse(format, "s", &fmt))
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


static char unpack__doc__[] = "\
unpack(fmt, string) -> (v1, v2, ...)\n\
Unpack the string, containing packed C structure data, according\n\
to fmt.  Requires len(string)==calcsize(fmt).\n\
See struct.__doc__ for more on format strings.";

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

DL_EXPORT(void)
initstruct(void)
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule4("struct", struct_methods, struct__doc__,
			   (PyObject*)NULL, PYTHON_API_VERSION);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	StructError = PyErr_NewException("struct.error", NULL, NULL);
	if (StructError == NULL)
		return;
	PyDict_SetItemString(d, "error", StructError);
}
