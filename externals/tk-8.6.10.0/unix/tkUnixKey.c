/*
 * tkUnixKey.c --
 *
 *	This file contains routines for dealing with international keyboard
 *	input.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
** Bug [3607830]: Before using Xkb, it must be initialized.  TkpOpenDisplay
**                does this and sets the USE_XKB flag if xkb is supported.
**                (should this be function ptr?)
*/
#ifdef HAVE_XKBKEYCODETOKEYSYM
#  include <X11/XKBlib.h>
#else
#  define XkbKeycodeToKeysym(D,K,G,L) XKeycodeToKeysym(D,K,L)
#endif
#define TkKeycodeToKeysym(D,K,G,L)		\
    ((D)->flags & TK_DISPLAY_USE_XKB) ?		\
      XkbKeycodeToKeysym((D)->display,K,G,L) :	\
      XKeycodeToKeysym((D)->display,K,L)

/*
 * Prototypes for local functions defined in this file:
 */

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetCaretPos --
 *
 *	This enables correct placement of the XIM caret. This is called by
 *	widgets to indicate their cursor placement. This is currently only
 *	used for over-the-spot XIM.
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
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;

    if ((dispPtr->caret.winPtr == winPtr)
	    && (dispPtr->caret.x == x)
	    && (dispPtr->caret.y == y)
	    && (dispPtr->caret.height == height)) {
	return;
    }

    dispPtr->caret.winPtr = winPtr;
    dispPtr->caret.x = x;
    dispPtr->caret.y = y;
    dispPtr->caret.height = height;

    /*
     * Adjust the XIM caret position.
     */

#ifdef TK_USE_INPUT_METHODS
    if ((dispPtr->flags & TK_DISPLAY_USE_IM)
	    && (dispPtr->inputStyle & XIMPreeditPosition)
	    && (winPtr->inputContext != NULL)) {
	XVaNestedList preedit_attr;
	XPoint spot;

	spot.x = dispPtr->caret.x;
	spot.y = dispPtr->caret.y + dispPtr->caret.height;
	preedit_attr = XVaCreateNestedList(0, XNSpotLocation, &spot, NULL);
	XSetICValues(winPtr->inputContext, XNPreeditAttributes, preedit_attr,
		NULL);
	XFree(preedit_attr);
    }
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetString --
 *
 *	Retrieve the UTF string associated with a keyboard event.
 *
 * Results:
 *	Returns the UTF string.
 *
 * Side effects:
 *	Stores the input string in the specified Tcl_DString. Modifies the
 *	internal input state. This routine can only be called once for a given
 *	event.
 *
 *----------------------------------------------------------------------
 */

const char *
TkpGetString(
    TkWindow *winPtr,		/* Window where event occurred */
    XEvent *eventPtr,		/* X keyboard event. */
    Tcl_DString *dsPtr)		/* Initialized, empty string to hold result. */
{
    int len;
    Tcl_DString buf;
    TkKeyEvent *kePtr = (TkKeyEvent *) eventPtr;

    /*
     * If we have the value cached already, use it now. [Bug 1373712]
     */

    if (kePtr->charValuePtr != NULL) {
	Tcl_DStringSetLength(dsPtr, kePtr->charValueLen);
	memcpy(Tcl_DStringValue(dsPtr), kePtr->charValuePtr,
		(unsigned) kePtr->charValueLen+1);
	return Tcl_DStringValue(dsPtr);
    }

    /*
     * Only do this for KeyPress events, otherwise
     * further Xlib function behavior might be undefined.
     */

    if (eventPtr->type != KeyPress) {
	len = 0;
	Tcl_DStringSetLength(dsPtr, len);
	goto done;
    }

#ifdef TK_USE_INPUT_METHODS
    if ((winPtr->dispPtr->flags & TK_DISPLAY_USE_IM)
	    && (winPtr->inputContext != NULL)
	    && (eventPtr->type == KeyPress)) {
	Status status;

#if X_HAVE_UTF8_STRING
	Tcl_DStringSetLength(dsPtr, TCL_DSTRING_STATIC_SIZE-1);
	len = Xutf8LookupString(winPtr->inputContext, &eventPtr->xkey,
		Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr),
		&kePtr->keysym, &status);

	if (status == XBufferOverflow) {
	    /*
	     * Expand buffer and try again.
	     */

	    Tcl_DStringSetLength(dsPtr, len);
	    len = Xutf8LookupString(winPtr->inputContext, &eventPtr->xkey,
		    Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr),
		    &kePtr->keysym, &status);
	}
	if ((status != XLookupChars) && (status != XLookupBoth)) {
	    len = 0;
	}
	Tcl_DStringSetLength(dsPtr, len);
