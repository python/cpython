/*
 * Copyright (c) 2003, Joe English
 * Helper routines for widget implementations.
 */

#ifndef _TTKWIDGET
#define _TTKWIDGET

/*
 * State flags for 'flags' field.
 */
#define WIDGET_DESTROYED	0x0001
#define REDISPLAY_PENDING 	0x0002	/* scheduled call to RedisplayWidget */
#define CURSOR_ON 		0x0020	/* See TtkBlinkCursor() */
#define WIDGET_USER_FLAG        0x0100  /* 0x0100 - 0x8000 for user flags */

/*
 * Bit fields for OptionSpec 'mask' field:
 */
#define READONLY_OPTION 	0x1
#define STYLE_CHANGED   	0x2
#define GEOMETRY_CHANGED	0x4

/*
 * Core widget elements
 */
typedef struct WidgetSpec_ WidgetSpec;	/* Forward */

typedef struct
{
    Tk_Window tkwin;		/* Window associated with widget */
    Tcl_Interp *interp;		/* Interpreter associated with widget. */
    WidgetSpec *widgetSpec;	/* Widget class hooks */
    Tcl_Command widgetCmd;	/* Token for widget command. */
    Tk_OptionTable optionTable;	/* Option table */
    Ttk_Layout layout;  	/* Widget layout */

    /*
     * Storage for resources:
     */
    Tcl_Obj *takeFocusPtr;	/* Storage for -takefocus option */
    Tcl_Obj *cursorObj;		/* Storage for -cursor option */
    Tcl_Obj *styleObj;		/* Name of currently-applied style */
    Tcl_Obj *classObj;		/* Class name (readonly option) */

    Ttk_State state;		/* Current widget state */
    unsigned int flags;		/* internal flags, see above */

} WidgetCore;

/*
 * Widget specifications:
 */
struct WidgetSpec_
{
    const char 		*className;	/* Widget class name */
    size_t 		recordSize;	/* #bytes in widget record */
    const Tk_OptionSpec	*optionSpecs;	/* Option specifications */
    const Ttk_Ensemble	*commands;	/* Widget instance subcommands */

    /*
     * Hooks:
     */
    void  	(*initializeProc)(Tcl_Interp *, void *recordPtr);
    void	(*cleanupProc)(void *recordPtr);
    int 	(*configureProc)(Tcl_Interp *, void *recordPtr, int flags);
    int 	(*postConfigureProc)(Tcl_Interp *, void *recordPtr, int flags);
    Ttk_Layout	(*getLayoutProc)(Tcl_Interp *,Ttk_Theme, void *recordPtr);
    int 	(*sizeProc)(void *recordPtr, int *widthPtr, int *heightPtr);
    void	(*layoutProc)(void *recordPtr);
    void	(*displayProc)(void *recordPtr, Drawable d);
};

/*
 * Common factors for widget implementations:
 */
MODULE_SCOPE void TtkNullInitialize(Tcl_Interp *, void *);
MODULE_SCOPE int  TtkNullPostConfigure(Tcl_Interp *, void *, int);
MODULE_SCOPE void TtkNullCleanup(void *recordPtr);
MODULE_SCOPE Ttk_Layout TtkWidgetGetLayout(
	Tcl_Interp *, Ttk_Theme, void *recordPtr);
MODULE_SCOPE Ttk_Layout TtkWidgetGetOrientedLayout(
	Tcl_Interp *, Ttk_Theme, void *recordPtr, Tcl_Obj *orientObj);
MODULE_SCOPE int  TtkWidgetSize(void *recordPtr, int *w, int *h);
MODULE_SCOPE void TtkWidgetDoLayout(void *recordPtr);
MODULE_SCOPE void TtkWidgetDisplay(void *recordPtr, Drawable);

MODULE_SCOPE int TtkCoreConfigure(Tcl_Interp*, void *, int mask);

/* Common widget commands:
 */
MODULE_SCOPE int TtkWidgetConfigureCommand(
	void *,Tcl_Interp *, int, Tcl_Obj*const[]);
MODULE_SCOPE int TtkWidgetCgetCommand(
	void *,Tcl_Interp *, int, Tcl_Obj*const[]);
MODULE_SCOPE int TtkWidgetInstateCommand(
	void *,Tcl_Interp *, int, Tcl_Obj*const[]);
