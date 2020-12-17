/*
 *----------------------------------------------------------------------
 *
 * tclTomMathInterface.c --
 *
 *	This file contains procedures that are used as a 'glue' layer between
 *	Tcl and libtommath.
 *
 * Copyright (c) 2005 by Kevin B. Kenny.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tommath.h"

MODULE_SCOPE const TclTomMathStubs tclTomMathStubs;

/*
 *----------------------------------------------------------------------
 *
 * TclTommath_Init --
 *
 *	Initializes the TclTomMath 'package', which exists as a
 *	placeholder so that the package data can be used to hold
 *	a stub table pointer.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Installs the stub table for tommath.
 *
 *----------------------------------------------------------------------
 */

int
TclTommath_Init(
    Tcl_Interp *interp)		/* Tcl interpreter */
{
    /* TIP #268: Full patchlevel instead of just major.minor */

    if (Tcl_PkgProvideEx(interp, "tcl::tommath", TCL_PATCH_LEVEL,
	    &tclTomMathStubs) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclBN_epoch --
 *
 *	Return the epoch number of the TclTomMath stubs table
 *
 * Results:
 *	Returns an arbitrary integer that does not decrease with
 *	release.  Stubs tables with different epochs are incompatible.
 *
 *----------------------------------------------------------------------
 */

int
TclBN_epoch(void)
{
    return TCLTOMMATH_EPOCH;
}

/*
 *----------------------------------------------------------------------
 *
 * TclBN_revision --
 *
 *	Returns the revision level of the TclTomMath stubs table
 *
 * Results:
 *	Returns an arbitrary integer that increases with revisions.
 *	If a client requires a given epoch and revision, any Stubs table
 *	with the same epoch and an equal or higher revision satisfies
 *	the request.
 *
 *----------------------------------------------------------------------
 */

int
TclBN_revision(void)
{
    return TCLTOMMATH_REVISION;
}

/*
 *----------------------------------------------------------------------
 *
 * TclBNInitBignumFromLong --
 *
 *	Allocate and initialize a 'bignum' from a native 'long'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The 'bignum' is constructed.
 *
 *----------------------------------------------------------------------
 */

int
TclBNInitBignumFromLong(
    mp_int *a,
    long initVal)
{
    int status;
    unsigned long v;
    mp_digit *p;

    /*
     * Allocate enough memory to hold the largest possible long
     */

    status = mp_init(a);
    if (status != MP_OKAY) {
	Tcl_Panic("initialization failure in TclBNInitBignumFromLong");
    }

    /*
     * Convert arg to sign and magnitude.
     */

    if (initVal < 0) {
	a->sign = MP_NEG;
	v = -initVal;
    } else {
	a->sign = MP_ZPOS;
	v = initVal;
    }

    /*
     * Store the magnitude in the bignum.
     */

    p = a->dp;
    while (v) {
	*p++ = (mp_digit) (v & MP_MASK);
	v >>= MP_DIGIT_BIT;
    }
    a->used = p - a->dp;
    return MP_OKAY;
}

/*
 *----------------------------------------------------------------------
 *
 * TclBNInitBignumFromWideInt --
 *
 *	Allocate and initialize a 'bignum' from a Tcl_WideInt
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The 'bignum' is constructed.
 *
 *----------------------------------------------------------------------
 */

int
TclBNInitBignumFromWideInt(
    mp_int *a,			/* Bignum to initialize */
    Tcl_WideInt v)		/* Initial value */
{
    if (v < (Tcl_WideInt)0) {
	TclBNInitBignumFromWideUInt(a, (Tcl_WideUInt)(-v));
	mp_neg(a, a);
    } else {
	TclBNInitBignumFromWideUInt(a, (Tcl_WideUInt)v);
    }
    return MP_OKAY;
}

/*
 *----------------------------------------------------------------------
 *
 * TclBNInitBignumFromWideUInt --
 *
 *	Allocate and initialize a 'bignum' from a Tcl_WideUInt
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The 'bignum' is constructed.
 *
 *----------------------------------------------------------------------
 */

int
TclBNInitBignumFromWideUInt(
    mp_int *a,			/* Bignum to initialize */
    Tcl_WideUInt v)		/* Initial value */
{
    int status;
    mp_digit *p;

    /*
     * Allocate enough memory to hold the largest possible Tcl_WideUInt.
     */

    status = mp_init(a);
    if (status != MP_OKAY) {
	Tcl_Panic("initialization failure in TclBNInitBignumFromWideUInt");
    }

    a->sign = 0;

    /*
     * Store the magnitude in the bignum.
     */

    p = a->dp;
    while (v) {
	*p++ = (mp_digit) (v & MP_MASK);
	v >>= MP_DIGIT_BIT;
    }
    a->used = p - a->dp;
    return MP_OKAY;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
