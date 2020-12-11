/*
 * Tk widget state utilities.
 *
 * Copyright (c) 2003 Joe English.  Freely redistributable.
 *
 */

#include <string.h>

#include <tk.h>
#include "ttkTheme.h"

/*
 * Table of state names.  Must be kept in sync with TTK_STATE_*
 * #defines in ttkTheme.h.
 */
static const char *const stateNames[] =
{
    "active",		/* Mouse cursor is over widget or element */
    "disabled",		/* Widget is disabled */
    "focus",		/* Widget has keyboard focus */
    "pressed",		/* Pressed or "armed" */
    "selected",		/* "on", "true", "current", etc. */
    "background",	/* Top-level window lost focus (Mac,Win "inactive") */
    "alternate",	/* Widget-specific alternate display style */
    "invalid",		/* Bad value */
    "readonly",		/* Editing/modification disabled */
    "hover",		/* Mouse cursor is over widget */
    "reserved1",	/* Reserved for future extension */
    "reserved2",	/* Reserved for future extension */
    "reserved3",	/* Reserved for future extension */
    "user3",		/* User-definable state */
    "user2",		/* User-definable state */
    "user1",		/* User-definable state */
    NULL
};

/*------------------------------------------------------------------------
 * +++ StateSpec object type:
 *
 * The string representation consists of a list of state names,
 * each optionally prefixed by an exclamation point (!).
 *
 * The internal representation uses the upper half of the longValue
 * to store the on bits and the lower half to store the off bits.
 * If we ever get more than 16 states, this will need to be reconsidered...
 */

static int  StateSpecSetFromAny(Tcl_Interp *interp, Tcl_Obj *obj);
/* static void StateSpecFreeIntRep(Tcl_Obj *); */
#define StateSpecFreeIntRep 0		/* not needed */
static void StateSpecDupIntRep(Tcl_Obj *, Tcl_Obj *);
static void StateSpecUpdateString(Tcl_Obj *);

static
struct Tcl_ObjType StateSpecObjType =
{
    "StateSpec",
    StateSpecFreeIntRep,
    StateSpecDupIntRep,
    StateSpecUpdateString,
    StateSpecSetFromAny
};

static void StateSpecDupIntRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
    copyPtr->internalRep.longValue = srcPtr->internalRep.longValue;
    copyPtr->typePtr = &StateSpecObjType;
}

static int StateSpecSetFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    int status;
    int objc;
    Tcl_Obj **objv;
    int i;
    unsigned int onbits = 0, offbits = 0;

    status = Tcl_ListObjGetElements(interp, objPtr, &objc, &objv);
    if (status != TCL_OK)
	return status;

    for (i = 0; i < objc; ++i) {
	const char *stateName = Tcl_GetString(objv[i]);
	int on, j;

	if (*stateName == '!') {
	    ++stateName;
	    on = 0;
	} else {
	    on = 1;
	}

	for (j = 0; stateNames[j] != 0; ++j) {
	    if (strcmp(stateName, stateNames[j]) == 0)
		break;
	}

    	if (stateNames[j] == 0) {
	    if (interp) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"Invalid state name %s", stateName));
		Tcl_SetErrorCode(interp, "TTK", "VALUE", "STATE", NULL);
	    }
	    return TCL_ERROR;
	}

	if (on) {
	    onbits |= (1<<j);
	} else {
	    offbits |= (1<<j);
	}
    }

    /* Invalidate old intrep:
     */
    if (objPtr->typePtr && objPtr->typePtr->freeIntRepProc) {
	objPtr->typePtr->freeIntRepProc(objPtr);
    }

    objPtr->typePtr = &StateSpecObjType;
    objPtr->internalRep.longValue = (onbits << 16) | offbits;

    return TCL_OK;
}

