#ifndef Py_NODE_H
#define Py_NODE_H
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

/* Parse tree node interface */

typedef struct _node {
	short		n_type;
	char		*n_str;
	short		n_lineno;
	short		n_nchildren;
	struct _node	*n_child;
} node;

extern DL_IMPORT(node *) PyNode_New Py_PROTO((int type));
extern DL_IMPORT(node *) PyNode_AddChild Py_PROTO((node *n, int type, char *str, int lineno));
extern DL_IMPORT(void) PyNode_Free Py_PROTO((node *n));

/* Node access functions */
#define NCH(n)		((n)->n_nchildren)
#define CHILD(n, i)	(&(n)->n_child[i])
#define TYPE(n)		((n)->n_type)
#define STR(n)		((n)->n_str)

/* Assert that the type of a node is what we expect */
#ifndef Py_DEBUG
#define REQ(n, type) { /*pass*/ ; }
#else
#define REQ(n, type) \
	{ if (TYPE(n) != (type)) { \
		fprintf(stderr, "FATAL: node type %d, required %d\n", \
			TYPE(n), type); \
		abort(); \
	} }
#endif

extern DL_IMPORT(void) PyNode_ListTree Py_PROTO((node *));

#ifdef __cplusplus
}
#endif
#endif /* !Py_NODE_H */
