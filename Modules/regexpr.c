/*

regexpr.c

Author: Tatu Ylonen <ylo@ngs.fi>

Copyright (c) 1991 Tatu Ylonen, Espoo, Finland

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies.  This
software is provided "as is" without express or implied warranty.

Created: Thu Sep 26 17:14:05 1991 ylo
Last modified: Mon Nov  4 17:06:48 1991 ylo
Ported to Think C: 19 Jan 1992 guido@cwi.nl

This code draws many ideas from the regular expression packages by
Henry Spencer of the University of Toronto and Richard Stallman of the
Free Software Foundation.

Emacs-specific code and syntax table code is almost directly borrowed
from GNU regexp.

*/

#include "config.h" /* For Win* specific redefinition of printf c.s. */

#include "myproto.h" /* For Py_PROTO macro --Guido */

#include <stdio.h>
#include <assert.h>
#include "regexpr.h"

#ifdef THINK_C
/* Think C on the Mac really needs these headers... --Guido */
#include <stdlib.h>
#include <string.h>
#else
#if defined(__STDC__) || defined(_MSC_VER)
/* Don't mess around, use the standard headers */
#include <stdlib.h>
#include <string.h>
#else
char *malloc();
void free();
char *realloc();
#endif /* __STDC__ */
#endif /* THINK_C */

#define MACRO_BEGIN do {
#define MACRO_END } while (0)

enum regexp_compiled_ops /* opcodes for compiled regexp */
{
  Cend,			/* end of pattern reached */
  Cbol,			/* beginning of line */
  Ceol,			/* end of line */
  Cset,			/* character set.  Followed by 32 bytes of set. */
  Cexact,		/* followed by a byte to match */
  Canychar,		/* matches any character except newline */
  Cstart_memory,	/* set register start addr (followed by reg number) */
  Cend_memory,		/* set register end addr (followed by reg number) */
  Cmatch_memory,	/* match a duplicate of reg contents (regnum follows)*/
  Cjump,		/* followed by two bytes (lsb,msb) of displacement. */
  Cstar_jump,		/* will change to jump/update_failure_jump at runtime */
  Cfailure_jump,	/* jump to addr on failure */
  Cupdate_failure_jump,	/* update topmost failure point and jump */
  Cdummy_failure_jump,	/* push a dummy failure point and jump */
  Cbegbuf,		/* match at beginning of buffer */
  Cendbuf,		/* match at end of buffer */
  Cwordbeg,		/* match at beginning of word */
  Cwordend,		/* match at end of word */
  Cwordbound,		/* match if at word boundary */
  Cnotwordbound,	/* match if not at word boundary */
#ifdef emacs
  Cemacs_at_dot,	/* emacs only: matches at dot */
#endif /* emacs */
  Csyntaxspec,		/* matches syntax code (1 byte follows) */
  Cnotsyntaxspec	/* matches if syntax code does not match (1 byte foll)*/
};

