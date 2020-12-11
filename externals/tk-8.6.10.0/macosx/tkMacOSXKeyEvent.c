/*
 * tkMacOSXKeyEvent.c --
 *
 *	This file implements functions that decode & handle keyboard events on
 *	MacOS X.
 *
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright (c) 2012 Adrian Robert.
 * Copyright 2015-2019 Marc Culler.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkMacOSXInt.h"
#include "tkMacOSXConstants.h"

/*
#ifdef TK_MAC_DEBUG
#define TK_MAC_DEBUG_KEYBOARD
#endif
*/
#define NS_KEYLOG 0

static Tk_Window keyboardGrabWinPtr = NULL;
				/* Current keyboard grab window. */
static NSWindow *keyboardGrabNSWindow = nil;
				/* NSWindow for the current keyboard grab
				 * window. */
static NSModalSession modalSession = nil;
static BOOL processingCompose = NO;
static Tk_Window composeWin = NULL;
static int caret_x = 0, caret_y = 0, caret_height = 0;
static unsigned short releaseCode;

static void setupXEvent(XEvent *xEvent, NSWindow *w, unsigned int state);
static unsigned	isFunctionKey(unsigned int code);


#pragma mark TKApplication(TKKeyEvent)

@implementation TKApplication(TKKeyEvent)

