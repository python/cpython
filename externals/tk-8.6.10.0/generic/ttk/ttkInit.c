/*
 * Copyright (c) 2003, Joe English
 *
 * Ttk package: initialization routine and miscellaneous utilities.
 */

#include <string.h>
#include <tk.h>
#include "ttkTheme.h"
#include "ttkWidget.h"

/*
 * Legal values for the button -default option.
 * See also: enum Ttk_ButtonDefaultState.
 */
const char *ttkDefaultStrings[] = {
    "normal", "active", "disabled", NULL
};

int Ttk_GetButtonDefaultStateFromObj(
    Tcl_Interp *interp, Tcl_Obj *objPtr, int *statePtr)
{
    *statePtr = TTK_BUTTON_DEFAULT_DISABLED;
    return Tcl_GetIndexFromObjStruct(interp, objPtr, ttkDefaultStrings,
	    sizeof(char *), "default state", 0, statePtr);
}

/*
 * Legal values for the -compound option.
 * See also: enum Ttk_Compound.
 */
const char *ttkCompoundStrings[] = {
    "none", "text", "image", "center",
    "top", "bottom", "left", "right", NULL
};

int Ttk_GetCompoundFromObj(
    Tcl_Interp *interp, Tcl_Obj *objPtr, int *statePtr)
{
    *statePtr = TTK_COMPOUND_NONE;
    return Tcl_GetIndexFromObjStruct(interp, objPtr, ttkCompoundStrings,
	    sizeof(char *), "compound layout", 0, statePtr);
}

/*
 * Legal values for the -orient option.
 * See also: enum Ttk_Orient.
 */
const char *ttkOrientStrings[] = {
    "horizontal", "vertical", NULL
};

int Ttk_GetOrientFromObj(
    Tcl_Interp *interp, Tcl_Obj *objPtr, int *resultPtr)
{
    *resultPtr = TTK_ORIENT_HORIZONTAL;
    return Tcl_GetIndexFromObjStruct(interp, objPtr, ttkOrientStrings,
	    sizeof(char *), "orientation", 0, resultPtr);
}

/*
 * Recognized values for the -state compatibility option.
 * Other options are accepted and interpreted as synonyms for "normal".
 */
static const char *ttkStateStrings[] = {
    "normal", "readonly", "disabled", "active", NULL
};
enum {
    TTK_COMPAT_STATE_NORMAL,
    TTK_COMPAT_STATE_READONLY,
    TTK_COMPAT_STATE_DISABLED,
    TTK_COMPAT_STATE_ACTIVE
};

/* TtkCheckStateOption --
 * 	Handle -state compatibility option.
 *
 *	NOTE: setting -state disabled / -state enabled affects the
 *	widget state, but the internal widget state does *not* affect
 *	the value of the -state option.
 *	This option is present for compatibility only.
 */
void TtkCheckStateOption(WidgetCore *corePtr, Tcl_Obj *objPtr)
{
    int stateOption = TTK_COMPAT_STATE_NORMAL;
    unsigned all = TTK_STATE_DISABLED|TTK_STATE_READONLY|TTK_STATE_ACTIVE;
#   define SETFLAGS(f) TtkWidgetChangeState(corePtr, f, all^f)

    (void)Tcl_GetIndexFromObjStruct(NULL, objPtr, ttkStateStrings,
	    sizeof(char *), "", 0, &stateOption);
    switch (stateOption) {
	case TTK_COMPAT_STATE_NORMAL:
	default:
	    SETFLAGS(0);
	    break;
	case TTK_COMPAT_STATE_READONLY:
	    SETFLAGS(TTK_STATE_READONLY);
	    break;
	case TTK_COMPAT_STATE_DISABLED:
	    SETFLAGS(TTK_STATE_DISABLED);
	    break;
	case TTK_COMPAT_STATE_ACTIVE:
	    SETFLAGS(TTK_STATE_ACTIVE);
	    break;
    }
#   undef SETFLAGS
}

/* TtkSendVirtualEvent --
 * 	Send a virtual event notification to the specified target window.
 * 	Equivalent to "event generate $tgtWindow <<$eventName>>"
 *
 * 	Note that we use Tk_QueueWindowEvent, not Tk_HandleEvent,
 * 	so this routine does not reenter the interpreter.
 */
void TtkSendVirtualEvent(Tk_Window tgtWin, const char *eventName)
{
    union {XEvent general; XVirtualEvent virtual;} event;

    memset(&event, 0, sizeof(event));
    event.general.xany.type = VirtualEvent;
    event.general.xany.serial = NextRequest(Tk_Display(tgtWin));
    event.general.xany.send_event = False;
    event.general.xany.window = Tk_WindowId(tgtWin);
    event.general.xany.display = Tk_Display(tgtWin);
    event.virtual.name = Tk_GetUid(eventName);

    Tk_QueueWindowEvent(&event.general, TCL_QUEUE_TAIL);
}

/* TtkEnumerateOptions, TtkGetOptionValue --
 *	Common factors for data accessor commands.
 */
