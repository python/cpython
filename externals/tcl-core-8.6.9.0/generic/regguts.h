/*
 * Internal interface definitions, etc., for the reg package
 *
 * Copyright (c) 1998, 1999 Henry Spencer.  All rights reserved.
 *
 * Development of this software was funded, in part, by Cray Research Inc.,
 * UUNET Communications Services Inc., Sun Microsystems Inc., and Scriptics
 * Corporation, none of whom are responsible for the results.  The author
 * thanks all of them.
 *
 * Redistribution and use in source and binary forms -- with or without
 * modification -- are permitted for any purpose, provided that
 * redistributions in source form retain this entire copyright notice and
 * indicate the origin and nature of any modifications.
 *
 * I'd appreciate being given credit for this package in the documentation of
 * software which uses it, but that is not a requirement.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * HENRY SPENCER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Environmental customization. It should not (I hope) be necessary to alter
 * the file you are now reading -- regcustom.h should handle it all, given
 * care here and elsewhere.
 */
#include "regcustom.h"

/*
 * Things that regcustom.h might override.
 */

/* assertions */
#ifndef assert
#ifndef REG_DEBUG
#ifndef NDEBUG
#define	NDEBUG		/* no assertions */
#endif
#endif /* !REG_DEBUG */
#include <assert.h>
#endif

/* voids */
#ifndef VOID
#define	VOID	void		/* for function return values */
#endif
#ifndef DISCARD
#define	DISCARD	void		/* for throwing values away */
#endif
#ifndef PVOID
#define	PVOID	void *		/* generic pointer */
#endif
#ifndef VS
#define	VS(x)	((void*)(x))	/* cast something to generic ptr */
#endif
#ifndef NOPARMS
#define	NOPARMS	void		/* for empty parm lists */
#endif

/* function-pointer declarator */
#ifndef FUNCPTR
#if __STDC__ >= 1
#define	FUNCPTR(name, args)	(*name)args
#else
#define	FUNCPTR(name, args)	(*name)()
#endif
#endif

/* memory allocation */
#ifndef MALLOC
#define	MALLOC(n)	malloc(n)
#endif
#ifndef REALLOC
#define	REALLOC(p, n)	realloc(VS(p), n)
#endif
#ifndef FREE
#define	FREE(p)		free(VS(p))
#endif

/* want size of a char in bits, and max value in bounded quantifiers */
#ifndef _POSIX2_RE_DUP_MAX
#define	_POSIX2_RE_DUP_MAX 255	/* normally from <limits.h> */
#endif

/*
 * misc
 */

#define	NOTREACHED	0
#define	xxx		1

#define	DUPMAX	_POSIX2_RE_DUP_MAX
#define	DUPINF	(DUPMAX+1)

#define	REMAGIC	0xfed7		/* magic number for main struct */

/*
 * debugging facilities
 */
#ifdef REG_DEBUG
/* FDEBUG does finite-state tracing */
#define	FDEBUG(arglist)	{ if (v->eflags&REG_FTRACE) printf arglist; }
/* MDEBUG does higher-level tracing */
#define	MDEBUG(arglist)	{ if (v->eflags&REG_MTRACE) printf arglist; }
#else
#define	FDEBUG(arglist)	{}
#define	MDEBUG(arglist)	{}
#endif

/*
 * bitmap manipulation
 */
#define	UBITS	(CHAR_BIT * sizeof(unsigned))
#define	BSET(uv, sn)	((uv)[(sn)/UBITS] |= (unsigned)1 << ((sn)%UBITS))
#define	ISBSET(uv, sn)	((uv)[(sn)/UBITS] & ((unsigned)1 << ((sn)%UBITS)))

/*
 * We dissect a chr into byts for colormap table indexing. Here we define a
 * byt, which will be the same as a byte on most machines... The exact size of
 * a byt is not critical, but about 8 bits is good, and extraction of 8-bit
 * chunks is sometimes especially fast.
 */

