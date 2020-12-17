/*
 * tkBind.c --
 *
 *	This file provides functions that associate Tcl commands with X events
 *	or sequences of X events.
 *
 * Copyright (c) 1989-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998 by Scriptics Corporation.
 * Copyright (c) 2018-2019 by Gregor Cramer.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "tkDList.h"
#include "tkArray.h"

#if defined(_WIN32)
#include "tkWinInt.h"
#elif defined(MAC_OSX_TK)
#include "tkMacOSXInt.h"
#else /* if defined(__unix__) */
#include "tkUnixInt.h"
#endif

#ifdef NDEBUG
# define DEBUG(expr)
#else
# define DEBUG(expr) expr
#endif

#ifdef _MSC_VER
/*
 * Earlier versions of MSVC don't know snprintf, but _snprintf is compatible.
 * Note that sprintf is deprecated.
 */
# define snprintf _snprintf
#endif

#define SIZE_OF_ARRAY(arr) (sizeof(arr)/sizeof(arr[0]))

/*
 * File structure:
 *
 * Structure definitions and static variables.
 *
 * Init/Free this package.
 *
 * Tcl "bind" command (actually located in tkCmds.c) core implementation, plus helpers.
 *
 * Tcl "event" command implementation, plus helpers.
 *
 * Package-specific common helpers.
 *
 * Non-package-specific helpers.
 */

/*
 * In old implementation (the one that used an event ring), <Double-1> and <1><1> were
 * equivalent sequences. However it is logical to give <Double-1> higher precedence.
 * This can be achieved by setting PREFER_MOST_SPECIALIZED_EVENT to 1.
 */

#ifndef PREFER_MOST_SPECIALIZED_EVENT
# define PREFER_MOST_SPECIALIZED_EVENT 0
#endif

/*
 * Traditionally motion events can be combined with buttons in this way: <B1-B2-Motion>.
 * However it should be allowed to express this as <Motion-1-2> in addition. This can be
 * achieved by setting SUPPORT_ADDITIONAL_MOTION_SYNTAX to 1.
 */

#ifndef SUPPORT_ADDITIONAL_MOTION_SYNTAX
# define SUPPORT_ADDITIONAL_MOTION_SYNTAX 0 /* set to 1 if wanted */
#endif

/*
 * The output for motion events is of the type <B1-Motion>. This can be changed to become
 * <Motion-1> instead by setting PRINT_SHORT_MOTION_SYNTAX to 1, however this would be a
 * backwards incompatibility.
 */

#ifndef PRINT_SHORT_MOTION_SYNTAX
# define PRINT_SHORT_MOTION_SYNTAX 0 /* set to 1 if wanted */
#endif

#if !SUPPORT_ADDITIONAL_MOTION_SYNTAX
# undef PRINT_SHORT_MOTION_SYNTAX
# define PRINT_SHORT_MOTION_SYNTAX 0
#endif

/*
 * For debugging only, normally set to zero.
 */

#ifdef SUPPORT_DEBUGGING
# undef SUPPORT_DEBUGGING
#endif
#define SUPPORT_DEBUGGING 0

/*
 * Test validity of PSEntry items.
 */

# define TEST_PSENTRY(psPtr) psPtr->number != 0xdeadbeef
# define MARK_PSENTRY(psPtr) psPtr->number = 0xdeadbeef

/*
 * The following union is used to hold the detail information from an XEvent
 * (including Tk's XVirtualEvent extension).
 */

typedef KeySym Info;

typedef union {
    Info info;		/* This either corresponds to xkey.keycode, or to xbutton.button,
    			 * or is meaningless, depending on event type. */
    Tk_Uid name;	/* Tk_Uid of virtual event. */
} Detail;

/*
 * We need an extended event definition.
 */

typedef struct {
    XEvent xev;		/* The original event from server. */
    Detail detail;	/* Additional information (for hashing). */
    unsigned countAny;	/* Count of multi-events, like multi-clicks, or repeated key pressing,
    			 * this count does not depend on detail (keySym or button). */
    unsigned countDetailed;
    			/* Count of multi-events, like multi-clicks, or repeated key pressing,
    			 * this count considers the detail (keySym or button). */
} Event;

/*
 * We need a structure providing a list of pattern sequences.
 */

typedef unsigned EventMask;
typedef unsigned long ModMask;

struct PatSeq; /* forward declaration */

/* We need this array for bookkeeping the last matching modifier mask per pattern. */
TK_ARRAY_DEFINE(PSModMaskArr, ModMask);

typedef struct PSEntry {
    TK_DLIST_LINKS(PSEntry);	/* Makes this struct a double linked list; must be first entry. */
    Window window;		/* Window of last match. */
    struct PatSeq* psPtr;	/* Pointer to pattern sequence. */
    PSModMaskArr *lastModMaskArr;
    				/* Last matching modifier mask per pattern (except last pattern).
    				 * Only needed if pattern sequence is not single (more than one
				 * pattern), and if one of these patterns contains a non-zero
				 * modifier mask. */
    unsigned count;		/* Only promote to next level if this count has reached count of
    				 * pattern. */
    unsigned expired:1;		/* Whether this entry is expired, this means it has to be removed
    				 * from promotion list. */
    unsigned keepIt:1;		/* Whether to keep this entry, even if expired. */
} PSEntry;

/* Defining the whole PSList_* stuff (list of PSEntry items). */
TK_DLIST_DEFINE(PSList, PSEntry);

/* Don't keep larger arrays of modifier masks inside PSEntry. */
#define MAX_MOD_MASK_ARR_SIZE 8

/*
 * Maps and lookup tables from an event to a list of patterns that match that event.
 */

typedef struct {
    Tcl_HashTable patternTable;	/* Keys are PatternTableKey structs, values are (PatSeq *). */
    Tcl_HashTable listTable;	/* Keys are PatternTableKey structs, values are (PSList *). */
    PSList entryPool;		/* Contains free (unused) list items. */
    unsigned number;		/* Needed for enumeration of pattern sequences. */
} LookupTables;

/*
 * The structure below represents a binding table. A binding table represents
 * a domain in which event bindings may occur. It includes a space of objects
 * relative to which events occur (usually windows, but not always), a history
 * of recent events in the domain, and a set of mappings that associate
 * particular Tcl commands with sequences of events in the domain. Multiple
 * binding tables may exist at once, either because there are multiple
 * applications open, or because there are multiple domains within an
 * application with separate event bindings for each (for example, each canvas
 * widget has a separate binding table for associating events with the items
 * in the canvas).
 */

/* defining the whole Promotion_* stuff (array of PSList entries) */
TK_ARRAY_DEFINE(PromArr, PSList);

typedef struct Tk_BindingTable_ {
    Event eventInfo[TK_LASTEVENT];
    				/* Containing the most recent event for every event type. */
    PromArr *promArr;		/* Contains the promoted pattern sequences. */
    Event *curEvent;		/* Pointing to most recent event. */
    ModMask curModMask;		/* Containing the current modifier mask. */
    LookupTables lookupTables;	/* Containing hash tables for fast lookup. */
    Tcl_HashTable objectTable;	/* Used to map from an object to a list of patterns associated with
    				 * that object. Keys are ClientData, values are (PatSeq *). */
    Tcl_Interp *interp;		/* Interpreter in which commands are executed. */
} BindingTable;

/*
 * The following structure represents virtual event table. A virtual event
 * table provides a way to map from platform-specific physical events such as
 * button clicks or key presses to virtual events such as <<Paste>>,
 * <<Close>>, or <<ScrollWindow>>.
 *
 * A virtual event is usually never part of the event stream, but instead is
 * synthesized inline by matching low-level events. However, a virtual event
 * may be generated by platform-specific code or by Tcl commands. In that case,
 * no lookup of the virtual event will need to be done using this table,
 * because the virtual event is actually in the event stream.
 */

typedef struct {
    LookupTables lookupTables;	/* Providing fast lookup tables to lists of pattern sequences. */
    Tcl_HashTable nameTable;	/* Used to map a virtual event name to the array of physical events
    				 * that can trigger it. Keys are the Tk_Uid names of the virtual
				 * events, values are PhysOwned structs. */
} VirtualEventTable;

/*
 * The following structure is used as a key in a patternTable for both binding
 * tables and a virtual event tables.
 *
 * In a binding table, the object field corresponds to the binding tag for the
 * widget whose bindings are being accessed.
 *
 * In a virtual event table, the object field is always NULL. Virtual events
 * are a global definiton and are not tied to a particular binding tag.
 *
 * The same key is used for both types of pattern tables so that the helper
 * functions that traverse and match patterns will work for both binding
 * tables and virtual event tables.
 */

typedef struct {
    ClientData object;		/* For binding table, identifies the binding tag of the object
    				 * (or class of objects) relative to which the event occurred.
				 * For virtual event table, always NULL. */
    unsigned type;		/* Type of event (from X). */
    Detail detail;		/* Additional information, such as keysym, button, Tk_Uid, or zero
    				 * if nothing additional. */
} PatternTableKey;

/*
 * The following structure defines a pattern, which is matched against X
 * events as part of the process of converting X events into Tcl commands.
 *
 * For technical reasons we do not use 'union Detail', although this would
 * be possible, instead 'info' and 'name' are both included.
 */

typedef struct {
    unsigned eventType;		/* Type of X event, e.g. ButtonPress. */
    unsigned count;		/* Multi-event count, e.g. double-clicks, triple-clicks, etc. */
    ModMask modMask;		/* Mask of modifiers that must be present (zero means no modifiers
    				 * are required). */
    Info info;			/* Additional information that must match event. Normally this is zero,
    				 * meaning no additional information must match. For KeyPress and
				 * KeyRelease events, it may be specified to select a particular
				 * keystroke (zero means any keystrokes). For button events, specifies
				 * a particular button (zero means any buttons are OK). */
    Tk_Uid name;		/* Specifies the Tk_Uid of the virtual event name. NULL if not a
    				 * virtual event. */
} TkPattern;

/*
 * The following structure keeps track of all the virtual events that are
 * associated with a particular physical event. It is pointed to by the 'owners'
 * field in a PatSeq in the patternTable of a virtual event table.
 */

TK_PTR_ARRAY_DEFINE(VirtOwners, Tcl_HashEntry); /* define array of hash entries */

/*
 * The following structure defines a pattern sequence, which consists of one
 * or more patterns. In order to trigger, a pattern sequence must match the
 * most recent X events (first pattern to most recent event, next pattern to
 * next event, and so on). It is used as the hash value in a patternTable for
 * both binding tables and virtual event tables.
 *
 * In a binding table, it is the sequence of physical events that make up a
 * binding for an object.
 *
 * In a virtual event table, it is the sequence of physical events that define
 * a virtual event.
 *
 * The same structure is used for both types of pattern tables so that the
 * helper functions that traverse and match patterns will work for both
 * binding tables and virtual event tables.
 */

typedef struct PatSeq {
    unsigned numPats;		/* Number of patterns in sequence (usually 1). */
    unsigned count;		/* Total number of repetition counts, summed over count in TkPattern. */
    unsigned number;		/* Needed for the decision whether a binding is less recently defined
    				 * than another, it is guaranteed that the most recently bound event
				 * has the highest number. */
    unsigned added:1;		/* Is this pattern sequence already added to lookup table? */
    unsigned modMaskUsed:1;	/* Does at least one pattern contain a non-zero modifier mask? */
    DEBUG(unsigned owned:1;)	/* For debugging purposes. */
    char *script;		/* Binding script to evaluate when sequence matches (ckalloc()ed) */
    Tcl_Obj* object;		/* Token for object with which binding is associated. For virtual
    				 * event table this is NULL. */
    struct PatSeq *nextSeqPtr;	/* Next in list of all pattern sequences that have the same initial
    				 * pattern. NULL means end of list. */
    Tcl_HashEntry *hPtr;	/* Pointer to hash table entry for the initial pattern. This is the
    				 * head of the list of which nextSeqPtr forms a part. */
    union {
	VirtOwners *owners;	/* In a binding table it has no meaning. In a virtual event table,
				 * identifies the array of virtual events that can be triggered
				 * by this event. */
	struct PatSeq *nextObj;	/* In a binding table, next in list of all pattern sequences for
				 * the same object (NULL for end of list). Needed to implement
				 * Tk_DeleteAllBindings. In a virtual event table it has no meaning. */
    } ptr;
    TkPattern pats[1];		/* Array of "numPats" patterns. Only one element is declared here
    				 * but in actuality enough space will be allocated for "numPats"
				 * patterns (but usually 1). */
} PatSeq;

/*
 * Compute memory size of struct PatSeq with given pattern size.
 * The caller must be sure that pattern size is greater than zero.
 */
#define PATSEQ_MEMSIZE(numPats) (sizeof(PatSeq) + (numPats - 1)*sizeof(TkPattern))

/*
 * Constants that define how close together two events must be in milliseconds
 * or pixels to meet the PAT_NEARBY constraint:
 */

#define NEARBY_PIXELS	5
#define NEARBY_MS	500

/*
 * Needed as "no-number" constant for integers. The value of this constant is
 * outside of integer range (type "int"). (Unfortunatly current version of
 * Tcl/Tk does not provide C99 integer support.)
 */

#define NO_NUMBER (((Tcl_WideInt) (~ (unsigned) 0)) + 1)

/*
 * The following structure is used in the nameTable of a virtual event table
 * to associate a virtual event with all the physical events that can trigger
 * it.
 */

TK_PTR_ARRAY_DEFINE(PhysOwned, PatSeq); /* define array of pattern seqs */

/*
 * One of the following structures exists for each interpreter. This structure
 * keeps track of the current display and screen in the interpreter, so that a
 * command can be invoked whenever the display/screen changes (the command does
 * things like point tk::Priv at a display-specific structure).
 */

typedef struct {
    TkDisplay *curDispPtr;	/* Display for last binding command invoked in this application. */
    int curScreenIndex;		/* Index of screen for last binding command */
    unsigned bindingDepth;	/* Number of active instances of Tk_BindEvent in this application. */
} ScreenInfo;

/*
 * The following structure keeps track of all the information local to the
 * binding package on a per interpreter basis.
 */

typedef struct TkBindInfo_ {
    VirtualEventTable virtualEventTable;
				/* The virtual events that exist in this interpreter. */
    ScreenInfo screenInfo;	/* Keeps track of the current display and screen, so it can be
    				 * restored after a binding has executed. */
    int deleted;		/* 1 if the application has been deleted but the structure has been
    				 * preserved. */
    Time lastEventTime;		/* Needed for time measurement. */
    Time lastCurrentTime;	/* Needed for time measurement. */
} BindInfo;

/*
 * In X11R4 and earlier versions, XStringToKeysym is ridiculously slow. The
 * data structure and hash table below, along with the code that uses them,
 * implement a fast mapping from strings to keysyms. In X11R5 and later
 * releases XStringToKeysym is plenty fast so this stuff isn't needed. The
 * #define REDO_KEYSYM_LOOKUP is normally undefined, so that XStringToKeysym
 * gets used. It can be set in the Makefile to enable the use of the hash
 * table below.
 */

#ifdef REDO_KEYSYM_LOOKUP
typedef struct {
    const char *name;		/* Name of keysym. */
    KeySym value;		/* Numeric identifier for keysym. */
} KeySymInfo;
static const KeySymInfo keyArray[] = {
#ifndef lint
#include "ks_names.h"
#endif
    {NULL, 0}
};
static Tcl_HashTable keySymTable;	/* keyArray hashed by keysym value. */
static Tcl_HashTable nameTable;		/* keyArray hashed by keysym name. */
#endif /* REDO_KEYSYM_LOOKUP */

/*
 * A hash table is kept to map from the string names of event modifiers to
 * information about those modifiers. The structure for storing this
 * information, and the hash table built at initialization time, are defined
 * below.
 */

typedef struct {
    const char *name;	/* Name of modifier. */
    ModMask mask;	/* Button/modifier mask value, such as Button1Mask. */
    unsigned flags;	/* Various flags; see below for definitions. */
} ModInfo;

/*
 * Flags for ModInfo structures:
 *
 * DOUBLE -		Non-zero means duplicate this event, e.g. for double-clicks.
 * TRIPLE -		Non-zero means triplicate this event, e.g. for triple-clicks.
 * QUADRUPLE -		Non-zero means quadruple this event, e.g. for 4-fold-clicks.
 * MULT_CLICKS -	Combination of all of above.
 */

#define DOUBLE		(1<<0)
#define TRIPLE		(1<<1)
#define QUADRUPLE	(1<<2)
#define MULT_CLICKS	(DOUBLE|TRIPLE|QUADRUPLE)

static const ModInfo modArray[] = {
    {"Control",		ControlMask,	0},
    {"Shift",		ShiftMask,	0},
    {"Lock",		LockMask,	0},
    {"Meta",		META_MASK,	0},
    {"M",		META_MASK,	0},
    {"Alt",		ALT_MASK,	0},
    {"Extended",	EXTENDED_MASK,	0},
    {"B1",		Button1Mask,	0},
    {"Button1",		Button1Mask,	0},
    {"B2",		Button2Mask,	0},
    {"Button2",		Button2Mask,	0},
    {"B3",		Button3Mask,	0},
    {"Button3",		Button3Mask,	0},
    {"B4",		Button4Mask,	0},
    {"Button4",		Button4Mask,	0},
    {"B5",		Button5Mask,	0},
    {"Button5",		Button5Mask,	0},
    {"Mod1",		Mod1Mask,	0},
    {"M1",		Mod1Mask,	0},
    {"Command",		Mod1Mask,	0},
    {"Mod2",		Mod2Mask,	0},
    {"M2",		Mod2Mask,	0},
    {"Option",		Mod2Mask,	0},
    {"Mod3",		Mod3Mask,	0},
    {"M3",		Mod3Mask,	0},
    {"Mod4",		Mod4Mask,	0},
    {"M4",		Mod4Mask,	0},
    {"Mod5",		Mod5Mask,	0},
    {"M5",		Mod5Mask,	0},
    {"Double",		0,		DOUBLE},
    {"Triple",		0,		TRIPLE},
    {"Quadruple",	0,		QUADRUPLE},
    {"Any",		0,		0},	/* Ignored: historical relic */
    {NULL,		0,		0}
};
static Tcl_HashTable modTable;

/*
 * This module also keeps a hash table mapping from event names to information
 * about those events. The structure, an array to use to initialize the hash
 * table, and the hash table are all defined below.
 */

typedef struct {
    const char *name;	/* Name of event. */
    unsigned type;	/* Event type for X, such as ButtonPress. */
    unsigned eventMask;	/* Mask bits (for XSelectInput) for this event type. */
} EventInfo;

/*
 * Note: some of the masks below are an OR-ed combination of several masks.
 * This is necessary because X doesn't report up events unless you also ask
 * for down events. Also, X doesn't report button state in motion events
 * unless you've asked about button events.
 */

static const EventInfo eventArray[] = {
    {"Key",		KeyPress,		KeyPressMask},
    {"KeyPress",	KeyPress,		KeyPressMask},
    {"KeyRelease",	KeyRelease,		KeyPressMask|KeyReleaseMask},
    {"Button",		ButtonPress,		ButtonPressMask},
    {"ButtonPress",	ButtonPress,		ButtonPressMask},
    {"ButtonRelease",	ButtonRelease,		ButtonPressMask|ButtonReleaseMask},
    {"Motion",		MotionNotify,		ButtonPressMask|PointerMotionMask},
    {"Enter",		EnterNotify,		EnterWindowMask},
    {"Leave",		LeaveNotify,		LeaveWindowMask},
    {"FocusIn",		FocusIn,		FocusChangeMask},
    {"FocusOut",	FocusOut,		FocusChangeMask},
    {"Expose",		Expose,			ExposureMask},
    {"Visibility",	VisibilityNotify,	VisibilityChangeMask},
    {"Destroy",		DestroyNotify,		StructureNotifyMask},
    {"Unmap",		UnmapNotify,		StructureNotifyMask},
    {"Map",		MapNotify,		StructureNotifyMask},
    {"Reparent",	ReparentNotify,		StructureNotifyMask},
    {"Configure",	ConfigureNotify,	StructureNotifyMask},
    {"Gravity",		GravityNotify,		StructureNotifyMask},
    {"Circulate",	CirculateNotify,	StructureNotifyMask},
    {"Property",	PropertyNotify,		PropertyChangeMask},
    {"Colormap",	ColormapNotify,		ColormapChangeMask},
    {"Activate",	ActivateNotify,		ActivateMask},
    {"Deactivate",	DeactivateNotify,	ActivateMask},
    {"MouseWheel",	MouseWheelEvent,	MouseWheelMask},
    {"CirculateRequest", CirculateRequest,	SubstructureRedirectMask},
    {"ConfigureRequest", ConfigureRequest,	SubstructureRedirectMask},
    {"Create",		CreateNotify,		SubstructureNotifyMask},
    {"MapRequest",	MapRequest,		SubstructureRedirectMask},
    {"ResizeRequest",	ResizeRequest,		ResizeRedirectMask},
    {NULL,		0,			0}
};
static Tcl_HashTable eventTable;

static int eventArrayIndex[TK_LASTEVENT];

