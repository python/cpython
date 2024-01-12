/* struct module -- pack values into and (out of) bytes objects */

/* New version supporting byte order, alignment and size options,
   character strings, and unsigned numbers */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_bytesobject.h"   // _PyBytesWriter
#include "pycore_long.h"          // _PyLong_AsByteArray()
#include "pycore_moduleobject.h"  // _PyModule_GetState()

#include <stddef.h>               // offsetof()

/*[clinic input]
class Struct "PyStructObject *" "&PyStructType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=9b032058a83ed7c3]*/

typedef struct {
    PyObject *cache;
    PyObject *PyStructType;
    PyObject *unpackiter_type;
    PyObject *StructError;
} _structmodulestate;

static inline _structmodulestate*
get_struct_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (_structmodulestate *)state;
}

static struct PyModuleDef _structmodule;

#define get_struct_state_structinst(self) \
    (get_struct_state(PyType_GetModuleByDef(Py_TYPE(self), &_structmodule)))
#define get_struct_state_iterinst(self) \
    (get_struct_state(PyType_GetModule(Py_TYPE(self))))

/* The translation function for each format character is table driven */
typedef struct _formatdef {
    char format;
    Py_ssize_t size;
    Py_ssize_t alignment;
    PyObject* (*unpack)(_structmodulestate *, const char *,
                        const struct _formatdef *);
    int (*pack)(_structmodulestate *, char *, PyObject *,
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

#define PyStruct_Check(op, state) PyObject_TypeCheck(op, (PyTypeObject *)(state)->PyStructType)

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

/*[python input]
class cache_struct_converter(CConverter):
    type = 'PyStructObject *'
    converter = 'cache_struct_converter'
    c_default = "NULL"
    broken_limited_capi = True

    def parse_arg(self, argname, displayname, *, limited_capi):
        assert not limited_capi
        return self.format_code("""
            if (!{converter}(module, {argname}, &{paramname})) {{{{
                goto exit;
            }}}}
            """,
            argname=argname,
            converter=self.converter)

    def cleanup(self):
        return "Py_XDECREF(%s);\n" % self.name
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=c33b27d6b06006c6]*/

static int cache_struct_converter(PyObject *, PyObject *, PyStructObject **);

#include "clinic/_struct.c.h"

/* Helper for integer format codes: converts an arbitrary Python object to a
   PyLongObject if possible, otherwise fails.  Caller should decref. */

static PyObject *
get_pylong(_structmodulestate *state, PyObject *v)
{
    assert(v != NULL);
    if (!PyLong_Check(v)) {
        /* Not an integer;  try to use __index__ to convert. */
        if (PyIndex_Check(v)) {
            v = _PyNumber_Index(v);
            if (v == NULL)
                return NULL;
        }
        else {
            PyErr_SetString(state->StructError,
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
get_long(_structmodulestate *state, PyObject *v, long *p)
{
    long x;

    v = get_pylong(state, v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsLong(v);
    Py_DECREF(v);
    if (x == (long)-1 && PyErr_Occurred()) {
        return -1;
    }
    *p = x;
    return 0;
}


/* Same, but handling unsigned long */

static int
get_ulong(_structmodulestate *state, PyObject *v, unsigned long *p)
{
    unsigned long x;

    v = get_pylong(state, v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsUnsignedLong(v);
    Py_DECREF(v);
    if (x == (unsigned long)-1 && PyErr_Occurred()) {
        return -1;
    }
    *p = x;
    return 0;
}

/* Same, but handling native long long. */

static int
get_longlong(_structmodulestate *state, PyObject *v, long long *p)
{
    long long x;

    v = get_pylong(state, v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsLongLong(v);
    Py_DECREF(v);
    if (x == (long long)-1 && PyErr_Occurred()) {
        return -1;
    }
    *p = x;
    return 0;
}

/* Same, but handling native unsigned long long. */

static int
get_ulonglong(_structmodulestate *state, PyObject *v, unsigned long long *p)
{
    unsigned long long x;

    v = get_pylong(state, v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsUnsignedLongLong(v);
    Py_DECREF(v);
    if (x == (unsigned long long)-1 && PyErr_Occurred()) {
        return -1;
    }
    *p = x;
    return 0;
}

/* Same, but handling Py_ssize_t */

static int
get_ssize_t(_structmodulestate *state, PyObject *v, Py_ssize_t *p)
{
    Py_ssize_t x;

    v = get_pylong(state, v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsSsize_t(v);
    Py_DECREF(v);
    if (x == (Py_ssize_t)-1 && PyErr_Occurred()) {
        return -1;
    }
    *p = x;
    return 0;
}

/* Same, but handling size_t */

static int
get_size_t(_structmodulestate *state, PyObject *v, size_t *p)
{
    size_t x;

    v = get_pylong(state, v);
    if (v == NULL)
        return -1;
    assert(PyLong_Check(v));
    x = PyLong_AsSize_t(v);
    Py_DECREF(v);
    if (x == (size_t)-1 && PyErr_Occurred()) {
        return -1;
    }
    *p = x;
    return 0;
}


#define RANGE_ERROR(state, f, flag) return _range_error(state, f, flag)


/* Floating point helpers */

static PyObject *
unpack_halffloat(const char *p,  /* start of 2-byte string */
                 int le)         /* true for little-endian, false for big-endian */
{
    double x = PyFloat_Unpack2(p, le);
    if (x == -1.0 && PyErr_Occurred()) {
        return NULL;
    }
    return PyFloat_FromDouble(x);
}

static int
pack_halffloat(_structmodulestate *state,
               char *p,      /* start of 2-byte string */
               PyObject *v,  /* value to pack */
               int le)       /* true for little-endian, false for big-endian */
{
    double x = PyFloat_AsDouble(v);
    if (x == -1.0 && PyErr_Occurred()) {
        PyErr_SetString(state->StructError,
                        "required argument is not a float");
        return -1;
    }
    return PyFloat_Pack2(x, p, le);
}

static PyObject *
unpack_float(const char *p,  /* start of 4-byte string */
         int le)             /* true for little-endian, false for big-endian */
{
    double x;

    x = PyFloat_Unpack4(p, le);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    return PyFloat_FromDouble(x);
}

static PyObject *
unpack_double(const char *p,  /* start of 8-byte string */
          int le)         /* true for little-endian, false for big-endian */
{
    double x;

    x = PyFloat_Unpack8(p, le);
    if (x == -1.0 && PyErr_Occurred())
        return NULL;
    return PyFloat_FromDouble(x);
}

/* Helper to format the range error exceptions */
static int
_range_error(_structmodulestate *state, const formatdef *f, int is_unsigned)
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
        PyErr_Format(state->StructError,
            "'%c' format requires 0 <= number <= %zu",
            f->format,
            ulargest);
    else {
        const Py_ssize_t largest = (Py_ssize_t)(ulargest >> 1);
        PyErr_Format(state->StructError,
            "'%c' format requires %zd <= number <= %zd",
            f->format,
            ~ largest,
            largest);
    }

    return -1;
}



/* A large number of small routines follow, with names of the form

   [bln][up]_TYPE

   [bln] distinguishes among big-endian, little-endian and native.
   [pu] distinguishes between pack (to struct) and unpack (from struct).
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
nu_char(_structmodulestate *state, const char *p, const formatdef *f)
{
    return PyBytes_FromStringAndSize(p, 1);
}

static PyObject *
nu_byte(_structmodulestate *state, const char *p, const formatdef *f)
{
    return PyLong_FromLong((long) *(signed char *)p);
}

static PyObject *
nu_ubyte(_structmodulestate *state, const char *p, const formatdef *f)
{
    return PyLong_FromLong((long) *(unsigned char *)p);
}

static PyObject *
nu_short(_structmodulestate *state, const char *p, const formatdef *f)
{
    short x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromLong((long)x);
}

static PyObject *
nu_ushort(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned short x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromLong((long)x);
}

static PyObject *
nu_int(_structmodulestate *state, const char *p, const formatdef *f)
{
    int x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromLong((long)x);
}

static PyObject *
nu_uint(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned int x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromUnsignedLong((unsigned long)x);
}

static PyObject *
nu_long(_structmodulestate *state, const char *p, const formatdef *f)
{
    long x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromLong(x);
}

static PyObject *
nu_ulong(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromUnsignedLong(x);
}

static PyObject *
nu_ssize_t(_structmodulestate *state, const char *p, const formatdef *f)
{
    Py_ssize_t x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromSsize_t(x);
}

static PyObject *
nu_size_t(_structmodulestate *state, const char *p, const formatdef *f)
{
    size_t x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromSize_t(x);
}

static PyObject *
nu_longlong(_structmodulestate *state, const char *p, const formatdef *f)
{
    long long x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromLongLong(x);
}

static PyObject *
nu_ulonglong(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long long x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromUnsignedLongLong(x);
}

static PyObject *
nu_bool(_structmodulestate *state, const char *p, const formatdef *f)
{
    _Bool x;
    memcpy((char *)&x, p, sizeof x);
    return PyBool_FromLong(x != 0);
}


static PyObject *
nu_halffloat(_structmodulestate *state, const char *p, const formatdef *f)
{
#if PY_LITTLE_ENDIAN
    return unpack_halffloat(p, 1);
#else
    return unpack_halffloat(p, 0);
#endif
}

static PyObject *
nu_float(_structmodulestate *state, const char *p, const formatdef *f)
{
    float x;
    memcpy((char *)&x, p, sizeof x);
    return PyFloat_FromDouble((double)x);
}

static PyObject *
nu_double(_structmodulestate *state, const char *p, const formatdef *f)
{
    double x;
    memcpy((char *)&x, p, sizeof x);
    return PyFloat_FromDouble(x);
}

static PyObject *
nu_void_p(_structmodulestate *state, const char *p, const formatdef *f)
{
    void *x;
    memcpy((char *)&x, p, sizeof x);
    return PyLong_FromVoidPtr(x);
}

static int
np_byte(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    long x;
    if (get_long(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 0);
        }
        return -1;
    }
    if (x < -128 || x > 127) {
        RANGE_ERROR(state, f, 0);
    }
    *p = (char)x;
    return 0;
}

static int
np_ubyte(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    long x;
    if (get_long(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 1);
        }
        return -1;
    }
    if (x < 0 || x > 255) {
        RANGE_ERROR(state, f, 1);
    }
    *(unsigned char *)p = (unsigned char)x;
    return 0;
}

static int
np_char(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    if (!PyBytes_Check(v) || PyBytes_Size(v) != 1) {
        PyErr_SetString(state->StructError,
                        "char format requires a bytes object of length 1");
        return -1;
    }
    *p = *PyBytes_AS_STRING(v);
    return 0;
}

static int
np_short(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    long x;
    short y;
    if (get_long(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 0);
        }
        return -1;
    }
    if (x < SHRT_MIN || x > SHRT_MAX) {
        RANGE_ERROR(state, f, 0);
    }
    y = (short)x;
    memcpy(p, (char *)&y, sizeof y);
    return 0;
}

static int
np_ushort(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    long x;
    unsigned short y;
    if (get_long(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 1);
        }
        return -1;
    }
    if (x < 0 || x > USHRT_MAX) {
        RANGE_ERROR(state, f, 1);
    }
    y = (unsigned short)x;
    memcpy(p, (char *)&y, sizeof y);
    return 0;
}

static int
np_int(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    long x;
    int y;
    if (get_long(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 0);
        }
        return -1;
    }
#if (SIZEOF_LONG > SIZEOF_INT)
    if ((x < ((long)INT_MIN)) || (x > ((long)INT_MAX)))
        RANGE_ERROR(state, f, 0);
#endif
    y = (int)x;
    memcpy(p, (char *)&y, sizeof y);
    return 0;
}

static int
np_uint(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    unsigned long x;
    unsigned int y;
    if (get_ulong(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 1);
        }
        return -1;
    }
    y = (unsigned int)x;
#if (SIZEOF_LONG > SIZEOF_INT)
    if (x > ((unsigned long)UINT_MAX))
        RANGE_ERROR(state, f, 1);
#endif
    memcpy(p, (char *)&y, sizeof y);
    return 0;
}

static int
np_long(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    long x;
    if (get_long(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 0);
        }
        return -1;
    }
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}

static int
np_ulong(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    unsigned long x;
    if (get_ulong(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 1);
        }
        return -1;
    }
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}

static int
np_ssize_t(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    Py_ssize_t x;
    if (get_ssize_t(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 0);
        }
        return -1;
    }
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}

static int
np_size_t(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    size_t x;
    if (get_size_t(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 1);
        }
        return -1;
    }
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}

static int
np_longlong(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    long long x;
    if (get_longlong(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            PyErr_Format(state->StructError,
                         "'%c' format requires %lld <= number <= %lld",
                         f->format,
                         LLONG_MIN,
                         LLONG_MAX);
        }
        return -1;
    }
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}

static int
np_ulonglong(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    unsigned long long x;
    if (get_ulonglong(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            PyErr_Format(state->StructError,
                         "'%c' format requires 0 <= number <= %llu",
                         f->format,
                         ULLONG_MAX);
        }
        return -1;
    }
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}


static int
np_bool(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
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
np_halffloat(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
#if PY_LITTLE_ENDIAN
    return pack_halffloat(state, p, v, 1);
#else
    return pack_halffloat(state, p, v, 0);
#endif
}

static int
np_float(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    float x = (float)PyFloat_AsDouble(v);
    if (x == -1 && PyErr_Occurred()) {
        PyErr_SetString(state->StructError,
                        "required argument is not a float");
        return -1;
    }
    memcpy(p, (char *)&x, sizeof x);
    return 0;
}

static int
np_double(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    double x = PyFloat_AsDouble(v);
    if (x == -1 && PyErr_Occurred()) {
        PyErr_SetString(state->StructError,
                        "required argument is not a float");
        return -1;
    }
    memcpy(p, (char *)&x, sizeof(double));
    return 0;
}

static int
np_void_p(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    void *x;

    v = get_pylong(state, v);
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
bu_short(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long x = 0;

    /* This function is only ever used in the case f->size == 2. */
    assert(f->size == 2);
    Py_ssize_t i = 2;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | *bytes++;
    } while (--i > 0);
    /* Extend sign, avoiding implementation-defined or undefined behaviour. */
    x = (x ^ 0x8000U) - 0x8000U;
    return PyLong_FromLong(x & 0x8000U ? -1 - (long)(~x) : (long)x);
}

static PyObject *
bu_int(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long x = 0;

    /* This function is only ever used in the case f->size == 4. */
    assert(f->size == 4);
    Py_ssize_t i = 4;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | *bytes++;
    } while (--i > 0);
    /* Extend sign, avoiding implementation-defined or undefined behaviour. */
    x = (x ^ 0x80000000U) - 0x80000000U;
    return PyLong_FromLong(x & 0x80000000U ? -1 - (long)(~x) : (long)x);
}

static PyObject *
bu_uint(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | *bytes++;
    } while (--i > 0);
    return PyLong_FromUnsignedLong(x);
}

static PyObject *
bu_longlong(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long long x = 0;

    /* This function is only ever used in the case f->size == 8. */
    assert(f->size == 8);
    Py_ssize_t i = 8;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | *bytes++;
    } while (--i > 0);
    /* Extend sign, avoiding implementation-defined or undefined behaviour. */
    x = (x ^ 0x8000000000000000U) - 0x8000000000000000U;
    return PyLong_FromLongLong(
        x & 0x8000000000000000U ? -1 - (long long)(~x) : (long long)x);
}

static PyObject *
bu_ulonglong(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | *bytes++;
    } while (--i > 0);
    return PyLong_FromUnsignedLongLong(x);
}

static PyObject *
bu_halffloat(_structmodulestate *state, const char *p, const formatdef *f)
{
    return unpack_halffloat(p, 0);
}

static PyObject *
bu_float(_structmodulestate *state, const char *p, const formatdef *f)
{
    return unpack_float(p, 0);
}

static PyObject *
bu_double(_structmodulestate *state, const char *p, const formatdef *f)
{
    return unpack_double(p, 0);
}

static PyObject *
bu_bool(_structmodulestate *state, const char *p, const formatdef *f)
{
    return PyBool_FromLong(*p != 0);
}

static int
bp_int(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    long x;
    Py_ssize_t i;
    unsigned char *q = (unsigned char *)p;
    if (get_long(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 0);
        }
        return -1;
    }
    i = f->size;
    if (i != SIZEOF_LONG) {
        if ((i == 2) && (x < -32768 || x > 32767))
            RANGE_ERROR(state, f, 0);
#if (SIZEOF_LONG != 4)
        else if ((i == 4) && (x < -2147483648L || x > 2147483647L))
            RANGE_ERROR(state, f, 0);
#endif
    }
    do {
        q[--i] = (unsigned char)(x & 0xffL);
        x >>= 8;
    } while (i > 0);
    return 0;
}

static int
bp_uint(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    unsigned long x;
    Py_ssize_t i;
    unsigned char *q = (unsigned char *)p;
    if (get_ulong(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 1);
        }
        return -1;
    }
    i = f->size;
    if (i != SIZEOF_LONG) {
        unsigned long maxint = 1;
        maxint <<= (unsigned long)(i * 8);
        if (x >= maxint)
            RANGE_ERROR(state, f, 1);
    }
    do {
        q[--i] = (unsigned char)(x & 0xffUL);
        x >>= 8;
    } while (i > 0);
    return 0;
}

static int
bp_longlong(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    int res;
    v = get_pylong(state, v);
    if (v == NULL)
        return -1;
    res = _PyLong_AsByteArray((PyLongObject *)v,
                              (unsigned char *)p,
                              8,
                              0, /* little_endian */
                              1  /* signed */);
    Py_DECREF(v);
    if (res == -1 && PyErr_Occurred()) {
        PyErr_Format(state->StructError,
                     "'%c' format requires %lld <= number <= %lld",
                     f->format,
                     LLONG_MIN,
                     LLONG_MAX);
        return -1;
    }
    return res;
}

static int
bp_ulonglong(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    int res;
    v = get_pylong(state, v);
    if (v == NULL)
        return -1;
    res = _PyLong_AsByteArray((PyLongObject *)v,
                              (unsigned char *)p,
                              8,
                              0, /* little_endian */
                              0  /* signed */);
    Py_DECREF(v);
    if (res == -1 && PyErr_Occurred()) {
        PyErr_Format(state->StructError,
                     "'%c' format requires 0 <= number <= %llu",
                     f->format,
                     ULLONG_MAX);
        return -1;
    }
    return res;
}

static int
bp_halffloat(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    return pack_halffloat(state, p, v, 0);
}

static int
bp_float(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    double x = PyFloat_AsDouble(v);
    if (x == -1 && PyErr_Occurred()) {
        PyErr_SetString(state->StructError,
                        "required argument is not a float");
        return -1;
    }
    return PyFloat_Pack4(x, p, 0);
}

static int
bp_double(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    double x = PyFloat_AsDouble(v);
    if (x == -1 && PyErr_Occurred()) {
        PyErr_SetString(state->StructError,
                        "required argument is not a float");
        return -1;
    }
    return PyFloat_Pack8(x, p, 0);
}

static int
bp_bool(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
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
    {'h',       2,              0,              bu_short,       bp_int},
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
lu_short(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long x = 0;

    /* This function is only ever used in the case f->size == 2. */
    assert(f->size == 2);
    Py_ssize_t i = 2;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | bytes[--i];
    } while (i > 0);
    /* Extend sign, avoiding implementation-defined or undefined behaviour. */
    x = (x ^ 0x8000U) - 0x8000U;
    return PyLong_FromLong(x & 0x8000U ? -1 - (long)(~x) : (long)x);
}

static PyObject *
lu_int(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long x = 0;

    /* This function is only ever used in the case f->size == 4. */
    assert(f->size == 4);
    Py_ssize_t i = 4;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | bytes[--i];
    } while (i > 0);
    /* Extend sign, avoiding implementation-defined or undefined behaviour. */
    x = (x ^ 0x80000000U) - 0x80000000U;
    return PyLong_FromLong(x & 0x80000000U ? -1 - (long)(~x) : (long)x);
}

static PyObject *
lu_uint(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | bytes[--i];
    } while (i > 0);
    return PyLong_FromUnsignedLong(x);
}

