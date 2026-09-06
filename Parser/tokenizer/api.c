#include "Python.h"
#include "errcode.h"
#include "pycore_token.h"

#include "tokenizer.h"
#include "reader.h"
#include "reader_internal.h"
#include "../lexer/state.h"

_PyTokenizer_Diagnostic
_PyTokenizer_GetDiagnostic(const struct tok_state *tok)
{
    return tok->diagnostic;
}

_PyTokenizer_Info
_PyTokenizer_GetInfo(const struct tok_state *tok)
{
    _PyTokenizer_Info info = {
        .status = tok->done,
        .location = {tok->lineno, tok->line_start < 0
            ? -1 : (int)(tok->cur - tok->line_start)},
        .cursor = tok->cur,
        .input_span = {tok->buf_offset, tok->inp},
        .line_span = {tok->line_start, tok->inp},
        .level = tok->level,
        .delimiter_loc = {-1, -1},
        .in_formatted_string = tok->ftstring_depth != 0,
        .is_interactive = tok->reader->kind == _PYTOK_READER_INTERACTIVE,
        .is_file = tok->fp != NULL && tok->fp != stdin,
        .filename = tok->filename,
        .module = tok->module,
        .encoding = tok->encoding,
    };
    if (tok->level > 0) {
        int level = tok->level - 1;
        info.delimiter = tok->parenstack[level];
        info.delimiter_loc = (_PyTok_Loc){
            tok->parenlinenostack[level], tok->parencolstack[level]};
    }
    return info;
}

const char *
_PyToken_TextView(const struct tok_state *tok, const struct token *token,
                  Py_ssize_t *length)
{
    assert(length != NULL);
    if (token->span.start < 0) {
        assert(token->span.start == -1 && token->span.end == -1);
        *length = 0;
        return "";
    }
    return _PyLexer_BufferSpanView(tok, token->span, length);
}

const char *
_PyTokenizer_SpanView(const struct tok_state *tok, _PyTok_Span span,
                      Py_ssize_t *length)
{
    return _PyLexer_BufferSpanView(tok, span, length);
}

void
_PyToken_GetView(const struct tok_state *tok, const struct token *token,
                 int type, _PyToken_View *view)
{
    assert(view != NULL);
    assert((token->span.start == -1 && token->span.end == -1) ||
           _PyTok_SpanIsValid(token->span));
    if (token->span.start >= 0) {
        (void)_PyLexer_BufferPointer(tok, token->span.end);
    }
    view->text = token->span.start < 0
        ? NULL : _PyLexer_BufferPointer(tok, token->span.start);
    view->length = token->span.end - token->span.start;
    view->end_line = _PyLexer_BufferPointer(tok, tok->line_start);
    view->line = ISSTRINGLIT(type)
        ? view->text - token->start_loc.byte_col : view->end_line;
    view->line_length = tok->inp - tok->line_start +
        (view->end_line - view->line);
    view->implicit_newline = tok->implicit_newline;
    view->at_eof = tok->done == E_EOF;
}

const char *
_PyTokenizer_LineView(const struct tok_state *tok, Py_ssize_t lineno,
                      Py_ssize_t *length)
{
    return _PyTok_SourceLineView(&tok->source, lineno, length);
}

const char *
_PyTokenizer_RetainedSource(const struct tok_state *tok)
{
    if (tok->reader->kind == _PYTOK_READER_PREPARED) {
        return _PyTok_SourceData(&tok->source);
    }
    if (tok->reader->kind == _PYTOK_READER_INTERACTIVE) {
        return tok->source.bytes;
    }
    return NULL;
}

void
_PyTokenizer_SetContext(struct tok_state *tok, PyObject *filename,
                        PyObject *module)
{
    Py_XINCREF(filename);
    Py_XINCREF(module);
    Py_XSETREF(tok->filename, filename);
    Py_XSETREF(tok->module, module);
}

void
_PyTokenizer_SetOptions(struct tok_state *tok, int extra_tokens,
                        int type_comments)
{
    tok->tok_extra_tokens = extra_tokens;
    tok->type_comments = type_comments;
}

void
_PyTokenizer_ImplyDedents(struct tok_state *tok)
{
    if (tok->indent != 0) {
        tok->pendin = -tok->indent;
        tok->indent = 0;
    }
}

int
_PyTokenizer_HasTrailingStatement(const struct tok_state *tok)
{
    const char *cur = _PyLexer_BufferPointer(tok, tok->cur);
    char c = *cur;
    for (;;) {
        while (c == ' ' || c == '\t' || c == '\n' || c == '\014') {
            c = *++cur;
        }
        if (!c) {
            return 0;
        }
        if (c != '#') {
            return 1;
        }
        while (c && c != '\n') {
            c = *++cur;
        }
    }
}

int
_PyTokenizer_IsInteractive(const struct tok_state *tok)
{
    return _PyTok_ReaderIsInteractive(tok);
}

void
_PyTokenizer_StopInteractive(struct tok_state *tok)
{
    _PyTok_ReaderStopInteractive(tok);
}
