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

/* Tokenizer implementation */

/* XXX This is rather old, should be restructured perhaps */
/* XXX Need a better interface to report errors than writing to stderr */
/* XXX Should use editor resource to fetch true tab size on Macintosh */

#include "pgenheaders.h"

#include <ctype.h>
#include "string.h"

#include "fgetsintr.h"
#include "tokenizer.h"
#include "errcode.h"

/* Don't ever change this -- it would break the portability of Python code */
#define TABSIZE 8

/* Forward */
static struct tok_state *tok_new PROTO((void));
static int tok_nextc PROTO((struct tok_state *tok));
static void tok_backup PROTO((struct tok_state *tok, int c));

/* Token names */

char *tok_name[] = {
	"ENDMARKER",
	"NAME",
	"NUMBER",
	"STRING",
	"NEWLINE",
	"INDENT",
	"DEDENT",
	"LPAR",
	"RPAR",
	"LSQB",
	"RSQB",
	"COLON",
	"COMMA",
	"SEMI",
	"PLUS",
	"MINUS",
	"STAR",
	"SLASH",
	"VBAR",
	"AMPER",
	"LESS",
	"GREATER",
	"EQUAL",
	"DOT",
	"PERCENT",
	"BACKQUOTE",
	"LBRACE",
	"RBRACE",
	"EQEQUAL",
	"NOTEQUAL",
	"LESSEQUAL",
	"GREATEREQUAL",
	"TILDE",
	"CIRCUMFLEX",
	"LEFTSHIFT",
	"RIGHTSHIFT",
	/* This table must match the #defines in token.h! */
	"OP",
	"<ERRORTOKEN>",
	"<N_TOKENS>"
};


/* Create and initialize a new tok_state structure */

static struct tok_state *
tok_new()
{
	struct tok_state *tok = NEW(struct tok_state, 1);
	if (tok == NULL)
		return NULL;
	tok->buf = tok->cur = tok->end = tok->inp = NULL;
	tok->done = E_OK;
	tok->fp = NULL;
	tok->tabsize = TABSIZE;
	tok->indent = 0;
	tok->indstack[0] = 0;
	tok->atbol = 1;
	tok->pendin = 0;
	tok->prompt = tok->nextprompt = NULL;
	tok->lineno = 0;
	return tok;
}


/* Set up tokenizer for string */

struct tok_state *
tok_setups(str)
	char *str;
{
	struct tok_state *tok = tok_new();
	if (tok == NULL)
		return NULL;
	tok->buf = tok->cur = str;
	tok->end = tok->inp = strchr(str, '\0');
	return tok;
}


/* Set up tokenizer for file */

struct tok_state *
tok_setupf(fp, ps1, ps2)
	FILE *fp;
	char *ps1, *ps2;
{
	struct tok_state *tok = tok_new();
	if (tok == NULL)
		return NULL;
	if ((tok->buf = NEW(char, BUFSIZ)) == NULL) {
		DEL(tok);
		return NULL;
	}
	tok->cur = tok->inp = tok->buf;
	tok->end = tok->buf + BUFSIZ;
	tok->fp = fp;
	tok->prompt = ps1;
	tok->nextprompt = ps2;
	return tok;
}


/* Free a tok_state structure */

void
tok_free(tok)
	struct tok_state *tok;
{
	/* XXX really need a separate flag to say 'my buffer' */
	if (tok->fp != NULL && tok->buf != NULL)
		DEL(tok->buf);
	DEL(tok);
}


/* Get next char, updating state; error code goes into tok->done */

