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

/* Computation of FIRST stets */

#include "pgenheaders.h"
#include "grammar.h"
#include "token.h"

extern int Py_DebugFlag;

/* Forward */
static void calcfirstset Py_PROTO((grammar *, dfa *));

void
addfirstsets(g)
	grammar *g;
{
	int i;
	dfa *d;
	
	printf("Adding FIRST sets ...\n");
	for (i = 0; i < g->g_ndfas; i++) {
		d = &g->g_dfa[i];
		if (d->d_first == NULL)
			calcfirstset(g, d);
	}
}

static void
calcfirstset(g, d)
	grammar *g;
	dfa *d;
{
	int i, j;
	state *s;
	arc *a;
	int nsyms;
	int *sym;
	int nbits;
	static bitset dummy;
	bitset result;
	int type;
	dfa *d1;
	label *l0;
	
	if (Py_DebugFlag)
		printf("Calculate FIRST set for '%s'\n", d->d_name);
	
	if (dummy == NULL)
		dummy = newbitset(1);
	if (d->d_first == dummy) {
		fprintf(stderr, "Left-recursion for '%s'\n", d->d_name);
		return;
	}
	if (d->d_first != NULL) {
		fprintf(stderr, "Re-calculating FIRST set for '%s' ???\n",
			d->d_name);
	}
	d->d_first = dummy;
	
	l0 = g->g_ll.ll_label;
	nbits = g->g_ll.ll_nlabels;
	result = newbitset(nbits);
	
	sym = PyMem_NEW(int, 1);
	if (sym == NULL)
		Py_FatalError("no mem for new sym in calcfirstset");
	nsyms = 1;
	sym[0] = findlabel(&g->g_ll, d->d_type, (char *)NULL);
	
	s = &d->d_state[d->d_initial];
	for (i = 0; i < s->s_narcs; i++) {
		a = &s->s_arc[i];
		for (j = 0; j < nsyms; j++) {
			if (sym[j] == a->a_lbl)
				break;
		}
		if (j >= nsyms) { /* New label */
			PyMem_RESIZE(sym, int, nsyms + 1);
			if (sym == NULL)
				Py_FatalError(
				    "no mem to resize sym in calcfirstset");
			sym[nsyms++] = a->a_lbl;
			type = l0[a->a_lbl].lb_type;
			if (ISNONTERMINAL(type)) {
				d1 = PyGrammar_FindDFA(g, type);
				if (d1->d_first == dummy) {
					fprintf(stderr,
						"Left-recursion below '%s'\n",
						d->d_name);
				}
				else {
					if (d1->d_first == NULL)
						calcfirstset(g, d1);
					mergebitset(result,
						    d1->d_first, nbits);
				}
			}
			else if (ISTERMINAL(type)) {
				addbit(result, a->a_lbl);
			}
		}
	}
	d->d_first = result;
	if (Py_DebugFlag) {
		printf("FIRST set for '%s': {", d->d_name);
		for (i = 0; i < nbits; i++) {
			if (testbit(result, i))
				printf(" %s", PyGrammar_LabelRepr(&l0[i]));
		}
		printf(" }\n");
	}
}
