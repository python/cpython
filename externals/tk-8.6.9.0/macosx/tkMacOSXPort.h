/*
 * tkMacOSXPort.h --
 *
 *	This file is included by all of the Tk C files. It contains
 *	information that may be configuration-dependent, such as
 *	#includes for system include files and a few other things.
 *
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2005-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _TKMACPORT
#define _TKMACPORT

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#ifdef HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif
#include <sys/stat.h>
#ifndef _TCL
#   include <tcl.h>
#endif
#if TIME_WITH_SYS_TIME
#   include <sys/time.h>
#   include <time.h>
#else
#   if HAVE_SYS_TIME_H
#	include <sys/time.h>
#   else
#	include <time.h>
#   endif
#endif
#if HAVE_INTTYPES_H
#    include <inttypes.h>
#endif
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xfuncproto.h>
#include <X11/Xutil.h>

/*
 * The following macro defines the type of the mask arguments to
 * select:
 */

#ifndef NO_FD_SET
#   define SELECT_MASK fd_set
#else
#   ifndef _AIX
	typedef long fd_mask;
#   endif
#   if defined(_IBMR2)
#	define SELECT_MASK void
#   else
#	define SELECT_MASK int
#   endif
#endif

/*
 * The following macro defines the number of fd_masks in an fd_set:
 */

#ifndef FD_SETSIZE
#   ifdef OPEN_MAX
#	define FD_SETSIZE OPEN_MAX
#   else
#	define FD_SETSIZE 256
#   endif
#endif
#if !defined(howmany)
#   define howmany(x, y) (((x)+((y)-1))/(y))
#endif
#ifndef NFDBITS
#   define NFDBITS NBBY*sizeof(fd_mask)
#endif
#define MASK_SIZE howmany(FD_SETSIZE, NFDBITS)

/*
 * Define "NBBY" (number of bits per byte) if it's not already defined.
 */

#ifndef NBBY
#   define NBBY 8
#endif

/*
 * The following define causes Tk to use its internal keysym hash table
 */

#define REDO_KEYSYM_LOOKUP

/*
 * Defines for X functions that are used by Tk but are treated as
 * no-op functions on the Macintosh.
 */

#define XFlush(display)
#define XFree(data) {if ((data) != NULL) ckfree(data);}
#define XGrabServer(display)
#define XNoOp(display) {display->request++;}
#define XUngrabServer(display)
#define XSynchronize(display, bool) {display->request++;}
#define XVisualIDFromVisual(visual) (visual->visualid)

/*
 * The following functions are not used on the Mac, so we stub them out.
 */

#define TkpCmapStressed(tkwin,colormap) (0)
#define TkpFreeColor(tkColPtr)
#define TkSetPixmapColormap(p,c) {}
#define TkpSync(display)

/*
 * The following macro returns the pixel value that corresponds to the
 * RGB values in the given XColor structure.
 */

#define PIXEL_MAGIC ((unsigned char) 0x69)
#define TkpGetPixel(p) ((((((PIXEL_MAGIC << 8) \
	| (((p)->red >> 8) & 0xff)) << 8) \
	| (((p)->green >> 8) & 0xff)) << 8) \
	| (((p)->blue >> 8) & 0xff))

/*
 * This macro stores a representation of the window handle in a string.
 */

#define TkpPrintWindowId(buf,w) \
	sprintf((buf), "0x%lx", (unsigned long) (w))

/*
 * Turn off Tk double-buffering as Aqua windows are already double-buffered.
 */

#define TK_NO_DOUBLE_BUFFERING 1

/*
 * Magic pixel code values for system colors.
 *
 * NOTE: values must be kept in sync with indices into the
 *	 systemColorMap array in tkMacOSXColor.c !
 */

#define TRANSPARENT_PIXEL		30
#define HIGHLIGHT_PIXEL			31
#define HIGHLIGHT_SECONDARY_PIXEL	32
#define HIGHLIGHT_TEXT_PIXEL		33
#define HIGHLIGHT_ALTERNATE_PIXEL	34
#define CONTROL_TEXT_PIXEL		35
#define CONTROL_BODY_PIXEL		37
#define CONTROL_FRAME_PIXEL		39
#define WINDOW_BODY_PIXEL		41
#define MENU_ACTIVE_PIXEL		43
#define MENU_ACTIVE_TEXT_PIXEL		45
#define MENU_BACKGROUND_PIXEL		47
#define MENU_DISABLED_PIXEL		49
#define MENU_TEXT_PIXEL			51
#define APPEARANCE_PIXEL		52

#endif /* _TKMACPORT */
