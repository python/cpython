/* JSON accelerator C extensor: _json module.
 *
 * It is built as a built-in module (Py_BUILD_CORE_BUILTIN define) on Windows
 * and as an extension module (Py_BUILD_CORE_MODULE define) on other
 * platforms. */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_ceval.h"         // _Py_EnterRecursiveCall()
#include "pycore_global_strings.h" // _Py_ID()
#include "pycore_pyerrors.h"      // _PyErr_FormatNote
#include "pycore_runtime.h"       // _PyRuntime
#include "pycore_unicodeobject.h" // _PyUnicode_CheckConsistency()

#include <stdbool.h>              // bool


typedef struct _PyScannerObject {
    PyObject_HEAD
    signed char strict;
    PyObject *object_hook;
    PyObject *object_pairs_hook;
    PyObject *parse_float;
    PyObject *parse_int;
    PyObject *parse_constant;
} PyScannerObject;

#define PyScannerObject_CAST(op)    ((PyScannerObject *)(op))

static PyMemberDef scanner_members[] = {
    {"strict", Py_T_BOOL, offsetof(PyScannerObject, strict), Py_READONLY, "strict"},
    {"object_hook", _Py_T_OBJECT, offsetof(PyScannerObject, object_hook), Py_READONLY, "object_hook"},
    {"object_pairs_hook", _Py_T_OBJECT, offsetof(PyScannerObject, object_pairs_hook), Py_READONLY},
    {"parse_float", _Py_T_OBJECT, offsetof(PyScannerObject, parse_float), Py_READONLY, "parse_float"},
    {"parse_int", _Py_T_OBJECT, offsetof(PyScannerObject, parse_int), Py_READONLY, "parse_int"},
    {"parse_constant", _Py_T_OBJECT, offsetof(PyScannerObject, parse_constant), Py_READONLY, "parse_constant"},
    {NULL}
};

typedef struct _PyEncoderObject {
    PyObject_HEAD
    PyObject *markers;
    PyObject *defaultfn;
    PyObject *encoder;
    PyObject *indent;
    PyObject *key_separator;
    PyObject *item_separator;
    char sort_keys;
    char skipkeys;
    int allow_nan;
    PyCFunction fast_encode;
} PyEncoderObject;

#define PyEncoderObject_CAST(op)    ((PyEncoderObject *)(op))

static PyMemberDef encoder_members[] = {
    {"markers", _Py_T_OBJECT, offsetof(PyEncoderObject, markers), Py_READONLY, "markers"},
    {"default", _Py_T_OBJECT, offsetof(PyEncoderObject, defaultfn), Py_READONLY, "default"},
    {"encoder", _Py_T_OBJECT, offsetof(PyEncoderObject, encoder), Py_READONLY, "encoder"},
    {"indent", _Py_T_OBJECT, offsetof(PyEncoderObject, indent), Py_READONLY, "indent"},
    {"key_separator", _Py_T_OBJECT, offsetof(PyEncoderObject, key_separator), Py_READONLY, "key_separator"},
    {"item_separator", _Py_T_OBJECT, offsetof(PyEncoderObject, item_separator), Py_READONLY, "item_separator"},
    {"sort_keys", Py_T_BOOL, offsetof(PyEncoderObject, sort_keys), Py_READONLY, "sort_keys"},
    {"skipkeys", Py_T_BOOL, offsetof(PyEncoderObject, skipkeys), Py_READONLY, "skipkeys"},
    {NULL}
};

/* Forward decls */

static PyObject *
ascii_escape_unicode(PyObject *pystr);
static PyObject *
py_encode_basestring_ascii(PyObject* Py_UNUSED(self), PyObject *pystr);

static PyObject *
scan_once_unicode(PyScannerObject *s, PyObject *memo, PyObject *pystr, Py_ssize_t idx, Py_ssize_t *next_idx_ptr);
static PyObject *
_build_rval_index_tuple(PyObject *rval, Py_ssize_t idx);
static PyObject *
scanner_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static void
scanner_dealloc(PyObject *self);
static int
scanner_clear(PyObject *self);

static PyObject *
encoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static void
encoder_dealloc(PyObject *self);
static int
encoder_clear(PyObject *self);
static int
encoder_listencode_list(PyEncoderObject *s, PyUnicodeWriter *writer, PyObject *seq, Py_ssize_t indent_level, PyObject *indent_cache);
static int
encoder_listencode_obj(PyEncoderObject *s, PyUnicodeWriter *writer, PyObject *obj, Py_ssize_t indent_level, PyObject *indent_cache);
static int
encoder_listencode_dict(PyEncoderObject *s, PyUnicodeWriter *writer, PyObject *dct, Py_ssize_t indent_level, PyObject *indent_cache);
static PyObject *
_encoded_const(PyObject *obj);
static void
raise_errmsg(const char *msg, PyObject *s, Py_ssize_t end);
static PyObject *
encoder_encode_string(PyEncoderObject *s, PyObject *obj);
static PyObject *
encoder_encode_float(PyEncoderObject *s, PyObject *obj);

#define S_CHAR(c) (c >= ' ' && c <= '~' && c != '\\' && c != '"')
#define IS_WHITESPACE(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\n') || ((c) == '\r'))

static Py_ssize_t
ascii_escape_unichar(Py_UCS4 c, unsigned char *output, Py_ssize_t chars)
{
    /* Escape unicode code point c to ASCII escape sequences
    in char *output. output must have at least 12 bytes unused to
    accommodate an escaped surrogate pair "\uXXXX\uXXXX" */
    output[chars++] = '\\';
    switch (c) {
        case '\\': output[chars++] = c; break;
        case '"': output[chars++] = c; break;
        case '\b': output[chars++] = 'b'; break;
        case '\f': output[chars++] = 'f'; break;
        case '\n': output[chars++] = 'n'; break;
        case '\r': output[chars++] = 'r'; break;
        case '\t': output[chars++] = 't'; break;
        default:
            if (c >= 0x10000) {
                /* UTF-16 surrogate pair */
                Py_UCS4 v = Py_UNICODE_HIGH_SURROGATE(c);
                output[chars++] = 'u';
                output[chars++] = Py_hexdigits[(v >> 12) & 0xf];
                output[chars++] = Py_hexdigits[(v >>  8) & 0xf];
                output[chars++] = Py_hexdigits[(v >>  4) & 0xf];
                output[chars++] = Py_hexdigits[(v      ) & 0xf];
                c = Py_UNICODE_LOW_SURROGATE(c);
                output[chars++] = '\\';
            }
            output[chars++] = 'u';
            output[chars++] = Py_hexdigits[(c >> 12) & 0xf];
            output[chars++] = Py_hexdigits[(c >>  8) & 0xf];
            output[chars++] = Py_hexdigits[(c >>  4) & 0xf];
            output[chars++] = Py_hexdigits[(c      ) & 0xf];
    }
    return chars;
}

