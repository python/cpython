/* Parse tree node interface */

typedef struct _node {
	int		n_type;
	char		*n_str;
	int		n_nchildren;
	struct _node	*n_child;
} node;

extern node *newnode PROTO((int type));
extern node *addchild PROTO((node *n, int type, char *str));
extern void freenode PROTO((node *n));

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
