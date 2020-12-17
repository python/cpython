/* $XConsortium: Xutil.h,v 11.73 91/07/30 16:21:37 rws Exp $ */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef _XUTIL_H_
#define _XUTIL_H_

/* You must include <X11/Xlib.h> before including this file */

#if defined(MAC_OSX_TK)
#   define Region XRegion
#endif

/*
 * Bitmask returned by XParseGeometry().  Each bit tells if the corresponding
 * value (x, y, width, height) was found in the parsed string.
 */
#define NoValue		0x0000
#define XValue  	0x0001
#define YValue		0x0002
#define WidthValue  	0x0004
#define HeightValue  	0x0008
#define AllValues 	0x000F
#define XNegative 	0x0010
#define YNegative 	0x0020

/*
 * new version containing base_width, base_height, and win_gravity fields;
 * used with WM_NORMAL_HINTS.
 */
typedef struct {
    	long flags;	/* marks which fields in this structure are defined */
	int x, y;		/* obsolete for new window mgrs, but clients */
	int width, height;	/* should set so old wm's don't mess up */
	int min_width, min_height;
	int max_width, max_height;
    	int width_inc, height_inc;
	struct {
		int x;	/* numerator */
		int y;	/* denominator */
	} min_aspect, max_aspect;
	int base_width, base_height;		/* added by ICCCM version 1 */
	int win_gravity;			/* added by ICCCM version 1 */
} XSizeHints;

/*
 * The next block of definitions are for window manager properties that
 * clients and applications use for communication.
 */

/* flags argument in size hints */
#define USPosition	(1L << 0) /* user specified x, y */
#define USSize		(1L << 1) /* user specified width, height */

#define PPosition	(1L << 2) /* program specified position */
#define PSize		(1L << 3) /* program specified size */
#define PMinSize	(1L << 4) /* program specified minimum size */
#define PMaxSize	(1L << 5) /* program specified maximum size */
#define PResizeInc	(1L << 6) /* program specified resize increments */
#define PAspect		(1L << 7) /* program specified min and max aspect ratios */
#define PBaseSize	(1L << 8) /* program specified base for incrementing */
#define PWinGravity	(1L << 9) /* program specified window gravity */

/* obsolete */
#define PAllHints (PPosition|PSize|PMinSize|PMaxSize|PResizeInc|PAspect)



typedef struct {
	long flags;	/* marks which fields in this structure are defined */
	Bool input;	/* does this application rely on the window manager to
			get keyboard input? */
	int initial_state;	/* see below */
	Pixmap icon_pixmap;	/* pixmap to be used as icon */
	Window icon_window; 	/* window to be used as icon */
	int icon_x, icon_y; 	/* initial position of icon */
	Pixmap icon_mask;	/* icon mask bitmap */
	XID window_group;	/* id of related window group */
	/* this structure may be extended in the future */
} XWMHints;

/* definition for flags of XWMHints */

#define InputHint 		(1L << 0)
#define StateHint 		(1L << 1)
#define IconPixmapHint		(1L << 2)
#define IconWindowHint		(1L << 3)
#define IconPositionHint 	(1L << 4)
#define IconMaskHint		(1L << 5)
#define WindowGroupHint		(1L << 6)
#define AllHints (InputHint|StateHint|IconPixmapHint|IconWindowHint| \
IconPositionHint|IconMaskHint|WindowGroupHint)

/* definitions for initial window state */
#define WithdrawnState 0	/* for windows that are not mapped */
#define NormalState 1	/* most applications want to start this way */
#define IconicState 3	/* application wants to start as an icon */

/*
 * Obsolete states no longer defined by ICCCM
 */
#define DontCareState 0	/* don't know or care */
#define ZoomState 2	/* application wants to start zoomed */
#define InactiveState 4	/* application believes it is seldom used; */
			/* some wm's may put it on inactive menu */


/*
 * new structure for manipulating TEXT properties; used with WM_NAME,
 * WM_ICON_NAME, WM_CLIENT_MACHINE, and WM_COMMAND.
 */
typedef struct {
    unsigned char *value;		/* same as Property routines */
    Atom encoding;			/* prop type */
    int format;				/* prop data format: 8, 16, or 32 */
    unsigned long nitems;		/* number of data items in value */
} XTextProperty;

#define XNoMemory -1
#define XLocaleNotSupported -2
#define XConverterNotFound -3