static PyObject *
lu_longlong(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long long x = 0;

    /* This function is only ever used in the case f->size == 8. */
    assert(f->size == 8);
    Py_ssize_t i = 8;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | bytes[--i];
    } while (i > 0);
    /* Extend sign, avoiding implementation-defined or undefined behaviour. */
    x = (x ^ 0x8000000000000000U) - 0x8000000000000000U;
    return PyLong_FromLongLong(
        x & 0x8000000000000000U ? -1 - (long long)(~x) : (long long)x);
}

static PyObject *
lu_ulonglong(_structmodulestate *state, const char *p, const formatdef *f)
{
    unsigned long long x = 0;
    Py_ssize_t i = f->size;
    const unsigned char *bytes = (const unsigned char *)p;
    do {
        x = (x<<8) | bytes[--i];
    } while (i > 0);
    return PyLong_FromUnsignedLongLong(x);
}

static PyObject *
lu_halffloat(_structmodulestate *state, const char *p, const formatdef *f)
{
    return unpack_halffloat(p, 1);
}

static PyObject *
lu_float(_structmodulestate *state, const char *p, const formatdef *f)
{
    return unpack_float(p, 1);
}

static PyObject *
lu_double(_structmodulestate *state, const char *p, const formatdef *f)
{
    return unpack_double(p, 1);
}