#ifndef BYTBITS
#define	BYTBITS	8		/* bits in a byt */
#endif
#define	BYTTAB	(1<<BYTBITS)	/* size of table with one entry per byt value */
#define	BYTMASK	(BYTTAB-1)	/* bit mask for byt */
#define	NBYTS	((CHRBITS+BYTBITS-1)/BYTBITS)
/* the definition of GETCOLOR(), below, assumes NBYTS <= 4 */

/*
 * As soon as possible, we map chrs into equivalence classes -- "colors" --
 * which are of much more manageable number.
 */

typedef short color;		/* colors of characters */
typedef int pcolor;		/* what color promotes to */
#define MAX_COLOR	SHRT_MAX /* max color value */
#define	COLORLESS	(-1)	/* impossible color */
#define	WHITE		0	/* default color, parent of all others */

/*
 * A colormap is a tree -- more precisely, a DAG -- indexed at each level by a
 * byt of the chr, to map the chr to a color efficiently. Because lower
 * sections of the tree can be shared, it can exploit the usual sparseness of
 * such a mapping table. The tree is always NBYTS levels deep (in the past it
 * was shallower during construction but was "filled" to full depth at the end
 * of that); areas that are unaltered as yet point to "fill blocks" which are
 * entirely WHITE in color.
 */

/* the tree itself */
struct colors {
    color ccolor[BYTTAB];
};
struct ptrs {
    union tree *pptr[BYTTAB];
};
union tree {
    struct colors colors;
    struct ptrs ptrs;
};
#define	tcolor	colors.ccolor
#define	tptr	ptrs.pptr

/* Internal per-color descriptor structure for the color machinery */
struct colordesc {
    uchr nchrs;			/* number of chars of this color */
    color sub;			/* open subcolor (if any); free chain ptr */
#define	NOSUB	COLORLESS
    struct arc *arcs;		/* color chain */
    int flags;
#define	FREECOL	01		/* currently free */
#define	PSEUDO	02		/* pseudocolor, no real chars */
#define	UNUSEDCOLOR(cd)	((cd)->flags&FREECOL)
    union tree *block;		/* block of solid color, if any */
};

/*
 * The color map itself
 *
 * Much of the data in the colormap struct is only used at compile time.
 * However, the bulk of the space usage is in the "tree" structure, so it's
 * not clear that there's much point in converting the rest to a more compact
 * form when compilation is finished.
 */
struct colormap {
    int magic;
#define	CMMAGIC	0x876
    struct vars *v;		/* for compile error reporting */
    size_t ncds;		/* number of colordescs */
    size_t max;			/* highest in use */
    color free;			/* beginning of free chain (if non-0) */
    struct colordesc *cd;
#define	CDEND(cm)	(&(cm)->cd[(cm)->max + 1])
#define	NINLINECDS	((size_t)10)
    struct colordesc cdspace[NINLINECDS];
    union tree tree[NBYTS];	/* tree top, plus fill blocks */
};

/* optimization magic to do fast chr->color mapping */
#define	B0(c)	((c) & BYTMASK)
#define	B1(c)	(((c)>>BYTBITS) & BYTMASK)
#define	B2(c)	(((c)>>(2*BYTBITS)) & BYTMASK)
#define	B3(c)	(((c)>>(3*BYTBITS)) & BYTMASK)
#if NBYTS == 1
#define	GETCOLOR(cm, c)	((cm)->tree->tcolor[B0(c)])
#endif
/* beware, for NBYTS>1, GETCOLOR() is unsafe -- 2nd arg used repeatedly */
#if NBYTS == 2
#define	GETCOLOR(cm, c)	((cm)->tree->tptr[B1(c)]->tcolor[B0(c)])
#endif
#if NBYTS == 4
#define	GETCOLOR(cm, c)	((cm)->tree->tptr[B3(c)]->tptr[B2(c)]->tptr[B1(c)]->tcolor[B0(c)])
#endif

/*
 * Interface definitions for locale-interface functions in locale.c.
 */

/* Representation of a set of characters. */
struct cvec {
    int nchrs;			/* number of chrs */
    int chrspace;		/* number of chrs possible */
    chr *chrs;			/* pointer to vector of chrs */
    int nranges;		/* number of ranges (chr pairs) */
    int rangespace;		/* number of chrs possible */
    chr *ranges;		/* pointer to vector of chr pairs */
};

