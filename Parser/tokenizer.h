#ifndef Py_TOKENIZER_H
#define Py_TOKENIZER_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.

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
	char *start;	/* Start of current token if not NULL */
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
	/* Stuff for checking on different tab sizes */
	char *filename;	/* For error messages */
	int altwarning;	/* Issue warning if alternate tabs don't match */
	int alterror;	/* Issue error if alternate tabs don't match */
	int alttabsize;	/* Alternate tab spacing */
	int altindstack[MAXINDENT];	/* Stack of alternate indents */
};

extern struct tok_state *PyTokenizer_FromString Py_PROTO((char *));
extern struct tok_state *PyTokenizer_FromFile
	Py_PROTO((FILE *, char *, char *));
extern void PyTokenizer_Free Py_PROTO((struct tok_state *));
extern int PyTokenizer_Get Py_PROTO((struct tok_state *, char **, char **));

#ifdef __cplusplus
}
#endif
#endif /* !Py_TOKENIZER_H */
