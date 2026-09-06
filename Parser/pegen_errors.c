#include <Python.h>
#include <errcode.h>

#include "pycore_pyerrors.h"      // _PyErr_ProgramDecodedTextObject()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_tuple.h"         // _PyTuple_FromPair
#include "pegen.h"
#include "tokenizer/tokenizer.h"

// TOKENIZER ERRORS

static inline void
raise_unclosed_parentheses_error(Parser *p) {
       _PyTokenizer_Info info = _PyTokenizer_GetInfo(p->tok);
       int error_lineno = info.delimiter_loc.lineno;
       int error_col = info.delimiter_loc.byte_col;
       RAISE_ERROR_KNOWN_LOCATION(p, PyExc_SyntaxError,
                                  error_lineno, error_col, error_lineno, -1,
                                  "'%c' was never closed",
                                  info.delimiter);
}

int
_Pypegen_tokenizer_error(Parser *p)
{
    if (PyErr_Occurred()) {
        return -1;
    }

    const char *msg = NULL;
    PyObject* errtype = PyExc_SyntaxError;
    Py_ssize_t col_offset = -1;
    p->error_indicator = 1;
    _PyTokenizer_Info info = _PyTokenizer_GetInfo(p->tok);
    switch (info.status) {
        case E_TOKEN:
            msg = "invalid token";
            break;
        case E_EOF:
            if (info.level) {
                raise_unclosed_parentheses_error(p);
            } else {
                RAISE_SYNTAX_ERROR("unexpected EOF while parsing");
            }
            return -1;
        case E_DEDENT:
            RAISE_INDENTATION_ERROR("unindent does not match any outer indentation level");
            return -1;
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
            col_offset = info.cursor - info.line_span.start - 1;
            msg = "unexpected character after line continuation character";
            break;
        }
        case E_COLUMNOVERFLOW:
            PyErr_SetString(PyExc_OverflowError,
                    "Parser column offset overflow - source line is too big");
            return -1;
        default:
            msg = "unknown parsing error";
    }

    RAISE_ERROR_KNOWN_LOCATION(p, errtype, info.location.lineno,
                               col_offset >= 0 ? col_offset : 0,
                               info.location.lineno, -1, msg);
    return -1;
}

int
_Pypegen_raise_decode_error(Parser *p)
{
    assert(PyErr_Occurred());
    const char *errtype = NULL;
    if (PyErr_ExceptionMatches(PyExc_UnicodeError)) {
        errtype = "unicode error";
    }
    else if (PyErr_ExceptionMatches(PyExc_ValueError)) {
        errtype = "value error";
    }
    if (errtype) {
        PyObject *type;
        PyObject *value;
        PyObject *tback;
        PyObject *errstr;
        PyErr_Fetch(&type, &value, &tback);
        errstr = PyObject_Str(value);
        if (errstr) {
            RAISE_SYNTAX_ERROR("(%s) %U", errtype, errstr);
            Py_DECREF(errstr);
        }
        else {
            PyErr_Clear();
            RAISE_SYNTAX_ERROR("(%s) unknown error", errtype);
        }
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(tback);
    }

    return -1;
}

static int
_PyPegen_tokenize_full_source_to_check_for_errors(Parser *p) {
    // Tokenize the whole input to see if there are any tokenization
    // errors such as mismatching parentheses. These will get priority
    // over generic syntax errors only if the line number of the error is
    // before the one that we had for the generic error.

    // We don't want to tokenize to the end for interactive input
    if (_PyTokenizer_IsInteractive(p->tok)) {
        return 0;
    }

    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);

    Token *current_token = p->known_err_token != NULL ? p->known_err_token : p->tokens[p->fill - 1];
    Py_ssize_t current_err_line = current_token->lineno;

    int ret = 0;
    struct token new_token;
    _PyToken_Init(&new_token);

    for (;;) {
        switch (_PyTokenizer_Get(p->tok, &new_token)) {
            case ERRORTOKEN: {
                if (PyErr_Occurred()) {
                    ret = -1;
                    goto exit;
                }
                _PyTokenizer_Info info = _PyTokenizer_GetInfo(p->tok);
                if (info.level != 0) {
                    int error_lineno = info.delimiter_loc.lineno;
                    if (current_err_line > error_lineno) {
                        raise_unclosed_parentheses_error(p);
                        ret = -1;
                        goto exit;
                    }
                }
                break;
            }
            case ENDMARKER:
                break;
            default:
                continue;
        }
        break;
    }


