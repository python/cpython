/* regexpr.c
 *
 * Author: Tatu Ylonen <ylo@ngs.fi>
 *
 * Copyright (c) 1991 Tatu Ylonen, Espoo, Finland
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies.
 * This software is provided "as is" without express or implied
 * warranty.
 *
 * Created: Thu Sep 26 17:14:05 1991 ylo
 * Last modified: Mon Nov  4 17:06:48 1991 ylo
 * Ported to Think C: 19 Jan 1992 guido@cwi.nl
 *
 * This code draws many ideas from the regular expression packages by
 * Henry Spencer of the University of Toronto and Richard Stallman of
 * the Free Software Foundation.
 *
 * Emacs-specific code and syntax table code is almost directly borrowed
 * from GNU regexp.
 *
 * Bugs fixed and lots of reorganization by Jeffrey C. Ollie, April
 * 1997 Thanks for bug reports and ideas from Andrew Kuchling, Tim
 * Peters, Guido van Rossum, Ka-Ping Yee, Sjoerd Mullender, and
 * probably one or two others that I'm forgetting.
 *
 * $Id$ */

#include "Python.h"
#include "regexpr.h"
#include <assert.h>

/* The original code blithely assumed that sizeof(short) == 2.  Not
 * always true.  Original instances of "(short)x" were replaced by
 * SHORT(x), where SHORT is #defined below.  */

#define SHORT(x) ((x) & 0x8000 ? (x) - 0x10000 : (x))

/* The stack implementation is taken from an idea by Andrew Kuchling.
 * It's a doubly linked list of arrays. The advantages of this over a
 * simple linked list are that the number of mallocs required are
 * reduced. It also makes it possible to statically allocate enough
 * space so that small patterns don't ever need to call malloc.
 *
 * The advantages over a single array is that is periodically
 * realloced when more space is needed is that we avoid ever copying
 * the stack. */

/* item_t is the basic stack element.  Defined as a union of
 * structures so that both registers, failure points, and counters can
 * be pushed/popped from the stack.  There's nothing built into the
 * item to keep track of whether a certain stack item is a register, a
 * failure point, or a counter. */

typedef union item_t
{
	struct
	{
		int num;
		int level;
		unsigned char *start;
		unsigned char *end;
	} reg;
	struct
	{
		int count;
		int level;
		int phantom;
		unsigned char *code;
		unsigned char *text;
	} fail;
	struct
	{
		int num;
		int level;
		int count;
	} cntr;
} item_t;

#define STACK_PAGE_SIZE 256
#define NUM_REGISTERS 256

/* A 'page' of stack items. */

typedef struct item_page_t
{
	item_t items[STACK_PAGE_SIZE];
	struct item_page_t *prev;
	struct item_page_t *next;
} item_page_t;


typedef struct match_state
{
	/* The number of registers that have been pushed onto the stack
	 * since the last failure point. */

	int count;

	/* Used to control when registers need to be pushed onto the
	 * stack. */
	
	int level;
	
	/* The number of failure points on the stack. */
	
	int point;
	
	/* Storage for the registers.  Each register consists of two
	 * pointers to characters.  So register N is represented as
	 * start[N] and end[N].  The pointers must be converted to
	 * offsets from the beginning of the string before returning the
	 * registers to the calling program. */
	
	unsigned char *start[NUM_REGISTERS];
	unsigned char *end[NUM_REGISTERS];
	
	/* Keeps track of whether a register has changed recently. */
	
	int changed[NUM_REGISTERS];
	
	/* Structure to encapsulate the stack. */
	struct
	{
		/* index into the curent page.  If index == 0 and you need
		 * to pop an item, move to the previous page and set index
		 * = STACK_PAGE_SIZE - 1.  Otherwise decrement index to
		 * push a page. If index == STACK_PAGE_SIZE and you need
		 * to push a page move to the next page and set index =
		 * 0. If there is no new next page, allocate a new page
		 * and link it in. Otherwise, increment index to push a
		 * page. */
		
		int index;
		item_page_t *current; /* Pointer to the current page. */
		item_page_t first; /* First page is statically allocated. */
	} stack;
} match_state;

/* Initialize a state object */

/* #define NEW_STATE(state) \ */
/* memset(&state, 0, (void *)(&state.stack) - (void *)(&state)); \ */
/* state.stack.current = &state.stack.first; \ */
/* state.stack.first.prev = NULL; \ */
/* state.stack.first.next = NULL; \ */
/* state.stack.index = 0; \ */
/* state.level = 1 */

#define NEW_STATE(state, nregs) \
{ \
	int i; \
	for (i = 0; i < nregs; i++) \
	{ \
		state.start[i] = NULL; \
		state.end[i] = NULL; \
		state.changed[i] = 0; \
	} \
	state.stack.current = &state.stack.first; \
	state.stack.first.prev = NULL; \
	state.stack.first.next = NULL; \
	state.stack.index = 0; \
	state.level = 1; \
	state.count = 0; \
	state.level = 0; \
	state.point = 0; \
}

/* Free any memory that might have been malloc'd */

#define FREE_STATE(state) \
while(state.stack.first.next != NULL) \
{ \
	state.stack.current = state.stack.first.next; \
	state.stack.first.next = state.stack.current->next; \
	free(state.stack.current); \
}

/* Discard the top 'count' stack items. */

#define STACK_DISCARD(stack, count, on_error) \
stack.index -= count; \
while (stack.index < 0) \
{ \
	if (stack.current->prev == NULL) \
		on_error; \
	stack.current = stack.current->prev; \
	stack.index += STACK_PAGE_SIZE; \
}

/* Store a pointer to the previous item on the stack. Used to pop an
 * item off of the stack. */

#define STACK_PREV(stack, top, on_error) \
if (stack.index == 0) \
{ \
	if (stack.current->prev == NULL) \
		on_error; \
	stack.current = stack.current->prev; \
	stack.index = STACK_PAGE_SIZE - 1; \
} \
else \
{ \
	stack.index--; \
} \
top = &(stack.current->items[stack.index])

/* Store a pointer to the next item on the stack. Used to push an item
 * on to the stack. */

#define STACK_NEXT(stack, top, on_error) \
if (stack.index == STACK_PAGE_SIZE) \
{ \
	if (stack.current->next == NULL) \
	{ \
		stack.current->next = (item_page_t *)malloc(sizeof(item_page_t)); \
		if (stack.current->next == NULL) \
			on_error; \
		stack.current->next->prev = stack.current; \
		stack.current->next->next = NULL; \
	} \
	stack.current = stack.current->next; \
	stack.index = 0; \
} \
top = &(stack.current->items[stack.index++])

/* Store a pointer to the item that is 'count' items back in the
 * stack. STACK_BACK(stack, top, 1, on_error) is equivalent to
 * STACK_TOP(stack, top, on_error).  */

#define STACK_BACK(stack, top, count, on_error) \
{ \
	int index; \
	item_page_t *current; \
	current = stack.current; \
	index = stack.index - (count); \
	while (index < 0) \
	{ \
		if (current->prev == NULL) \
			on_error; \
		current = current->prev; \
		index += STACK_PAGE_SIZE; \
	} \
	top = &(current->items[index]); \
}

/* Store a pointer to the top item on the stack. Execute the
 * 'on_error' code if there are no items on the stack. */