typedef enum {
    XStringStyle,		/* STRING */
    XCompoundTextStyle,		/* COMPOUND_TEXT */
    XTextStyle,			/* text in owner's encoding (current locale)*/
    XStdICCTextStyle		/* STRING, else COMPOUND_TEXT */
} XICCEncodingStyle;

typedef struct {
	int min_width, min_height;
	int max_width, max_height;
	int width_inc, height_inc;
} XIconSize;

typedef struct {
	char *res_name;
	char *res_class;
} XClassHint;

/*
 * These macros are used to give some sugar to the image routines so that
 * naive people are more comfortable with them.
 */
#define XDestroyImage(ximage) \
	((*((ximage)->f.destroy_image))((ximage)))
#define XGetPixel(ximage, x, y) \
	((*((ximage)->f.get_pixel))((ximage), (x), (y)))
#define XPutPixel(ximage, x, y, pixel) \
	((*((ximage)->f.put_pixel))((ximage), (x), (y), (pixel)))
#define XSubImage(ximage, x, y, width, height)  \
	((*((ximage)->f.sub_image))((ximage), (x), (y), (width), (height)))
#define XAddPixel(ximage, value) \
	((*((ximage)->f.add_pixel))((ximage), (value)))

/*
 * Compose sequence status structure, used in calling XLookupString.
 */
typedef struct _XComposeStatus {
    XPointer compose_ptr;	/* state table pointer */
    int chars_matched;		/* match state */
} XComposeStatus;

/*
 * Keysym macros, used on Keysyms to test for classes of symbols
 */
#define IsKeypadKey(keysym) \
  (((unsigned)(keysym) >= XK_KP_Space) && ((unsigned)(keysym) <= XK_KP_Equal))

#define IsCursorKey(keysym) \
  (((unsigned)(keysym) >= XK_Home)     && ((unsigned)(keysym) <  XK_Select))

#define IsPFKey(keysym) \
  (((unsigned)(keysym) >= XK_KP_F1)     && ((unsigned)(keysym) <= XK_KP_F4))

#define IsFunctionKey(keysym) \
  (((unsigned)(keysym) >= XK_F1)       && ((unsigned)(keysym) <= XK_F35))

#define IsMiscFunctionKey(keysym) \
  (((unsigned)(keysym) >= XK_Select)   && ((unsigned)(keysym) <= XK_Break))

#define IsModifierKey(keysym) \
  ((((unsigned)(keysym) >= XK_Shift_L) && ((unsigned)(keysym) <= XK_Hyper_R)) \
   || ((unsigned)(keysym) == XK_Mode_switch) \
   || ((unsigned)(keysym) == XK_Num_Lock))
/*
 * opaque reference to Region data type
 */
typedef struct _XRegion *Region;

/* Return values from XRectInRegion() */

#define RectangleOut 0
#define RectangleIn  1
#define RectanglePart 2


/*
 * Information used by the visual utility routines to find desired visual
 * type from the many visuals a display may support.
 */

typedef struct {
  Visual *visual;
  VisualID visualid;
  int screen;
  int depth;
#if defined(__cplusplus) || defined(c_plusplus)
  int c_class;					/* C++ */
#else
  int class;
#endif
  unsigned long red_mask;
  unsigned long green_mask;
  unsigned long blue_mask;
  int colormap_size;
  int bits_per_rgb;
} XVisualInfo;

#define VisualNoMask		0x0
#define VisualIDMask 		0x1
#define VisualScreenMask	0x2
#define VisualDepthMask		0x4
#define VisualClassMask		0x8
#define VisualRedMaskMask	0x10
#define VisualGreenMaskMask	0x20
#define VisualBlueMaskMask	0x40
#define VisualColormapSizeMask	0x80
#define VisualBitsPerRGBMask	0x100
#define VisualAllMask		0x1FF

/*
 * This defines a window manager property that clients may use to
 * share standard color maps of type RGB_COLOR_MAP:
 */
typedef struct {
	Colormap colormap;
	unsigned long red_max;
	unsigned long red_mult;
	unsigned long green_max;
	unsigned long green_mult;
	unsigned long blue_max;
	unsigned long blue_mult;
	unsigned long base_pixel;
	VisualID visualid;		/* added by ICCCM version 1 */
	XID killid;			/* added by ICCCM version 1 */
} XStandardColormap;

#define ReleaseByFreeingColormap ((XID) 1L)  /* for killid field above */


/*
 * return codes for XReadBitmapFile and XWriteBitmapFile
 */
#define BitmapSuccess		0
#define BitmapOpenFailed 	1
#define BitmapFileInvalid 	2
#define BitmapNoMemory		3

