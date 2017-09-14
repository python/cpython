/* struct module -- pack values into and (out of) bytes objects */

/* New version supporting byte order, alignment and size options,
   character strings, and unsigned numbers */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structmember.h"
#include <ctype.h>

static PyTypeObject PyStructType;

/* The translation function for each format character is table driven */
typedef struct _formatdef {
    char format;
    Py_ssize_t size;
    Py_ssize_t alignment;
    PyObject* (*unpack)(const char *,
                        const struct _formatdef *);
    int (*pack)(char *, PyObject *,
                const struct _formatdef *);
} formatdef;

typedef struct _formatcode {
    const struct _formatdef *fmtdef;
    Py_ssize_t offset;
    Py_ssize_t size;
    Py_ssize_t repeat;
} formatcode;

/* Struct object interface */

typedef struct {
    PyObject_HEAD
    Py_ssize_t s_size;
    Py_ssize_t s_len;
    formatcode *s_codes;
    PyObject *s_format;
    PyObject *weakreflist; /* List of weak references */
} PyStructObject;


#define PyStruct_Check(op) PyObject_TypeCheck(op, &PyStructType)
#define PyStruct_CheckExact(op) (Py_TYPE(op) == &PyStructType)


/* Exception */

static PyObject *StructError;


/* Define various structs to figure out the alignments of types */


typedef struct { char c; short x; } st_short;
typedef struct { char c; int x; } st_int;
typedef struct { char c; long x; } st_long;
typedef struct { char c; float x; } st_float;
typedef struct { char c; double x; } st_double;
typedef struct { char c; void *x; } st_void_p;
typedef struct { char c; size_t x; } st_size_t;
typedef struct { char c; _Bool x; } st_bool;

#define SHORT_ALIGN (sizeof(st_short) - sizeof(short))
#define INT_ALIGN (sizeof(st_int) - sizeof(int))
#define LONG_ALIGN (sizeof(st_long) - sizeof(long))
#define FLOAT_ALIGN (sizeof(st_float) - sizeof(float))
#define DOUBLE_ALIGN (sizeof(st_double) - sizeof(double))
#define VOID_P_ALIGN (sizeof(st_void_p) - sizeof(void *))
#define SIZE_T_ALIGN (sizeof(st_size_t) - sizeof(size_t))
#define BOOL_ALIGN (sizeof(st_bool) - sizeof(_Bool))

/* We can't support q and Q in native mode unless the compiler does;
   in std mode, they're 8 bytes on all platforms. */
typedef struct { char c; long long x; } s_long_long;
#define LONG_LONG_ALIGN (sizeof(s_long_long) - sizeof(long long))

#ifdef __powerc
#pragma options align=reset
#endif

/* Helper for integer format codes: converts an arbitrary Python object to a
   PyLongObject if possible, otherwise fails.  Caller should decref. */

static PyObject *
get_pylong(PyObject *v)
{
    assert(v != NULL);
    if (!PyLong_Check(v)) {
        /* Not an integer;  try to use __index__ to convert. */
        if (PyIndex_Check(v)) {
            v = PyNumber_Index(v);
            if (v == NULL)
                return NULL;
        }
        else {
            PyErr_SetString(StructError,
                            "required argument is not an integer");
            return NULL;
        }
    }
    else
        Py_INCREF(v);

    assert(PyLong_Check(v));
    return v;
}

/* Helper routine to get a C long and raise the appropriate error if it isn't
   one */

