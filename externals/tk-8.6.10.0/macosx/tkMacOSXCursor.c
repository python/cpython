/*
 * tkMacOSXCursor.c --
 *
 *	This file contains Macintosh specific cursor related routines.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkMacOSXCursors.h"
#include "tkMacOSXXCursors.h"

/*
 * Mac Cursor Types.
 */

#define NONE		-1	/* Hidden cursor */
#define SELECTOR	1	/* NSCursor class method */
#define IMAGENAMED	2	/* Named NSImage */
#define IMAGEPATH	3	/* Path to NSImage */
#define IMAGEBITMAP	4	/* Pointer to 16x16 cursor bitmap data */

#define pix		16	/* Pixel width & height of cursor bitmap data */

/*
 * The following data structure contains the system specific data necessary to
 * control Windows cursors.
 */

typedef struct {
    TkCursor info;		/* Generic cursor info used by tkCursor.c */
    NSCursor *macCursor;	/* Macintosh cursor */
    int type;			/* Type of Mac cursor */
} TkMacOSXCursor;

/*
 * The table below is used to map from the name of a predefined cursor
 * to a NSCursor.
 */

struct CursorName {
    const char *name;
    const int kind;
    id id1, id2;
    NSPoint hotspot;
};

#define MacCursorData(n)	((id)tkMacOSXCursors[TK_MAC_CURSOR_##n])
#define MacXCursorData(n)	((id)tkMacOSXXCursors[TK_MAC_XCURSOR_##n])

