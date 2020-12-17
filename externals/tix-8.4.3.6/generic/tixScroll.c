
/*	$Id: tixScroll.c,v 1.2 2004/03/28 02:44:57 hobbs Exp $	*/

/*
 * tixScroll.c -- Handle all the mess of Tk scroll bars
 *
 *
 *
 */

#include <tixInt.h>


void Tix_InitScrollInfo(siPtr, type)
    Tix_ScrollInfo * siPtr;
    int type;
{
    Tix_IntScrollInfo*    isiPtr = (Tix_IntScrollInfo*)   siPtr;
    Tix_DoubleScrollInfo* dsiPtr = (Tix_DoubleScrollInfo*)siPtr;

    siPtr->command 	= NULL;
    siPtr->type 	= type;

    if (type == TIX_SCROLL_INT) {
	isiPtr->total	= 1;
	isiPtr->window	= 1;
	isiPtr->offset	= 0;
	isiPtr->unit	= 1;
    }
    else {
	dsiPtr->total	= 1.0;
	dsiPtr->window	= 1.0;
	dsiPtr->offset	= 0.0;
	dsiPtr->unit	= 1.0;
    }
}

/*----------------------------------------------------------------------
 * Tix_GetScrollFractions --
 *
 * Compute the fractions of a scroll-able widget.
 *
 */
void Tix_GetScrollFractions(siPtr, first_ret, last_ret)
    Tix_ScrollInfo * siPtr;
    double * first_ret;
    double * last_ret;
{
    Tix_IntScrollInfo*    isiPtr = (Tix_IntScrollInfo*)   siPtr;
    Tix_DoubleScrollInfo* dsiPtr = (Tix_DoubleScrollInfo*)siPtr;
    double total, window, first;

    if (siPtr->type == TIX_SCROLL_INT) {
	total  = isiPtr->total;
	window = isiPtr->window;
	first  = isiPtr->offset;
    } else {
	total  = dsiPtr->total;
	window = dsiPtr->window;
	first  = dsiPtr->offset;
    }

    if (total == 0 || total < window) {
	*first_ret = 0.0;
	*last_ret  = 1.0;
    } else {
	*first_ret = first / total;
	*last_ret  = (first+window) / total;
    }
}

void Tix_UpdateScrollBar(interp, siPtr)
    Tcl_Interp *interp;
    Tix_ScrollInfo * siPtr;
{
    Tix_IntScrollInfo*    isiPtr = (Tix_IntScrollInfo*)   siPtr;
    Tix_DoubleScrollInfo* dsiPtr = (Tix_DoubleScrollInfo*)siPtr;
    double d_first, d_last;
    char string[100];

    if (siPtr->type == TIX_SCROLL_INT) {
	/* Check whether the topPixel is out of bound */
	if (isiPtr->offset < 0) {
	    isiPtr->offset = 0;
	} else {
	    if (isiPtr->window > isiPtr->total) {
		isiPtr->offset = 0;
	    }
	    else if((isiPtr->offset+isiPtr->window) > isiPtr->total) {
		isiPtr->offset = isiPtr->total - isiPtr->window;
	    }
	}
    } else {
	/* Check whether the topPixel is out of bound */
	if (dsiPtr->offset < 0) {
	    dsiPtr->offset = 0;
	} else {
	    if (dsiPtr->window > dsiPtr->total) {
		dsiPtr->offset = 0;
	    }
	    else if((dsiPtr->offset+dsiPtr->window) > dsiPtr->total) {
		dsiPtr->offset = dsiPtr->total - dsiPtr->window;
	    }
	}
    }


    if (siPtr->command) {
	Tix_GetScrollFractions(siPtr, &d_first, &d_last);

	sprintf(string, " %f %f", d_first, d_last);
	if (Tcl_VarEval(interp, siPtr->command, string, 
	    (char *) NULL) != TCL_OK) {
	    Tcl_AddErrorInfo(interp,
		"\n    (scrolling command executed by tixTList)");
	    Tk_BackgroundError(interp);
	}
    }
}

int Tix_SetScrollBarView(interp, siPtr, argc, argv, compat)
    Tcl_Interp *interp;		/* Current interpreter. */
    Tix_ScrollInfo * siPtr;
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
    int compat;			/* compatibility mode */
{
    Tix_IntScrollInfo*    isiPtr = (Tix_IntScrollInfo*)   siPtr;
    Tix_DoubleScrollInfo* dsiPtr = (Tix_DoubleScrollInfo*)siPtr;
    int offset;

    if (compat && Tcl_GetInt(interp, argv[0], &offset) == TCL_OK) {
	/* backward-compatible mode */
	if (siPtr->type == TIX_SCROLL_INT) {
	    isiPtr->offset = offset;
	}
	else {
	    dsiPtr->offset = (double)offset;
	}

	return TCL_OK;
    }
    else {
	int type, count;
	double fraction;

	Tcl_ResetResult(interp);

	/* Tk_GetScrollInfo () wants strange argc,argv combinations .. */
	type = Tk_GetScrollInfo(interp, argc+2, argv-2, &fraction, &count);

	if (siPtr->type == TIX_SCROLL_INT) {
	    switch (type) {
	      case TK_SCROLL_ERROR:
		return TCL_ERROR;

	      case TK_SCROLL_MOVETO:
		isiPtr->offset = 
		  (int)(fraction * (double)isiPtr->total);
		break;

	      case TK_SCROLL_PAGES:
		isiPtr->offset += count * isiPtr->window;
		break;

	      case TK_SCROLL_UNITS:
		isiPtr->offset += count * isiPtr->unit;
		break;
	    }
	} else {
	    switch (type) {
	      case TK_SCROLL_ERROR:
		return TCL_ERROR;

	      case TK_SCROLL_MOVETO:
		dsiPtr->offset = 
		  fraction * dsiPtr->total;
		break;

	      case TK_SCROLL_PAGES:
		dsiPtr->offset += count * dsiPtr->window;
		break;

	      case TK_SCROLL_UNITS:
		dsiPtr->offset += count * dsiPtr->unit;
		break;
	    }
	}
    }
    return TCL_OK;
}