/*
 * The defines and table below are used to classify events into various
 * groups. The reason for this is that logically identical fields (e.g.
 * "state") appear at different places in different types of events. The
 * classification masks can be used to figure out quickly where to extract
 * information from events.
 */

#define KEY			(1<<0)
#define BUTTON			(1<<1)
#define MOTION			(1<<2)
#define CROSSING		(1<<3)
#define FOCUS			(1<<4)
#define EXPOSE			(1<<5)
#define VISIBILITY		(1<<6)
#define CREATE			(1<<7)
#define DESTROY			(1<<8)
#define UNMAP			(1<<9)
#define MAP			(1<<10)
#define REPARENT		(1<<11)
#define CONFIG			(1<<12)
#define GRAVITY			(1<<13)
#define CIRC			(1<<14)
#define PROP			(1<<15)
#define COLORMAP		(1<<16)
#define VIRTUAL			(1<<17)
#define ACTIVATE		(1<<18)
#define	MAPREQ			(1<<19)
#define	CONFIGREQ		(1<<20)
#define	RESIZEREQ		(1<<21)
#define CIRCREQ			(1<<22)

#define KEY_BUTTON_MOTION_VIRTUAL	(KEY|BUTTON|MOTION|VIRTUAL)
#define KEY_BUTTON_MOTION_CROSSING	(KEY|BUTTON|MOTION|VIRTUAL|CROSSING)

static const int flagArray[TK_LASTEVENT] = {
   /* Not used */		0,
   /* Not used */		0,
   /* KeyPress */		KEY,
   /* KeyRelease */		KEY,
   /* ButtonPress */		BUTTON,
   /* ButtonRelease */		BUTTON,
   /* MotionNotify */		MOTION,
   /* EnterNotify */		CROSSING,
   /* LeaveNotify */		CROSSING,
   /* FocusIn */		FOCUS,
   /* FocusOut */		FOCUS,
   /* KeymapNotify */		0,
   /* Expose */			EXPOSE,
   /* GraphicsExpose */		EXPOSE,
   /* NoExpose */		0,
   /* VisibilityNotify */	VISIBILITY,
   /* CreateNotify */		CREATE,
   /* DestroyNotify */		DESTROY,
   /* UnmapNotify */		UNMAP,
   /* MapNotify */		MAP,
   /* MapRequest */		MAPREQ,
   /* ReparentNotify */		REPARENT,
   /* ConfigureNotify */	CONFIG,
   /* ConfigureRequest */	CONFIGREQ,
   /* GravityNotify */		GRAVITY,
   /* ResizeRequest */		RESIZEREQ,
   /* CirculateNotify */	CIRC,
   /* CirculateRequest */	0,
   /* PropertyNotify */		PROP,
   /* SelectionClear */		0,
   /* SelectionRequest */	0,
   /* SelectionNotify */	0,
   /* ColormapNotify */		COLORMAP,
   /* ClientMessage */		0,
   /* MappingNotify */		0,
   /* VirtualEvent */		VIRTUAL,
   /* Activate */		ACTIVATE,
   /* Deactivate */		ACTIVATE,
   /* MouseWheel */		KEY
};

/*
 * The following table is used to map between the location where an generated
 * event should be queued and the string used to specify the location.
 */

static const TkStateMap queuePosition[] = {
    {-1,		"now"},
    {TCL_QUEUE_HEAD,	"head"},
    {TCL_QUEUE_MARK,	"mark"},
    {TCL_QUEUE_TAIL,	"tail"},
    {-2,		NULL}
};

/*
 * The following tables are used as a two-way map between X's internal numeric
 * values for fields in an XEvent and the strings used in Tcl. The tables are
 * used both when constructing an XEvent from user input and when providing
 * data from an XEvent to the user.
 */

static const TkStateMap notifyMode[] = {
    {NotifyNormal,		"NotifyNormal"},
    {NotifyGrab,		"NotifyGrab"},
    {NotifyUngrab,		"NotifyUngrab"},
    {NotifyWhileGrabbed,	"NotifyWhileGrabbed"},
    {-1, NULL}
};

static const TkStateMap notifyDetail[] = {
    {NotifyAncestor,		"NotifyAncestor"},
    {NotifyVirtual,		"NotifyVirtual"},
    {NotifyInferior,		"NotifyInferior"},
    {NotifyNonlinear,		"NotifyNonlinear"},
    {NotifyNonlinearVirtual,	"NotifyNonlinearVirtual"},
    {NotifyPointer,		"NotifyPointer"},
    {NotifyPointerRoot,		"NotifyPointerRoot"},
    {NotifyDetailNone,		"NotifyDetailNone"},
    {-1, NULL}
};

static const TkStateMap circPlace[] = {
    {PlaceOnTop,	"PlaceOnTop"},
    {PlaceOnBottom,	"PlaceOnBottom"},
    {-1, NULL}
};

static const TkStateMap visNotify[] = {
    {VisibilityUnobscured,	  "VisibilityUnobscured"},
    {VisibilityPartiallyObscured, "VisibilityPartiallyObscured"},
    {VisibilityFullyObscured,	  "VisibilityFullyObscured"},
    {-1, NULL}
};

static const TkStateMap configureRequestDetail[] = {
    {None,	"None"},
    {Above,	"Above"},
    {Below,	"Below"},
    {BottomIf,	"BottomIf"},
    {TopIf,	"TopIf"},
    {Opposite,	"Opposite"},
    {-1, NULL}
};

static const TkStateMap propNotify[] = {
    {PropertyNewValue,	"NewValue"},
    {PropertyDelete,	"Delete"},
    {-1, NULL}
};

DEBUG(static int countTableItems = 0;)
DEBUG(static int countEntryItems = 0;)
DEBUG(static int countListItems = 0;)
DEBUG(static int countBindItems = 0;)
DEBUG(static int countSeqItems = 0;)

/*
 * Prototypes for local functions defined in this file:
 */

static void		ChangeScreen(Tcl_Interp *interp, char *dispName, int screenIndex);
static int		CreateVirtualEvent(Tcl_Interp *interp, VirtualEventTable *vetPtr,
			    char *virtString, const char *eventString);
static int		DeleteVirtualEvent(Tcl_Interp *interp, VirtualEventTable *vetPtr,
			    char *virtString, const char *eventString);
static void		DeleteVirtualEventTable(VirtualEventTable *vetPtr);
static void		ExpandPercents(TkWindow *winPtr, const char *before, Event *eventPtr,
			    unsigned scriptCount, Tcl_DString *dsPtr);
static PatSeq *		FindSequence(Tcl_Interp *interp, LookupTables *lookupTables,
			    ClientData object, const char *eventString, int create,
			    int allowVirtual, EventMask *maskPtr);
static void		GetAllVirtualEvents(Tcl_Interp *interp, VirtualEventTable *vetPtr);
static const char *	GetField(const char *p, char *copy, unsigned size);
static Tcl_Obj *	GetPatternObj(const PatSeq *psPtr);
static int		GetVirtualEvent(Tcl_Interp *interp, VirtualEventTable *vetPtr,
			    Tcl_Obj *virtName);
static Tk_Uid		GetVirtualEventUid(Tcl_Interp *interp, char *virtString);
static int		HandleEventGenerate(Tcl_Interp *interp, Tk_Window main,
			    int objc, Tcl_Obj *const objv[]);
static void		InitVirtualEventTable(VirtualEventTable *vetPtr);
static PatSeq *		MatchPatterns(TkDisplay *dispPtr, Tk_BindingTable bindPtr, PSList *psList,
			    PSList *psSuccList, unsigned patIndex, const Event *eventPtr,
			    ClientData object, PatSeq **physPtrPtr);
static int		NameToWindow(Tcl_Interp *interp, Tk_Window main,
			    Tcl_Obj *objPtr, Tk_Window *tkwinPtr);
static unsigned		ParseEventDescription(Tcl_Interp *interp, const char **eventStringPtr,
			    TkPattern *patPtr, EventMask *eventMaskPtr);
static void		DoWarp(ClientData clientData);
static PSList *		GetLookupForEvent(LookupTables* lookupPtr, const Event *eventPtr,
			    Tcl_Obj *object, int onlyConsiderDetailedEvents);
static void		ClearLookupTable(LookupTables *lookupTables, ClientData object);
static void		ClearPromotionLists(Tk_BindingTable bindPtr, ClientData object);
static PSEntry *	MakeListEntry(PSList *pool, PatSeq *psPtr, int needModMasks);
static void		RemovePatSeqFromLookup(LookupTables *lookupTables, PatSeq *psPtr);
static void		RemovePatSeqFromPromotionLists(Tk_BindingTable bindPtr, PatSeq *psPtr);
static PatSeq *		DeletePatSeq(PatSeq *psPtr);
static void		InsertPatSeq(LookupTables *lookupTables, PatSeq *psPtr);
#if SUPPORT_DEBUGGING
void			TkpDumpPS(const PatSeq *psPtr);
void			TkpDumpPSList(const PSList *psList);
#endif

/*
 * Some useful helper functions.
 */
#ifdef SUPPORT_DEBUGGING
static int BindCount = 0;
#endif

static unsigned Max(unsigned a, unsigned b) { return a < b ? b : a; }
static int Abs(int n) { return n < 0 ? -n : n; }
static int IsOdd(int n) { return n & 1; }

static int TestNearbyTime(int lhs, int rhs) { return Abs(lhs - rhs) <= NEARBY_MS; }
static int TestNearbyCoords(int lhs, int rhs) { return Abs(lhs - rhs) <= NEARBY_PIXELS; }

static int
IsSubsetOf(
    ModMask lhsMask,	/* this is a subset */
    ModMask rhsMask)	/* of this bit field? */
{
    return (lhsMask & rhsMask) == lhsMask;
}

static const char*
SkipSpaces(
    const char* s)
{
    assert(s);
    while (isspace(UCHAR(*s)))
	++s;
    return s;
}

static const char*
SkipFieldDelims(
    const char* s)
{
    assert(s);
    while (*s == '-' || isspace(UCHAR(*s))) {
	++s;
    }
    return s;
}

static unsigned
GetButtonNumber(
    const char *field)
{
    assert(field);
    return (field[0] >= '1' && field[0] <= '5' && field[1] == '\0') ? field[0] - '0' : 0;
}

static Time
CurrentTimeInMilliSecs()
{
    Tcl_Time now;
    Tcl_GetTime(&now);
    return ((Time) now.sec)*1000 + ((Time) now.usec)/1000;
}

static Info
GetInfo(
    const PatSeq *psPtr,
    unsigned index)
{
    assert(psPtr);
    assert(index < psPtr->numPats);

    return psPtr->pats[index].info;
}

static unsigned
GetCount(
    const PatSeq *psPtr,
    unsigned index)
{
    assert(psPtr);
    assert(index < psPtr->numPats);

    return psPtr->pats[index].count;
}

static int
CountSpecialized(
    const PatSeq *fstMatchPtr,
    const PatSeq *sndMatchPtr)
{
    int fstCount = 0;
    int sndCount = 0;
    unsigned i;

    assert(fstMatchPtr);
    assert(sndMatchPtr);

    for (i = 0; i < fstMatchPtr->numPats; ++i) {
	if (GetInfo(fstMatchPtr, i)) { fstCount += GetCount(fstMatchPtr, i); }
    }
    for (i = 0; i < sndMatchPtr->numPats; ++i) {
	if (GetInfo(sndMatchPtr, i)) { sndCount += GetCount(sndMatchPtr, i); }
    }

    return sndCount - fstCount;
}

static int
MatchEventNearby(
    const XEvent *lhs,	/* previous button event */
    const XEvent *rhs)	/* current button event */
{
    assert(lhs);
    assert(rhs);
    assert(lhs->type == ButtonPress || lhs->type == ButtonRelease);
    assert(lhs->type == rhs->type);

    /* assert: lhs->xbutton.time <= rhs->xbutton.time */

    return TestNearbyTime(rhs->xbutton.time, lhs->xbutton.time)
	    && TestNearbyCoords(rhs->xbutton.x_root, lhs->xbutton.x_root)
	    && TestNearbyCoords(rhs->xbutton.y_root, lhs->xbutton.y_root);
}

static int
MatchEventRepeat(
    const XEvent *lhs,	/* previous key event */
    const XEvent *rhs)	/* current key event */
{
    assert(lhs);
    assert(rhs);
    assert(lhs->type == KeyPress || lhs->type == KeyRelease);
    assert(lhs->type == rhs->type);

    /* assert: lhs->xkey.time <= rhs->xkey.time */
    return TestNearbyTime(rhs->xkey.time, lhs->xkey.time);
}

static void
FreePatSeq(
    PatSeq *psPtr)
{
    assert(psPtr);
    assert(!psPtr->owned);
    DEBUG(MARK_PSENTRY(psPtr);)
    ckfree(psPtr->script);
    if (!psPtr->object) {
	VirtOwners_Free(&psPtr->ptr.owners);
    }
    ckfree(psPtr);
    DEBUG(countSeqItems -= 1;)
}

static void
RemoveListEntry(
    PSList *pool,
    PSEntry *psEntry)
{
    assert(pool);
    assert(psEntry);

    if (PSModMaskArr_Capacity(psEntry->lastModMaskArr) > MAX_MOD_MASK_ARR_SIZE) {
	PSModMaskArr_Free(&psEntry->lastModMaskArr);
    }
    PSList_Remove(psEntry);
    PSList_Append(pool, psEntry);
}

static void
ClearList(
    PSList *psList,
    PSList *pool,
    ClientData object)
{
    assert(psList);
    assert(pool);

    if (object) {
	PSEntry *psEntry;
	PSEntry *psNext;

	for (psEntry = PSList_First(psList); psEntry; psEntry = psNext) {
	    psNext = PSList_Next(psEntry);
	    if (psEntry->psPtr->object == object) {
		RemoveListEntry(pool, psEntry);
	    }
	}
    } else {
	PSList_Move(pool, psList);
    }
}

static PSEntry *
FreePatSeqEntry(
    PSList *pool,
    PSEntry *entry)
{
    PSEntry *next = PSList_Next(entry);
    PSModMaskArr_Free(&entry->lastModMaskArr);
    ckfree(entry);
    return next;
}

static unsigned
ResolveModifiers(
    TkDisplay *dispPtr,
    unsigned modMask)
{
    assert(dispPtr);

    if (dispPtr->metaModMask) {
	if (modMask & META_MASK) {
	    modMask &= ~(ModMask)META_MASK;
	    modMask |= dispPtr->metaModMask;
	}
    }
    if (dispPtr->altModMask) {
	if (modMask & ALT_MASK) {
	    modMask &= ~(ModMask)ALT_MASK;
	    modMask |= dispPtr->altModMask;
	}
    }

    return modMask;
}

static int
ButtonNumberFromState(
    unsigned state)
{
    if (!(state & ALL_BUTTONS)) { return 0; }
    if (state & Button1Mask) { return 1; }
    if (state & Button2Mask) { return 2; }
    if (state & Button3Mask) { return 3; }
    if (state & Button4Mask) { return 4; }
    return 5;
}

static void
SetupPatternKey(
    PatternTableKey *key,
    const PatSeq *psPtr)
{
    const TkPattern *patPtr;

    assert(key);
    assert(psPtr);

    /* otherwise on some systems the key contains uninitialized bytes */
    memset(key, 0, sizeof(PatternTableKey));

    patPtr = psPtr->pats;
    assert(!patPtr->info || !patPtr->name);

    key->object = psPtr->object;
    key->type = patPtr->eventType;
    if (patPtr->info) {
	key->detail.info = patPtr->info;
    } else {
	key->detail.name = patPtr->name;
    }
}

/*
 *--------------------------------------------------------------
 *
 * MakeListEntry --
 *
 *	Makes new entry item for lookup table. We are using a
 *	pool of items, this avoids superfluous memory allocation/
 *	deallocation.
 *
 * Results:
 *	New entry item.
 *
 * Side effects:
 *	Memory allocated.
 *
 *--------------------------------------------------------------
 */

static PSEntry *
MakeListEntry(
    PSList *pool,
    PatSeq *psPtr,
    int needModMasks)
{
    PSEntry *newEntry = NULL;

    assert(pool);
    assert(psPtr);
    assert(psPtr->numPats > 0);
    assert(TEST_PSENTRY(psPtr));

    if (PSList_IsEmpty(pool)) {
	newEntry = ckalloc(sizeof(PSEntry));
	newEntry->lastModMaskArr = NULL;
	DEBUG(countEntryItems += 1;)
    } else {
	newEntry = PSList_First(pool);
	PSList_RemoveHead(pool);
    }

    if (!needModMasks) {
	PSModMaskArr_SetSize(newEntry->lastModMaskArr, 0);
    } else {
	if (PSModMaskArr_Capacity(newEntry->lastModMaskArr) < psPtr->numPats - 1) {
	    PSModMaskArr_Resize(&newEntry->lastModMaskArr, psPtr->numPats - 1);
	}
	PSModMaskArr_SetSize(newEntry->lastModMaskArr, psPtr->numPats - 1);
    }

    newEntry->psPtr = psPtr;
    newEntry->window = None;
    newEntry->expired = 0;
    newEntry->keepIt = 1;
    newEntry->count = 1;
    DEBUG(psPtr->owned = 0;)

    return newEntry;
}

/*
 *--------------------------------------------------------------
 *
 * GetLookupForEvent --
 *
 *	Get specific pattern sequence table for given event.
 *
 * Results:
 *	Specific pattern sequence table for given event.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static PSList *
GetLookupForEvent(
    LookupTables* lookupTables,
    const Event *eventPtr,
    Tcl_Obj *object,
    int onlyConsiderDetailedEvents)
{
    PatternTableKey key;
    Tcl_HashEntry *hPtr;

    assert(lookupTables);
    assert(eventPtr);

    /* otherwise on some systems the key contains uninitialized bytes */
    memset(&key, 0, sizeof(PatternTableKey));

    if (onlyConsiderDetailedEvents) {
	switch (eventPtr->xev.type) {
	case ButtonPress:   /* fallthru */
	case ButtonRelease: key.detail.info = eventPtr->xev.xbutton.button; break;
	case MotionNotify:  key.detail.info = ButtonNumberFromState(eventPtr->xev.xmotion.state); break;
	case KeyPress:      /* fallthru */
	case KeyRelease:    key.detail.info = eventPtr->detail.info; break;
	case VirtualEvent:  key.detail.name = eventPtr->detail.name; break;
	}
	if (!key.detail.name) {
	    assert(!key.detail.info);
	    return NULL;
	}
    }

    key.object = object;
    key.type = eventPtr->xev.type;
    hPtr = Tcl_FindHashEntry(&lookupTables->listTable, (char *) &key);
    return hPtr ? Tcl_GetHashValue(hPtr) : NULL;
}

/*
 *--------------------------------------------------------------
 *
 * ClearLookupTable --
 *
 *	Clear lookup table in given binding table, but only those
 *	bindings associated to given object. If object is NULL
 *	then remove all entries.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
static void
ClearLookupTable(
    LookupTables *lookupTables,
    ClientData object)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    Tcl_HashEntry *nextPtr;
    PSList *pool = &lookupTables->entryPool;

    assert(lookupTables);

    for (hPtr = Tcl_FirstHashEntry(&lookupTables->listTable, &search); hPtr; hPtr = nextPtr) {
	PSList *psList;

	nextPtr = Tcl_NextHashEntry(&search);

	if (object) {
	    const PatternTableKey *key = Tcl_GetHashKey(&lookupTables->listTable, hPtr);
	    if (key->object != object) {
		continue;
	    }
	    Tcl_DeleteHashEntry(hPtr);
	}

	psList = Tcl_GetHashValue(hPtr);
	PSList_Move(pool, psList);
	ckfree(psList);
	DEBUG(countListItems -= 1;)
    }
}

/*
 *--------------------------------------------------------------
 *
 * ClearPromotionLists --
 *
 *	Clear all the lists holding the promoted pattern
 *	sequences, which belongs to given object. If object
 *	is NULL then remove all patterns.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
ClearPromotionLists(
    Tk_BindingTable bindPtr,
    ClientData object)
{
    unsigned newArraySize = 0;
    unsigned i;

    assert(bindPtr);

    for (i = 0; i < PromArr_Size(bindPtr->promArr); ++i) {
	PSList *psList = PromArr_Get(bindPtr->promArr, i);
	ClearList(psList, &bindPtr->lookupTables.entryPool, object);
	if (!PSList_IsEmpty(psList)) {
	    newArraySize = i + 1;
	}
    }

    PromArr_SetSize(bindPtr->promArr, newArraySize);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkBindInit --
 *
 *	This function is called when an application is created. It initializes
 *	all the structures used by bindings and virtual events. It must be
 *	called before any other functions in this file are called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

/*
 * Windoze compiler does not allow the definition of these static variables inside a function,
 * otherwise this should belong to function TkBindInit().
 */
TCL_DECLARE_MUTEX(bindMutex);
static int initialized = 0;

