/*
 *	WETabs.h
 *
 *	WASTE TABS PACKAGE
 *	Public C/C++ interface
 *
 *	version 1.3.2 (August 1996)
 *
 *	Copyright (c) 1993-1998 Marco Piovanelli
 *	All Rights Reserved
 *
 */


#ifndef WITHOUT_FRAMEWORKS
#include <Carbon/Carbon.h>
#endif
#ifndef _WASTE_
#include "WASTE.h"
#endif

enum {
	kMinTabSize				=	1,		//	must be greater than zero
	kDefaultTabSize			=	32,
	kMaxTabSize				=	1024	//	arbitrary value
};

#ifdef __cplusplus
extern "C" {
#endif

pascal OSErr WEInstallTabHooks(WEReference we);
pascal OSErr WERemoveTabHooks(WEReference we);
pascal Boolean WEIsTabHooks(WEReference we);
pascal SInt16 WEGetTabSize(WEReference we);
pascal OSErr WESetTabSize(SInt16 tabWidth, WEReference we);

#ifdef __cplusplus
}
#endif
