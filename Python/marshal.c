
/* Write Python objects to files and read them back.
   This is primarily intended for writing and reading compiled Python code,
   even though dicts, lists, sets and frozensets, not commonly seen in
   code objects, are supported.
   Version 3 of this protocol properly supports circular links
   and sharing. */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "longintrepr.h"
#include "code.h"
#include "marshal.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))

/* High water mark to determine when the marshalled object is dangerously deep
 * and risks coring the interpreter.  When the object stack gets this deep,
 * raise an exception instead of continuing.
 * On Windows debug builds, reduce this value.
 */
#if defined(MS_WINDOWS) && defined(_DEBUG)
#define MAX_MARSHAL_STACK_DEPTH 1500
#else
#define MAX_MARSHAL_STACK_DEPTH 2000
#endif

#define TYPE_NULL               '0'
#define TYPE_NONE               'N'
#define TYPE_FALSE              'F'
#define TYPE_TRUE               'T'
#define TYPE_STOPITER           'S'
#define TYPE_ELLIPSIS           '.'
#define TYPE_INT                'i'
/* TYPE_INT64 is deprecated. It is not
   generated anymore, and support for reading it
   will be removed in Python 3.4. */
#define TYPE_INT64              'I'
#define TYPE_FLOAT              'f'
#define TYPE_BINARY_FLOAT       'g'
#define TYPE_COMPLEX            'x'
#define TYPE_BINARY_COMPLEX     'y'
#define TYPE_LONG               'l'
#define TYPE_STRING             's'
#define TYPE_INTERNED           't'
#define TYPE_REF                'r'
#define TYPE_TUPLE              '('
#define TYPE_LIST               '['
#define TYPE_DICT               '{'
#define TYPE_CODE               'c'
#define TYPE_UNICODE            'u'
#define TYPE_UNKNOWN            '?'
#define TYPE_SET                '<'
#define TYPE_FROZENSET          '>'
#define FLAG_REF                '\x80' /* with a type, add obj to index */

#define WFERR_OK 0
#define WFERR_UNMARSHALLABLE 1
#define WFERR_NESTEDTOODEEP 2
#define WFERR_NOMEMORY 3

typedef struct {
    FILE *fp;
    int error;  /* see WFERR_* values */
    int depth;
    /* If fp == NULL, the following are valid: */
    PyObject *readable;    /* Stream-like object being read from */
    PyObject *str;
    PyObject *current_filename;
    char *ptr;
    char *end;
    PyObject *refs; /* dict on marshal, list on unmarshal */
    int version;
} WFILE;

#define w_byte(c, p) if (((p)->fp)) putc((c), (p)->fp); \
                      else if ((p)->ptr != (p)->end) *(p)->ptr++ = (c); \
                           else w_more(c, p)

static void
w_more(int c, WFILE *p)
{
    Py_ssize_t size, newsize;
    if (p->str == NULL)
        return; /* An error already occurred */
    size = PyBytes_Size(p->str);
    newsize = size + size + 1024;
    if (newsize > 32*1024*1024) {
        newsize = size + (size >> 3);           /* 12.5% overallocation */
    }
    if (_PyBytes_Resize(&p->str, newsize) != 0) {
        p->ptr = p->end = NULL;
    }
    else {
        p->ptr = PyBytes_AS_STRING((PyBytesObject *)p->str) + size;
        p->end =
            PyBytes_AS_STRING((PyBytesObject *)p->str) + newsize;
        *p->ptr++ = Py_SAFE_DOWNCAST(c, int, char);
    }
}