static PyObject *
ascii_escape_unicode(PyObject *pystr)
{
    /* Take a PyUnicode pystr and return a new ASCII-only escaped PyUnicode */
    Py_ssize_t i;
    Py_ssize_t input_chars;
    Py_ssize_t output_size;
    Py_ssize_t chars;
    PyObject *rval;
    const void *input;
    Py_UCS1 *output;
    int kind;

    input_chars = PyUnicode_GET_LENGTH(pystr);
    input = PyUnicode_DATA(pystr);
    kind = PyUnicode_KIND(pystr);

    /* Compute the output size */
    for (i = 0, output_size = 2; i < input_chars; i++) {
        Py_UCS4 c = PyUnicode_READ(kind, input, i);
        Py_ssize_t d;
        if (S_CHAR(c)) {
            d = 1;
        }
        else {
            switch(c) {
            case '\\': case '"': case '\b': case '\f':
            case '\n': case '\r': case '\t':
                d = 2; break;
            default:
                d = c >= 0x10000 ? 12 : 6;
            }
        }
        if (output_size > PY_SSIZE_T_MAX - d) {
            PyErr_SetString(PyExc_OverflowError, "string is too long to escape");
            return NULL;
        }
        output_size += d;
    }

    rval = PyUnicode_New(output_size, 127);
    if (rval == NULL) {
        return NULL;
    }
    output = PyUnicode_1BYTE_DATA(rval);
    chars = 0;
    output[chars++] = '"';
    for (i = 0; i < input_chars; i++) {
        Py_UCS4 c = PyUnicode_READ(kind, input, i);
        if (S_CHAR(c)) {
            output[chars++] = c;
        }
        else {
            chars = ascii_escape_unichar(c, output, chars);
        }
    }
    output[chars++] = '"';
#ifdef Py_DEBUG
    assert(_PyUnicode_CheckConsistency(rval, 1));
#endif
    return rval;
}

static PyObject *
escape_unicode(PyObject *pystr)
{
    /* Take a PyUnicode pystr and return a new escaped PyUnicode */
    Py_ssize_t i;
    Py_ssize_t input_chars;
    Py_ssize_t output_size;
    Py_ssize_t chars;
    PyObject *rval;
    const void *input;
    int kind;
    Py_UCS4 maxchar;

    maxchar = PyUnicode_MAX_CHAR_VALUE(pystr);
    input_chars = PyUnicode_GET_LENGTH(pystr);
    input = PyUnicode_DATA(pystr);
    kind = PyUnicode_KIND(pystr);

    /* Compute the output size */
    for (i = 0, output_size = 2; i < input_chars; i++) {
        Py_UCS4 c = PyUnicode_READ(kind, input, i);
        Py_ssize_t d;
        switch (c) {
        case '\\': case '"': case '\b': case '\f':
        case '\n': case '\r': case '\t':
            d = 2;
            break;
        default:
            if (c <= 0x1f)
                d = 6;
            else
                d = 1;
        }
        if (output_size > PY_SSIZE_T_MAX - d) {
            PyErr_SetString(PyExc_OverflowError, "string is too long to escape");
            return NULL;
        }
        output_size += d;
    }

    rval = PyUnicode_New(output_size, maxchar);
    if (rval == NULL)
        return NULL;

    kind = PyUnicode_KIND(rval);

#define ENCODE_OUTPUT do { \
        chars = 0; \
        output[chars++] = '"'; \
        for (i = 0; i < input_chars; i++) { \
            Py_UCS4 c = PyUnicode_READ(kind, input, i); \
            switch (c) { \
            case '\\': output[chars++] = '\\'; output[chars++] = c; break; \
            case '"':  output[chars++] = '\\'; output[chars++] = c; break; \
            case '\b': output[chars++] = '\\'; output[chars++] = 'b'; break; \
            case '\f': output[chars++] = '\\'; output[chars++] = 'f'; break; \
            case '\n': output[chars++] = '\\'; output[chars++] = 'n'; break; \
            case '\r': output[chars++] = '\\'; output[chars++] = 'r'; break; \
            case '\t': output[chars++] = '\\'; output[chars++] = 't'; break; \
            default: \
                if (c <= 0x1f) { \
                    output[chars++] = '\\'; \
                    output[chars++] = 'u'; \
                    output[chars++] = '0'; \
                    output[chars++] = '0'; \
                    output[chars++] = Py_hexdigits[(c >> 4) & 0xf]; \
                    output[chars++] = Py_hexdigits[(c     ) & 0xf]; \
                } else { \
                    output[chars++] = c; \
                } \
            } \
        } \
        output[chars++] = '"'; \
    } while (0)

    if (kind == PyUnicode_1BYTE_KIND) {
        Py_UCS1 *output = PyUnicode_1BYTE_DATA(rval);
        ENCODE_OUTPUT;
    } else if (kind == PyUnicode_2BYTE_KIND) {
        Py_UCS2 *output = PyUnicode_2BYTE_DATA(rval);
        ENCODE_OUTPUT;
    } else {
        Py_UCS4 *output = PyUnicode_4BYTE_DATA(rval);
        assert(kind == PyUnicode_4BYTE_KIND);
        ENCODE_OUTPUT;
    }
#undef ENCODE_OUTPUT

#ifdef Py_DEBUG
    assert(_PyUnicode_CheckConsistency(rval, 1));
#endif
    return rval;
}

static void
raise_errmsg(const char *msg, PyObject *s, Py_ssize_t end)
{
    /* Use JSONDecodeError exception to raise a nice looking ValueError subclass */
    _Py_DECLARE_STR(json_decoder, "json.decoder");
    PyObject *JSONDecodeError =
         PyImport_ImportModuleAttr(&_Py_STR(json_decoder), &_Py_ID(JSONDecodeError));
    if (JSONDecodeError == NULL) {
        return;
    }

    PyObject *exc;
    exc = PyObject_CallFunction(JSONDecodeError, "zOn", msg, s, end);
    Py_DECREF(JSONDecodeError);
    if (exc) {
        PyErr_SetObject(JSONDecodeError, exc);
        Py_DECREF(exc);
    }
}

static void
raise_stop_iteration(Py_ssize_t idx)
{
    PyObject *value = PyLong_FromSsize_t(idx);
    if (value != NULL) {
        PyErr_SetObject(PyExc_StopIteration, value);
        Py_DECREF(value);
    }
}

static PyObject *
_build_rval_index_tuple(PyObject *rval, Py_ssize_t idx) {
    /* return (rval, idx) tuple, stealing reference to rval */
    PyObject *tpl;
    PyObject *pyidx;
    /*
    steal a reference to rval, returns (rval, idx)
    */
    if (rval == NULL) {
        return NULL;
    }
    pyidx = PyLong_FromSsize_t(idx);
    if (pyidx == NULL) {
        Py_DECREF(rval);
        return NULL;
    }
    tpl = PyTuple_New(2);
    if (tpl == NULL) {
        Py_DECREF(pyidx);
        Py_DECREF(rval);
        return NULL;
    }
    PyTuple_SET_ITEM(tpl, 0, rval);
    PyTuple_SET_ITEM(tpl, 1, pyidx);
    return tpl;
}

static inline int
_PyUnicodeWriter_IsEmpty(PyUnicodeWriter *writer_pub)
{
    _PyUnicodeWriter *writer = (_PyUnicodeWriter*)writer_pub;
    return (writer->pos == 0);
}