#define STACK_TOP(stack, top, on_error) \
if (stack.index == 0) \
{ \
	if (stack.current->prev == NULL) \
		on_error; \
	top = &(stack.current->prev->items[STACK_PAGE_SIZE - 1]); \
} \
else \
{ \
	top = &(stack.current->items[stack.index - 1]); \
}

/* Test to see if the stack is empty */

#define STACK_EMPTY(stack) ((stack.index == 0) && \
			    (stack.current->prev == NULL))

/* Return the start of register 'reg' */

#define GET_REG_START(state, reg) (state.start[reg])

/* Return the end of register 'reg' */

#define GET_REG_END(state, reg) (state.end[reg])

/* Set the start of register 'reg'. If the state of the register needs
 * saving, push it on the stack. */

#define SET_REG_START(state, reg, text, on_error) \
if(state.changed[reg] < state.level) \
{ \
	item_t *item; \
	STACK_NEXT(state.stack, item, on_error); \
	item->reg.num = reg; \
	item->reg.start = state.start[reg]; \
	item->reg.end = state.end[reg]; \
	item->reg.level = state.changed[reg]; \
	state.changed[reg] = state.level; \
	state.count++; \
} \
state.start[reg] = text

/* Set the end of register 'reg'. If the state of the register needs
 * saving, push it on the stack. */

#define SET_REG_END(state, reg, text, on_error) \
if(state.changed[reg] < state.level) \
{ \
	item_t *item; \
	STACK_NEXT(state.stack, item, on_error); \
	item->reg.num = reg; \
	item->reg.start = state.start[reg]; \
	item->reg.end = state.end[reg]; \
	item->reg.level = state.changed[reg]; \
	state.changed[reg] = state.level; \
	state.count++; \
} \
state.end[reg] = text

#define PUSH_FAILURE(state, xcode, xtext, on_error) \
{ \
	item_t *item; \
	STACK_NEXT(state.stack, item, on_error); \
	item->fail.code = xcode; \
	item->fail.text = xtext; \
	item->fail.count = state.count; \
	item->fail.level = state.level; \
	item->fail.phantom = 0; \
	state.count = 0; \
	state.level++; \
	state.point++; \
}

/* Update the last failure point with a new position in the text. */

#define UPDATE_FAILURE(state, xtext, on_error) \
{ \
	item_t *item; \
	STACK_BACK(state.stack, item, state.count + 1, on_error); \
	if (!item->fail.phantom) \
	{ \
		item_t *item2; \
		STACK_NEXT(state.stack, item2, on_error); \
		item2->fail.code = item->fail.code; \
		item2->fail.text = xtext; \
		item2->fail.count = state.count; \
		item2->fail.level = state.level; \
		item2->fail.phantom = 1; \
		state.count = 0; \
		state.level++; \
		state.point++; \
	} \
	else \
	{ \
		STACK_DISCARD(state.stack, state.count, on_error); \
		STACK_TOP(state.stack, item, on_error); \
		item->fail.text = xtext; \
		state.count = 0; \
		state.level++; \
	} \
}

#define POP_FAILURE(state, xcode, xtext, on_empty, on_error) \
{ \
	item_t *item; \
	do \
	{ \
		while(state.count > 0) \
		{ \
			STACK_PREV(state.stack, item, on_error); \
			state.start[item->reg.num] = item->reg.start; \
			state.end[item->reg.num] = item->reg.end; \
			state.changed[item->reg.num] = item->reg.level; \
			state.count--; \
		} \
		STACK_PREV(state.stack, item, on_empty); \
		xcode = item->fail.code; \
		xtext = item->fail.text; \
		state.count = item->fail.count; \
		state.level = item->fail.level; \
		state.point--; \
	} \
	while (item->fail.text == NULL); \
}

enum regexp_compiled_ops /* opcodes for compiled regexp */
{
	Cend,		      /* end of pattern reached */
	Cbol,		      /* beginning of line */
	Ceol,		      /* end of line */
	Cset,		      /* character set.  Followed by 32 bytes of set. */
	Cexact,		      /* followed by a byte to match */
	Canychar,	      /* matches any character except newline */
	Cstart_memory,	      /* set register start addr (followed by reg number) */
	Cend_memory,	      /* set register end addr (followed by reg number) */
	Cmatch_memory,	      /* match a duplicate of reg contents (regnum follows)*/
	Cjump,		      /* followed by two bytes (lsb,msb) of displacement. */
	Cstar_jump,	      /* will change to jump/update_failure_jump at runtime */
	Cfailure_jump,	      /* jump to addr on failure */
	Cupdate_failure_jump, /* update topmost failure point and jump */
	Cdummy_failure_jump,  /* push a dummy failure point and jump */
	Cbegbuf,	      /* match at beginning of buffer */
	Cendbuf,	      /* match at end of buffer */
	Cwordbeg,	      /* match at beginning of word */
	Cwordend,	      /* match at end of word */
	Cwordbound,	      /* match if at word boundary */
	Cnotwordbound,        /* match if not at word boundary */
	Csyntaxspec,	      /* matches syntax code (1 byte follows) */
	Cnotsyntaxspec,       /* matches if syntax code does not match (1 byte follows) */
	Crepeat1
};

enum regexp_syntax_op	/* syntax codes for plain and quoted characters */
{
	Rend,		  /* special code for end of regexp */
	Rnormal,	  /* normal character */
	Ranychar,	  /* any character except newline */
	Rquote,		  /* the quote character */
	Rbol,		  /* match beginning of line */
	Reol,		  /* match end of line */
	Roptional,	  /* match preceding expression optionally */
	Rstar,		  /* match preceding expr zero or more times */
	Rplus,		  /* match preceding expr one or more times */
	Ror,		  /* match either of alternatives */
	Ropenpar,	  /* opening parenthesis */
	Rclosepar,	  /* closing parenthesis */
	Rmemory,	  /* match memory register */
	Rextended_memory, /* \vnn to match registers 10-99 */
	Ropenset,	  /* open set.  Internal syntax hard-coded below. */
	/* the following are gnu extensions to "normal" regexp syntax */
	Rbegbuf,	  /* beginning of buffer */
	Rendbuf,	  /* end of buffer */
	Rwordchar,	  /* word character */
	Rnotwordchar,	  /* not word character */
	Rwordbeg,	  /* beginning of word */
	Rwordend,	  /* end of word */
	Rwordbound,	  /* word bound */
	Rnotwordbound,	  /* not word bound */
	Rnum_ops
};

static int re_compile_initialized = 0;
static int regexp_syntax = 0;
int re_syntax = 0; /* Exported copy of regexp_syntax */
static unsigned char regexp_plain_ops[256];
static unsigned char regexp_quoted_ops[256];
static unsigned char regexp_precedences[Rnum_ops];
static int regexp_context_indep_ops;
static int regexp_ansi_sequences;

#define NUM_LEVELS  5    /* number of precedence levels in use */
#define MAX_NESTING 100  /* max nesting level of operators */

#define SYNTAX(ch) re_syntax_table[(unsigned char)(ch)]

unsigned char re_syntax_table[256];

