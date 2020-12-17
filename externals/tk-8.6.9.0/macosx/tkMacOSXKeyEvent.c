/*
 * tkMacOSXKeyEvent.c --
 *
 *	This file implements functions that decode & handle keyboard events on
 *	MacOS X.
 *
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright (c) 2012 Adrian Robert.
 * Copyright 2015 Marc Culler.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkMacOSXEvent.h"
#include "tkMacOSXConstants.h"

/*
#ifdef TK_MAC_DEBUG
#define TK_MAC_DEBUG_KEYBOARD
#endif
*/
#define NS_KEYLOG 0


static Tk_Window grabWinPtr = NULL;
				/* Current grab window, NULL if no grab. */
static Tk_Window keyboardGrabWinPtr = NULL;
				/* Current keyboard grab window. */
static NSWindow *keyboardGrabNSWindow = nil;
                               /* NSWindow for the current keyboard grab window. */
static NSModalSession modalSession = nil;

static BOOL processingCompose = NO;
static BOOL finishedCompose = NO;

static int caret_x = 0, caret_y = 0, caret_height = 0;

static void setupXEvent(XEvent *xEvent, NSWindow *w, unsigned int state);
static unsigned isFunctionKey(unsigned int code);

unsigned short releaseCode;


#pragma mark TKApplication(TKKeyEvent)

@implementation TKApplication(TKKeyEvent)

- (NSEvent *) tkProcessKeyEvent: (NSEvent *) theEvent
{
#ifdef TK_MAC_DEBUG_EVENTS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, theEvent);
#endif
    NSWindow*	    w;
    NSEventType	    type = [theEvent type];
    NSUInteger	    modifiers, len = 0;
    BOOL	    repeat = NO;
    unsigned short  keyCode;
    NSString	    *characters = nil, *charactersIgnoringModifiers = nil;
    static NSUInteger savedModifiers = 0;
    static NSMutableArray *nsEvArray;

    if (nsEvArray == nil)
      {
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
	modifiers = [theEvent modifierFlags];
	keyCode = [theEvent keyCode];

#if defined(TK_MAC_DEBUG_EVENTS) || NS_KEYLOG == 1
	TKLog(@"-[%@(%p) %s] r=%d mods=%u '%@' '%@' code=%u c=%d %@ %d", [self class], self, _cmd, repeat, modifiers, characters, charactersIgnoringModifiers, keyCode,([charactersIgnoringModifiers length] == 0) ? 0 : [charactersIgnoringModifiers characterAtIndex: 0], w, type);
#endif
	break;

    default:
	return theEvent; /* Unrecognized key event. */
    }

    /* Create an Xevent to add to the Tk queue. */
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
         * The focus must be in the FrontWindow on the Macintosh. We then query Tk
         * to determine the exact Tk window that owns the focus.
         */

        TkWindow *winPtr = TkMacOSXGetTkWindow(w);
        Tk_Window tkwin = (Tk_Window) winPtr;

        if (!tkwin) {
          TkMacOSXDbgMsg("tkwin == NULL");
          return theEvent;
        }
        tkwin = (Tk_Window) winPtr->dispPtr->focusPtr;
        if (!tkwin) {
          TkMacOSXDbgMsg("tkwin == NULL");
          return theEvent;  /* Give up. No window for this event. */
        }

        /*
         * If it's a function key, or we have modifiers other than Shift or Alt,
         * pass it straight to Tk.  Otherwise we'll send for input processing.
         */
        int code = (len == 0) ?
          0 : [charactersIgnoringModifiers characterAtIndex: 0];
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
               * Use special '-1' to signify a special keycode to our platform
               * specific code in tkMacOSXKeyboard.c. This is rather like what
               * happens on Windows.
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

              /* For command key, take input manager's word so things
                 like dvorak / qwerty layout work. */
              if ((modifiers & NSCommandKeyMask) == NSCommandKeyMask
                  && (modifiers & NSAlternateKeyMask) != NSAlternateKeyMask
                  && len > 0 && !isFunctionKey(code)) {
                // head off keycode-based translation in tkMacOSXKeyboard.c
                xEvent.xkey.nbytes = [characters length]; //len
              }

              if ([characters length] > 0) {
                xEvent.xkey.keycode =
                  (keyCode << 16) | (UInt16) [characters characterAtIndex:0];
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
          }  /* if send straight to TK */

      }  /* if not processing compose */

    if (type == NSKeyDown) {
        if (NS_KEYLOG)
          fprintf (stderr, "keyDown: %s compose sequence.\n",
                   processingCompose == YES ? "Continue" : "Begin");
        processingCompose = YES;
        [nsEvArray addObject: theEvent];
        [[w contentView] interpretKeyEvents: nsEvArray];
        [nsEvArray removeObject: theEvent];
      }

    savedModifiers = modifiers;

    return theEvent;
}
@end



@implementation TKContentView
/* <NSTextInput> implementation (called through interpretKeyEvents:]). */

/* <NSTextInput>: called when done composing;
   NOTE: also called when we delete over working text, followed immed.
         by doCommandBySelector: deleteBackward: */