static int
get_long(PyObject *v, long *p)
{
    long x;

    v = get_pylong(v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsLong(v);
    Py_DECREF(v);
    if (x == (long)-1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            PyErr_SetString(StructError,
                            "argument out of range");
        return -1;
    }
    *p = x;
    return 0;
}


/* Same, but handling unsigned long */

static int
get_ulong(PyObject *v, unsigned long *p)
{
    unsigned long x;

    v = get_pylong(v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsUnsignedLong(v);
    Py_DECREF(v);
    if (x == (unsigned long)-1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            PyErr_SetString(StructError,
                            "argument out of range");
        return -1;
    }
    *p = x;
    return 0;
}

/* Same, but handling native long long. */

static int
get_longlong(PyObject *v, long long *p)
{
    long long x;

    v = get_pylong(v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsLongLong(v);
    Py_DECREF(v);
    if (x == (long long)-1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            PyErr_SetString(StructError,
                            "argument out of range");
        return -1;
    }
    *p = x;
    return 0;
}

/* Same, but handling native unsigned long long. */

static int
get_ulonglong(PyObject *v, unsigned long long *p)
{
    unsigned long long x;

    v = get_pylong(v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsUnsignedLongLong(v);
    Py_DECREF(v);
    if (x == (unsigned long long)-1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            PyErr_SetString(StructError,
                            "argument out of range");
        return -1;
    }
    *p = x;
    return 0;
}

/* Same, but handling Py_ssize_t */

static int
get_ssize_t(PyObject *v, Py_ssize_t *p)
{
    Py_ssize_t x;

    v = get_pylong(v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsSsize_t(v);
    Py_DECREF(v);
    if (x == (Py_ssize_t)-1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            PyErr_SetString(StructError,
                            "argument out of range");
        return -1;
    }
    *p = x;
    return 0;
}

/* Same, but handling size_t */

static int
get_size_t(PyObject *v, size_t *p)
{
    size_t x;

    v = get_pylong(v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsSize_t(v);
    Py_DECREF(v);
    if (x == (size_t)-1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            PyErr_SetString(StructError,
                            "argument out of range");
        return -1;
    }
    *p = x;
    return 0;
}


#define RANGE_ERROR(x, f, flag, mask) return _range_error(f, flag)


/* Floating point helpers */

static PyObject *
unpack_halffloat(const char *p,  /* start of 2-byte string */
                 int le)         /* true for little-endian, false for big-endian */
{
    double x;

    x = _PyFloat_Unpack2((unsigned char *)p, le);
    if (x == -1.0 && PyErr_Occurred()) {
        return NULL;
    }
    return PyFloat_FromDouble(x);
}

static int
pack_halffloat(char *p,      /* start of 2-byte string */
               PyObject *v,  /* value to pack */
               int le)       /* true for little-endian, false for big-endian */
{
    double x = PyFloat_AsDouble(v);
    if (x == -1.0 && PyErr_Occurred()) {
        PyErr_SetString(StructError,
                        "required argument is not a float");
        return -1;
    }
    return _PyFloat_Pack2(x, (unsigned char *)p, le);
}

static PyObject *
unpack_float(const char *p,  /* start of 4-byte string */
         int le)             /* true for little-endian, false for big-endian */
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

/* Helper to format the range error exceptions */
static int
_range_error(const formatdef *f, int is_unsigned)
{
    /* ulargest is the largest unsigned value with f->size bytes.
     * Note that the simpler:
     *     ((size_t)1 << (f->size * 8)) - 1
     * doesn't work when f->size == sizeof(size_t) because C doesn't
     * define what happens when a left shift count is >= the number of
     * bits in the integer being shifted; e.g., on some boxes it doesn't
     * shift at all when they're equal.
     */
    const size_t ulargest = (size_t)-1 >> ((SIZEOF_SIZE_T - f->size)*8);
    assert(f->size >= 1 && f->size <= SIZEOF_SIZE_T);
    if (is_unsigned)
        PyErr_Format(StructError,
            "'%c' format requires 0 <= number <= %zu",
            f->format,
            ulargest);
    else {
        const Py_ssize_t largest = (Py_ssize_t)(ulargest >> 1);
        PyErr_Format(StructError,
            "'%c' format requires %zd <= number <= %zd",
            f->format,
            ~ largest,
            largest);
    }

    return -1;
}



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
    return PyBytes_FromStringAndSize(p, 1);
}

static PyObject *
nu_byte(const char *p, const formatdef *f)
{
    return PyLong_FromLong((long) *(signed char *)p);
}

static PyObject *
nu_ubyte(const char *p, const formatdef *f)
{
    return PyLong_FromLong((long) *(unsigned char *)p);
}

static PyObject *
nu_short(const char *p, const formatdef *f)
{
    short x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromLong((long)x);
}

static PyObject *
nu_ushort(const char *p, const formatdef *f)
{
    unsigned short x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromLong((long)x);
}

static PyObject *
nu_int(const char *p, const formatdef *f)
{
    int x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromLong((long)x);
}

static PyObject *
nu_uint(const char *p, const formatdef *f)
{
    unsigned int x;
    memcpy((char *)&x, p, sizeof x);
#if (SIZEOF_LONG > SIZEOF_INT)
    return PyLong_FromLong((long)x);
#else
    if (x <= ((unsigned int)LONG_MAX))
        return PyLong_FromLong((long)x);
    return PyLong_FromUnsignedLong((unsigned long)x);
#endif
}

static PyObject *
nu_long(const char *p, const formatdef *f)
{
    long x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromLong(x);
}

static PyObject *
nu_ulong(const char *p, const formatdef *f)
{
    unsigned long x;
    memcpy((char *)&x, p, sizeof x);
    if (x <= LONG_MAX)
        return PyLong_FromLong((long)x);
    return PyLong_FromUnsignedLong(x);
}

static PyObject *
nu_ssize_t(const char *p, const formatdef *f)
{
    Py_ssize_t x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromSsize_t(x);
}

static PyObject *
nu_size_t(const char *p, const formatdef *f)
{
    size_t x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromSize_t(x);
}


/* Native mode doesn't support q or Q unless the platform C supports
   long long (or, on Windows, __int64). */

static PyObject *
nu_longlong(const char *p, const formatdef *f)
{
    long long x;
    memcpy((char *)&x, p, sizeof x);
    if (x >= LONG_MIN && x <= LONG_MAX)
        return PyLong_FromLong(Py_SAFE_DOWNCAST(x, long long, long));
    return PyLong_FromLongLong(x);
}

static PyObject *
nu_ulonglong(const char *p, const formatdef *f)
{
    unsigned long long x;
    memcpy((char *)&x, p, sizeof x);
    if (x <= LONG_MAX)
        return PyLong_FromLong(Py_SAFE_DOWNCAST(x, unsigned long long, long));
    return PyLong_FromUnsignedLongLong(x);
}

static PyObject *
nu_bool(const char *p, const formatdef *f)
{
    _Bool x;
    memcpy((char *)&x, p, sizeof x);
    return PyBool_FromLong(x != 0);
}


static PyObject *
nu_halffloat(const char *p, const formatdef *f)
{
#if PY_LITTLE_ENDIAN
    return unpack_halffloat(p, 1);
#else
    return unpack_halffloat(p, 0);
#endif
}

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
                        "byte format requires -128 <= number <= 127");
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
                        "ubyte format requires 0 <= number <= 255");
        return -1;
    }
    *(unsigned char *)p = (unsigned char)x;
    return 0;
}

static int
np_char(char *p, PyObject *v, const formatdef *f)
{
    if (!PyBytes_Check(v) || PyBytes_Size(v) != 1) {
        PyErr_SetString(StructError,
                        "char format requires a bytes object of length 1");
        return -1;
    }
    *p = *PyBytes_AsString(v);
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
                        "short format requires " Py_STRINGIFY(SHRT_MIN)
                        " <= number <= " Py_STRINGIFY(SHRT_MAX));
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
                        "ushort format requires 0 <= number <= "
                        Py_STRINGIFY(USHRT_MAX));
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
#if (SIZEOF_LONG > SIZEOF_INT)
    if ((x < ((long)INT_MIN)) || (x > ((long)INT_MAX)))
        RANGE_ERROR(x, f, 0, -1);
#endif
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
#if (SIZEOF_LONG > SIZEOF_INT)
    if (x > ((unsigned long)UINT_MAX))
        RANGE_ERROR(y, f, 1, -1);
#endif
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

static int
np_ssize_t(char *p, PyObject *v, const formatdef *f)
{
    Py_ssize_t x;
    if (get_ssize_t(v, &x) < 0)
        return -1;
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}

static int
np_size_t(char *p, PyObject *v, const formatdef *f)
{
    size_t x;
    if (get_size_t(v, &x) < 0)
        return -1;
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}

static int
np_longlong(char *p, PyObject *v, const formatdef *f)
{
    long long x;
    if (get_longlong(v, &x) < 0)
        return -1;
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}

static int
np_ulonglong(char *p, PyObject *v, const formatdef *f)
{
    unsigned long long x;
    if (get_ulonglong(v, &x) < 0)
        return -1;
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}


static int
np_bool(char *p, PyObject *v, const formatdef *f)
{
    int y;
    _Bool x;
    y = PyObject_IsTrue(v);
    if (y < 0)
        return -1;
    x = y;
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}

