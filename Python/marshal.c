
/* Write Python objects to files and read them back.
   This is primarily intended for writing and reading compiled Python code,
   even though dicts, lists, sets and frozensets, not commonly seen in
   code objects, are supported.
   Version 3 of this protocol properly supports circular links
   and sharing. */

#include "Python.h"
#include "pycore_call.h"             // _PyObject_CallNoArgs()
#include "pycore_code.h"             // _PyCode_New()
#include "pycore_hashtable.h"        // _Py_hashtable_t
#include "pycore_long.h"             // _PyLong_IsZero()
#include "pycore_pystate.h"          // _PyInterpreterState_GET()
#include "pycore_setobject.h"        // _PySet_NextEntryRef()
#include "pycore_unicodeobject.h"    // _PyUnicode_InternImmortal()

#include "marshal.h"                 // Py_MARSHAL_VERSION

#ifdef __APPLE__
#  include "TargetConditionals.h"
#endif /* __APPLE__ */


/*[clinic input]
module marshal
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=c982b7930dee17db]*/

#include "clinic/marshal.c.h"

/* High water mark to determine when the marshalled object is dangerously deep
 * and risks coring the interpreter.  When the object stack gets this deep,
 * raise an exception instead of continuing.
 * On Windows debug builds, reduce this value.
 *
 * BUG: https://bugs.python.org/issue33720
 * On Windows PGO builds, the r_object function overallocates its stack and
 * can cause a stack overflow. We reduce the maximum depth for all Windows
 * releases to protect against this.
 * #if defined(MS_WINDOWS) && defined(Py_DEBUG)
 */
#if defined(MS_WINDOWS)
#  define MAX_MARSHAL_STACK_DEPTH 1000
#elif defined(__wasi__)
#  define MAX_MARSHAL_STACK_DEPTH 1500
// TARGET_OS_IPHONE covers any non-macOS Apple platform.
// It won't be defined on older macOS SDKs
#elif defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#  define MAX_MARSHAL_STACK_DEPTH 1500
#else
#  define MAX_MARSHAL_STACK_DEPTH 2000
#endif

/* Supported types */
#define TYPE_NULL               '0'
#define TYPE_NONE               'N'
#define TYPE_FALSE              'F'
#define TYPE_TRUE               'T'
#define TYPE_STOPITER           'S'
#define TYPE_ELLIPSIS           '.'
#define TYPE_BINARY_FLOAT       'g'  // Version 0 uses TYPE_FLOAT instead.
#define TYPE_BINARY_COMPLEX     'y'  // Version 0 uses TYPE_COMPLEX instead.
#define TYPE_LONG               'l'  // See also TYPE_INT.
#define TYPE_STRING             's'  // Bytes. (Name comes from Python 2.)
#define TYPE_TUPLE              '('  // See also TYPE_SMALL_TUPLE.
#define TYPE_LIST               '['
#define TYPE_DICT               '{'
#define TYPE_CODE               'c'
#define TYPE_UNICODE            'u'
#define TYPE_UNKNOWN            '?'
// added in version 2:
#define TYPE_SET                '<'
#define TYPE_FROZENSET          '>'
// added in version 5:
#define TYPE_SLICE              ':'
// Remember to update the version and documentation when adding new types.

/* Special cases for unicode strings (added in version 4) */
#define TYPE_INTERNED           't' // Version 1+
#define TYPE_ASCII              'a'
#define TYPE_ASCII_INTERNED     'A'
#define TYPE_SHORT_ASCII        'z'
#define TYPE_SHORT_ASCII_INTERNED 'Z'

/* Special cases for small objects */
#define TYPE_INT                'i'  // All versions. 32-bit encoding.
#define TYPE_SMALL_TUPLE        ')'  // Version 4+

/* Supported for backwards compatibility */
#define TYPE_COMPLEX            'x'  // Generated for version 0 only.
#define TYPE_FLOAT              'f'  // Generated for version 0 only.
#define TYPE_INT64              'I'  // Not generated any more.

/* References (added in version 3) */
#define TYPE_REF                'r'
#define FLAG_REF                '\x80' /* with a type, add obj to index */


// Error codes:
#define WFERR_OK 0
#define WFERR_UNMARSHALLABLE 1
#define WFERR_NESTEDTOODEEP 2
#define WFERR_NOMEMORY 3
#define WFERR_CODE_NOT_ALLOWED 4

typedef struct {
    FILE *fp;
    int error;  /* see WFERR_* values */
    int depth;
    PyObject *str;
    char *ptr;
    const char *end;
    char *buf;
    _Py_hashtable_t *hashtable;
    int version;
    int allow_code;
} WFILE;

#define w_byte(c, p) do {                               \
        if ((p)->ptr != (p)->end || w_reserve((p), 1))  \
            *(p)->ptr++ = (c);                          \
    } while(0)

static void
w_flush(WFILE *p)
{
    assert(p->fp != NULL);
    fwrite(p->buf, 1, p->ptr - p->buf, p->fp);
    p->ptr = p->buf;
}

static int
w_reserve(WFILE *p, Py_ssize_t needed)
{
    Py_ssize_t pos, size, delta;
    if (p->ptr == NULL)
        return 0; /* An error already occurred */
    if (p->fp != NULL) {
        w_flush(p);
        return needed <= p->end - p->ptr;
    }
    assert(p->str != NULL);
    pos = p->ptr - p->buf;
    size = PyBytes_GET_SIZE(p->str);
    if (size > 16*1024*1024)
        delta = (size >> 3);            /* 12.5% overallocation */
    else
        delta = size + 1024;
    delta = Py_MAX(delta, needed);
    if (delta > PY_SSIZE_T_MAX - size) {
        p->error = WFERR_NOMEMORY;
        return 0;
    }
    size += delta;
    if (_PyBytes_Resize(&p->str, size) != 0) {
        p->end = p->ptr = p->buf = NULL;
        return 0;
    }
    else {
        p->buf = PyBytes_AS_STRING(p->str);
        p->ptr = p->buf + pos;
        p->end = p->buf + size;
        return 1;
    }
}

static void
w_string(const void *s, Py_ssize_t n, WFILE *p)
{
    Py_ssize_t m;
    if (!n || p->ptr == NULL)
        return;
    m = p->end - p->ptr;
    if (p->fp != NULL) {
        if (n <= m) {
            memcpy(p->ptr, s, n);
            p->ptr += n;
        }
        else {
            w_flush(p);
            fwrite(s, 1, n, p->fp);
        }
    }
    else {
        if (n <= m || w_reserve(p, n - m)) {
            memcpy(p->ptr, s, n);
            p->ptr += n;
        }
    }
}

static void
w_short(int x, WFILE *p)
{
    w_byte((char)( x      & 0xff), p);
    w_byte((char)((x>> 8) & 0xff), p);
}

static void
w_long(long x, WFILE *p)
{
    w_byte((char)( x      & 0xff), p);
    w_byte((char)((x>> 8) & 0xff), p);
    w_byte((char)((x>>16) & 0xff), p);
    w_byte((char)((x>>24) & 0xff), p);
}

#define SIZE32_MAX  0x7FFFFFFF

#if SIZEOF_SIZE_T > 4
# define W_SIZE(n, p)  do {                     \
        if ((n) > SIZE32_MAX) {                 \
            (p)->depth--;                       \
            (p)->error = WFERR_UNMARSHALLABLE;  \
            return;                             \
        }                                       \
        w_long((long)(n), p);                   \
    } while(0)
#else
# define W_SIZE  w_long
#endif

static void
w_pstring(const void *s, Py_ssize_t n, WFILE *p)
{
        W_SIZE(n, p);
        w_string(s, n, p);
}

static void
w_short_pstring(const void *s, Py_ssize_t n, WFILE *p)
{
    w_byte(Py_SAFE_DOWNCAST(n, Py_ssize_t, unsigned char), p);
    w_string(s, n, p);
}

/* We assume that Python ints are stored internally in base some power of
   2**15; for the sake of portability we'll always read and write them in base
   exactly 2**15. */