- (NSEvent *) tkProcessKeyEvent: (NSEvent *) theEvent
{
#ifdef TK_MAC_DEBUG_EVENTS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, theEvent);
#endif
    NSWindow *w;
    NSEventType type = [theEvent type];
    NSUInteger modifiers = ([theEvent modifierFlags] &
			    NSDeviceIndependentModifierFlagsMask);
    NSUInteger len = 0;
    BOOL repeat = NO;
    unsigned short keyCode = [theEvent keyCode];
    NSString *characters = nil, *charactersIgnoringModifiers = nil;
    static NSUInteger savedModifiers = 0;
    static NSMutableArray *nsEvArray;

    if (nsEvArray == nil) {
        nsEvArray = [[NSMutableArray alloc] initWithCapacity: 1];
        processingCompose = NO;
    }

    w = [theEvent window];
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);
    Tk_Window tkwin = (Tk_Window) winPtr;
    XEvent xEvent;

    if (!winPtr) {
	return theEvent;
    }

    /*
     * Control-Tab and Control-Shift-Tab are used to switch tabs in a tabbed
     * window.  We do not want to generate an Xevent for these since that might
     * cause the deselected tab to be reactivated.
     */

    if (keyCode == 48 && (modifiers & NSControlKeyMask) == NSControlKeyMask) {
	return theEvent;
    }

    switch (type) {
    case NSKeyUp:
	/*Fix for bug #1ba71a86bb: key release firing on key press.*/
	setupXEvent(&xEvent, w, 0);
	xEvent.xany.type = KeyRelease;
	xEvent.xkey.keycode = releaseCode;
	xEvent.xany.serial = LastKnownRequestProcessed(Tk_Display(tkwin));
    case NSKeyDown:
	repeat = [theEvent isARepeat];
	characters = [theEvent characters];
	charactersIgnoringModifiers = [theEvent charactersIgnoringModifiers];
        len = [charactersIgnoringModifiers length];
    case NSFlagsChanged:

#if defined(TK_MAC_DEBUG_EVENTS) || NS_KEYLOG == 1
	TKLog(@"-[%@(%p) %s] r=%d mods=%u '%@' '%@' code=%u c=%d %@ %d", [self class], self, _cmd, repeat, modifiers, characters, charactersIgnoringModifiers, keyCode,([charactersIgnoringModifiers length] == 0) ? 0 : [charactersIgnoringModifiers characterAtIndex: 0], w, type);
#endif
	break;

    default:
	return theEvent; /* Unrecognized key event. */
    }

    /*
     * Create an Xevent to add to the Tk queue.
     */

    if (!processingCompose) {
        unsigned int state = 0;

        if (modifiers & NSAlphaShiftKeyMask) {
	    state |= LockMask;
        }
        if (modifiers & NSShiftKeyMask) {
	    state |= ShiftMask;
        }
        if (modifiers & NSControlKeyMask) {
	    state |= ControlMask;
        }
        if (modifiers & NSCommandKeyMask) {
	    state |= Mod1Mask;		/* command key */
        }
        if (modifiers & NSAlternateKeyMask) {
	    state |= Mod2Mask;		/* option key */
        }
        if (modifiers & NSNumericPadKeyMask) {
	    state |= Mod3Mask;
        }
        if (modifiers & NSFunctionKeyMask) {
	    state |= Mod4Mask;
        }

        /*
         * Key events are only received for the front Window on the Macintosh. So
	 * to build an XEvent we look up the Tk window associated to the Front
	 * window.
         */

        TkWindow *winPtr = TkMacOSXGetTkWindow(w);
        Tk_Window tkwin = (Tk_Window) winPtr;

	if (tkwin) {
	    TkWindow *grabWinPtr = winPtr->dispPtr->grabWinPtr;

	    /*
	     * If a local grab is in effect, key events for windows in the
	     * grabber's application are redirected to the grabber.  Key events
	     * for other applications are delivered normally.  If a global
	     * grab is in effect all key events are redirected to the grabber.
	     */

	    if (grabWinPtr) {
		if (winPtr->dispPtr->grabFlags ||  /* global grab */
		    grabWinPtr->mainPtr == winPtr->mainPtr){ /* same appl. */
			tkwin = (Tk_Window) winPtr->dispPtr->focusPtr;
		    }
	    }
	} else {
	    tkwin = (Tk_Window) winPtr->dispPtr->focusPtr;
	}
        if (!tkwin) {
	    TkMacOSXDbgMsg("tkwin == NULL");
	    return theEvent;  /* Give up. No window for this event. */
        }

        /*
         * If it's a function key, or we have modifiers other than Shift or
         * Alt, pass it straight to Tk.  Otherwise we'll send for input
         * processing.
         */

        int code = (len == 0) ? 0 :
		[charactersIgnoringModifiers characterAtIndex: 0];
        if (type != NSKeyDown || isFunctionKey(code)
		|| (len > 0 && state & (ControlMask | Mod1Mask | Mod3Mask | Mod4Mask))) {
            XEvent xEvent;

            setupXEvent(&xEvent, w, state);
            if (type == NSFlagsChanged) {
		if (savedModifiers > modifiers) {
		    xEvent.xany.type = KeyRelease;
		} else {
		    xEvent.xany.type = KeyPress;
		}

		/*
		 * Use special '-1' to signify a special keycode to our
		 * platform specific code in tkMacOSXKeyboard.c. This is rather
		 * like what happens on Windows.
		 */

		xEvent.xany.send_event = -1;

		/*
		 * Set keycode (which was zero) to the changed modifier
		 */

		xEvent.xkey.keycode = (modifiers ^ savedModifiers);
            } else {
		if (type == NSKeyUp || repeat) {
		    xEvent.xany.type = KeyRelease;
		} else {
		    xEvent.xany.type = KeyPress;
		}

		if ([characters length] > 0) {
		    xEvent.xkey.keycode = (keyCode << 16) |
			    (UInt16) [characters characterAtIndex:0];
		    if (![characters getCString:xEvent.xkey.trans_chars
			    maxLength:XMaxTransChars encoding:NSUTF8StringEncoding]) {
			/* prevent SF bug 2907388 (crash on some composite chars) */
			//PENDING: we might not need this anymore
			TkMacOSXDbgMsg("characters too long");
			return theEvent;
		    }
		}

		if (repeat) {
		    Tk_QueueWindowEvent(&xEvent, TCL_QUEUE_TAIL);
		    xEvent.xany.type = KeyPress;
		    xEvent.xany.serial = LastKnownRequestProcessed(Tk_Display(tkwin));
		}
            }
            Tk_QueueWindowEvent(&xEvent, TCL_QUEUE_TAIL);
            savedModifiers = modifiers;
            return theEvent;
	}  /* if this is a function key or has modifiers */
    }  /* if not processing compose */

    if (type == NSKeyDown) {
        if (NS_KEYLOG) {
	    TKLog(@"keyDown: %s compose sequence.\n",
		    processingCompose == YES ? "Continue" : "Begin");
	}

	/*
	 * Call the interpretKeyEvents method to interpret composition key
	 * strokes.  When it detects a complete composition sequence it will
	 * call our implementation of insertText: replacementRange, which
	 * generates a key down XEvent with the appropriate character.  In IME
	 * when multiple characters have the same composition sequence and the
	 * chosen character is not the default it may be necessary to hit the
	 * enter key multiple times before the character is accepted and
	 * rendered. We send enter key events until inputText has cleared
	 * the processingCompose flag.
	 */

	processingCompose = YES;
	while(processingCompose) {
	    [nsEvArray addObject: theEvent];
	    [[w contentView] interpretKeyEvents: nsEvArray];
	    [nsEvArray removeObject: theEvent];
	    if ([theEvent keyCode] != 36) {
		break;
	    }
	}
    }
    savedModifiers = modifiers;
    return theEvent;
}
@end