static int
np_halffloat(char *p, PyObject *v, const formatdef *f)
{
#if PY_LITTLE_ENDIAN
    return pack_halffloat(p, v, 1);
#else
    return pack_halffloat(p, v, 0);
#endif
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

static const formatdef native_table[] = {
    {'x',       sizeof(char),   0,              NULL},
    {'b',       sizeof(char),   0,              nu_byte,        np_byte},
    {'B',       sizeof(char),   0,              nu_ubyte,       np_ubyte},
    {'c',       sizeof(char),   0,              nu_char,        np_char},
    {'s',       sizeof(char),   0,              NULL},
    {'p',       sizeof(char),   0,              NULL},
    {'h',       sizeof(short),  SHORT_ALIGN,    nu_short,       np_short},
    {'H',       sizeof(short),  SHORT_ALIGN,    nu_ushort,      np_ushort},
    {'i',       sizeof(int),    INT_ALIGN,      nu_int,         np_int},
    {'I',       sizeof(int),    INT_ALIGN,      nu_uint,        np_uint},
    {'l',       sizeof(long),   LONG_ALIGN,     nu_long,        np_long},
    {'L',       sizeof(long),   LONG_ALIGN,     nu_ulong,       np_ulong},
    {'n',       sizeof(size_t), SIZE_T_ALIGN,   nu_ssize_t,     np_ssize_t},
    {'N',       sizeof(size_t), SIZE_T_ALIGN,   nu_size_t,      np_size_t},
    {'q',       sizeof(long long), LONG_LONG_ALIGN, nu_longlong, np_longlong},
    {'Q',       sizeof(long long), LONG_LONG_ALIGN, nu_ulonglong,np_ulonglong},
    {'?',       sizeof(_Bool),      BOOL_ALIGN,     nu_bool,        np_bool},
    {'e',       sizeof(short),  SHORT_ALIGN,    nu_halffloat,   np_halffloat},
    {'f',       sizeof(float),  FLOAT_ALIGN,    nu_float,       np_float},
    {'d',       sizeof(double), DOUBLE_ALIGN,   nu_double,      np_double},
    {'P',       sizeof(void *), VOID_P_ALIGN,   nu_void_p,      np_void_p},
    {0}
};

/* Big-endian routines. *****************************************************/

static PyObject *
bu_int(const char *p, const formatdef *f)
{
    long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | *bytes++;
    } while (--i > 0);
    /* Extend the sign bit. */
    if (SIZEOF_LONG > f->size)
        x |= -(x & (1L << ((8 * f->size) - 1)));
    return PyLong_FromLong(x);
}

static PyObject *
bu_uint(const char *p, const formatdef *f)
{
    unsigned long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | *bytes++;
    } while (--i > 0);
    if (x <= LONG_MAX)
        return PyLong_FromLong((long)x);
    return PyLong_FromUnsignedLong(x);
}

static PyObject *
bu_longlong(const char *p, const formatdef *f)
{
    long long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | *bytes++;
    } while (--i > 0);
    /* Extend the sign bit. */
    if (SIZEOF_LONG_LONG > f->size)
        x |= -(x & ((long long)1 << ((8 * f->size) - 1)));
    if (x >= LONG_MIN && x <= LONG_MAX)
        return PyLong_FromLong(Py_SAFE_DOWNCAST(x, long long, long));
    return PyLong_FromLongLong(x);
}

static PyObject *
bu_ulonglong(const char *p, const formatdef *f)
{
    unsigned long long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | *bytes++;
    } while (--i > 0);
    if (x <= LONG_MAX)
        return PyLong_FromLong(Py_SAFE_DOWNCAST(x, unsigned long long, long));
    return PyLong_FromUnsignedLongLong(x);
}

static PyObject *
bu_halffloat(const char *p, const formatdef *f)
{
    return unpack_halffloat(p, 0);
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

static PyObject *
bu_bool(const char *p, const formatdef *f)
{
    char x;
    memcpy((char *)&x, p, sizeof x);
    return PyBool_FromLong(x != 0);
}

static int
bp_int(char *p, PyObject *v, const formatdef *f)
{
    long x;
    Py_ssize_t i;
    unsigned char *q = (unsigned char *)p;
    if (get_long(v, &x) < 0)
        return -1;
    i = f->size;
    if (i != SIZEOF_LONG) {
        if ((i == 2) && (x < -32768 || x > 32767))
            RANGE_ERROR(x, f, 0, 0xffffL);
#if (SIZEOF_LONG != 4)
        else if ((i == 4) && (x < -2147483648L || x > 2147483647L))
            RANGE_ERROR(x, f, 0, 0xffffffffL);
#endif
    }
    do {
        q[--i] = (unsigned char)(x & 0xffL);
        x >>= 8;
    } while (i > 0);
    return 0;
}

static int
bp_uint(char *p, PyObject *v, const formatdef *f)
{
    unsigned long x;
    Py_ssize_t i;
    unsigned char *q = (unsigned char *)p;
    if (get_ulong(v, &x) < 0)
        return -1;
    i = f->size;
    if (i != SIZEOF_LONG) {
        unsigned long maxint = 1;
        maxint <<= (unsigned long)(i * 8);
        if (x >= maxint)
            RANGE_ERROR(x, f, 1, maxint - 1);
    }
    do {
        q[--i] = (unsigned char)(x & 0xffUL);
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
bp_halffloat(char *p, PyObject *v, const formatdef *f)
{
    return pack_halffloat(p, v, 0);
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

static int
bp_bool(char *p, PyObject *v, const formatdef *f)
{
    int y;
    y = PyObject_IsTrue(v);
    if (y < 0)
        return -1;
    *p = (char)y;
    return 0;
}

static formatdef bigendian_table[] = {
    {'x',       1,              0,              NULL},
    {'b',       1,              0,              nu_byte,        np_byte},
    {'B',       1,              0,              nu_ubyte,       np_ubyte},
    {'c',       1,              0,              nu_char,        np_char},
    {'s',       1,              0,              NULL},
    {'p',       1,              0,              NULL},
    {'h',       2,              0,              bu_int,         bp_int},
    {'H',       2,              0,              bu_uint,        bp_uint},
    {'i',       4,              0,              bu_int,         bp_int},
    {'I',       4,              0,              bu_uint,        bp_uint},
    {'l',       4,              0,              bu_int,         bp_int},
    {'L',       4,              0,              bu_uint,        bp_uint},
    {'q',       8,              0,              bu_longlong,    bp_longlong},
    {'Q',       8,              0,              bu_ulonglong,   bp_ulonglong},
    {'?',       1,              0,              bu_bool,        bp_bool},
    {'e',       2,              0,              bu_halffloat,   bp_halffloat},
    {'f',       4,              0,              bu_float,       bp_float},
    {'d',       8,              0,              bu_double,      bp_double},
    {0}
};

/* Little-endian routines. *****************************************************/

static PyObject *
lu_int(const char *p, const formatdef *f)
{
    long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | bytes[--i];
    } while (i > 0);
    /* Extend the sign bit. */
    if (SIZEOF_LONG > f->size)
        x |= -(x & (1L << ((8 * f->size) - 1)));
    return PyLong_FromLong(x);
}

static PyObject *
lu_uint(const char *p, const formatdef *f)
{
    unsigned long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | bytes[--i];
    } while (i > 0);
    if (x <= LONG_MAX)
        return PyLong_FromLong((long)x);
    return PyLong_FromUnsignedLong((long)x);
}

static PyObject *
lu_longlong(const char *p, const formatdef *f)
{
    long long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | bytes[--i];
    } while (i > 0);
    /* Extend the sign bit. */
    if (SIZEOF_LONG_LONG > f->size)
        x |= -(x & ((long long)1 << ((8 * f->size) - 1)));
    if (x >= LONG_MIN && x <= LONG_MAX)
        return PyLong_FromLong(Py_SAFE_DOWNCAST(x, long long, long));
    return PyLong_FromLongLong(x);
}

static PyObject *
lu_ulonglong(const char *p, const formatdef *f)
{
    unsigned long long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | bytes[--i];
    } while (i > 0);
    if (x <= LONG_MAX)
        return PyLong_FromLong(Py_SAFE_DOWNCAST(x, unsigned long long, long));
    return PyLong_FromUnsignedLongLong(x);
}

