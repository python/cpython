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

/* Set up tokenizer for UTF-8 string */
struct tok_state *
_PyTokenizer_FromUTF8(const char *str, int exec_input, int preserve_crlf)
{
    struct tok_state *tok = _PyTokenizer_tok_new();
    char *translated;
    if (tok == NULL)
        return NULL;
    tok->input = translated = _PyTokenizer_translate_newlines(str, exec_input, preserve_crlf, tok);
    if (translated == NULL) {
        _PyTokenizer_Free(tok);
        return NULL;
    }
    tok->decoding_state = STATE_NORMAL;
    tok->enc = NULL;
    tok->str = translated;
    tok->encoding = _PyTokenizer_new_string("utf-8", 5, tok);
    if (!tok->encoding) {
        _PyTokenizer_Free(tok);
        return NULL;
    }

    tok->buf = tok->cur = tok->inp = translated;
    tok->end = translated;
    tok->underflow = &tok_underflow_string;
    return tok;
}
