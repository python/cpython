/*
 * tkUnixCursor.c --
 *
 *	This file contains X specific cursor manipulation routines.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * The following data structure is a superset of the TkCursor structure
 * defined in tkCursor.c. Each system specific cursor module will define a
 * different cursor structure. All of these structures must have the same
 * header consisting of the fields in TkCursor.
 */

typedef struct {
    TkCursor info;		/* Generic cursor info used by tkCursor.c */
    Display *display;		/* Display for which cursor is valid. */
} TkUnixCursor;

/*
 * The table below is used to map from the name of a cursor to its index in
 * the official cursor font:
 */

static const struct CursorName {
    const char *name;
    unsigned int shape;
} cursorNames[] = {
    {"X_cursor",		XC_X_cursor},
    {"arrow",			XC_arrow},
    {"based_arrow_down",	XC_based_arrow_down},
    {"based_arrow_up",		XC_based_arrow_up},
    {"boat",			XC_boat},
    {"bogosity",		XC_bogosity},
    {"bottom_left_corner",	XC_bottom_left_corner},
    {"bottom_right_corner",	XC_bottom_right_corner},
    {"bottom_side",		XC_bottom_side},
    {"bottom_tee",		XC_bottom_tee},
    {"box_spiral",		XC_box_spiral},
    {"center_ptr",		XC_center_ptr},
    {"circle",			XC_circle},
    {"clock",			XC_clock},
    {"coffee_mug",		XC_coffee_mug},
    {"cross",			XC_cross},
    {"cross_reverse",		XC_cross_reverse},
    {"crosshair",		XC_crosshair},
    {"diamond_cross",		XC_diamond_cross},
    {"dot",			XC_dot},
    {"dotbox",			XC_dotbox},
    {"double_arrow",		XC_double_arrow},
    {"draft_large",		XC_draft_large},
    {"draft_small",		XC_draft_small},
    {"draped_box",		XC_draped_box},
    {"exchange",		XC_exchange},
    {"fleur",			XC_fleur},
    {"gobbler",			XC_gobbler},
    {"gumby",			XC_gumby},
    {"hand1",			XC_hand1},
    {"hand2",			XC_hand2},
    {"heart",			XC_heart},
    {"icon",			XC_icon},
    {"iron_cross",		XC_iron_cross},
    {"left_ptr",		XC_left_ptr},
    {"left_side",		XC_left_side},
    {"left_tee",		XC_left_tee},
    {"leftbutton",		XC_leftbutton},
    {"ll_angle",		XC_ll_angle},
    {"lr_angle",		XC_lr_angle},
    {"man",			XC_man},
    {"middlebutton",		XC_middlebutton},
    {"mouse",			XC_mouse},
    {"pencil",			XC_pencil},
    {"pirate",			XC_pirate},
    {"plus",			XC_plus},
    {"question_arrow",		XC_question_arrow},
    {"right_ptr",		XC_right_ptr},
    {"right_side",		XC_right_side},
    {"right_tee",		XC_right_tee},
    {"rightbutton",		XC_rightbutton},
    {"rtl_logo",		XC_rtl_logo},
    {"sailboat",		XC_sailboat},
    {"sb_down_arrow",		XC_sb_down_arrow},
    {"sb_h_double_arrow",	XC_sb_h_double_arrow},
    {"sb_left_arrow",		XC_sb_left_arrow},
    {"sb_right_arrow",		XC_sb_right_arrow},
    {"sb_up_arrow",		XC_sb_up_arrow},
    {"sb_v_double_arrow",	XC_sb_v_double_arrow},
    {"shuttle",			XC_shuttle},
    {"sizing",			XC_sizing},
    {"spider",			XC_spider},
    {"spraycan",		XC_spraycan},
    {"star",			XC_star},
    {"target",			XC_target},
    {"tcross",			XC_tcross},
    {"top_left_arrow",		XC_top_left_arrow},
    {"top_left_corner",		XC_top_left_corner},
    {"top_right_corner",	XC_top_right_corner},
    {"top_side",		XC_top_side},
    {"top_tee",			XC_top_tee},
    {"trek",			XC_trek},
    {"ul_angle",		XC_ul_angle},
    {"umbrella",		XC_umbrella},
    {"ur_angle",		XC_ur_angle},
    {"watch",			XC_watch},
    {"xterm",			XC_xterm},
    {NULL,			0}
};

