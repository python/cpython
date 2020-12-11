/*
 * tkMacOSXEvent.c --
 *
 *	This file contains the basic Mac OS X Event handling routines.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2005-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkMacOSXEvent.h"
#include "tkMacOSXDebug.h"
#include "tkMacOSXConstants.h"

#pragma mark TKApplication(TKEvent)

enum {
    NSWindowWillMoveEventType = 20
};

@implementation TKApplication(TKEvent)
/* TODO: replace by +[addLocalMonitorForEventsMatchingMask ? */
- (NSEvent *) tkProcessEvent: (NSEvent *) theEvent
{
#ifdef TK_MAC_DEBUG_EVENTS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, theEvent);
#endif
    NSEvent	    *processedEvent = theEvent;
    NSEventType	    type = [theEvent type];
    NSInteger	    subtype;

    switch ((NSInteger)type) {
    case NSAppKitDefined:
        subtype = [theEvent subtype];

	switch (subtype) {
	    /* Ignored at the moment. */
	case NSApplicationActivatedEventType:
	    break;
	case NSApplicationDeactivatedEventType:
	    break;
	case NSWindowExposedEventType:
	    break;
	case NSScreenChangedEventType:
	    break;
	case NSWindowMovedEventType:
	    break;
        case NSWindowWillMoveEventType:
            break;

        default:
            break;
	}
	break; /* AppkitEvent. Return theEvent */
    case NSKeyUp:
    case NSKeyDown:
    case NSFlagsChanged:
	processedEvent = [self tkProcessKeyEvent:theEvent];
	break; /* Key event.  Return the processed event. */
    case NSLeftMouseDown:
    case NSLeftMouseUp:
    case NSRightMouseDown:
    case NSRightMouseUp:
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    case NSMouseMoved:
    case NSMouseEntered:
    case NSMouseExited:
    case NSScrollWheel:
    case NSOtherMouseDown:
    case NSOtherMouseUp:
    case NSOtherMouseDragged:
    case NSTabletPoint:
    case NSTabletProximity:
	processedEvent = [self tkProcessMouseEvent:theEvent];
	break; /* Mouse event.  Return the processed event. */
#if 0
    case NSSystemDefined:
        subtype = [theEvent subtype];
	break;
    case NSApplicationDefined: {
	id win;
	win = [theEvent window];
	break;
	}
    case NSCursorUpdate:
        break;
    case NSEventTypeGesture:
    case NSEventTypeMagnify:
    case NSEventTypeRotate:
    case NSEventTypeSwipe:
    case NSEventTypeBeginGesture:
    case NSEventTypeEndGesture:
        break;
#endif

    default:
	break; /* return theEvent */
    }
    return processedEvent;
}
@end

#pragma mark -

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXFlushWindows --
 *
 *	This routine is a stub called by XSync, which is called during the Tk
 *      update command.  The language specification does not require that the
 *      update command be synchronous but many of the tests implicitly assume
 *      that it is.  It is definitely asynchronous on macOS since many idle
 *      tasks are run inside of the drawRect method of a window's contentView,
 *      which will not be called until after this function returns.
 *
 * Results:
 *	None.
 *
 * Side effects: Processes all pending idle events then calls the display
 *	method of each visible window.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE void
TkMacOSXFlushWindows(void)
{
    if (Tk_GetNumMainWindows() == 0) {
	return;
    }
    while (Tcl_DoOneEvent(TCL_IDLE_EVENTS)){}
    for (NSWindow *w in [NSApp orderedWindows]) {
	[w display];
    }
}


/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
