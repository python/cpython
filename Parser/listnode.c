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

/* List a node on a file */

#include "pgenheaders.h"
#include "token.h"
#include "node.h"

/* Forward */
static void list1node PROTO((FILE *, node *));

void
listtree(n)
	node *n;
{
	listnode(stdout, n);
}

static int level, atbol;

void
listnode(fp, n)
	FILE *fp;
	node *n;
{
	level = 0;
	atbol = 1;
	list1node(fp, n);
}

static void
list1node(fp, n)
	FILE *fp;
	node *n;
{
	if (n == 0)
		return;
	if (ISNONTERMINAL(TYPE(n))) {
		int i;
		for (i = 0; i < NCH(n); i++)
			list1node(fp, CHILD(n, i));
	}
	else if (ISTERMINAL(TYPE(n))) {
		switch (TYPE(n)) {
		case INDENT:
			++level;
			break;
		case DEDENT:
			--level;
			break;
		default:
			if (atbol) {
				int i;
				for (i = 0; i < level; ++i)
					fprintf(fp, "\t");
				atbol = 0;
			}
			if (TYPE(n) == NEWLINE) {
				if (STR(n) != NULL)
					fprintf(fp, "%s", STR(n));
				fprintf(fp, "\n");
				atbol = 1;
			}
			else
				fprintf(fp, "%s ", STR(n));
			break;
		}
	}
	else
		fprintf(fp, "? ");
}