/*
 * definitions for non-deterministic finite autmaton (NFA) internal
 * representation
 *
 * Having a "from" pointer within each arc may seem redundant, but it saves a
 * lot of hassle.
 */

struct state;

struct arc {
    int type;			/* 0 if free, else an NFA arc type code */
    color co;
    struct state *from;		/* where it's from (and contained within) */
    struct state *to;		/* where it's to */
    struct arc *outchain;	/* link in *from's outs chain or free chain */
    struct arc *outchainRev;	/* back-link in *from's outs chain */
#define	freechain outchain	/* we do not maintain "freechainRev" */
    struct arc *inchain;	/* *to's ins chain */
    struct arc *inchainRev;	/* back-link in *to's ins chain */
    struct arc *colorchain;	/* color's arc chain */
    struct arc *colorchainRev;	/* back-link in color's arc chain */
};

struct arcbatch {		/* for bulk allocation of arcs */
    struct arcbatch *next;
#define	ABSIZE	10
    struct arc a[ABSIZE];
};

struct state {
    int no;
#define	FREESTATE	(-1)
    char flag;			/* marks special states */
    int nins;			/* number of inarcs */
    struct arc *ins;		/* chain of inarcs */
    int nouts;			/* number of outarcs */
    struct arc *outs;		/* chain of outarcs */
    struct arc *free;		/* chain of free arcs */
    struct state *tmp;		/* temporary for traversal algorithms */
    struct state *next;		/* chain for traversing all */
    struct state *prev;		/* back chain */
    struct arcbatch oas;	/* first arcbatch, avoid malloc in easy case */
    int noas;			/* number of arcs used in first arcbatch */
};

struct nfa {
    struct state *pre;		/* pre-initial state */
    struct state *init;		/* initial state */
    struct state *final;	/* final state */
    struct state *post;		/* post-final state */
    int nstates;		/* for numbering states */
    struct state *states;	/* state-chain header */
    struct state *slast;	/* tail of the chain */
    struct state *free;		/* free list */
    struct colormap *cm;	/* the color map */
    color bos[2];		/* colors, if any, assigned to BOS and BOL */
    color eos[2];		/* colors, if any, assigned to EOS and EOL */
    struct vars *v;		/* simplifies compile error reporting */
    struct nfa *parent;		/* parent NFA, if any */
};

/*
 * definitions for compacted NFA
 *
 * The main space savings in a compacted NFA is from making the arcs as small
 * as possible.  We store only the transition color and next-state number for
 * each arc.  The list of out arcs for each state is an array beginning at
 * cnfa.states[statenumber], and terminated by a dummy carc struct with
 * co == COLORLESS.
 *
 * The non-dummy carc structs are of two types: plain arcs and LACON arcs.
 * Plain arcs just store the transition color number as "co".  LACON arcs
 * store the lookahead constraint number plus cnfa.ncolors as "co".  LACON
 * arcs can be distinguished from plain by testing for co >= cnfa.ncolors.
 */

struct carc {
    color co;			/* COLORLESS is list terminator */
    int to;			/* next-state number */
};

struct cnfa {
    int nstates;		/* number of states */
    int ncolors;		/* number of colors */
    int flags;
#define	HASLACONS	01	/* uses lookahead constraints */
    int pre;			/* setup state number */
    int post;			/* teardown state number */
    color bos[2];		/* colors, if any, assigned to BOS and BOL */
    color eos[2];		/* colors, if any, assigned to EOS and EOL */
    char *stflags;		/* vector of per-state flags bytes */
#define CNFA_NOPROGRESS	01	/* flag bit for a no-progress state */
    struct carc **states;	/* vector of pointers to outarc lists */
    /* states[n] are pointers into a single malloc'd array of arcs */
    struct carc *arcs;		/* the area for the lists */
};
#define	ZAPCNFA(cnfa)	((cnfa).nstates = 0)
#define	NULLCNFA(cnfa)	((cnfa).nstates == 0)