static int
tok_nextc(tok)
	register struct tok_state *tok;
{
	for (;;) {
		if (tok->cur != tok->inp)
			return *tok->cur++; /* Fast path */
		if (tok->done != E_OK)
			return EOF;
		if (tok->fp == NULL) {
			tok->done = E_EOF;
			return EOF;
		}
#ifdef USE_READLINE
		if (tok->prompt != NULL) {
			extern char *readline PROTO((char *prompt));
			static int been_here;
			if (!been_here) {
				/* Force rebind of TAB to insert-tab */
				extern int rl_insert();
				rl_bind_key('\t', rl_insert);
				been_here++;
			}
			if (tok->buf != NULL)
				free(tok->buf);
			tok->buf = readline(tok->prompt);
			(void) intrcheck(); /* Clear pending interrupt */
			if (tok->nextprompt != NULL)
				tok->prompt = tok->nextprompt;
			if (tok->buf == NULL) {
				tok->done = E_EOF;
			}
			else {
				tok->end = strchr(tok->buf, '\0');
				if (tok->end > tok->buf)
					add_history(tok->buf);
				/* Replace trailing '\n' by '\0'
				   (we don't need a '\0', but the
				   tokenizer wants a '\n'...) */
				*tok->end++ = '\n';
				tok->inp = tok->end;
				tok->cur = tok->buf;
			}
		}
		else
#endif
		{
			if (tok->prompt != NULL) {
				fprintf(stderr, "%s", tok->prompt);
				if (tok->nextprompt != NULL)
					tok->prompt = tok->nextprompt;
			}
			if (tok->buf == NULL) {
				tok->buf = NEW(char, BUFSIZ);
				if (tok->buf == NULL) {
					tok->done = E_NOMEM;
					return EOF;
				}
				tok->end = tok->buf + BUFSIZ;
			}
			tok->done = fgets_intr(tok->buf,
				(int)(tok->end - tok->buf), tok->fp);
			tok->inp = strchr(tok->buf, '\0');
			/* Read until '\n' or EOF */
			while (tok->inp+1==tok->end && tok->inp[-1]!='\n') {
				int curvalid = tok->inp - tok->buf;
				int cursize = tok->end - tok->buf;
				int newsize = cursize + BUFSIZ;
				char *newbuf = tok->buf;
				RESIZE(newbuf, char, newsize);
				if (newbuf == NULL) {
					tok->done = E_NOMEM;
					tok->cur = tok->inp;
					return EOF;
				}
				tok->buf = newbuf;
				tok->inp = tok->buf + curvalid;
				tok->end = tok->buf + newsize;
				if (fgets_intr(tok->inp,
					       (int)(tok->end - tok->inp),
					       tok->fp) != E_OK)
					break;
				tok->inp = strchr(tok->inp, '\0');
			}
			tok->cur = tok->buf;
		}
		if (tok->done != E_OK) {
			if (tok->prompt != NULL)
				fprintf(stderr, "\n");
			tok->cur = tok->inp;
			return EOF;
		}
	}
	/*NOTREACHED*/
}


/* Back-up one character */

static void
tok_backup(tok, c)
	register struct tok_state *tok;
	register int c;
{
	if (c != EOF) {
		if (--tok->cur < tok->buf) {
			fprintf(stderr, "tok_backup: begin of buffer\n");
			abort();
		}
		if (*tok->cur != c)
			*tok->cur = c;
	}
}


/* Return the token corresponding to a single character */

int
tok_1char(c)
	int c;
{
	switch (c) {
	case '(':	return LPAR;
	case ')':	return RPAR;
	case '[':	return LSQB;
	case ']':	return RSQB;
	case ':':	return COLON;
	case ',':	return COMMA;
	case ';':	return SEMI;
	case '+':	return PLUS;
	case '-':	return MINUS;
	case '*':	return STAR;
	case '/':	return SLASH;
	case '|':	return VBAR;
	case '&':	return AMPER;
	case '<':	return LESS;
	case '>':	return GREATER;
	case '=':	return EQUAL;
	case '.':	return DOT;
	case '%':	return PERCENT;
	case '`':	return BACKQUOTE;
	case '{':	return LBRACE;
	case '}':	return RBRACE;
	case '^':	return CIRCUMFLEX;
	case '~':	return TILDE;
	default:	return OP;
	}
}