static PyObject *
lu_halffloat(const char *p, const formatdef *f)
{
    return unpack_halffloat(p, 1);
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
    Py_ssize_t i;
    unsigned char *q = (unsigned char *)p;
    if (get_long(v, &x) < 0)
        return -1;
    i = f->size;
    if (i != SIZEOF_LONG) {
        if ((i == 2) && (x < -32768 || x > 32767))
            RANGE_ERROR(x, f, 0, 0xffffL);
#if (SIZEOF_LONG != 4)
        else if ((i == 4) && (x < -2147483648L || x > 2147483647L))
            RANGE_ERROR(x, f, 0, 0xffffffffL);
#endif
    }
    do {
        *q++ = (unsigned char)(x & 0xffL);
        x >>= 8;
    } while (--i > 0);
    return 0;
}

static int
lp_uint(char *p, PyObject *v, const formatdef *f)
{
    unsigned long x;
    Py_ssize_t i;
    unsigned char *q = (unsigned char *)p;
    if (get_ulong(v, &x) < 0)
        return -1;
    i = f->size;
    if (i != SIZEOF_LONG) {
        unsigned long maxint = 1;
        maxint <<= (unsigned long)(i * 8);
        if (x >= maxint)
            RANGE_ERROR(x, f, 1, maxint - 1);
    }
    do {
        *q++ = (unsigned char)(x & 0xffUL);
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
lp_halffloat(char *p, PyObject *v, const formatdef *f)
{
    return pack_halffloat(p, v, 1);
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
    {'x',       1,              0,              NULL},
    {'b',       1,              0,              nu_byte,        np_byte},
    {'B',       1,              0,              nu_ubyte,       np_ubyte},
    {'c',       1,              0,              nu_char,        np_char},
    {'s',       1,              0,              NULL},
    {'p',       1,              0,              NULL},
    {'h',       2,              0,              lu_int,         lp_int},
    {'H',       2,              0,              lu_uint,        lp_uint},
    {'i',       4,              0,              lu_int,         lp_int},
    {'I',       4,              0,              lu_uint,        lp_uint},
    {'l',       4,              0,              lu_int,         lp_int},
    {'L',       4,              0,              lu_uint,        lp_uint},
    {'q',       8,              0,              lu_longlong,    lp_longlong},
    {'Q',       8,              0,              lu_ulonglong,   lp_ulonglong},
    {'?',       1,              0,              bu_bool,        bp_bool}, /* Std rep not endian dep,
        but potentially different from native rep -- reuse bx_bool funcs. */
    {'e',       2,              0,              lu_halffloat,   lp_halffloat},
    {'f',       4,              0,              lu_float,       lp_float},
    {'d',       8,              0,              lu_double,      lp_double},
    {0}
};


static const formatdef *
whichtable(const char **pfmt)
{
    const char *fmt = (*pfmt)++; /* May be backed out of later */
    switch (*fmt) {
    case '<':
        return lilendian_table;
    case '>':
    case '!': /* Network byte order is big-endian */
        return bigendian_table;
    case '=': { /* Host byte order -- different from native in alignment! */
#if PY_LITTLE_ENDIAN
        return lilendian_table;
#else
        return bigendian_table;
#endif
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


/* Align a size according to a format code.  Return -1 on overflow. */

static Py_ssize_t
align(Py_ssize_t size, char c, const formatdef *e)
{
    Py_ssize_t extra;

    if (e->format == c) {
        if (e->alignment && size > 0) {
            extra = (e->alignment - 1) - (size - 1) % (e->alignment);
            if (extra > PY_SSIZE_T_MAX - size)
                return -1;
            size += extra;
        }
    }
    return size;
}

/*
 * Struct object implementation.
 */

/* calculate the size of a format string */

static int
prepare_s(PyStructObject *self)
{
    const formatdef *f;
    const formatdef *e;
    formatcode *codes;

    const char *s;
    const char *fmt;
    char c;
    Py_ssize_t size, len, num, itemsize;
    size_t ncodes;

    fmt = PyBytes_AS_STRING(self->s_format);

    f = whichtable(&fmt);

    s = fmt;
    size = 0;
    len = 0;
    ncodes = 0;
    while ((c = *s++) != '\0') {
        if (Py_ISSPACE(Py_CHARMASK(c)))
            continue;
        if ('0' <= c && c <= '9') {
            num = c - '0';
            while ('0' <= (c = *s++) && c <= '9') {
                /* overflow-safe version of
                   if (num*10 + (c - '0') > PY_SSIZE_T_MAX) { ... } */
                if (num >= PY_SSIZE_T_MAX / 10 && (
                        num > PY_SSIZE_T_MAX / 10 ||
                        (c - '0') > PY_SSIZE_T_MAX % 10))
                    goto overflow;
                num = num*10 + (c - '0');
            }
            if (c == '\0') {
                PyErr_SetString(StructError,
                                "repeat count given without format specifier");
                return -1;
            }
        }
        else
            num = 1;

        e = getentry(c, f);
        if (e == NULL)
            return -1;

        switch (c) {
            case 's': /* fall through */
            case 'p': len++; ncodes++; break;
            case 'x': break;
            default: len += num; if (num) ncodes++; break;
        }

        itemsize = e->size;
        size = align(size, c, e);
        if (size == -1)
            goto overflow;

        /* if (size + num * itemsize > PY_SSIZE_T_MAX) { ... } */
        if (num > (PY_SSIZE_T_MAX - size) / itemsize)
            goto overflow;
        size += num * itemsize;
    }

    /* check for overflow */
    if ((ncodes + 1) > ((size_t)PY_SSIZE_T_MAX / sizeof(formatcode))) {
        PyErr_NoMemory();
        return -1;
    }

    self->s_size = size;
    self->s_len = len;
    codes = PyMem_MALLOC((ncodes + 1) * sizeof(formatcode));
    if (codes == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    /* Free any s_codes value left over from a previous initialization. */
    if (self->s_codes != NULL)
        PyMem_FREE(self->s_codes);
    self->s_codes = codes;

    s = fmt;
    size = 0;
    while ((c = *s++) != '\0') {
        if (Py_ISSPACE(Py_CHARMASK(c)))
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

        size = align(size, c, e);
        if (c == 's' || c == 'p') {
            codes->offset = size;
            codes->size = num;
            codes->fmtdef = e;
            codes->repeat = 1;
            codes++;
            size += num;
        } else if (c == 'x') {
            size += num;
        } else if (num) {
            codes->offset = size;
            codes->size = e->size;
            codes->fmtdef = e;
            codes->repeat = num;
            codes++;
            size += e->size * num;
        }
    }
    codes->fmtdef = NULL;
    codes->offset = size;
    codes->size = 0;
    codes->repeat = 0;

    return 0;

  overflow:
    PyErr_SetString(StructError,
                    "total struct size too long");
    return -1;
}

static PyObject *
s_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *self;

    assert(type != NULL && type->tp_alloc != NULL);

    self = type->tp_alloc(type, 0);
    if (self != NULL) {
        PyStructObject *s = (PyStructObject*)self;
        Py_INCREF(Py_None);
        s->s_format = Py_None;
        s->s_codes = NULL;
        s->s_size = -1;
        s->s_len = -1;
    }
    return self;
}

static int
s_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyStructObject *soself = (PyStructObject *)self;
    PyObject *o_format = NULL;
    int ret = 0;
    static char *kwlist[] = {"format", 0};

    assert(PyStruct_Check(self));

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:Struct", kwlist,
                                     &o_format))
        return -1;

    if (PyUnicode_Check(o_format)) {
        o_format = PyUnicode_AsASCIIString(o_format);
        if (o_format == NULL)
            return -1;
    }
    /* XXX support buffer interface, too */
    else {
        Py_INCREF(o_format);
    }

    if (!PyBytes_Check(o_format)) {
        Py_DECREF(o_format);
        PyErr_Format(PyExc_TypeError,
                     "Struct() argument 1 must be a str or bytes object, "
                     "not %.200s",
                     Py_TYPE(o_format)->tp_name);
        return -1;
    }

    Py_XSETREF(soself->s_format, o_format);

    ret = prepare_s(soself);
    return ret;
}

static void
s_dealloc(PyStructObject *s)
{
    if (s->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)s);
    if (s->s_codes != NULL) {
        PyMem_FREE(s->s_codes);
    }
    Py_XDECREF(s->s_format);
    Py_TYPE(s)->tp_free((PyObject *)s);
}

static PyObject *
s_unpack_internal(PyStructObject *soself, const char *startfrom) {
    formatcode *code;
    Py_ssize_t i = 0;
    PyObject *result = PyTuple_New(soself->s_len);
    if (result == NULL)
        return NULL;

    for (code = soself->s_codes; code->fmtdef != NULL; code++) {
        const formatdef *e = code->fmtdef;
        const char *res = startfrom + code->offset;
        Py_ssize_t j = code->repeat;
        while (j--) {
            PyObject *v;
            if (e->format == 's') {
                v = PyBytes_FromStringAndSize(res, code->size);
            } else if (e->format == 'p') {
                Py_ssize_t n = *(unsigned char*)res;
                if (n >= code->size)
                    n = code->size - 1;
                v = PyBytes_FromStringAndSize(res + 1, n);
            } else {
                v = e->unpack(res, e);
            }
            if (v == NULL)
                goto fail;
            PyTuple_SET_ITEM(result, i++, v);
            res += code->size;
        }
    }

    return result;
fail:
    Py_DECREF(result);
    return NULL;
}


PyDoc_STRVAR(s_unpack__doc__,
"S.unpack(buffer) -> (v1, v2, ...)\n\
\n\
Return a tuple containing values unpacked according to the format\n\
string S.format.  The buffer's size in bytes must be S.size.  See\n\
help(struct) for more on format strings.");

static PyObject *
s_unpack(PyObject *self, PyObject *input)
{
    Py_buffer vbuf;
    PyObject *result;
    PyStructObject *soself = (PyStructObject *)self;

    assert(PyStruct_Check(self));
    assert(soself->s_codes != NULL);
    if (PyObject_GetBuffer(input, &vbuf, PyBUF_SIMPLE) < 0)
        return NULL;
    if (vbuf.len != soself->s_size) {
        PyErr_Format(StructError,
                     "unpack requires a buffer of %zd bytes",
                     soself->s_size);
        PyBuffer_Release(&vbuf);
        return NULL;
    }
    result = s_unpack_internal(soself, vbuf.buf);
    PyBuffer_Release(&vbuf);
    return result;
}

PyDoc_STRVAR(s_unpack_from__doc__,
"S.unpack_from(buffer, offset=0) -> (v1, v2, ...)\n\
\n\
Return a tuple containing values unpacked according to the format\n\
string S.format.  The buffer's size in bytes, minus offset, must be at\n\
least S.size.  See help(struct) for more on format strings.");

static PyObject *
s_unpack_from(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"buffer", "offset", 0};

    PyObject *input;
    Py_ssize_t offset = 0;
    Py_buffer vbuf;
    PyObject *result;
    PyStructObject *soself = (PyStructObject *)self;

    assert(PyStruct_Check(self));
    assert(soself->s_codes != NULL);

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O|n:unpack_from", kwlist,
                                     &input, &offset))
        return NULL;
    if (PyObject_GetBuffer(input, &vbuf, PyBUF_SIMPLE) < 0)
        return NULL;
    if (offset < 0)
        offset += vbuf.len;
    if (offset < 0 || vbuf.len - offset < soself->s_size) {
        PyErr_Format(StructError,
            "unpack_from requires a buffer of at least %zd bytes",
            soself->s_size);
        PyBuffer_Release(&vbuf);
        return NULL;
    }
    result = s_unpack_internal(soself, (char*)vbuf.buf + offset);
    PyBuffer_Release(&vbuf);
    return result;
}


