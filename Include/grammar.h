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

/* Grammar interface */

#include "bitset.h" /* Sigh... */

/* A label of an arc */

typedef struct _label {
	int	lb_type;
	char	*lb_str;
} label;

#define EMPTY 0		/* Label number 0 is by definition the empty label */

/* A list of labels */

typedef struct _labellist {
	int	ll_nlabels;
	label	*ll_label;
} labellist;

/* An arc from one state to another */

typedef struct _arc {
	short		a_lbl;		/* Label of this arc */
	short		a_arrow;	/* State where this arc goes to */
} arc;

/* A state in a DFA */

typedef struct _state {
	int		 s_narcs;
	arc		*s_arc;		/* Array of arcs */
	
	/* Optional accelerators */
	int		 s_lower;	/* Lowest label index */
	int		 s_upper;	/* Highest label index */
	int		*s_accel;	/* Accelerator */
	int		 s_accept;	/* Nonzero for accepting state */
} state;

/* A DFA */

typedef struct _dfa {
	int		 d_type;	/* Non-terminal this represents */
	char		*d_name;	/* For printing */
	int		 d_initial;	/* Initial state */
	int		 d_nstates;
	state		*d_state;	/* Array of states */
	bitset		 d_first;
} dfa;

/* A grammar */

typedef struct _grammar {
	int		 g_ndfas;
	dfa		*g_dfa;		/* Array of DFAs */
	labellist	 g_ll;
	int		 g_start;	/* Start symbol of the grammar */
	int		 g_accel;	/* Set if accelerators present */
} grammar;

/* FUNCTIONS */

grammar *newgrammar PROTO((int start));
dfa *adddfa PROTO((grammar *g, int type, char *name));
int addstate PROTO((dfa *d));
void addarc PROTO((dfa *d, int from, int to, int lbl));
dfa *finddfa PROTO((grammar *g, int type));
char *typename PROTO((grammar *g, int lbl));

int addlabel PROTO((labellist *ll, int type, char *str));
int findlabel PROTO((labellist *ll, int type, char *str));
char *labelrepr PROTO((label *lb));
void translatelabels PROTO((grammar *g));

void addfirstsets PROTO((grammar *g));

void addaccellerators PROTO((grammar *g));

void printgrammar PROTO((grammar *g, FILE *fp));
void printnonterminals PROTO((grammar *g, FILE *fp));
