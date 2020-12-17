
/*	$Id: tixWinPort.h,v 1.3 2004/03/28 02:44:57 hobbs Exp $	*/

/*
 * tixWinPort.h --
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

#ifndef _TIX_WINPORT_H_
#define _TIX_WINPORT_H_

#include <malloc.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

struct _TixpSubRegion {
    Pixmap pixmap;
    int origX, origY;
    int x, y;
    int width, height;
};

typedef unsigned char UNSIGNED_CHAR;

#endif /* _TIX_WINPORT_H_ */
