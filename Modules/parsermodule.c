/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

/* Raw interface to the parser. */

#include "allobjects.h"
#include "node.h"
#include "token.h"
#include "pythonrun.h"
#include "graminit.h"
#include "errcode.h"

object *
node2tuple(n)
	node *n;
{
	if (n == NULL) {
		INCREF(None);
		return None;
	}
	if (ISNONTERMINAL(TYPE(n))) {
		int i;
		object *v, *w;
		v = newtupleobject(1 + NCH(n));
		if (v == NULL)
			return v;
		w = newintobject(TYPE(n));
		if (w == NULL) {
			DECREF(v);
			return NULL;
		}
		settupleitem(v, 0, w);
		for (i = 0; i < NCH(n); i++) {
			w = node2tuple(CHILD(n, i));
			if (w == NULL) {
				DECREF(v);
				return NULL;
			}
			settupleitem(v, i+1, w);
		}
		return v;
	}
	else if (ISTERMINAL(TYPE(n))) {
		return mkvalue("(is)", TYPE(n), STR(n));
	}
	else {
		err_setstr(SystemError, "unrecognized parse tree node type");
		return NULL;
	}
}

static object *
parser_parsefile(self, args)
	object *self;
	object *args;
{
	char *filename;
	FILE *fp;
	node *n = NULL;
	object *res;
	if (!getargs(args, "s", &filename))
		return NULL;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		err_errno(IOError);
		return NULL;
	}
	n = parse_file(fp, filename, file_input);
	fclose(fp);
	if (n == NULL)
		return NULL;
	res = node2tuple(n);
	freetree(n);
	return res;
}

static struct methodlist parser_methods[] = {
	{"parsefile", parser_parsefile},
	{0, 0} /* Sentinel */
};

void
initparser()
{
	initmodule("parser", parser_methods);
}