static int
lp_int(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    long x;
    Py_ssize_t i;
    unsigned char *q = (unsigned char *)p;
    if (get_long(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 0);
        }
        return -1;
    }
    i = f->size;
    if (i != SIZEOF_LONG) {
        if ((i == 2) && (x < -32768 || x > 32767))
            RANGE_ERROR(state, f, 0);
#if (SIZEOF_LONG != 4)
        else if ((i == 4) && (x < -2147483648L || x > 2147483647L))
            RANGE_ERROR(state, f, 0);
#endif
    }
    do {
        *q++ = (unsigned char)(x & 0xffL);
        x >>= 8;
    } while (--i > 0);
    return 0;
}

static int
lp_uint(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    unsigned long x;
    Py_ssize_t i;
    unsigned char *q = (unsigned char *)p;
    if (get_ulong(state, v, &x) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            RANGE_ERROR(state, f, 1);
        }
        return -1;
    }
    i = f->size;
    if (i != SIZEOF_LONG) {
        unsigned long maxint = 1;
        maxint <<= (unsigned long)(i * 8);
        if (x >= maxint)
            RANGE_ERROR(state, f, 1);
    }
    do {
        *q++ = (unsigned char)(x & 0xffUL);
        x >>= 8;
    } while (--i > 0);
    return 0;
}

