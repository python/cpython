/*
 * ttkTheme.c --
 *
 *	This file implements the widget styles and themes support.
 *
 * Copyright (c) 2002 Frederic Bonnet
 * Copyright (c) 2003 Joe English
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "ttkThemeInt.h"

#define PKG_ASSOC_KEY "Ttk"

#ifdef MAC_OSX_TK
    extern void TkMacOSXFlushWindows(void);
    #define UPDATE_WINDOWS() TkMacOSXFlushWindows()
#else
    #define UPDATE_WINDOWS()
#endif

/*------------------------------------------------------------------------
 * +++ Styles.
 *
 * Invariants:
 * 	If styleName contains a dot, parentStyle->styleName is everything
 * 	after the first dot; otherwise, parentStyle is the theme's root
 * 	style ".".  The root style's parentStyle is NULL.
 *
 */

typedef struct Ttk_Style_
{
    const char		*styleName;	/* points to hash table key */
    Tcl_HashTable	settingsTable;	/* KEY: string; VALUE: StateMap */
    Tcl_HashTable	defaultsTable;	/* KEY: string; VALUE: resource */
    Ttk_LayoutTemplate	layoutTemplate;	/* Layout template for style, or NULL */
    Ttk_Style		parentStyle;	/* Previous style in chain */
    Ttk_ResourceCache	cache;		/* Back-pointer to resource cache */
} Style;

static Style *NewStyle()
{
    Style *stylePtr = ckalloc(sizeof(Style));

    stylePtr->styleName = NULL;
    stylePtr->parentStyle = NULL;
    stylePtr->layoutTemplate = NULL;
    stylePtr->cache = NULL;
    Tcl_InitHashTable(&stylePtr->settingsTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&stylePtr->defaultsTable, TCL_STRING_KEYS);

    return stylePtr;
}

static void FreeStyle(Style *stylePtr)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_FirstHashEntry(&stylePtr->settingsTable, &search);
    while (entryPtr != NULL) {
	Ttk_StateMap stateMap = Tcl_GetHashValue(entryPtr);
	Tcl_DecrRefCount(stateMap);
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&stylePtr->settingsTable);

    entryPtr = Tcl_FirstHashEntry(&stylePtr->defaultsTable, &search);
    while (entryPtr != NULL) {
	Tcl_Obj *defaultValue = Tcl_GetHashValue(entryPtr);
	Tcl_DecrRefCount(defaultValue);
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&stylePtr->defaultsTable);

    Ttk_FreeLayoutTemplate(stylePtr->layoutTemplate);

    ckfree(stylePtr);
}

/*
 * Ttk_StyleMap --
 * 	Look up state-specific option value from specified style.
 */
Tcl_Obj *Ttk_StyleMap(Ttk_Style style, const char *optionName, Ttk_State state)
{
    while (style) {
	Tcl_HashEntry *entryPtr =
	    Tcl_FindHashEntry(&style->settingsTable, optionName);
	if (entryPtr) {
	    Ttk_StateMap stateMap = Tcl_GetHashValue(entryPtr);
	    return Ttk_StateMapLookup(NULL, stateMap, state);
	}
	style = style->parentStyle;
    }
    return 0;
}

/*
 * Ttk_StyleDefault --
 * 	Look up default resource setting the in the specified style.
 */
Tcl_Obj *Ttk_StyleDefault(Ttk_Style style, const char *optionName)
{
    while (style) {
	Tcl_HashEntry *entryPtr =
	    Tcl_FindHashEntry(&style->defaultsTable, optionName);
	if (entryPtr)
	    return Tcl_GetHashValue(entryPtr);
	style= style->parentStyle;
    }
    return 0;
}

/*------------------------------------------------------------------------
 * +++ Elements.
 */
typedef const Tk_OptionSpec **OptionMap;
    /* array of Tk_OptionSpecs mapping widget options to element options */

struct Ttk_ElementClass_ {
    const char *name;		/* Points to hash table key */
    Ttk_ElementSpec *specPtr;	/* Template provided during registration. */
    void *clientData;		/* Client data passed in at registration time */
    void *elementRecord;	/* Scratch buffer for element record storage */
    int nResources;		/* #Element options */
    Tcl_Obj **defaultValues;	/* Array of option default values */
    Tcl_HashTable optMapCache;	/* Map: Tk_OptionTable * -> OptionMap */
};

/* TTKGetOptionSpec --
 * 	Look up a Tk_OptionSpec by name from a Tk_OptionTable,
 * 	and verify that it's compatible with the specified Tk_OptionType,
 * 	along with other constraints (see below).
 */
static const Tk_OptionSpec *TTKGetOptionSpec(
    const char *optionName,
    Tk_OptionTable optionTable,
    Tk_OptionType optionType)
{
    const Tk_OptionSpec *optionSpec = TkGetOptionSpec(optionName, optionTable);

    if (!optionSpec)
	return 0;

    /* Make sure widget option has a Tcl_Obj* entry:
     */
    if (optionSpec->objOffset < 0) {
	return 0;
    }

    /* Grrr.  Ignore accidental mismatches caused by prefix-matching:
     */
    if (strcmp(optionSpec->optionName, optionName)) {
	return 0;
    }

    /* Ensure that the widget option type is compatible with
     * the element option type.
     *
     * TK_OPTION_STRING element options are compatible with anything.
     * As a workaround for the workaround for Bug #967209,
     * TK_OPTION_STRING widget options are also compatible with anything
     * (see <<NOTE-NULLOPTIONS>>).
     */
    if (   optionType != TK_OPTION_STRING
	&& optionSpec->type != TK_OPTION_STRING
	&& optionType != optionSpec->type)
    {
	return 0;
    }

    return optionSpec;
}

/* BuildOptionMap --
 * 	Construct the mapping from element options to widget options.
 */
static OptionMap
BuildOptionMap(Ttk_ElementClass *elementClass, Tk_OptionTable optionTable)
{
    OptionMap optionMap = ckalloc(
	    sizeof(const Tk_OptionSpec) * elementClass->nResources + 1);
    int i;

    for (i = 0; i < elementClass->nResources; ++i) {
	Ttk_ElementOptionSpec *e = elementClass->specPtr->options+i;
	optionMap[i] = TTKGetOptionSpec(e->optionName, optionTable, e->type);
    }

    return optionMap;
}

/* GetOptionMap --
 * 	Return a cached OptionMap matching the specified optionTable
 * 	for the specified element, creating it if necessary.
 */
static OptionMap
GetOptionMap(Ttk_ElementClass *elementClass, Tk_OptionTable optionTable)
{
    OptionMap optionMap;
    int isNew;
    Tcl_HashEntry *entryPtr = Tcl_CreateHashEntry(
	&elementClass->optMapCache, (void*)optionTable, &isNew);

    if (isNew) {
	optionMap = BuildOptionMap(elementClass, optionTable);
	Tcl_SetHashValue(entryPtr, optionMap);
    } else {
	optionMap = Tcl_GetHashValue(entryPtr);
    }

    return optionMap;
}