void
TkBindInit(
    TkMainInfo *mainPtr)	/* The newly created application. */
{
    BindInfo *bindInfoPtr;

    assert(mainPtr);

    /* otherwise virtual events can't be supported */
    assert(sizeof(XEvent) >= sizeof(XVirtualEvent));

    /* type of TkPattern.info is well defined? */
    assert(sizeof(Info) >= sizeof(KeySym));
    assert(sizeof(Info) >= sizeof(unsigned));

    /* ensure that our matching algorithm is working (when testing detail) */
    assert(sizeof(Detail) == sizeof(Tk_Uid));

    /* test that constant NO_NUMBER is indeed out of integer range */
    assert(sizeof(NO_NUMBER) > sizeof(int));
    assert(((int) NO_NUMBER) == 0 && NO_NUMBER != 0);

    /* test expected indices of Button1..Button5, otherwise our button handling is not working */
    assert(Button1 == 1 && Button2 == 2 && Button3 == 3 && Button4 == 4 && Button5 == 5);
    assert(Button2Mask == (Button1Mask << 1));
    assert(Button3Mask == (Button1Mask << 2));
    assert(Button4Mask == (Button1Mask << 3));
    assert(Button5Mask == (Button1Mask << 4));

    /* test expected values of button motion masks, otherwise our button handling is not working */
    assert(Button1MotionMask == Button1Mask);
    assert(Button2MotionMask == Button2Mask);
    assert(Button3MotionMask == Button3Mask);
    assert(Button4MotionMask == Button4Mask);
    assert(Button5MotionMask == Button5Mask);

    /* because we expect zero if keySym is empty */
    assert(NoSymbol == 0L);

    /* this must be a union, not a struct, otherwise comparison with NULL will not work */
    assert(Tk_Offset(Detail, name) == Tk_Offset(Detail, info));

    /* we use some constraints about X*Event */
    assert(Tk_Offset(XButtonEvent, time) == Tk_Offset(XMotionEvent, time));
    assert(Tk_Offset(XButtonEvent, x_root) == Tk_Offset(XMotionEvent, x_root));
    assert(Tk_Offset(XButtonEvent, y_root) == Tk_Offset(XMotionEvent, y_root));
    assert(Tk_Offset(XCreateWindowEvent, border_width) == Tk_Offset(XConfigureEvent, border_width));
    assert(Tk_Offset(XCreateWindowEvent, width) == Tk_Offset(XConfigureEvent, width));
    assert(Tk_Offset(XCreateWindowEvent, window) == Tk_Offset(XCirculateRequestEvent, window));
    assert(Tk_Offset(XCreateWindowEvent, window) == Tk_Offset(XConfigureEvent, window));
    assert(Tk_Offset(XCreateWindowEvent, window) == Tk_Offset(XGravityEvent, window));
    assert(Tk_Offset(XCreateWindowEvent, window) == Tk_Offset(XMapEvent, window));
    assert(Tk_Offset(XCreateWindowEvent, window) == Tk_Offset(XReparentEvent, window));
    assert(Tk_Offset(XCreateWindowEvent, window) == Tk_Offset(XUnmapEvent, window));
    assert(Tk_Offset(XCreateWindowEvent, x) == Tk_Offset(XConfigureEvent, x));
    assert(Tk_Offset(XCreateWindowEvent, x) == Tk_Offset(XGravityEvent, x));
    assert(Tk_Offset(XCreateWindowEvent, y) == Tk_Offset(XConfigureEvent, y));
    assert(Tk_Offset(XCreateWindowEvent, y) == Tk_Offset(XGravityEvent, y));
    assert(Tk_Offset(XCrossingEvent, time) == Tk_Offset(XEnterWindowEvent, time));
    assert(Tk_Offset(XCrossingEvent, time) == Tk_Offset(XLeaveWindowEvent, time));
    assert(Tk_Offset(XCrossingEvent, time) == Tk_Offset(XKeyEvent, time));
    assert(Tk_Offset(XKeyEvent, root) == Tk_Offset(XButtonEvent, root));
    assert(Tk_Offset(XKeyEvent, root) == Tk_Offset(XCrossingEvent, root));
    assert(Tk_Offset(XKeyEvent, root) == Tk_Offset(XMotionEvent, root));
    assert(Tk_Offset(XKeyEvent, state) == Tk_Offset(XButtonEvent, state));
    assert(Tk_Offset(XKeyEvent, state) == Tk_Offset(XMotionEvent, state));
    assert(Tk_Offset(XKeyEvent, subwindow) == Tk_Offset(XButtonEvent, subwindow));
    assert(Tk_Offset(XKeyEvent, subwindow) == Tk_Offset(XCrossingEvent, subwindow));
    assert(Tk_Offset(XKeyEvent, subwindow) == Tk_Offset(XMotionEvent, subwindow));
    assert(Tk_Offset(XKeyEvent, time) == Tk_Offset(XButtonEvent, time));
    assert(Tk_Offset(XKeyEvent, time) == Tk_Offset(XMotionEvent, time));
    assert(Tk_Offset(XKeyEvent, x) == Tk_Offset(XButtonEvent, x));
    assert(Tk_Offset(XKeyEvent, x) == Tk_Offset(XCrossingEvent, x));
    assert(Tk_Offset(XKeyEvent, x) == Tk_Offset(XMotionEvent, x));
    assert(Tk_Offset(XKeyEvent, x_root) == Tk_Offset(XButtonEvent, x_root));
    assert(Tk_Offset(XKeyEvent, x_root) == Tk_Offset(XCrossingEvent, x_root));
    assert(Tk_Offset(XKeyEvent, x_root) == Tk_Offset(XMotionEvent, x_root));
    assert(Tk_Offset(XKeyEvent, y) == Tk_Offset(XButtonEvent, y));
    assert(Tk_Offset(XKeyEvent, y) == Tk_Offset(XCrossingEvent, y));
    assert(Tk_Offset(XKeyEvent, y) == Tk_Offset(XMotionEvent, y));
    assert(Tk_Offset(XKeyEvent, y_root) == Tk_Offset(XButtonEvent, y_root));
    assert(Tk_Offset(XKeyEvent, y_root) == Tk_Offset(XCrossingEvent, y_root));
    assert(Tk_Offset(XKeyEvent, y_root) == Tk_Offset(XMotionEvent, y_root));

    /*
     * Initialize the static data structures used by the binding package. They
     * are only initialized once, no matter how many interps are created.
     */

    if (!initialized) {
	Tcl_MutexLock(&bindMutex);
	if (!initialized) {
	    Tcl_HashEntry *hPtr;
	    const ModInfo *modPtr;
	    const EventInfo *eiPtr;
	    int newEntry;
	    unsigned i;
#ifdef REDO_KEYSYM_LOOKUP
	    const KeySymInfo *kPtr;

	    Tcl_InitHashTable(&keySymTable, TCL_STRING_KEYS);
	    Tcl_InitHashTable(&nameTable, TCL_ONE_WORD_KEYS);
	    for (kPtr = keyArray; kPtr->name; ++kPtr) {
		hPtr = Tcl_CreateHashEntry(&keySymTable, kPtr->name, &newEntry);
		Tcl_SetHashValue(hPtr, kPtr->value);
		hPtr = Tcl_CreateHashEntry(&nameTable, (char *) kPtr->value, &newEntry);
		if (newEntry) {
		    Tcl_SetHashValue(hPtr, kPtr->name);
		}
	    }
#endif /* REDO_KEYSYM_LOOKUP */

	    for (i = 0; i < SIZE_OF_ARRAY(eventArrayIndex); ++i) {
		eventArrayIndex[i] = -1;
	    }
	    for (i = 0; i < SIZE_OF_ARRAY(eventArray); ++i) {
		unsigned type = eventArray[i].type;
		assert(type < TK_LASTEVENT);
		assert(type > 0 || i == SIZE_OF_ARRAY(eventArray) - 1);
		if (type > 0 && eventArrayIndex[type] == -1) {
		    eventArrayIndex[type] = i;
		}
	    }

	    Tcl_InitHashTable(&modTable, TCL_STRING_KEYS);
	    for (modPtr = modArray; modPtr->name; ++modPtr) {
		hPtr = Tcl_CreateHashEntry(&modTable, modPtr->name, &newEntry);
		Tcl_SetHashValue(hPtr, modPtr);
	    }

	    Tcl_InitHashTable(&eventTable, TCL_STRING_KEYS);
	    for (eiPtr = eventArray; eiPtr->name; ++eiPtr) {
		hPtr = Tcl_CreateHashEntry(&eventTable, eiPtr->name, &newEntry);
		Tcl_SetHashValue(hPtr, eiPtr);
	    }

	    initialized = 1;
	}
	Tcl_MutexUnlock(&bindMutex);
    }

    mainPtr->bindingTable = Tk_CreateBindingTable(mainPtr->interp);

    bindInfoPtr = ckalloc(sizeof(BindInfo));
    InitVirtualEventTable(&bindInfoPtr->virtualEventTable);
    bindInfoPtr->screenInfo.curDispPtr = NULL;
    bindInfoPtr->screenInfo.curScreenIndex = -1;
    bindInfoPtr->screenInfo.bindingDepth = 0;
    bindInfoPtr->deleted = 0;
    bindInfoPtr->lastCurrentTime = CurrentTimeInMilliSecs();
    bindInfoPtr->lastEventTime = 0;
    mainPtr->bindInfo = bindInfoPtr;
    DEBUG(countBindItems += 1;)

    TkpInitializeMenuBindings(mainPtr->interp, mainPtr->bindingTable);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkBindFree --
 *
 *	This function is called when an application is deleted. It deletes all
 *	the structures used by bindings and virtual events.
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
TkBindFree(
    TkMainInfo *mainPtr)	/* The newly created application. */
{
    BindInfo *bindInfoPtr;

    assert(mainPtr);

    Tk_DeleteBindingTable(mainPtr->bindingTable);
    mainPtr->bindingTable = NULL;
    bindInfoPtr = mainPtr->bindInfo;
    DeleteVirtualEventTable(&bindInfoPtr->virtualEventTable);
    bindInfoPtr->deleted = 1;
    Tcl_EventuallyFree(bindInfoPtr, TCL_DYNAMIC);
    mainPtr->bindInfo = NULL;

    DEBUG(countBindItems -= 1;)
    assert(countBindItems > 0 || countTableItems == 0);
    assert(countBindItems > 0 || countEntryItems == 0);
    assert(countBindItems > 0 || countListItems == 0);
    assert(countBindItems > 0 || countSeqItems == 0);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CreateBindingTable --
 *
 *	Set up a new domain in which event bindings may be created.
 *
 * Results:
 *	The return value is a token for the new table, which must be passed to
 *	functions like Tk_CreateBinding.
 *
 * Side effects:
 *	Memory is allocated for the new table.
 *
 *--------------------------------------------------------------
 */

Tk_BindingTable
Tk_CreateBindingTable(
    Tcl_Interp *interp)		/* Interpreter to associate with the binding table: commands are
    				 * executed in this interpreter. */
{
    BindingTable *bindPtr = ckalloc(sizeof(BindingTable));
    unsigned i;

    assert(interp);
    DEBUG(countTableItems += 1;)

    /*
     * Create and initialize a new binding table.
     */

    memset(bindPtr, 0, sizeof(BindingTable));
    for (i = 0; i < SIZE_OF_ARRAY(bindPtr->eventInfo); ++i) {
	bindPtr->eventInfo[i].xev.type = -1;
    }
    bindPtr->curEvent = bindPtr->eventInfo; /* do not assign NULL */
    bindPtr->lookupTables.number = 0;
    PromArr_ResizeAndClear(&bindPtr->promArr, 2);
    Tcl_InitHashTable(&bindPtr->lookupTables.listTable, sizeof(PatternTableKey)/sizeof(int));
    Tcl_InitHashTable(&bindPtr->lookupTables.patternTable, sizeof(PatternTableKey)/sizeof(int));
    Tcl_InitHashTable(&bindPtr->objectTable, TCL_ONE_WORD_KEYS);
    bindPtr->interp = interp;
    return bindPtr;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_DeleteBindingTable --
 *
 *	Destroy a binding table and free up all its memory. The caller should
 *	not use bindingTable again after this function returns.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *--------------------------------------------------------------
 */

void
Tk_DeleteBindingTable(
    Tk_BindingTable bindPtr)	/* Token for the binding table to destroy. */
{
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    assert(bindPtr);

    /*
     * Find and delete all of the patterns associated with the binding table.
     */

    hPtr = Tcl_FirstHashEntry(&bindPtr->lookupTables.patternTable, &search);
    for ( ; hPtr; hPtr = Tcl_NextHashEntry(&search)) {
	PatSeq *nextPtr;
	PatSeq *psPtr;

	for (psPtr = Tcl_GetHashValue(hPtr); psPtr; psPtr = nextPtr) {
	    assert(TEST_PSENTRY(psPtr));
	    nextPtr = psPtr->nextSeqPtr;
	    FreePatSeq(psPtr);
	}
    }

    /*
     * Don't forget to release lookup elements.
     */

    ClearLookupTable(&bindPtr->lookupTables, NULL);
    ClearPromotionLists(bindPtr, NULL);
    PromArr_Free(&bindPtr->promArr);
    DEBUG(countEntryItems -= PSList_Size(&bindPtr->lookupTables.entryPool);)
    PSList_Traverse(&bindPtr->lookupTables.entryPool, FreePatSeqEntry);

    /*
     * Clean up the rest of the information associated with the binding table.
     */

    Tcl_DeleteHashTable(&bindPtr->lookupTables.patternTable);
    Tcl_DeleteHashTable(&bindPtr->lookupTables.listTable);
    Tcl_DeleteHashTable(&bindPtr->objectTable);

    ckfree(bindPtr);
    DEBUG(countTableItems -= 1;)
}

/*
 *--------------------------------------------------------------
 *
 * InsertPatSeq --
 *
 *	Insert given pattern sequence into lookup table for fast
 *	access.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated.
 *
 *--------------------------------------------------------------
 */

static void
InsertPatSeq(
    LookupTables *lookupTables,
    PatSeq *psPtr)
{
    assert(lookupTables);
    assert(psPtr);
    assert(TEST_PSENTRY(psPtr));
    assert(psPtr->numPats >= 1u);

    if (!(psPtr->added)) {
	PatternTableKey key;
	Tcl_HashEntry *hPtr;
	int isNew;
	PSList *psList;
	PSEntry *psEntry;

	SetupPatternKey(&key, psPtr);
	hPtr = Tcl_CreateHashEntry(&lookupTables->listTable, (char *) &key, &isNew);

	if (isNew) {
	    psList = ckalloc(sizeof(PSList));
	    PSList_Init(psList);
	    Tcl_SetHashValue(hPtr, psList);
	    DEBUG(countListItems += 1;)
	} else {
	    psList = Tcl_GetHashValue(hPtr);
	}

	psEntry = MakeListEntry(&lookupTables->entryPool, psPtr, 0);
	PSList_Append(psList, psEntry);
	psPtr->added = 1;
    }
}
/*
 *--------------------------------------------------------------
 *
 * Tk_CreateBinding --
 *
 *	Add a binding to a binding table, so that future calls to Tk_BindEvent
 *	may execute the command in the binding.
 *
 * Results:
 *	The return value is 0 if an error occurred while setting up the
 *	binding. In this case, an error message will be left in the interp's
 *	result. If all went well then the return value is a mask of the event
 *	types that must be made available to Tk_BindEvent in order to properly
 *	detect when this binding triggers. This value can be used to determine
 *	what events to select for in a window, for example.
 *
 * Side effects:
 *	An existing binding on the same event sequence may be replaced. The
 *	new binding may cause future calls to Tk_BindEvent to behave
 *	differently than they did previously.
 *
 *--------------------------------------------------------------
 */

unsigned long
Tk_CreateBinding(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_BindingTable bindPtr,	/* Table in which to create binding. */
    ClientData object,		/* Token for object with which binding is associated. */
    const char *eventString,	/* String describing event sequence that triggers binding. */
    const char *script,		/* Contains Tcl script to execute when binding triggers. */
    int append)		/* 0 means replace any existing binding for eventString;
    				 * 1 means append to that binding. If the existing binding is
				 * for a callback function and not a Tcl command string, the
				 * existing binding will always be replaced. */
{
    PatSeq *psPtr;
    EventMask eventMask;
    char *oldStr;
    char *newStr;

    assert(bindPtr);
    assert(object);
    assert(eventString);
    assert(script);

    psPtr = FindSequence(interp, &bindPtr->lookupTables, object, eventString,
	    !!*script, 1, &eventMask);

    if (!*script) {
	assert(!psPtr || psPtr->added);
	/* Silently ignore empty scripts -- see SF#3006842 */
	return eventMask;
    }
    if (!psPtr) {
	return 0;
    }
    assert(TEST_PSENTRY(psPtr));

    if (psPtr->numPats > PromArr_Capacity(bindPtr->promArr)) {
	/*
	 * We have to increase the size of array containing the lists of promoted sequences.
	 * Normally the maximal size is 1, only in very seldom cases a bigger size is needed.
	 * Note that for technical reasons the capacity should be one higher than the expected
	 * maximal size.
	 */
	PromArr_ResizeAndClear(&bindPtr->promArr, psPtr->numPats);
    }

    if (!psPtr->script) {
	Tcl_HashEntry *hPtr;
	int isNew;

	/*
	 * This pattern sequence was just created. Link the pattern into the
	 * list associated with the object, so that if the object goes away,
	 * these bindings will all automatically be deleted.
	 */

	hPtr = Tcl_CreateHashEntry(&bindPtr->objectTable, (char *) object, &isNew);
	psPtr->ptr.nextObj = isNew ? NULL : Tcl_GetHashValue(hPtr);
	Tcl_SetHashValue(hPtr, psPtr);
	InsertPatSeq(&bindPtr->lookupTables, psPtr);
    }

    oldStr = psPtr->script;
    if (append && oldStr) {
	size_t length1 = strlen(oldStr);
	size_t length2 = strlen(script);

	newStr = ckalloc(length1 + length2 + 2);
	memcpy(newStr, oldStr, length1);
	newStr[length1] = '\n';
	memcpy(newStr + length1 + 1, script, length2 + 1);
    } else {
	size_t length = strlen(script);

	newStr = ckalloc(length + 1);
	memcpy(newStr, script, length + 1);
    }
    ckfree(oldStr);
    psPtr->script = newStr;
    return eventMask;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_DeleteBinding --
 *
 *	Remove an event binding from a binding table.
 *
 * Results:
 *	The result is a standard Tcl return value. If an error occurs then the
 *	interp's result will contain an error message.
 *
 * Side effects:
 *	The binding given by object and eventString is removed from
 *	bindingTable.
 *
 *--------------------------------------------------------------
 */

int
Tk_DeleteBinding(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_BindingTable bindPtr,	/* Table in which to delete binding. */
    ClientData object,		/* Token for object with which binding is associated. */
    const char *eventString)	/* String describing event sequence that triggers binding. */
{
    PatSeq *psPtr;

    assert(bindPtr);
    assert(object);
    assert(eventString);

    psPtr = FindSequence(interp, &bindPtr->lookupTables, object, eventString, 0, 1, NULL);
    if (!psPtr) {
	Tcl_ResetResult(interp);
    } else {
	Tcl_HashEntry *hPtr;
	PatSeq *prevPtr;

	assert(TEST_PSENTRY(psPtr));

	/*
	 * Unlink the binding from the list for its object.
	 */

	if (!(hPtr = Tcl_FindHashEntry(&bindPtr->objectTable, (char *) object))) {
	    Tcl_Panic("Tk_DeleteBinding couldn't find object table entry");
	}
	prevPtr = Tcl_GetHashValue(hPtr);
	if (prevPtr == psPtr) {
	    Tcl_SetHashValue(hPtr, psPtr->ptr.nextObj);
	} else {
	    for ( ; ; prevPtr = prevPtr->ptr.nextObj) {
		if (!prevPtr) {
		    Tcl_Panic("Tk_DeleteBinding couldn't find on object list");
		}
		if (prevPtr->ptr.nextObj == psPtr) {
		    prevPtr->ptr.nextObj = psPtr->ptr.nextObj;
		    break;
		}
	    }
	}

	RemovePatSeqFromLookup(&bindPtr->lookupTables, psPtr);
	RemovePatSeqFromPromotionLists(bindPtr, psPtr);
	DeletePatSeq(psPtr);
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_GetBinding --
 *
 *	Return the script associated with a given event string.
 *
 * Results:
 *	The return value is a pointer to the script associated with
 *	eventString for object in the domain given by bindingTable. If there
 *	is no binding for eventString, or if eventString is improperly formed,
 *	then NULL is returned and an error message is left in the interp's
 *	result. The return value is semi-static: it will persist until the
 *	binding is changed or deleted.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

const char *
Tk_GetBinding(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_BindingTable bindPtr,	/* Table in which to look for binding. */
    ClientData object,		/* Token for object with which binding is associated. */
    const char *eventString)	/* String describing event sequence that triggers binding. */
{
    const PatSeq *psPtr;

    assert(bindPtr);
    assert(object);
    assert(eventString);

    psPtr = FindSequence(interp, &bindPtr->lookupTables, object, eventString, 0, 1, NULL);
    assert(!psPtr || TEST_PSENTRY(psPtr));
    return psPtr ? psPtr->script : NULL;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_GetAllBindings --
 *
 *	Return a list of event strings for all the bindings associated with a
 *	given object.
 *
 * Results:
 *	There is no return value. The interp's result is modified to hold a
 *	Tcl list with one entry for each binding associated with object in
 *	bindingTable. Each entry in the list contains the event string
 *	associated with one binding.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
Tk_GetAllBindings(
    Tcl_Interp *interp,		/* Interpreter returning result or error. */
    Tk_BindingTable bindPtr,	/* Table in which to look for bindings. */
    ClientData object)		/* Token for object. */
{
    Tcl_HashEntry *hPtr;

    assert(bindPtr);
    assert(object);

    if ((hPtr = Tcl_FindHashEntry(&bindPtr->objectTable, (char *) object))) {
	const PatSeq *psPtr;
	Tcl_Obj *resultObj = Tcl_NewObj();

	/*
	 * For each binding, output information about each of the patterns in its sequence.
	 */

	for (psPtr = Tcl_GetHashValue(hPtr); psPtr; psPtr = psPtr->ptr.nextObj) {
	    assert(TEST_PSENTRY(psPtr));
	    Tcl_ListObjAppendElement(NULL, resultObj, GetPatternObj(psPtr));
	}
	Tcl_SetObjResult(interp, resultObj);
    }
}

/*
 *--------------------------------------------------------------
 *
 * RemovePatSeqFromLookup --
 *
 *	Remove given pattern sequence from lookup tables. This
 *	can be required before deleting the pattern sequence.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
RemovePatSeqFromLookup(
    LookupTables *lookupTables,	/* Remove from this lookup tables. */
    PatSeq *psPtr)		/* Remove this pattern sequence. */
{
    PatternTableKey key;
    Tcl_HashEntry *hPtr;

    assert(lookupTables);
    assert(psPtr);

    SetupPatternKey(&key, psPtr);

    if ((hPtr = Tcl_FindHashEntry(&lookupTables->listTable, (char *) &key))) {
	PSList *psList = Tcl_GetHashValue(hPtr);
	PSEntry *psEntry;

	TK_DLIST_FOREACH(psEntry, psList) {
	    if (psEntry->psPtr == psPtr) {
		psPtr->added = 0;
		RemoveListEntry(&lookupTables->entryPool, psEntry);
		return;
	    }
	}
    }

    assert(!"couldn't find pattern sequence in lookup");
}

/*
 *--------------------------------------------------------------
 *
 * RemovePatSeqFromPromotionLists --
 *
 *	Remove given pattern sequence from promotion lists. This
 *	can be required before deleting the pattern sequence.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
RemovePatSeqFromPromotionLists(
    Tk_BindingTable bindPtr,	/* Table in which to look for bindings. */
    PatSeq *psPtr)		/* Remove this pattern sequence. */
{
    unsigned i;

    assert(bindPtr);
    assert(psPtr);

    for (i = 0; i < PromArr_Size(bindPtr->promArr); ++i) {
	PSList *psList = PromArr_Get(bindPtr->promArr, i);
	PSEntry *psEntry;

	TK_DLIST_FOREACH(psEntry, psList) {
	    if (psEntry->psPtr == psPtr) {
		RemoveListEntry(&bindPtr->lookupTables.entryPool, psEntry);
		break;
	    }
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * DeletePatSeq --
 *
 *	Delete given pattern sequence. Possibly it is required
 *	to invoke RemovePatSeqFromLookup(), and RemovePatSeqFromPromotionLists()
 *	before.
 *
 * Results:
 *	Pointer to succeeding pattern sequence.
 *
 * Side effects:
 *	Deallocation of memory.
 *
 *--------------------------------------------------------------
 */

static PatSeq *
DeletePatSeq(
    PatSeq *psPtr)		/* Delete this pattern sequence. */
{
    PatSeq *prevPtr;
    PatSeq *nextPtr;

    assert(psPtr);
    assert(!psPtr->added);
    assert(!psPtr->owned);

    prevPtr = Tcl_GetHashValue(psPtr->hPtr);
    nextPtr = psPtr->ptr.nextObj;

    /*
     * Be sure to remove each binding from its hash chain in the pattern
     * table. If this is the last pattern in the chain, then delete the
     * hash entry too.
     */

    if (prevPtr == psPtr) {
	if (!psPtr->nextSeqPtr) {
	    Tcl_DeleteHashEntry(psPtr->hPtr);
	} else {
	    Tcl_SetHashValue(psPtr->hPtr, psPtr->nextSeqPtr);
	}
    } else {
	for ( ; ; prevPtr = prevPtr->nextSeqPtr) {
	    if (!prevPtr) {
		Tcl_Panic("DeletePatSeq couldn't find on hash chain");
	    }
	    if (prevPtr->nextSeqPtr == psPtr) {
		prevPtr->nextSeqPtr = psPtr->nextSeqPtr;
		break;
	    }
	}
    }

    FreePatSeq(psPtr);
    return nextPtr;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_DeleteAllBindings --
 *
 *	Remove all bindings associated with a given object in a given binding
 *	table.
 *
 * Results:
 *	All bindings associated with object are removed from bindingTable.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
Tk_DeleteAllBindings(
    Tk_BindingTable bindPtr,	/* Table in which to delete bindings. */
    ClientData object)		/* Token for object. */
{
    PatSeq *psPtr;
    PatSeq *nextPtr;
    Tcl_HashEntry *hPtr;

    assert(bindPtr);
    assert(object);

    if (!(hPtr = Tcl_FindHashEntry(&bindPtr->objectTable, (char *) object))) {
	return;
    }

    /*
     * Don't forget to clear lookup tables.
     */

    ClearLookupTable(&bindPtr->lookupTables, object);
    ClearPromotionLists(bindPtr, object);

    for (psPtr = Tcl_GetHashValue(hPtr); psPtr; psPtr = nextPtr) {
	assert(TEST_PSENTRY(psPtr));
	DEBUG(psPtr->added = 0;)
	nextPtr = DeletePatSeq(psPtr);
    }

    Tcl_DeleteHashEntry(hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_BindEvent --
 *
 *	This function is invoked to process an X event. The event is added to
 *	those recorded for the binding table. Then each of the objects at
 *	*objectPtr is checked in order to see if it has a binding that matches
 *	the recent events. If so, the most specific binding is invoked for
 *	each object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the script associated with the matching binding.
 *
 *	All Tcl binding scripts for each object are accumulated before the
 *	first binding is evaluated. If the action of a Tcl binding is to
 *	change or delete a binding, or delete the window associated with the
 *	binding, all the original Tcl binding scripts will still fire.
 *
 *---------------------------------------------------------------------------
 */

/* helper function */
static void
ResetCounters(
    Event *eventInfo,
    unsigned eventType,
    Window window)
{
    Event *curEvent;

    assert(eventInfo);
    curEvent = eventInfo + eventType;

    if (curEvent->xev.xany.window == window) {
	curEvent->xev.xany.window = None;
	eventInfo[eventType].countAny = 0;
	eventInfo[eventType].countDetailed = 0;
    }
}

/* helper function */
static int
IsBetterMatch(
    const PatSeq *fstMatchPtr,
    const PatSeq *sndMatchPtr)	/* this is a better match? */
{
    int diff;

    if (!sndMatchPtr) { return 0; }
    if (!fstMatchPtr) { return 1; }

    diff = CountSpecialized(fstMatchPtr, sndMatchPtr);
    if (diff > 0) { return 1; }
    if (diff < 0) { return 0; }

#if PREFER_MOST_SPECIALIZED_EVENT
    {	/* local scope */
#define M (Tcl_WideUInt)1000000
	static const Tcl_WideUInt weight[5] = { 0, 1, M, M*M, M*M*M };
#undef M
	Tcl_WideUInt fstCount = 0;
	Tcl_WideUInt sndCount = 0;
	unsigned i;

	/*
	 * Count the most high-ordered patterns.
	 *
	 * (This computation assumes that a sequence does not contain more than
	 * 1,000,000 single patterns. It can be precluded that in practice this
	 * assumption will not be violated.)
	 */

	for (i = 0; i < fstMatchPtr->numPats; ++i) {
	    assert(GetCount(fstMatchPtr, i) < SIZE_OF_ARRAY(weight));
	    fstCount += weight[GetCount(fstMatchPtr, i)];
	}
	for (i = 0; i < sndMatchPtr->numPats; ++i) {
	    assert(GetCount(sndMatchPtr, i) < SIZE_OF_ARRAY(weight));
	    sndCount += weight[GetCount(sndMatchPtr, i)];
	}
	if (sndCount > fstCount) { return 1; }
	if (sndCount < fstCount) { return 0; }
    }
#endif

    return sndMatchPtr->number > fstMatchPtr->number;
}

void
Tk_BindEvent(
    Tk_BindingTable bindPtr,	/* Table in which to look for bindings. */
    XEvent *eventPtr,		/* What actually happened. */
    Tk_Window tkwin,		/* Window on display where event occurred (needed in order to
    				 * locate display information). */
    int numObjects,		/* Number of objects at *objArr. */
    ClientData *objArr)		/* Array of one or more objects to check for a matching binding. */
{
    Tcl_Interp *interp;
    ScreenInfo *screenPtr;
    TkDisplay *dispPtr;
    TkDisplay *oldDispPtr;
    Event *curEvent;
    TkWindow *winPtr = (TkWindow *) tkwin;
    BindInfo *bindInfoPtr;
    Tcl_InterpState interpState;
    LookupTables *physTables;
    PatSeq *psPtr[2];
    PatSeq *matchPtrBuf[32];
    PatSeq **matchPtrArr = matchPtrBuf;
    PSList *psl[2];
    Tcl_DString scripts;
    const char *p;
    const char *end;
    unsigned scriptCount;
    int oldScreen;
    unsigned flags;
    unsigned arraySize;
    unsigned newArraySize;
    unsigned i, k;

    assert(bindPtr);
    assert(eventPtr);
    assert(tkwin);
    assert(numObjects >= 0);

    /*
     * Ignore events on windows that don't have names: these are windows like
     * wrapper windows that shouldn't be visible to the application.
     */

    if (!winPtr->pathName) {
	return;
    }

    flags = flagArray[eventPtr->type];

    /*
     * Ignore event types which are not in flagArray and all zeroes there.
     */

    if (eventPtr->type >= TK_LASTEVENT || !flags) {
	return;
    }

    if (flags & KEY_BUTTON_MOTION_VIRTUAL) {
	bindPtr->curModMask = eventPtr->xkey.state;
    } else if (flags & CROSSING) {
	bindPtr->curModMask = eventPtr->xcrossing.state;
    }

    dispPtr = ((TkWindow *) tkwin)->dispPtr;
    bindInfoPtr = winPtr->mainPtr->bindInfo;
    curEvent = bindPtr->eventInfo + eventPtr->type;

    /*
     * Ignore the event completely if it is an Enter, Leave, FocusIn, or
     * FocusOut event with detail NotifyInferior. The reason for ignoring
     * these events is that we don't want transitions between a window and its
     * children to visible to bindings on the parent: this would cause
     * problems for mega-widgets, since the internal structure of a
     * mega-widget isn't supposed to be visible to people watching the parent.
     *
     * Furthermore we have to compute current time, needed for "event generate".
     */

    switch (eventPtr->type) {
    case EnterNotify:
    case LeaveNotify:
	if (eventPtr->xcrossing.time) {
	    bindInfoPtr->lastCurrentTime = CurrentTimeInMilliSecs();
	    bindInfoPtr->lastEventTime = eventPtr->xcrossing.time;
	}
	if (eventPtr->xcrossing.detail == NotifyInferior) {
	    return;
	}
	break;
    case FocusIn:
    case FocusOut:
	if (eventPtr->xfocus.detail == NotifyInferior) {
	    return;
	}
	break;
    case KeyPress:
    case KeyRelease: {
	int reset = 1;

	if (eventPtr->xkey.time) {
	    bindInfoPtr->lastCurrentTime = CurrentTimeInMilliSecs();
	    bindInfoPtr->lastEventTime = eventPtr->xkey.time;
	}
	/* modifier keys should not influence button events */
	for (i = 0; i < (unsigned) dispPtr->numModKeyCodes; ++i) {
	    if (dispPtr->modKeyCodes[i] == eventPtr->xkey.keycode) {
		reset = 0;
	    }
	}
	if (reset) {
	    /* reset repetition count for button events */
	    bindPtr->eventInfo[ButtonPress].countAny = 0;
	    bindPtr->eventInfo[ButtonPress].countDetailed = 0;
	    bindPtr->eventInfo[ButtonRelease].countAny = 0;
	    bindPtr->eventInfo[ButtonRelease].countDetailed = 0;
	}
	break;
    }
    case ButtonPress:
    case ButtonRelease:
	/* reset repetition count for key events */
	bindPtr->eventInfo[KeyPress].countAny = 0;
	bindPtr->eventInfo[KeyPress].countDetailed = 0;
	bindPtr->eventInfo[KeyRelease].countAny = 0;
	bindPtr->eventInfo[KeyRelease].countDetailed = 0;
	/* fallthru */
    case MotionNotify:
	if (eventPtr->xmotion.time) {
	    bindInfoPtr->lastCurrentTime = CurrentTimeInMilliSecs();
	    bindInfoPtr->lastEventTime = eventPtr->xmotion.time;
	}
	break;
    case PropertyNotify:
	if (eventPtr->xproperty.time) {
	    bindInfoPtr->lastCurrentTime = CurrentTimeInMilliSecs();
	    bindInfoPtr->lastEventTime = eventPtr->xproperty.time;
	}
	break;
    case DestroyNotify:
	ResetCounters(bindPtr->eventInfo, KeyPress, eventPtr->xany.window);
	ResetCounters(bindPtr->eventInfo, KeyRelease, eventPtr->xany.window);
	ResetCounters(bindPtr->eventInfo, ButtonPress, eventPtr->xany.window);
	ResetCounters(bindPtr->eventInfo, ButtonRelease, eventPtr->xany.window);
	break;
    }

    /*
     * Now check whether this is a repeating event (multi-click, repeated key press, and so on).
     */

    /* NOTE: if curEvent is not yet set, then the following cannot match: */
    if (curEvent->xev.xany.window == eventPtr->xany.window) {
	switch (eventPtr->type) {
	case KeyPress:
	case KeyRelease:
	    if (MatchEventRepeat(&curEvent->xev, eventPtr)) {
		if (curEvent->xev.xkey.keycode == eventPtr->xkey.keycode) {
		    ++curEvent->countDetailed;
		} else {
		    curEvent->countDetailed = 1;
		}
		++curEvent->countAny;
	    } else {
		curEvent->countAny = curEvent->countDetailed = 1;
	    }
	    break;
	case ButtonPress:
	case ButtonRelease:
	    if (MatchEventNearby(&curEvent->xev, eventPtr)) {
		if (curEvent->xev.xbutton.button == eventPtr->xbutton.button) {
		    ++curEvent->countDetailed;
		} else {
		    curEvent->countDetailed = 1;
		}
		++curEvent->countAny;
	    } else {
		curEvent->countAny = curEvent->countDetailed = 1;
	    }
	    break;
	case EnterNotify:
	case LeaveNotify:
	    if (TestNearbyTime(eventPtr->xcrossing.time, curEvent->xev.xcrossing.time)) {
		++curEvent->countAny;
	    } else {
		curEvent->countAny = 1;
	    }
	    break;
	case PropertyNotify:
	    if (TestNearbyTime(eventPtr->xproperty.time, curEvent->xev.xproperty.time)) {
		++curEvent->countAny;
	    } else {
		curEvent->countAny = 1;
	    }
	    break;
	default:
	    ++curEvent->countAny;
	    break;
	}
    } else {
	curEvent->countAny = curEvent->countDetailed = 1;
    }

    /*
     * Now update the details.
     */

    curEvent->xev = *eventPtr;
    if (flags & KEY) {
	curEvent->detail.info = TkpGetKeySym(dispPtr, eventPtr);
    } else if (flags & BUTTON) {
	curEvent->detail.info = eventPtr->xbutton.button;
    } else if (flags & MOTION) {
	curEvent->detail.info = ButtonNumberFromState(eventPtr->xmotion.state);
    } else if (flags & VIRTUAL) {
	curEvent->detail.name = ((XVirtualEvent *) eventPtr)->name;
    }

    bindPtr->curEvent = curEvent;
    physTables = &bindPtr->lookupTables;
    scriptCount = 0;
    arraySize = 0;
    Tcl_DStringInit(&scripts);

    if ((size_t) numObjects > SIZE_OF_ARRAY(matchPtrBuf)) {
	/* it's unrealistic that the buffer size is too small, but who knows? */
	matchPtrArr = ckalloc(numObjects*sizeof(matchPtrArr[0]));
    }
    memset(matchPtrArr, 0, numObjects*sizeof(matchPtrArr[0]));

    if (!PromArr_IsEmpty(bindPtr->promArr)) {
	for (k = 0; k < (unsigned) numObjects; ++k) {
	    psl[1] = PromArr_Last(bindPtr->promArr);
	    psl[0] = psl[1] - 1;

	    /*
	     * Loop over all promoted bindings, finding the longest matching one.
	     *
	     * Note that we must process all lists, because all matching patterns
	     * have to be promoted. Normally at most one list will be processed, and
	     * usually this list only contains one or two patterns.
	     */

	    for (i = PromArr_Size(bindPtr->promArr); i > 0; --i, --psl[0], --psl[1]) {
		psPtr[0] = MatchPatterns(dispPtr, bindPtr, psl[0], psl[1], i, curEvent, objArr[k], NULL);

		if (IsBetterMatch(matchPtrArr[k], psPtr[0])) {
		    /* we will process it later, because we still may find a pattern with better match */
		    matchPtrArr[k] = psPtr[0];
		}
		if (!PSList_IsEmpty(psl[1])) {
		    /* we have promoted sequences, adjust array size */
		    arraySize = Max(i + 1, arraySize);
		}
	    }
	}
    }

    /*
     * 1. Look for bindings for the specific detail (button and key events).
     * 2. Look for bindings without detail.
     */

    for (k = 0; k < (unsigned) numObjects; ++k) {
	PSList *psSuccList = PromArr_First(bindPtr->promArr);
	PatSeq *bestPtr;

	psl[0] = GetLookupForEvent(physTables, curEvent, objArr[k], 1);
	psl[1] = GetLookupForEvent(physTables, curEvent, objArr[k], 0);

	assert(psl[0] == NULL || psl[0] != psl[1]);

	psPtr[0] = MatchPatterns(dispPtr, bindPtr, psl[0], psSuccList, 0, curEvent, objArr[k], NULL);
	psPtr[1] = MatchPatterns(dispPtr, bindPtr, psl[1], psSuccList, 0, curEvent, objArr[k], NULL);

	if (!PSList_IsEmpty(psSuccList)) {
	    /* we have promoted sequences, adjust array size */
	    arraySize = Max(1u, arraySize);
	}

	bestPtr = psPtr[0] ? psPtr[0] : psPtr[1];

	if (matchPtrArr[k]) {
	    if (IsBetterMatch(matchPtrArr[k], bestPtr)) {
		matchPtrArr[k] = bestPtr;
	    } else {
		/*
		 * We've already found a higher level match, nevertheless it was required to
		 * process the level zero patterns because of possible promotions.
		 */
	    }
	    /*
	     * Now we have to catch up the processing of the script.
	     */
	} else {
	    /*
	     * We have to look whether we can find a better match in virtual table, provided that we
	     * don't have a higher level match.
	     */

	    matchPtrArr[k] = bestPtr;

	    if (eventPtr->type != VirtualEvent) {
		LookupTables *virtTables = &bindInfoPtr->virtualEventTable.lookupTables;
		PatSeq *matchPtr = matchPtrArr[k];
		PatSeq *mPtr;
		PSList *psl[2];

		/*
		 * Note that virtual events cannot promote.
		 */

		psl[0] = GetLookupForEvent(virtTables, curEvent, NULL, 1);
		psl[1] = GetLookupForEvent(virtTables, curEvent, NULL, 0);

		assert(psl[0] == NULL || psl[0] != psl[1]);

		mPtr = MatchPatterns(dispPtr, bindPtr, psl[0], NULL, 0, curEvent, objArr[k], &matchPtr);
		if (mPtr) {
		    matchPtrArr[k] = matchPtr;
		    matchPtr = mPtr;
		}
		if (MatchPatterns(dispPtr, bindPtr, psl[1], NULL, 0, curEvent, objArr[k], &matchPtr)) {
		    matchPtrArr[k] = matchPtr;
		}
	    }
	}

	if (matchPtrArr[k]) {
	    ExpandPercents(winPtr, matchPtrArr[k]->script, curEvent, scriptCount++, &scripts);
	    /* nul is added to the scripts string to separate the various scripts */
	    Tcl_DStringAppend(&scripts, "", 1);
	}
    }

    PromArr_SetSize(bindPtr->promArr, arraySize);

    /*
     * Remove expired pattern sequences.
     */

    for (i = 0, newArraySize = 0; i < arraySize; ++i) {
	PSList *psList = PromArr_Get(bindPtr->promArr, i);
	PSEntry *psEntry;
	PSEntry *psNext;

	for (psEntry = PSList_First(psList); psEntry; psEntry = psNext) {
	    const TkPattern *patPtr;

	    assert(i + 1 < psEntry->psPtr->numPats);

	    psNext = PSList_Next(psEntry);
	    patPtr = &psEntry->psPtr->pats[i + 1];

	    /*
	     * We have to remove the following entries from promotion list (but
	     * only if we don't want to keep it):
	     * ------------------------------------------------------------------
	     * 1) It is marked as expired (see MatchPatterns()).
	     * 2) If we have a Key event, and current entry is matching a Button.
	     * 3) If we have a Button event, and current entry is matching a Key.
	     * 4) If we have a detailed event, current entry it is also detailed,
	     *    we have matching event types, but the details are different.
	     * 5) Current entry has been matched with a different window.
	     */

	    if (psEntry->keepIt) {
		assert(!psEntry->expired);
		psEntry->keepIt = 0;
	    } else if (psEntry->expired
		    || psEntry->window != curEvent->xev.xany.window
		    || (patPtr->info
			&& curEvent->detail.info
			&& patPtr->eventType == (unsigned) curEvent->xev.type
			&& patPtr->info != curEvent->detail.info)) {
		RemoveListEntry(&bindPtr->lookupTables.entryPool, psEntry);
	    } else {
		switch (patPtr->eventType) {
		case ButtonPress:
		case ButtonRelease:
		    if (curEvent->xev.type == KeyPress || curEvent->xev.type == KeyRelease) {
			RemoveListEntry(&bindPtr->lookupTables.entryPool, psEntry);
		    }
		    break;
		case KeyPress:
		case KeyRelease:
		    if (curEvent->xev.type == ButtonPress || curEvent->xev.type == ButtonRelease) {
			RemoveListEntry(&bindPtr->lookupTables.entryPool, psEntry);
		    }
		    break;
		}
	    }
	}

	if (!PSList_IsEmpty(psList)) {
	    /* we still have promoted sequences, adjust array size */
	    newArraySize = Max(i + 1, newArraySize);
	}
    }

    PromArr_SetSize(bindPtr->promArr, newArraySize);

    if (matchPtrArr != matchPtrBuf) {
	ckfree(matchPtrArr);
    }

    if (Tcl_DStringLength(&scripts) == 0) {
	return; /* nothing to do */
    }

    /*
     * Now go back through and evaluate the binding for each object, in order,
     * dealing with "break" and "continue" exceptions appropriately.
     *
     * There are two tricks here:
     * 1. Bindings can be invoked from in the middle of Tcl commands, where
     *    the interp's result is significant (for example, a widget might be
     *    deleted because of an error in creating it, so the result contains
     *    an error message that is eventually going to be returned by the
     *    creating command). To preserve the result, we save it in a dynamic
     *    string.
     * 2. The binding's action can potentially delete the binding, so bindPtr
     *    may not point to anything valid once the action completes. Thus we
     *    have to save bindPtr->interp in a local variable in order to restore
     *    the result.
     */

    interp = bindPtr->interp;

    /*
     * Save information about the current screen, then invoke a script if the
     * screen has changed.
     */

    interpState = Tcl_SaveInterpState(interp, TCL_OK);
    screenPtr = &bindInfoPtr->screenInfo;
    oldDispPtr = screenPtr->curDispPtr;
    oldScreen = screenPtr->curScreenIndex;

    if (dispPtr != screenPtr->curDispPtr || Tk_ScreenNumber(tkwin) != screenPtr->curScreenIndex) {
	screenPtr->curDispPtr = dispPtr;
	screenPtr->curScreenIndex = Tk_ScreenNumber(tkwin);
	ChangeScreen(interp, dispPtr->name, screenPtr->curScreenIndex);
    }

    /*
     * Be careful when dereferencing screenPtr or bindInfoPtr. If we evaluate
     * something that destroys ".", bindInfoPtr would have been freed, but we
     * can tell that by first checking to see if winPtr->mainPtr == NULL.
     */

    Tcl_Preserve(bindInfoPtr);

    for (p = Tcl_DStringValue(&scripts), end = p + Tcl_DStringLength(&scripts); p < end; ) {
	unsigned len = strlen(p);
	int code;

	if (!bindInfoPtr->deleted) {
	    ++screenPtr->bindingDepth;
	}
	Tcl_AllowExceptions(interp);

	code = Tcl_EvalEx(interp, p, len, TCL_EVAL_GLOBAL);
	p += len + 1;

	if (!bindInfoPtr->deleted) {
	    --screenPtr->bindingDepth;
	}
	if (code != TCL_OK && code != TCL_CONTINUE) {
	    if (code != TCL_BREAK) {
		Tcl_AddErrorInfo(interp, "\n    (command bound to event)");
		Tcl_BackgroundException(interp, code);
	    }
	    break;
	}
    }

    if (!bindInfoPtr->deleted
	    && screenPtr->bindingDepth > 0
	    && (oldDispPtr != screenPtr->curDispPtr || oldScreen != screenPtr->curScreenIndex)) {
	/*
	 * Some other binding script is currently executing, but its screen is
	 * no longer current. Change the current display back again.
	 */
	screenPtr->curDispPtr = oldDispPtr;
	screenPtr->curScreenIndex = oldScreen;
	ChangeScreen(interp, oldDispPtr->name, oldScreen);
    }
    Tcl_RestoreInterpState(interp, interpState);
    Tcl_DStringFree(&scripts);
    Tcl_Release(bindInfoPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * MatchPatterns --
 *
 *	Given a list of pattern sequences and the recent event, return
 *	the pattern sequence that best matches this event, if there is
 *	one.
 *
 * Results:
 *
 *	The return value is NULL if no match is found. Otherwise the
 *	return value is the most specific pattern sequence among all
 *	those that match the event table.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* helper function */
static int
VirtPatIsBound(
    Tk_BindingTable bindPtr,	/* Table in which to look for bindings. */
    PatSeq *psPtr,		/* Test this pattern. */
    ClientData object,		/* Check for this binding tag. */
    PatSeq **physPtrPtr)	/* Input: the best physical event.
    				 * Output: the physical event associated with matching virtual event. */
{
    PatternTableKey key;
    const struct VirtOwners *owners;
    unsigned i;

    assert(bindPtr);
    assert(psPtr);
    assert(!psPtr->object);
    assert(physPtrPtr);

    if (*physPtrPtr) {
	const TkPattern *physPatPtr = (*physPtrPtr)->pats;
	const TkPattern *virtPatPtr = psPtr->pats;

	if (physPatPtr->info || !virtPatPtr->info) {
	    if (IsSubsetOf(virtPatPtr->modMask, physPatPtr->modMask)) {
		return 0; /* we cannot surpass this match */
	    }
	}
    }

    /* otherwise on some systems the key contains uninitialized bytes */
    memset(&key, 0, sizeof(key));

    key.object = object;
    key.type = VirtualEvent;
    owners = psPtr->ptr.owners;

    for (i = 0; i < VirtOwners_Size(owners); ++i) {
	Tcl_HashEntry *hPtr = VirtOwners_Get(owners, i);

	key.detail.name = (Tk_Uid) Tcl_GetHashKey(hPtr->tablePtr, hPtr);

	if ((hPtr = Tcl_FindHashEntry(&bindPtr->lookupTables.patternTable, (char *) &key))) {
	    /* The physical event matches this virtual event's definition. */
	    *physPtrPtr = (PatSeq *) Tcl_GetHashValue(hPtr);
	    return 1;
	}
    }

    return 0;
}

/* helper function */
static int
Compare(
    const PatSeq *fstMatchPtr,
    const PatSeq *sndMatchPtr) /* most recent match */
{
    int diff;

    if (!fstMatchPtr) { return +1; }
    assert(sndMatchPtr);
    diff = CountSpecialized(fstMatchPtr, sndMatchPtr);
    return diff ? diff : (int) sndMatchPtr->count - (int) fstMatchPtr->count;
}

/* helper function */
static int
CompareModMasks(
    const PSModMaskArr *fstModMaskArr,
    const PSModMaskArr *sndModMaskArr,
    ModMask fstModMask,
    ModMask sndModMask)
{
    int fstCount = 0;
    int sndCount = 0;
    int i;

    if (PSModMaskArr_IsEmpty(fstModMaskArr)) {
	if (!PSModMaskArr_IsEmpty(sndModMaskArr)) {
	    for (i = PSModMaskArr_Size(sndModMaskArr) - 1; i >= 0; --i) {
		if (*PSModMaskArr_Get(sndModMaskArr, i)) {
		    ++sndCount;
		}
	    }
	}
    } else if (PSModMaskArr_IsEmpty(sndModMaskArr)) {
	for (i = PSModMaskArr_Size(fstModMaskArr) - 1; i >= 0; --i) {
	    if (*PSModMaskArr_Get(fstModMaskArr, i)) {
		++fstCount;
	    }
	}
    } else {
	assert(PSModMaskArr_Size(fstModMaskArr) == PSModMaskArr_Size(sndModMaskArr));

	for (i = PSModMaskArr_Size(fstModMaskArr) - 1; i >= 0; --i) {
	    ModMask fstModMask = *PSModMaskArr_Get(fstModMaskArr, i);
	    ModMask sndModMask = *PSModMaskArr_Get(sndModMaskArr, i);

	    if (IsSubsetOf(fstModMask, sndModMask)) { ++sndCount; }
	    if (IsSubsetOf(sndModMask, fstModMask)) { ++fstCount; }
	}
    }

    /* Finally compare modifier masks of last pattern. */

    if (IsSubsetOf(fstModMask, sndModMask)) { ++sndCount; }
    if (IsSubsetOf(sndModMask, fstModMask)) { ++fstCount; }

    return fstCount - sndCount;
}

static PatSeq *
MatchPatterns(
    TkDisplay *dispPtr,		/* Display from which the event came. */
    Tk_BindingTable bindPtr,	/* Table in which to look for bindings. */
    PSList *psList,		/* List of potentially matching patterns, can be NULL. */
    PSList *psSuccList,		/* Add all matching higher-level pattern sequences to this list.
    				 * Can be NULL. */
    unsigned patIndex,		/* Match only this tag in sequence. */
    const Event *curEvent,	/* Match this event. */
    ClientData object,		/* Check for this binding tag. */
    PatSeq **physPtrPtr)	/* Input: the best physical event; NULL if we test physical events.
    				 * Output: the associated physical event for the best matching virtual
				 * event; NULL when we match physical events. */
{
    Window window;
    PSEntry *psEntry;
    PatSeq *bestPtr;
    PatSeq *bestPhysPtr;
    ModMask bestModMask;
    const PSModMaskArr *bestModMaskArr = NULL;

    assert(dispPtr);
    assert(bindPtr);
    assert(curEvent);

    if (!psList) {
	return NULL;
    }

    bestModMask = 0;
    bestPtr = NULL;
    bestPhysPtr = NULL;
    window = curEvent->xev.xany.window;

    for (psEntry = PSList_First(psList); psEntry; psEntry = PSList_Next(psEntry)) {
	if (patIndex == 0 || psEntry->window == window) {
	    PatSeq* psPtr = psEntry->psPtr;

	    assert(TEST_PSENTRY(psPtr));
	    assert((psPtr->object == NULL) == (physPtrPtr != NULL));
	    assert(psPtr->object || patIndex == 0);
	    assert(psPtr->numPats > patIndex);

	    if (psPtr->object
		    ? psPtr->object == object
		    : VirtPatIsBound(bindPtr, psPtr, object, physPtrPtr)) {
		TkPattern *patPtr = psPtr->pats + patIndex;

		if (patPtr->eventType == (unsigned) curEvent->xev.type
			&& (curEvent->xev.type != CreateNotify
				|| curEvent->xev.xcreatewindow.parent == window)
			&& (!patPtr->name || patPtr->name == curEvent->detail.name)
			&& (!patPtr->info || patPtr->info == curEvent->detail.info)) {
		    /*
		     * Resolve the modifier mask for Alt and Mod keys. Unfortunately this
		     * cannot be done in ParseEventDescription, otherwise this function would
		     * be the better place.
		     */
		    ModMask modMask = ResolveModifiers(dispPtr, patPtr->modMask);
		    ModMask curModMask = ResolveModifiers(dispPtr, bindPtr->curModMask);

		    psEntry->expired = 1; /* remove it from promotion list */

		    if ((modMask & ~curModMask) == 0) {
			unsigned count = patPtr->info ? curEvent->countDetailed : curEvent->countAny;

			if (patIndex < PSModMaskArr_Size(psEntry->lastModMaskArr)) {
			    PSModMaskArr_Set(psEntry->lastModMaskArr, patIndex, &modMask);
			}

			/*
			 * This pattern is finally matching.
			 */

			if (psPtr->numPats == patIndex + 1) {
			    if (patPtr->count <= count) {
				/*
				 * This is also a final pattern.
				 * We always prefer the pattern with better match.
				 * If completely equal than prefer most recently defined pattern.
				 */

				int cmp = Compare(bestPtr, psPtr);

				if (cmp == 0) {
				    cmp = CompareModMasks(psEntry->lastModMaskArr, bestModMaskArr,
					    modMask, bestModMask);
				}

				if (cmp > 0 || (cmp == 0 && bestPtr->number < psPtr->number)) {
				    bestPtr = psPtr;
				    bestModMask = modMask;
				    bestModMaskArr = psEntry->lastModMaskArr;
				    if (physPtrPtr) {
					bestPhysPtr = *physPtrPtr;
				    }
				}
			    } else {
				DEBUG(psEntry->expired = 0;)
				psEntry->keepIt = 1; /* don't remove it from promotion list */
			    }
			} else if (psSuccList) {
			    /*
			     * Not a final pattern, but matching, so promote it to next level.
			     * But do not promote if count of current pattern is not yet reached.
			     */
			    if (patPtr->count == psEntry->count) {
				PSEntry *psNewEntry;

				assert(!patPtr->name);
				psNewEntry = MakeListEntry(
				    &bindPtr->lookupTables.entryPool, psPtr, psPtr->modMaskUsed);
				if (!PSModMaskArr_IsEmpty(psNewEntry->lastModMaskArr)) {
				    PSModMaskArr_Set(psNewEntry->lastModMaskArr, patIndex, &modMask);
				}
				assert(psNewEntry->keepIt);
				assert(psNewEntry->count == 1u);
				PSList_Append(psSuccList, psNewEntry);
				psNewEntry->window = window; /* bind to current window */
			    } else {
				assert(psEntry->count < patPtr->count);
				DEBUG(psEntry->expired = 0;)
				psEntry->count += 1;
				psEntry->keepIt = 1; /* don't remove it from promotion list */
			    }
			}
		    }
		}
	    }
	}
    }

    if (bestPhysPtr) {
	assert(physPtrPtr);
	*physPtrPtr = bestPhysPtr;
    }
    return bestPtr;
}

/*
 *--------------------------------------------------------------
 *
 * ExpandPercents --
 *
 *	Given a command and an event, produce a new command by replacing %
 *	constructs in the original command with information from the X event.
 *
 * Results:
 *	The new expanded command is appended to the dynamic string given by
 *	dsPtr.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
ExpandPercents(
    TkWindow *winPtr,		/* Window where event occurred: needed to get input context. */
    const char *before,		/* Command containing percent expressions to be replaced. */
    Event *eventPtr,		/* Event containing information to be used in % replacements. */
    unsigned scriptCount,	/* The number of script-based binding patterns matched so far for
    				 * this event. */
    Tcl_DString *dsPtr)		/* Dynamic string in which to append new command. */
{
    unsigned flags;
    Tcl_DString buf;
    XEvent *evPtr;

    assert(winPtr);
    assert(before);
    assert(eventPtr);
    assert(dsPtr);

    Tcl_DStringInit(&buf);
    evPtr = &eventPtr->xev;
    flags = (evPtr->type < TK_LASTEVENT) ? flagArray[evPtr->type] : 0;

    while (1) {
	char numStorage[TCL_INTEGER_SPACE];
	const char *string;
	Tcl_WideInt number;

	/*
	 * Find everything up to the next % character and append it to the
	 * result string.
	 */

	for (string = before; *string && *string != '%'; ++string)
	    ;
	if (string != before) {
	    Tcl_DStringAppend(dsPtr, before, string - before);
	    before = string;
	}
	if (!*before) {
	    break;
	}

	/*
	 * There's a percent sequence here. Process it.
	 */

	number = NO_NUMBER;
	string = "??";

	switch (before[1]) {
	case '#':
	    number = evPtr->xany.serial;
	    break;
	case 'a':
	    if (flags & CONFIG) {
		TkpPrintWindowId(numStorage, evPtr->xconfigure.above);
		string = numStorage;
	    }
	    break;
	case 'b':
	    if (flags & BUTTON) {
		number = evPtr->xbutton.button;
	    }
	    break;
	case 'c':
	    if (flags & EXPOSE) {
		number = evPtr->xexpose.count;
	    }
	    break;
	case 'd':
	    if (flags & (CROSSING|FOCUS)) {
		int detail = (flags & FOCUS) ? evPtr->xfocus.detail : evPtr->xcrossing.detail;
		string = TkFindStateString(notifyDetail, detail);
	    } else if (flags & CONFIGREQ) {
		if (evPtr->xconfigurerequest.value_mask & CWStackMode) {
		    string = TkFindStateString(configureRequestDetail, evPtr->xconfigurerequest.detail);
		} else {
		    string = "";
		}
	    } else if (flags & VIRTUAL) {
		XVirtualEvent *vePtr = (XVirtualEvent *) evPtr;
		string = vePtr->user_data ? Tcl_GetString(vePtr->user_data) : "";
	    }
	    break;
	case 'f':
	    if (flags & CROSSING) {
		number = evPtr->xcrossing.focus;
	    }
	    break;
	case 'h':
	    if (flags & EXPOSE) {
		number = evPtr->xexpose.height;
	    } else if (flags & CONFIG) {
		number = evPtr->xconfigure.height;
	    } else if (flags & CREATE) {
		number = evPtr->xcreatewindow.height;
	    } else if (flags & CONFIGREQ) {
		number = evPtr->xconfigurerequest.height;
	    } else if (flags & RESIZEREQ) {
		number = evPtr->xresizerequest.height;
	    }
	    break;
	case 'i':
	    if (flags & CREATE) {
		TkpPrintWindowId(numStorage, evPtr->xcreatewindow.window);
	    } else if (flags & CONFIGREQ) {
		TkpPrintWindowId(numStorage, evPtr->xconfigurerequest.window);
	    } else if (flags & MAPREQ) {
		TkpPrintWindowId(numStorage, evPtr->xmaprequest.window);
	    } else {
		TkpPrintWindowId(numStorage, evPtr->xany.window);
	    }
	    string = numStorage;
	    break;
	case 'k':
	    if ((flags & KEY) && evPtr->type != MouseWheelEvent) {
		number = evPtr->xkey.keycode;
	    }
	    break;
	case 'm':
	    if (flags & CROSSING) {
		string = TkFindStateString(notifyMode, evPtr->xcrossing.mode);
	    } else if (flags & FOCUS) {
		string = TkFindStateString(notifyMode, evPtr->xfocus.mode);
	    }
	    break;
	case 'o':
	    if (flags & CREATE) {
		number = evPtr->xcreatewindow.override_redirect;
	    } else if (flags & MAP) {
		number = evPtr->xmap.override_redirect;
	    } else if (flags & REPARENT) {
		number = evPtr->xreparent.override_redirect;
	    } else if (flags & CONFIG) {
		number = evPtr->xconfigure.override_redirect;
	    }
	    break;
	case 'p':
	    if (flags & CIRC) {
		string = TkFindStateString(circPlace, evPtr->xcirculate.place);
	    } else if (flags & CIRCREQ) {
		string = TkFindStateString(circPlace, evPtr->xcirculaterequest.place);
	    }
	    break;
	case 's':
	    if (flags & KEY_BUTTON_MOTION_VIRTUAL) {
		number = evPtr->xkey.state;
	    } else if (flags & CROSSING) {
		number = evPtr->xcrossing.state;
	    } else if (flags & PROP) {
		string = TkFindStateString(propNotify, evPtr->xproperty.state);
	    } else if (flags & VISIBILITY) {
		string = TkFindStateString(visNotify, evPtr->xvisibility.state);
	    }
	    break;
	case 't':
	    if (flags & KEY_BUTTON_MOTION_VIRTUAL) {
		number = (int) evPtr->xkey.time;
	    } else if (flags & CROSSING) {
		number = (int) evPtr->xcrossing.time;
	    } else if (flags & PROP) {
		number = (int) evPtr->xproperty.time;
	    }
	    break;
	case 'v':
	    number = evPtr->xconfigurerequest.value_mask;
	    break;
	case 'w':
	    if (flags & EXPOSE) {
		number = evPtr->xexpose.width;
	    } else if (flags & CONFIG) {
		number = evPtr->xconfigure.width;
	    } else if (flags & CREATE) {
		number = evPtr->xcreatewindow.width;
	    } else if (flags & CONFIGREQ) {
		number = evPtr->xconfigurerequest.width;
	    } else if (flags & RESIZEREQ) {
		number = evPtr->xresizerequest.width;
	    }
	    break;
	case 'x':
	    if (flags & KEY_BUTTON_MOTION_VIRTUAL) {
		number = evPtr->xkey.x;
	    } else if (flags & CROSSING) {
		number = evPtr->xcrossing.x;
	    } else if (flags & EXPOSE) {
		number = evPtr->xexpose.x;
	    } else if (flags & (CREATE|CONFIG|GRAVITY)) {
		number = evPtr->xcreatewindow.x;
	    } else if (flags & REPARENT) {
		number = evPtr->xreparent.x;
	    } else if (flags & CREATE) {
		number = evPtr->xcreatewindow.x;
	    } else if (flags & CONFIGREQ) {
		number = evPtr->xconfigurerequest.x;
	    }
	    break;
	case 'y':
	    if (flags & KEY_BUTTON_MOTION_VIRTUAL) {
		number = evPtr->xkey.y;
	    } else if (flags & EXPOSE) {
		number = evPtr->xexpose.y;
	    } else if (flags & (CREATE|CONFIG|GRAVITY)) {
		number = evPtr->xcreatewindow.y;
	    } else if (flags & REPARENT) {
		number = evPtr->xreparent.y;
	    } else if (flags & CROSSING) {
		number = evPtr->xcrossing.y;
	    } else if (flags & CREATE) {
		number = evPtr->xcreatewindow.y;
	    } else if (flags & CONFIGREQ) {
		number = evPtr->xconfigurerequest.y;
	    }
	    break;
	case 'A':
	    if ((flags & KEY) && evPtr->type != MouseWheelEvent) {
		Tcl_DStringFree(&buf);
		string = TkpGetString(winPtr, evPtr, &buf);
	    }
	    break;
	case 'B':
	    if (flags & CREATE) {
		number = evPtr->xcreatewindow.border_width;
	    } else if (flags & CONFIGREQ) {
		number = evPtr->xconfigurerequest.border_width;
	    } else if (flags & CONFIG) {
		number = evPtr->xconfigure.border_width;
	    }
	    break;
	case 'D':
	    /*
	     * This is used only by the MouseWheel event.
	     */
	    if ((flags & KEY) && evPtr->type == MouseWheelEvent) {
		number = evPtr->xkey.keycode;
	    }
	    break;
	case 'E':
	    number = (int) evPtr->xany.send_event;
	    break;
	case 'K':
	    if ((flags & KEY) && evPtr->type != MouseWheelEvent) {
		const char *name = TkKeysymToString(eventPtr->detail.info);
		if (name) {
		    string = name;
		}
	    }
	    break;
	case 'M':
	    number = scriptCount;
	    break;
	case 'N':
	    if ((flags & KEY) && evPtr->type != MouseWheelEvent) {
		number = (int) eventPtr->detail.info;
	    }
	    break;
	case 'P':
	    if (flags & PROP) {
		string = Tk_GetAtomName((Tk_Window) winPtr, evPtr->xproperty.atom);
	    }
	    break;
	case 'R':
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		TkpPrintWindowId(numStorage, evPtr->xkey.root);
		string = numStorage;
	    }
	    break;
	case 'S':
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		TkpPrintWindowId(numStorage, evPtr->xkey.subwindow);
		string = numStorage;
	    }
	    break;
	case 'T':
	    number = evPtr->type;
	    break;
	case 'W': {
	    Tk_Window tkwin = Tk_IdToWindow(evPtr->xany.display, evPtr->xany.window);
	    if (tkwin) {
		string = Tk_PathName(tkwin);
	    }
	    break;
	}
	case 'X':
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		number = evPtr->xkey.x_root;
	    }
	    break;
	case 'Y':
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		number = evPtr->xkey.y_root;
	    }
	    break;
	default:
	    numStorage[0] = before[1];
	    numStorage[1] = '\0';
	    string = numStorage;
	    break;
	}

	if (number != NO_NUMBER) {
	    snprintf(numStorage, sizeof(numStorage), "%d", (int) number);
	    string = numStorage;
	}
	{   /* local scope */
	    int cvtFlags;
	    unsigned spaceNeeded = Tcl_ScanElement(string, &cvtFlags);
	    unsigned length = Tcl_DStringLength(dsPtr);

	    Tcl_DStringSetLength(dsPtr, length + spaceNeeded);
	    spaceNeeded = Tcl_ConvertElement(
		    string, Tcl_DStringValue(dsPtr) + length, cvtFlags | TCL_DONT_USE_BRACES);
	    Tcl_DStringSetLength(dsPtr, length + spaceNeeded);
	    before += 2;
	}
    }

    Tcl_DStringFree(&buf);
}

/*
 *----------------------------------------------------------------------
 *
 * ChangeScreen --
 *
 *	This function is invoked whenever the current screen changes in an
 *	application. It invokes a Tcl command named "tk::ScreenChanged",
 *	passing it the screen name as argument. tk::ScreenChanged does things
 *	like making the tk::Priv variable point to an array for the current
 *	display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on what tk::ScreenChanged does. If an error occurs then
 *	bgerror will be invoked.
 *
 *----------------------------------------------------------------------
 */

static void
ChangeScreen(
    Tcl_Interp *interp,		/* Interpreter in which to invoke command. */
    char *dispName,		/* Name of new display. */
    int screenIndex)		/* Index of new screen. */
{
    Tcl_Obj *cmdObj = Tcl_ObjPrintf("::tk::ScreenChanged %s.%d", dispName, screenIndex);
    int code;

    Tcl_IncrRefCount(cmdObj);
    code = Tcl_EvalObjEx(interp, cmdObj, TCL_EVAL_GLOBAL);
    if (code != TCL_OK) {
	Tcl_AddErrorInfo(interp, "\n    (changing screen in event binding)");
	Tcl_BackgroundException(interp, code);
    }
    Tcl_DecrRefCount(cmdObj);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_EventCmd --
 *
 *	This function is invoked to process the "event" Tcl command. It is
 *	used to define and generate events.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_EventObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int index, i;
    char *name;
    const char *event;
    Tk_Window tkwin;
    TkBindInfo bindInfo;
    VirtualEventTable *vetPtr;

    static const char *const optionStrings[] = { "add", "delete", "generate", "info", NULL };
    enum options { EVENT_ADD, EVENT_DELETE, EVENT_GENERATE, EVENT_INFO };

    assert(clientData);

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObjStruct(
	    interp, objv[1], optionStrings, sizeof(char *), "option", 0, &index) != TCL_OK) {
#ifdef SUPPORT_DEBUGGING
    	if (strcmp(Tcl_GetString(objv[1]), "debug") == 0) {
	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "debug number");
		return TCL_ERROR;
	    }
	    Tcl_GetIntFromObj(interp, objv[2], &BindCount);
	    return TCL_OK;
	}
#endif
	return TCL_ERROR;
    }

    tkwin = (Tk_Window) clientData;
    bindInfo = ((TkWindow *) tkwin)->mainPtr->bindInfo;
    vetPtr = &bindInfo->virtualEventTable;

    switch ((enum options) index) {
    case EVENT_ADD:
	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "virtual sequence ?sequence ...?");
	    return TCL_ERROR;
	}
	name = Tcl_GetString(objv[2]);
	for (i = 3; i < objc; ++i) {
	    event = Tcl_GetString(objv[i]);
	    if (!CreateVirtualEvent(interp, vetPtr, name, event)) {
		return TCL_ERROR;
	    }
	}
	break;
    case EVENT_DELETE:
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "virtual ?sequence ...?");
	    return TCL_ERROR;
	}
	name = Tcl_GetString(objv[2]);
	if (objc == 3) {
	    return DeleteVirtualEvent(interp, vetPtr, name, NULL);
	}
	for (i = 3; i < objc; ++i) {
	    event = Tcl_GetString(objv[i]);
	    if (DeleteVirtualEvent(interp, vetPtr, name, event) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	break;
    case EVENT_GENERATE:
	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "window event ?-option value ...?");
	    return TCL_ERROR;
	}
	return HandleEventGenerate(interp, tkwin, objc - 2, objv + 2);
    case EVENT_INFO:
	if (objc == 2) {
	    GetAllVirtualEvents(interp, vetPtr);
	    return TCL_OK;
	}
	if (objc == 3) {
	    return GetVirtualEvent(interp, vetPtr, objv[2]);
	}
	Tcl_WrongNumArgs(interp, 2, objv, "?virtual?");
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitVirtualEventTable --
 *
 *	Given storage for a virtual event table, set up the fields to prepare
 *	a new domain in which virtual events may be defined.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	*vetPtr is now initialized.
 *
 *---------------------------------------------------------------------------
 */

static void
InitVirtualEventTable(
    VirtualEventTable *vetPtr)	/* Pointer to virtual event table. Memory is supplied by the caller. */
{
    assert(vetPtr);
    memset(vetPtr, 0, sizeof(*vetPtr));
    Tcl_InitHashTable(&vetPtr->lookupTables.patternTable, sizeof(PatternTableKey)/sizeof(int));
    Tcl_InitHashTable(&vetPtr->lookupTables.listTable, sizeof(PatternTableKey)/sizeof(int));
    Tcl_InitHashTable(&vetPtr->nameTable, TCL_ONE_WORD_KEYS);
    PSList_Init(&vetPtr->lookupTables.entryPool);
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteVirtualEventTable --
 *
 *	Delete the contents of a virtual event table. The caller is
 *	responsible for freeing any memory used by the table itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *---------------------------------------------------------------------------
 */

static void
DeleteVirtualEventTable(
    VirtualEventTable *vetPtr)	/* The virtual event table to delete. */
{
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    assert(vetPtr);

    hPtr = Tcl_FirstHashEntry(&vetPtr->lookupTables.patternTable, &search);
    for ( ; hPtr; hPtr = Tcl_NextHashEntry(&search)) {
	PatSeq *nextPtr;
	PatSeq *psPtr;

	for (psPtr = Tcl_GetHashValue(hPtr); psPtr; psPtr = nextPtr) {
	    assert(TEST_PSENTRY(psPtr));
	    nextPtr = psPtr->nextSeqPtr;
	    DEBUG(psPtr->owned = 0;)
	    FreePatSeq(psPtr);
	}
    }
    Tcl_DeleteHashTable(&vetPtr->lookupTables.patternTable);

    hPtr = Tcl_FirstHashEntry(&vetPtr->nameTable, &search);
    for ( ; hPtr; hPtr = Tcl_NextHashEntry(&search)) {
	ckfree(Tcl_GetHashValue(hPtr));
    }
    Tcl_DeleteHashTable(&vetPtr->nameTable);
    Tcl_DeleteHashTable(&vetPtr->lookupTables.listTable);

    ClearLookupTable(&vetPtr->lookupTables, NULL);
    DEBUG(countEntryItems -= PSList_Size(&vetPtr->lookupTables.entryPool);)
    PSList_Traverse(&vetPtr->lookupTables.entryPool, FreePatSeqEntry);
}

/*
 *----------------------------------------------------------------------
 *
 * CreateVirtualEvent --
 *
 *	Add a new definition for a virtual event. If the virtual event is
 *	already defined, the new definition augments those that already exist.
 *
 * Results:
 *	The return value is TCL_ERROR if an error occured while creating the
 *	virtual binding. In this case, an error message will be left in the
 *	interp's result. If all went well then the return value is TCL_OK.
 *
 * Side effects:
 *	The virtual event may cause future calls to Tk_BindEvent to behave
 *	differently than they did previously.
 *
 *----------------------------------------------------------------------
 */

static int
CreateVirtualEvent(
    Tcl_Interp *interp,		/* Used for error reporting. */
    VirtualEventTable *vetPtr,	/* Table in which to augment virtual event. */
    char *virtString,		/* Name of new virtual event. */
    const char *eventString)	/* String describing physical event that triggers virtual event. */
{
    PatSeq *psPtr;
    int dummy;
    Tcl_HashEntry *vhPtr;
    PhysOwned *owned;
    Tk_Uid virtUid;

    assert(vetPtr);
    assert(virtString);
    assert(eventString);

    if (!(virtUid = GetVirtualEventUid(interp, virtString))) {
	return 0;
    }

    /*
     * Find/create physical event
     */

    if (!(psPtr = FindSequence(interp, &vetPtr->lookupTables, NULL, eventString, 1, 0, NULL))) {
	return 0;
    }
    assert(TEST_PSENTRY(psPtr));

    /*
     * Find/create virtual event.
     */

    vhPtr = Tcl_CreateHashEntry(&vetPtr->nameTable, virtUid, &dummy);

    /*
     * Make virtual event own the physical event.
     */

    owned = Tcl_GetHashValue(vhPtr);

    if (!PhysOwned_Contains(owned, psPtr)) {
	PhysOwned_Append(&owned, psPtr);
	Tcl_SetHashValue(vhPtr, owned);
	DEBUG(psPtr->owned = 1;)
	InsertPatSeq(&vetPtr->lookupTables, psPtr);
	/* Make physical event so it can trigger the virtual event. */
	VirtOwners_Append(&psPtr->ptr.owners, vhPtr);
    }

    return 1;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteVirtualEvent --
 *
 *	Remove the definition of a given virtual event. If the event string is
 *	NULL, all definitions of the virtual event will be removed.
 *	Otherwise, just the specified definition of the virtual event will be
 *	removed.
 *
 * Results:
 *	The result is a standard Tcl return value. If an error occurs then the
 *	interp's result will contain an error message. It is not an error to
 *	attempt to delete a virtual event that does not exist or a definition
 *	that does not exist.
 *
 * Side effects:
 *	The virtual event given by virtString may be removed from the virtual
 *	event table.
 *
 *--------------------------------------------------------------
 */

static int
DeleteVirtualEvent(
    Tcl_Interp *interp,		/* Used for error reporting. */
    VirtualEventTable *vetPtr,	/* Table in which to delete event. */
    char *virtString,		/* String describing event sequence that triggers binding. */
    const char *eventString)	/* The event sequence that should be deleted, or NULL to delete
    				 * all event sequences for the entire virtual event. */
{
    int iPhys;
    Tk_Uid virtUid;
    Tcl_HashEntry *vhPtr;
    PhysOwned *owned;
    const PatSeq *eventPSPtr;
    PatSeq *lastElemPtr;

    assert(vetPtr);
    assert(virtString);

    if (!(virtUid = GetVirtualEventUid(interp, virtString))) {
	return TCL_ERROR;
    }

    if (!(vhPtr = Tcl_FindHashEntry(&vetPtr->nameTable, virtUid))) {
	return TCL_OK;
    }
    owned = Tcl_GetHashValue(vhPtr);

    eventPSPtr = NULL;
    if (eventString) {
	LookupTables *lookupTables = &vetPtr->lookupTables;

	/*
	 * Delete only the specific physical event associated with the virtual
	 * event. If the physical event doesn't already exist, or the virtual
	 * event doesn't own that physical event, return w/o doing anything.
	 */

	eventPSPtr = FindSequence(interp, lookupTables, NULL, eventString, 0, 0, NULL);
	if (!eventPSPtr) {
	    const char *string = Tcl_GetString(Tcl_GetObjResult(interp));
	    return string[0] ? TCL_ERROR : TCL_OK;
	}
    }

    for (iPhys = PhysOwned_Size(owned); --iPhys >= 0; ) {
	PatSeq *psPtr = PhysOwned_Get(owned, iPhys);

	assert(TEST_PSENTRY(psPtr));

	if (!eventPSPtr || psPtr == eventPSPtr) {
	    VirtOwners *owners = psPtr->ptr.owners;
	    int iVirt = VirtOwners_Find(owners, vhPtr);

	    assert(iVirt != -1); /* otherwise we couldn't find owner, and this should not happen */

	    /*
	     * Remove association between this physical event and the given
	     * virtual event that it triggers.
	     */

	    if (VirtOwners_Size(owners) > 1) {
		/*
		 * This physical event still triggers some other virtual
		 * event(s). Consolidate the list of virtual owners for this
		 * physical event so it no longer triggers the given virtual
		 * event.
		 */
		VirtOwners_Set(owners, iVirt, VirtOwners_Back(owners));
		VirtOwners_PopBack(owners);
	    } else {
		/*
		 * Removed last reference to this physical event, so remove it
		 * from lookup table.
		 */
		DEBUG(psPtr->owned = 0;)
		RemovePatSeqFromLookup(&vetPtr->lookupTables, psPtr);
		DeletePatSeq(psPtr);
	    }

	    /*
	     * Now delete the virtual event's reference to the physical event.
	     */

	    lastElemPtr = PhysOwned_Back(owned);

	    if (PhysOwned_PopBack(owned) > 0 && eventPSPtr) {
		/*
		 * Just deleting this one physical event. Consolidate list of
		 * owned physical events and return.
		 */
		if ((size_t) iPhys < PhysOwned_Size(owned)) {
		    PhysOwned_Set(owned, iPhys, lastElemPtr);
		}
		return TCL_OK;
	    }
	}
    }

    if (PhysOwned_IsEmpty(owned)) {
	/*
	 * All the physical events for this virtual event were deleted, either
	 * because there was only one associated physical event or because the
	 * caller was deleting the entire virtual event. Now the virtual event
	 * itself should be deleted.
	 */
	PhysOwned_Free(&owned);
	Tcl_DeleteHashEntry(vhPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetVirtualEvent --
 *
 *	Return the list of physical events that can invoke the given virtual
 *	event.
 *
 * Results:
 *	The return value is TCL_OK and the interp's result is filled with the
 *	string representation of the physical events associated with the
 *	virtual event; if there are no physical events for the given virtual
 *	event, the interp's result is filled with and empty string. If the
 *	virtual event string is improperly formed, then TCL_ERROR is returned
 *	and an error message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
GetVirtualEvent(
    Tcl_Interp *interp,		/* Interpreter for reporting. */
    VirtualEventTable *vetPtr,	/* Table in which to look for event. */
    Tcl_Obj *virtName)		/* String describing virtual event. */
{
    Tcl_HashEntry *vhPtr;
    unsigned iPhys;
    const PhysOwned *owned;
    Tk_Uid virtUid;
    Tcl_Obj *resultObj;

    assert(vetPtr);
    assert(virtName);

    if (!(virtUid = GetVirtualEventUid(interp, Tcl_GetString(virtName)))) {
	return TCL_ERROR;
    }

    if (!(vhPtr = Tcl_FindHashEntry(&vetPtr->nameTable, virtUid))) {
	return TCL_OK;
    }

    resultObj = Tcl_NewObj();
    owned = Tcl_GetHashValue(vhPtr);
    for (iPhys = 0; iPhys < PhysOwned_Size(owned); ++iPhys) {
	Tcl_ListObjAppendElement(NULL, resultObj, GetPatternObj(PhysOwned_Get(owned, iPhys)));
    }
    Tcl_SetObjResult(interp, resultObj);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * GetAllVirtualEvents --
 *
 *	Return a list that contains the names of all the virtual event
 *	defined.
 *
 * Results:
 *	There is no return value. The interp's result is modified to hold a
 *	Tcl list with one entry for each virtual event in nameTable.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
GetAllVirtualEvents(
    Tcl_Interp *interp,		/* Interpreter returning result. */
    VirtualEventTable *vetPtr)	/* Table containing events. */
{
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;
    Tcl_Obj *resultObj;

    assert(vetPtr);

    resultObj = Tcl_NewObj();
    hPtr = Tcl_FirstHashEntry(&vetPtr->nameTable, &search);
    for ( ; hPtr; hPtr = Tcl_NextHashEntry(&search)) {
	Tcl_Obj* msg = Tcl_ObjPrintf("<<%s>>", (char *) Tcl_GetHashKey(hPtr->tablePtr, hPtr));
	Tcl_ListObjAppendElement(NULL, resultObj, msg);
    }
    Tcl_SetObjResult(interp, resultObj);
}

/*
 *---------------------------------------------------------------------------
 *
 * HandleEventGenerate --
 *
 *	Helper function for the "event generate" command. Generate and process
 *	an XEvent, constructed from information parsed from the event
 *	description string and its optional arguments.
 *
 *	argv[0] contains name of the target window.
 *	argv[1] contains pattern string for one event (e.g, <Control-v>).
 *	argv[2..argc-1] contains -field/option pairs for specifying additional
 *			detail in the generated event.
 *
 *	Either virtual or physical events can be generated this way. The event
 *	description string must contain the specification for only one event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When constructing the event,
 *	    event.xany.serial is filled with the current X serial number.
 *	    event.xany.window is filled with the target window.
 *	    event.xany.display is filled with the target window's display.
 *	Any other fields in eventPtr which are not specified by the pattern
 *	string or the optional arguments, are set to 0.
 *
 *	The event may be handled synchronously or asynchronously, depending on
 *	the value specified by the optional "-when" option. The default
 *	setting is synchronous.
 *
 *---------------------------------------------------------------------------
 */

static int
HandleEventGenerate(
    Tcl_Interp *interp,		/* Interp for errors return and name lookup. */
    Tk_Window mainWin,		/* Main window associated with interp. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    union { XEvent general; XVirtualEvent virtual; } event;

    const char *p;
    const char *name;
    const char *windowName;
    Tcl_QueuePosition pos;
    TkPattern pat;
    Tk_Window tkwin;
    Tk_Window tkwin2;
    TkWindow *mainPtr;
    EventMask eventMask;
    Tcl_Obj *userDataObj;
    int synch;
    int warp;
    unsigned count;
    unsigned flags;
    int number;
    unsigned i;

    static const char *const fieldStrings[] = {
	"-when",	"-above",	"-borderwidth",	"-button",
	"-count",	"-data",	"-delta",	"-detail",
	"-focus",	"-height",
	"-keycode",	"-keysym",	"-mode",	"-override",
	"-place",	"-root",	"-rootx",	"-rooty",
	"-sendevent",	"-serial",	"-state",	"-subwindow",
	"-time",	"-warp",	"-width",	"-window",
	"-x",		"-y",		NULL
    };
    enum field {
	EVENT_WHEN,	EVENT_ABOVE,	EVENT_BORDER,	EVENT_BUTTON,
	EVENT_COUNT,	EVENT_DATA,	EVENT_DELTA,	EVENT_DETAIL,
	EVENT_FOCUS,	EVENT_HEIGHT,
	EVENT_KEYCODE,	EVENT_KEYSYM,	EVENT_MODE,	EVENT_OVERRIDE,
	EVENT_PLACE,	EVENT_ROOT,	EVENT_ROOTX,	EVENT_ROOTY,
	EVENT_SEND,	EVENT_SERIAL,	EVENT_STATE,	EVENT_SUBWINDOW,
	EVENT_TIME,	EVENT_WARP,	EVENT_WIDTH,	EVENT_WINDOW,
	EVENT_X,	EVENT_Y
    };

    assert(mainWin);

    windowName = Tcl_GetString(objv[0]);
    if (!windowName[0]) {
	tkwin = mainWin;
    } else if (!NameToWindow(interp, mainWin, objv[0], &tkwin)) {
	return TCL_ERROR;
    }

    mainPtr = (TkWindow *) mainWin;
    if (!tkwin || mainPtr->mainPtr != ((TkWindow *) tkwin)->mainPtr) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"window id \"%s\" doesn't exist in this application",
		Tcl_GetString(objv[0])));
	Tcl_SetErrorCode(interp, "TK", "LOOKUP", "WINDOW", Tcl_GetString(objv[0]), NULL);
	return TCL_ERROR;
    }

    name = Tcl_GetString(objv[1]);
    p = name;
    eventMask = 0;
    userDataObj = NULL;
    if ((count = ParseEventDescription(interp, &p, &pat, &eventMask)) == 0) {
	return TCL_ERROR;
    }
    if (count != 1u) {
	Tcl_SetObjResult(interp,
		Tcl_NewStringObj("Double, Triple, or Quadruple modifier not allowed", -1));
	Tcl_SetErrorCode(interp, "TK", "EVENT", "BAD_MODIFIER", NULL);
	return TCL_ERROR;
    }
    if (*p) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("only one event specification allowed", -1));
	Tcl_SetErrorCode(interp, "TK", "EVENT", "MULTIPLE", NULL);
	return TCL_ERROR;
    }

    memset(&event, 0, sizeof(event));
    event.general.xany.type = pat.eventType;
    event.general.xany.serial = NextRequest(Tk_Display(tkwin));
    event.general.xany.send_event = 0;
    if (windowName[0]) {
	event.general.xany.window = Tk_WindowId(tkwin);
    } else {
	event.general.xany.window = RootWindow(Tk_Display(tkwin), Tk_ScreenNumber(tkwin));
    }
    event.general.xany.display = Tk_Display(tkwin);

    flags = flagArray[event.general.xany.type];
    if (flags & DESTROY) {
	/*
	 * Event DestroyNotify should be generated by destroying the window.
	 */
	Tk_DestroyWindow(tkwin);
	return TCL_OK;
    }
    if (flags & KEY_BUTTON_MOTION_VIRTUAL) {
	event.general.xkey.state = pat.modMask;
	if ((flags & KEY) && event.general.xany.type != MouseWheelEvent) {
	    TkpSetKeycodeAndState(tkwin, pat.info, &event.general);
	} else if (flags & BUTTON) {
	    event.general.xbutton.button = pat.info;
	} else if (flags & VIRTUAL) {
	    event.virtual.name = pat.name;
	}
    }
    if (flags & (CREATE|UNMAP|MAP|REPARENT|CONFIG|GRAVITY|CIRC)) {
	event.general.xcreatewindow.window = event.general.xany.window;
    }

    if (flags & KEY_BUTTON_MOTION_CROSSING) {
	event.general.xkey.x_root = -1;
	event.general.xkey.y_root = -1;
    }

    if (event.general.xany.type == FocusIn || event.general.xany.type == FocusOut) {
	event.general.xany.send_event = GENERATED_FOCUS_EVENT_MAGIC;
    }

    /*
     * Process the remaining arguments to fill in additional fields of the event.
     */

    synch = 1;
    warp = 0;
    pos = TCL_QUEUE_TAIL;

    for (i = 2; i < (unsigned) objc; i += 2) {
	Tcl_Obj *optionPtr, *valuePtr;
	int badOpt = 0;
	int index;

	optionPtr = objv[i];
	valuePtr = objv[i + 1];

	if (Tcl_GetIndexFromObjStruct(interp, optionPtr, fieldStrings,
		sizeof(char *), "option", TCL_EXACT, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (IsOdd(objc)) {
	    /*
	     * This test occurs after Tcl_GetIndexFromObj() so that "event
	     * generate <Button> -xyz" will return the error message that
	     * "-xyz" is a bad option, rather than that the value for "-xyz"
	     * is missing.
	     */

	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "value for \"%s\" missing", Tcl_GetString(optionPtr)));
	    Tcl_SetErrorCode(interp, "TK", "EVENT", "MISSING_VALUE", NULL);
	    return TCL_ERROR;
	}

	switch ((enum field) index) {
	case EVENT_WARP:
	    if (Tcl_GetBooleanFromObj(interp, valuePtr, &warp) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (!(flags & KEY_BUTTON_MOTION_VIRTUAL)) {
		badOpt = 1;
	    }
	    break;
	case EVENT_WHEN:
	    pos = (Tcl_QueuePosition) TkFindStateNumObj(interp, optionPtr, queuePosition, valuePtr);
	    if ((int) pos < -1) {
		return TCL_ERROR;
	    }
	    synch = ((int) pos == -1);
	    break;
	case EVENT_ABOVE:
	    if (!NameToWindow(interp, tkwin, valuePtr, &tkwin2)) {
		return TCL_ERROR;
	    }
	    if (flags & CONFIG) {
		event.general.xconfigure.above = Tk_WindowId(tkwin2);
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_BORDER:
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & (CREATE|CONFIG)) {
		event.general.xcreatewindow.border_width = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_BUTTON:
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & BUTTON) {
		event.general.xbutton.button = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_COUNT:
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & EXPOSE) {
		event.general.xexpose.count = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_DATA:
	    if (flags & VIRTUAL) {
		/*
		 * Do not increment reference count until after parsing
		 * completes and we know that the event generation is really
		 * going to happen.
		 */
		userDataObj = valuePtr;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_DELTA:
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if ((flags & KEY) && event.general.xkey.type == MouseWheelEvent) {
		event.general.xkey.keycode = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_DETAIL:
	    number = TkFindStateNumObj(interp, optionPtr, notifyDetail, valuePtr);
	    if (number < 0) {
		return TCL_ERROR;
	    }
	    if (flags & FOCUS) {
		event.general.xfocus.detail = number;
	    } else if (flags & CROSSING) {
		event.general.xcrossing.detail = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_FOCUS:
	    if (Tcl_GetBooleanFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & CROSSING) {
		event.general.xcrossing.focus = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_HEIGHT:
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & EXPOSE) {
		event.general.xexpose.height = number;
	    } else if (flags & CONFIG) {
		event.general.xconfigure.height = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_KEYCODE:
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if ((flags & KEY) && event.general.xkey.type != MouseWheelEvent) {
		event.general.xkey.keycode = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_KEYSYM: {
	    KeySym keysym;
	    const char *value;

	    value = Tcl_GetString(valuePtr);
	    keysym = TkStringToKeysym(value);
	    if (keysym == NoSymbol) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("unknown keysym \"%s\"", value));
		Tcl_SetErrorCode(interp, "TK", "LOOKUP", "KEYSYM", value, NULL);
		return TCL_ERROR;
	    }

	    TkpSetKeycodeAndState(tkwin, keysym, &event.general);
	    if (event.general.xkey.keycode == 0) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("no keycode for keysym \"%s\"", value));
		Tcl_SetErrorCode(interp, "TK", "LOOKUP", "KEYCODE", value, NULL);
		return TCL_ERROR;
	    }
	    if (!(flags & KEY) || (event.general.xkey.type == MouseWheelEvent)) {
		badOpt = 1;
	    }
	    break;
	}
	case EVENT_MODE:
	    if ((number = TkFindStateNumObj(interp, optionPtr, notifyMode, valuePtr)) < 0) {
		return TCL_ERROR;
	    }
	    if (flags & CROSSING) {
		event.general.xcrossing.mode = number;
	    } else if (flags & FOCUS) {
		event.general.xfocus.mode = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_OVERRIDE:
	    if (Tcl_GetBooleanFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & CREATE) {
		event.general.xcreatewindow.override_redirect = number;
	    } else if (flags & MAP) {
		event.general.xmap.override_redirect = number;
	    } else if (flags & REPARENT) {
		event.general.xreparent.override_redirect = number;
	    } else if (flags & CONFIG) {
		event.general.xconfigure.override_redirect = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_PLACE:
	    if ((number = TkFindStateNumObj(interp, optionPtr, circPlace, valuePtr)) < 0) {
		return TCL_ERROR;
	    }
	    if (flags & CIRC) {
		event.general.xcirculate.place = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_ROOT:
	    if (!NameToWindow(interp, tkwin, valuePtr, &tkwin2)) {
		return TCL_ERROR;
	    }
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		event.general.xkey.root = Tk_WindowId(tkwin2);
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_ROOTX:
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		event.general.xkey.x_root = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_ROOTY:
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		event.general.xkey.y_root = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_SEND: {
	    const char *value;

	    value = Tcl_GetString(valuePtr);
	    if (isdigit(UCHAR(value[0]))) {
		/*
		 * Allow arbitrary integer values for the field; they are
		 * needed by a few of the tests in the Tk test suite.
		 */
		if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (number) {
		    /*
		     * send_event only expects 1 or 0. We cannot allow arbitrary non-zero
		     * values, otherwise the thing with GENERATED_FOCUS_EVENT_MAGIC will not
		     * work.
		     */
		    number = 1;
		}
	    } else if (Tcl_GetBooleanFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    event.general.xany.send_event |= number;
	    break;
	}
	case EVENT_SERIAL:
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    event.general.xany.serial = number;
	    break;
	case EVENT_STATE:
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (flags & KEY_BUTTON_MOTION_VIRTUAL) {
		    event.general.xkey.state = number;
		} else {
		    event.general.xcrossing.state = number;
		}
	    } else if (flags & VISIBILITY) {
		if ((number = TkFindStateNumObj(interp, optionPtr, visNotify, valuePtr)) < 0) {
		    return TCL_ERROR;
		}
		event.general.xvisibility.state = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_SUBWINDOW:
	    if (!NameToWindow(interp, tkwin, valuePtr, &tkwin2)) {
		return TCL_ERROR;
	    }
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		event.general.xkey.subwindow = Tk_WindowId(tkwin2);
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_TIME: {
	    if (strcmp(Tcl_GetString(valuePtr), "current") == 0) {
		TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;
		BindInfo *biPtr = mainPtr->mainPtr->bindInfo;
		number = dispPtr->lastEventTime + (CurrentTimeInMilliSecs() - biPtr->lastCurrentTime);
	    } else if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		event.general.xkey.time = number;
	    } else if (flags & PROP) {
		event.general.xproperty.time = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	}
	case EVENT_WIDTH:
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & EXPOSE) {
		event.general.xexpose.width = number;
	    } else if (flags & (CREATE|CONFIG)) {
		event.general.xcreatewindow.width = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_WINDOW:
	    if (!NameToWindow(interp, tkwin, valuePtr, &tkwin2)) {
		return TCL_ERROR;
	    }
	    if (flags & (CREATE|UNMAP|MAP|REPARENT|CONFIG|GRAVITY|CIRC)) {
		event.general.xcreatewindow.window = Tk_WindowId(tkwin2);
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_X:
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		event.general.xkey.x = number;

		/*
		 * Only modify rootx as well if it hasn't been changed.
		 */

		if (event.general.xkey.x_root == -1) {
		    int rootX, rootY;

		    Tk_GetRootCoords(tkwin, &rootX, &rootY);
		    event.general.xkey.x_root = rootX + number;
		}
	    } else if (flags & EXPOSE) {
		event.general.xexpose.x = number;
	    } else if (flags & (CREATE|CONFIG|GRAVITY)) {
		event.general.xcreatewindow.x = number;
	    } else if (flags & REPARENT) {
		event.general.xreparent.x = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	case EVENT_Y:
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & KEY_BUTTON_MOTION_CROSSING) {
		event.general.xkey.y = number;

		/*
		 * Only modify rooty as well if it hasn't been changed.
		 */

		if (event.general.xkey.y_root == -1) {
		    int rootX, rootY;

		    Tk_GetRootCoords(tkwin, &rootX, &rootY);
		    event.general.xkey.y_root = rootY + number;
		}
	    } else if (flags & EXPOSE) {
		event.general.xexpose.y = number;
	    } else if (flags & (CREATE|CONFIG|GRAVITY)) {
		event.general.xcreatewindow.y = number;
	    } else if (flags & REPARENT) {
		event.general.xreparent.y = number;
	    } else {
		badOpt = 1;
	    }
	    break;
	}

    	if (badOpt) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "%s event doesn't accept \"%s\" option", name, Tcl_GetString(optionPtr)));
	    Tcl_SetErrorCode(interp, "TK", "EVENT", "BAD_OPTION", NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * Don't generate events for windows that don't exist yet.
     */

    if (event.general.xany.window) {
	if (userDataObj) {
	    /*
	     * Must be virtual event to set that variable to non-NULL. Now we want
	     * to install the object into the event. Note that we must incr the
	     * refcount before firing it into the low-level event subsystem; the
	     * refcount will be decremented once the event has been processed.
	     */
	    event.virtual.user_data = userDataObj;
	    Tcl_IncrRefCount(userDataObj);
	}

	/*
	 * Now we have constructed the event, inject it into the event handling
	 * code.
	 */

	if (synch) {
	    Tk_HandleEvent(&event.general);
	} else {
	    Tk_QueueWindowEvent(&event.general, pos);
	}

	/*
	 * We only allow warping if the window is mapped.
	 */

	if (warp && Tk_IsMapped(tkwin)) {
	    TkDisplay *dispPtr = TkGetDisplay(event.general.xmotion.display);

	    Tk_Window warpWindow = Tk_IdToWindow(dispPtr->display, event.general.xmotion.window);

	    if (!(dispPtr->flags & TK_DISPLAY_IN_WARP)) {
		Tcl_DoWhenIdle(DoWarp, dispPtr);
		dispPtr->flags |= TK_DISPLAY_IN_WARP;
	    }

	    if (warpWindow != dispPtr->warpWindow) {
		if (warpWindow) {
		    Tcl_Preserve(warpWindow);
		}
		if (dispPtr->warpWindow) {
		    Tcl_Release(dispPtr->warpWindow);
		}
		dispPtr->warpWindow = warpWindow;
	    }
	    dispPtr->warpMainwin = mainWin;
	    dispPtr->warpX = event.general.xmotion.x;
	    dispPtr->warpY = event.general.xmotion.y;
	}
    }

    Tcl_ResetResult(interp);
    return TCL_OK;
}
/*
 *---------------------------------------------------------------------------
 *
 * NameToWindow --
 *
 *	Helper function for lookup of a window given its path name. Our
 *	version is able to handle window id's.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
NameToWindow(
    Tcl_Interp *interp,		/* Interp for error return and name lookup. */
    Tk_Window mainWin,		/* Main window of application. */
    Tcl_Obj *objPtr,		/* Contains name or id string of window. */
    Tk_Window *tkwinPtr)	/* Filled with token for window. */
{
    const char *name;
    Tk_Window tkwin;

    assert(mainWin);
    assert(objPtr);
    assert(tkwinPtr);

    name = Tcl_GetString(objPtr);

    if (name[0] == '.') {
	if (!(tkwin = Tk_NameToWindow(interp, name, mainWin))) {
	    return 0;
	}
    } else {
	Window id;

	tkwin = NULL;

	/*
	 * Check for the winPtr being valid, even if it looks okay to
	 * TkpScanWindowId. [Bug #411307]
	 */

	if (TkpScanWindowId(NULL, name, &id) == TCL_OK) {
	    tkwin = Tk_IdToWindow(Tk_Display(mainWin), id);
	}

	if (!tkwin) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad window name/identifier \"%s\"", name));
	    Tcl_SetErrorCode(interp, "TK", "LOOKUP", "WINDOW_ID", name, NULL);
	    return 0;
	}
    }

    assert(tkwin);
    *tkwinPtr = tkwin;
    return 1;
}

/*
 *-------------------------------------------------------------------------
 *
 * DoWarp --
 *
 *	Perform Warping of X pointer. Executed as an idle handler only.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	X Pointer will move to a new location.
 *
 *-------------------------------------------------------------------------
 */

static void
DoWarp(
    ClientData clientData)
{
    TkDisplay *dispPtr = clientData;

    assert(clientData);

    /*
     * DoWarp was scheduled only if the window was mapped. It needs to be
     * still mapped at the time the present idle callback is executed. Also
     * one needs to guard against window destruction in the meantime.
     * Finally, the case warpWindow == NULL is special in that it means
     * the whole screen.
     */

    if (!dispPtr->warpWindow ||
            (Tk_IsMapped(dispPtr->warpWindow) && Tk_WindowId(dispPtr->warpWindow) != None)) {
        TkpWarpPointer(dispPtr);
        XForceScreenSaver(dispPtr->display, ScreenSaverReset);
    }

    if (dispPtr->warpWindow) {
	Tcl_Release(dispPtr->warpWindow);
	dispPtr->warpWindow = NULL;
    }
    dispPtr->flags &= ~TK_DISPLAY_IN_WARP;
}

/*
 *-------------------------------------------------------------------------
 *
 * GetVirtualEventUid --
 *
 *	Determine if the given string is in the proper format for a virtual
 *	event.
 *
 * Results:
 *	The return value is NULL if the virtual event string was not in the
 *	proper format. In this case, an error message will be left in the
 *	interp's result. Otherwise the return value is a Tk_Uid that
 *	represents the virtual event.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static Tk_Uid
GetVirtualEventUid(
    Tcl_Interp *interp,
    char *virtString)
{
    Tk_Uid uid;
    size_t length;

    assert(virtString);

    length = strlen(virtString);

    if (length < 5
	    || virtString[0] != '<'
	    || virtString[1] != '<'
	    || virtString[length - 2] != '>'
	    || virtString[length - 1] != '>') {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("virtual event \"%s\" is badly formed", virtString));
	Tcl_SetErrorCode(interp, "TK", "EVENT", "VIRTUAL", "MALFORMED", NULL);
	return NULL;
    }
    virtString[length - 2] = '\0';
    uid = Tk_GetUid(virtString + 2);
    virtString[length - 2] = '>';

    return uid;
}

/*
 *----------------------------------------------------------------------
 *
 * FindSequence --
 *
 *	Find the entry in the pattern table that corresponds to a particular
 *	pattern string, and return a pointer to that entry.
 *
 * Results:
 *	The return value is normally a pointer to the PatSeq in patternTable
 *	that corresponds to eventString. If an error was found while parsing
 *	eventString, or if "create" is 0 and no pattern sequence previously
 *	existed, then NULL is returned and the interp's result contains a
 *	message describing the problem. If no pattern sequence previously
 *	existed for eventString, then a new one is created with a NULL command
 *	field. In a successful return, *maskPtr is filled in with a mask of
 *	the event types on which the pattern sequence depends.
 *
 * Side effects:
 *	A new pattern sequence may be allocated.
 *
 *----------------------------------------------------------------------
 */

static PatSeq *
FindSequence(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    LookupTables *lookupTables,	/* Tables used for lookup. */
    ClientData object,		/* For binding table, token for object with which binding is
    				 * associated. For virtual event table, NULL. */
    const char *eventString,	/* String description of pattern to match on. See user
    				 * documentation for details. */
    int create,		/* 0 means don't create the entry if it doesn't already exist.
    				 * 1 means create. */
    int allowVirtual,		/* 0 means that virtual events are not allowed in the sequence.
    				 * 1 otherwise. */
    EventMask *maskPtr)		/* *maskPtr is filled in with the event types on which this
    				 * pattern sequence depends. */
{
    unsigned patsBufSize = 1;
    unsigned numPats;
    unsigned totalCount = 0;
    int virtualFound = 0;
    const char *p = eventString;
    TkPattern *patPtr;
    PatSeq *psPtr;
    Tcl_HashEntry *hPtr;
    int isNew;
    unsigned count;
    unsigned maxCount = 0;
    EventMask eventMask = 0;
    ModMask modMask = 0;
    PatternTableKey key;

    assert(lookupTables);
    assert(eventString);

    psPtr = ckalloc(PATSEQ_MEMSIZE(patsBufSize));

    /*
     *------------------------------------------------------------------
     * Step 1: parse the pattern string to produce an array of Patterns.
     *------------------------------------------------------------------
     */

    for (patPtr = psPtr->pats, numPats = 0; *(p = SkipSpaces(p)); ++patPtr, ++numPats) {
	if (numPats >= patsBufSize) {
	    unsigned pos = patPtr - psPtr->pats;
	    patsBufSize += patsBufSize;
	    psPtr = ckrealloc(psPtr, PATSEQ_MEMSIZE(patsBufSize));
	    patPtr = psPtr->pats + pos;
	}

	if ((count = ParseEventDescription(interp, &p, patPtr, &eventMask)) == 0) {
	    /* error encountered */
	    ckfree(psPtr);
	    return NULL;
	}

	if (eventMask & VirtualEventMask) {
	    if (!allowVirtual) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"virtual event not allowed in definition of another virtual event", -1));
		Tcl_SetErrorCode(interp, "TK", "EVENT", "VIRTUAL", "INNER", NULL);
		ckfree(psPtr);
		return NULL;
	    }
	    virtualFound = 1;
	}

	if (count > 1u) {
	    maxCount = Max(count, maxCount);
	}

	totalCount += count;
	modMask |= patPtr->modMask;
    }

    /*
     *------------------------------------------------------------------
     * Step 2: find the sequence in the binding table if it exists, and
     * add a new sequence to the table if it doesn't.
     *------------------------------------------------------------------
     */

    if (numPats == 0) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("no events specified in binding", -1));
	Tcl_SetErrorCode(interp, "TK", "EVENT", "NO_EVENTS", NULL);
	ckfree(psPtr);
	return NULL;
    }
    if (numPats > 1u && virtualFound) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("virtual events may not be composed", -1));
	Tcl_SetErrorCode(interp, "TK", "EVENT", "VIRTUAL", "COMPOSITION", NULL);
	ckfree(psPtr);
	return NULL;
    }
    if (patsBufSize > numPats) {
	psPtr = ckrealloc(psPtr, PATSEQ_MEMSIZE(numPats));
    }

    patPtr = psPtr->pats;
    psPtr->object = object;
    SetupPatternKey(&key, psPtr);
    hPtr = Tcl_CreateHashEntry(&lookupTables->patternTable, (char *) &key, &isNew);
    if (!isNew) {
	unsigned sequenceSize = numPats*sizeof(TkPattern);
	PatSeq *psPtr2;

	for (psPtr2 = Tcl_GetHashValue(hPtr); psPtr2; psPtr2 = psPtr2->nextSeqPtr) {
	    assert(TEST_PSENTRY(psPtr2));
	    if (numPats == psPtr2->numPats && memcmp(patPtr, psPtr2->pats, sequenceSize) == 0) {
		ckfree(psPtr);
		if (maskPtr) {
		    *maskPtr = eventMask;
		}
		return psPtr2;
	    }
	}
    }
    if (!create) {
	if (isNew) {
	    Tcl_DeleteHashEntry(hPtr);
	}

	/*
	 * No binding exists for the sequence, so return an empty error. This
	 * is a special error that the caller will check for in order to
	 * silently ignore this case. This is a hack that maintains backward
	 * compatibility for Tk_GetBinding but the various "bind" commands
	 * silently ignore missing bindings.
	 */

	ckfree(psPtr);
	return NULL;
    }

    DEBUG(countSeqItems += 1;)

    psPtr->numPats = numPats;
    psPtr->count = totalCount;
    psPtr->number = lookupTables->number++;
    psPtr->added = 0;
    psPtr->modMaskUsed = (modMask != 0);
    psPtr->script = NULL;
    psPtr->nextSeqPtr = Tcl_GetHashValue(hPtr);
    psPtr->hPtr = hPtr;
    psPtr->ptr.nextObj = NULL;
    assert(psPtr->ptr.owners == NULL);
    DEBUG(psPtr->owned = 0;)
    Tcl_SetHashValue(hPtr, psPtr);

    if (maskPtr) {
	*maskPtr = eventMask;
    }
    return psPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseEventDescription --
 *
 *	Fill Pattern buffer with information about event from event string.
 *
 * Results:
 *	Leaves error message in interp and returns 0 if there was an error due
 *	to a badly formed event string. Returns 1 if proper event was
 *	specified, 2 if Double modifier was used in event string, or 3 if
 *	Triple was used.
 *
 * Side effects:
 *	On exit, eventStringPtr points to rest of event string (after the
 *	closing '>', so that this function can be called repeatedly to parse
 *	all the events in the entire sequence.
 *
 *---------------------------------------------------------------------------
 */

/* helper function */
static unsigned
FinalizeParseEventDescription(
    Tcl_Interp *interp,
    TkPattern *patPtr,
    unsigned count,
    Tcl_Obj* errorObj,
    const char* errCode)
{
    assert(patPtr);
    assert(!errorObj == (count > 0));

    if (errorObj) {
	Tcl_SetObjResult(interp, errorObj);
	Tcl_SetErrorCode(interp, "TK", "EVENT", errCode, NULL);
    }
    patPtr->count = count;
    return count;
}

static unsigned
ParseEventDescription(
    Tcl_Interp *interp,		/* For error messages. */
    const char **eventStringPtr,/* On input, holds a pointer to start of event string. On exit,
    				 * gets pointer to rest of string after parsed event. */
    TkPattern *patPtr,		/* Filled with the pattern parsed from the event string. */
    EventMask *eventMaskPtr)	/* Filled with event mask of matched event. */
{
    const char *p;
    EventMask eventMask = 0;
    unsigned count = 1;

    assert(eventStringPtr);
    assert(patPtr);
    assert(eventMaskPtr);

    p = *eventStringPtr;
    memset(patPtr, 0, sizeof(TkPattern)); /* otherwise memcmp doesn't work */

    /*
     * Handle simple ASCII characters.
     */

    if (*p != '<') {
	char string[2];

	patPtr->eventType = KeyPress;
	eventMask = KeyPressMask;
	string[0] = *p;
	string[1] = '\0';
	patPtr->info = TkStringToKeysym(string);
	if (patPtr->info == NoSymbol) {
	    if (!isprint(UCHAR(*p))) {
		return FinalizeParseEventDescription(
			interp,
			patPtr, 0,
			Tcl_ObjPrintf("bad ASCII character 0x%x", UCHAR(*p)), "BAD_CHAR");
	    }
	    patPtr->info = *p;
	}
	++p;
    } else {
	/*
	 * A fancier event description. This can be either a virtual event or a physical event.
	 *
	 * A virtual event description consists of:
	 *
	 * 1. double open angle brackets.
	 * 2. virtual event name.
	 * 3. double close angle brackets.
	 *
	 * A physical event description consists of:
	 *
	 * 1. open angle bracket.
	 * 2. any number of modifiers, each followed by spaces or dashes.
	 * 3. an optional event name.
	 * 4. an option button or keysym name. Either this or item 3 *must* be present; if both
	 *    are present then they are separated by spaces or dashes.
	 * 5. a close angle bracket.
	 */

	++p;
	if (*p == '<') {
	    /*
	     * This is a virtual event: soak up all the characters up to the next '>'.
	     */

	    const char *field = p + 1;
	    char buf[256];
	    char* bufPtr = buf;
	    unsigned size;

	    p = strchr(field, '>');
	    if (p == field) {
		return FinalizeParseEventDescription(
			interp,
			patPtr, 0,
			Tcl_NewStringObj("virtual event \"<<>>\" is badly formed", -1), "MALFORMED");
	    }
	    if (!p || p[1] != '>') {
		return FinalizeParseEventDescription(
			interp,
			patPtr, 0,
			Tcl_NewStringObj("missing \">\" in virtual binding", -1), "MALFORMED");
	    }

	    size = p - field;
	    if (size >= sizeof(buf)) {
		bufPtr = ckalloc(size + 1);
	    }
	    strncpy(bufPtr, field, size);
	    bufPtr[size] = '\0';
	    eventMask = VirtualEventMask;
	    patPtr->eventType = VirtualEvent;
	    patPtr->name = Tk_GetUid(bufPtr);
	    if (bufPtr != buf) {
		ckfree(bufPtr);
	    }
	    p += 2;
	} else {
	    unsigned eventFlags;
	    char field[512];
	    Tcl_HashEntry *hPtr;

	    while (1) {
		ModInfo *modPtr;

		p = GetField(p, field, sizeof(field));
		if (*p == '>') {
		    /*
		     * This solves the problem of, e.g., <Control-M> being
		     * misinterpreted as Control + Meta + missing keysym instead of
		     * Control + KeyPress + M.
		     */

		     break;
		}
		if (!(hPtr = Tcl_FindHashEntry(&modTable, field))) {
		    break;
		}
		modPtr = Tcl_GetHashValue(hPtr);
		patPtr->modMask |= modPtr->mask;
		if (modPtr->flags & MULT_CLICKS) {
		    unsigned i = modPtr->flags & MULT_CLICKS;

		    count = 2;
		    while (i >>= 1) {
			++count;
		    }
		}
		p = SkipFieldDelims(p);
	    }

	    eventFlags = 0;
	    if ((hPtr = Tcl_FindHashEntry(&eventTable, field))) {
		const EventInfo *eiPtr = Tcl_GetHashValue(hPtr);

		patPtr->eventType = eiPtr->type;
		eventFlags = flagArray[eiPtr->type];
		eventMask = eiPtr->eventMask;
		p = GetField(SkipFieldDelims(p), field, sizeof(field));
	    }
	    if (*field) {
		unsigned button = GetButtonNumber(field);

		if ((eventFlags & BUTTON)
			|| (button && eventFlags == 0)
			|| (SUPPORT_ADDITIONAL_MOTION_SYNTAX && (eventFlags & MOTION) && button == 0)) {
		    /* This must be a button (or bad motion) event */
		    if (button == 0) {
			return FinalizeParseEventDescription(
				interp,
				patPtr, 0,
				Tcl_ObjPrintf("bad button number \"%s\"", field), "BUTTON");
		    }
		    patPtr->info = button;
		    if (!(eventFlags & BUTTON)) {
			patPtr->eventType = ButtonPress;
			eventMask = ButtonPressMask;
		    }
		} else if ((eventFlags & KEY) || eventFlags == 0) {
		    /* This must be a key event */
		    patPtr->info = TkStringToKeysym(field);
		    if (patPtr->info == NoSymbol) {
			return FinalizeParseEventDescription(
				interp,
				patPtr, 0,
				Tcl_ObjPrintf("bad event type or keysym \"%s\"", field), "KEYSYM");
		    }
		    if (!(eventFlags & KEY)) {
			patPtr->eventType = KeyPress;
			eventMask = KeyPressMask;
		    }
		} else if (button) {
		    if (!SUPPORT_ADDITIONAL_MOTION_SYNTAX || patPtr->eventType != MotionNotify) {
			return FinalizeParseEventDescription(
				interp,
				patPtr, 0,
				Tcl_ObjPrintf("specified button \"%s\" for non-button event", field),
				"NON_BUTTON");
		    }
#if SUPPORT_ADDITIONAL_MOTION_SYNTAX
		    patPtr->modMask |= TkGetButtonMask(button);
		    p = SkipFieldDelims(p);
		    while (*p && *p != '>') {
			p = SkipFieldDelims(GetField(p, field, sizeof(field)));
			if ((button = GetButtonNumber(field)) == 0) {
			    return FinalizeParseEventDescription(
				    interp,
				    patPtr, 0,
				    Tcl_ObjPrintf("bad button number \"%s\"", field), "BUTTON");
			}
			patPtr->modMask |= TkGetButtonMask(button);
		    }
		    patPtr->info = ButtonNumberFromState(patPtr->modMask);
#endif
		} else {
		    return FinalizeParseEventDescription(
			    interp,
			    patPtr, 0,
			    Tcl_ObjPrintf("specified keysym \"%s\" for non-key event", field),
			    "NON_KEY");
		}
	    } else if (eventFlags == 0) {
		return FinalizeParseEventDescription(
			interp,
			patPtr, 0,
			Tcl_NewStringObj("no event type or button # or keysym", -1), "UNMODIFIABLE");
	    } else if (patPtr->eventType == MotionNotify) {
		patPtr->info = ButtonNumberFromState(patPtr->modMask);
	    }

	    p = SkipFieldDelims(p);

	    if (*p != '>') {
		while (*p) {
		    ++p;
		    if (*p == '>') {
			return FinalizeParseEventDescription(
				interp,
				patPtr, 0,
				Tcl_NewStringObj("extra characters after detail in binding", -1),
				"PAST_DETAIL");
		    }
		}
		return FinalizeParseEventDescription(
			interp,
			patPtr, 0,
			Tcl_NewStringObj("missing \">\" in binding", -1), "MALFORMED");
	    }
	    ++p;
	}
    }

    *eventStringPtr = p;
    *eventMaskPtr |= eventMask;
    return FinalizeParseEventDescription(interp, patPtr, count, NULL, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * GetField --
 *
 *	Used to parse pattern descriptions. Copies up to size characters from
 *	p to copy, stopping at end of string, space, "-", ">", or whenever
 *	size is exceeded.
 *
 * Results:
 *	The return value is a pointer to the character just after the last one
 *	copied (usually "-" or space or ">", but could be anything if size was
 *	exceeded). Also places NULL-terminated string (up to size character,
 *	including NULL), at copy.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static const char *
GetField(
    const char *p,	/* Pointer to part of pattern. */
    char *copy,		/* Place to copy field. */
    unsigned size)	/* Maximum number of characters to copy. */
{
    assert(p);
    assert(copy);

    for ( ; *p && !isspace(UCHAR(*p)) && *p != '>' && *p != '-' && size > 1u; --size) {
	*copy++ = *p++;
    }
    *copy = '\0';
    return p;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetPatternObj --
 *
 *	Produce a string version of the given event, for displaying to the
 *	user.
 *
 * Results:
 *	The string is returned as a Tcl_Obj.
 *
 * Side effects:
 *	It is the caller's responsibility to arrange for the object to be
 *	released; it starts with a refCount of zero.
 *
 *---------------------------------------------------------------------------
 */

static Tcl_Obj *
GetPatternObj(
    const PatSeq *psPtr)
{
    Tcl_Obj *patternObj = Tcl_NewObj();
    unsigned i;

    assert(psPtr);

    for (i = 0; i < psPtr->numPats; ++i) {
	const TkPattern *patPtr = psPtr->pats + i;

	/*
	 * Check for simple case of an ASCII character.
	 */
	if (patPtr->eventType == KeyPress
		&& patPtr->count == 1
		&& patPtr->modMask == 0
		&& patPtr->info < 128
		&& isprint(UCHAR(patPtr->info))
		&& patPtr->info != '<'
		&& patPtr->info != ' ') {
	    char c = (char) patPtr->info;
	    Tcl_AppendToObj(patternObj, &c, 1);
	} else if (patPtr->eventType == VirtualEvent) {
	    assert(patPtr->name);
	    Tcl_AppendPrintfToObj(patternObj, "<<%s>>", patPtr->name);
	} else {
	    ModMask modMask;
	    const ModInfo *modPtr;

	    /*
	     * It's a more general event specification. First check for "Double",
	     * "Triple", "Quadruple", then modifiers, then event type, then keysym
	     * or button detail.
	     */

	    Tcl_AppendToObj(patternObj, "<", 1);

	    switch (patPtr->count) {
	    case 2: Tcl_AppendToObj(patternObj, "Double-", 7); break;
	    case 3: Tcl_AppendToObj(patternObj, "Triple-", 7); break;
	    case 4: Tcl_AppendToObj(patternObj, "Quadruple-", 10); break;
	    }

	    modMask = patPtr->modMask;
#if PRINT_SHORT_MOTION_SYNTAX
	    if (patPtr->eventType == MotionNotify) {
		modMask &= ~(ModMask)ALL_BUTTONS;
	    }
#endif

	    for (modPtr = modArray; modMask; ++modPtr) {
		if (modPtr->mask & modMask) {
		    modMask &= ~modPtr->mask;
		    Tcl_AppendPrintfToObj(patternObj, "%s-", modPtr->name);
		}
	    }

	    assert(patPtr->eventType < TK_LASTEVENT);
	    assert(((size_t) eventArrayIndex[patPtr->eventType]) < SIZE_OF_ARRAY(eventArray));
	    Tcl_AppendToObj(patternObj, eventArray[eventArrayIndex[patPtr->eventType]].name, -1);

	    if (patPtr->info) {
		switch (patPtr->eventType) {
		case KeyPress:
		case KeyRelease: {
		    const char *string = TkKeysymToString(patPtr->info);
		    if (string) {
			Tcl_AppendToObj(patternObj, "-", 1);
			Tcl_AppendToObj(patternObj, string, -1);
		    }
		    break;
		}
		case ButtonPress:
		case ButtonRelease:
		    assert(patPtr->info <= Button5);
		    Tcl_AppendPrintfToObj(patternObj, "-%d", (int) patPtr->info);
		    break;
#if PRINT_SHORT_MOTION_SYNTAX
		case MotionNotify: {
		    ModMask mask = patPtr->modMask;
		    while (mask & ALL_BUTTONS) {
			unsigned button = ButtonNumberFromState(mask);
			Tcl_AppendPrintfToObj(patternObj, "-%u", button);
			mask &= ~TkGetButtonMask(button);
		    }
		    break;
		}
#endif
		}
	    }

	    Tcl_AppendToObj(patternObj, ">", 1);
	}
    }

    return patternObj;
}

/*
 *----------------------------------------------------------------------
 *
 * TkStringToKeysym --
 *
 *	This function finds the keysym associated with a given keysym name.
 *
 * Results:
 *	The return value is the keysym that corresponds to name, or NoSymbol
 *	if there is no such keysym.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

KeySym
TkStringToKeysym(
    const char *name)		/* Name of a keysym. */
{
#ifdef REDO_KEYSYM_LOOKUP
    Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&keySymTable, name);

    if (hPtr) {
	return (KeySym) Tcl_GetHashValue(hPtr);
    }
    assert(name);
    if (strlen(name) == 1u) {
	KeySym keysym = (KeySym) (unsigned char) name[0];

	if (TkKeysymToString(keysym)) {
	    return keysym;
	}
    }
#endif /* REDO_KEYSYM_LOOKUP */
    assert(name);
    return XStringToKeysym(name);
}

/*
 *----------------------------------------------------------------------
 *
 * TkKeysymToString --
 *
 *	This function finds the keysym name associated with a given keysym.
 *
 * Results:
 *	The return value is a pointer to a static string containing the name
 *	of the given keysym, or NULL if there is no known name.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
TkKeysymToString(
    KeySym keysym)
{
#ifdef REDO_KEYSYM_LOOKUP
    Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&nameTable, (char *)keysym);

    if (hPtr) {
	return Tcl_GetHashValue(hPtr);
    }
#endif /* REDO_KEYSYM_LOOKUP */

    if (keysym > (KeySym)0x1008FFFF) {
	return NULL;
    }
    return XKeysymToString(keysym);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetBindingXEvent --
 *
 *	This function returns the XEvent associated with the currently
 *	executing binding. This function can only be invoked while a binding
 *	is executing.
 *
 * Results:
 *	Returns a pointer to the XEvent that caused the current binding code
 *	to be run.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

XEvent *
TkpGetBindingXEvent(
    Tcl_Interp *interp)		/* Interpreter. */
{
    TkWindow *winPtr = (TkWindow *) Tk_MainWindow(interp);
    BindingTable *bindPtr = winPtr->mainPtr->bindingTable;

    return &bindPtr->curEvent->xev;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpCancelWarp --
 *
 *	This function cancels an outstanding pointer warp and
 *	is called during tear down of the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkpCancelWarp(
    TkDisplay *dispPtr)
{
    assert(dispPtr);

    if (dispPtr->flags & TK_DISPLAY_IN_WARP) {
	Tcl_CancelIdleCall(DoWarp, dispPtr);
	dispPtr->flags &= ~TK_DISPLAY_IN_WARP;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDumpPS --
 *
 *	Dump given pattern sequence to stdout.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#if SUPPORT_DEBUGGING
void
TkpDumpPS(
    const PatSeq *psPtr)
{
    if (!psPtr) {
	fprintf(stdout, "<null>\n");
    } else {
	Tcl_Obj* result = GetPatternObj(psPtr);
	Tcl_IncrRefCount(result);
	fprintf(stdout, "%s", Tcl_GetString(result));
	if (psPtr->object) {
	    fprintf(stdout, " (%s)", (char *) psPtr->object);
	}
	fprintf(stdout, "\n");
	Tcl_DecrRefCount(result);
    }
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * TkpDumpPSList --
 *
 *	Dump given pattern sequence list to stdout.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#if SUPPORT_DEBUGGING
void
TkpDumpPSList(
    const PSList *psList)
{
    if (!psList) {
	fprintf(stdout, "<null>\n");
    } else {
	const PSEntry *psEntry;

	fprintf(stdout, "Dump PSList ========================================\n");
	TK_DLIST_FOREACH(psEntry, psList) {
	    TkpDumpPS(psEntry->psPtr);
	}
	fprintf(stdout, "====================================================\n");
    }
}
#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 105
 * End:
 * vi:set ts=8 sw=4:
 */
