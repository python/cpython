#include "Python.h"
#include "errcode.h"
#include "../Parser/tokenizer.h"
#include "../Parser/pegen.h"      // _PyPegen_byte_offset_to_character_offset()
#include "../Parser/pegen.h"      // _PyPegen_byte_offset_to_character_offset()

static struct PyModuleDef _tokenizemodule;

typedef struct {
    PyTypeObject *TokenizerIter;
} tokenize_state;

static tokenize_state *
get_tokenize_state(PyObject *module) {
    return (tokenize_state *)PyModule_GetState(module);
}

#define _tokenize_get_state_by_type(type) \
    get_tokenize_state(PyType_GetModuleByDef(type, &_tokenizemodule))

#include "pycore_runtime.h"
#include "clinic/Python-tokenize.c.h"

/*[clinic input]
module _tokenizer
class _tokenizer.tokenizeriter "tokenizeriterobject *" "_tokenize_get_state_by_type(type)->TokenizerIter"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=96d98ee2fef7a8bc]*/

typedef struct
{
    PyObject_HEAD struct tok_state *tok;
    int done;

    /* Needed to cache line for performance */
    PyObject *last_line;
    Py_ssize_t last_lineno;
    Py_ssize_t last_end_lineno;
    Py_ssize_t byte_col_offset_diff;
} tokenizeriterobject;

/*[clinic input]
@classmethod
_tokenizer.tokenizeriter.__new__ as tokenizeriter_new

    readline: object
    /
    *
    extra_tokens: bool
    encoding: str(c_default="NULL") = 'utf-8'
[clinic start generated code]*/

static PyObject *
tokenizeriter_new_impl(PyTypeObject *type, PyObject *readline,
                       int extra_tokens, const char *encoding)
