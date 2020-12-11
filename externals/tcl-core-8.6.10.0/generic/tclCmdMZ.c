/*
 * tclCmdMZ.c --
 *
 *	This file contains the top-level command routines for most of the Tcl
 *	built-in commands whose names begin with the letters M to Z. It
 *	contains only commands in the generic core (i.e. those that don't
 *	depend much upon UNIX facilities).
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 Scriptics Corporation.
 * Copyright (c) 2002 ActiveState Corporation.
 * Copyright (c) 2003-2009 Donal K. Fellows.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclCompile.h"
#include "tclRegexp.h"
#include "tclStringTrim.h"

static inline Tcl_Obj *	During(Tcl_Interp *interp, int resultCode,
			    Tcl_Obj *oldOptions, Tcl_Obj *errorInfo);
static Tcl_NRPostProc	SwitchPostProc;
static Tcl_NRPostProc	TryPostBody;
static Tcl_NRPostProc	TryPostFinal;
static Tcl_NRPostProc	TryPostHandler;
static int		UniCharIsAscii(int character);
static int		UniCharIsHexDigit(int character);

/*
 * Default set of characters to trim in [string trim] and friends. This is a
 * UTF-8 literal string containing all Unicode space characters [TIP #413]
 */

const char tclDefaultTrimSet[] =
	"\x09\x0a\x0b\x0c\x0d " /* ASCII */
	"\xc0\x80" /*     nul (U+0000) */
	"\xc2\x85" /*     next line (U+0085) */
	"\xc2\xa0" /*     non-breaking space (U+00a0) */
	"\xe1\x9a\x80" /* ogham space mark (U+1680) */
	"\xe1\xa0\x8e" /* mongolian vowel separator (U+180e) */
	"\xe2\x80\x80" /* en quad (U+2000) */
	"\xe2\x80\x81" /* em quad (U+2001) */
	"\xe2\x80\x82" /* en space (U+2002) */
	"\xe2\x80\x83" /* em space (U+2003) */
	"\xe2\x80\x84" /* three-per-em space (U+2004) */
	"\xe2\x80\x85" /* four-per-em space (U+2005) */
	"\xe2\x80\x86" /* six-per-em space (U+2006) */
	"\xe2\x80\x87" /* figure space (U+2007) */
	"\xe2\x80\x88" /* punctuation space (U+2008) */
	"\xe2\x80\x89" /* thin space (U+2009) */
	"\xe2\x80\x8a" /* hair space (U+200a) */
	"\xe2\x80\x8b" /* zero width space (U+200b) */
	"\xe2\x80\xa8" /* line separator (U+2028) */
	"\xe2\x80\xa9" /* paragraph separator (U+2029) */
	"\xe2\x80\xaf" /* narrow no-break space (U+202f) */
	"\xe2\x81\x9f" /* medium mathematical space (U+205f) */
	"\xe2\x81\xa0" /* word joiner (U+2060) */
	"\xe3\x80\x80" /* ideographic space (U+3000) */
	"\xef\xbb\xbf" /* zero width no-break space (U+feff) */
