#ifndef _REGEX_H_
#define	_REGEX_H_	/* never again */

#include "tclInt.h"

/*
 * regular expressions
 *
 * Copyright (c) 1998, 1999 Henry Spencer.  All rights reserved.
 *
 * Development of this software was funded, in part, by Cray Research Inc.,
 * UUNET Communications Services Inc., Sun Microsystems Inc., and Scriptics
 * Corporation, none of whom are responsible for the results. The author
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
 *
 *
 * Prototypes etc. marked with "^" within comments get gathered up (and
 * possibly edited) by the regfwd program and inserted near the bottom of this
 * file.
 *
 * We offer the option of declaring one wide-character version of the RE
 * functions as well as the char versions. To do that, define __REG_WIDE_T to
 * the type of wide characters (unfortunately, there is no consensus that
 * wchar_t is suitable) and __REG_WIDE_COMPILE and __REG_WIDE_EXEC to the
 * names to be used for the compile and execute functions (suggestion:
 * re_Xcomp and re_Xexec, where X is a letter suggestive of the wide type,
 * e.g. re_ucomp and re_uexec for Unicode). For cranky old compilers, it may
 * be necessary to do something like:
 * #define	__REG_WIDE_COMPILE(a,b,c,d)	re_Xcomp(a,b,c,d)
 * #define	__REG_WIDE_EXEC(a,b,c,d,e,f,g)	re_Xexec(a,b,c,d,e,f,g)
 * rather than just #defining the names as parameterless macros.
 *
 * For some specialized purposes, it may be desirable to suppress the
 * declarations of the "front end" functions, regcomp() and regexec(), or of
 * the char versions of the compile and execute functions. To suppress the
 * front-end functions, define __REG_NOFRONT. To suppress the char versions,
 * define __REG_NOCHAR.
 *
 * The right place to do those defines (and some others you may want, see
 * below) would be <sys/types.h>. If you don't have control of that file, the
 * right place to add your own defines to this file is marked below. This is
 * normally done automatically, by the makefile and regmkhdr, based on the
 * contents of regcustom.h.
 */

/*
 * voodoo for C++
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Add your own defines, if needed, here.
 */

/*
 * Location where a chunk of regcustom.h is automatically spliced into this
 * file (working from its prototype, regproto.h).
 */

/* --- begin --- */
/* ensure certain things don't sneak in from system headers */
#ifdef __REG_WIDE_T
#undef __REG_WIDE_T
#endif
#ifdef __REG_WIDE_COMPILE
#undef __REG_WIDE_COMPILE
#endif
#ifdef __REG_WIDE_EXEC
#undef __REG_WIDE_EXEC
#endif
#ifdef __REG_REGOFF_T
#undef __REG_REGOFF_T
#endif
#ifdef __REG_NOFRONT
#undef __REG_NOFRONT
#endif
#ifdef __REG_NOCHAR
#undef __REG_NOCHAR
#endif
/* interface types */
#define	__REG_WIDE_T	Tcl_UniChar
#define	__REG_REGOFF_T	long	/* not really right, but good enough... */
/* names and declarations */
#define	__REG_WIDE_COMPILE	TclReComp
#define	__REG_WIDE_EXEC		TclReExec
#define	__REG_NOFRONT		/* don't want regcomp() and regexec() */
#define	__REG_NOCHAR		/* or the char versions */
#define	regfree		TclReFree
#define	regerror	TclReError
/* --- end --- */

/*
 * interface types etc.
 */

/*
 * regoff_t has to be large enough to hold either off_t or ssize_t, and must
 * be signed; it's only a guess that long is suitable, so we offer
 * <sys/types.h> an override.
 */
#ifdef __REG_REGOFF_T
typedef __REG_REGOFF_T regoff_t;
#else
typedef long regoff_t;
#endif

/*
 * other interface types
 */

/* the biggie, a compiled RE (or rather, a front end to same) */
typedef struct {
    int re_magic;		/* magic number */
    size_t re_nsub;		/* number of subexpressions */
    long re_info;		/* information about RE */
#define	REG_UBACKREF		000001
#define	REG_ULOOKAHEAD		000002
#define	REG_UBOUNDS		000004
#define	REG_UBRACES		000010
#define	REG_UBSALNUM		000020
#define	REG_UPBOTCH		000040
#define	REG_UBBS		000100
#define	REG_UNONPOSIX		000200
#define	REG_UUNSPEC		000400
#define	REG_UUNPORT		001000
#define	REG_ULOCALE		002000
#define	REG_UEMPTYMATCH		004000
#define	REG_UIMPOSSIBLE		010000
#define	REG_USHORTEST		020000
    int re_csize;		/* sizeof(character) */
    char *re_endp;		/* backward compatibility kludge */
    /* the rest is opaque pointers to hidden innards */
    char *re_guts;		/* `char *' is more portable than `void *' */
    char *re_fns;
} regex_t;

/* result reporting (may acquire more fields later) */
typedef struct {
    regoff_t rm_so;		/* start of substring */
    regoff_t rm_eo;		/* end of substring */
} regmatch_t;

/* supplementary control and reporting */
typedef struct {
    regmatch_t rm_extend;	/* see REG_EXPECT */
} rm_detail_t;

/*
 * compilation
 ^ #ifndef __REG_NOCHAR
 ^ int re_comp(regex_t *, const char *, size_t, int);
 ^ #endif
 ^ #ifndef __REG_NOFRONT
 ^ int regcomp(regex_t *, const char *, int);
 ^ #endif
 ^ #ifdef __REG_WIDE_T
 ^ int __REG_WIDE_COMPILE(regex_t *, const __REG_WIDE_T *, size_t, int);
 ^ #endif
 */