void re_compile_initialize()
{
	int a;
  
	static int syntax_table_inited = 0;

	if (!syntax_table_inited)
	{
		syntax_table_inited = 1;
		memset(re_syntax_table, 0, 256);
		for (a = 'a'; a <= 'z'; a++)
			re_syntax_table[a] = Sword;
		for (a = 'A'; a <= 'Z'; a++)
			re_syntax_table[a] = Sword;
		for (a = '0'; a <= '9'; a++)
			re_syntax_table[a] = Sword | Sdigit | Shexdigit;
		for (a = '0'; a <= '7'; a++)
			re_syntax_table[a] |= Soctaldigit;
		for (a = 'A'; a <= 'F'; a++)
			re_syntax_table[a] |= Shexdigit;
		for (a = 'a'; a <= 'f'; a++)
			re_syntax_table[a] |= Shexdigit;
		re_syntax_table['_'] = Sword;
		for (a = 9; a <= 13; a++)
			re_syntax_table[a] = Swhitespace;
		re_syntax_table[' '] = Swhitespace;
	}
	re_compile_initialized = 1;
	for (a = 0; a < 256; a++)
	{
		regexp_plain_ops[a] = Rnormal;
		regexp_quoted_ops[a] = Rnormal;
	}
	for (a = '0'; a <= '9'; a++)
		regexp_quoted_ops[a] = Rmemory;
	regexp_plain_ops['\134'] = Rquote;
	if (regexp_syntax & RE_NO_BK_PARENS)
	{
		regexp_plain_ops['('] = Ropenpar;
		regexp_plain_ops[')'] = Rclosepar;
	}
	else
	{
		regexp_quoted_ops['('] = Ropenpar;
		regexp_quoted_ops[')'] = Rclosepar;
	}
	if (regexp_syntax & RE_NO_BK_VBAR)
		regexp_plain_ops['\174'] = Ror;
	else
		regexp_quoted_ops['\174'] = Ror;
	regexp_plain_ops['*'] = Rstar;
	if (regexp_syntax & RE_BK_PLUS_QM)
	{
		regexp_quoted_ops['+'] = Rplus;
		regexp_quoted_ops['?'] = Roptional;
	}
	else
	{
		regexp_plain_ops['+'] = Rplus;
		regexp_plain_ops['?'] = Roptional;
	}
	if (regexp_syntax & RE_NEWLINE_OR)
		regexp_plain_ops['\n'] = Ror;
	regexp_plain_ops['\133'] = Ropenset;
	regexp_plain_ops['\136'] = Rbol;
	regexp_plain_ops['$'] = Reol;
	regexp_plain_ops['.'] = Ranychar;
	if (!(regexp_syntax & RE_NO_GNU_EXTENSIONS))
	{
		regexp_quoted_ops['w'] = Rwordchar;
		regexp_quoted_ops['W'] = Rnotwordchar;
		regexp_quoted_ops['<'] = Rwordbeg;
		regexp_quoted_ops['>'] = Rwordend;
		regexp_quoted_ops['b'] = Rwordbound;
		regexp_quoted_ops['B'] = Rnotwordbound;
		regexp_quoted_ops['`'] = Rbegbuf;
		regexp_quoted_ops['\''] = Rendbuf;
	}
	if (regexp_syntax & RE_ANSI_HEX)
		regexp_quoted_ops['v'] = Rextended_memory;
	for (a = 0; a < Rnum_ops; a++)
		regexp_precedences[a] = 4;
	if (regexp_syntax & RE_TIGHT_VBAR)
	{
		regexp_precedences[Ror] = 3;
		regexp_precedences[Rbol] = 2;
		regexp_precedences[Reol] = 2;
	}
	else
	{
		regexp_precedences[Ror] = 2;
		regexp_precedences[Rbol] = 3;
		regexp_precedences[Reol] = 3;
	}
	regexp_precedences[Rclosepar] = 1;
	regexp_precedences[Rend] = 0;
	regexp_context_indep_ops = (regexp_syntax & RE_CONTEXT_INDEP_OPS) != 0;
	regexp_ansi_sequences = (regexp_syntax & RE_ANSI_HEX) != 0;
}

int re_set_syntax(syntax)
	int syntax;
{
	int ret;
	
	ret = regexp_syntax;
	regexp_syntax = syntax;
	re_syntax = syntax; /* Exported copy */
	re_compile_initialize();
	return ret;
}

static int hex_char_to_decimal(ch)
	int ch;
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	return 16;
}

static void re_compile_fastmap_aux(code,
				   pos,
				   visited,
				   can_be_null,
				   fastmap)
	unsigned char *code;
	int pos;
	unsigned char *visited;
	unsigned char *can_be_null;
	unsigned char *fastmap;
{
	int a;
	int b;
	int syntaxcode;
	
	if (visited[pos])
		return;  /* we have already been here */
	visited[pos] = 1;
	for (;;)
		switch (code[pos++]) {
		case Cend:
			{
				*can_be_null = 1;
				return;
			}
		case Cbol:
		case Cbegbuf:
		case Cendbuf:
		case Cwordbeg:
		case Cwordend:
		case Cwordbound:
		case Cnotwordbound:
		{
			for (a = 0; a < 256; a++)
				fastmap[a] = 1;
			break;
		}
		case Csyntaxspec:
		{
			syntaxcode = code[pos++];
			for (a = 0; a < 256; a++)
				if (SYNTAX(a) & syntaxcode) 
					fastmap[a] = 1;
			return;
		}
		case Cnotsyntaxspec:
		{
			syntaxcode = code[pos++];
			for (a = 0; a < 256; a++)
				if (!(SYNTAX(a) & syntaxcode) )
					fastmap[a] = 1;
			return;
		}
		case Ceol:
		{
			fastmap['\n'] = 1;
			if (*can_be_null == 0)
				*can_be_null = 2; /* can match null, but only at end of buffer*/
			return;
		}
		case Cset:
		{
			for (a = 0; a < 256/8; a++)
				if (code[pos + a] != 0)
					for (b = 0; b < 8; b++)
						if (code[pos + a] & (1 << b))
							fastmap[(a << 3) + b] = 1;
			pos += 256/8;
			return;
		}
		case Cexact:
		{
			fastmap[(unsigned char)code[pos]] = 1;
			return;
		}
		case Canychar:
		{
			for (a = 0; a < 256; a++)
				if (a != '\n')
					fastmap[a] = 1;
			return;
		}
		case Cstart_memory:
		case Cend_memory:
		{
			pos++;
			break;
		}
		case Cmatch_memory:
		{
			for (a = 0; a < 256; a++)
				fastmap[a] = 1;
			*can_be_null = 1;
			return;
		}
		case Cjump:
		case Cdummy_failure_jump:
		case Cupdate_failure_jump:
		case Cstar_jump:
		{
			a = (unsigned char)code[pos++];
			a |= (unsigned char)code[pos++] << 8;
			pos += (int)SHORT(a);
			if (visited[pos])
			{
				/* argh... the regexp contains empty loops.  This is not
				   good, as this may cause a failure stack overflow when
				   matching.  Oh well. */
				/* this path leads nowhere; pursue other paths. */
				return;
			}
			visited[pos] = 1;
			break;
		}
		case Cfailure_jump:
		{
			a = (unsigned char)code[pos++];
			a |= (unsigned char)code[pos++] << 8;
			a = pos + (int)SHORT(a);
			re_compile_fastmap_aux(code, a, visited, can_be_null, fastmap);
			break;
		}
		case Crepeat1:
		{
			pos += 2;
			break;
		}
		default:
		{
		        PyErr_SetString(PyExc_SystemError, "Unknown regex opcode: memory corrupted?");
		        return;
			/*NOTREACHED*/
		}
		}
}