/*
 * NewElementClass --
 * 	Allocate and initialize an element class record
 * 	from the specified element specification.
 */
static Ttk_ElementClass *
NewElementClass(const char *name, Ttk_ElementSpec *specPtr,void *clientData)
{
    Ttk_ElementClass *elementClass = ckalloc(sizeof(Ttk_ElementClass));
    int i;

    elementClass->name = name;
    elementClass->specPtr = specPtr;
    elementClass->clientData = clientData;
    elementClass->elementRecord = ckalloc(specPtr->elementSize);

    /* Count #element resources:
     */
    for (i = 0; specPtr->options[i].optionName != 0; ++i)
	continue;
    elementClass->nResources = i;

    /* Initialize default values:
     */
    elementClass->defaultValues =
	ckalloc(elementClass->nResources * sizeof(Tcl_Obj *) + 1);
    for (i=0; i < elementClass->nResources; ++i) {
        const char *defaultValue = specPtr->options[i].defaultValue;
	if (defaultValue) {
	    elementClass->defaultValues[i] = Tcl_NewStringObj(defaultValue,-1);
	    Tcl_IncrRefCount(elementClass->defaultValues[i]);
	} else {
	    elementClass->defaultValues[i] = 0;
	}
    }

    /* Initialize option map cache:
     */
    Tcl_InitHashTable(&elementClass->optMapCache, TCL_ONE_WORD_KEYS);

    return elementClass;
}

/*
 * FreeElementClass --
 * 	Release resources associated with an element class record.
 */
static void FreeElementClass(Ttk_ElementClass *elementClass)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;
    int i;

    /*
     * Free default values:
     */
    for (i = 0; i < elementClass->nResources; ++i) {
	if (elementClass->defaultValues[i]) {
	    Tcl_DecrRefCount(elementClass->defaultValues[i]);
	}
    }
    ckfree(elementClass->defaultValues);

    /*
     * Free option map cache:
     */
    entryPtr = Tcl_FirstHashEntry(&elementClass->optMapCache, &search);
    while (entryPtr != NULL) {
	ckfree(Tcl_GetHashValue(entryPtr));
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&elementClass->optMapCache);

    ckfree(elementClass->elementRecord);
    ckfree(elementClass);
}

/*------------------------------------------------------------------------
 * +++ Themes.
 */

static int ThemeEnabled(Ttk_Theme theme, void *clientData) { return 1; }
    /* Default ThemeEnabledProc -- always return true */

typedef struct Ttk_Theme_
{
    Ttk_Theme parentPtr;             	/* Parent theme. */
    Tcl_HashTable elementTable;	     	/* Map element names to class records */
    Tcl_HashTable styleTable;	     	/* Map style names to Styles */
    Ttk_Style rootStyle;		/* "." style, root of chain */
    Ttk_ThemeEnabledProc *enabledProc;	/* Function called by SetTheme */
    void *enabledData;              	/* ClientData for enabledProc */
    Ttk_ResourceCache cache;		/* Back-pointer to resource cache */
} Theme;

static Theme *NewTheme(Ttk_ResourceCache cache, Ttk_Theme parent)
{
    Theme *themePtr = ckalloc(sizeof(Theme));
    Tcl_HashEntry *entryPtr;
    int unused;

    themePtr->parentPtr = parent;
    themePtr->enabledProc = ThemeEnabled;
    themePtr->enabledData = NULL;
    themePtr->cache = cache;
    Tcl_InitHashTable(&themePtr->elementTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&themePtr->styleTable, TCL_STRING_KEYS);

    /*
     * Create root style "."
     */
    entryPtr = Tcl_CreateHashEntry(&themePtr->styleTable, ".", &unused);
    themePtr->rootStyle = NewStyle();
    themePtr->rootStyle->styleName =
	Tcl_GetHashKey(&themePtr->styleTable, entryPtr);
    themePtr->rootStyle->cache = themePtr->cache;
    Tcl_SetHashValue(entryPtr, themePtr->rootStyle);

    return themePtr;
}

static void FreeTheme(Theme *themePtr)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;

    /*
     * Free element table:
     */
    entryPtr = Tcl_FirstHashEntry(&themePtr->elementTable, &search);
    while (entryPtr != NULL) {
	Ttk_ElementClass *elementClass = Tcl_GetHashValue(entryPtr);
	FreeElementClass(elementClass);
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&themePtr->elementTable);

    /*
     * Free style table:
     */
    entryPtr = Tcl_FirstHashEntry(&themePtr->styleTable, &search);
    while (entryPtr != NULL) {
	Style *stylePtr = Tcl_GetHashValue(entryPtr);
	FreeStyle(stylePtr);
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&themePtr->styleTable);

    /*
     * Free theme record:
     */
    ckfree(themePtr);

    return;
}

/*
 * Element constructors.
 */
typedef struct {
    Ttk_ElementFactory	factory;
    void		*clientData;
} FactoryRec;

/*
 * Cleanup records:
 */
typedef struct CleanupStruct {
    void *clientData;
    Ttk_CleanupProc *cleanupProc;
    struct CleanupStruct *next;
} Cleanup;

/*------------------------------------------------------------------------
 * +++ Master style package data structure.
 */
typedef struct
{
    Tcl_Interp *interp;			/* Owner interp */
    Tcl_HashTable themeTable;		/* KEY: name; VALUE: Theme pointer */
    Tcl_HashTable factoryTable; 	/* KEY: name; VALUE: FactoryRec ptr */
    Theme *defaultTheme;		/* Default theme; global fallback*/
    Theme *currentTheme;		/* Currently-selected theme */
    Cleanup *cleanupList;		/* Cleanup records */
    Ttk_ResourceCache cache;		/* Resource cache */
    int themeChangePending;		/* scheduled ThemeChangedProc call? */
} StylePackageData;

static void ThemeChangedProc(ClientData);	/* Forward */

/* Ttk_StylePkgFree --
 *	Cleanup procedure for StylePackageData.
 */
static void Ttk_StylePkgFree(ClientData clientData, Tcl_Interp *interp)
{
    StylePackageData *pkgPtr = clientData;
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;
    Cleanup *cleanup;

    /*
     * Cancel any pending ThemeChanged calls:
     */
    if (pkgPtr->themeChangePending) {
	Tcl_CancelIdleCall(ThemeChangedProc, pkgPtr);
    }

    /*
     * Free themes.
     */
    entryPtr = Tcl_FirstHashEntry(&pkgPtr->themeTable, &search);
    while (entryPtr != NULL) {
	Theme *themePtr = Tcl_GetHashValue(entryPtr);
	FreeTheme(themePtr);
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&pkgPtr->themeTable);

    /*
     * Free element constructor table:
     */
    entryPtr = Tcl_FirstHashEntry(&pkgPtr->factoryTable, &search);
    while (entryPtr != NULL) {
	ckfree(Tcl_GetHashValue(entryPtr));
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&pkgPtr->factoryTable);

    /*
     * Release cache:
     */
    Ttk_FreeResourceCache(pkgPtr->cache);

    /*
     * Call all registered cleanup procedures:
     */
    cleanup = pkgPtr->cleanupList;
    while (cleanup) {
	Cleanup *next = cleanup->next;
	cleanup->cleanupProc(cleanup->clientData);
	ckfree(cleanup);
	cleanup = next;
    }

    ckfree(pkgPtr);
}