/*
 * This symbol limits the transient heap space used by the regex compiler,
 * and thereby also the maximum complexity of NFAs that we'll deal with.
 * Currently we only count NFA states and arcs against this; the other
 * transient data is generally not large enough to notice compared to those.
 * Note that we do not charge anything for the final output data structures
 * (the compacted NFA and the colormap).
 */
#ifndef REG_MAX_COMPILE_SPACE
#define REG_MAX_COMPILE_SPACE  \
	(100000 * sizeof(struct state) + 100000 * sizeof(struct arcbatch))
#endif

/*
 * subexpression tree
 *
 * "op" is one of:
 *	'='  plain regex without interesting substructure (implemented as DFA)
 *	'b'  back-reference (has no substructure either)
 *	'('  capture node: captures the match of its single child
 *	'.'  concatenation: matches a match for left, then a match for right
 *	'|'  alternation: matches a match for left or a match for right
 *	'*'  iteration: matches some number of matches of its single child
 *
 * Note: the right child of an alternation must be another alternation or
 * NULL; hence, an N-way branch requires N alternation nodes, not N-1 as you
 * might expect.  This could stand to be changed.  Actually I'd rather see
 * a single alternation node with N children, but that will take revising
 * the representation of struct subre.
 *
 * Note: when a backref is directly quantified, we stick the min/max counts
 * into the backref rather than plastering an iteration node on top.  This is
 * for efficiency: there is no need to search for possible division points.
 */

struct subre {
    char op;			/* see type codes above */
    char flags;
#define	LONGER	01		/* prefers longer match */
#define	SHORTER	02		/* prefers shorter match */
#define	MIXED	04		/* mixed preference below */
#define	CAP	010		/* capturing parens below */
#define	BACKR	020		/* back reference below */
#define	INUSE	0100		/* in use in final tree */
#define	NOPROP	03		/* bits which may not propagate up */
#define	LMIX(f)	((f)<<2)	/* LONGER -> MIXED */
#define	SMIX(f)	((f)<<1)	/* SHORTER -> MIXED */
#define	UP(f)	(((f)&~NOPROP) | (LMIX(f) & SMIX(f) & MIXED))
#define	MESSY(f)	((f)&(MIXED|CAP|BACKR))
#define	PREF(f)	((f)&NOPROP)
#define	PREF2(f1, f2)	((PREF(f1) != 0) ? PREF(f1) : PREF(f2))
#define	COMBINE(f1, f2)	(UP((f1)|(f2)) | PREF2(f1, f2))
    short id;			/* ID of subre (1..ntree-1) */
    int subno;			/* subexpression number (for 'b' and '(') */
    short min;			/* min repetitions for iteration or backref */
    short max;			/* max repetitions for iteration or backref */
    struct subre *left;		/* left child, if any (also freelist chain) */
    struct subre *right;	/* right child, if any */
    struct state *begin;	/* outarcs from here... */
    struct state *end;		/* ...ending in inarcs here */
    struct cnfa cnfa;		/* compacted NFA, if any */
    struct subre *chain;	/* for bookkeeping and error cleanup */
};

/*
 * table of function pointers for generic manipulation functions. A regex_t's
 * re_fns points to one of these.
 */

struct fns {
    void FUNCPTR(free, (regex_t *));
};

/*
 * the insides of a regex_t, hidden behind a void *
 */

struct guts {
    int magic;
#define	GUTSMAGIC	0xfed9
    int cflags;			/* copy of compile flags */
    long info;			/* copy of re_info */
    size_t nsub;		/* copy of re_nsub */
    struct subre *tree;
    struct cnfa search;		/* for fast preliminary search */
    int ntree;			/* number of subre's, plus one */
    struct colormap cmap;
    int FUNCPTR(compare, (const chr *, const chr *, size_t));
    struct subre *lacons;	/* lookahead-constraint vector */
    int nlacons;		/* size of lacons */
};

/*
 * Magic for allocating a variable workspace. This default version is
 * stack-hungry.
 */

#ifndef AllocVars
#define AllocVars(vPtr) \
    struct vars var; \
    register struct vars *vPtr = &var
#endif
#ifndef FreeVars
#define FreeVars(vPtr) ((void) 0)
#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