static int re_do_compile_fastmap(buffer,
				 used,
				 pos,
				 can_be_null,
				 fastmap)
	unsigned char *buffer;
	int used;
	int pos;
	unsigned char *can_be_null;
	unsigned char *fastmap;
{
	unsigned char small_visited[512], *visited;
   
	if (used <= sizeof(small_visited))
		visited = small_visited;
	else
	{
		visited = malloc(used);
		if (!visited)
			return 0;
	}
	*can_be_null = 0;
	memset(fastmap, 0, 256);
	memset(visited, 0, used);
	re_compile_fastmap_aux(buffer, pos, visited, can_be_null, fastmap);
	if (visited != small_visited)
		free(visited);
	return 1;
}

void re_compile_fastmap(bufp)
	regexp_t bufp;
{
	if (!bufp->fastmap || bufp->fastmap_accurate)
		return;
	assert(bufp->used > 0);
	if (!re_do_compile_fastmap(bufp->buffer,
				   bufp->used,
				   0,
				   &bufp->can_be_null,
				   bufp->fastmap))
		return;
	if (PyErr_Occurred()) return;
	if (bufp->buffer[0] == Cbol)
		bufp->anchor = 1;   /* begline */
	else
		if (bufp->buffer[0] == Cbegbuf)
			bufp->anchor = 2; /* begbuf */
		else
			bufp->anchor = 0; /* none */
	bufp->fastmap_accurate = 1;
}

/* 
 * star is coded as:
 * 1: failure_jump 2
 *    ... code for operand of star
 *    star_jump 1
 * 2: ... code after star
 *
 * We change the star_jump to update_failure_jump if we can determine
 * that it is safe to do so; otherwise we change it to an ordinary
 * jump.
 *
 * plus is coded as
 *
 *    jump 2
 * 1: failure_jump 3
 * 2: ... code for operand of plus
 *    star_jump 1
 * 3: ... code after plus
 *
 * For star_jump considerations this is processed identically to star.
 *
 */

static int re_optimize_star_jump(bufp, code)
	regexp_t bufp;
	unsigned char *code;
{
	unsigned char map[256];
	unsigned char can_be_null;
	unsigned char *p1;
	unsigned char *p2;
	unsigned char ch;
	int a;
	int b;
	int num_instructions = 0;

	a = (unsigned char)*code++;
	a |= (unsigned char)*code++ << 8;
	a = (int)SHORT(a);
	
	p1 = code + a + 3; /* skip the failure_jump */
	/* Check that the jump is within the pattern */
	if (p1<bufp->buffer || bufp->buffer+bufp->used<p1)
	  {
	    PyErr_SetString(PyExc_SystemError, "Regex VM jump out of bounds (failure_jump opt)");
	    return 0;
	  }
	
	assert(p1[-3] == Cfailure_jump);
	p2 = code;
	/* p1 points inside loop, p2 points to after loop */
	if (!re_do_compile_fastmap(bufp->buffer, bufp->used,
				   p2 - bufp->buffer, &can_be_null, map))
		goto make_normal_jump;
	
	/* If we might introduce a new update point inside the
	 * loop, we can't optimize because then update_jump would
	 * update a wrong failure point.  Thus we have to be
	 * quite careful here.
	 */
	
	/* loop until we find something that consumes a character */
  loop_p1:
	num_instructions++;
	switch (*p1++)
	{
	case Cbol:
	case Ceol:
	case Cbegbuf:
	case Cendbuf:
	case Cwordbeg:
	case Cwordend:
	case Cwordbound:
	case Cnotwordbound:
	{
		goto loop_p1;
	}
	case Cstart_memory:
	case Cend_memory:
	{
		p1++;
		goto loop_p1;
	}
	case Cexact:
	{
		ch = (unsigned char)*p1++;
		if (map[(int)ch])
			goto make_normal_jump;
		break;
	}
	case Canychar:
	{
		for (b = 0; b < 256; b++)
			if (b != '\n' && map[b])
				goto make_normal_jump;
		break;
	}
	case Cset:
	{
		for (b = 0; b < 256; b++)
			if ((p1[b >> 3] & (1 << (b & 7))) && map[b])
				goto make_normal_jump;
		p1 += 256/8;
		break;
	}
	default:
	{
		goto make_normal_jump;
	}
	}
	/* now we know that we can't backtrack. */
	while (p1 != p2 - 3)
	{
		num_instructions++;
		switch (*p1++)
		{
		case Cend:
		{
			return 0;
		}
		case Cbol:
		case Ceol:
		case Canychar:
		case Cbegbuf:
		case Cendbuf:
		case Cwordbeg:
		case Cwordend:
		case Cwordbound:
		case Cnotwordbound:
		{
			break;
		}
		case Cset:
		{
			p1 += 256/8;
			break;
		}
		case Cexact:
		case Cstart_memory:
		case Cend_memory:
		case Cmatch_memory:
		case Csyntaxspec:
		case Cnotsyntaxspec:
		{
			p1++;
			break;
		}
		case Cjump:
		case Cstar_jump:
		case Cfailure_jump:
		case Cupdate_failure_jump:
		case Cdummy_failure_jump:
		{
			goto make_normal_jump;
		}
		default:
		{
			return 0;
		}
		}
	}
	
	/* make_update_jump: */
	code -= 3;
	a += 3;  /* jump to after the Cfailure_jump */
	code[0] = Cupdate_failure_jump;
	code[1] = a & 0xff;
	code[2] = a >> 8;
	if (num_instructions > 1)
		return 1;
	assert(num_instructions == 1);
	/* if the only instruction matches a single character, we can do
	 * better */
	p1 = code + 3 + a;   /* start of sole instruction */
	if (*p1 == Cset || *p1 == Cexact || *p1 == Canychar ||
	    *p1 == Csyntaxspec || *p1 == Cnotsyntaxspec)
		code[0] = Crepeat1;
	return 1;
	
  make_normal_jump:
	code -= 3;
	*code = Cjump;
	return 1;
}

static int re_optimize(bufp)
	regexp_t bufp;
{
	unsigned char *code;
	
	code = bufp->buffer;
	
	while(1)
	{
		switch (*code++)
		{
		case Cend:
		{
			return 1;
		}
		case Canychar:
		case Cbol:
		case Ceol:
		case Cbegbuf:
		case Cendbuf:
		case Cwordbeg:
		case Cwordend:
		case Cwordbound:
		case Cnotwordbound:
		{
			break;
		}
		case Cset:
		{
			code += 256/8;
			break;
		}
		case Cexact:
		case Cstart_memory:
		case Cend_memory:
		case Cmatch_memory:
		case Csyntaxspec:
		case Cnotsyntaxspec:
		{
			code++;
			break;
		}
		case Cstar_jump:
		{
			if (!re_optimize_star_jump(bufp, code))
			{
				return 0;
			}
			/* fall through */
		}
		case Cupdate_failure_jump:
		case Cjump:
		case Cdummy_failure_jump:
		case Cfailure_jump:
		case Crepeat1:
		{
			code += 2;
			break;
		}
		default:
		{
			return 0;
		}
		}
	}
}