/*
 * GetStylePackageData --
 * 	Look up the package data registered with the interp.
 */

static StylePackageData *GetStylePackageData(Tcl_Interp *interp)
{
    return Tcl_GetAssocData(interp, PKG_ASSOC_KEY, NULL);
}

/*
 * Ttk_RegisterCleanup --
 *
 *	Register a function to be called when a theme engine is deleted.
 *	(This only happens when the main interp is destroyed). The cleanup
 *	function is called with the current Tcl interpreter and the client
 *	data provided here.
 *
 */
void Ttk_RegisterCleanup(
    Tcl_Interp *interp, ClientData clientData, Ttk_CleanupProc *cleanupProc)
{
    StylePackageData *pkgPtr = GetStylePackageData(interp);
    Cleanup *cleanup = ckalloc(sizeof(*cleanup));

    cleanup->clientData = clientData;
    cleanup->cleanupProc = cleanupProc;
    cleanup->next = pkgPtr->cleanupList;
    pkgPtr->cleanupList = cleanup;
}

/* ThemeChangedProc --
 * 	Notify all widgets that the theme has been changed.
 * 	Scheduled as an idle callback; clientData is a StylePackageData *.
 *
 * 	Sends a <<ThemeChanged>> event to every widget in the hierarchy.
 * 	Widgets respond to this by calling the  WorldChanged class proc,
 * 	which in turn recreates the layout.
 *
 * 	The Tk C API doesn't doesn't provide an easy way to traverse
 * 	the widget hierarchy, so this is done by evaluating a Tcl script.
 */

static void ThemeChangedProc(ClientData clientData)
{
    static char ThemeChangedScript[] = "ttk::ThemeChanged";
    StylePackageData *pkgPtr = clientData;

    int code = Tcl_EvalEx(pkgPtr->interp, ThemeChangedScript, -1, TCL_EVAL_GLOBAL);
    if (code != TCL_OK) {
	Tcl_BackgroundException(pkgPtr->interp, code);
    }
    pkgPtr->themeChangePending = 0;
    UPDATE_WINDOWS();
}

/*
 * ThemeChanged --
 * 	Schedule a call to ThemeChanged if one is not already pending.
 */
static void ThemeChanged(StylePackageData *pkgPtr)
{
    if (!pkgPtr->themeChangePending) {
	Tcl_DoWhenIdle(ThemeChangedProc, pkgPtr);
	pkgPtr->themeChangePending = 1;
    }
}

/*
 * Ttk_CreateTheme --
 *	Create a new theme and register it in the global theme table.
 *
 * Returns:
 * 	Pointer to new Theme structure; NULL if named theme already exists.
 * 	Leaves an error message in interp's result on error.
 */

Ttk_Theme
Ttk_CreateTheme(
    Tcl_Interp *interp,		/* Interpreter in which to create theme */
    const char *name,		/* Name of the theme to create. */
    Ttk_Theme parent) 		/* Parent/fallback theme, NULL for default */
{
    StylePackageData *pkgPtr = GetStylePackageData(interp);
    Tcl_HashEntry *entryPtr;
    int newEntry;
    Theme *themePtr;

    entryPtr = Tcl_CreateHashEntry(&pkgPtr->themeTable, name, &newEntry);
    if (!newEntry) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Theme %s already exists", name));
	Tcl_SetErrorCode(interp, "TTK", "THEME", "EXISTS", NULL);
	return NULL;
    }

    /*
     * Initialize new theme:
     */
    if (!parent) parent = pkgPtr->defaultTheme;

    themePtr = NewTheme(pkgPtr->cache, parent);
    Tcl_SetHashValue(entryPtr, themePtr);

    return themePtr;
}

/*
 * Ttk_SetThemeEnabledProc --
 *	Sets a procedure that is used to check that this theme is available.
 */

void Ttk_SetThemeEnabledProc(
    Ttk_Theme theme, Ttk_ThemeEnabledProc enabledProc, void *enabledData)
{
    theme->enabledProc = enabledProc;
    theme->enabledData = enabledData;
}

/*
 * LookupTheme --
 *	Retrieve a registered theme by name.  If not found,
 *	returns NULL and leaves an error message in interp's result.
 */

static Ttk_Theme LookupTheme(
    Tcl_Interp *interp,		/* where to leave error messages */
    StylePackageData *pkgPtr,	/* style package master record */
    const char *name)		/* theme name */
{
    Tcl_HashEntry *entryPtr;

    entryPtr = Tcl_FindHashEntry(&pkgPtr->themeTable, name);
    if (!entryPtr) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"theme \"%s\" doesn't exist", name));
	Tcl_SetErrorCode(interp, "TTK", "LOOKUP", "THEME", name, NULL);
	return NULL;
    }

    return Tcl_GetHashValue(entryPtr);
}

/*
 * Ttk_GetTheme --
 *	Public interface to LookupTheme.
 */
Ttk_Theme Ttk_GetTheme(Tcl_Interp *interp, const char *themeName)
{
    StylePackageData *pkgPtr = GetStylePackageData(interp);

    return LookupTheme(interp, pkgPtr, themeName);
}

Ttk_Theme Ttk_GetCurrentTheme(Tcl_Interp *interp)
{
    StylePackageData *pkgPtr = GetStylePackageData(interp);
    return pkgPtr->currentTheme;
}

Ttk_Theme Ttk_GetDefaultTheme(Tcl_Interp *interp)
{
    StylePackageData *pkgPtr = GetStylePackageData(interp);
    return pkgPtr->defaultTheme;
}

/*
 * Ttk_UseTheme --
 * 	Set the current theme, notify all widgets that the theme has changed.
 */
int Ttk_UseTheme(Tcl_Interp *interp, Ttk_Theme  theme)
{
    StylePackageData *pkgPtr = GetStylePackageData(interp);

    /*
     * Check if selected theme is enabled:
     */
    while (theme && !theme->enabledProc(theme, theme->enabledData)) {
    	theme = theme->parentPtr;
    }
    if (!theme) {
    	/* This shouldn't happen -- default theme should always work */
	Tcl_Panic("No themes available?");
	return TCL_ERROR;
    }

    pkgPtr->currentTheme = theme;
    ThemeChanged(pkgPtr);
    return TCL_OK;
}

/*
 * Ttk_GetResourceCache --
 * 	Return the resource cache associated with 'interp'
 */
Ttk_ResourceCache
Ttk_GetResourceCache(Tcl_Interp *interp)
{
    StylePackageData *pkgPtr = GetStylePackageData(interp);
    return pkgPtr->cache;
}

/*
 * Register a new layout specification with a style.
 * @@@ TODO: Make sure layoutName is not ".", root style must not have a layout
 */
