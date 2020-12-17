/*
 * tkStyle.c --
 *
 *	This file implements the widget styles and themes support.
 *
 * Copyright (c) 1990-1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * The following structure is used to cache widget option specs matching an
 * element's required options defined by Tk_ElementOptionSpecs. It also holds
 * information behind Tk_StyledElement opaque tokens.
 */

typedef struct StyledWidgetSpec {
    struct StyledElement *elementPtr;
				/* Pointer to the element holding this
				 * structure. */
    Tk_OptionTable optionTable;	/* Option table for the widget class using the
				 * element. */
    const Tk_OptionSpec **optionsPtr;
				/* Table of option spec pointers, matching the
				 * option list provided during element
				 * registration. Malloc'd. */
} StyledWidgetSpec;

/*
 * Elements are declared using static templates. But static information must
 * be completed by dynamic information only accessible at runtime. For each
 * registered element, an instance of the following structure is stored in
 * each style engine and used to cache information about the widget types
 * (identified by their optionTable) that use the given element.
 */

typedef struct StyledElement {
    struct Tk_ElementSpec *specPtr;
				/* Filled with template provided during
				 * registration. NULL means no implementation
				 * is available for the current engine. */
    int nbWidgetSpecs;		/* Size of the array below. Number of distinct
				 * widget classes (actually, distinct option
				 * tables) that used the element so far. */
    StyledWidgetSpec *widgetSpecs;
				/* See above for the structure definition.
				 * Table grows dynamically as new widgets use
				 * the element. Malloc'd. */
} StyledElement;

/*
 * The following structure holds information behind Tk_StyleEngine opaque
 * tokens.
 */

typedef struct StyleEngine {
    const char *name;		/* Name of engine. Points to a hash key. */
    StyledElement *elements;	/* Table of widget element descriptors. Each
				 * element is indexed by a unique system-wide
				 * ID. Table grows dynamically as new elements
				 * are registered. Malloc'd. */
    struct StyleEngine *parentPtr;
				/* Parent engine. Engines may be layered to
				 * form a fallback chain, terminated by the
				 * default system engine. */
} StyleEngine;

/*
 * Styles are instances of style engines. The following structure holds
 * information behind Tk_Style opaque tokens.
 */

typedef struct Style {
    const char *name;		/* Name of style. Points to a hash key. */
    StyleEngine *enginePtr;	/* Style engine of which the style is an
				 * instance. */
    ClientData clientData;	/* Data provided during registration. */
} Style;

/*
 * Each registered element uses an instance of the following structure.
 */

typedef struct Element {
    const char *name;		/* Name of element. Points to a hash key. */
    int id;			/* Id of element. */
    int genericId;		/* Id of generic element. */
    int created;		/* Boolean, whether the element was created
				 * explicitly (was registered) or implicitly
				 * (by a derived element). */
} Element;

/*
 * Thread-local data.
 */

typedef struct ThreadSpecificData {
    int nbInit;			/* Number of calls to the init proc. */
    Tcl_HashTable engineTable;	/* Map a name to a style engine. Keys are
				 * strings, values are Tk_StyleEngine
				 * pointers. */
    StyleEngine *defaultEnginePtr;
				/* Default, core-defined style engine. Global
				 * fallback for all engines. */
    Tcl_HashTable styleTable;	/* Map a name to a style. Keys are strings,
				 * values are Tk_Style pointers.*/
    int nbElements;		/* Size of the below tables. */
    Tcl_HashTable elementTable;	/* Map a name to an element Id. Keys are
				 * strings, values are integer element IDs. */
    Element *elements;		/* Array of Elements. */
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * Forward declarations for functions defined later in this file:
 */

static int		CreateElement(const char *name, int create);
static void		DupStyleObjProc(Tcl_Obj *srcObjPtr,
			    Tcl_Obj *dupObjPtr);
static void		FreeElement(Element *elementPtr);
static void		FreeStyledElement(StyledElement *elementPtr);
static void		FreeStyleEngine(StyleEngine *enginePtr);
static void		FreeStyleObjProc(Tcl_Obj *objPtr);
static void		FreeWidgetSpec(StyledWidgetSpec *widgetSpecPtr);
static StyledElement *	GetStyledElement(StyleEngine *enginePtr,
			    int elementId);
static StyledWidgetSpec*GetWidgetSpec(StyledElement *elementPtr,
			    Tk_OptionTable optionTable);
static void		InitElement(Element *elementPtr, const char *name,
			    int id, int genericId, int created);
static void		InitStyle(Style *stylePtr, const char *name,
			    StyleEngine *enginePtr, ClientData clientData);
static void		InitStyledElement(StyledElement *elementPtr);
static void		InitStyleEngine(StyleEngine *enginePtr,
			    const char *name, StyleEngine *parentPtr);
static void		InitWidgetSpec(StyledWidgetSpec *widgetSpecPtr,
			    StyledElement *elementPtr,
			    Tk_OptionTable optionTable);
static int		SetStyleFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr);