#define PyLong_MARSHAL_SHIFT 15
#define PyLong_MARSHAL_BASE ((short)1 << PyLong_MARSHAL_SHIFT)
#define PyLong_MARSHAL_MASK (PyLong_MARSHAL_BASE - 1)

#define W_TYPE(t, p) do { \
    w_byte((t) | flag, (p)); \
} while(0)

static PyObject *
_PyMarshal_WriteObjectToString(PyObject *x, int version, int allow_code);

#define _r_digits(bitsize)                                                \
static void                                                               \
_r_digits##bitsize(const uint ## bitsize ## _t *digits, Py_ssize_t n,     \
                   uint8_t negative, Py_ssize_t marshal_ratio, WFILE *p)  \
{                                                                         \
    /* set l to number of base PyLong_MARSHAL_BASE digits */              \
    Py_ssize_t l = (n - 1)*marshal_ratio;                                 \
    uint ## bitsize ## _t d = digits[n - 1];                              \
                                                                          \
    assert(marshal_ratio > 0);                                            \
    assert(n >= 1);                                                       \
    assert(d != 0); /* a PyLong is always normalized */                   \
    do {                                                                  \
        d >>= PyLong_MARSHAL_SHIFT;                                       \
        l++;                                                              \
    } while (d != 0);                                                     \
    if (l > SIZE32_MAX) {                                                 \
        p->depth--;                                                       \
        p->error = WFERR_UNMARSHALLABLE;                                  \
        return;                                                           \
    }                                                                     \
    w_long((long)(negative ? -l : l), p);                                 \
                                                                          \
    for (Py_ssize_t i = 0; i < n - 1; i++) {                              \
        d = digits[i];                                                    \
        for (Py_ssize_t j = 0; j < marshal_ratio; j++) {                  \
            w_short(d & PyLong_MARSHAL_MASK, p);                          \
            d >>= PyLong_MARSHAL_SHIFT;                                   \
        }                                                                 \
        assert(d == 0);                                                   \
    }                                                                     \
    d = digits[n - 1];                                                    \
    do {                                                                  \
        w_short(d & PyLong_MARSHAL_MASK, p);                              \
        d >>= PyLong_MARSHAL_SHIFT;                                       \
    } while (d != 0);                                                     \
}
_r_digits(16)
_r_digits(32)
#undef _r_digits

static void
w_PyLong(const PyLongObject *ob, char flag, WFILE *p)
{
    W_TYPE(TYPE_LONG, p);
    if (_PyLong_IsZero(ob)) {
        w_long((long)0, p);
        return;
    }

    PyLongExport long_export;

    if (PyLong_Export((PyObject *)ob, &long_export) < 0) {
        p->depth--;
        p->error = WFERR_UNMARSHALLABLE;
        return;
    }
    if (!long_export.digits) {
        int8_t sign = long_export.value < 0 ? -1 : 1;
        uint64_t abs_value = Py_ABS(long_export.value);
        uint64_t d = abs_value;
        long l = 0;

        /* set l to number of base PyLong_MARSHAL_BASE digits */
        do {
            d >>= PyLong_MARSHAL_SHIFT;
            l += sign;
        } while (d);
        w_long(l, p);

        d = abs_value;
        do {
            w_short(d & PyLong_MARSHAL_MASK, p);
            d >>= PyLong_MARSHAL_SHIFT;
        } while (d);
        return;
    }

    const PyLongLayout *layout = PyLong_GetNativeLayout();
    Py_ssize_t marshal_ratio = layout->bits_per_digit/PyLong_MARSHAL_SHIFT;

    /* must be a multiple of PyLong_MARSHAL_SHIFT */
    assert(layout->bits_per_digit % PyLong_MARSHAL_SHIFT == 0);
    assert(layout->bits_per_digit >= PyLong_MARSHAL_SHIFT);

    /* other assumptions on PyLongObject internals */
    assert(layout->bits_per_digit <= 32);
    assert(layout->digits_order == -1);
    assert(layout->digit_endianness == (PY_LITTLE_ENDIAN ? -1 : 1));
    assert(layout->digit_size == 2 || layout->digit_size == 4);

    if (layout->digit_size == 4) {
        _r_digits32(long_export.digits, long_export.ndigits,
                    long_export.negative, marshal_ratio, p);
    }
    else {
        _r_digits16(long_export.digits, long_export.ndigits,
                    long_export.negative, marshal_ratio, p);
    }
    PyLong_FreeExport(&long_export);
}

static void
w_float_bin(double v, WFILE *p)
{
    char buf[8];
    if (PyFloat_Pack8(v, buf, 1) < 0) {
        p->error = WFERR_UNMARSHALLABLE;
        return;
    }
    w_string(buf, 8, p);
}

static void
w_float_str(double v, WFILE *p)
{
    char *buf = PyOS_double_to_string(v, 'g', 17, 0, NULL);
    if (!buf) {
        p->error = WFERR_NOMEMORY;
        return;
    }
    w_short_pstring(buf, strlen(buf), p);
    PyMem_Free(buf);
}

static int
w_ref(PyObject *v, char *flag, WFILE *p)
{
    _Py_hashtable_entry_t *entry;
    int w;

    if (p->version < 3 || p->hashtable == NULL)
        return 0; /* not writing object references */

    /* If it has only one reference, it definitely isn't shared.
     * But we use TYPE_REF always for interned string, to PYC file stable
     * as possible.
     */
    if (Py_REFCNT(v) == 1 &&
            !(PyUnicode_CheckExact(v) && PyUnicode_CHECK_INTERNED(v))) {
        return 0;
    }

    entry = _Py_hashtable_get_entry(p->hashtable, v);
    if (entry != NULL) {
        /* write the reference index to the stream */
        w = (int)(uintptr_t)entry->value;
        /* we don't store "long" indices in the dict */
        assert(0 <= w && w <= 0x7fffffff);
        w_byte(TYPE_REF, p);
        w_long(w, p);
        return 1;
    } else {
        size_t s = p->hashtable->nentries;
        /* we don't support long indices */
        if (s >= 0x7fffffff) {
            PyErr_SetString(PyExc_ValueError, "too many objects");
            goto err;
        }
        w = (int)s;
        if (_Py_hashtable_set(p->hashtable, Py_NewRef(v),
                              (void *)(uintptr_t)w) < 0) {
            Py_DECREF(v);
            goto err;
        }
        *flag |= FLAG_REF;
        return 0;
    }
err:
    p->error = WFERR_UNMARSHALLABLE;
    return 1;
}

static void
w_complex_object(PyObject *v, char flag, WFILE *p);

static void
w_object(PyObject *v, WFILE *p)
{
    char flag = '\0';

    p->depth++;

    if (p->depth > MAX_MARSHAL_STACK_DEPTH) {
        p->error = WFERR_NESTEDTOODEEP;
    }
    else if (v == NULL) {
        w_byte(TYPE_NULL, p);
    }
    else if (v == Py_None) {
        w_byte(TYPE_NONE, p);
    }
    else if (v == PyExc_StopIteration) {
        w_byte(TYPE_STOPITER, p);
    }
    else if (v == Py_Ellipsis) {
        w_byte(TYPE_ELLIPSIS, p);
    }
    else if (v == Py_False) {
        w_byte(TYPE_FALSE, p);
    }
    else if (v == Py_True) {
        w_byte(TYPE_TRUE, p);
    }
    else if (!w_ref(v, &flag, p))
        w_complex_object(v, flag, p);

    p->depth--;
}