MODULE_SCOPE
void Ttk_RegisterLayoutTemplate(
    Ttk_Theme theme,			/* Target theme */
    const char *layoutName,		/* Name of new layout */
    Ttk_LayoutTemplate layoutTemplate)	/* Template */
{
    Ttk_Style style = Ttk_GetStyle(theme, layoutName);
    if (style->layoutTemplate) {
	Ttk_FreeLayoutTemplate(style->layoutTemplate);
    }
    style->layoutTemplate = layoutTemplate;
}

void Ttk_RegisterLayout(
    Ttk_Theme themePtr,		/* Target theme */
    const char *layoutName,	/* Name of new layout */
    Ttk_LayoutSpec specPtr)	/* Static layout information */
{
    Ttk_LayoutTemplate layoutTemplate = Ttk_BuildLayoutTemplate(specPtr);
    Ttk_RegisterLayoutTemplate(themePtr, layoutName, layoutTemplate);
}

/*
 * Ttk_GetStyle --
 * 	Look up a Style from a Theme, create new style if not found.
 */
Ttk_Style Ttk_GetStyle(Ttk_Theme themePtr, const char *styleName)
{
    Tcl_HashEntry *entryPtr;
    int newStyle;

    entryPtr = Tcl_CreateHashEntry(&themePtr->styleTable, styleName, &newStyle);
    if (newStyle) {
	Ttk_Style stylePtr = NewStyle();
	const char *dot = strchr(styleName, '.');

	if (dot) {
	    stylePtr->parentStyle = Ttk_GetStyle(themePtr, dot + 1);
	} else {
	    stylePtr->parentStyle = themePtr->rootStyle;
	}

	stylePtr->styleName = Tcl_GetHashKey(&themePtr->styleTable, entryPtr);
	stylePtr->cache = stylePtr->parentStyle->cache;
	Tcl_SetHashValue(entryPtr, stylePtr);
	return stylePtr;
    }
    return Tcl_GetHashValue(entryPtr);
}

/* FindLayoutTemplate --
 * 	Locate a layout template in the layout table, checking
 * 	generic names to specific names first, then looking for
 * 	the full name in the parent theme.
 */
Ttk_LayoutTemplate
Ttk_FindLayoutTemplate(Ttk_Theme themePtr, const char *layoutName)
{
    while (themePtr) {
	Ttk_Style stylePtr = Ttk_GetStyle(themePtr, layoutName);
	while (stylePtr) {
	    if (stylePtr->layoutTemplate) {
		return stylePtr->layoutTemplate;
	    }
	    stylePtr = stylePtr->parentStyle;
	}
	themePtr = themePtr->parentPtr;
    }
    return NULL;
}

const char *Ttk_StyleName(Ttk_Style stylePtr)
{
    return stylePtr->styleName;
}

/*
 * Ttk_GetElement --
 *	Look up an element class by name in a given theme.
 *	If not found, try generic element names in this theme, then
 *	repeat the lookups in the parent theme.
 *	If not found, return the null element.
 */
Ttk_ElementClass *Ttk_GetElement(Ttk_Theme themePtr, const char *elementName)
{
    Tcl_HashEntry *entryPtr;
    const char *dot = elementName;

    /*
     * Check if element has already been registered:
     */
    entryPtr = Tcl_FindHashEntry(&themePtr->elementTable, elementName);
    if (entryPtr) {
	return Tcl_GetHashValue(entryPtr);
    }

    /*
     * Check generic names:
     */
    while (!entryPtr && ((dot = strchr(dot, '.')) != NULL)) {
	dot++;
	entryPtr = Tcl_FindHashEntry(&themePtr->elementTable, dot);
    }
    if (entryPtr) {
	return Tcl_GetHashValue(entryPtr);
    }

    /*
     * Check parent theme:
     */
    if (themePtr->parentPtr) {
	return Ttk_GetElement(themePtr->parentPtr, elementName);
    }

    /*
     * Not found, and this is the root theme; return null element, "".
     * (@@@ SHOULD: signal a background error)
     */
    entryPtr = Tcl_FindHashEntry(&themePtr->elementTable, "");
    /* ASSERT: entryPtr != 0 */
    return Tcl_GetHashValue(entryPtr);
}

const char *Ttk_ElementClassName(Ttk_ElementClass *elementClass)
{
    return elementClass->name;
}

/*
 * Ttk_RegisterElementFactory --
 *	Register a new element factory.
 */
int Ttk_RegisterElementFactory(
    Tcl_Interp *interp,	const char *name,
    Ttk_ElementFactory factory, void *clientData)
{
    StylePackageData *pkgPtr = GetStylePackageData(interp);
    FactoryRec *recPtr = ckalloc(sizeof(*recPtr));
    Tcl_HashEntry *entryPtr;
    int newEntry;

    recPtr->factory = factory;
    recPtr->clientData = clientData;

    entryPtr = Tcl_CreateHashEntry(&pkgPtr->factoryTable, name, &newEntry);
    if (!newEntry) {
    	/* Free old factory: */
	ckfree(Tcl_GetHashValue(entryPtr));
    }
    Tcl_SetHashValue(entryPtr, recPtr);

    return TCL_OK;
}

/* Ttk_CloneElement -- element factory procedure.
 * 	(style element create $name) "from" $theme ?$element?
 */
static int Ttk_CloneElement(
    Tcl_Interp *interp, void *clientData,
    Ttk_Theme theme, const char *elementName,
    int objc, Tcl_Obj *const objv[])
{
    Ttk_Theme fromTheme;
    Ttk_ElementClass *fromElement;

    if (objc <= 0 || objc > 2) {
	Tcl_WrongNumArgs(interp, 0, objv, "theme ?element?");
	return TCL_ERROR;
    }

    fromTheme = Ttk_GetTheme(interp, Tcl_GetString(objv[0]));
    if (!fromTheme) {
	return TCL_ERROR;
    }

    if (objc == 2) {
	fromElement = Ttk_GetElement(fromTheme, Tcl_GetString(objv[1]));
    } else {
	fromElement = Ttk_GetElement(fromTheme, elementName);
    }
    if (!fromElement) {
	return TCL_ERROR;
    }

    if (Ttk_RegisterElement(interp, theme, elementName,
		fromElement->specPtr, fromElement->clientData) == NULL)
    {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/* Ttk_RegisterElement--
 *	Register an element in the given theme.
 *	Returns: Element handle if successful, NULL otherwise.
 *	On failure, leaves an error message in interp's result
 *	if interp is non-NULL.
 */

Ttk_ElementClass *Ttk_RegisterElement(
    Tcl_Interp *interp,		/* Where to leave error messages */
    Ttk_Theme theme,		/* Style engine providing the implementation. */
    const char *name,		/* Name of new element */
    Ttk_ElementSpec *specPtr, 	/* Static template information */
    void *clientData)		/* application-specific data */
{
    Ttk_ElementClass *elementClass;
    Tcl_HashEntry *entryPtr;
    int newEntry;

    if (specPtr->version != TK_STYLE_VERSION_2) {
	/* Version mismatch */
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Internal error: Ttk_RegisterElement (%s): invalid version",
		name));
	    Tcl_SetErrorCode(interp, "TTK", "REGISTER_ELEMENT", "VERSION",
		NULL);
	}
	return 0;
    }

    entryPtr = Tcl_CreateHashEntry(&theme->elementTable, name, &newEntry);
    if (!newEntry) {
	if (interp) {
	    Tcl_ResetResult(interp);
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Duplicate element %s", name));
	    Tcl_SetErrorCode(interp, "TTK", "REGISTER_ELEMENT", "DUPE", NULL);
	}
	return 0;
    }

    name = Tcl_GetHashKey(&theme->elementTable, entryPtr);
    elementClass = NewElementClass(name, specPtr, clientData);
    Tcl_SetHashValue(entryPtr, elementClass);

    return elementClass;
}