static PyObject *
scanstring_unicode(PyObject *pystr, Py_ssize_t end, int strict, Py_ssize_t *next_end_ptr)
{
    /* Read the JSON string from PyUnicode pystr.
    end is the index of the first character after the quote.
    if strict is zero then literal control characters are allowed
    *next_end_ptr is a return-by-reference index of the character
        after the end quote

    Return value is a new PyUnicode
    */
    PyObject *rval = NULL;
    Py_ssize_t len;
    Py_ssize_t begin = end - 1;
    Py_ssize_t next /* = begin */;
    const void *buf;
    int kind;

    PyUnicodeWriter *writer = PyUnicodeWriter_Create(0);
    if (writer == NULL) {
        goto bail;
    }

    len = PyUnicode_GET_LENGTH(pystr);
    buf = PyUnicode_DATA(pystr);
    kind = PyUnicode_KIND(pystr);

    if (end < 0 || len < end) {
        PyErr_SetString(PyExc_ValueError, "end is out of bounds");
        goto bail;
    }
    while (1) {
        /* Find the end of the string or the next escape */
        Py_UCS4 c;
        {
            // Use tight scope variable to help register allocation.
            Py_UCS4 d = 0;
            for (next = end; next < len; next++) {
                d = PyUnicode_READ(kind, buf, next);
                if (d == '"' || d == '\\') {
                    break;
                }
                if (d <= 0x1f && strict) {
                    raise_errmsg("Invalid control character at", pystr, next);
                    goto bail;
                }
            }
            c = d;
        }

        if (c == '"') {
            // Fast path for simple case.
            if (_PyUnicodeWriter_IsEmpty(writer)) {
                PyObject *ret = PyUnicode_Substring(pystr, end, next);
                if (ret == NULL) {
                    goto bail;
                }
                PyUnicodeWriter_Discard(writer);
                *next_end_ptr = next + 1;;
                return ret;
            }
        }
        else if (c != '\\') {
            raise_errmsg("Unterminated string starting at", pystr, begin);
            goto bail;
        }

        /* Pick up this chunk if it's not zero length */
        if (next != end) {
            if (PyUnicodeWriter_WriteSubstring(writer, pystr, end, next) < 0) {
                goto bail;
            }
        }
        next++;
        if (c == '"') {
            end = next;
            break;
        }
        if (next == len) {
            raise_errmsg("Unterminated string starting at", pystr, begin);
            goto bail;
        }
        c = PyUnicode_READ(kind, buf, next);
        if (c != 'u') {
            /* Non-unicode backslash escapes */
            end = next + 1;
            switch (c) {
                case '"': break;
                case '\\': break;
                case '/': break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                default: c = 0;
            }
            if (c == 0) {
                raise_errmsg("Invalid \\escape", pystr, end - 2);
                goto bail;
            }
        }
        else {
            c = 0;
            next++;
            end = next + 4;
            if (end >= len) {
                raise_errmsg("Invalid \\uXXXX escape", pystr, next - 1);
                goto bail;
            }
            /* Decode 4 hex digits */
            for (; next < end; next++) {
                Py_UCS4 digit = PyUnicode_READ(kind, buf, next);
                c <<= 4;
                switch (digit) {
                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                        c |= (digit - '0'); break;
                    case 'a': case 'b': case 'c': case 'd': case 'e':
                    case 'f':
                        c |= (digit - 'a' + 10); break;
                    case 'A': case 'B': case 'C': case 'D': case 'E':
                    case 'F':
                        c |= (digit - 'A' + 10); break;
                    default:
                        raise_errmsg("Invalid \\uXXXX escape", pystr, end - 5);
                        goto bail;
                }
            }
            /* Surrogate pair */
            if (Py_UNICODE_IS_HIGH_SURROGATE(c) && end + 6 < len &&
                PyUnicode_READ(kind, buf, next++) == '\\' &&
                PyUnicode_READ(kind, buf, next++) == 'u') {
                Py_UCS4 c2 = 0;
                end += 6;
                /* Decode 4 hex digits */
                for (; next < end; next++) {
                    Py_UCS4 digit = PyUnicode_READ(kind, buf, next);
                    c2 <<= 4;
                    switch (digit) {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                            c2 |= (digit - '0'); break;
                        case 'a': case 'b': case 'c': case 'd': case 'e':
                        case 'f':
                            c2 |= (digit - 'a' + 10); break;
                        case 'A': case 'B': case 'C': case 'D': case 'E':
                        case 'F':
                            c2 |= (digit - 'A' + 10); break;
                        default:
                            raise_errmsg("Invalid \\uXXXX escape", pystr, end - 5);
                            goto bail;
                    }
                }
                if (Py_UNICODE_IS_LOW_SURROGATE(c2))
                    c = Py_UNICODE_JOIN_SURROGATES(c, c2);
                else
                    end -= 6;
            }
        }
        if (PyUnicodeWriter_WriteChar(writer, c) < 0) {
            goto bail;
        }
    }

    rval = PyUnicodeWriter_Finish(writer);
    *next_end_ptr = end;
    return rval;

bail:
    *next_end_ptr = -1;
    PyUnicodeWriter_Discard(writer);
    return NULL;
}

PyDoc_STRVAR(pydoc_scanstring,
    "scanstring(string, end, strict=True) -> (string, end)\n"
    "\n"
    "Scan the string s for a JSON string. End is the index of the\n"
    "character in s after the quote that started the JSON string.\n"
    "Unescapes all valid JSON string escape sequences and raises ValueError\n"
    "on attempt to decode an invalid string. If strict is False then literal\n"
    "control characters are allowed in the string.\n"
    "\n"
    "Returns a tuple of the decoded string and the index of the character in s\n"
    "after the end quote."
);

static PyObject *
py_scanstring(PyObject* Py_UNUSED(self), PyObject *args)
{
    PyObject *pystr;
    PyObject *rval;
    Py_ssize_t end;
    Py_ssize_t next_end = -1;
    int strict = 1;
    if (!PyArg_ParseTuple(args, "On|p:scanstring", &pystr, &end, &strict)) {
        return NULL;
    }
    if (PyUnicode_Check(pystr)) {
        rval = scanstring_unicode(pystr, end, strict, &next_end);
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "first argument must be a string, not %.80s",
                     Py_TYPE(pystr)->tp_name);
        return NULL;
    }
    return _build_rval_index_tuple(rval, next_end);
}

PyDoc_STRVAR(pydoc_encode_basestring_ascii,
    "encode_basestring_ascii(string) -> string\n"
    "\n"
    "Return an ASCII-only JSON representation of a Python string"
);

static PyObject *
py_encode_basestring_ascii(PyObject* Py_UNUSED(self), PyObject *pystr)
{
    PyObject *rval;
    /* Return an ASCII-only JSON representation of a Python string */
    /* METH_O */
    if (PyUnicode_Check(pystr)) {
        rval = ascii_escape_unicode(pystr);
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "first argument must be a string, not %.80s",
                     Py_TYPE(pystr)->tp_name);
        return NULL;
    }
    return rval;
}


PyDoc_STRVAR(pydoc_encode_basestring,
    "encode_basestring(string) -> string\n"
    "\n"
    "Return a JSON representation of a Python string"
);

static PyObject *
py_encode_basestring(PyObject* Py_UNUSED(self), PyObject *pystr)
{
    PyObject *rval;
    /* Return a JSON representation of a Python string */
    /* METH_O */
    if (PyUnicode_Check(pystr)) {
        rval = escape_unicode(pystr);
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "first argument must be a string, not %.80s",
                     Py_TYPE(pystr)->tp_name);
        return NULL;
    }
    return rval;
}

static void
scanner_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(self);
    (void)scanner_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static int
scanner_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyScannerObject *self = PyScannerObject_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->object_hook);
    Py_VISIT(self->object_pairs_hook);
    Py_VISIT(self->parse_float);
    Py_VISIT(self->parse_int);
    Py_VISIT(self->parse_constant);
    return 0;
}

static int
scanner_clear(PyObject *op)
{
    PyScannerObject *self = PyScannerObject_CAST(op);
    Py_CLEAR(self->object_hook);
    Py_CLEAR(self->object_pairs_hook);
    Py_CLEAR(self->parse_float);
    Py_CLEAR(self->parse_int);
    Py_CLEAR(self->parse_constant);
    return 0;
}

