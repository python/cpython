/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

/* Parser-tokenizer link implementation */

#include "pgenheaders.h"
#include "tokenizer.h"
#include "node.h"
#include "grammar.h"
#include "parser.h"
#include "parsetok.h"
#include "errcode.h"


/* Forward */
static int parsetok PROTO((struct tok_state *, grammar *, int, node **));


/* Parse input coming from a string.  Return error code, print some errors. */

int
parsestring(s, g, start, n_ret)
	char *s;
	grammar *g;
	int start;
	node **n_ret;
{
	struct tok_state *tok = tok_setups(s);
	int ret;
	
	if (tok == NULL) {
		fprintf(stderr, "no mem for tok_setups\n");
		return E_NOMEM;
	}
	ret = parsetok(tok, g, start, n_ret);
	if (ret == E_TOKEN || ret == E_SYNTAX) {
		fprintf(stderr, "String parsing error at line %d\n",
			tok->lineno);
	}
	tok_free(tok);
	return ret;
}


/* Parse input coming from a file.  Return error code, print some errors. */

int
parsefile(fp, filename, g, start, ps1, ps2, n_ret)
	FILE *fp;
	char *filename;
	grammar *g;
	int start;
	char *ps1, *ps2;
	node **n_ret;
{
	struct tok_state *tok = tok_setupf(fp, ps1, ps2);
	int ret;
	
	if (tok == NULL) {
		fprintf(stderr, "no mem for tok_setupf\n");
		return E_NOMEM;
	}
	ret = parsetok(tok, g, start, n_ret);
	if (ret == E_TOKEN || ret == E_SYNTAX) {
		char *p;
		fprintf(stderr, "Parsing error: file %s, line %d:\n",
						filename, tok->lineno);
		*tok->inp = '\0';
		if (tok->inp > tok->buf && tok->inp[-1] == '\n')
			tok->inp[-1] = '\0';
		fprintf(stderr, "%s\n", tok->buf);
		for (p = tok->buf; p < tok->cur; p++) {
			if (*p == '\t')
				putc('\t', stderr);
			else
				putc(' ', stderr);
		}
		fprintf(stderr, "^\n");
	}
	tok_free(tok);
	return ret;
}


/* Parse input coming from the given tokenizer structure.
   Return error code. */

static int
parsetok(tok, g, start, n_ret)
	struct tok_state *tok;
	grammar *g;
	int start;
	node **n_ret;
{
	parser_state *ps;
	int ret;
	
	if ((ps = newparser(g, start)) == NULL) {
		fprintf(stderr, "no mem for new parser\n");
		return E_NOMEM;
	}
	
	for (;;) {
		char *a, *b;
		int type;
		int len;
		char *str;
		
		type = tok_get(tok, &a, &b);
		if (type == ERRORTOKEN) {
			ret = tok->done;
			break;
		}
		len = b - a;
		str = NEW(char, len + 1);
		if (str == NULL) {
			fprintf(stderr, "no mem for next token\n");
			ret = E_NOMEM;
			break;
		}
		strncpy(str, a, len);
		str[len] = '\0';
		ret = addtoken(ps, (int)type, str, tok->lineno);
		if (ret != E_OK) {
			if (ret == E_DONE) {
				*n_ret = ps->p_tree;
				ps->p_tree = NULL;
			}
			else if (tok->lineno <= 1 && tok->done == E_EOF)
				ret = E_EOF;
			break;
		}
	}
	
	delparser(ps);
	return ret;
}
