#include <Python.h>
#include "pycore_bytesobject.h"   // _PyBytes_DecodeEscape()
#include "pycore_unicodeobject.h" // _PyUnicode_DecodeUnicodeEscapeInternal()

#include "lexer/state.h"
#include "pegen.h"
#include "string_parser.h"

#include <stdbool.h>

//// STRING HANDLING FUNCTIONS ////

static int
warn_invalid_escape_sequence(Parser *p, const char* buffer, const char *first_invalid_escape, Token *t)
{
    if (p->call_invalid_rules) {
        // Do not report warnings if we are in the second pass of the parser
        // to avoid showing the warning twice.
        return 0;
    }
    unsigned char c = (unsigned char)*first_invalid_escape;
    if ((t->type == FSTRING_MIDDLE || t->type == FSTRING_END) && (c == '{' || c == '}')) {
        // in this case the tokenizer has already emitted a warning,
        // see Parser/tokenizer/helpers.c:warn_invalid_escape_sequence
        return 0;
    }

    int octal = ('4' <= c && c <= '7');
    PyObject *msg =
        octal
        ? PyUnicode_FromFormat("invalid octal escape sequence '\\%.3s'",
                               first_invalid_escape)
        : PyUnicode_FromFormat("invalid escape sequence '\\%c'", c);
    if (msg == NULL) {
        return -1;
    }
    PyObject *category;
    if (p->feature_version >= 12) {
        category = PyExc_SyntaxWarning;
    }
    else {
        category = PyExc_DeprecationWarning;
    }

    // Calculate the lineno and the col_offset of the invalid escape sequence
    const char *start = buffer;
    const char *end = first_invalid_escape;
    int lineno = t->lineno;
    int col_offset = t->col_offset;
    while (start < end) {
        if (*start == '\n') {
            lineno++;
            col_offset = 0;
        }
        else {
            col_offset++;
        }
        start++;
    }

    // Count the number of quotes in the token
    char first_quote = 0;
    if (lineno == t->lineno) {
        int quote_count = 0;
        char* tok = PyBytes_AsString(t->bytes);
        for (int i = 0; i < PyBytes_Size(t->bytes); i++) {
            if (tok[i] == '\'' || tok[i] == '\"') {
                if (quote_count == 0) {
                    first_quote = tok[i];
                }
                if (tok[i] == first_quote) {
                    quote_count++;
                }
            } else {
                break;
            }
        }

        col_offset += quote_count;
    }

    if (PyErr_WarnExplicitObject(category, msg, p->tok->filename,
                                 lineno, NULL, NULL) < 0) {
        if (PyErr_ExceptionMatches(category)) {
            /* Replace the Syntax/DeprecationWarning exception with a SyntaxError
               to get a more accurate error report */
            PyErr_Clear();

            /* This is needed, in order for the SyntaxError to point to the token t,
               since _PyPegen_raise_error uses p->tokens[p->fill - 1] for the
               error location, if p->known_err_token is not set. */
            p->known_err_token = t;
            if (octal) {
                RAISE_ERROR_KNOWN_LOCATION(p, PyExc_SyntaxError, lineno, col_offset-1, lineno, col_offset+1,
                "invalid octal escape sequence '\\%.3s'", first_invalid_escape);
            }
            else {
                RAISE_ERROR_KNOWN_LOCATION(p, PyExc_SyntaxError, lineno, col_offset-1, lineno, col_offset+1,
                "invalid escape sequence '\\%c'", c);
            }
        }
        Py_DECREF(msg);
        return -1;
    }
    Py_DECREF(msg);
    return 0;
}

static PyObject *
decode_utf8(const char **sPtr, const char *end)
{
    const char *s;
    const char *t;
    t = s = *sPtr;
    while (s < end && (*s & 0x80)) {
        s++;
    }
    *sPtr = s;
    return PyUnicode_DecodeUTF8(t, s - t, NULL);
}