/*
 * The table below is used to map from a cursor name to the data that defines
 * the cursor. This table is used for cursors defined by Tk that don't exist
 * in the X cursor table.
 */

#define CURSOR_NONE_DATA \
"#define none_width 1\n" \
"#define none_height 1\n" \
"#define none_x_hot 0\n" \
"#define none_y_hot 0\n" \
"static unsigned char none_bits[] = {\n" \
"  0x00};"

/*
 * Define test cursor to check that mask fg and bg color settings are working.
 *
 * . configure -cursor {center_ptr green red}
 * . configure -cursor {@myarrow.xbm myarrow-mask.xbm green red}
 * . configure -cursor {myarrow green red}
 */

/*#define DEFINE_MYARROW_CURSOR*/

#ifdef DEFINE_MYARROW_CURSOR
#define CURSOR_MYARROW_DATA \
"#define myarrow_width 16\n" \
"#define myarrow_height 16\n" \
"#define myarrow_x_hot 7\n" \
"#define myarrow_y_hot 0\n" \
"static unsigned char myarrow_bits[] = {\n" \
"   0x7f, 0xff, 0xbf, 0xfe, 0xdf, 0xfd, 0xef, 0xfb, 0xf7, 0xf7, 0xfb, 0xef,\n" \
"   0xfd, 0xdf, 0xfe, 0xbf, 0x80, 0x00, 0xbf, 0xfe, 0xbf, 0xfe, 0xbf, 0xfe,\n" \
"   0xbf, 0xfe, 0xbf, 0xfe, 0xbf, 0xfe, 0x3f, 0xfe};"

#define CURSOR_MYARROW_MASK \
"#define myarrow-mask_width 16\n" \
"#define myarrow-mask_height 16\n" \
"#define myarrow-mask_x_hot 7\n" \
"#define myarrow-mask_y_hot 0\n" \
"static unsigned char myarrow-mask_bits[] = {\n" \
"   0x80, 0x00, 0xc0, 0x01, 0xe0, 0x03, 0xf0, 0x07, 0xf8, 0x0f, 0xfc, 0x1f,\n" \
"   0xfe, 0x3f, 0xff, 0x7f, 0xff, 0xff, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,\n" \
"   0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01};"

#endif /* DEFINE_MYARROW_CURSOR */

static const struct TkCursorName {
    const char *name;
    const char *data;
    char *mask;
} tkCursorNames[] = {
    {"none",	CURSOR_NONE_DATA,	NULL},
#ifdef DEFINE_MYARROW_CURSOR
    {"myarrow",	CURSOR_MYARROW_DATA,	CURSOR_MYARROW_MASK},
#endif /* DEFINE_MYARROW_CURSOR */
    {NULL,	NULL,			NULL}
};

/*
 * Font to use for cursors:
 */

#ifndef CURSORFONT
#define CURSORFONT "cursor"
#endif

static Cursor		CreateCursorFromTableOrFile(Tcl_Interp *interp,
			    Tk_Window tkwin, int argc, const char **argv,
			    const struct TkCursorName *tkCursorPtr);

