#include "Python.h"

#define DEFAULT_ENCODING "utf-8"
#define S_CHAR(c) (c >= ' ' && c <= '~' && c != '\\' && c != '"')
#define MIN_EXPANSION 6

#ifdef Py_UNICODE_WIDE
#define MAX_EXPANSION (2 * MIN_EXPANSION)
#else
#define MAX_EXPANSION MIN_EXPANSION
#endif

static Py_ssize_t
ascii_escape_char(Py_UNICODE c, char *output, Py_ssize_t chars)
{
    Py_UNICODE x;
    output[chars++] = '\\';
    switch (c) {
        case '\\': output[chars++] = (char)c; break;
        case '"': output[chars++] = (char)c; break;
        case '\b': output[chars++] = 'b'; break;
        case '\f': output[chars++] = 'f'; break;
        case '\n': output[chars++] = 'n'; break;
        case '\r': output[chars++] = 'r'; break;
        case '\t': output[chars++] = 't'; break;
        default:
#ifdef Py_UNICODE_WIDE
            if (c >= 0x10000) {
                /* UTF-16 surrogate pair */
                Py_UNICODE v = c - 0x10000;
                c = 0xd800 | ((v >> 10) & 0x3ff);
                output[chars++] = 'u';
                x = (c & 0xf000) >> 12;
                output[chars++] = (x < 10) ? '0' + x : 'a' + (x - 10);
                x = (c & 0x0f00) >> 8;
                output[chars++] = (x < 10) ? '0' + x : 'a' + (x - 10);
                x = (c & 0x00f0) >> 4;
                output[chars++] = (x < 10) ? '0' + x : 'a' + (x - 10);
                x = (c & 0x000f);
                output[chars++] = (x < 10) ? '0' + x : 'a' + (x - 10);
                c = 0xdc00 | (v & 0x3ff);
                output[chars++] = '\\';
            }
#endif
            output[chars++] = 'u';
            x = (c & 0xf000) >> 12;
            output[chars++] = (x < 10) ? '0' + x : 'a' + (x - 10);
            x = (c & 0x0f00) >> 8;
            output[chars++] = (x < 10) ? '0' + x : 'a' + (x - 10);
            x = (c & 0x00f0) >> 4;
            output[chars++] = (x < 10) ? '0' + x : 'a' + (x - 10);
            x = (c & 0x000f);
            output[chars++] = (x < 10) ? '0' + x : 'a' + (x - 10);
    }
    return chars;
}

static PyObject *
ascii_escape_unicode(PyObject *pystr)
{
    Py_ssize_t i;
    Py_ssize_t input_chars;
    Py_ssize_t output_size;
    Py_ssize_t chars;
    PyObject *rval;
    char *output;
    Py_UNICODE *input_unicode;

    input_chars = PyUnicode_GET_SIZE(pystr);
    input_unicode = PyUnicode_AS_UNICODE(pystr);
    /* One char input can be up to 6 chars output, estimate 4 of these */
    output_size = 2 + (MIN_EXPANSION * 4) + input_chars;
    rval = PyString_FromStringAndSize(NULL, output_size);
    if (rval == NULL) {
        return NULL;
    }
    output = PyString_AS_STRING(rval);
    chars = 0;
    output[chars++] = '"';
    for (i = 0; i < input_chars; i++) {
        Py_UNICODE c = input_unicode[i];
        if (S_CHAR(c)) {
            output[chars++] = (char)c;
        }
	else {
            chars = ascii_escape_char(c, output, chars);
        }
        if (output_size - chars < (1 + MAX_EXPANSION)) {
            /* There's more than four, so let's resize by a lot */
            output_size *= 2;
            /* This is an upper bound */
            if (output_size > 2 + (input_chars * MAX_EXPANSION)) {
                output_size = 2 + (input_chars * MAX_EXPANSION);
            }
            if (_PyString_Resize(&rval, output_size) == -1) {
                return NULL;
            }
            output = PyString_AS_STRING(rval);
        }
    }
    output[chars++] = '"';
    if (_PyString_Resize(&rval, chars) == -1) {
        return NULL;
    }
    return rval;
}

