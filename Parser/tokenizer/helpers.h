#ifndef _PY_TOKENIZER_HELPERS_H_
#define _PY_TOKENIZER_HELPERS_H_

#include "Python.h"

#include "../lexer/state.h"

#define ADVANCE_LINENO() \
            tok->lineno++; \
            tok->col_offset = 0;

int _PyTokenizer_syntaxerror(struct tok_state *tok, const char *format, ...);
int _PyTokenizer_syntaxerror_known_range(struct tok_state *tok, int col_offset, int end_col_offset, const char *format, ...);
int _PyTokenizer_indenterror(struct tok_state *tok);
int _PyTokenizer_warn_invalid_escape_sequence(struct tok_state *tok, int first_invalid_escape_char);
int _PyTokenizer_parser_warn(struct tok_state *tok, PyObject *category, const char *format, ...);
char *_PyTokenizer_error_ret(struct tok_state *tok);

char *_PyTokenizer_new_string(const char *s, Py_ssize_t len, struct tok_state *tok);
char *_PyTokenizer_translate_newlines(const char *s, int exec_input, int preserve_crlf, struct tok_state *tok);
PyObject *_PyTokenizer_translate_into_utf8(const char* str, const char* enc);

int _PyTokenizer_check_bom(int get_char(struct tok_state *),
          void unget_char(int, struct tok_state *),
          int set_readline(struct tok_state *, const char *),
          struct tok_state *tok);
int _PyTokenizer_check_coding_spec(const char* line, Py_ssize_t size, struct tok_state *tok,
                  int set_readline(struct tok_state *, const char *));
int _PyTokenizer_ensure_utf8(char *line, struct tok_state *tok);

#ifdef Py_DEBUG
void _PyTokenizer_print_escape(FILE *f, const char *s, Py_ssize_t size);
void _PyTokenizer_tok_dump(int type, char *start, char *end);
#endif


#endif
