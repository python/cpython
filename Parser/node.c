/* Parse tree node implementation */

#include "Python.h"
#include "node.h"
#include "errcode.h"

node *
PyNode_New(int type)
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

int
PyNode_AddChild(register node *n1, int type, char *str, int lineno)
{
	register int nch = n1->n_nchildren;
	register int nch1 = nch+1;
	register node *n;
	if (nch == INT_MAX || nch < 0)
		return E_OVERFLOW;
	if (XXXROUNDUP(nch) < nch1) {
		n = n1->n_child;
		nch1 = XXXROUNDUP(nch1);
		PyMem_RESIZE(n, node, nch1);
		if (n == NULL)
			return E_NOMEM;
		n1->n_child = n;
	}
	n = &n1->n_child[n1->n_nchildren++];
	n->n_type = type;
	n->n_str = str;
	n->n_lineno = lineno;
	n->n_nchildren = 0;
	n->n_child = NULL;
	return 0;
}

/* Forward */
static void freechildren(node *);


void
PyNode_Free(node *n)
{
	if (n != NULL) {
		freechildren(n);
		PyMem_DEL(n);
	}
}

static void
freechildren(node *n)
{
	int i;
	for (i = NCH(n); --i >= 0; )
		freechildren(CHILD(n, i));
	if (n->n_child != NULL)
		PyMem_DEL(n->n_child);
	if (STR(n) != NULL)
		PyMem_DEL(STR(n));
}
