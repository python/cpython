/*

Unicode implementation based on original code by Fredrik Lundh,
modified by Marc-Andre Lemburg <mal@lemburg.com>.

Major speed upgrades to the method implementations at the Reykjavik
NeedForSpeed sprint, by Fredrik Lundh and Andrew Dalke.

Copyright (c) Corporation for National Research Initiatives.

--------------------------------------------------------------------
The original string type implementation is:

  Copyright (c) 1999 by Secret Labs AB
  Copyright (c) 1999 by Fredrik Lundh

By obtaining, using, and/or copying this software and/or its
associated documentation, you agree that you have read, understood,
and will comply with the following terms and conditions:

Permission to use, copy, modify, and distribute this software and its
associated documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies, and that both that copyright notice and this permission notice
appear in supporting documentation, and that the name of Secret Labs
AB or the author not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

SECRET LABS AB AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS.  IN NO EVENT SHALL SECRET LABS AB OR THE AUTHOR BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
--------------------------------------------------------------------

*/

// PyUnicode_Format() implementation

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_format.h"        // F_ALT
#include "pycore_long.h"          // _PyLong_FormatWriter()
#include "pycore_object.h"        // _PyObject_IsUniquelyReferenced()
#include "pycore_unicodeobject.h" // _Py_MAX_UNICODE


#define MAX_UNICODE _Py_MAX_UNICODE
#define ensure_unicode _PyUnicode_EnsureUnicode

struct unicode_formatter_t {
    PyObject *args;
    int args_owned;
    Py_ssize_t arglen, argidx;
    PyObject *dict;

    int fmtkind;
    Py_ssize_t fmtcnt, fmtpos;
    const void *fmtdata;
    PyObject *fmtstr;

    _PyUnicodeWriter writer;
};


struct unicode_format_arg_t {
    Py_UCS4 ch;
    int flags;
    Py_ssize_t width;
    int prec;
    int sign;
};


static PyObject *
unicode_format_getnextarg(struct unicode_formatter_t *ctx)
{
    Py_ssize_t argidx = ctx->argidx;

    if (argidx < ctx->arglen) {
        ctx->argidx++;
        if (ctx->arglen < 0)
            return ctx->args;
        else
            return PyTuple_GetItem(ctx->args, argidx);
    }
    PyErr_SetString(PyExc_TypeError,
                    "not enough arguments for format string");
    return NULL;
}


/* Returns a new reference to a PyUnicode object, or NULL on failure. */

/* Format a float into the writer if the writer is not NULL, or into *p_output
   otherwise.

   Return 0 on success, raise an exception and return -1 on error. */
static int
formatfloat(PyObject *v, struct unicode_format_arg_t *arg,
            PyObject **p_output,
            _PyUnicodeWriter *writer)
{
    char *p;
    double x;
    Py_ssize_t len;
    int prec;
    int dtoa_flags = 0;

    x = PyFloat_AsDouble(v);
    if (x == -1.0 && PyErr_Occurred())
        return -1;

    prec = arg->prec;
    if (prec < 0)
        prec = 6;

    if (arg->flags & F_ALT)
        dtoa_flags |= Py_DTSF_ALT;
    p = PyOS_double_to_string(x, arg->ch, prec, dtoa_flags, NULL);
    if (p == NULL)
        return -1;
    len = strlen(p);
    if (writer) {
        if (_PyUnicodeWriter_WriteASCIIString(writer, p, len) < 0) {
            PyMem_Free(p);
            return -1;
        }
    }
    else
        *p_output = _PyUnicode_FromASCII(p, len);
    PyMem_Free(p);
    return 0;
}


/* formatlong() emulates the format codes d, u, o, x and X, and
 * the F_ALT flag, for Python's long (unbounded) ints.  It's not used for
 * Python's regular ints.
 * Return value:  a new PyUnicodeObject*, or NULL if error.
 *     The output string is of the form
 *         "-"? ("0x" | "0X")? digit+
 *     "0x"/"0X" are present only for x and X conversions, with F_ALT
 *         set in flags.  The case of hex digits will be correct,
 *     There will be at least prec digits, zero-filled on the left if
 *         necessary to get that many.
 * val          object to be converted
 * flags        bitmask of format flags; only F_ALT is looked at
 * prec         minimum number of digits; 0-fill on left if needed
 * type         a character in [duoxX]; u acts the same as d
 *
 * CAUTION:  o, x and X conversions on regular ints can never
 * produce a '-' sign, but can for Python's unbounded ints.
 */
