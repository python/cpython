/* Parser interface */

#define MAXSTACK 100

typedef struct _stackentry {
	int		 s_state;	/* State in current DFA */
	dfa		*s_dfa;		/* Current DFA */
	struct _node	*s_parent;	/* Where to add next node */
} stackentry;

typedef struct _stack {
	stackentry	*s_top;		/* Top entry */
	stackentry	 s_base[MAXSTACK];/* Array of stack entries */
					/* NB The stack grows down */
} stack;

typedef struct {
	struct _stack	 p_stack;	/* Stack of parser states */
	struct _grammar	*p_grammar;	/* Grammar to use */
	struct _node	*p_tree;	/* Top of parse tree */
} parser_state;

parser_state *newparser PROTO((struct _grammar *g, int start));
void delparser PROTO((parser_state *ps));
int addtoken PROTO((parser_state *ps, int type, char *str, int lineno));
void addaccelerators PROTO((grammar *g));
