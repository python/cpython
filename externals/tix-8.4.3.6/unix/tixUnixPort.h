
/*	$Id: tixUnixPort.h,v 1.3 2005/03/25 20:15:53 hobbs Exp $	*/

/*
 * tixUnixPort.h --
 *
 *	This header file handles porting issues that occur because of
 *	differences between systems.  It reads in platform specific
 *	portability files.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _TIX_UNIXPORT_H_
#define _TIX_UNIXPORT_H_

struct _TixpSubRegion {
#ifdef MAC_OSX_TK
    Pixmap pixmap;
    int x, y;
#endif
    XRectangle rect;
    int rectUsed;
    int origX;
    int origY;
};

#ifdef UCHAR_SUPPORTED
typedef unsigned char UNSIGNED_CHAR;
#else
typedef char UNSIGNED_CHAR;
#endif

#endif /* _TIX_UNIXPORT_H_ */