static void
w_complex_object(PyObject *v, char flag, WFILE *p)
{
    Py_ssize_t i, n;

    if (PyLong_CheckExact(v)) {
        int overflow;
        long x = PyLong_AsLongAndOverflow(v, &overflow);
        if (overflow) {
            w_PyLong((PyLongObject *)v, flag, p);
        }
        else {
#if SIZEOF_LONG > 4
            long y = Py_ARITHMETIC_RIGHT_SHIFT(long, x, 31);
            if (y && y != -1) {
                /* Too large for TYPE_INT */
                w_PyLong((PyLongObject*)v, flag, p);
            }
            else
#endif
            {
                W_TYPE(TYPE_INT, p);
                w_long(x, p);
            }
        }
    }
    else if (PyFloat_CheckExact(v)) {
        if (p->version > 1) {
            W_TYPE(TYPE_BINARY_FLOAT, p);
            w_float_bin(PyFloat_AS_DOUBLE(v), p);
        }
        else {
            W_TYPE(TYPE_FLOAT, p);
            w_float_str(PyFloat_AS_DOUBLE(v), p);
        }
    }
    else if (PyComplex_CheckExact(v)) {
        if (p->version > 1) {
            W_TYPE(TYPE_BINARY_COMPLEX, p);
            w_float_bin(PyComplex_RealAsDouble(v), p);
            w_float_bin(PyComplex_ImagAsDouble(v), p);
        }
        else {
            W_TYPE(TYPE_COMPLEX, p);
            w_float_str(PyComplex_RealAsDouble(v), p);
            w_float_str(PyComplex_ImagAsDouble(v), p);
        }
    }
    else if (PyBytes_CheckExact(v)) {
        W_TYPE(TYPE_STRING, p);
        w_pstring(PyBytes_AS_STRING(v), PyBytes_GET_SIZE(v), p);
    }
    else if (PyUnicode_CheckExact(v)) {
        if (p->version >= 4 && PyUnicode_IS_ASCII(v)) {
            int is_short = PyUnicode_GET_LENGTH(v) < 256;
            if (is_short) {
                if (PyUnicode_CHECK_INTERNED(v))
                    W_TYPE(TYPE_SHORT_ASCII_INTERNED, p);
                else
                    W_TYPE(TYPE_SHORT_ASCII, p);
                w_short_pstring(PyUnicode_1BYTE_DATA(v),
                                PyUnicode_GET_LENGTH(v), p);
            }
            else {
                if (PyUnicode_CHECK_INTERNED(v))
                    W_TYPE(TYPE_ASCII_INTERNED, p);
                else
                    W_TYPE(TYPE_ASCII, p);
                w_pstring(PyUnicode_1BYTE_DATA(v),
                          PyUnicode_GET_LENGTH(v), p);
            }
        }
        else {
            PyObject *utf8;
            utf8 = PyUnicode_AsEncodedString(v, "utf8", "surrogatepass");
            if (utf8 == NULL) {
                p->depth--;
                p->error = WFERR_UNMARSHALLABLE;
                return;
            }
            if (p->version >= 3 &&  PyUnicode_CHECK_INTERNED(v))
                W_TYPE(TYPE_INTERNED, p);
            else
                W_TYPE(TYPE_UNICODE, p);
            w_pstring(PyBytes_AS_STRING(utf8), PyBytes_GET_SIZE(utf8), p);
            Py_DECREF(utf8);
        }
    }
    else if (PyTuple_CheckExact(v)) {
        n = PyTuple_GET_SIZE(v);
        if (p->version >= 4 && n < 256) {
            W_TYPE(TYPE_SMALL_TUPLE, p);
            w_byte((unsigned char)n, p);
        }
        else {
            W_TYPE(TYPE_TUPLE, p);
            W_SIZE(n, p);
        }
        for (i = 0; i < n; i++) {
            w_object(PyTuple_GET_ITEM(v, i), p);
        }
    }
    else if (PyList_CheckExact(v)) {
        W_TYPE(TYPE_LIST, p);
        n = PyList_GET_SIZE(v);
        W_SIZE(n, p);
        for (i = 0; i < n; i++) {
            w_object(PyList_GET_ITEM(v, i), p);
        }
    }
    else if (PyDict_CheckExact(v)) {
        Py_ssize_t pos;
        PyObject *key, *value;
        W_TYPE(TYPE_DICT, p);
        /* This one is NULL object terminated! */
        pos = 0;
        while (PyDict_Next(v, &pos, &key, &value)) {
            w_object(key, p);
            w_object(value, p);
        }
        w_object((PyObject *)NULL, p);
    }
    else if (PyAnySet_CheckExact(v)) {
        PyObject *value;
        Py_ssize_t pos = 0;
        Py_hash_t hash;

        if (PyFrozenSet_CheckExact(v))
            W_TYPE(TYPE_FROZENSET, p);
        else
            W_TYPE(TYPE_SET, p);
        n = PySet_GET_SIZE(v);
        W_SIZE(n, p);
        // bpo-37596: To support reproducible builds, sets and frozensets need
        // to have their elements serialized in a consistent order (even when
        // they have been scrambled by hash randomization). To ensure this, we
        // use an order equivalent to sorted(v, key=marshal.dumps):
        PyObject *pairs = PyList_New(n);
        if (pairs == NULL) {
            p->error = WFERR_NOMEMORY;
            return;
        }
        Py_ssize_t i = 0;
        Py_BEGIN_CRITICAL_SECTION(v);
        while (_PySet_NextEntryRef(v, &pos, &value, &hash)) {
            PyObject *dump = _PyMarshal_WriteObjectToString(value,
                                    p->version, p->allow_code);
            if (dump == NULL) {
                p->error = WFERR_UNMARSHALLABLE;
                Py_DECREF(value);
                break;
            }
            PyObject *pair = PyTuple_Pack(2, dump, value);
            Py_DECREF(dump);
            Py_DECREF(value);
            if (pair == NULL) {
                p->error = WFERR_NOMEMORY;
                break;
            }
            PyList_SET_ITEM(pairs, i++, pair);
        }
        Py_END_CRITICAL_SECTION();
        if (p->error == WFERR_UNMARSHALLABLE || p->error == WFERR_NOMEMORY) {
            Py_DECREF(pairs);
            return;
        }
        assert(i == n);
        if (PyList_Sort(pairs)) {
            p->error = WFERR_NOMEMORY;
            Py_DECREF(pairs);
            return;
        }
        for (Py_ssize_t i = 0; i < n; i++) {
            PyObject *pair = PyList_GET_ITEM(pairs, i);
            value = PyTuple_GET_ITEM(pair, 1);
            w_object(value, p);
        }
        Py_DECREF(pairs);
    }
    else if (PyCode_Check(v)) {
        if (!p->allow_code) {
            p->error = WFERR_CODE_NOT_ALLOWED;
            return;
        }
        PyCodeObject *co = (PyCodeObject *)v;
        PyObject *co_code = _PyCode_GetCode(co);
        if (co_code == NULL) {
            p->error = WFERR_NOMEMORY;
            return;
        }
        W_TYPE(TYPE_CODE, p);
        w_long(co->co_argcount, p);
        w_long(co->co_posonlyargcount, p);
        w_long(co->co_kwonlyargcount, p);
        w_long(co->co_stacksize, p);
        w_long(co->co_flags, p);
        w_object(co_code, p);
        w_object(co->co_consts, p);
        w_object(co->co_names, p);
        w_object(co->co_localsplusnames, p);
        w_object(co->co_localspluskinds, p);
        w_object(co->co_filename, p);
        w_object(co->co_name, p);
        w_object(co->co_qualname, p);
        w_long(co->co_firstlineno, p);
        w_object(co->co_linetable, p);
        w_object(co->co_exceptiontable, p);
        Py_DECREF(co_code);
    }
    else if (PyObject_CheckBuffer(v)) {
        /* Write unknown bytes-like objects as a bytes object */
        Py_buffer view;
        if (PyObject_GetBuffer(v, &view, PyBUF_SIMPLE) != 0) {
            w_byte(TYPE_UNKNOWN, p);
            p->depth--;
            p->error = WFERR_UNMARSHALLABLE;
            return;
        }
        W_TYPE(TYPE_STRING, p);
        w_pstring(view.buf, view.len, p);
        PyBuffer_Release(&view);
    }
    else if (PySlice_Check(v)) {
        if (p->version < 5) {
            w_byte(TYPE_UNKNOWN, p);
            p->error = WFERR_UNMARSHALLABLE;
            return;
        }
        PySliceObject *slice = (PySliceObject *)v;
        W_TYPE(TYPE_SLICE, p);
        w_object(slice->start, p);
        w_object(slice->stop, p);
        w_object(slice->step, p);
    }
    else {
        W_TYPE(TYPE_UNKNOWN, p);
        p->error = WFERR_UNMARSHALLABLE;
    }
}

