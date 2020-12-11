/*
 * tkMacOSXKeyboard.c --
 *
 *	Routines to support keyboard events on the Macintosh.
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
#include "tkMacOSXConstants.h"
/*
 * A couple of simple definitions to make code a bit more self-explaining.
 *
 * For the assignments of Mod1==meta==command and Mod2==alt==option, see also
 * tkMacOSXMouseEvent.c.
 */

#define LATIN1_MAX	 255
#define MAC_KEYCODE_MAX	 0x7F
#define MAC_KEYCODE_MASK 0x7F
#define COMMAND_MASK	 Mod1Mask
#define OPTION_MASK	 Mod2Mask


/*
 * Tables enumerating the special keys defined on Mac keyboards. These are
 * necessary for correct keysym mappings for all keys where the keysyms are
 * not identical with their ASCII or Latin-1 code points.
 */

typedef struct {
    int keycode;		/* Macintosh keycode. */
    KeySym keysym;		/* X windows keysym. */
} KeyInfo;

/*
 * Notes on keyArray:
 *
 * 0x34, XK_Return - Powerbooks use this and some keymaps define it.
 *
 * 0x4C, XK_Return - XFree86 and Apple's X11 call this one XK_KP_Enter.
 *
 * 0x47, XK_Clear - This key is NumLock when used on PCs, but Mac
 * applications don't use it like that, nor does Apple's X11.
 *
 * All other keycodes are taken from the published ADB keyboard layouts.
 */

static KeyInfo keyArray[] = {
    {0x24,	XK_Return},
    {0x30,	XK_Tab},
    {0x33,	XK_BackSpace},
    {0x34,	XK_Return},
    {0x35,	XK_Escape},

    {0x47,	XK_Clear},
    {0x4C,	XK_KP_Enter},

    {0x72,	XK_Help},
    {0x73,	XK_Home},
    {0x74,	XK_Page_Up},
    {0x75,	XK_Delete},
    {0x77,	XK_End},
    {0x79,	XK_Page_Down},

    {0x7B,	XK_Left},
    {0x7C,	XK_Right},
    {0x7D,	XK_Down},
    {0x7E,	XK_Up},

    {0,		0}
};

static KeyInfo virtualkeyArray[] = {
    {122,	XK_F1},
    {120,	XK_F2},
    {99,	XK_F3},
    {118,	XK_F4},
    {96,	XK_F5},
    {97,	XK_F6},
    {98,	XK_F7},
    {100,	XK_F8},
    {101,	XK_F9},
    {109,	XK_F10},
    {103,	XK_F11},
    {111,	XK_F12},
    {105,	XK_F13},
    {107,	XK_F14},
    {113,	XK_F15},
    {0,		0}
};

#define NUM_MOD_KEYCODES 14
static KeyCode modKeyArray[NUM_MOD_KEYCODES] = {
    XK_Shift_L,
    XK_Shift_R,
    XK_Control_L,
    XK_Control_R,
    XK_Caps_Lock,
    XK_Shift_Lock,
    XK_Meta_L,
    XK_Meta_R,
    XK_Alt_L,
    XK_Alt_R,
    XK_Super_L,
    XK_Super_R,
    XK_Hyper_L,
    XK_Hyper_R,
};

static int initialized = 0;
static Tcl_HashTable keycodeTable;	/* keyArray hashed by keycode value. */
static Tcl_HashTable vkeyTable;		/* virtualkeyArray hashed by virtual
					 * keycode value. */

static int latin1Table[LATIN1_MAX+1];	/* Reverse mapping table for
					 * controls, ASCII and Latin-1. */

static int keyboardChanged = 1;

/*
 * Prototypes for static functions used in this file.
 */

static void	InitKeyMaps (void);
static void	InitLatin1Table(Display *display);
static int	XKeysymToMacKeycode(Display *display, KeySym keysym);
static int	KeycodeToUnicode(UniChar * uniChars, int maxChars,
			UInt16 keyaction, UInt32 keycode, UInt32 modifiers,
			UInt32 * deadKeyStatePtr);

#pragma mark TKApplication(TKKeyboard)

@implementation TKApplication(TKKeyboard)
- (void) keyboardChanged: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    keyboardChanged = 1;
}
@end

#pragma mark -

