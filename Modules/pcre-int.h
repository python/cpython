/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/


#define PCRE_VERSION       "1.09 28-Apr-1998"


/* This is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language. See
the file Tech.Notes for some information on the internals.

Written by: Philip Hazel <ph10@cam.ac.uk>

           Copyright (c) 1998 University of Cambridge

-----------------------------------------------------------------------------
Permission is granted to anyone to use this software for any purpose on any
computer system, and to redistribute it freely, subject to the following
restrictions:

1. This software is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

2. The origin of this software must not be misrepresented, either by
   explicit claim or by omission.

3. Altered versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
-----------------------------------------------------------------------------
*/

/* This header contains definitions that are shared between the different
modules, but which are not relevant to the outside. */


/* To cope with SunOS4 and other systems that lack memmove() but have bcopy(),
define a macro for memmove() if USE_BCOPY is defined. */

#ifdef USE_BCOPY
#undef  memmove        /* some systems may have a macro */
#define memmove(a, b, c) bcopy(b, a, c)
#endif
 
/* Standard C headers plus the external interface definition */

#include <ctype.h>
#include <limits.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pcre.h"

/* In case there is no definition of offsetof() provided - though any proper
Standard C system should have one. */

#ifndef offsetof
#define offsetof(p_type,field) ((size_t)&(((p_type *)0)->field))
#endif

/* Private options flags start at the most significant end of the two bytes.
The public options defined in pcre.h start at the least significant end. Make
sure they don't overlap! */

#define PCRE_FIRSTSET           0x8000  /* first_char is set */
#define PCRE_STARTLINE          0x4000  /* start after \n for multiline */
#define PCRE_COMPILED_CASELESS  0x2000  /* like it says */

/* Options for the "extra" block produced by pcre_study(). */

#define PCRE_STUDY_CASELESS 0x01     /* study was caseless */
#define PCRE_STUDY_MAPPED   0x02     /* a map of starting chars exists */

/* Masks for identifying the public options: all permitted at compile time,
only some permitted at run or study time. */

#ifdef FOR_PYTHON
#define PUBLIC_OPTIONS \
  (PCRE_CASELESS|PCRE_EXTENDED|PCRE_ANCHORED|PCRE_MULTILINE| \
   PCRE_DOTALL|PCRE_DOLLAR_ENDONLY|PCRE_EXTRA|PCRE_UNGREEDY| \
   PCRE_LOCALE)
#else
#define PUBLIC_OPTIONS \
  (PCRE_CASELESS|PCRE_EXTENDED|PCRE_ANCHORED|PCRE_MULTILINE| \
   PCRE_DOTALL|PCRE_DOLLAR_ENDONLY|PCRE_EXTRA|PCRE_UNGREEDY)
#endif
#define PUBLIC_EXEC_OPTIONS \
  (PCRE_CASELESS|PCRE_ANCHORED|PCRE_MULTILINE|PCRE_NOTBOL|PCRE_NOTEOL| \
   PCRE_DOTALL|PCRE_DOLLAR_ENDONLY)

#define PUBLIC_STUDY_OPTIONS (PCRE_CASELESS)

/* Magic number to provide a small check against being handed junk. */

#define MAGIC_NUMBER  0x50435245   /* 'PCRE' */

/* Miscellaneous definitions */

typedef int BOOL;

#define FALSE   0
#define TRUE    1

/* These are escaped items that aren't just an encoding of a particular data
value such as \n. They must have non-zero values, as check_escape() returns
their negation. Also, they must appear in the same order as in the opcode
definitions below, up to ESC_Z. The final one must be ESC_REF as subsequent
values are used for \1, \2, \3, etc. There is a test in the code for an escape
greater than ESC_b and less than ESC_X to detect the types that may be
repeated. If any new escapes are put in-between that don't consume a character,
that code will have to change. */

enum { ESC_A = 1, ESC_B, ESC_b, ESC_D, ESC_d, ESC_S, ESC_s, ESC_W, ESC_w,