exit:
    _PyToken_Free(&new_token);
    _PyTokenizer_Info info = _PyTokenizer_GetInfo(p->tok);
    // Preserve expression errors over later formatted-string errors.
    if (PyErr_Occurred() && !info.in_formatted_string) {
        Py_XDECREF(value);
        Py_XDECREF(type);
        Py_XDECREF(traceback);
    } else {
        PyErr_Restore(type, value, traceback);
    }
    return ret;
}

// PARSER ERRORS

void *
_PyPegen_raise_error(Parser *p, PyObject *errtype, int use_mark, const char *errmsg, ...)
{
    // Bail out if we already have an error set.
    if (p->error_indicator && PyErr_Occurred()) {
        return NULL;
    }
    if (p->fill == 0) {
        va_list va;
        va_start(va, errmsg);
        _PyPegen_raise_error_known_location(p, errtype, 0, 0, 0, -1, errmsg, va);
        va_end(va);
        return NULL;
    }
    if (use_mark && p->mark == p->fill && _PyPegen_fill_token(p) < 0) {
        p->error_indicator = 1;
        return NULL;
    }
    Token *t = p->known_err_token != NULL
                   ? p->known_err_token
                   : p->tokens[use_mark ? p->mark : p->fill - 1];
    Py_ssize_t col_offset;
    Py_ssize_t end_col_offset = -1;
    if (t->col_offset == -1) {
        _PyTokenizer_Diagnostic diagnostic = _PyTokenizer_GetDiagnostic(p->tok);
        _PyTokenizer_Info info = _PyTokenizer_GetInfo(p->tok);
        if (diagnostic.location.lineno != 0) {
            col_offset = diagnostic.location.byte_col;
        } else if (info.cursor == info.input_span.start) {
            col_offset = 0;
        } else {
            col_offset = Py_SAFE_DOWNCAST(
                info.cursor - info.line_span.start, intptr_t, int);
        }
    } else {
        col_offset = t->col_offset + 1;
    }

    if (t->end_col_offset != -1) {
        end_col_offset = t->end_col_offset + 1;
    }

    va_list va;
    va_start(va, errmsg);
    _PyPegen_raise_error_known_location(p, errtype, t->lineno, col_offset, t->end_lineno, end_col_offset, errmsg, va);
    va_end(va);

    return NULL;
}

static PyObject *
get_error_line_from_source(Parser *p, Py_ssize_t lineno)
{
    if (_PyTokenizer_RetainedSource(p->tok) == NULL) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_STR);
    }
    Py_ssize_t relative_lineno = p->starting_lineno ? lineno - p->starting_lineno + 1 : lineno;
    Py_ssize_t len;
    const char *line = _PyTokenizer_LineView(p->tok, relative_lineno, &len);
    return PyUnicode_DecodeUTF8(line, len, "replace");
}