/* Unpack iterator type */

typedef struct {
    PyObject_HEAD
    PyStructObject *so;
    Py_buffer buf;
    Py_ssize_t index;
} unpackiterobject;

static void
unpackiter_dealloc(unpackiterobject *self)
{
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->so);
    PyBuffer_Release(&self->buf);
    PyObject_GC_Del(self);
}

static int
unpackiter_traverse(unpackiterobject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->so);
    Py_VISIT(self->buf.obj);
    return 0;
}

static PyObject *
unpackiter_len(unpackiterobject *self)
{
    Py_ssize_t len;
    if (self->so == NULL)
        len = 0;
    else
        len = (self->buf.len - self->index) / self->so->s_size;
    return PyLong_FromSsize_t(len);
}

static PyMethodDef unpackiter_methods[] = {
    {"__length_hint__", (PyCFunction) unpackiter_len, METH_NOARGS, NULL},
    {NULL,              NULL}           /* sentinel */
};

static PyObject *
unpackiter_iternext(unpackiterobject *self)
{
    PyObject *result;
    if (self->so == NULL)
        return NULL;
    if (self->index >= self->buf.len) {
        /* Iterator exhausted */
        Py_CLEAR(self->so);
        PyBuffer_Release(&self->buf);
        return NULL;
    }
    assert(self->index + self->so->s_size <= self->buf.len);
    result = s_unpack_internal(self->so,
                               (char*) self->buf.buf + self->index);
    self->index += self->so->s_size;
    return result;
}