@implementation TKContentView

-(id)init {
    self = [super init];
    if (self) {
        _needsRedisplay = NO;
    }
    return self;
}

/*
 * Implementation of the NSTextInputClient protocol.
 */

/* [NSTextInputClient inputText: replacementRange:] is called by
 * interpretKeyEvents when a composition sequence is complete.  It is also
 * called when we delete over working text.  In that case the call is followed
 * immediately by doCommandBySelector: deleteBackward:
 */
- (void)insertText: (id)aString
  replacementRange: (NSRange)repRange
{
    int i, len;
    XEvent xEvent;
    NSString *str;

    str = ([aString isKindOfClass: [NSAttributedString class]]) ?
        [aString string] : aString;
    len = [str length];

    if (NS_KEYLOG) {
	TKLog(@"insertText '%@'\tlen = %d", aString, len);
    }

    processingCompose = NO;

    /*
     * Clear any working text.
     */

    if (privateWorkingText != nil) {
    	[self deleteWorkingText];
    }

    /*
     * Insert the string as a sequence of keystrokes.
     */

    setupXEvent(&xEvent, [self window], 0);
    xEvent.xany.type = KeyPress;

    /*
     * Apple evidently sets location to 0 to signal that an accented letter has
     * been selected from the accent menu.  An unaccented letter has already
     * been displayed and we need to erase it before displaying the accented
     * letter.
     */

    if (repRange.location == 0) {
	TkWindow *winPtr = TkMacOSXGetTkWindow([self window]);
	Tk_Window focusWin = (Tk_Window) winPtr->dispPtr->focusPtr;
	TkSendVirtualEvent(focusWin, "TkAccentBackspace", NULL);
    }

    /*
     * Next we generate an XEvent for each unicode character in our string.
     *
     * NSString uses UTF-16 internally, which means that a non-BMP character is
     * represented by a sequence of two 16-bit "surrogates".  In principle we
     * could record this in the XEvent by setting the keycode to the 32-bit
     * unicode code point and setting the trans_chars string to the 4-byte
     * UTF-8 string for the non-BMP character.  However, that will not work
     * when TCL_UTF_MAX is set to 3, as is the case for Tcl 8.6.  A workaround
     * used internally by Tcl 8.6 is to encode each surrogate as a 3-byte
     * sequence using the UTF-8 algorithm (ignoring the fact that the UTF-8
     * encoding specification does not allow encoding UTF-16 surrogates).
     * This gives a 6-byte encoding of the non-BMP character which we write into
     * the trans_chars field of the XEvent.
     */

    for (i = 0; i < len; i++) {
	xEvent.xkey.nbytes = TclUniAtIndex(str, i, xEvent.xkey.trans_chars,
					   &xEvent.xkey.keycode);
	if (xEvent.xkey.keycode > 0xffff){
	    i++;
	}
    	xEvent.xany.type = KeyPress;
    	Tk_QueueWindowEvent(&xEvent, TCL_QUEUE_TAIL);
    }
    releaseCode = (UInt16) [str characterAtIndex: 0];
}

/*
 * This required method is allowed to return nil.
 */

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)theRange
      actualRange:(NSRangePointer)thePointer
{
    return nil;
}

/*
 * This method is supposed to insert (or replace selected text with) the string
 * argument. If the argument is an NSString, it should be displayed with a
 * distinguishing appearance, e.g underlined.
 */