#define NEXTCHAR(var) \
{ \
	if (pos >= size) \
		goto ends_prematurely; \
	(var) = regex[pos]; \
	pos++; \
}

#define ALLOC(amount) \
{ \
	  if (pattern_offset+(amount) > alloc) \
	  { \
		  alloc += 256 + (amount); \
		  pattern = realloc(pattern, alloc); \
		  if (!pattern) \
			  goto out_of_memory; \
	  } \
}

#define STORE(ch) pattern[pattern_offset++] = (ch)

#define CURRENT_LEVEL_START (starts[starts_base + current_level])

#define SET_LEVEL_START starts[starts_base + current_level] = pattern_offset

#define PUSH_LEVEL_STARTS \
if (starts_base < (MAX_NESTING-1)*NUM_LEVELS) \
	starts_base += NUM_LEVELS; \
else \
	goto too_complex \

#define POP_LEVEL_STARTS starts_base -= NUM_LEVELS

#define PUT_ADDR(offset,addr) \
{ \
	int disp = (addr) - (offset) - 2; \
	pattern[(offset)] = disp & 0xff; \
	pattern[(offset)+1] = (disp>>8) & 0xff; \
}

#define INSERT_JUMP(pos,type,addr) \
{ \
	int a, p = (pos), t = (type), ad = (addr); \
	for (a = pattern_offset - 1; a >= p; a--) \
		pattern[a + 3] = pattern[a]; \
	pattern[p] = t; \
	PUT_ADDR(p+1,ad); \
	pattern_offset += 3; \
}

#define SETBIT(buf,offset,bit) (buf)[(offset)+(bit)/8] |= (1<<((bit) & 7))

#define SET_FIELDS \
{ \
	bufp->allocated = alloc; \
	bufp->buffer = pattern; \
	bufp->used = pattern_offset; \
}
    
#define GETHEX(var) \
{ \
	unsigned char gethex_ch, gethex_value; \
	NEXTCHAR(gethex_ch); \
	gethex_value = hex_char_to_decimal(gethex_ch); \
	if (gethex_value == 16) \
		goto hex_error; \
	NEXTCHAR(gethex_ch); \
	gethex_ch = hex_char_to_decimal(gethex_ch); \
	if (gethex_ch == 16) \
		goto hex_error; \
	(var) = gethex_value * 16 + gethex_ch; \
}

#define ANSI_TRANSLATE(ch) \
{ \
	switch (ch) \
	{ \
	case 'a': \
	case 'A': \
	{ \
		ch = 7; /* audible bell */ \
		break; \
	} \
	case 'b': \
	case 'B': \
	{ \
		ch = 8; /* backspace */ \
		break; \
	} \
	case 'f': \
	case 'F': \
	{ \
		ch = 12; /* form feed */ \
		break; \
	} \
	case 'n': \
	case 'N': \
	{ \
		ch = 10; /* line feed */ \
		break; \
	} \
	case 'r': \
	case 'R': \
	{ \
		ch = 13; /* carriage return */ \
		break; \
	} \
	case 't': \
	case 'T': \
	{ \
	      ch = 9; /* tab */ \
	      break; \
	} \
	case 'v': \
	case 'V': \
	{ \
		ch = 11; /* vertical tab */ \
		break; \
	} \
	case 'x': /* hex code */ \
	case 'X': \
	{ \
		GETHEX(ch); \
		break; \
	} \
	default: \
	{ \
		/* other characters passed through */ \
		if (translate) \
			ch = translate[(unsigned char)ch]; \
		break; \
	} \
	} \
}