;

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PwdObjCmd --
 *
 *	This procedure is invoked to process the "pwd" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_PwdObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Obj *retVal;

    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
	return TCL_ERROR;
    }

    retVal = Tcl_FSGetCwd(interp);
    if (retVal == NULL) {
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, retVal);
    Tcl_DecrRefCount(retVal);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegexpObjCmd --
 *
 *	This procedure is invoked to process the "regexp" Tcl command. See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RegexpObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int i, indices, match, about, offset, all, doinline, numMatchesSaved;
    int cflags, eflags, stringLength, matchLength;
    Tcl_RegExp regExpr;
    Tcl_Obj *objPtr, *startIndex = NULL, *resultPtr = NULL;
    Tcl_RegExpInfo info;
    static const char *const options[] = {
	"-all",		"-about",	"-indices",	"-inline",
	"-expanded",	"-line",	"-linestop",	"-lineanchor",
	"-nocase",	"-start",	"--",		NULL
    };
    enum options {
	REGEXP_ALL,	REGEXP_ABOUT,	REGEXP_INDICES,	REGEXP_INLINE,
	REGEXP_EXPANDED,REGEXP_LINE,	REGEXP_LINESTOP,REGEXP_LINEANCHOR,
	REGEXP_NOCASE,	REGEXP_START,	REGEXP_LAST
    };

    indices = 0;
    about = 0;
    cflags = TCL_REG_ADVANCED;
    offset = 0;
    all = 0;
    doinline = 0;

    for (i = 1; i < objc; i++) {
	const char *name;
	int index;

	name = TclGetString(objv[i]);
	if (name[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", TCL_EXACT,
		&index) != TCL_OK) {
	    goto optionError;
	}
	switch ((enum options) index) {
	case REGEXP_ALL:
	    all = 1;
	    break;
	case REGEXP_INDICES:
	    indices = 1;
	    break;
	case REGEXP_INLINE:
	    doinline = 1;
	    break;
	case REGEXP_NOCASE:
	    cflags |= TCL_REG_NOCASE;
	    break;
	case REGEXP_ABOUT:
	    about = 1;
	    break;
	case REGEXP_EXPANDED:
	    cflags |= TCL_REG_EXPANDED;
	    break;
	case REGEXP_LINE:
	    cflags |= TCL_REG_NEWLINE;
	    break;
	case REGEXP_LINESTOP:
	    cflags |= TCL_REG_NLSTOP;
	    break;
	case REGEXP_LINEANCHOR:
	    cflags |= TCL_REG_NLANCH;
	    break;
	case REGEXP_START: {
	    int temp;
	    if (++i >= objc) {
		goto endOfForLoop;
	    }
	    if (TclGetIntForIndexM(interp, objv[i], 0, &temp) != TCL_OK) {
		goto optionError;
	    }
	    if (startIndex) {
		Tcl_DecrRefCount(startIndex);
	    }
	    startIndex = objv[i];
	    Tcl_IncrRefCount(startIndex);
	    break;
	}
	case REGEXP_LAST:
	    i++;
	    goto endOfForLoop;
	}
    }

  endOfForLoop:
    if ((objc - i) < (2 - about)) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-option ...? exp string ?matchVar? ?subMatchVar ...?");
	goto optionError;
    }
    objc -= i;
    objv += i;

    /*
     * Check if the user requested -inline, but specified match variables; a
     * no-no.
     */

    if (doinline && ((objc - 2) != 0)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"regexp match variables not allowed when using -inline", -1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "REGEXP",
		"MIX_VAR_INLINE", NULL);
	goto optionError;
    }

    /*
     * Handle the odd about case separately.
     */

    if (about) {
	regExpr = Tcl_GetRegExpFromObj(interp, objv[0], cflags);
	if ((regExpr == NULL) || (TclRegAbout(interp, regExpr) < 0)) {
	optionError:
	    if (startIndex) {
		Tcl_DecrRefCount(startIndex);
	    }
	    return TCL_ERROR;
	}
	return TCL_OK;
    }

    /*
     * Get the length of the string that we are matching against so we can do
     * the termination test for -all matches. Do this before getting the
     * regexp to avoid shimmering problems.
     */

    objPtr = objv[1];
    stringLength = Tcl_GetCharLength(objPtr);

    if (startIndex) {
	TclGetIntForIndexM(NULL, startIndex, stringLength, &offset);
	Tcl_DecrRefCount(startIndex);
	if (offset < 0) {
	    offset = 0;
	}
    }

    regExpr = Tcl_GetRegExpFromObj(interp, objv[0], cflags);
    if (regExpr == NULL) {
	return TCL_ERROR;
    }

    objc -= 2;
    objv += 2;

    if (doinline) {
	/*
	 * Save all the subexpressions, as we will return them as a list
	 */

	numMatchesSaved = -1;
    } else {
	/*
	 * Save only enough subexpressions for matches we want to keep, expect
	 * in the case of -all, where we need to keep at least one to know
	 * where to move the offset.
	 */

	numMatchesSaved = (objc == 0) ? all : objc;
    }

    /*
     * The following loop is to handle multiple matches within the same source
     * string; each iteration handles one match. If "-all" hasn't been
     * specified then the loop body only gets executed once. We terminate the
     * loop when the starting offset is past the end of the string.
     */

    while (1) {
	/*
	 * Pass either 0 or TCL_REG_NOTBOL in the eflags. Passing
	 * TCL_REG_NOTBOL indicates that the character at offset should not be
	 * considered the start of the line. If for example the pattern {^} is
	 * passed and -start is positive, then the pattern will not match the
	 * start of the string unless the previous character is a newline.
	 */

	if (offset == 0) {
	    eflags = 0;
	} else if (offset > stringLength) {
	    eflags = TCL_REG_NOTBOL;
	} else if (Tcl_GetUniChar(objPtr, offset-1) == '\n') {
	    eflags = 0;
	} else {
	    eflags = TCL_REG_NOTBOL;
	}

	match = Tcl_RegExpExecObj(interp, regExpr, objPtr, offset,
		numMatchesSaved, eflags);
	if (match < 0) {
	    return TCL_ERROR;
	}

	if (match == 0) {
	    /*
	     * We want to set the value of the intepreter result only when
	     * this is the first time through the loop.
	     */

	    if (all <= 1) {
		/*
		 * If inlining, the interpreter's object result remains an
		 * empty list, otherwise set it to an integer object w/ value
		 * 0.
		 */

		if (!doinline) {
		    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
		}
		return TCL_OK;
	    }
	    break;
	}

	/*
	 * If additional variable names have been specified, return index
	 * information in those variables.
	 */

	Tcl_RegExpGetInfo(regExpr, &info);
	if (doinline) {
	    /*
	     * It's the number of substitutions, plus one for the matchVar at
	     * index 0
	     */

	    objc = info.nsubs + 1;
	    if (all <= 1) {
		resultPtr = Tcl_NewObj();
	    }
	}
	for (i = 0; i < objc; i++) {
	    Tcl_Obj *newPtr;

	    if (indices) {
		int start, end;
		Tcl_Obj *objs[2];

		/*
		 * Only adjust the match area if there was a match for that
		 * area. (Scriptics Bug 4391/SF Bug #219232)
		 */

		if (i <= info.nsubs && info.matches[i].start >= 0) {
		    start = offset + info.matches[i].start;
		    end = offset + info.matches[i].end;

		    /*
		     * Adjust index so it refers to the last character in the
		     * match instead of the first character after the match.
		     */

		    if (end >= offset) {
			end--;
		    }
		} else {
		    start = -1;
		    end = -1;
		}

		objs[0] = Tcl_NewLongObj(start);
		objs[1] = Tcl_NewLongObj(end);

		newPtr = Tcl_NewListObj(2, objs);
	    } else {
		if (i <= info.nsubs) {
		    newPtr = Tcl_GetRange(objPtr,
			    offset + info.matches[i].start,
			    offset + info.matches[i].end - 1);
		} else {
		    newPtr = Tcl_NewObj();
		}
	    }
	    if (doinline) {
		if (Tcl_ListObjAppendElement(interp, resultPtr, newPtr)
			!= TCL_OK) {
		    Tcl_DecrRefCount(newPtr);
		    Tcl_DecrRefCount(resultPtr);
		    return TCL_ERROR;
		}
	    } else {
		if (Tcl_ObjSetVar2(interp, objv[i], NULL, newPtr,
			TCL_LEAVE_ERR_MSG) == NULL) {
		    return TCL_ERROR;
		}
	    }
	}

	if (all == 0) {
	    break;
	}

	/*
	 * Adjust the offset to the character just after the last one in the
	 * matchVar and increment all to count how many times we are making a
	 * match. We always increment the offset by at least one to prevent
	 * endless looping (as in the case: regexp -all {a*} a). Otherwise,
	 * when we match the NULL string at the end of the input string, we
	 * will loop indefinately (because the length of the match is 0, so
	 * offset never changes).
	 */

	matchLength = (info.matches[0].end - info.matches[0].start);

	offset += info.matches[0].end;

	/*
	 * A match of length zero could happen for {^} {$} or {.*} and in
	 * these cases we always want to bump the index up one.
	 */

	if (matchLength == 0) {
	    offset++;
	}
	all++;
	if (offset >= stringLength) {
	    break;
	}
    }

    /*
     * Set the interpreter's object result to an integer object with value 1
     * if -all wasn't specified, otherwise it's all-1 (the number of times
     * through the while - 1).
     */

    if (doinline) {
	Tcl_SetObjResult(interp, resultPtr);
    } else {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(all ? all-1 : 1));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegsubObjCmd --
 *
 *	This procedure is invoked to process the "regsub" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RegsubObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int idx, result, cflags, all, wlen, wsublen, numMatches, offset;
    int start, end, subStart, subEnd, match;
    Tcl_RegExp regExpr;
    Tcl_RegExpInfo info;
    Tcl_Obj *resultPtr, *subPtr, *objPtr, *startIndex = NULL;
    Tcl_UniChar ch, *wsrc, *wfirstChar, *wstring, *wsubspec, *wend;

    static const char *const options[] = {
	"-all",		"-nocase",	"-expanded",
	"-line",	"-linestop",	"-lineanchor",	"-start",
	"--",		NULL
    };
    enum options {
	REGSUB_ALL,	REGSUB_NOCASE,	REGSUB_EXPANDED,
	REGSUB_LINE,	REGSUB_LINESTOP, REGSUB_LINEANCHOR,	REGSUB_START,
	REGSUB_LAST
    };

    cflags = TCL_REG_ADVANCED;
    all = 0;
    offset = 0;
    resultPtr = NULL;

    for (idx = 1; idx < objc; idx++) {
	const char *name;
	int index;

	name = TclGetString(objv[idx]);
	if (name[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[idx], options, "option",
		TCL_EXACT, &index) != TCL_OK) {
	    goto optionError;
	}
	switch ((enum options) index) {
	case REGSUB_ALL:
	    all = 1;
	    break;
	case REGSUB_NOCASE:
	    cflags |= TCL_REG_NOCASE;
	    break;
	case REGSUB_EXPANDED:
	    cflags |= TCL_REG_EXPANDED;
	    break;
	case REGSUB_LINE:
	    cflags |= TCL_REG_NEWLINE;
	    break;
	case REGSUB_LINESTOP:
	    cflags |= TCL_REG_NLSTOP;
	    break;
	case REGSUB_LINEANCHOR:
	    cflags |= TCL_REG_NLANCH;
	    break;
	case REGSUB_START: {
	    int temp;
	    if (++idx >= objc) {
		goto endOfForLoop;
	    }
	    if (TclGetIntForIndexM(interp, objv[idx], 0, &temp) != TCL_OK) {
		goto optionError;
	    }
	    if (startIndex) {
		Tcl_DecrRefCount(startIndex);
	    }
	    startIndex = objv[idx];
	    Tcl_IncrRefCount(startIndex);
	    break;
	}
	case REGSUB_LAST:
	    idx++;
	    goto endOfForLoop;
	}
    }

  endOfForLoop:
    if (objc-idx < 3 || objc-idx > 4) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-option ...? exp string subSpec ?varName?");
    optionError:
	if (startIndex) {
	    Tcl_DecrRefCount(startIndex);
	}
	return TCL_ERROR;
    }

    objc -= idx;
    objv += idx;

    if (startIndex) {
	int stringLength = Tcl_GetCharLength(objv[1]);

	TclGetIntForIndexM(NULL, startIndex, stringLength, &offset);
	Tcl_DecrRefCount(startIndex);
	if (offset < 0) {
	    offset = 0;
	}
    }

    if (all && (offset == 0)
	    && (strpbrk(TclGetString(objv[2]), "&\\") == NULL)
	    && (strpbrk(TclGetString(objv[0]), "*+?{}()[].\\|^$") == NULL)) {
	/*
	 * This is a simple one pair string map situation. We make use of a
	 * slightly modified version of the one pair STR_MAP code.
	 */

	int slen, nocase;
	int (*strCmpFn)(const Tcl_UniChar*,const Tcl_UniChar*,unsigned long);
	Tcl_UniChar *p, wsrclc;

	numMatches = 0;
	nocase = (cflags & TCL_REG_NOCASE);
	strCmpFn = nocase ? Tcl_UniCharNcasecmp : Tcl_UniCharNcmp;

	wsrc = Tcl_GetUnicodeFromObj(objv[0], &slen);
	wstring = Tcl_GetUnicodeFromObj(objv[1], &wlen);
	wsubspec = Tcl_GetUnicodeFromObj(objv[2], &wsublen);
	wend = wstring + wlen - (slen ? slen - 1 : 0);
	result = TCL_OK;

	if (slen == 0) {
	    /*
	     * regsub behavior for "" matches between each character. 'string
	     * map' skips the "" case.
	     */

	    if (wstring < wend) {
		resultPtr = Tcl_NewUnicodeObj(wstring, 0);
		Tcl_IncrRefCount(resultPtr);
		for (; wstring < wend; wstring++) {
		    Tcl_AppendUnicodeToObj(resultPtr, wsubspec, wsublen);
		    Tcl_AppendUnicodeToObj(resultPtr, wstring, 1);
		    numMatches++;
		}
		wlen = 0;
	    }
	} else {
	    wsrclc = Tcl_UniCharToLower(*wsrc);
	    for (p = wfirstChar = wstring; wstring < wend; wstring++) {
		if ((*wstring == *wsrc ||
			(nocase && Tcl_UniCharToLower(*wstring)==wsrclc)) &&
			(slen==1 || (strCmpFn(wstring, wsrc,
				(unsigned long) slen) == 0))) {
		    if (numMatches == 0) {
			resultPtr = Tcl_NewUnicodeObj(wstring, 0);
			Tcl_IncrRefCount(resultPtr);
		    }
		    if (p != wstring) {
			Tcl_AppendUnicodeToObj(resultPtr, p, wstring - p);
			p = wstring + slen;
		    } else {
			p += slen;
		    }
		    wstring = p - 1;

		    Tcl_AppendUnicodeToObj(resultPtr, wsubspec, wsublen);
		    numMatches++;
		}
	    }
	    if (numMatches) {
		wlen    = wfirstChar + wlen - p;
		wstring = p;
	    }
	}
	objPtr = NULL;
	subPtr = NULL;
	goto regsubDone;
    }

    regExpr = Tcl_GetRegExpFromObj(interp, objv[0], cflags);
    if (regExpr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Make sure to avoid problems where the objects are shared. This can
     * cause RegExpObj <> UnicodeObj shimmering that causes data corruption.
     * [Bug #461322]
     */

    if (objv[1] == objv[0]) {
	objPtr = Tcl_DuplicateObj(objv[1]);
    } else {
	objPtr = objv[1];
    }
    wstring = Tcl_GetUnicodeFromObj(objPtr, &wlen);
    if (objv[2] == objv[0]) {
	subPtr = Tcl_DuplicateObj(objv[2]);
    } else {
	subPtr = objv[2];
    }
    wsubspec = Tcl_GetUnicodeFromObj(subPtr, &wsublen);

    result = TCL_OK;

    /*
     * The following loop is to handle multiple matches within the same source
     * string; each iteration handles one match and its corresponding
     * substitution. If "-all" hasn't been specified then the loop body only
     * gets executed once. We must use 'offset <= wlen' in particular for the
     * case where the regexp pattern can match the empty string - this is
     * useful when doing, say, 'regsub -- ^ $str ...' when $str might be
     * empty.
     */

    numMatches = 0;
    for ( ; offset <= wlen; ) {

	/*
	 * The flags argument is set if string is part of a larger string, so
	 * that "^" won't match.
	 */

	match = Tcl_RegExpExecObj(interp, regExpr, objPtr, offset,
		10 /* matches */, ((offset > 0 &&
		(wstring[offset-1] != (Tcl_UniChar)'\n'))
		? TCL_REG_NOTBOL : 0));

	if (match < 0) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (match == 0) {
	    break;
	}
	if (numMatches == 0) {
	    resultPtr = Tcl_NewUnicodeObj(wstring, 0);
	    Tcl_IncrRefCount(resultPtr);
	    if (offset > 0) {
		/*
		 * Copy the initial portion of the string in if an offset was
		 * specified.
		 */

		Tcl_AppendUnicodeToObj(resultPtr, wstring, offset);
	    }
	}
	numMatches++;

	/*
	 * Copy the portion of the source string before the match to the
	 * result variable.
	 */

	Tcl_RegExpGetInfo(regExpr, &info);
	start = info.matches[0].start;
	end = info.matches[0].end;
	Tcl_AppendUnicodeToObj(resultPtr, wstring + offset, start);

	/*
	 * Append the subSpec argument to the variable, making appropriate
	 * substitutions. This code is a bit hairy because of the backslash
	 * conventions and because the code saves up ranges of characters in
	 * subSpec to reduce the number of calls to Tcl_SetVar.
	 */

	wsrc = wfirstChar = wsubspec;
	wend = wsubspec + wsublen;
	for (ch = *wsrc; wsrc != wend; wsrc++, ch = *wsrc) {
	    if (ch == '&') {
		idx = 0;
	    } else if (ch == '\\') {
		ch = wsrc[1];
		if ((ch >= '0') && (ch <= '9')) {
		    idx = ch - '0';
		} else if ((ch == '\\') || (ch == '&')) {
		    *wsrc = ch;
		    Tcl_AppendUnicodeToObj(resultPtr, wfirstChar,
			    wsrc - wfirstChar + 1);
		    *wsrc = '\\';
		    wfirstChar = wsrc + 2;
		    wsrc++;
		    continue;
		} else {
		    continue;
		}
	    } else {
		continue;
	    }

	    if (wfirstChar != wsrc) {
		Tcl_AppendUnicodeToObj(resultPtr, wfirstChar,
			wsrc - wfirstChar);
	    }

	    if (idx <= info.nsubs) {
		subStart = info.matches[idx].start;
		subEnd = info.matches[idx].end;
		if ((subStart >= 0) && (subEnd >= 0)) {
		    Tcl_AppendUnicodeToObj(resultPtr,
			    wstring + offset + subStart, subEnd - subStart);
		}
	    }

	    if (*wsrc == '\\') {
		wsrc++;
	    }
	    wfirstChar = wsrc + 1;
	}

	if (wfirstChar != wsrc) {
	    Tcl_AppendUnicodeToObj(resultPtr, wfirstChar, wsrc - wfirstChar);
	}

	if (end == 0) {
	    /*
	     * Always consume at least one character of the input string in
	     * order to prevent infinite loops.
	     */

	    if (offset < wlen) {
		Tcl_AppendUnicodeToObj(resultPtr, wstring + offset, 1);
	    }
	    offset++;
	} else {
	    offset += end;
	    if (start == end) {
		/*
		 * We matched an empty string, which means we must go forward
		 * one more step so we don't match again at the same spot.
		 */

		if (offset < wlen) {
		    Tcl_AppendUnicodeToObj(resultPtr, wstring + offset, 1);
		}
		offset++;
	    }
	}
	if (!all) {
	    break;
	}
    }

    /*
     * Copy the portion of the source string after the last match to the
     * result variable.
     */

  regsubDone:
    if (numMatches == 0) {
	/*
	 * On zero matches, just ignore the offset, since it shouldn't matter
	 * to us in this case, and the user may have skewed it.
	 */

	resultPtr = objv[1];
	Tcl_IncrRefCount(resultPtr);
    } else if (offset < wlen) {
	Tcl_AppendUnicodeToObj(resultPtr, wstring + offset, wlen - offset);
    }
    if (objc == 4) {
	if (Tcl_ObjSetVar2(interp, objv[3], NULL, resultPtr,
		TCL_LEAVE_ERR_MSG) == NULL) {
	    result = TCL_ERROR;
	} else {
	    /*
	     * Set the interpreter's object result to an integer object
	     * holding the number of matches.
	     */

	    Tcl_SetObjResult(interp, Tcl_NewIntObj(numMatches));
	}
    } else {
	/*
	 * No varname supplied, so just return the modified string.
	 */

	Tcl_SetObjResult(interp, resultPtr);
    }

  done:
    if (objPtr && (objv[1] == objv[0])) {
	Tcl_DecrRefCount(objPtr);
    }
    if (subPtr && (objv[2] == objv[0])) {
	Tcl_DecrRefCount(subPtr);
    }
    if (resultPtr) {
	Tcl_DecrRefCount(resultPtr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RenameObjCmd --
 *
 *	This procedure is invoked to process the "rename" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RenameObjCmd(
    ClientData dummy,		/* Arbitrary value passed to the command. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *oldName, *newName;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "oldName newName");
	return TCL_ERROR;
    }

    oldName = TclGetString(objv[1]);
    newName = TclGetString(objv[2]);
    return TclRenameCommand(interp, oldName, newName);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ReturnObjCmd --
 *
 *	This object-based procedure is invoked to process the "return" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ReturnObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int code, level;
    Tcl_Obj *returnOpts;

    /*
     * General syntax: [return ?-option value ...? ?result?]
     * An even number of words means an explicit result argument is present.
     */

    int explicitResult = (0 == (objc % 2));
    int numOptionWords = objc - 1 - explicitResult;

    if (TCL_ERROR == TclMergeReturnOptions(interp, numOptionWords, objv+1,
	    &returnOpts, &code, &level)) {
	return TCL_ERROR;
    }

    code = TclProcessReturn(interp, code, level, returnOpts);
    if (explicitResult) {
	Tcl_SetObjResult(interp, objv[objc-1]);
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SourceObjCmd --
 *
 *	This procedure is invoked to process the "source" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SourceObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return Tcl_NRCallObjProc(interp, TclNRSourceObjCmd, dummy, objc, objv);
}

int
TclNRSourceObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *encodingName = NULL;
    Tcl_Obj *fileName;

    if (objc != 2 && objc !=4) {
	Tcl_WrongNumArgs(interp, 1, objv, "?-encoding name? fileName");
	return TCL_ERROR;
    }

    fileName = objv[objc-1];

    if (objc == 4) {
	static const char *const options[] = {
	    "-encoding", NULL
	};
	int index;

	if (TCL_ERROR == Tcl_GetIndexFromObj(interp, objv[1], options,
		"option", TCL_EXACT, &index)) {
	    return TCL_ERROR;
	}
	encodingName = TclGetString(objv[2]);
    }

    return TclNREvalFile(interp, fileName, encodingName);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SplitObjCmd --
 *
 *	This procedure is invoked to process the "split" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SplitObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_UniChar ch = 0;
    int len;
    const char *splitChars;
    const char *stringPtr;
    const char *end;
    int splitCharLen, stringLen;
    Tcl_Obj *listPtr, *objPtr;

    if (objc == 2) {
	splitChars = " \n\t\r";
	splitCharLen = 4;
    } else if (objc == 3) {
	splitChars = TclGetStringFromObj(objv[2], &splitCharLen);
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "string ?splitChars?");
	return TCL_ERROR;
    }

    stringPtr = TclGetStringFromObj(objv[1], &stringLen);
    end = stringPtr + stringLen;
    listPtr = Tcl_NewObj();

    if (stringLen == 0) {
	/*
	 * Do nothing.
	 */
    } else if (splitCharLen == 0) {
	Tcl_HashTable charReuseTable;
	Tcl_HashEntry *hPtr;
	int isNew;

	/*
	 * Handle the special case of splitting on every character.
	 *
	 * Uses a hash table to ensure that each kind of character has only
	 * one Tcl_Obj instance (multiply-referenced) in the final list. This
	 * is a *major* win when splitting on a long string (especially in the
	 * megabyte range!) - DKF
	 */

	Tcl_InitHashTable(&charReuseTable, TCL_ONE_WORD_KEYS);

	for ( ; stringPtr < end; stringPtr += len) {
	    int fullchar;
	    len = TclUtfToUniChar(stringPtr, &ch);
	    fullchar = ch;

#if TCL_UTF_MAX == 4
	    if ((ch >= 0xD800) && (len < 3)) {
		len += TclUtfToUniChar(stringPtr + len, &ch);
		fullchar = (((fullchar & 0x3ff) << 10) | (ch & 0x3ff)) + 0x10000;
	    }
#endif

	    /*
	     * Assume Tcl_UniChar is an integral type...
	     */

	    hPtr = Tcl_CreateHashEntry(&charReuseTable, INT2PTR(fullchar),
		    &isNew);
	    if (isNew) {
		TclNewStringObj(objPtr, stringPtr, len);

		/*
		 * Don't need to fiddle with refcount...
		 */

		Tcl_SetHashValue(hPtr, objPtr);
	    } else {
		objPtr = Tcl_GetHashValue(hPtr);
	    }
	    Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
	}
	Tcl_DeleteHashTable(&charReuseTable);

    } else if (splitCharLen == 1) {
	char *p;

	/*
	 * Handle the special case of splitting on a single character. This is
	 * only true for the one-char ASCII case, as one unicode char is > 1
	 * byte in length.
	 */

	while (*stringPtr && (p=strchr(stringPtr,(int)*splitChars)) != NULL) {
	    objPtr = Tcl_NewStringObj(stringPtr, p - stringPtr);
	    Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
	    stringPtr = p + 1;
	}
	TclNewStringObj(objPtr, stringPtr, end - stringPtr);
	Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
    } else {
	const char *element, *p, *splitEnd;
	int splitLen;
	Tcl_UniChar splitChar = 0;

	/*
	 * Normal case: split on any of a given set of characters. Discard
	 * instances of the split characters.
	 */

	splitEnd = splitChars + splitCharLen;

	for (element = stringPtr; stringPtr < end; stringPtr += len) {
	    len = TclUtfToUniChar(stringPtr, &ch);
	    for (p = splitChars; p < splitEnd; p += splitLen) {
		splitLen = TclUtfToUniChar(p, &splitChar);
		if (ch == splitChar) {
		    TclNewStringObj(objPtr, element, stringPtr - element);
		    Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
		    element = stringPtr + len;
		    break;
		}
	    }
	}

	TclNewStringObj(objPtr, element, stringPtr - element);
	Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringFirstCmd --
 *
 *	This procedure is invoked to process the "string first" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringFirstCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_UniChar *needleStr, *haystackStr;
    int match, start, needleLen, haystackLen;

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"needleString haystackString ?startIndex?");
	return TCL_ERROR;
    }

    /*
     * We are searching haystackStr for the sequence needleStr.
     */

    match = -1;
    start = 0;
    haystackLen = -1;

    needleStr = Tcl_GetUnicodeFromObj(objv[1], &needleLen);
    haystackStr = Tcl_GetUnicodeFromObj(objv[2], &haystackLen);

    if (objc == 4) {
	/*
	 * If a startIndex is specified, we will need to fast forward to that
	 * point in the string before we think about a match.
	 */

	if (TclGetIntForIndexM(interp, objv[3], haystackLen-1,
		&start) != TCL_OK){
	    return TCL_ERROR;
	}

	/*
	 * Reread to prevent shimmering problems.
	 */

	needleStr = Tcl_GetUnicodeFromObj(objv[1], &needleLen);
	haystackStr = Tcl_GetUnicodeFromObj(objv[2], &haystackLen);

	if (start >= haystackLen) {
	    goto str_first_done;
	} else if (start > 0) {
	    haystackStr += start;
	    haystackLen -= start;
	} else if (start < 0) {
	    /*
	     * Invalid start index mapped to string start; Bug #423581
	     */

	    start = 0;
	}
    }

    /*
     * If the length of the needle is more than the length of the haystack, it
     * cannot be contained in there so we can avoid searching. [Bug 2960021]
     */

    if (needleLen > 0 && needleLen <= haystackLen) {
	register Tcl_UniChar *p, *end;

	end = haystackStr + haystackLen - needleLen + 1;
	for (p = haystackStr;  p < end;  p++) {
	    /*
	     * Scan forward to find the first character.
	     */

	    if ((*p == *needleStr) && (TclUniCharNcmp(needleStr, p,
		    (unsigned long) needleLen) == 0)) {
		match = p - haystackStr;
		break;
	    }
	}
    }

    /*
     * Compute the character index of the matching string by counting the
     * number of characters before the match.
     */

    if ((match != -1) && (objc == 4)) {
	match += start;
    }

  str_first_done:
    Tcl_SetObjResult(interp, Tcl_NewIntObj(match));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringLastCmd --
 *
 *	This procedure is invoked to process the "string last" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringLastCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_UniChar *needleStr, *haystackStr, *p;
    int match, start, needleLen, haystackLen;

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"needleString haystackString ?startIndex?");
	return TCL_ERROR;
    }

    /*
     * We are searching haystackString for the sequence needleString.
     */

    match = -1;
    start = 0;
    haystackLen = -1;

    needleStr = Tcl_GetUnicodeFromObj(objv[1], &needleLen);
    haystackStr = Tcl_GetUnicodeFromObj(objv[2], &haystackLen);

    if (objc == 4) {
	/*
	 * If a startIndex is specified, we will need to restrict the string
	 * range to that char index in the string
	 */

	if (TclGetIntForIndexM(interp, objv[3], haystackLen-1,
		&start) != TCL_OK){
	    return TCL_ERROR;
	}

	/*
	 * Reread to prevent shimmering problems.
	 */

	needleStr = Tcl_GetUnicodeFromObj(objv[1], &needleLen);
	haystackStr = Tcl_GetUnicodeFromObj(objv[2], &haystackLen);

	if (start < 0) {
	    goto str_last_done;
	} else if (start < haystackLen) {
	    p = haystackStr + start + 1 - needleLen;
	} else {
	    p = haystackStr + haystackLen - needleLen;
	}
    } else {
	p = haystackStr + haystackLen - needleLen;
    }

    /*
     * If the length of the needle is more than the length of the haystack, it
     * cannot be contained in there so we can avoid searching. [Bug 2960021]
     */

    if (needleLen > 0 && needleLen <= haystackLen) {
	for (; p >= haystackStr; p--) {
	    /*
	     * Scan backwards to find the first character.
	     */

	    if ((*p == *needleStr) && !memcmp(needleStr, p,
		    sizeof(Tcl_UniChar) * (size_t)needleLen)) {
		match = p - haystackStr;
		break;
	    }
	}
    }

  str_last_done:
    Tcl_SetObjResult(interp, Tcl_NewIntObj(match));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringIndexCmd --
 *
 *	This procedure is invoked to process the "string index" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringIndexCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int length, index;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "string charIndex");
	return TCL_ERROR;
    }

    /*
     * Get the char length to calulate what 'end' means.
     */

    length = Tcl_GetCharLength(objv[1]);
    if (TclGetIntForIndexM(interp, objv[2], length-1, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    if ((index >= 0) && (index < length)) {
	Tcl_UniChar ch = Tcl_GetUniChar(objv[1], index);

	/*
	 * If we have a ByteArray object, we're careful to generate a new
	 * bytearray for a result.
	 */

	if (TclIsPureByteArray(objv[1])) {
	    unsigned char uch = (unsigned char) ch;

	    Tcl_SetObjResult(interp, Tcl_NewByteArrayObj(&uch, 1));
	} else {
	    char buf[TCL_UTF_MAX] = "";

	    length = Tcl_UniCharToUtf(ch, buf);
#if TCL_UTF_MAX > 3
	    if ((ch >= 0xD800) && (length < 3)) {
		length += Tcl_UniCharToUtf(-1, buf + length);
	    }
#endif
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(buf, length));
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringIsCmd --
 *
 *	This procedure is invoked to process the "string is" Tcl command. See
 *	the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringIsCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *string1, *end, *stop;
    Tcl_UniChar ch = 0;
    int (*chcomp)(int) = NULL;	/* The UniChar comparison function. */
    int i, failat = 0, result = 1, strict = 0, index, length1, length2;
    Tcl_Obj *objPtr, *failVarObj = NULL;
    Tcl_WideInt w;

    static const char *const isClasses[] = {
	"alnum",	"alpha",	"ascii",	"control",
	"boolean",	"digit",	"double",	"entier",
	"false",	"graph",	"integer",	"list",
	"lower",	"print",	"punct",	"space",
	"true",		"upper",	"wideinteger",	"wordchar",
	"xdigit",	NULL
    };
    enum isClasses {
	STR_IS_ALNUM,	STR_IS_ALPHA,	STR_IS_ASCII,	STR_IS_CONTROL,
	STR_IS_BOOL,	STR_IS_DIGIT,	STR_IS_DOUBLE,	STR_IS_ENTIER,
	STR_IS_FALSE,	STR_IS_GRAPH,	STR_IS_INT,	STR_IS_LIST,
	STR_IS_LOWER,	STR_IS_PRINT,	STR_IS_PUNCT,	STR_IS_SPACE,
	STR_IS_TRUE,	STR_IS_UPPER,	STR_IS_WIDE,	STR_IS_WORD,
	STR_IS_XDIGIT
    };
    static const char *const isOptions[] = {
	"-strict", "-failindex", NULL
    };
    enum isOptions {
	OPT_STRICT, OPT_FAILIDX
    };

    if (objc < 3 || objc > 6) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"class ?-strict? ?-failindex var? str");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], isClasses, "class", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    if (objc != 3) {
	for (i = 2; i < objc-1; i++) {
	    int idx2;

	    if (Tcl_GetIndexFromObj(interp, objv[i], isOptions, "option", 0,
		    &idx2) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch ((enum isOptions) idx2) {
	    case OPT_STRICT:
		strict = 1;
		break;
	    case OPT_FAILIDX:
		if (i+1 >= objc-1) {
		    Tcl_WrongNumArgs(interp, 2, objv,
			    "?-strict? ?-failindex var? str");
		    return TCL_ERROR;
		}
		failVarObj = objv[++i];
		break;
	    }
	}
    }

    /*
     * We get the objPtr so that we can short-cut for some classes by checking
     * the object type (int and double), but we need the string otherwise,
     * because we don't want any conversion of type occuring (as, for example,
     * Tcl_Get*FromObj would do).
     */

    objPtr = objv[objc-1];

    /*
     * When entering here, result == 1 and failat == 0.
     */

    switch ((enum isClasses) index) {
    case STR_IS_ALNUM:
	chcomp = Tcl_UniCharIsAlnum;
	break;
    case STR_IS_ALPHA:
	chcomp = Tcl_UniCharIsAlpha;
	break;
    case STR_IS_ASCII:
	chcomp = UniCharIsAscii;
	break;
    case STR_IS_BOOL:
    case STR_IS_TRUE:
    case STR_IS_FALSE:
	if ((objPtr->typePtr != &tclBooleanType)
		&& (TCL_OK != TclSetBooleanFromAny(NULL, objPtr))) {
	    if (strict) {
		result = 0;
	    } else {
		string1 = TclGetStringFromObj(objPtr, &length1);
		result = length1 == 0;
	    }
	} else if (((index == STR_IS_TRUE) &&
		objPtr->internalRep.longValue == 0)
	    || ((index == STR_IS_FALSE) &&
		objPtr->internalRep.longValue != 0)) {
	    result = 0;
	}
	break;
    case STR_IS_CONTROL:
	chcomp = Tcl_UniCharIsControl;
	break;
    case STR_IS_DIGIT:
	chcomp = Tcl_UniCharIsDigit;
	break;
    case STR_IS_DOUBLE: {
	if ((objPtr->typePtr == &tclDoubleType) ||
		(objPtr->typePtr == &tclIntType) ||
#ifndef TCL_WIDE_INT_IS_LONG
		(objPtr->typePtr == &tclWideIntType) ||
#endif
		(objPtr->typePtr == &tclBignumType)) {
	    break;
	}
	string1 = TclGetStringFromObj(objPtr, &length1);
	if (length1 == 0) {
	    if (strict) {
		result = 0;
	    }
	    goto str_is_done;
	}
	end = string1 + length1;
	if (TclParseNumber(NULL, objPtr, NULL, NULL, -1,
		(const char **) &stop, 0) != TCL_OK) {
	    result = 0;
	    failat = 0;
	} else {
	    failat = stop - string1;
	    if (stop < end) {
		result = 0;
		TclFreeIntRep(objPtr);
	    }
	}
	break;
    }
    case STR_IS_GRAPH:
	chcomp = Tcl_UniCharIsGraph;
	break;
    case STR_IS_INT:
	if (TCL_OK == TclGetIntFromObj(NULL, objPtr, &i)) {
	    break;
	}
	goto failedIntParse;
    case STR_IS_ENTIER:
	if ((objPtr->typePtr == &tclIntType) ||
#ifndef TCL_WIDE_INT_IS_LONG
		(objPtr->typePtr == &tclWideIntType) ||
#endif
		(objPtr->typePtr == &tclBignumType)) {
	    break;
	}
	string1 = TclGetStringFromObj(objPtr, &length1);
	if (length1 == 0) {
	    if (strict) {
		result = 0;
	    }
	    goto str_is_done;
	}
	end = string1 + length1;
	if (TclParseNumber(NULL, objPtr, NULL, NULL, -1,
		(const char **) &stop, TCL_PARSE_INTEGER_ONLY) == TCL_OK) {
	    if (stop == end) {
		/*
		 * Entire string parses as an integer.
		 */

		break;
	    } else {
		/*
		 * Some prefix parsed as an integer, but not the whole string,
		 * so return failure index as the point where parsing stopped.
		 * Clear out the internal rep, since keeping it would leave
		 * *objPtr in an inconsistent state.
		 */

		result = 0;
		failat = stop - string1;
		TclFreeIntRep(objPtr);
	    }
	} else {
	    /*
	     * No prefix is a valid integer. Fail at beginning.
	     */

	    result = 0;
	    failat = 0;
	}
	break;
    case STR_IS_WIDE:
	if (TCL_OK == TclGetWideIntFromObj(NULL, objPtr, &w)) {
	    break;
	}

    failedIntParse:
	string1 = TclGetStringFromObj(objPtr, &length1);
	if (length1 == 0) {
	    if (strict) {
		result = 0;
	    }
	    goto str_is_done;
	}
	result = 0;
	if (failVarObj == NULL) {
	    /*
	     * Don't bother computing the failure point if we're not going to
	     * return it.
	     */

	    break;
	}
	end = string1 + length1;
	if (TclParseNumber(NULL, objPtr, NULL, NULL, -1,
		(const char **) &stop, TCL_PARSE_INTEGER_ONLY) == TCL_OK) {
	    if (stop == end) {
		/*
		 * Entire string parses as an integer, but rejected by
		 * Tcl_Get(Wide)IntFromObj() so we must have overflowed the
		 * target type, and our convention is to return failure at
		 * index -1 in that situation.
		 */

		failat = -1;
	    } else {
		/*
		 * Some prefix parsed as an integer, but not the whole string,
		 * so return failure index as the point where parsing stopped.
		 * Clear out the internal rep, since keeping it would leave
		 * *objPtr in an inconsistent state.
		 */

		failat = stop - string1;
		TclFreeIntRep(objPtr);
	    }
	} else {
	    /*
	     * No prefix is a valid integer. Fail at beginning.
	     */

	    failat = 0;
	}
	break;
    case STR_IS_LIST:
	/*
	 * We ignore the strictness here, since empty strings are always
	 * well-formed lists.
	 */

	if (TCL_OK == TclListObjLength(NULL, objPtr, &length2)) {
	    break;
	}

	if (failVarObj != NULL) {
	    /*
	     * Need to figure out where the list parsing failed, which is
	     * fairly expensive. This is adapted from the core of
	     * SetListFromAny().
	     */

	    const char *elemStart, *nextElem;
	    int lenRemain, elemSize;
	    register const char *p;

	    string1 = TclGetStringFromObj(objPtr, &length1);
	    end = string1 + length1;
	    failat = -1;
	    for (p=string1, lenRemain=length1; lenRemain > 0;
		    p=nextElem, lenRemain=end-nextElem) {
		if (TCL_ERROR == TclFindElement(NULL, p, lenRemain,
			&elemStart, &nextElem, &elemSize, NULL)) {
		    Tcl_Obj *tmpStr;

		    /*
		     * This is the simplest way of getting the number of
		     * characters parsed. Note that this is not the same as
		     * the number of bytes when parsing strings with non-ASCII
		     * characters in them.
		     *
		     * Skip leading spaces first. This is only really an issue
		     * if it is the first "element" that has the failure.
		     */

		    while (TclIsSpaceProc(*p)) {
			p++;
		    }
		    TclNewStringObj(tmpStr, string1, p-string1);
		    failat = Tcl_GetCharLength(tmpStr);
		    TclDecrRefCount(tmpStr);
		    break;
		}
	    }
	}
	result = 0;
	break;
    case STR_IS_LOWER:
	chcomp = Tcl_UniCharIsLower;
	break;
    case STR_IS_PRINT:
	chcomp = Tcl_UniCharIsPrint;
	break;
    case STR_IS_PUNCT:
	chcomp = Tcl_UniCharIsPunct;
	break;
    case STR_IS_SPACE:
	chcomp = Tcl_UniCharIsSpace;
	break;
    case STR_IS_UPPER:
	chcomp = Tcl_UniCharIsUpper;
	break;
    case STR_IS_WORD:
	chcomp = Tcl_UniCharIsWordChar;
	break;
    case STR_IS_XDIGIT:
	chcomp = UniCharIsHexDigit;
	break;
    }

    if (chcomp != NULL) {
	string1 = TclGetStringFromObj(objPtr, &length1);
	if (length1 == 0) {
	    if (strict) {
		result = 0;
	    }
	    goto str_is_done;
	}
	end = string1 + length1;
	for (; string1 < end; string1 += length2, failat++) {
	    int fullchar;
	    length2 = TclUtfToUniChar(string1, &ch);
	    fullchar = ch;
#if TCL_UTF_MAX == 4
	    if ((ch >= 0xD800) && (length2 < 3)) {
	    	length2 += TclUtfToUniChar(string1 + length2, &ch);
	    	fullchar = (((fullchar & 0x3ff) << 10) | (ch & 0x3ff)) + 0x10000;
	    }
#endif
	    if (!chcomp(fullchar)) {
		result = 0;
		break;
	    }
	}
    }

    /*
     * Only set the failVarObj when we will return 0 and we have indicated a
     * valid fail index (>= 0).
     */

 str_is_done:
    if ((result == 0) && (failVarObj != NULL) &&
	Tcl_ObjSetVar2(interp, failVarObj, NULL, Tcl_NewIntObj(failat),
		TCL_LEAVE_ERR_MSG) == NULL) {
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result));
    return TCL_OK;
}