static void
w_string(char *s, Py_ssize_t n, WFILE *p)
{
    if (p->fp != NULL) {
        fwrite(s, 1, n, p->fp);
    }
    else {
        while (--n >= 0) {
            w_byte(*s, p);
            s++;
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

/* We assume that Python longs are stored internally in base some power of
   2**15; for the sake of portability we'll always read and write them in base
   exactly 2**15. */

#define PyLong_MARSHAL_SHIFT 15
#define PyLong_MARSHAL_BASE ((short)1 << PyLong_MARSHAL_SHIFT)
#define PyLong_MARSHAL_MASK (PyLong_MARSHAL_BASE - 1)
#if PyLong_SHIFT % PyLong_MARSHAL_SHIFT != 0
#error "PyLong_SHIFT must be a multiple of PyLong_MARSHAL_SHIFT"
#endif
#define PyLong_MARSHAL_RATIO (PyLong_SHIFT / PyLong_MARSHAL_SHIFT)

#define W_TYPE(t, p) do { \
    w_byte((t) | flag, (p)); \
} while(0)

static void
w_PyLong(const PyLongObject *ob, char flag, WFILE *p)
{
    Py_ssize_t i, j, n, l;
    digit d;

    W_TYPE(TYPE_LONG, p);
    if (Py_SIZE(ob) == 0) {
        w_long((long)0, p);
        return;
    }

    /* set l to number of base PyLong_MARSHAL_BASE digits */
    n = ABS(Py_SIZE(ob));
    l = (n-1) * PyLong_MARSHAL_RATIO;
    d = ob->ob_digit[n-1];
    assert(d != 0); /* a PyLong is always normalized */
    do {
        d >>= PyLong_MARSHAL_SHIFT;
        l++;
    } while (d != 0);
    if (l > SIZE32_MAX) {
        p->depth--;
        p->error = WFERR_UNMARSHALLABLE;
        return;
    }
    w_long((long)(Py_SIZE(ob) > 0 ? l : -l), p);

    for (i=0; i < n-1; i++) {
        d = ob->ob_digit[i];
        for (j=0; j < PyLong_MARSHAL_RATIO; j++) {
            w_short(d & PyLong_MARSHAL_MASK, p);
            d >>= PyLong_MARSHAL_SHIFT;
        }
        assert (d == 0);
    }
    d = ob->ob_digit[n-1];
    do {
        w_short(d & PyLong_MARSHAL_MASK, p);
        d >>= PyLong_MARSHAL_SHIFT;
    } while (d != 0);
}

static int
w_ref(PyObject *v, char *flag, WFILE *p)
{
    PyObject *id;
    PyObject *idx;

    if (p->version < 3 || p->refs == NULL)
        return 0; /* not writing object references */

    /* if it has only one reference, it definitely isn't shared */
    if (Py_REFCNT(v) == 1)
        return 0;

    id = PyLong_FromVoidPtr((void*)v);
    if (id == NULL)
        goto err;
    idx = PyDict_GetItem(p->refs, id);
    if (idx != NULL) {
        /* write the reference index to the stream */
        long w = PyLong_AsLong(idx);
        Py_DECREF(id);
        if (w == -1 && PyErr_Occurred()) {
            goto err;
        }
        /* we don't store "long" indices in the dict */
        assert(0 <= w && w <= 0x7fffffff);
        w_byte(TYPE_REF, p);
        w_long(w, p);
        return 1;
    } else {
        int ok;
        Py_ssize_t s = PyDict_Size(p->refs);
        /* we don't support long indices */
        if (s >= 0x7fffffff) {
            PyErr_SetString(PyExc_ValueError, "too many objects");
            goto err;
        }
        idx = PyLong_FromSsize_t(s);
        ok = idx && PyDict_SetItem(p->refs, id, idx) == 0;
        Py_DECREF(id);
        Py_XDECREF(idx);
        if (!ok)
            goto err;
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
        long x = PyLong_AsLong(v);
        if ((x == -1)  && PyErr_Occurred()) {
            PyLongObject *ob = (PyLongObject *)v;
            PyErr_Clear();
            w_PyLong(ob, flag, p);
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
            unsigned char buf[8];
            if (_PyFloat_Pack8(PyFloat_AsDouble(v),
                               buf, 1) < 0) {
                p->error = WFERR_UNMARSHALLABLE;
                return;
            }
            W_TYPE(TYPE_BINARY_FLOAT, p);
            w_string((char*)buf, 8, p);
        }
        else {
            char *buf = PyOS_double_to_string(PyFloat_AS_DOUBLE(v),
                                              'g', 17, 0, NULL);
            if (!buf) {
                p->error = WFERR_NOMEMORY;
                return;
            }
            n = strlen(buf);
            W_TYPE(TYPE_FLOAT, p);
            w_byte((int)n, p);
            w_string(buf, n, p);
            PyMem_Free(buf);
        }
    }
    else if (PyComplex_CheckExact(v)) {
        if (p->version > 1) {
            unsigned char buf[8];
            if (_PyFloat_Pack8(PyComplex_RealAsDouble(v),
                               buf, 1) < 0) {
                p->error = WFERR_UNMARSHALLABLE;
                return;
            }
            W_TYPE(TYPE_BINARY_COMPLEX, p);
            w_string((char*)buf, 8, p);
            if (_PyFloat_Pack8(PyComplex_ImagAsDouble(v),
                               buf, 1) < 0) {
                p->error = WFERR_UNMARSHALLABLE;
                return;
            }
            w_string((char*)buf, 8, p);
        }
        else {
            char *buf;
            W_TYPE(TYPE_COMPLEX, p);
            buf = PyOS_double_to_string(PyComplex_RealAsDouble(v),
                                        'g', 17, 0, NULL);
            if (!buf) {
                p->error = WFERR_NOMEMORY;
                return;
            }
            n = strlen(buf);
            w_byte((int)n, p);
            w_string(buf, n, p);
            PyMem_Free(buf);
            buf = PyOS_double_to_string(PyComplex_ImagAsDouble(v),
                                        'g', 17, 0, NULL);
            if (!buf) {
                p->error = WFERR_NOMEMORY;
                return;
            }
            n = strlen(buf);
            w_byte((int)n, p);
            w_string(buf, n, p);
            PyMem_Free(buf);
        }
    }
    else if (PyBytes_CheckExact(v)) {
        W_TYPE(TYPE_STRING, p);
        n = PyBytes_GET_SIZE(v);
        W_SIZE(n, p);
        w_string(PyBytes_AS_STRING(v), n, p);
    }
    else if (PyUnicode_CheckExact(v)) {
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
        n = PyBytes_GET_SIZE(utf8);
        W_SIZE(n, p);
        w_string(PyBytes_AS_STRING(utf8), n, p);
        Py_DECREF(utf8);
    }
    else if (PyTuple_CheckExact(v)) {
        W_TYPE(TYPE_TUPLE, p);
        n = PyTuple_Size(v);
        W_SIZE(n, p);
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
        PyObject *value, *it;

        if (PyObject_TypeCheck(v, &PySet_Type))
            W_TYPE(TYPE_SET, p);
        else
            W_TYPE(TYPE_FROZENSET, p);
        n = PyObject_Size(v);
        if (n == -1) {
            p->depth--;
            p->error = WFERR_UNMARSHALLABLE;
            return;
        }
        W_SIZE(n, p);
        it = PyObject_GetIter(v);
        if (it == NULL) {
            p->depth--;
            p->error = WFERR_UNMARSHALLABLE;
            return;
        }
        while ((value = PyIter_Next(it)) != NULL) {
            w_object(value, p);
            Py_DECREF(value);
        }
        Py_DECREF(it);
        if (PyErr_Occurred()) {
            p->depth--;
            p->error = WFERR_UNMARSHALLABLE;
            return;
        }
    }
    else if (PyCode_Check(v)) {
        PyCodeObject *co = (PyCodeObject *)v;
        W_TYPE(TYPE_CODE, p);
        w_long(co->co_argcount, p);
        w_long(co->co_kwonlyargcount, p);
        w_long(co->co_nlocals, p);
        w_long(co->co_stacksize, p);
        w_long(co->co_flags, p);
        w_object(co->co_code, p);
        w_object(co->co_consts, p);
        w_object(co->co_names, p);
        w_object(co->co_varnames, p);
        w_object(co->co_freevars, p);
        w_object(co->co_cellvars, p);
        w_object(co->co_filename, p);
        w_object(co->co_name, p);
        w_long(co->co_firstlineno, p);
        w_object(co->co_lnotab, p);
    }
    else if (PyObject_CheckBuffer(v)) {
        /* Write unknown buffer-style objects as a string */
        char *s;
        Py_buffer view;
        if (PyObject_GetBuffer(v, &view, PyBUF_SIMPLE) != 0) {
            w_byte(TYPE_UNKNOWN, p);
            p->depth--;
            p->error = WFERR_UNMARSHALLABLE;
            return;
        }
        W_TYPE(TYPE_STRING, p);
        n = view.len;
        s = view.buf;
        W_SIZE(n, p);
        w_string(s, n, p);
        PyBuffer_Release(&view);
    }
    else {
        W_TYPE(TYPE_UNKNOWN, p);
        p->error = WFERR_UNMARSHALLABLE;
    }
}

/* version currently has no effect for writing longs. */
void
PyMarshal_WriteLongToFile(long x, FILE *fp, int version)
{
    WFILE wf;
    wf.fp = fp;
    wf.error = WFERR_OK;
    wf.depth = 0;
    wf.refs = NULL;
    wf.version = version;
    w_long(x, &wf);
}

void
PyMarshal_WriteObjectToFile(PyObject *x, FILE *fp, int version)
{
    WFILE wf;
    wf.fp = fp;
    wf.error = WFERR_OK;
    wf.depth = 0;
    if (version >= 3) {
        if ((wf.refs = PyDict_New()) == NULL)
            return; /* caller mush check PyErr_Occurred() */
    } else
        wf.refs = NULL;
    wf.version = version;
    w_object(x, &wf);
    Py_XDECREF(wf.refs);
}

typedef WFILE RFILE; /* Same struct with different invariants */

#define rs_byte(p) (((p)->ptr < (p)->end) ? (unsigned char)*(p)->ptr++ : EOF)

static Py_ssize_t
r_string(char *s, Py_ssize_t n, RFILE *p)
{
    char *ptr;
    Py_ssize_t read, left;

    if (!p->readable) {
        if (p->fp != NULL)
            /* The result fits into int because it must be <=n. */
            read = fread(s, 1, n, p->fp);
        else {
            left = p->end - p->ptr;
            read = (left < n) ? left : n;
            memcpy(s, p->ptr, read);
            p->ptr += read;
        }
    }
    else {
        _Py_IDENTIFIER(read);

        PyObject *data = _PyObject_CallMethodId(p->readable, &PyId_read, "n", n);
        read = 0;
        if (data != NULL) {
            if (!PyBytes_Check(data)) {
                PyErr_Format(PyExc_TypeError,
                             "f.read() returned not bytes but %.100s",
                             data->ob_type->tp_name);
            }
            else {
                read = (int)PyBytes_GET_SIZE(data);
                if (read > 0) {
                    ptr = PyBytes_AS_STRING(data);
                    memcpy(s, ptr, read);
                }
            }
            Py_DECREF(data);
        }
    }
    if (!PyErr_Occurred() && (read < n)) {
        PyErr_SetString(PyExc_EOFError, "EOF read where not expected");
    }
    return read;
}


static int
r_byte(RFILE *p)
{
    int c = EOF;
    unsigned char ch;
    Py_ssize_t n;

    if (!p->readable)
        c = p->fp ? getc(p->fp) : rs_byte(p);
    else {
        n = r_string((char *) &ch, 1, p);
        if (n > 0)
            c = ch;
    }
    return c;
}

static int
r_short(RFILE *p)
{
    register short x;
    unsigned char buffer[2];

    r_string((char *) buffer, 2, p);
    x = buffer[0];
    x |= buffer[1] << 8;
    /* Sign-extension, in case short greater than 16 bits */
    x |= -(x & 0x8000);
    return x;
}

static long
r_long(RFILE *p)
{
    register long x;
    unsigned char buffer[4];

    r_string((char *) buffer, 4, p);
    x = buffer[0];
    x |= (long)buffer[1] << 8;
    x |= (long)buffer[2] << 16;
    x |= (long)buffer[3] << 24;
#if SIZEOF_LONG > 4
    /* Sign extension for 64-bit machines */
    x |= -(x & 0x80000000L);
#endif
    return x;
}

/* r_long64 deals with the TYPE_INT64 code.  On a machine with
   sizeof(long) > 4, it returns a Python int object, else a Python long
   object.  Note that w_long64 writes out TYPE_INT if 32 bits is enough,
   so there's no inefficiency here in returning a PyLong on 32-bit boxes
   for everything written via TYPE_INT64 (i.e., if an int is written via
   TYPE_INT64, it *needs* more than 32 bits).
*/
static PyObject *
r_long64(RFILE *p)
{
    PyObject *result = NULL;
    long lo4 = r_long(p);
    long hi4 = r_long(p);

    if (!PyErr_Occurred()) {
#if SIZEOF_LONG > 4
        long x = (hi4 << 32) | (lo4 & 0xFFFFFFFFL);
        result = PyLong_FromLong(x);
#else
        unsigned char buf[8];
        int one = 1;
        int is_little_endian = (int)*(char*)&one;
        if (is_little_endian) {
            memcpy(buf, &lo4, 4);
            memcpy(buf+4, &hi4, 4);
        }
        else {
            memcpy(buf, &hi4, 4);
            memcpy(buf+4, &lo4, 4);
        }
        result = _PyLong_FromByteArray(buf, 8, is_little_endian, 1);
#endif
    }
    return result;
}

static PyObject *
r_PyLong(RFILE *p)
{
    PyLongObject *ob;
    long n, size, i;
    int j, md, shorts_in_top_digit;
    digit d;

    n = r_long(p);
    if (PyErr_Occurred())
        return NULL;
    if (n == 0)
        return (PyObject *)_PyLong_New(0);
    if (n < -SIZE32_MAX || n > SIZE32_MAX) {
        PyErr_SetString(PyExc_ValueError,
                       "bad marshal data (long size out of range)");
        return NULL;
    }

    size = 1 + (ABS(n) - 1) / PyLong_MARSHAL_RATIO;
    shorts_in_top_digit = 1 + (ABS(n) - 1) % PyLong_MARSHAL_RATIO;
    ob = _PyLong_New(size);
    if (ob == NULL)
        return NULL;
    Py_SIZE(ob) = n > 0 ? size : -size;

    for (i = 0; i < size-1; i++) {
        d = 0;
        for (j=0; j < PyLong_MARSHAL_RATIO; j++) {
            md = r_short(p);
            if (PyErr_Occurred())
                break;
            if (md < 0 || md > PyLong_MARSHAL_BASE)
                goto bad_digit;
            d += (digit)md << j*PyLong_MARSHAL_SHIFT;
        }
        ob->ob_digit[i] = d;
    }
    d = 0;
    for (j=0; j < shorts_in_top_digit; j++) {
        md = r_short(p);
        if (PyErr_Occurred())
            break;
        if (md < 0 || md > PyLong_MARSHAL_BASE)
            goto bad_digit;
        /* topmost marshal digit should be nonzero */
        if (md == 0 && j == shorts_in_top_digit - 1) {
            Py_DECREF(ob);
            PyErr_SetString(PyExc_ValueError,
                "bad marshal data (unnormalized long data)");
            return NULL;
        }
        d += (digit)md << j*PyLong_MARSHAL_SHIFT;
    }
    if (PyErr_Occurred()) {
        Py_DECREF(ob);
        return NULL;
    }
    /* top digit should be nonzero, else the resulting PyLong won't be
       normalized */
    ob->ob_digit[size-1] = d;
    return (PyObject *)ob;
  bad_digit:
    Py_DECREF(ob);
    PyErr_SetString(PyExc_ValueError,
                    "bad marshal data (digit out of range in long)");
    return NULL;
}

/* allocate the reflist index */
static PyObject *
r_ref_reserve(PyObject *o, Py_ssize_t *idx, int flag, RFILE *p)
{
    if (flag) { /* currently only FLAG_REF is defined */
        *idx = PyList_Size(p->refs);
        if (*idx < 0)
            goto err;
        if (*idx >= 0x7ffffffe) {
            PyErr_SetString(PyExc_ValueError, "bad marshal data (index list too large)");
            goto err;
        }
        if (PyList_Append(p->refs, Py_None) < 0)
            goto err;
    } else
        *idx = 0;
    return o;
err:
    Py_XDECREF(o);  /* release the new object */
    *idx = -1;
    return NULL;
}

/* insert actual object to the reflist */
static PyObject *
r_ref_insert(PyObject *o, Py_ssize_t idx, int flag, RFILE *p)
{
    if (o && (flag & FLAG_REF)) {
        if (PyList_SetItem(p->refs, idx, o) < 0) {
            Py_DECREF(o); /* release the new object */
            return NULL;
        } else {
            Py_INCREF(o); /* a reference for the list */
        }
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
    if (o && (flag & FLAG_REF)) {
        if (PyList_Append(p->refs, o) < 0) {
            Py_DECREF(o); /* release the new object */
            return NULL;
        }
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
    int type = r_byte(p);
    int flag;
    PyObject *retval;

    p->depth++;

    if (p->depth > MAX_MARSHAL_STACK_DEPTH) {
        p->depth--;
        PyErr_SetString(PyExc_ValueError, "recursion limit exceeded");
        return NULL;
    }

    flag = type & FLAG_REF;
    type = type & ~FLAG_REF;

#define R_REF(O) do{\
    O = r_ref(O, flag, p);\
} while (0)

    switch (type) {

    case EOF:
        PyErr_SetString(PyExc_EOFError,
                        "EOF read where object expected");
        retval = NULL;
        break;

    case TYPE_NULL:
        retval = NULL;
        break;

    case TYPE_NONE:
        Py_INCREF(Py_None);
        retval = Py_None;
        break;

    case TYPE_STOPITER:
        Py_INCREF(PyExc_StopIteration);
        retval = PyExc_StopIteration;
        break;

    case TYPE_ELLIPSIS:
        Py_INCREF(Py_Ellipsis);
        retval = Py_Ellipsis;
        break;

    case TYPE_FALSE:
        Py_INCREF(Py_False);
        retval = Py_False;
        break;

    case TYPE_TRUE:
        Py_INCREF(Py_True);
        retval = Py_True;
        break;

    case TYPE_INT:
        n = r_long(p);
        retval = PyErr_Occurred() ? NULL : PyLong_FromLong(n);
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
            char buf[256];
            double dx;
            retval = NULL;
            n = r_byte(p);
            if (n == EOF || r_string(buf, n, p) != n) {
                PyErr_SetString(PyExc_EOFError,
                    "EOF read where object expected");
                break;
            }
            buf[n] = '\0';
            dx = PyOS_string_to_double(buf, NULL, NULL);
            if (dx == -1.0 && PyErr_Occurred())
                break;
            retval = PyFloat_FromDouble(dx);
            R_REF(retval);
            break;
        }

    case TYPE_BINARY_FLOAT:
        {
            unsigned char buf[8];
            double x;
            if (r_string((char*)buf, 8, p) != 8) {
                PyErr_SetString(PyExc_EOFError,
                    "EOF read where object expected");
                retval = NULL;
                break;
            }
            x = _PyFloat_Unpack8(buf, 1);
            if (x == -1.0 && PyErr_Occurred()) {
                retval = NULL;
                break;
            }
            retval = PyFloat_FromDouble(x);
            R_REF(retval);
            break;
        }

    case TYPE_COMPLEX:
        {
            char buf[256];
            Py_complex c;
            retval = NULL;
            n = r_byte(p);
            if (n == EOF || r_string(buf, n, p) != n) {
                PyErr_SetString(PyExc_EOFError,
                    "EOF read where object expected");
                break;
            }
            buf[n] = '\0';
            c.real = PyOS_string_to_double(buf, NULL, NULL);
            if (c.real == -1.0 && PyErr_Occurred())
                break;
            n = r_byte(p);
            if (n == EOF || r_string(buf, n, p) != n) {
                PyErr_SetString(PyExc_EOFError,
                    "EOF read where object expected");
                break;
            }
            buf[n] = '\0';
            c.imag = PyOS_string_to_double(buf, NULL, NULL);
            if (c.imag == -1.0 && PyErr_Occurred())
                break;
            retval = PyComplex_FromCComplex(c);
            R_REF(retval);
            break;
        }

    case TYPE_BINARY_COMPLEX:
        {
            unsigned char buf[8];
            Py_complex c;
            if (r_string((char*)buf, 8, p) != 8) {
                PyErr_SetString(PyExc_EOFError,
                    "EOF read where object expected");
                retval = NULL;
                break;
            }
            c.real = _PyFloat_Unpack8(buf, 1);
            if (c.real == -1.0 && PyErr_Occurred()) {
                retval = NULL;
                break;
            }
            if (r_string((char*)buf, 8, p) != 8) {
                PyErr_SetString(PyExc_EOFError,
                    "EOF read where object expected");
                retval = NULL;
                break;
            }
            c.imag = _PyFloat_Unpack8(buf, 1);
            if (c.imag == -1.0 && PyErr_Occurred()) {
                retval = NULL;
                break;
            }
            retval = PyComplex_FromCComplex(c);
            R_REF(retval);
            break;
        }

    case TYPE_STRING:
        n = r_long(p);
        if (PyErr_Occurred()) {
            retval = NULL;
            break;
        }
        if (n < 0 || n > SIZE32_MAX) {
            PyErr_SetString(PyExc_ValueError, "bad marshal data (string size out of range)");
            retval = NULL;
            break;
        }
        v = PyBytes_FromStringAndSize((char *)NULL, n);
        if (v == NULL) {
            retval = NULL;
            break;
        }
        if (r_string(PyBytes_AS_STRING(v), n, p) != n) {
            Py_DECREF(v);
            PyErr_SetString(PyExc_EOFError,
                            "EOF read where object expected");
            retval = NULL;
            break;
        }
        retval = v;
        R_REF(retval);
        break;

    case TYPE_UNICODE:
    case TYPE_INTERNED:
        {
        char *buffer;

        n = r_long(p);
        if (PyErr_Occurred()) {
            retval = NULL;
            break;
        }
        if (n < 0 || n > SIZE32_MAX) {
            PyErr_SetString(PyExc_ValueError, "bad marshal data (unicode size out of range)");
            retval = NULL;
            break;
        }
        buffer = PyMem_NEW(char, n);
        if (buffer == NULL) {
            retval = PyErr_NoMemory();
            break;
        }
        if (r_string(buffer, n, p) != n) {
            PyMem_DEL(buffer);
            PyErr_SetString(PyExc_EOFError,
                "EOF read where object expected");
            retval = NULL;
            break;
        }
        v = PyUnicode_DecodeUTF8(buffer, n, "surrogatepass");
        PyMem_DEL(buffer);
        if (type == TYPE_INTERNED)
            PyUnicode_InternInPlace(&v);
        retval = v;
        R_REF(retval);
        break;
        }

    case TYPE_TUPLE:
        n = r_long(p);
        if (PyErr_Occurred()) {
            retval = NULL;
            break;
        }
        if (n < 0 || n > SIZE32_MAX) {
            PyErr_SetString(PyExc_ValueError, "bad marshal data (tuple size out of range)");
            retval = NULL;
            break;
        }
        v = PyTuple_New(n);
        R_REF(v);
        if (v == NULL) {
            retval = NULL;
            break;
        }
        for (i = 0; i < n; i++) {
            v2 = r_object(p);
            if ( v2 == NULL ) {
                if (!PyErr_Occurred())
                    PyErr_SetString(PyExc_TypeError,
                        "NULL object in marshal data for tuple");
                Py_DECREF(v);
                v = NULL;
                break;
            }
            PyTuple_SET_ITEM(v, i, v2);
        }
        retval = v;
        break;

    case TYPE_LIST:
        n = r_long(p);
        if (PyErr_Occurred()) {
            retval = NULL;
            break;
        }
        if (n < 0 || n > SIZE32_MAX) {
            PyErr_SetString(PyExc_ValueError, "bad marshal data (list size out of range)");
            retval = NULL;
            break;
        }
        v = PyList_New(n);
        R_REF(v);
        if (v == NULL) {
            retval = NULL;
            break;
        }
        for (i = 0; i < n; i++) {
            v2 = r_object(p);
            if ( v2 == NULL ) {
                if (!PyErr_Occurred())
                    PyErr_SetString(PyExc_TypeError,
                        "NULL object in marshal data for list");
                Py_DECREF(v);
                v = NULL;
                break;
            }
            PyList_SET_ITEM(v, i, v2);
        }
        retval = v;
        break;

    case TYPE_DICT:
        v = PyDict_New();
        R_REF(v);
        if (v == NULL) {
            retval = NULL;
            break;
        }
        for (;;) {
            PyObject *key, *val;
            key = r_object(p);
            if (key == NULL)
                break;
            val = r_object(p);
            if (val != NULL)
                PyDict_SetItem(v, key, val);
            Py_DECREF(key);
            Py_XDECREF(val);
        }
        if (PyErr_Occurred()) {
            Py_DECREF(v);
            v = NULL;
        }
        retval = v;
        break;

    case TYPE_SET:
    case TYPE_FROZENSET:
        n = r_long(p);
        if (PyErr_Occurred()) {
            retval = NULL;
            break;
        }
        if (n < 0 || n > SIZE32_MAX) {
            PyErr_SetString(PyExc_ValueError, "bad marshal data (set size out of range)");
            retval = NULL;
            break;
        }
        v = (type == TYPE_SET) ? PySet_New(NULL) : PyFrozenSet_New(NULL);
        /* must use delayed registration of frozensets because they must
         * be init with a refcount of 1
         */
        if (type == TYPE_SET)
            R_REF(v);
        else
            v = r_ref_reserve(v, &idx, flag, p);
        if (v == NULL) {
            retval = NULL;
            break;
        }
        for (i = 0; i < n; i++) {
            v2 = r_object(p);
            if ( v2 == NULL ) {
                if (!PyErr_Occurred())
                    PyErr_SetString(PyExc_TypeError,
                        "NULL object in marshal data for set");
                Py_DECREF(v);
                v = NULL;
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
        break;

    case TYPE_CODE:
        {
            int argcount;
            int kwonlyargcount;
            int nlocals;
            int stacksize;
            int flags;
            PyObject *code = NULL;
            PyObject *consts = NULL;
            PyObject *names = NULL;
            PyObject *varnames = NULL;
            PyObject *freevars = NULL;
            PyObject *cellvars = NULL;
            PyObject *filename = NULL;
            PyObject *name = NULL;
            int firstlineno;
            PyObject *lnotab = NULL;
            
            r_ref_reserve(NULL, &idx, flag, p);
            if (idx < 0) {
                retval = NULL;
                break;
            }

            v = NULL;

            /* XXX ignore long->int overflows for now */
            argcount = (int)r_long(p);
            if (PyErr_Occurred())
                goto code_error;
            kwonlyargcount = (int)r_long(p);
            if (PyErr_Occurred())
                goto code_error;
            nlocals = (int)r_long(p);
            if (PyErr_Occurred())
                goto code_error;
            stacksize = (int)r_long(p);
            if (PyErr_Occurred())
                goto code_error;
            flags = (int)r_long(p);
            if (PyErr_Occurred())
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
            varnames = r_object(p);
            if (varnames == NULL)
                goto code_error;
            freevars = r_object(p);
            if (freevars == NULL)
                goto code_error;
            cellvars = r_object(p);
            if (cellvars == NULL)
                goto code_error;
            filename = r_object(p);
            if (filename == NULL)
                goto code_error;
            if (PyUnicode_CheckExact(filename)) {
                if (p->current_filename != NULL) {
                    if (!PyUnicode_Compare(filename, p->current_filename)) {
                        Py_DECREF(filename);
                        Py_INCREF(p->current_filename);
                        filename = p->current_filename;
                    }
                }
                else {
                    p->current_filename = filename;
                }
            }
            name = r_object(p);
            if (name == NULL)
                goto code_error;
            firstlineno = (int)r_long(p);
            lnotab = r_object(p);
            if (lnotab == NULL)
                goto code_error;

            v = (PyObject *) PyCode_New(
                            argcount, kwonlyargcount,
                            nlocals, stacksize, flags,
                            code, consts, names, varnames,
                            freevars, cellvars, filename, name,
                            firstlineno, lnotab);
            v = r_ref_insert(v, idx, flag, p);

          code_error:
            Py_XDECREF(code);
            Py_XDECREF(consts);
            Py_XDECREF(names);
            Py_XDECREF(varnames);
            Py_XDECREF(freevars);
            Py_XDECREF(cellvars);
            Py_XDECREF(filename);
            Py_XDECREF(name);
            Py_XDECREF(lnotab);
        }
        retval = v;
        break;

    case TYPE_REF:
        n = r_long(p);
        if (n < 0 || n >= PyList_GET_SIZE(p->refs)) {
            PyErr_SetString(PyExc_ValueError, "bad marshal data (invalid reference)");
            retval = NULL;
            break;
        }
        v = PyList_GET_ITEM(p->refs, n);
        if (v == Py_None) {
            PyErr_SetString(PyExc_ValueError, "bad marshal data (invalid reference)");
            retval = NULL;
            break;
        }
        Py_INCREF(v);
        retval = v;
        break;

    default:
        /* Bogus data got written, which isn't ideal.
           This will let you keep working and recover. */
        PyErr_SetString(PyExc_ValueError, "bad marshal data (unknown type code)");
        retval = NULL;
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
    v = r_object(p);
    if (v == NULL && !PyErr_Occurred())
        PyErr_SetString(PyExc_TypeError, "NULL object in marshal data for object");
    return v;
}

int
PyMarshal_ReadShortFromFile(FILE *fp)
{
    RFILE rf;
    assert(fp);
    rf.readable = NULL;
    rf.fp = fp;
    rf.current_filename = NULL;
    rf.end = rf.ptr = NULL;
    return r_short(&rf);
}

long
PyMarshal_ReadLongFromFile(FILE *fp)
{
    RFILE rf;
    rf.fp = fp;
    rf.readable = NULL;
    rf.current_filename = NULL;
    rf.ptr = rf.end = NULL;
    return r_long(&rf);
}

#ifdef HAVE_FSTAT
/* Return size of file in bytes; < 0 if unknown. */
static off_t
getfilesize(FILE *fp)
{
    struct stat st;
    if (fstat(fileno(fp), &st) != 0)
        return -1;
    else
        return st.st_size;
}
#endif

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
#ifdef HAVE_FSTAT
    off_t filesize;
    filesize = getfilesize(fp);
    if (filesize > 0 && filesize <= REASONABLE_FILE_LIMIT) {
        char* pBuf = (char *)PyMem_MALLOC(filesize);
        if (pBuf != NULL) {
            size_t n = fread(pBuf, 1, (size_t)filesize, fp);
            PyObject* v = PyMarshal_ReadObjectFromString(pBuf, n);
            PyMem_FREE(pBuf);
            return v;
        }

    }
#endif
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
    rf.fp = fp;
    rf.readable = NULL;
    rf.current_filename = NULL;
    rf.depth = 0;
    rf.ptr = rf.end = NULL;
    rf.refs = PyList_New(0);
    if (rf.refs == NULL)
        return NULL;
    result = r_object(&rf);
    Py_DECREF(rf.refs);
    return result;
}

PyObject *
PyMarshal_ReadObjectFromString(char *str, Py_ssize_t len)
{
    RFILE rf;
    PyObject *result;
    rf.fp = NULL;
    rf.readable = NULL;
    rf.current_filename = NULL;
    rf.ptr = str;
    rf.end = str + len;
    rf.depth = 0;
    rf.refs = PyList_New(0);
    if (rf.refs == NULL)
        return NULL;
    result = r_object(&rf);
    Py_DECREF(rf.refs);
    return result;
}

PyObject *
PyMarshal_WriteObjectToString(PyObject *x, int version)
{
    WFILE wf;

    wf.fp = NULL;
    wf.readable = NULL;
    wf.str = PyBytes_FromStringAndSize((char *)NULL, 50);
    if (wf.str == NULL)
        return NULL;
    wf.ptr = PyBytes_AS_STRING((PyBytesObject *)wf.str);
    wf.end = wf.ptr + PyBytes_Size(wf.str);
    wf.error = WFERR_OK;
    wf.depth = 0;
    wf.version = version;
    if (version >= 3) {
        if ((wf.refs = PyDict_New()) == NULL)
            return NULL;
    } else
        wf.refs = NULL;
    w_object(x, &wf);
    Py_XDECREF(wf.refs);
    if (wf.str != NULL) {
        char *base = PyBytes_AS_STRING((PyBytesObject *)wf.str);
        if (wf.ptr - base > PY_SSIZE_T_MAX) {
            Py_DECREF(wf.str);
            PyErr_SetString(PyExc_OverflowError,
                            "too much marshal data for a string");
            return NULL;
        }
        if (_PyBytes_Resize(&wf.str, (Py_ssize_t)(wf.ptr - base)) < 0)
            return NULL;
    }
    if (wf.error != WFERR_OK) {
        Py_XDECREF(wf.str);
        if (wf.error == WFERR_NOMEMORY)
            PyErr_NoMemory();
        else
            PyErr_SetString(PyExc_ValueError,
              (wf.error==WFERR_UNMARSHALLABLE)?"unmarshallable object"
               :"object too deeply nested to marshal");
        return NULL;
    }
    return wf.str;
}

/* And an interface for Python programs... */

static PyObject *
marshal_dump(PyObject *self, PyObject *args)
{
    /* XXX Quick hack -- need to do this differently */
    PyObject *x;
    PyObject *f;
    int version = Py_MARSHAL_VERSION;
    PyObject *s;
    PyObject *res;
    _Py_IDENTIFIER(write);

    if (!PyArg_ParseTuple(args, "OO|i:dump", &x, &f, &version))
        return NULL;
    s = PyMarshal_WriteObjectToString(x, version);
    if (s == NULL)
        return NULL;
    res = _PyObject_CallMethodId(f, &PyId_write, "O", s);
    Py_DECREF(s);
    return res;
}

PyDoc_STRVAR(dump_doc,
"dump(value, file[, version])\n\
\n\
Write the value on the open file. The value must be a supported type.\n\
The file must be an open file object such as sys.stdout or returned by\n\
open() or os.popen(). It must be opened in binary mode ('wb' or 'w+b').\n\
\n\
If the value has (or contains an object that has) an unsupported type, a\n\
ValueError exception is raised — but garbage data will also be written\n\
to the file. The object will not be properly read back by load()\n\
\n\
The version argument indicates the data format that dump should use.");

static PyObject *
marshal_load(PyObject *self, PyObject *f)
{
    PyObject *data, *result;
    _Py_IDENTIFIER(read);
    RFILE rf;

    /*
     * Make a call to the read method, but read zero bytes.
     * This is to ensure that the object passed in at least
     * has a read method which returns bytes.
     * This can be removed if we guarantee good error handling
     * for r_string()
     */
    data = _PyObject_CallMethodId(f, &PyId_read, "i", 0);
    if (data == NULL)
        return NULL;
    if (!PyBytes_Check(data)) {
        PyErr_Format(PyExc_TypeError,
                     "f.read() returned not bytes but %.100s",
                     data->ob_type->tp_name);
        result = NULL;
    }
    else {
        rf.depth = 0;
        rf.fp = NULL;
        rf.readable = f;
        rf.current_filename = NULL;
        if ((rf.refs = PyList_New(0)) != NULL) {
            result = read_object(&rf);
            Py_DECREF(rf.refs);
        } else
            result = NULL;
    }
    Py_DECREF(data);
    return result;
}

PyDoc_STRVAR(load_doc,
"load(file)\n\
\n\
Read one value from the open file and return it. If no valid value is\n\
read (e.g. because the data has a different Python version’s\n\
incompatible marshal format), raise EOFError, ValueError or TypeError.\n\
The file must be an open file object opened in binary mode ('rb' or\n\
'r+b').\n\
\n\
Note: If an object containing an unsupported type was marshalled with\n\
dump(), load() will substitute None for the unmarshallable type.");


static PyObject *
marshal_dumps(PyObject *self, PyObject *args)
{
    PyObject *x;
    int version = Py_MARSHAL_VERSION;
    if (!PyArg_ParseTuple(args, "O|i:dumps", &x, &version))
        return NULL;
    return PyMarshal_WriteObjectToString(x, version);
}

PyDoc_STRVAR(dumps_doc,
"dumps(value[, version])\n\
\n\
Return the string that would be written to a file by dump(value, file).\n\
The value must be a supported type. Raise a ValueError exception if\n\
value has (or contains an object that has) an unsupported type.\n\
\n\
The version argument indicates the data format that dumps should use.");


static PyObject *
marshal_loads(PyObject *self, PyObject *args)
{
    RFILE rf;
    Py_buffer p;
    char *s;
    Py_ssize_t n;
    PyObject* result;
    if (!PyArg_ParseTuple(args, "y*:loads", &p))
        return NULL;
    s = p.buf;
    n = p.len;
    rf.fp = NULL;
    rf.readable = NULL;
    rf.current_filename = NULL;
    rf.ptr = s;
    rf.end = s + n;
    rf.depth = 0;
    if ((rf.refs = PyList_New(0)) == NULL)
        return NULL;
    result = read_object(&rf);
    PyBuffer_Release(&p);
    Py_DECREF(rf.refs);
    return result;
}

PyDoc_STRVAR(loads_doc,
"loads(bytes)\n\
\n\
Convert the bytes object to a value. If no valid value is found, raise\n\
EOFError, ValueError or TypeError. Extra characters in the input are\n\
ignored.");

static PyMethodDef marshal_methods[] = {
    {"dump",            marshal_dump,   METH_VARARGS,   dump_doc},
    {"load",            marshal_load,   METH_O,         load_doc},
    {"dumps",           marshal_dumps,  METH_VARARGS,   dumps_doc},
    {"loads",           marshal_loads,  METH_VARARGS,   loads_doc},
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
None, integers, floating point numbers, strings, bytes, bytearrays,\n\
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
    uses a binary format for floating point numbers.\n\
    Version 3 shares common object references (New in version 3.4).\n\
\n\
Functions:\n\
\n\
dump() -- write value to a file\n\
load() -- read value from a file\n\
dumps() -- write value to a string\n\
loads() -- read value from a string");



static struct PyModuleDef marshalmodule = {
    PyModuleDef_HEAD_INIT,
    "marshal",
    module_doc,
    0,
    marshal_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyMarshal_Init(void)
{
    PyObject *mod = PyModule_Create(&marshalmodule);
    if (mod == NULL)
        return NULL;
    PyModule_AddIntConstant(mod, "version", Py_MARSHAL_VERSION);
    return mod;
}