- (void)setMarkedText: (id)aString
	selectedRange: (NSRange)selRange
     replacementRange: (NSRange)repRange
{
    TkWindow *winPtr = TkMacOSXGetTkWindow([self window]);
    Tk_Window focusWin = (Tk_Window) winPtr->dispPtr->focusPtr;
    NSString *temp;
    NSString *str;

    str = ([aString isKindOfClass: [NSAttributedString class]]) ?
        [aString string] : aString;

    if (focusWin) {

	/*
	 * Remember the widget where the composition is happening, in case it
	 * gets defocussed during the composition.
	 */

	composeWin = focusWin;
    } else {
	return;
    }
    if (NS_KEYLOG) {
	TKLog(@"setMarkedText '%@' len =%lu range %lu from %lu", str,
	      (unsigned long) [str length], (unsigned long) selRange.length,
	      (unsigned long) selRange.location);
    }

    if (privateWorkingText != nil) {
	[self deleteWorkingText];
    }

    if ([str length] == 0) {
	return;
    }

    /*
     * Use our insertText method to display the marked text.
     */

    TkSendVirtualEvent(focusWin, "TkStartIMEMarkedText", NULL);
    temp = [str copy];
    [self insertText: temp replacementRange:repRange];
    privateWorkingText = temp;
    processingCompose = YES;
    TkSendVirtualEvent(focusWin, "TkEndIMEMarkedText", NULL);
}

- (BOOL)hasMarkedText
{
    return privateWorkingText != nil;
}


- (NSRange)markedRange
{
    NSRange rng = privateWorkingText != nil
	? NSMakeRange(0, [privateWorkingText length])
	: NSMakeRange(NSNotFound, 0);

    if (NS_KEYLOG) {
	TKLog(@"markedRange request");
    }
    return rng;
}

- (void)cancelComposingText
{
    if (NS_KEYLOG) {
	TKLog(@"cancelComposingText");
    }
    [self deleteWorkingText];
    processingCompose = NO;
}

- (void)unmarkText
{
    if (NS_KEYLOG) {
	TKLog(@"unmarkText");
    }
    [self deleteWorkingText];
    processingCompose = NO;
}


/*
 * Called by the system to get a position for popup character selection windows
 * such as a Character Palette, or a selection menu for IME.
 */

- (NSRect)firstRectForCharacterRange: (NSRange)theRange
			 actualRange: (NSRangePointer)thePointer
{
    NSRect rect;
    NSPoint pt;
    pt.x = caret_x;
    pt.y = caret_y;

    pt = [self convertPoint: pt toView: nil];
    pt = [[self window] tkConvertPointToScreen: pt];
    pt.y -= caret_height;

    rect.origin = pt;
    rect.size.width = 0;
    rect.size.height = caret_height;
    return rect;
}

- (NSInteger)conversationIdentifier
{
    return (NSInteger) self;
}

- (void)doCommandBySelector: (SEL)aSelector
{
    if (NS_KEYLOG) {
	TKLog(@"doCommandBySelector: %@", NSStringFromSelector(aSelector));
    }
    processingCompose = NO;
    if (aSelector == @selector (deleteBackward:)) {
	TkWindow *winPtr = TkMacOSXGetTkWindow([self window]);
	Tk_Window focusWin = (Tk_Window) winPtr->dispPtr->focusPtr;
	TkSendVirtualEvent(focusWin, "TkAccentBackspace", NULL);
    }
}

- (NSArray *)validAttributesForMarkedText
{
    static NSArray *arr = nil;
    if (arr == nil) {
	arr = [[NSArray alloc] initWithObjects:
	    NSUnderlineStyleAttributeName,
	    NSUnderlineColorAttributeName,
	    nil];
	[arr retain];
    }
    return arr;
}

- (NSRange)selectedRange
{
    if (NS_KEYLOG) {
	TKLog(@"selectedRange request");
    }
    return NSMakeRange(0, 0);
}

- (NSUInteger)characterIndexForPoint: (NSPoint)thePoint
{
    if (NS_KEYLOG) {
	TKLog(@"characterIndexForPoint request");
    }
    return NSNotFound;
}