static PyObject *
_parse_object_unicode(PyScannerObject *s, PyObject *memo, PyObject *pystr, Py_ssize_t idx, Py_ssize_t *next_idx_ptr)
{
    /* Read a JSON object from PyUnicode pystr.
    idx is the index of the first character after the opening curly brace.
    *next_idx_ptr is a return-by-reference index to the first character after
        the closing curly brace.

    Returns a new PyObject (usually a dict, but object_hook can change that)
    */
    const void *str;
    int kind;
    Py_ssize_t end_idx;
    PyObject *val = NULL;
    PyObject *rval = NULL;
    PyObject *key = NULL;
    int has_pairs_hook = (s->object_pairs_hook != Py_None);
    Py_ssize_t next_idx;
    Py_ssize_t comma_idx;

    str = PyUnicode_DATA(pystr);
    kind = PyUnicode_KIND(pystr);
    end_idx = PyUnicode_GET_LENGTH(pystr) - 1;

    if (has_pairs_hook)
        rval = PyList_New(0);
    else
        rval = PyDict_New();
    if (rval == NULL)
        return NULL;

    /* skip whitespace after { */
    while (idx <= end_idx && IS_WHITESPACE(PyUnicode_READ(kind,str, idx))) idx++;

    /* only loop if the object is non-empty */
    if (idx > end_idx || PyUnicode_READ(kind, str, idx) != '}') {
        while (1) {
            PyObject *memokey;

            /* read key */
            if (idx > end_idx || PyUnicode_READ(kind, str, idx) != '"') {
                raise_errmsg("Expecting property name enclosed in double quotes", pystr, idx);
                goto bail;
            }
            key = scanstring_unicode(pystr, idx + 1, s->strict, &next_idx);
            if (key == NULL)
                goto bail;
            if (PyDict_SetDefaultRef(memo, key, key, &memokey) < 0) {
                goto bail;
            }
            Py_SETREF(key, memokey);
            idx = next_idx;

            /* skip whitespace between key and : delimiter, read :, skip whitespace */
            while (idx <= end_idx && IS_WHITESPACE(PyUnicode_READ(kind, str, idx))) idx++;
            if (idx > end_idx || PyUnicode_READ(kind, str, idx) != ':') {
                raise_errmsg("Expecting ':' delimiter", pystr, idx);
                goto bail;
            }
            idx++;
            while (idx <= end_idx && IS_WHITESPACE(PyUnicode_READ(kind, str, idx))) idx++;

            /* read any JSON term */
            val = scan_once_unicode(s, memo, pystr, idx, &next_idx);
            if (val == NULL)
                goto bail;

            if (has_pairs_hook) {
                PyObject *item = PyTuple_Pack(2, key, val);
                if (item == NULL)
                    goto bail;
                Py_CLEAR(key);
                Py_CLEAR(val);
                if (PyList_Append(rval, item) == -1) {
                    Py_DECREF(item);
                    goto bail;
                }
                Py_DECREF(item);
            }
            else {
                if (PyDict_SetItem(rval, key, val) < 0)
                    goto bail;
                Py_CLEAR(key);
                Py_CLEAR(val);
            }
            idx = next_idx;

            /* skip whitespace before } or , */
            while (idx <= end_idx && IS_WHITESPACE(PyUnicode_READ(kind, str, idx))) idx++;

            /* bail if the object is closed or we didn't get the , delimiter */
            if (idx <= end_idx && PyUnicode_READ(kind, str, idx) == '}')
                break;
            if (idx > end_idx || PyUnicode_READ(kind, str, idx) != ',') {
                raise_errmsg("Expecting ',' delimiter", pystr, idx);
                goto bail;
            }
            comma_idx = idx;
            idx++;

            /* skip whitespace after , delimiter */
            while (idx <= end_idx && IS_WHITESPACE(PyUnicode_READ(kind, str, idx))) idx++;

            if (idx <= end_idx && PyUnicode_READ(kind, str, idx) == '}') {
                raise_errmsg("Illegal trailing comma before end of object", pystr, comma_idx);
                goto bail;
            }
        }
    }

    *next_idx_ptr = idx + 1;

    if (has_pairs_hook) {
        val = PyObject_CallOneArg(s->object_pairs_hook, rval);
        Py_DECREF(rval);
        return val;
    }

    /* if object_hook is not None: rval = object_hook(rval) */
    if (s->object_hook != Py_None) {
        val = PyObject_CallOneArg(s->object_hook, rval);
        Py_DECREF(rval);
        return val;
    }
    return rval;
bail:
    Py_XDECREF(key);
    Py_XDECREF(val);
    Py_XDECREF(rval);
    return NULL;
}

static PyObject *
_parse_array_unicode(PyScannerObject *s, PyObject *memo, PyObject *pystr, Py_ssize_t idx, Py_ssize_t *next_idx_ptr) {
    /* Read a JSON array from PyUnicode pystr.
    idx is the index of the first character after the opening brace.
    *next_idx_ptr is a return-by-reference index to the first character after
        the closing brace.

    Returns a new PyList
    */
    const void *str;
    int kind;
    Py_ssize_t end_idx;
    PyObject *val = NULL;
    PyObject *rval;
    Py_ssize_t next_idx;
    Py_ssize_t comma_idx;

    rval = PyList_New(0);
    if (rval == NULL)
        return NULL;

    str = PyUnicode_DATA(pystr);
    kind = PyUnicode_KIND(pystr);
    end_idx = PyUnicode_GET_LENGTH(pystr) - 1;

    /* skip whitespace after [ */
    while (idx <= end_idx && IS_WHITESPACE(PyUnicode_READ(kind, str, idx))) idx++;

    /* only loop if the array is non-empty */
    if (idx > end_idx || PyUnicode_READ(kind, str, idx) != ']') {
        while (1) {

            /* read any JSON term  */
            val = scan_once_unicode(s, memo, pystr, idx, &next_idx);
            if (val == NULL)
                goto bail;

            if (PyList_Append(rval, val) == -1)
                goto bail;

            Py_CLEAR(val);
            idx = next_idx;

            /* skip whitespace between term and , */
            while (idx <= end_idx && IS_WHITESPACE(PyUnicode_READ(kind, str, idx))) idx++;

            /* bail if the array is closed or we didn't get the , delimiter */
            if (idx <= end_idx && PyUnicode_READ(kind, str, idx) == ']')
                break;
            if (idx > end_idx || PyUnicode_READ(kind, str, idx) != ',') {
                raise_errmsg("Expecting ',' delimiter", pystr, idx);
                goto bail;
            }
            comma_idx = idx;
            idx++;

            /* skip whitespace after , */
            while (idx <= end_idx && IS_WHITESPACE(PyUnicode_READ(kind, str, idx))) idx++;

            if (idx <= end_idx && PyUnicode_READ(kind, str, idx) == ']') {
                raise_errmsg("Illegal trailing comma before end of array", pystr, comma_idx);
                goto bail;
            }
        }
    }

    /* verify that idx < end_idx, PyUnicode_READ(kind, str, idx) should be ']' */
    if (idx > end_idx || PyUnicode_READ(kind, str, idx) != ']') {
        raise_errmsg("Expecting value", pystr, end_idx);
        goto bail;
    }
    *next_idx_ptr = idx + 1;
    return rval;
bail:
    Py_XDECREF(val);
    Py_DECREF(rval);
    return NULL;
}

static PyObject *
_parse_constant(PyScannerObject *s, const char *constant, Py_ssize_t idx, Py_ssize_t *next_idx_ptr) {
    /* Read a JSON constant.
    constant is the constant string that was found
        ("NaN", "Infinity", "-Infinity").
    idx is the index of the first character of the constant
    *next_idx_ptr is a return-by-reference index to the first character after
        the constant.

    Returns the result of parse_constant
    */
    PyObject *cstr;
    PyObject *rval;
    /* constant is "NaN", "Infinity", or "-Infinity" */
    cstr = PyUnicode_InternFromString(constant);
    if (cstr == NULL)
        return NULL;

    /* rval = parse_constant(constant) */
    rval = PyObject_CallOneArg(s->parse_constant, cstr);
    idx += PyUnicode_GET_LENGTH(cstr);
    Py_DECREF(cstr);
    *next_idx_ptr = idx;
    return rval;
}