/* Ttk_RegisterElementSpec (deprecated) --
 * 	Register a new element.
 */
int Ttk_RegisterElementSpec(Ttk_Theme theme,
    const char *name, Ttk_ElementSpec *specPtr, void *clientData)
{
    return Ttk_RegisterElement(NULL, theme, name, specPtr, clientData)
	   ? TCL_OK : TCL_ERROR;
}

/*------------------------------------------------------------------------
 * +++ Element record initialization.
 */

/*
 * AllocateResource --
 * 	Extra initialization for element options like TK_OPTION_COLOR, etc.
 *
 * Returns: 1 if OK, 0 on failure.
 *
 * Note: if resource allocation fails at this point (just prior
 * to drawing an element), there's really no good place to
 * report the error.  Instead we just silently fail.
 */

static int AllocateResource(
    Ttk_ResourceCache cache,
    Tk_Window tkwin,
    Tcl_Obj **destPtr,
    int optionType)
{
    Tcl_Obj *resource = *destPtr;

    switch (optionType)
    {
	case TK_OPTION_FONT:
	    return (*destPtr = Ttk_UseFont(cache, tkwin, resource)) != NULL;
	case TK_OPTION_COLOR:
	    return (*destPtr = Ttk_UseColor(cache, tkwin, resource)) != NULL;
	case TK_OPTION_BORDER:
	    return (*destPtr = Ttk_UseBorder(cache, tkwin, resource)) != NULL;
	default:
	    /* no-op; always succeeds */
	    return 1;
    }
}

/*
 * InitializeElementRecord --
 *
 * 	Fill in the element record based on the element's option table.
 * 	Resources are initialized from:
 * 	the corresponding widget option if present and non-NULL,
 * 	otherwise the dynamic state map if specified,
 * 	otherwise from the corresponding widget resource if present,
 * 	otherwise the default value specified at registration time.
 *
 * Returns:
 * 	1 if OK, 0 if an error is detected.
 *
 * NOTES:
 * 	Tcl_Obj * reference counts are _NOT_ adjusted.
 */

static
int InitializeElementRecord(
    Ttk_ElementClass *eclass,	/* Element instance to initialize */
    Ttk_Style style,		/* Style table */
    char *widgetRecord,		/* Source of widget option values */
    Tk_OptionTable optionTable,	/* Option table describing widget record */
    Tk_Window tkwin,		/* Corresponding window */
    Ttk_State state)	/* Widget or element state */
{
    char *elementRecord = eclass->elementRecord;
    OptionMap optionMap = GetOptionMap(eclass,optionTable);
    int nResources = eclass->nResources;
    Ttk_ResourceCache cache = style->cache;
    Ttk_ElementOptionSpec *elementOption = eclass->specPtr->options;

    int i;
    for (i=0; i<nResources; ++i, ++elementOption) {
	Tcl_Obj **dest = (Tcl_Obj **)
	    (elementRecord + elementOption->offset);
	const char *optionName = elementOption->optionName;
	Tcl_Obj *dynamicSetting = Ttk_StyleMap(style, optionName, state);
	Tcl_Obj *widgetValue = 0;
	Tcl_Obj *elementDefault = eclass->defaultValues[i];

	if (optionMap[i]) {
	    widgetValue = *(Tcl_Obj **)
		(widgetRecord + optionMap[i]->objOffset);
	}

	if (widgetValue) {
	    *dest = widgetValue;
	} else if (dynamicSetting) {
	    *dest = dynamicSetting;
	} else {
	    Tcl_Obj *styleDefault = Ttk_StyleDefault(style, optionName);
	    *dest = styleDefault ? styleDefault : elementDefault;
	}

	if (!AllocateResource(cache, tkwin, dest, elementOption->type)) {
	    return 0;
	}
    }

    return 1;
}

/*------------------------------------------------------------------------
 * +++ Public API.
 */

/*
 * Ttk_QueryStyle --
 * 	Look up a style option based on the current state.
 */
Tcl_Obj *Ttk_QueryStyle(
    Ttk_Style style,		/* Style to query */
    void *recordPtr,		/* Widget record */
    Tk_OptionTable optionTable,	/* Option table describing widget record */
    const char *optionName,	/* Option name */
    Ttk_State state) 		/* Current state */
{
    const Tk_OptionSpec *optionSpec;
    Tcl_Obj *result;

    /*
     * Check widget record:
     */
    optionSpec = TTKGetOptionSpec(optionName, optionTable, TK_OPTION_ANY);
    if (optionSpec) {
	result = *(Tcl_Obj**)(((char*)recordPtr) + optionSpec->objOffset);
	if (result) {
	    return result;
	}
    }

    /*
     * Check dynamic settings:
     */
    result = Ttk_StyleMap(style, optionName, state);
    if (result) {
	return result;
    }

    /*
     * Use style default:
     */
    return Ttk_StyleDefault(style, optionName);
}

/*
 * Ttk_ElementSize --
 *	Compute the requested size of the given element.
 */

void
Ttk_ElementSize(
    Ttk_ElementClass *eclass,		/* Element to query */
    Ttk_Style style,			/* Style settings */
    char *recordPtr,			/* The widget record. */
    Tk_OptionTable optionTable,		/* Description of widget record */
    Tk_Window tkwin,			/* The widget window. */
    Ttk_State state,			/* Current widget state */
    int *widthPtr, 			/* Requested width */
    int *heightPtr,			/* Reqested height */
    Ttk_Padding *paddingPtr)		/* Requested inner border */
{
    paddingPtr->left = paddingPtr->right = paddingPtr->top = paddingPtr->bottom
	= *widthPtr = *heightPtr = 0;

    if (!InitializeElementRecord(
	    eclass, style, recordPtr, optionTable, tkwin,  state))
    {
	return;
    }
    eclass->specPtr->size(
	eclass->clientData, eclass->elementRecord,
	tkwin, widthPtr, heightPtr, paddingPtr);
}

/*
 * Ttk_DrawElement --
 *	Draw the given widget element in a given drawable area.
 */

