#include "Python.h"
#include "errcode.h"
#include "pycore_token.h"

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

    args = Py_BuildValue("(O(OiiNii))", errmsg, tok->filename, tok->lineno,
                         col_offset, errtext, tok->lineno, end_col_offset);
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
    // This errors are cleaned on startup. Todo: Fix it.
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

char *
_PyTokenizer_error_ret(struct tok_state *tok) /* XXX */
{
    tok->decoding_erred = 1;
    if ((tok->fp != NULL || tok->readline != NULL) && tok->buf != NULL) {/* see _PyTokenizer_Free */
        PyMem_Free(tok->buf);
    }
    tok->buf = tok->cur = tok->inp = NULL;
    tok->start = NULL;
    tok->end = NULL;
    tok->done = E_DECODE;
    return NULL;                /* as if it were EOF */
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
                                 tok->lineno, NULL, NULL) < 0) {
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
                                 tok->lineno, NULL, NULL) < 0) {
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


/* ############## STRING MANIPULATION ############## */

char *
_PyTokenizer_new_string(const char *s, Py_ssize_t len, struct tok_state *tok)
{
    char* result = (char *)PyMem_Malloc(len + 1);
    if (!result) {
        tok->done = E_NOMEM;
        return NULL;
    }
    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}

PyObject *
_PyTokenizer_translate_into_utf8(const char* str, const char* enc) {
    PyObject *utf8;
    PyObject* buf = PyUnicode_Decode(str, strlen(str), enc, NULL);
    if (buf == NULL)
        return NULL;
    utf8 = PyUnicode_AsUTF8String(buf);
    Py_DECREF(buf);
    return utf8;
}

char *
_PyTokenizer_translate_newlines(const char *s, int exec_input, int preserve_crlf,
                   struct tok_state *tok) {
    int skip_next_lf = 0;
    size_t needed_length = strlen(s) + 2, final_length;
    char *buf, *current;
    char c = '\0';
    buf = PyMem_Malloc(needed_length);
    if (buf == NULL) {
        tok->done = E_NOMEM;
        return NULL;
    }
    for (current = buf; *s; s++, current++) {
        c = *s;
        if (skip_next_lf) {
            skip_next_lf = 0;
            if (c == '\n') {
                c = *++s;
                if (!c)
                    break;
            }
        }
        if (!preserve_crlf && c == '\r') {
            skip_next_lf = 1;
            c = '\n';
        }
        *current = c;
    }
    /* If this is exec input, add a newline to the end of the string if
       there isn't one already. */
    if (exec_input && c != '\n' && c != '\0') {
        *current = '\n';
        current++;
    }
    *current = '\0';
    final_length = current - buf + 1;
    if (final_length < needed_length && final_length) {
        /* should never fail */
        char* result = PyMem_Realloc(buf, final_length);
        if (result == NULL) {
            PyMem_Free(buf);
        }
        buf = result;
    }
    return buf;
}

/* ############## ENCODING STUFF ############## */


/* See whether the file starts with a BOM. If it does,
   invoke the set_readline function with the new encoding.
   Return 1 on success, 0 on failure.  */
int
_PyTokenizer_check_bom(int get_char(struct tok_state *),
          void unget_char(int, struct tok_state *),
          int set_readline(struct tok_state *, const char *),
          struct tok_state *tok)
{
    int ch1, ch2, ch3;
    ch1 = get_char(tok);
    tok->decoding_state = STATE_SEEK_CODING;
    if (ch1 == EOF) {
        return 1;
    } else if (ch1 == 0xEF) {
        ch2 = get_char(tok);
        if (ch2 != 0xBB) {
            unget_char(ch2, tok);
            unget_char(ch1, tok);
            return 1;
        }
        ch3 = get_char(tok);
        if (ch3 != 0xBF) {
            unget_char(ch3, tok);
            unget_char(ch2, tok);
            unget_char(ch1, tok);
            return 1;
        }
    } else {
        unget_char(ch1, tok);
        return 1;
    }
    if (tok->encoding != NULL)
        PyMem_Free(tok->encoding);
    tok->encoding = _PyTokenizer_new_string("utf-8", 5, tok);
    if (!tok->encoding)
        return 0;
    /* No need to set_readline: input is already utf-8 */
    return 1;
}

static const char *
get_normal_name(const char *s)  /* for utf-8 and latin-1 */
{
    char buf[13];
    int i;
    for (i = 0; i < 12; i++) {
        int c = s[i];
        if (c == '\0')
            break;
        else if (c == '_')
            buf[i] = '-';
        else
            buf[i] = Py_TOLOWER(c);
    }
    buf[i] = '\0';
    if (strcmp(buf, "utf-8") == 0 ||
        strncmp(buf, "utf-8-", 6) == 0)
        return "utf-8";
    else if (strcmp(buf, "latin-1") == 0 ||
             strcmp(buf, "iso-8859-1") == 0 ||
             strcmp(buf, "iso-latin-1") == 0 ||
             strncmp(buf, "latin-1-", 8) == 0 ||
             strncmp(buf, "iso-8859-1-", 11) == 0 ||
             strncmp(buf, "iso-latin-1-", 12) == 0)
        return "iso-8859-1";
    else
        return s;
}

/* Return the coding spec in S, or NULL if none is found.  */
static int
get_coding_spec(const char *s, char **spec, Py_ssize_t size, struct tok_state *tok)
{
    Py_ssize_t i;
    *spec = NULL;
    /* Coding spec must be in a comment, and that comment must be
     * the only statement on the source code line. */
    for (i = 0; i < size - 6; i++) {
        if (s[i] == '#')
            break;
        if (s[i] != ' ' && s[i] != '\t' && s[i] != '\014')
            return 1;
    }
    for (; i < size - 6; i++) { /* XXX inefficient search */
        const char* t = s + i;
        if (memcmp(t, "coding", 6) == 0) {
            const char* begin = NULL;
            t += 6;
            if (t[0] != ':' && t[0] != '=')
                continue;
            do {
                t++;
            } while (t[0] == ' ' || t[0] == '\t');

            begin = t;
            while (Py_ISALNUM(t[0]) ||
                   t[0] == '-' || t[0] == '_' || t[0] == '.')
                t++;

            if (begin < t) {
                char* r = _PyTokenizer_new_string(begin, t - begin, tok);
                const char* q;
                if (!r)
                    return 0;
                q = get_normal_name(r);
                if (r != q) {
                    PyMem_Free(r);
                    r = _PyTokenizer_new_string(q, strlen(q), tok);
                    if (!r)
                        return 0;
                }
                *spec = r;
                break;
            }
        }
    }
    return 1;
}

/* Check whether the line contains a coding spec. If it does,
   invoke the set_readline function for the new encoding.
   This function receives the tok_state and the new encoding.
   Return 1 on success, 0 on failure.  */
int
_PyTokenizer_check_coding_spec(const char* line, Py_ssize_t size, struct tok_state *tok,
                  int set_readline(struct tok_state *, const char *))
{
    char *cs;
    if (tok->cont_line) {
        /* It's a continuation line, so it can't be a coding spec. */
        tok->decoding_state = STATE_NORMAL;
        return 1;
    }
    if (!get_coding_spec(line, &cs, size, tok)) {
        return 0;
    }
    if (!cs) {
        Py_ssize_t i;
        for (i = 0; i < size; i++) {
            if (line[i] == '#' || line[i] == '\n' || line[i] == '\r')
                break;
            if (line[i] != ' ' && line[i] != '\t' && line[i] != '\014') {
                /* Stop checking coding spec after a line containing
                 * anything except a comment. */
                tok->decoding_state = STATE_NORMAL;
                break;
            }
        }
        return 1;
    }
    tok->decoding_state = STATE_NORMAL;
    if (tok->encoding == NULL) {
        assert(tok->decoding_readline == NULL);
        if (strcmp(cs, "utf-8") != 0 && !set_readline(tok, cs)) {
            _PyTokenizer_error_ret(tok);
            PyErr_Format(PyExc_SyntaxError, "encoding problem: %s", cs);
            PyMem_Free(cs);
            return 0;
        }
        tok->encoding = cs;
    } else {                /* then, compare cs with BOM */
        if (strcmp(tok->encoding, cs) != 0) {
            _PyTokenizer_error_ret(tok);
            PyErr_Format(PyExc_SyntaxError,
                         "encoding problem: %s with BOM", cs);
            PyMem_Free(cs);
            return 0;
        }
        PyMem_Free(cs);
    }
    return 1;
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
    for (; expected; expected--)
        if (s[expected] < 0x80 || s[expected] >= 0xC0)
            return 0;
    return length;
}

int
_PyTokenizer_ensure_utf8(char *line, struct tok_state *tok)
{
    int badchar = 0;
    unsigned char *c;
    int length;
    for (c = (unsigned char *)line; *c; c += length) {
        if (!(length = valid_utf8(c))) {
            badchar = *c;
            break;
        }
    }
    if (badchar) {
        PyErr_Format(PyExc_SyntaxError,
                     "Non-UTF-8 code starting with '\\x%.2x' "
                     "in file %U on line %i, "
                     "but no encoding declared; "
                     "see https://peps.python.org/pep-0263/ for details",
                     badchar, tok->filename, tok->lineno);
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