PyObject *
_PyUnicode_FormatLong(PyObject *val, int alt, int prec, int type)
{
    PyObject *result = NULL;
    char *buf;
    Py_ssize_t i;
    int sign;           /* 1 if '-', else 0 */
    int len;            /* number of characters */
    Py_ssize_t llen;
    int numdigits;      /* len == numnondigits + numdigits */
    int numnondigits = 0;

    /* Avoid exceeding SSIZE_T_MAX */
    if (prec > INT_MAX-3) {
        PyErr_SetString(PyExc_OverflowError,
                        "precision too large");
        return NULL;
    }

    assert(PyLong_Check(val));

    switch (type) {
    default:
        Py_UNREACHABLE();
    case 'd':
    case 'i':
    case 'u':
        /* int and int subclasses should print numerically when a numeric */
        /* format code is used (see issue18780) */
        result = PyNumber_ToBase(val, 10);
        break;
    case 'o':
        numnondigits = 2;
        result = PyNumber_ToBase(val, 8);
        break;
    case 'x':
    case 'X':
        numnondigits = 2;
        result = PyNumber_ToBase(val, 16);
        break;
    }
    if (!result)
        return NULL;

    assert(_PyUnicode_IsModifiable(result));
    assert(PyUnicode_IS_ASCII(result));

    /* To modify the string in-place, there can only be one reference. */
    if (!_PyObject_IsUniquelyReferenced(result)) {
        Py_DECREF(result);
        PyErr_BadInternalCall();
        return NULL;
    }
    buf = PyUnicode_DATA(result);
    llen = PyUnicode_GET_LENGTH(result);
    if (llen > INT_MAX) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_ValueError,
                        "string too large in _PyUnicode_FormatLong");
        return NULL;
    }
    len = (int)llen;
    sign = buf[0] == '-';
    numnondigits += sign;
    numdigits = len - numnondigits;
    assert(numdigits > 0);

    /* Get rid of base marker unless F_ALT */
    if (((alt) == 0 &&
        (type == 'o' || type == 'x' || type == 'X'))) {
        assert(buf[sign] == '0');
        assert(buf[sign+1] == 'x' || buf[sign+1] == 'X' ||
               buf[sign+1] == 'o');
        numnondigits -= 2;
        buf += 2;
        len -= 2;
        if (sign)
            buf[0] = '-';
        assert(len == numnondigits + numdigits);
        assert(numdigits > 0);
    }

    /* Fill with leading zeroes to meet minimum width. */
    if (prec > numdigits) {
        PyObject *r1 = PyBytes_FromStringAndSize(NULL,
                                numnondigits + prec);
        char *b1;
        if (!r1) {
            Py_DECREF(result);
            return NULL;
        }
        b1 = PyBytes_AS_STRING(r1);
        for (i = 0; i < numnondigits; ++i)
            *b1++ = *buf++;
        for (i = 0; i < prec - numdigits; i++)
            *b1++ = '0';
        for (i = 0; i < numdigits; i++)
            *b1++ = *buf++;
        *b1 = '\0';
        Py_SETREF(result, r1);
        buf = PyBytes_AS_STRING(result);
        len = numnondigits + prec;
    }

    /* Fix up case for hex conversions. */
    if (type == 'X') {
        /* Need to convert all lower case letters to upper case.
           and need to convert 0x to 0X (and -0x to -0X). */
        for (i = 0; i < len; i++)
            if (buf[i] >= 'a' && buf[i] <= 'x')
                buf[i] -= 'a'-'A';
    }
    if (!PyUnicode_Check(result)
        || buf != PyUnicode_DATA(result)) {
        PyObject *unicode;
        unicode = _PyUnicode_FromASCII(buf, len);
        Py_SETREF(result, unicode);
    }
    else if (len != PyUnicode_GET_LENGTH(result)) {
        if (PyUnicode_Resize(&result, len) < 0)
            Py_CLEAR(result);
    }
    return result;
}


/* Format an integer or a float as an integer.
 * Return 1 if the number has been formatted into the writer,
 *        0 if the number has been formatted into *p_output
 *       -1 and raise an exception on error */
