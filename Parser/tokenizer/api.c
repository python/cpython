#include "Python.h"
#include "errcode.h"
#include "pycore_token.h"

#include "tokenizer.h"
#include "reader.h"
#include "reader_internal.h"
#include "../lexer/state.h"

_PyTokenizer_Info
_PyTokenizer_GetInfo(const struct tok_state *tok)
{
    _PyTokenizer_Info info = {
        .status = tok->done,
        .diagnostic = tok->diagnostic,
        .location = {tok->lineno, tok->line_start < 0
            ? -1 : (int)(tok->cur - tok->line_start)},
        .cursor = tok->cur,
        .input_span = {tok->buf_offset, tok->inp},
        .line_span = {tok->line_start, tok->inp},
        .level = tok->level,
        .delimiter_loc = {-1, -1},
        .in_formatted_string = tok->ftstring_depth != 0,
        .is_interactive = _PyTok_ReaderIsInteractive(tok),
        .is_file = tok->reader->fp != NULL && tok->reader->fp != stdin,
        .filename = tok->filename,
        .module = tok->module,
        .encoding = tok->reader->encoding,
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
    return _PyTok_SourceSpanView(&tok->source, token->span, length);
}

const char *
_PyTokenizer_SpanView(const struct tok_state *tok, _PyTok_Span span,
                      Py_ssize_t *length)
{
    return _PyTok_SourceSpanView(&tok->source, span, length);
}

void
_PyToken_GetView(const struct tok_state *tok, const struct token *token,
                 _PyToken_View *view)
{
    assert(view != NULL);
    view->text = _PyToken_TextView(tok, token, &view->length);
    view->end_line_start = tok->line_start;
    view->line_span = (_PyTok_Span){
        ISSTRINGLIT(token->type)
            ? token->span.start - token->start_loc.byte_col : tok->line_start,
        tok->inp,
    };
    view->line = _PyTok_SourcePointer(&tok->source, view->line_span.start);
    view->implicit_newline = tok->implicit_newline;
    view->at_eof = tok->done == E_EOF;
}

const char *
_PyTokenizer_LineView(const struct tok_state *tok, Py_ssize_t lineno,
                      Py_ssize_t *length)
{
    return _PyTok_SourceLineView(&tok->source, lineno, length);
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
    _PyLexer_ImplyDedents(tok);
}

int
_PyTokenizer_HasTrailingStatement(const struct tok_state *tok)
{
    _PyTok_Off cur = tok->cur;
    _PyTok_Off end = tok->source.base_offset + tok->source.len;
    while (cur < end) {
        int c = _PyTok_SourceByte(&tok->source, cur++);
        if (c == '\0') {
            return 0;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\014') {
            continue;
        }
        if (c != '#') {
            return 1;
        }
        while (cur < end) {
            c = _PyTok_SourceByte(&tok->source, cur);
            if (c == '\0' || c == '\n') {
                break;
            }
            cur++;
        }
    }
    return 0;
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