static PyObject *
ascii_escape_str(PyObject *pystr)
{
    Py_ssize_t i;
    Py_ssize_t input_chars;
    Py_ssize_t output_size;
    Py_ssize_t chars;
    PyObject *rval;
    char *output;
    char *input_str;

    input_chars = PyString_GET_SIZE(pystr);
    input_str = PyString_AS_STRING(pystr);
    /* One char input can be up to 6 chars output, estimate 4 of these */
    output_size = 2 + (MIN_EXPANSION * 4) + input_chars;
    rval = PyString_FromStringAndSize(NULL, output_size);
    if (rval == NULL) {
        return NULL;
    }
    output = PyString_AS_STRING(rval);
    chars = 0;
    output[chars++] = '"';
    for (i = 0; i < input_chars; i++) {
        Py_UNICODE c = (Py_UNICODE)input_str[i];
        if (S_CHAR(c)) {
            output[chars++] = (char)c;
        }
	else if (c > 0x7F) {
            /* We hit a non-ASCII character, bail to unicode mode */
            PyObject *uni;
            Py_DECREF(rval);
            uni = PyUnicode_DecodeUTF8(input_str, input_chars, "strict");
            if (uni == NULL) {
                return NULL;
            }
            rval = ascii_escape_unicode(uni);
            Py_DECREF(uni);
            return rval;
        }
	else {
            chars = ascii_escape_char(c, output, chars);
        }
        /* An ASCII char can't possibly expand to a surrogate! */
        if (output_size - chars < (1 + MIN_EXPANSION)) {
            /* There's more than four, so let's resize by a lot */
            output_size *= 2;
            if (output_size > 2 + (input_chars * MIN_EXPANSION)) {
                output_size = 2 + (input_chars * MIN_EXPANSION);
            }
            if (_PyString_Resize(&rval, output_size) == -1) {
                return NULL;
            }
            output = PyString_AS_STRING(rval);
        }
    }
    output[chars++] = '"';
    if (_PyString_Resize(&rval, chars) == -1) {
        return NULL;
    }
    return rval;
}

void
raise_errmsg(char *msg, PyObject *s, Py_ssize_t end)
{
    static PyObject *errmsg_fn = NULL;
    PyObject *pymsg;
    if (errmsg_fn == NULL) {
        PyObject *decoder = PyImport_ImportModule("json.decoder");
        if (decoder == NULL)
            return;
        errmsg_fn = PyObject_GetAttrString(decoder, "errmsg");
        if (errmsg_fn == NULL)
            return;
        Py_XDECREF(decoder);
    }
    pymsg = PyObject_CallFunction(errmsg_fn, "(zOn)", msg, s, end);
    PyErr_SetObject(PyExc_ValueError, pymsg);
    Py_DECREF(pymsg);
/*

def linecol(doc, pos):
    lineno = doc.count('\n', 0, pos) + 1
    if lineno == 1:
        colno = pos
    else:
        colno = pos - doc.rindex('\n', 0, pos)
    return lineno, colno

def errmsg(msg, doc, pos, end=None):
    lineno, colno = linecol(doc, pos)
    if end is None:
        return '%s: line %d column %d (char %d)' % (msg, lineno, colno, pos)
    endlineno, endcolno = linecol(doc, end)
    return '%s: line %d column %d - line %d column %d (char %d - %d)' % (
        msg, lineno, colno, endlineno, endcolno, pos, end)

*/
}

static PyObject *
join_list_unicode(PyObject *lst)
{
    static PyObject *ustr = NULL;
    static PyObject *joinstr = NULL;
    if (ustr == NULL) {
        Py_UNICODE c = 0;
        ustr = PyUnicode_FromUnicode(&c, 0);
    }
    if (joinstr == NULL) {
        joinstr = PyString_InternFromString("join");
    }
    if (joinstr == NULL || ustr == NULL) {
        return NULL;
    }
    return PyObject_CallMethodObjArgs(ustr, joinstr, lst, NULL);
}