char *re_compile_pattern(regex, size, bufp)
	unsigned char *regex;
	int size;
	regexp_t bufp;
{
	int a;
	int pos;
	int op;
	int current_level;
	int level;
	int opcode;
	int pattern_offset = 0, alloc;
	int starts[NUM_LEVELS * MAX_NESTING];
	int starts_base;
	int future_jumps[MAX_NESTING];
	int num_jumps;
	unsigned char ch = '\0';
	unsigned char *pattern;
	unsigned char *translate;
	int next_register;
	int paren_depth;
	int num_open_registers;
	int open_registers[RE_NREGS];
	int beginning_context;
	
	if (!re_compile_initialized)
		re_compile_initialize();
	bufp->used = 0;
	bufp->fastmap_accurate = 0;
	bufp->uses_registers = 1;
	bufp->num_registers = 1;
	translate = bufp->translate;
	pattern = bufp->buffer;
	alloc = bufp->allocated;
	if (alloc == 0 || pattern == NULL)
	{
		alloc = 256;
		pattern = malloc(alloc);
		if (!pattern)
			goto out_of_memory;
	}
	pattern_offset = 0;
	starts_base = 0;
	num_jumps = 0;
	current_level = 0;
	SET_LEVEL_START;
	num_open_registers = 0;
	next_register = 1;
	paren_depth = 0;
	beginning_context = 1;
	op = -1;
	/* we use Rend dummy to ensure that pending jumps are updated
	   (due to low priority of Rend) before exiting the loop. */
	pos = 0;
	while (op != Rend)
	{
		if (pos >= size)
			op = Rend;
		else
		{
			NEXTCHAR(ch);
			if (translate)
				ch = translate[(unsigned char)ch];
			op = regexp_plain_ops[(unsigned char)ch];
			if (op == Rquote)
			{
				NEXTCHAR(ch);
				op = regexp_quoted_ops[(unsigned char)ch];
				if (op == Rnormal && regexp_ansi_sequences)
					ANSI_TRANSLATE(ch);
			}
		}
		level = regexp_precedences[op];
		/* printf("ch='%c' op=%d level=%d current_level=%d
		   curlevstart=%d\n", ch, op, level, current_level,
		   CURRENT_LEVEL_START); */
		if (level > current_level)
		{
			for (current_level++; current_level < level; current_level++)
				SET_LEVEL_START;
			SET_LEVEL_START;
		}
		else
			if (level < current_level)
			{
				current_level = level;
				for (;num_jumps > 0 &&
					     future_jumps[num_jumps-1] >= CURRENT_LEVEL_START;
				     num_jumps--)
					PUT_ADDR(future_jumps[num_jumps-1], pattern_offset);
			}
		switch (op)
		{
		case Rend:
		{
			break;
		}
		case Rnormal:
		{
		  normal_char:
			opcode = Cexact;
		  store_opcode_and_arg: /* opcode & ch must be set */
			SET_LEVEL_START;
			ALLOC(2);
			STORE(opcode);
			STORE(ch);
			break;
		}
		case Ranychar:
		{
			opcode = Canychar;
		  store_opcode:
			SET_LEVEL_START;
			ALLOC(1);
			STORE(opcode);
			break;
		}
		case Rquote:
		{
			abort();
			/*NOTREACHED*/
		}
		case Rbol:
		{
			if (!beginning_context) {
				if (regexp_context_indep_ops)
					goto op_error;
				else
					goto normal_char;
			}
			opcode = Cbol;
			goto store_opcode;
		}
		case Reol:
		{
			if (!((pos >= size) ||
			      ((regexp_syntax & RE_NO_BK_VBAR) ?
			       (regex[pos] == '\174') :
			       (pos+1 < size && regex[pos] == '\134' &&
				regex[pos+1] == '\174')) ||
			      ((regexp_syntax & RE_NO_BK_PARENS)?
			       (regex[pos] == ')'):
			       (pos+1 < size && regex[pos] == '\134' &&
				regex[pos+1] == ')')))) {
				if (regexp_context_indep_ops)
					goto op_error;
				else
					goto normal_char;
			}
			opcode = Ceol;
			goto store_opcode;
			/* NOTREACHED */
			break;
		}
		case Roptional:
		{
			if (beginning_context) {
				if (regexp_context_indep_ops)
					goto op_error;
				else
					goto normal_char;
			}
			if (CURRENT_LEVEL_START == pattern_offset)
				break; /* ignore empty patterns for ? */
			ALLOC(3);
			INSERT_JUMP(CURRENT_LEVEL_START, Cfailure_jump,
				    pattern_offset + 3);
			break;
		}
		case Rstar:
		case Rplus:
		{
			if (beginning_context) {
				if (regexp_context_indep_ops)
					goto op_error;
				else
					goto normal_char;
			}
			if (CURRENT_LEVEL_START == pattern_offset)
				break; /* ignore empty patterns for + and * */
			ALLOC(9);
			INSERT_JUMP(CURRENT_LEVEL_START, Cfailure_jump,
				    pattern_offset + 6);
			INSERT_JUMP(pattern_offset, Cstar_jump, CURRENT_LEVEL_START);
			if (op == Rplus)  /* jump over initial failure_jump */
				INSERT_JUMP(CURRENT_LEVEL_START, Cdummy_failure_jump,
					    CURRENT_LEVEL_START + 6);
			break;
		}
		case Ror:
		{
			ALLOC(6);
			INSERT_JUMP(CURRENT_LEVEL_START, Cfailure_jump,
				    pattern_offset + 6);
			if (num_jumps >= MAX_NESTING)
				goto too_complex;
			STORE(Cjump);
			future_jumps[num_jumps++] = pattern_offset;
			STORE(0);
			STORE(0);
			SET_LEVEL_START;
			break;
		}
		case Ropenpar:
		{
			SET_LEVEL_START;
			if (next_register < RE_NREGS)
			{
				bufp->uses_registers = 1;
				ALLOC(2);
				STORE(Cstart_memory);
				STORE(next_register);
				open_registers[num_open_registers++] = next_register;
				bufp->num_registers++;
				next_register++;
			}
			paren_depth++;
			PUSH_LEVEL_STARTS;
			current_level = 0;
			SET_LEVEL_START;
			break;
		}
		case Rclosepar:
		{
			if (paren_depth <= 0)
				goto parenthesis_error;
			POP_LEVEL_STARTS;
			current_level = regexp_precedences[Ropenpar];
			paren_depth--;
			if (paren_depth < num_open_registers)
			{
				bufp->uses_registers = 1;
				ALLOC(2);
				STORE(Cend_memory);
				num_open_registers--;
				STORE(open_registers[num_open_registers]);
			}
			break;
		}
		case Rmemory:
		{
			if (ch == '0')
				goto bad_match_register;
			assert(ch >= '0' && ch <= '9');
			bufp->uses_registers = 1;
			opcode = Cmatch_memory;
			ch -= '0';
			goto store_opcode_and_arg;
		}
		case Rextended_memory:
		{
			NEXTCHAR(ch);
			if (ch < '0' || ch > '9')
				goto bad_match_register;
			NEXTCHAR(a);
			if (a < '0' || a > '9')
				goto bad_match_register;
			ch = 10 * (a - '0') + ch - '0';
			if (ch <= 0 || ch >= RE_NREGS)
				goto bad_match_register;
			bufp->uses_registers = 1;
			opcode = Cmatch_memory;
			goto store_opcode_and_arg;
		}
		case Ropenset:
		{
			int complement;
			int prev;
			int offset;
			int range;
			int firstchar;
	    
			SET_LEVEL_START;
			ALLOC(1+256/8);
			STORE(Cset);
			offset = pattern_offset;
			for (a = 0; a < 256/8; a++)
				STORE(0);
			NEXTCHAR(ch);
			if (translate)
				ch = translate[(unsigned char)ch];
			if (ch == '\136')
			{
				complement = 1;
				NEXTCHAR(ch);
				if (translate)
					ch = translate[(unsigned char)ch];
			}
			else
				complement = 0;
			prev = -1;
			range = 0;
			firstchar = 1;
			while (ch != '\135' || firstchar)
			{
				firstchar = 0;
				if (regexp_ansi_sequences && ch == '\134')
				{
					NEXTCHAR(ch);
					ANSI_TRANSLATE(ch);
				}
				if (range)
				{
					for (a = prev; a <= (int)ch; a++)
						SETBIT(pattern, offset, a);
					prev = -1;
					range = 0;
				}
				else
					if (prev != -1 && ch == '-')
						range = 1;
					else
					{
						SETBIT(pattern, offset, ch);
						prev = ch;
					}
				NEXTCHAR(ch);
				if (translate)
					ch = translate[(unsigned char)ch];
			}
			if (range)
				SETBIT(pattern, offset, '-');
			if (complement)
			{
				for (a = 0; a < 256/8; a++)
					pattern[offset+a] ^= 0xff;
			}
			break;
		}
		case Rbegbuf:
		{
			opcode = Cbegbuf;
			goto store_opcode;
		}
		case Rendbuf:
		{
			opcode = Cendbuf;
			goto store_opcode;
		}
		case Rwordchar:
		{
			opcode = Csyntaxspec;
			ch = Sword;
			goto store_opcode_and_arg;
		}
		case Rnotwordchar:
		{
			opcode = Cnotsyntaxspec;
			ch = Sword;
			goto store_opcode_and_arg;
		}
		case Rwordbeg:
		{
			opcode = Cwordbeg;
			goto store_opcode;
		}
		case Rwordend:
		{
			opcode = Cwordend;
			goto store_opcode;
		}
		case Rwordbound:
		{
			opcode = Cwordbound;
			goto store_opcode;
		}
		case Rnotwordbound:
		{
			opcode = Cnotwordbound;
			goto store_opcode;
		}
		default:
		{
			abort();
		}
		}
		beginning_context = (op == Ropenpar || op == Ror);
	}
	if (starts_base != 0)
		goto parenthesis_error;
	assert(num_jumps == 0);
	ALLOC(1);
	STORE(Cend);
	SET_FIELDS;
	if(!re_optimize(bufp))
		return "Optimization error";
	return NULL;

  op_error:
	SET_FIELDS;
	return "Badly placed special character";

  bad_match_register:
	SET_FIELDS;
	return "Bad match register number";
   
  hex_error:
	SET_FIELDS;
	return "Bad hexadecimal number";
   
  parenthesis_error:
	SET_FIELDS;
	return "Badly placed parenthesis";
   
  out_of_memory:
	SET_FIELDS;
	return "Out of memory";
   
  ends_prematurely:
	SET_FIELDS;
	return "Regular expression ends prematurely";

  too_complex:
	SET_FIELDS;
	return "Regular expression too complex";
}