static void
w_decref_entry(void *key)
{
    PyObject *entry_key = (PyObject *)key;
    Py_XDECREF(entry_key);
}

static int
w_init_refs(WFILE *wf, int version)
{
    if (version >= 3) {
        wf->hashtable = _Py_hashtable_new_full(_Py_hashtable_hash_ptr,
                                               _Py_hashtable_compare_direct,
                                               w_decref_entry, NULL, NULL);
        if (wf->hashtable == NULL) {
            PyErr_NoMemory();
            return -1;
        }
    }
    return 0;
}

static void
w_clear_refs(WFILE *wf)
{
    if (wf->hashtable != NULL) {
        _Py_hashtable_destroy(wf->hashtable);
    }
}

/* version currently has no effect for writing ints. */
/* Note that while the documentation states that this function
 * can error, currently it never does. Setting an exception in
 * this function should be regarded as an API-breaking change.
 */
void
PyMarshal_WriteLongToFile(long x, FILE *fp, int version)
{
    char buf[4];
    WFILE wf;
    memset(&wf, 0, sizeof(wf));
    wf.fp = fp;
    wf.ptr = wf.buf = buf;
    wf.end = wf.ptr + sizeof(buf);
    wf.error = WFERR_OK;
    wf.version = version;
    w_long(x, &wf);
    w_flush(&wf);
}

void
PyMarshal_WriteObjectToFile(PyObject *x, FILE *fp, int version)
{
    char buf[BUFSIZ];
    WFILE wf;
    if (PySys_Audit("marshal.dumps", "Oi", x, version) < 0) {
        return; /* caller must check PyErr_Occurred() */
    }
    memset(&wf, 0, sizeof(wf));
    wf.fp = fp;
    wf.ptr = wf.buf = buf;
    wf.end = wf.ptr + sizeof(buf);
    wf.error = WFERR_OK;
    wf.version = version;
    wf.allow_code = 1;
    if (w_init_refs(&wf, version)) {
        return; /* caller must check PyErr_Occurred() */
    }
    w_object(x, &wf);
    w_clear_refs(&wf);
    w_flush(&wf);
}

typedef struct {
    FILE *fp;
    int depth;
    PyObject *readable;  /* Stream-like object being read from */
    const char *ptr;
    const char *end;
    char *buf;
    Py_ssize_t buf_size;
    PyObject *refs;  /* a list */
    int allow_code;
} RFILE;