static int
mainformatlong(PyObject *v,
               struct unicode_format_arg_t *arg,
               PyObject **p_output,
               _PyUnicodeWriter *writer)
{
    PyObject *iobj, *res;
    char type = (char)arg->ch;

    if (!PyNumber_Check(v))
        goto wrongtype;

    /* make sure number is a type of integer for o, x, and X */
    if (!PyLong_Check(v)) {
        if (type == 'o' || type == 'x' || type == 'X') {
            iobj = _PyNumber_Index(v);
        }
        else {
            iobj = PyNumber_Long(v);
        }
        if (iobj == NULL ) {
            if (PyErr_ExceptionMatches(PyExc_TypeError))
                goto wrongtype;
            return -1;
        }
        assert(PyLong_Check(iobj));
    }
    else {
        iobj = Py_NewRef(v);
    }

    if (PyLong_CheckExact(v)
        && arg->width == -1 && arg->prec == -1
        && !(arg->flags & (F_SIGN | F_BLANK))
        && type != 'X')
    {
        /* Fast path */
        int alternate = arg->flags & F_ALT;
        int base;

        switch(type)
        {
            default:
                Py_UNREACHABLE();
            case 'd':
            case 'i':
            case 'u':
                base = 10;
                break;
            case 'o':
                base = 8;
                break;
            case 'x':
            case 'X':
                base = 16;
                break;
        }

        if (_PyLong_FormatWriter(writer, v, base, alternate) == -1) {
            Py_DECREF(iobj);
            return -1;
        }
        Py_DECREF(iobj);
        return 1;
    }

    res = _PyUnicode_FormatLong(iobj, arg->flags & F_ALT, arg->prec, type);
    Py_DECREF(iobj);
    if (res == NULL)
        return -1;
    *p_output = res;
    return 0;

wrongtype:
    switch(type)
    {
        case 'o':
        case 'x':
        case 'X':
            PyErr_Format(PyExc_TypeError,
                    "%%%c format: an integer is required, "
                    "not %.200s",
                    type, Py_TYPE(v)->tp_name);
            break;
        default:
            PyErr_Format(PyExc_TypeError,
                    "%%%c format: a real number is required, "
                    "not %.200s",
                    type, Py_TYPE(v)->tp_name);
            break;
    }
    return -1;
}


static Py_UCS4
formatchar(PyObject *v)
{
    /* presume that the buffer is at least 3 characters long */
    if (PyUnicode_Check(v)) {
        if (PyUnicode_GET_LENGTH(v) == 1) {
            return PyUnicode_READ_CHAR(v, 0);
        }
        PyErr_Format(PyExc_TypeError,
                     "%%c requires an int or a unicode character, "
                     "not a string of length %zd",
                     PyUnicode_GET_LENGTH(v));
        return (Py_UCS4) -1;
    }
    else {
        int overflow;
        long x = PyLong_AsLongAndOverflow(v, &overflow);
        if (x == -1 && PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                PyErr_Format(PyExc_TypeError,
                             "%%c requires an int or a unicode character, not %T",
                             v);
                return (Py_UCS4) -1;
            }
            return (Py_UCS4) -1;
        }

        if (x < 0 || x > MAX_UNICODE) {
            /* this includes an overflow in converting to C long */
            PyErr_SetString(PyExc_OverflowError,
                            "%c arg not in range(0x110000)");
            return (Py_UCS4) -1;
        }

        return (Py_UCS4) x;
    }
}


/* Parse options of an argument: flags, width, precision.
   Handle also "%(name)" syntax.

   Return 0 if the argument has been formatted into arg->str.
   Return 1 if the argument has been written into ctx->writer,
   Raise an exception and return -1 on error. */
static int
unicode_format_arg_parse(struct unicode_formatter_t *ctx,
                         struct unicode_format_arg_t *arg)
{
#define FORMAT_READ(ctx) \
        PyUnicode_READ((ctx)->fmtkind, (ctx)->fmtdata, (ctx)->fmtpos)

    PyObject *v;

