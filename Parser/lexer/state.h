#ifndef _PY_LEXER_H_
#define _PY_LEXER_H_

#include "object.h"
#include "tokenizer_mode.h"

#define MAXINDENT 100       /* Max indentation level */
#define MAXLEVEL 200        /* Max parentheses level */

enum decoding_state {
    STATE_INIT,
    STATE_SEEK_CODING,
    STATE_NORMAL
};

enum interactive_underflow_t {
    /* Normal mode of operation: return a new token when asked in interactive mode */
    IUNDERFLOW_NORMAL,
    /* Forcefully return ENDMARKER when asked for a new token in interactive mode. This
     * can be used to prevent the tokenizer to prompt the user for new tokens */
    IUNDERFLOW_STOP,
};

struct token {
    int level;
    int lineno, col_offset, end_lineno, end_col_offset;
    const char *start, *end;
    PyObject *metadata;
};

/* Tokenizer state */
struct tok_state {
    /* Input state; buf <= cur <= inp <= end */
    /* NB an entire line is held in the buffer */
    char *buf;          /* Input buffer, or NULL; malloc'ed if fp != NULL or readline != NULL */
    char *cur;          /* Next character in buffer */
    char *inp;          /* End of data in buffer */
    int fp_interactive; /* If the file descriptor is interactive */
    char *interactive_src_start; /* The start of the source parsed so far in interactive mode */
    char *interactive_src_end; /* The end of the source parsed so far in interactive mode */
    const char *end;    /* End of input buffer if buf != NULL */
    const char *start;  /* Start of current token if not NULL */
    int done;           /* E_OK normally, E_EOF at EOF, otherwise error code */
    /* NB If done != E_OK, cur must be == inp!!! */
    FILE *fp;           /* Rest of input; NULL if tokenizing a string */
    int tabsize;        /* Tab spacing */
    int indent;         /* Current indentation index */
    int indstack[MAXINDENT];            /* Stack of indents */
    int atbol;          /* Nonzero if at begin of new line */
    int pendin;         /* Pending indents (if > 0) or dedents (if < 0) */
    const char *prompt, *nextprompt;          /* For interactive prompting */
    int lineno;         /* Current line number */
    int first_lineno;   /* First line of a single line or multi line string
                           expression (cf. issue 16806) */
    int starting_col_offset; /* The column offset at the beginning of a token */
    int col_offset;     /* Current col offset */
    int level;          /* () [] {} Parentheses nesting level */
            /* Used to allow free continuations inside them */
    char parenstack[MAXLEVEL];
    int parenlinenostack[MAXLEVEL];
    int parencolstack[MAXLEVEL];
    PyObject *filename;
    /* Stuff for checking on different tab sizes */
    int altindstack[MAXINDENT];         /* Stack of alternate indents */
    /* Stuff for PEP 0263 */
    enum decoding_state decoding_state;
    int decoding_erred;         /* whether erred in decoding  */
    char *encoding;         /* Source encoding. */
    int cont_line;          /* whether we are in a continuation line. */
    const char* line_start;     /* pointer to start of current line */
    const char* multi_line_start; /* pointer to start of first line of
                                     a single line or multi line string
                                     expression (cf. issue 16806) */
    PyObject *decoding_readline; /* open(...).readline */
    PyObject *decoding_buffer;
    PyObject *readline;     /* readline() function */
    const char* enc;        /* Encoding for the current str. */
    char* str;          /* Source string being tokenized (if tokenizing from a string)*/
    char* input;       /* Tokenizer's newline translated copy of the string. */

    int type_comments;      /* Whether to look for type comments */

    /* How to proceed when asked for a new token in interactive mode */
    enum interactive_underflow_t interactive_underflow;
    int (*underflow)(struct tok_state *); /* Function to call when buffer is empty and we need to refill it*/

    int report_warnings;
    /* Tokenizer mode stack for handling nested f-strings and t-strings.
     * See tokenizer_mode.h for details. */
    tokenizer_mode tok_mode_stack[MAXFSTRINGLEVEL];
    int tok_mode_stack_index;
    int tok_extra_tokens;
    int comment_newline;
    int implicit_newline;
#ifdef Py_DEBUG
    int debug;
#endif
};

int _PyLexer_type_comment_token_setup(struct tok_state *tok, struct token *token, int type, int col_offset,
                         int end_col_offset, const char *start, const char *end);
int _PyLexer_token_setup(struct tok_state *tok, struct token *token, int type, const char *start, const char *end);

struct tok_state *_PyTokenizer_tok_new(void);
void _PyTokenizer_Free(struct tok_state *);
void _PyToken_Free(struct token *);
void _PyToken_Init(struct token *);


#endif
