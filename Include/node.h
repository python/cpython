
/* Parse tree node interface */

#ifndef Py_NODE_H
#define Py_NODE_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _node {
    short		n_type;
    char		*n_str;
    int			n_lineno;
    int			n_nchildren;
    struct _node	*n_child;
} node;

extern DL_IMPORT(node *) PyNode_New(int type);
extern DL_IMPORT(int) PyNode_AddChild(node *n, int type,
                                      char *str, int lineno);
extern DL_IMPORT(void) PyNode_Free(node *n);

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

extern DL_IMPORT(void) PyNode_ListTree(node *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_NODE_H */
