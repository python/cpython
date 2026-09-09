#include "Python.h"
#include "pycore_pystate.h"
#include "pycore_token.h"
#include "errcode.h"

#include "state.h"
#include "../tokenizer/helpers.h"
#include "../tokenizer/reader.h"

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
    tok->indent = 0;
    tok->indstack[0] = 0;
    tok->atbol = 1;
    tok->pendin = 0;
    tok->prompt = NULL;
    tok->lineno = 0;
    tok->start_loc = (_PyTok_Loc){-1, -1};
    tok->level = 0;
    tok->altindstack[0] = 0;
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
    tok->ftstring_stack = tok->ftstring_stack_inline;
    tok->ftstring_capacity = FTSTRING_STACK_INLINE_CAPACITY;
#ifdef Py_DEBUG
    tok->debug = _Py_GetConfig()->parser_debug;
#endif
    return tok;
}

ftstring_state *
_PyLexer_PushFTString(struct tok_state *tok)
{
    assert(tok->ftstring_depth >= 0 && tok->ftstring_depth <= tok->ftstring_capacity);
    int next_depth = tok->ftstring_depth + 1;
    if (next_depth >= MAXFTSTRINGLEVEL) {
        _PyTokenizer_syntaxerror(
            tok, "too many nested f-strings or t-strings");
        return NULL;
    }
    if (tok->ftstring_depth == tok->ftstring_capacity) {
        int capacity = Py_MIN(Py_MAX(tok->ftstring_capacity * 2, 4),
                              MAXFTSTRINGLEVEL);
        size_t size = (size_t)capacity * sizeof(*tok->ftstring_stack);
        ftstring_state *stack;
        if (tok->ftstring_stack == tok->ftstring_stack_inline) {
            stack = PyMem_Malloc(size);
            if (stack != NULL) {
                memcpy(stack, tok->ftstring_stack,
                       (size_t)tok->ftstring_depth * sizeof(*stack));
            }
        }
        else {
            stack = PyMem_Realloc(tok->ftstring_stack, size);
        }
        if (stack == NULL) {
            PyErr_NoMemory();
            tok->done = E_NOMEM;
            return NULL;
        }
        tok->ftstring_stack = stack;
        tok->ftstring_capacity = capacity;
    }
    ftstring_state *state = &tok->ftstring_stack[tok->ftstring_depth];
    tok->ftstring_depth = next_depth;
    *state = (ftstring_state){0};
    return state;
}

void
_PyLexer_PopFTString(struct tok_state *tok)
{
    ftstring_state *state = _PyLexer_CurrentFTString(tok);
    assert(state != NULL);
    PyMem_Free(state->comments);
    tok->ftstring_depth--;
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
    for (int i = 0; i < tok->ftstring_depth; i++) {
        PyMem_Free(tok->ftstring_stack[i].comments);
    }
    if (tok->ftstring_stack != tok->ftstring_stack_inline) {
        PyMem_Free(tok->ftstring_stack);
    }
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

int
_PyLexer_token_setup(struct tok_state *tok, struct token *token, int type, const char *start, const char *end)
{
    token->level = tok->level;
    token->span = _PyLexer_BufferSpan(tok, start, end);
    if (start != NULL && end != NULL) {
        token->start_loc = tok->start_loc;
        token->end_loc = (_PyTok_Loc){tok->lineno, _PyLexer_ByteColumn(tok)};
    }
    else {
        token->start_loc = token->end_loc = (_PyTok_Loc){tok->lineno, -1};
    }
    return type;
}