enum regexp_syntax_op	/* syntax codes for plain and quoted characters */
{
  Rend,			/* special code for end of regexp */
  Rnormal,		/* normal character */
  Ranychar,		/* any character except newline */
  Rquote,		/* the quote character */
  Rbol,			/* match beginning of line */
  Reol,			/* match end of line */
  Roptional,		/* match preceding expression optionally */
  Rstar,		/* match preceding expr zero or more times */
  Rplus,		/* match preceding expr one or more times */
  Ror,			/* match either of alternatives */
  Ropenpar,		/* opening parenthesis */
  Rclosepar,		/* closing parenthesis */
  Rmemory,		/* match memory register */
  Rextended_memory,	/* \vnn to match registers 10-99 */
  Ropenset,		/* open set.  Internal syntax hard-coded below. */
  /* the following are gnu extensions to "normal" regexp syntax */
  Rbegbuf,		/* beginning of buffer */
  Rendbuf,		/* end of buffer */
  Rwordchar,		/* word character */
  Rnotwordchar,		/* not word character */
  Rwordbeg,		/* beginning of word */
  Rwordend,		/* end of word */
  Rwordbound,		/* word bound */
  Rnotwordbound,	/* not word bound */
#ifdef emacs
  Remacs_at_dot,	/* emacs: at dot */
  Remacs_syntaxspec,	/* syntaxspec */
  Remacs_notsyntaxspec,	/* notsyntaxspec */
#endif /* emacs */
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

#ifdef emacs

/* This code is for emacs compatibility only. */

#include "config.h"
#include "lisp.h"
#include "buffer.h"
#include "syntax.h"

/* emacs defines NULL in some strange way? */
#undef NULL
#define NULL 0

#else /* emacs */

#define SYNTAX(ch) re_syntax_table[(unsigned char)(ch)]
#define Sword 1

#ifdef SYNTAX_TABLE
char *re_syntax_table;
#else
static char re_syntax_table[256];
#endif /* SYNTAX_TABLE */

#endif /* emacs */

static void re_compile_initialize Py_PROTO((void));
static void re_compile_initialize()
{
  int a;
  
#if !defined(emacs) && !defined(SYNTAX_TABLE)
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
	re_syntax_table[a] = Sword;
    }
#endif /* !emacs && !SYNTAX_TABLE */
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
#ifdef emacs
      regexp_quoted_ops['='] = Remacs_at_dot;
      regexp_quoted_ops['s'] = Remacs_syntaxspec;
      regexp_quoted_ops['S'] = Remacs_notsyntaxspec;
#endif /* emacs */
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

static int hex_char_to_decimal Py_PROTO((int));
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

char *re_compile_pattern(regex, size, bufp)
char *regex;
int size;
regexp_t bufp;
{
  int a, pos, op, current_level, level, opcode;
  int pattern_offset = 0, alloc;
  int starts[NUM_LEVELS * MAX_NESTING], starts_base;
  int future_jumps[MAX_NESTING], num_jumps;
  unsigned char ch = '\0';
  char *pattern, *translate;
  int next_register, paren_depth, num_open_registers, open_registers[RE_NREGS];
  int beginning_context;

#define NEXTCHAR(var)			\
  MACRO_BEGIN				\
    if (pos >= size)			\
      goto ends_prematurely;		\
    (var) = regex[pos];			\
    pos++;				\
  MACRO_END

#define ALLOC(amount)				\
  MACRO_BEGIN					\
    if (pattern_offset+(amount) > alloc)	\
      {						\
	alloc += 256 + (amount);		\
	pattern = realloc(pattern, alloc);	\
	if (!pattern)				\
	  goto out_of_memory;			\
      }						\
  MACRO_END

#define STORE(ch) pattern[pattern_offset++] = (ch)

#define CURRENT_LEVEL_START (starts[starts_base + current_level])

#define SET_LEVEL_START starts[starts_base + current_level] = pattern_offset

#define PUSH_LEVEL_STARTS if (starts_base < (MAX_NESTING-1)*NUM_LEVELS) \
		            starts_base += NUM_LEVELS;			\
                          else						\
			    goto too_complex

#define POP_LEVEL_STARTS starts_base -= NUM_LEVELS

#define PUT_ADDR(offset,addr)				\
  MACRO_BEGIN						\
    int disp = (addr) - (offset) - 2;			\
    pattern[(offset)] = disp & 0xff;			\
    pattern[(offset)+1] = (disp>>8) & 0xff;		\
  MACRO_END

#define INSERT_JUMP(pos,type,addr)			\
  MACRO_BEGIN						\
    int a, p = (pos), t = (type), ad = (addr);		\
    for (a = pattern_offset - 1; a >= p; a--)		\
      pattern[a + 3] = pattern[a];			\
    pattern[p] = t;					\
    PUT_ADDR(p+1,ad);					\
    pattern_offset += 3;				\
  MACRO_END

#define SETBIT(buf,offset,bit) (buf)[(offset)+(bit)/8] |= (1<<((bit) & 7))

#define SET_FIELDS				\
  MACRO_BEGIN					\
    bufp->allocated = alloc;			\
    bufp->buffer = pattern;			\
    bufp->used = pattern_offset;		\
  MACRO_END
    
#define GETHEX(var)						\
  MACRO_BEGIN							\
    char gethex_ch, gethex_value;				\
    NEXTCHAR(gethex_ch);					\
    gethex_value = hex_char_to_decimal(gethex_ch);		\
    if (gethex_value == 16)					\
      goto hex_error;						\
    NEXTCHAR(gethex_ch);					\
    gethex_ch = hex_char_to_decimal(gethex_ch);			\
    if (gethex_ch == 16)					\
      goto hex_error;						\
    (var) = gethex_value * 16 + gethex_ch;			\
  MACRO_END

#define ANSI_TRANSLATE(ch)				\
  MACRO_BEGIN						\
    switch (ch)						\
      {							\
      case 'a':						\
      case 'A':						\
	ch = 7; /* audible bell */			\
	break;						\
      case 'b':						\
      case 'B':						\
	ch = 8; /* backspace */				\
	break;						\
      case 'f':						\
      case 'F':						\
	ch = 12; /* form feed */			\
	break;						\
      case 'n':						\
      case 'N':						\
	ch = 10; /* line feed */			\
	break;						\
      case 'r':						\
      case 'R':						\
	ch = 13; /* carriage return */			\
	break;						\
      case 't':						\
      case 'T':						\
	ch = 9; /* tab */				\
	break;						\
      case 'v':						\
      case 'V':						\
	ch = 11; /* vertical tab */			\
	break;						\
      case 'x': /* hex code */				\
      case 'X':						\
	GETHEX(ch);					\
	break;						\
      default:						\
	/* other characters passed through */		\
	if (translate)					\
	  ch = translate[(unsigned char)ch];		\
	break;						\
      }							\
  MACRO_END

  if (!re_compile_initialized)
    re_compile_initialize();
  bufp->used = 0;
  bufp->fastmap_accurate = 0;
  bufp->uses_registers = 0;
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
  /* we use Rend dummy to ensure that pending jumps are updated (due to
     low priority of Rend) before exiting the loop. */
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
      /* printf("ch='%c' op=%d level=%d current_level=%d curlevstart=%d\n",
	     ch, op, level, current_level, CURRENT_LEVEL_START); */
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
	  break;
	case Rnormal:
	normal_char:
	  opcode = Cexact;
	store_opcode_and_arg: /* opcode & ch must be set */
	  SET_LEVEL_START;
	  ALLOC(2);
	  STORE(opcode);
	  STORE(ch);
	  break;
	case Ranychar:
	  opcode = Canychar;
	store_opcode:
	  SET_LEVEL_START;
	  ALLOC(1);
	  STORE(opcode);
	  break;
	case Rquote:
	  abort();
	  /*NOTREACHED*/
	case Rbol:
	  if (!beginning_context)
	    if (regexp_context_indep_ops)
	      goto op_error;
	    else
	      goto normal_char;
	  opcode = Cbol;
	  goto store_opcode;
	case Reol:
	  if (!((pos >= size) ||
		((regexp_syntax & RE_NO_BK_VBAR) ?
		 (regex[pos] == '\174') :
		 (pos+1 < size && regex[pos] == '\134' &&
		  regex[pos+1] == '\174')) ||
		((regexp_syntax & RE_NO_BK_PARENS)?
		 (regex[pos] == ')'):
		 (pos+1 < size && regex[pos] == '\134' &&
		  regex[pos+1] == ')'))))
	    if (regexp_context_indep_ops)
	      goto op_error;
	    else
	      goto normal_char;
	  opcode = Ceol;
	  goto store_opcode;
	  /* NOTREACHED */
	  break;
	case Roptional:
	  if (beginning_context)
	    if (regexp_context_indep_ops)
	      goto op_error;
	    else
	      goto normal_char;
	  if (CURRENT_LEVEL_START == pattern_offset)
	    break; /* ignore empty patterns for ? */
	  ALLOC(3);
	  INSERT_JUMP(CURRENT_LEVEL_START, Cfailure_jump,
		      pattern_offset + 3);
	  break;
	case Rstar:
	case Rplus:
	  if (beginning_context)
	    if (regexp_context_indep_ops)
	      goto op_error;
	    else
	      goto normal_char;
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
	case Ror:
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
	case Ropenpar:
	  SET_LEVEL_START;
	  if (next_register < RE_NREGS)
	    {
	      bufp->uses_registers = 1;
	      ALLOC(2);
	      STORE(Cstart_memory);
	      STORE(next_register);
	      open_registers[num_open_registers++] = next_register;
	      next_register++;
	    }
	  paren_depth++;
	  PUSH_LEVEL_STARTS;
	  current_level = 0;
	  SET_LEVEL_START;
	  break;
	case Rclosepar:
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
	case Rmemory:
	  if (ch == '0')
	    goto bad_match_register;
	  assert(ch >= '0' && ch <= '9');
	  bufp->uses_registers = 1;
	  opcode = Cmatch_memory;
	  ch -= '0';
	  goto store_opcode_and_arg;
	case Rextended_memory:
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
	case Ropenset:
	  {
	    int complement,prev,offset,range,firstchar;
	    
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
	  opcode = Cbegbuf;
	  goto store_opcode;
	case Rendbuf:
	  opcode = Cendbuf;
	  goto store_opcode;
	case Rwordchar:
	  opcode = Csyntaxspec;
	  ch = Sword;
	  goto store_opcode_and_arg;
	case Rnotwordchar:
	  opcode = Cnotsyntaxspec;
	  ch = Sword;
	  goto store_opcode_and_arg;
	case Rwordbeg:
	  opcode = Cwordbeg;
	  goto store_opcode;
	case Rwordend:
	  opcode = Cwordend;
	  goto store_opcode;
	case Rwordbound:
	  opcode = Cwordbound;
	  goto store_opcode;
	case Rnotwordbound:
	  opcode = Cnotwordbound;
	  goto store_opcode;
#ifdef emacs
	case Remacs_at_dot:
	  opcode = Cemacs_at_dot;
	  goto store_opcode;
	case Remacs_syntaxspec:
	  NEXTCHAR(ch);
	  if (translate)
	    ch = translate[(unsigned char)ch];
	  opcode = Csyntaxspec;
	  ch = syntax_spec_code[(unsigned char)ch];
	  goto store_opcode_and_arg;
	case Remacs_notsyntaxspec:
	  NEXTCHAR(ch);
	  if (translate)
	    ch = translate[(unsigned char)ch];
	  opcode = Cnotsyntaxspec;
	  ch = syntax_spec_code[(unsigned char)ch];
	  goto store_opcode_and_arg;
#endif /* emacs */
	default:
	  abort();
	}
      beginning_context = (op == Ropenpar || op == Ror);
    }
  if (starts_base != 0)
    goto parenthesis_error;
  assert(num_jumps == 0);
  ALLOC(1);
  STORE(Cend);
  SET_FIELDS;
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

static void re_compile_fastmap_aux
	Py_PROTO((char *, int, char *, char *, char *));
static void re_compile_fastmap_aux(code, pos, visited, can_be_null, fastmap)
char *code, *visited, *can_be_null, *fastmap;
int pos;
{
  int a, b, syntaxcode;

  if (visited[pos])
    return;  /* we have already been here */
  visited[pos] = 1;
  for (;;)
    switch (code[pos++])
      {
      case Cend:
	*can_be_null = 1;
	return;
      case Cbol:
      case Cbegbuf:
      case Cendbuf:
      case Cwordbeg:
      case Cwordend:
      case Cwordbound:
      case Cnotwordbound:
#ifdef emacs
      case Cemacs_at_dot:
#endif /* emacs */
	break;
      case Csyntaxspec:
	syntaxcode = code[pos++];
	for (a = 0; a < 256; a++)
	  if (SYNTAX(a) == syntaxcode)
	    fastmap[a] = 1;
	return;
      case Cnotsyntaxspec:
	syntaxcode = code[pos++];
	for (a = 0; a < 256; a++)
	  if (SYNTAX(a) != syntaxcode)
	    fastmap[a] = 1;
	return;
      case Ceol:
	fastmap['\n'] = 1;
	if (*can_be_null == 0)
	  *can_be_null = 2;  /* can match null, but only at end of buffer*/
	return;
      case Cset:
	for (a = 0; a < 256/8; a++)
	  if (code[pos + a] != 0)
	    for (b = 0; b < 8; b++)
	      if (code[pos + a] & (1 << b))
		fastmap[(a << 3) + b] = 1;
	pos += 256/8;
	return;
      case Cexact:
	fastmap[(unsigned char)code[pos]] = 1;
	return;
      case Canychar:
	for (a = 0; a < 256; a++)
	  if (a != '\n')
	    fastmap[a] = 1;
	return;
      case Cstart_memory:
      case Cend_memory:
	pos++;
	break;
      case Cmatch_memory:
	/* should this ever happen for sensible patterns??? */
	*can_be_null = 1;
	return;
      case Cjump:
      case Cdummy_failure_jump:
      case Cupdate_failure_jump:
      case Cstar_jump:
	a = (unsigned char)code[pos++];
	a |= (unsigned char)code[pos++] << 8;
	pos += (int)(short)a;
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
      case Cfailure_jump:
	a = (unsigned char)code[pos++];
	a |= (unsigned char)code[pos++] << 8;
	a = pos + (int)(short)a;
	re_compile_fastmap_aux(code, a, visited, can_be_null, fastmap);
	break;
      default:
	abort();  /* probably some opcode is missing from this switch */
	/*NOTREACHED*/
      }
}

static int re_do_compile_fastmap Py_PROTO((char *, int, int, char *, char *));
static int re_do_compile_fastmap(buffer, used, pos, can_be_null, fastmap)
char *buffer, *fastmap, *can_be_null;
int used, pos;
{
  char small_visited[512], *visited;

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
  if (!re_do_compile_fastmap(bufp->buffer, bufp->used, 0, &bufp->can_be_null,
			     bufp->fastmap))
    return;
  if (bufp->buffer[0] == Cbol)
    bufp->anchor = 1;   /* begline */
  else
    if (bufp->buffer[0] == Cbegbuf)
      bufp->anchor = 2; /* begbuf */
    else
      bufp->anchor = 0; /* none */
  bufp->fastmap_accurate = 1;
}

#define INITIAL_FAILURES  128  /* initial # failure points to allocate */
#define MAX_FAILURES     4100L /* max # of failure points before failing */

int re_match_2(bufp, string1, size1, string2, size2, pos, regs, mstop)
regexp_t bufp;
char *string1, *string2;
int size1, size2, pos, mstop;
regexp_registers_t regs;
{
  struct failure_point { char *text, *partend, *code; }
    *failure_stack_start, *failure_sp, *failure_stack_end,
    initial_failure_stack[INITIAL_FAILURES];
  char *code, *translate, *text, *textend, *partend, *part_2_end;
  char *regstart_text[RE_NREGS], *regstart_partend[RE_NREGS];
  char *regend_text[RE_NREGS], *regend_partend[RE_NREGS];
  int a, b, ch, reg, regch, match_end;
  char *regtext, *regpartend, *regtextend;

#define PREFETCH					\
  MACRO_BEGIN						\
    if (text == partend)				\
      {							\
	if (text == textend)				\
	  goto fail;					\
	text = string2;					\
	partend = part_2_end;				\
      }							\
  MACRO_END

#define NEXTCHAR(var)				\
  MACRO_BEGIN					\
    PREFETCH;					\
    (var) = (unsigned char)*text++;		\
    if (translate)				\
      (var) = (unsigned char)translate[(var)];	\
  MACRO_END

  assert(pos >= 0 && size1 >= 0 && size2 >= 0 && mstop >= 0);
  assert(mstop <= size1 + size2);
  assert(pos <= mstop);

  if (pos <= size1)
    {
      text = string1 + pos;
      if (mstop <= size1)
	{
	  partend = string1 + mstop;
	  textend = partend;
	}
      else
	{
	  partend = string1 + size1;
	  textend = string2 + mstop - size1;
	}
      part_2_end = string2 + mstop - size1;
    }
  else
    {
      text = string2 + pos - size1;
      partend = string2 + mstop - size1;
      textend = partend;
      part_2_end = partend;
    }

  if (bufp->uses_registers && regs != NULL)
    for (a = 0; a < RE_NREGS; a++)
      regend_text[a] = NULL;

  code = bufp->buffer;
  translate = bufp->translate;
  failure_stack_start = failure_sp = initial_failure_stack;
  failure_stack_end = initial_failure_stack + INITIAL_FAILURES;

#if 0
  /* re_search_2 has already done this, and otherwise we get little benefit
     from this.  So I'll leave this out. */
  if (bufp->fastmap_accurate && !bufp->can_be_null &&
      text != textend &&
      !bufp->fastmap[translate ?
		     (unsigned char)translate[(unsigned char)*text] :
		     (unsigned char)*text])
    return -1;  /* it can't possibly match */
#endif

 continue_matching:
  for (;;)
    {
      switch (*code++)
	{
	case Cend:
	  if (partend != part_2_end)
	    match_end = text - string1;
	  else
	    match_end = text - string2 + size1;
	  if (regs)
	    {
	      regs->start[0] = pos;
	      regs->end[0] = match_end;
	      if (!bufp->uses_registers)
		{
		  for (a = 1; a < RE_NREGS; a++)
		    {
		      regs->start[a] = -1;
		      regs->end[a] = -1;
		    }
		}
	      else
		{
		  for (a = 1; a < RE_NREGS; a++)
		    {
		      if (regend_text[a] == NULL)
			{
			  regs->start[a] = -1;
			  regs->end[a] = -1;
			  continue;
			}
		      if (regstart_partend[a] != part_2_end)
			regs->start[a] = regstart_text[a] - string1;
		      else
			regs->start[a] = regstart_text[a] - string2 + size1;
		      if (regend_partend[a] != part_2_end)
			regs->end[a] = regend_text[a] - string1;
		      else
			regs->end[a] = regend_text[a] - string2 + size1;
		    }
		}
	    }
	  if (failure_stack_start != initial_failure_stack)
	    free((char *)failure_stack_start);
	  return match_end - pos;
	case Cbol:
	  if (text == string1 || text[-1] == '\n') /* text[-1] always valid */
	    break;
	  goto fail;
	case Ceol:
	  if (text == string2 + size2 ||
	      (text == string1 + size1 ?
	       (size2 == 0 || *string2 == '\n') :
	       *text == '\n'))
	    break;
	  goto fail;
	case Cset:
	  NEXTCHAR(ch);
	  if (code[ch/8] & (1<<(ch & 7)))
	    {
	      code += 256/8;
	      break;
	    }
	  goto fail;
	case Cexact:
	  NEXTCHAR(ch);
	  if (ch != (unsigned char)*code++)
	    goto fail;
	  break;
	case Canychar:
	  NEXTCHAR(ch);
	  if (ch == '\n')
	    goto fail;
	  break;
	case Cstart_memory:
	  reg = *code++;
	  regstart_text[reg] = text;
	  regstart_partend[reg] = partend;
	  break;
	case Cend_memory:
	  reg = *code++;
	  regend_text[reg] = text;
	  regend_partend[reg] = partend;
	  break;
	case Cmatch_memory:
	  reg = *code++;
	  if (regend_text[reg] == NULL)
	    goto fail;  /* or should we just match nothing? */
	  regtext = regstart_text[reg];
	  regtextend = regend_text[reg];
	  if (regstart_partend[reg] == regend_partend[reg])
	    regpartend = regtextend;
	  else
	    regpartend = string1 + size1;
	  
	  for (;regtext != regtextend;)
	    {
	      NEXTCHAR(ch);
	      if (regtext == regpartend)
		regtext = string2;
	      regch = (unsigned char)*regtext++;
	      if (translate)
		regch = (unsigned char)translate[regch];
	      if (regch != ch)
		goto fail;
	    }
	  break;
	case Cstar_jump:
	  /* star is coded as:
	       1: failure_jump 2
	          ... code for operand of star
		  star_jump 1
	       2: ... code after star
	     We change the star_jump to update_failure_jump if we can determine
	     that it is safe to do so; otherwise we change it to an ordinary
	     jump.
	     plus is coded as
	          jump 2
	       1: failure_jump 3
	       2: ... code for operand of plus
	          star_jump 1
	       3: ... code after plus
	     For star_jump considerations this is processed identically
	     to star. */
	  a = (unsigned char)*code++;
	  a |= (unsigned char)*code++ << 8;
	  a = (int)(short)a;
	  {
	    char map[256], can_be_null;
	    char *p1, *p2;

	    p1 = code + a + 3; /* skip the failure_jump */
	    assert(p1[-3] == Cfailure_jump);
	    p2 = code;
	    /* p1 points inside loop, p2 points to after loop */
	    if (!re_do_compile_fastmap(bufp->buffer, bufp->used,
				       p2 - bufp->buffer, &can_be_null, map))
	      goto make_normal_jump;
	    /* If we might introduce a new update point inside the loop,
	       we can't optimize because then update_jump would update a
	       wrong failure point.  Thus we have to be quite careful here. */
	  loop_p1:
	    /* loop until we find something that consumes a character */
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
#ifdef emacs
              case Cemacs_at_dot:
#endif /* emacs */
                goto loop_p1;
              case Cstart_memory:
              case Cend_memory:
                p1++;
                goto loop_p1;
	      case Cexact:
		ch = (unsigned char)*p1++;
		if (map[ch])
		  goto make_normal_jump;
		break;
	      case Canychar:
		for (b = 0; b < 256; b++)
		  if (b != '\n' && map[b])
		    goto make_normal_jump;
		break;
	      case Cset:
		for (b = 0; b < 256; b++)
		  if ((p1[b >> 3] & (1 << (b & 7))) && map[b])
		    goto make_normal_jump;
		p1 += 256/8;
		break;
	      default:
		goto make_normal_jump;
	      }
	    /* now we know that we can't backtrack. */
	    while (p1 != p2 - 3)
	      {
		switch (*p1++)
		  {
		  case Cend:
		    abort();  /* we certainly shouldn't get this inside loop */
		    /*NOTREACHED*/
		  case Cbol:
		  case Ceol:
		  case Canychar:
		  case Cbegbuf:
		  case Cendbuf:
		  case Cwordbeg:
		  case Cwordend:
		  case Cwordbound:
		  case Cnotwordbound:
#ifdef emacs
		  case Cemacs_at_dot:
#endif /* emacs */
		    break;
		  case Cset:
		    p1 += 256/8;
		    break;
		  case Cexact:
		  case Cstart_memory:
		  case Cend_memory:
		  case Cmatch_memory:
		  case Csyntaxspec:
		  case Cnotsyntaxspec:
		    p1++;
		    break;
		  case Cjump:
		  case Cstar_jump:
		  case Cfailure_jump:
		  case Cupdate_failure_jump:
		  case Cdummy_failure_jump:
		    goto make_normal_jump;
		  default:
		    printf("regexpr.c: processing star_jump: unknown op %d\n", p1[-1]);
		    break;
		  }
	      }
	    goto make_update_jump;
	  }
	make_normal_jump:
	  /* printf("changing to normal jump\n"); */
	  code -= 3;
	  *code = Cjump;
	  break;
	make_update_jump:
	  /* printf("changing to update jump\n"); */
	  code -= 2;
	  a += 3;  /* jump to after the Cfailure_jump */
	  code[-1] = Cupdate_failure_jump;
	  code[0] = a & 0xff;
	  code[1] = a >> 8;
	  /* fall to next case */
	case Cupdate_failure_jump:
	  failure_sp[-1].text = text;
	  failure_sp[-1].partend = partend;
	  /* fall to next case */
	case Cjump:
	  a = (unsigned char)*code++;
	  a |= (unsigned char)*code++ << 8;
	  code += (int)(short)a;
	  break;
	case Cdummy_failure_jump:
	case Cfailure_jump:
	  if (failure_sp == failure_stack_end)
	    {
	      if (failure_stack_start != initial_failure_stack)
		goto error;
	      failure_stack_start = (struct failure_point *)
		malloc(MAX_FAILURES * sizeof(*failure_stack_start));
	      if (failure_stack_start == NULL)
	        {
		  failure_stack_start = initial_failure_stack;
		  goto error;
		}
	      failure_stack_end = failure_stack_start + MAX_FAILURES;
	      memcpy((char *)failure_stack_start, (char *)initial_failure_stack,
		     INITIAL_FAILURES * sizeof(*failure_stack_start));
	      failure_sp = failure_stack_start + INITIAL_FAILURES;
	    }
	  a = (unsigned char)*code++;
	  a |= (unsigned char)*code++ << 8;
	  a = (int)(short)a;
	  if (code[-3] == Cdummy_failure_jump)
	    { /* this is only used in plus */
	      assert(*code == Cfailure_jump);
	      b = (unsigned char)code[1];
	      b |= (unsigned char)code[2] << 8;
	      failure_sp->code = code + (int)(short)b + 3;
	      failure_sp->text = NULL;
	      code += a;
	    }
	  else
	    {
	      failure_sp->code = code + a;
	      failure_sp->text = text;
	      failure_sp->partend = partend;
	    }
	  failure_sp++;
	  break;
	case Cbegbuf:
	  if (text == string1)
	    break;
	  goto fail;
	case Cendbuf:
	  if (size2 == 0 ? text == string1 + size1 : text == string2 + size2)
	    break;
	  goto fail;
	case Cwordbeg:
	  if (text == string2 + size2)
	    goto fail;
	  if (size2 == 0 && text == string1 + size1)
	    goto fail;
	  if (SYNTAX(text == string1 + size1 ? *string1 : *text) != Sword)
	    goto fail;
	  if (text == string1)
	    break;
	  if (SYNTAX(text[-1]) != Sword)
	    break;
	  goto fail;
	case Cwordend:
	  if (text == string1)
	    goto fail;
	  if (SYNTAX(text[-1]) != Sword)
	    goto fail;
	  if (text == string2 + size2)
	    break;
	  if (size2 == 0 && text == string1 + size1)
	    break;
	  if (SYNTAX(*text) == Sword)
	    goto fail;
	  break;
	case Cwordbound:
	  /* Note: as in gnu regexp, this also matches at the beginning
	     and end of buffer. */
	  if (text == string1 || text == string2 + size2 ||
	      (size2 == 0 && text == string1 + size1))
	    break;
	  if ((SYNTAX(text[-1]) == Sword) ^
	      (SYNTAX(text == string1 + size1 ? *string2 : *text) == Sword))
	    break;
	  goto fail;
	case Cnotwordbound:
	  /* Note: as in gnu regexp, this never matches at the beginning
	     and end of buffer. */
	  if (text == string1 || text == string2 + size2 ||
	      (size2 == 0 && text == string1 + size1))
	    goto fail;
	  if (!((SYNTAX(text[-1]) == Sword) ^
		(SYNTAX(text == string1 + size1 ? *string2 : *text) == Sword)))
	    goto fail;
	  break;
	case Csyntaxspec:
	  NEXTCHAR(ch);
	  if (SYNTAX(ch) != (unsigned char)*code++)
	    goto fail;
	  break;
	case Cnotsyntaxspec:
	  NEXTCHAR(ch);
	  if (SYNTAX(ch) != (unsigned char)*code++)
	    break;
	  goto fail;
#ifdef emacs
	case Cemacs_at_dot:
	  if (PTR_CHAR_POS((unsigned char *)text) + 1 != point)
	    goto fail;
	  break;
#endif /* emacs */
	default:
	  abort();
	  /*NOTREACHED*/
	}
    }
#if 0 /* This line is never reached --Guido */
  abort();
#endif
  /*NOTREACHED*/

