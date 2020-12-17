/*
 * tkTextTag.c --
 *
 *	This module implements the "tag" subcommand of the widget command for
 *	text widgets, plus most of the other high-level functions related to
 *	tags.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "default.h"
#include "tkInt.h"
#include "tkText.h"

/*
 * The 'TkWrapMode' enum in tkText.h is used to define a type for the -wrap
 * option of tags in a Text widget. These values are used as indices into the
 * string table below. Tags are allowed an empty wrap value, but the widget as
 * a whole is not.
 */

static const char *const wrapStrings[] = {
    "char", "none", "word", "", NULL
};

/*
 * The 'TkTextTabStyle' enum in tkText.h is used to define a type for the
 * -tabstyle option of the Text widget. These values are used as indices into
 * the string table below. Tags are allowed an empty tabstyle value, but the
 * widget as a whole is not.
 */

static const char *const tabStyleStrings[] = {
    "tabular", "wordprocessor", "", NULL
};

static const Tk_OptionSpec tagOptionSpecs[] = {
    {TK_OPTION_BORDER, "-background", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, border), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BITMAP, "-bgstipple", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, bgStipple), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-borderwidth", NULL, NULL,
	NULL, Tk_Offset(TkTextTag, borderWidthPtr), Tk_Offset(TkTextTag, borderWidth),
	TK_OPTION_NULL_OK|TK_OPTION_DONT_SET_DEFAULT, 0, 0},
    {TK_OPTION_STRING, "-elide", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, elideString),
	TK_OPTION_NULL_OK|TK_OPTION_DONT_SET_DEFAULT, 0, 0},
    {TK_OPTION_BITMAP, "-fgstipple", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, fgStipple), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_FONT, "-font", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, tkfont), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-foreground", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, fgColor), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-justify", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, justifyString), TK_OPTION_NULL_OK, 0,0},
    {TK_OPTION_STRING, "-lmargin1", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, lMargin1String), TK_OPTION_NULL_OK,0,0},
    {TK_OPTION_STRING, "-lmargin2", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, lMargin2String), TK_OPTION_NULL_OK,0,0},
    {TK_OPTION_BORDER, "-lmargincolor", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, lMarginColor), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-offset", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, offsetString), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-overstrike", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, overstrikeString),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-overstrikefg", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, overstrikeColor),
        TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-relief", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, reliefString), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-rmargin", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, rMarginString), TK_OPTION_NULL_OK, 0,0},
    {TK_OPTION_BORDER, "-rmargincolor", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, rMarginColor), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BORDER, "-selectbackground", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, selBorder), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-selectforeground", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, selFgColor), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-spacing1", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, spacing1String), TK_OPTION_NULL_OK,0,0},
    {TK_OPTION_STRING, "-spacing2", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, spacing2String), TK_OPTION_NULL_OK,0,0},
    {TK_OPTION_STRING, "-spacing3", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, spacing3String), TK_OPTION_NULL_OK,0,0},
    {TK_OPTION_STRING, "-tabs", NULL, NULL,
	NULL, Tk_Offset(TkTextTag, tabStringPtr), -1, TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING_TABLE, "-tabstyle", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, tabStyle),
	TK_OPTION_NULL_OK, tabStyleStrings, 0},
    {TK_OPTION_STRING, "-underline", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, underlineString),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-underlinefg", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, underlineColor),
        TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING_TABLE, "-wrap", NULL, NULL,
	NULL, -1, Tk_Offset(TkTextTag, wrapMode),
	TK_OPTION_NULL_OK, wrapStrings, 0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0}
};

/*
 * Forward declarations for functions defined later in this file:
 */

static void		ChangeTagPriority(TkText *textPtr, TkTextTag *tagPtr,
			    int prio);
static TkTextTag *	FindTag(Tcl_Interp *interp, TkText *textPtr,
			    Tcl_Obj *tagName);
static void		SortTags(int numTags, TkTextTag **tagArrayPtr);
static int		TagSortProc(const void *first, const void *second);
static void		TagBindEvent(TkText *textPtr, XEvent *eventPtr,
			    int numTags, TkTextTag **tagArrayPtr);