static PyObject *
decode_unicode_with_escapes(Parser *parser, const char *s, size_t len, Token *t)
{
    PyObject *v;
    PyObject *u;
    char *buf;
    char *p;
    const char *end;

    /* check for integer overflow */
    if (len > (size_t)PY_SSIZE_T_MAX / 6) {
        return NULL;
    }
    /* "ä" (2 bytes) may become "\U000000E4" (10 bytes), or 1:5
       "\ä" (3 bytes) may become "\u005c\U000000E4" (16 bytes), or ~1:6 */
    u = PyBytes_FromStringAndSize((char *)NULL, (Py_ssize_t)len * 6);
    if (u == NULL) {
        return NULL;
    }
    p = buf = PyBytes_AsString(u);
    if (p == NULL) {
        return NULL;
    }
    end = s + len;
    while (s < end) {
        if (*s == '\\') {
            *p++ = *s++;
            if (s >= end || *s & 0x80) {
                strcpy(p, "u005c");
                p += 5;
                if (s >= end) {
                    break;
                }
            }
        }
        if (*s & 0x80) {
            PyObject *w;
            int kind;
            const void *data;
            Py_ssize_t w_len;
            Py_ssize_t i;
            w = decode_utf8(&s, end);
            if (w == NULL) {
                Py_DECREF(u);
                return NULL;
            }
            kind = PyUnicode_KIND(w);
            data = PyUnicode_DATA(w);
            w_len = PyUnicode_GET_LENGTH(w);
            for (i = 0; i < w_len; i++) {
                Py_UCS4 chr = PyUnicode_READ(kind, data, i);
                sprintf(p, "\\U%08x", chr);
                p += 10;
            }
            /* Should be impossible to overflow */
            assert(p - buf <= PyBytes_GET_SIZE(u));
            Py_DECREF(w);
        }
        else {
            *p++ = *s++;
        }
    }
    len = (size_t)(p - buf);
    s = buf;

    const char *first_invalid_escape;
    v = _PyUnicode_DecodeUnicodeEscapeInternal(s, (Py_ssize_t)len, NULL, NULL, &first_invalid_escape);

    // HACK: later we can simply pass the line no, since we don't preserve the tokens
    // when we are decoding the string but we preserve the line numbers.
    if (v != NULL && first_invalid_escape != NULL && t != NULL) {
        if (warn_invalid_escape_sequence(parser, s, first_invalid_escape, t) < 0) {
            /* We have not decref u before because first_invalid_escape points
               inside u. */
            Py_XDECREF(u);
            Py_DECREF(v);
            return NULL;
        }
    }
    Py_XDECREF(u);
    return v;
}

static PyObject *
decode_bytes_with_escapes(Parser *p, const char *s, Py_ssize_t len, Token *t)
{
    const char *first_invalid_escape;
    PyObject *result = _PyBytes_DecodeEscape(s, len, NULL, &first_invalid_escape);
    if (result == NULL) {
        return NULL;
    }

    if (first_invalid_escape != NULL) {
        if (warn_invalid_escape_sequence(p, s, first_invalid_escape, t) < 0) {
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

PyObject *
_PyPegen_decode_string(Parser *p, int raw, const char *s, size_t len, Token *t)
{
    if (raw) {
        return PyUnicode_DecodeUTF8Stateful(s, (Py_ssize_t)len, NULL, NULL);
    }
    return decode_unicode_with_escapes(p, s, len, t);
}

/* s must include the bracketing quote characters, and r, b &/or f prefixes
    (if any), and embedded escape sequences (if any). (f-strings are handled by the parser)
   _PyPegen_parse_string parses it, and returns the decoded Python string object. */
PyObject *
_PyPegen_parse_string(Parser *p, Token *t)
{
    const char *s = PyBytes_AsString(t->bytes);
    if (s == NULL) {
        return NULL;
    }

    size_t len;
    int quote = Py_CHARMASK(*s);
    int bytesmode = 0;
    int rawmode = 0;

    if (Py_ISALPHA(quote)) {
        while (!bytesmode || !rawmode) {
            if (quote == 'b' || quote == 'B') {
                quote =(unsigned char)*++s;
                bytesmode = 1;
            }
            else if (quote == 'u' || quote == 'U') {
                quote = (unsigned char)*++s;
            }
            else if (quote == 'r' || quote == 'R') {
                quote = (unsigned char)*++s;
                rawmode = 1;
            }
            else {
                break;
            }
        }
    }

    if (quote != '\'' && quote != '\"') {
        PyErr_BadInternalCall();
        return NULL;
    }

    /* Skip the leading quote char. */
    s++;
    len = strlen(s);
    // gh-120155: 's' contains at least the trailing quote,
    // so the code '--len' below is safe.
    assert(len >= 1);

    if (len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "string to parse is too long");
        return NULL;
    }
    if (s[--len] != quote) {
        /* Last quote char must match the first. */
        PyErr_BadInternalCall();
        return NULL;
    }
    if (len >= 4 && s[0] == quote && s[1] == quote) {
        /* A triple quoted string. We've already skipped one quote at
           the start and one at the end of the string. Now skip the
           two at the start. */
        s += 2;
        len -= 2;
        /* And check that the last two match. */
        if (s[--len] != quote || s[--len] != quote) {
            PyErr_BadInternalCall();
            return NULL;
        }
    }

    /* Avoid invoking escape decoding routines if possible. */
    rawmode = rawmode || strchr(s, '\\') == NULL;
    if (bytesmode) {
        /* Disallow non-ASCII characters. */
        const char *ch;
        for (ch = s; *ch; ch++) {
            if (Py_CHARMASK(*ch) >= 0x80) {
                RAISE_SYNTAX_ERROR_KNOWN_LOCATION(
                                   t,
                                   "bytes can only contain ASCII "
                                   "literal characters");
                return NULL;
            }
        }
        if (rawmode) {
            return PyBytes_FromStringAndSize(s, (Py_ssize_t)len);
        }
        return decode_bytes_with_escapes(p, s, (Py_ssize_t)len, t);
    }
    return _PyPegen_decode_string(p, rawmode, s, len, t);
}
