/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Parse tree node implementation */

#include "pgenheaders.h"
#include "node.h"

node *
PyNode_New(type)
	int type;
{
	node *n = PyMem_NEW(node, 1);
	if (n == NULL)
		return NULL;
	n->n_type = type;
	n->n_str = NULL;
	n->n_lineno = 0;
	n->n_nchildren = 0;
	n->n_child = NULL;
	return n;
}

#define XXX 3 /* Node alignment factor to speed up realloc */
#define XXXROUNDUP(n) ((n) == 1 ? 1 : ((n) + XXX - 1) / XXX * XXX)

node *
PyNode_AddChild(n1, type, str, lineno)
	register node *n1;
	int type;
	char *str;
	int lineno;
{
	register int nch = n1->n_nchildren;
	register int nch1 = nch+1;
	register node *n;
	if (XXXROUNDUP(nch) < nch1) {
		n = n1->n_child;
		nch1 = XXXROUNDUP(nch1);
		PyMem_RESIZE(n, node, nch1);
		if (n == NULL)
			return NULL;
		n1->n_child = n;
	}
	n = &n1->n_child[n1->n_nchildren++];
	n->n_type = type;
	n->n_str = str;
	n->n_lineno = lineno;
	n->n_nchildren = 0;
	n->n_child = NULL;
	return n;
}

/* Forward */
static void freechildren Py_PROTO((node *));


void
PyNode_Free(n)
	node *n;
{
	if (n != NULL) {
		freechildren(n);
		PyMem_DEL(n);
	}
}

static void
freechildren(n)
	node *n;
{
	int i;
	for (i = NCH(n); --i >= 0; )
		freechildren(CHILD(n, i));
	if (n->n_child != NULL)
		PyMem_DEL(n->n_child);
	if (STR(n) != NULL)
		PyMem_DEL(STR(n));
}