static PyTypeObject unpackiter_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "unpack_iterator",                          /* tp_name */
    sizeof(unpackiterobject),                   /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)unpackiter_dealloc,             /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)unpackiter_traverse,          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)unpackiter_iternext,          /* tp_iternext */
    unpackiter_methods                          /* tp_methods */
};

PyDoc_STRVAR(s_iter_unpack__doc__,
"S.iter_unpack(buffer) -> iterator(v1, v2, ...)\n\
\n\
Return an iterator yielding tuples unpacked from the given bytes\n\
source, like a repeated invocation of unpack_from().  Requires\n\
that the bytes length be a multiple of the struct size.");

static PyObject *
s_iter_unpack(PyObject *_so, PyObject *input)
{
    PyStructObject *so = (PyStructObject *) _so;
    unpackiterobject *self;

    assert(PyStruct_Check(_so));
    assert(so->s_codes != NULL);

    if (so->s_size == 0) {
        PyErr_Format(StructError,
                     "cannot iteratively unpack with a struct of length 0");
        return NULL;
    }

    self = (unpackiterobject *) PyType_GenericAlloc(&unpackiter_type, 0);
    if (self == NULL)
        return NULL;

    if (PyObject_GetBuffer(input, &self->buf, PyBUF_SIMPLE) < 0) {
        Py_DECREF(self);
        return NULL;
    }
    if (self->buf.len % so->s_size != 0) {
        PyErr_Format(StructError,
                     "iterative unpacking requires a buffer of "
                     "a multiple of %zd bytes",
                     so->s_size);
        Py_DECREF(self);
        return NULL;
    }
    Py_INCREF(so);
    self->so = so;
    self->index = 0;
    return (PyObject *) self;
}


/*
 * Guts of the pack function.
 *
 * Takes a struct object, a tuple of arguments, and offset in that tuple of
 * argument for where to start processing the arguments for packing, and a
 * character buffer for writing the packed string.  The caller must insure
 * that the buffer may contain the required length for packing the arguments.
 * 0 is returned on success, 1 is returned if there is an error.
 *
 */
static int
s_pack_internal(PyStructObject *soself, PyObject *args, int offset, char* buf)
{
    formatcode *code;
    /* XXX(nnorwitz): why does i need to be a local?  can we use
       the offset parameter or do we need the wider width? */
    Py_ssize_t i;

    memset(buf, '\0', soself->s_size);
    i = offset;
    for (code = soself->s_codes; code->fmtdef != NULL; code++) {
        const formatdef *e = code->fmtdef;
        char *res = buf + code->offset;
        Py_ssize_t j = code->repeat;
        while (j--) {
            PyObject *v = PyTuple_GET_ITEM(args, i++);
            if (e->format == 's') {
                Py_ssize_t n;
                int isstring;
                void *p;
                isstring = PyBytes_Check(v);
                if (!isstring && !PyByteArray_Check(v)) {
                    PyErr_SetString(StructError,
                                    "argument for 's' must be a bytes object");
                    return -1;
                }
                if (isstring) {
                    n = PyBytes_GET_SIZE(v);
                    p = PyBytes_AS_STRING(v);
                }
                else {
                    n = PyByteArray_GET_SIZE(v);
                    p = PyByteArray_AS_STRING(v);
                }
                if (n > code->size)
                    n = code->size;
                if (n > 0)
                    memcpy(res, p, n);
            } else if (e->format == 'p') {
                Py_ssize_t n;
                int isstring;
                void *p;
                isstring = PyBytes_Check(v);
                if (!isstring && !PyByteArray_Check(v)) {
                    PyErr_SetString(StructError,
                                    "argument for 'p' must be a bytes object");
                    return -1;
                }
                if (isstring) {
                    n = PyBytes_GET_SIZE(v);
                    p = PyBytes_AS_STRING(v);
                }
                else {
                    n = PyByteArray_GET_SIZE(v);
                    p = PyByteArray_AS_STRING(v);
                }
                if (n > (code->size - 1))
                    n = code->size - 1;
                if (n > 0)
                    memcpy(res + 1, p, n);
                if (n > 255)
                    n = 255;
                *res = Py_SAFE_DOWNCAST(n, Py_ssize_t, unsigned char);
            } else {
                if (e->pack(res, v, e) < 0) {
                    if (PyLong_Check(v) && PyErr_ExceptionMatches(PyExc_OverflowError))
                        PyErr_SetString(StructError,
                                        "int too large to convert");
                    return -1;
                }
            }
            res += code->size;
        }
    }

    /* Success */
    return 0;
}


PyDoc_STRVAR(s_pack__doc__,
"S.pack(v1, v2, ...) -> bytes\n\
\n\
Return a bytes object containing values v1, v2, ... packed according\n\
to the format string S.format.  See help(struct) for more on format\n\
strings.");

