/* Parse tree node implementation */

#include "PROTO.h"
#include "malloc.h"
#include "node.h"

node *
newnode(type)
	int type;
{
	node *n = NEW(node, 1);
	if (n == NULL)
		return NULL;
	n->n_type = type;
	n->n_str = NULL;
	n->n_nchildren = 0;
	n->n_child = NULL;
	return n;
}

#define XXX 3 /* Node alignment factor to speed up realloc */
#define XXXROUNDUP(n) ((n) == 1 ? 1 : ((n) + XXX - 1) / XXX * XXX)

node *
addchild(n1, type, str)
	register node *n1;
	int type;
	char *str;
{
	register int nch = n1->n_nchildren;
	register int nch1 = nch+1;
	register node *n;
	if (XXXROUNDUP(nch) < nch1) {
		n = n1->n_child;
		nch1 = XXXROUNDUP(nch1);
		RESIZE(n, node, nch1);
		if (n == NULL)
			return NULL;
		n1->n_child = n;
	}
	n = &n1->n_child[n1->n_nchildren++];
	n->n_type = type;
	n->n_str = str;
	n->n_nchildren = 0;
	n->n_child = NULL;
	return n;
}

static void
freechildren(n)
	node *n;
{
	int i;
	for (i = NCH(n); --i >= 0; )
		freechildren(CHILD(n, i));
	if (n->n_child != NULL)
		DEL(n->n_child);
	if (STR(n) != NULL)
		DEL(STR(n));
}

void
freenode(n)
	node *n;
{
	if (n != NULL) {
		freechildren(n);
		DEL(n);
	}
}