static int
UniCharIsAscii(
    int character)
{
    return (character >= 0) && (character < 0x80);
}

static int
UniCharIsHexDigit(
    int character)
{
    return (character >= 0) && (character < 0x80) && isxdigit(character);
}

/*
 *----------------------------------------------------------------------
 *
 * StringMapCmd --
 *
 *	This procedure is invoked to process the "string map" Tcl command. See
 *	the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringMapCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int length1, length2, mapElemc, index;
    int nocase = 0, mapWithDict = 0, copySource = 0;
    Tcl_Obj **mapElemv, *sourceObj, *resultPtr;
    Tcl_UniChar *ustring1, *ustring2, *p, *end;
    int (*strCmpFn)(const Tcl_UniChar*, const Tcl_UniChar*, unsigned long);

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "?-nocase? charMap string");
	return TCL_ERROR;
    }

    if (objc == 4) {
	const char *string = TclGetStringFromObj(objv[1], &length2);

	if ((length2 > 1) &&
		strncmp(string, "-nocase", length2) == 0) {
	    nocase = 1;
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad option \"%s\": must be -nocase", string));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "INDEX", "option",
		    string, NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * This test is tricky, but has to be that way or you get other strange
     * inconsistencies (see test string-10.20 for illustration why!)
     */

    if (objv[objc-2]->typePtr == &tclDictType && objv[objc-2]->bytes == NULL){
	int i, done;
	Tcl_DictSearch search;

	/*
	 * We know the type exactly, so all dict operations will succeed for
	 * sure. This shortens this code quite a bit.
	 */

	Tcl_DictObjSize(interp, objv[objc-2], &mapElemc);
	if (mapElemc == 0) {
	    /*
	     * Empty charMap, just return whatever string was given.
	     */

	    Tcl_SetObjResult(interp, objv[objc-1]);
	    return TCL_OK;
	}

	mapElemc *= 2;
	mapWithDict = 1;

	/*
	 * Copy the dictionary out into an array; that's the easiest way to
	 * adapt this code...
	 */

	mapElemv = TclStackAlloc(interp, sizeof(Tcl_Obj *) * mapElemc);
	Tcl_DictObjFirst(interp, objv[objc-2], &search, mapElemv+0,
		mapElemv+1, &done);
	for (i=2 ; i<mapElemc ; i+=2) {
	    Tcl_DictObjNext(&search, mapElemv+i, mapElemv+i+1, &done);
	}
	Tcl_DictObjDone(&search);
    } else {
	if (TclListObjGetElements(interp, objv[objc-2], &mapElemc,
		&mapElemv) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (mapElemc == 0) {
	    /*
	     * empty charMap, just return whatever string was given.
	     */

	    Tcl_SetObjResult(interp, objv[objc-1]);
	    return TCL_OK;
	} else if (mapElemc & 1) {
	    /*
	     * The charMap must be an even number of key/value items.
	     */

	    Tcl_SetObjResult(interp,
		    Tcl_NewStringObj("char map list unbalanced", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "MAP",
		    "UNBALANCED", NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * Take a copy of the source string object if it is the same as the map
     * string to cut out nasty sharing crashes. [Bug 1018562]
     */

    if (objv[objc-2] == objv[objc-1]) {
	sourceObj = Tcl_DuplicateObj(objv[objc-1]);
	copySource = 1;
    } else {
	sourceObj = objv[objc-1];
    }
    ustring1 = Tcl_GetUnicodeFromObj(sourceObj, &length1);
    if (length1 == 0) {
	/*
	 * Empty input string, just stop now.
	 */

	goto done;
    }
    end = ustring1 + length1;

    strCmpFn = (nocase ? Tcl_UniCharNcasecmp : Tcl_UniCharNcmp);

    /*
     * Force result to be Unicode
     */

    resultPtr = Tcl_NewUnicodeObj(ustring1, 0);

    if (mapElemc == 2) {
	/*
	 * Special case for one map pair which avoids the extra for loop and
	 * extra calls to get Unicode data. The algorithm is otherwise
	 * identical to the multi-pair case. This will be >30% faster on
	 * larger strings.
	 */

	int mapLen;
	Tcl_UniChar *mapString, u2lc;

	ustring2 = Tcl_GetUnicodeFromObj(mapElemv[0], &length2);
	p = ustring1;
	if ((length2 > length1) || (length2 == 0)) {
	    /*
	     * Match string is either longer than input or empty.
	     */

	    ustring1 = end;
	} else {
	    mapString = Tcl_GetUnicodeFromObj(mapElemv[1], &mapLen);
	    u2lc = (nocase ? Tcl_UniCharToLower(*ustring2) : 0);
	    for (; ustring1 < end; ustring1++) {
		if (((*ustring1 == *ustring2) ||
			(nocase&&Tcl_UniCharToLower(*ustring1)==u2lc)) &&
			(length2==1 || strCmpFn(ustring1, ustring2,
				(unsigned long) length2) == 0)) {
		    if (p != ustring1) {
			Tcl_AppendUnicodeToObj(resultPtr, p, ustring1-p);
			p = ustring1 + length2;
		    } else {
			p += length2;
		    }
		    ustring1 = p - 1;

		    Tcl_AppendUnicodeToObj(resultPtr, mapString, mapLen);
		}
	    }
	}
    } else {
	Tcl_UniChar **mapStrings, *u2lc = NULL;
	int *mapLens;

	/*
	 * Precompute pointers to the unicode string and length. This saves us
	 * repeated function calls later, significantly speeding up the
	 * algorithm. We only need the lowercase first char in the nocase
	 * case.
	 */

	mapStrings = TclStackAlloc(interp, mapElemc*2*sizeof(Tcl_UniChar *));
	mapLens = TclStackAlloc(interp, mapElemc * 2 * sizeof(int));
	if (nocase) {
	    u2lc = TclStackAlloc(interp, mapElemc * sizeof(Tcl_UniChar));
	}
	for (index = 0; index < mapElemc; index++) {
	    mapStrings[index] = Tcl_GetUnicodeFromObj(mapElemv[index],
		    mapLens+index);
	    if (nocase && ((index % 2) == 0)) {
		u2lc[index/2] = Tcl_UniCharToLower(*mapStrings[index]);
	    }
	}
	for (p = ustring1; ustring1 < end; ustring1++) {
	    for (index = 0; index < mapElemc; index += 2) {
		/*
		 * Get the key string to match on.
		 */

		ustring2 = mapStrings[index];
		length2 = mapLens[index];
		if ((length2 > 0) && ((*ustring1 == *ustring2) || (nocase &&
			(Tcl_UniCharToLower(*ustring1) == u2lc[index/2]))) &&
			/* Restrict max compare length. */
			(end-ustring1 >= length2) && ((length2 == 1) ||
			!strCmpFn(ustring2, ustring1, (unsigned) length2))) {
		    if (p != ustring1) {
			/*
			 * Put the skipped chars onto the result first.
			 */

			Tcl_AppendUnicodeToObj(resultPtr, p, ustring1-p);
			p = ustring1 + length2;
		    } else {
			p += length2;
		    }

		    /*
		     * Adjust len to be full length of matched string.
		     */

		    ustring1 = p - 1;

		    /*
		     * Append the map value to the unicode string.
		     */

		    Tcl_AppendUnicodeToObj(resultPtr,
			    mapStrings[index+1], mapLens[index+1]);
		    break;
		}
	    }
	}
	if (nocase) {
	    TclStackFree(interp, u2lc);
	}
	TclStackFree(interp, mapLens);
	TclStackFree(interp, mapStrings);
    }
    if (p != ustring1) {
	/*
	 * Put the rest of the unmapped chars onto result.
	 */

	Tcl_AppendUnicodeToObj(resultPtr, p, ustring1 - p);
    }
    Tcl_SetObjResult(interp, resultPtr);
  done:
    if (mapWithDict) {
	TclStackFree(interp, mapElemv);
    }
    if (copySource) {
	Tcl_DecrRefCount(sourceObj);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringMatchCmd --
 *
 *	This procedure is invoked to process the "string match" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringMatchCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int nocase = 0;

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "?-nocase? pattern string");
	return TCL_ERROR;
    }

    if (objc == 4) {
	int length;
	const char *string = TclGetStringFromObj(objv[1], &length);

	if ((length > 1) &&
	    strncmp(string, "-nocase", length) == 0) {
	    nocase = TCL_MATCH_NOCASE;
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad option \"%s\": must be -nocase", string));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "INDEX", "option",
		    string, NULL);
	    return TCL_ERROR;
	}
    }
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
		TclStringMatchObj(objv[objc-1], objv[objc-2], nocase)));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringRangeCmd --
 *
 *	This procedure is invoked to process the "string range" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringRangeCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int length, first, last;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "string first last");
	return TCL_ERROR;
    }

    /*
     * Get the length in actual characters; Then reduce it by one because
     * 'end' refers to the last character, not one past it.
     */

    length = Tcl_GetCharLength(objv[1]) - 1;

    if (TclGetIntForIndexM(interp, objv[2], length, &first) != TCL_OK ||
	    TclGetIntForIndexM(interp, objv[3], length, &last) != TCL_OK) {
	return TCL_ERROR;
    }

    if (first < 0) {
	first = 0;
    }
    if (last >= length) {
	last = length;
    }
    if (last >= first) {
	Tcl_SetObjResult(interp, Tcl_GetRange(objv[1], first, last));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringReptCmd --
 *
 *	This procedure is invoked to process the "string repeat" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringReptCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *string1;
    char *string2;
    int count, index, length1, length2;
    Tcl_Obj *resultPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "string count");
	return TCL_ERROR;
    }

    if (TclGetIntFromObj(interp, objv[2], &count) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Check for cases that allow us to skip copying stuff.
     */

    if (count == 1) {
	Tcl_SetObjResult(interp, objv[1]);
	goto done;
    } else if (count < 1) {
	goto done;
    }
    string1 = TclGetStringFromObj(objv[1], &length1);
    if (length1 <= 0) {
	goto done;
    }

    /*
     * Only build up a string that has data. Instead of building it up with
     * repeated appends, we just allocate the necessary space once and copy
     * the string value in.
     *
     * We have to worry about overflow [Bugs 714106, 2561746].
     * At this point we know 1 <= length1 <= INT_MAX and 2 <= count <= INT_MAX.
     * We need to keep 2 <= length2 <= INT_MAX.
     */

    if (count > INT_MAX/length1) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"result exceeds max size for a Tcl value (%d bytes)",
		INT_MAX));
	Tcl_SetErrorCode(interp, "TCL", "MEMORY", NULL);
	return TCL_ERROR;
    }
    length2 = length1 * count;

    /*
     * Include space for the NUL.
     */

    string2 = attemptckalloc((unsigned) length2 + 1);
    if (string2 == NULL) {
	/*
	 * Alloc failed. Note that in this case we try to do an error message
	 * since this is a case that's most likely when the alloc is large and
	 * that's easy to do with this API. Note that if we fail allocating a
	 * short string, this will likely keel over too (and fatally).
	 */

	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"string size overflow, out of memory allocating %u bytes",
		length2 + 1));
	Tcl_SetErrorCode(interp, "TCL", "MEMORY", NULL);
	return TCL_ERROR;
    }
    for (index = 0; index < count; index++) {
	memcpy(string2 + (length1 * index), string1, (size_t) length1);
    }
    string2[length2] = '\0';

    /*
     * We have to directly assign this instead of using Tcl_SetStringObj (and
     * indirectly TclInitStringRep) because that makes another copy of the
     * data.
     */

    TclNewObj(resultPtr);
    resultPtr->bytes = string2;
    resultPtr->length = length2;
    Tcl_SetObjResult(interp, resultPtr);

  done:
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringRplcCmd --
 *
 *	This procedure is invoked to process the "string replace" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringRplcCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_UniChar *ustring;
    int first, last, length, end;

    if (objc < 4 || objc > 5) {
	Tcl_WrongNumArgs(interp, 1, objv, "string first last ?string?");
	return TCL_ERROR;
    }

    ustring = Tcl_GetUnicodeFromObj(objv[1], &length);
    end = length - 1;

    if (TclGetIntForIndexM(interp, objv[2], end, &first) != TCL_OK ||
	    TclGetIntForIndexM(interp, objv[3], end, &last) != TCL_OK){
	return TCL_ERROR;
    }

    /*
     * The following test screens out most empty substrings as
     * candidates for replacement. When they are detected, no
     * replacement is done, and the result is the original string,
     */
    if ((last < 0) ||		/* Range ends before start of string */
	    (first > end) ||	/* Range begins after end of string */
	    (last < first)) {	/* Range begins after it starts */

	/*
	 * BUT!!! when (end < 0) -- an empty original string -- we can
	 * have (first <= end < 0 <= last) and an empty string is permitted
	 * to be replaced.
	 */
	Tcl_SetObjResult(interp, objv[1]);
    } else {
	Tcl_Obj *resultPtr;

	/*
	 * We are re-fetching in case the string argument is same value as
	 * an index argument, and shimmering cost us our ustring.
	 */

	ustring = Tcl_GetUnicodeFromObj(objv[1], &length);
	end = length-1;

	if (first < 0) {
	    first = 0;
	}

	resultPtr = Tcl_NewUnicodeObj(ustring, first);
	if (objc == 5) {
	    Tcl_AppendObjToObj(resultPtr, objv[4]);
	}
	if (last < end) {
	    Tcl_AppendUnicodeToObj(resultPtr, ustring + last + 1,
		    end - last);
	}
	Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringRevCmd --
 *
 *	This procedure is invoked to process the "string reverse" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringRevCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "string");
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, TclStringObjReverse(objv[1]));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringStartCmd --
 *
 *	This procedure is invoked to process the "string wordstart" Tcl
 *	command. See the user documentation for details on what it does. Note
 *	that this command only functions correctly on properly formed Tcl UTF
 *	strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringStartCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_UniChar ch = 0;
    const char *p, *string;
    int cur, index, length, numChars;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "string index");
	return TCL_ERROR;
    }

    string = TclGetStringFromObj(objv[1], &length);
    numChars = Tcl_NumUtfChars(string, length);
    if (TclGetIntForIndexM(interp, objv[2], numChars-1, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    string = TclGetStringFromObj(objv[1], &length);
    if (index >= numChars) {
	index = numChars - 1;
    }
    cur = 0;
    if (index > 0) {
	p = Tcl_UtfAtIndex(string, index);
	for (cur = index; cur >= 0; cur--) {
	    TclUtfToUniChar(p, &ch);
	    if (!Tcl_UniCharIsWordChar(ch)) {
		break;
	    }
	    p = Tcl_UtfPrev(p, string);
	}
	if (cur != index) {
	    cur += 1;
	}
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(cur));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringEndCmd --
 *
 *	This procedure is invoked to process the "string wordend" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringEndCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_UniChar ch = 0;
    const char *p, *end, *string;
    int cur, index, length, numChars;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "string index");
	return TCL_ERROR;
    }

    string = TclGetStringFromObj(objv[1], &length);
    numChars = Tcl_NumUtfChars(string, length);
    if (TclGetIntForIndexM(interp, objv[2], numChars-1, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    string = TclGetStringFromObj(objv[1], &length);
    if (index < 0) {
	index = 0;
    }
    if (index < numChars) {
	p = Tcl_UtfAtIndex(string, index);
	end = string+length;
	for (cur = index; p < end; cur++) {
	    p += TclUtfToUniChar(p, &ch);
	    if (!Tcl_UniCharIsWordChar(ch)) {
		break;
	    }
	}
	if (cur == index) {
	    cur++;
	}
    } else {
	cur = numChars;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(cur));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringEqualCmd --
 *
 *	This procedure is invoked to process the "string equal" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringEqualCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    /*
     * Remember to keep code here in some sync with the byte-compiled versions
     * in tclExecute.c (INST_STR_EQ, INST_STR_NEQ and INST_STR_CMP as well as
     * the expr string comparison in INST_EQ/INST_NEQ/INST_LT/...).
     */

    const char *string2;
    int length2, i, match, nocase = 0, reqlength = -1;

    if (objc < 3 || objc > 6) {
    str_cmp_args:
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-nocase? ?-length int? string1 string2");
	return TCL_ERROR;
    }

    for (i = 1; i < objc-2; i++) {
	string2 = TclGetStringFromObj(objv[i], &length2);
	if ((length2 > 1) && !strncmp(string2, "-nocase", length2)) {
	    nocase = 1;
	} else if ((length2 > 1)
		&& !strncmp(string2, "-length", length2)) {
	    if (i+1 >= objc-2) {
		goto str_cmp_args;
	    }
	    i++;
	    if (TclGetIntFromObj(interp, objv[i], &reqlength) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad option \"%s\": must be -nocase or -length",
		    string2));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "INDEX", "option",
		    string2, NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * From now on, we only access the two objects at the end of the argument
     * array.
     */

    objv += objc-2;
    match = TclStringCmp(objv[0], objv[1], 0, nocase, reqlength);
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(match ? 0 : 1));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringCmpCmd --
 *
 *	This procedure is invoked to process the "string compare" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringCmpCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    /*
     * Remember to keep code here in some sync with the byte-compiled versions
     * in tclExecute.c (INST_STR_EQ, INST_STR_NEQ and INST_STR_CMP as well as
     * the expr string comparison in INST_EQ/INST_NEQ/INST_LT/...).
     */

    int match, nocase, reqlength, status;

    status = TclStringCmpOpts(interp, objc, objv, &nocase, &reqlength);
    if (status != TCL_OK) {
	return status;
    }

    objv += objc-2;
    match = TclStringCmp(objv[0], objv[1], 0, nocase, reqlength);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(match));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclStringCmp --
 *
 *	This is the core of Tcl's string comparison. It only handles byte
 *	arrays, UNICODE strings and UTF-8 strings correctly.
 *
 * Results:
 *	-1 if value1Ptr is less than value2Ptr, 0 if they are equal, or 1 if
 *	value1Ptr is greater.
 *
 * Side effects:
 *	May cause string representations of objects to be allocated.
 *
 *----------------------------------------------------------------------
 */

int
TclStringCmp(
    Tcl_Obj *value1Ptr,
    Tcl_Obj *value2Ptr,
    int checkEq,		/* comparison is only for equality */
    int nocase,			/* comparison is not case sensitive */
    int reqlength)		/* requested length; -1 to compare whole
				 * strings */
{
    const char *s1, *s2;
    int empty, length, match, s1len, s2len;
    memCmpFn_t memCmpFn;

    if ((reqlength == 0) || (value1Ptr == value2Ptr)) {
	/*
	 * Always match at 0 chars or if it is the same obj.
	 */
	return 0;
    }

    if (!nocase && TclIsPureByteArray(value1Ptr)
	    && TclIsPureByteArray(value2Ptr)) {
	/*
	 * Use binary versions of comparisons since that won't cause undue
	 * type conversions and it is much faster. Only do this if we're
	 * case-sensitive (which is all that really makes sense with byte
	 * arrays anyway, and we have no memcasecmp() for some reason... :^)
	 */

	s1 = (char *) Tcl_GetByteArrayFromObj(value1Ptr, &s1len);
	s2 = (char *) Tcl_GetByteArrayFromObj(value2Ptr, &s2len);
	memCmpFn = memcmp;
    } else if ((value1Ptr->typePtr == &tclStringType)
	    && (value2Ptr->typePtr == &tclStringType)) {
	/*
	 * Do a unicode-specific comparison if both of the args are of String
	 * type. If the char length == byte length, we can do a memcmp. In
	 * benchmark testing this proved the most efficient check between the
	 * unicode and string comparison operations.
	 */

	if (nocase) {
	    s1 = (char *) Tcl_GetUnicodeFromObj(value1Ptr, &s1len);
	    s2 = (char *) Tcl_GetUnicodeFromObj(value2Ptr, &s2len);
	    memCmpFn = (memCmpFn_t)Tcl_UniCharNcasecmp;
	} else {
	    s1len = Tcl_GetCharLength(value1Ptr);
	    s2len = Tcl_GetCharLength(value2Ptr);
	    if ((s1len == value1Ptr->length)
		    && (value1Ptr->bytes != NULL)
		    && (s2len == value2Ptr->length)
		    && (value2Ptr->bytes != NULL)) {
		s1 = value1Ptr->bytes;
		s2 = value2Ptr->bytes;
		memCmpFn = memcmp;
	    } else {
		s1 = (char *) Tcl_GetUnicode(value1Ptr);
		s2 = (char *) Tcl_GetUnicode(value2Ptr);
		if (
#ifdef WORDS_BIGENDIAN
			1
#else
			checkEq
#endif /* WORDS_BIGENDIAN */
		        ) {
		    memCmpFn = memcmp;
		    s1len *= sizeof(Tcl_UniChar);
		    s2len *= sizeof(Tcl_UniChar);
		} else {
		    memCmpFn = (memCmpFn_t) Tcl_UniCharNcmp;
		}
	    }
	}
    } else {
	/*
	 * Get the string representations, being careful in case we have
	 * special empty string objects about.
	 */

	empty = TclCheckEmptyString(value1Ptr);
	if (empty > 0) {
	    switch (TclCheckEmptyString(value2Ptr)) {
	    case -1:
		s1 = "";
		s1len = 0;
		s2 = TclGetStringFromObj(value2Ptr, &s2len);
		break;
	    case 0:
		return -1;
	    default: /* avoid warn: `s2` may be used uninitialized */
		return 0;
	    }
	} else if (TclCheckEmptyString(value2Ptr) > 0) {
	    switch (empty) {
	    case -1:
		s2 = "";
		s2len = 0;
		s1 = TclGetStringFromObj(value1Ptr, &s1len);
		break;
	    case 0:
		return 1;
	    default: /* avoid warn: `s1` may be used uninitialized */
		return 0;
	    }
	} else {
	    s1 = TclGetStringFromObj(value1Ptr, &s1len);
	    s2 = TclGetStringFromObj(value2Ptr, &s2len);
	}

	if (!nocase && checkEq) {
	    /*
	     * When we have equal-length we can check only for (in)equality.
	     * We can use memcmp() in all (n)eq cases because we don't need to
	     * worry about lexical LE/BE variance.
	     */
	    memCmpFn = memcmp;
	} else {
	    /*
	     * As a catch-all we will work with UTF-8. We cannot use memcmp()
	     * as that is unsafe with any string containing NUL (\xC0\x80 in
	     * Tcl's utf rep). We can use the more efficient TclpUtfNcmp2 if
	     * we are case-sensitive and no specific length was requested.
	     */

	    if ((reqlength < 0) && !nocase) {
		memCmpFn = (memCmpFn_t) TclpUtfNcmp2;
	    } else {
		s1len = Tcl_NumUtfChars(s1, s1len);
		s2len = Tcl_NumUtfChars(s2, s2len);
		memCmpFn = (memCmpFn_t)
			(nocase ? Tcl_UtfNcasecmp : Tcl_UtfNcmp);
	    }
	}
    }

    length = (s1len < s2len) ? s1len : s2len;
    if (reqlength > 0 && reqlength < length) {
	length = reqlength;
    } else if (reqlength < 0) {
	/*
	 * The requested length is negative, so we ignore it by setting it to
	 * length + 1 so we correct the match var.
	 */

	reqlength = length + 1;
    }

    if (checkEq && (s1len != s2len)) {
	match = 1;		/* This will be reversed below. */
    }  else {
	/*
	 * The comparison function should compare up to the minimum byte
	 * length only.
	 */
	match = memCmpFn(s1, s2, (size_t) length);
    }
    if ((match == 0) && (reqlength > length)) {
	match = s1len - s2len;
    }
    return (match > 0) ? 1 : (match < 0) ? -1 : 0;
}

int TclStringCmpOpts(
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[],	/* Argument objects. */
    int *nocase,
    int *reqlength)
{
    int i, length;
    const char *string;

    *reqlength = -1;
    *nocase = 0;
    if (objc < 3 || objc > 6) {
    str_cmp_args:
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-nocase? ?-length int? string1 string2");
	return TCL_ERROR;
    }

    for (i = 1; i < objc-2; i++) {
	string = TclGetStringFromObj(objv[i], &length);
	if ((length > 1) && !strncmp(string, "-nocase", length)) {
	    *nocase = 1;
	} else if ((length > 1)
		&& !strncmp(string, "-length", length)) {
	    if (i+1 >= objc-2) {
		goto str_cmp_args;
	    }
	    i++;
	    if (TclGetIntFromObj(interp, objv[i], reqlength) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad option \"%s\": must be -nocase or -length",
		    string));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "INDEX", "option",
		    string, NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringCatCmd --
 *
 *	This procedure is invoked to process the "string cat" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringCatCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int i;
    Tcl_Obj *objResultPtr;

    if (objc < 2) {
	/*
	 * If there are no args, the result is an empty object.
	 * Just leave the preset empty interp result.
	 */
	return TCL_OK;
    }
    if (objc == 2) {
	/*
	 * Other trivial case, single arg, just return it.
	 */
	Tcl_SetObjResult(interp, objv[1]);
	return TCL_OK;
    }
    objResultPtr = objv[1];
    if (Tcl_IsShared(objResultPtr)) {
	objResultPtr = Tcl_DuplicateObj(objResultPtr);
    }
    for(i = 2;i < objc;i++) {
	Tcl_AppendObjToObj(objResultPtr, objv[i]);
    }
    Tcl_SetObjResult(interp, objResultPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringBytesCmd --
 *
 *	This procedure is invoked to process the "string bytelength" Tcl
 *	command. See the user documentation for details on what it does. Note
 *	that this command only functions correctly on properly formed Tcl UTF
 *	strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringBytesCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int length;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "string");
	return TCL_ERROR;
    }

    (void) TclGetStringFromObj(objv[1], &length);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(length));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringLenCmd --
 *
 *	This procedure is invoked to process the "string length" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringLenCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "string");
	return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(Tcl_GetCharLength(objv[1])));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringLowerCmd --
 *
 *	This procedure is invoked to process the "string tolower" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringLowerCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int length1, length2;
    const char *string1;
    char *string2;

    if (objc < 2 || objc > 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "string ?first? ?last?");
	return TCL_ERROR;
    }

    string1 = TclGetStringFromObj(objv[1], &length1);

    if (objc == 2) {
	Tcl_Obj *resultPtr = Tcl_NewStringObj(string1, length1);

	length1 = Tcl_UtfToLower(TclGetString(resultPtr));
	Tcl_SetObjLength(resultPtr, length1);
	Tcl_SetObjResult(interp, resultPtr);
    } else {
	int first, last;
	const char *start, *end;
	Tcl_Obj *resultPtr;

	length1 = Tcl_NumUtfChars(string1, length1) - 1;
	if (TclGetIntForIndexM(interp,objv[2],length1, &first) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (first < 0) {
	    first = 0;
	}
	last = first;

	if ((objc == 4) && (TclGetIntForIndexM(interp, objv[3], length1,
		&last) != TCL_OK)) {
	    return TCL_ERROR;
	}

	if (last >= length1) {
	    last = length1;
	}
	if (last < first) {
	    Tcl_SetObjResult(interp, objv[1]);
	    return TCL_OK;
	}

	string1 = TclGetStringFromObj(objv[1], &length1);
	start = Tcl_UtfAtIndex(string1, first);
	end = Tcl_UtfAtIndex(start, last - first + 1);
	resultPtr = Tcl_NewStringObj(string1, end - string1);
	string2 = TclGetString(resultPtr) + (start - string1);

	length2 = Tcl_UtfToLower(string2);
	Tcl_SetObjLength(resultPtr, length2 + (start - string1));

	Tcl_AppendToObj(resultPtr, end, -1);
	Tcl_SetObjResult(interp, resultPtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringUpperCmd --
 *
 *	This procedure is invoked to process the "string toupper" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringUpperCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int length1, length2;
    const char *string1;
    char *string2;

    if (objc < 2 || objc > 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "string ?first? ?last?");
	return TCL_ERROR;
    }

    string1 = TclGetStringFromObj(objv[1], &length1);

    if (objc == 2) {
	Tcl_Obj *resultPtr = Tcl_NewStringObj(string1, length1);

	length1 = Tcl_UtfToUpper(TclGetString(resultPtr));
	Tcl_SetObjLength(resultPtr, length1);
	Tcl_SetObjResult(interp, resultPtr);
    } else {
	int first, last;
	const char *start, *end;
	Tcl_Obj *resultPtr;

	length1 = Tcl_NumUtfChars(string1, length1) - 1;
	if (TclGetIntForIndexM(interp,objv[2],length1, &first) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (first < 0) {
	    first = 0;
	}
	last = first;

	if ((objc == 4) && (TclGetIntForIndexM(interp, objv[3], length1,
		&last) != TCL_OK)) {
	    return TCL_ERROR;
	}

	if (last >= length1) {
	    last = length1;
	}
	if (last < first) {
	    Tcl_SetObjResult(interp, objv[1]);
	    return TCL_OK;
	}

	string1 = TclGetStringFromObj(objv[1], &length1);
	start = Tcl_UtfAtIndex(string1, first);
	end = Tcl_UtfAtIndex(start, last - first + 1);
	resultPtr = Tcl_NewStringObj(string1, end - string1);
	string2 = TclGetString(resultPtr) + (start - string1);

	length2 = Tcl_UtfToUpper(string2);
	Tcl_SetObjLength(resultPtr, length2 + (start - string1));

	Tcl_AppendToObj(resultPtr, end, -1);
	Tcl_SetObjResult(interp, resultPtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringTitleCmd --
 *
 *	This procedure is invoked to process the "string totitle" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringTitleCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int length1, length2;
    const char *string1;
    char *string2;

    if (objc < 2 || objc > 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "string ?first? ?last?");
	return TCL_ERROR;
    }

    string1 = TclGetStringFromObj(objv[1], &length1);

    if (objc == 2) {
	Tcl_Obj *resultPtr = Tcl_NewStringObj(string1, length1);

	length1 = Tcl_UtfToTitle(TclGetString(resultPtr));
	Tcl_SetObjLength(resultPtr, length1);
	Tcl_SetObjResult(interp, resultPtr);
    } else {
	int first, last;
	const char *start, *end;
	Tcl_Obj *resultPtr;

	length1 = Tcl_NumUtfChars(string1, length1) - 1;
	if (TclGetIntForIndexM(interp,objv[2],length1, &first) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (first < 0) {
	    first = 0;
	}
	last = first;

	if ((objc == 4) && (TclGetIntForIndexM(interp, objv[3], length1,
		&last) != TCL_OK)) {
	    return TCL_ERROR;
	}

	if (last >= length1) {
	    last = length1;
	}
	if (last < first) {
	    Tcl_SetObjResult(interp, objv[1]);
	    return TCL_OK;
	}

	string1 = TclGetStringFromObj(objv[1], &length1);
	start = Tcl_UtfAtIndex(string1, first);
	end = Tcl_UtfAtIndex(start, last - first + 1);
	resultPtr = Tcl_NewStringObj(string1, end - string1);
	string2 = TclGetString(resultPtr) + (start - string1);

	length2 = Tcl_UtfToTitle(string2);
	Tcl_SetObjLength(resultPtr, length2 + (start - string1));

	Tcl_AppendToObj(resultPtr, end, -1);
	Tcl_SetObjResult(interp, resultPtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringTrimCmd --
 *
 *	This procedure is invoked to process the "string trim" Tcl command.
 *	See the user documentation for details on what it does. Note that this
 *	command only functions correctly on properly formed Tcl UTF strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringTrimCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *string1, *string2;
    int triml, trimr, length1, length2;

    if (objc == 3) {
	string2 = TclGetStringFromObj(objv[2], &length2);
    } else if (objc == 2) {
	string2 = tclDefaultTrimSet;
	length2 = strlen(tclDefaultTrimSet);
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "string ?chars?");
	return TCL_ERROR;
    }
    string1 = TclGetStringFromObj(objv[1], &length1);

    triml = TclTrim(string1, length1, string2, length2, &trimr);

    Tcl_SetObjResult(interp,
	    Tcl_NewStringObj(string1 + triml, length1 - triml - trimr));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringTrimLCmd --
 *
 *	This procedure is invoked to process the "string trimleft" Tcl
 *	command. See the user documentation for details on what it does. Note
 *	that this command only functions correctly on properly formed Tcl UTF
 *	strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringTrimLCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *string1, *string2;
    int trim, length1, length2;

    if (objc == 3) {
	string2 = TclGetStringFromObj(objv[2], &length2);
    } else if (objc == 2) {
	string2 = tclDefaultTrimSet;
	length2 = strlen(tclDefaultTrimSet);
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "string ?chars?");
	return TCL_ERROR;
    }
    string1 = TclGetStringFromObj(objv[1], &length1);

    trim = TclTrimLeft(string1, length1, string2, length2);

    Tcl_SetObjResult(interp, Tcl_NewStringObj(string1+trim, length1-trim));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StringTrimRCmd --
 *
 *	This procedure is invoked to process the "string trimright" Tcl
 *	command. See the user documentation for details on what it does. Note
 *	that this command only functions correctly on properly formed Tcl UTF
 *	strings.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
StringTrimRCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    const char *string1, *string2;
    int trim, length1, length2;

    if (objc == 3) {
	string2 = TclGetStringFromObj(objv[2], &length2);
    } else if (objc == 2) {
	string2 = tclDefaultTrimSet;
	length2 = strlen(tclDefaultTrimSet);
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "string ?chars?");
	return TCL_ERROR;
    }
    string1 = TclGetStringFromObj(objv[1], &length1);

    trim = TclTrimRight(string1, length1, string2, length2);

    Tcl_SetObjResult(interp, Tcl_NewStringObj(string1, length1-trim));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitStringCmd --
 *
 *	This procedure creates the "string" Tcl command. See the user
 *	documentation for details on what it does. Note that this command only
 *	functions correctly on properly formed Tcl UTF strings.
 *
 *	Also note that the primary methods here (equal, compare, match, ...)
 *	have bytecode equivalents. You will find the code for those in
 *	tclExecute.c. The code here will only be used in the non-bc case (like
 *	in an 'eval').
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
TclInitStringCmd(
    Tcl_Interp *interp)		/* Current interpreter. */
{
    static const EnsembleImplMap stringImplMap[] = {
	{"bytelength",	StringBytesCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
	{"cat",		StringCatCmd,	TclCompileStringCatCmd, NULL, NULL, 0},
	{"compare",	StringCmpCmd,	TclCompileStringCmpCmd, NULL, NULL, 0},
	{"equal",	StringEqualCmd,	TclCompileStringEqualCmd, NULL, NULL, 0},
	{"first",	StringFirstCmd,	TclCompileStringFirstCmd, NULL, NULL, 0},
	{"index",	StringIndexCmd,	TclCompileStringIndexCmd, NULL, NULL, 0},
	{"is",		StringIsCmd,	TclCompileStringIsCmd, NULL, NULL, 0},
	{"last",	StringLastCmd,	TclCompileStringLastCmd, NULL, NULL, 0},
	{"length",	StringLenCmd,	TclCompileStringLenCmd, NULL, NULL, 0},
	{"map",		StringMapCmd,	TclCompileStringMapCmd, NULL, NULL, 0},
	{"match",	StringMatchCmd,	TclCompileStringMatchCmd, NULL, NULL, 0},
	{"range",	StringRangeCmd,	TclCompileStringRangeCmd, NULL, NULL, 0},
	{"repeat",	StringReptCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},
	{"replace",	StringRplcCmd,	TclCompileStringReplaceCmd, NULL, NULL, 0},
	{"reverse",	StringRevCmd,	TclCompileBasic1ArgCmd, NULL, NULL, 0},
	{"tolower",	StringLowerCmd,	TclCompileStringToLowerCmd, NULL, NULL, 0},
	{"toupper",	StringUpperCmd,	TclCompileStringToUpperCmd, NULL, NULL, 0},
	{"totitle",	StringTitleCmd,	TclCompileStringToTitleCmd, NULL, NULL, 0},
	{"trim",	StringTrimCmd,	TclCompileStringTrimCmd, NULL, NULL, 0},
	{"trimleft",	StringTrimLCmd,	TclCompileStringTrimLCmd, NULL, NULL, 0},
	{"trimright",	StringTrimRCmd,	TclCompileStringTrimRCmd, NULL, NULL, 0},
	{"wordend",	StringEndCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},
	{"wordstart",	StringStartCmd,	TclCompileBasic2ArgCmd, NULL, NULL, 0},
	{NULL, NULL, NULL, NULL, NULL, 0}
    };

    return TclMakeEnsemble(interp, "string", stringImplMap);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SubstObjCmd --
 *
 *	This procedure is invoked to process the "subst" Tcl command. See the
 *	user documentation for details on what it does. This command relies on
 *	Tcl_SubstObj() for its implementation.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
TclSubstOptions(
    Tcl_Interp *interp,
    int numOpts,
    Tcl_Obj *const opts[],
    int *flagPtr)
{
    static const char *const substOptions[] = {
	"-nobackslashes", "-nocommands", "-novariables", NULL
    };
    enum {
	SUBST_NOBACKSLASHES, SUBST_NOCOMMANDS, SUBST_NOVARS
    };
    int i, flags = TCL_SUBST_ALL;

    for (i = 0; i < numOpts; i++) {
	int optionIndex;

	if (Tcl_GetIndexFromObj(interp, opts[i], substOptions, "option", 0,
		&optionIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch (optionIndex) {
	case SUBST_NOBACKSLASHES:
	    flags &= ~TCL_SUBST_BACKSLASHES;
	    break;
	case SUBST_NOCOMMANDS:
	    flags &= ~TCL_SUBST_COMMANDS;
	    break;
	case SUBST_NOVARS:
	    flags &= ~TCL_SUBST_VARIABLES;
	    break;
	default:
	    Tcl_Panic("Tcl_SubstObjCmd: bad option index to SubstOptions");
	}
    }
    *flagPtr = flags;
    return TCL_OK;
}

int
Tcl_SubstObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return Tcl_NRCallObjProc(interp, TclNRSubstObjCmd, dummy, objc, objv);
}

int
TclNRSubstObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int flags;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-nobackslashes? ?-nocommands? ?-novariables? string");
	return TCL_ERROR;
    }

    if (TclSubstOptions(interp, objc-2, objv+1, &flags) != TCL_OK) {
	return TCL_ERROR;
    }
    return Tcl_NRSubstObj(interp, objv[objc-1], flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SwitchObjCmd --
 *
 *	This object-based procedure is invoked to process the "switch" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SwitchObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return Tcl_NRCallObjProc(interp, TclNRSwitchObjCmd, dummy, objc, objv);
}
int
TclNRSwitchObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int i,j, index, mode, foundmode, splitObjs, numMatchesSaved;
    int noCase, patternLength;
    const char *pattern;
    Tcl_Obj *stringObj, *indexVarObj, *matchVarObj;
    Tcl_Obj *const *savedObjv = objv;
    Tcl_RegExp regExpr = NULL;
    Interp *iPtr = (Interp *) interp;
    int pc = 0;
    int bidx = 0;		/* Index of body argument. */
    Tcl_Obj *blist = NULL;	/* List obj which is the body */
    CmdFrame *ctxPtr;		/* Copy of the topmost cmdframe, to allow us
				 * to mess with the line information */

    /*
     * If you add options that make -e and -g not unique prefixes of -exact or
     * -glob, you *must* fix TclCompileSwitchCmd's option parser as well.
     */

    static const char *const options[] = {
	"-exact", "-glob", "-indexvar", "-matchvar", "-nocase", "-regexp",
	"--", NULL
    };
    enum options {
	OPT_EXACT, OPT_GLOB, OPT_INDEXV, OPT_MATCHV, OPT_NOCASE, OPT_REGEXP,
	OPT_LAST
    };
    typedef int (*strCmpFn_t)(const char *, const char *);
    strCmpFn_t strCmpFn = strcmp;

    mode = OPT_EXACT;
    foundmode = 0;
    indexVarObj = NULL;
    matchVarObj = NULL;
    numMatchesSaved = 0;
    noCase = 0;
    for (i = 1; i < objc-2; i++) {
	if (TclGetString(objv[i])[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum options) index) {
	    /*
	     * General options.
	     */

	case OPT_LAST:
	    i++;
	    goto finishedOptions;
	case OPT_NOCASE:
	    strCmpFn = TclUtfCasecmp;
	    noCase = 1;
	    break;

	    /*
	     * Handle the different switch mode options.
	     */

	default:
	    if (foundmode) {
		/*
		 * Mode already set via -exact, -glob, or -regexp.
		 */

		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"bad option \"%s\": %s option already found",
			TclGetString(objv[i]), options[mode]));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "SWITCH",
			"DOUBLEOPT", NULL);
		return TCL_ERROR;
	    }
	    foundmode = 1;
	    mode = index;
	    break;

	    /*
	     * Check for TIP#75 options specifying the variables to write
	     * regexp information into.
	     */

	case OPT_INDEXV:
	    i++;
	    if (i >= objc-2) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"missing variable name argument to %s option",
			"-indexvar"));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "SWITCH",
			"NOVAR", NULL);
		return TCL_ERROR;
	    }
	    indexVarObj = objv[i];
	    numMatchesSaved = -1;
	    break;
	case OPT_MATCHV:
	    i++;
	    if (i >= objc-2) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"missing variable name argument to %s option",
			"-matchvar"));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "SWITCH",
			"NOVAR", NULL);
		return TCL_ERROR;
	    }
	    matchVarObj = objv[i];
	    numMatchesSaved = -1;
	    break;
	}
    }

  finishedOptions:
    if (objc - i < 2) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-option ...? string ?pattern body ...? ?default body?");
	return TCL_ERROR;
    }
    if (indexVarObj != NULL && mode != OPT_REGEXP) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"%s option requires -regexp option", "-indexvar"));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "SWITCH",
		"MODERESTRICTION", NULL);
	return TCL_ERROR;
    }
    if (matchVarObj != NULL && mode != OPT_REGEXP) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"%s option requires -regexp option", "-matchvar"));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "SWITCH",
		"MODERESTRICTION", NULL);
	return TCL_ERROR;
    }

    stringObj = objv[i];
    objc -= i + 1;
    objv += i + 1;
    bidx = i + 1;		/* First after the match string. */

    /*
     * If all of the pattern/command pairs are lumped into a single argument,
     * split them out again.
     *
     * TIP #280: Determine the lines the words in the list start at, based on
     * the same data for the list word itself. The cmdFramePtr line
     * information is manipulated directly.
     */

    splitObjs = 0;
    if (objc == 1) {
	Tcl_Obj **listv;

	blist = objv[0];
	if (TclListObjGetElements(interp, objv[0], &objc, &listv) != TCL_OK){
	    return TCL_ERROR;
	}

	/*
	 * Ensure that the list is non-empty.
	 */

	if (objc < 1) {
	    Tcl_WrongNumArgs(interp, 1, savedObjv,
		    "?-option ...? string {?pattern body ...? ?default body?}");
	    return TCL_ERROR;
	}
	objv = listv;
	splitObjs = 1;
    }

    /*
     * Complain if there is an odd number of words in the list of patterns and
     * bodies.
     */

    if (objc % 2) {
	Tcl_ResetResult(interp);
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"extra switch pattern with no body", -1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "SWITCH", "BADARM",
		NULL);

	/*
	 * Check if this can be due to a badly placed comment in the switch
	 * block.
	 *
	 * The following is an heuristic to detect the infamous "comment in
	 * switch" error: just check if a pattern begins with '#'.
	 */

	if (splitObjs) {
	    for (i=0 ; i<objc ; i+=2) {
		if (TclGetString(objv[i])[0] == '#') {
		    Tcl_AppendToObj(Tcl_GetObjResult(interp),
			    ", this may be due to a comment incorrectly"
			    " placed outside of a switch body - see the"
			    " \"switch\" documentation", -1);
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "SWITCH",
			    "BADARM", "COMMENT?", NULL);
		    break;
		}
	    }
	}

	return TCL_ERROR;
    }

    /*
     * Complain if the last body is a continuation. Note that this check
     * assumes that the list is non-empty!
     */

    if (strcmp(TclGetString(objv[objc-1]), "-") == 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"no body specified for pattern \"%s\"",
		TclGetString(objv[objc-2])));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "SWITCH", "BADARM",
		"FALLTHROUGH", NULL);
	return TCL_ERROR;
    }

    for (i = 0; i < objc; i += 2) {
	/*
	 * See if the pattern matches the string.
	 */

	pattern = TclGetStringFromObj(objv[i], &patternLength);

	if ((i == objc - 2) && (*pattern == 'd')
		&& (strcmp(pattern, "default") == 0)) {
	    Tcl_Obj *emptyObj = NULL;

	    /*
	     * If either indexVarObj or matchVarObj are non-NULL, we're in
	     * REGEXP mode but have reached the default clause anyway. TIP#75
	     * specifies that we set the variables to empty lists (== empty
	     * objects) in that case.
	     */

	    if (indexVarObj != NULL) {
		TclNewObj(emptyObj);
		if (Tcl_ObjSetVar2(interp, indexVarObj, NULL, emptyObj,
			TCL_LEAVE_ERR_MSG) == NULL) {
		    return TCL_ERROR;
		}
	    }
	    if (matchVarObj != NULL) {
		if (emptyObj == NULL) {
		    TclNewObj(emptyObj);
		}
		if (Tcl_ObjSetVar2(interp, matchVarObj, NULL, emptyObj,
			TCL_LEAVE_ERR_MSG) == NULL) {
		    return TCL_ERROR;
		}
	    }
	    goto matchFound;
	}

	switch (mode) {
	case OPT_EXACT:
	    if (strCmpFn(TclGetString(stringObj), pattern) == 0) {
		goto matchFound;
	    }
	    break;
	case OPT_GLOB:
	    if (Tcl_StringCaseMatch(TclGetString(stringObj),pattern,noCase)) {
		goto matchFound;
	    }
	    break;
	case OPT_REGEXP:
	    regExpr = Tcl_GetRegExpFromObj(interp, objv[i],
		    TCL_REG_ADVANCED | (noCase ? TCL_REG_NOCASE : 0));
	    if (regExpr == NULL) {
		return TCL_ERROR;
	    } else {
		int matched = Tcl_RegExpExecObj(interp, regExpr, stringObj, 0,
			numMatchesSaved, 0);

		if (matched < 0) {
		    return TCL_ERROR;
		} else if (matched) {
		    goto matchFoundRegexp;
		}
	    }
	    break;
	}
    }
    return TCL_OK;

  matchFoundRegexp:
    /*
     * We are operating in REGEXP mode and we need to store information about
     * what we matched in some user-nominated arrays. So build the lists of
     * values and indices to write here. [TIP#75]
     */

    if (numMatchesSaved) {
	Tcl_RegExpInfo info;
	Tcl_Obj *matchesObj, *indicesObj = NULL;

	Tcl_RegExpGetInfo(regExpr, &info);
	if (matchVarObj != NULL) {
	    TclNewObj(matchesObj);
	} else {
	    matchesObj = NULL;
	}
	if (indexVarObj != NULL) {
	    TclNewObj(indicesObj);
	}

	for (j=0 ; j<=info.nsubs ; j++) {
	    if (indexVarObj != NULL) {
		Tcl_Obj *rangeObjAry[2];

		if (info.matches[j].end > 0) {
		    rangeObjAry[0] = Tcl_NewLongObj(info.matches[j].start);
		    rangeObjAry[1] = Tcl_NewLongObj(info.matches[j].end-1);
		} else {
		    rangeObjAry[0] = rangeObjAry[1] = Tcl_NewIntObj(-1);
		}

		/*
		 * Never fails; the object is always clean at this point.
		 */

		Tcl_ListObjAppendElement(NULL, indicesObj,
			Tcl_NewListObj(2, rangeObjAry));
	    }

	    if (matchVarObj != NULL) {
		Tcl_Obj *substringObj;

		substringObj = Tcl_GetRange(stringObj,
			info.matches[j].start, info.matches[j].end-1);

		/*
		 * Never fails; the object is always clean at this point.
		 */

		Tcl_ListObjAppendElement(NULL, matchesObj, substringObj);
	    }
	}

	if (indexVarObj != NULL) {
	    if (Tcl_ObjSetVar2(interp, indexVarObj, NULL, indicesObj,
		    TCL_LEAVE_ERR_MSG) == NULL) {
		/*
		 * Careful! Check to see if we have allocated the list of
		 * matched strings; if so (but there was an error assigning
		 * the indices list) we have a potential memory leak because
		 * the match list has not been written to a variable. Except
		 * that we'll clean that up right now.
		 */

		if (matchesObj != NULL) {
		    Tcl_DecrRefCount(matchesObj);
		}
		return TCL_ERROR;
	    }
	}
	if (matchVarObj != NULL) {
	    if (Tcl_ObjSetVar2(interp, matchVarObj, NULL, matchesObj,
		    TCL_LEAVE_ERR_MSG) == NULL) {
		/*
		 * Unlike above, if indicesObj is non-NULL at this point, it
		 * will have been written to a variable already and will hence
		 * not be leaked.
		 */

		return TCL_ERROR;
	    }
	}
    }

    /*
     * We've got a match. Find a body to execute, skipping bodies that are
     * "-".
     */

  matchFound:
    ctxPtr = TclStackAlloc(interp, sizeof(CmdFrame));
    *ctxPtr = *iPtr->cmdFramePtr;

    if (splitObjs) {
	/*
	 * We have to perform the GetSrc and other type dependent handling of
	 * the frame here because we are munging with the line numbers,
	 * something the other commands like if, etc. are not doing. Them are
	 * fine with simply passing the CmdFrame through and having the
	 * special handling done in 'info frame', or the bc compiler
	 */

	if (ctxPtr->type == TCL_LOCATION_BC) {
	    /*
	     * Type BC => ctxPtr->data.eval.path    is not used.
	     *		  ctxPtr->data.tebc.codePtr is used instead.
	     */

	    TclGetSrcInfoForPc(ctxPtr);
	    pc = 1;

	    /*
	     * The line information in the cmdFrame is now a copy we do not
	     * own.
	     */
	}

	if (ctxPtr->type == TCL_LOCATION_SOURCE && ctxPtr->line[bidx] >= 0) {
	    int bline = ctxPtr->line[bidx];

	    ctxPtr->line = ckalloc(objc * sizeof(int));
	    ctxPtr->nline = objc;
	    TclListLines(blist, bline, objc, ctxPtr->line, objv);
	} else {
	    /*
	     * This is either a dynamic code word, when all elements are
	     * relative to themselves, or something else less expected and
	     * where we have no information. The result is the same in both
	     * cases; tell the code to come that it doesn't know where it is,
	     * which triggers reversion to the old behavior.
	     */

	    int k;

	    ctxPtr->line = ckalloc(objc * sizeof(int));
	    ctxPtr->nline = objc;
	    for (k=0; k < objc; k++) {
		ctxPtr->line[k] = -1;
	    }
	}
    }

    for (j = i + 1; ; j += 2) {
	if (j >= objc) {
	    /*
	     * This shouldn't happen since we've checked that the last body is
	     * not a continuation...
	     */

	    Tcl_Panic("fall-out when searching for body to match pattern");
	}
	if (strcmp(TclGetString(objv[j]), "-") != 0) {
	    break;
	}
    }

    /*
     * TIP #280: Make invoking context available to switch branch.
     */

    Tcl_NRAddCallback(interp, SwitchPostProc, INT2PTR(splitObjs), ctxPtr,
	    INT2PTR(pc), (ClientData) pattern);
    return TclNREvalObjEx(interp, objv[j], 0, ctxPtr, splitObjs ? j : bidx+j);
}

