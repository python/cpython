
/* Parser accelerator module */

/* The parser as originally conceived had disappointing performance.
   This module does some precomputation that speeds up the selection
   of a DFA based upon a token, turning a search through an array
   into a simple indexing operation.  The parser now cannot work
   without the accelerators installed.  Note that the accelerators
   are installed dynamically when the parser is initialized, they
   are not part of the static data structure written on graminit.[ch]
   by the parser generator. */

#include "pgenheaders.h"
#include "grammar.h"
#include "node.h"
#include "token.h"
#include "parser.h"

/* Forward references */
static void fixdfa(grammar *, dfa *);
static void fixstate(grammar *, state *);

void
PyGrammar_AddAccelerators(grammar *g)
{
	dfa *d;
	int i;
	d = g->g_dfa;
	for (i = g->g_ndfas; --i >= 0; d++)
		fixdfa(g, d);
	g->g_accel = 1;
}

void
PyGrammar_RemoveAccelerators(grammar *g)
{
	dfa *d;
	int i;
	g->g_accel = 0;
	d = g->g_dfa;
	for (i = g->g_ndfas; --i >= 0; d++) {
		state *s;
		int j;
		s = d->d_state;
		for (j = 0; j < d->d_nstates; j++, s++) {
			if (s->s_accel)
				PyObject_FREE(s->s_accel);
			s->s_accel = NULL;
		}
	}
}

static void
fixdfa(grammar *g, dfa *d)
{
	state *s;
	int j;
	s = d->d_state;
	for (j = 0; j < d->d_nstates; j++, s++)
		fixstate(g, s);
}

static void
fixstate(grammar *g, state *s)
{
	arc *a;
	int k;
	int *accel;
	int nl = g->g_ll.ll_nlabels;
	s->s_accept = 0;
	accel = (int *) PyObject_MALLOC(nl * sizeof(int));
	if (accel == NULL) {
		fprintf(stderr, "no mem to build parser accelerators\n");
		exit(1);
	}
	for (k = 0; k < nl; k++)
		accel[k] = -1;
	a = s->s_arc;
	for (k = s->s_narcs; --k >= 0; a++) {
		int lbl = a->a_lbl;
		label *l = &g->g_ll.ll_label[lbl];
		int type = l->lb_type;
		if (a->a_arrow >= (1 << 7)) {
			printf("XXX too many states!\n");
			continue;
		}
		if (ISNONTERMINAL(type)) {
			dfa *d1 = PyGrammar_FindDFA(g, type);
			int ibit;
			if (type - NT_OFFSET >= (1 << 7)) {
				printf("XXX too high nonterminal number!\n");
				continue;
			}
			for (ibit = 0; ibit < g->g_ll.ll_nlabels; ibit++) {
				if (testbit(d1->d_first, ibit)) {
					if (accel[ibit] != -1)
						printf("XXX ambiguity!\n");
					accel[ibit] = a->a_arrow | (1 << 7) |
						((type - NT_OFFSET) << 8);
				}
			}
		}
		else if (lbl == EMPTY)
			s->s_accept = 1;
		else if (lbl >= 0 && lbl < nl)
			accel[lbl] = a->a_arrow;
	}
	while (nl > 0 && accel[nl-1] == -1)
		nl--;
	for (k = 0; k < nl && accel[k] == -1;)
		k++;
	if (k < nl) {
		int i;
		s->s_accel = (int *) PyObject_MALLOC((nl-k) * sizeof(int));
		if (s->s_accel == NULL) {
			fprintf(stderr, "no mem to add parser accelerators\n");
			exit(1);
		}
		s->s_lower = k;
		s->s_upper = nl;
		for (i = 0; k < nl; i++, k++)
			s->s_accel[i] = accel[k];
	}
	PyObject_FREE(accel);
}
