#ifndef Py_PARSER_H
#define Py_PARSER_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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

/* Parser interface */

#define MAXSTACK 500

typedef struct {
	int		 s_state;	/* State in current DFA */
	dfa		*s_dfa;		/* Current DFA */
	struct _node	*s_parent;	/* Where to add next node */
} stackentry;

typedef struct {
	stackentry	*s_top;		/* Top entry */
	stackentry	 s_base[MAXSTACK];/* Array of stack entries */
					/* NB The stack grows down */
} stack;

typedef struct {
	stack	 	p_stack;	/* Stack of parser states */
	grammar		*p_grammar;	/* Grammar to use */
	node		*p_tree;	/* Top of parse tree */
} parser_state;

parser_state *newparser PROTO((grammar *g, int start));
void delparser PROTO((parser_state *ps));
int addtoken PROTO((parser_state *ps, int type, char *str, int lineno));
void addaccelerators PROTO((grammar *g));

#ifdef __cplusplus
}
#endif
#endif /* !Py_PARSER_H */
