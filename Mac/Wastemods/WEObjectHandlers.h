/*
	WASTE Demo Project:
	Sample WASTE Object Handlers

	Copyright © 1993-1998 Marco Piovanelli
	All Rights Reserved
*/
#ifndef WITHOUT_FRAMEWORKS
#include <Carbon/Carbon.h>
#endif
#ifndef _WASTE_
#include "WASTE.h"
#endif


// PICTURES

pascal OSErr HandleNewPicture(Point *defaultObjectSize, WEObjectReference objectRef);
pascal OSErr HandleDisposePicture(WEObjectReference objectRef);
pascal OSErr HandleDrawPicture(const Rect *destRect, WEObjectReference objectRef);

// SOUNDS

enum {
	kSoundIconID	=	550
};

pascal OSErr HandleNewSound(Point *defaultObjectSize, WEObjectReference objectRef);
pascal OSErr HandleDrawSound(const Rect *destRect, WEObjectReference objectRef);
pascal Boolean HandleClickSound(Point hitPt, EventModifiers modifiers,
					UInt32 clickTime, WEObjectReference objectRef);