void *
_PyPegen_raise_error_known_location(Parser *p, PyObject *errtype,
                                    Py_ssize_t lineno, Py_ssize_t col_offset,
                                    Py_ssize_t end_lineno, Py_ssize_t end_col_offset,
                                    const char *errmsg, va_list va)
{
    // Bail out if we already have an error set.
    if (p->error_indicator && PyErr_Occurred()) {
        return NULL;
    }
    PyObject *value = NULL;
    PyObject *errstr = NULL;
    PyObject *error_line = NULL;
    PyObject *tmp = NULL;
    p->error_indicator = 1;
    _PyTokenizer_Info info = _PyTokenizer_GetInfo(p->tok);
    _PyTokenizer_Diagnostic diagnostic = _PyTokenizer_GetDiagnostic(p->tok);
    _PyTok_Loc location = diagnostic.location.lineno != 0
        ? diagnostic.location : info.location;
    _PyTok_Span text_span = diagnostic.location.lineno != 0
        ? diagnostic.text_span : info.line_span;

    if (end_lineno == CURRENT_POS) {
        end_lineno = location.lineno;
    }
    if (end_col_offset == CURRENT_POS) {
        end_col_offset = location.byte_col;
    }

    errstr = PyUnicode_FromFormatV(errmsg, va);
    if (!errstr) {
        goto error;
    }

    if (info.is_interactive && _PyTokenizer_RetainedSource(p->tok) != NULL) {
        error_line = get_error_line_from_source(p, lineno);
    }
    else if (p->start_rule == Py_file_input) {
        error_line = _PyErr_ProgramDecodedTextObject(info.filename,
                                                     (int) lineno, info.encoding);
    }

    if (!error_line) {
        /* PyErr_ProgramTextObject was not called or returned NULL. If it was not called,
           then we need to find the error line from some other source, because
           p->start_rule != Py_file_input. If it returned NULL, then it either unexpectedly
           failed or we're parsing from a string or the REPL. There's a third edge case where
           we're actually parsing from a file, which has an E_EOF SyntaxError and in that case
           `PyErr_ProgramTextObject` fails because lineno points to last_file_line + 1, which
           does not physically exist */
        assert(!info.is_file || info.status == E_EOF);

        if (location.lineno <= lineno &&
                info.input_span.end > info.input_span.start) {
            Py_ssize_t size;
            const char *line = _PyTokenizer_SpanView(
                p->tok, text_span, &size);
            error_line = PyUnicode_DecodeUTF8(line, size, "replace");
        }
        else if (!info.is_file) {
            error_line = get_error_line_from_source(p, lineno);
        }
        else {
            error_line = Py_GetConstant(Py_CONSTANT_EMPTY_STR);
        }
        if (!error_line) {
            goto error;
        }
    }

    Py_ssize_t col_number = col_offset;
    Py_ssize_t end_col_number = end_col_offset;

    col_number = _PyPegen_byte_offset_to_character_offset(error_line, col_offset);
    if (col_number < 0) {
        goto error;
    }

    if (end_col_offset > 0) {
        end_col_number = _PyPegen_byte_offset_to_character_offset(error_line, end_col_offset);
        if (end_col_number < 0) {
            goto error;
        }
    }

    tmp = Py_BuildValue("(OnnNnn)", info.filename, lineno, col_number, error_line, end_lineno, end_col_number);
    if (!tmp) {
        goto error;
    }
    value = _PyTuple_FromPair(errstr, tmp);
    Py_DECREF(tmp);
    if (!value) {
        goto error;
    }
    PyErr_SetObject(errtype, value);

    Py_DECREF(errstr);
    Py_DECREF(value);
    return NULL;

error:
    Py_XDECREF(errstr);
    Py_XDECREF(error_line);
    return NULL;
}

void
_Pypegen_set_syntax_error(Parser* p, Token* last_token) {
    _PyTokenizer_Info info = _PyTokenizer_GetInfo(p->tok);
    // Existing syntax error
    if (PyErr_Occurred()) {
        // Prioritize tokenizer errors to custom syntax errors raised
        // on the second phase only if the errors come from the parser.
        int is_tok_ok = (info.status == E_DONE || info.status == E_OK);
        if (is_tok_ok && PyErr_ExceptionMatches(PyExc_SyntaxError)) {
            _PyPegen_tokenize_full_source_to_check_for_errors(p);
        }
        // Propagate the existing syntax error.
        return;
    }
    // Initialization error
    if (p->fill == 0) {
        RAISE_SYNTAX_ERROR("error at start before reading any input");
    }
    // Parser encountered EOF (End of File) unexpectedtly
    if (last_token->type == ERRORTOKEN && info.status == E_EOF) {
        if (info.level) {
            raise_unclosed_parentheses_error(p);
        } else {
            RAISE_SYNTAX_ERROR("unexpected EOF while parsing");
        }
        return;
    }
    // Indentation error in the tokenizer
    if (last_token->type == INDENT || last_token->type == DEDENT) {
        RAISE_INDENTATION_ERROR(last_token->type == INDENT ? "unexpected indent" : "unexpected unindent");
        return;
    }
    // Unknown error (generic case)

    // Use the last token we found on the first pass to avoid reporting
    // incorrect locations for generic syntax errors just because we reached
    // further away when trying to find specific syntax errors in the second
    // pass.
    RAISE_SYNTAX_ERROR_KNOWN_LOCATION(last_token, "invalid syntax");
    // _PyPegen_tokenize_full_source_to_check_for_errors will override the existing
    // generic SyntaxError we just raised if errors are found.
    _PyPegen_tokenize_full_source_to_check_for_errors(p);
}

void
_Pypegen_stack_overflow(Parser *p)
{
    p->error_indicator = 1;
    PyErr_SetString(PyExc_MemoryError,
        "Parser stack overflowed - Python source too complex to parse");
}