static const struct CursorName cursorNames[] = {
    {"none",			NONE,	    nil},
    {"arrow",			SELECTOR,    @"arrowCursor"},
    {"top_left_arrow",		SELECTOR,    @"arrowCursor"},
    {"left_ptr",		SELECTOR,    @"arrowCursor"},
    {"copyarrow",		SELECTOR,    @"dragCopyCursor", @"_copyDragCursor"},
    {"aliasarrow",		SELECTOR,    @"dragLinkCursor", @"_linkDragCursor"},
    {"contextualmenuarrow",	SELECTOR,    @"contextualMenuCursor"},
    {"movearrow",		SELECTOR,    @"_moveCursor"},
    {"ibeam",			SELECTOR,    @"IBeamCursor"},
    {"text",			SELECTOR,    @"IBeamCursor"},
    {"xterm",			SELECTOR,    @"IBeamCursor"},
    {"cross",			SELECTOR,    @"crosshairCursor"},
    {"crosshair",		SELECTOR,    @"crosshairCursor"},
    {"cross-hair",		SELECTOR,    @"crosshairCursor"},
    {"tcross",			SELECTOR,    @"crosshairCursor"},
    {"hand",			SELECTOR,    @"openHandCursor"},
    {"openhand",		SELECTOR,    @"openHandCursor"},
    {"closedhand",		SELECTOR,    @"closedHandCursor"},
    {"fist",			SELECTOR,    @"closedHandCursor"},
    {"pointinghand",		SELECTOR,    @"pointingHandCursor"},
    {"resize",			SELECTOR,    @"arrowCursor"},
    {"resizeleft",		SELECTOR,    @"resizeLeftCursor"},
    {"resizeright",		SELECTOR,    @"resizeRightCursor"},
    {"resizeleftright",		SELECTOR,    @"resizeLeftRightCursor"},
    {"resizeup",		SELECTOR,    @"resizeUpCursor"},
    {"resizedown",		SELECTOR,    @"resizeDownCursor"},
    {"resizeupdown",		SELECTOR,    @"resizeUpDownCursor"},
    {"resizebottomleft",	SELECTOR,    @"_bottomLeftResizeCursor"},
    {"resizetopleft",		SELECTOR,    @"_topLeftResizeCursor"},
    {"resizebottomright",	SELECTOR,    @"_bottomRightResizeCursor"},
    {"resizetopright",		SELECTOR,    @"_topRightResizeCursor"},
    {"notallowed",		SELECTOR,    @"operationNotAllowedCursor"},
    {"poof",			SELECTOR,    @"disappearingItemCursor"},
    {"wait",			SELECTOR,    @"busyButClickableCursor"},
    {"spinning",		SELECTOR,    @"busyButClickableCursor"},
    {"countinguphand",		SELECTOR,    @"busyButClickableCursor"},
    {"countingdownhand",	SELECTOR,    @"busyButClickableCursor"},
    {"countingupanddownhand",	SELECTOR,    @"busyButClickableCursor"},
    {"help",			IMAGENAMED,  @"NSHelpCursor", nil, {8, 8}},
//  {"hand",			IMAGEBITMAP, MacCursorData(hand)},
    {"bucket",			IMAGEBITMAP, MacCursorData(bucket)},
    {"cancel",			IMAGEBITMAP, MacCursorData(cancel)},
//  {"resize",			IMAGEBITMAP, MacCursorData(resize)},
    {"eyedrop",			IMAGEBITMAP, MacCursorData(eyedrop)},
    {"eyedrop-full",		IMAGEBITMAP, MacCursorData(eyedrop_full)},
    {"zoom-in",			IMAGEBITMAP, MacCursorData(zoom_in)},
    {"zoom-out",		IMAGEBITMAP, MacCursorData(zoom_out)},
    {"X_cursor",		IMAGEBITMAP, MacXCursorData(X_cursor)},
//  {"arrow",			IMAGEBITMAP, MacXCursorData(arrow)},
    {"based_arrow_down",	IMAGEBITMAP, MacXCursorData(based_arrow_down)},
    {"based_arrow_up",		IMAGEBITMAP, MacXCursorData(based_arrow_up)},
    {"boat",			IMAGEBITMAP, MacXCursorData(boat)},
    {"bogosity",		IMAGEBITMAP, MacXCursorData(bogosity)},
    {"bottom_left_corner",	IMAGEBITMAP, MacXCursorData(bottom_left_corner)},
    {"bottom_right_corner",	IMAGEBITMAP, MacXCursorData(bottom_right_corner)},
    {"bottom_side",		IMAGEBITMAP, MacXCursorData(bottom_side)},
    {"bottom_tee",		IMAGEBITMAP, MacXCursorData(bottom_tee)},
    {"box_spiral",		IMAGEBITMAP, MacXCursorData(box_spiral)},
    {"center_ptr",		IMAGEBITMAP, MacXCursorData(center_ptr)},
    {"circle",			IMAGEBITMAP, MacXCursorData(circle)},
    {"clock",			IMAGEBITMAP, MacXCursorData(clock)},
    {"coffee_mug",		IMAGEBITMAP, MacXCursorData(coffee_mug)},
//  {"cross",			IMAGEBITMAP, MacXCursorData(cross)},
    {"cross_reverse",		IMAGEBITMAP, MacXCursorData(cross_reverse)},
//  {"crosshair",		IMAGEBITMAP, MacXCursorData(crosshair)},
    {"diamond_cross",		IMAGEBITMAP, MacXCursorData(diamond_cross)},
    {"dot",			IMAGEBITMAP, MacXCursorData(dot)},
    {"dotbox",			IMAGEBITMAP, MacXCursorData(dotbox)},
    {"double_arrow",		IMAGEBITMAP, MacXCursorData(double_arrow)},
    {"draft_large",		IMAGEBITMAP, MacXCursorData(draft_large)},
    {"draft_small",		IMAGEBITMAP, MacXCursorData(draft_small)},
    {"draped_box",		IMAGEBITMAP, MacXCursorData(draped_box)},
    {"exchange",		IMAGEBITMAP, MacXCursorData(exchange)},
    {"fleur",			IMAGEBITMAP, MacXCursorData(fleur)},
    {"gobbler",			IMAGEBITMAP, MacXCursorData(gobbler)},
    {"gumby",			IMAGEBITMAP, MacXCursorData(gumby)},
    {"hand1",			IMAGEBITMAP, MacXCursorData(hand1)},
    {"hand2",			IMAGEBITMAP, MacXCursorData(hand2)},
    {"heart",			IMAGEBITMAP, MacXCursorData(heart)},
    {"icon",			IMAGEBITMAP, MacXCursorData(icon)},
    {"iron_cross",		IMAGEBITMAP, MacXCursorData(iron_cross)},
//  {"left_ptr",		IMAGEBITMAP, MacXCursorData(left_ptr)},
    {"left_side",		IMAGEBITMAP, MacXCursorData(left_side)},
    {"left_tee",		IMAGEBITMAP, MacXCursorData(left_tee)},
    {"leftbutton",		IMAGEBITMAP, MacXCursorData(leftbutton)},
    {"ll_angle",		IMAGEBITMAP, MacXCursorData(ll_angle)},
    {"lr_angle",		IMAGEBITMAP, MacXCursorData(lr_angle)},
    {"man",			IMAGEBITMAP, MacXCursorData(man)},
    {"middlebutton",		IMAGEBITMAP, MacXCursorData(middlebutton)},
    {"mouse",			IMAGEBITMAP, MacXCursorData(mouse)},
    {"pencil",			IMAGEBITMAP, MacXCursorData(pencil)},
    {"pirate",			IMAGEBITMAP, MacXCursorData(pirate)},
    {"plus",			IMAGEBITMAP, MacXCursorData(plus)},
    {"question_arrow",		IMAGEBITMAP, MacXCursorData(question_arrow)},
    {"right_ptr",		IMAGEBITMAP, MacXCursorData(right_ptr)},
    {"right_side",		IMAGEBITMAP, MacXCursorData(right_side)},
    {"right_tee",		IMAGEBITMAP, MacXCursorData(right_tee)},
    {"rightbutton",		IMAGEBITMAP, MacXCursorData(rightbutton)},
    {"rtl_logo",		IMAGEBITMAP, MacXCursorData(rtl_logo)},
    {"sailboat",		IMAGEBITMAP, MacXCursorData(sailboat)},
    {"sb_down_arrow",		IMAGEBITMAP, MacXCursorData(sb_down_arrow)},
    {"sb_h_double_arrow",	IMAGEBITMAP, MacXCursorData(sb_h_double_arrow)},
    {"sb_left_arrow",		IMAGEBITMAP, MacXCursorData(sb_left_arrow)},
    {"sb_right_arrow",		IMAGEBITMAP, MacXCursorData(sb_right_arrow)},
    {"sb_up_arrow",		IMAGEBITMAP, MacXCursorData(sb_up_arrow)},
    {"sb_v_double_arrow",	IMAGEBITMAP, MacXCursorData(sb_v_double_arrow)},
    {"shuttle",			IMAGEBITMAP, MacXCursorData(shuttle)},
    {"sizing",			IMAGEBITMAP, MacXCursorData(sizing)},
    {"spider",			IMAGEBITMAP, MacXCursorData(spider)},
    {"spraycan",		IMAGEBITMAP, MacXCursorData(spraycan)},
    {"star",			IMAGEBITMAP, MacXCursorData(star)},
    {"target",			IMAGEBITMAP, MacXCursorData(target)},
//  {"tcross",			IMAGEBITMAP, MacXCursorData(tcross)},
//  {"top_left_arrow",		IMAGEBITMAP, MacXCursorData(top_left_arrow)},
    {"top_left_corner",		IMAGEBITMAP, MacXCursorData(top_left_corner)},
    {"top_right_corner",	IMAGEBITMAP, MacXCursorData(top_right_corner)},
    {"top_side",		IMAGEBITMAP, MacXCursorData(top_side)},
    {"top_tee",			IMAGEBITMAP, MacXCursorData(top_tee)},
    {"trek",			IMAGEBITMAP, MacXCursorData(trek)},
    {"ul_angle",		IMAGEBITMAP, MacXCursorData(ul_angle)},
    {"umbrella",		IMAGEBITMAP, MacXCursorData(umbrella)},
    {"ur_angle",		IMAGEBITMAP, MacXCursorData(ur_angle)},
    {"watch",			IMAGEBITMAP, MacXCursorData(watch)},
//  {"xterm",			IMAGEBITMAP, MacXCursorData(xterm)},
    {NULL}
};

