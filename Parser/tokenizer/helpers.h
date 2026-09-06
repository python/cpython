#ifndef _PY_TOKENIZER_HELPERS_H_
#define _PY_TOKENIZER_HELPERS_H_

#include "Python.h"

#include "../lexer/state.h"

int _PyTokenizer_syntaxerror(struct tok_state *tok, const char *format, ...);
/* Positive range columns are 1-based byte columns. A start column of -1
   derives the character column from the reporting cursor; an end column of
   -1 uses the start column. */
int _PyTokenizer_syntaxerror_known_range(struct tok_state *tok, int col_offset, int end_col_offset, const char *format, ...);
int _PyTokenizer_syntaxerror_at(
    struct tok_state *tok, const char *line_start, Py_ssize_t cursor_offset,
    int lineno, int col_offset, int end_col_offset, const char *format, ...);
int _PyTokenizer_indenterror(struct tok_state *tok);
int _PyTokenizer_warn_invalid_escape_sequence(struct tok_state *tok, int first_invalid_escape_char);
int _PyTokenizer_parser_warn(struct tok_state *tok, PyObject *category, const char *format, ...);

int _PyTokenizer_ensure_utf8(const char *line, struct tok_state *tok, int lineno);

#ifdef Py_DEBUG
void _PyTokenizer_print_escape(FILE *f, const char *s, Py_ssize_t size);
void _PyTokenizer_tok_dump(int type, char *start, char *end);
#endif


#endif