#else /* !X_HAVE_UTF8_STRING */
	/*
	 * Overallocate the dstring to the maximum stack amount.
	 */

	Tcl_DStringInit(&buf);
	Tcl_DStringSetLength(&buf, TCL_DSTRING_STATIC_SIZE-1);
	len = XmbLookupString(winPtr->inputContext, &eventPtr->xkey,
		Tcl_DStringValue(&buf), Tcl_DStringLength(&buf),
                &kePtr->keysym, &status);

	/*
	 * If the buffer wasn't big enough, grow the buffer and try again.
	 */

	if (status == XBufferOverflow) {
	    Tcl_DStringSetLength(&buf, len);
	    len = XmbLookupString(winPtr->inputContext, &eventPtr->xkey,
		    Tcl_DStringValue(&buf), len, &kePtr->keysym, &status);
	}
	if ((status != XLookupChars) && (status != XLookupBoth)) {
	    len = 0;
	}
	Tcl_DStringSetLength(&buf, len);
	Tcl_ExternalToUtfDString(NULL, Tcl_DStringValue(&buf), len, dsPtr);
	Tcl_DStringFree(&buf);
#endif /* X_HAVE_UTF8_STRING */
    } else
#endif /* TK_USE_INPUT_METHODS */
    {
	/*
	 * Fall back to convert a keyboard event to a UTF-8 string using
	 * XLookupString. This is used when input methods are turned off and
	 * for KeyRelease events.
	 *
	 * Note: XLookupString() normally returns a single ISO Latin 1 or
	 * ASCII control character.
	 */

	Tcl_DStringInit(&buf);
	Tcl_DStringSetLength(&buf, TCL_DSTRING_STATIC_SIZE-1);
	len = XLookupString(&eventPtr->xkey, Tcl_DStringValue(&buf),
		TCL_DSTRING_STATIC_SIZE, &kePtr->keysym, 0);
	Tcl_DStringValue(&buf)[len] = '\0';

	if (len == 1) {
	    len = TkUniCharToUtf((unsigned char) Tcl_DStringValue(&buf)[0],
		    Tcl_DStringValue(dsPtr));
	    Tcl_DStringSetLength(dsPtr, len);
	} else {
	    /*
	     * len > 1 should only happen if someone has called XRebindKeysym.
	     * Assume UTF-8.
	     */

	    Tcl_DStringSetLength(dsPtr, len);
	    strncpy(Tcl_DStringValue(dsPtr), Tcl_DStringValue(&buf), len);
	}
    }

    /*
     * Cache the string in the event so that if/when we return to this
     * function, we will be able to produce it without asking X. This stops us
     * from having to reenter the XIM engine. [Bug 1373712]
     */

done:
    kePtr->charValuePtr = ckalloc(len + 1);
    kePtr->charValueLen = len;
    memcpy(kePtr->charValuePtr, Tcl_DStringValue(dsPtr), (unsigned) len + 1);
    return Tcl_DStringValue(dsPtr);
}

/*
 * When mapping from a keysym to a keycode, need information about the
 * modifier state to be used so that when they call TkKeycodeToKeysym taking
 * into account the xkey.state, they will get back the original keysym.
 */

