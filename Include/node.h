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

/* Parse tree node interface */

typedef struct _node {
	int		n_type;
	char		*n_str;
	int		n_lineno;
	int		n_nchildren;
	struct _node	*n_child;
} node;

extern node *newtree PROTO((int type));
extern node *addchild PROTO((node *n, int type, char *str, int lineno));
extern void freetree PROTO((node *n));

/* Node access functions */
#define NCH(n)		((n)->n_nchildren)
#define CHILD(n, i)	(&(n)->n_child[i])
#define TYPE(n)		((n)->n_type)
#define STR(n)		((n)->n_str)

/* Assert that the type of a node is what we expect */
#ifndef DEBUG
#define REQ(n, type) { /*pass*/ ; }
#else
#define REQ(n, type) \
	{ if (TYPE(n) != (type)) { \
		fprintf(stderr, "FATAL: node type %d, required %d\n", \
			TYPE(n), type); \
		abort(); \
	} }
#endif

extern void listtree PROTO((node *));
extern void listnode PROTO((FILE *, node *));