static int
SwitchPostProc(
    ClientData data[],		/* Data passed from Tcl_NRAddCallback above */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int result)			/* Result to return*/
{
    /* Unpack the preserved data */

    int splitObjs = PTR2INT(data[0]);
    CmdFrame *ctxPtr = data[1];
    int pc = PTR2INT(data[2]);
    const char *pattern = data[3];
    int patternLength = strlen(pattern);

    /*
     * Clean up TIP 280 context information
     */

    if (splitObjs) {
	ckfree(ctxPtr->line);
	if (pc && (ctxPtr->type == TCL_LOCATION_SOURCE)) {
	    /*
	     * Death of SrcInfo reference.
	     */

	    Tcl_DecrRefCount(ctxPtr->data.eval.path);
	}
    }

    /*
     * Generate an error message if necessary.
     */

    if (result == TCL_ERROR) {
	int limit = 50;
	int overflow = (patternLength > limit);

	Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		"\n    (\"%.*s%s\" arm line %d)",
		(overflow ? limit : patternLength), pattern,
		(overflow ? "..." : ""), Tcl_GetErrorLine(interp)));
    }
    TclStackFree(interp, ctxPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ThrowObjCmd --
 *
 *	This procedure is invoked to process the "throw" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_ThrowObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Obj *options;
    int len;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "type message");
	return TCL_ERROR;
    }

    /*
     * The type must be a list of at least length 1.
     */

    if (Tcl_ListObjLength(interp, objv[1], &len) != TCL_OK) {
	return TCL_ERROR;
    } else if (len < 1) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"type must be non-empty list", -1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "THROW", "BADEXCEPTION",
		NULL);
	return TCL_ERROR;
    }

    /*
     * Now prepare the result options dictionary. We use the list API as it is
     * slightly more convenient.
     */

    TclNewLiteralStringObj(options, "-code error -level 0 -errorcode");
    Tcl_ListObjAppendElement(NULL, options, objv[1]);

    /*
     * We're ready to go. Fire things into the low-level result machinery.
     */

    Tcl_SetObjResult(interp, objv[2]);
    return Tcl_SetReturnOptions(interp, options);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TimeObjCmd --
 *
 *	This object-based procedure is invoked to process the "time" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_TimeObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    register Tcl_Obj *objPtr;
    Tcl_Obj *objs[4];
    register int i, result;
    int count;
    double totalMicroSec;