- (void)insertText: (id)aString
{
  int i, len = [(NSString *)aString length];
  XEvent xEvent;

  if (NS_KEYLOG)
    TKLog (@"insertText '%@'\tlen = %d", aString, len);
  processingCompose = NO;
  finishedCompose = YES;

  /* first, clear any working text */
  if (privateWorkingText != nil)
    [self deleteWorkingText];

  /* now insert the string as keystrokes */
  setupXEvent(&xEvent, [self window], 0);
  xEvent.xany.type = KeyPress;

  for (i =0; i<len; i++)
      {
	  xEvent.xkey.keycode = (UInt16) [aString characterAtIndex: i];
	  [[aString substringWithRange: NSMakeRange(i,1)]
	      getCString: xEvent.xkey.trans_chars
	       maxLength: XMaxTransChars encoding: NSUTF8StringEncoding];
	  xEvent.xkey.nbytes = strlen(xEvent.xkey.trans_chars);
	  xEvent.xany.type = KeyPress;
	  releaseCode =  (UInt16) [aString characterAtIndex: 0];
	  Tk_QueueWindowEvent(&xEvent, TCL_QUEUE_TAIL);
      }
  releaseCode =  (UInt16) [aString characterAtIndex: 0];
}


/* <NSTextInput>: inserts display of composing characters */
- (void)setMarkedText: (id)aString selectedRange: (NSRange)selRange
{
  NSString *str = [aString respondsToSelector: @selector (string)] ?
    [aString string] : aString;
  if (NS_KEYLOG)
    TKLog (@"setMarkedText '%@' len =%lu range %lu from %lu", str,
	   (unsigned long) [str length], (unsigned long) selRange.length,
	   (unsigned long) selRange.location);

  if (privateWorkingText != nil)
    [self deleteWorkingText];
  if ([str length] == 0)
    return;

  processingCompose = YES;
  privateWorkingText = [str copy];

  //PENDING: insert workingText underlined
}


- (BOOL)hasMarkedText
{
  return privateWorkingText != nil;
}


- (NSRange)markedRange
{
  NSRange rng = privateWorkingText != nil
    ? NSMakeRange (0, [privateWorkingText length]) : NSMakeRange (NSNotFound, 0);
  if (NS_KEYLOG)
    TKLog (@"markedRange request");
  return rng;
}


- (void)unmarkText
{
  if (NS_KEYLOG)
    TKLog (@"unmark (accept) text");
  [self deleteWorkingText];
  processingCompose = NO;
}


/* used to position char selection windows, etc. */
- (NSRect)firstRectForCharacterRange: (NSRange)theRange
{
  NSRect rect;
  NSPoint pt;

  pt.x = caret_x;
  pt.y = caret_y;

  pt = [self convertPoint: pt toView: nil];
  pt = [[self window] tkConvertPointToScreen: pt];
  pt.y -= caret_height;

  rect.origin = pt;
  rect.size.width = caret_height;
  rect.size.height = caret_height;
  return rect;
}


- (NSInteger)conversationIdentifier
{
  return (NSInteger)self;
}


- (void)doCommandBySelector: (SEL)aSelector
{
  if (NS_KEYLOG)
    TKLog (@"doCommandBySelector: %@", NSStringFromSelector (aSelector));
  processingCompose = NO;
  if (aSelector == @selector (deleteBackward:))
    {
      /* happens when user backspaces over an ongoing composition:
         throw a 'delete' into the event queue */
      XEvent xEvent;
      setupXEvent(&xEvent, [self window], 0);
      xEvent.xany.type = KeyPress;
      xEvent.xkey.nbytes = 1;
      xEvent.xkey.keycode = (0x33 << 16) | 0x7F;
      xEvent.xkey.trans_chars[0] = 0x7F;
      xEvent.xkey.trans_chars[1] = 0x0;
      Tk_QueueWindowEvent(&xEvent, TCL_QUEUE_TAIL);
    }
}


- (NSArray *)validAttributesForMarkedText
{
  static NSArray *arr = nil;
  if (arr == nil) arr = [NSArray new];
 /* [[NSArray arrayWithObject: NSUnderlineStyleAttributeName] retain]; */
  return arr;
}


- (NSRange)selectedRange
{
  if (NS_KEYLOG)
    TKLog (@"selectedRange request");
  return NSMakeRange (NSNotFound, 0);
}


- (NSUInteger)characterIndexForPoint: (NSPoint)thePoint
{
  if (NS_KEYLOG)
    TKLog (@"characterIndexForPoint request");
  return 0;
}


- (NSAttributedString *)attributedSubstringFromRange: (NSRange)theRange
{
  static NSAttributedString *str = nil;
  if (str == nil) str = [NSAttributedString new];
  if (NS_KEYLOG)
    TKLog (@"attributedSubstringFromRange request");
  return str;
}
/* End <NSTextInput> impl. */
@end


@implementation TKContentView(TKKeyEvent)
/* delete display of composing characters [not in <NSTextInput>] */
- (void)deleteWorkingText
{
  if (privateWorkingText == nil)
    return;
  if (NS_KEYLOG)
    TKLog(@"deleteWorkingText len = %lu\n",
	    (unsigned long)[privateWorkingText length]);
  [privateWorkingText release];
  privateWorkingText = nil;
  processingCompose = NO;

  //PENDING: delete working text
}
@end