void
TkpSetKeycodeAndState(
    Tk_Window tkwin,
    KeySym keySym,
    XEvent *eventPtr)
{
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;
    int state, mincode, maxcode;
    KeyCode keycode;

    if (keySym == NoSymbol) {
	keycode = 0;
    } else {
	keycode = XKeysymToKeycode(dispPtr->display, keySym);
    }
    eventPtr->xkey.keycode = keycode;
    if (keycode != 0) {
	for (state = 0; state < 4; state++) {
	    if (XLookupKeysym(&eventPtr->xkey, state) == keySym) {
		if (state & 1) {
		    eventPtr->xkey.state |= ShiftMask;
		}
		if (state & 2) {
		    eventPtr->xkey.state |= dispPtr->modeModMask;
		}
		break;
	    }
	}
    }

    /*
     * Filter keycodes out of range, otherwise further Xlib function
     * behavior might be undefined, in particular XIM could cause crashes.
     */

    mincode = 0;
    maxcode = -1;
    XDisplayKeycodes(dispPtr->display, &mincode, &maxcode);
    if (keycode < mincode) {
	keycode = mincode;
    } else if (keycode > maxcode) {
	keycode = maxcode;
    }

    eventPtr->xkey.keycode = keycode;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetKeySym --
 *
 *	Given an X KeyPress or KeyRelease event, map the keycode in the event
 *	into a KeySym.
 *
 * Results:
 *	The return value is the KeySym corresponding to eventPtr, or NoSymbol
 *	if no matching Keysym could be found.
 *
 * Side effects:
 *	In the first call for a given display, keycode-to-KeySym maps get
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
    TkKeyEvent* kePtr = (TkKeyEvent*) eventPtr;

    /*
     * Refresh the mapping information if it's stale. This must happen before
     * we do any input method processing. [Bug 3599312]
     */

    if (dispPtr->bindInfoStale) {
	TkpInitKeymapInfo(dispPtr);
    }

#ifdef TK_USE_INPUT_METHODS
    /*
     * If input methods are active, we may already have determined a keysym.
     * Return it.
     */

    if (eventPtr->type == KeyPress && dispPtr
	    && (dispPtr->flags & TK_DISPLAY_USE_IM)) {
	if (kePtr->charValuePtr == NULL) {
	    Tcl_DString ds;
	    TkWindow *winPtr = (TkWindow *)
		Tk_IdToWindow(eventPtr->xany.display, eventPtr->xany.window);

	    Tcl_DStringInit(&ds);
	    (void) TkpGetString(winPtr, eventPtr, &ds);
	    Tcl_DStringFree(&ds);
	}
	if (kePtr->charValuePtr != NULL) {
	    return kePtr->keysym;
	}
    }
#endif

    /*
     * Figure out which of the four slots in the keymap vector to use for this
     * key. Refer to Xlib documentation for more info on how this computation
     * works.
     */

    index = 0;
    if (eventPtr->xkey.state & dispPtr->modeModMask) {
	index = 2;
    }
    if ((eventPtr->xkey.state & ShiftMask)
	    || ((dispPtr->lockUsage != LU_IGNORE)
	    && (eventPtr->xkey.state & LockMask))) {
	index += 1;
    }
    sym = TkKeycodeToKeysym(dispPtr, eventPtr->xkey.keycode, 0,
	    index);

    /*
     * Special handling: if the key was shifted because of Lock, but lock is
     * only caps lock, not shift lock, and the shifted keysym isn't upper-case
     * alphabetic, then switch back to the unshifted keysym.
     */

    if ((index & 1) && !(eventPtr->xkey.state & ShiftMask)
	    && (dispPtr->lockUsage == LU_CAPS)) {
	if (!(((sym >= XK_A) && (sym <= XK_Z))
		|| ((sym >= XK_Agrave) && (sym <= XK_Odiaeresis))
		|| ((sym >= XK_Ooblique) && (sym <= XK_Thorn)))) {
	    index &= ~1;
	    sym = TkKeycodeToKeysym(dispPtr, eventPtr->xkey.keycode,
		    0, index);
	}
    }

    /*
     * Another bit of special handling: if this is a shifted key and there is
     * no keysym defined, then use the keysym for the unshifted key.
     */

    if ((index & 1) && (sym == NoSymbol)) {
	sym = TkKeycodeToKeysym(dispPtr, eventPtr->xkey.keycode,
		0, index & ~1);
    }
    return sym;
}

