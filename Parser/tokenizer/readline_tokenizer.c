#include "Python.h"
#include "errcode.h"

#include "helpers.h"
#include "../lexer/lexer.h"
#include "../lexer/state.h"
#include "../lexer/buffer.h"

static int
tok_readline_string(struct tok_state* tok) {
    PyObject* line = NULL;
    PyObject* raw_line = PyObject_CallNoArgs(tok->readline);
    if (raw_line == NULL) {
        if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
            PyErr_Clear();
            return 1;
        }
        _PyTokenizer_error_ret(tok);
        goto error;
    }
    if(tok->encoding != NULL) {
        if (!PyBytes_Check(raw_line)) {
            PyErr_Format(PyExc_TypeError, "readline() returned a non-bytes object");
            _PyTokenizer_error_ret(tok);
            goto error;
        }
        line = PyUnicode_Decode(PyBytes_AS_STRING(raw_line), PyBytes_GET_SIZE(raw_line),
                                tok->encoding, "replace");
        Py_CLEAR(raw_line);
        if (line == NULL) {
            _PyTokenizer_error_ret(tok);
            goto error;
        }
    } else {
        if(!PyUnicode_Check(raw_line)) {
            PyErr_Format(PyExc_TypeError, "readline() returned a non-string object");
            _PyTokenizer_error_ret(tok);
            goto error;
        }
        line = raw_line;
        raw_line = NULL;
    }
    Py_ssize_t buflen;
    const char* buf = PyUnicode_AsUTF8AndSize(line, &buflen);
    if (buf == NULL) {
        _PyTokenizer_error_ret(tok);
        goto error;
    }

    // Make room for the null terminator *and* potentially
    // an extra newline character that we may need to artificially
    // add.
    size_t buffer_size = buflen + 2;
    if (!_PyLexer_tok_reserve_buf(tok, buffer_size)) {
        goto error;
    }
    memcpy(tok->inp, buf, buflen);
    tok->inp += buflen;
    *tok->inp = '\0';

    tok->line_start = tok->cur;
    Py_DECREF(line);
    return 1;
error:
    Py_XDECREF(raw_line);
    Py_XDECREF(line);
    return 0;
}

static int
tok_underflow_readline(struct tok_state* tok) {
    assert(tok->decoding_state == STATE_NORMAL);
    assert(tok->fp == NULL && tok->input == NULL && tok->decoding_readline == NULL);
    if (tok->start == NULL && !INSIDE_FSTRING(tok)) {
        tok->cur = tok->inp = tok->buf;
    }
    if (!tok_readline_string(tok)) {
        return 0;
    }
    if (tok->inp == tok->cur) {
        tok->done = E_EOF;
        return 0;
    }
    tok->implicit_newline = 0;
    if (tok->inp[-1] != '\n') {
        assert(tok->inp + 1 < tok->end);
        /* Last line does not end in \n, fake one */
        *tok->inp++ = '\n';
        *tok->inp = '\0';
        tok->implicit_newline = 1;
    }

    if (tok->tok_mode_stack_index && !_PyLexer_update_fstring_expr(tok, 0)) {
        return 0;
    }

    ADVANCE_LINENO();
    /* The default encoding is UTF-8, so make sure we don't have any
       non-UTF-8 sequences in it. */
    if (!tok->encoding && !_PyTokenizer_ensure_utf8(tok->cur, tok)) {
        _PyTokenizer_error_ret(tok);
        return 0;
    }
    assert(tok->done == E_OK);
    return tok->done == E_OK;
}

struct tok_state *
_PyTokenizer_FromReadline(PyObject* readline, const char* enc,
                          int exec_input, int preserve_crlf)
{
    struct tok_state *tok = _PyTokenizer_tok_new();
    if (tok == NULL)
        return NULL;
    if ((tok->buf = (char *)PyMem_Malloc(BUFSIZ)) == NULL) {
        _PyTokenizer_Free(tok);
        return NULL;
    }
    tok->cur = tok->inp = tok->buf;
    tok->end = tok->buf + BUFSIZ;
    tok->fp = NULL;
    if (enc != NULL) {
        tok->encoding = _PyTokenizer_new_string(enc, strlen(enc), tok);
        if (!tok->encoding) {
            _PyTokenizer_Free(tok);
            return NULL;
        }
    }
    tok->decoding_state = STATE_NORMAL;
    tok->underflow = &tok_underflow_readline;
    Py_INCREF(readline);
    tok->readline = readline;
    return tok;
}