/*
 *----------------------------------------------------------------------
 *
 * InitKeyMaps --
 *
 *	Creates hash tables used by some of the functions in this file.
 *
 *	FIXME: As keycodes are defined to be in the limited range 0-127, it
 *	would be easier and more efficient to use directly initialized plain
 *	arrays and drop this function.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates memory & creates some hash tables.
 *
 *----------------------------------------------------------------------
 */

static void
InitKeyMaps(void)
{
    Tcl_HashEntry *hPtr;
    KeyInfo *kPtr;
    int dummy;

    Tcl_InitHashTable(&keycodeTable, TCL_ONE_WORD_KEYS);
    for (kPtr = keyArray; kPtr->keycode != 0; kPtr++) {
	hPtr = Tcl_CreateHashEntry(&keycodeTable, INT2PTR(kPtr->keycode),
		&dummy);
	Tcl_SetHashValue(hPtr, INT2PTR(kPtr->keysym));
    }
    Tcl_InitHashTable(&vkeyTable, TCL_ONE_WORD_KEYS);
    for (kPtr = virtualkeyArray; kPtr->keycode != 0; kPtr++) {
	hPtr = Tcl_CreateHashEntry(&vkeyTable, INT2PTR(kPtr->keycode),
		&dummy);
	Tcl_SetHashValue(hPtr, INT2PTR(kPtr->keysym));
    }
    initialized = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * InitLatin1Table --
 *
 *	Creates a simple table to be used for mapping from keysyms to keycodes.
 *	Always needs to be called before using latin1Table, because the
 *	keyboard layout may have changed, and than the table must be
 *	re-computed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the global latin1Table.
 *
 *----------------------------------------------------------------------
 */

static void
InitLatin1Table(
    Display *display)
{
    int keycode;
    KeySym keysym;
    int state;
    int modifiers;

    memset(latin1Table, 0, sizeof(latin1Table));

    /*
     * In the common X11 implementations, a keymap has four columns
     * "plain", "Shift", "Mode_switch" and "Mode_switch + Shift". We don't
     * use "Mode_switch", but we use "Option" instead. (This is similar to
     * Apple's X11 implementation, where "Mode_switch" is used as an alias
     * for "Option".)
     *
     * So here we go through all 4 columns of the keymap and find all
     * Latin-1 compatible keycodes. We go through the columns back-to-front
     * from the more exotic columns to the more simple, so that simple
     * keycode-modifier combinations are preferred in the resulting table.
     */

    for (state = 3; state >= 0; state--) {
	modifiers = 0;
	if (state & 1) {
	    modifiers |= shiftKey;
	}
	if (state & 2) {
	    modifiers |= optionKey;
	}

	for (keycode = 0; keycode <= MAC_KEYCODE_MAX; keycode++) {
	    keysym = XKeycodeToKeysym(display,keycode<<16,state);
	    if (keysym <= LATIN1_MAX) {
		latin1Table[keysym] = keycode | modifiers;
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * KeycodeToUnicode --
 *
 *	Given MacOS key event data this function generates the Unicode
 *	characters. It does this using OS resources and APIs.
 *
 *	The parameter deadKeyStatePtr can be NULL, if no deadkey handling is
 *	needed.
 *
 *	This function is called from XKeycodeToKeysym() in tkMacOSKeyboard.c.
 *
 * Results:
 *	The number of characters generated if any, 0 if we are waiting for
 *	another byte of a dead-key sequence. Fills in the uniChars array with a
 *	Unicode string.
 *
 * Side Effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int
KeycodeToUnicode(
    UniChar *uniChars,
    int maxChars,
    UInt16 keyaction,
    UInt32 keycode,
    UInt32 modifiers,
    UInt32 *deadKeyStatePtr)
{
    static const void *uchr = NULL;
    static UInt32 keyboardType = 0;
    UniCharCount actuallength = 0;

    if (keyboardChanged) {
	TISInputSourceRef currentKeyboardLayout =
		TISCopyCurrentKeyboardLayoutInputSource();

	if (currentKeyboardLayout) {
	    CFDataRef keyLayoutData = (CFDataRef) TISGetInputSourceProperty(
		    currentKeyboardLayout, kTISPropertyUnicodeKeyLayoutData);

	    if (keyLayoutData) {
		uchr = CFDataGetBytePtr(keyLayoutData);
		keyboardType = LMGetKbdType();
	    }
	    CFRelease(currentKeyboardLayout);
	}
	keyboardChanged = 0;
    }
    if (uchr) {
	OptionBits options = 0;
	UInt32 dummyState;
	OSStatus err;

	keycode &= 0xFF;
	modifiers = (modifiers >> 8) & 0xFF;

	if (!deadKeyStatePtr) {
	    options = kUCKeyTranslateNoDeadKeysMask;
	    dummyState = 0;
	    deadKeyStatePtr = &dummyState;
	}

	err = ChkErr(UCKeyTranslate, uchr, keycode, keyaction, modifiers,
		keyboardType, options, deadKeyStatePtr, maxChars,
		&actuallength, uniChars);

	if (!actuallength && *deadKeyStatePtr) {
	    /*
	     * More data later
	     */

	    return 0;
	}
	*deadKeyStatePtr = 0;
	if (err != noErr) {
	    actuallength = 0;
	}
    }
    return actuallength;
}

/*
 *----------------------------------------------------------------------
 *
 * XKeycodeToKeysym --
 *
 *	Translate from a system-dependent keycode to a system-independent
 *	keysym.
 *
 * Results:
 *	Returns the translated keysym, or NoSymbol on failure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

KeySym
XKeycodeToKeysym(
    Display* display,
    KeyCode keycode,
    int index)
{
    register Tcl_HashEntry *hPtr;
    int newKeycode;
    UniChar newChar;

    (void) display; /*unused*/

    if (!initialized) {
	InitKeyMaps();
    }

    /*
     * When determining what keysym to produce we first check to see if the key
     * is a function key. We then check to see if the character is another
     * non-printing key. Finally, we return the key syms for all ASCII and
     * Latin-1 chars.
     */

    newKeycode = keycode >> 16;

    if ((keycode & 0xFFFF) >= 0xF700) { /* NSEvent.h function key unicodes */
	hPtr = Tcl_FindHashEntry(&vkeyTable, INT2PTR(newKeycode));
	if (hPtr != NULL) {
	    return (KeySym) Tcl_GetHashValue(hPtr);
	}
    }
    hPtr = Tcl_FindHashEntry(&keycodeTable, INT2PTR(newKeycode));
    if (hPtr != NULL) {
	return (KeySym) Tcl_GetHashValue(hPtr);
    }

    /*
     * Add in the Mac modifier flags for shift and option.
     */

    if (index & 1) {
	newKeycode |= shiftKey;
    }
    if (index & 2) {
	newKeycode |= optionKey;
    }

    newChar = 0;
    KeycodeToUnicode(&newChar, 1, kUCKeyActionDown, newKeycode & 0x00FF,
	    newKeycode & 0xFF00, NULL);

    /*
     * X11 keysyms are identical to Unicode for ASCII and Latin-1. Give up for
     * other characters for now.
     */

    if ((newChar >= XK_space) && (newChar <= LATIN1_MAX)) {
	return newChar;
    }

    return NoSymbol;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetString --
 *
 *	Retrieve the string equivalent for the given keyboard event.
 *
 * Results:
 *	Returns the UTF string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
TkpGetString(
    TkWindow *winPtr,		/* Window where event occurred: Needed to get
				 * input context. */
    XEvent *eventPtr,		/* X keyboard event. */
    Tcl_DString *dsPtr)		/* Uninitialized or empty string to hold
				 * result. */
{
    (void) winPtr; /*unused*/
    int ch;

    Tcl_DStringInit(dsPtr);
    return Tcl_DStringAppend(dsPtr, eventPtr->xkey.trans_chars,
	    TkUtfToUniChar(eventPtr->xkey.trans_chars, &ch));
}

/*
 *----------------------------------------------------------------------
 *
 * XGetModifierMapping --
 *
 *	Fetch the current keycodes used as modifiers.
 *
 * Results:
 *	Returns a new modifier map.
 *
 * Side effects:
 *	Allocates a new modifier map data structure.
 *
 *----------------------------------------------------------------------
 */

XModifierKeymap *
XGetModifierMapping(
    Display *display)
{
    XModifierKeymap *modmap;

    (void) display; /*unused*/

    /*
     * MacOSX doesn't use the key codes for the modifiers for anything, and we
     * don't generate them either. So there is no modifier map.
     */
    modmap = ckalloc(sizeof(XModifierKeymap));
    modmap->max_keypermod = 0;
    modmap->modifiermap = NULL;
    return modmap;
}

/*
 *----------------------------------------------------------------------
 *
 * XFreeModifiermap --
 *
 *	Deallocate a modifier map that was created by XGetModifierMapping.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the datastructure referenced by modmap.
 *
 *----------------------------------------------------------------------
 */

int
XFreeModifiermap(
    XModifierKeymap *modmap)
{
    if (modmap->modifiermap != NULL) {
	ckfree(modmap->modifiermap);
    }
    ckfree(modmap);
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * XKeysymToString, XStringToKeysym --
 *
 *	These X window functions map keysyms to strings & strings to keysyms.
 *	However, Tk already does this for the most common keysyms. Therefore,
 *	these functions only need to support keysyms that will be specific to
 *	the Macintosh. Currently, there are none.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
XKeysymToString(
    KeySym keysym)
{
    return NULL;
}

KeySym
XStringToKeysym(
    const char* string)
{
    return NoSymbol;
}

/*
 *----------------------------------------------------------------------
 *
 * XKeysymToMacKeycode --
 *
 *	An internal function like XKeysymToKeycode but only generating the Mac
 *	specific keycode plus the modifiers Shift and Option.
 *
 * Results:
 *	A Mac keycode with the actual keycode in the low byte and Mac-style
 *	modifier bits in the high byte.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
XKeysymToMacKeycode(
    Display *display,
    KeySym keysym)
{
    KeyInfo *kPtr;
    if (keysym <= LATIN1_MAX) {
	/*
	 * Handle keysyms in the Latin-1 range where keysym and Unicode
	 * character code point are the same.
	 */

	if (keyboardChanged) {
	    InitLatin1Table(display);
	    keyboardChanged = 0;
	}
	return latin1Table[keysym];
    }

    /*
     * Handle special keys from our exception tables. Don't mind if this is
     * slow, neither the test suite nor [event generate] need to be optimized
     * (we hope).
     */

    for (kPtr = keyArray; kPtr->keycode != 0; kPtr++) {
	if (kPtr->keysym == keysym) {
	    return kPtr->keycode;
	}
    }
    for (kPtr = virtualkeyArray; kPtr->keycode != 0; kPtr++) {
	if (kPtr->keysym == keysym) {
	    return kPtr->keycode;
	}
    }

    /*
     * Modifier keycodes only come from generated events.  No translation
     * is needed.
     */

    for (int i=0; i < NUM_MOD_KEYCODES; i++) {
	if (keysym == modKeyArray[i]) {
	    return keysym;
	}
    }

    /*
     * For other keysyms (not Latin-1 and not special keys), we'd need a
     * generic keysym-to-unicode table. We don't have that, so we give up here.
     */

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * XKeysymToKeycode --
 *
 *	The function XKeysymToKeycode takes an X11 keysym and converts it into
 *	a Mac keycode. It is in the stubs table for compatibility but not used
 *	anywhere in the core.
 *
 * Results:
 *	A 32 bit keycode with the the mac keycode (without modifiers) in the
 *	higher 16 bits of the keycode and the ASCII or Latin-1 code in the
 *	lower 8 bits of the keycode.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

KeyCode
XKeysymToKeycode(
    Display* display,
    KeySym keysym)
{
    int macKeycode = XKeysymToMacKeycode(display, keysym);
    KeyCode result;

    /*
     * See also TkpSetKeycodeAndState. The 0x0010 magic is used in
     * XKeycodeToKeysym. For special keys like XK_Return the lower 8 bits of
     * the keysym are usually a related ASCII control code.
     */

    if ((keysym >= XK_F1) && (keysym <= XK_F35)) {
	result = 0x0010;
    } else {
	result = 0x00FF & keysym;
    }
    result |= (macKeycode & MAC_KEYCODE_MASK) << 16;

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetKeycodeAndState --
 *
 *	The function TkpSetKeycodeAndState takes a keysym and fills in the
 *	appropriate members of an XEvent. It is similar to XKeysymToKeycode,
 *	but it also sets the modifier mask in the XEvent. It is used by [event
 *	generate] and it is in the stubs table.
 *
 * Results:
 *	Fills an XEvent, sets the member xkey.keycode with a keycode
 *	formatted the same as XKeysymToKeycode and the member xkey.state with
 *	the modifiers implied by the keysym. Also fills in xkey.trans_chars,
 *	so that the actual characters can be retrieved later.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetKeycodeAndState(
    Tk_Window tkwin,
    KeySym keysym,
    XEvent *eventPtr)
{
    if (keysym == NoSymbol) {
	eventPtr->xkey.keycode = 0;
    } else if ( modKeyArray[0] <= keysym &&
		keysym <= modKeyArray[NUM_MOD_KEYCODES - 1]) {
	/*
	 * Keysyms for pure modifiers only arise in generated events.
	 * We should just copy them to the keycode.
	 */
	eventPtr->xkey.keycode = keysym;
    } else {
	Display *display = Tk_Display(tkwin);
	int macKeycode = XKeysymToMacKeycode(display, keysym);

	/*
	 * See also XKeysymToKeycode.
	 */
	if ((keysym >= XK_F1) && (keysym <= XK_F35)) {
	    eventPtr->xkey.keycode = 0x0010;
	} else {
	    eventPtr->xkey.keycode = 0x00FF & keysym;
	}
	eventPtr->xkey.keycode |= (macKeycode & MAC_KEYCODE_MASK) << 16;

	if (shiftKey & macKeycode) {
	    eventPtr->xkey.state |= ShiftMask;
	}
	if (optionKey & macKeycode) {
	    eventPtr->xkey.state |= OPTION_MASK;
	}

	if (keysym <= LATIN1_MAX) {
	    int done = TkUniCharToUtf(keysym, eventPtr->xkey.trans_chars);

	    eventPtr->xkey.trans_chars[done] = 0;
	} else {
	    eventPtr->xkey.trans_chars[0] = 0;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetKeySym --
 *
 *	Given an X KeyPress or KeyRelease event, map the keycode in the event
 *	into a keysym.
 *
 * Results:
 *	The return value is the keysym corresponding to eventPtr, or NoSymbol
 *	if no matching keysym could be found.
 *
 * Side effects:
 *	In the first call for a given display, keycode-to-keysym maps get
 *	loaded.
 *
 *----------------------------------------------------------------------
 */

KeySym
TkpGetKeySym(
    TkDisplay *dispPtr,		/* Display in which to map keycode. */
    XEvent *eventPtr)		/* Description of X event. */
{
    KeySym sym;
    int index;

    /*
     * Refresh the mapping information if it's stale.
     */

    if (dispPtr->bindInfoStale) {
	TkpInitKeymapInfo(dispPtr);
    }

    /*
     * Handle pure modifier keys specially. We use -1 as a signal for
     * this.
     */

    if (eventPtr->xany.send_event == -1) {
	int modifier = eventPtr->xkey.keycode & NSDeviceIndependentModifierFlagsMask;

	if (modifier == NSCommandKeyMask) {
	    return XK_Meta_L;
	} else if (modifier == NSShiftKeyMask) {
	    return XK_Shift_L;
	} else if (modifier == NSAlphaShiftKeyMask) {
	    return XK_Caps_Lock;
	} else if (modifier == NSAlternateKeyMask) {
	    return XK_Alt_L;
	} else if (modifier == NSControlKeyMask) {
	    return XK_Control_L;
	} else if (modifier == NSNumericPadKeyMask) {
	    return XK_Num_Lock;
	} else if (modifier == NSFunctionKeyMask) {
	    return XK_Super_L;
/*
	} else if (modifier == rightShiftKey) {
	    return XK_Shift_R;
	} else if (modifier == rightOptionKey) {
	    return XK_Alt_R;
	} else if (modifier == rightControlKey) {
	    return XK_Control_R;
*/
	} else {
	    /*
	     * If we get here, we probably need to implement something new.
	     */

	    return NoSymbol;
	}
    }

    /* If nbytes has been set, it's not a function key, but a regular key that
       has been translated in tkMacOSXKeyEvent.c; just use that. */
    if (eventPtr->xkey.nbytes) {
      return eventPtr->xkey.keycode;
    }

    /*
     * Figure out which of the four slots in the keymap vector to use for this
     * key. Refer to Xlib documentation for more info on how this computation
     * works. (Note: We use "Option" in keymap columns 2 and 3 where other
     * implementations have "Mode_switch".)
     */

    index = 0;

    /*
     * We want Option key combinations to use their base chars as keysyms, so
     * we ignore the option modifier here.
     */

#if 0
    if (eventPtr->xkey.state & OPTION_MASK) {
	index |= 2;
    }
#endif

    if ((eventPtr->xkey.state & ShiftMask)
	    || (/* (dispPtr->lockUsage != LU_IGNORE)
	    && */ (eventPtr->xkey.state & LockMask))) {
	index |= 1;
    }

    /*
     * First try of the actual translation.
     */

    sym = XKeycodeToKeysym(dispPtr->display, eventPtr->xkey.keycode, index);

    /*
     * Special handling: If the key was shifted because of Lock, but lock is
     * only caps lock, not shift lock, and the shifted keysym isn't upper-case
     * alphabetic, then switch back to the unshifted keysym.
     */

    if ((index & 1) && !(eventPtr->xkey.state & ShiftMask)
	    /*&& (dispPtr->lockUsage == LU_CAPS)*/ ) {
	/*
	 * FIXME: Keysyms are only identical to Unicode for ASCII and Latin-1,
	 * so we can't use Tcl_UniCharIsUpper() for keysyms outside that range.
	 * This may be a serious problem here.
	 */

	if ((sym == NoSymbol) || (sym > LATIN1_MAX)
		|| !Tcl_UniCharIsUpper(sym)) {
	    index &= ~1;
	    sym = XKeycodeToKeysym(dispPtr->display, eventPtr->xkey.keycode,
		    index);
	}
    }

    /*
     * Another bit of special handling: If this is a shifted key and there is
     * no keysym defined, then use the keysym for the unshifted key.
     */

    if ((index & 1) && (sym == NoSymbol)) {
	sym = XKeycodeToKeysym(dispPtr->display, eventPtr->xkey.keycode,
		index & ~1);
    }
    return sym;
}

/*
 *--------------------------------------------------------------
 *
 * TkpInitKeymapInfo --
 *
 *	This procedure is invoked to scan keymap information to recompute stuff
 *	that's important for binding, such as the modifier key (if any) that
 *	corresponds to the "Mode_switch" keysym.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Keymap-related information in dispPtr is updated.
 *
 *--------------------------------------------------------------
 */

void
TkpInitKeymapInfo(
    TkDisplay *dispPtr)		/* Display for which to recompute keymap
				 * information. */
{
    dispPtr->bindInfoStale = 0;

    /*
     * Behaviours that are variable on X11 are defined constant on MacOSX.
     * lockUsage is only used above in TkpGetKeySym(), nowhere else currently.
     * There is no offical "Mode_switch" key.
     */

    dispPtr->lockUsage = LU_CAPS;
    dispPtr->modeModMask = 0;

#if 0
    /*
     * With this, <Alt> and <Meta> become synonyms for <Command> and <Option>
     * in bindings like they are (and always have been) in the keysyms that
     * are reported by KeyPress events. But the init scripts like text.tcl
     * have some disabling bindings for <Meta>, so we don't want this without
     * some changes in those scripts. See also bug #700311.
     */

    dispPtr->altModMask = OPTION_MASK;
    dispPtr->metaModMask = COMMAND_MASK;
#else
    dispPtr->altModMask = 0;
    dispPtr->metaModMask = 0;
#endif

    /*
     * MacOSX doesn't create a key event when a modifier key is pressed or
     * released.  However, it is possible to generate key events for
     * modifier keys, and this is done in the tests.  So we construct an array
     * containing the keycodes of the standard modifier keys from static data.
     */

    if (dispPtr->modKeyCodes != NULL) {
	ckfree(dispPtr->modKeyCodes);
    }
    dispPtr->numModKeyCodes = NUM_MOD_KEYCODES;
    dispPtr->modKeyCodes = (KeyCode *)ckalloc(NUM_MOD_KEYCODES * sizeof(KeyCode));
    for (int i = 0; i < NUM_MOD_KEYCODES; i++) {
	dispPtr->modKeyCodes[i] = modKeyArray[i];
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
