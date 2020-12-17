/*
 * Copyright (c) 1998, 1999 Henry Spencer.  All rights reserved.
 *
 * Development of this software was funded, in part, by Cray Research Inc.,
 * UUNET Communications Services Inc., Sun Microsystems Inc., and Scriptics
 * Corporation, none of whom are responsible for the results. The author
 * thanks all of them.
 *
 * Redistribution and use in source and binary forms - with or without
 * modification - are permitted for any purpose, provided that redistributions
 * in source form retain this entire copyright notice and indicate the origin
 * and nature of any modifications.
 *
 * I'd appreciate being given credit for this package in the documentation of
 * software which uses it, but that is not a requirement.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * HENRY SPENCER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Headers if any.
 */

#include "regex.h"

/*
 * Overrides for regguts.h definitions, if any.
 */

#define	FUNCPTR(name, args)	(*name)args
#define	MALLOC(n)		VS(attemptckalloc(n))
#define	FREE(p)			ckfree(VS(p))
#define	REALLOC(p,n)		VS(attemptckrealloc(VS(p),n))

/*
 * Do not insert extras between the "begin" and "end" lines - this chunk is
 * automatically extracted to be fitted into regex.h.
 */

/* --- begin --- */
/* Ensure certain things don't sneak in from system headers. */
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
/* Interface types */
#define	__REG_WIDE_T	Tcl_UniChar
#define	__REG_REGOFF_T	long	/* Not really right, but good enough... */
/* Names and declarations */
#define	__REG_WIDE_COMPILE	TclReComp
#define	__REG_WIDE_EXEC		TclReExec
#define	__REG_NOFRONT		/* Don't want regcomp() and regexec() */
#define	__REG_NOCHAR		/* Or the char versions */
#define	regfree		TclReFree
#define	regerror	TclReError
/* --- end --- */

/*
 * Internal character type and related.
 */

typedef Tcl_UniChar chr;	/* The type itself. */
typedef int pchr;		/* What it promotes to. */
typedef unsigned uchr;		/* Unsigned type that will hold a chr. */
typedef int celt;		/* Type to hold chr, or NOCELT */
#define	NOCELT (-1)		/* Celt value which is not valid chr */
#define	CHR(c) (UCHAR(c))	/* Turn char literal into chr literal */
#define	DIGITVAL(c) ((c)-'0')	/* Turn chr digit into its value */
#if TCL_UTF_MAX > 4
#define	CHRBITS	32		/* Bits in a chr; must not use sizeof */
#define	CHR_MIN	0x00000000	/* Smallest and largest chr; the value */
#define	CHR_MAX	0xffffffff	/* CHR_MAX-CHR_MIN+1 should fit in uchr */
#else
#define	CHRBITS	16		/* Bits in a chr; must not use sizeof */
#define	CHR_MIN	0x0000		/* Smallest and largest chr; the value */
#define	CHR_MAX	0xffff		/* CHR_MAX-CHR_MIN+1 should fit in uchr */
#endif

/*
 * Functions operating on chr.
 */

#define	iscalnum(x)	Tcl_UniCharIsAlnum(x)
#define	iscalpha(x)	Tcl_UniCharIsAlpha(x)
#define	iscdigit(x)	Tcl_UniCharIsDigit(x)
#define	iscspace(x)	Tcl_UniCharIsSpace(x)

/*
 * Name the external functions.
 */

#define	compile		TclReComp
#define	exec		TclReExec

/*
& Enable/disable debugging code (by whether REG_DEBUG is defined or not).
*/

#if 0				/* No debug unless requested by makefile. */
#define	REG_DEBUG	/* */
#endif

/*
 * Method of allocating a local workspace. We used a thread-specific data
 * space to store this because the regular expression engine is never
 * reentered from the same thread; it doesn't make any callbacks.
 */

#if 1
#define AllocVars(vPtr) \
    static Tcl_ThreadDataKey varsKey; \
    register struct vars *vPtr = (struct vars *) \
	    Tcl_GetThreadData(&varsKey, sizeof(struct vars))
#else
/*
 * This strategy for allocating workspace is "more proper" in some sense, but
 * quite a bit slower. Using TSD (as above) leads to code that is quite a bit
 * faster in practice (measured!)
 */
#define AllocVars(vPtr) \
    register struct vars *vPtr = (struct vars *) MALLOC(sizeof(struct vars))
#define FreeVars(vPtr) \
    FREE(vPtr)
#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
