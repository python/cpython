#ifndef Py_GRAMMAR_H
#define Py_GRAMMAR_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.

******************************************************************/

/* Grammar interface */

#include "bitset.h" /* Sigh... */

/* A label of an arc */

typedef struct {
	int	lb_type;
	char	*lb_str;
} label;

#define EMPTY 0		/* Label number 0 is by definition the empty label */

/* A list of labels */

typedef struct {
	int	ll_nlabels;
	label	*ll_label;
} labellist;

/* An arc from one state to another */

typedef struct {
	short		a_lbl;		/* Label of this arc */
	short		a_arrow;	/* State where this arc goes to */
} arc;

/* A state in a DFA */

typedef struct {
	int		 s_narcs;
	arc		*s_arc;		/* Array of arcs */
	
	/* Optional accelerators */
	int		 s_lower;	/* Lowest label index */
	int		 s_upper;	/* Highest label index */
	int		*s_accel;	/* Accelerator */
	int		 s_accept;	/* Nonzero for accepting state */
} state;

/* A DFA */

typedef struct {
	int		 d_type;	/* Non-terminal this represents */
	char		*d_name;	/* For printing */
	int		 d_initial;	/* Initial state */
	int		 d_nstates;
	state		*d_state;	/* Array of states */
	bitset		 d_first;
} dfa;

/* A grammar */

typedef struct {
	int		 g_ndfas;
	dfa		*g_dfa;		/* Array of DFAs */
	labellist	 g_ll;
	int		 g_start;	/* Start symbol of the grammar */
	int		 g_accel;	/* Set if accelerators present */
} grammar;

/* FUNCTIONS */

grammar *newgrammar Py_PROTO((int start));
dfa *adddfa Py_PROTO((grammar *g, int type, char *name));
int addstate Py_PROTO((dfa *d));
void addarc Py_PROTO((dfa *d, int from, int to, int lbl));
dfa *PyGrammar_FindDFA Py_PROTO((grammar *g, int type));
char *typename Py_PROTO((grammar *g, int lbl));

int addlabel Py_PROTO((labellist *ll, int type, char *str));
int findlabel Py_PROTO((labellist *ll, int type, char *str));
char *PyGrammar_LabelRepr Py_PROTO((label *lb));
void translatelabels Py_PROTO((grammar *g));

void addfirstsets Py_PROTO((grammar *g));

void PyGrammar_AddAccelerators Py_PROTO((grammar *g));
void PyGrammar_RemoveAccelerators Py_PROTO((grammar *));

void printgrammar Py_PROTO((grammar *g, FILE *fp));
void printnonterminals Py_PROTO((grammar *g, FILE *fp));

#ifdef __cplusplus
}
#endif
#endif /* !Py_GRAMMAR_H */
