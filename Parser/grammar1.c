
/* Grammar subroutines needed by parser */

#include "pgenheaders.h"
#include "assert.h"
#include "grammar.h"
#include "token.h"

/* Return the DFA for the given type */

dfa *
PyGrammar_FindDFA(grammar *g, register int type)
{
	register dfa *d;
#if 1
	/* Massive speed-up */
	d = &g->g_dfa[type - NT_OFFSET];
	assert(d->d_type == type);
	return d;
#else
	/* Old, slow version */
	register int i;
	
	for (i = g->g_ndfas, d = g->g_dfa; --i >= 0; d++) {
		if (d->d_type == type)
			return d;
	}
	assert(0);
	/* NOTREACHED */
#endif
}

char *
PyGrammar_LabelRepr(label *lb)
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
			return _PyParser_TokenNames[lb->lb_type];
		else {
			sprintf(buf, "%.32s(%.32s)",
				_PyParser_TokenNames[lb->lb_type], lb->lb_str);
			return buf;
		}
	}
}