static PyObject *
_match_number_unicode(PyScannerObject *s, PyObject *pystr, Py_ssize_t start, Py_ssize_t *next_idx_ptr) {
    /* Read a JSON number from PyUnicode pystr.
    idx is the index of the first character of the number
    *next_idx_ptr is a return-by-reference index to the first character after
        the number.

    Returns a new PyObject representation of that number:
        PyLong, or PyFloat.
        May return other types if parse_int or parse_float are set
    */
    const void *str;
    int kind;
    Py_ssize_t end_idx;
    Py_ssize_t idx = start;
    int is_float = 0;
    PyObject *rval;
    PyObject *numstr = NULL;
    PyObject *custom_func;

    str = PyUnicode_DATA(pystr);
    kind = PyUnicode_KIND(pystr);
    end_idx = PyUnicode_GET_LENGTH(pystr) - 1;

    /* read a sign if it's there, make sure it's not the end of the string */
    if (PyUnicode_READ(kind, str, idx) == '-') {
        idx++;
        if (idx > end_idx) {
            raise_stop_iteration(start);
            return NULL;
        }
    }

    /* read as many integer digits as we find as long as it doesn't start with 0 */
    if (PyUnicode_READ(kind, str, idx) >= '1' && PyUnicode_READ(kind, str, idx) <= '9') {
        idx++;
        while (idx <= end_idx && PyUnicode_READ(kind, str, idx) >= '0' && PyUnicode_READ(kind, str, idx) <= '9') idx++;
    }
    /* if it starts with 0 we only expect one integer digit */
    else if (PyUnicode_READ(kind, str, idx) == '0') {
        idx++;
    }
    /* no integer digits, error */
    else {
        raise_stop_iteration(start);
        return NULL;
    }

    /* if the next char is '.' followed by a digit then read all float digits */
    if (idx < end_idx && PyUnicode_READ(kind, str, idx) == '.' && PyUnicode_READ(kind, str, idx + 1) >= '0' && PyUnicode_READ(kind, str, idx + 1) <= '9') {
        is_float = 1;
        idx += 2;
        while (idx <= end_idx && PyUnicode_READ(kind, str, idx) >= '0' && PyUnicode_READ(kind, str, idx) <= '9') idx++;
    }

    /* if the next char is 'e' or 'E' then maybe read the exponent (or backtrack) */
    if (idx < end_idx && (PyUnicode_READ(kind, str, idx) == 'e' || PyUnicode_READ(kind, str, idx) == 'E')) {
        Py_ssize_t e_start = idx;
        idx++;

        /* read an exponent sign if present */
        if (idx < end_idx && (PyUnicode_READ(kind, str, idx) == '-' || PyUnicode_READ(kind, str, idx) == '+')) idx++;

        /* read all digits */
        while (idx <= end_idx && PyUnicode_READ(kind, str, idx) >= '0' && PyUnicode_READ(kind, str, idx) <= '9') idx++;

        /* if we got a digit, then parse as float. if not, backtrack */
        if (PyUnicode_READ(kind, str, idx - 1) >= '0' && PyUnicode_READ(kind, str, idx - 1) <= '9') {
            is_float = 1;
        }
        else {
            idx = e_start;
        }
    }

    if (is_float && s->parse_float != (PyObject *)&PyFloat_Type)
        custom_func = s->parse_float;
    else if (!is_float && s->parse_int != (PyObject *) &PyLong_Type)
        custom_func = s->parse_int;
    else
        custom_func = NULL;

    if (custom_func) {
        /* copy the section we determined to be a number */
        numstr = PyUnicode_FromKindAndData(kind,
                                           (char*)str + kind * start,
                                           idx - start);
        if (numstr == NULL)
            return NULL;
        rval = PyObject_CallOneArg(custom_func, numstr);
    }
    else {
        Py_ssize_t i, n;
        char *buf;
        /* Straight conversion to ASCII, to avoid costly conversion of
           decimal unicode digits (which cannot appear here) */
        n = idx - start;
        numstr = PyBytes_FromStringAndSize(NULL, n);
        if (numstr == NULL)
            return NULL;
        buf = PyBytes_AS_STRING(numstr);
        for (i = 0; i < n; i++) {
            buf[i] = (char) PyUnicode_READ(kind, str, i + start);
        }
        if (is_float)
            rval = PyFloat_FromString(numstr);
        else
            rval = PyLong_FromString(buf, NULL, 10);
    }
    Py_DECREF(numstr);
    *next_idx_ptr = idx;
    return rval;
}

static PyObject *
scan_once_unicode(PyScannerObject *s, PyObject *memo, PyObject *pystr, Py_ssize_t idx, Py_ssize_t *next_idx_ptr)
{
    /* Read one JSON term (of any kind) from PyUnicode pystr.
    idx is the index of the first character of the term
    *next_idx_ptr is a return-by-reference index to the first character after
        the number.

    Returns a new PyObject representation of the term.
    */
    PyObject *res;
    const void *str;
    int kind;
    Py_ssize_t length;

    str = PyUnicode_DATA(pystr);
    kind = PyUnicode_KIND(pystr);
    length = PyUnicode_GET_LENGTH(pystr);

    if (idx < 0) {
        PyErr_SetString(PyExc_ValueError, "idx cannot be negative");
        return NULL;
    }
    if (idx >= length) {
        raise_stop_iteration(idx);
        return NULL;
    }

    switch (PyUnicode_READ(kind, str, idx)) {
        case '"':
            /* string */
            return scanstring_unicode(pystr, idx + 1, s->strict, next_idx_ptr);
        case '{':
            /* object */
            if (_Py_EnterRecursiveCall(" while decoding a JSON object "
                                       "from a unicode string"))
                return NULL;
            res = _parse_object_unicode(s, memo, pystr, idx + 1, next_idx_ptr);
            _Py_LeaveRecursiveCall();
            return res;
        case '[':
            /* array */
            if (_Py_EnterRecursiveCall(" while decoding a JSON array "
                                       "from a unicode string"))
                return NULL;
            res = _parse_array_unicode(s, memo, pystr, idx + 1, next_idx_ptr);
            _Py_LeaveRecursiveCall();
            return res;
        case 'n':
            /* null */
            if ((idx + 3 < length) && PyUnicode_READ(kind, str, idx + 1) == 'u' && PyUnicode_READ(kind, str, idx + 2) == 'l' && PyUnicode_READ(kind, str, idx + 3) == 'l') {
                *next_idx_ptr = idx + 4;
                Py_RETURN_NONE;
            }
            break;
        case 't':
            /* true */
            if ((idx + 3 < length) && PyUnicode_READ(kind, str, idx + 1) == 'r' && PyUnicode_READ(kind, str, idx + 2) == 'u' && PyUnicode_READ(kind, str, idx + 3) == 'e') {
                *next_idx_ptr = idx + 4;
                Py_RETURN_TRUE;
            }
            break;
        case 'f':
            /* false */
            if ((idx + 4 < length) && PyUnicode_READ(kind, str, idx + 1) == 'a' &&
                PyUnicode_READ(kind, str, idx + 2) == 'l' &&
                PyUnicode_READ(kind, str, idx + 3) == 's' &&
                PyUnicode_READ(kind, str, idx + 4) == 'e') {
                *next_idx_ptr = idx + 5;
                Py_RETURN_FALSE;
            }
            break;
        case 'N':
            /* NaN */
            if ((idx + 2 < length) && PyUnicode_READ(kind, str, idx + 1) == 'a' &&
                PyUnicode_READ(kind, str, idx + 2) == 'N') {
                return _parse_constant(s, "NaN", idx, next_idx_ptr);
            }
            break;
        case 'I':
            /* Infinity */
            if ((idx + 7 < length) && PyUnicode_READ(kind, str, idx + 1) == 'n' &&
                PyUnicode_READ(kind, str, idx + 2) == 'f' &&
                PyUnicode_READ(kind, str, idx + 3) == 'i' &&
                PyUnicode_READ(kind, str, idx + 4) == 'n' &&
                PyUnicode_READ(kind, str, idx + 5) == 'i' &&
                PyUnicode_READ(kind, str, idx + 6) == 't' &&
                PyUnicode_READ(kind, str, idx + 7) == 'y') {
                return _parse_constant(s, "Infinity", idx, next_idx_ptr);
            }
            break;
        case '-':
            /* -Infinity */
            if ((idx + 8 < length) && PyUnicode_READ(kind, str, idx + 1) == 'I' &&
                PyUnicode_READ(kind, str, idx + 2) == 'n' &&
                PyUnicode_READ(kind, str, idx + 3) == 'f' &&
                PyUnicode_READ(kind, str, idx + 4) == 'i' &&
                PyUnicode_READ(kind, str, idx + 5) == 'n' &&
                PyUnicode_READ(kind, str, idx + 6) == 'i' &&
                PyUnicode_READ(kind, str, idx + 7) == 't' &&
                PyUnicode_READ(kind, str, idx + 8) == 'y') {
                return _parse_constant(s, "-Infinity", idx, next_idx_ptr);
            }
            break;
    }
    /* Didn't find a string, object, array, or named constant. Look for a number. */
    return _match_number_unicode(s, pystr, idx, next_idx_ptr);
}

