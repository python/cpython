/*
 *	WETabs.c
 *
 *	WASTE TABS PACKAGE
 *	Routines for installing/removing tab hooks; accessors
 *
 */


#include "WETabs.h"
#include "WETabHooks.h"

#if !defined(__ERRORS__) && defined(WITHOUT_FRAMEWORKS)
#include <Errors.h>
#endif

/* static UPP's */
static WEDrawTextUPP		_weTabDrawTextProc = nil;
static WEPixelToCharUPP		_weTabPixelToCharProc = nil;
static WECharToPixelUPP		_weTabCharToPixelProc = nil;
static WELineBreakUPP		_weTabLineBreakProc = nil;

pascal OSErr WEInstallTabHooks(WEReference we)
{
	OSErr err;

	/* if first time, create routine descriptors */
	if (_weTabDrawTextProc == nil)
	{
		_weTabDrawTextProc = NewWEDrawTextProc(_WETabDrawText);
		_weTabPixelToCharProc = NewWEPixelToCharProc(_WETabPixelToChar);
		_weTabCharToPixelProc = NewWECharToPixelProc(_WETabCharToPixel);
		_weTabLineBreakProc = NewWELineBreakProc(_WETabLineBreak);
	}

	if ((err = WESetInfo( weDrawTextHook, &_weTabDrawTextProc, we )) != noErr)
	{
		goto cleanup;
	}
	if ((err = WESetInfo( wePixelToCharHook, &_weTabPixelToCharProc, we )) != noErr)
	{
		goto cleanup;
	}
	if ((err = WESetInfo( weCharToPixelHook, &_weTabCharToPixelProc, we )) != noErr)
	{
		goto cleanup;
	}
	if ((err = WESetInfo( weLineBreakHook, &_weTabLineBreakProc, we )) != noErr)
	{
		goto cleanup;
	}

cleanup:
	return err;
}

pascal OSErr WERemoveTabHooks(WEReference we)
{
	UniversalProcPtr nullHook = nil;
	OSErr err;

	if ((err = WESetInfo( weDrawTextHook, &nullHook, we )) != noErr)
	{
		goto cleanup;
	}
	if ((err = WESetInfo( wePixelToCharHook, &nullHook, we )) != noErr)
	{
		goto cleanup;
	}
	if ((err = WESetInfo( weCharToPixelHook, &nullHook, we )) != noErr)
	{
		goto cleanup;
	}
	if ((err = WESetInfo( weLineBreakHook, &nullHook, we )) != noErr)
	{
		goto cleanup;
	}

cleanup:
	return err;
}

pascal Boolean WEIsTabHooks(WEReference we)
{
	WEPixelToCharUPP hook = nil;

	/* return true if our tab hooks are installed */

	return 	( _weTabPixelToCharProc != nil ) &&
			( WEGetInfo( wePixelToCharHook, &hook, we ) == noErr) &&
			( _weTabPixelToCharProc == hook );
}

pascal SInt16 WEGetTabSize(WEReference we)
{
	SInt32 result;

	if (WEGetUserInfo( kTabSizeTag, &result, we ) != noErr)
	{
		result = kDefaultTabSize;
	}
	return result;
}

pascal OSErr WESetTabSize(SInt16 tabSize, WEReference we)
{
	//	make sure tabSize is a reasonable size
	if ((tabSize < kMinTabSize) || (tabSize > kMaxTabSize))
	{
		return paramErr;
	}
	else
	{
		return WESetUserInfo( kTabSizeTag, tabSize, we );
	}
}