/****************************************************************
 *
 * Context Management
 *
 ****************************************************************/


/* Associative lookup table return codes */

#define XCSUCCESS 0	/* No error. */
#define XCNOMEM   1    /* Out of memory */
#define XCNOENT   2    /* No entry in table */

typedef int XContext;

#define XUniqueContext()       ((XContext) XrmUniqueQuark())
#define XStringToContext(string)   ((XContext) XrmStringToQuark(string))

_XFUNCPROTOBEGIN

/* The following declarations are alphabetized. */

extern XClassHint *XAllocClassHint (
#if NeedFunctionPrototypes
    void
#endif
);

extern XIconSize *XAllocIconSize (
#if NeedFunctionPrototypes
    void
#endif
);

extern XSizeHints *XAllocSizeHints (
#if NeedFunctionPrototypes
    void
#endif
);

extern XStandardColormap *XAllocStandardColormap (
#if NeedFunctionPrototypes
    void
#endif
);

extern XWMHints *XAllocWMHints (
#if NeedFunctionPrototypes
    void
#endif
);

extern void XClipBox(
#if NeedFunctionPrototypes
    Region		/* r */,
    XRectangle*		/* rect_return */
#endif
);

extern Region XCreateRegion(
#if NeedFunctionPrototypes
    void
#endif
);

extern char *XDefaultString(
#if NeedFunctionPrototypes
    void
#endif
);

extern int XDeleteContext(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XID			/* rid */,
    XContext		/* context */
#endif
);

extern void XDestroyRegion(
#if NeedFunctionPrototypes
    Region		/* r */
#endif
);

extern Bool XEmptyRegion(
#if NeedFunctionPrototypes
    Region		/* r */
#endif
);

extern Bool XEqualRegion(
#if NeedFunctionPrototypes
    Region		/* r1 */,
    Region		/* r2 */
#endif
);

extern int XFindContext(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XID			/* rid */,
    XContext		/* context */,
    XPointer*		/* data_return */
#endif
);

extern Status XGetClassHint(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XClassHint*		/* class_hints_return */
#endif
);

extern Status XGetIconSizes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XIconSize**		/* size_list_return */,
    int*		/* count_return */
#endif
);

extern Status XGetNormalHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints_return */
#endif
);

extern Status XGetRGBColormaps(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XStandardColormap** /* stdcmap_return */,
    int*		/* count_return */,
    Atom		/* property */
#endif
);

extern Status XGetSizeHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints_return */,
    Atom		/* property */
#endif
);

extern Status XGetStandardColormap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XStandardColormap*	/* colormap_return */,
    Atom		/* property */
#endif
);

extern Status XGetTextProperty(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* window */,
    XTextProperty*	/* text_prop_return */,
    Atom		/* property */
#endif
);


extern Status XGetWMClientMachine(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop_return */
#endif
);

extern XWMHints *XGetWMHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern Status XGetWMIconName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop_return */
#endif
);

extern Status XGetWMName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop_return */
#endif
);

extern Status XGetWMNormalHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints_return */,
    long*		/* supplied_return */
#endif
);

extern Status XGetWMSizeHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints_return */,
    long*		/* supplied_return */,
    Atom		/* property */
#endif
);

extern Status XGetZoomHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* zhints_return */
#endif
);

extern void XIntersectRegion(
#if NeedFunctionPrototypes
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
#endif
);

extern int XLookupString(
#if NeedFunctionPrototypes
    XKeyEvent*		/* event_struct */,
    char*		/* buffer_return */,
    int			/* bytes_buffer */,
    KeySym*		/* keysym_return */,
    XComposeStatus*	/* status_in_out */
#endif
);

extern Status XMatchVisualInfo(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen */,
    int			/* depth */,
    int			/* class */,
    XVisualInfo*	/* vinfo_return */
#endif
);

extern int XOffsetRegion(
#if NeedFunctionPrototypes
    Region		/* r */,
    int			/* dx */,
    int			/* dy */
#endif
);

extern Bool XPointInRegion(
#if NeedFunctionPrototypes
    Region		/* r */,
    int			/* x */,
    int			/* y */
#endif
);

extern Region XPolygonRegion(
#if NeedFunctionPrototypes
    XPoint*		/* points */,
    int			/* n */,
    int			/* fill_rule */
#endif
);

extern int XRectInRegion(
#if NeedFunctionPrototypes
    Region		/* r */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */
#endif
);