    if (arg->ch == '(') {
        /* Get argument value from a dictionary. Example: "%(name)s". */
        Py_ssize_t keystart;
        Py_ssize_t keylen;
        PyObject *key;
        int pcount = 1;

        if (ctx->dict == NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "format requires a mapping");
            return -1;
        }
        ++ctx->fmtpos;
        --ctx->fmtcnt;
        keystart = ctx->fmtpos;
        /* Skip over balanced parentheses */
        while (pcount > 0 && --ctx->fmtcnt >= 0) {
            arg->ch = FORMAT_READ(ctx);
            if (arg->ch == ')')
                --pcount;
            else if (arg->ch == '(')
                ++pcount;
            ctx->fmtpos++;
        }
        keylen = ctx->fmtpos - keystart - 1;
        if (ctx->fmtcnt < 0 || pcount > 0) {
            PyErr_SetString(PyExc_ValueError,
                            "incomplete format key");
            return -1;
        }
        key = PyUnicode_Substring(ctx->fmtstr,
                                  keystart, keystart + keylen);
        if (key == NULL)
            return -1;
        if (ctx->args_owned) {
            ctx->args_owned = 0;
            Py_DECREF(ctx->args);
        }
        ctx->args = PyObject_GetItem(ctx->dict, key);
        Py_DECREF(key);
        if (ctx->args == NULL)
            return -1;
        ctx->args_owned = 1;
        ctx->arglen = -1;
        ctx->argidx = -2;
    }

    /* Parse flags. Example: "%+i" => flags=F_SIGN. */
    while (--ctx->fmtcnt >= 0) {
        arg->ch = FORMAT_READ(ctx);
        ctx->fmtpos++;
        switch (arg->ch) {
        case '-': arg->flags |= F_LJUST; continue;
        case '+': arg->flags |= F_SIGN; continue;
        case ' ': arg->flags |= F_BLANK; continue;
        case '#': arg->flags |= F_ALT; continue;
        case '0': arg->flags |= F_ZERO; continue;
        }
        break;
    }

    /* Parse width. Example: "%10s" => width=10 */
    if (arg->ch == '*') {
        v = unicode_format_getnextarg(ctx);
        if (v == NULL)
            return -1;
        if (!PyLong_Check(v)) {
            PyErr_SetString(PyExc_TypeError,
                            "* wants int");
            return -1;
        }
        arg->width = PyLong_AsSsize_t(v);
        if (arg->width == -1 && PyErr_Occurred())
            return -1;
        if (arg->width < 0) {
            arg->flags |= F_LJUST;
            arg->width = -arg->width;
        }
        if (--ctx->fmtcnt >= 0) {
            arg->ch = FORMAT_READ(ctx);
            ctx->fmtpos++;
        }
    }
    else if (arg->ch >= '0' && arg->ch <= '9') {
        arg->width = arg->ch - '0';
        while (--ctx->fmtcnt >= 0) {
            arg->ch = FORMAT_READ(ctx);
            ctx->fmtpos++;
            if (arg->ch < '0' || arg->ch > '9')
                break;
            /* Since arg->ch is unsigned, the RHS would end up as unsigned,
               mixing signed and unsigned comparison. Since arg->ch is between
               '0' and '9', casting to int is safe. */
            if (arg->width > (PY_SSIZE_T_MAX - ((int)arg->ch - '0')) / 10) {
                PyErr_SetString(PyExc_ValueError,
                                "width too big");
                return -1;
            }
            arg->width = arg->width*10 + (arg->ch - '0');
        }
    }

    /* Parse precision. Example: "%.3f" => prec=3 */
    if (arg->ch == '.') {
        arg->prec = 0;
        if (--ctx->fmtcnt >= 0) {
            arg->ch = FORMAT_READ(ctx);
            ctx->fmtpos++;
        }
        if (arg->ch == '*') {
            v = unicode_format_getnextarg(ctx);
            if (v == NULL)
                return -1;
            if (!PyLong_Check(v)) {
                PyErr_SetString(PyExc_TypeError,
                                "* wants int");
                return -1;
            }
            arg->prec = PyLong_AsInt(v);
            if (arg->prec == -1 && PyErr_Occurred())
                return -1;
            if (arg->prec < 0)
                arg->prec = 0;
            if (--ctx->fmtcnt >= 0) {
                arg->ch = FORMAT_READ(ctx);
                ctx->fmtpos++;
            }
        }
        else if (arg->ch >= '0' && arg->ch <= '9') {
            arg->prec = arg->ch - '0';
            while (--ctx->fmtcnt >= 0) {
                arg->ch = FORMAT_READ(ctx);
                ctx->fmtpos++;
                if (arg->ch < '0' || arg->ch > '9')
                    break;
                if (arg->prec > (INT_MAX - ((int)arg->ch - '0')) / 10) {
                    PyErr_SetString(PyExc_ValueError,
                                    "precision too big");
                    return -1;
                }
                arg->prec = arg->prec*10 + (arg->ch - '0');
            }
        }
    }

    /* Ignore "h", "l" and "L" format prefix (ex: "%hi" or "%ls") */
    if (ctx->fmtcnt >= 0) {
        if (arg->ch == 'h' || arg->ch == 'l' || arg->ch == 'L') {
            if (--ctx->fmtcnt >= 0) {
                arg->ch = FORMAT_READ(ctx);
                ctx->fmtpos++;
            }
        }
    }
    if (ctx->fmtcnt < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "incomplete format");
        return -1;
    }
    return 0;