/*
 *--------------------------------------------------------------
 *
 * TkTextTagCmd --
 *
 *	This function is invoked to process the "tag" options of the widget
 *	command for text widgets. See the user documentation for details on
 *	what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
TkTextTagCmd(
    register TkText *textPtr,	/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. Someone else has already
				 * parsed this command enough to know that
				 * objv[1] is "tag". */
{
    static const char *const tagOptionStrings[] = {
	"add", "bind", "cget", "configure", "delete", "lower", "names",
	"nextrange", "prevrange", "raise", "ranges", "remove", NULL
    };
    enum tagOptions {
	TAG_ADD, TAG_BIND, TAG_CGET, TAG_CONFIGURE, TAG_DELETE, TAG_LOWER,
	TAG_NAMES, TAG_NEXTRANGE, TAG_PREVRANGE, TAG_RAISE, TAG_RANGES,
	TAG_REMOVE
    };
    int optionIndex, i;
    register TkTextTag *tagPtr;
    TkTextIndex index1, index2;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[2], tagOptionStrings,
	    sizeof(char *), "tag option", 0, &optionIndex) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum tagOptions)optionIndex) {
    case TAG_ADD:
    case TAG_REMOVE: {
	int addTag;

	if (((enum tagOptions)optionIndex) == TAG_ADD) {
	    addTag = 1;
	} else {
	    addTag = 0;
	}
	if (objc < 5) {
	    Tcl_WrongNumArgs(interp, 3, objv,
		    "tagName index1 ?index2 index1 index2 ...?");
	    return TCL_ERROR;
	}
	tagPtr = TkTextCreateTag(textPtr, Tcl_GetString(objv[3]), NULL);
	if (tagPtr->elide) {
		/*
		* Indices are potentially obsolete after adding or removing
		* elided character ranges, especially indices having "display"
		* or "any" submodifier, therefore increase the epoch.
		*/
		textPtr->sharedTextPtr->stateEpoch++;
	}
	for (i = 4; i < objc; i += 2) {
	    if (TkTextGetObjIndex(interp, textPtr, objv[i],
		    &index1) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (objc > (i+1)) {
		if (TkTextGetObjIndex(interp, textPtr, objv[i+1],
			&index2) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (TkTextIndexCmp(&index1, &index2) >= 0) {
		    return TCL_OK;
		}
	    } else {
		index2 = index1;
		TkTextIndexForwChars(NULL,&index2, 1, &index2, COUNT_INDICES);
	    }

	    if (tagPtr->affectsDisplay) {
		TkTextRedrawTag(textPtr->sharedTextPtr, NULL, &index1, &index2,
			tagPtr, !addTag);
	    } else {
		/*
		 * Still need to trigger enter/leave events on tags that have
		 * changed.
		 */

		TkTextEventuallyRepick(textPtr);
	    }
	    if (TkBTreeTag(&index1, &index2, tagPtr, addTag)) {
		/*
		 * If the tag is "sel", and we actually adjusted something
		 * then grab the selection if we're supposed to export it and
		 * don't already have it.
		 *
		 * Also, invalidate partially-completed selection retrievals.
		 * We only need to check whether the tag is "sel" for this
		 * textPtr (not for other peer widget's "sel" tags) because we
		 * cannot reach this code path with a different widget's "sel"
		 * tag.
		 */

		if (tagPtr == textPtr->selTagPtr) {
		    /*
		     * Send an event that the selection changed. This is
		     * equivalent to:
		     *	   event generate $textWidget <<Selection>>
		     */

		    TkTextSelectionEvent(textPtr);

		    if (addTag && textPtr->exportSelection
			    && (!Tcl_IsSafe(textPtr->interp))
			    && !(textPtr->flags & GOT_SELECTION)) {
			Tk_OwnSelection(textPtr->tkwin, XA_PRIMARY,
				TkTextLostSelection, textPtr);
			textPtr->flags |= GOT_SELECTION;
		    }
		    textPtr->abortSelections = 1;
		}
	    }
	}
	break;
    }
    case TAG_BIND:
	if ((objc < 4) || (objc > 6)) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName ?sequence? ?command?");
	    return TCL_ERROR;
	}
	tagPtr = TkTextCreateTag(textPtr, Tcl_GetString(objv[3]), NULL);

	/*
	 * Make a binding table if the widget doesn't already have one.
	 */

	if (textPtr->sharedTextPtr->bindingTable == NULL) {
	    textPtr->sharedTextPtr->bindingTable =
		    Tk_CreateBindingTable(interp);
	}

	if (objc == 6) {
	    int append = 0;
	    unsigned long mask;
	    const char *fifth = Tcl_GetString(objv[5]);

	    if (fifth[0] == 0) {
		return Tk_DeleteBinding(interp,
			textPtr->sharedTextPtr->bindingTable,
			(ClientData) tagPtr->name, Tcl_GetString(objv[4]));
	    }
	    if (fifth[0] == '+') {
		fifth++;
		append = 1;
	    }
	    mask = Tk_CreateBinding(interp,
		    textPtr->sharedTextPtr->bindingTable,
		    (ClientData) tagPtr->name, Tcl_GetString(objv[4]), fifth,
		    append);
	    if (mask == 0) {
		return TCL_ERROR;
	    }
	    if (mask & ~(unsigned long)(ButtonMotionMask|Button1MotionMask
		    |Button2MotionMask|Button3MotionMask|Button4MotionMask
		    |Button5MotionMask|ButtonPressMask|ButtonReleaseMask
		    |EnterWindowMask|LeaveWindowMask|KeyPressMask
		    |KeyReleaseMask|PointerMotionMask|VirtualEventMask)) {
		Tk_DeleteBinding(interp, textPtr->sharedTextPtr->bindingTable,
			(ClientData) tagPtr->name, Tcl_GetString(objv[4]));
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"requested illegal events; only key, button, motion,"
			" enter, leave, and virtual events may be used", -1));
		Tcl_SetErrorCode(interp, "TK", "TEXT", "TAG_BIND_EVENT",NULL);
		return TCL_ERROR;
	    }
	} else if (objc == 5) {
	    const char *command;

	    command = Tk_GetBinding(interp,
		    textPtr->sharedTextPtr->bindingTable,
		    (ClientData) tagPtr->name, Tcl_GetString(objv[4]));
	    if (command == NULL) {
		const char *string = Tcl_GetString(Tcl_GetObjResult(interp));

		/*
		 * Ignore missing binding errors. This is a special hack that
		 * relies on the error message returned by FindSequence in
		 * tkBind.c.
		 */

		if (string[0] != '\0') {
		    return TCL_ERROR;
		}
		Tcl_ResetResult(interp);
	    } else {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(command, -1));
	    }
	} else {
	    Tk_GetAllBindings(interp, textPtr->sharedTextPtr->bindingTable,
		    (ClientData) tagPtr->name);
	}
	break;
    case TAG_CGET:
	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 1, objv, "tag cget tagName option");
	    return TCL_ERROR;
	} else {
	    Tcl_Obj *objPtr;

	    tagPtr = FindTag(interp, textPtr, objv[3]);
	    if (tagPtr == NULL) {
		return TCL_ERROR;
	    }
	    objPtr = Tk_GetOptionValue(interp, (char *) tagPtr,
		    tagPtr->optionTable, objv[4], textPtr->tkwin);
	    if (objPtr == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	    return TCL_OK;
	}
	break;
    case TAG_CONFIGURE: {
	int newTag;

	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 3, objv,
		    "tagName ?-option? ?value? ?-option value ...?");
	    return TCL_ERROR;
	}
	tagPtr = TkTextCreateTag(textPtr, Tcl_GetString(objv[3]), &newTag);
	if (objc <= 5) {
	    Tcl_Obj *objPtr = Tk_GetOptionInfo(interp, (char *) tagPtr,
		    tagPtr->optionTable,
		    (objc == 5) ? objv[4] : NULL, textPtr->tkwin);

	    if (objPtr == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	    return TCL_OK;
	} else {
	    int result = TCL_OK;

	    if (Tk_SetOptions(interp, (char *) tagPtr, tagPtr->optionTable,
		    objc-4, objv+4, textPtr->tkwin, NULL, NULL) != TCL_OK) {
		return TCL_ERROR;
	    }

	    /*
	     * Some of the configuration options, like -underline and
	     * -justify, require additional translation (this is needed
	     * because we need to distinguish a particular value of an option
	     * from "unspecified").
	     */

	    if (tagPtr->borderWidth < 0) {
		tagPtr->borderWidth = 0;
	    }
	    if (tagPtr->reliefString != NULL) {
		if (Tk_GetRelief(interp, tagPtr->reliefString,
			&tagPtr->relief) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->justifyString != NULL) {
		if (Tk_GetJustify(interp, tagPtr->justifyString,
			&tagPtr->justify) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->lMargin1String != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->lMargin1String, &tagPtr->lMargin1) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->lMargin2String != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->lMargin2String, &tagPtr->lMargin2) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->offsetString != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin, tagPtr->offsetString,
			&tagPtr->offset) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->overstrikeString != NULL) {
		if (Tcl_GetBoolean(interp, tagPtr->overstrikeString,
			&tagPtr->overstrike) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->rMarginString != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->rMarginString, &tagPtr->rMargin) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->spacing1String != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->spacing1String, &tagPtr->spacing1) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (tagPtr->spacing1 < 0) {
		    tagPtr->spacing1 = 0;
		}
	    }
	    if (tagPtr->spacing2String != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->spacing2String, &tagPtr->spacing2) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (tagPtr->spacing2 < 0) {
		    tagPtr->spacing2 = 0;
		}
	    }
	    if (tagPtr->spacing3String != NULL) {
		if (Tk_GetPixels(interp, textPtr->tkwin,
			tagPtr->spacing3String, &tagPtr->spacing3) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (tagPtr->spacing3 < 0) {
		    tagPtr->spacing3 = 0;
		}
	    }
	    if (tagPtr->tabArrayPtr != NULL) {
		ckfree(tagPtr->tabArrayPtr);
		tagPtr->tabArrayPtr = NULL;
	    }
	    if (tagPtr->tabStringPtr != NULL) {
		tagPtr->tabArrayPtr =
			TkTextGetTabs(interp, textPtr, tagPtr->tabStringPtr);
		if (tagPtr->tabArrayPtr == NULL) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->underlineString != NULL) {
		if (Tcl_GetBoolean(interp, tagPtr->underlineString,
			&tagPtr->underline) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    if (tagPtr->elideString != NULL) {
		if (Tcl_GetBoolean(interp, tagPtr->elideString,
			&tagPtr->elide) != TCL_OK) {
		    return TCL_ERROR;
		}

		/*
		 * Indices are potentially obsolete after changing -elide,
		 * especially those computed with "display" or "any"
		 * submodifier, therefore increase the epoch.
		 */

		textPtr->sharedTextPtr->stateEpoch++;
	    }

	    /*
	     * If the "sel" tag was changed, be sure to mirror information
	     * from the tag back into the text widget record. NOTE: we don't
	     * have to free up information in the widget record before
	     * overwriting it, because it was mirrored in the tag and hence
	     * freed when the tag field was overwritten.
	     */

	    if (tagPtr == textPtr->selTagPtr) {
                if (tagPtr->selBorder == NULL) {
                    textPtr->selBorder = tagPtr->border;
                } else {
                    textPtr->selBorder = tagPtr->selBorder;
                }
		textPtr->selBorderWidth = tagPtr->borderWidth;
		textPtr->selBorderWidthPtr = tagPtr->borderWidthPtr;
                if (tagPtr->selFgColor == NULL) {
                    textPtr->selFgColorPtr = tagPtr->fgColor;
                } else {
                    textPtr->selFgColorPtr = tagPtr->selFgColor;
                }
	    }

	    tagPtr->affectsDisplay = 0;
	    tagPtr->affectsDisplayGeometry = 0;
	    if ((tagPtr->elideString != NULL)
		    || (tagPtr->tkfont != NULL)
		    || (tagPtr->justifyString != NULL)
		    || (tagPtr->lMargin1String != NULL)
		    || (tagPtr->lMargin2String != NULL)
		    || (tagPtr->offsetString != NULL)
		    || (tagPtr->rMarginString != NULL)
		    || (tagPtr->spacing1String != NULL)
		    || (tagPtr->spacing2String != NULL)
		    || (tagPtr->spacing3String != NULL)
		    || (tagPtr->tabStringPtr != NULL)
		    || (tagPtr->tabStyle != TK_TEXT_TABSTYLE_NONE)
		    || (tagPtr->wrapMode != TEXT_WRAPMODE_NULL)) {
		tagPtr->affectsDisplay = 1;
		tagPtr->affectsDisplayGeometry = 1;
	    }
	    if ((tagPtr->border != NULL)
		    || (tagPtr->selBorder != NULL)
		    || (tagPtr->reliefString != NULL)
		    || (tagPtr->bgStipple != None)
		    || (tagPtr->fgColor != NULL)
		    || (tagPtr->selFgColor != NULL)
		    || (tagPtr->fgStipple != None)
		    || (tagPtr->overstrikeString != NULL)
                    || (tagPtr->overstrikeColor != NULL)
		    || (tagPtr->underlineString != NULL)
                    || (tagPtr->underlineColor != NULL)
                    || (tagPtr->lMarginColor != NULL)
                    || (tagPtr->rMarginColor != NULL)) {
		tagPtr->affectsDisplay = 1;
	    }
	    if (!newTag) {
		/*
		 * This line is not necessary if this is a new tag, since it
		 * can't possibly have been applied to anything yet.
		 */

		/*
		 * VMD: If this is the 'sel' tag, then we don't need to call
		 * this for all peers, unless we actually want to synchronize
		 * sel-style changes across the peers.
		 */

		TkTextRedrawTag(textPtr->sharedTextPtr, NULL,
			NULL, NULL, tagPtr, 1);
	    }
	    return result;
	}
	break;
    }
    case TAG_DELETE: {
	Tcl_HashEntry *hPtr;

	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName ?tagName ...?");
	    return TCL_ERROR;
	}
	for (i = 3; i < objc; i++) {
	    hPtr = Tcl_FindHashEntry(&textPtr->sharedTextPtr->tagTable,
		    Tcl_GetString(objv[i]));
	    if (hPtr == NULL) {
		/*
		 * Either this tag doesn't exist or it's the 'sel' tag (which
		 * is not in the hash table). Either way we don't want to
		 * delete it.
		 */

		continue;
	    }
	    tagPtr = Tcl_GetHashValue(hPtr);
	    if (tagPtr == textPtr->selTagPtr) {
		continue;
	    }
	    if (tagPtr->affectsDisplay) {
		TkTextRedrawTag(textPtr->sharedTextPtr, NULL,
			NULL, NULL, tagPtr, 1);
	    }
	    TkTextDeleteTag(textPtr, tagPtr);
	    Tcl_DeleteHashEntry(hPtr);
	}
	break;
    }
    case TAG_LOWER: {
	TkTextTag *tagPtr2;
	int prio;

	if ((objc != 4) && (objc != 5)) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName ?belowThis?");
	    return TCL_ERROR;
	}
	tagPtr = FindTag(interp, textPtr, objv[3]);
	if (tagPtr == NULL) {
	    return TCL_ERROR;
	}
	if (objc == 5) {
	    tagPtr2 = FindTag(interp, textPtr, objv[4]);
	    if (tagPtr2 == NULL) {
		return TCL_ERROR;
	    }
	    if (tagPtr->priority < tagPtr2->priority) {
		prio = tagPtr2->priority - 1;
	    } else {
		prio = tagPtr2->priority;
	    }
	} else {
	    prio = 0;
	}
	ChangeTagPriority(textPtr, tagPtr, prio);

	/*
	 * If this is the 'sel' tag, then we don't actually need to call this
	 * for all peers.
	 */

	TkTextRedrawTag(textPtr->sharedTextPtr, NULL, NULL, NULL, tagPtr, 1);
	break;
    }
    case TAG_NAMES: {
	TkTextTag **arrayPtr;
	int arraySize;
	Tcl_Obj *listObj;

	if ((objc != 3) && (objc != 4)) {
	    Tcl_WrongNumArgs(interp, 3, objv, "?index?");
	    return TCL_ERROR;
	}
	if (objc == 3) {
	    Tcl_HashSearch search;
	    Tcl_HashEntry *hPtr;

	    arrayPtr = ckalloc(textPtr->sharedTextPtr->numTags
		    * sizeof(TkTextTag *));
	    for (i=0, hPtr = Tcl_FirstHashEntry(
		    &textPtr->sharedTextPtr->tagTable, &search);
		    hPtr != NULL; i++, hPtr = Tcl_NextHashEntry(&search)) {
		arrayPtr[i] = Tcl_GetHashValue(hPtr);
	    }

	    /*
	     * The 'sel' tag is not in the hash table.
	     */

	    arrayPtr[i] = textPtr->selTagPtr;
	    arraySize = ++i;
	} else {
	    if (TkTextGetObjIndex(interp, textPtr, objv[3],
		    &index1) != TCL_OK) {
		return TCL_ERROR;
	    }
	    arrayPtr = TkBTreeGetTags(&index1, textPtr, &arraySize);
	    if (arrayPtr == NULL) {
		return TCL_OK;
	    }
	}

	SortTags(arraySize, arrayPtr);
	listObj = Tcl_NewListObj(0, NULL);

	for (i = 0; i < arraySize; i++) {
	    tagPtr = arrayPtr[i];
	    Tcl_ListObjAppendElement(interp, listObj,
		    Tcl_NewStringObj(tagPtr->name,-1));
	}
	Tcl_SetObjResult(interp, listObj);
	ckfree(arrayPtr);
	break;
    }
    case TAG_NEXTRANGE: {
	TkTextIndex last;
	TkTextSearch tSearch;
	char position[TK_POS_CHARS];
	Tcl_Obj *resultObj;

	if ((objc != 5) && (objc != 6)) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName index1 ?index2?");
	    return TCL_ERROR;
	}
	tagPtr = FindTag(NULL, textPtr, objv[3]);
	if (tagPtr == NULL) {
	    return TCL_OK;
	}
	if (TkTextGetObjIndex(interp, textPtr, objv[4], &index1) != TCL_OK) {
	    return TCL_ERROR;
	}
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
		TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr),
		0, &last);
	if (objc == 5) {
	    index2 = last;
	} else if (TkTextGetObjIndex(interp, textPtr, objv[5],
		&index2) != TCL_OK) {
	    return TCL_ERROR;
	}

	/*
	 * The search below is a bit tricky. Rather than use the B-tree
	 * facilities to stop the search at index2, let it search up until the
	 * end of the file but check for a position past index2 ourselves.
	 * The reason for doing it this way is that we only care whether the
	 * *start* of the range is before index2; once we find the start, we
	 * don't want TkBTreeNextTag to abort the search because the end of
	 * the range is after index2.
	 */

	TkBTreeStartSearch(&index1, &last, tagPtr, &tSearch);
	if (TkBTreeCharTagged(&index1, tagPtr)) {
	    TkTextSegment *segPtr;
	    int offset;

	    /*
	     * The first character is tagged. See if there is an on-toggle
	     * just before the character. If not, then skip to the end of this
	     * tagged range.
	     */

	    for (segPtr = index1.linePtr->segPtr, offset = index1.byteIndex;
		    offset >= 0;
		    offset -= segPtr->size, segPtr = segPtr->nextPtr) {
		if ((offset == 0) && (segPtr->typePtr == &tkTextToggleOnType)
			&& (segPtr->body.toggle.tagPtr == tagPtr)) {
		    goto gotStart;
		}
	    }
	    if (!TkBTreeNextTag(&tSearch)) {
		return TCL_OK;
	    }
	}

	/*
	 * Find the start of the tagged range.
	 */

	if (!TkBTreeNextTag(&tSearch)) {
	    return TCL_OK;
	}

    gotStart:
	if (TkTextIndexCmp(&tSearch.curIndex, &index2) >= 0) {
	    return TCL_OK;
	}
	resultObj = Tcl_NewObj();
	TkTextPrintIndex(textPtr, &tSearch.curIndex, position);
	Tcl_ListObjAppendElement(NULL, resultObj,
		Tcl_NewStringObj(position, -1));
	TkBTreeNextTag(&tSearch);
	TkTextPrintIndex(textPtr, &tSearch.curIndex, position);
	Tcl_ListObjAppendElement(NULL, resultObj,
		Tcl_NewStringObj(position, -1));
	Tcl_SetObjResult(interp, resultObj);
	break;
    }
    case TAG_PREVRANGE: {
	TkTextIndex last;
	TkTextSearch tSearch;
	char position1[TK_POS_CHARS];
	char position2[TK_POS_CHARS];
	Tcl_Obj *resultObj;

	if ((objc != 5) && (objc != 6)) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName index1 ?index2?");
	    return TCL_ERROR;
	}
	tagPtr = FindTag(NULL, textPtr, objv[3]);
	if (tagPtr == NULL) {
	    return TCL_OK;
	}
	if (TkTextGetObjIndex(interp, textPtr, objv[4], &index1) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (objc == 5) {
	    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, 0, 0,
		    &index2);
	} else if (TkTextGetObjIndex(interp, textPtr, objv[5],
		&index2) != TCL_OK) {
	    return TCL_ERROR;
	}

	/*
	 * The search below is a bit weird. The previous toggle can be either
	 * an on or off toggle. If it is an on toggle, then we need to turn
	 * around and search forward for the end toggle. Otherwise we keep
	 * searching backwards.
	 */

	TkBTreeStartSearchBack(&index1, &index2, tagPtr, &tSearch);

	if (!TkBTreePrevTag(&tSearch)) {
	    /*
	     * Special case, there may be a tag off toggle at index1, and a
	     * tag on toggle before the start of a partial peer widget. In
	     * this case we missed it.
	     */

	    if (textPtr->start != NULL && (textPtr->start == index2.linePtr)
		    && (index2.byteIndex == 0)
		    && TkBTreeCharTagged(&index2, tagPtr)
		    && (TkTextIndexCmp(&index2, &index1) < 0)) {
		/*
		 * The first character is tagged, so just add the range from
		 * the first char to the start of the range.
		 */

		TkTextPrintIndex(textPtr, &index2, position1);
		TkTextPrintIndex(textPtr, &index1, position2);
		goto gotPrevIndexPair;
	    }
	    return TCL_OK;
	}

	if (tSearch.segPtr->typePtr == &tkTextToggleOnType) {
	    TkTextPrintIndex(textPtr, &tSearch.curIndex, position1);
	    if (textPtr->start != NULL) {
		/*
		 * Make sure the first index is not before the first allowed
		 * text index in this widget.
		 */

		TkTextIndex firstIndex;

		firstIndex.linePtr = textPtr->start;
		firstIndex.byteIndex = 0;
		firstIndex.textPtr = NULL;
		if (TkTextIndexCmp(&tSearch.curIndex, &firstIndex) < 0) {
		    if (TkTextIndexCmp(&firstIndex, &index1) >= 0) {
			/*
			 * But now the new first index is actually too far
			 * along in the text, so nothing is returned.
			 */

			return TCL_OK;
		    }
		    TkTextPrintIndex(textPtr, &firstIndex, position1);
		}
	    }
	    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
		    TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr),
		    0, &last);
	    TkBTreeStartSearch(&tSearch.curIndex, &last, tagPtr, &tSearch);
	    TkBTreeNextTag(&tSearch);
	    TkTextPrintIndex(textPtr, &tSearch.curIndex, position2);
	} else {
	    TkTextPrintIndex(textPtr, &tSearch.curIndex, position2);
	    TkBTreePrevTag(&tSearch);
	    TkTextPrintIndex(textPtr, &tSearch.curIndex, position1);
	    if (TkTextIndexCmp(&tSearch.curIndex, &index2) < 0) {
		if (textPtr->start != NULL && index2.linePtr == textPtr->start
			&& index2.byteIndex == 0) {
		    /* It's ok */
		    TkTextPrintIndex(textPtr, &index2, position1);
		} else {
		    return TCL_OK;
		}
	    }
	}

    gotPrevIndexPair:
	resultObj = Tcl_NewObj();
	Tcl_ListObjAppendElement(NULL, resultObj,
		Tcl_NewStringObj(position1, -1));
	Tcl_ListObjAppendElement(NULL, resultObj,
		Tcl_NewStringObj(position2, -1));
	Tcl_SetObjResult(interp, resultObj);
	break;
    }
    case TAG_RAISE: {
	TkTextTag *tagPtr2;
	int prio;

	if ((objc != 4) && (objc != 5)) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName ?aboveThis?");
	    return TCL_ERROR;
	}
	tagPtr = FindTag(interp, textPtr, objv[3]);
	if (tagPtr == NULL) {
	    return TCL_ERROR;
	}
	if (objc == 5) {
	    tagPtr2 = FindTag(interp, textPtr, objv[4]);
	    if (tagPtr2 == NULL) {
		return TCL_ERROR;
	    }
	    if (tagPtr->priority <= tagPtr2->priority) {
		prio = tagPtr2->priority;
	    } else {
		prio = tagPtr2->priority + 1;
	    }
	} else {
	    prio = textPtr->sharedTextPtr->numTags-1;
	}
	ChangeTagPriority(textPtr, tagPtr, prio);

	/*
	 * If this is the 'sel' tag, then we don't actually need to call this
	 * for all peers.
	 */

	TkTextRedrawTag(textPtr->sharedTextPtr, NULL, NULL, NULL, tagPtr, 1);
	break;
    }
    case TAG_RANGES: {
	TkTextIndex first, last;
	TkTextSearch tSearch;
	Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
	int count = 0;

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "tagName");
	    return TCL_ERROR;
	}
	tagPtr = FindTag(NULL, textPtr, objv[3]);
	if (tagPtr == NULL) {
	    return TCL_OK;
	}
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, 0, 0,
		&first);
	TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
		TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr),
		0, &last);
	TkBTreeStartSearch(&first, &last, tagPtr, &tSearch);
	if (TkBTreeCharTagged(&first, tagPtr)) {
	    Tcl_ListObjAppendElement(NULL, listObj,
		    TkTextNewIndexObj(textPtr, &first));
	    count++;
	}
	while (TkBTreeNextTag(&tSearch)) {
	    Tcl_ListObjAppendElement(NULL, listObj,
		    TkTextNewIndexObj(textPtr, &tSearch.curIndex));
	    count++;
	}
	if (count % 2 == 1) {
	    /*
	     * If a text widget uses '-end', it won't necessarily run to the
	     * end of the B-tree, and therefore the tag range might not be
	     * closed. In this case we add the end of the range.
	     */

	    Tcl_ListObjAppendElement(NULL, listObj,
		    TkTextNewIndexObj(textPtr, &last));
	}
	Tcl_SetObjResult(interp, listObj);
	break;
    }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextCreateTag --
 *
 *	Find the record describing a tag within a given text widget, creating
 *	a new record if one doesn't already exist.
 *
 * Results:
 *	The return value is a pointer to the TkTextTag record for tagName.
 *
 * Side effects:
 *	A new tag record is created if there isn't one already defined for
 *	tagName.
 *
 *----------------------------------------------------------------------
 */