static PyObject *
scanner_call(PyObject *self, PyObject *args, PyObject *kwds)
{
    /* Python callable interface to scan_once_{str,unicode} */
    PyObject *pystr;
    PyObject *rval;
    Py_ssize_t idx;
    Py_ssize_t next_idx = -1;
    static char *kwlist[] = {"string", "idx", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "On:scan_once", kwlist, &pystr, &idx))
        return NULL;

    if (!PyUnicode_Check(pystr)) {
        PyErr_Format(PyExc_TypeError,
                     "first argument must be a string, not %.80s",
                     Py_TYPE(pystr)->tp_name);
        return NULL;
    }

    PyObject *memo = PyDict_New();
    if (memo == NULL) {
        return NULL;
    }
    rval = scan_once_unicode(PyScannerObject_CAST(self),
                             memo, pystr, idx, &next_idx);
    Py_DECREF(memo);
    if (rval == NULL)
        return NULL;
    return _build_rval_index_tuple(rval, next_idx);
}

static PyObject *
scanner_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyScannerObject *s;
    PyObject *ctx;
    PyObject *strict;
    static char *kwlist[] = {"context", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:make_scanner", kwlist, &ctx))
        return NULL;

    s = (PyScannerObject *)type->tp_alloc(type, 0);
    if (s == NULL) {
        return NULL;
    }

    /* All of these will fail "gracefully" so we don't need to verify them */
    strict = PyObject_GetAttrString(ctx, "strict");
    if (strict == NULL)
        goto bail;
    s->strict = PyObject_IsTrue(strict);
    Py_DECREF(strict);
    if (s->strict < 0)
        goto bail;
    s->object_hook = PyObject_GetAttrString(ctx, "object_hook");
    if (s->object_hook == NULL)
        goto bail;
    s->object_pairs_hook = PyObject_GetAttrString(ctx, "object_pairs_hook");
    if (s->object_pairs_hook == NULL)
        goto bail;
    s->parse_float = PyObject_GetAttrString(ctx, "parse_float");
    if (s->parse_float == NULL)
        goto bail;
    s->parse_int = PyObject_GetAttrString(ctx, "parse_int");
    if (s->parse_int == NULL)
        goto bail;
    s->parse_constant = PyObject_GetAttrString(ctx, "parse_constant");
    if (s->parse_constant == NULL)
        goto bail;

    return (PyObject *)s;

bail:
    Py_DECREF(s);
    return NULL;
}

PyDoc_STRVAR(scanner_doc, "JSON scanner object");

static PyType_Slot PyScannerType_slots[] = {
    {Py_tp_doc, (void *)scanner_doc},
    {Py_tp_dealloc, scanner_dealloc},
    {Py_tp_call, scanner_call},
    {Py_tp_traverse, scanner_traverse},
    {Py_tp_clear, scanner_clear},
    {Py_tp_members, scanner_members},
    {Py_tp_new, scanner_new},
    {0, 0}
};

static PyType_Spec PyScannerType_spec = {
    .name = "_json.Scanner",
    .basicsize = sizeof(PyScannerObject),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .slots = PyScannerType_slots,
};

static PyObject *
encoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"markers", "default", "encoder", "indent", "key_separator", "item_separator", "sort_keys", "skipkeys", "allow_nan", NULL};

    PyEncoderObject *s;
    PyObject *markers = Py_None, *defaultfn, *encoder, *indent, *key_separator;
    PyObject *item_separator;
    int sort_keys, skipkeys, allow_nan;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!?OOOUUppp:make_encoder", kwlist,
        &PyDict_Type, &markers, &defaultfn, &encoder, &indent,
        &key_separator, &item_separator,
        &sort_keys, &skipkeys, &allow_nan))
        return NULL;

    s = (PyEncoderObject *)type->tp_alloc(type, 0);
    if (s == NULL)
        return NULL;

    s->defaultfn = Py_NewRef(defaultfn);
    s->encoder = Py_NewRef(encoder);
    s->indent = Py_NewRef(indent);
    s->key_separator = Py_NewRef(key_separator);
    s->item_separator = Py_NewRef(item_separator);
    s->sort_keys = sort_keys;
    s->skipkeys = skipkeys;
    s->allow_nan = allow_nan;
    s->fast_encode = NULL;

    if (PyCFunction_Check(s->encoder)) {
        PyCFunction f = PyCFunction_GetFunction(s->encoder);
        if (f == py_encode_basestring_ascii || f == py_encode_basestring) {
            s->fast_encode = f;
        }
    }

    return (PyObject *)s;
}


/* indent_cache is a list that contains intermixed values at even and odd
 * positions:
 *
 * 2*k   : '\n' + indent * (k + initial_indent_level)
 *         strings written after opening and before closing brackets
 * 2*k-1 : item_separator + '\n' + indent * (k + initial_indent_level)
 *         strings written between items
 *
 * Its size is always an odd number.
 */
static PyObject *
create_indent_cache(PyEncoderObject *s, Py_ssize_t indent_level)
{
    PyObject *newline_indent = PyUnicode_FromOrdinal('\n');
    if (newline_indent != NULL && indent_level) {
        PyUnicode_AppendAndDel(&newline_indent,
                               PySequence_Repeat(s->indent, indent_level));
    }
    if (newline_indent == NULL) {
        return NULL;
    }
    PyObject *indent_cache = PyList_New(1);
    if (indent_cache == NULL) {
        Py_DECREF(newline_indent);
        return NULL;
    }
    PyList_SET_ITEM(indent_cache, 0, newline_indent);
    return indent_cache;
}

/* Extend indent_cache by adding values for the next level.
 * It should have values for the indent_level-1 level before the call.
 */
static int
update_indent_cache(PyEncoderObject *s,
                    Py_ssize_t indent_level, PyObject *indent_cache)
{
    assert(indent_level * 2 == PyList_GET_SIZE(indent_cache) + 1);
    assert(indent_level > 0);
    PyObject *newline_indent = PyList_GET_ITEM(indent_cache, (indent_level - 1)*2);
    newline_indent = PyUnicode_Concat(newline_indent, s->indent);
    if (newline_indent == NULL) {
        return -1;
    }
    PyObject *separator_indent = PyUnicode_Concat(s->item_separator, newline_indent);
    if (separator_indent == NULL) {
        Py_DECREF(newline_indent);
        return -1;
    }

    if (PyList_Append(indent_cache, separator_indent) < 0 ||
        PyList_Append(indent_cache, newline_indent) < 0)
    {
        Py_DECREF(separator_indent);
        Py_DECREF(newline_indent);
        return -1;
    }
    Py_DECREF(separator_indent);
    Py_DECREF(newline_indent);
    return 0;
}

