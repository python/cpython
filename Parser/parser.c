
/* Parser implementation */

/* For a description, see the comments at end of this file */

/* XXX To do: error recovery */

#include "Python.h"
#include "pgenheaders.h"
#include "token.h"
#include "grammar.h"
#include "node.h"
#include "parser.h"
#include "errcode.h"


#ifdef Py_DEBUG
extern int Py_DebugFlag;
#define D(x) if (!Py_DebugFlag); else x
#else
#define D(x)
#endif


/* STACK DATA TYPE */

static void s_reset(stack *);

static void
s_reset(stack *s)
{
	s->s_top = &s->s_base[MAXSTACK];
}

#define s_empty(s) ((s)->s_top == &(s)->s_base[MAXSTACK])

static int
s_push(register stack *s, dfa *d, node *parent)
{
	register stackentry *top;
	if (s->s_top == s->s_base) {
		fprintf(stderr, "s_push: parser stack overflow\n");
		return E_NOMEM;
	}
	top = --s->s_top;
	top->s_dfa = d;
	top->s_parent = parent;
	top->s_state = 0;
	return 0;
}

#ifdef Py_DEBUG

static void
s_pop(register stack *s)
{
	if (s_empty(s))
		Py_FatalError("s_pop: parser stack underflow -- FATAL");
	s->s_top++;
}

#else /* !Py_DEBUG */

#define s_pop(s) (s)->s_top++

#endif


/* PARSER CREATION */

parser_state *
PyParser_New(grammar *g, int start)
{
	parser_state *ps;
	
	if (!g->g_accel)
		PyGrammar_AddAccelerators(g);
	ps = (parser_state *)PyMem_MALLOC(sizeof(parser_state));
	if (ps == NULL)
		return NULL;
	ps->p_grammar = g;
#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
	ps->p_flags = 0;
#endif
	ps->p_tree = PyNode_New(start);
	if (ps->p_tree == NULL) {
		PyMem_FREE(ps);
		return NULL;
	}
	s_reset(&ps->p_stack);
	(void) s_push(&ps->p_stack, PyGrammar_FindDFA(g, start), ps->p_tree);
	return ps;
}

void
PyParser_Delete(parser_state *ps)
{
	/* NB If you want to save the parse tree,
	   you must set p_tree to NULL before calling delparser! */
	PyNode_Free(ps->p_tree);
	PyMem_FREE(ps);
}


/* PARSER STACK OPERATIONS */

static int
shift(register stack *s, int type, char *str, int newstate, int lineno, int col_offset)
{
	int err;
	assert(!s_empty(s));
	err = PyNode_AddChild(s->s_top->s_parent, type, str, lineno, col_offset);
	if (err)
		return err;
	s->s_top->s_state = newstate;
	return 0;
}

static int
push(register stack *s, int type, dfa *d, int newstate, int lineno, int col_offset)
{
	int err;
	register node *n;
	n = s->s_top->s_parent;
	assert(!s_empty(s));
	err = PyNode_AddChild(n, type, (char *)NULL, lineno, col_offset);
	if (err)
		return err;
	s->s_top->s_state = newstate;
	return s_push(s, d, CHILD(n, NCH(n)-1));
}


/* PARSER PROPER */

static int
classify(parser_state *ps, int type, char *str)
{
	grammar *g = ps->p_grammar;
	register int n = g->g_ll.ll_nlabels;
	
	if (type == NAME) {
		register char *s = str;
		register label *l = g->g_ll.ll_label;
		register int i;
		for (i = n; i > 0; i--, l++) {
			if (l->lb_type != NAME || l->lb_str == NULL ||
			    l->lb_str[0] != s[0] ||
			    strcmp(l->lb_str, s) != 0)
				continue;
#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
                        /* Leaving this in as an example */
			if (!(ps->p_flags & CO_FUTURE_WITH_STATEMENT)) {
				if (s[0] == 'w' && strcmp(s, "with") == 0)
					break; /* not a keyword yet */
				else if (s[0] == 'a' && strcmp(s, "as") == 0)
					break; /* not a keyword yet */
			}
#endif
			D(printf("It's a keyword\n"));
			return n - i;
		}
	}
	
	{
		register label *l = g->g_ll.ll_label;
		register int i;
		for (i = n; i > 0; i--, l++) {
			if (l->lb_type == type && l->lb_str == NULL) {
				D(printf("It's a token we know\n"));
				return n - i;
			}
		}
	}
	
	D(printf("Illegal token\n"));
	return -1;
}