void
Ttk_DrawElement(
    Ttk_ElementClass *eclass,		/* Element instance */
    Ttk_Style style,			/* Style settings */
    char *recordPtr,			/* The widget record. */
    Tk_OptionTable optionTable,		/* Description of option table */
    Tk_Window tkwin,			/* The widget window. */
    Drawable d,				/* Where to draw element. */
    Ttk_Box b,				/* Element area */
    Ttk_State state)			/* Widget or element state flags. */
{
    if (b.width <= 0 || b.height <= 0)
	return;
    if (!InitializeElementRecord(
	    eclass, style, recordPtr, optionTable, tkwin,  state))
    {
	return;
    }
    eclass->specPtr->draw(
	eclass->clientData, eclass->elementRecord,
	tkwin, d, b, state);
}

/*------------------------------------------------------------------------
 * +++ 'style' command ensemble procedures.
 */

/*
 * TtkEnumerateHashTable --
 * 	Helper routine.  Sets interp's result to the list of all keys
 * 	in the hash table.
 *
 * Returns: TCL_OK.
 * Side effects: Sets interp's result.
 */

MODULE_SCOPE
int TtkEnumerateHashTable(Tcl_Interp *interp, Tcl_HashTable *ht)
{
    Tcl_HashSearch search;
    Tcl_Obj *result = Tcl_NewListObj(0, NULL);
    Tcl_HashEntry *entryPtr = Tcl_FirstHashEntry(ht, &search);

    while (entryPtr != NULL) {
	Tcl_Obj *nameObj = Tcl_NewStringObj(Tcl_GetHashKey(ht, entryPtr),-1);
	Tcl_ListObjAppendElement(interp, result, nameObj);
	entryPtr = Tcl_NextHashEntry(&search);
    }

    Tcl_SetObjResult(interp, result);
    return TCL_OK;
}

/* HashTableToDict --
 * 	Helper routine.  Converts a TCL_STRING_KEYS Tcl_HashTable
 * 	with Tcl_Obj * entries into a dictionary.
 */
static Tcl_Obj* HashTableToDict(Tcl_HashTable *ht)
{
    Tcl_HashSearch search;
    Tcl_Obj *result = Tcl_NewListObj(0, NULL);
    Tcl_HashEntry *entryPtr = Tcl_FirstHashEntry(ht, &search);

    while (entryPtr != NULL) {
	Tcl_Obj *nameObj = Tcl_NewStringObj(Tcl_GetHashKey(ht, entryPtr),-1);
	Tcl_Obj *valueObj = Tcl_GetHashValue(entryPtr);
	Tcl_ListObjAppendElement(NULL, result, nameObj);
	Tcl_ListObjAppendElement(NULL, result, valueObj);
	entryPtr = Tcl_NextHashEntry(&search);
    }

    return result;
}

/* + style map $style ? -resource statemap ... ?
 *
 * 	Note that resource names are unconstrained; the Style
 * 	doesn't know what resources individual elements may use.
 */
static int
StyleMapCmd(
    ClientData clientData,		/* Master StylePackageData pointer */
    Tcl_Interp *interp,			/* Current interpreter */
    int objc,				/* Number of arguments */
    Tcl_Obj *const objv[])		/* Argument objects */
{
    StylePackageData *pkgPtr = clientData;
    Ttk_Theme theme = pkgPtr->currentTheme;
    const char *styleName;
    Style *stylePtr;
    int i;

    if (objc < 3) {
usage:
	Tcl_WrongNumArgs(interp,2,objv,"style ?-option ?value...??");
	return TCL_ERROR;
    }

    styleName = Tcl_GetString(objv[2]);
    stylePtr = Ttk_GetStyle(theme, styleName);

    /* NOTE: StateMaps are actually Tcl_Obj *s, so HashTableToDict works
     * for settingsTable.
     */
    if (objc == 3) {		/* style map $styleName */
	Tcl_SetObjResult(interp, HashTableToDict(&stylePtr->settingsTable));
	return TCL_OK;
    } else if (objc == 4) {	/* style map $styleName -option */
	const char *optionName = Tcl_GetString(objv[3]);
	Tcl_HashEntry *entryPtr =
	    Tcl_FindHashEntry(&stylePtr->settingsTable, optionName);
	if (entryPtr) {
	    Tcl_SetObjResult(interp, (Tcl_Obj*)Tcl_GetHashValue(entryPtr));
	}
	return TCL_OK;
    } else if (objc % 2 != 1) {
	goto usage;
    }

    for (i = 3; i < objc; i += 2) {
	const char *optionName = Tcl_GetString(objv[i]);
	Tcl_Obj *stateMap = objv[i+1];
	Tcl_HashEntry *entryPtr;
	int newEntry;

	/* Make sure 'stateMap' is legal:
	 * (@@@ SHOULD: check for valid resource values as well,
	 * but we don't know what types they should be at this level.)
	 */
	if (!Ttk_GetStateMapFromObj(interp, stateMap))
	    return TCL_ERROR;

	entryPtr = Tcl_CreateHashEntry(
		&stylePtr->settingsTable,optionName,&newEntry);

	Tcl_IncrRefCount(stateMap);
	if (!newEntry) {
	    Tcl_DecrRefCount((Tcl_Obj*)Tcl_GetHashValue(entryPtr));
	}
	Tcl_SetHashValue(entryPtr, stateMap);
    }
    ThemeChanged(pkgPtr);
    return TCL_OK;
}

/* + style configure $style -option ?value...
 */
static int StyleConfigureCmd(
    ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    StylePackageData *pkgPtr = clientData;
    Ttk_Theme theme = pkgPtr->currentTheme;
    const char *styleName;
    Style *stylePtr;
    int i;

    if (objc < 3) {
usage:
	Tcl_WrongNumArgs(interp,2,objv,"style ?-option ?value...??");
	return TCL_ERROR;
    }

    styleName = Tcl_GetString(objv[2]);
    stylePtr = Ttk_GetStyle(theme, styleName);

    if (objc == 3) {		/* style default $styleName */
	Tcl_SetObjResult(interp, HashTableToDict(&stylePtr->defaultsTable));
	return TCL_OK;
    } else if (objc == 4) {	/* style default $styleName -option */
	const char *optionName = Tcl_GetString(objv[3]);
	Tcl_HashEntry *entryPtr =
	    Tcl_FindHashEntry(&stylePtr->defaultsTable, optionName);
	if (entryPtr) {
	    Tcl_SetObjResult(interp, (Tcl_Obj*)Tcl_GetHashValue(entryPtr));
	}
	return TCL_OK;
    } else if (objc % 2 != 1) {
	goto usage;
    }

    for (i = 3; i < objc; i += 2) {
	const char *optionName = Tcl_GetString(objv[i]);
	Tcl_Obj *value = objv[i+1];
	Tcl_HashEntry *entryPtr;
	int newEntry;

	entryPtr = Tcl_CreateHashEntry(
		&stylePtr->defaultsTable,optionName,&newEntry);

	Tcl_IncrRefCount(value);
	if (!newEntry) {
	    Tcl_DecrRefCount((Tcl_Obj*)Tcl_GetHashValue(entryPtr));
	}
	Tcl_SetHashValue(entryPtr, value);
    }

    ThemeChanged(pkgPtr);
    return TCL_OK;
}