static PyObject *
get_item_separator(PyEncoderObject *s,
                   Py_ssize_t indent_level, PyObject *indent_cache)
{
    assert(indent_level > 0);
    if (indent_level * 2 > PyList_GET_SIZE(indent_cache)) {
        if (update_indent_cache(s, indent_level, indent_cache) < 0) {
            return NULL;
        }
    }
    assert(indent_level * 2 < PyList_GET_SIZE(indent_cache));
    return PyList_GET_ITEM(indent_cache, indent_level * 2 - 1);
}

static int
write_newline_indent(PyUnicodeWriter *writer,
                     Py_ssize_t indent_level, PyObject *indent_cache)
{
    PyObject *newline_indent = PyList_GET_ITEM(indent_cache, indent_level * 2);
    return PyUnicodeWriter_WriteStr(writer, newline_indent);
}


static PyObject *
encoder_call(PyObject *op, PyObject *args, PyObject *kwds)
{
    /* Python callable interface to encode_listencode_obj */
    static char *kwlist[] = {"obj", "_current_indent_level", NULL};
    PyObject *obj;
    Py_ssize_t indent_level;
    PyEncoderObject *self = PyEncoderObject_CAST(op);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "On:_iterencode", kwlist,
                                     &obj, &indent_level))
        return NULL;

    PyUnicodeWriter *writer = PyUnicodeWriter_Create(0);
    if (writer == NULL) {
        return NULL;
    }

    PyObject *indent_cache = NULL;
    if (self->indent != Py_None) {
        indent_cache = create_indent_cache(self, indent_level);
        if (indent_cache == NULL) {
            PyUnicodeWriter_Discard(writer);
            return NULL;
        }
    }
    if (encoder_listencode_obj(self, writer, obj, indent_level, indent_cache)) {
        PyUnicodeWriter_Discard(writer);
        Py_XDECREF(indent_cache);
        return NULL;
    }
    Py_XDECREF(indent_cache);

    PyObject *str = PyUnicodeWriter_Finish(writer);
    if (str == NULL) {
        return NULL;
    }
    PyObject *result = PyTuple_Pack(1, str);
    Py_DECREF(str);
    return result;
}

static PyObject *
_encoded_const(PyObject *obj)
{
    /* Return the JSON string representation of None, True, False */
    if (obj == Py_None) {
        return &_Py_ID(null);
    }
    else if (obj == Py_True) {
        return &_Py_ID(true);
    }
    else if (obj == Py_False) {
        return &_Py_ID(false);
    }
    else {
        PyErr_SetString(PyExc_ValueError, "not a const");
        return NULL;
    }
}

static PyObject *
encoder_encode_float(PyEncoderObject *s, PyObject *obj)
{
    /* Return the JSON representation of a PyFloat. */
    double i = PyFloat_AS_DOUBLE(obj);
    if (!isfinite(i)) {
        if (!s->allow_nan) {
            PyErr_Format(
                    PyExc_ValueError,
                    "Out of range float values are not JSON compliant: %R",
                    obj
                    );
            return NULL;
        }
        if (i > 0) {
            return PyUnicode_FromString("Infinity");
        }
        else if (i < 0) {
            return PyUnicode_FromString("-Infinity");
        }
        else {
            return PyUnicode_FromString("NaN");
        }
    }
    return PyFloat_Type.tp_repr(obj);
}

static PyObject *
encoder_encode_string(PyEncoderObject *s, PyObject *obj)
{
    /* Return the JSON representation of a string */
    PyObject *encoded;

    if (s->fast_encode) {
        return s->fast_encode(NULL, obj);
    }
    encoded = PyObject_CallOneArg(s->encoder, obj);
    if (encoded != NULL && !PyUnicode_Check(encoded)) {
        PyErr_Format(PyExc_TypeError,
                     "encoder() must return a string, not %.80s",
                     Py_TYPE(encoded)->tp_name);
        Py_DECREF(encoded);
        return NULL;
    }
    return encoded;
}

static int
_steal_accumulate(PyUnicodeWriter *writer, PyObject *stolen)
{
    /* Append stolen and then decrement its reference count */
    int rval = PyUnicodeWriter_WriteStr(writer, stolen);
    Py_DECREF(stolen);
    return rval;
}

static int
encoder_listencode_obj(PyEncoderObject *s, PyUnicodeWriter *writer,
                       PyObject *obj,
                       Py_ssize_t indent_level, PyObject *indent_cache)
{
    /* Encode Python object obj to a JSON term */
    PyObject *newobj;
    int rv;

    if (obj == Py_None) {
      return PyUnicodeWriter_WriteASCII(writer, "null", 4);
    }
    else if (obj == Py_True) {
      return PyUnicodeWriter_WriteASCII(writer, "true", 4);
    }
    else if (obj == Py_False) {
      return PyUnicodeWriter_WriteASCII(writer, "false", 5);
    }
    else if (PyUnicode_Check(obj)) {
        PyObject *encoded = encoder_encode_string(s, obj);
        if (encoded == NULL)
            return -1;
        return _steal_accumulate(writer, encoded);
    }
    else if (PyLong_Check(obj)) {
        if (PyLong_CheckExact(obj)) {
            // Fast-path for exact integers
            return PyUnicodeWriter_WriteRepr(writer, obj);
        }
        PyObject *encoded = PyLong_Type.tp_repr(obj);
        if (encoded == NULL)
            return -1;
        return _steal_accumulate(writer, encoded);
    }
    else if (PyFloat_Check(obj)) {
        PyObject *encoded = encoder_encode_float(s, obj);
        if (encoded == NULL)
            return -1;
        return _steal_accumulate(writer, encoded);
    }
    else if (PyList_Check(obj) || PyTuple_Check(obj)) {
        if (_Py_EnterRecursiveCall(" while encoding a JSON object"))
            return -1;
        rv = encoder_listencode_list(s, writer, obj, indent_level, indent_cache);
        _Py_LeaveRecursiveCall();
        return rv;
    }
    else if (PyDict_Check(obj)) {
        if (_Py_EnterRecursiveCall(" while encoding a JSON object"))
            return -1;
        rv = encoder_listencode_dict(s, writer, obj, indent_level, indent_cache);
        _Py_LeaveRecursiveCall();
        return rv;
    }
    else {
        newobj = PyObject_CallOneArg(s->defaultfn, obj);
        if (newobj == NULL) {
            return -1;
        }

        if (_Py_EnterRecursiveCall(" while encoding a JSON object")) {
            Py_DECREF(newobj);
            return -1;
        }
        rv = encoder_listencode_obj(s, writer, newobj, indent_level, indent_cache);
        _Py_LeaveRecursiveCall();

        Py_DECREF(newobj);
        if (rv) {
            _PyErr_FormatNote("when serializing %T object", obj);
            return -1;
        }
        return rv;
    }
}

static int
encoder_encode_key_value(PyEncoderObject *s, PyUnicodeWriter *writer, bool *first,
                         PyObject *dct, PyObject *key, PyObject *value,
                         Py_ssize_t indent_level, PyObject *indent_cache,
                         PyObject *item_separator)
{
    PyObject *keystr = NULL;
    PyObject *encoded;

    if (PyUnicode_Check(key)) {
        keystr = Py_NewRef(key);
    }
    else if (PyFloat_Check(key)) {
        keystr = encoder_encode_float(s, key);
    }
    else if (key == Py_True || key == Py_False || key == Py_None) {
                    /* This must come before the PyLong_Check because
                       True and False are also 1 and 0.*/
        keystr = _encoded_const(key);
    }
    else if (PyLong_Check(key)) {
        keystr = PyLong_Type.tp_repr(key);
    }
    else if (s->skipkeys) {
        return 0;
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "keys must be str, int, float, bool or None, "
                     "not %.100s", Py_TYPE(key)->tp_name);
        return -1;
    }

    if (keystr == NULL) {
        return -1;
    }

    if (*first) {
        *first = false;
    }
    else {
        if (PyUnicodeWriter_WriteStr(writer, item_separator) < 0) {
            Py_DECREF(keystr);
            return -1;
        }
    }

    encoded = encoder_encode_string(s, keystr);
    Py_DECREF(keystr);
    if (encoded == NULL) {
        return -1;
    }

    if (_steal_accumulate(writer, encoded) < 0) {
        return -1;
    }
    if (PyUnicodeWriter_WriteStr(writer, s->key_separator) < 0) {
        return -1;
    }
    if (encoder_listencode_obj(s, writer, value, indent_level, indent_cache) < 0) {
        _PyErr_FormatNote("when serializing %T item %R", dct, key);
        return -1;
    }
    return 0;
}

