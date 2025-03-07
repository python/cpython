#include "Python.h"
#include "pycore_pystate.h"
#include "pycore_token.h"
#include "errcode.h"

#include "state.h"

/* Never change this */
#define TABSIZE 8

/* Create and initialize a new tok_state structure */
struct tok_state *
_PyTokenizer_tok_new(void)
{
    struct tok_state *tok = (struct tok_state *)PyMem_Calloc(
                                            1,
                                            sizeof(struct tok_state));
    if (tok == NULL)
        return NULL;
    tok->buf = tok->cur = tok->inp = NULL;
    tok->fp_interactive = 0;
    tok->interactive_src_start = NULL;
    tok->interactive_src_end = NULL;
    tok->start = NULL;
    tok->end = NULL;
    tok->done = E_OK;
    tok->fp = NULL;
    tok->input = NULL;
    tok->tabsize = TABSIZE;
    tok->indent = 0;
    tok->indstack[0] = 0;
    tok->atbol = 1;
    tok->pendin = 0;
    tok->prompt = tok->nextprompt = NULL;
    tok->lineno = 0;
    tok->starting_col_offset = -1;
    tok->col_offset = -1;
    tok->level = 0;
    tok->altindstack[0] = 0;
    tok->decoding_state = STATE_INIT;
    tok->decoding_erred = 0;
    tok->enc = NULL;
    tok->encoding = NULL;
    tok->cont_line = 0;
    tok->filename = NULL;
    tok->decoding_readline = NULL;
    tok->decoding_buffer = NULL;
    tok->readline = NULL;
    tok->type_comments = 0;
    tok->interactive_underflow = IUNDERFLOW_NORMAL;
    tok->underflow = NULL;
    tok->str = NULL;
    tok->report_warnings = 1;
    tok->tok_extra_tokens = 0;
    tok->comment_newline = 0;
    tok->implicit_newline = 0;
    tok->tok_mode_stack[0] = (tokenizer_mode){.kind =TOK_REGULAR_MODE, .f_string_quote='\0', .f_string_quote_size = 0, .f_string_debug=0};
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
    Py_XDECREF(tok->decoding_readline);
    Py_XDECREF(tok->decoding_buffer);
    Py_XDECREF(tok->readline);
    Py_XDECREF(tok->filename);
    if ((tok->readline != NULL || tok->fp != NULL ) && tok->buf != NULL) {
        PyMem_Free(tok->buf);
    }
    if (tok->input) {
        PyMem_Free(tok->input);
    }
    if (tok->interactive_src_start != NULL) {
        PyMem_Free(tok->interactive_src_start);
    }
    free_fstring_expressions(tok);
    PyMem_Free(tok);
}

void
_PyToken_Free(struct token *token) {
    Py_XDECREF(token->metadata);
}

void
_PyToken_Init(struct token *token) {
    token->metadata = NULL;
}

int
_PyLexer_type_comment_token_setup(struct tok_state *tok, struct token *token, int type, int col_offset,
                         int end_col_offset, const char *start, const char *end)
{
    token->level = tok->level;
    token->lineno = token->end_lineno = tok->lineno;
    token->col_offset = col_offset;
    token->end_col_offset = end_col_offset;
    token->start = start;
    token->end = end;
    return type;
}

int
_PyLexer_token_setup(struct tok_state *tok, struct token *token, int type, const char *start, const char *end)
{
    assert((start == NULL && end == NULL) || (start != NULL && end != NULL));
    token->level = tok->level;
    if (ISSTRINGLIT(type)) {
        token->lineno = tok->first_lineno;
    }
    else {
        token->lineno = tok->lineno;
    }
    token->end_lineno = tok->lineno;
    token->col_offset = token->end_col_offset = -1;
    token->start = start;
    token->end = end;

    if (start != NULL && end != NULL) {
        token->col_offset = tok->starting_col_offset;
        token->end_col_offset = tok->col_offset;
    }
    return type;
}