#undef CHARAT
#undef NEXTCHAR
#undef GETHEX
#undef ALLOC
#undef STORE
#undef CURRENT_LEVEL_START
#undef SET_LEVEL_START
#undef PUSH_LEVEL_STARTS
#undef POP_LEVEL_STARTS
#undef PUT_ADDR
#undef INSERT_JUMP
#undef SETBIT
#undef SET_FIELDS

#define PREFETCH if (text == textend) goto fail

#define NEXTCHAR(var) \
PREFETCH; \
var = (unsigned char)*text++; \
if (translate) \
	var = translate[var]

int re_match(bufp,
	     string,
	     size,
	     pos,
	     old_regs)
	regexp_t bufp;
	unsigned char *string;
	int size;
	int pos;
	regexp_registers_t old_regs;
{
	unsigned char *code;
	unsigned char *translate;
	unsigned char *text;
	unsigned char *textstart;
	unsigned char *textend;
	int a;
	int b;
	int ch;
	int reg;
	int match_end;
	unsigned char *regstart;
	unsigned char *regend;
	int regsize;
	match_state state;
  
	assert(pos >= 0 && size >= 0);
	assert(pos <= size);
  
	text = string + pos;
	textstart = string;
	textend = string + size;
  
	code = bufp->buffer;
  
	translate = bufp->translate;
  
	NEW_STATE(state, bufp->num_registers);

  continue_matching:
	switch (*code++)
	{
	case Cend:
	{
		match_end = text - textstart;
		if (old_regs)
		{
			old_regs->start[0] = pos;
			old_regs->end[0] = match_end;
			if (!bufp->uses_registers)
			{
				for (a = 1; a < RE_NREGS; a++)
				{
					old_regs->start[a] = -1;
					old_regs->end[a] = -1;
				}
			}
			else
			{
				for (a = 1; a < bufp->num_registers; a++)
				{
					if ((GET_REG_START(state, a) == NULL) ||
					    (GET_REG_END(state, a) == NULL))
					{
						old_regs->start[a] = -1;
						old_regs->end[a] = -1;
						continue;
					}
					old_regs->start[a] = GET_REG_START(state, a) - textstart;
					old_regs->end[a] = GET_REG_END(state, a) - textstart;
				}
				for (; a < RE_NREGS; a++)
				{
					old_regs->start[a] = -1;
					old_regs->end[a] = -1;
				}
			}
		}
		FREE_STATE(state);
		return match_end - pos;
	}
	case Cbol:
	{
		if (text == textstart || text[-1] == '\n')
			goto continue_matching;
		goto fail;
	}
	case Ceol:
	{
		if (text == textend || *text == '\n')
			goto continue_matching;
		goto fail;
	}
	case Cset:
	{
		NEXTCHAR(ch);
		if (code[ch/8] & (1<<(ch & 7)))
		{
			code += 256/8;
			goto continue_matching;
		}
		goto fail;
	}
	case Cexact:
	{
		NEXTCHAR(ch);
		if (ch != (unsigned char)*code++)
			goto fail;
		goto continue_matching;
	}
	case Canychar:
	{
		NEXTCHAR(ch);
		if (ch == '\n')
			goto fail;
		goto continue_matching;
	}
	case Cstart_memory:
	{
		reg = *code++;
		SET_REG_START(state, reg, text, goto error);
		goto continue_matching;
	}
	case Cend_memory:
	{
		reg = *code++;
		SET_REG_END(state, reg, text, goto error);
		goto continue_matching;
	}
	case Cmatch_memory:
	{
		reg = *code++;
		regstart = GET_REG_START(state, reg);
		regend = GET_REG_END(state, reg);
		if ((regstart == NULL) || (regend == NULL))
			goto fail;  /* or should we just match nothing? */
		regsize = regend - regstart;

		if (regsize > (textend - text))
			goto fail;
		if(translate)
		{
			for (; regstart < regend; regstart++, text++)
				if (translate[*regstart] != translate[*text])
					goto fail;
		}
		else
			for (; regstart < regend; regstart++, text++)
				if (*regstart != *text)
					goto fail;
		goto continue_matching;
	}
	case Cupdate_failure_jump:
	{
		UPDATE_FAILURE(state, text, goto error);
		/* fall to next case */
	}
	/* treat Cstar_jump just like Cjump if it hasn't been optimized */
	case Cstar_jump:
	case Cjump:
	{
		a = (unsigned char)*code++;
		a |= (unsigned char)*code++ << 8;
		code += (int)SHORT(a);
		if (code<bufp->buffer || bufp->buffer+bufp->used<code) {
		        PyErr_SetString(PyExc_SystemError, "Regex VM jump out of bounds (Cjump)");
			FREE_STATE(state);
            	        return -2;
         	}
		goto continue_matching;
	}
	case Cdummy_failure_jump:
	{
                unsigned char *failuredest;
	  
		a = (unsigned char)*code++;
		a |= (unsigned char)*code++ << 8;
		a = (int)SHORT(a);
		assert(*code == Cfailure_jump);
		b = (unsigned char)code[1];
		b |= (unsigned char)code[2] << 8;
                failuredest = code + (int)SHORT(b) + 3;
		if (failuredest<bufp->buffer || bufp->buffer+bufp->used < failuredest) {
		        PyErr_SetString(PyExc_SystemError, "Regex VM jump out of bounds (Cdummy_failure_jump failuredest)");
			FREE_STATE(state);
            	        return -2;
		}
		PUSH_FAILURE(state, failuredest, NULL, goto error);
		code += a;
		if (code<bufp->buffer || bufp->buffer+bufp->used < code) {
		        PyErr_SetString(PyExc_SystemError, "Regex VM jump out of bounds (Cdummy_failure_jump code)");
			FREE_STATE(state);
            	        return -2;
         	}
		goto continue_matching;
	}
	case Cfailure_jump:
	{
		a = (unsigned char)*code++;
		a |= (unsigned char)*code++ << 8;
		a = (int)SHORT(a);
		if (code+a<bufp->buffer || bufp->buffer+bufp->used < code+a) {
		        PyErr_SetString(PyExc_SystemError, "Regex VM jump out of bounds (Cfailure_jump)");
			FREE_STATE(state);
            	        return -2;
         	}
		PUSH_FAILURE(state, code + a, text, goto error);
		goto continue_matching;
	}
	case Crepeat1:
	{
		unsigned char *pinst;
		a = (unsigned char)*code++;
		a |= (unsigned char)*code++ << 8;
		a = (int)SHORT(a);
		pinst = code + a;
		if (pinst<bufp->buffer || bufp->buffer+bufp->used<pinst) {
		        PyErr_SetString(PyExc_SystemError, "Regex VM jump out of bounds (Crepeat1)");
			FREE_STATE(state);
            	        return -2;
         	}
		/* pinst is sole instruction in loop, and it matches a
		 * single character.  Since Crepeat1 was originally a
		 * Cupdate_failure_jump, we also know that backtracking
		 * is useless: so long as the single-character
		 * expression matches, it must be used.  Also, in the
		 * case of +, we've already matched one character, so +
		 * can't fail: nothing here can cause a failure.  */
		switch (*pinst++)
		{
		case Cset:
		  {
		        if (translate)
			{
				while (text < textend)
				{
					ch = translate[(unsigned char)*text];
					if (pinst[ch/8] & (1<<(ch & 7)))
						text++;
					else
						break;
				}
			}
			else
			{
				while (text < textend)
				{
					ch = (unsigned char)*text;
					if (pinst[ch/8] & (1<<(ch & 7)))
						text++;
					else
						break;
				}
			}
			break;
                }
		case Cexact:
		{
			ch = (unsigned char)*pinst;
			if (translate)
			{
				while (text < textend &&
				       translate[(unsigned char)*text] == ch)
					text++;
			}
			else
			{
				while (text < textend && (unsigned char)*text == ch)
					text++;
			}
			break;
		}
		case Canychar:
		{
			while (text < textend && (unsigned char)*text != '\n')
				text++;
			break;
		}
		case Csyntaxspec:
		{
			a = (unsigned char)*pinst;
			if (translate)
			{
				while (text < textend &&
				       (SYNTAX(translate[*text]) & a) )
					text++;
			}
			else
			{
				while (text < textend && (SYNTAX(*text) & a) )
					text++;
			}
			break;
		}
		case Cnotsyntaxspec:
		{
			a = (unsigned char)*pinst;
			if (translate)
			{
				while (text < textend &&
				       !(SYNTAX(translate[*text]) & a) )
					text++;
			}
			else
			{
				while (text < textend && !(SYNTAX(*text) & a) )
					text++;
			}
			break;
		}
		default:
		{
		        FREE_STATE(state);
		        PyErr_SetString(PyExc_SystemError, "Unknown regex opcode: memory corrupted?");
		        return -2;
			/*NOTREACHED*/
		}
		}
		/* due to the funky way + and * are compiled, the top
		 * failure- stack entry at this point is actually a
		 * success entry -- update it & pop it */
		UPDATE_FAILURE(state, text, goto error);
		goto fail;      /* i.e., succeed <wink/sigh> */
	}
	case Cbegbuf:
	{
		if (text == textstart)
			goto continue_matching;
		goto fail;
	}
	case Cendbuf:
	{
		if (text == textend)
			goto continue_matching;
		goto fail;
	}
	case Cwordbeg:
	{
		if (text == textend)
			goto fail;
		if (!(SYNTAX(*text) & Sword)) 
			goto fail;
		if (text == textstart)
			goto continue_matching;
		if (!(SYNTAX(text[-1]) & Sword))
			goto continue_matching;
		goto fail;
	}
	case Cwordend:
	{
		if (text == textstart)
			goto fail;
		if (!(SYNTAX(text[-1]) & Sword))
			goto fail;
		if (text == textend)
			goto continue_matching;
		if (!(SYNTAX(*text) & Sword))
		        goto continue_matching;
                goto fail;
	}
	case Cwordbound:
	{
		/* Note: as in gnu regexp, this also matches at the
		 * beginning and end of buffer.  */

		if (text == textstart || text == textend)
			goto continue_matching;
		if ((SYNTAX(text[-1]) & Sword) ^ (SYNTAX(*text) & Sword))
			goto continue_matching;
		goto fail;
	}
	case Cnotwordbound:
	{
		/* Note: as in gnu regexp, this never matches at the
		 * beginning and end of buffer.  */
		if (text == textstart || text == textend)
			goto fail;
		if (!((SYNTAX(text[-1]) & Sword) ^ (SYNTAX(*text) & Sword)))
      		        goto continue_matching;
		goto fail;
	}
	case Csyntaxspec:
	{
		NEXTCHAR(ch);
		if (!(SYNTAX(ch) & (unsigned char)*code++))
			goto fail;
		goto continue_matching;
	}
	case Cnotsyntaxspec:
	{
		NEXTCHAR(ch);
		if (SYNTAX(ch) & (unsigned char)*code++)
			goto fail;
		goto continue_matching;
	}
	default:
	{
	        FREE_STATE(state);
	        PyErr_SetString(PyExc_SystemError, "Unknown regex opcode: memory corrupted?");
		return -2;
		/*NOTREACHED*/
	}
	}
	
	

#if 0 /* This line is never reached --Guido */
	abort();
#endif
	/*
	 *NOTREACHED
	 */

	/* Using "break;" in the above switch statement is equivalent to "goto fail;" */
  fail:
	POP_FAILURE(state, code, text, goto done_matching, goto error);
	goto continue_matching;
  
  done_matching:
/*   if(translated != NULL) */
/*      free(translated); */
	FREE_STATE(state);
	return -1;

  error:
/*   if (translated != NULL) */
/*      free(translated); */
	FREE_STATE(state);
	return -2;
}
	