#undef FORMAT_READ
}


/* Format one argument. Supported conversion specifiers:

   - "s", "r", "a": any type
   - "i", "d", "u": int or float
   - "o", "x", "X": int
   - "e", "E", "f", "F", "g", "G": float
   - "c": int or str (1 character)

   When possible, the output is written directly into the Unicode writer
   (ctx->writer). A string is created when padding is required.

   Return 0 if the argument has been formatted into *p_str,
          1 if the argument has been written into ctx->writer,
         -1 on error. */
static int
unicode_format_arg_format(struct unicode_formatter_t *ctx,
                          struct unicode_format_arg_t *arg,
                          PyObject **p_str)
{
    PyObject *v;
    _PyUnicodeWriter *writer = &ctx->writer;

    if (ctx->fmtcnt == 0)
        ctx->writer.overallocate = 0;

    v = unicode_format_getnextarg(ctx);
    if (v == NULL)
        return -1;


    switch (arg->ch) {
    case 's':
    case 'r':
    case 'a':
        if (PyLong_CheckExact(v) && arg->width == -1 && arg->prec == -1) {
            /* Fast path */
            if (_PyLong_FormatWriter(writer, v, 10, arg->flags & F_ALT) == -1)
                return -1;
            return 1;
        }

        if (PyUnicode_CheckExact(v) && arg->ch == 's') {
            *p_str = Py_NewRef(v);
        }
        else {
            if (arg->ch == 's')
                *p_str = PyObject_Str(v);
            else if (arg->ch == 'r')
                *p_str = PyObject_Repr(v);
            else
                *p_str = PyObject_ASCII(v);
        }
        break;

    case 'i':
    case 'd':
    case 'u':
    case 'o':
    case 'x':
    case 'X':
    {
        int ret = mainformatlong(v, arg, p_str, writer);
        if (ret != 0)
            return ret;
        arg->sign = 1;
        break;
    }

    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
        if (arg->width == -1 && arg->prec == -1
            && !(arg->flags & (F_SIGN | F_BLANK)))
        {
            /* Fast path */
            if (formatfloat(v, arg, NULL, writer) == -1)
                return -1;
            return 1;
        }

        arg->sign = 1;
        if (formatfloat(v, arg, p_str, NULL) == -1)
            return -1;
        break;

    case 'c':
    {
        Py_UCS4 ch = formatchar(v);
        if (ch == (Py_UCS4) -1)
            return -1;
        if (arg->width == -1 && arg->prec == -1) {
            /* Fast path */
            if (_PyUnicodeWriter_WriteCharInline(writer, ch) < 0)
                return -1;
            return 1;
        }
        *p_str = PyUnicode_FromOrdinal(ch);
        break;
    }

    default:
        PyErr_Format(PyExc_ValueError,
                     "unsupported format character '%c' (0x%x) "
                     "at index %zd",
                     (31<=arg->ch && arg->ch<=126) ? (char)arg->ch : '?',
                     (int)arg->ch,
                     ctx->fmtpos - 1);
        return -1;
    }
    if (*p_str == NULL)
        return -1;
    assert (PyUnicode_Check(*p_str));
    return 0;
}


