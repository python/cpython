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

/* Print a bunch of C initializers that represent a grammar */

#include "pgenheaders.h"
#include "grammar.h"

/* Forward */
static void printarcs PROTO((int, dfa *, FILE *));
static void printstates PROTO((grammar *, FILE *));
static void printdfas PROTO((grammar *, FILE *));
static void printlabels PROTO((grammar *, FILE *));

void
printgrammar(g, fp)
	grammar *g;
	FILE *fp;
{
	fprintf(fp, "#include \"pgenheaders.h\"\n");
	fprintf(fp, "#include \"grammar.h\"\n");
	printdfas(g, fp);
	printlabels(g, fp);
	fprintf(fp, "grammar gram = {\n");
	fprintf(fp, "\t%d,\n", g->g_ndfas);
	fprintf(fp, "\tdfas,\n");
	fprintf(fp, "\t{%d, labels},\n", g->g_ll.ll_nlabels);
	fprintf(fp, "\t%d\n", g->g_start);
	fprintf(fp, "};\n");
}

void
printnonterminals(g, fp)
	grammar *g;
	FILE *fp;
{
	dfa *d;
	int i;
	
	d = g->g_dfa;
	for (i = g->g_ndfas; --i >= 0; d++)
		fprintf(fp, "#define %s %d\n", d->d_name, d->d_type);
}

static void
printarcs(i, d, fp)
	int i;
	dfa *d;
	FILE *fp;
{
	arc *a;
	state *s;
	int j, k;
	
	s = d->d_state;
	for (j = 0; j < d->d_nstates; j++, s++) {
		fprintf(fp, "static arc arcs_%d_%d[%d] = {\n",
			i, j, s->s_narcs);
		a = s->s_arc;
		for (k = 0; k < s->s_narcs; k++, a++)
			fprintf(fp, "\t{%d, %d},\n", a->a_lbl, a->a_arrow);
		fprintf(fp, "};\n");
	}
}

static void
printstates(g, fp)
	grammar *g;
	FILE *fp;
{
	state *s;
	dfa *d;
	int i, j;
	
	d = g->g_dfa;
	for (i = 0; i < g->g_ndfas; i++, d++) {
		printarcs(i, d, fp);
		fprintf(fp, "static state states_%d[%d] = {\n",
			i, d->d_nstates);
		s = d->d_state;
		for (j = 0; j < d->d_nstates; j++, s++)
			fprintf(fp, "\t{%d, arcs_%d_%d},\n",
				s->s_narcs, i, j);
		fprintf(fp, "};\n");
	}
}

static void
printdfas(g, fp)
	grammar *g;
	FILE *fp;
{
	dfa *d;
	int i, j;
	
	printstates(g, fp);
	fprintf(fp, "static dfa dfas[%d] = {\n", g->g_ndfas);
	d = g->g_dfa;
	for (i = 0; i < g->g_ndfas; i++, d++) {
		fprintf(fp, "\t{%d, \"%s\", %d, %d, states_%d,\n",
			d->d_type, d->d_name, d->d_initial, d->d_nstates, i);
		fprintf(fp, "\t \"");
		for (j = 0; j < NBYTES(g->g_ll.ll_nlabels); j++)
			fprintf(fp, "\\%03o", d->d_first[j] & 0xff);
		fprintf(fp, "\"},\n");
	}
	fprintf(fp, "};\n");
}

static void
printlabels(g, fp)
	grammar *g;
	FILE *fp;
{
	label *l;
	int i;
	
	fprintf(fp, "static label labels[%d] = {\n", g->g_ll.ll_nlabels);
	l = g->g_ll.ll_label;
	for (i = g->g_ll.ll_nlabels; --i >= 0; l++) {
		if (l->lb_str == NULL)
			fprintf(fp, "\t{%d, 0},\n", l->lb_type);
		else
			fprintf(fp, "\t{%d, \"%s\"},\n",
				l->lb_type, l->lb_str);
	}
	fprintf(fp, "};\n");
}
