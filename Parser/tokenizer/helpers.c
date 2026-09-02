#include "Python.h"
#include "errcode.h"
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_token.h"
#include "pycore_tuple.h"         // _PyTuple_FromPair

#include "../lexer/state.h"


/* ############## ERRORS ############## */

static int
_syntaxerror_range(struct tok_state *tok, const char *format,
                   int col_offset, int end_col_offset,
                   va_list vargs)
{
    // In release builds, we don't want to overwrite a previous error, but in debug builds we
    // want to fail if we are not doing it so we can fix it.
    assert(tok->done != E_ERROR);
    if (tok->done == E_ERROR) {
        return ERRORTOKEN;
    }
    PyObject *errmsg, *errtext, *args;
    errmsg = PyUnicode_FromFormatV(format, vargs);
    if (!errmsg) {
        goto error;
    }

    errtext = PyUnicode_DecodeUTF8(tok->line_start, tok->cur - tok->line_start,
                                   "replace");
    if (!errtext) {
        goto error;
    }

    if (col_offset == -1) {
        col_offset = (int)PyUnicode_GET_LENGTH(errtext);
    }
    if (end_col_offset == -1) {
        end_col_offset = col_offset;
    }

    Py_ssize_t line_len = strcspn(tok->line_start, "\n");
    if (line_len != tok->cur - tok->line_start) {
        Py_DECREF(errtext);
        errtext = PyUnicode_DecodeUTF8(tok->line_start, line_len,
                                       "replace");
    }
    if (!errtext) {
        goto error;
    }

    args = Py_BuildValue("(O(OiiNii))", errmsg,
                         tok->filename ? tok->filename : Py_None,
                         tok->lineno, col_offset, errtext,
                         tok->lineno, end_col_offset);
    if (args) {
        PyErr_SetObject(PyExc_SyntaxError, args);
        Py_DECREF(args);
    }

error:
    Py_XDECREF(errmsg);
    tok->done = E_ERROR;
    return ERRORTOKEN;
}

int
_PyTokenizer_syntaxerror(struct tok_state *tok, const char *format, ...)
{
    // These errors are cleaned on startup. Todo: Fix it.
    va_list vargs;
    va_start(vargs, format);
    int ret = _syntaxerror_range(tok, format, -1, -1, vargs);
    va_end(vargs);
    return ret;
}

int
_PyTokenizer_syntaxerror_known_range(struct tok_state *tok,
                        int col_offset, int end_col_offset,
                        const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    int ret = _syntaxerror_range(tok, format, col_offset, end_col_offset, vargs);
    va_end(vargs);
    return ret;
}

int
_PyTokenizer_indenterror(struct tok_state *tok)
{
    tok->done = E_TABSPACE;
    tok->cur = tok->inp;
    return ERRORTOKEN;
}

int
_PyTokenizer_warn_invalid_escape_sequence(struct tok_state *tok, int first_invalid_escape_char)
{
    if (!tok->report_warnings) {
        return 0;
    }

    PyObject *msg = PyUnicode_FromFormat(
        "\"\\%c\" is an invalid escape sequence. "
        "Such sequences will not work in the future. "
        "Did you mean \"\\\\%c\"? A raw string is also an option.",
        (char) first_invalid_escape_char,
        (char) first_invalid_escape_char
    );

    if (msg == NULL) {
        return -1;
    }

    if (PyErr_WarnExplicitObject(PyExc_SyntaxWarning, msg, tok->filename,
                                 tok->lineno, tok->module, NULL) < 0) {
        Py_DECREF(msg);

        if (PyErr_ExceptionMatches(PyExc_SyntaxWarning)) {
            /* Replace the SyntaxWarning exception with a SyntaxError
               to get a more accurate error report */
            PyErr_Clear();

            return _PyTokenizer_syntaxerror(tok,
                "\"\\%c\" is an invalid escape sequence. "
                "Did you mean \"\\\\%c\"? A raw string is also an option.",
                (char) first_invalid_escape_char,
                (char) first_invalid_escape_char);
        }

        return -1;
    }

    Py_DECREF(msg);
    return 0;
}

void
_PyTokenizer_raise_init_error(PyObject *filename)
{
    if (!(PyErr_ExceptionMatches(PyExc_LookupError)
          || PyErr_ExceptionMatches(PyExc_SyntaxError)
          || PyErr_ExceptionMatches(PyExc_ValueError)
          || PyErr_ExceptionMatches(PyExc_UnicodeDecodeError))) {
        return;
    }
    PyObject *errstr = NULL;
    PyObject *tuple = NULL;
    PyObject *type;
    PyObject *value;
    PyObject *tback;
    PyErr_Fetch(&type, &value, &tback);
    if (PyErr_GivenExceptionMatches(value, PyExc_SyntaxError)) {
        if (PyObject_SetAttr(value, &_Py_ID(filename), filename)) {
            goto error;
        }
        PyErr_Restore(type, value, tback);
        return;
    }
    errstr = PyObject_Str(value);
    if (!errstr) {
        goto error;
    }

    PyObject *tmp = Py_BuildValue("(OiiO)", filename, 0, -1, Py_None);
    if (!tmp) {
        goto error;
    }

    tuple = _PyTuple_FromPair(errstr, tmp);
    Py_DECREF(tmp);
    if (!tuple) {
        goto error;
    }
    PyErr_SetObject(PyExc_SyntaxError, tuple);

error:
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(tback);
    Py_XDECREF(errstr);
    Py_XDECREF(tuple);
}