static int
unicode_format_arg_output(struct unicode_formatter_t *ctx,
                          struct unicode_format_arg_t *arg,
                          PyObject *str)
{
    Py_ssize_t len;
    int kind;
    const void *pbuf;
    Py_ssize_t pindex;
    Py_UCS4 signchar;
    Py_ssize_t buflen;
    Py_UCS4 maxchar;
    Py_ssize_t sublen;
    _PyUnicodeWriter *writer = &ctx->writer;
    Py_UCS4 fill;

    fill = ' ';
    if (arg->sign && arg->flags & F_ZERO)
        fill = '0';

    len = PyUnicode_GET_LENGTH(str);
    if ((arg->width == -1 || arg->width <= len)
        && (arg->prec == -1 || arg->prec >= len)
        && !(arg->flags & (F_SIGN | F_BLANK)))
    {
        /* Fast path */
        if (_PyUnicodeWriter_WriteStr(writer, str) == -1)
            return -1;
        return 0;
    }

    /* Truncate the string for "s", "r" and "a" formats
       if the precision is set */
    if (arg->ch == 's' || arg->ch == 'r' || arg->ch == 'a') {
        if (arg->prec >= 0 && len > arg->prec)
            len = arg->prec;
    }

    /* Adjust sign and width */
    kind = PyUnicode_KIND(str);
    pbuf = PyUnicode_DATA(str);
    pindex = 0;
    signchar = '\0';
    if (arg->sign) {
        Py_UCS4 ch = PyUnicode_READ(kind, pbuf, pindex);
        if (ch == '-' || ch == '+') {
            signchar = ch;
            len--;
            pindex++;
        }
        else if (arg->flags & F_SIGN)
            signchar = '+';
        else if (arg->flags & F_BLANK)
            signchar = ' ';
        else
            arg->sign = 0;
    }
    if (arg->width < len)
        arg->width = len;

    /* Prepare the writer */
    maxchar = writer->maxchar;
    if (!(arg->flags & F_LJUST)) {
        if (arg->sign) {
            if ((arg->width-1) > len)
                maxchar = Py_MAX(maxchar, fill);
        }
        else {
            if (arg->width > len)
                maxchar = Py_MAX(maxchar, fill);
        }
    }
    if (PyUnicode_MAX_CHAR_VALUE(str) > maxchar) {
        Py_UCS4 strmaxchar = _PyUnicode_FindMaxChar(str, 0, pindex+len);
        maxchar = Py_MAX(maxchar, strmaxchar);
    }

    buflen = arg->width;
    if (arg->sign && len == arg->width)
        buflen++;
    if (_PyUnicodeWriter_Prepare(writer, buflen, maxchar) == -1)
        return -1;

    /* Write the sign if needed */
    if (arg->sign) {
        if (fill != ' ') {
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos, signchar);
            writer->pos += 1;
        }
        if (arg->width > len)
            arg->width--;
    }

    /* Write the numeric prefix for "x", "X" and "o" formats
       if the alternate form is used.
       For example, write "0x" for the "%#x" format. */
    if ((arg->flags & F_ALT) && (arg->ch == 'x' || arg->ch == 'X' || arg->ch == 'o')) {
        assert(PyUnicode_READ(kind, pbuf, pindex) == '0');
        assert(PyUnicode_READ(kind, pbuf, pindex + 1) == arg->ch);
        if (fill != ' ') {
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos, '0');
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos+1, arg->ch);
            writer->pos += 2;
            pindex += 2;
        }
        arg->width -= 2;
        if (arg->width < 0)
            arg->width = 0;
        len -= 2;
    }

    /* Pad left with the fill character if needed */
    if (arg->width > len && !(arg->flags & F_LJUST)) {
        sublen = arg->width - len;
        _PyUnicode_Fill(writer->kind, writer->data, fill, writer->pos, sublen);
        writer->pos += sublen;
        arg->width = len;
    }

    /* If padding with spaces: write sign if needed and/or numeric prefix if
       the alternate form is used */
    if (fill == ' ') {
        if (arg->sign) {
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos, signchar);
            writer->pos += 1;
        }
        if ((arg->flags & F_ALT) && (arg->ch == 'x' || arg->ch == 'X' || arg->ch == 'o')) {
            assert(PyUnicode_READ(kind, pbuf, pindex) == '0');
            assert(PyUnicode_READ(kind, pbuf, pindex+1) == arg->ch);
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos, '0');
            PyUnicode_WRITE(writer->kind, writer->data, writer->pos+1, arg->ch);
            writer->pos += 2;
            pindex += 2;
        }
    }

    /* Write characters */
    if (len) {
        _PyUnicode_FastCopyCharacters(writer->buffer, writer->pos,
                                      str, pindex, len);
        writer->pos += len;
    }

    /* Pad right with the fill character if needed */
    if (arg->width > len) {
        sublen = arg->width - len;
        _PyUnicode_Fill(writer->kind, writer->data, ' ', writer->pos, sublen);
        writer->pos += sublen;
    }
    return 0;
}