int TtkEnumerateOptions(
    Tcl_Interp *interp, void *recordPtr, const Tk_OptionSpec *specPtr,
    Tk_OptionTable optionTable, Tk_Window tkwin)
{
    Tcl_Obj *result = Tcl_NewListObj(0,0);
    while (specPtr->type != TK_OPTION_END)
    {
	Tcl_Obj *optionName = Tcl_NewStringObj(specPtr->optionName, -1);
	Tcl_Obj *optionValue =
	    Tk_GetOptionValue(interp,recordPtr,optionTable,optionName,tkwin);
	if (optionValue) {
	    Tcl_ListObjAppendElement(interp, result, optionName);
	    Tcl_ListObjAppendElement(interp, result, optionValue);
	}
	++specPtr;

	if (specPtr->type == TK_OPTION_END && specPtr->clientData != NULL) {
	    /* Chain to next option spec array: */
	    specPtr = specPtr->clientData;
	}
    }
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
}

int TtkGetOptionValue(
    Tcl_Interp *interp, void *recordPtr, Tcl_Obj *optionName,
    Tk_OptionTable optionTable, Tk_Window tkwin)
{
    Tcl_Obj *result =
	Tk_GetOptionValue(interp,recordPtr,optionTable,optionName,tkwin);
    if (result) {
	Tcl_SetObjResult(interp, result);
	return TCL_OK;
    }
    return TCL_ERROR;
}


/*------------------------------------------------------------------------
 * Core Option specifications:
 * type name dbName dbClass default objOffset intOffset flags clientData mask
 */

/* public */
Tk_OptionSpec ttkCoreOptionSpecs[] =
{
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor", NULL,
	Tk_Offset(WidgetCore, cursorObj), -1, TK_OPTION_NULL_OK,0,0 },
    {TK_OPTION_STRING, "-style", "style", "Style", "",
	Tk_Offset(WidgetCore,styleObj), -1, 0,0,STYLE_CHANGED},
    {TK_OPTION_STRING, "-class", "", "", NULL,
	Tk_Offset(WidgetCore,classObj), -1, 0,0,READONLY_OPTION},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0}
};

/*------------------------------------------------------------------------
 * +++ Initialization: elements and element factories.
 */

extern void TtkElements_Init(Tcl_Interp *);
extern void TtkLabel_Init(Tcl_Interp *);
extern void TtkImage_Init(Tcl_Interp *);

static void RegisterElements(Tcl_Interp *interp)
{
    TtkElements_Init(interp);
    TtkLabel_Init(interp);
    TtkImage_Init(interp);
}

/*------------------------------------------------------------------------
 * +++ Initialization: Widget definitions.
 */

extern void TtkButton_Init(Tcl_Interp *);
extern void TtkEntry_Init(Tcl_Interp *);
extern void TtkFrame_Init(Tcl_Interp *);
extern void TtkNotebook_Init(Tcl_Interp *);
extern void TtkPanedwindow_Init(Tcl_Interp *);
extern void TtkProgressbar_Init(Tcl_Interp *);
extern void TtkScale_Init(Tcl_Interp *);
extern void TtkScrollbar_Init(Tcl_Interp *);
extern void TtkSeparator_Init(Tcl_Interp *);
extern void TtkTreeview_Init(Tcl_Interp *);

#ifdef TTK_SQUARE_WIDGET
extern int TtkSquareWidget_Init(Tcl_Interp *);
#endif

static void RegisterWidgets(Tcl_Interp *interp)
{
    TtkButton_Init(interp);
    TtkEntry_Init(interp);
    TtkFrame_Init(interp);
    TtkNotebook_Init(interp);
    TtkPanedwindow_Init(interp);
    TtkProgressbar_Init(interp);
    TtkScale_Init(interp);
    TtkScrollbar_Init(interp);
    TtkSeparator_Init(interp);
    TtkTreeview_Init(interp);
#ifdef TTK_SQUARE_WIDGET
    TtkSquareWidget_Init(interp);
#endif
}

/*------------------------------------------------------------------------
 * +++ Initialization: Built-in themes.
 */

extern int TtkAltTheme_Init(Tcl_Interp *);
extern int TtkClassicTheme_Init(Tcl_Interp *);
extern int TtkClamTheme_Init(Tcl_Interp *);

static void RegisterThemes(Tcl_Interp *interp)
{

    TtkAltTheme_Init(interp);
    TtkClassicTheme_Init(interp);
    TtkClamTheme_Init(interp);
}

/*
 * Ttk initialization.
 */

extern const TtkStubs ttkStubs;

MODULE_SCOPE int
Ttk_Init(Tcl_Interp *interp)
{
    /*
     * This will be run for both safe and regular interp init.
     * Use Tcl_IsSafe if necessary to not initialize unsafe bits.
     */
    Ttk_StylePkgInit(interp);

    RegisterElements(interp);
    RegisterWidgets(interp);
    RegisterThemes(interp);

    Ttk_PlatformInit(interp);

    Tcl_PkgProvideEx(interp, "Ttk", TTK_PATCH_LEVEL, (ClientData)&ttkStubs);

    return TCL_OK;
}

/*EOF*/