static int
encoder_listencode_dict(PyEncoderObject *s, PyUnicodeWriter *writer,
                        PyObject *dct,
                       Py_ssize_t indent_level, PyObject *indent_cache)
{
    /* Encode Python dict dct a JSON term */
    PyObject *items = NULL;
    PyObject *key, *value;
    bool first = true;

    if (PyDict_GET_SIZE(dct) == 0) {
        /* Fast path */
        return PyUnicodeWriter_WriteASCII(writer, "{}", 2);
    }

    if (PyUnicodeWriter_WriteChar(writer, '{')) {
        goto bail;
    }

    PyObject *separator = s->item_separator; // borrowed reference
    if (s->indent != Py_None) {
        indent_level++;
        separator = get_item_separator(s, indent_level, indent_cache);
        if (separator == NULL ||
            write_newline_indent(writer, indent_level, indent_cache) < 0)
        {
            goto bail;
        }
    }

    if (s->sort_keys || !PyDict_CheckExact(dct)) {
        items = PyMapping_Items(dct);
        if (items == NULL || (s->sort_keys && PyList_Sort(items) < 0))
            goto bail;

        for (Py_ssize_t  i = 0; i < PyList_GET_SIZE(items); i++) {
            PyObject *item = PyList_GET_ITEM(items, i);

            if (!PyTuple_Check(item) || PyTuple_GET_SIZE(item) != 2) {
                PyErr_SetString(PyExc_ValueError, "items must return 2-tuples");
                goto bail;
            }

            key = PyTuple_GET_ITEM(item, 0);
            value = PyTuple_GET_ITEM(item, 1);
            if (encoder_encode_key_value(s, writer, &first, dct, key, value,
                                         indent_level, indent_cache,
                                         separator) < 0)
                goto bail;
        }
        Py_CLEAR(items);

    } else {
        Py_ssize_t pos = 0;
        while (PyDict_Next(dct, &pos, &key, &value)) {
            if (encoder_encode_key_value(s, writer, &first, dct, key, value,
                                         indent_level, indent_cache,
                                         separator) < 0)
                goto bail;
        }
    }

    if (s->indent != Py_None) {
        indent_level--;
        if (write_newline_indent(writer, indent_level, indent_cache) < 0) {
            goto bail;
        }
    }

    if (PyUnicodeWriter_WriteChar(writer, '}')) {
        goto bail;
    }
    return 0;

bail:
    Py_XDECREF(items);
    return -1;
}

static int
encoder_listencode_list(PyEncoderObject *s, PyUnicodeWriter *writer,
                        PyObject *seq,
                        Py_ssize_t indent_level, PyObject *indent_cache)
{
    PyObject *s_fast = NULL;
    Py_ssize_t i;

    s_fast = PySequence_Fast(seq, "_iterencode_list needs a sequence");
    if (s_fast == NULL)
        return -1;
    if (PySequence_Fast_GET_SIZE(s_fast) == 0) {
        Py_DECREF(s_fast);
        return PyUnicodeWriter_WriteASCII(writer, "[]", 2);
    }

    if (PyUnicodeWriter_WriteChar(writer, '[')) {
        goto bail;
    }

    PyObject *separator = s->item_separator; // borrowed reference
    if (s->indent != Py_None) {
        indent_level++;
        separator = get_item_separator(s, indent_level, indent_cache);
        if (separator == NULL ||
            write_newline_indent(writer, indent_level, indent_cache) < 0)
        {
            goto bail;
        }
    }
    for (i = 0; i < PySequence_Fast_GET_SIZE(s_fast); i++) {
        PyObject *obj = PySequence_Fast_GET_ITEM(s_fast, i);
        if (i) {
            if (PyUnicodeWriter_WriteStr(writer, separator) < 0)
                goto bail;
        }
        if (encoder_listencode_obj(s, writer, obj, indent_level, indent_cache)) {
            _PyErr_FormatNote("when serializing %T item %zd", seq, i);
            goto bail;
        }
    }

    if (s->indent != Py_None) {
        indent_level--;
        if (write_newline_indent(writer, indent_level, indent_cache) < 0) {
            goto bail;
        }
    }

    if (PyUnicodeWriter_WriteChar(writer, ']')) {
        goto bail;
    }
    Py_DECREF(s_fast);
    return 0;

bail:
    Py_DECREF(s_fast);
    return -1;
}

static void
encoder_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(self);
    (void)encoder_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static int
encoder_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyEncoderObject *self = PyEncoderObject_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->defaultfn);
    Py_VISIT(self->encoder);
    Py_VISIT(self->indent);
    Py_VISIT(self->key_separator);
    Py_VISIT(self->item_separator);
    return 0;
}

static int
encoder_clear(PyObject *op)
{
    PyEncoderObject *self = PyEncoderObject_CAST(op);
    /* Deallocate Encoder */
    Py_CLEAR(self->defaultfn);
    Py_CLEAR(self->encoder);
    Py_CLEAR(self->indent);
    Py_CLEAR(self->key_separator);
    Py_CLEAR(self->item_separator);
    return 0;
}

PyDoc_STRVAR(encoder_doc, "Encoder(markers, default, encoder, indent, key_separator, item_separator, sort_keys, skipkeys, allow_nan)");

static PyType_Slot PyEncoderType_slots[] = {
    {Py_tp_doc, (void *)encoder_doc},
    {Py_tp_dealloc, encoder_dealloc},
    {Py_tp_call, encoder_call},
    {Py_tp_traverse, encoder_traverse},
    {Py_tp_clear, encoder_clear},
    {Py_tp_members, encoder_members},
    {Py_tp_new, encoder_new},
    {0, 0}
};

static PyType_Spec PyEncoderType_spec = {
    .name = "_json.Encoder",
    .basicsize = sizeof(PyEncoderObject),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .slots = PyEncoderType_slots
};

static PyMethodDef speedups_methods[] = {
    {"encode_basestring_ascii",
        py_encode_basestring_ascii,
        METH_O,
        pydoc_encode_basestring_ascii},
    {"encode_basestring",
        py_encode_basestring,
        METH_O,
        pydoc_encode_basestring},
    {"scanstring",
        py_scanstring,
        METH_VARARGS,
        pydoc_scanstring},
    {NULL, NULL, 0, NULL}
};

PyDoc_STRVAR(module_doc,
"json speedups\n");

static int
_json_exec(PyObject *module)
{
    PyObject *PyScannerType = PyType_FromSpec(&PyScannerType_spec);
    if (PyModule_Add(module, "make_scanner", PyScannerType) < 0) {
        return -1;
    }

    PyObject *PyEncoderType = PyType_FromSpec(&PyEncoderType_spec);
    if (PyModule_Add(module, "make_encoder", PyEncoderType) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot _json_slots[] = {
    {Py_mod_exec, _json_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef jsonmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_json",
    .m_doc = module_doc,
    .m_methods = speedups_methods,
    .m_slots = _json_slots,
};

PyMODINIT_FUNC
PyInit__json(void)
{
    return PyModuleDef_Init(&jsonmodule);
}