/*
 *  Set up basic fields in xevent for keyboard input.
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
    xEvent->xany.send_event = false;
    xEvent->xany.display = Tk_Display(tkwin);
    xEvent->xany.window = Tk_WindowId(tkwin);

    xEvent->xkey.root = XRootWindow(Tk_Display(tkwin), 0);
    xEvent->xkey.subwindow = None;
    xEvent->xkey.time = TkpGetMS();
    xEvent->xkey.state = state;
    xEvent->xkey.same_screen = true;
    xEvent->xkey.trans_chars[0] = 0;
    xEvent->xkey.nbytes = 0;
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
    if (keyboardGrabWinPtr && grabWinPtr) {
	NSWindow *w = TkMacOSXDrawableWindow(grab_window);
	MacDrawable *macWin = (MacDrawable *) grab_window;

	if (w && macWin->toplevel->winPtr == (TkWindow*) grabWinPtr) {
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

void
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
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetCapture --
 *
 * Results:
 *	Returns the current grab window
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
TkMacOSXGetCapture(void)
{
    return grabWinPtr;
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
 * TkpSetCapture --
 *
 *	This function captures the mouse so that all future events will be
 *	reported to this window, even if the mouse is outside the window. If
 *	the specified window is NULL, then the mouse is released.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the capture flag and captures the mouse.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetCapture(
    TkWindow *winPtr)		/* Capture window, or NULL. */
{
    while (winPtr && !Tk_IsTopLevel(winPtr)) {
	winPtr = winPtr->parentPtr;
    }
    grabWinPtr = (Tk_Window) winPtr;
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

    /* But adjust for fact that NS uses flipped view. */
    y = Tk_Height(tkwin) - y;

    caret_x = x;
    caret_y = y;
    caret_height = height;
}


static unsigned convert_ns_to_X_keysym[] =
{
  NSHomeFunctionKey,            0x50,
  NSLeftArrowFunctionKey,       0x51,
  NSUpArrowFunctionKey,         0x52,
  NSRightArrowFunctionKey,      0x53,
  NSDownArrowFunctionKey,       0x54,
  NSPageUpFunctionKey,          0x55,
  NSPageDownFunctionKey,        0x56,
  NSEndFunctionKey,             0x57,
  NSBeginFunctionKey,           0x58,
  NSSelectFunctionKey,          0x60,
  NSPrintFunctionKey,           0x61,
  NSExecuteFunctionKey,         0x62,
  NSInsertFunctionKey,          0x63,
  NSUndoFunctionKey,            0x65,
  NSRedoFunctionKey,            0x66,
  NSMenuFunctionKey,            0x67,
  NSFindFunctionKey,            0x68,
  NSHelpFunctionKey,            0x6A,
  NSBreakFunctionKey,           0x6B,

  NSF1FunctionKey,              0xBE,
  NSF2FunctionKey,              0xBF,
  NSF3FunctionKey,              0xC0,
  NSF4FunctionKey,              0xC1,
  NSF5FunctionKey,              0xC2,
  NSF6FunctionKey,              0xC3,
  NSF7FunctionKey,              0xC4,
  NSF8FunctionKey,              0xC5,
  NSF9FunctionKey,              0xC6,
  NSF10FunctionKey,             0xC7,
  NSF11FunctionKey,             0xC8,
  NSF12FunctionKey,             0xC9,
  NSF13FunctionKey,             0xCA,
  NSF14FunctionKey,             0xCB,
  NSF15FunctionKey,             0xCC,
  NSF16FunctionKey,             0xCD,
  NSF17FunctionKey,             0xCE,
  NSF18FunctionKey,             0xCF,
  NSF19FunctionKey,             0xD0,
  NSF20FunctionKey,             0xD1,
  NSF21FunctionKey,             0xD2,
  NSF22FunctionKey,             0xD3,
  NSF23FunctionKey,             0xD4,
  NSF24FunctionKey,             0xD5,

  NSBackspaceCharacter,         0x08,  /* 8: Not on some KBs. */
  NSDeleteCharacter,            0xFF,  /* 127: Big 'delete' key upper right. */
  NSDeleteFunctionKey,          0x9F,  /* 63272: Del forw key off main array. */

  NSTabCharacter,		0x09,
  0x19,				0x09,  /* left tab->regular since pass shift */
  NSCarriageReturnCharacter,	0x0D,
  NSNewlineCharacter,		0x0D,
  NSEnterCharacter,		0x8D,

  0x1B,				0x1B   /* escape */
};


static unsigned isFunctionKey(unsigned code)
{
    const unsigned last_keysym = (sizeof (convert_ns_to_X_keysym)
                                / sizeof (convert_ns_to_X_keysym[0]));
  unsigned keysym;
  for (keysym = 0; keysym < last_keysym; keysym += 2)
    if (code == convert_ns_to_X_keysym[keysym])
      return 0xFF00 | convert_ns_to_X_keysym[keysym+1];
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