#ifndef TCL_WIDE_CLICKS
    Tcl_Time start, stop;
#else
    Tcl_WideInt start, stop;
#endif

    if (objc == 2) {
	count = 1;
    } else if (objc == 3) {
	result = TclGetIntFromObj(interp, objv[2], &count);
	if (result != TCL_OK) {
	    return result;
	}
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "command ?count?");
	return TCL_ERROR;
    }

    objPtr = objv[1];
    i = count;
#ifndef TCL_WIDE_CLICKS
    Tcl_GetTime(&start);
#else
    start = TclpGetWideClicks();
#endif
    while (i-- > 0) {
	result = TclEvalObjEx(interp, objPtr, 0, NULL, 0);
	if (result != TCL_OK) {
	    return result;
	}
    }
#ifndef TCL_WIDE_CLICKS
    Tcl_GetTime(&stop);
    totalMicroSec = ((double) (stop.sec - start.sec)) * 1.0e6
	    + (stop.usec - start.usec);
#else
    stop = TclpGetWideClicks();
    totalMicroSec = ((double) TclpWideClicksToNanoseconds(stop - start))/1.0e3;
#endif

    if (count <= 1) {
	/*
	 * Use int obj since we know time is not fractional. [Bug 1202178]
	 */

	objs[0] = Tcl_NewWideIntObj((count <= 0) ? 0 : (Tcl_WideInt)totalMicroSec);
    } else {
	objs[0] = Tcl_NewDoubleObj(totalMicroSec/count);
    }

    /*
     * Construct the result as a list because many programs have always parsed
     * as such (extracting the first element, typically).
     */

    TclNewLiteralStringObj(objs[1], "microseconds");
    TclNewLiteralStringObj(objs[2], "per");
    TclNewLiteralStringObj(objs[3], "iteration");
    Tcl_SetObjResult(interp, Tcl_NewListObj(4, objs));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TimeRateObjCmd --
 *
 *	This object-based procedure is invoked to process the "timerate" Tcl
 *	command.
 *
 *	This is similar to command "time", except the execution limited by
 *	given time (in milliseconds) instead of repetition count.
 *
 * Example:
 *	timerate {after 5} 1000; # equivalent to: time {after 5} [expr 1000/5]
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_TimeRateObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static double measureOverhead = 0;
				/* global measure-overhead */
    double overhead = -1;	/* given measure-overhead */
    register Tcl_Obj *objPtr;
    register int result, i;
    Tcl_Obj *calibrate = NULL, *direct = NULL;
    TclWideMUInt count = 0;	/* Holds repetition count */
    Tcl_WideInt maxms = WIDE_MIN;
				/* Maximal running time (in milliseconds) */
    TclWideMUInt maxcnt = WIDE_MAX;
				/* Maximal count of iterations. */
    TclWideMUInt threshold = 1;	/* Current threshold for check time (faster
				 * repeat count without time check) */
    TclWideMUInt maxIterTm = 1;	/* Max time of some iteration as max
				 * threshold, additionally avoiding divide to
				 * zero (i.e., never < 1) */
    unsigned short factor = 50;	/* Factor (4..50) limiting threshold to avoid
				 * growth of execution time. */
    register Tcl_WideInt start, middle, stop;