/*
 *----------------------------------------------------------------------
 *
 * TkGetCursorByName --
 *
 *	Retrieve a cursor by name. Parse the cursor name into fields and
 *	create a cursor, either from the standard cursor font or from bitmap
 *	files.
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
    Tk_Uid string)		/* Description of cursor. See manual entry for
				 * details on legal syntax. */
{
    TkUnixCursor *cursorPtr = NULL;
    Cursor cursor = None;
    int argc;
    const char **argv = NULL;
    Display *display = Tk_Display(tkwin);
    int inTkTable = 0;
    const struct TkCursorName *tkCursorPtr = NULL;

    if (Tcl_SplitList(interp, string, &argc, &argv) != TCL_OK) {
	return NULL;
    }
    if (argc == 0) {
	goto badString;
    }

    /*
     * Check Tk specific table of cursor names. The cursor names don't overlap
     * with cursors defined in the X table so search order does not matter.
     */

    if (argv[0][0] != '@') {
	for (tkCursorPtr = tkCursorNames; ; tkCursorPtr++) {
	    if (tkCursorPtr->name == NULL) {
		tkCursorPtr = NULL;
		break;
	    }
	    if ((tkCursorPtr->name[0] == argv[0][0]) &&
		    (strcmp(tkCursorPtr->name, argv[0]) == 0)) {
		inTkTable = 1;
		break;
	    }
	}
    }

    if ((argv[0][0] != '@') && !inTkTable) {
	XColor fg, bg;
	unsigned int maskIndex;
	register const struct CursorName *namePtr;
	TkDisplay *dispPtr;

	/*
	 * The cursor is to come from the standard cursor font. If one arg, it
	 * is cursor name (use black and white for fg and bg). If two args,
	 * they are name and fg color (ignore mask). If three args, they are
	 * name, fg, bg. Some of the code below is stolen from the
	 * XCreateFontCursor Xlib function.
	 */

	if (argc > 3) {
	    goto badString;
	}
	for (namePtr = cursorNames; ; namePtr++) {
	    if (namePtr->name == NULL) {
		goto badString;
	    }
	    if ((namePtr->name[0] == argv[0][0])
		    && (strcmp(namePtr->name, argv[0]) == 0)) {
		break;
	    }
	}

	maskIndex = namePtr->shape + 1;
	if (argc == 1) {
	    fg.red = fg.green = fg.blue = 0;
	    bg.red = bg.green = bg.blue = 65535;
	} else {
	    if (TkParseColor(display, Tk_Colormap(tkwin), argv[1], &fg) == 0) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"invalid color name \"%s\"", argv[1]));
		Tcl_SetErrorCode(interp, "TK", "CURSOR", "COLOR", NULL);
		goto cleanup;
	    }
	    if (argc == 2) {
		bg.red = bg.green = bg.blue = 0;
		maskIndex = namePtr->shape;
	    } else if (TkParseColor(display, Tk_Colormap(tkwin), argv[2],
		    &bg) == 0) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"invalid color name \"%s\"", argv[2]));
		Tcl_SetErrorCode(interp, "TK", "CURSOR", "COLOR", NULL);
		goto cleanup;
	    }
	}
	dispPtr = ((TkWindow *) tkwin)->dispPtr;
	if (dispPtr->cursorFont == None) {
	    dispPtr->cursorFont = XLoadFont(display, CURSORFONT);
	    if (dispPtr->cursorFont == None) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"couldn't load cursor font", -1));
		Tcl_SetErrorCode(interp, "TK", "CURSOR", "FONT", NULL);
		goto cleanup;
	    }
	}
	cursor = XCreateGlyphCursor(display, dispPtr->cursorFont,
		dispPtr->cursorFont, namePtr->shape, maskIndex,
		&fg, &bg);
    } else {
	/*
	 * Prevent file system access in safe interpreters.
	 */

	if (!inTkTable && Tcl_IsSafe(interp)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't get cursor from a file in a safe interpreter",
		    -1));
	    Tcl_SetErrorCode(interp, "TK", "SAFE", "CURSOR_FILE", NULL);
	    cursorPtr = NULL;
	    goto cleanup;
	}

	/*
	 * If the cursor is to be created from bitmap files, then there should
	 * be either two elements in the list (source, color) or four (source
	 * mask fg bg). A cursor defined in the Tk table accepts the same
	 * arguments as an X cursor.
	 */

	if (inTkTable && (argc != 1) && (argc != 2) && (argc != 3)) {
	    goto badString;
	}

	if (!inTkTable && (argc != 2) && (argc != 4)) {
	    goto badString;
	}

	cursor = CreateCursorFromTableOrFile(interp, tkwin, argc, argv,
		tkCursorPtr);
    }

    if (cursor != None) {
	cursorPtr = ckalloc(sizeof(TkUnixCursor));
	cursorPtr->info.cursor = (Tk_Cursor) cursor;
	cursorPtr->display = display;
    }

  cleanup:
    if (argv != NULL) {
	ckfree(argv);
    }
    return (TkCursor *) cursorPtr;

  badString:
    if (argv) {
	ckfree(argv);
    }
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad cursor spec \"%s\"", string));
    Tcl_SetErrorCode(interp, "TK", "VALUE", "CURSOR", NULL);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateCursorFromTableOrFile --
 *
 *	Create a cursor defined in a file or the Tk static cursor table. A
 *	cursor defined in a file starts with the '@' character. This method
 *	assumes that the number of arguments in argv has been validated
 *	already.
 *
 * Results:
 *	Returns a new cursor, or None on error.
 *
 * Side effects:
 *	Allocates a new X cursor.
 *
 *----------------------------------------------------------------------
 */

