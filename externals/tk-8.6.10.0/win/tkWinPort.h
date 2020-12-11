/*
 * tkWinPort.h --
 *
 *	This header file handles porting issues that occur because of
 *	differences between Windows and Unix. It should be the only
 *	file that contains #ifdefs to handle different flavors of OS.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _WINPORT
#define _WINPORT

/*
 *---------------------------------------------------------------------------
 * The following sets of #includes and #ifdefs are required to get Tcl to
 * compile under the windows compilers.
 *---------------------------------------------------------------------------
 */

#include <wchar.h>
#include <io.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <limits.h>

/*
 * Need to block out this include for building extensions with MetroWerks
 * compiler for Win32.
 */

#ifndef __MWERKS__
#include <sys/stat.h>
#endif

#include <time.h>

#ifdef _MSC_VER
#   ifndef hypot
#	define hypot _hypot
#   endif
#endif /* _MSC_VER */

/*
 *  Pull in the typedef of TCHAR for windows.
 */
#include <tchar.h>
#ifndef _TCHAR_DEFINED
    /* Borland seems to forget to set this. */
    typedef _TCHAR TCHAR;
#   define _TCHAR_DEFINED
#endif
#if defined(_MSC_VER) && defined(__STDC__)
    /* VS2005 SP1 misses this. See [Bug #3110161] */
    typedef _TCHAR TCHAR;
#endif

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#ifndef __GNUC__
#    define strncasecmp _strnicmp
#    define strcasecmp _stricmp
#endif

#define NBBY 8

#ifndef OPEN_MAX
#define OPEN_MAX 32
#endif

/*
 * The following define causes Tk to use its internal keysym hash table
 */

#define REDO_KEYSYM_LOOKUP

/*
 * See ticket [916c1095438eae56]: GetVersionExW triggers warnings
 */
#if defined(_MSC_VER)
#   pragma warning(disable:4267)
#   pragma warning(disable:4244)
#   pragma warning(disable:4311)
#   pragma warning(disable:4312)
#   pragma warning(disable:4996)
#endif

/*
 * The following macro checks to see whether there is buffered
 * input data available for a stdio FILE.
 */

#ifdef _MSC_VER
#    define TK_READ_DATA_PENDING(f) ((f)->_cnt > 0)
#else /* _MSC_VER */
#    define TK_READ_DATA_PENDING(f) ((f)->level > 0)
#endif /* _MSC_VER */

/*
 * The following Tk functions are implemented as macros under Windows.
 */

#define TkpGetPixel(p) (((((p)->red >> 8) & 0xff) \
	| ((p)->green & 0xff00) | (((p)->blue << 8) & 0xff0000)) | 0x20000000)

/*
 * These calls implement native bitmaps which are not currently
 * supported under Windows.  The macros eliminate the calls.
 */

#define TkpDefineNativeBitmaps()
#define TkpCreateNativeBitmap(display, source) None
#define TkpGetNativeAppBitmap(display, name, w, h) None

#endif /* _WINPORT */