static PyObject *
s_pack(PyObject *self, PyObject *args)
{
    PyStructObject *soself;
    PyObject *result;

    /* Validate arguments. */
    soself = (PyStructObject *)self;
    assert(PyStruct_Check(self));
    assert(soself->s_codes != NULL);
    if (PyTuple_GET_SIZE(args) != soself->s_len)
    {
        PyErr_Format(StructError,
            "pack expected %zd items for packing (got %zd)", soself->s_len, PyTuple_GET_SIZE(args));
        return NULL;
    }

    /* Allocate a new string */
    result = PyBytes_FromStringAndSize((char *)NULL, soself->s_size);
    if (result == NULL)
        return NULL;

    /* Call the guts */
    if ( s_pack_internal(soself, args, 0, PyBytes_AS_STRING(result)) != 0 ) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

PyDoc_STRVAR(s_pack_into__doc__,
"S.pack_into(buffer, offset, v1, v2, ...)\n\
\n\
Pack the values v1, v2, ... according to the format string S.format\n\
and write the packed bytes into the writable buffer buf starting at\n\
offset.  Note that the offset is a required argument.  See\n\
help(struct) for more on format strings.");

static PyObject *
s_pack_into(PyObject *self, PyObject *args)
{
    PyStructObject *soself;
    Py_buffer buffer;
    Py_ssize_t offset;

    /* Validate arguments.  +1 is for the first arg as buffer. */
    soself = (PyStructObject *)self;
    assert(PyStruct_Check(self));
    assert(soself->s_codes != NULL);
    if (PyTuple_GET_SIZE(args) != (soself->s_len + 2))
    {
        if (PyTuple_GET_SIZE(args) == 0) {
            PyErr_Format(StructError,
                        "pack_into expected buffer argument");
        }
        else if (PyTuple_GET_SIZE(args) == 1) {
            PyErr_Format(StructError,
                        "pack_into expected offset argument");
        }
        else {
            PyErr_Format(StructError,
                        "pack_into expected %zd items for packing (got %zd)",
                        soself->s_len, (PyTuple_GET_SIZE(args) - 2));
        }
        return NULL;
    }

    /* Extract a writable memory buffer from the first argument */
    if (!PyArg_Parse(PyTuple_GET_ITEM(args, 0), "w*", &buffer))
        return NULL;
    assert(buffer.len >= 0);

    /* Extract the offset from the first argument */
    offset = PyNumber_AsSsize_t(PyTuple_GET_ITEM(args, 1), PyExc_IndexError);
    if (offset == -1 && PyErr_Occurred()) {
        PyBuffer_Release(&buffer);
        return NULL;
    }

    /* Support negative offsets. */
    if (offset < 0)
        offset += buffer.len;

    /* Check boundaries */
    if (offset < 0 || (buffer.len - offset) < soself->s_size) {
        PyErr_Format(StructError,
                     "pack_into requires a buffer of at least %zd bytes",
                     soself->s_size);
        PyBuffer_Release(&buffer);
        return NULL;
    }

    /* Call the guts */
    if (s_pack_internal(soself, args, 2, (char*)buffer.buf + offset) != 0) {
        PyBuffer_Release(&buffer);
        return NULL;
    }

    PyBuffer_Release(&buffer);
    Py_RETURN_NONE;
}

static PyObject *
s_get_format(PyStructObject *self, void *unused)
{
    Py_INCREF(self->s_format);
    return self->s_format;
}

static PyObject *
s_get_size(PyStructObject *self, void *unused)
{
    return PyLong_FromSsize_t(self->s_size);
}

PyDoc_STRVAR(s_sizeof__doc__,
"S.__sizeof__() -> size of S in memory, in bytes");

static PyObject *
s_sizeof(PyStructObject *self, void *unused)
{
    Py_ssize_t size;
    formatcode *code;

    size = _PyObject_SIZE(Py_TYPE(self)) + sizeof(formatcode);
    for (code = self->s_codes; code->fmtdef != NULL; code++)
        size += sizeof(formatcode);
    return PyLong_FromSsize_t(size);
}

/* List of functions */

static struct PyMethodDef s_methods[] = {
    {"iter_unpack",     s_iter_unpack,  METH_O, s_iter_unpack__doc__},
    {"pack",            s_pack,         METH_VARARGS, s_pack__doc__},
    {"pack_into",       s_pack_into,    METH_VARARGS, s_pack_into__doc__},
    {"unpack",          s_unpack,       METH_O, s_unpack__doc__},
    {"unpack_from",     (PyCFunction)s_unpack_from, METH_VARARGS|METH_KEYWORDS,
                    s_unpack_from__doc__},
    {"__sizeof__",      (PyCFunction)s_sizeof, METH_NOARGS, s_sizeof__doc__},
    {NULL,       NULL}          /* sentinel */
};

PyDoc_STRVAR(s__doc__,
"Struct(fmt) --> compiled struct object\n"
"\n"
"Return a new Struct object which writes and reads binary data according to\n"
"the format string fmt.  See help(struct) for more on format strings.");

#define OFF(x) offsetof(PyStructObject, x)

static PyGetSetDef s_getsetlist[] = {
    {"format", (getter)s_get_format, (setter)NULL, "struct format string", NULL},
    {"size", (getter)s_get_size, (setter)NULL, "struct size in bytes", NULL},
    {NULL} /* sentinel */
};

static
PyTypeObject PyStructType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Struct",
    sizeof(PyStructObject),
    0,
    (destructor)s_dealloc,      /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    s__doc__,                           /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(PyStructObject, weakreflist),      /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    s_methods,                          /* tp_methods */
    NULL,                               /* tp_members */
    s_getsetlist,               /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    s_init,                             /* tp_init */
    PyType_GenericAlloc,/* tp_alloc */
    s_new,                              /* tp_new */
    PyObject_Del,               /* tp_free */
};


/* ---- Standalone functions  ---- */

#define MAXCACHE 100
static PyObject *cache = NULL;

static PyObject *
cache_struct(PyObject *fmt)
{
    PyObject * s_object;

    if (cache == NULL) {
        cache = PyDict_New();
        if (cache == NULL)
            return NULL;
    }

    s_object = PyDict_GetItem(cache, fmt);
    if (s_object != NULL) {
        Py_INCREF(s_object);
        return s_object;
    }

    s_object = PyObject_CallFunctionObjArgs((PyObject *)(&PyStructType), fmt, NULL);
    if (s_object != NULL) {
        if (PyDict_Size(cache) >= MAXCACHE)
            PyDict_Clear(cache);
        /* Attempt to cache the result */
        if (PyDict_SetItem(cache, fmt, s_object) == -1)
            PyErr_Clear();
    }
    return s_object;
}

PyDoc_STRVAR(clearcache_doc,
"Clear the internal cache.");

