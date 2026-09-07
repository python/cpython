#include "Python.h"
#include "pycore_pystate.h"
#include "pycore_token.h"
#include "errcode.h"

#include "state.h"
#include "../tokenizer/reader.h"

/* Never change this */
#define TABSIZE 8

/* Create and initialize a new tok_state structure */
struct tok_state *
_PyTokenizer_tok_new(void)
{
    struct tok_state *tok = (struct tok_state *)PyMem_Calloc(
                                            1,
                                            sizeof(struct tok_state));
    if (tok == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    tok->buf = tok->cur = tok->inp = NULL;
    tok->fp_interactive = 0;
    tok->interactive_src_start = NULL;
    tok->interactive_src_end = NULL;
    tok->start = NULL;
    tok->done = E_OK;
    tok->fp = NULL;
    tok->tabsize = TABSIZE;
    tok->indent = 0;
    tok->indstack[0] = 0;
    tok->atbol = 1;
    tok->pendin = 0;
    tok->prompt = NULL;
    tok->lineno = 0;
    tok->starting_col_offset = -1;
    tok->col_offset = -1;
    tok->level = 0;
    tok->altindstack[0] = 0;
    tok->input_error = 0;
    tok->encoding = NULL;
    tok->filename = NULL;
    tok->module = NULL;
    tok->type_comments = 0;
    tok->interactive_underflow = IUNDERFLOW_NORMAL;
    tok->str = NULL;
    tok->report_warnings = 1;
    tok->tok_extra_tokens = 0;
    tok->comment_newline = 0;
    tok->implicit_newline = 0;
    _PyTok_SourceInit(&tok->source);
    tok->reader = NULL;
    tok->tok_mode_stack[0] = (tokenizer_mode){.kind =TOK_REGULAR_MODE, .quote='\0', .quote_size = 0, .in_debug=0};
    tok->tok_mode_stack_index = 0;
#ifdef Py_DEBUG
    tok->debug = _Py_GetConfig()->parser_debug;
#endif
    return tok;
}

static void
free_fstring_expressions(struct tok_state *tok)
{
    int index;
    tokenizer_mode *mode;

    for (index = tok->tok_mode_stack_index; index >= 0; --index) {
        mode = &(tok->tok_mode_stack[index]);
        if (mode->last_expr_buffer != NULL) {
            PyMem_Free(mode->last_expr_buffer);
            mode->last_expr_buffer = NULL;
            mode->last_expr_size = 0;
            mode->last_expr_end = -1;
            mode->in_format_spec = 0;
        }
    }
}

/* Free a tok_state structure */
void
_PyTokenizer_Free(struct tok_state *tok)
{
    if (tok->encoding != NULL) {
        PyMem_Free(tok->encoding);
    }
    Py_XDECREF(tok->filename);
    Py_XDECREF(tok->module);
    _PyTok_ReaderFree(tok);
    _PyTok_SourceClear(&tok->source);
    free_fstring_expressions(tok);
    PyMem_Free(tok);
}

void
_PyToken_Free(struct token *token) {
    Py_XDECREF(token->metadata);
}

void
_PyToken_Init(struct token *token) {
#ifdef Py_DEBUG
    token->span = (_PyTok_Span){-1, -1};
    token->start_loc = (_PyTok_Loc){-1, -1};
    token->end_loc = (_PyTok_Loc){-1, -1};
#endif
    token->metadata = NULL;
}

static inline _PyTok_Span
buffer_span(const struct tok_state *tok, const char *start, const char *end)
{
    if (start == NULL) {
        assert(end == NULL);
        return (_PyTok_Span){-1, -1};
    }
    assert(end != NULL);
    const char *base = tok->buf;
    assert(base != NULL);
    assert(tok->inp >= base);
    Py_ssize_t start_offset = start - base;
    Py_ssize_t end_offset = end - base;
    assert(start_offset >= 0 && start_offset <= end_offset);
    assert(end_offset <= tok->inp - base);
    assert(tok->buf_offset <= PY_SSIZE_T_MAX - end_offset);
    return _PyTok_SpanFromBounds(
        tok->buf_offset + start_offset, tok->buf_offset + end_offset);
}

int
_PyLexer_token_setup(struct tok_state *tok, struct token *token, int type, const char *start, const char *end)
{
    token->level = tok->level;
    token->span = buffer_span(tok, start, end);
    int lineno = ISSTRINGLIT(type) ? tok->first_lineno : tok->lineno;
    token->start_loc = (_PyTok_Loc){lineno, -1};
    token->end_loc = (_PyTok_Loc){tok->lineno, -1};

    if (start != NULL && end != NULL) {
        token->start_loc.byte_col = tok->starting_col_offset;
        token->end_loc.byte_col = tok->col_offset;
    }
    return type;
}
