/* Tokenizer interface */

#include "token.h"	/* For token types */

#define MAXINDENT 100	/* Max indentation level */

/* Tokenizer state */
struct tok_state {
	/* Input state; buf <= cur <= inp <= end */
	/* NB an entire token must fit in the buffer */
	char *buf;	/* Input buffer */
	char *cur;	/* Next character in buffer */
	char *inp;	/* End of data in buffer */
	char *end;	/* End of input buffer */
	int done;	/* 0 normally, 1 at EOF, -1 after error */
	FILE *fp;	/* Rest of input; NULL if tokenizing a string */
	int tabsize;	/* Tab spacing */
	int indent;	/* Current indentation index */
	int indstack[MAXINDENT];	/* Stack of indents */
	int atbol;	/* Nonzero if at begin of new line */
	int pendin;	/* Pending indents (if > 0) or dedents (if < 0) */
	char *prompt, *nextprompt;	/* For interactive prompting */
	int lineno;	/* Current line number */
};

extern struct tok_state *tok_setups PROTO((char *));
extern struct tok_state *tok_setupf PROTO((FILE *, char *ps1, char *ps2));
extern void tok_free PROTO((struct tok_state *));
extern int tok_get PROTO((struct tok_state *, char **, char **));