/*
 * Declarations of static variables used in this file.
 */

static TkMacOSXCursor *gCurrentCursor = NULL;
				/* A pointer to the current cursor. */
static int gResizeOverride = false;
				/* A boolean indicating whether we should use
				 * the resize cursor during installations. */
static int gTkOwnsCursor = true;/* A boolean indicating whether Tk owns the
				 * cursor. If not (for instance, in the case
				 * where a Tk window is embedded in another
				 * app's window, and the cursor is out of the
				 * Tk window, we will not attempt to adjust
				 * the cursor. */

/*
 * Declarations of procedures local to this file
 */

static void FindCursorByName(TkMacOSXCursor *macCursorPtr, const char *string);

/*
 *----------------------------------------------------------------------
 *
 * FindCursorByName --
 *
 *	Retrieve a system cursor by name, and fill the macCursorPtr
 *	structure. If the cursor cannot be found, the macCursor field
 *	will be nil.
 *
 * Results:
 *	Fills the macCursorPtr record.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

void
FindCursorByName(
    TkMacOSXCursor *macCursorPtr,
    const char *name)
{
    NSString *path = nil;
    NSImage *image = nil;
    NSPoint hotSpot = NSZeroPoint;
    int haveHotSpot = 0, result = TCL_ERROR;
    NSCursor *macCursor = nil;

    if (name[0] == '@') {
	/*
	 * System cursor of type @filename
	 */

	macCursorPtr->type = IMAGEPATH;
	path = [NSString stringWithUTF8String:&name[1]];
    } else {
	Tcl_Obj *strPtr = Tcl_NewStringObj(name, -1);
	int idx;

	result = Tcl_GetIndexFromObjStruct(NULL, strPtr, cursorNames,
		    sizeof(struct CursorName), NULL, TCL_EXACT, &idx);
	Tcl_DecrRefCount(strPtr);
	if (result == TCL_OK) {
	    macCursorPtr->type = cursorNames[idx].kind;
	    switch (cursorNames[idx].kind) {
	    case SELECTOR: {
		SEL selector = NSSelectorFromString(cursorNames[idx].id1);
		if ([NSCursor respondsToSelector:selector]) {
		    macCursor = [[NSCursor performSelector:selector] retain];
		} else if (cursorNames[idx].id2) {
		    selector = NSSelectorFromString(cursorNames[idx].id2);
		    if ([NSCursor respondsToSelector:selector]) {
			macCursor = [[NSCursor performSelector:selector] retain];
		    }
		}
		break;
	    }
	    case IMAGENAMED:
		image = [[NSImage imageNamed:cursorNames[idx].id1] retain];
		hotSpot = cursorNames[idx].hotspot;
		haveHotSpot = 1;
		break;
	    case IMAGEPATH:
		path = [NSApp tkFrameworkImagePath:cursorNames[idx].id1];
		break;
	    case IMAGEBITMAP: {
		unsigned char *bitmap = (unsigned char *)(cursorNames[idx].id1);
		NSBitmapImageRep *bitmapImageRep = NULL;
		CGImageRef img = NULL, mask = NULL, maskedImg = NULL;
		static const CGFloat decodeWB[] = {1, 0};
		CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(
			kCGColorSpaceGenericGray);
		CGDataProviderRef provider = CGDataProviderCreateWithData(NULL,
			bitmap, pix*pix/8, NULL);

		if (provider) {
		    img = CGImageCreate(pix, pix, 1, 1, pix/8, colorspace,
			    kCGBitmapByteOrderDefault, provider, decodeWB, 0,
			    kCGRenderingIntentDefault);
		    CFRelease(provider);
		}
		provider = CGDataProviderCreateWithData(NULL, bitmap +
			pix*pix/8, pix*pix/8, NULL);
		if (provider) {
		    mask = CGImageMaskCreate(pix, pix, 1, 1, pix/8, provider,
			    decodeWB, 0);
		    CFRelease(provider);
		}
		if (img && mask) {
		    maskedImg = CGImageCreateWithMask(img, mask);
		}
		if (maskedImg) {
		    bitmapImageRep = [[NSBitmapImageRep alloc]
			    initWithCGImage:maskedImg];
		    CFRelease(maskedImg);
		}
		if (mask) {
		    CFRelease(mask);
		}
		if (img) {
		    CFRelease(img);
		}
		if (colorspace) {
		    CFRelease(colorspace);
		}
		if (bitmapImageRep) {
		    image = [[NSImage alloc] initWithSize:NSMakeSize(pix, pix)];
		    [image addRepresentation:bitmapImageRep];
		    [bitmapImageRep release];
		}

		uint16_t *hotSpotData = (uint16_t*)(bitmap + 2*pix*pix/8);
		hotSpot.y = CFSwapInt16BigToHost(*hotSpotData++);
		hotSpot.x = CFSwapInt16BigToHost(*hotSpotData);
		haveHotSpot = 1;
		break;
	    }
	    }
	}
    }
    if (path) {
	image = [[NSImage alloc] initWithContentsOfFile:path];
    }
    if (!image && !macCursor && result != TCL_OK) {
	macCursorPtr->type = IMAGENAMED;
	image = [[NSImage imageNamed:[NSString stringWithUTF8String:name]]
		retain];
	haveHotSpot = 0;
    }
    if (image) {
	if (!haveHotSpot && [[path pathExtension] isEqualToString:@"cur"]) {
	    NSData *data = [NSData dataWithContentsOfFile:path];
	    if ([data length] > 14) {
		uint16_t *hotSpotData = (uint16_t*)((char*) [data bytes] + 10);
		hotSpot.x = CFSwapInt16LittleToHost(*hotSpotData++);
		hotSpot.y = CFSwapInt16LittleToHost(*hotSpotData);
		haveHotSpot = 1;
	    }
	}
	if (!haveHotSpot) {
	    NSSize size = [image size];
	    hotSpot.x = size.width * 0.5;
	    hotSpot.y = size.height * 0.5;
	}
	hotSpot.y = -hotSpot.y;
	macCursor = [[NSCursor alloc] initWithImage:image hotSpot:hotSpot];
	[image release];
    }
    macCursorPtr->macCursor = macCursor;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetCursorByName --
 *
 *	Retrieve a system cursor by name.
 *
 * Results:
 *	Returns a new cursor, or NULL on errors.
 *
 * Side effects:
 *	Allocates a new cursor.
 *
 *----------------------------------------------------------------------
 */