#ifndef TCL_WIDE_CLICKS
    Tcl_Time now;
#endif /* !TCL_WIDE_CLICKS */
    static const char *const options[] = {
	"-direct",	"-overhead",	"-calibrate",	"--",	NULL
    };
    enum options {
	TMRT_EV_DIRECT,	TMRT_OVERHEAD,	TMRT_CALIBRATE,	TMRT_LAST
    };
    NRE_callback *rootPtr;
    ByteCode *codePtr = NULL;

    for (i = 1; i < objc - 1; i++) {
	int index;

	if (Tcl_GetIndexFromObj(NULL, objv[i], options, "option", TCL_EXACT,
		&index) != TCL_OK) {
	    break;
	}
	if (index == TMRT_LAST) {
	    i++;
	    break;
	}
	switch (index) {
	case TMRT_EV_DIRECT:
	    direct = objv[i];
	    break;
	case TMRT_OVERHEAD:
	    if (++i >= objc - 1) {
		goto usage;
	    }
	    if (Tcl_GetDoubleFromObj(interp, objv[i], &overhead) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case TMRT_CALIBRATE:
	    calibrate = objv[i];
	    break;
	}
    }

    if (i >= objc || i < objc - 3) {
    usage:
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-direct? ?-calibrate? ?-overhead double? "
		"command ?time ?max-count??");
	return TCL_ERROR;
    }
    objPtr = objv[i++];
    if (i < objc) {	/* max-time */
	result = Tcl_GetWideIntFromObj(interp, objv[i++], &maxms);
	if (result != TCL_OK) {
	    return result;
	}
	if (i < objc) {	/* max-count*/
	    Tcl_WideInt v;

	    result = Tcl_GetWideIntFromObj(interp, objv[i], &v);
	    if (result != TCL_OK) {
		return result;
	    }
	    maxcnt = (v > 0) ? v : 0;
	}
    }

    /*
     * If we are doing calibration.
     */

    if (calibrate) {
	/*
	 * If no time specified for the calibration.
	 */

	if (maxms == WIDE_MIN) {
	    Tcl_Obj *clobjv[6];
	    Tcl_WideInt maxCalTime = 5000;
	    double lastMeasureOverhead = measureOverhead;

	    clobjv[0] = objv[0];
	    i = 1;
	    if (direct) {
		clobjv[i++] = direct;
	    }
	    clobjv[i++] = objPtr;

	    /*
	     * Reset last measurement overhead.
	     */

	    measureOverhead = (double) 0;

	    /*
	     * Self-call with 100 milliseconds to warm-up, before entering the
	     * calibration cycle.
	     */

	    TclNewLongObj(clobjv[i], 100);
	    Tcl_IncrRefCount(clobjv[i]);
	    result = Tcl_TimeRateObjCmd(NULL, interp, i + 1, clobjv);
	    Tcl_DecrRefCount(clobjv[i]);
	    if (result != TCL_OK) {
		return result;
	    }

	    i--;
	    clobjv[i++] = calibrate;
	    clobjv[i++] = objPtr;

	    /*
	     * Set last measurement overhead to max.
	     */

	    measureOverhead = (double) UWIDE_MAX;

	    /*
	     * Run the calibration cycle until it is more precise.
	     */

	    maxms = -1000;
	    do {
		lastMeasureOverhead = measureOverhead;
		TclNewLongObj(clobjv[i], (int) maxms);
		Tcl_IncrRefCount(clobjv[i]);
		result = Tcl_TimeRateObjCmd(NULL, interp, i + 1, clobjv);
		Tcl_DecrRefCount(clobjv[i]);
		if (result != TCL_OK) {
		    return result;
		}
		maxCalTime += maxms;

		/*
		 * Increase maxms for more precise calibration.
		 */

		maxms -= -maxms / 4;

		/*
		 * As long as new value more as 0.05% better
		 */
	    } while ((measureOverhead >= lastMeasureOverhead
		    || measureOverhead / lastMeasureOverhead <= 0.9995)
		    && maxCalTime > 0);

	    return result;
	}
	if (maxms == 0) {
	    /*
	     * Reset last measurement overhead
	     */

	    measureOverhead = 0;
	    Tcl_SetObjResult(interp, Tcl_NewLongObj(0));
	    return TCL_OK;
	}

	/*
	 * If time is negative, make current overhead more precise.
	 */

	if (maxms > 0) {
	    /*
	     * Set last measurement overhead to max.
	     */

	    measureOverhead = (double) UWIDE_MAX;
	} else {
	    maxms = -maxms;
	}
    }

    if (maxms == WIDE_MIN) {
	maxms = 1000;
    }
    if (overhead == -1) {
	overhead = measureOverhead;
    }

    /*
     * Ensure that resetting of result will not smudge the further
     * measurement.
     */

    Tcl_ResetResult(interp);

    /*
     * Compile object if needed.
     */

    if (!direct) {
	if (TclInterpReady(interp) != TCL_OK) {
	    return TCL_ERROR;
	}
	codePtr = TclCompileObj(interp, objPtr, NULL, 0);
	TclPreserveByteCode(codePtr);
    }

    /*
     * Get start and stop time.
     */