#undef PREFETCH
#undef NEXTCHAR

int re_search(bufp,
	      string,
	      size,
	      pos,
	      range,
	      regs)
	regexp_t bufp;
	unsigned char *string;
	int size;
	int pos;
	int range;
	regexp_registers_t regs;
{
	unsigned char *fastmap;
	unsigned char *translate;
	unsigned char *text;
	unsigned char *partstart;
	unsigned char *partend;
	int dir;
	int ret;
	unsigned char anchor;
  
	assert(size >= 0 && pos >= 0);
	assert(pos + range >= 0 && pos + range <= size); /* Bugfix by ylo */
  
	fastmap = bufp->fastmap;
	translate = bufp->translate;
	if (fastmap && !bufp->fastmap_accurate) {
                re_compile_fastmap(bufp);
	        if (PyErr_Occurred()) return -2;
	}
	
	anchor = bufp->anchor;
	if (bufp->can_be_null == 1) /* can_be_null == 2: can match null at eob */
		fastmap = NULL;

	if (range < 0)
	{
		dir = -1;
		range = -range;
	}
	else
		dir = 1;

	if (anchor == 2) {
		if (pos != 0)
			return -1;
		else
			range = 0;
	}

	for (; range >= 0; range--, pos += dir)
	{
		if (fastmap)
		{
			if (dir == 1)
			{ /* searching forwards */

				text = string + pos;
				partend = string + size;
				partstart = text;
				if (translate)
					while (text != partend &&
					       !fastmap[(unsigned char) translate[(unsigned char)*text]])
						text++;
				else
					while (text != partend && !fastmap[(unsigned char)*text])
						text++;
				pos += text - partstart;
				range -= text - partstart;
				if (pos == size && bufp->can_be_null == 0)
					return -1;
			}
			else
			{ /* searching backwards */
				text = string + pos;
				partstart = string + pos - range;
				partend = text;
				if (translate)
					while (text != partstart &&
					       !fastmap[(unsigned char)
						       translate[(unsigned char)*text]])
						text--;
				else
					while (text != partstart &&
					       !fastmap[(unsigned char)*text])
						text--;
				pos -= partend - text;
				range -= partend - text;
			}
		}
		if (anchor == 1)
		{ /* anchored to begline */
			if (pos > 0 && (string[pos - 1] != '\n'))
				continue;
		}
		assert(pos >= 0 && pos <= size);
		ret = re_match(bufp, string, size, pos, regs);
		if (ret >= 0)
			return pos;
		if (ret == -2)
			return -2;
	}
	return -1;
}

/*
** Local Variables:
** mode: c
** c-file-style: "python"
** End:
*/