TkTextTag *
TkTextCreateTag(
    TkText *textPtr,		/* Widget in which tag is being used. */
    const char *tagName,	/* Name of desired tag. */
    int *newTag)		/* If non-NULL, then return 1 if new, or 0 if
				 * already exists. */
{
    register TkTextTag *tagPtr;
    Tcl_HashEntry *hPtr = NULL;
    int isNew;
    const char *name;

    if (!strcmp(tagName, "sel")) {
	if (textPtr->selTagPtr != NULL) {
	    if (newTag != NULL) {
		*newTag = 0;
	    }
	    return textPtr->selTagPtr;
	}
	if (newTag != NULL) {
	    *newTag = 1;
	}
	name = "sel";
    } else {
	hPtr = Tcl_CreateHashEntry(&textPtr->sharedTextPtr->tagTable,
		tagName, &isNew);
	if (newTag != NULL) {
	    *newTag = isNew;
	}
	if (!isNew) {
	    return Tcl_GetHashValue(hPtr);
	}
	name = Tcl_GetHashKey(&textPtr->sharedTextPtr->tagTable, hPtr);
    }

    /*
     * No existing entry. Create a new one, initialize it, and add a pointer
     * to it to the hash table entry.
     */

    tagPtr = ckalloc(sizeof(TkTextTag));
    tagPtr->name = name;
    tagPtr->textPtr = NULL;
    tagPtr->toggleCount = 0;
    tagPtr->tagRootPtr = NULL;
    tagPtr->priority = textPtr->sharedTextPtr->numTags;
    tagPtr->border = NULL;
    tagPtr->borderWidth = 0;
    tagPtr->borderWidthPtr = NULL;
    tagPtr->reliefString = NULL;
    tagPtr->relief = TK_RELIEF_FLAT;
    tagPtr->bgStipple = None;
    tagPtr->fgColor = NULL;
    tagPtr->tkfont = NULL;
    tagPtr->fgStipple = None;
    tagPtr->justifyString = NULL;
    tagPtr->justify = TK_JUSTIFY_LEFT;
    tagPtr->lMargin1String = NULL;
    tagPtr->lMargin1 = 0;
    tagPtr->lMargin2String = NULL;
    tagPtr->lMargin2 = 0;
    tagPtr->lMarginColor = NULL;
    tagPtr->offsetString = NULL;
    tagPtr->offset = 0;
    tagPtr->overstrikeString = NULL;
    tagPtr->overstrike = 0;
    tagPtr->overstrikeColor = NULL;
    tagPtr->rMarginString = NULL;
    tagPtr->rMargin = 0;
    tagPtr->rMarginColor = NULL;
    tagPtr->selBorder = NULL;
    tagPtr->selFgColor = NULL;
    tagPtr->spacing1String = NULL;
    tagPtr->spacing1 = 0;
    tagPtr->spacing2String = NULL;
    tagPtr->spacing2 = 0;
    tagPtr->spacing3String = NULL;
    tagPtr->spacing3 = 0;
    tagPtr->tabStringPtr = NULL;
    tagPtr->tabArrayPtr = NULL;
    tagPtr->tabStyle = TK_TEXT_TABSTYLE_NONE;
    tagPtr->underlineString = NULL;
    tagPtr->underline = 0;
    tagPtr->underlineColor = NULL;
    tagPtr->elideString = NULL;
    tagPtr->elide = 0;
    tagPtr->wrapMode = TEXT_WRAPMODE_NULL;
    tagPtr->affectsDisplay = 0;
    tagPtr->affectsDisplayGeometry = 0;
    textPtr->sharedTextPtr->numTags++;
    if (!strcmp(tagName, "sel")) {
	tagPtr->textPtr = textPtr;
	textPtr->refCount++;
    } else {
	CLANG_ASSERT(hPtr);
	Tcl_SetHashValue(hPtr, tagPtr);
    }
    tagPtr->optionTable =
	    Tk_CreateOptionTable(textPtr->interp, tagOptionSpecs);
    return tagPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FindTag --
 *
 *	See if tag is defined for a given widget.
 *
 * Results:
 *	If tagName is defined in textPtr, a pointer to its TkTextTag structure
 *	is returned. Otherwise NULL is returned and an error message is
 *	recorded in the interp's result unless interp is NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static TkTextTag *
FindTag(
    Tcl_Interp *interp,		/* Interpreter to use for error message; if
				 * NULL, then don't record an error
				 * message. */
    TkText *textPtr,		/* Widget in which tag is being used. */
    Tcl_Obj *tagName)		/* Name of desired tag. */
{
    Tcl_HashEntry *hPtr;
    int len;
    const char *str;

    str = Tcl_GetStringFromObj(tagName, &len);
    if (len == 3 && !strcmp(str, "sel")) {
	return textPtr->selTagPtr;
    }
    hPtr = Tcl_FindHashEntry(&textPtr->sharedTextPtr->tagTable,
	    Tcl_GetString(tagName));
    if (hPtr != NULL) {
	return Tcl_GetHashValue(hPtr);
    }
    if (interp != NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"tag \"%s\" isn't defined in text widget",
		Tcl_GetString(tagName)));
	Tcl_SetErrorCode(interp, "TK", "LOOKUP", "TEXT_TAG",
		Tcl_GetString(tagName), NULL);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextDeleteTag --
 *
 *	This function is called to carry out most actions associated with the
 *	'tag delete' sub-command. It will remove all evidence of the tag from
 *	the B-tree, and then call TkTextFreeTag to clean up the tag structure
 *	itself.
 *
 *	The only actions this doesn't carry out it to check if the deletion of
 *	the tag requires something to be re-displayed, and to remove the tag
 *	from the tagTable (hash table) if that is necessary (i.e. if it's not
 *	the 'sel' tag). It is expected that the caller carry out both of these
 *	actions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory and other resources are freed, the B-tree is manipulated.
 *
 *----------------------------------------------------------------------
 */

void
TkTextDeleteTag(
    TkText *textPtr,		/* Info about overall widget. */
    register TkTextTag *tagPtr)	/* Tag being deleted. */
{
    TkTextIndex first, last;

    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr, 0, 0, &first);
    TkTextMakeByteIndex(textPtr->sharedTextPtr->tree, textPtr,
	    TkBTreeNumLines(textPtr->sharedTextPtr->tree, textPtr), 0, &last),
    TkBTreeTag(&first, &last, tagPtr, 0);

    if (tagPtr == textPtr->selTagPtr) {
	/*
	 * Send an event that the selection changed. This is equivalent to:
	 *	event generate $textWidget <<Selection>>
	 */

	TkTextSelectionEvent(textPtr);
    } else {
	/*
	 * Since all peer widgets have an independent "sel" tag, we
	 * don't want removal of one sel tag to remove bindings which
	 * are still valid in other peer widgets.
	 */

	if (textPtr->sharedTextPtr->bindingTable != NULL) {
	    Tk_DeleteAllBindings(textPtr->sharedTextPtr->bindingTable,
		    (ClientData) tagPtr->name);
	}
    }

    /*
     * Update the tag priorities to reflect the deletion of this tag.
     */

    ChangeTagPriority(textPtr, tagPtr, textPtr->sharedTextPtr->numTags-1);
    textPtr->sharedTextPtr->numTags -= 1;
    TkTextFreeTag(textPtr, tagPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkTextFreeTag --
 *
 *	This function is called when a tag is deleted to free up the memory
 *	and other resources associated with the tag.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory and other resources are freed.
 *
 *----------------------------------------------------------------------
 */

void
TkTextFreeTag(
    TkText *textPtr,		/* Info about overall widget. */
    register TkTextTag *tagPtr)	/* Tag being deleted. */
{
    int i;

    /*
     * Let Tk do most of the hard work for us.
     */

    Tk_FreeConfigOptions((char *) tagPtr, tagPtr->optionTable,
	    textPtr->tkwin);

    /*
     * This associated information is managed by us.
     */

    if (tagPtr->tabArrayPtr != NULL) {
	ckfree(tagPtr->tabArrayPtr);
    }

    /*
     * Make sure this tag isn't referenced from the 'current' tag array.
     */

    for (i = 0; i < textPtr->numCurTags; i++) {
	if (textPtr->curTagArrayPtr[i] == tagPtr) {
	    for (; i < textPtr->numCurTags-1; i++) {
		textPtr->curTagArrayPtr[i] = textPtr->curTagArrayPtr[i+1];
	    }
	    textPtr->curTagArrayPtr[textPtr->numCurTags-1] = NULL;
	    textPtr->numCurTags--;
	    break;
	}
    }

    /*
     * If this tag is widget-specific (peer widgets) then clean up the
     * refCount it holds.
     */

    if (tagPtr->textPtr != NULL) {
	if (textPtr != tagPtr->textPtr) {
	    Tcl_Panic("Tag being deleted from wrong widget");
	}
	if (textPtr->refCount-- <= 1) {
	    ckfree(textPtr);
	}
	tagPtr->textPtr = NULL;
    }

    /*
     * Finally free the tag's memory.
     */

    ckfree(tagPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * SortTags --
 *
 *	This function sorts an array of tag pointers in increasing order of
 *	priority, optimizing for the common case where the array is small.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
SortTags(
    int numTags,		/* Number of tag pointers at *tagArrayPtr. */
    TkTextTag **tagArrayPtr)	/* Pointer to array of pointers. */
{
    int i, j, prio;
    register TkTextTag **tagPtrPtr;
    TkTextTag **maxPtrPtr, *tmp;

    if (numTags < 2) {
	return;
    }
    if (numTags < 20) {
	for (i = numTags-1; i > 0; i--, tagArrayPtr++) {
	    maxPtrPtr = tagPtrPtr = tagArrayPtr;
	    prio = tagPtrPtr[0]->priority;
	    for (j = i, tagPtrPtr++; j > 0; j--, tagPtrPtr++) {
		if (tagPtrPtr[0]->priority < prio) {
		    prio = tagPtrPtr[0]->priority;
		    maxPtrPtr = tagPtrPtr;
		}
	    }
	    tmp = *maxPtrPtr;
	    *maxPtrPtr = *tagArrayPtr;
	    *tagArrayPtr = tmp;
	}
    } else {
	qsort(tagArrayPtr,(unsigned)numTags,sizeof(TkTextTag *),TagSortProc);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TagSortProc --
 *
 *	This function is called by qsort() when sorting an array of tags in
 *	priority order.
 *
 * Results:
 *	The return value is -1 if the first argument should be before the
 *	second element (i.e. it has lower priority), 0 if it's equivalent
 *	(this should never happen!), and 1 if it should be after the second
 *	element.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TagSortProc(
    const void *first,
    const void *second)		/* Elements to be compared. */
{
    TkTextTag *tagPtr1, *tagPtr2;

    tagPtr1 = * (TkTextTag **) first;
    tagPtr2 = * (TkTextTag **) second;
    return tagPtr1->priority - tagPtr2->priority;
}

/*
 *----------------------------------------------------------------------
 *
 * ChangeTagPriority --
 *
 *	This function changes the priority of a tag by modifying its priority
 *	and the priorities of other tags that are affected by the change.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Priorities may be changed for some or all of the tags in textPtr. The
 *	tags will be arranged so that there is exactly one tag at each
 *	priority level between 0 and textPtr->sharedTextPtr->numTags-1, with
 *	tagPtr at priority "prio".
 *
 *----------------------------------------------------------------------
 */

static void
ChangeTagPriority(
    TkText *textPtr,		/* Information about text widget. */
    TkTextTag *tagPtr,		/* Tag whose priority is to be changed. */
    int prio)			/* New priority for tag. */
{
    int low, high, delta;
    register TkTextTag *tagPtr2;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    if (prio < 0) {
	prio = 0;
    }
    if (prio >= textPtr->sharedTextPtr->numTags) {
	prio = textPtr->sharedTextPtr->numTags-1;
    }
    if (prio == tagPtr->priority) {
	return;
    }
    if (prio < tagPtr->priority) {
	low = prio;
	high = tagPtr->priority-1;
	delta = 1;
    } else {
	low = tagPtr->priority+1;
	high = prio;
	delta = -1;
    }

    /*
     * Adjust first the 'sel' tag, then all others from the hash table
     */

    if ((textPtr->selTagPtr->priority >= low)
	    && (textPtr->selTagPtr->priority <= high)) {
	textPtr->selTagPtr->priority += delta;
    }
    for (hPtr = Tcl_FirstHashEntry(&textPtr->sharedTextPtr->tagTable, &search);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	tagPtr2 = Tcl_GetHashValue(hPtr);
	if ((tagPtr2->priority >= low) && (tagPtr2->priority <= high)) {
	    tagPtr2->priority += delta;
	}
    }
    tagPtr->priority = prio;
}

/*
 *--------------------------------------------------------------
 *
 * TkTextBindProc --
 *
 *	This function is invoked by the Tk dispatcher to handle events
 *	associated with bindings on items.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the command invoked as part of the binding (if there was
 *	any).
 *
 *--------------------------------------------------------------
 */

void
TkTextBindProc(
    ClientData clientData,	/* Pointer to canvas structure. */
    XEvent *eventPtr)		/* Pointer to X event that just happened. */
{
    TkText *textPtr = clientData;
    int repick = 0;

    textPtr->refCount++;

    /*
     * This code simulates grabs for mouse buttons by keeping track of whether
     * a button is pressed and refusing to pick a new current character while
     * a button is pressed.
     */

    if (eventPtr->type == ButtonPress) {
	textPtr->flags |= BUTTON_DOWN;
    } else if (eventPtr->type == ButtonRelease) {
	unsigned int mask;

	mask = TkGetButtonMask(eventPtr->xbutton.button);
	if ((eventPtr->xbutton.state & ALL_BUTTONS) == mask) {
	    textPtr->flags &= ~BUTTON_DOWN;
	    repick = 1;
	}
    } else if ((eventPtr->type == EnterNotify)
	    || (eventPtr->type == LeaveNotify)) {
	if (eventPtr->xcrossing.state & ALL_BUTTONS) {
	    textPtr->flags |= BUTTON_DOWN;
	} else {
	    textPtr->flags &= ~BUTTON_DOWN;
	}
	TkTextPickCurrent(textPtr, eventPtr);
	goto done;
    } else if (eventPtr->type == MotionNotify) {
	if (eventPtr->xmotion.state & ALL_BUTTONS) {
	    textPtr->flags |= BUTTON_DOWN;
	} else {
	    textPtr->flags &= ~BUTTON_DOWN;
	}
	TkTextPickCurrent(textPtr, eventPtr);
    }
    if ((textPtr->numCurTags > 0)
	    && (textPtr->sharedTextPtr->bindingTable != NULL)
	    && (textPtr->tkwin != NULL) && !(textPtr->flags & DESTROYED)) {
	TagBindEvent(textPtr, eventPtr, textPtr->numCurTags,
		textPtr->curTagArrayPtr);
    }
    if (repick) {
	unsigned int oldState;

	oldState = eventPtr->xbutton.state;
	eventPtr->xbutton.state &= ~(unsigned long)ALL_BUTTONS;
	if (!(textPtr->flags & DESTROYED)) {
	    TkTextPickCurrent(textPtr, eventPtr);
	}
	eventPtr->xbutton.state = oldState;
    }

  done:
    if (textPtr->refCount-- <= 1) {
	ckfree(textPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkTextPickCurrent --
 *
 *	Find the character containing the coordinates in an event and place
 *	the "current" mark on that character. If the "current" mark has moved
 *	then generate a fake leave event on the old current character and a
 *	fake enter event on the new current character.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current mark for textPtr may change. If it does, then the commands
 *	associated with character entry and leave could do just about
 *	anything. For example, the text widget might be deleted. It is up to
 *	the caller to protect itself by incrementing the refCount of the text
 *	widget.
 *
 *--------------------------------------------------------------
 */

void
TkTextPickCurrent(
    register TkText *textPtr,	/* Text widget in which to select current
				 * character. */
    XEvent *eventPtr)		/* Event describing location of mouse cursor.
				 * Must be EnterWindow, LeaveWindow,
				 * ButtonRelease, or MotionNotify. */
{
    TkTextIndex index;
    TkTextTag **oldArrayPtr, **newArrayPtr;
    TkTextTag **copyArrayPtr = NULL;
				/* Initialization needed to prevent compiler
				 * warning. */
    int numOldTags, numNewTags, i, j, size, nearby;
    XEvent event;

    /*
     * If a button is down, then don't do anything at all; we'll be called
     * again when all buttons are up, and we can repick then. This implements
     * a form of mouse grabbing.
     */

    if (textPtr->flags & BUTTON_DOWN) {
	if (((eventPtr->type == EnterNotify)
		|| (eventPtr->type == LeaveNotify))
		&& ((eventPtr->xcrossing.mode == NotifyGrab)
		|| (eventPtr->xcrossing.mode == NotifyUngrab))) {
	    /*
	     * Special case: the window is being entered or left because of a
	     * grab or ungrab. In this case, repick after all. Furthermore,
	     * clear BUTTON_DOWN to release the simulated grab.
	     */

	    textPtr->flags &= ~BUTTON_DOWN;
	} else {
	    return;
	}
    }

    /*
     * Save information about this event in the widget in case we have to
     * synthesize more enter and leave events later (e.g. because a character
     * was deleted, causing a new character to be underneath the mouse
     * cursor). Also translate MotionNotify events into EnterNotify events,
     * since that's what gets reported to event handlers when the current
     * character changes.
     */

    if (eventPtr != &textPtr->pickEvent) {
	if ((eventPtr->type == MotionNotify)
		|| (eventPtr->type == ButtonRelease)) {
	    textPtr->pickEvent.xcrossing.type = EnterNotify;
	    textPtr->pickEvent.xcrossing.serial = eventPtr->xmotion.serial;
	    textPtr->pickEvent.xcrossing.send_event
		    = eventPtr->xmotion.send_event;
	    textPtr->pickEvent.xcrossing.display = eventPtr->xmotion.display;
	    textPtr->pickEvent.xcrossing.window = eventPtr->xmotion.window;
	    textPtr->pickEvent.xcrossing.root = eventPtr->xmotion.root;
	    textPtr->pickEvent.xcrossing.subwindow = None;
	    textPtr->pickEvent.xcrossing.time = eventPtr->xmotion.time;
	    textPtr->pickEvent.xcrossing.x = eventPtr->xmotion.x;
	    textPtr->pickEvent.xcrossing.y = eventPtr->xmotion.y;
	    textPtr->pickEvent.xcrossing.x_root = eventPtr->xmotion.x_root;
	    textPtr->pickEvent.xcrossing.y_root = eventPtr->xmotion.y_root;
	    textPtr->pickEvent.xcrossing.mode = NotifyNormal;
	    textPtr->pickEvent.xcrossing.detail = NotifyNonlinear;
	    textPtr->pickEvent.xcrossing.same_screen
		    = eventPtr->xmotion.same_screen;
	    textPtr->pickEvent.xcrossing.focus = False;
	    textPtr->pickEvent.xcrossing.state = eventPtr->xmotion.state;
	} else {
	    textPtr->pickEvent = *eventPtr;
	}
    }

    /*
     * Find the new current character, then find and sort all of the tags
     * associated with it.
     */

    if (textPtr->pickEvent.type != LeaveNotify) {
	TkTextPixelIndex(textPtr, textPtr->pickEvent.xcrossing.x,
		textPtr->pickEvent.xcrossing.y, &index, &nearby);
	if (nearby) {
	    newArrayPtr = NULL;
	    numNewTags = 0;
	} else {
	    newArrayPtr = TkBTreeGetTags(&index, textPtr, &numNewTags);
	    SortTags(numNewTags, newArrayPtr);
	}
    } else {
	newArrayPtr = NULL;
	numNewTags = 0;
    }

    /*
     * Resort the tags associated with the previous marked character (the
     * priorities might have changed), then make a copy of the new tags, and
     * compare the old tags to the copy, nullifying any tags that are present
     * in both groups (i.e. the tags that haven't changed).
     */

    SortTags(textPtr->numCurTags, textPtr->curTagArrayPtr);
    if (numNewTags > 0) {
	size = numNewTags * sizeof(TkTextTag *);
	copyArrayPtr = ckalloc(size);
	memcpy(copyArrayPtr, newArrayPtr, (size_t) size);
	for (i = 0; i < textPtr->numCurTags; i++) {
	    for (j = 0; j < numNewTags; j++) {
		if (textPtr->curTagArrayPtr[i] == copyArrayPtr[j]) {
		    textPtr->curTagArrayPtr[i] = NULL;
		    copyArrayPtr[j] = NULL;
		    break;
		}
	    }
	}
    }

    /*
     * Invoke the binding system with a LeaveNotify event for all of the tags
     * that have gone away. We have to be careful here, because it's possible
     * that the binding could do something (like calling tkwait) that
     * eventually modifies textPtr->curTagArrayPtr. To avoid problems in
     * situations like this, update curTagArrayPtr to its new value before
     * invoking any bindings, and don't use it any more here.
     */

    numOldTags = textPtr->numCurTags;
    textPtr->numCurTags = numNewTags;
    oldArrayPtr = textPtr->curTagArrayPtr;
    textPtr->curTagArrayPtr = newArrayPtr;
    if (numOldTags != 0) {
	if ((textPtr->sharedTextPtr->bindingTable != NULL)
		&& (textPtr->tkwin != NULL)
		&& !(textPtr->flags & DESTROYED)) {
	    event = textPtr->pickEvent;
	    event.type = LeaveNotify;

	    /*
	     * Always use a detail of NotifyAncestor. Besides being
	     * consistent, this avoids problems where the binding code will
	     * discard NotifyInferior events.
	     */

	    event.xcrossing.detail = NotifyAncestor;
	    TagBindEvent(textPtr, &event, numOldTags, oldArrayPtr);
	}
	ckfree(oldArrayPtr);
    }

    /*
     * Reset the "current" mark (be careful to recompute its location, since
     * it might have changed during an event binding). Then invoke the binding
     * system with an EnterNotify event for all of the tags that have just
     * appeared.
     */

    TkTextPixelIndex(textPtr, textPtr->pickEvent.xcrossing.x,
	    textPtr->pickEvent.xcrossing.y, &index, &nearby);
    TkTextSetMark(textPtr, "current", &index);
    if (numNewTags != 0) {
	if ((textPtr->sharedTextPtr->bindingTable != NULL)
		&& (textPtr->tkwin != NULL)
		&& !(textPtr->flags & DESTROYED) && !nearby) {
	    event = textPtr->pickEvent;
	    event.type = EnterNotify;
	    event.xcrossing.detail = NotifyAncestor;
	    TagBindEvent(textPtr, &event, numNewTags, copyArrayPtr);
	}
	ckfree(copyArrayPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TagBindEvent --
 *
 *	Trigger given events for all tags that match the relevant bindings.
 *	To handle the "sel" tag correctly in all peer widgets, we must use the
 *	name of the tags as the binding table element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Almost anything can be triggered by tag bindings, including deletion
 *	of the text widget.
 *
 *--------------------------------------------------------------
 */

static void
TagBindEvent(
    TkText *textPtr,		/* Text widget to fire bindings in. */
    XEvent *eventPtr,		/* What actually happened. */
    int numTags,		/* Number of relevant tags. */
    TkTextTag **tagArrayPtr)	/* Array of relevant tags. */
{
#   define NUM_BIND_TAGS 10
    const char *nameArray[NUM_BIND_TAGS];
    const char **nameArrPtr;
    int i;

    /*
     * Try to avoid allocation unless there are lots of tags.
     */

    if (numTags > NUM_BIND_TAGS) {
	nameArrPtr = ckalloc(numTags * sizeof(const char *));
    } else {
	nameArrPtr = nameArray;
    }

    /*
     * We use tag names as keys in the hash table. We do this instead of using
     * the actual tagPtr objects because we want one "sel" tag binding for all
     * peer widgets, despite the fact that each has its own tagPtr object.
     */

    for (i = 0; i < numTags; i++) {
	TkTextTag *tagPtr = tagArrayPtr[i];

	if (tagPtr != NULL) {
	    nameArrPtr[i] = tagPtr->name;
	} else {
	    /*
	     * Tag has been deleted elsewhere, and therefore nulled out in
	     * this array. Tk_BindEvent is clever enough to cope with NULLs
	     * being thrown at it.
	     */

	    nameArrPtr[i] = NULL;
	}
    }
    Tk_BindEvent(textPtr->sharedTextPtr->bindingTable, eventPtr,
	    textPtr->tkwin, numTags, (ClientData *) nameArrPtr);

    if (numTags > NUM_BIND_TAGS) {
	ckfree(nameArrPtr);
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