/* + style lookup $style -option ?statespec? ?defaultValue?
 */
static int StyleLookupCmd(
    ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    StylePackageData *pkgPtr = clientData;
    Ttk_Theme theme = pkgPtr->currentTheme;
    Ttk_Style style = NULL;
    const char *optionName;
    Ttk_State state = 0ul;
    Tcl_Obj *result;

    if (objc < 4 || objc > 6) {
	Tcl_WrongNumArgs(interp, 2, objv, "style -option ?state? ?default?");
	return TCL_ERROR;
    }

    style = Ttk_GetStyle(theme, Tcl_GetString(objv[2]));
    if (!style) {
	return TCL_ERROR;
    }
    optionName = Tcl_GetString(objv[3]);

    if (objc >= 5) {
	Ttk_StateSpec stateSpec;
	/* @@@ SB: Ttk_GetStateFromObj(); 'offbits' spec is ignored */
	if (Ttk_GetStateSpecFromObj(interp, objv[4], &stateSpec) != TCL_OK) {
	    return TCL_ERROR;
	}
	state = stateSpec.onbits;
    }

    result = Ttk_QueryStyle(style, NULL,NULL, optionName, state);
    if (result == NULL && objc >= 6) { /* Use caller-supplied fallback */
	result = objv[5];
    }

    if (result) {
	Tcl_SetObjResult(interp, result);
    }

    return TCL_OK;
}

static int StyleThemeCurrentCmd(
    ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[])
{
    StylePackageData *pkgPtr = clientData;
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr = NULL;
    const char *name = NULL;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 3, objv, "");
	return TCL_ERROR;
    }

    entryPtr = Tcl_FirstHashEntry(&pkgPtr->themeTable, &search);
    while (entryPtr != NULL) {
	Theme *ptr = Tcl_GetHashValue(entryPtr);
	if (ptr == pkgPtr->currentTheme) {
	    name = Tcl_GetHashKey(&pkgPtr->themeTable, entryPtr);
	    break;
	}
	entryPtr = Tcl_NextHashEntry(&search);
    }

    if (name == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"error: failed to get theme name", -1));
	Tcl_SetErrorCode(interp, "TTK", "THEME", "NAMELESS", NULL);
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewStringObj(name, -1));
    return TCL_OK;
}

/* + style theme create name ?-parent $theme? ?-settings { script }?
 */
static int StyleThemeCreateCmd(
    ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    StylePackageData *pkgPtr = clientData;
    static const char *optStrings[] =
    	 { "-parent", "-settings", NULL };
    enum { OP_PARENT, OP_SETTINGS };
    Ttk_Theme parentTheme = pkgPtr->defaultTheme, newTheme;
    Tcl_Obj *settingsScript = NULL;
    const char *themeName;
    int i;

    if (objc < 4 || objc % 2 != 0) {
	Tcl_WrongNumArgs(interp, 3, objv, "name ?-option value ...?");
	return TCL_ERROR;
    }

    themeName = Tcl_GetString(objv[3]);

    for (i=4; i < objc; i +=2) {
	int option;
	if (Tcl_GetIndexFromObjStruct(interp, objv[i], optStrings,
	    sizeof(char *), "option", 0, &option) != TCL_OK)
	{
	    return TCL_ERROR;
	}

	switch (option) {
	    case OP_PARENT:
		parentTheme = LookupTheme(
		    interp, pkgPtr, Tcl_GetString(objv[i+1]));
		if (!parentTheme)
		    return TCL_ERROR;
	    	break;
	    case OP_SETTINGS:
		settingsScript = objv[i+1];
		break;
	}
    }

    newTheme = Ttk_CreateTheme(interp, themeName, parentTheme);
    if (!newTheme) {
	return TCL_ERROR;
    }

    /*
     * Evaluate the -settings script, if supplied:
     */
    if (settingsScript) {
	Ttk_Theme oldTheme = pkgPtr->currentTheme;
	int status;

	pkgPtr->currentTheme = newTheme;
	status = Tcl_EvalObjEx(interp, settingsScript, 0);
	pkgPtr->currentTheme = oldTheme;
	return status;
    } else {
	return TCL_OK;
    }
}

/* + style theme names --
 * 	Return list of registered themes.
 */
static int StyleThemeNamesCmd(
    ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    StylePackageData *pkgPtr = clientData;
    return TtkEnumerateHashTable(interp, &pkgPtr->themeTable);
}

/* + style theme settings $theme $script
 *
 * 	Temporarily sets the current theme to $themeName,
 * 	evaluates $script, then restores the old theme.
 */
static int
StyleThemeSettingsCmd(
    ClientData clientData,		/* Master StylePackageData pointer */
    Tcl_Interp *interp,			/* Current interpreter */
    int objc,				/* Number of arguments */
    Tcl_Obj *const objv[])		/* Argument objects */
{
    StylePackageData *pkgPtr = clientData;
    Ttk_Theme oldTheme = pkgPtr->currentTheme;
    Ttk_Theme newTheme;
    int status;

    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 3, objv, "theme script");
	return TCL_ERROR;
    }

    newTheme = LookupTheme(interp, pkgPtr, Tcl_GetString(objv[3]));
    if (!newTheme)
	return TCL_ERROR;

    pkgPtr->currentTheme = newTheme;
    status = Tcl_EvalObjEx(interp, objv[4], 0);
    pkgPtr->currentTheme = oldTheme;

    return status;
}

/* + style element create name type ? ...args ?
 */
static int StyleElementCreateCmd(
    ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    StylePackageData *pkgPtr = clientData;
    Ttk_Theme theme = pkgPtr->currentTheme;
    const char *elementName, *factoryName;
    Tcl_HashEntry *entryPtr;
    FactoryRec *recPtr;

    if (objc < 5) {
	Tcl_WrongNumArgs(interp, 3, objv, "name type ?-option value ...?");
	return TCL_ERROR;
    }

    elementName = Tcl_GetString(objv[3]);
    factoryName = Tcl_GetString(objv[4]);

    entryPtr = Tcl_FindHashEntry(&pkgPtr->factoryTable, factoryName);
    if (!entryPtr) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"No such element type %s", factoryName));
	Tcl_SetErrorCode(interp, "TTK", "LOOKUP", "ELEMENT_TYPE", factoryName,
		NULL);
	return TCL_ERROR;
    }

    recPtr = Tcl_GetHashValue(entryPtr);

    return recPtr->factory(interp, recPtr->clientData,
	    theme, elementName, objc - 5, objv + 5);
}

/* + style element names --
 * 	Return a list of elements defined in the current theme.
 */
static int StyleElementNamesCmd(
    ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    StylePackageData *pkgPtr = clientData;
    Ttk_Theme theme = pkgPtr->currentTheme;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 3, objv, NULL);
	return TCL_ERROR;
    }
    return TtkEnumerateHashTable(interp, &theme->elementTable);
}

/* + style element options $element --
 * 	Return list of element options for specified element
 */
