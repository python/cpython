/* Grammar subroutines needed by parser */

#include "PROTO.h"
#define NULL 0
#include "assert.h"
#include "grammar.h"
#include "token.h"

/* Return the DFA for the given type */

dfa *
finddfa(g, type)
	grammar *g;
	register int type;
{
	register int i;
	register dfa *d;
	
	for (i = g->g_ndfas, d = g->g_dfa; --i >= 0; d++) {
		if (d->d_type == type)
			return d;
	}
	assert(0);
	/* NOTREACHED */
}

char *
labelrepr(lb)
	label *lb;
{
	static char buf[100];
	
	if (lb->lb_type == ENDMARKER)
		return "EMPTY";
	else if (ISNONTERMINAL(lb->lb_type)) {
		if (lb->lb_str == NULL) {
			sprintf(buf, "NT%d", lb->lb_type);
			return buf;
		}
		else
			return lb->lb_str;
	}
	else {
		if (lb->lb_str == NULL)
			return tok_name[lb->lb_type];
		else {
			sprintf(buf, "%.32s(%.32s)",
				tok_name[lb->lb_type], lb->lb_str);
			return buf;
		}
	}
}
