#ifndef Py_TOKENIZER_H
#define Py_TOKENIZER_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Tokenizer interface */

#include "token.h"	/* For token types */

#define MAXINDENT 100	/* Max indentation level */

/* Tokenizer state */
struct tok_state {
	/* Input state; buf <= cur <= inp <= end */
	/* NB an entire line is held in the buffer */
	char *buf;	/* Input buffer, or NULL; malloc'ed if fp != NULL */
	char *cur;	/* Next character in buffer */
	char *inp;	/* End of data in buffer */
	char *end;	/* End of input buffer if buf != NULL */
	int done;	/* E_OK normally, E_EOF at EOF, otherwise error code */
	/* NB If done != E_OK, cur must be == inp!!! */
	FILE *fp;	/* Rest of input; NULL if tokenizing a string */
	int tabsize;	/* Tab spacing */
	int indent;	/* Current indentation index */
	int indstack[MAXINDENT];	/* Stack of indents */
	int atbol;	/* Nonzero if at begin of new line */
	int pendin;	/* Pending indents (if > 0) or dedents (if < 0) */
	char *prompt, *nextprompt;	/* For interactive prompting */
	int lineno;	/* Current line number */
	int level;	/* () [] {} Parentheses nesting level */
			/* Used to allow free continuations inside them */
};

extern struct tok_state *tok_setups PROTO((char *));
extern struct tok_state *tok_setupf PROTO((FILE *, char *, char *));
extern void tok_free PROTO((struct tok_state *));
extern int tok_get PROTO((struct tok_state *, char **, char **));

#ifdef __cplusplus
}
#endif
#endif /* !Py_TOKENIZER_H */