static int StyleElementOptionsCmd(
    ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    StylePackageData *pkgPtr = clientData;
    Ttk_Theme theme = pkgPtr->currentTheme;
    const char *elementName;
    Ttk_ElementClass *elementClass;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 3, objv, "element");
	return TCL_ERROR;
    }

    elementName = Tcl_GetString(objv[3]);
    elementClass = Ttk_GetElement(theme, elementName);
    if (elementClass) {
	Ttk_ElementSpec *specPtr = elementClass->specPtr;
	Ttk_ElementOptionSpec *option = specPtr->options;
	Tcl_Obj *result = Tcl_NewListObj(0,0);

	while (option->optionName) {
	    Tcl_ListObjAppendElement(
		interp, result, Tcl_NewStringObj(option->optionName,-1));
	    ++option;
	}

	Tcl_SetObjResult(interp, result);
	return TCL_OK;
    }

    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	"element %s not found", elementName));
    Tcl_SetErrorCode(interp, "TTK", "LOOKUP", "ELEMENT", elementName, NULL);
    return TCL_ERROR;
}

/* + style layout name ?spec?
 */
static int StyleLayoutCmd(
    ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    StylePackageData *pkgPtr = clientData;
    Ttk_Theme theme = pkgPtr->currentTheme;
    const char *layoutName;
    Ttk_LayoutTemplate layoutTemplate;

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "name ?spec?");
	return TCL_ERROR;
    }

    layoutName = Tcl_GetString(objv[2]);

    if (objc == 3) {
	layoutTemplate = Ttk_FindLayoutTemplate(theme, layoutName);
	if (!layoutTemplate) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Layout %s not found", layoutName));
	    Tcl_SetErrorCode(interp, "TTK", "LOOKUP", "LAYOUT", layoutName,
		NULL);
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Ttk_UnparseLayoutTemplate(layoutTemplate));
    } else {
	layoutTemplate = Ttk_ParseLayoutTemplate(interp, objv[3]);
	if (!layoutTemplate) {
	    return TCL_ERROR;
	}
	Ttk_RegisterLayoutTemplate(theme, layoutName, layoutTemplate);
	ThemeChanged(pkgPtr);
    }
    return TCL_OK;
}

/* + style theme use $theme --
 *  	Sets the current theme to $theme
 */
static int
StyleThemeUseCmd(
    ClientData clientData,		/* Master StylePackageData pointer */
    Tcl_Interp *interp,			/* Current interpreter */
    int objc,				/* Number of arguments */
    Tcl_Obj *const objv[])		/* Argument objects */
{
    StylePackageData *pkgPtr = clientData;
    Ttk_Theme theme;

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 3, objv, "?theme?");
	return TCL_ERROR;
    }

    if (objc == 3) {
	return StyleThemeCurrentCmd(clientData, interp, objc, objv);
    }

    theme = LookupTheme(interp, pkgPtr, Tcl_GetString(objv[3]));
    if (!theme) {
	return TCL_ERROR;
    }

    return Ttk_UseTheme(interp, theme);
}

/*
 * StyleObjCmd --
 *	Implementation of the [style] command.
 */

static const Ttk_Ensemble StyleThemeEnsemble[] = {
    { "create", StyleThemeCreateCmd, 0 },
    { "names", StyleThemeNamesCmd, 0 },
    { "settings", StyleThemeSettingsCmd, 0 },
    { "use", StyleThemeUseCmd, 0 },
    { NULL, 0, 0 }
};

static const Ttk_Ensemble StyleElementEnsemble[] = {
    { "create", StyleElementCreateCmd, 0 },
    { "names", StyleElementNamesCmd, 0 },
    { "options", StyleElementOptionsCmd, 0 },
    { NULL, 0, 0 }
};

static const Ttk_Ensemble StyleEnsemble[] = {
    { "configure", StyleConfigureCmd, 0 },
    { "map", StyleMapCmd, 0 },
    { "lookup", StyleLookupCmd, 0 },
    { "layout", StyleLayoutCmd, 0 },
    { "theme", 0, StyleThemeEnsemble },
    { "element", 0, StyleElementEnsemble },
    { NULL, 0, 0 }
};

static int
StyleObjCmd(
    ClientData clientData,		/* Master StylePackageData pointer */
    Tcl_Interp *interp,			/* Current interpreter */
    int objc,				/* Number of arguments */
    Tcl_Obj *const objv[])		/* Argument objects */
{
    return Ttk_InvokeEnsemble(StyleEnsemble, 1, clientData,interp,objc,objv);
}

MODULE_SCOPE
int Ttk_InvokeEnsemble(	/* Run an ensemble command */
    const Ttk_Ensemble *ensemble, int cmdIndex,
    void *clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    while (cmdIndex < objc) {
	int index;
	if (Tcl_GetIndexFromObjStruct(interp,
		objv[cmdIndex], ensemble, sizeof(ensemble[0]),
		"command", 0, &index)
	    != TCL_OK)
	{
	    return TCL_ERROR;
	}

	if (ensemble[index].command) {
	    return ensemble[index].command(clientData, interp, objc, objv);
	}
	ensemble = ensemble[index].ensemble;
	++cmdIndex;
    }
    Tcl_WrongNumArgs(interp, cmdIndex, objv, "option ?arg ...?");
    return TCL_ERROR;
}

/*
 * Ttk_StylePkgInit --
 *	Initializes all the structures that are used by the style
 *	package on a per-interp basis.
 */

void Ttk_StylePkgInit(Tcl_Interp *interp)
{
    Tcl_Namespace *nsPtr;

    StylePackageData *pkgPtr = ckalloc(sizeof(StylePackageData));

    pkgPtr->interp = interp;
    Tcl_InitHashTable(&pkgPtr->themeTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&pkgPtr->factoryTable, TCL_STRING_KEYS);
    pkgPtr->cleanupList = NULL;
    pkgPtr->cache = Ttk_CreateResourceCache(interp);
    pkgPtr->themeChangePending = 0;

    Tcl_SetAssocData(interp, PKG_ASSOC_KEY, Ttk_StylePkgFree, pkgPtr);

    /*
     * Create the default system theme:
     *
     * pkgPtr->defaultTheme must be initialized to 0 before
     * calling Ttk_CreateTheme for the first time, since it's used
     * as the parent theme.
     */
    pkgPtr->defaultTheme = 0;
    pkgPtr->defaultTheme = pkgPtr->currentTheme =
	Ttk_CreateTheme(interp, "default", NULL);

    /*
     * Register null element, used as a last-resort fallback:
     */
    Ttk_RegisterElement(interp, pkgPtr->defaultTheme, "", &ttkNullElementSpec, 0);

    /*
     * Register commands:
     */
    Tcl_CreateObjCommand(interp, "::ttk::style", StyleObjCmd, pkgPtr, 0);

    nsPtr = Tcl_FindNamespace(interp, "::ttk", NULL, TCL_LEAVE_ERR_MSG);
    Tcl_Export(interp, nsPtr, "style", 0 /* dontResetList */);

    Ttk_RegisterElementFactory(interp, "from", Ttk_CloneElement, 0);
}

/*EOF*/