static PyObject *
scanstring_str(PyObject *pystr, Py_ssize_t end, char *encoding, int strict)
{
    PyObject *rval;
    Py_ssize_t len = PyString_GET_SIZE(pystr);
    Py_ssize_t begin = end - 1;
    Py_ssize_t next = begin;
    char *buf = PyString_AS_STRING(pystr);
    PyObject *chunks = PyList_New(0);
    if (chunks == NULL) {
        goto bail;
    }
    while (1) {
        /* Find the end of the string or the next escape */
        Py_UNICODE c = 0;
        PyObject *chunk = NULL;
        for (next = end; next < len; next++) {
            c = buf[next];
            if (c == '"' || c == '\\') {
                break;
            }
            else if (strict && c <= 0x1f) {
                raise_errmsg("Invalid control character at", pystr, begin);
                goto bail;
            }
        }
        if (!(c == '"' || c == '\\')) {
            raise_errmsg("Unterminated string starting at", pystr, begin);
            goto bail;
        }
        /* Pick up this chunk if it's not zero length */
        if (next != end) {
            PyObject *strchunk = PyBuffer_FromMemory(&buf[end], next - end);
            if (strchunk == NULL) {
                goto bail;
            }
            chunk = PyUnicode_FromEncodedObject(strchunk, encoding, NULL);
            Py_DECREF(strchunk);
            if (chunk == NULL) {
                goto bail;
            }
            if (PyList_Append(chunks, chunk)) {
                goto bail;
            }
            Py_DECREF(chunk);
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
        c = buf[next];
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
                Py_ssize_t shl = (end - next - 1) << 2;
                Py_UNICODE digit = buf[next];
                switch (digit) {
                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                        c |= (digit - '0') << shl; break;
                    case 'a': case 'b': case 'c': case 'd': case 'e':
                    case 'f':
                        c |= (digit - 'a' + 10) << shl; break;
                    case 'A': case 'B': case 'C': case 'D': case 'E':
                    case 'F':
                        c |= (digit - 'A' + 10) << shl; break;
                    default:
                        raise_errmsg("Invalid \\uXXXX escape", pystr, end - 5);
                        goto bail;
                }
            }
#ifdef Py_UNICODE_WIDE
            /* Surrogate pair */
            if (c >= 0xd800 && c <= 0xdbff) {
                Py_UNICODE c2 = 0;
                if (end + 6 >= len) {
                    raise_errmsg("Invalid \\uXXXX\\uXXXX surrogate pair", pystr,
                        end - 5);
                }
                if (buf[next++] != '\\' || buf[next++] != 'u') {
                    raise_errmsg("Invalid \\uXXXX\\uXXXX surrogate pair", pystr,
                        end - 5);
                }
                end += 6;
                /* Decode 4 hex digits */
                for (; next < end; next++) {
                    Py_ssize_t shl = (end - next - 1) << 2;
                    Py_UNICODE digit = buf[next];
                    switch (digit) {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                            c2 |= (digit - '0') << shl; break;
                        case 'a': case 'b': case 'c': case 'd': case 'e':
                        case 'f':
                            c2 |= (digit - 'a' + 10) << shl; break;
                        case 'A': case 'B': case 'C': case 'D': case 'E':
                        case 'F':
                            c2 |= (digit - 'A' + 10) << shl; break;
                        default:
                            raise_errmsg("Invalid \\uXXXX escape", pystr, end - 5);
                            goto bail;
                    }
                }
                c = 0x10000 + (((c - 0xd800) << 10) | (c2 - 0xdc00));
            }
#endif
        }
        chunk = PyUnicode_FromUnicode(&c, 1);
        if (chunk == NULL) {
            goto bail;
        }
        if (PyList_Append(chunks, chunk)) {
            goto bail;
        }
        Py_DECREF(chunk);
    }

    rval = join_list_unicode(chunks);
    if (rval == NULL) {
        goto bail;
    }
    Py_DECREF(chunks);
    chunks = NULL;
    return Py_BuildValue("(Nn)", rval, end);
bail:
    Py_XDECREF(chunks);
    return NULL;
}