static void StateSpecUpdateString(Tcl_Obj *objPtr)
{
    unsigned int onbits = (objPtr->internalRep.longValue & 0xFFFF0000) >> 16;
    unsigned int offbits = objPtr->internalRep.longValue & 0x0000FFFF;
    unsigned int mask = onbits | offbits;
    Tcl_DString result;
    int i;
    int len;

    Tcl_DStringInit(&result);

    for (i=0; stateNames[i] != NULL; ++i) {
	if (mask & (1<<i)) {
	    if (offbits & (1<<i))
		Tcl_DStringAppend(&result, "!", 1);
	    Tcl_DStringAppend(&result, stateNames[i], -1);
	    Tcl_DStringAppend(&result, " ", 1);
	}
    }

    len = Tcl_DStringLength(&result);
    if (len) {
	/* 'len' includes extra trailing ' ' */
	objPtr->bytes = ckalloc(len);
	objPtr->length = len-1;
	strncpy(objPtr->bytes, Tcl_DStringValue(&result), len-1);
	objPtr->bytes[len-1] = '\0';
    } else {
	/* empty string */
	objPtr->length = 0;
	objPtr->bytes = ckalloc(1);
	*objPtr->bytes = '\0';
    }

    Tcl_DStringFree(&result);
}

Tcl_Obj *Ttk_NewStateSpecObj(unsigned int onbits, unsigned int offbits)
{
    Tcl_Obj *objPtr = Tcl_NewObj();

    Tcl_InvalidateStringRep(objPtr);
    objPtr->typePtr = &StateSpecObjType;
    objPtr->internalRep.longValue = (onbits << 16) | offbits;

    return objPtr;
}

int Ttk_GetStateSpecFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Ttk_StateSpec *spec)
{
    if (objPtr->typePtr != &StateSpecObjType) {
	int status = StateSpecSetFromAny(interp, objPtr);
	if (status != TCL_OK)
	    return status;
    }

    spec->onbits = (objPtr->internalRep.longValue & 0xFFFF0000) >> 16;
    spec->offbits = objPtr->internalRep.longValue & 0x0000FFFF;
    return TCL_OK;
}


/*
 * Tk_StateMapLookup --
 *
 * 	A state map is a paired list of StateSpec / value pairs.
 *	Returns the value corresponding to the first matching state
 *	specification, or NULL if not found or an error occurs.
 */
Tcl_Obj *Ttk_StateMapLookup(
    Tcl_Interp *interp,		/* Where to leave error messages; may be NULL */
    Ttk_StateMap map,		/* State map */
    Ttk_State state)    	/* State to look up */
{
    Tcl_Obj **specs;
    int nSpecs;
    int j, status;

    status = Tcl_ListObjGetElements(interp, map, &nSpecs, &specs);
    if (status != TCL_OK)
	return NULL;

    for (j = 0; j < nSpecs; j += 2) {
	Ttk_StateSpec spec;
	status = Ttk_GetStateSpecFromObj(interp, specs[j], &spec);
	if (status != TCL_OK)
	    return NULL;
	if (Ttk_StateMatches(state, &spec))
	    return specs[j+1];
    }
    if (interp) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("No match in state map", -1));
	Tcl_SetErrorCode(interp, "TTK", "STATE", "UNMATCHED", NULL);
    }
    return NULL;
}

/* Ttk_GetStateMapFromObj --
 * 	Returns a Ttk_StateMap from a Tcl_Obj*.
 * 	Since a Ttk_StateMap is just a specially-formatted Tcl_Obj,
 * 	this basically just checks for errors.
 */
Ttk_StateMap Ttk_GetStateMapFromObj(
    Tcl_Interp *interp,		/* Where to leave error messages; may be NULL */
    Tcl_Obj *mapObj)		/* State map */
{
    Tcl_Obj **specs;
    int nSpecs;
    int j, status;

    status = Tcl_ListObjGetElements(interp, mapObj, &nSpecs, &specs);
    if (status != TCL_OK)
	return NULL;

    if (nSpecs % 2 != 0) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "State map must have an even number of elements", -1));
	    Tcl_SetErrorCode(interp, "TTK", "VALUE", "STATEMAP", NULL);
	}
	return 0;
    }

    for (j = 0; j < nSpecs; j += 2) {
	Ttk_StateSpec spec;
	if (Ttk_GetStateSpecFromObj(interp, specs[j], &spec) != TCL_OK)
	    return NULL;
    }

    return mapObj;
}

/*
 * Ttk_StateTableLooup --
 * 	Look up an index from a statically allocated state table.
 */
int Ttk_StateTableLookup(Ttk_StateTable *map, unsigned int state)
{
    while ((state & map->onBits) != map->onBits
	    || (~state & map->offBits) != map->offBits)
    {
	++map;
    }
    return map->index;
}

/*EOF*/
