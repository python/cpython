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

/* Grammar implementation */

#include "pgenheaders.h"

#include <ctype.h>

#include "assert.h"
#include "token.h"
#include "grammar.h"

extern int debugging;

grammar *
newgrammar(start)
	int start;
{
	grammar *g;
	
	g = NEW(grammar, 1);
	if (g == NULL)
		fatal("no mem for new grammar");
	g->g_ndfas = 0;
	g->g_dfa = NULL;
	g->g_start = start;
	g->g_ll.ll_nlabels = 0;
	g->g_ll.ll_label = NULL;
	return g;
}

dfa *
adddfa(g, type, name)
	grammar *g;
	int type;
	char *name;
{
	dfa *d;
	
	RESIZE(g->g_dfa, dfa, g->g_ndfas + 1);
	if (g->g_dfa == NULL)
		fatal("no mem to resize dfa in adddfa");
	d = &g->g_dfa[g->g_ndfas++];
	d->d_type = type;
	d->d_name = name;
	d->d_nstates = 0;
	d->d_state = NULL;
	d->d_initial = -1;
	d->d_first = NULL;
	return d; /* Only use while fresh! */
}

int
addstate(d)
	dfa *d;
{
	state *s;
	
	RESIZE(d->d_state, state, d->d_nstates + 1);
	if (d->d_state == NULL)
		fatal("no mem to resize state in addstate");
	s = &d->d_state[d->d_nstates++];
	s->s_narcs = 0;
	s->s_arc = NULL;
	return s - d->d_state;
}

void
addarc(d, from, to, lbl)
	dfa *d;
	int lbl;
{
	state *s;
	arc *a;
	
	assert(0 <= from && from < d->d_nstates);
	assert(0 <= to && to < d->d_nstates);
	
	s = &d->d_state[from];
	RESIZE(s->s_arc, arc, s->s_narcs + 1);
	if (s->s_arc == NULL)
		fatal("no mem to resize arc list in addarc");
	a = &s->s_arc[s->s_narcs++];
	a->a_lbl = lbl;
	a->a_arrow = to;
}

int
addlabel(ll, type, str)
	labellist *ll;
	int type;
	char *str;
{
	int i;
	label *lb;
	
	for (i = 0; i < ll->ll_nlabels; i++) {
		if (ll->ll_label[i].lb_type == type &&
			strcmp(ll->ll_label[i].lb_str, str) == 0)
			return i;
	}
	RESIZE(ll->ll_label, label, ll->ll_nlabels + 1);
	if (ll->ll_label == NULL)
		fatal("no mem to resize labellist in addlabel");
	lb = &ll->ll_label[ll->ll_nlabels++];
	lb->lb_type = type;
	lb->lb_str = str; /* XXX strdup(str) ??? */
	return lb - ll->ll_label;
}

/* Same, but rather dies than adds */

int
findlabel(ll, type, str)
	labellist *ll;
	int type;
	char *str;
{
	int i;
	label *lb;
	
	for (i = 0; i < ll->ll_nlabels; i++) {
		if (ll->ll_label[i].lb_type == type /*&&
			strcmp(ll->ll_label[i].lb_str, str) == 0*/)
			return i;
	}
	fprintf(stderr, "Label %d/'%s' not found\n", type, str);
	abort();
}

/* Forward */
static void translabel PROTO((grammar *, label *));

void
translatelabels(g)
	grammar *g;
{
	int i;
	
	printf("Translating labels ...\n");
	/* Don't translate EMPTY */
	for (i = EMPTY+1; i < g->g_ll.ll_nlabels; i++)
		translabel(g, &g->g_ll.ll_label[i]);
}

static void
translabel(g, lb)
	grammar *g;
	label *lb;
{
	int i;
	
	if (debugging)
		printf("Translating label %s ...\n", labelrepr(lb));
	
	if (lb->lb_type == NAME) {
		for (i = 0; i < g->g_ndfas; i++) {
			if (strcmp(lb->lb_str, g->g_dfa[i].d_name) == 0) {
				if (debugging)
					printf("Label %s is non-terminal %d.\n",
						lb->lb_str,
						g->g_dfa[i].d_type);
				lb->lb_type = g->g_dfa[i].d_type;
				lb->lb_str = NULL;
				return;
			}
		}
		for (i = 0; i < (int)N_TOKENS; i++) {
			if (strcmp(lb->lb_str, tok_name[i]) == 0) {
				if (debugging)
					printf("Label %s is terminal %d.\n",
						lb->lb_str, i);
				lb->lb_type = i;
				lb->lb_str = NULL;
				return;
			}
		}
		printf("Can't translate NAME label '%s'\n", lb->lb_str);
		return;
	}
	
	if (lb->lb_type == STRING) {
		if (isalpha(lb->lb_str[1])) {
			char *p, *strchr();
			if (debugging)
				printf("Label %s is a keyword\n", lb->lb_str);
			lb->lb_type = NAME;
			lb->lb_str++;
			p = strchr(lb->lb_str, '\'');
			if (p)
				*p = '\0';
		}
		else {
			if (lb->lb_str[2] == lb->lb_str[0]) {
				int type = (int) tok_1char(lb->lb_str[1]);
				if (type != OP) {
					lb->lb_type = type;
					lb->lb_str = NULL;
				}
				else
					printf("Unknown OP label %s\n",
						lb->lb_str);
			}
			else
				printf("Can't translate STRING label %s\n",
					lb->lb_str);
		}
	}
	else
		printf("Can't translate label '%s'\n", labelrepr(lb));
}