extern int XSaveContext(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XID			/* rid */,
    XContext		/* context */,
    _Xconst char*	/* data */
#endif
);

extern void XSetClassHint(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XClassHint*		/* class_hints */
#endif
);

extern void XSetIconSizes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XIconSize*		/* size_list */,
    int			/* count */
#endif
);

extern void XSetNormalHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints */
#endif
);

extern void XSetRGBColormaps(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XStandardColormap*	/* stdcmaps */,
    int			/* count */,
    Atom		/* property */
#endif
);

extern void XSetSizeHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints */,
    Atom		/* property */
#endif
);

extern void XSetStandardProperties(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    _Xconst char*	/* window_name */,
    _Xconst char*	/* icon_name */,
    Pixmap		/* icon_pixmap */,
    char**		/* argv */,
    int			/* argc */,
    XSizeHints*		/* hints */
#endif
);

extern void XSetTextProperty(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop */,
    Atom		/* property */
#endif
);

extern void XSetWMHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XWMHints*		/* wm_hints */
#endif
);

extern void XSetWMIconName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop */
#endif
);

extern void XSetWMName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop */
#endif
);

extern void XSetWMNormalHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints */
#endif
);

extern void XSetWMProperties(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* window_name */,
    XTextProperty*	/* icon_name */,
    char**		/* argv */,
    int			/* argc */,
    XSizeHints*		/* normal_hints */,
    XWMHints*		/* wm_hints */,
    XClassHint*		/* class_hints */
#endif
);

extern void XmbSetWMProperties(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    _Xconst char*	/* window_name */,
    _Xconst char*	/* icon_name */,
    char**		/* argv */,
    int			/* argc */,
    XSizeHints*		/* normal_hints */,
    XWMHints*		/* wm_hints */,
    XClassHint*		/* class_hints */
#endif
);

extern void XSetWMSizeHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints */,
    Atom		/* property */
#endif
);

extern void XSetRegion(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    Region		/* r */
#endif
);

extern void XSetStandardColormap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XStandardColormap*	/* colormap */,
    Atom		/* property */
#endif
);

extern void XSetZoomHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* zhints */
#endif
);

extern void XShrinkRegion(
#if NeedFunctionPrototypes
    Region		/* r */,
    int			/* dx */,
    int			/* dy */
#endif
);

extern void XSubtractRegion(
#if NeedFunctionPrototypes
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
#endif
);

extern int XmbTextListToTextProperty(
#if NeedFunctionPrototypes
    Display*		/* display */,
    char**		/* list */,
    int			/* count */,
    XICCEncodingStyle	/* style */,
    XTextProperty*	/* text_prop_return */
#endif
);

extern int XwcTextListToTextProperty(
#if NeedFunctionPrototypes
    Display*		/* display */,
    wchar_t**		/* list */,
    int			/* count */,
    XICCEncodingStyle	/* style */,
    XTextProperty*	/* text_prop_return */
#endif
);

extern void XwcFreeStringList(
#if NeedFunctionPrototypes
    wchar_t**		/* list */
#endif
);

extern Status XTextPropertyToStringList(
#if NeedFunctionPrototypes
    XTextProperty*	/* text_prop */,
    char***		/* list_return */,
    int*		/* count_return */
#endif
);

extern int XmbTextPropertyToTextList(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XTextProperty*	/* text_prop */,
    char***		/* list_return */,
    int*		/* count_return */
#endif
);

extern int XwcTextPropertyToTextList(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XTextProperty*	/* text_prop */,
    wchar_t***		/* list_return */,
    int*		/* count_return */
#endif
);

extern void XUnionRectWithRegion(
#if NeedFunctionPrototypes
    XRectangle*		/* rectangle */,
    Region		/* src_region */,
    Region		/* dest_region_return */
#endif
);

extern int XUnionRegion(
#if NeedFunctionPrototypes
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
#endif
);

extern int XWMGeometry(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */,
    _Xconst char*	/* user_geometry */,
    _Xconst char*	/* default_geometry */,
    unsigned int	/* border_width */,
    XSizeHints*		/* hints */,
    int*		/* x_return */,
    int*		/* y_return */,
    int*		/* width_return */,
    int*		/* height_return */,
    int*		/* gravity_return */
#endif
);

extern void XXorRegion(
#if NeedFunctionPrototypes
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
#endif
);

_XFUNCPROTOEND

#if defined(MAC_OSX_TK)
#   undef Region
#endif

#endif /* _XUTIL_H_ */