#ifdef TCL_WIDE_CLICKS
    start = middle = TclpGetWideClicks();

    /*
     * Time to stop execution (in wide clicks).
     */

    stop = start + (maxms * 1000 / TclpWideClickInMicrosec());
#else
    Tcl_GetTime(&now);
    start = now.sec;
    start *= 1000000;
    start += now.usec;
    middle = start;

    /*
     * Time to stop execution (in microsecs).
     */

    stop = start + maxms * 1000;
#endif /* TCL_WIDE_CLICKS */

    /*
     * Start measurement.
     */

    if (maxcnt > 0) {
	while (1) {
	    /*
	     * Evaluate a single iteration.
	     */

	    count++;
	    if (!direct) {		/* precompiled */
		rootPtr = TOP_CB(interp);
		/*
		 * Use loop optimized TEBC call (TCL_EVAL_DISCARD_RESULT): it's a part of
		 * iteration, this way evaluation will be more similar to a cycle (also
		 * avoids extra overhead to set result to interp, etc.)
		 */
		((Interp *)interp)->evalFlags |= TCL_EVAL_DISCARD_RESULT;
		result = TclNRExecuteByteCode(interp, codePtr);
		result = TclNRRunCallbacks(interp, result, rootPtr);
	    } else {			/* eval */
		result = TclEvalObjEx(interp, objPtr, 0, NULL, 0);
	    }
	    /*
	     * Allow break and continue from measurement cycle (used for
	     * conditional stop and flow control of iterations).
	     */

	    switch (result) {
		case TCL_OK:
		    break;
		case TCL_BREAK:
		    /*
		     * Force stop immediately.
		     */
		    threshold = 1;
		    maxcnt = 0;
		    /* FALLTHRU */
		case TCL_CONTINUE:
		    result = TCL_OK;
		    break;
		default:
		    goto done;
	    }

	    /*
	     * Don't check time up to threshold.
	     */

	    if (--threshold > 0) {
		continue;
	    }

	    /*
	     * Check stop time reached, estimate new threshold.
	     */

#ifdef TCL_WIDE_CLICKS
	    middle = TclpGetWideClicks();
#else
	    Tcl_GetTime(&now);
	    middle = now.sec;
	    middle *= 1000000;
	    middle += now.usec;
#endif /* TCL_WIDE_CLICKS */

	    if (middle >= stop || count >= maxcnt) {
		break;
	    }

	    /*
	     * Don't calculate threshold by few iterations, because sometimes
	     * first iteration(s) can be too fast or slow (cached, delayed
	     * clean up, etc).
	     */

	    if (count < 10) {
		threshold = 1;
		continue;
	    }

	    /*
	     * Average iteration time in microsecs.
	     */

	    threshold = (middle - start) / count;
	    if (threshold > maxIterTm) {
		maxIterTm = threshold;

		/*
		 * Iterations seem to be longer.
		 */

		if (threshold > maxIterTm * 2) {
		    factor *= 2;
		    if (factor > 50) {
			factor = 50;
		    }
		} else {
		    if (factor < 50) {
			factor++;
		    }
		}
	    } else if (factor > 4) {
		/*
		 * Iterations seem to be shorter.
		 */

		if (threshold < (maxIterTm / 2)) {
		    factor /= 2;
		    if (factor < 4) {
			factor = 4;
		    }
		} else {
		    factor--;
		}
	    }

	    /*
	     * As relation between remaining time and time since last check,
	     * maximal some % of time (by factor), so avoid growing of the
	     * execution time if iterations are not consistent, e.g. was
	     * continuously on time).
	     */

	    threshold = ((stop - middle) / maxIterTm) / factor + 1;
	    if (threshold > 100000) {	/* fix for too large threshold */
		threshold = 100000;
	    }

	    /*
	     * Consider max-count
	     */

	    if (threshold > maxcnt - count) {
		threshold = maxcnt - count;
	    }
	}
    }

    {
	Tcl_Obj *objarr[8], **objs = objarr;
	TclWideMUInt usec, val;
	int digits;

	/*
	 * Absolute execution time in microseconds or in wide clicks.
	 */
	usec = (TclWideMUInt)(middle - start);

#ifdef TCL_WIDE_CLICKS
	/*
	 * convert execution time (in wide clicks) to microsecs.
	 */

	usec *= TclpWideClickInMicrosec();
#endif /* TCL_WIDE_CLICKS */

	if (!count) {		/* no iterations - avoid divide by zero */
	    objs[0] = objs[2] = objs[4] = Tcl_NewWideIntObj(0);
	    goto retRes;
	}

	/*
	 * If not calibrating...
	 */

	if (!calibrate) {
	    /*
	     * Minimize influence of measurement overhead.
	     */

	    if (overhead > 0) {
		/*
		 * Estimate the time of overhead (microsecs).
		 */

		TclWideMUInt curOverhead = overhead * count;

		if (usec > curOverhead) {
		    usec -= curOverhead;
		} else {
		    usec = 0;
		}
	    }
	} else {
	    /*
	     * Calibration: obtaining new measurement overhead.
	     */

	    if (measureOverhead > ((double) usec) / count) {
		measureOverhead = ((double) usec) / count;
	    }
	    objs[0] = Tcl_NewDoubleObj(measureOverhead);
	    TclNewLiteralStringObj(objs[1], "\xC2\xB5s/#-overhead"); /* mics */
	    objs += 2;
	}

	val = usec / count;		/* microsecs per iteration */
	if (val >= 1000000) {
	    objs[0] = Tcl_NewWideIntObj(val);
	} else {
	    if (val < 10) {
		digits = 6;
	    } else if (val < 100) {
		digits = 4;
	    } else if (val < 1000) {
		digits = 3;
	    } else if (val < 10000) {
		digits = 2;
	    } else {
		digits = 1;
	    }
	    objs[0] = Tcl_ObjPrintf("%.*f", digits, ((double) usec)/count);
	}

	objs[2] = Tcl_NewWideIntObj(count); /* iterations */

	/*
	 * Calculate speed as rate (count) per sec
	 */

	if (!usec) {
	    usec++;			/* Avoid divide by zero. */
	}
	if (count < (WIDE_MAX / 1000000)) {
	    val = (count * 1000000) / usec;
	    if (val < 100000) {
		if (val < 100) {
		    digits = 3;
		} else if (val < 1000) {
		    digits = 2;
		} else {
		    digits = 1;
		}
		objs[4] = Tcl_ObjPrintf("%.*f",
			digits, ((double) (count * 1000000)) / usec);
	    } else {
		objs[4] = Tcl_NewWideIntObj(val);
	    }
	} else {
	    objs[4] = Tcl_NewWideIntObj((count / usec) * 1000000);
	}

    retRes:
	/*
	 * Estimated net execution time (in millisecs).
	 */

	if (!calibrate) {
	    if (usec >= 1) {
		objs[6] = Tcl_ObjPrintf("%.3f", (double)usec / 1000);
	    } else {
		objs[6] = Tcl_NewWideIntObj(0);
	    }
	    TclNewLiteralStringObj(objs[7], "net-ms");
	}

	/*
	 * Construct the result as a list because many programs have always
	 * parsed as such (extracting the first element, typically).
	 */

	TclNewLiteralStringObj(objs[1], "\xC2\xB5s/#"); /* mics/# */
	TclNewLiteralStringObj(objs[3], "#");
	TclNewLiteralStringObj(objs[5], "#/sec");
	Tcl_SetObjResult(interp, Tcl_NewListObj(8, objarr));
    }

  done:
    if (codePtr != NULL) {
	TclReleaseByteCode(codePtr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TryObjCmd, TclNRTryObjCmd --
 *
 *	This procedure is invoked to process the "try" Tcl command. See the
 *	user documentation (or TIP #329) for details on what it does.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_TryObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return Tcl_NRCallObjProc(interp, TclNRTryObjCmd, dummy, objc, objv);
}

int
TclNRTryObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Obj *bodyObj, *handlersObj, *finallyObj = NULL;
    int i, bodyShared, haveHandlers, dummy, code;
    static const char *const handlerNames[] = {
	"finally", "on", "trap", NULL
    };
    enum Handlers {
	TryFinally, TryOn, TryTrap
    };

    /*
     * Parse the arguments. The handlers are passed to subsequent callbacks as
     * a Tcl_Obj list of the 5-tuples like (type, returnCode, errorCodePrefix,
     * bindVariables, script), and the finally script is just passed as it is.
     */

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"body ?handler ...? ?finally script?");
	return TCL_ERROR;
    }
    bodyObj = objv[1];
    handlersObj = Tcl_NewObj();
    bodyShared = 0;
    haveHandlers = 0;
    for (i=2 ; i<objc ; i++) {
	int type;
	Tcl_Obj *info[5];

	if (Tcl_GetIndexFromObj(interp, objv[i], handlerNames, "handler type",
		0, &type) != TCL_OK) {
	    Tcl_DecrRefCount(handlersObj);
	    return TCL_ERROR;
	}
	switch ((enum Handlers) type) {
	case TryFinally:	/* finally script */
	    if (i < objc-2) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"finally clause must be last", -1));
		Tcl_DecrRefCount(handlersObj);
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "TRY", "FINALLY",
			"NONTERMINAL", NULL);
		return TCL_ERROR;
	    } else if (i == objc-1) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"wrong # args to finally clause: must be"
			" \"... finally script\"", -1));
		Tcl_DecrRefCount(handlersObj);
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "TRY", "FINALLY",
			"ARGUMENT", NULL);
		return TCL_ERROR;
	    }
	    finallyObj = objv[++i];
	    break;

	case TryOn:		/* on code variableList script */
	    if (i > objc-4) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"wrong # args to on clause: must be \"... on code"
			" variableList script\"", -1));
		Tcl_DecrRefCount(handlersObj);
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "TRY", "ON",
			"ARGUMENT", NULL);
		return TCL_ERROR;
	    }
	    if (TclGetCompletionCodeFromObj(interp, objv[i+1],
		    &code) != TCL_OK) {
		Tcl_DecrRefCount(handlersObj);
		return TCL_ERROR;
	    }
	    info[2] = NULL;
	    goto commonHandler;

	case TryTrap:		/* trap pattern variableList script */
	    if (i > objc-4) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"wrong # args to trap clause: "
			"must be \"... trap pattern variableList script\"",
			-1));
		Tcl_DecrRefCount(handlersObj);
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "TRY", "TRAP",
			"ARGUMENT", NULL);
		return TCL_ERROR;
	    }
	    code = 1;
	    if (Tcl_ListObjLength(NULL, objv[i+1], &dummy) != TCL_OK) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"bad prefix '%s': must be a list",
			Tcl_GetString(objv[i+1])));
		Tcl_DecrRefCount(handlersObj);
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "TRY", "TRAP",
			"EXNFORMAT", NULL);
		return TCL_ERROR;
	    }
	    info[2] = objv[i+1];

	commonHandler:
	    if (Tcl_ListObjLength(interp, objv[i+2], &dummy) != TCL_OK) {
		Tcl_DecrRefCount(handlersObj);
		return TCL_ERROR;
	    }

	    info[0] = objv[i];			/* type */
	    TclNewIntObj(info[1], code);	/* returnCode */
	    if (info[2] == NULL) {		/* errorCodePrefix */
		TclNewObj(info[2]);
	    }
	    info[3] = objv[i+2];		/* bindVariables */
	    info[4] = objv[i+3];		/* script */

	    bodyShared = !strcmp(TclGetString(objv[i+3]), "-");
	    Tcl_ListObjAppendElement(NULL, handlersObj,
		    Tcl_NewListObj(5, info));
	    haveHandlers = 1;
	    i += 3;
	    break;
	}
    }
    if (bodyShared) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"last non-finally clause must not have a body of \"-\"", -1));
	Tcl_DecrRefCount(handlersObj);
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "TRY", "BADFALLTHROUGH",
		NULL);
	return TCL_ERROR;
    }
    if (!haveHandlers) {
	Tcl_DecrRefCount(handlersObj);
	handlersObj = NULL;
    }

    /*
     * Execute the body.
     */

    Tcl_NRAddCallback(interp, TryPostBody, handlersObj, finallyObj,
	    (ClientData)objv, INT2PTR(objc));
    return TclNREvalObjEx(interp, bodyObj, 0,
	    ((Interp *) interp)->cmdFramePtr, 1);
}