static const char *
r_string(Py_ssize_t n, RFILE *p)
{
    Py_ssize_t read = -1;

    if (p->ptr != NULL) {
        /* Fast path for loads() */
        const char *res = p->ptr;
        Py_ssize_t left = p->end - p->ptr;
        if (left < n) {
            PyErr_SetString(PyExc_EOFError,
                            "marshal data too short");
            return NULL;
        }
        p->ptr += n;
        return res;
    }
    if (p->buf == NULL) {
        p->buf = PyMem_Malloc(n);
        if (p->buf == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        p->buf_size = n;
    }
    else if (p->buf_size < n) {
        char *tmp = PyMem_Realloc(p->buf, n);
        if (tmp == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        p->buf = tmp;
        p->buf_size = n;
    }

    if (!p->readable) {
        assert(p->fp != NULL);
        read = fread(p->buf, 1, n, p->fp);
    }
    else {
        PyObject *res, *mview;
        Py_buffer buf;

        if (PyBuffer_FillInfo(&buf, NULL, p->buf, n, 0, PyBUF_CONTIG) == -1)
            return NULL;
        mview = PyMemoryView_FromBuffer(&buf);
        if (mview == NULL)
            return NULL;

        res = _PyObject_CallMethod(p->readable, &_Py_ID(readinto), "N", mview);
        if (res != NULL) {
            read = PyNumber_AsSsize_t(res, PyExc_ValueError);
            Py_DECREF(res);
        }
    }
    if (read != n) {
        if (!PyErr_Occurred()) {
            if (read > n)
                PyErr_Format(PyExc_ValueError,
                             "read() returned too much data: "
                             "%zd bytes requested, %zd returned",
                             n, read);
            else
                PyErr_SetString(PyExc_EOFError,
                                "EOF read where not expected");
        }
        return NULL;
    }
    return p->buf;
}

static int
r_byte(RFILE *p)
{
    if (p->ptr != NULL) {
        if (p->ptr < p->end) {
            return (unsigned char) *p->ptr++;
        }
    }
    else if (!p->readable) {
        assert(p->fp);
        int c = getc(p->fp);
        if (c != EOF) {
            return c;
        }
    }
    else {
        const char *ptr = r_string(1, p);
        if (ptr != NULL) {
            return *(const unsigned char *) ptr;
        }
        return EOF;
    }
    PyErr_SetString(PyExc_EOFError,
                    "EOF read where not expected");
    return EOF;
}

static int
r_short(RFILE *p)
{
    short x = -1;
    const unsigned char *buffer;

    buffer = (const unsigned char *) r_string(2, p);
    if (buffer != NULL) {
        x = buffer[0];
        x |= buffer[1] << 8;
        /* Sign-extension, in case short greater than 16 bits */
        x |= -(x & 0x8000);
    }
    return x;
}

static long
r_long(RFILE *p)
{
    long x = -1;
    const unsigned char *buffer;

    buffer = (const unsigned char *) r_string(4, p);
    if (buffer != NULL) {
        x = buffer[0];
        x |= (long)buffer[1] << 8;
        x |= (long)buffer[2] << 16;
        x |= (long)buffer[3] << 24;
#if SIZEOF_LONG > 4
        /* Sign extension for 64-bit machines */
        x |= -(x & 0x80000000L);
#endif
    }
    return x;
}

/* r_long64 deals with the TYPE_INT64 code. */
static PyObject *
r_long64(RFILE *p)
{
    const unsigned char *buffer = (const unsigned char *) r_string(8, p);
    if (buffer == NULL) {
        return NULL;
    }
    return _PyLong_FromByteArray(buffer, 8,
                                 1 /* little endian */,
                                 1 /* signed */);
}

#define _w_digits(bitsize)                                              \
static int                                                              \
_w_digits##bitsize(uint ## bitsize ## _t *digits, Py_ssize_t size,      \
                   Py_ssize_t marshal_ratio,                            \
                   int shorts_in_top_digit, RFILE *p)                   \
{                                                                       \
    uint ## bitsize ## _t d;                                            \
                                                                        \
    assert(size >= 1);                                                  \
    for (Py_ssize_t i = 0; i < size - 1; i++) {                         \
        d = 0;                                                          \
        for (Py_ssize_t j = 0; j < marshal_ratio; j++) {                \
            int md = r_short(p);                                        \
            if (md < 0 || md > PyLong_MARSHAL_BASE) {                   \
                goto bad_digit;                                         \
            }                                                           \
            d += (uint ## bitsize ## _t)md << j*PyLong_MARSHAL_SHIFT;   \
        }                                                               \
        digits[i] = d;                                                  \
    }                                                                   \
                                                                        \
    d = 0;                                                              \
    for (Py_ssize_t j = 0; j < shorts_in_top_digit; j++) {              \
        int md = r_short(p);                                            \
        if (md < 0 || md > PyLong_MARSHAL_BASE) {                       \
            goto bad_digit;                                             \
        }                                                               \
        /* topmost marshal digit should be nonzero */                   \
        if (md == 0 && j == shorts_in_top_digit - 1) {                  \
            PyErr_SetString(PyExc_ValueError,                           \
                "bad marshal data (unnormalized long data)");           \
            return -1;                                                  \
        }                                                               \
        d += (uint ## bitsize ## _t)md << j*PyLong_MARSHAL_SHIFT;       \
    }                                                                   \
    assert(!PyErr_Occurred());                                          \
    /* top digit should be nonzero, else the resulting PyLong won't be  \
       normalized */                                                    \
    digits[size - 1] = d;                                               \
    return 0;                                                           \
                                                                        \
bad_digit:                                                              \
    if (!PyErr_Occurred()) {                                            \
        PyErr_SetString(PyExc_ValueError,                               \
            "bad marshal data (digit out of range in long)");           \
    }                                                                   \
    return -1;                                                          \
}
_w_digits(32)
_w_digits(16)
#undef _w_digits

static PyObject *
r_PyLong(RFILE *p)
{
    long n = r_long(p);
    if (n == -1 && PyErr_Occurred()) {
        return NULL;
    }
    if (n < -SIZE32_MAX || n > SIZE32_MAX) {
        PyErr_SetString(PyExc_ValueError,
                       "bad marshal data (long size out of range)");
        return NULL;
    }

    const PyLongLayout *layout = PyLong_GetNativeLayout();
    Py_ssize_t marshal_ratio = layout->bits_per_digit/PyLong_MARSHAL_SHIFT;

    /* must be a multiple of PyLong_MARSHAL_SHIFT */
    assert(layout->bits_per_digit % PyLong_MARSHAL_SHIFT == 0);
    assert(layout->bits_per_digit >= PyLong_MARSHAL_SHIFT);

    /* other assumptions on PyLongObject internals */
    assert(layout->bits_per_digit <= 32);
    assert(layout->digits_order == -1);
    assert(layout->digit_endianness == (PY_LITTLE_ENDIAN ? -1 : 1));
    assert(layout->digit_size == 2 || layout->digit_size == 4);

    Py_ssize_t size = 1 + (Py_ABS(n) - 1) / marshal_ratio;

    assert(size >= 1);

    int shorts_in_top_digit = 1 + (Py_ABS(n) - 1) % marshal_ratio;
    void *digits;
    PyLongWriter *writer = PyLongWriter_Create(n < 0, size, &digits);

    if (writer == NULL) {
        return NULL;
    }

    int ret;

    if (layout->digit_size == 4) {
        ret = _w_digits32(digits, size, marshal_ratio, shorts_in_top_digit, p);
    }
    else {
        ret = _w_digits16(digits, size, marshal_ratio, shorts_in_top_digit, p);
    }
    if (ret < 0) {
        PyLongWriter_Discard(writer);
        return NULL;
    }
    return PyLongWriter_Finish(writer);
}

static double
r_float_bin(RFILE *p)
{
    const char *buf = r_string(8, p);
    if (buf == NULL)
        return -1;
    return PyFloat_Unpack8(buf, 1);
}

/* Issue #33720: Disable inlining for reducing the C stack consumption
   on PGO builds. */
Py_NO_INLINE static double
r_float_str(RFILE *p)
{
    int n;
    char buf[256];
    const char *ptr;
    n = r_byte(p);
    if (n == EOF) {
        return -1;
    }
    ptr = r_string(n, p);
    if (ptr == NULL) {
        return -1;
    }
    memcpy(buf, ptr, n);
    buf[n] = '\0';
    return PyOS_string_to_double(buf, NULL, NULL);
}

/* allocate the reflist index for a new object. Return -1 on failure */
static Py_ssize_t
r_ref_reserve(int flag, RFILE *p)
{
    if (flag) { /* currently only FLAG_REF is defined */
        Py_ssize_t idx = PyList_GET_SIZE(p->refs);
        if (idx >= 0x7ffffffe) {
            PyErr_SetString(PyExc_ValueError, "bad marshal data (index list too large)");
            return -1;
        }
        if (PyList_Append(p->refs, Py_None) < 0)
            return -1;
        return idx;
    } else
        return 0;
}

/* insert the new object 'o' to the reflist at previously
 * allocated index 'idx'.
 * 'o' can be NULL, in which case nothing is done.
 * if 'o' was non-NULL, and the function succeeds, 'o' is returned.
 * if 'o' was non-NULL, and the function fails, 'o' is released and
 * NULL returned. This simplifies error checking at the call site since
 * a single test for NULL for the function result is enough.
 */
static PyObject *
r_ref_insert(PyObject *o, Py_ssize_t idx, int flag, RFILE *p)
{
    if (o != NULL && flag) { /* currently only FLAG_REF is defined */
        PyObject *tmp = PyList_GET_ITEM(p->refs, idx);
        PyList_SET_ITEM(p->refs, idx, Py_NewRef(o));
        Py_DECREF(tmp);
    }
    return o;
}

/* combination of both above, used when an object can be
 * created whenever it is seen in the file, as opposed to
 * after having loaded its sub-objects.
 */
static PyObject *
r_ref(PyObject *o, int flag, RFILE *p)
{
    assert(flag & FLAG_REF);
    if (o == NULL)
        return NULL;
    if (PyList_Append(p->refs, o) < 0) {
        Py_DECREF(o); /* release the new object */
        return NULL;
    }
    return o;
}

static PyObject *
r_object(RFILE *p)
{
    /* NULL is a valid return value, it does not necessarily means that
       an exception is set. */
    PyObject *v, *v2;
    Py_ssize_t idx = 0;
    long i, n;
    int type, code = r_byte(p);
    int flag, is_interned = 0;
    PyObject *retval = NULL;

    if (code == EOF) {
        if (PyErr_ExceptionMatches(PyExc_EOFError)) {
            PyErr_SetString(PyExc_EOFError,
                            "EOF read where object expected");
        }
        return NULL;
    }

    p->depth++;

    if (p->depth > MAX_MARSHAL_STACK_DEPTH) {
        p->depth--;
        PyErr_SetString(PyExc_ValueError, "recursion limit exceeded");
        return NULL;
    }

    flag = code & FLAG_REF;
    type = code & ~FLAG_REF;

#define R_REF(O) do{\
    if (flag) \
        O = r_ref(O, flag, p);\
} while (0)

    switch (type) {

    case TYPE_NULL:
        break;

    case TYPE_NONE:
        retval = Py_None;
        break;

    case TYPE_STOPITER:
        retval = Py_NewRef(PyExc_StopIteration);
        break;

    case TYPE_ELLIPSIS:
        retval = Py_Ellipsis;
        break;

    case TYPE_FALSE:
        retval = Py_False;
        break;

    case TYPE_TRUE:
        retval = Py_True;
        break;

    case TYPE_INT:
        n = r_long(p);
        if (n == -1 && PyErr_Occurred()) {
            break;
        }
        retval = PyLong_FromLong(n);
        R_REF(retval);
        break;

    case TYPE_INT64:
        retval = r_long64(p);
        R_REF(retval);
        break;

    case TYPE_LONG:
        retval = r_PyLong(p);
        R_REF(retval);
        break;

    case TYPE_FLOAT:
        {
            double x = r_float_str(p);
            if (x == -1.0 && PyErr_Occurred())
                break;
            retval = PyFloat_FromDouble(x);
            R_REF(retval);
            break;
        }

    case TYPE_BINARY_FLOAT:
        {
            double x = r_float_bin(p);
            if (x == -1.0 && PyErr_Occurred())
                break;
            retval = PyFloat_FromDouble(x);
            R_REF(retval);
            break;
        }

    case TYPE_COMPLEX:
        {
            Py_complex c;
            c.real = r_float_str(p);
            if (c.real == -1.0 && PyErr_Occurred())
                break;
            c.imag = r_float_str(p);
            if (c.imag == -1.0 && PyErr_Occurred())
                break;
            retval = PyComplex_FromCComplex(c);
            R_REF(retval);
            break;
        }

    case TYPE_BINARY_COMPLEX:
        {
            Py_complex c;
            c.real = r_float_bin(p);
            if (c.real == -1.0 && PyErr_Occurred())
                break;
            c.imag = r_float_bin(p);
            if (c.imag == -1.0 && PyErr_Occurred())
                break;
            retval = PyComplex_FromCComplex(c);
            R_REF(retval);
            break;
        }

    case TYPE_STRING:
        {
            const char *ptr;
            n = r_long(p);
            if (n < 0 || n > SIZE32_MAX) {
                if (!PyErr_Occurred()) {
                    PyErr_SetString(PyExc_ValueError,
                        "bad marshal data (bytes object size out of range)");
                }
                break;
            }
            v = PyBytes_FromStringAndSize((char *)NULL, n);
            if (v == NULL)
                break;
            ptr = r_string(n, p);
            if (ptr == NULL) {
                Py_DECREF(v);
                break;
            }
            memcpy(PyBytes_AS_STRING(v), ptr, n);
            retval = v;
            R_REF(retval);
            break;
        }

    case TYPE_ASCII_INTERNED:
        is_interned = 1;
        _Py_FALLTHROUGH;
    case TYPE_ASCII:
        n = r_long(p);
        if (n < 0 || n > SIZE32_MAX) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError,
                    "bad marshal data (string size out of range)");
            }
            break;
        }
        goto _read_ascii;

    case TYPE_SHORT_ASCII_INTERNED:
        is_interned = 1;
        _Py_FALLTHROUGH;
    case TYPE_SHORT_ASCII:
        n = r_byte(p);
        if (n == EOF) {
            break;
        }
    _read_ascii:
        {
            const char *ptr;
            ptr = r_string(n, p);
            if (ptr == NULL)
                break;
            v = PyUnicode_FromKindAndData(PyUnicode_1BYTE_KIND, ptr, n);
            if (v == NULL)
                break;
            if (is_interned) {
                // marshal is meant to serialize .pyc files with code
                // objects, and code-related strings are currently immortal.
                PyInterpreterState *interp = _PyInterpreterState_GET();
                _PyUnicode_InternImmortal(interp, &v);
            }
            retval = v;
            R_REF(retval);
            break;
        }

    case TYPE_INTERNED:
        is_interned = 1;
        _Py_FALLTHROUGH;
    case TYPE_UNICODE:
        {
        const char *buffer;

        n = r_long(p);
        if (n < 0 || n > SIZE32_MAX) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError,
                    "bad marshal data (string size out of range)");
            }
            break;
        }
        if (n != 0) {
            buffer = r_string(n, p);
            if (buffer == NULL)
                break;
            v = PyUnicode_DecodeUTF8(buffer, n, "surrogatepass");
        }
        else {
            v = Py_GetConstant(Py_CONSTANT_EMPTY_STR);
        }
        if (v == NULL)
            break;
        if (is_interned) {
            // marshal is meant to serialize .pyc files with code
            // objects, and code-related strings are currently immortal.
            PyInterpreterState *interp = _PyInterpreterState_GET();
            _PyUnicode_InternImmortal(interp, &v);
        }
        retval = v;
        R_REF(retval);
        break;
        }

    case TYPE_SMALL_TUPLE:
        n = r_byte(p);
        if (n == EOF) {
            break;
        }
        goto _read_tuple;
    case TYPE_TUPLE:
        n = r_long(p);
        if (n < 0 || n > SIZE32_MAX) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError,
                    "bad marshal data (tuple size out of range)");
            }
            break;
        }
    _read_tuple:
        v = PyTuple_New(n);
        R_REF(v);
        if (v == NULL)
            break;

        for (i = 0; i < n; i++) {
            v2 = r_object(p);
            if ( v2 == NULL ) {
                if (!PyErr_Occurred())
                    PyErr_SetString(PyExc_TypeError,
                        "NULL object in marshal data for tuple");
                Py_SETREF(v, NULL);
                break;
            }
            PyTuple_SET_ITEM(v, i, v2);
        }
        retval = v;
        break;

    case TYPE_LIST:
        n = r_long(p);
        if (n < 0 || n > SIZE32_MAX) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError,
                    "bad marshal data (list size out of range)");
            }
            break;
        }
        v = PyList_New(n);
        R_REF(v);
        if (v == NULL)
            break;
        for (i = 0; i < n; i++) {
            v2 = r_object(p);
            if ( v2 == NULL ) {
                if (!PyErr_Occurred())
                    PyErr_SetString(PyExc_TypeError,
                        "NULL object in marshal data for list");
                Py_SETREF(v, NULL);
                break;
            }
            PyList_SET_ITEM(v, i, v2);
        }
        retval = v;
        break;

    case TYPE_DICT:
        v = PyDict_New();
        R_REF(v);
        if (v == NULL)
            break;
        for (;;) {
            PyObject *key, *val;
            key = r_object(p);
            if (key == NULL)
                break;
            val = r_object(p);
            if (val == NULL) {
                Py_DECREF(key);
                break;
            }
            if (PyDict_SetItem(v, key, val) < 0) {
                Py_DECREF(key);
                Py_DECREF(val);
                break;
            }
            Py_DECREF(key);
            Py_DECREF(val);
        }
        if (PyErr_Occurred()) {
            Py_SETREF(v, NULL);
        }
        retval = v;
        break;

    case TYPE_SET:
    case TYPE_FROZENSET:
        n = r_long(p);
        if (n < 0 || n > SIZE32_MAX) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError,
                    "bad marshal data (set size out of range)");
            }
            break;
        }

        if (n == 0 && type == TYPE_FROZENSET) {
            /* call frozenset() to get the empty frozenset singleton */
            v = _PyObject_CallNoArgs((PyObject*)&PyFrozenSet_Type);
            if (v == NULL)
                break;
            R_REF(v);
            retval = v;
        }
        else {
            v = (type == TYPE_SET) ? PySet_New(NULL) : PyFrozenSet_New(NULL);
            if (type == TYPE_SET) {
                R_REF(v);
            } else {
                /* must use delayed registration of frozensets because they must
                 * be init with a refcount of 1
                 */
                idx = r_ref_reserve(flag, p);
                if (idx < 0)
                    Py_CLEAR(v); /* signal error */
            }
            if (v == NULL)
                break;

            for (i = 0; i < n; i++) {
                v2 = r_object(p);
                if ( v2 == NULL ) {
                    if (!PyErr_Occurred())
                        PyErr_SetString(PyExc_TypeError,
                            "NULL object in marshal data for set");
                    Py_SETREF(v, NULL);
                    break;
                }
                if (PySet_Add(v, v2) == -1) {
                    Py_DECREF(v);
                    Py_DECREF(v2);
                    v = NULL;
                    break;
                }
                Py_DECREF(v2);
            }
            if (type != TYPE_SET)
                v = r_ref_insert(v, idx, flag, p);
            retval = v;
        }
        break;

    case TYPE_CODE:
        {
            int argcount;
            int posonlyargcount;
            int kwonlyargcount;
            int stacksize;
            int flags;
            PyObject *code = NULL;
            PyObject *consts = NULL;
            PyObject *names = NULL;
            PyObject *localsplusnames = NULL;
            PyObject *localspluskinds = NULL;
            PyObject *filename = NULL;
            PyObject *name = NULL;
            PyObject *qualname = NULL;
            int firstlineno;
            PyObject* linetable = NULL;
            PyObject *exceptiontable = NULL;

            if (!p->allow_code) {
                PyErr_SetString(PyExc_ValueError,
                                "unmarshalling code objects is disallowed");
                break;
            }
            idx = r_ref_reserve(flag, p);
            if (idx < 0)
                break;

            v = NULL;

            /* XXX ignore long->int overflows for now */
            argcount = (int)r_long(p);
            if (argcount == -1 && PyErr_Occurred())
                goto code_error;
            posonlyargcount = (int)r_long(p);
            if (posonlyargcount == -1 && PyErr_Occurred()) {
                goto code_error;
            }
            kwonlyargcount = (int)r_long(p);
            if (kwonlyargcount == -1 && PyErr_Occurred())
                goto code_error;
            stacksize = (int)r_long(p);
            if (stacksize == -1 && PyErr_Occurred())
                goto code_error;
            flags = (int)r_long(p);
            if (flags == -1 && PyErr_Occurred())
                goto code_error;
            code = r_object(p);
            if (code == NULL)
                goto code_error;
            consts = r_object(p);
            if (consts == NULL)
                goto code_error;
            names = r_object(p);
            if (names == NULL)
                goto code_error;
            localsplusnames = r_object(p);
            if (localsplusnames == NULL)
                goto code_error;
            localspluskinds = r_object(p);
            if (localspluskinds == NULL)
                goto code_error;
            filename = r_object(p);
            if (filename == NULL)
                goto code_error;
            name = r_object(p);
            if (name == NULL)
                goto code_error;
            qualname = r_object(p);
            if (qualname == NULL)
                goto code_error;
            firstlineno = (int)r_long(p);
            if (firstlineno == -1 && PyErr_Occurred())
                break;
            linetable = r_object(p);
            if (linetable == NULL)
                goto code_error;
            exceptiontable = r_object(p);
            if (exceptiontable == NULL)
                goto code_error;

            struct _PyCodeConstructor con = {
                .filename = filename,
                .name = name,
                .qualname = qualname,
                .flags = flags,

                .code = code,
                .firstlineno = firstlineno,
                .linetable = linetable,

                .consts = consts,
                .names = names,

                .localsplusnames = localsplusnames,
                .localspluskinds = localspluskinds,

                .argcount = argcount,
                .posonlyargcount = posonlyargcount,
                .kwonlyargcount = kwonlyargcount,

                .stacksize = stacksize,

                .exceptiontable = exceptiontable,
            };

            if (_PyCode_Validate(&con) < 0) {
                goto code_error;
            }

            v = (PyObject *)_PyCode_New(&con);
            if (v == NULL) {
                goto code_error;
            }

            v = r_ref_insert(v, idx, flag, p);

          code_error:
            if (v == NULL && !PyErr_Occurred()) {
                PyErr_SetString(PyExc_TypeError,
                    "NULL object in marshal data for code object");
            }
            Py_XDECREF(code);
            Py_XDECREF(consts);
            Py_XDECREF(names);
            Py_XDECREF(localsplusnames);
            Py_XDECREF(localspluskinds);
            Py_XDECREF(filename);
            Py_XDECREF(name);
            Py_XDECREF(qualname);
            Py_XDECREF(linetable);
            Py_XDECREF(exceptiontable);
        }
        retval = v;
        break;

    case TYPE_REF:
        n = r_long(p);
        if (n < 0 || n >= PyList_GET_SIZE(p->refs)) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError,
                    "bad marshal data (invalid reference)");
            }
            break;
        }
        v = PyList_GET_ITEM(p->refs, n);
        if (v == Py_None) {
            PyErr_SetString(PyExc_ValueError, "bad marshal data (invalid reference)");
            break;
        }
        retval = Py_NewRef(v);
        break;

    case TYPE_SLICE:
    {
        Py_ssize_t idx = r_ref_reserve(flag, p);
        if (idx < 0) {
            break;
        }
        PyObject *stop = NULL;
        PyObject *step = NULL;
        PyObject *start = r_object(p);
        if (start == NULL) {
            goto cleanup;
        }
        stop = r_object(p);
        if (stop == NULL) {
            goto cleanup;
        }
        step = r_object(p);
        if (step == NULL) {
            goto cleanup;
        }
        retval = PySlice_New(start, stop, step);
        r_ref_insert(retval, idx, flag, p);
    cleanup:
        Py_XDECREF(start);
        Py_XDECREF(stop);
        Py_XDECREF(step);
        break;
    }

    default:
        /* Bogus data got written, which isn't ideal.
           This will let you keep working and recover. */
        PyErr_SetString(PyExc_ValueError, "bad marshal data (unknown type code)");
        break;

    }
    p->depth--;
    return retval;
}

