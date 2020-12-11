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
#include <assert.h>
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

#undef XFlush
#define XFlush(display) (0)
#undef XFree
#define XFree(data) (((data) != NULL) ? (ckfree(data),0) : 0)
#undef XGrabServer
#define XGrabServer(display) (0)
#undef XNoOp
#define XNoOp(display) (display->request++,0)
#undef XUngrabServer
#define XUngrabServer(display) (0)
#undef XSynchronize
#define XSynchronize(display, onoff) (display->request++,NULL)
#undef XVisualIDFromVisual
#define XVisualIDFromVisual(visual) (visual->visualid)

/*
 * The following functions are not used on the Mac, so we stub them out.
 */

#define TkpCmapStressed(tkwin,colormap) (0)
#define TkpFreeColor(tkColPtr)
#define TkSetPixmapColormap(p,c) {}
#define TkpSync(display)

/*
 * TkMacOSXGetCapture is a legacy function used on the Mac. When fixing
 * [943d5ebe51], TkpGetCapture was added to the Windows port. Both
 * are actually the same feature and should bear the same name. However,
 * in order to avoid potential backwards incompatibilities, renaming
 * TkMacOSXGetCapture into TkpGetCapture in *PlatDecls.h shall not be
 * done in a patch release, therefore use a define here.
 */

#define TkpGetCapture TkMacOSXGetCapture

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
#define APPEARANCE_PIXEL		52
#define PIXEL_MAGIC ((unsigned char) 0x69)

/*
 * The following macro returns the pixel value that corresponds to the
 * 16-bit RGB values in the given XColor structure.
 * The format is: (PIXEL_MAGIC <<< 24) | (R << 16) | (G << 8) | B
 * where each of R, G and B is the high order byte of a 16-bit component.
 */

#define TkpGetPixel(p) ((((((PIXEL_MAGIC << 8) \
	| (((p)->red >> 8) & 0xff)) << 8) \
	| (((p)->green >> 8) & 0xff)) << 8) \
	| (((p)->blue >> 8) & 0xff))


#endif /* _TKMACPORT */