int
tok_2char(c1, c2)
	int c1, c2;
{
	switch (c1) {
	case '=':
		switch (c2) {
		case '=':	return EQEQUAL;
		}
		break;
	case '!':
		switch (c2) {
		case '=':	return NOTEQUAL;
		}
		break;
	case '<':
		switch (c2) {
		case '>':	return NOTEQUAL;
		case '=':	return LESSEQUAL;
		case '<':	return LEFTSHIFT;
		}
		break;
	case '>':
		switch (c2) {
		case '=':	return GREATEREQUAL;
		case '>':	return RIGHTSHIFT;
		}
		break;
	}
	return OP;
}


/* Get next token, after space stripping etc. */

int
tok_get(tok, p_start, p_end)
	register struct tok_state *tok; /* In/out: tokenizer state */
	char **p_start, **p_end; /* Out: point to start/end of token */
{
	register int c;
	int blankline;

  nextline:
	blankline = 0;

	/* Get indentation level */
	if (tok->atbol) {
		register int col = 0;
		tok->atbol = 0;
		tok->lineno++;
		for (;;) {
			c = tok_nextc(tok);
			if (c == ' ')
				col++;
			else if (c == '\t')
				col = (col/tok->tabsize + 1) * tok->tabsize;
			else
				break;
		}
		tok_backup(tok, c);
		if (c == '#' || c == '\n') {
			/* Lines with only whitespace and/or comments
			   shouldn't affect the indentation and are
			   not passed to the parser as NEWLINE tokens,
			   except *totally* empty lines in interactive
			   mode, which signal the end of a command group. */
			if (col == 0 && c == '\n' && tok->prompt != NULL)
				blankline = 0; /* Let it through */
			else
				blankline = 1; /* Ignore completely */
			/* We can't jump back right here since we still
			   may need to skip to the end of a comment */
		}
		if (!blankline) {
			if (col == tok->indstack[tok->indent]) {
				/* No change */
			}
			else if (col > tok->indstack[tok->indent]) {
				/* Indent -- always one */
				if (tok->indent+1 >= MAXINDENT) {
					fprintf(stderr, "excessive indent\n");
					tok->done = E_TOKEN;
					tok->cur = tok->inp;
					return ERRORTOKEN;
				}
				tok->pendin++;
				tok->indstack[++tok->indent] = col;
			}
			else /* col < tok->indstack[tok->indent] */ {
				/* Dedent -- any number, must be consistent */
				while (tok->indent > 0 &&
					col < tok->indstack[tok->indent]) {
					tok->indent--;
					tok->pendin--;
				}
				if (col != tok->indstack[tok->indent]) {
					fprintf(stderr, "inconsistent dedent\n");
					tok->done = E_TOKEN;
					tok->cur = tok->inp;
					return ERRORTOKEN;
				}
			}
		}
	}
	
	*p_start = *p_end = tok->cur;
	
	/* Return pending indents/dedents */
	if (tok->pendin != 0) {
		if (tok->pendin < 0) {
			tok->pendin++;
			return DEDENT;
		}
		else {
			tok->pendin--;
			return INDENT;
		}
	}
	
 again:
	/* Skip spaces */
	do {
		c = tok_nextc(tok);
	} while (c == ' ' || c == '\t');
	
	/* Set start of current token */
	*p_start = tok->cur - 1;
	
	/* Skip comment */
	if (c == '#') {
		/* Hack to allow overriding the tabsize in the file.
		   This is also recognized by vi, when it occurs near the
		   beginning or end of the file.  (Will vi never die...?)
		   For Python it must be at the beginning of the file! */
		int x;
		/* XXX The cast to (unsigned char *) is needed by THINK C 3.0 */
		if (sscanf(/*(unsigned char *)*/tok->cur,
				" vi:set tabsize=%d:", &x) == 1 &&
						x >= 1 && x <= 40) {
			/* fprintf(stderr, "# vi:set tabsize=%d:\n", x); */
			tok->tabsize = x;
		}
		do {
			c = tok_nextc(tok);
		} while (c != EOF && c != '\n');
	}
	
	/* Check for EOF and errors now */
	if (c == EOF) {
		*p_start = *p_end = tok->cur;
		return tok->done == E_EOF ? ENDMARKER : ERRORTOKEN;
	}
	
	/* Identifier (most frequent token!) */
	if (isalpha(c) || c == '_') {
		do {
			c = tok_nextc(tok);
		} while (isalnum(c) || c == '_');
		tok_backup(tok, c);
		*p_end = tok->cur;
		return NAME;
	}
	
	/* Newline */
	if (c == '\n') {
		tok->atbol = 1;
		if (blankline)
			goto nextline;
		*p_end = tok->cur - 1; /* Leave '\n' out of the string */
		return NEWLINE;
	}
	
	/* Period or number starting with period? */
	if (c == '.') {
		c = tok_nextc(tok);
		if (isdigit(c)) {
			goto fraction;
		}
		else {
			tok_backup(tok, c);
			*p_end = tok->cur;
			return DOT;
		}
	}
	
	/* Number */
	if (isdigit(c)) {
		if (c == '0') {
			/* Hex or octal */
			c = tok_nextc(tok);
			if (c == '.')
				goto fraction;
			if (c == 'x' || c == 'X') {
				/* Hex */
				do {
					c = tok_nextc(tok);
				} while (isxdigit(c));
			}
			else {
				/* XXX This is broken!  E.g.,
				   09.9 should be accepted as float! */
				/* Octal; c is first char of it */
				/* There's no 'isoctdigit' macro, sigh */
				while ('0' <= c && c < '8') {
					c = tok_nextc(tok);
				}
			}
			if (c == 'l' || c == 'L')
				c = tok_nextc(tok);
		}
		else {
			/* Decimal */
			do {
				c = tok_nextc(tok);
			} while (isdigit(c));
			if (c == 'l' || c == 'L')
				c = tok_nextc(tok);
			else {
				/* Accept floating point numbers.
				   XXX This accepts incomplete things like
				   XXX 12e or 1e+; worry run-time.
				   XXX Doesn't accept numbers
				   XXX starting with a dot */
				if (c == '.') {
		fraction:
					/* Fraction */
					do {
						c = tok_nextc(tok);
					} while (isdigit(c));
				}
				if (c == 'e' || c == 'E') {
					/* Exponent part */
					c = tok_nextc(tok);
					if (c == '+' || c == '-')
						c = tok_nextc(tok);
					while (isdigit(c)) {
						c = tok_nextc(tok);
					}
				}
			}
		}
		tok_backup(tok, c);
		*p_end = tok->cur;
		return NUMBER;
	}
	
	/* String */
	if (c == '\'') {
		for (;;) {
			c = tok_nextc(tok);
			if (c == '\n' || c == EOF) {
				tok->done = E_TOKEN;
				tok->cur = tok->inp;
				return ERRORTOKEN;
			}
			if (c == '\\') {
				c = tok_nextc(tok);
				*p_end = tok->cur;
				if (c == '\n' || c == EOF) {
					tok->done = E_TOKEN;
					tok->cur = tok->inp;
					return ERRORTOKEN;
				}
				continue;
			}
			if (c == '\'')
				break;
		}
		*p_end = tok->cur;
		return STRING;
	}
	
	/* Line continuation */
	if (c == '\\') {
		c = tok_nextc(tok);
		if (c != '\n') {
			tok->done = E_TOKEN;
			tok->cur = tok->inp;
			return ERRORTOKEN;
		}
		tok->lineno++;
		goto again; /* Read next line */
	}
	
	/* Check for two-character token */
	{
		int c2 = tok_nextc(tok);
		int token = tok_2char(c, c2);
		if (token != OP) {
			*p_end = tok->cur;
			return token;
		}
		tok_backup(tok, c2);
	}
	
	/* Punctuation character */
	*p_end = tok->cur;
	return tok_1char(c);
}


#ifdef DEBUG

void
tok_dump(type, start, end)
	int type;
	char *start, *end;
{
	printf("%s", tok_name[type]);
	if (type == NAME || type == NUMBER || type == STRING || type == OP)
		printf("(%.*s)", (int)(end - start), start);
}

#endif