static int
lp_longlong(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    int res;
    v = get_pylong(state, v);
    if (v == NULL)
        return -1;
    res = _PyLong_AsByteArray((PyLongObject*)v,
                              (unsigned char *)p,
                              8,
                              1, /* little_endian */
                              1  /* signed */);
    Py_DECREF(v);
    if (res == -1 && PyErr_Occurred()) {
        PyErr_Format(state->StructError,
                     "'%c' format requires %lld <= number <= %lld",
                     f->format,
                     LLONG_MIN,
                     LLONG_MAX);
        return -1;
    }
    return res;
}

static int
lp_ulonglong(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    int res;
    v = get_pylong(state, v);
    if (v == NULL)
        return -1;
    res = _PyLong_AsByteArray((PyLongObject*)v,
                              (unsigned char *)p,
                              8,
                              1, /* little_endian */
                              0  /* signed */);
    Py_DECREF(v);
    if (res == -1 && PyErr_Occurred()) {
        PyErr_Format(state->StructError,
                     "'%c' format requires 0 <= number <= %llu",
                     f->format,
                     ULLONG_MAX);
        return -1;
    }
    return res;
}

static int
lp_halffloat(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    return pack_halffloat(state, p, v, 1);
}

static int
lp_float(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    double x = PyFloat_AsDouble(v);
    if (x == -1 && PyErr_Occurred()) {
        PyErr_SetString(state->StructError,
                        "required argument is not a float");
        return -1;
    }
    return PyFloat_Pack4(x, p, 1);
}

static int
lp_double(_structmodulestate *state, char *p, PyObject *v, const formatdef *f)
{
    double x = PyFloat_AsDouble(v);
    if (x == -1 && PyErr_Occurred()) {
        PyErr_SetString(state->StructError,
                        "required argument is not a float");
        return -1;
    }
    return PyFloat_Pack8(x, p, 1);
}