/*
 * The following structure defines the implementation of the "style" Tcl
 * object, used for drawing. The internalRep.twoPtrValue.ptr1 field of each
 * style object points to the Style structure for the stylefont, or NULL.
 */

static const Tcl_ObjType styleObjType = {
    "style",			/* name */
    FreeStyleObjProc,		/* freeIntRepProc */
    DupStyleObjProc,		/* dupIntRepProc */
    NULL,			/* updateStringProc */
    SetStyleFromAny		/* setFromAnyProc */
};

/*
 *---------------------------------------------------------------------------
 *
 * TkStylePkgInit --
 *
 *	This function is called when an application is created. It initializes
 *	all the structures that are used by the style package on a per
 *	application basis.
 *
 * Results:
 *	Stores data in thread-local storage.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

void
TkStylePkgInit(
    TkMainInfo *mainPtr)	/* The application being created. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->nbInit != 0) {
	return;
    }

    /*
     * Initialize tables.
     */

    Tcl_InitHashTable(&tsdPtr->engineTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&tsdPtr->styleTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&tsdPtr->elementTable, TCL_STRING_KEYS);
    tsdPtr->nbElements = 0;
    tsdPtr->elements = NULL;

    /*
     * Create the default system engine.
     */

    tsdPtr->defaultEnginePtr = (StyleEngine *)
	    Tk_RegisterStyleEngine(NULL, NULL);

    /*
     * Create the default system style.
     */

    Tk_CreateStyle(NULL, (Tk_StyleEngine) tsdPtr->defaultEnginePtr, NULL);

    tsdPtr->nbInit++;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkStylePkgFree --
 *
 *	This function is called when an application is deleted. It deletes all
 *	the structures that were used by the style package for this
 *	application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

void
TkStylePkgFree(
    TkMainInfo *mainPtr)	/* The application being deleted. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;
    StyleEngine *enginePtr;
    int i;

    tsdPtr->nbInit--;
    if (tsdPtr->nbInit != 0) {
	return;
    }

    /*
     * Free styles.
     */

    entryPtr = Tcl_FirstHashEntry(&tsdPtr->styleTable, &search);
    while (entryPtr != NULL) {
	ckfree(Tcl_GetHashValue(entryPtr));
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&tsdPtr->styleTable);

    /*
     * Free engines.
     */

    entryPtr = Tcl_FirstHashEntry(&tsdPtr->engineTable, &search);
    while (entryPtr != NULL) {
	enginePtr = Tcl_GetHashValue(entryPtr);
	FreeStyleEngine(enginePtr);
	ckfree(enginePtr);
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&tsdPtr->engineTable);

    /*
     * Free elements.
     */

    for (i = 0; i < tsdPtr->nbElements; i++) {
	FreeElement(tsdPtr->elements+i);
    }
    Tcl_DeleteHashTable(&tsdPtr->elementTable);
    ckfree(tsdPtr->elements);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_RegisterStyleEngine --
 *
 *	This function is called to register a new style engine. Style engines
 *	are stored in thread-local space.
 *
 * Results:
 *	The newly allocated engine, or NULL if an engine with the same name
 *	exists.
 *
 * Side effects:
 *	Memory allocated. Data added to thread-local table.
 *
 *---------------------------------------------------------------------------
 */

Tk_StyleEngine
Tk_RegisterStyleEngine(
    const char *name,		/* Name of the engine to create. NULL or empty
				 * means the default system engine. */
    Tk_StyleEngine parent)	/* The engine's parent. NULL means the default
				 * system engine. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr;
    int newEntry;
    StyleEngine *enginePtr;

    /*
     * Attempt to create a new entry in the engine table.
     */

    entryPtr = Tcl_CreateHashEntry(&tsdPtr->engineTable,
	    (name != NULL ? name : ""), &newEntry);
    if (!newEntry) {
	/*
	 * An engine was already registered by that name.
	 */

	return NULL;
    }

    /*
     * Allocate and intitialize a new engine.
     */

    enginePtr = ckalloc(sizeof(StyleEngine));
    InitStyleEngine(enginePtr, Tcl_GetHashKey(&tsdPtr->engineTable, entryPtr),
	    (StyleEngine *) parent);
    Tcl_SetHashValue(entryPtr, enginePtr);

    return (Tk_StyleEngine) enginePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitStyleEngine --
 *
 *	Initialize a newly allocated style engine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