static PyObject *
read_object(RFILE *p)
{
    PyObject *v;
    if (PyErr_Occurred()) {
        fprintf(stderr, "XXX readobject called with exception set\n");
        return NULL;
    }
    if (p->ptr && p->end) {
        if (PySys_Audit("marshal.loads", "y#", p->ptr, (Py_ssize_t)(p->end - p->ptr)) < 0) {
            return NULL;
        }
    } else if (p->fp || p->readable) {
        if (PySys_Audit("marshal.load", NULL) < 0) {
            return NULL;
        }
    }
    v = r_object(p);
    if (v == NULL && !PyErr_Occurred())
        PyErr_SetString(PyExc_TypeError, "NULL object in marshal data for object");
    return v;
}

int
PyMarshal_ReadShortFromFile(FILE *fp)
{
    RFILE rf;
    int res;
    assert(fp);
    rf.readable = NULL;
    rf.fp = fp;
    rf.end = rf.ptr = NULL;
    rf.buf = NULL;
    res = r_short(&rf);
    if (rf.buf != NULL)
        PyMem_Free(rf.buf);
    return res;
}

long
PyMarshal_ReadLongFromFile(FILE *fp)
{
    RFILE rf;
    long res;
    rf.fp = fp;
    rf.readable = NULL;
    rf.ptr = rf.end = NULL;
    rf.buf = NULL;
    res = r_long(&rf);
    if (rf.buf != NULL)
        PyMem_Free(rf.buf);
    return res;
}