static PyObject *
scanstring_unicode(PyObject *pystr, Py_ssize_t end, int strict)
{
    PyObject *rval;
    Py_ssize_t len = PyUnicode_GET_SIZE(pystr);
    Py_ssize_t begin = end - 1;
    Py_ssize_t next = begin;
    const Py_UNICODE *buf = PyUnicode_AS_UNICODE(pystr);
    PyObject *chunks = PyList_New(0);
    if (chunks == NULL) {
        goto bail;
    }
    while (1) {
        /* Find the end of the string or the next escape */
        Py_UNICODE c = 0;
        PyObject *chunk = NULL;
        for (next = end; next < len; next++) {
            c = buf[next];
            if (c == '"' || c == '\\') {
                break;
            }
            else if (strict && c <= 0x1f) {
                raise_errmsg("Invalid control character at", pystr, begin);
                goto bail;
            }
        }
        if (!(c == '"' || c == '\\')) {
            raise_errmsg("Unterminated string starting at", pystr, begin);
            goto bail;
        }
        /* Pick up this chunk if it's not zero length */
        if (next != end) {
            chunk = PyUnicode_FromUnicode(&buf[end], next - end);
            if (chunk == NULL) {
                goto bail;
            }
            if (PyList_Append(chunks, chunk)) {
                goto bail;
            }
            Py_DECREF(chunk);
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
        c = buf[next];
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
                Py_ssize_t shl = (end - next - 1) << 2;
                Py_UNICODE digit = buf[next];
                switch (digit) {
                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                        c |= (digit - '0') << shl; break;
                    case 'a': case 'b': case 'c': case 'd': case 'e':
                    case 'f':
                        c |= (digit - 'a' + 10) << shl; break;
                    case 'A': case 'B': case 'C': case 'D': case 'E':
                    case 'F':
                        c |= (digit - 'A' + 10) << shl; break;
                    default:
                        raise_errmsg("Invalid \\uXXXX escape", pystr, end - 5);
                        goto bail;
                }
            }
#ifdef Py_UNICODE_WIDE
            /* Surrogate pair */
            if (c >= 0xd800 && c <= 0xdbff) {
                Py_UNICODE c2 = 0;
                if (end + 6 >= len) {
                    raise_errmsg("Invalid \\uXXXX\\uXXXX surrogate pair", pystr,
                        end - 5);
                }
                if (buf[next++] != '\\' || buf[next++] != 'u') {
                    raise_errmsg("Invalid \\uXXXX\\uXXXX surrogate pair", pystr,
                        end - 5);
                }
                end += 6;
                /* Decode 4 hex digits */
                for (; next < end; next++) {
                    Py_ssize_t shl = (end - next - 1) << 2;
                    Py_UNICODE digit = buf[next];
                    switch (digit) {
                        case '0': case '1': case '2': case '3': case '4':
                        case '5': case '6': case '7': case '8': case '9':
                            c2 |= (digit - '0') << shl; break;
                        case 'a': case 'b': case 'c': case 'd': case 'e':
                        case 'f':
                            c2 |= (digit - 'a' + 10) << shl; break;
                        case 'A': case 'B': case 'C': case 'D': case 'E':
                        case 'F':
                            c2 |= (digit - 'A' + 10) << shl; break;
                        default:
                            raise_errmsg("Invalid \\uXXXX escape", pystr, end - 5);
                            goto bail;
                    }
                }
                c = 0x10000 + (((c - 0xd800) << 10) | (c2 - 0xdc00));
            }
#endif
        }
        chunk = PyUnicode_FromUnicode(&c, 1);
        if (chunk == NULL) {
            goto bail;
        }
        if (PyList_Append(chunks, chunk)) {
            goto bail;
        }
        Py_DECREF(chunk);
    }

    rval = join_list_unicode(chunks);
    if (rval == NULL) {
        goto bail;
    }
    Py_DECREF(chunks);
    chunks = NULL;
    return Py_BuildValue("(Nn)", rval, end);
bail:
    Py_XDECREF(chunks);
    return NULL;
}

PyDoc_STRVAR(pydoc_scanstring,
"scanstring(basestring, end, encoding) -> (str, end)\n");

static PyObject *
py_scanstring(PyObject* self, PyObject *args)
{
    PyObject *pystr;
    Py_ssize_t end;
    char *encoding = NULL;
    int strict = 0;
    if (!PyArg_ParseTuple(args, "On|zi:scanstring", &pystr, &end, &encoding, &strict)) {
        return NULL;
    }
    if (encoding == NULL) {
        encoding = DEFAULT_ENCODING;
    }
    if (PyString_Check(pystr)) {
        return scanstring_str(pystr, end, encoding, strict);
    }
    else if (PyUnicode_Check(pystr)) {
        return scanstring_unicode(pystr, end, strict);
    }
    else {
        PyErr_Format(PyExc_TypeError, 
                     "first argument must be a string or unicode, not %.80s",
                     Py_TYPE(pystr)->tp_name);
        return NULL;
    }
}

PyDoc_STRVAR(pydoc_encode_basestring_ascii,
"encode_basestring_ascii(basestring) -> str\n");

static PyObject *
py_encode_basestring_ascii(PyObject* self, PyObject *pystr)
{
    /* METH_O */
    if (PyString_Check(pystr)) {
        return ascii_escape_str(pystr);
    }
    else if (PyUnicode_Check(pystr)) {
        return ascii_escape_unicode(pystr);
    }
    else {
        PyErr_Format(PyExc_TypeError, 
                     "first argument must be a string or unicode, not %.80s",
                     Py_TYPE(pystr)->tp_name);
        return NULL;
    }
}

static PyMethodDef json_methods[] = {
    {"encode_basestring_ascii", (PyCFunction)py_encode_basestring_ascii,
     METH_O, pydoc_encode_basestring_ascii},
    {"scanstring", (PyCFunction)py_scanstring, METH_VARARGS,
     pydoc_scanstring},
    {NULL, NULL, 0, NULL}
};

PyDoc_STRVAR(module_doc,
"json speedups\n");

void
init_json(void)
{
    PyObject *m;
    m = Py_InitModule3("_json", json_methods, module_doc);
}