static void
InitStyleEngine(
    StyleEngine *enginePtr,	/* Points to an uninitialized engine. */
    const char *name,		/* Name of the registered engine. NULL or empty
				 * means the default system engine. Usually
				 * points to the hash key. */
    StyleEngine *parentPtr)	/* The engine's parent. NULL means the default
				 * system engine. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    int elementId;

    if (name == NULL || *name == '\0') {
	/*
	 * This is the default style engine.
	 */

	enginePtr->parentPtr = NULL;
    } else if (parentPtr == NULL) {
	/*
	 * The default style engine is the parent.
	 */

	enginePtr->parentPtr = tsdPtr->defaultEnginePtr;
    } else {
	enginePtr->parentPtr = parentPtr;
    }

    /*
     * Allocate and initialize elements array.
     */

    if (tsdPtr->nbElements > 0) {
	enginePtr->elements = ckalloc(
		sizeof(StyledElement) * tsdPtr->nbElements);
	for (elementId = 0; elementId < tsdPtr->nbElements; elementId++) {
	    InitStyledElement(enginePtr->elements+elementId);
	}
    } else {
	enginePtr->elements = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeStyleEngine --
 *
 *	Free an engine and its associated data.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeStyleEngine(
    StyleEngine *enginePtr)	/* The style engine to free. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    int elementId;

    /*
     * Free allocated elements.
     */

    for (elementId = 0; elementId < tsdPtr->nbElements; elementId++) {
	FreeStyledElement(enginePtr->elements+elementId);
    }
    ckfree(enginePtr->elements);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetStyleEngine --
 *
 *	Retrieve a registered style engine by its name.
 *
 * Results:
 *	A pointer to the style engine, or NULL if none found.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tk_StyleEngine
Tk_GetStyleEngine(
    const char *name)		/* Name of the engine to retrieve. NULL or
				 * empty means the default system engine. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr;

    if (name == NULL) {
	return (Tk_StyleEngine) tsdPtr->defaultEnginePtr;
    }

    entryPtr = Tcl_FindHashEntry(&tsdPtr->engineTable, (name!=NULL?name:""));
    if (!entryPtr) {
	return NULL;
    }

    return Tcl_GetHashValue(entryPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * InitElement --
 *
 *	Initialize a newly allocated element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
InitElement(
    Element *elementPtr,	/* Points to an uninitialized element.*/
    const char *name,		/* Name of the registered element. Usually
				 * points to the hash key. */
    int id,			/* Unique element ID. */
    int genericId,		/* ID of generic element. -1 means none. */
    int created)		/* Boolean, whether the element was created
				 * explicitly (was registered) or implicitly
				 * (by a derived element). */
{
    elementPtr->name = name;
    elementPtr->id = id;
    elementPtr->genericId = genericId;
    elementPtr->created = (created?1:0);
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeElement --
 *
 *	Free an element and its associated data.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeElement(
    Element *elementPtr)	/* The element to free. */
{
    /* Nothing to do. */
}

/*
 *---------------------------------------------------------------------------
 *
 * InitStyledElement --
 *
 *	Initialize a newly allocated styled element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
InitStyledElement(
    StyledElement *elementPtr)	/* Points to an uninitialized element.*/
{
    memset(elementPtr, 0, sizeof(StyledElement));
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeStyledElement --
 *
 *	Free a styled element and its associated data.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeStyledElement(
    StyledElement *elementPtr)	/* The styled element to free. */
{
    int i;

    /*
     * Free allocated widget specs.
     */

    for (i = 0; i < elementPtr->nbWidgetSpecs; i++) {
	FreeWidgetSpec(elementPtr->widgetSpecs+i);
    }
    ckfree(elementPtr->widgetSpecs);
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateElement --
 *
 *	Find an existing or create a new element.
 *
 * Results:
 *	The unique ID for the created or found element.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

static int
CreateElement(
    const char *name,		/* Name of the element. */
    int create)			/* Boolean, whether the element is being
				 * created explicitly (being registered) or
				 * implicitly (by a derived element). */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr, *engineEntryPtr;
    Tcl_HashSearch search;
    int newEntry, elementId, genericId = -1;
    char *dot;
    StyleEngine *enginePtr;

    /*
     * Find or create the element.
     */

    entryPtr = Tcl_CreateHashEntry(&tsdPtr->elementTable, name, &newEntry);
    if (!newEntry) {
	elementId = PTR2INT(Tcl_GetHashValue(entryPtr));
	if (create) {
	    tsdPtr->elements[elementId].created = 1;
	}
	return elementId;
    }

    /*
     * The element didn't exist. If it's a derived element, find or create its
     * generic element ID.
     */

    dot = strchr(name, '.');
    if (dot) {
	genericId = CreateElement(dot+1, 0);
    }

    elementId = tsdPtr->nbElements++;
    Tcl_SetHashValue(entryPtr, INT2PTR(elementId));

    /*
     * Reallocate element table.
     */

    tsdPtr->elements = ckrealloc(tsdPtr->elements,
	    sizeof(Element) * tsdPtr->nbElements);
    InitElement(tsdPtr->elements+elementId,
	    Tcl_GetHashKey(&tsdPtr->elementTable, entryPtr), elementId,
	    genericId, create);

    /*
     * Reallocate style engines' element table.
     */

    engineEntryPtr = Tcl_FirstHashEntry(&tsdPtr->engineTable, &search);
    while (engineEntryPtr != NULL) {
	enginePtr = Tcl_GetHashValue(engineEntryPtr);

	enginePtr->elements = ckrealloc(enginePtr->elements,
		sizeof(StyledElement) * tsdPtr->nbElements);
	InitStyledElement(enginePtr->elements+elementId);

	engineEntryPtr = Tcl_NextHashEntry(&search);
    }

    return elementId;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetElementId --
 *
 *	Find an existing element.
 *
 * Results:
 *	The unique ID for the found element, or -1 if not found.
 *
 * Side effects:
 *	Generic elements may be created.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_GetElementId(
    const char *name)		/* Name of the element. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr;
    int genericId = -1;
    char *dot;

    /*
     * Find the element Id.
     */

    entryPtr = Tcl_FindHashEntry(&tsdPtr->elementTable, name);
    if (entryPtr) {
	return PTR2INT(Tcl_GetHashValue(entryPtr));
    }

    /*
     * Element not found. If the given name was derived, then first search for
     * the generic element. If found, create the new derived element.
     */

    dot = strchr(name, '.');
    if (!dot) {
	return -1;
    }
    genericId = Tk_GetElementId(dot+1);
    if (genericId == -1) {
	return -1;
    }
    if (!tsdPtr->elements[genericId].created) {
	/*
	 * The generic element was created implicitly and thus has no real
	 * existence.
	 */

	return -1;
    } else {
	/*
	 * The generic element was created explicitly. Create the derived
	 * element.
	 */

	return CreateElement(name, 1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_RegisterStyledElement --
 *
 *	Register an implementation of a new or existing element for the given
 *	style engine.
 *
 * Results:
 *	The unique ID for the created or found element.
 *
 * Side effects:
 *	Elements may be created. Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_RegisterStyledElement(
    Tk_StyleEngine engine,	/* Style engine providing the
				 * implementation. */
    Tk_ElementSpec *templatePtr)/* Static template information about the
				 * element. */
{
    int elementId;
    StyledElement *elementPtr;
    Tk_ElementSpec *specPtr;
    int nbOptions;
    register Tk_ElementOptionSpec *srcOptions, *dstOptions;

    if (templatePtr->version != TK_STYLE_VERSION_1) {
	/*
	 * Version mismatch. Do nothing.
	 */

	return -1;
    }

    if (engine == NULL) {
	engine = Tk_GetStyleEngine(NULL);
    }

    /*
     * Register the element, allocating storage in the various engines if
     * necessary.
     */

    elementId = CreateElement(templatePtr->name, 1);

    /*
     * Initialize the styled element.
     */

    elementPtr = ((StyleEngine *) engine)->elements+elementId;

    specPtr = ckalloc(sizeof(Tk_ElementSpec));
    specPtr->version = templatePtr->version;
    specPtr->name = ckalloc(strlen(templatePtr->name)+1);
    strcpy(specPtr->name, templatePtr->name);
    nbOptions = 0;
    for (nbOptions = 0, srcOptions = templatePtr->options;
	    srcOptions->name != NULL; nbOptions++, srcOptions++) {
	/* empty body */
    }
    specPtr->options =
	    ckalloc(sizeof(Tk_ElementOptionSpec) * (nbOptions+1));
    for (srcOptions = templatePtr->options, dstOptions = specPtr->options;
	    /* End condition within loop */; srcOptions++, dstOptions++) {
	if (srcOptions->name == NULL) {
	    dstOptions->name = NULL;
	    break;
	}

	dstOptions->name = ckalloc(strlen(srcOptions->name)+1);
	strcpy(dstOptions->name, srcOptions->name);
	dstOptions->type = srcOptions->type;
    }
    specPtr->getSize = templatePtr->getSize;
    specPtr->getBox = templatePtr->getBox;
    specPtr->getBorderWidth = templatePtr->getBorderWidth;
    specPtr->draw = templatePtr->draw;

    elementPtr->specPtr = specPtr;
    elementPtr->nbWidgetSpecs = 0;
    elementPtr->widgetSpecs = NULL;

    return elementId;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetStyledElement --
 *
 *	Get a registered implementation of an existing element for the given
 *	style engine.
 *
 * Results:
 *	The styled element descriptor, or NULL if not found.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static StyledElement *
GetStyledElement(
    StyleEngine *enginePtr,	/* Style engine providing the implementation.
				 * NULL means the default system engine. */
    int elementId)		/* Unique element ID */
{
    StyledElement *elementPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    StyleEngine *enginePtr2;

    if (enginePtr == NULL) {
	enginePtr = tsdPtr->defaultEnginePtr;
    }

    while (elementId >= 0 && elementId < tsdPtr->nbElements) {
	/*
	 * Look for an implemented element through the engine chain.
	 */

	enginePtr2 = enginePtr;
	do {
	    elementPtr = enginePtr2->elements+elementId;
	    if (elementPtr->specPtr != NULL) {
		return elementPtr;
	    }
	    enginePtr2 = enginePtr2->parentPtr;
	} while (enginePtr2 != NULL);

	/*
	 * None found, try with the generic element.
	 */

	elementId = tsdPtr->elements[elementId].genericId;
    }

    /*
     * No matching element found.
     */

    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitWidgetSpec --
 *
 *	Initialize a newly allocated widget spec.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

static void
InitWidgetSpec(
    StyledWidgetSpec *widgetSpecPtr,
				/* Points to an uninitialized widget spec. */
    StyledElement *elementPtr,	/* Styled element descriptor. */
    Tk_OptionTable optionTable)	/* The widget's option table. */
{
    int i, nbOptions;
    Tk_ElementOptionSpec *elementOptionPtr;
    const Tk_OptionSpec *widgetOptionPtr;

    widgetSpecPtr->elementPtr = elementPtr;
    widgetSpecPtr->optionTable = optionTable;

    /*
     * Count the number of options.
     */

    for (nbOptions = 0, elementOptionPtr = elementPtr->specPtr->options;
	    elementOptionPtr->name != NULL; nbOptions++, elementOptionPtr++) {
	/* empty body */
    }

    /*
     * Build the widget option list.
     */

    widgetSpecPtr->optionsPtr =
	    ckalloc(sizeof(Tk_OptionSpec *) * nbOptions);
    for (i = 0, elementOptionPtr = elementPtr->specPtr->options;
	    i < nbOptions; i++, elementOptionPtr++) {
	widgetOptionPtr = TkGetOptionSpec(elementOptionPtr->name, optionTable);

	/*
	 * Check that the widget option type is compatible with one of the
	 * element's required types.
	 */

	if (elementOptionPtr->type == TK_OPTION_END
	    || elementOptionPtr->type == widgetOptionPtr->type) {
	    widgetSpecPtr->optionsPtr[i] = widgetOptionPtr;
	} else {
	    widgetSpecPtr->optionsPtr[i] = NULL;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeWidgetSpec --
 *
 *	Free a widget spec and its associated data.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeWidgetSpec(
    StyledWidgetSpec *widgetSpecPtr)
				/* The widget spec to free. */
{
    ckfree(widgetSpecPtr->optionsPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetWidgetSpec --
 *
 *	Return a new or existing widget spec for the given element and widget
 *	type (identified by its option table).
 *
 * Results:
 *	A pointer to the matching widget spec.
 *
 * Side effects:
 *	Memory may be allocated.
 *
 *---------------------------------------------------------------------------
 */

static StyledWidgetSpec *
GetWidgetSpec(
    StyledElement *elementPtr,	/* Styled element descriptor. */
    Tk_OptionTable optionTable)	/* The widget's option table. */
{
    StyledWidgetSpec *widgetSpecPtr;
    int i;

    /*
     * Try to find an existing widget spec.
     */

    for (i = 0; i < elementPtr->nbWidgetSpecs; i++) {
	widgetSpecPtr = elementPtr->widgetSpecs+i;
	if (widgetSpecPtr->optionTable == optionTable) {
	    return widgetSpecPtr;
	}
    }

    /*
     * Create and initialize a new widget spec.
     */

    i = elementPtr->nbWidgetSpecs++;
    elementPtr->widgetSpecs = ckrealloc(elementPtr->widgetSpecs,
	    sizeof(StyledWidgetSpec) * elementPtr->nbWidgetSpecs);
    widgetSpecPtr = elementPtr->widgetSpecs+i;
    InitWidgetSpec(widgetSpecPtr, elementPtr, optionTable);

    return widgetSpecPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetStyledElement --
 *
 *	This function returns a styled instance of the given element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cached data may be allocated or updated.
 *
 *---------------------------------------------------------------------------
 */

Tk_StyledElement
Tk_GetStyledElement(
    Tk_Style style,		/* The widget style. */
    int elementId,		/* Unique element ID. */
    Tk_OptionTable optionTable)	/* Option table for the widget. */
{
    Style *stylePtr = (Style *) style;
    StyledElement *elementPtr;

    /*
     * Get an element implementation and call corresponding hook.
     */

    elementPtr = GetStyledElement((stylePtr?stylePtr->enginePtr:NULL),
	    elementId);
    if (!elementPtr) {
	return NULL;
    }

    return (Tk_StyledElement) GetWidgetSpec(elementPtr, optionTable);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetElementSize --
 *
 *	This function computes the size of the given widget element according
 *	to its style.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cached data may be allocated or updated.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_GetElementSize(
    Tk_Style style,		/* The widget style. */
    Tk_StyledElement element,	/* The styled element, previously returned by
				 * Tk_GetStyledElement. */
    char *recordPtr,		/* The widget record. */
    Tk_Window tkwin,		/* The widget window. */
    int width, int height,	/* Requested size. */
    int inner,			/* If TRUE, compute the outer size according
				 * to the requested minimum inner size. If
				 * FALSE, compute the inner size according to
				 * the requested maximum outer size. */
    int *widthPtr, int *heightPtr)
				/* Returned size. */
{
    Style *stylePtr = (Style *) style;
    StyledWidgetSpec *widgetSpecPtr = (StyledWidgetSpec *) element;

    widgetSpecPtr->elementPtr->specPtr->getSize(stylePtr->clientData,
	    recordPtr, widgetSpecPtr->optionsPtr, tkwin, width, height, inner,
	    widthPtr, heightPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetElementBox --
 *
 *	This function computes the bounding or inscribed box coordinates of
 *	the given widget element according to its style and within the given
 *	limits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cached data may be allocated or updated.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_GetElementBox(
    Tk_Style style,		/* The widget style. */
    Tk_StyledElement element,	/* The styled element, previously returned by
				 * Tk_GetStyledElement. */
    char *recordPtr,		/* The widget record. */
    Tk_Window tkwin,		/* The widget window. */
    int x, int y,		/* Top left corner of available area. */
    int width, int height,	/* Size of available area. */
    int inner,			/* Boolean. If TRUE, compute the bounding box
				 * according to the requested inscribed box
				 * size. If FALSE, compute the inscribed box
				 * according to the requested bounding box. */
    int *xPtr, int *yPtr,	/* Returned top left corner. */
    int *widthPtr, int *heightPtr)
				/* Returned size. */
{
    Style *stylePtr = (Style *) style;
    StyledWidgetSpec *widgetSpecPtr = (StyledWidgetSpec *) element;

    widgetSpecPtr->elementPtr->specPtr->getBox(stylePtr->clientData,
	    recordPtr, widgetSpecPtr->optionsPtr, tkwin, x, y, width, height,
	    inner, xPtr, yPtr, widthPtr, heightPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetElementBorderWidth --
 *
 *	This function computes the border widthof the given widget element
 *	according to its style and within the given limits.
 *
 * Results:
 *	Border width in pixels. This value is uniform for all four sides.
 *
 * Side effects:
 *	Cached data may be allocated or updated.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_GetElementBorderWidth(
    Tk_Style style,		/* The widget style. */
    Tk_StyledElement element,	/* The styled element, previously returned by
				 * Tk_GetStyledElement. */
    char *recordPtr,		/* The widget record. */
    Tk_Window tkwin)		/* The widget window. */
{
    Style *stylePtr = (Style *) style;
    StyledWidgetSpec *widgetSpecPtr = (StyledWidgetSpec *) element;

    return widgetSpecPtr->elementPtr->specPtr->getBorderWidth(
	    stylePtr->clientData, recordPtr, widgetSpecPtr->optionsPtr, tkwin);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_DrawElement --
 *
 *	This function draw the given widget element in a given drawable area.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Cached data may be allocated or updated.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_DrawElement(
    Tk_Style style,		/* The widget style. */
    Tk_StyledElement element,	/* The styled element, previously returned by
				 * Tk_GetStyledElement. */
    char *recordPtr,		/* The widget record. */
    Tk_Window tkwin,		/* The widget window. */
    Drawable d,			/* Where to draw element. */
    int x, int y,		/* Top left corner of element. */
    int width, int height,	/* Size of element. */
    int state)			/* Drawing state flags. */
{
    Style *stylePtr = (Style *) style;
    StyledWidgetSpec *widgetSpecPtr = (StyledWidgetSpec *) element;

    widgetSpecPtr->elementPtr->specPtr->draw(stylePtr->clientData,
	    recordPtr, widgetSpecPtr->optionsPtr, tkwin, d, x, y, width,
	    height, state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_CreateStyle --
 *
 *	This function is called to create a new style as an instance of the
 *	given engine. Styles are stored in thread-local space.
 *
 * Results:
 *	The newly allocated style, or NULL if the style already exists.
 *
 * Side effects:
 *	Memory allocated. Data added to thread-local table.
 *
 *---------------------------------------------------------------------------
 */

Tk_Style
Tk_CreateStyle(
    const char *name,		/* Name of the style to create. NULL or empty
				 * means the default system style. */
    Tk_StyleEngine engine,	/* The style engine. */
    ClientData clientData)	/* Private data passed as is to engine code. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr;
    int newEntry;
    Style *stylePtr;

    /*
     * Attempt to create a new entry in the style table.
     */

    entryPtr = Tcl_CreateHashEntry(&tsdPtr->styleTable, (name?name:""),
	    &newEntry);
    if (!newEntry) {
	/*
	 * A style was already registered by that name.
	 */

	return NULL;
    }

    /*
     * Allocate and intitialize a new style.
     */

    stylePtr = ckalloc(sizeof(Style));
    InitStyle(stylePtr, Tcl_GetHashKey(&tsdPtr->styleTable, entryPtr),
	    (engine!=NULL ? (StyleEngine*) engine : tsdPtr->defaultEnginePtr),
	    clientData);
    Tcl_SetHashValue(entryPtr, stylePtr);

    return (Tk_Style) stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_NameOfStyle --
 *
 *	Given a style, return its registered name.
 *
 * Results:
 *	The return value is the name that was passed to Tk_CreateStyle() to
 *	create the style. The storage for the returned string is private (it
 *	points to the corresponding hash key) The caller should not modify
 *	this string.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

const char *
Tk_NameOfStyle(
    Tk_Style style)		/* Style whose name is desired. */
{
    Style *stylePtr = (Style *) style;

    return stylePtr->name;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitStyle --
 *
 *	Initialize a newly allocated style.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
InitStyle(
    Style *stylePtr,		/* Points to an uninitialized style. */
    const char *name,		/* Name of the registered style. NULL or empty
				 * means the default system style. Usually
				 * points to the hash key. */
    StyleEngine *enginePtr,	/* The style engine. */
    ClientData clientData)	/* Private data passed as is to engine code. */
{
    stylePtr->name = name;
    stylePtr->enginePtr = enginePtr;
    stylePtr->clientData = clientData;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetStyle --
 *
 *	Retrieve a registered style by its name.
 *
 * Results:
 *	A pointer to the style engine, or NULL if none found. In the latter
 *	case and if the interp is not NULL, an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tk_Style
Tk_GetStyle(
    Tcl_Interp *interp,		/* Interp for error return. */
    const char *name)		/* Name of the style to retrieve. NULL or empty
				 * means the default system style. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr;
    Style *stylePtr;

    /*
     * Search for a corresponding entry in the style table.
     */

    entryPtr = Tcl_FindHashEntry(&tsdPtr->styleTable, (name!=NULL?name:""));
    if (entryPtr == NULL) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "style \"%s\" doesn't exist", name));
	    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "STYLE", name, NULL);
	}
	return (Tk_Style) NULL;
    }
    stylePtr = Tcl_GetHashValue(entryPtr);

    return (Tk_Style) stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_FreeStyle --
 *
 *	No-op. Present only for stubs compatibility.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_FreeStyle(
    Tk_Style style)
{
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_AllocStyleFromObj --
 *
 *	Map the string name of a style to a corresponding Tk_Style. The style
 *	must have already been created by Tk_CreateStyle.
 *
 * Results:
 *	The return value is a token for the style that matches objPtr, or NULL
 *	if none found. If NULL is returned, an error message will be left in
 *	interp's result object.
 *
 *---------------------------------------------------------------------------
 */

Tk_Style
Tk_AllocStyleFromObj(
    Tcl_Interp *interp,		/* Interp for error return. */
    Tcl_Obj *objPtr)		/* Object containing name of the style to
				 * retrieve. */
{
    Style *stylePtr;

    if (objPtr->typePtr != &styleObjType) {
	SetStyleFromAny(interp, objPtr);
    }
    stylePtr = objPtr->internalRep.twoPtrValue.ptr1;

    return (Tk_Style) stylePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetStyleFromObj --
 *
 *	Find the style that corresponds to a given object. The style must have
 *	already been created by Tk_CreateStyle.
 *
 * Results:
 *	The return value is a token for the style that matches objPtr, or NULL
 *	if none found.
 *
 * Side effects:
 *	If the object is not already a style ref, the conversion will free any
 *	old internal representation.
 *
 *----------------------------------------------------------------------
 */

Tk_Style
Tk_GetStyleFromObj(
    Tcl_Obj *objPtr)		/* The object from which to get the style. */
{
    if (objPtr->typePtr != &styleObjType) {
	SetStyleFromAny(NULL, objPtr);
    }

    return objPtr->internalRep.twoPtrValue.ptr1;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_FreeStyleFromObj --
 *
 *	No-op. Present only for stubs compatibility.
 *
 *---------------------------------------------------------------------------
 */
void
Tk_FreeStyleFromObj(
    Tcl_Obj *objPtr)
{
}

/*
 *----------------------------------------------------------------------
 *
 * SetStyleFromAny --
 *
 *	Convert the internal representation of a Tcl object to the style
 *	internal form.
 *
 * Results:
 *	Always returns TCL_OK. If an error occurs is returned (e.g. the style
 *	doesn't exist), an error message will be left in interp's result.
 *
 * Side effects:
 *	The object is left with its typePtr pointing to styleObjType.
 *
 *----------------------------------------------------------------------
 */

static int
SetStyleFromAny(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr)		/* The object to convert. */
{
    const Tcl_ObjType *typePtr;
    const char *name;

    /*
     * Free the old internalRep before setting the new one.
     */

    name = Tcl_GetString(objPtr);
    typePtr = objPtr->typePtr;
    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	typePtr->freeIntRepProc(objPtr);
    }

    objPtr->typePtr = &styleObjType;
    objPtr->internalRep.twoPtrValue.ptr1 = Tk_GetStyle(interp, name);

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeStyleObjProc --
 *
 *	This proc is called to release an object reference to a style. Called
 *	when the object's internal rep is released.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeStyleObjProc(
    Tcl_Obj *objPtr)		/* The object we are releasing. */
{
    objPtr->internalRep.twoPtrValue.ptr1 = NULL;
    objPtr->typePtr = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * DupStyleObjProc --
 *
 *	When a cached style object is duplicated, this is called to update the
 *	internal reps.
 *
 *---------------------------------------------------------------------------
 */

static void
DupStyleObjProc(
    Tcl_Obj *srcObjPtr,		/* The object we are copying from. */
    Tcl_Obj *dupObjPtr)		/* The object we are copying to. */
{
    dupObjPtr->typePtr = srcObjPtr->typePtr;
    dupObjPtr->internalRep.twoPtrValue.ptr1 =
	    srcObjPtr->internalRep.twoPtrValue.ptr1;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