MODULE_SCOPE int TtkWidgetStateCommand(
	void *,Tcl_Interp *, int, Tcl_Obj*const[]);
MODULE_SCOPE int TtkWidgetIdentifyCommand(
	void *,Tcl_Interp *, int, Tcl_Obj*const[]);

/* Widget constructor:
 */
MODULE_SCOPE int TtkWidgetConstructorObjCmd(
	ClientData, Tcl_Interp*, int, Tcl_Obj*const[]);

#define RegisterWidget(interp, name, specPtr) \
    Tcl_CreateObjCommand(interp, name, \
	TtkWidgetConstructorObjCmd, (ClientData)specPtr,NULL)

/* WIDGET_TAKEFOCUS_TRUE --
 * WIDGET_TAKEFOCUS_FALSE --
 *	Add one or the other of these to each OptionSpecs table 
 *	to indicate whether the widget should take focus 
 *	during keyboard traversal.
 */
#define WIDGET_TAKEFOCUS_TRUE \
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus", \
	"ttk::takefocus", Tk_Offset(WidgetCore, takeFocusPtr), -1, 0,0,0 }
#define WIDGET_TAKEFOCUS_FALSE \
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus", \
	"", Tk_Offset(WidgetCore, takeFocusPtr), -1, 0,0,0 }

/* WIDGET_INHERIT_OPTIONS(baseOptionSpecs) --
 * Add this at the end of an OptionSpecs table to inherit
 * the options from 'baseOptionSpecs'.
 */
#define WIDGET_INHERIT_OPTIONS(baseOptionSpecs) \
    {TK_OPTION_END, 0,0,0, NULL, -1,-1, 0, (ClientData)baseOptionSpecs, 0}

/* All widgets should inherit from ttkCoreOptionSpecs[].
 */
MODULE_SCOPE Tk_OptionSpec ttkCoreOptionSpecs[];

/*
 * Useful routines for use inside widget implementations:
 */
/* extern int WidgetDestroyed(WidgetCore *); */
#define WidgetDestroyed(corePtr) ((corePtr)->flags & WIDGET_DESTROYED)

MODULE_SCOPE void TtkWidgetChangeState(WidgetCore *,
	unsigned int setBits, unsigned int clearBits);

MODULE_SCOPE void TtkRedisplayWidget(WidgetCore *);
MODULE_SCOPE void TtkResizeWidget(WidgetCore *);

MODULE_SCOPE void TtkTrackElementState(WidgetCore *);
MODULE_SCOPE void TtkBlinkCursor(WidgetCore *);

/*
 * -state option values (compatibility)
 */
MODULE_SCOPE void TtkCheckStateOption(WidgetCore *, Tcl_Obj *);

/*
 * Variable traces:
 */
typedef void (*Ttk_TraceProc)(void *recordPtr, const char *value);
typedef struct TtkTraceHandle_ Ttk_TraceHandle;

MODULE_SCOPE Ttk_TraceHandle *Ttk_TraceVariable(
    Tcl_Interp*, Tcl_Obj *varnameObj, Ttk_TraceProc callback, void *clientData);
MODULE_SCOPE void Ttk_UntraceVariable(Ttk_TraceHandle *);
MODULE_SCOPE int Ttk_FireTrace(Ttk_TraceHandle *);

/*
 * Virtual events:
 */
MODULE_SCOPE void TtkSendVirtualEvent(Tk_Window tgtWin, const char *eventName);

/*
 * Helper routines for data accessor commands:
 */
MODULE_SCOPE int TtkEnumerateOptions(
    Tcl_Interp *, void *, const Tk_OptionSpec *, Tk_OptionTable, Tk_Window);
MODULE_SCOPE int TtkGetOptionValue(
    Tcl_Interp *, void *, Tcl_Obj *optName, Tk_OptionTable, Tk_Window);

/*
 * Helper routines for scrolling widgets (see scroll.c).
 */
typedef struct {
    int 	first;		/* First visible item */
    int 	last;		/* Last visible item */
    int 	total;		/* Total #items */
    char 	*scrollCmd;	/* Widget option */
} Scrollable;

typedef struct ScrollHandleRec *ScrollHandle;