- (NSAttributedString *)attributedSubstringFromRange: (NSRange)theRange
{
    static NSAttributedString *str = nil;
    if (str == nil) {
	str = [NSAttributedString new];
    }
    if (NS_KEYLOG) {
	TKLog(@"attributedSubstringFromRange request");
    }
    return str;
}
/* End of NSTextInputClient implementation. */

@synthesize needsRedisplay = _needsRedisplay;
@end


@implementation TKContentView(TKKeyEvent)

/*
 * Tell the widget to erase the displayed composing characters.  This
 * is not part of the NSTextInputClient protocol.
 */

- (void)deleteWorkingText
{
    if (privateWorkingText == nil) {
	return;
    } else {

	if (NS_KEYLOG) {
	    TKLog(@"deleteWorkingText len = %lu\n",
		  (unsigned long)[privateWorkingText length]);
	}

	[privateWorkingText release];
	privateWorkingText = nil;
	processingCompose = NO;
	if (composeWin) {
	    TkSendVirtualEvent(composeWin, "TkClearIMEMarkedText", NULL);
	}
    }
}
@end

/*
 * Set up basic fields in xevent for keyboard input.
 */

static void
setupXEvent(XEvent *xEvent, NSWindow *w, unsigned int state)
{
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);
    Tk_Window tkwin = (Tk_Window) winPtr;

    if (!winPtr) {
	return;
    }

    memset(xEvent, 0, sizeof(XEvent));
    xEvent->xany.serial = LastKnownRequestProcessed(Tk_Display(tkwin));
    xEvent->xany.display = Tk_Display(tkwin);
    xEvent->xany.window = Tk_WindowId(tkwin);

    xEvent->xkey.root = XRootWindow(Tk_Display(tkwin), 0);
    xEvent->xkey.time = TkpGetMS();
    xEvent->xkey.state = state;
    xEvent->xkey.same_screen = true;
    /* No need to initialize other fields implicitly here,
     * because of the memset() above. */
}

#pragma mark -

/*
 *----------------------------------------------------------------------
 *
 * XGrabKeyboard --
 *
 *	Simulates a keyboard grab by setting the focus.
 *
 * Results:
 *	Always returns GrabSuccess.
 *
 * Side effects:
 *	Sets the keyboard focus to the specified window.
 *
 *----------------------------------------------------------------------
 */

int
XGrabKeyboard(
    Display* display,
    Window grab_window,
    Bool owner_events,
    int pointer_mode,
    int keyboard_mode,
    Time time)
{
    keyboardGrabWinPtr = Tk_IdToWindow(display, grab_window);
    TkWindow *captureWinPtr = (TkWindow *) TkpGetCapture();

    if (keyboardGrabWinPtr && captureWinPtr) {
	NSWindow *w = TkMacOSXDrawableWindow(grab_window);
	MacDrawable *macWin = (MacDrawable *) grab_window;

	if (w && macWin->toplevel->winPtr == (TkWindow *) captureWinPtr) {
	    if (modalSession) {
		Tcl_Panic("XGrabKeyboard: already grabbed");
	    }
	    keyboardGrabNSWindow = w;
	    [w retain];
	    modalSession = [NSApp beginModalSessionForWindow:w];
	}
    }
    return GrabSuccess;
}

/*
 *----------------------------------------------------------------------
 *
 * XUngrabKeyboard --
 *
 *	Releases the simulated keyboard grab.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the keyboard focus back to the value before the grab.
 *
 *----------------------------------------------------------------------
 */