                    /* These are not Perl escapes, so can't appear in the */
       ESC_X,       /* simple table-lookup because they must be conditional */
                    /* on PCRE_EXTRA. */
       ESC_Z,
       ESC_REF };

/* Opcode table: OP_BRA must be last, as all values >= it are used for brackets
that extract substrings. Starting from 1 (i.e. after OP_END), the values up to
OP_EOD must correspond in order to the list of escapes immediately above. */

enum {
  OP_END,            /* End of pattern */

  /* Values corresponding to backslashed metacharacters */

  OP_SOD,            /* Start of data: \A */
  OP_NOT_WORD_BOUNDARY,  /* \B */
  OP_WORD_BOUNDARY,      /* \b */
  OP_NOT_DIGIT,          /* \D */
  OP_DIGIT,              /* \d */
  OP_NOT_WHITESPACE,     /* \S */
  OP_WHITESPACE,         /* \s */
  OP_NOT_WORDCHAR,       /* \W */
  OP_WORDCHAR,           /* \w */
  OP_CUT,            /* The analogue of Prolog's "cut" operation (extension) */
  OP_EOD,            /* End of data: \Z. */

  OP_NOT_WORD_BOUNDARY_L,  /* localized \B */
  OP_WORD_BOUNDARY_L,      /* localized \b */
  OP_NOT_WORDCHAR_L,       /* localized \W */
  OP_WORDCHAR_L,           /* localized \w */

  OP_CIRC,           /* Start of line - varies with multiline switch */
  OP_DOLL,           /* End of line - varies with multiline switch */
  OP_ANY,            /* Match any character */
  OP_CHARS,          /* Match string of characters */
  OP_NOT,            /* Match anything but the following char */

  OP_STAR,           /* The maximizing and minimizing versions of */
  OP_MINSTAR,        /* all these opcodes must come in pairs, with */
  OP_PLUS,           /* the minimizing one second. */
  OP_MINPLUS,        /* This first set applies to single characters */
  OP_QUERY,
  OP_MINQUERY,
  OP_UPTO,           /* From 0 to n matches */
  OP_MINUPTO,
  OP_EXACT,          /* Exactly n matches */

  OP_NOTSTAR,        /* The maximizing and minimizing versions of */
  OP_NOTMINSTAR,     /* all these opcodes must come in pairs, with */
  OP_NOTPLUS,        /* the minimizing one second. */
  OP_NOTMINPLUS,     /* This first set applies to "not" single characters */
  OP_NOTQUERY,
  OP_NOTMINQUERY,
  OP_NOTUPTO,        /* From 0 to n matches */
  OP_NOTMINUPTO,
  OP_NOTEXACT,       /* Exactly n matches */

  OP_TYPESTAR,       /* The maximizing and minimizing versions of */
  OP_TYPEMINSTAR,    /* all these opcodes must come in pairs, with */
  OP_TYPEPLUS,       /* the minimizing one second. These codes must */
  OP_TYPEMINPLUS,    /* be in exactly the same order as those above. */
  OP_TYPEQUERY,      /* This set applies to character types such as \d */
  OP_TYPEMINQUERY,
  OP_TYPEUPTO,       /* From 0 to n matches */
  OP_TYPEMINUPTO,
  OP_TYPEEXACT,      /* Exactly n matches */

  OP_CRSTAR,         /* The maximizing and minimizing versions of */
  OP_CRMINSTAR,      /* all these opcodes must come in pairs, with */
  OP_CRPLUS,         /* the minimizing one second. These codes must */
  OP_CRMINPLUS,      /* be in exactly the same order as those above. */
  OP_CRQUERY,        /* These are for character classes and back refs */
  OP_CRMINQUERY,
  OP_CRRANGE,        /* These are different to the three seta above. */
  OP_CRMINRANGE,

  OP_CLASS,          /* Match a character class */
  OP_NEGCLASS,       /* Match a character class, specified negatively */
  OP_CLASS_L,        /* Match a character class */
  OP_REF,            /* Match a back reference */

