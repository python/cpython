#include "Python.h"
#include "errcode.h"

#include "helpers.h"
#include "../lexer/state.h"

static int
tok_underflow_string(struct tok_state *tok) {
    char *end = strchr(tok->inp, '\n');
    if (end != NULL) {
        end++;
    }
    else {
        end = strchr(tok->inp, '\0');
        if (end == tok->inp) {
            tok->done = E_EOF;
            return 0;
        }
    }
    if (tok->start == NULL) {
        tok->buf = tok->cur;
    }
    tok->line_start = tok->cur;
    ADVANCE_LINENO();
    tok->inp = end;
    return 1;
}

/* Fetch a byte from TOK, using the string buffer. */
static int
buf_getc(struct tok_state *tok) {
    return Py_CHARMASK(*tok->str++);
}

/* Unfetch a byte from TOK, using the string buffer. */
static void
buf_ungetc(int c, struct tok_state *tok) {
    tok->str--;
    assert(Py_CHARMASK(*tok->str) == c);        /* tok->cur may point to read-only segment */
}

/* Set the readline function for TOK to ENC. For the string-based
   tokenizer, this means to just record the encoding. */
static int
buf_setreadl(struct tok_state *tok, const char* enc) {
    tok->enc = enc;
    return 1;
}

/* Decode a byte string STR for use as the buffer of TOK.
   Look for encoding declarations inside STR, and record them
   inside TOK.  */
static char *
decode_str(const char *input, int single, struct tok_state *tok, int preserve_crlf)
{
    PyObject* utf8 = NULL;
    char *str;
    const char *s;
    const char *newl[2] = {NULL, NULL};
    int lineno = 0;
    tok->input = str = _PyTokenizer_translate_newlines(input, single, preserve_crlf, tok);
    if (str == NULL)
        return NULL;
    tok->enc = NULL;
    tok->str = str;
    if (!_PyTokenizer_check_bom(buf_getc, buf_ungetc, buf_setreadl, tok))
        return _PyTokenizer_error_ret(tok);
    str = tok->str;             /* string after BOM if any */
    assert(str);
    if (tok->enc != NULL) {
        utf8 = _PyTokenizer_translate_into_utf8(str, tok->enc);
        if (utf8 == NULL)
            return _PyTokenizer_error_ret(tok);
        str = PyBytes_AsString(utf8);
    }
    for (s = str;; s++) {
        if (*s == '\0') break;
        else if (*s == '\n') {
            assert(lineno < 2);
            newl[lineno] = s;
            lineno++;
            if (lineno == 2) break;
        }
    }
    tok->enc = NULL;
    /* need to check line 1 and 2 separately since check_coding_spec
       assumes a single line as input */
    if (newl[0]) {
        tok->lineno = 1;
        if (!_PyTokenizer_check_coding_spec(str, newl[0] - str, tok, buf_setreadl)) {
            return NULL;
        }
        if (tok->enc == NULL && tok->decoding_state != STATE_NORMAL && newl[1]) {
            tok->lineno = 2;
            if (!_PyTokenizer_check_coding_spec(newl[0]+1, newl[1] - newl[0],
                                   tok, buf_setreadl))
                return NULL;
        }
    }
    tok->lineno = 0;
    if (tok->enc != NULL) {
        assert(utf8 == NULL);
        utf8 = _PyTokenizer_translate_into_utf8(str, tok->enc);
        if (utf8 == NULL)
            return _PyTokenizer_error_ret(tok);
        str = PyBytes_AS_STRING(utf8);
    }
    else if (!_PyTokenizer_ensure_utf8(str, tok, 1)) {
        return _PyTokenizer_error_ret(tok);
    }
    assert(tok->decoding_buffer == NULL);
    tok->decoding_buffer = utf8; /* CAUTION */
    return str;
}

/* Set up tokenizer for string */
struct tok_state *
_PyTokenizer_FromString(const char *str, int exec_input, int preserve_crlf)
{
    struct tok_state *tok = _PyTokenizer_tok_new();
    char *decoded;

    if (tok == NULL)
        return NULL;
    decoded = decode_str(str, exec_input, tok, preserve_crlf);
    if (decoded == NULL) {
        _PyTokenizer_Free(tok);
        return NULL;
    }

    tok->buf = tok->cur = tok->inp = decoded;
    tok->end = decoded;
    tok->underflow = &tok_underflow_string;
    return tok;
}