/*
 *----------------------------------------------------------------------
 *
 * During --
 *
 *	This helper function patches together the updates to the interpreter's
 *	return options that are needed when things fail during the processing
 *	of a handler or finally script for the [try] command.
 *
 * Returns:
 *	The new option dictionary.
 *
 *----------------------------------------------------------------------
 */

static inline Tcl_Obj *
During(
    Tcl_Interp *interp,
    int resultCode,		/* The result code from the just-evaluated
				 * script. */
    Tcl_Obj *oldOptions,	/* The old option dictionary. */
    Tcl_Obj *errorInfo)		/* An object to append to the errorinfo and
				 * release, or NULL if nothing is to be added.
				 * Designed to be used with Tcl_ObjPrintf. */
{
    Tcl_Obj *during, *options;

    if (errorInfo != NULL) {
	Tcl_AppendObjToErrorInfo(interp, errorInfo);
    }
    options = Tcl_GetReturnOptions(interp, resultCode);
    TclNewLiteralStringObj(during, "-during");
    Tcl_IncrRefCount(during);
    Tcl_DictObjPut(interp, options, during, oldOptions);
    Tcl_DecrRefCount(during);
    Tcl_IncrRefCount(options);
    Tcl_DecrRefCount(oldOptions);
    return options;
}

/*
 *----------------------------------------------------------------------
 *
 * TryPostBody --
 *
 *	Callback to handle the outcome of the execution of the body of a 'try'
 *	command.
 *
 *----------------------------------------------------------------------
 */

static int
TryPostBody(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Obj *resultObj, *options, *handlersObj, *finallyObj, *cmdObj, **objv;
    int i, dummy, code, objc;
    int numHandlers = 0;

    handlersObj = data[0];
    finallyObj = data[1];
    objv = data[2];
    objc = PTR2INT(data[3]);

    cmdObj = objv[0];

    /*
     * Check for limits/rewinding, which override normal trapping behaviour.
     */

    if (((Interp*) interp)->execEnvPtr->rewind || Tcl_LimitExceeded(interp)) {
	Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		"\n    (\"%s\" body line %d)", TclGetString(cmdObj),
		Tcl_GetErrorLine(interp)));
	if (handlersObj != NULL) {
	    Tcl_DecrRefCount(handlersObj);
	}
	return TCL_ERROR;
    }

    /*
     * Basic processing of the outcome of the script, including adding of
     * errorinfo trace.
     */

    if (result == TCL_ERROR) {
	Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		"\n    (\"%s\" body line %d)", TclGetString(cmdObj),
		Tcl_GetErrorLine(interp)));
    }
    resultObj = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(resultObj);
    options = Tcl_GetReturnOptions(interp, result);
    Tcl_IncrRefCount(options);
    Tcl_ResetResult(interp);

    /*
     * Handle the results.
     */

    if (handlersObj != NULL) {
	int found = 0;
	Tcl_Obj **handlers, **info;

	Tcl_ListObjGetElements(NULL, handlersObj, &numHandlers, &handlers);
	for (i=0 ; i<numHandlers ; i++) {
	    Tcl_Obj *handlerBodyObj;

	    Tcl_ListObjGetElements(NULL, handlers[i], &dummy, &info);
	    if (!found) {
		Tcl_GetIntFromObj(NULL, info[1], &code);
		if (code != result) {
		    continue;
		}

		/*
		 * When processing an error, we must also perform list-prefix
		 * matching of the errorcode list. However, if this was an
		 * 'on' handler, the list that we are matching against will be
		 * empty.
		 */

		if (code == TCL_ERROR) {
		    Tcl_Obj *errorCodeName, *errcode, **bits1, **bits2;
		    int len1, len2, j;

		    TclNewLiteralStringObj(errorCodeName, "-errorcode");
		    Tcl_DictObjGet(NULL, options, errorCodeName, &errcode);
		    Tcl_DecrRefCount(errorCodeName);
		    Tcl_ListObjGetElements(NULL, info[2], &len1, &bits1);
		    if (Tcl_ListObjGetElements(NULL, errcode, &len2,
			    &bits2) != TCL_OK) {
			continue;
		    }
		    if (len2 < len1) {
			continue;
		    }
		    for (j=0 ; j<len1 ; j++) {
			if (strcmp(TclGetString(bits1[j]),
				TclGetString(bits2[j])) != 0) {
			    /*
			     * Really want 'continue outerloop;', but C does
			     * not give us that.
			     */

			    goto didNotMatch;
			}
		    }
		}

		found = 1;
	    }

	    /*
	     * Now we need to scan forward over "-" bodies. Note that we've
	     * already checked that the last body is not a "-", so this search
	     * will terminate successfully.
	     */

	    if (!strcmp(TclGetString(info[4]), "-")) {
		continue;
	    }

	    /*
	     * Bind the variables. We already know this is a list of variable
	     * names, but it might be empty.
	     */

	    Tcl_ResetResult(interp);
	    result = TCL_ERROR;
	    Tcl_ListObjLength(NULL, info[3], &dummy);
	    if (dummy > 0) {
		Tcl_Obj *varName;

		Tcl_ListObjIndex(NULL, info[3], 0, &varName);
		if (Tcl_ObjSetVar2(interp, varName, NULL, resultObj,
			TCL_LEAVE_ERR_MSG) == NULL) {
		    Tcl_DecrRefCount(resultObj);
		    goto handlerFailed;
		}
		Tcl_DecrRefCount(resultObj);
		if (dummy > 1) {
		    Tcl_ListObjIndex(NULL, info[3], 1, &varName);
		    if (Tcl_ObjSetVar2(interp, varName, NULL, options,
			    TCL_LEAVE_ERR_MSG) == NULL) {
			goto handlerFailed;
		    }
		}
	    } else {
		/*
		 * Dispose of the result to prevent a memleak. [Bug 2910044]
		 */

		Tcl_DecrRefCount(resultObj);
	    }

	    /*
	     * Evaluate the handler body and process the outcome. Note that we
	     * need to keep the kind of handler for debugging purposes, and in
	     * any case anything we want from info[] must be extracted right
	     * now because the info[] array is about to become invalid. There
	     * is very little refcount handling here however, since we know
	     * that the objects that we still want to refer to now were input
	     * arguments to [try] and so are still on the Tcl value stack.
	     */

	    handlerBodyObj = info[4];
	    Tcl_NRAddCallback(interp, TryPostHandler, objv, options, info[0],
		    INT2PTR((finallyObj == NULL) ? 0 : objc - 1));
	    Tcl_DecrRefCount(handlersObj);
	    return TclNREvalObjEx(interp, handlerBodyObj, 0,
		    ((Interp *) interp)->cmdFramePtr, 4*i + 5);

	handlerFailed:
	    resultObj = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(resultObj);
	    options = During(interp, result, options, NULL);
	    break;

	didNotMatch:
	    continue;
	}

	/*
	 * No handler matched; get rid of the list of handlers.
	 */

	Tcl_DecrRefCount(handlersObj);
    }

    /*
     * Process the finally clause.
     */

    if (finallyObj != NULL) {
	Tcl_NRAddCallback(interp, TryPostFinal, resultObj, options, cmdObj,
		NULL);
	return TclNREvalObjEx(interp, finallyObj, 0,
		((Interp *) interp)->cmdFramePtr, objc - 1);
    }

    /*
     * Install the correct result/options into the interpreter and clean up
     * any temporary storage.
     */

    result = Tcl_SetReturnOptions(interp, options);
    Tcl_DecrRefCount(options);
    Tcl_SetObjResult(interp, resultObj);
    Tcl_DecrRefCount(resultObj);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TryPostHandler --
 *
 *	Callback to handle the outcome of the execution of a handler of a
 *	'try' command.
 *
 *----------------------------------------------------------------------
 */

static int
TryPostHandler(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Obj *resultObj, *cmdObj, *options, *handlerKindObj, **objv;
    Tcl_Obj *finallyObj;
    int finally;

    objv = data[0];
    options = data[1];
    handlerKindObj = data[2];
    finally = PTR2INT(data[3]);

    cmdObj = objv[0];
    finallyObj = finally ? objv[finally] : 0;

    /*
     * Check for limits/rewinding, which override normal trapping behaviour.
     */

    if (((Interp*) interp)->execEnvPtr->rewind || Tcl_LimitExceeded(interp)) {
	options = During(interp, result, options, Tcl_ObjPrintf(
		"\n    (\"%s ... %s\" handler line %d)",
		TclGetString(cmdObj), TclGetString(handlerKindObj),
		Tcl_GetErrorLine(interp)));
	Tcl_DecrRefCount(options);
	return TCL_ERROR;
    }

    /*
     * The handler result completely substitutes for the result of the body.
     */

    resultObj = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(resultObj);
    if (result == TCL_ERROR) {
	options = During(interp, result, options, Tcl_ObjPrintf(
		"\n    (\"%s ... %s\" handler line %d)",
		TclGetString(cmdObj), TclGetString(handlerKindObj),
		Tcl_GetErrorLine(interp)));
    } else {
	Tcl_DecrRefCount(options);
	options = Tcl_GetReturnOptions(interp, result);
	Tcl_IncrRefCount(options);
    }

    /*
     * Process the finally clause if it is present.
     */

    if (finallyObj != NULL) {
	Interp *iPtr = (Interp *) interp;

	Tcl_NRAddCallback(interp, TryPostFinal, resultObj, options, cmdObj,
		NULL);

	/* The 'finally' script is always the last argument word. */
	return TclNREvalObjEx(interp, finallyObj, 0, iPtr->cmdFramePtr,
		finally);
    }

    /*
     * Install the correct result/options into the interpreter and clean up
     * any temporary storage.
     */

    result = Tcl_SetReturnOptions(interp, options);
    Tcl_DecrRefCount(options);
    Tcl_SetObjResult(interp, resultObj);
    Tcl_DecrRefCount(resultObj);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TryPostFinal --
 *
 *	Callback to handle the outcome of the execution of the finally script
 *	of a 'try' command.
 *
 *----------------------------------------------------------------------
 */

static int
TryPostFinal(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Obj *resultObj, *options, *cmdObj;

    resultObj = data[0];
    options = data[1];
    cmdObj = data[2];

    /*
     * If the result wasn't OK, we need to adjust the result options.
     */

    if (result != TCL_OK) {
	Tcl_DecrRefCount(resultObj);
	resultObj = NULL;
	if (result == TCL_ERROR) {
	    options = During(interp, result, options, Tcl_ObjPrintf(
		    "\n    (\"%s ... finally\" body line %d)",
		    TclGetString(cmdObj), Tcl_GetErrorLine(interp)));
	} else {
	    Tcl_Obj *origOptions = options;

	    options = Tcl_GetReturnOptions(interp, result);
	    Tcl_IncrRefCount(options);
	    Tcl_DecrRefCount(origOptions);
	}
    }

    /*
     * Install the correct result/options into the interpreter and clean up
     * any temporary storage.
     */

    result = Tcl_SetReturnOptions(interp, options);
    Tcl_DecrRefCount(options);
    if (resultObj != NULL) {
	Tcl_SetObjResult(interp, resultObj);
	Tcl_DecrRefCount(resultObj);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_WhileObjCmd --
 *
 *	This procedure is invoked to process the "while" Tcl command. See the
 *	user documentation for details on what it does.
 *
 *	With the bytecode compiler, this procedure is only called when a
 *	command name is computed at runtime, and is "while" or the name to
 *	which "while" was renamed: e.g., "set z while; $z {$i<100} {}"
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_WhileObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return Tcl_NRCallObjProc(interp, TclNRWhileObjCmd, dummy, objc, objv);
}

int
TclNRWhileObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    ForIterData *iterPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "test command");
	return TCL_ERROR;
    }

    /*
     * We reuse [for]'s callback, passing a NULL for the 'next' script.
     */

    TclSmallAllocEx(interp, sizeof(ForIterData), iterPtr);
    iterPtr->cond = objv[1];
    iterPtr->body = objv[2];
    iterPtr->next = NULL;
    iterPtr->msg  = "\n    (\"while\" body line %d)";
    iterPtr->word = 2;

    TclNRAddCallback(interp, TclNRForIterCallback, iterPtr, NULL,
	    NULL, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclListLines --
 *
 *	???
 *
 * Results:
 *	Filled in array of line numbers?
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TclListLines(
    Tcl_Obj *listObj,		/* Pointer to obj holding a string with list
				 * structure. Assumed to be valid. Assumed to
				 * contain n elements. */
    int line,			/* Line the list as a whole starts on. */
    int n,			/* #elements in lines */
    int *lines,			/* Array of line numbers, to fill. */
    Tcl_Obj *const *elems)      /* The list elems as Tcl_Obj*, in need of
				 * derived continuation data */
{
    const char *listStr = Tcl_GetString(listObj);
    const char *listHead = listStr;
    int i, length = strlen(listStr);
    const char *element = NULL, *next = NULL;
    ContLineLoc *clLocPtr = TclContinuationsGet(listObj);
    int *clNext = (clLocPtr ? &clLocPtr->loc[0] : NULL);

    for (i = 0; i < n; i++) {
	TclFindElement(NULL, listStr, length, &element, &next, NULL, NULL);

	TclAdvanceLines(&line, listStr, element);
				/* Leading whitespace */
	TclAdvanceContinuations(&line, &clNext, element - listHead);
	if (elems && clNext) {
	    TclContinuationsEnterDerived(elems[i], element-listHead, clNext);
	}
	lines[i] = line;
	length -= (next - listStr);
	TclAdvanceLines(&line, element, next);
				/* Element */
	listStr = next;

	if (*element == 0) {
	    /* ASSERT i == n */
	    break;
	}
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