MODULE_SCOPE ScrollHandle TtkCreateScrollHandle(WidgetCore *, Scrollable *);
MODULE_SCOPE void TtkFreeScrollHandle(ScrollHandle);

MODULE_SCOPE int TtkScrollviewCommand(
    Tcl_Interp *interp, int objc, Tcl_Obj *const objv[], ScrollHandle);

MODULE_SCOPE void TtkUpdateScrollInfo(ScrollHandle h);
MODULE_SCOPE void TtkScrollTo(ScrollHandle, int newFirst, int updateScrollInfo);
MODULE_SCOPE void TtkScrolled(ScrollHandle, int first, int last, int total);
MODULE_SCOPE void TtkScrollbarUpdateRequired(ScrollHandle);

/*
 * Tag sets (work in progress, half-baked)
 */

typedef struct TtkTag *Ttk_Tag;
typedef struct TtkTagTable *Ttk_TagTable;
typedef struct TtkTagSet {	/* TODO: make opaque */
    Ttk_Tag	*tags;
    int 	nTags;
} *Ttk_TagSet;

MODULE_SCOPE Ttk_TagTable Ttk_CreateTagTable(
	Tcl_Interp *, Tk_Window tkwin, Tk_OptionSpec[], int recordSize);
MODULE_SCOPE void Ttk_DeleteTagTable(Ttk_TagTable);

MODULE_SCOPE Ttk_Tag Ttk_GetTag(Ttk_TagTable, const char *tagName);
MODULE_SCOPE Ttk_Tag Ttk_GetTagFromObj(Ttk_TagTable, Tcl_Obj *);

MODULE_SCOPE Tcl_Obj *Ttk_TagOptionValue(
    Tcl_Interp *, Ttk_TagTable, Ttk_Tag, Tcl_Obj *optionName);

MODULE_SCOPE int Ttk_EnumerateTagOptions(
    Tcl_Interp *, Ttk_TagTable, Ttk_Tag);

MODULE_SCOPE int Ttk_EnumerateTags(Tcl_Interp *, Ttk_TagTable);

MODULE_SCOPE int Ttk_ConfigureTag(
    Tcl_Interp *interp, Ttk_TagTable tagTable, Ttk_Tag tag,
    int objc, Tcl_Obj *const objv[]);

MODULE_SCOPE Ttk_TagSet Ttk_GetTagSetFromObj(
    Tcl_Interp *interp, Ttk_TagTable, Tcl_Obj *objPtr);
MODULE_SCOPE Tcl_Obj *Ttk_NewTagSetObj(Ttk_TagSet);

MODULE_SCOPE void Ttk_FreeTagSet(Ttk_TagSet);

MODULE_SCOPE int Ttk_TagSetContains(Ttk_TagSet, Ttk_Tag tag);
MODULE_SCOPE int Ttk_TagSetAdd(Ttk_TagSet, Ttk_Tag tag);
MODULE_SCOPE int Ttk_TagSetRemove(Ttk_TagSet, Ttk_Tag tag);

MODULE_SCOPE void Ttk_TagSetValues(Ttk_TagTable, Ttk_TagSet, void *record);
MODULE_SCOPE void Ttk_TagSetApplyStyle(Ttk_TagTable,Ttk_Style,Ttk_State,void*);

/*
 * String tables for widget resource specifications:
 */

MODULE_SCOPE const char *ttkOrientStrings[];
MODULE_SCOPE const char *ttkCompoundStrings[];
MODULE_SCOPE const char *ttkDefaultStrings[];

/*
 * ... other option types...
 */
MODULE_SCOPE int TtkGetLabelAnchorFromObj(
	Tcl_Interp*, Tcl_Obj*, Ttk_PositionSpec *);

/*
 * Platform-specific initialization.
 */

#ifdef _WIN32
#define Ttk_PlatformInit Ttk_WinPlatformInit
MODULE_SCOPE int Ttk_PlatformInit(Tcl_Interp *);
#elif defined(MAC_OSX_TK)
#define Ttk_PlatformInit Ttk_MacOSXPlatformInit
MODULE_SCOPE int Ttk_PlatformInit(Tcl_Interp *);
#else
#define Ttk_PlatformInit(interp) /* TTK_X11PlatformInit() */
#endif

#endif /* _TTKWIDGET */
