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