/* Helper of PyUnicode_Format(): format one arg.
   Return 0 on success, raise an exception and return -1 on error. */
static int
unicode_format_arg(struct unicode_formatter_t *ctx)
{
    struct unicode_format_arg_t arg;
    PyObject *str;
    int ret;

    arg.ch = PyUnicode_READ(ctx->fmtkind, ctx->fmtdata, ctx->fmtpos);
    if (arg.ch == '%') {
        ctx->fmtpos++;
        ctx->fmtcnt--;
        if (_PyUnicodeWriter_WriteCharInline(&ctx->writer, '%') < 0)
            return -1;
        return 0;
    }
    arg.flags = 0;
    arg.width = -1;
    arg.prec = -1;
    arg.sign = 0;
    str = NULL;

    ret = unicode_format_arg_parse(ctx, &arg);
    if (ret == -1)
        return -1;

    ret = unicode_format_arg_format(ctx, &arg, &str);
    if (ret == -1)
        return -1;

    if (ret != 1) {
        ret = unicode_format_arg_output(ctx, &arg, str);
        Py_DECREF(str);
        if (ret == -1)
            return -1;
    }

    if (ctx->dict && (ctx->argidx < ctx->arglen)) {
        PyErr_SetString(PyExc_TypeError,
                        "not all arguments converted during string formatting");
        return -1;
    }
    return 0;
}


PyObject *
PyUnicode_Format(PyObject *format, PyObject *args)
{
    struct unicode_formatter_t ctx;

    if (format == NULL || args == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (ensure_unicode(format) < 0)
        return NULL;

    ctx.fmtstr = format;
    ctx.fmtdata = PyUnicode_DATA(ctx.fmtstr);
    ctx.fmtkind = PyUnicode_KIND(ctx.fmtstr);
    ctx.fmtcnt = PyUnicode_GET_LENGTH(ctx.fmtstr);
    ctx.fmtpos = 0;

    _PyUnicodeWriter_Init(&ctx.writer);
    ctx.writer.min_length = ctx.fmtcnt + 100;
    ctx.writer.overallocate = 1;

    if (PyTuple_Check(args)) {
        ctx.arglen = PyTuple_Size(args);
        ctx.argidx = 0;
    }
    else {
        ctx.arglen = -1;
        ctx.argidx = -2;
    }
    ctx.args_owned = 0;
    if (PyMapping_Check(args) && !PyTuple_Check(args) && !PyUnicode_Check(args))
        ctx.dict = args;
    else
        ctx.dict = NULL;
    ctx.args = args;

    while (--ctx.fmtcnt >= 0) {
        if (PyUnicode_READ(ctx.fmtkind, ctx.fmtdata, ctx.fmtpos) != '%') {
            Py_ssize_t nonfmtpos;

            nonfmtpos = ctx.fmtpos++;
            while (ctx.fmtcnt >= 0 &&
                   PyUnicode_READ(ctx.fmtkind, ctx.fmtdata, ctx.fmtpos) != '%') {
                ctx.fmtpos++;
                ctx.fmtcnt--;
            }
            if (ctx.fmtcnt < 0) {
                ctx.fmtpos--;
                ctx.writer.overallocate = 0;
            }

            if (_PyUnicodeWriter_WriteSubstring(&ctx.writer, ctx.fmtstr,
                                                nonfmtpos, ctx.fmtpos) < 0)
                goto onError;
        }
        else {
            ctx.fmtpos++;
            if (unicode_format_arg(&ctx) == -1)
                goto onError;
        }
    }

    if (ctx.argidx < ctx.arglen && !ctx.dict) {
        PyErr_SetString(PyExc_TypeError,
                        "not all arguments converted during string formatting");
        goto onError;
    }

    if (ctx.args_owned) {
        Py_DECREF(ctx.args);
    }
    return _PyUnicodeWriter_Finish(&ctx.writer);

  onError:
    _PyUnicodeWriter_Dealloc(&ctx.writer);
    if (ctx.args_owned) {
        Py_DECREF(ctx.args);
    }
    return NULL;
}