static formatdef lilendian_table[] = {
    {'x',       1,              0,              NULL},
    {'b',       1,              0,              nu_byte,        np_byte},
    {'B',       1,              0,              nu_ubyte,       np_ubyte},
    {'c',       1,              0,              nu_char,        np_char},
    {'s',       1,              0,              NULL},
    {'p',       1,              0,              NULL},
    {'h',       2,              0,              lu_short,       lp_int},
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
getentry(_structmodulestate *state, int c, const formatdef *f)
{
    for (; f->format != '\0'; f++) {
        if (f->format == c) {
            return f;
        }
    }
    PyErr_SetString(state->StructError, "bad char in struct format");
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

    _structmodulestate *state = get_struct_state_structinst(self);

    fmt = PyBytes_AS_STRING(self->s_format);
    if (strlen(fmt) != (size_t)PyBytes_GET_SIZE(self->s_format)) {
        PyErr_SetString(state->StructError,
                        "embedded null character");
        return -1;
    }

    f = whichtable(&fmt);

    s = fmt;
    size = 0;
    len = 0;
    ncodes = 0;
    while ((c = *s++) != '\0') {
        if (Py_ISSPACE(c))
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
                PyErr_SetString(state->StructError,
                                "repeat count given without format specifier");
                return -1;
            }
        }
        else
            num = 1;

        e = getentry(state, c, f);
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
    codes = PyMem_Malloc((ncodes + 1) * sizeof(formatcode));
    if (codes == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    /* Free any s_codes value left over from a previous initialization. */
    if (self->s_codes != NULL)
        PyMem_Free(self->s_codes);
    self->s_codes = codes;

    s = fmt;
    size = 0;
    while ((c = *s++) != '\0') {
        if (Py_ISSPACE(c))
            continue;
        if ('0' <= c && c <= '9') {
            num = c - '0';
            while ('0' <= (c = *s++) && c <= '9')
                num = num*10 + (c - '0');
        }
        else
            num = 1;

        e = getentry(state, c, f);

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
    PyErr_SetString(state->StructError,
                    "total struct size too long");
    return -1;
}

static PyObject *
s_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *self;

    assert(type != NULL);
    allocfunc alloc_func = PyType_GetSlot(type, Py_tp_alloc);
    assert(alloc_func != NULL);

    self = alloc_func(type, 0);
    if (self != NULL) {
        PyStructObject *s = (PyStructObject*)self;
        s->s_format = Py_NewRef(Py_None);
        s->s_codes = NULL;
        s->s_size = -1;
        s->s_len = -1;
    }
    return self;
}

/*[clinic input]
Struct.__init__

    format: object

Create a compiled struct object.

Return a new Struct object which writes and reads binary data according to
the format string.

See help(struct) for more on format strings.
[clinic start generated code]*/

static int
Struct___init___impl(PyStructObject *self, PyObject *format)
/*[clinic end generated code: output=b8e80862444e92d0 input=192a4575a3dde802]*/
{
    int ret = 0;

    if (PyUnicode_Check(format)) {
        format = PyUnicode_AsASCIIString(format);
        if (format == NULL)
            return -1;
    }
    else {
        Py_INCREF(format);
    }

    if (!PyBytes_Check(format)) {
        Py_DECREF(format);
        PyErr_Format(PyExc_TypeError,
                     "Struct() argument 1 must be a str or bytes object, "
                     "not %.200s",
                     _PyType_Name(Py_TYPE(format)));
        return -1;
    }

    Py_SETREF(self->s_format, format);

    ret = prepare_s(self);
    return ret;
}

static int
s_clear(PyStructObject *s)
{
    Py_CLEAR(s->s_format);
    return 0;
}

static int
s_traverse(PyStructObject *s, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(s));
    Py_VISIT(s->s_format);
    return 0;
}

static void
s_dealloc(PyStructObject *s)
{
    PyTypeObject *tp = Py_TYPE(s);
    PyObject_GC_UnTrack(s);
    if (s->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)s);
    if (s->s_codes != NULL) {
        PyMem_Free(s->s_codes);
    }
    Py_XDECREF(s->s_format);
    freefunc free_func = PyType_GetSlot(Py_TYPE(s), Py_tp_free);
    free_func(s);
    Py_DECREF(tp);
}

static PyObject *
s_unpack_internal(PyStructObject *soself, const char *startfrom,
                  _structmodulestate *state) {
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
                v = e->unpack(state, res, e);
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


/*[clinic input]
Struct.unpack

    buffer: Py_buffer
    /

Return a tuple containing unpacked values.

Unpack according to the format string Struct.format. The buffer's size
in bytes must be Struct.size.

See help(struct) for more on format strings.
[clinic start generated code]*/

static PyObject *
Struct_unpack_impl(PyStructObject *self, Py_buffer *buffer)
/*[clinic end generated code: output=873a24faf02e848a input=3113f8e7038b2f6c]*/
{
    _structmodulestate *state = get_struct_state_structinst(self);
    assert(self->s_codes != NULL);
    if (buffer->len != self->s_size) {
        PyErr_Format(state->StructError,
                     "unpack requires a buffer of %zd bytes",
                     self->s_size);
        return NULL;
    }
    return s_unpack_internal(self, buffer->buf, state);
}

/*[clinic input]
Struct.unpack_from

    buffer: Py_buffer
    offset: Py_ssize_t = 0

Return a tuple containing unpacked values.

Values are unpacked according to the format string Struct.format.

The buffer's size in bytes, starting at position offset, must be
at least Struct.size.

See help(struct) for more on format strings.
[clinic start generated code]*/

static PyObject *
Struct_unpack_from_impl(PyStructObject *self, Py_buffer *buffer,
                        Py_ssize_t offset)
/*[clinic end generated code: output=57fac875e0977316 input=cafd4851d473c894]*/
{
    _structmodulestate *state = get_struct_state_structinst(self);
    assert(self->s_codes != NULL);

    if (offset < 0) {
        if (offset + self->s_size > 0) {
            PyErr_Format(state->StructError,
                         "not enough data to unpack %zd bytes at offset %zd",
                         self->s_size,
                         offset);
            return NULL;
        }

        if (offset + buffer->len < 0) {
            PyErr_Format(state->StructError,
                         "offset %zd out of range for %zd-byte buffer",
                         offset,
                         buffer->len);
            return NULL;
        }
        offset += buffer->len;
    }

    if ((buffer->len - offset) < self->s_size) {
        PyErr_Format(state->StructError,
                     "unpack_from requires a buffer of at least %zu bytes for "
                     "unpacking %zd bytes at offset %zd "
                     "(actual buffer size is %zd)",
                     (size_t)self->s_size + (size_t)offset,
                     self->s_size,
                     offset,
                     buffer->len);
        return NULL;
    }
    return s_unpack_internal(self, (char*)buffer->buf + offset, state);
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
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->so);
    PyBuffer_Release(&self->buf);
    PyObject_GC_Del(self);
    Py_DECREF(tp);
}