#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
/* Leaving this in as an example */
static void
future_hack(parser_state *ps)
{
	node *n = ps->p_stack.s_top->s_parent;
	node *ch, *cch;
	int i;

	/* from __future__ import ..., must have at least 4 children */
	n = CHILD(n, 0);
	if (NCH(n) < 4)
		return;
	ch = CHILD(n, 0);
	if (STR(ch) == NULL || strcmp(STR(ch), "from") != 0)
		return;
	ch = CHILD(n, 1);
	if (NCH(ch) == 1 && STR(CHILD(ch, 0)) &&
	    strcmp(STR(CHILD(ch, 0)), "__future__") != 0)
		return;
	ch = CHILD(n, 3);
	/* ch can be a star, a parenthesis or import_as_names */
	if (TYPE(ch) == STAR)
		return;
	if (TYPE(ch) == LPAR)
		ch = CHILD(n, 4);
	
	for (i = 0; i < NCH(ch); i += 2) {
		cch = CHILD(ch, i);
		if (NCH(cch) >= 1 && TYPE(CHILD(cch, 0)) == NAME) {
			char *str_ch = STR(CHILD(cch, 0));
			if (strcmp(str_ch, FUTURE_WITH_STATEMENT) == 0) {
				ps->p_flags |= CO_FUTURE_WITH_STATEMENT;
				break;
			} else if (strcmp(str_ch, FUTURE_PRINT_FUNCTION) == 0) {
				ps->p_flags |= CO_FUTURE_PRINT_FUNCTION;
				break;
			} else if (strcmp(str_ch, FUTURE_UNICODE_LITERALS) == 0) {
				ps->p_flags |= CO_FUTURE_UNICODE_LITERALS;
				break;
			}
		}
	}
}
#endif /* future keyword */

int
PyParser_AddToken(register parser_state *ps, register int type, char *str,
	          int lineno, int col_offset, int *expected_ret)
{
	register int ilabel;
	int err;
	
	D(printf("Token %s/'%s' ... ", _PyParser_TokenNames[type], str));
	
	/* Find out which label this token is */
	ilabel = classify(ps, type, str);
	if (ilabel < 0)
		return E_SYNTAX;
	
	/* Loop until the token is shifted or an error occurred */
	for (;;) {
		/* Fetch the current dfa and state */
		register dfa *d = ps->p_stack.s_top->s_dfa;
		register state *s = &d->d_state[ps->p_stack.s_top->s_state];
		
		D(printf(" DFA '%s', state %d:",
			d->d_name, ps->p_stack.s_top->s_state));
		
		/* Check accelerator */
		if (s->s_lower <= ilabel && ilabel < s->s_upper) {
			register int x = s->s_accel[ilabel - s->s_lower];
			if (x != -1) {
				if (x & (1<<7)) {
					/* Push non-terminal */
					int nt = (x >> 8) + NT_OFFSET;
					int arrow = x & ((1<<7)-1);
					dfa *d1 = PyGrammar_FindDFA(
						ps->p_grammar, nt);
					if ((err = push(&ps->p_stack, nt, d1,
						arrow, lineno, col_offset)) > 0) {
						D(printf(" MemError: push\n"));
						return err;
					}
					D(printf(" Push ...\n"));
					continue;
				}
				
				/* Shift the token */
				if ((err = shift(&ps->p_stack, type, str,
						x, lineno, col_offset)) > 0) {
					D(printf(" MemError: shift.\n"));
					return err;
				}
				D(printf(" Shift.\n"));
				/* Pop while we are in an accept-only state */
				while (s = &d->d_state
						[ps->p_stack.s_top->s_state],
					s->s_accept && s->s_narcs == 1) {
					D(printf("  DFA '%s', state %d: "
						 "Direct pop.\n",
						 d->d_name,
						 ps->p_stack.s_top->s_state));
#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
					if (d->d_name[0] == 'i' &&
					    strcmp(d->d_name,
						   "import_stmt") == 0)
						future_hack(ps);
#endif
					s_pop(&ps->p_stack);
					if (s_empty(&ps->p_stack)) {
						D(printf("  ACCEPT.\n"));
						return E_DONE;
					}
					d = ps->p_stack.s_top->s_dfa;
				}
				return E_OK;
			}
		}
		
		if (s->s_accept) {
#ifdef PY_PARSER_REQUIRES_FUTURE_KEYWORD
			if (d->d_name[0] == 'i' &&
			    strcmp(d->d_name, "import_stmt") == 0)
				future_hack(ps);
#endif
			/* Pop this dfa and try again */
			s_pop(&ps->p_stack);
			D(printf(" Pop ...\n"));
			if (s_empty(&ps->p_stack)) {
				D(printf(" Error: bottom of stack.\n"));
				return E_SYNTAX;
			}
			continue;
		}
		
		/* Stuck, report syntax error */
		D(printf(" Error.\n"));
		if (expected_ret) {
			if (s->s_lower == s->s_upper - 1) {
				/* Only one possible expected token */
				*expected_ret = ps->p_grammar->
				    g_ll.ll_label[s->s_lower].lb_type;
			}
			else 
		        	*expected_ret = -1;
		}
		return E_SYNTAX;
	}
}