#define	REG_BASIC	000000	/* BREs (convenience) */
#define	REG_EXTENDED	000001	/* EREs */
#define	REG_ADVF	000002	/* advanced features in EREs */
#define	REG_ADVANCED	000003	/* AREs (which are also EREs) */
#define	REG_QUOTE	000004	/* no special characters, none */
#define	REG_NOSPEC	REG_QUOTE	/* historical synonym */
#define	REG_ICASE	000010	/* ignore case */
#define	REG_NOSUB	000020	/* don't care about subexpressions */
#define	REG_EXPANDED	000040	/* expanded format, white space & comments */
#define	REG_NLSTOP	000100	/* \n doesn't match . or [^ ] */
#define	REG_NLANCH	000200	/* ^ matches after \n, $ before */
#define	REG_NEWLINE	000300	/* newlines are line terminators */
#define	REG_PEND	000400	/* ugh -- backward-compatibility hack */
#define	REG_EXPECT	001000	/* report details on partial/limited matches */
#define	REG_BOSONLY	002000	/* temporary kludge for BOS-only matches */
#define	REG_DUMP	004000	/* none of your business :-) */
#define	REG_FAKE	010000	/* none of your business :-) */
#define	REG_PROGRESS	020000	/* none of your business :-) */

/*
 * execution
 ^ #ifndef __REG_NOCHAR
 ^ int re_exec(regex_t *, const char *, size_t,
 ^				rm_detail_t *, size_t, regmatch_t [], int);
 ^ #endif
 ^ #ifndef __REG_NOFRONT
 ^ int regexec(regex_t *, const char *, size_t, regmatch_t [], int);
 ^ #endif
 ^ #ifdef __REG_WIDE_T
 ^ int __REG_WIDE_EXEC(regex_t *, const __REG_WIDE_T *, size_t,
 ^				rm_detail_t *, size_t, regmatch_t [], int);
 ^ #endif
 */
#define	REG_NOTBOL	0001	/* BOS is not BOL */
#define	REG_NOTEOL	0002	/* EOS is not EOL */
#define	REG_STARTEND	0004	/* backward compatibility kludge */
#define	REG_FTRACE	0010	/* none of your business */
#define	REG_MTRACE	0020	/* none of your business */
#define	REG_SMALL	0040	/* none of your business */

/*
 * misc generics (may be more functions here eventually)
 ^ void regfree(regex_t *);
 */

/*
 * error reporting
 * Be careful if modifying the list of error codes -- the table used by
 * regerror() is generated automatically from this file!
 *
 * Note that there is no wide-char variant of regerror at this time; what kind
 * of character is used for error reports is independent of what kind is used
 * in matching.
 *
 ^ extern size_t regerror(int, const regex_t *, char *, size_t);
 */
#define	REG_OKAY	 0	/* no errors detected */
#define	REG_NOMATCH	 1	/* failed to match */
#define	REG_BADPAT	 2	/* invalid regexp */
#define	REG_ECOLLATE	 3	/* invalid collating element */
#define	REG_ECTYPE	 4	/* invalid character class */
#define	REG_EESCAPE	 5	/* invalid escape \ sequence */
#define	REG_ESUBREG	 6	/* invalid backreference number */
#define	REG_EBRACK	 7	/* brackets [] not balanced */
#define	REG_EPAREN	 8	/* parentheses () not balanced */
#define	REG_EBRACE	 9	/* braces {} not balanced */
#define	REG_BADBR	10	/* invalid repetition count(s) */
#define	REG_ERANGE	11	/* invalid character range */
#define	REG_ESPACE	12	/* out of memory */
#define	REG_BADRPT	13	/* quantifier operand invalid */
#define	REG_ASSERT	15	/* "can't happen" -- you found a bug */
#define	REG_INVARG	16	/* invalid argument to regex function */
#define	REG_MIXED	17	/* character widths of regex and string differ */
#define	REG_BADOPT	18	/* invalid embedded option */
#define	REG_ETOOBIG	19	/* regular expression is too complex */
#define	REG_ECOLORS	20	/* too many colors */
/* two specials for debugging and testing */
#define	REG_ATOI	101	/* convert error-code name to number */
#define	REG_ITOA	102	/* convert error-code number to name */

/*
 * the prototypes, as possibly munched by regfwd
 */
/* =====^!^===== begin forwards =====^!^===== */
/* automatically gathered by fwd; do not hand-edit */
/* === regproto.h === */
#ifndef __REG_NOCHAR
int re_comp(regex_t *, const char *, size_t, int);
#endif
#ifndef __REG_NOFRONT
int regcomp(regex_t *, const char *, int);
#endif
#ifdef __REG_WIDE_T
MODULE_SCOPE int __REG_WIDE_COMPILE(regex_t *, const __REG_WIDE_T *, size_t, int);
#endif
#ifndef __REG_NOCHAR
int re_exec(regex_t *, const char *, size_t, rm_detail_t *, size_t, regmatch_t [], int);
#endif
#ifndef __REG_NOFRONT
int regexec(regex_t *, const char *, size_t, regmatch_t [], int);
#endif
#ifdef __REG_WIDE_T
MODULE_SCOPE int __REG_WIDE_EXEC(regex_t *, const __REG_WIDE_T *, size_t, rm_detail_t *, size_t, regmatch_t [], int);
#endif
MODULE_SCOPE void regfree(regex_t *);
MODULE_SCOPE size_t regerror(int, const regex_t *, char *, size_t);
/* automatically gathered by fwd; do not hand-edit */
/* =====^!^===== end forwards =====^!^===== */

/*
 * more C++ voodoo
 */
#ifdef __cplusplus
}
#endif

#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
