/*
 * Copyright (c) 2004, Joe English
 *
 * ttk::separator and ttk::sizegrip widgets.
 */

#include <tk.h>

#include "ttkTheme.h"
#include "ttkWidget.h"

/* +++ Separator widget record:
 */
typedef struct
{
    Tcl_Obj	*orientObj;
    int 	orient;
} SeparatorPart;

typedef struct
{
    WidgetCore core;
    SeparatorPart separator;
} Separator;

static Tk_OptionSpec SeparatorOptionSpecs[] = {
    {TK_OPTION_STRING_TABLE, "-orient", "orient", "Orient", "horizontal",
	Tk_Offset(Separator,separator.orientObj),
	Tk_Offset(Separator,separator.orient),
	0,(ClientData)ttkOrientStrings,STYLE_CHANGED },

    WIDGET_TAKEFOCUS_FALSE,
    WIDGET_INHERIT_OPTIONS(ttkCoreOptionSpecs)
};

/*
 * GetLayout hook --
 * 	Choose layout based on -orient option.
 */
static Ttk_Layout SeparatorGetLayout(
    Tcl_Interp *interp, Ttk_Theme theme, void *recordPtr)
{
    Separator *sep = recordPtr;
    return TtkWidgetGetOrientedLayout(
	interp, theme, recordPtr, sep->separator.orientObj);
}

/*
 * Widget commands:
 */
static const Ttk_Ensemble SeparatorCommands[] = {
    { "configure",	TtkWidgetConfigureCommand,0 },
    { "cget",		TtkWidgetCgetCommand,0 },
    { "identify",	TtkWidgetIdentifyCommand,0 },
    { "instate",	TtkWidgetInstateCommand,0 },
    { "state",  	TtkWidgetStateCommand,0 },
    { 0,0,0 }
};

/*
 * Widget specification:
 */
static WidgetSpec SeparatorWidgetSpec =
{
    "TSeparator",		/* className */
    sizeof(Separator),		/* recordSize */
    SeparatorOptionSpecs,	/* optionSpecs */
    SeparatorCommands,		/* subcommands */
    TtkNullInitialize,		/* initializeProc */
    TtkNullCleanup,		/* cleanupProc */
    TtkCoreConfigure,		/* configureProc */
    TtkNullPostConfigure,	/* postConfigureProc */
    SeparatorGetLayout,		/* getLayoutProc */
    TtkWidgetSize, 		/* sizeProc */
    TtkWidgetDoLayout,		/* layoutProc */
    TtkWidgetDisplay		/* displayProc */
};

TTK_BEGIN_LAYOUT(SeparatorLayout)
    TTK_NODE("Separator.separator", TTK_FILL_BOTH)
TTK_END_LAYOUT

/* +++ Sizegrip widget:
 * 	Has no options or methods other than the standard ones.
 */

static Tk_OptionSpec SizegripOptionSpecs[] = {
    WIDGET_TAKEFOCUS_FALSE,
    WIDGET_INHERIT_OPTIONS(ttkCoreOptionSpecs)
};

static const Ttk_Ensemble SizegripCommands[] = {
    { "configure",	TtkWidgetConfigureCommand,0 },
    { "cget",		TtkWidgetCgetCommand,0 },
    { "identify",	TtkWidgetIdentifyCommand,0 },
    { "instate",	TtkWidgetInstateCommand,0 },
    { "state",  	TtkWidgetStateCommand,0 },
    { 0,0,0 }
};

static WidgetSpec SizegripWidgetSpec =
{
    "TSizegrip",		/* className */
    sizeof(WidgetCore),		/* recordSize */
    SizegripOptionSpecs, 	/* optionSpecs */
    SizegripCommands,		/* subcommands */
    TtkNullInitialize,		/* initializeProc */
    TtkNullCleanup,		/* cleanupProc */
    TtkCoreConfigure,		/* configureProc */
    TtkNullPostConfigure,	/* postConfigureProc */
    TtkWidgetGetLayout, 	/* getLayoutProc */
    TtkWidgetSize, 		/* sizeProc */
    TtkWidgetDoLayout,		/* layoutProc */
    TtkWidgetDisplay		/* displayProc */
};

TTK_BEGIN_LAYOUT(SizegripLayout)
    TTK_NODE("Sizegrip.sizegrip", TTK_PACK_BOTTOM|TTK_STICK_S|TTK_STICK_E)
TTK_END_LAYOUT

/* +++ Initialization:
 */

MODULE_SCOPE
void TtkSeparator_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme = Ttk_GetDefaultTheme(interp);

    Ttk_RegisterLayout(theme, "TSeparator", SeparatorLayout);
    Ttk_RegisterLayout(theme, "TSizegrip", SizegripLayout);

    RegisterWidget(interp, "ttk::separator", &SeparatorWidgetSpec);
    RegisterWidget(interp, "ttk::sizegrip", &SizegripWidgetSpec);
}

/*EOF*/
