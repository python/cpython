/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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
static node *parsetok PROTO((struct tok_state *, grammar *, int,
			     perrdetail *));

/* Parse input coming from a string.  Return error code, print some errors. */

node *
parsestring(s, g, start, err_ret)
	char *s;
	grammar *g;
	int start;
	perrdetail *err_ret;
{
	struct tok_state *tok;

	err_ret->error = E_OK;
	err_ret->filename = NULL;
	err_ret->lineno = 0;
	err_ret->offset = 0;
	err_ret->text = NULL;

	if ((tok = tok_setups(s)) == NULL) {
		err_ret->error = E_NOMEM;
		return NULL;
	}

	return parsetok(tok, g, start, err_ret);
}


/* Parse input coming from a file.  Return error code, print some errors. */

node *
parsefile(fp, filename, g, start, ps1, ps2, err_ret)
	FILE *fp;
	char *filename;
	grammar *g;
	int start;
	char *ps1, *ps2;
	perrdetail *err_ret;
{
	struct tok_state *tok;

	err_ret->error = E_OK;
	err_ret->filename = filename;
	err_ret->lineno = 0;
	err_ret->offset = 0;
	err_ret->text = NULL;

	if ((tok = tok_setupf(fp, ps1, ps2)) == NULL) {
		err_ret->error = E_NOMEM;
		return NULL;
	}

#ifdef macintosh
	{
		int tabsize = guesstabsize(filename);
		if (tabsize > 0)
			tok->tabsize = tabsize;
	}
#endif

	return parsetok(tok, g, start, err_ret);
}

/* Parse input coming from the given tokenizer structure.
   Return error code. */

static node *
parsetok(tok, g, start, err_ret)
	struct tok_state *tok;
	grammar *g;
	int start;
	perrdetail *err_ret;
{
	parser_state *ps;
	node *n;
	int started = 0;

	if ((ps = newparser(g, start)) == NULL) {
		fprintf(stderr, "no mem for new parser\n");
		err_ret->error = E_NOMEM;
		return NULL;
	}

	for (;;) {
		char *a, *b;
		int type;
		int len;
		char *str;

		type = tok_get(tok, &a, &b);
		if (type == ERRORTOKEN) {
			err_ret->error = tok->done;
			break;
		}
		if (type == ENDMARKER && started) {
			type = NEWLINE; /* Add an extra newline */
			started = 0;
		}
		else
			started = 1;
		len = b - a; /* XXX this may compute NULL - NULL */
		str = NEW(char, len + 1);
		if (str == NULL) {
			fprintf(stderr, "no mem for next token\n");
			err_ret->error = E_NOMEM;
			break;
		}
		if (len > 0)
			strncpy(str, a, len);
		str[len] = '\0';
		if ((err_ret->error =
		     addtoken(ps, (int)type, str, tok->lineno)) != E_OK)
			break;
	}

	if (err_ret->error == E_DONE) {
		n = ps->p_tree;
		ps->p_tree = NULL;
	}
	else
		n = NULL;

	delparser(ps);

	if (n == NULL) {
		if (tok->lineno <= 1 && tok->done == E_EOF)
			err_ret->error = E_EOF;
		err_ret->lineno = tok->lineno;
		err_ret->offset = tok->cur - tok->buf;
		if (tok->buf != NULL) {
			int len = tok->inp - tok->buf;
			err_ret->text = malloc(len + 1);
			if (err_ret->text != NULL) {
				if (len > 0)
					strncpy(err_ret->text, tok->buf, len);
				err_ret->text[len] = '\0';
			}
		}
	}

	tok_free(tok);

	return n;
}
