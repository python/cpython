#include "config.h"

/* comp.sources.misc strtod(), as posted in comp.lang.tcl,
   with bugfix for "123000.0" and acceptance of space after 'e' sign nuked.

   ************************************************************
   * YOU MUST EDIT THE MACHINE-DEPENDENT DEFINITIONS BELOW!!! *
   ************************************************************
*/

/*  File   : stdtod.c (Modified version of str2dbl.c)
    Author : Richard A. O'Keefe @ Quintus Computer Systems, Inc.
    Updated: Tuesday August 2nd, 1988
    Defines: double strtod (char *str, char**ptr)
*/

/*  This is an implementation of the strtod() function described in the 
    System V manuals, with a different name to avoid linker problems.
    All that str2dbl() does itself is check that the argument is well-formed
    and is in range.  It leaves the work of conversion to atof(), which is
    assumed to exist and deliver correct results (if they can be represented).

    There are two reasons why this should be provided to the net:
    (a) some UNIX systems do not yet have strtod(), or do not have it
        available in the BSD "universe" (but they do have atof()).
    (b) some of the UNIX systems that *do* have it get it wrong.
	(some crash with large arguments, some assign the wrong *ptr value).
    There is a reason why *we* are providing it: we need a correct version
    of strtod(), and if we give this one away maybe someone will look for
    mistakes in it and fix them for us (:-).
*/
    
/*  The following constants are machine-specific.  MD{MIN,MAX}EXPT are
    integers and MD{MIN,MAX}FRAC are strings such that
	0.${MDMAXFRAC}e${MDMAXEXPT} is the largest representable double,
	0.${MDMINFRAC}e${MDMINEXPT} is the smallest representable +ve double
    MD{MIN,MAX}FRAC must not have any trailing zeros.
    The values here are for IEEE-754 64-bit floats.
    It is not perfectly clear to me whether an IEEE infinity should be
    returned for overflow, nor what a portable way of writing one is,
    so HUGE is just 0.MAXFRAC*10**MAXEXPT (this seems still to be the
    UNIX convention).

    I do know about <values.h>, but the whole point of this file is that
    we can't always trust that stuff to be there or to be correct.
*/
static	int	MDMINEXPT	= {-323};
static	char	MDMINFRAC[]	= "494065645841246544";
static	double	ZERO		= 0.0;

static	int	MDMAXEXPT	= { 309};
static	char	MDMAXFRAC[]	= "17976931348623157";
static	double	HUGE		= 1.7976931348623157e308;

extern	double	atof(const char *);		/* Only called when result known to be ok */

#ifndef DONT_HAVE_ERRNO_H
#include <errno.h>
#endif
extern	int	errno;

double strtod(char *str, char **ptr)
{
	int sign, scale, dotseen;
	int esign, expt;
	char *save;
	register char *sp, *dp;
	register int c;
	char *buforg, *buflim;
	char buffer[64];		/* 45-digit significant + */
					/* 13-digit exponent */
	sp = str;
	while (*sp == ' ') sp++;
	sign = 1;
	if (*sp == '-') sign -= 2, sp++;
	dotseen = 0, scale = 0;
	dp = buffer;	
	*dp++ = '0'; *dp++ = '.';
	buforg = dp, buflim = buffer+48;
	for (save = sp; c = *sp; sp++)
	    if (c == '.') {
		if (dotseen) break;
		dotseen++;
	    } else
	    if ((unsigned)(c-'0') > (unsigned)('9'-'0')) {
		break;
	    } else
	    if (c == '0') {
		if (dp != buforg) {
		    /* This is not the first digit, so we want to keep it */
		    if (dp < buflim) *dp++ = c;
		    if (!dotseen) scale++;
		} else {
		    /* No non-zero digits seen yet */
		    /* If a . has been seen, scale must be adjusted */
		    if (dotseen) scale--;
		}
	    } else {
		/* This is a nonzero digit, so we want to keep it */
		if (dp < buflim) *dp++ = c;
		/* If it precedes a ., scale must be adjusted */
		if (!dotseen) scale++;
	    }
	if (sp == save) {
	    if (ptr) *ptr = str;
	    errno = EDOM;		/* what should this be? */
	    return ZERO;
	}
	
	while (dp > buforg && dp[-1] == '0') --dp;
	if (dp == buforg) *dp++ = '0';
	*dp = '\0';
	/*  Now the contents of buffer are
	    +--+--------+-+--------+
	    |0.|fraction|\|leftover|
	    +--+--------+-+--------+
			 ^dp points here
	    where fraction begins with 0 iff it is "0", and has at most
	    45 digits in it, and leftover is at least 16 characters.
	*/
	save = sp, expt = 0, esign = 1;
	do {
	    c = *sp++;
	    if (c != 'e' && c != 'E') break;
	    c = *sp++;
	    if (c == '-') esign -= 2, c = *sp++; else
	    if (c == '+' /* || c == ' ' */ ) c = *sp++;
	    if ((unsigned)(c-'0') > (unsigned)('9'-'0')) break;
	    while (c == '0') c = *sp++;
	    for (; (unsigned)(c-'0') <= (unsigned)('9'-'0'); c = *sp++)
		expt = expt*10 + c-'0';	    
	    if (esign < 0) expt = -expt;
	    save = sp-1;
	} while (0);
	if (ptr) *ptr = save;
	expt += scale;
	/*  Now the number is sign*0.fraction*10**expt  */
	errno = ERANGE;
	if (expt > MDMAXEXPT) {
	    return HUGE*sign;
	} else
	if (expt == MDMAXEXPT) {
	    if (strcmp(buforg, MDMAXFRAC) > 0) return HUGE*sign;
	} else
	if (expt < MDMINEXPT) {
	    return ZERO*sign;
	} else
	if (expt == MDMINEXPT) {
	    if (strcmp(buforg, MDMINFRAC) < 0) return ZERO*sign;
	}
	/*  We have now established that the number can be  */
	/*  represented without overflow or underflow  */
	(void) sprintf(dp, "E%d", expt);
	errno = 0;
	return atof(buffer)*sign;
}