int
XUngrabKeyboard(
    Display* display,
    Time time)
{
    if (modalSession) {
	[NSApp endModalSession:modalSession];
	modalSession = nil;
    }
    if (keyboardGrabNSWindow) {
	[keyboardGrabNSWindow release];
	keyboardGrabNSWindow = nil;
    }
    keyboardGrabWinPtr = NULL;
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetModalSession --
 *
 * Results:
 *	Returns the current modal session
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE NSModalSession
TkMacOSXGetModalSession(void)
{
    return modalSession;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetCaretPos --
 *
 *	This enables correct placement of the XIM caret. This is called by
 *	widgets to indicate their cursor placement, and the caret location is
 *	used by TkpGetString to place the XIM caret.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetCaretPos(
    Tk_Window tkwin,
    int x,
    int y,
    int height)
 {
    TkCaret *caretPtr = &(((TkWindow *) tkwin)->dispPtr->caret);

    /*
     * Prevent processing anything if the values haven't changed. Windows only
     * has one display, so we can do this with statics.
     */

    if ((caretPtr->winPtr == ((TkWindow *) tkwin))
	    && (caretPtr->x == x) && (caretPtr->y == y)) {
	return;
    }

    caretPtr->winPtr = ((TkWindow *) tkwin);
    caretPtr->x = x;
    caretPtr->y = y;
    caretPtr->height = height;

    /*
     * As in Windows, adjust to the toplevel to get the coords right.
     */

    while (!Tk_IsTopLevel(tkwin)) {
	x += Tk_X(tkwin);
	y += Tk_Y(tkwin);
	tkwin = Tk_Parent(tkwin);
	if (tkwin == NULL) {
	    return;
	}
    }

    /*
     * But adjust for fact that NS uses flipped view.
     */

    y = Tk_Height(tkwin) - y;

    caret_x = x;
    caret_y = y;
    caret_height = height;
}


static unsigned convert_ns_to_X_keysym[] =
{
    NSHomeFunctionKey,		0x50,
    NSLeftArrowFunctionKey,	0x51,
    NSUpArrowFunctionKey,	0x52,
    NSRightArrowFunctionKey,	0x53,
    NSDownArrowFunctionKey,	0x54,
    NSPageUpFunctionKey,	0x55,
    NSPageDownFunctionKey,	0x56,
    NSEndFunctionKey,		0x57,
    NSBeginFunctionKey,		0x58,
    NSSelectFunctionKey,	0x60,
    NSPrintFunctionKey,		0x61,
    NSExecuteFunctionKey,	0x62,
    NSInsertFunctionKey,	0x63,
    NSUndoFunctionKey,		0x65,
    NSRedoFunctionKey,		0x66,
    NSMenuFunctionKey,		0x67,
    NSFindFunctionKey,		0x68,
    NSHelpFunctionKey,		0x6A,
    NSBreakFunctionKey,		0x6B,

    NSF1FunctionKey,		0xBE,
    NSF2FunctionKey,		0xBF,
    NSF3FunctionKey,		0xC0,
    NSF4FunctionKey,		0xC1,
    NSF5FunctionKey,		0xC2,
    NSF6FunctionKey,		0xC3,
    NSF7FunctionKey,		0xC4,
    NSF8FunctionKey,		0xC5,
    NSF9FunctionKey,		0xC6,
    NSF10FunctionKey,		0xC7,
    NSF11FunctionKey,		0xC8,
    NSF12FunctionKey,		0xC9,
    NSF13FunctionKey,		0xCA,
    NSF14FunctionKey,		0xCB,
    NSF15FunctionKey,		0xCC,
    NSF16FunctionKey,		0xCD,
    NSF17FunctionKey,		0xCE,
    NSF18FunctionKey,		0xCF,
    NSF19FunctionKey,		0xD0,
    NSF20FunctionKey,		0xD1,
    NSF21FunctionKey,		0xD2,
    NSF22FunctionKey,		0xD3,
    NSF23FunctionKey,		0xD4,
    NSF24FunctionKey,		0xD5,

    NSBackspaceCharacter,	0x08,  /* 8: Not on some KBs. */
    NSDeleteCharacter,		0xFF,  /* 127: Big 'delete' key upper right. */
    NSDeleteFunctionKey,	0x9F,  /* 63272: Del forw key off main array. */

    NSTabCharacter,		0x09,
    0x19,			0x09,  /* left tab->regular since pass shift */
    NSCarriageReturnCharacter,	0x0D,
    NSNewlineCharacter,		0x0D,
    NSEnterCharacter,		0x8D,

    0x1B,			0x1B   /* escape */
};


static unsigned
isFunctionKey(
    unsigned code)
{
    const unsigned last_keysym = (sizeof(convert_ns_to_X_keysym)
                                / sizeof(convert_ns_to_X_keysym[0]));
    unsigned keysym;

    for (keysym = 0; keysym < last_keysym; keysym += 2) {
	if (code == convert_ns_to_X_keysym[keysym]) {
	    return 0xFF00 | convert_ns_to_X_keysym[keysym + 1];
	}
    }
    return 0;
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