/* Return size of file in bytes; < 0 if unknown or INT_MAX if too big */
static off_t
getfilesize(FILE *fp)
{
    struct _Py_stat_struct st;
    if (_Py_fstat_noraise(fileno(fp), &st) != 0)
        return -1;
#if SIZEOF_OFF_T == 4
    else if (st.st_size >= INT_MAX)
        return (off_t)INT_MAX;
#endif
    else
        return (off_t)st.st_size;
}

/* If we can get the size of the file up-front, and it's reasonably small,
 * read it in one gulp and delegate to ...FromString() instead.  Much quicker
 * than reading a byte at a time from file; speeds .pyc imports.
 * CAUTION:  since this may read the entire remainder of the file, don't
 * call it unless you know you're done with the file.
 */
PyObject *
PyMarshal_ReadLastObjectFromFile(FILE *fp)
{
/* REASONABLE_FILE_LIMIT is by defn something big enough for Tkinter.pyc. */
#define REASONABLE_FILE_LIMIT (1L << 18)
    off_t filesize;
    filesize = getfilesize(fp);
    if (filesize > 0 && filesize <= REASONABLE_FILE_LIMIT) {
        char* pBuf = (char *)PyMem_Malloc(filesize);
        if (pBuf != NULL) {
            size_t n = fread(pBuf, 1, (size_t)filesize, fp);
            PyObject* v = PyMarshal_ReadObjectFromString(pBuf, n);
            PyMem_Free(pBuf);
            return v;
        }

    }
    /* We don't have fstat, or we do but the file is larger than
     * REASONABLE_FILE_LIMIT or malloc failed -- read a byte at a time.
     */
    return PyMarshal_ReadObjectFromFile(fp);

#undef REASONABLE_FILE_LIMIT
}

PyObject *
PyMarshal_ReadObjectFromFile(FILE *fp)
{
    RFILE rf;
    PyObject *result;
    rf.allow_code = 1;
    rf.fp = fp;
    rf.readable = NULL;
    rf.depth = 0;
    rf.ptr = rf.end = NULL;
    rf.buf = NULL;
    rf.refs = PyList_New(0);
    if (rf.refs == NULL)
        return NULL;
    result = read_object(&rf);
    Py_DECREF(rf.refs);
    if (rf.buf != NULL)
        PyMem_Free(rf.buf);
    return result;
}

PyObject *
PyMarshal_ReadObjectFromString(const char *str, Py_ssize_t len)
{
    RFILE rf;
    PyObject *result;
    rf.allow_code = 1;
    rf.fp = NULL;
    rf.readable = NULL;
    rf.ptr = str;
    rf.end = str + len;
    rf.buf = NULL;
    rf.depth = 0;
    rf.refs = PyList_New(0);
    if (rf.refs == NULL)
        return NULL;
    result = read_object(&rf);
    Py_DECREF(rf.refs);
    if (rf.buf != NULL)
        PyMem_Free(rf.buf);
    return result;
}