static int
unpackiter_traverse(unpackiterobject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->so);
    Py_VISIT(self->buf.obj);
    return 0;
}

static PyObject *
unpackiter_len(unpackiterobject *self, PyObject *Py_UNUSED(ignored))
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
    _structmodulestate *state = get_struct_state_iterinst(self);
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
                               (char*) self->buf.buf + self->index,
                               state);
    self->index += self->so->s_size;
    return result;
}

static PyType_Slot unpackiter_type_slots[] = {
    {Py_tp_dealloc, unpackiter_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_traverse, unpackiter_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, unpackiter_iternext},
    {Py_tp_methods, unpackiter_methods},
    {0, 0},
};

static PyType_Spec unpackiter_type_spec = {
    "_struct.unpack_iterator",
    sizeof(unpackiterobject),
    0,
    (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
     Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    unpackiter_type_slots
};

/*[clinic input]
Struct.iter_unpack

    buffer: object
    /

Return an iterator yielding tuples.

Tuples are unpacked from the given bytes source, like a repeated
invocation of unpack_from().

Requires that the bytes length be a multiple of the struct size.
[clinic start generated code]*/

static PyObject *
Struct_iter_unpack(PyStructObject *self, PyObject *buffer)
/*[clinic end generated code: output=172d83d0cd15dbab input=6d65b3f3107dbc99]*/
{
    _structmodulestate *state = get_struct_state_structinst(self);
    unpackiterobject *iter;

    assert(self->s_codes != NULL);

    if (self->s_size == 0) {
        PyErr_Format(state->StructError,
                     "cannot iteratively unpack with a struct of length 0");
        return NULL;
    }

    iter = (unpackiterobject *) PyType_GenericAlloc((PyTypeObject *)state->unpackiter_type, 0);
    if (iter == NULL)
        return NULL;

    if (PyObject_GetBuffer(buffer, &iter->buf, PyBUF_SIMPLE) < 0) {
        Py_DECREF(iter);
        return NULL;
    }
    if (iter->buf.len % self->s_size != 0) {
        PyErr_Format(state->StructError,
                     "iterative unpacking requires a buffer of "
                     "a multiple of %zd bytes",
                     self->s_size);
        Py_DECREF(iter);
        return NULL;
    }
    iter->so = (PyStructObject*)Py_NewRef(self);
    iter->index = 0;
    return (PyObject *)iter;
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
s_pack_internal(PyStructObject *soself, PyObject *const *args, int offset,
                char* buf, _structmodulestate *state)
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
            PyObject *v = args[i++];
            if (e->format == 's') {
                Py_ssize_t n;
                int isstring;
                const void *p;
                isstring = PyBytes_Check(v);
                if (!isstring && !PyByteArray_Check(v)) {
                    PyErr_SetString(state->StructError,
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
                const void *p;
                isstring = PyBytes_Check(v);
                if (!isstring && !PyByteArray_Check(v)) {
                    PyErr_SetString(state->StructError,
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
                if (e->pack(state, res, v, e) < 0) {
                    if (PyLong_Check(v) && PyErr_ExceptionMatches(PyExc_OverflowError))
                        PyErr_SetString(state->StructError,
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
s_pack(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    char *buf;
    PyStructObject *soself;
    _structmodulestate *state = get_struct_state_structinst(self);

    /* Validate arguments. */
    soself = (PyStructObject *)self;
    assert(PyStruct_Check(self, state));
    assert(soself->s_codes != NULL);
    if (nargs != soself->s_len)
    {
        PyErr_Format(state->StructError,
            "pack expected %zd items for packing (got %zd)", soself->s_len, nargs);
        return NULL;
    }

    /* Allocate a new string */
    _PyBytesWriter writer;
    _PyBytesWriter_Init(&writer);
    buf = _PyBytesWriter_Alloc(&writer, soself->s_size);
    if (buf == NULL) {
        _PyBytesWriter_Dealloc(&writer);
        return NULL;
    }

    /* Call the guts */
    if ( s_pack_internal(soself, args, 0, buf, state) != 0 ) {
        _PyBytesWriter_Dealloc(&writer);
        return NULL;
    }

    return _PyBytesWriter_Finish(&writer, buf + soself->s_size);
}

PyDoc_STRVAR(s_pack_into__doc__,
"S.pack_into(buffer, offset, v1, v2, ...)\n\
\n\
Pack the values v1, v2, ... according to the format string S.format\n\
and write the packed bytes into the writable buffer buf starting at\n\
offset.  Note that the offset is a required argument.  See\n\
help(struct) for more on format strings.");

static PyObject *
s_pack_into(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyStructObject *soself;
    Py_buffer buffer;
    Py_ssize_t offset;
    _structmodulestate *state = get_struct_state_structinst(self);

    /* Validate arguments.  +1 is for the first arg as buffer. */
    soself = (PyStructObject *)self;
    assert(PyStruct_Check(self, state));
    assert(soself->s_codes != NULL);
    if (nargs != (soself->s_len + 2))
    {
        if (nargs == 0) {
            PyErr_Format(state->StructError,
                        "pack_into expected buffer argument");
        }
        else if (nargs == 1) {
            PyErr_Format(state->StructError,
                        "pack_into expected offset argument");
        }
        else {
            PyErr_Format(state->StructError,
                        "pack_into expected %zd items for packing (got %zd)",
                        soself->s_len, (nargs - 2));
        }
        return NULL;
    }

    /* Extract a writable memory buffer from the first argument */
    if (!PyArg_Parse(args[0], "w*", &buffer))
        return NULL;
    assert(buffer.len >= 0);

    /* Extract the offset from the first argument */
    offset = PyNumber_AsSsize_t(args[1], PyExc_IndexError);
    if (offset == -1 && PyErr_Occurred()) {
        PyBuffer_Release(&buffer);
        return NULL;
    }

    /* Support negative offsets. */
    if (offset < 0) {
         /* Check that negative offset is low enough to fit data */
        if (offset + soself->s_size > 0) {
            PyErr_Format(state->StructError,
                         "no space to pack %zd bytes at offset %zd",
                         soself->s_size,
                         offset);
            PyBuffer_Release(&buffer);
            return NULL;
        }

        /* Check that negative offset is not crossing buffer boundary */
        if (offset + buffer.len < 0) {
            PyErr_Format(state->StructError,
                         "offset %zd out of range for %zd-byte buffer",
                         offset,
                         buffer.len);
            PyBuffer_Release(&buffer);
            return NULL;
        }

        offset += buffer.len;
    }

    /* Check boundaries */
    if ((buffer.len - offset) < soself->s_size) {
        assert(offset >= 0);
        assert(soself->s_size >= 0);

        PyErr_Format(state->StructError,
                     "pack_into requires a buffer of at least %zu bytes for "
                     "packing %zd bytes at offset %zd "
                     "(actual buffer size is %zd)",
                     (size_t)soself->s_size + (size_t)offset,
                     soself->s_size,
                     offset,
                     buffer.len);
        PyBuffer_Release(&buffer);
        return NULL;
    }

    /* Call the guts */
    if (s_pack_internal(soself, args, 2, (char*)buffer.buf + offset, state) != 0) {
        PyBuffer_Release(&buffer);
        return NULL;
    }

    PyBuffer_Release(&buffer);
    Py_RETURN_NONE;
}

static PyObject *
s_get_format(PyStructObject *self, void *unused)
{
    return PyUnicode_FromStringAndSize(PyBytes_AS_STRING(self->s_format),
                                       PyBytes_GET_SIZE(self->s_format));
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
    size_t size = _PyObject_SIZE(Py_TYPE(self)) + sizeof(formatcode);
    for (formatcode *code = self->s_codes; code->fmtdef != NULL; code++) {
        size += sizeof(formatcode);
    }
    return PyLong_FromSize_t(size);
}

static PyObject *
s_repr(PyStructObject *self)
{
    PyObject* fmt = PyUnicode_FromStringAndSize(
        PyBytes_AS_STRING(self->s_format), PyBytes_GET_SIZE(self->s_format));
    if (fmt == NULL) {
        return NULL;
    }
    PyObject* s = PyUnicode_FromFormat("%s(%R)", _PyType_Name(Py_TYPE(self)), fmt);
    Py_DECREF(fmt);
    return s;
}

/* List of functions */

static struct PyMethodDef s_methods[] = {
    STRUCT_ITER_UNPACK_METHODDEF
    {"pack",            _PyCFunction_CAST(s_pack), METH_FASTCALL, s_pack__doc__},
    {"pack_into",       _PyCFunction_CAST(s_pack_into), METH_FASTCALL, s_pack_into__doc__},
    STRUCT_UNPACK_METHODDEF
    STRUCT_UNPACK_FROM_METHODDEF
    {"__sizeof__",      (PyCFunction)s_sizeof, METH_NOARGS, s_sizeof__doc__},
    {NULL,       NULL}          /* sentinel */
};

static PyMemberDef s_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(PyStructObject, weakreflist), Py_READONLY},
    {NULL}  /* sentinel */
};

static PyGetSetDef s_getsetlist[] = {
    {"format", (getter)s_get_format, (setter)NULL, PyDoc_STR("struct format string"), NULL},
    {"size", (getter)s_get_size, (setter)NULL, PyDoc_STR("struct size in bytes"), NULL},
    {NULL} /* sentinel */
};

PyDoc_STRVAR(s__doc__,
"Struct(fmt) --> compiled struct object\n"
"\n"
);

static PyType_Slot PyStructType_slots[] = {
    {Py_tp_dealloc, s_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_setattro, PyObject_GenericSetAttr},
    {Py_tp_repr, s_repr},
    {Py_tp_doc, (void*)s__doc__},
    {Py_tp_traverse, s_traverse},
    {Py_tp_clear, s_clear},
    {Py_tp_methods, s_methods},
    {Py_tp_members, s_members},
    {Py_tp_getset, s_getsetlist},
    {Py_tp_init, Struct___init__},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_new, s_new},
    {Py_tp_free, PyObject_GC_Del},
    {0, 0},
};

static PyType_Spec PyStructType_spec = {
    "_struct.Struct",
    sizeof(PyStructObject),
    0,
    (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
     Py_TPFLAGS_BASETYPE | Py_TPFLAGS_IMMUTABLETYPE),
    PyStructType_slots
};


/* ---- Standalone functions  ---- */

#define MAXCACHE 100

static int
cache_struct_converter(PyObject *module, PyObject *fmt, PyStructObject **ptr)
{
    PyObject * s_object;
    _structmodulestate *state = get_struct_state(module);

    if (fmt == NULL) {
        Py_SETREF(*ptr, NULL);
        return 1;
    }

    if (PyDict_GetItemRef(state->cache, fmt, &s_object) < 0) {
        return 0;
    }
    if (s_object != NULL) {
        *ptr = (PyStructObject *)s_object;
        return Py_CLEANUP_SUPPORTED;
    }

    s_object = PyObject_CallOneArg(state->PyStructType, fmt);
    if (s_object != NULL) {
        if (PyDict_GET_SIZE(state->cache) >= MAXCACHE)
            PyDict_Clear(state->cache);
        /* Attempt to cache the result */
        if (PyDict_SetItem(state->cache, fmt, s_object) == -1)
            PyErr_Clear();
        *ptr = (PyStructObject *)s_object;
        return Py_CLEANUP_SUPPORTED;
    }
    return 0;
}

/*[clinic input]
_clearcache

Clear the internal cache.
[clinic start generated code]*/

static PyObject *
_clearcache_impl(PyObject *module)
/*[clinic end generated code: output=ce4fb8a7bf7cb523 input=463eaae04bab3211]*/
{
    PyDict_Clear(get_struct_state(module)->cache);
    Py_RETURN_NONE;
}


/*[clinic input]
calcsize -> Py_ssize_t

    format as s_object: cache_struct
    /

Return size in bytes of the struct described by the format string.
[clinic start generated code]*/

static Py_ssize_t
calcsize_impl(PyObject *module, PyStructObject *s_object)
/*[clinic end generated code: output=db7d23d09c6932c4 input=96a6a590c7717ecd]*/
{
    return s_object->s_size;
}

PyDoc_STRVAR(pack_doc,
"pack(format, v1, v2, ...) -> bytes\n\
\n\
Return a bytes object containing the values v1, v2, ... packed according\n\
to the format string.  See help(struct) for more on format strings.");

static PyObject *
pack(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *s_object = NULL;
    PyObject *format, *result;

    if (nargs == 0) {
        PyErr_SetString(PyExc_TypeError, "missing format argument");
        return NULL;
    }
    format = args[0];

    if (!cache_struct_converter(module, format, (PyStructObject **)&s_object)) {
        return NULL;
    }
    result = s_pack(s_object, args + 1, nargs - 1);
    Py_DECREF(s_object);
    return result;
}

PyDoc_STRVAR(pack_into_doc,
"pack_into(format, buffer, offset, v1, v2, ...)\n\
\n\
Pack the values v1, v2, ... according to the format string and write\n\
the packed bytes into the writable buffer buf starting at offset.  Note\n\
that the offset is a required argument.  See help(struct) for more\n\
on format strings.");

static PyObject *
pack_into(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *s_object = NULL;
    PyObject *format, *result;

    if (nargs == 0) {
        PyErr_SetString(PyExc_TypeError, "missing format argument");
        return NULL;
    }
    format = args[0];

    if (!cache_struct_converter(module, format, (PyStructObject **)&s_object)) {
        return NULL;
    }
    result = s_pack_into(s_object, args + 1, nargs - 1);
    Py_DECREF(s_object);
    return result;
}

/*[clinic input]
unpack

    format as s_object: cache_struct
    buffer: Py_buffer
    /

Return a tuple containing values unpacked according to the format string.

The buffer's size in bytes must be calcsize(format).

See help(struct) for more on format strings.
[clinic start generated code]*/

static PyObject *
unpack_impl(PyObject *module, PyStructObject *s_object, Py_buffer *buffer)
/*[clinic end generated code: output=48ddd4d88eca8551 input=05fa3b91678da727]*/
{
    return Struct_unpack_impl(s_object, buffer);
}

/*[clinic input]
unpack_from

    format as s_object: cache_struct
    /
    buffer: Py_buffer
    offset: Py_ssize_t = 0

Return a tuple containing values unpacked according to the format string.

The buffer's size, minus offset, must be at least calcsize(format).

See help(struct) for more on format strings.
[clinic start generated code]*/

static PyObject *
unpack_from_impl(PyObject *module, PyStructObject *s_object,
                 Py_buffer *buffer, Py_ssize_t offset)
/*[clinic end generated code: output=1042631674c6e0d3 input=6e80a5398e985025]*/
{
    return Struct_unpack_from_impl(s_object, buffer, offset);
}

/*[clinic input]
iter_unpack

    format as s_object: cache_struct
    buffer: object
    /

Return an iterator yielding tuples unpacked from the given bytes.

The bytes are unpacked according to the format string, like
a repeated invocation of unpack_from().

Requires that the bytes length be a multiple of the format struct size.
[clinic start generated code]*/

static PyObject *
iter_unpack_impl(PyObject *module, PyStructObject *s_object,
                 PyObject *buffer)
/*[clinic end generated code: output=0ae50e250d20e74d input=b214a58869a3c98d]*/
{
    return Struct_iter_unpack(s_object, buffer);
}

static struct PyMethodDef module_functions[] = {
    _CLEARCACHE_METHODDEF
    CALCSIZE_METHODDEF
    ITER_UNPACK_METHODDEF
    {"pack",            _PyCFunction_CAST(pack), METH_FASTCALL,   pack_doc},
    {"pack_into",       _PyCFunction_CAST(pack_into), METH_FASTCALL,   pack_into_doc},
    UNPACK_METHODDEF
    UNPACK_FROM_METHODDEF
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


static int
_structmodule_traverse(PyObject *module, visitproc visit, void *arg)
{
    _structmodulestate *state = get_struct_state(module);
    if (state) {
        Py_VISIT(state->cache);
        Py_VISIT(state->PyStructType);
        Py_VISIT(state->unpackiter_type);
        Py_VISIT(state->StructError);
    }
    return 0;
}

static int
_structmodule_clear(PyObject *module)
{
    _structmodulestate *state = get_struct_state(module);
    if (state) {
        Py_CLEAR(state->cache);
        Py_CLEAR(state->PyStructType);
        Py_CLEAR(state->unpackiter_type);
        Py_CLEAR(state->StructError);
    }
    return 0;
}

static void
_structmodule_free(void *module)
{
    _structmodule_clear((PyObject *)module);
}

static int
_structmodule_exec(PyObject *m)
{
    _structmodulestate *state = get_struct_state(m);

    state->cache = PyDict_New();
    if (state->cache == NULL) {
        return -1;
    }

    state->PyStructType = PyType_FromModuleAndSpec(
        m, &PyStructType_spec, NULL);
    if (state->PyStructType == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject *)state->PyStructType) < 0) {
        return -1;
    }

    state->unpackiter_type = PyType_FromModuleAndSpec(
        m, &unpackiter_type_spec, NULL);
    if (state->unpackiter_type == NULL) {
        return -1;
    }

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
                    /* Skip _Bool, semantics are different for standard size */
                    if (ptr->format == '?')
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
    state->StructError = PyErr_NewException("struct.error", NULL, NULL);
    if (state->StructError == NULL) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "error", state->StructError) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot _structmodule_slots[] = {
    {Py_mod_exec, _structmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static struct PyModuleDef _structmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_struct",
    .m_doc = module_doc,
    .m_size = sizeof(_structmodulestate),
    .m_methods = module_functions,
    .m_slots = _structmodule_slots,
    .m_traverse = _structmodule_traverse,
    .m_clear = _structmodule_clear,
    .m_free = _structmodule_free,
};

PyMODINIT_FUNC
PyInit__struct(void)
{
    return PyModuleDef_Init(&_structmodule);
}
