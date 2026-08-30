#include "Python.h"
#include "pycore_token.h"
#include "errcode.h"

#include "state.h"
#include "../tokenizer/reader.h"

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
    for (int i = 0; i <= tok->tok_mode_stack_index; i++) {
        PyMem_Free(tok->tok_mode_stack[i].comments);
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