TkCursor *
TkGetCursorByName(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    Tk_Window tkwin,		/* Window in which cursor will be used. */
    Tk_Uid string)		/* Description of cursor. See manual entry
				 * for details on legal syntax. */
{
    TkMacOSXCursor *macCursorPtr = NULL;
    const char **argv = NULL;
    int argc;

    /*
     * All cursor names are valid lists of one element (for
     * TkX11-compatibility), even unadorned system cursor names.
     */

    if (Tcl_SplitList(interp, string, &argc, &argv) == TCL_OK) {
	if (argc) {
	    macCursorPtr = ckalloc(sizeof(TkMacOSXCursor));
	    macCursorPtr->info.cursor = (Tk_Cursor) macCursorPtr;
	    macCursorPtr->macCursor = nil;
	    macCursorPtr->type = 0;
	    FindCursorByName(macCursorPtr, argv[0]);
	}
	ckfree(argv);
    }
    if (!macCursorPtr || (!macCursorPtr->macCursor &&
	    macCursorPtr->type != NONE)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad cursor spec \"%s\"", string));
	Tcl_SetErrorCode(interp, "TK", "VALUE", "CURSOR", NULL);
	if (macCursorPtr) {
	    ckfree(macCursorPtr);
	    macCursorPtr = NULL;
	}
    }
    return (TkCursor *) macCursorPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkCreateCursorFromData --
 *
 *	Creates a cursor from the source and mask bits.
 *
 * Results:
 *	Returns a new cursor, or NULL on errors.
 *
 * Side effects:
 *	Allocates a new cursor.
 *
 *----------------------------------------------------------------------
 */

