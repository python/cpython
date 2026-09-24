#include "Python.h"
#include "errcode.h"
#include "pycore_token.h"

#include "lexer_internal.h"
#include "../tokenizer/helpers.h"
#include "../tokenizer/reader.h"

#define TABSIZE 8
#define ALTTABSIZE 1

int
_PyLexer_ContinueLine(struct tok_state *tok)
{
    int c = tok_nextc(tok);
    if (c == '\r') {
        c = tok_nextc(tok);
    }
    if (c != '\n') {
        tok->done = E_LINECONT;
        return -1;
    }
    c = tok_nextc(tok);
    if (c == EOF) {
        tok->done = E_EOF;
        tok->cur = tok->inp;
        return -1;
    } else {
        tok_backup(tok, c);
    }
    return c;
}


static int
update_indentation(struct tok_state *tok, int col, int altcol)
{
    lexer_layout_state *layout = &tok->layout;
    if (col == layout->stack[layout->depth].column) {
        if (altcol != layout->stack[layout->depth].alternate_column) {
            _PyTokenizer_indenterror(tok);
            return -1;
        }
    }
    else if (col > layout->stack[layout->depth].column) {
        if (layout->depth + 1 >= MAXINDENT) {
            tok->done = E_TOODEEP;
            tok->cur = tok->inp;
            return -1;
        }
        if (altcol <= layout->stack[layout->depth].alternate_column) {
            _PyTokenizer_indenterror(tok);
            return -1;
        }
        layout->pending++;
        layout->stack[++layout->depth] = (indentation_level){col, altcol};
    }
    else {
        while (layout->depth > 0 &&
            col < layout->stack[layout->depth].column) {
            layout->pending--;
            layout->depth--;
        }
        if (col != layout->stack[layout->depth].column) {
            tok->done = E_DEDENT;
            tok->cur = tok->inp;
            return -1;
        }
        if (altcol != layout->stack[layout->depth].alternate_column) {
            _PyTokenizer_indenterror(tok);
            return -1;
        }
    }
    return 0;
}

int
_PyLexer_BeginLine(struct tok_state *tok)
{
    assert(tok->layout.at_bol);
    int c;
    int blankline = 0;
    int col = 0;
    int altcol = 0;
    tok->layout.at_bol = 0;
    int cont_line_col = 0;
    for (;;) {
        c = tok_nextc(tok);
        if (c == ' ') {
            col++, altcol++;
        }
        else if (c == '\t') {
            col = (col / TABSIZE + 1) * TABSIZE;
            altcol = (altcol / ALTTABSIZE + 1) * ALTTABSIZE;
        }
        else if (c == '\014')  {/* Control-L (formfeed) */
            col = altcol = 0; /* For Emacs users */
        }
        else if (c == '\\') {
            // Indentation cannot be split over multiple physical lines
            // using backslashes. This means that if we found a backslash
            // preceded by whitespace, **the first one we find** determines
            // the level of indentation of whatever comes next.
            cont_line_col = cont_line_col ? cont_line_col : col;
            if ((c = _PyLexer_ContinueLine(tok)) == -1) {
                return -1;
            }
        }
        else if (c == EOF && PyErr_Occurred()) {
            return -1;
        }
        else {
            break;
        }
    }
    tok_backup(tok, c);
    if (c == '#' || c == '\n' || c == '\r') {
        int interactive = _PyTok_ReaderIsInteractive(tok);
        /* Lines with only whitespace and/or comments
           shouldn't affect the indentation and are
           not passed to the parser as NEWLINE tokens,
           except *totally* empty lines in interactive
           mode, which signal the end of a command group. */
        if (col == 0 && c == '\n' && interactive) {
            blankline = 0; /* Let it through */
        }
        else if (interactive && tok->lineno == 1) {
            /* In interactive mode, if the first line contains
               only spaces and/or a comment, let it through. */
            blankline = 0;
            col = altcol = 0;
        }
        else {
            blankline = 1; /* Ignore completely */
        }
    }
    if (!blankline && tok->level == 0) {
        col = cont_line_col ? cont_line_col : col;
        altcol = cont_line_col ? cont_line_col : altcol;
        if (update_indentation(tok, col, altcol) < 0) {
            return -1;
        }
    }
    return blankline;
}

int
_PyLexer_IndentationToken(struct tok_state *tok, struct token *token)
{
    assert(tok->layout.pending != 0);
    _PyTok_Off p_start = -1;
    _PyTok_Off p_end = -1;
    if (tok->layout.pending < 0) {
        if (tok->tok_extra_tokens) {
            p_start = tok->cur;
            p_end = tok->cur;
        }
        tok->layout.pending++;
        return _PyLexer_token_setup(tok, token, DEDENT, p_start, p_end);
    }
    else {
        if (tok->tok_extra_tokens) {
            p_start = tok->buf_offset;
            p_end = tok->cur;
        }
        tok->layout.pending--;
        return _PyLexer_token_setup(tok, token, INDENT, p_start, p_end);
    }
}

int
_PyLexer_Newline(struct tok_state *tok, struct token *token, int blankline)
{
    tok->layout.at_bol = 1;
    if (blankline || tok->level > 0) {
        if (!tok->tok_extra_tokens) {
            return 0;
        }
    }
    else if (!tok->layout.comment_newline || !tok->tok_extra_tokens) {
        return _PyLexer_token_setup(tok, token, NEWLINE,
                                    tok->start, tok->cur - 1);
    }
    tok->layout.comment_newline = 0;
    return _PyLexer_token_setup(tok, token, NL, tok->start, tok->cur);
}

void
_PyLexer_ImplyDedents(struct tok_state *tok)
{
    if (tok->layout.depth != 0) {
        tok->layout.pending = -tok->layout.depth;
        tok->layout.depth = 0;
    }
}