static PyObject *
clearcache(PyObject *self)
{
    Py_CLEAR(cache);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(calcsize_doc,
"calcsize(fmt) -> integer\n\
\n\
Return size in bytes of the struct described by the format string fmt.");

static PyObject *
calcsize(PyObject *self, PyObject *fmt)
{
    Py_ssize_t n;
    PyObject *s_object = cache_struct(fmt);
    if (s_object == NULL)
        return NULL;
    n = ((PyStructObject *)s_object)->s_size;
    Py_DECREF(s_object);
    return PyLong_FromSsize_t(n);
}

PyDoc_STRVAR(pack_doc,
"pack(fmt, v1, v2, ...) -> bytes\n\
\n\
Return a bytes object containing the values v1, v2, ... packed according\n\
to the format string fmt.  See help(struct) for more on format strings.");

static PyObject *
pack(PyObject *self, PyObject *args)
{
    PyObject *s_object, *fmt, *newargs, *result;
    Py_ssize_t n = PyTuple_GET_SIZE(args);

    if (n == 0) {
        PyErr_SetString(PyExc_TypeError, "missing format argument");
        return NULL;
    }
    fmt = PyTuple_GET_ITEM(args, 0);
    newargs = PyTuple_GetSlice(args, 1, n);
    if (newargs == NULL)
        return NULL;

    s_object = cache_struct(fmt);
    if (s_object == NULL) {
        Py_DECREF(newargs);
        return NULL;
    }
    result = s_pack(s_object, newargs);
    Py_DECREF(newargs);
    Py_DECREF(s_object);
    return result;
}

PyDoc_STRVAR(pack_into_doc,
"pack_into(fmt, buffer, offset, v1, v2, ...)\n\
\n\
Pack the values v1, v2, ... according to the format string fmt and write\n\
the packed bytes into the writable buffer buf starting at offset.  Note\n\
that the offset is a required argument.  See help(struct) for more\n\
on format strings.");

static PyObject *
pack_into(PyObject *self, PyObject *args)
{
    PyObject *s_object, *fmt, *newargs, *result;
    Py_ssize_t n = PyTuple_GET_SIZE(args);

    if (n == 0) {
        PyErr_SetString(PyExc_TypeError, "missing format argument");
        return NULL;
    }
    fmt = PyTuple_GET_ITEM(args, 0);
    newargs = PyTuple_GetSlice(args, 1, n);
    if (newargs == NULL)
        return NULL;

    s_object = cache_struct(fmt);
    if (s_object == NULL) {
        Py_DECREF(newargs);
        return NULL;
    }
    result = s_pack_into(s_object, newargs);
    Py_DECREF(newargs);
    Py_DECREF(s_object);
    return result;
}

PyDoc_STRVAR(unpack_doc,
"unpack(fmt, buffer) -> (v1, v2, ...)\n\
\n\
Return a tuple containing values unpacked according to the format string\n\
fmt.  The buffer's size in bytes must be calcsize(fmt). See help(struct)\n\
for more on format strings.");

static PyObject *
unpack(PyObject *self, PyObject *args)
{
    PyObject *s_object, *fmt, *inputstr, *result;

    if (!PyArg_UnpackTuple(args, "unpack", 2, 2, &fmt, &inputstr))
        return NULL;

    s_object = cache_struct(fmt);
    if (s_object == NULL)
        return NULL;
    result = s_unpack(s_object, inputstr);
    Py_DECREF(s_object);
    return result;
}

PyDoc_STRVAR(unpack_from_doc,
"unpack_from(fmt, buffer, offset=0) -> (v1, v2, ...)\n\
\n\
Return a tuple containing values unpacked according to the format string\n\
fmt.  The buffer's size, minus offset, must be at least calcsize(fmt).\n\
See help(struct) for more on format strings.");

static PyObject *
unpack_from(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *s_object, *fmt, *newargs, *result;
    Py_ssize_t n = PyTuple_GET_SIZE(args);

    if (n == 0) {
        PyErr_SetString(PyExc_TypeError, "missing format argument");
        return NULL;
    }
    fmt = PyTuple_GET_ITEM(args, 0);
    newargs = PyTuple_GetSlice(args, 1, n);
    if (newargs == NULL)
        return NULL;

    s_object = cache_struct(fmt);
    if (s_object == NULL) {
        Py_DECREF(newargs);
        return NULL;
    }
    result = s_unpack_from(s_object, newargs, kwds);
    Py_DECREF(newargs);
    Py_DECREF(s_object);
    return result;
}

PyDoc_STRVAR(iter_unpack_doc,
"iter_unpack(fmt, buffer) -> iterator(v1, v2, ...)\n\
\n\
Return an iterator yielding tuples unpacked from the given bytes\n\
source according to the format string, like a repeated invocation of\n\
unpack_from().  Requires that the bytes length be a multiple of the\n\
format struct size.");

static PyObject *
iter_unpack(PyObject *self, PyObject *args)
{
    PyObject *s_object, *fmt, *input, *result;

    if (!PyArg_ParseTuple(args, "OO:iter_unpack", &fmt, &input))
        return NULL;

    s_object = cache_struct(fmt);
    if (s_object == NULL)
        return NULL;
    result = s_iter_unpack(s_object, input);
    Py_DECREF(s_object);
    return result;
}

static struct PyMethodDef module_functions[] = {
    {"_clearcache",     (PyCFunction)clearcache,        METH_NOARGS,    clearcache_doc},
    {"calcsize",        calcsize,       METH_O, calcsize_doc},
    {"iter_unpack",     iter_unpack,    METH_VARARGS,   iter_unpack_doc},
    {"pack",            pack,           METH_VARARGS,   pack_doc},
    {"pack_into",       pack_into,      METH_VARARGS,   pack_into_doc},
    {"unpack",          unpack, METH_VARARGS,   unpack_doc},
    {"unpack_from",     (PyCFunction)unpack_from,
                    METH_VARARGS|METH_KEYWORDS,         unpack_from_doc},
    {NULL,       NULL}          /* sentinel */
};


/* Module initialization */

PyDoc_STRVAR(module_doc,
"Functions to convert between Python values and C structs.\n\
Python bytes objects are used to hold the data representing the C struct\n\
and also as format strings (explained below) to describe the layout of data\n\
in the C struct.\n\
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
  ?: _Bool (requires C99; if not available, char is used instead)\n\
  h:short; H:unsigned short; i:int; I:unsigned int;\n\
  l:long; L:unsigned long; f:float; d:double; e:half-float.\n\
Special cases (preceding decimal count indicates length):\n\
  s:string (array of char); p: pascal string (with count byte).\n\
Special cases (only available in native format):\n\
  n:ssize_t; N:size_t;\n\
  P:an integer type that is wide enough to hold a pointer.\n\
Special case (not in native mode unless 'long long' in platform C):\n\
  q:long long; Q:unsigned long long\n\
Whitespace between formats is ignored.\n\
\n\
The variable struct.error is an exception raised on errors.\n");


static struct PyModuleDef _structmodule = {
    PyModuleDef_HEAD_INIT,
    "_struct",
    module_doc,
    -1,
    module_functions,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__struct(void)
{
    PyObject *m;

    m = PyModule_Create(&_structmodule);
    if (m == NULL)
        return NULL;

    Py_TYPE(&PyStructType) = &PyType_Type;
    if (PyType_Ready(&PyStructType) < 0)
        return NULL;

    /* Check endian and swap in faster functions */
    {
        const formatdef *native = native_table;
        formatdef *other, *ptr;
#if PY_LITTLE_ENDIAN
        other = lilendian_table;
#else
        other = bigendian_table;
#endif
        /* Scan through the native table, find a matching
           entry in the endian table and swap in the
           native implementations whenever possible
           (64-bit platforms may not have "standard" sizes) */
        while (native->format != '\0' && other->format != '\0') {
            ptr = other;
            while (ptr->format != '\0') {
                if (ptr->format == native->format) {
                    /* Match faster when formats are
                       listed in the same order */
                    if (ptr == other)
                        other++;
                    /* Only use the trick if the
                       size matches */
                    if (ptr->size != native->size)
                        break;
                    /* Skip float and double, could be
                       "unknown" float format */
                    if (ptr->format == 'd' || ptr->format == 'f')
                        break;
                    ptr->pack = native->pack;
                    ptr->unpack = native->unpack;
                    break;
                }
                ptr++;
            }
            native++;
        }
    }

    /* Add some symbolic constants to the module */
    if (StructError == NULL) {
        StructError = PyErr_NewException("struct.error", NULL, NULL);
        if (StructError == NULL)
            return NULL;
    }

    Py_INCREF(StructError);
    PyModule_AddObject(m, "error", StructError);

    Py_INCREF((PyObject*)&PyStructType);
    PyModule_AddObject(m, "Struct", (PyObject*)&PyStructType);

    return m;
}