int
_PyTokenizer_parser_warn(struct tok_state *tok, PyObject *category, const char *format, ...)
{
    if (!tok->report_warnings) {
        return 0;
    }

    PyObject *errmsg;
    va_list vargs;
    va_start(vargs, format);
    errmsg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (!errmsg) {
        goto error;
    }

    if (PyErr_WarnExplicitObject(category, errmsg, tok->filename,
                                 tok->lineno, tok->module, NULL) < 0) {
        if (PyErr_ExceptionMatches(category)) {
            /* Replace the DeprecationWarning exception with a SyntaxError
               to get a more accurate error report */
            PyErr_Clear();
            _PyTokenizer_syntaxerror(tok, "%U", errmsg);
        }
        goto error;
    }
    Py_DECREF(errmsg);
    return 0;

error:
    Py_XDECREF(errmsg);
    tok->done = E_ERROR;
    return -1;
}


/* Check whether the characters at s start a valid
   UTF-8 sequence. Return the number of characters forming
   the sequence if yes, 0 if not.  The special cases match
   those in stringlib/codecs.h:utf8_decode.
*/
static int
valid_utf8(const unsigned char* s)
{
    int expected = 0;
    int length;
    if (*s < 0x80) {
        /* single-byte code */
        return 1;
    }
    else if (*s < 0xE0) {
        /* \xC2\x80-\xDF\xBF -- 0080-07FF */
        if (*s < 0xC2) {
            /* invalid sequence
               \x80-\xBF -- continuation byte
               \xC0-\xC1 -- fake 0000-007F */
            return 0;
        }
        expected = 1;
    }
    else if (*s < 0xF0) {
        /* \xE0\xA0\x80-\xEF\xBF\xBF -- 0800-FFFF */
        if (*s == 0xE0 && *(s + 1) < 0xA0) {
            /* invalid sequence
               \xE0\x80\x80-\xE0\x9F\xBF -- fake 0000-0800 */
            return 0;
        }
        else if (*s == 0xED && *(s + 1) >= 0xA0) {
            /* Decoding UTF-8 sequences in range \xED\xA0\x80-\xED\xBF\xBF
               will result in surrogates in range D800-DFFF. Surrogates are
               not valid UTF-8 so they are rejected.
               See https://www.unicode.org/versions/Unicode5.2.0/ch03.pdf
               (table 3-7) and http://www.rfc-editor.org/rfc/rfc3629.txt */
            return 0;
        }
        expected = 2;
    }
    else if (*s < 0xF5) {
        /* \xF0\x90\x80\x80-\xF4\x8F\xBF\xBF -- 10000-10FFFF */
        if (*(s + 1) < 0x90 ? *s == 0xF0 : *s == 0xF4) {
            /* invalid sequence -- one of:
               \xF0\x80\x80\x80-\xF0\x8F\xBF\xBF -- fake 0000-FFFF
               \xF4\x90\x80\x80- -- 110000- overflow */
            return 0;
        }
        expected = 3;
    }
    else {
        /* invalid start byte */
        return 0;
    }
    length = expected + 1;
    for (int i = 1; i <= expected; i++) {
        if (s[i] < 0x80 || s[i] >= 0xC0) {
            return 0;
        }
    }
    return length;
}

int
_PyTokenizer_ensure_utf8(const char *line, struct tok_state *tok, int lineno)
{
    const char *badchar = NULL;
    const char *c;
    int length;
    int col_offset = 0;
    const char *line_start = line;
    for (c = line; *c; c += length) {
        if (!(length = valid_utf8((const unsigned char *)c))) {
            badchar = c;
            break;
        }
        col_offset++;
        if (*c == '\n') {
            lineno++;
            col_offset = 0;
            line_start = c + 1;
        }
    }
    if (badchar) {
        tok->lineno = lineno;
        tok->line_start = line_start;
        tok->cur = (char *)badchar;
        _PyTokenizer_syntaxerror_known_range(tok,
                col_offset + 1, col_offset + 1,
                "Non-UTF-8 code starting with '\\x%.2x'"
                "%s%V on line %i, "
                "but no encoding declared; "
                "see https://peps.python.org/pep-0263/ for details",
                (unsigned char)*badchar,
                tok->filename ? " in file " : "", tok->filename, "",
                lineno);
        return 0;
    }
    return 1;
}


/* ############## DEBUGGING STUFF ############## */

#ifdef Py_DEBUG
void
_PyTokenizer_print_escape(FILE *f, const char *s, Py_ssize_t size)
{
    if (s == NULL) {
        fputs("NULL", f);
        return;
    }
    putc('"', f);
    while (size-- > 0) {
        unsigned char c = *s++;
        switch (c) {
            case '\n': fputs("\\n", f); break;
            case '\r': fputs("\\r", f); break;
            case '\t': fputs("\\t", f); break;
            case '\f': fputs("\\f", f); break;
            case '\'': fputs("\\'", f); break;
            case '"': fputs("\\\"", f); break;
            default:
                if (0x20 <= c && c <= 0x7f)
                    putc(c, f);
                else
                    fprintf(f, "\\x%02x", c);
        }
    }
    putc('"', f);
}

void
_PyTokenizer_tok_dump(int type, char *start, char *end)
{
    fprintf(stderr, "%s", _PyParser_TokenNames[type]);
    if (type == NAME || type == NUMBER || type == STRING || type == OP)
        fprintf(stderr, "(%.*s)", (int)(end - start), start);
}
#endif