/*
 *--------------------------------------------------------------
 *
 * TkpInitKeymapInfo --
 *
 *	This function is invoked to scan keymap information to recompute stuff
 *	that's important for binding, such as the modifier key (if any) that
 *	corresponds to "mode switch".
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
    XModifierKeymap *modMapPtr;
    KeyCode *codePtr;
    KeySym keysym;
    int count, i, j, max, arraySize;
#define KEYCODE_ARRAY_SIZE 20

    dispPtr->bindInfoStale = 0;
    modMapPtr = XGetModifierMapping(dispPtr->display);

    /*
     * Check the keycodes associated with the Lock modifier. If any of them is
     * associated with the XK_Shift_Lock modifier, then Lock has to be
     * interpreted as Shift Lock, not Caps Lock.
     */

    dispPtr->lockUsage = LU_IGNORE;
    codePtr = modMapPtr->modifiermap + modMapPtr->max_keypermod*LockMapIndex;
    for (count = modMapPtr->max_keypermod; count > 0; count--, codePtr++) {
	if (*codePtr == 0) {
	    continue;
	}
	keysym = TkKeycodeToKeysym(dispPtr, *codePtr, 0, 0);
	if (keysym == XK_Shift_Lock) {
	    dispPtr->lockUsage = LU_SHIFT;
	    break;
	}
	if (keysym == XK_Caps_Lock) {
	    dispPtr->lockUsage = LU_CAPS;
	    break;
	}
    }

    /*
     * Look through the keycodes associated with modifiers to see if the the
     * "mode switch", "meta", or "alt" keysyms are associated with any
     * modifiers. If so, remember their modifier mask bits.
     */

    dispPtr->modeModMask = 0;
    dispPtr->metaModMask = 0;
    dispPtr->altModMask = 0;
    codePtr = modMapPtr->modifiermap;
    max = 8 * modMapPtr->max_keypermod;
    for (i = 0; i < max; i++, codePtr++) {
	if (*codePtr == 0) {
	    continue;
	}
	keysym = TkKeycodeToKeysym(dispPtr, *codePtr, 0, 0);
	if (keysym == XK_Mode_switch) {
	    dispPtr->modeModMask |= ShiftMask << (i/modMapPtr->max_keypermod);
	}
	if ((keysym == XK_Meta_L) || (keysym == XK_Meta_R)) {
	    dispPtr->metaModMask |= ShiftMask << (i/modMapPtr->max_keypermod);
	}
	if ((keysym == XK_Alt_L) || (keysym == XK_Alt_R)) {
	    dispPtr->altModMask |= ShiftMask << (i/modMapPtr->max_keypermod);
	}
    }

    /*
     * Create an array of the keycodes for all modifier keys.
     */

    if (dispPtr->modKeyCodes != NULL) {
	ckfree(dispPtr->modKeyCodes);
    }
    dispPtr->numModKeyCodes = 0;
    arraySize = KEYCODE_ARRAY_SIZE;
    dispPtr->modKeyCodes = ckalloc(KEYCODE_ARRAY_SIZE * sizeof(KeyCode));
    for (i = 0, codePtr = modMapPtr->modifiermap; i < max; i++, codePtr++) {
	if (*codePtr == 0) {
	    continue;
	}

	/*
	 * Make sure that the keycode isn't already in the array.
	 */

	for (j = 0; j < dispPtr->numModKeyCodes; j++) {
	    if (dispPtr->modKeyCodes[j] == *codePtr) {
		/*
		 * 'continue' the outer loop.
		 */

		goto nextModCode;
	    }
	}
	if (dispPtr->numModKeyCodes >= arraySize) {
	    KeyCode *newCodes;

	    /*
	     * Ran out of space in the array; grow it.
	     */

	    arraySize *= 2;
	    newCodes = ckalloc(arraySize * sizeof(KeyCode));
	    memcpy(newCodes, dispPtr->modKeyCodes,
		    dispPtr->numModKeyCodes * sizeof(KeyCode));
	    ckfree(dispPtr->modKeyCodes);
	    dispPtr->modKeyCodes = newCodes;
	}
	dispPtr->modKeyCodes[dispPtr->numModKeyCodes] = *codePtr;
	dispPtr->numModKeyCodes++;
    nextModCode:
	continue;
    }
    XFreeModifiermap(modMapPtr);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