TkCursor *
TkCreateCursorFromData(
    Tk_Window tkwin,		/* Window in which cursor will be used. */
    const char *source,		/* Bitmap data for cursor shape. */
    const char *mask,		/* Bitmap data for cursor mask. */
    int width, int height,	/* Dimensions of cursor. */
    int xHot, int yHot,		/* Location of hot-spot in cursor. */
    XColor fgColor,		/* Foreground color for cursor. */
    XColor bgColor)		/* Background color for cursor. */
{
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpFreeCursor --
 *
 *	This procedure is called to release a cursor allocated by
 *	TkGetCursorByName.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor data structure is deallocated.
 *
 *----------------------------------------------------------------------
 */

void
TkpFreeCursor(
    TkCursor *cursorPtr)
{
    TkMacOSXCursor *macCursorPtr = (TkMacOSXCursor *) cursorPtr;

    [macCursorPtr->macCursor release];
    macCursorPtr->macCursor = NULL;
    if (macCursorPtr == gCurrentCursor) {
	gCurrentCursor = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXInstallCursor --
 *
 *	Installs either the current cursor as defined by TkpSetCursor or a
 *	resize cursor as the cursor the Macintosh should currently display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the Macintosh mouse cursor.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXInstallCursor(
    int resizeOverride)
{
    TkMacOSXCursor *macCursorPtr = gCurrentCursor;
    static int cursorHidden = 0;
    int cursorNone = 0;

    gResizeOverride = resizeOverride;

    if (resizeOverride || !macCursorPtr) {
	[[NSCursor arrowCursor] set];
    } else {
	switch (macCursorPtr->type) {
	case NONE:
	    if (!cursorHidden) {
		cursorHidden = 1;
		[NSCursor hide];
	    }
	    cursorNone = 1;
	    break;
	case SELECTOR:
	case IMAGENAMED:
	case IMAGEPATH:
	case IMAGEBITMAP:
	default:
	    [macCursorPtr->macCursor set];
	    break;
	}
    }
    if (cursorHidden && !cursorNone) {
	cursorHidden = 0;
	[NSCursor unhide];
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetCursor --
 *
 *	Set the current cursor and install it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the current cursor.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetCursor(
    TkpCursor cursor)
{
    int cursorChanged = 1;

    if (!gTkOwnsCursor) {
	return;
    }

    if (cursor == None) {
	/*
	 * This is a little tricky. We can't really tell whether
	 * gCurrentCursor is NULL because it was NULL last time around or
	 * because we just freed the current cursor. So if the input cursor is
	 * NULL, we always need to reset it, we can't trust the cursorChanged
	 * logic.
	 */

	gCurrentCursor = NULL;
    } else {
	if (gCurrentCursor == (TkMacOSXCursor *) cursor) {
	    cursorChanged = 0;
	}
	gCurrentCursor = (TkMacOSXCursor *) cursor;
    }

    if (Tk_MacOSXIsAppInFront() && cursorChanged) {
	TkMacOSXInstallCursor(gResizeOverride);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MacOSXTkOwnsCursor --
 *
 *	Sets whether Tk has the right to adjust the cursor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May keep Tk from changing the cursor.
 *
 *----------------------------------------------------------------------
 */

void
Tk_MacOSXTkOwnsCursor(
    int tkOwnsIt)
{
    gTkOwnsCursor = tkOwnsIt;
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