static Cursor
CreateCursorFromTableOrFile(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. */
    Tk_Window tkwin,		/* Window in which cursor will be used. */
    int argc,
    const char **argv,		/* Cursor spec parsed into elements. */
    const struct TkCursorName *tkCursorPtr)
				/* Non-NULL when cursor is defined in Tk
				 * table. */
{
    Cursor cursor = None;

    int width, height, maskWidth, maskHeight;
    int xHot = -1, yHot = -1;
    int dummy1, dummy2;
    XColor fg, bg;
    const char *fgColor;
    const char *bgColor;
    int inTkTable = (tkCursorPtr != NULL);

    Display *display = Tk_Display(tkwin);
    Drawable drawable = RootWindowOfScreen(Tk_Screen(tkwin));

    Pixmap source = None;
    Pixmap mask = None;

    /*
     * A cursor defined in a file accepts either 2 or 4 arguments.
     *
     * {srcfile fg}
     * {srcfile maskfile fg bg}
     *
     * A cursor defined in the Tk table accepts 1, 2, or 3 arguments.
     *
     * {tkcursorname}
     * {tkcursorname fg}
     * {tkcursorname fg bg}
     */

    if (inTkTable) {
	/*
	 * This logic is like TkReadBitmapFile().
	 */

	char *data;

	data = TkGetBitmapData(NULL, tkCursorPtr->data, NULL,
		&width, &height, &xHot, &yHot);
	if (data == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error reading bitmap data for \"%s\"", argv[0]));
	    Tcl_SetErrorCode(interp, "TK", "CURSOR", "BITMAP_DATA", NULL);
	    goto cleanup;
	}

	source = XCreateBitmapFromData(display, drawable, data, width,height);
	ckfree(data);
    } else {
	if (TkReadBitmapFile(display, drawable, &argv[0][1],
		(unsigned *) &width, (unsigned *) &height,
		&source, &xHot, &yHot) != BitmapSuccess) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "cleanup reading bitmap file \"%s\"", &argv[0][1]));
	    Tcl_SetErrorCode(interp, "TK", "CURSOR", "BITMAP_FILE", NULL);
	    goto cleanup;
	}
    }

    if ((xHot < 0) || (yHot < 0) || (xHot >= width) || (yHot >= height)) {
	if (inTkTable) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad hot spot in bitmap data for \"%s\"", argv[0]));
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad hot spot in bitmap file \"%s\"", &argv[0][1]));
	}
	Tcl_SetErrorCode(interp, "TK", "CURSOR", "HOTSPOT", NULL);
	goto cleanup;
    }

    /*
     * Parse color names from optional fg and bg arguments
     */

    if (argc == 1) {
	fg.red = fg.green = fg.blue = 0;
	bg.red = bg.green = bg.blue = 65535;
    } else if (argc == 2) {
	fgColor = argv[1];
	if (TkParseColor(display, Tk_Colormap(tkwin), fgColor, &fg) == 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "invalid color name \"%s\"", fgColor));
	    Tcl_SetErrorCode(interp, "TK", "CURSOR", "COLOR", NULL);
	    goto cleanup;
	}
	if (inTkTable) {
	    bg.red = bg.green = bg.blue = 0;
	} else {
	    bg = fg;
	}
    } else {
	/* 3 or 4 arguments */
	if (inTkTable) {
	    fgColor = argv[1];
	    bgColor = argv[2];
	} else {
	    fgColor = argv[2];
	    bgColor = argv[3];
	}
	if (TkParseColor(display, Tk_Colormap(tkwin), fgColor, &fg) == 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "invalid color name \"%s\"", fgColor));
	    Tcl_SetErrorCode(interp, "TK", "CURSOR", "COLOR", NULL);
	    goto cleanup;
	}
	if (TkParseColor(display, Tk_Colormap(tkwin), bgColor, &bg) == 0) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "invalid color name \"%s\"", bgColor));
	    Tcl_SetErrorCode(interp, "TK", "CURSOR", "COLOR", NULL);
	    goto cleanup;
	}
    }

    /*
     * If there is no mask data, then create the cursor now.
     */

    if ((!inTkTable && (argc == 2)) || (inTkTable && tkCursorPtr->mask == NULL)) {
	cursor = XCreatePixmapCursor(display, source, source,
		&fg, &fg, (unsigned) xHot, (unsigned) yHot);
	goto cleanup;
    }

    /*
     * Parse bitmap mask data and create cursor with fg and bg colors.
     */

    if (inTkTable) {
	/*
	 * This logic is like TkReadBitmapFile().
	 */

	char *data;

	data = TkGetBitmapData(NULL, tkCursorPtr->mask, NULL,
		&maskWidth, &maskHeight, &dummy1, &dummy2);
	if (data == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "error reading bitmap mask data for \"%s\"", argv[0]));
	    Tcl_SetErrorCode(interp, "TK", "CURSOR", "MASK_DATA", NULL);
	    goto cleanup;
	}

	mask = XCreateBitmapFromData(display, drawable, data, maskWidth,
		maskHeight);

	ckfree(data);
    } else {
	if (TkReadBitmapFile(display, drawable, argv[1],
		(unsigned int *) &maskWidth, (unsigned int *) &maskHeight,
		&mask, &dummy1, &dummy2) != BitmapSuccess) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "cleanup reading bitmap file \"%s\"", argv[1]));
	    Tcl_SetErrorCode(interp, "TK", "CURSOR", "MASK_FILE", NULL);
	    goto cleanup;
	}
    }

    if ((maskWidth != width) || (maskHeight != height)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"source and mask bitmaps have different sizes", -1));
	Tcl_SetErrorCode(interp, "TK", "CURSOR", "SIZE_MATCH", NULL);
	goto cleanup;
    }

    cursor = XCreatePixmapCursor(display, source, mask,
	    &fg, &bg, (unsigned) xHot, (unsigned) yHot);

  cleanup:
    if (source != None) {
	Tk_FreePixmap(display, source);
    }
    if (mask != None) {
	Tk_FreePixmap(display, mask);
    }
    return cursor;
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
    Cursor cursor;
    Pixmap sourcePixmap, maskPixmap;
    TkUnixCursor *cursorPtr = NULL;
    Display *display = Tk_Display(tkwin);

    sourcePixmap = XCreateBitmapFromData(display,
	    RootWindowOfScreen(Tk_Screen(tkwin)), source, (unsigned) width,
	    (unsigned) height);
    maskPixmap = XCreateBitmapFromData(display,
	    RootWindowOfScreen(Tk_Screen(tkwin)), mask, (unsigned) width,
	    (unsigned) height);
    cursor = XCreatePixmapCursor(display, sourcePixmap,
	    maskPixmap, &fgColor, &bgColor, (unsigned) xHot, (unsigned) yHot);
    Tk_FreePixmap(display, sourcePixmap);
    Tk_FreePixmap(display, maskPixmap);

    if (cursor != None) {
	cursorPtr = ckalloc(sizeof(TkUnixCursor));
	cursorPtr->info.cursor = (Tk_Cursor) cursor;
	cursorPtr->display = display;
    }
    return (TkCursor *) cursorPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpFreeCursor --
 *
 *	This function is called to release a cursor allocated by
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
    TkUnixCursor *unixCursorPtr = (TkUnixCursor *) cursorPtr;

    XFreeCursor(unixCursorPtr->display, (Cursor) unixCursorPtr->info.cursor);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