  OP_ALT,            /* Start of alternation */
  OP_KET,            /* End of group that doesn't have an unbounded repeat */
  OP_KETRMAX,        /* These two must remain together and in this */
  OP_KETRMIN,        /* order. They are for groups the repeat for ever. */

  OP_ASSERT,
  OP_ASSERT_NOT,
  OP_ONCE,           /* Once matched, don't back up into the subpattern */

  OP_BRAZERO,        /* These two must remain together and in this */
  OP_BRAMINZERO,     /* order. */

  OP_BRA             /* This and greater values are used for brackets that
                        extract substrings. */
};

/* The highest extraction number. This is limited by the number of opcodes
left after OP_BRA, i.e. 255 - OP_BRA. We actually set it somewhat lower. */

#define EXTRACT_MAX  99

/* The texts of compile-time error messages are defined as macros here so that
they can be accessed by the POSIX wrapper and converted into error codes.  Yes,
I could have used error codes in the first place, but didn't feel like changing
just to accommodate the POSIX wrapper. */

#define ERR1  "\\ at end of pattern"
#define ERR2  "\\c at end of pattern"
#define ERR3  "unrecognized character follows \\"
#define ERR4  "numbers out of order in {} quantifier"
#define ERR5  "number too big in {} quantifier"
#define ERR6  "missing terminating ] for character class"
#define ERR7  "invalid escape sequence in character class"
#define ERR8  "range out of order in character class"
#define ERR9  "nothing to repeat"
#define ERR10 "operand of unlimited repeat could match the empty string"
#define ERR11 "internal error: unexpected repeat"
#define ERR12 "unrecognized character after (?"
#define ERR13 "too many capturing parenthesized sub-patterns"
#define ERR14 "missing )"
#define ERR15 "back reference to non-existent subpattern"
#define ERR16 "erroffset passed as NULL"
#define ERR17 "unknown option bit(s) set"
#define ERR18 "missing ) after comment"
#define ERR19 "too many sets of parentheses"
#define ERR20 "regular expression too large"
#define ERR21 "failed to get memory"
#define ERR22 "unmatched brackets"
#define ERR23 "internal error: code overflow"

/* All character handling must be done as unsigned characters. Otherwise there
are problems with top-bit-set characters and functions such as isspace().
However, we leave the interface to the outside world as char *, because that
should make things easier for callers. We define a short type for unsigned char
to save lots of typing. I tried "uchar", but it causes problems on Digital
Unix, where it is defined in sys/types, so use "uschar" instead. */

typedef unsigned char uschar;

/* The real format of the start of the pcre block; the actual code vector
runs on as long as necessary after the end. */

typedef struct real_pcre {
  unsigned int  magic_number;
  unsigned short int options;
  unsigned char top_bracket;
  unsigned char top_backref;
  unsigned char first_char;
  unsigned char code[1];
} real_pcre;

/* The real format of the extra block returned by pcre_study(). */

typedef struct real_pcre_extra {
  unsigned char options;
  unsigned char start_bits[32];
} real_pcre_extra;

/* Global tables from chartables.c */

extern uschar pcre_lcc[];
extern uschar pcre_fcc[];
extern uschar pcre_cbits[];
extern uschar pcre_ctypes[];

/* Bit definitions for entries in pcre_ctypes[]. */

#define ctype_space   0x01
#define ctype_letter  0x02
#define ctype_digit   0x04
#define ctype_xdigit  0x08
#define ctype_word    0x10   /* alphameric or '_' */
#define ctype_odigit  0x20   /* octal digit */
#define ctype_meta    0x80   /* regexp meta char or zero (end pattern) */

/* Offsets for the bitmap tables */

#define cbit_digit    0
#define cbit_letter  32
#define cbit_word    64
#define cbit_space   96
#define cbit_length 128      /* Length of the cbits table */

/* End of internal.h */