 fail:
  if (failure_sp != failure_stack_start)
    {
      failure_sp--;
      text = failure_sp->text;
      if (text == NULL)
	goto fail;
      partend = failure_sp->partend;
      code = failure_sp->code;
      goto continue_matching;
    }
  if (failure_stack_start != initial_failure_stack)
    free((char *)failure_stack_start);
  return -1;

 error:
  if (failure_stack_start != initial_failure_stack)
    free((char *)failure_stack_start);
  return -2;
}

#undef PREFETCH
#undef NEXTCHAR
#undef PUSH_FAILURE

int re_match(bufp, string, size, pos, regs)
regexp_t bufp;
char *string;
int size, pos;
regexp_registers_t regs;
{
  return re_match_2(bufp, string, size, (char *)NULL, 0, pos, regs, size);
}

int re_search_2(bufp, string1, size1, string2, size2, pos, range, regs,
		mstop)
regexp_t bufp;
char *string1, *string2;
int size1, size2, pos, range, mstop;
regexp_registers_t regs;
{
  char *fastmap, *translate, *text, *partstart, *partend;
  int dir, ret;
  char anchor;
  
  assert(size1 >= 0 && size2 >= 0 && pos >= 0 && mstop >= 0);
  assert(pos + range >= 0 && pos + range <= size1 + size2); /* Bugfix by ylo */
  assert(pos <= mstop);
  
  fastmap = bufp->fastmap;
  translate = bufp->translate;
  if (fastmap && !bufp->fastmap_accurate)
    re_compile_fastmap(bufp);
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
  if (anchor == 2)
    if (pos != 0)
      return -1;
    else
      range = 0;
  for (; range >= 0; range--, pos += dir)
    {
      if (fastmap)
	{
	  if (dir == 1)
	    { /* searching forwards */
	      if (pos < size1)
		{
		  text = string1 + pos;
		  if (pos + range > size1)
		    partend = string1 + size1;
		  else
		    partend = string1 + pos + range;
		}
	      else
		{
		  text = string2 + pos - size1;
		  partend = string2 + pos + range - size1;
		}
	      partstart = text;
	      if (translate)
		while (text != partend &&
		       !fastmap[(unsigned char)
				translate[(unsigned char)*text]])
		  text++;
	      else
		while (text != partend && !fastmap[(unsigned char)*text])
		  text++;
	      pos += text - partstart;
	      range -= text - partstart;
	      if (pos == size1 + size2 && bufp->can_be_null == 0)
		return -1;
	    }
	  else
	    { /* searching backwards */
	      if (pos <= size1)
		{
		  text = string1 + pos;
		  partstart = string1 + pos - range;
		}
	      else
		{
		  text = string2 + pos - size1;
		  if (range < pos - size1)
		    partstart = string2 + pos - size1 - range;
		  else
		    partstart = string2;
		}
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
	  if (pos > 0 &&
	      (pos <= size1 ? string1[pos - 1] :
	       string2[pos - size1 - 1]) != '\n')
	    continue;
	}
      assert(pos >= 0 && pos <= size1 + size2);
      ret = re_match_2(bufp, string1, size1, string2, size2, pos, regs, mstop);
      if (ret >= 0)
	return pos;
      if (ret == -2)
	return -2;
    }
  return -1;
}

int re_search(bufp, string, size, startpos, range, regs)
regexp_t bufp;
char *string;
int size, startpos, range;
regexp_registers_t regs;
{
  return re_search_2(bufp, string, size, (char *)NULL, 0,
		     startpos, range, regs, size);
}

#ifdef UNUSED

static struct re_pattern_buffer re_comp_buf;

char *re_comp(s)
char *s;
{
  if (s == NULL)
    {
      if (!re_comp_buf.buffer)
	return "Out of memory";
      return NULL;
    }
  if (!re_comp_buf.buffer)
    {
      /* the buffer will be allocated automatically */
      re_comp_buf.fastmap = malloc(256);
      re_comp_buf.translate = NULL;
      if (re_comp_buf.fastmap == NULL)
	return "Out of memory";
    }
  return re_compile_pattern(s, strlen(s), &re_comp_buf);
}

int re_exec(s)
char *s;
{
  int len = strlen(s);
  
  return re_search(&re_comp_buf, s, len, 0, len, (regexp_registers_t)NULL) >= 0;
}

#endif

#ifdef TEST_REGEXP

int main()
{
  char buf[500];
  char *cp;
  struct re_pattern_buffer exp;
  struct re_registers regs;
  int a,pos;
  char fastmap[256];

  exp.allocated = 0;
  exp.buffer = 0;
  exp.translate = NULL;
  exp.fastmap = fastmap;

  /* re_set_syntax(RE_NO_BK_PARENS|RE_NO_BK_VBAR|RE_ANSI_HEX); */

  while (1)
    {
      printf("Enter regexp:\n");
      gets(buf);
      cp=re_compile_pattern(buf, strlen(buf), &exp);
      if (cp)
	{
	  printf("Error: %s\n", cp);
	  continue;
	}
      re_compile_fastmap(&exp);
      printf("dump:\n");
      for (pos = 0; pos < exp.used;)
	{
	  printf("%d: ", pos);
	  switch (exp.buffer[pos++])
	    {
	    case Cend:
	      strcpy(buf, "end");
	      break;
	    case Cbol:
	      strcpy(buf, "bol");
	      break;
	    case Ceol:
	      strcpy(buf, "eol");
	      break;
	    case Cset:
	      strcpy(buf, "set ");
	      for (a = 0; a < 256/8; a++)
		sprintf(buf+strlen(buf)," %02x",
			(unsigned char)exp.buffer[pos++]);
	      break;
	    case Cexact:
	      sprintf(buf, "exact '%c' 0x%x", exp.buffer[pos],
		      (unsigned char)exp.buffer[pos]);
	      pos++;
	      break;
	    case Canychar:
	      strcpy(buf, "anychar");
	      break;
	    case Cstart_memory:
	      sprintf(buf, "start_memory %d", exp.buffer[pos++]);
	      break;
	    case Cend_memory:
	      sprintf(buf, "end_memory %d", exp.buffer[pos++]);
	      break;
	    case Cmatch_memory:
	      sprintf(buf, "match_memory %d", exp.buffer[pos++]);
	      break;
	    case Cjump:
	    case Cdummy_failure_jump:
	    case Cstar_jump:
	    case Cfailure_jump:
	    case Cupdate_failure_jump:
	      a = (unsigned char)exp.buffer[pos++];
	      a += (unsigned char)exp.buffer[pos++] << 8;
	      a = (int)(short)a;
	      switch (exp.buffer[pos-3])
		{
		case Cjump:
		  cp = "jump";
		  break;
		case Cstar_jump:
		  cp = "star_jump";
		  break;
		case Cfailure_jump:
		  cp = "failure_jump";
		  break;
		case Cupdate_failure_jump:
		  cp = "update_failure_jump";
		  break;
		case Cdummy_failure_jump:
		  cp = "dummy_failure_jump";
		  break;
		default:
		  cp = "unknown jump";
		  break;
		}
	      sprintf(buf, "%s %d", cp, a + pos);
	      break;
	    case Cbegbuf:
	      strcpy(buf,"begbuf");
	      break;
	    case Cendbuf:
	      strcpy(buf,"endbuf");
	      break;
	    case Cwordbeg:
	      strcpy(buf,"wordbeg");
	      break;
	    case Cwordend:
	      strcpy(buf,"wordend");
	      break;
	    case Cwordbound:
	      strcpy(buf,"wordbound");
	      break;
	    case Cnotwordbound:
	      strcpy(buf,"notwordbound");
	      break;
	    default:
	      sprintf(buf, "unknown code %d",
		      (unsigned char)exp.buffer[pos - 1]);
	      break;
	    }
	  printf("%s\n", buf);
	}
      printf("can_be_null = %d uses_registers = %d anchor = %d\n",
	     exp.can_be_null, exp.uses_registers, exp.anchor);
      
      printf("fastmap:");
      for (a = 0; a < 256; a++)
	if (exp.fastmap[a])
	  printf(" %d", a);
      printf("\n");
      printf("Enter strings.  An empty line terminates.\n");
      while (fgets(buf, sizeof(buf), stdin))
	{
	  if (buf[0] == '\n')
	    break;
	  a = re_search(&exp, buf, strlen(buf), 0, strlen(buf), &regs);
	  printf("search returns %d\n", a);
	  if (a != -1)
	    {
	      for (a = 0; a < RE_NREGS; a++)
		{
		  printf("buf %d: %d to %d\n", a, regs.start[a], regs.end[a]);
		}
	    }
	}
    }
}

#endif /* TEST_REGEXP */