#ifdef Py_DEBUG

/* DEBUG OUTPUT */

void
dumptree(grammar *g, node *n)
{
	int i;
	
	if (n == NULL)
		printf("NIL");
	else {
		label l;
		l.lb_type = TYPE(n);
		l.lb_str = STR(n);
		printf("%s", PyGrammar_LabelRepr(&l));
		if (ISNONTERMINAL(TYPE(n))) {
			printf("(");
			for (i = 0; i < NCH(n); i++) {
				if (i > 0)
					printf(",");
				dumptree(g, CHILD(n, i));
			}
			printf(")");
		}
	}
}

void
showtree(grammar *g, node *n)
{
	int i;
	
	if (n == NULL)
		return;
	if (ISNONTERMINAL(TYPE(n))) {
		for (i = 0; i < NCH(n); i++)
			showtree(g, CHILD(n, i));
	}
	else if (ISTERMINAL(TYPE(n))) {
		printf("%s", _PyParser_TokenNames[TYPE(n)]);
		if (TYPE(n) == NUMBER || TYPE(n) == NAME)
			printf("(%s)", STR(n));
		printf(" ");
	}
	else
		printf("? ");
}

void
printtree(parser_state *ps)
{
	if (Py_DebugFlag) {
		printf("Parse tree:\n");
		dumptree(ps->p_grammar, ps->p_tree);
		printf("\n");
		printf("Tokens:\n");
		showtree(ps->p_grammar, ps->p_tree);
		printf("\n");
	}
	printf("Listing:\n");
	PyNode_ListTree(ps->p_tree);
	printf("\n");
}

#endif /* Py_DEBUG */

/*

Description
-----------

The parser's interface is different than usual: the function addtoken()
must be called for each token in the input.  This makes it possible to
turn it into an incremental parsing system later.  The parsing system
constructs a parse tree as it goes.

A parsing rule is represented as a Deterministic Finite-state Automaton
(DFA).  A node in a DFA represents a state of the parser; an arc represents
a transition.  Transitions are either labeled with terminal symbols or
with non-terminals.  When the parser decides to follow an arc labeled
with a non-terminal, it is invoked recursively with the DFA representing
the parsing rule for that as its initial state; when that DFA accepts,
the parser that invoked it continues.  The parse tree constructed by the
recursively called parser is inserted as a child in the current parse tree.

The DFA's can be constructed automatically from a more conventional
language description.  An extended LL(1) grammar (ELL(1)) is suitable.
Certain restrictions make the parser's life easier: rules that can produce
the empty string should be outlawed (there are other ways to put loops
or optional parts in the language).  To avoid the need to construct
FIRST sets, we can require that all but the last alternative of a rule
(really: arc going out of a DFA's state) must begin with a terminal
symbol.

As an example, consider this grammar:

expr:	term (OP term)*
term:	CONSTANT | '(' expr ')'

The DFA corresponding to the rule for expr is:

------->.---term-->.------->
	^          |
	|          |
	\----OP----/

The parse tree generated for the input a+b is:

(expr: (term: (NAME: a)), (OP: +), (term: (NAME: b)))

*/