static PyObject *
_PyMarshal_WriteObjectToString(PyObject *x, int version, int allow_code)
{
    WFILE wf;

    if (PySys_Audit("marshal.dumps", "Oi", x, version) < 0) {
        return NULL;
    }
    memset(&wf, 0, sizeof(wf));
    wf.str = PyBytes_FromStringAndSize((char *)NULL, 50);
    if (wf.str == NULL)
        return NULL;
    wf.ptr = wf.buf = PyBytes_AS_STRING(wf.str);
    wf.end = wf.ptr + PyBytes_GET_SIZE(wf.str);
    wf.error = WFERR_OK;
    wf.version = version;
    wf.allow_code = allow_code;
    if (w_init_refs(&wf, version)) {
        Py_DECREF(wf.str);
        return NULL;
    }
    w_object(x, &wf);
    w_clear_refs(&wf);
    if (wf.str != NULL) {
        const char *base = PyBytes_AS_STRING(wf.str);
        if (_PyBytes_Resize(&wf.str, (Py_ssize_t)(wf.ptr - base)) < 0)
            return NULL;
    }
    if (wf.error != WFERR_OK) {
        Py_XDECREF(wf.str);
        switch (wf.error) {
        case WFERR_NOMEMORY:
            PyErr_NoMemory();
            break;
        case WFERR_NESTEDTOODEEP:
            PyErr_SetString(PyExc_ValueError,
                            "object too deeply nested to marshal");
            break;
        case WFERR_CODE_NOT_ALLOWED:
            PyErr_SetString(PyExc_ValueError,
                            "marshalling code objects is disallowed");
            break;
        default:
        case WFERR_UNMARSHALLABLE:
            PyErr_SetString(PyExc_ValueError,
                            "unmarshallable object");
            break;
        }
        return NULL;
    }
    return wf.str;
}

PyObject *
PyMarshal_WriteObjectToString(PyObject *x, int version)
{
    return _PyMarshal_WriteObjectToString(x, version, 1);
}

/* And an interface for Python programs... */
/*[clinic input]
marshal.dump

    value: object
        Must be a supported type.
    file: object
        Must be a writeable binary file.
    version: int(c_default="Py_MARSHAL_VERSION") = version
        Indicates the data format that dump should use.
    /
    *
    allow_code: bool = True
        Allow to write code objects.

Write the value on the open file.

If the value has (or contains an object that has) an unsupported type, a
ValueError exception is raised - but garbage data will also be written
to the file. The object will not be properly read back by load().
[clinic start generated code]*/

static PyObject *
marshal_dump_impl(PyObject *module, PyObject *value, PyObject *file,
                  int version, int allow_code)
/*[clinic end generated code: output=429e5fd61c2196b9 input=041f7f6669b0aafb]*/
{
    /* XXX Quick hack -- need to do this differently */
    PyObject *s;
    PyObject *res;

    s = _PyMarshal_WriteObjectToString(value, version, allow_code);
    if (s == NULL)
        return NULL;
    res = PyObject_CallMethodOneArg(file, &_Py_ID(write), s);
    Py_DECREF(s);
    return res;
}

/*[clinic input]
marshal.load

    file: object
        Must be readable binary file.
    /
    *
    allow_code: bool = True
        Allow to load code objects.

Read one value from the open file and return it.

If no valid value is read (e.g. because the data has a different Python
version's incompatible marshal format), raise EOFError, ValueError or
TypeError.

Note: If an object containing an unsupported type was marshalled with
dump(), load() will substitute None for the unmarshallable type.
[clinic start generated code]*/

static PyObject *
marshal_load_impl(PyObject *module, PyObject *file, int allow_code)
/*[clinic end generated code: output=0c1aaf3546ae3ed3 input=2dca7b570653b82f]*/
{
    PyObject *data, *result;
    RFILE rf;

    /*
     * Make a call to the read method, but read zero bytes.
     * This is to ensure that the object passed in at least
     * has a read method which returns bytes.
     * This can be removed if we guarantee good error handling
     * for r_string()
     */
    data = _PyObject_CallMethod(file, &_Py_ID(read), "i", 0);
    if (data == NULL)
        return NULL;
    if (!PyBytes_Check(data)) {
        PyErr_Format(PyExc_TypeError,
                     "file.read() returned not bytes but %.100s",
                     Py_TYPE(data)->tp_name);
        result = NULL;
    }
    else {
        rf.allow_code = allow_code;
        rf.depth = 0;
        rf.fp = NULL;
        rf.readable = file;
        rf.ptr = rf.end = NULL;
        rf.buf = NULL;
        if ((rf.refs = PyList_New(0)) != NULL) {
            result = read_object(&rf);
            Py_DECREF(rf.refs);
            if (rf.buf != NULL)
                PyMem_Free(rf.buf);
        } else
            result = NULL;
    }
    Py_DECREF(data);
    return result;
}

/*[clinic input]
@permit_long_summary
@permit_long_docstring_body
marshal.dumps

    value: object
        Must be a supported type.
    version: int(c_default="Py_MARSHAL_VERSION") = version
        Indicates the data format that dumps should use.
    /
    *
    allow_code: bool = True
        Allow to write code objects.

Return the bytes object that would be written to a file by dump(value, file).

Raise a ValueError exception if value has (or contains an object that has) an
unsupported type.
[clinic start generated code]*/

static PyObject *
marshal_dumps_impl(PyObject *module, PyObject *value, int version,
                   int allow_code)
/*[clinic end generated code: output=115f90da518d1d49 input=80cd3f30c1637ade]*/
{
    return _PyMarshal_WriteObjectToString(value, version, allow_code);
}

/*[clinic input]
marshal.loads

    bytes: Py_buffer
    /
    *
    allow_code: bool = True
        Allow to load code objects.

Convert the bytes-like object to a value.

If no valid value is found, raise EOFError, ValueError or TypeError.  Extra
bytes in the input are ignored.
[clinic start generated code]*/

static PyObject *
marshal_loads_impl(PyObject *module, Py_buffer *bytes, int allow_code)
/*[clinic end generated code: output=62c0c538d3edc31f input=14de68965b45aaa7]*/
{
    RFILE rf;
    char *s = bytes->buf;
    Py_ssize_t n = bytes->len;
    PyObject* result;
    rf.allow_code = allow_code;
    rf.fp = NULL;
    rf.readable = NULL;
    rf.ptr = s;
    rf.end = s + n;
    rf.depth = 0;
    if ((rf.refs = PyList_New(0)) == NULL)
        return NULL;
    result = read_object(&rf);
    Py_DECREF(rf.refs);
    return result;
}

static PyMethodDef marshal_methods[] = {
    MARSHAL_DUMP_METHODDEF
    MARSHAL_LOAD_METHODDEF
    MARSHAL_DUMPS_METHODDEF
    MARSHAL_LOADS_METHODDEF
    {NULL,              NULL}           /* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module contains functions that can read and write Python values in\n\
a binary format. The format is specific to Python, but independent of\n\
machine architecture issues.\n\
\n\
Not all Python object types are supported; in general, only objects\n\
whose value is independent from a particular invocation of Python can be\n\
written and read by this module. The following types are supported:\n\
None, integers, floating-point numbers, strings, bytes, bytearrays,\n\
tuples, lists, sets, dictionaries, and code objects, where it\n\
should be understood that tuples, lists and dictionaries are only\n\
supported as long as the values contained therein are themselves\n\
supported; and recursive lists and dictionaries should not be written\n\
(they will cause infinite loops).\n\
\n\
Variables:\n\
\n\
version -- indicates the format that the module uses. Version 0 is the\n\
    historical format, version 1 shares interned strings and version 2\n\
    uses a binary format for floating-point numbers.\n\
    Version 3 shares common object references (New in version 3.4).\n\
\n\
Functions:\n\
\n\
dump() -- write value to a file\n\
load() -- read value from a file\n\
dumps() -- marshal value as a bytes object\n\
loads() -- read value from a bytes-like object");


static int
marshal_module_exec(PyObject *mod)
{
    if (PyModule_AddIntConstant(mod, "version", Py_MARSHAL_VERSION) < 0) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot marshalmodule_slots[] = {
    {Py_mod_exec, marshal_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef marshalmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "marshal",
    .m_doc = module_doc,
    .m_methods = marshal_methods,
    .m_slots = marshalmodule_slots,
};

PyMODINIT_FUNC
PyMarshal_Init(void)
{
    return PyModuleDef_Init(&marshalmodule);
}
