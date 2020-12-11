
/*	$Id: tixGrUtl.c,v 1.2 2004/03/28 02:44:56 hobbs Exp $	*/

/* 
 * tixGrUtl.c --
 *
 *	Utility functions for Grid
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixDef.h>
#include <tixGrid.h>

#ifndef UCHAR
#define UCHAR(c) ((unsigned char) (c))
#endif

/* string must be a real number plus "char". E.g, "3.0char" */
int
Tix_GetChars(interp, string, doublePtr)
    Tcl_Interp *interp;		/* Use this for error reporting. */
    CONST84 char *string;	/* String describing a justification style. */
    double *doublePtr;		/* Place to store converted result. */
{
    char *end;
    double d;

    d = strtod(string, &end);
    if (end == string) {
	goto error;
    }
    while ((*end != '\0') && isspace(*end)) {
	end++;
    }
    if (strncmp(end, "char", 4) != 0) {
	goto error;
    }
    for (end+=4; (*end != '\0') && isspace(UCHAR(*end)); end++) {
	;
    }
    if (*end != '\0') {
	goto error;
    }
    if (d < 0) {
	goto error;
    }

    *doublePtr = d;
    return TCL_OK;

  error:
    Tcl_AppendResult(interp, "bad screen distance \"", string,
	"\"", (char *) NULL);
    return TCL_ERROR;
}


int Tix_GrConfigSize(interp, wPtr, argc, argv, sizePtr, argcErrorMsg,
	changed_ret)
    Tcl_Interp *interp;
    WidgetPtr wPtr;
    int argc;
    CONST84 char **argv;
    TixGridSize *sizePtr;
    CONST84 char * argcErrorMsg;
    int *changed_ret;
{
    int pixels;
    double chars;
    int i;
    TixGridSize newSize;
    int changed = 0;

    if (argc == 0) {
	char buff[40];

	Tcl_AppendResult(interp, "-size ", NULL);

	switch (sizePtr->sizeType) {
	  case TIX_GR_AUTO:
	    Tcl_AppendResult(interp, "auto", NULL);
	    break;

	  case TIX_GR_DEFAULT:
	    Tcl_AppendResult(interp, "default", NULL);
	    break;

	  case TIX_GR_DEFINED_PIXEL:
	    sprintf(buff, "%d", sizePtr->sizeValue);
	    Tcl_AppendResult(interp, buff, NULL);
	    break;

	  case TIX_GR_DEFINED_CHAR:
	    sprintf(buff, "%fchar", sizePtr->charValue);
	    Tcl_AppendResult(interp, buff, NULL);
	    break;

	  default:
	    Tcl_AppendResult(interp, "default", NULL);
	    break;
	}

	Tcl_AppendResult(interp, " -pad0 ", NULL);
	sprintf(buff, "%d", sizePtr->pad0);
	Tcl_AppendResult(interp, buff, NULL);

	Tcl_AppendResult(interp, " -pad1 ", NULL);
	sprintf(buff, "%d", sizePtr->pad1);
	Tcl_AppendResult(interp, buff, NULL);

	return TCL_OK;
    }

    if ((argc %2) != 0) {
	Tcl_AppendResult(interp, "value missing for option \"",
	    argv[argc-1], "\"", NULL);
	return TCL_ERROR;
    }

    newSize = *sizePtr;

    for (i=0; i<argc; i+=2) {

	if (strncmp("-size", argv[i], strlen(argv[i])) == 0) {
	    if (strcmp(argv[i+1], "auto")==0) {
		newSize.sizeType  = TIX_GR_AUTO;
		newSize.sizeValue = 0;
	    }
	    else if (strcmp(argv[i+1], "default")==0) {
		newSize.sizeType  = TIX_GR_DEFAULT;
		newSize.sizeValue = 0;
	    }
	    else if (Tk_GetPixels(interp, wPtr->dispData.tkwin, argv[i+1],
		 &pixels) == TCL_OK) {

		newSize.sizeType  = TIX_GR_DEFINED_PIXEL;
		newSize.sizeValue = pixels;
	    }
	    else {
		Tcl_ResetResult(interp);
		if (Tix_GetChars(interp, argv[i+1], &chars) == TCL_OK) {
		    newSize.sizeType  = TIX_GR_DEFINED_CHAR;
		    newSize.charValue = chars;
		}
		else {
		    return TCL_ERROR;
		}
	    }
	}
	else if (strcmp("-pad0", argv[i]) == 0) {
	    if (Tk_GetPixels(interp, wPtr->dispData.tkwin, argv[i+1],
		 &pixels) == TCL_OK) {

		newSize.pad0 = pixels;
	    }
	    else {
		return TCL_ERROR;
	    }
	}
	else if (strcmp("-pad1", argv[i]) == 0) {
	    if (Tk_GetPixels(interp, wPtr->dispData.tkwin, argv[i+1],
		 &pixels) == TCL_OK) {

		newSize.pad1 = pixels;
	    }
	    else {
		return TCL_ERROR;
	    }
	}
	else {
	    Tcl_AppendResult(interp, "Unknown option \"", argv[i],
		"\"; must be -pad0, -pad1 or -size", NULL);
	    return TCL_ERROR;
	}
    }

    if (changed_ret) {
	if (sizePtr->sizeType  != newSize.sizeType) {
	    changed = 1;
	}
	if (sizePtr->sizeValue != newSize.sizeValue) {
	    changed = 1;
	}
	if (sizePtr->charValue != newSize.charValue) {
	    changed = 1;
	}
	if (sizePtr->pad1      != newSize.pad0) {
	    changed = 1;
	}
	if (sizePtr->pad1      != newSize.pad1) {
	    changed = 1;
	}
	*changed_ret = changed;
    }

    *sizePtr = newSize;
    return TCL_OK;
}