/*[clinic end generated code: output=7501a1211683ce16 input=f7dddf8a613ae8bd]*/
{
    tokenizeriterobject *self = (tokenizeriterobject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    PyObject *filename = PyUnicode_FromString("<string>");
    if (filename == NULL) {
        return NULL;
    }
    self->tok = _PyTokenizer_FromReadline(readline, encoding, 1, 1);
    if (self->tok == NULL) {
        Py_DECREF(filename);
        return NULL;
    }
    self->tok->filename = filename;
    if (extra_tokens) {
        self->tok->tok_extra_tokens = 1;
    }
    self->done = 0;

    self->last_line = NULL;
    self->byte_col_offset_diff = 0;
    self->last_lineno = 0;
    self->last_end_lineno = 0;

    return (PyObject *)self;
}

static int
_tokenizer_error(struct tok_state *tok)
{
    if (PyErr_Occurred()) {
        return -1;
    }

    const char *msg = NULL;
    PyObject* errtype = PyExc_SyntaxError;
    switch (tok->done) {
        case E_TOKEN:
            msg = "invalid token";
            break;
        case E_EOF:
            PyErr_SetString(PyExc_SyntaxError, "unexpected EOF in multi-line statement");
            PyErr_SyntaxLocationObject(tok->filename, tok->lineno,
                                       tok->inp - tok->buf < 0 ? 0 : (int)(tok->inp - tok->buf));
            return -1;
        case E_DEDENT:
            msg = "unindent does not match any outer indentation level";
            errtype = PyExc_IndentationError;
            break;
        case E_INTR:
            if (!PyErr_Occurred()) {
                PyErr_SetNone(PyExc_KeyboardInterrupt);
            }
            return -1;
        case E_NOMEM:
            PyErr_NoMemory();
            return -1;
        case E_TABSPACE:
            errtype = PyExc_TabError;
            msg = "inconsistent use of tabs and spaces in indentation";
            break;
        case E_TOODEEP:
            errtype = PyExc_IndentationError;
            msg = "too many levels of indentation";
            break;
        case E_LINECONT: {
            msg = "unexpected character after line continuation character";
            break;
        }
        default:
            msg = "unknown tokenization error";
    }

    PyObject* errstr = NULL;
    PyObject* error_line = NULL;
    PyObject* tmp = NULL;
    PyObject* value = NULL;
    int result = 0;

    Py_ssize_t size = tok->inp - tok->buf;
    assert(tok->buf[size-1] == '\n');
    size -= 1; // Remove the newline character from the end of the line
    error_line = PyUnicode_DecodeUTF8(tok->buf, size, "replace");
    if (!error_line) {
        result = -1;
        goto exit;
    }

    Py_ssize_t offset = _PyPegen_byte_offset_to_character_offset(error_line, tok->inp - tok->buf);
    if (offset == -1) {
        result = -1;
        goto exit;
    }
    tmp = Py_BuildValue("(OnnOOO)", tok->filename, tok->lineno, offset, error_line, Py_None, Py_None);
    if (!tmp) {
        result = -1;
        goto exit;
    }

    errstr = PyUnicode_FromString(msg);
    if (!errstr) {
        result = -1;
        goto exit;
    }

    value = PyTuple_Pack(2, errstr, tmp);
    if (!value) {
        result = -1;
        goto exit;
    }

    PyErr_SetObject(errtype, value);

exit:
    Py_XDECREF(errstr);
    Py_XDECREF(error_line);
    Py_XDECREF(tmp);
    Py_XDECREF(value);
    return result;
}

static PyObject *
tokenizeriter_next(tokenizeriterobject *it)
{
    PyObject* result = NULL;
    struct token token;
    _PyToken_Init(&token);

    int type = _PyTokenizer_Get(it->tok, &token);
    if (type == ERRORTOKEN) {
        if(!PyErr_Occurred()) {
            _tokenizer_error(it->tok);
            assert(PyErr_Occurred());
        }
        goto exit;
    }
    if (it->done || type == ERRORTOKEN) {
        PyErr_SetString(PyExc_StopIteration, "EOF");
        it->done = 1;
        goto exit;
    }
    PyObject *str = NULL;
    if (token.start == NULL || token.end == NULL) {
        str = PyUnicode_FromString("");
    }
    else {
        str = PyUnicode_FromStringAndSize(token.start, token.end - token.start);
    }
    if (str == NULL) {
        goto exit;
    }

    int is_trailing_token = 0;
    if (type == ENDMARKER || (type == DEDENT && it->tok->done == E_EOF)) {
        is_trailing_token = 1;
    }

    const char *line_start = ISSTRINGLIT(type) ? it->tok->multi_line_start : it->tok->line_start;
    PyObject* line = NULL;
    int line_changed = 1;
    if (it->tok->tok_extra_tokens && is_trailing_token) {
        line = PyUnicode_FromString("");
    } else {
        Py_ssize_t size = it->tok->inp - line_start;
        if (size >= 1 && it->tok->implicit_newline) {
            size -= 1;
        }

        if (it->tok->lineno != it->last_lineno) {
            // Line has changed since last token, so we fetch the new line and cache it
            // in the iter object.
            Py_XDECREF(it->last_line);
            line = PyUnicode_DecodeUTF8(line_start, size, "replace");
            it->last_line = line;
            it->byte_col_offset_diff = 0;
        } else {
            // Line hasn't changed so we reuse the cached one.
            line = it->last_line;
            line_changed = 0;
        }
    }
    if (line == NULL) {
        Py_DECREF(str);
        goto exit;
    }

    Py_ssize_t lineno = ISSTRINGLIT(type) ? it->tok->first_lineno : it->tok->lineno;
    Py_ssize_t end_lineno = it->tok->lineno;
    it->last_lineno = lineno;
    it->last_end_lineno = end_lineno;

    Py_ssize_t col_offset = -1;
    Py_ssize_t end_col_offset = -1;
    Py_ssize_t byte_offset = -1;
    if (token.start != NULL && token.start >= line_start) {
        byte_offset = token.start - line_start;
        if (line_changed) {
            col_offset = _PyPegen_byte_offset_to_character_offset_line(line, 0, byte_offset);
            it->byte_col_offset_diff = byte_offset - col_offset;
        }
        else {
            col_offset = byte_offset - it->byte_col_offset_diff;
        }
    }
    if (token.end != NULL && token.end >= it->tok->line_start) {
        Py_ssize_t end_byte_offset = token.end - it->tok->line_start;
        if (lineno == end_lineno) {
            // If the whole token is at the same line, we can just use the token.start
            // buffer for figuring out the new column offset, since using line is not
            // performant for very long lines.
            Py_ssize_t token_col_offset = _PyPegen_byte_offset_to_character_offset_line(line, byte_offset, end_byte_offset);
            end_col_offset = col_offset + token_col_offset;
            it->byte_col_offset_diff += token.end - token.start - token_col_offset;
        } else {
            end_col_offset = _PyPegen_byte_offset_to_character_offset_raw(it->tok->line_start, end_byte_offset);
            it->byte_col_offset_diff += end_byte_offset - end_col_offset;
        }
    }

    if (it->tok->tok_extra_tokens) {
        if (is_trailing_token) {
            lineno = end_lineno = lineno + 1;
            col_offset = end_col_offset = 0;
        }
        // Necessary adjustments to match the original Python tokenize
        // implementation
        if (type > DEDENT && type < OP) {
            type = OP;
        }
        else if (type == ASYNC || type == AWAIT) {
            type = NAME;
        }
        else if (type == NEWLINE) {
            Py_DECREF(str);
            if (!it->tok->implicit_newline) {
                if (it->tok->start[0] == '\r') {
                    str = PyUnicode_FromString("\r\n");
                } else {
                    str = PyUnicode_FromString("\n");
                }
            }
            end_col_offset++;
        }
        else if (type == NL) {
            if (it->tok->implicit_newline) {
                Py_DECREF(str);
                str = PyUnicode_FromString("");
            }
        }

        if (str == NULL) {
            Py_DECREF(line);
            goto exit;
        }
    }

    result = Py_BuildValue("(iN(nn)(nn)O)", type, str, lineno, col_offset, end_lineno, end_col_offset, line);
exit:
    _PyToken_Free(&token);
    if (type == ENDMARKER) {
        it->done = 1;
    }
    return result;
}

static void
tokenizeriter_dealloc(tokenizeriterobject *it)
{
    PyTypeObject *tp = Py_TYPE(it);
    Py_XDECREF(it->last_line);
    _PyTokenizer_Free(it->tok);
    tp->tp_free(it);
    Py_DECREF(tp);
}

static PyType_Slot tokenizeriter_slots[] = {
    {Py_tp_new, tokenizeriter_new},
    {Py_tp_dealloc, tokenizeriter_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, tokenizeriter_next},
    {0, NULL},
};

static PyType_Spec tokenizeriter_spec = {
    .name = "_tokenize.TokenizerIter",
    .basicsize = sizeof(tokenizeriterobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = tokenizeriter_slots,
};

static int
tokenizemodule_exec(PyObject *m)
{
    tokenize_state *state = get_tokenize_state(m);
    if (state == NULL) {
        return -1;
    }

    state->TokenizerIter = (PyTypeObject *)PyType_FromModuleAndSpec(m, &tokenizeriter_spec, NULL);
    if (state->TokenizerIter == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, state->TokenizerIter) < 0) {
        return -1;
    }

    return 0;
}

static PyMethodDef tokenize_methods[] = {
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static PyModuleDef_Slot tokenizemodule_slots[] = {
    {Py_mod_exec, tokenizemodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static int
tokenizemodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    tokenize_state *state = get_tokenize_state(m);
    Py_VISIT(state->TokenizerIter);
    return 0;
}

static int
tokenizemodule_clear(PyObject *m)
{
    tokenize_state *state = get_tokenize_state(m);
    Py_CLEAR(state->TokenizerIter);
    return 0;
}

static void
tokenizemodule_free(void *m)
{
    tokenizemodule_clear((PyObject *)m);
}

static struct PyModuleDef _tokenizemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_tokenize",
    .m_size = sizeof(tokenize_state),
    .m_slots = tokenizemodule_slots,
    .m_methods = tokenize_methods,
    .m_traverse = tokenizemodule_traverse,
    .m_clear = tokenizemodule_clear,
    .m_free = tokenizemodule_free,
};

PyMODINIT_FUNC
PyInit__tokenize(void)
{
    return PyModuleDef_Init(&_tokenizemodule);
}
