/*
 * tclUtil.c --
 *
 *	This file contains utility functions that are used by many Tcl
 *	commands.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 * Copyright (c) 2001 by Kevin B. Kenny. All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclParse.h"
#include "tclStringTrim.h"
#include <math.h>

/*
 * The absolute pathname of the executable in which this Tcl library is
 * running.
 */

static ProcessGlobalValue executableName = {
    0, 0, NULL, NULL, NULL, NULL, NULL
};

/*
 * The following values are used in the flags arguments of Tcl*Scan*Element
 * and Tcl*Convert*Element.  The values TCL_DONT_USE_BRACES and
 * TCL_DONT_QUOTE_HASH are defined in tcl.h, like so:
 *
#define TCL_DONT_USE_BRACES     1
#define TCL_DONT_QUOTE_HASH     8
 *
 * Those are public flag bits which callers of the public routines
 * Tcl_Convert*Element() can use to indicate:
 *
 * TCL_DONT_USE_BRACES -	1 means the caller is insisting that brace
 *				quoting not be used when converting the list
 *				element.
 * TCL_DONT_QUOTE_HASH -	1 means the caller insists that a leading hash
 *				character ('#') should *not* be quoted. This
 *				is appropriate when the caller can guarantee
 *				the element is not the first element of a
 *				list, so [eval] cannot mis-parse the element
 *				as a comment.
 *
 * The remaining values which can be carried by the flags of these routines
 * are for internal use only.  Make sure they do not overlap with the public
 * values above.
 *
 * The Tcl*Scan*Element() routines make a determination which of 4 modes of
 * conversion is most appropriate for Tcl*Convert*Element() to perform, and
 * sets two bits of the flags value to indicate the mode selected.
 *
 * CONVERT_NONE		The element needs no quoting. Its literal string is
 *			suitable as is.
 * CONVERT_BRACE	The conversion should be enclosing the literal string
 *			in braces.
 * CONVERT_ESCAPE	The conversion should be using backslashes to escape
 *			any characters in the string that require it.
 * CONVERT_MASK		A mask value used to extract the conversion mode from
 *			the flags argument.
 *			Also indicates a strange conversion mode where all
 *			special characters are escaped with backslashes
 *			*except for braces*. This is a strange and unnecessary
 *			case, but it's part of the historical way in which
 *			lists have been formatted in Tcl. To experiment with
 *			removing this case, set the value of COMPAT to 0.
 *
 * One last flag value is used only by callers of TclScanElement(). The flag
 * value produced by a call to Tcl*Scan*Element() will never leave this bit
 * set.
 *
 * CONVERT_ANY		The caller of TclScanElement() declares it can make no
 *			promise about what public flags will be passed to the
 *			matching call of TclConvertElement(). As such,
 *			TclScanElement() has to determine the worst case
 *			destination buffer length over all possibilities, and
 *			in other cases this means an overestimate of the
 *			required size.
 *
 * For more details, see the comments on the Tcl*Scan*Element and
 * Tcl*Convert*Element routines.
 */

#define COMPAT 1
#define CONVERT_NONE	0
#define CONVERT_BRACE	2
#define CONVERT_ESCAPE	4
#define CONVERT_MASK	(CONVERT_BRACE | CONVERT_ESCAPE)
#define CONVERT_ANY	16

/*
 * The following key is used by Tcl_PrintDouble and TclPrecTraceProc to
 * access the precision to be used for double formatting.
 */

static Tcl_ThreadDataKey precisionKey;

/*
 * Prototypes for functions defined later in this file.
 */

static void		ClearHash(Tcl_HashTable *tablePtr);
static void		FreeProcessGlobalValue(ClientData clientData);
static void		FreeThreadHash(ClientData clientData);
static int		GetEndOffsetFromObj(Tcl_Obj *objPtr, int endValue,
			    int *indexPtr);
static Tcl_HashTable *	GetThreadHash(Tcl_ThreadDataKey *keyPtr);
static int		SetEndOffsetFromAny(Tcl_Interp *interp,
			    Tcl_Obj *objPtr);
static void		UpdateStringOfEndOffset(Tcl_Obj *objPtr);
static int		FindElement(Tcl_Interp *interp, const char *string,
			    int stringLength, const char *typeStr,
			    const char *typeCode, const char **elementPtr,
			    const char **nextPtr, int *sizePtr,
			    int *literalPtr);
/*
 * The following is the Tcl object type definition for an object that
 * represents a list index in the form, "end-offset". It is used as a
 * performance optimization in TclGetIntForIndex. The internal rep is an
 * integer, so no memory management is required for it.
 */

const Tcl_ObjType tclEndOffsetType = {
    "end-offset",			/* name */
    NULL,				/* freeIntRepProc */
    NULL,				/* dupIntRepProc */
    UpdateStringOfEndOffset,		/* updateStringProc */
    SetEndOffsetFromAny
};

/*
 *	*	STRING REPRESENTATION OF LISTS	*	*	*
 *
 * The next several routines implement the conversions of strings to and from
 * Tcl lists. To understand their operation, the rules of parsing and
 * generating the string representation of lists must be known.  Here we
 * describe them in one place.
 *
 * A list is made up of zero or more elements. Any string is a list if it is
 * made up of alternating substrings of element-separating ASCII whitespace
 * and properly formatted elements.
 *
 * The ASCII characters which can make up the whitespace between list elements
 * are:
 *
 *	\u0009	\t	TAB
 *	\u000A	\n	NEWLINE
 *	\u000B	\v	VERTICAL TAB
 *	\u000C	\f	FORM FEED
 * 	\u000D	\r	CARRIAGE RETURN
 *	\u0020		SPACE
 *
 * NOTE: differences between this and other places where Tcl defines a role
 * for "whitespace".
 *
 *	* Unlike command parsing, here NEWLINE is just another whitespace
 *	  character; its role as a command terminator in a script has no
 *	  importance here.
 *
 *	* Unlike command parsing, the BACKSLASH NEWLINE sequence is not
 *	  considered to be a whitespace character.
 *
 *	* Other Unicode whitespace characters (recognized by [string is space]
 *	  or Tcl_UniCharIsSpace()) do not play any role as element separators
 *	  in Tcl lists.
 *
 *	* The NUL byte ought not appear, as it is not in strings properly
 *	  encoded for Tcl, but if it is present, it is not treated as
 *	  separating whitespace, or a string terminator. It is just another
 *	  character in a list element.
 *
 * The interpretation of a formatted substring as a list element follows rules
 * similar to the parsing of the words of a command in a Tcl script. Backslash
 * substitution plays a key role, and is defined exactly as it is in command
 * parsing. The same routine, TclParseBackslash() is used in both command
 * parsing and list parsing.
 *
 * NOTE: This means that if and when backslash substitution rules ever change
 * for command parsing, the interpretation of strings as lists also changes.
 *
 * Backslash substitution replaces an "escape sequence" of one or more
 * characters starting with
 *		\u005c	\	BACKSLASH
 * with a single character. The one character escape sequence case happens only
 * when BACKSLASH is the last character in the string. In all other cases, the
 * escape sequence is at least two characters long.
 *
 * The formatted substrings are interpreted as element values according to the
 * following cases:
 *
 * * If the first character of a formatted substring is
 *		\u007b	{	OPEN BRACE
 *   then the end of the substring is the matching
 *		\u007d	}	CLOSE BRACE
 *   character, where matching is determined by counting nesting levels, and
 *   not including any brace characters that are contained within a backslash
 *   escape sequence in the nesting count. Having found the matching brace,
 *   all characters between the braces are the string value of the element.
 *   If no matching close brace is found before the end of the string, the
 *   string is not a Tcl list. If the character following the close brace is
 *   not an element separating whitespace character, or the end of the string,
 *   then the string is not a Tcl list.
 *
 *   NOTE: this differs from a brace-quoted word in the parsing of a Tcl
 *   command only in its treatment of the backslash-newline sequence. In a
 *   list element, the literal characters in the backslash-newline sequence
 *   become part of the element value. In a script word, conversion to a
 *   single SPACE character is done.
 *
 *   NOTE: Most list element values can be represented by a formatted
 *   substring using brace quoting. The exceptions are any element value that
 *   includes an unbalanced brace not in a backslash escape sequence, and any
 *   value that ends with a backslash not itself in a backslash escape
 *   sequence.
 *
 * * If the first character of a formatted substring is
 *		\u0022	"	QUOTE
 *   then the end of the substring is the next QUOTE character, not counting
 *   any QUOTE characters that are contained within a backslash escape
 *   sequence. If no next QUOTE is found before the end of the string, the
 *   string is not a Tcl list. If the character following the closing QUOTE is
 *   not an element separating whitespace character, or the end of the string,
 *   then the string is not a Tcl list. Having found the limits of the
 *   substring, the element value is produced by performing backslash
 *   substitution on the character sequence between the open and close QUOTEs.
 *
 *   NOTE: Any element value can be represented by this style of formatting,
 *   given suitable choice of backslash escape sequences.
 *
 * * All other formatted substrings are terminated by the next element
 *   separating whitespace character in the string.  Having found the limits
 *   of the substring, the element value is produced by performing backslash
 *   substitution on it.
 *
 *   NOTE: Any element value can be represented by this style of formatting,
 *   given suitable choice of backslash escape sequences, with one exception.
 *   The empty string cannot be represented as a list element without the use
 *   of either braces or quotes to delimit it.
 *
 * This collection of parsing rules is implemented in the routine
 * FindElement().
 *
 * In order to produce lists that can be parsed by these rules, we need the
 * ability to distinguish between characters that are part of a list element
 * value from characters providing syntax that define the structure of the
 * list. This means that our code that generates lists must at a minimum be
 * able to produce escape sequences for the 10 characters identified above
 * that have significance to a list parser.
 *
 *	*	*	CANONICAL LISTS	*	*	*	*	*
 *
 * In addition to the basic rules for parsing strings into Tcl lists, there
 * are additional properties to be met by the set of list values that are
 * generated by Tcl.  Such list values are often said to be in "canonical
 * form":
 *
 * * When any canonical list is evaluated as a Tcl script, it is a script of
 *   either zero commands (an empty list) or exactly one command. The command
 *   word is exactly the first element of the list, and each argument word is
 *   exactly one of the following elements of the list. This means that any
 *   characters that have special meaning during script evaluation need
 *   special treatment when canonical lists are produced:
 *
 *	* Whitespace between elements may not include NEWLINE.
 *	* The command terminating character,
 *		\u003b	;	SEMICOLON
 *	  must be BRACEd, QUOTEd, or escaped so that it does not terminate the
 * 	  command prematurely.
 *	* Any of the characters that begin substitutions in scripts,
 *		\u0024	$	DOLLAR
 *		\u005b	[	OPEN BRACKET
 *		\u005c	\	BACKSLASH
 *	  need to be BRACEd or escaped.
 *	* In any list where the first character of the first element is
 *		\u0023	#	HASH
 *	  that HASH character must be BRACEd, QUOTEd, or escaped so that it
 *	  does not convert the command into a comment.
 *	* Any list element that contains the character sequence BACKSLASH
 *	  NEWLINE cannot be formatted with BRACEs. The BACKSLASH character
 *	  must be represented by an escape sequence, and unless QUOTEs are
 *	  used, the NEWLINE must be as well.
 *
 * * It is also guaranteed that one can use a canonical list as a building
 *   block of a larger script within command substitution, as in this example:
 *	set script "puts \[[list $cmd $arg]]"; eval $script
 *   To support this usage, any appearance of the character
 *		\u005d	]	CLOSE BRACKET
 *   in a list element must be BRACEd, QUOTEd, or escaped.
 *
 * * Finally it is guaranteed that enclosing a canonical list in braces
 *   produces a new value that is also a canonical list.  This new list has
 *   length 1, and its only element is the original canonical list.  This same
 *   guarantee also makes it possible to construct scripts where an argument
 *   word is given a list value by enclosing the canonical form of that list
 *   in braces:
 *	set script "puts {[list $one $two $three]}"; eval $script
 *   This sort of coding was once fairly common, though it's become more
 *   idiomatic to see the following instead:
 *	set script [list puts [list $one $two $three]]; eval $script
 *   In order to support this guarantee, every canonical list must have
 *   balance when counting those braces that are not in escape sequences.
 *
 * Within these constraints, the canonical list generation routines
 * TclScanElement() and TclConvertElement() attempt to generate the string for
 * any list that is easiest to read. When an element value is itself
 * acceptable as the formatted substring, it is usually used (CONVERT_NONE).
 * When some quoting or escaping is required, use of BRACEs (CONVERT_BRACE) is
 * usually preferred over the use of escape sequences (CONVERT_ESCAPE). There
 * are some exceptions to both of these preferences for reasons of code
 * simplicity, efficiency, and continuation of historical habits. Canonical
 * lists never use the QUOTE formatting to delimit their elements because that
 * form of quoting does not nest, which makes construction of nested lists far
 * too much trouble.  Canonical lists always use only a single SPACE character
 * for element-separating whitespace.
 *
 *	*	*	FUTURE CONSIDERATIONS	*	*	*
 *
 * When a list element requires quoting or escaping due to a CLOSE BRACKET
 * character or an internal QUOTE character, a strange formatting mode is
 * recommended. For example, if the value "a{b]c}d" is converted by the usual
 * modes:
 *
 *	CONVERT_BRACE:	a{b]c}d		=> {a{b]c}d}
 *	CONVERT_ESCAPE:	a{b]c}d		=> a\{b\]c\}d
 *
 * we get perfectly usable formatted list elements. However, this is not what
 * Tcl releases have been producing. Instead, we have:
 *
 *	CONVERT_MASK:	a{b]c}d		=> a{b\]c}d
 *
 * where the CLOSE BRACKET is escaped, but the BRACEs are not. The same effect
 * can be seen replacing ] with " in this example. There does not appear to be
 * any functional or aesthetic purpose for this strange additional mode. The
 * sole purpose I can see for preserving it is to keep generating the same
 * formatted lists programmers have become accustomed to, and perhaps written
 * tests to expect. That is, compatibility only. The additional code
 * complexity required to support this mode is significant. The lines of code
 * supporting it are delimited in the routines below with #if COMPAT
 * directives. This makes it easy to experiment with eliminating this
 * formatting mode simply with "#define COMPAT 0" above. I believe this is
 * worth considering.
 *
 * Another consideration is the treatment of QUOTE characters in list
 * elements. TclConvertElement() must have the ability to produce the escape
 * sequence \" so that when a list element begins with a QUOTE we do not
 * confuse that first character with a QUOTE used as list syntax to define
 * list structure. However, that is the only place where QUOTE characters need
 * quoting. In this way, handling QUOTE could really be much more like the way
 * we handle HASH which also needs quoting and escaping only in particular
 * situations. Following up this could increase the set of list elements that
 * can use the CONVERT_NONE formatting mode.
 *
 * More speculative is that the demands of canonical list form require brace
 * balance for the list as a whole, while the current implementation achieves
 * this by establishing brace balance for every element.
 *
 * Finally, a reminder that the rules for parsing and formatting lists are
 * closely tied together with the rules for parsing and evaluating scripts,
 * and will need to evolve in sync.
 */

/*
 *----------------------------------------------------------------------
 *
 * TclMaxListLength --
 *
 *	Given 'bytes' pointing to 'numBytes' bytes, scan through them and
 *	count the number of whitespace runs that could be list element
 *	separators. If 'numBytes' is -1, scan to the terminating '\0'. Not a
 *	full list parser. Typically used to get a quick and dirty overestimate
 *	of length size in order to allocate space for an actual list parser to
 *	operate with.
 *
 * Results:
 *	Returns the largest number of list elements that could possibly be in
 *	this string, interpreted as a Tcl list. If 'endPtr' is not NULL,
 *	writes a pointer to the end of the string scanned there.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclMaxListLength(
    const char *bytes,
    int numBytes,
    const char **endPtr)
{
    int count = 0;

    if ((numBytes == 0) || ((numBytes == -1) && (*bytes == '\0'))) {
	/* Empty string case - quick exit */
	goto done;
    }

    /*
     * No list element before leading white space.
     */

    count += 1 - TclIsSpaceProc(*bytes);

    /*
     * Count white space runs as potential element separators.
     */

    while (numBytes) {
	if ((numBytes == -1) && (*bytes == '\0')) {
	    break;
	}
	if (TclIsSpaceProc(*bytes)) {
	    /*
	     * Space run started; bump count.
	     */

	    count++;
	    do {
		bytes++;
		numBytes -= (numBytes != -1);
	    } while (numBytes && TclIsSpaceProc(*bytes));
	    if ((numBytes == 0) || ((numBytes == -1) && (*bytes == '\0'))) {
		break;
	    }

	    /*
	     * (*bytes) is non-space; return to counting state.
	     */
	}
	bytes++;
	numBytes -= (numBytes != -1);
    }

    /*
     * No list element following trailing white space.
     */

    count -= TclIsSpaceProc(bytes[-1]);

  done:
    if (endPtr) {
	*endPtr = bytes;
    }
    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFindElement --
 *
 *	Given a pointer into a Tcl list, locate the first (or next) element in
 *	the list.
 *
 * Results:
 *	The return value is normally TCL_OK, which means that the element was
 *	successfully located. If TCL_ERROR is returned it means that list
 *	didn't have proper list structure; the interp's result contains a more
 *	detailed error message.
 *
 *	If TCL_OK is returned, then *elementPtr will be set to point to the
 *	first element of list, and *nextPtr will be set to point to the
 *	character just after any white space following the last character
 *	that's part of the element. If this is the last argument in the list,
 *	then *nextPtr will point just after the last character in the list
 *	(i.e., at the character at list+listLength). If sizePtr is non-NULL,
 *	*sizePtr is filled in with the number of bytes in the element. If the
 *	element is in braces, then *elementPtr will point to the character
 *	after the opening brace and *sizePtr will not include either of the
 *	braces. If there isn't an element in the list, *sizePtr will be zero,
 *	and both *elementPtr and *nextPtr will point just after the last
 *	character in the list. If literalPtr is non-NULL, *literalPtr is set
 *	to a boolean value indicating whether the substring returned as the
 *	values of **elementPtr and *sizePtr is the literal value of a list
 *	element. If not, a call to TclCopyAndCollapse() is needed to produce
 *	the actual value of the list element. Note: this function does NOT
 *	collapse backslash sequences, but uses *literalPtr to tell callers
 *	when it is required for them to do so.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclFindElement(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. If
				 * NULL, then no error message is left after
				 * errors. */
    const char *list,		/* Points to the first byte of a string
				 * containing a Tcl list with zero or more
				 * elements (possibly in braces). */
    int listLength,		/* Number of bytes in the list's string. */
    const char **elementPtr,	/* Where to put address of first significant
				 * character in first element of list. */
    const char **nextPtr,	/* Fill in with location of character just
				 * after all white space following end of
				 * argument (next arg or end of list). */
    int *sizePtr,		/* If non-zero, fill in with size of
				 * element. */
    int *literalPtr)		/* If non-zero, fill in with non-zero/zero to
				 * indicate that the substring of *sizePtr
				 * bytes starting at **elementPtr is/is not
				 * the literal list element and therefore
				 * does not/does require a call to
				 * TclCopyAndCollapse() by the caller. */
{
    return FindElement(interp, list, listLength, "list", "LIST", elementPtr,
	    nextPtr, sizePtr, literalPtr);
}

int
TclFindDictElement(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. If
				 * NULL, then no error message is left after
				 * errors. */
    const char *dict,		/* Points to the first byte of a string
				 * containing a Tcl dictionary with zero or
				 * more keys and values (possibly in
				 * braces). */
    int dictLength,		/* Number of bytes in the dict's string. */
    const char **elementPtr,	/* Where to put address of first significant
				 * character in the first element (i.e., key
				 * or value) of dict. */
    const char **nextPtr,	/* Fill in with location of character just
				 * after all white space following end of
				 * element (next arg or end of list). */
    int *sizePtr,		/* If non-zero, fill in with size of
				 * element. */
    int *literalPtr)		/* If non-zero, fill in with non-zero/zero to
				 * indicate that the substring of *sizePtr
				 * bytes starting at **elementPtr is/is not
				 * the literal key or value and therefore
				 * does not/does require a call to
				 * TclCopyAndCollapse() by the caller. */
{
    return FindElement(interp, dict, dictLength, "dict", "DICTIONARY",
	    elementPtr, nextPtr, sizePtr, literalPtr);
}

static int
FindElement(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. If
				 * NULL, then no error message is left after
				 * errors. */
    const char *string,		/* Points to the first byte of a string
				 * containing a Tcl list or dictionary with
				 * zero or more elements (possibly in
				 * braces). */
    int stringLength,		/* Number of bytes in the string. */
    const char *typeStr,	/* The name of the type of thing we are
				 * parsing, for error messages. */
    const char *typeCode,	/* The type code for thing we are parsing, for
				 * error messages. */
    const char **elementPtr,	/* Where to put address of first significant
				 * character in first element. */
    const char **nextPtr,	/* Fill in with location of character just
				 * after all white space following end of
				 * argument (next arg or end of list/dict). */
    int *sizePtr,		/* If non-zero, fill in with size of
				 * element. */
    int *literalPtr)		/* If non-zero, fill in with non-zero/zero to
				 * indicate that the substring of *sizePtr
				 * bytes starting at **elementPtr is/is not
				 * the literal list/dict element and therefore
				 * does not/does require a call to
				 * TclCopyAndCollapse() by the caller. */
{
    const char *p = string;
    const char *elemStart;	/* Points to first byte of first element. */
    const char *limit;		/* Points just after list/dict's last byte. */
    int openBraces = 0;		/* Brace nesting level during parse. */
    int inQuotes = 0;
    int size = 0;		/* lint. */
    int numChars;
    int literal = 1;
    const char *p2;

    /*
     * Skim off leading white space and check for an opening brace or quote.
     * We treat embedded NULLs in the list/dict as bytes belonging to a list
     * element (or dictionary key or value).
     */

    limit = (string + stringLength);
    while ((p < limit) && (TclIsSpaceProc(*p))) {
	p++;
    }
    if (p == limit) {		/* no element found */
	elemStart = limit;
	goto done;
    }

    if (*p == '{') {
	openBraces = 1;
	p++;
    } else if (*p == '"') {
	inQuotes = 1;
	p++;
    }
    elemStart = p;

    /*
     * Find element's end (a space, close brace, or the end of the string).
     */

    while (p < limit) {
	switch (*p) {
	    /*
	     * Open brace: don't treat specially unless the element is in
	     * braces. In this case, keep a nesting count.
	     */

	case '{':
	    if (openBraces != 0) {
		openBraces++;
	    }
	    break;

	    /*
	     * Close brace: if element is in braces, keep nesting count and
	     * quit when the last close brace is seen.
	     */

	case '}':
	    if (openBraces > 1) {
		openBraces--;
	    } else if (openBraces == 1) {
		size = (p - elemStart);
		p++;
		if ((p >= limit) || TclIsSpaceProc(*p)) {
		    goto done;
		}

		/*
		 * Garbage after the closing brace; return an error.
		 */

		if (interp != NULL) {
		    p2 = p;
		    while ((p2 < limit) && (!TclIsSpaceProc(*p2))
			    && (p2 < p+20)) {
			p2++;
		    }
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "%s element in braces followed by \"%.*s\" "
			    "instead of space", typeStr, (int) (p2-p), p));
		    Tcl_SetErrorCode(interp, "TCL", "VALUE", typeCode, "JUNK",
			    NULL);
		}
		return TCL_ERROR;
	    }
	    break;

	    /*
	     * Backslash: skip over everything up to the end of the backslash
	     * sequence.
	     */

	case '\\':
	    if (openBraces == 0) {
		/*
		 * A backslash sequence not within a brace quoted element
		 * means the value of the element is different from the
		 * substring we are parsing. A call to TclCopyAndCollapse() is
		 * needed to produce the element value. Inform the caller.
		 */

		literal = 0;
	    }
	    TclParseBackslash(p, limit - p, &numChars, NULL);
	    p += (numChars - 1);
	    break;

	    /*
	     * Space: ignore if element is in braces or quotes; otherwise
	     * terminate element.
	     */

	case ' ':
	case '\f':
	case '\n':
	case '\r':
	case '\t':
	case '\v':
	    if ((openBraces == 0) && !inQuotes) {
		size = (p - elemStart);
		goto done;
	    }
	    break;

	    /*
	     * Double-quote: if element is in quotes then terminate it.
	     */

	case '"':
	    if (inQuotes) {
		size = (p - elemStart);
		p++;
		if ((p >= limit) || TclIsSpaceProc(*p)) {
		    goto done;
		}

		/*
		 * Garbage after the closing quote; return an error.
		 */

		if (interp != NULL) {
		    p2 = p;
		    while ((p2 < limit) && (!TclIsSpaceProc(*p2))
			    && (p2 < p+20)) {
			p2++;
		    }
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			    "%s element in quotes followed by \"%.*s\" "
			    "instead of space", typeStr, (int) (p2-p), p));
		    Tcl_SetErrorCode(interp, "TCL", "VALUE", typeCode, "JUNK",
			    NULL);
		}
		return TCL_ERROR;
	    }
	    break;
	}
	p++;
    }

    /*
     * End of list/dict: terminate element.
     */

    if (p == limit) {
	if (openBraces != 0) {
	    if (interp != NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"unmatched open brace in %s", typeStr));
		Tcl_SetErrorCode(interp, "TCL", "VALUE", typeCode, "BRACE",
			NULL);
	    }
	    return TCL_ERROR;
	} else if (inQuotes) {
	    if (interp != NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"unmatched open quote in %s", typeStr));
		Tcl_SetErrorCode(interp, "TCL", "VALUE", typeCode, "QUOTE",
			NULL);
	    }
	    return TCL_ERROR;
	}
	size = (p - elemStart);
    }

  done:
    while ((p < limit) && (TclIsSpaceProc(*p))) {
	p++;
    }
    *elementPtr = elemStart;
    *nextPtr = p;
    if (sizePtr != 0) {
	*sizePtr = size;
    }
    if (literalPtr != 0) {
	*literalPtr = literal;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCopyAndCollapse --
 *
 *	Copy a string and substitute all backslash escape sequences
 *
 * Results:
 *	Count bytes get copied from src to dst. Along the way, backslash
 *	sequences are substituted in the copy. After scanning count bytes from
 *	src, a null character is placed at the end of dst. Returns the number
 *	of bytes that got written to dst.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclCopyAndCollapse(
    int count,			/* Number of byte to copy from src. */
    const char *src,		/* Copy from here... */
    char *dst)			/* ... to here. */
{
    int newCount = 0;

    while (count > 0) {
	char c = *src;

	if (c == '\\') {
	    int numRead;
	    int backslashCount = TclParseBackslash(src, count, &numRead, dst);

	    dst += backslashCount;
	    newCount += backslashCount;
	    src += numRead;
	    count -= numRead;
	} else {
	    *dst = c;
	    dst++;
	    newCount++;
	    src++;
	    count--;
	}
    }
    *dst = 0;
    return newCount;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SplitList --
 *
 *	Splits a list up into its constituent fields.
 *
 * Results
 *	The return value is normally TCL_OK, which means that the list was
 *	successfully split up. If TCL_ERROR is returned, it means that "list"
 *	didn't have proper list structure; the interp's result will contain a
 *	more detailed error message.
 *
 *	*argvPtr will be filled in with the address of an array whose elements
 *	point to the elements of list, in order. *argcPtr will get filled in
 *	with the number of valid elements in the array. A single block of
 *	memory is dynamically allocated to hold both the argv array and a copy
 *	of the list (with backslashes and braces removed in the standard way).
 *	The caller must eventually free this memory by calling free() on
 *	*argvPtr. Note: *argvPtr and *argcPtr are only modified if the
 *	function returns normally.
 *
 * Side effects:
 *	Memory is allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SplitList(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. If
				 * NULL, no error message is left. */
    const char *list,		/* Pointer to string with list structure. */
    int *argcPtr,		/* Pointer to location to fill in with the
				 * number of elements in the list. */
    const char ***argvPtr)	/* Pointer to place to store pointer to array
				 * of pointers to list elements. */
{
    const char **argv, *end, *element;
    char *p;
    int length, size, i, result, elSize;

    /*
     * Allocate enough space to work in. A (const char *) for each (possible)
     * list element plus one more for terminating NULL, plus as many bytes as
     * in the original string value, plus one more for a terminating '\0'.
     * Space used to hold element separating white space in the original
     * string gets re-purposed to hold '\0' characters in the argv array.
     */

    size = TclMaxListLength(list, -1, &end) + 1;
    length = end - list;
    argv = ckalloc((size * sizeof(char *)) + length + 1);

    for (i = 0, p = ((char *) argv) + size*sizeof(char *);
	    *list != 0;  i++) {
	const char *prevList = list;
	int literal;

	result = TclFindElement(interp, list, length, &element, &list,
		&elSize, &literal);
	length -= (list - prevList);
	if (result != TCL_OK) {
	    ckfree(argv);
	    return result;
	}
	if (*element == 0) {
	    break;
	}
	if (i >= size) {
	    ckfree(argv);
	    if (interp != NULL) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"internal error in Tcl_SplitList", -1));
		Tcl_SetErrorCode(interp, "TCL", "INTERNAL", "Tcl_SplitList",
			NULL);
	    }
	    return TCL_ERROR;
	}
	argv[i] = p;
	if (literal) {
	    memcpy(p, element, (size_t) elSize);
	    p += elSize;
	    *p = 0;
	    p++;
	} else {
	    p += 1 + TclCopyAndCollapse(elSize, element, p);
	}
    }

    argv[i] = NULL;
    *argvPtr = argv;
    *argcPtr = i;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ScanElement --
 *
 *	This function is a companion function to Tcl_ConvertElement. It scans
 *	a string to see what needs to be done to it (e.g. add backslashes or
 *	enclosing braces) to make the string into a valid Tcl list element.
 *
 * Results:
 *	The return value is an overestimate of the number of bytes that will
 *	be needed by Tcl_ConvertElement to produce a valid list element from
 *	src. The word at *flagPtr is filled in with a value needed by
 *	Tcl_ConvertElement when doing the actual conversion.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ScanElement(
    register const char *src,	/* String to convert to list element. */
    register int *flagPtr)	/* Where to store information to guide
				 * Tcl_ConvertCountedElement. */
{
    return Tcl_ScanCountedElement(src, -1, flagPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ScanCountedElement --
 *
 *	This function is a companion function to Tcl_ConvertCountedElement. It
 *	scans a string to see what needs to be done to it (e.g. add
 *	backslashes or enclosing braces) to make the string into a valid Tcl
 *	list element. If length is -1, then the string is scanned from src up
 *	to the first null byte.
 *
 * Results:
 *	The return value is an overestimate of the number of bytes that will
 *	be needed by Tcl_ConvertCountedElement to produce a valid list element
 *	from src. The word at *flagPtr is filled in with a value needed by
 *	Tcl_ConvertCountedElement when doing the actual conversion.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ScanCountedElement(
    const char *src,		/* String to convert to Tcl list element. */
    int length,			/* Number of bytes in src, or -1. */
    int *flagPtr)		/* Where to store information to guide
				 * Tcl_ConvertElement. */
{
    char flags = CONVERT_ANY;
    int numBytes = TclScanElement(src, length, &flags);

    *flagPtr = flags;
    return numBytes;
}

/*
 *----------------------------------------------------------------------
 *
 * TclScanElement --
 *
 *	This function is a companion function to TclConvertElement. It scans a
 *	string to see what needs to be done to it (e.g. add backslashes or
 *	enclosing braces) to make the string into a valid Tcl list element. If
 *	length is -1, then the string is scanned from src up to the first null
 *	byte. A NULL value for src is treated as an empty string. The incoming
 *	value of *flagPtr is a report from the caller what additional flags it
 *	will pass to TclConvertElement().
 *
 * Results:
 *	The recommended formatting mode for the element is determined and a
 *	value is written to *flagPtr indicating that recommendation. This
 *	recommendation is combined with the incoming flag values in *flagPtr
 *	set by the caller to determine how many bytes will be needed by
 *	TclConvertElement() in which to write the formatted element following
 *	the recommendation modified by the flag values. This number of bytes
 *	is the return value of the routine.  In some situations it may be an
 *	overestimate, but so long as the caller passes the same flags to
 *	TclConvertElement(), it will be large enough.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclScanElement(
    const char *src,		/* String to convert to Tcl list element. */
    int length,			/* Number of bytes in src, or -1. */
    char *flagPtr)		/* Where to store information to guide
				 * Tcl_ConvertElement. */
{
    const char *p = src;
    int nestingLevel = 0;	/* Brace nesting count */
    int forbidNone = 0;		/* Do not permit CONVERT_NONE mode. Something
				 * needs protection or escape. */
    int requireEscape = 0;	/* Force use of CONVERT_ESCAPE mode.  For some
				 * reason bare or brace-quoted form fails. */
    int extra = 0;		/* Count of number of extra bytes needed for
				 * formatted element, assuming we use escape
				 * sequences in formatting. */
    int bytesNeeded;		/* Buffer length computed to complete the
				 * element formatting in the selected mode. */
#if COMPAT
    int preferEscape = 0;	/* Use preferences to track whether to use */
    int preferBrace = 0;	/* CONVERT_MASK mode. */
    int braceCount = 0;		/* Count of all braces '{' '}' seen. */
#endif /* COMPAT */

    if ((p == NULL) || (length == 0) || ((*p == '\0') && (length == -1))) {
	/*
	 * Empty string element must be brace quoted.
	 */

	*flagPtr = CONVERT_BRACE;
	return 2;
    }

#if COMPAT
    /*
     * We have an established history in TclConvertElement() when quoting
     * because of a leading hash character to force what would be the
     * CONVERT_MASK mode into the CONVERT_BRACE mode. That is, we format
     * the element #{a"b} like this:
     *			{#{a"b}}
     * and not like this:
     *			\#{a\"b}
     * This is inconsistent with [list x{a"b}], but we will not change that now.
     * Set that preference here so that we compute a tight size requirement.
     */
    if ((*src == '#') && !(*flagPtr & TCL_DONT_QUOTE_HASH)) {
	preferBrace = 1;
    }
#endif

    if ((*p == '{') || (*p == '"')) {
	/*
	 * Must escape or protect so leading character of value is not
	 * misinterpreted as list element delimiting syntax.
	 */

	forbidNone = 1;
#if COMPAT
	preferBrace = 1;
#endif /* COMPAT */
    }

    while (length) {
      if (CHAR_TYPE(*p) != TYPE_NORMAL) {
	switch (*p) {
	case '{':	/* TYPE_BRACE */
#if COMPAT
	    braceCount++;
#endif /* COMPAT */
	    extra++;				/* Escape '{' => '\{' */
	    nestingLevel++;
	    break;
	case '}':	/* TYPE_BRACE */
#if COMPAT
	    braceCount++;
#endif /* COMPAT */
	    extra++;				/* Escape '}' => '\}' */
	    nestingLevel--;
	    if (nestingLevel < 0) {
		/*
		 * Unbalanced braces!  Cannot format with brace quoting.
		 */

		requireEscape = 1;
	    }
	    break;
	case ']':	/* TYPE_CLOSE_BRACK */
	case '"':	/* TYPE_SPACE */
#if COMPAT
	    forbidNone = 1;
	    extra++;		/* Escapes all just prepend a backslash */
	    preferEscape = 1;
	    break;
#else
	    /* FLOW THROUGH */
#endif /* COMPAT */
	case '[':	/* TYPE_SUBS */
	case '$':	/* TYPE_SUBS */
	case ';':	/* TYPE_COMMAND_END */
	case ' ':	/* TYPE_SPACE */
	case '\f':	/* TYPE_SPACE */
	case '\n':	/* TYPE_COMMAND_END */
	case '\r':	/* TYPE_SPACE */
	case '\t':	/* TYPE_SPACE */
	case '\v':	/* TYPE_SPACE */
	    forbidNone = 1;
	    extra++;		/* Escape sequences all one byte longer. */
#if COMPAT
	    preferBrace = 1;
#endif /* COMPAT */
	    break;
	case '\\':	/* TYPE_SUBS */
	    extra++;				/* Escape '\' => '\\' */
	    if ((length == 1) || ((length == -1) && (p[1] == '\0'))) {
		/*
		 * Final backslash. Cannot format with brace quoting.
		 */

		requireEscape = 1;
		break;
	    }
	    if (p[1] == '\n') {
		extra++;	/* Escape newline => '\n', one byte longer */

		/*
		 * Backslash newline sequence.  Brace quoting not permitted.
		 */

		requireEscape = 1;
		length -= (length > 0);
		p++;
		break;
	    }
	    if ((p[1] == '{') || (p[1] == '}') || (p[1] == '\\')) {
		extra++;	/* Escape sequences all one byte longer. */
		length -= (length > 0);
		p++;
	    }
	    forbidNone = 1;
#if COMPAT
	    preferBrace = 1;
#endif /* COMPAT */
	    break;
	case '\0':	/* TYPE_SUBS */
	    if (length == -1) {
		goto endOfString;
	    }
	    /* TODO: Panic on improper encoding? */
	    break;
	}
      }
	length -= (length > 0);
	p++;
    }

  endOfString:
    if (nestingLevel != 0) {
	/*
	 * Unbalanced braces!  Cannot format with brace quoting.
	 */

	requireEscape = 1;
    }

    /*
     * We need at least as many bytes as are in the element value...
     */

    bytesNeeded = p - src;

    if (requireEscape) {
	/*
	 * We must use escape sequences.  Add all the extra bytes needed to
	 * have room to create them.
	 */

	bytesNeeded += extra;

	/*
	 * Make room to escape leading #, if needed.
	 */

	if ((*src == '#') && !(*flagPtr & TCL_DONT_QUOTE_HASH)) {
	    bytesNeeded++;
	}
	*flagPtr = CONVERT_ESCAPE;
	goto overflowCheck;
    }
    if (*flagPtr & CONVERT_ANY) {
	/*
	 * The caller has not let us know what flags it will pass to
	 * TclConvertElement() so compute the max size we might need for any
	 * possible choice.  Normally the formatting using escape sequences is
	 * the longer one, and a minimum "extra" value of 2 makes sure we
	 * don't request too small a buffer in those edge cases where that's
	 * not true.
	 */

	if (extra < 2) {
	    extra = 2;
	}
	*flagPtr &= ~CONVERT_ANY;
	*flagPtr |= TCL_DONT_USE_BRACES;
    }
    if (forbidNone) {
	/*
	 * We must request some form of quoting of escaping...
	 */

#if COMPAT
	if (preferEscape && !preferBrace) {
	    /*
	     * If we are quoting solely due to ] or internal " characters use
	     * the CONVERT_MASK mode where we escape all special characters
	     * except for braces. "extra" counted space needed to escape
	     * braces too, so substract "braceCount" to get our actual needs.
	     */

	    bytesNeeded += (extra - braceCount);
	    /* Make room to escape leading #, if needed. */
	    if ((*src == '#') && !(*flagPtr & TCL_DONT_QUOTE_HASH)) {
		bytesNeeded++;
	    }

	    /*
	     * If the caller reports it will direct TclConvertElement() to
	     * use full escapes on the element, add back the bytes needed to
	     * escape the braces.
	     */

	    if (*flagPtr & TCL_DONT_USE_BRACES) {
		bytesNeeded += braceCount;
	    }
	    *flagPtr = CONVERT_MASK;
	    goto overflowCheck;
	}
#endif /* COMPAT */
	if (*flagPtr & TCL_DONT_USE_BRACES) {
	    /*
	     * If the caller reports it will direct TclConvertElement() to
	     * use escapes, add the extra bytes needed to have room for them.
	     */

	    bytesNeeded += extra;

	    /*
	     * Make room to escape leading #, if needed.
	     */

	    if ((*src == '#') && !(*flagPtr & TCL_DONT_QUOTE_HASH)) {
		bytesNeeded++;
	    }
	} else {
	    /*
	     * Add 2 bytes for room for the enclosing braces.
	     */

	    bytesNeeded += 2;
	}
	*flagPtr = CONVERT_BRACE;
	goto overflowCheck;
    }

    /*
     * So far, no need to quote or escape anything.
     */

    if ((*src == '#') && !(*flagPtr & TCL_DONT_QUOTE_HASH)) {
	/*
	 * If we need to quote a leading #, make room to enclose in braces.
	 */

	bytesNeeded += 2;
    }
    *flagPtr = CONVERT_NONE;

  overflowCheck:
    if (bytesNeeded < 0) {
	Tcl_Panic("TclScanElement: string length overflow");
    }
    return bytesNeeded;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConvertElement --
 *
 *	This is a companion function to Tcl_ScanElement. Given the information
 *	produced by Tcl_ScanElement, this function converts a string to a list
 *	element equal to that string.
 *
 * Results:
 *	Information is copied to *dst in the form of a list element identical
 *	to src (i.e. if Tcl_SplitList is applied to dst it will produce a
 *	string identical to src). The return value is a count of the number of
 *	characters copied (not including the terminating NULL character).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ConvertElement(
    register const char *src,	/* Source information for list element. */
    register char *dst,		/* Place to put list-ified element. */
    register int flags)		/* Flags produced by Tcl_ScanElement. */
{
    return Tcl_ConvertCountedElement(src, -1, dst, flags);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConvertCountedElement --
 *
 *	This is a companion function to Tcl_ScanCountedElement. Given the
 *	information produced by Tcl_ScanCountedElement, this function converts
 *	a string to a list element equal to that string.
 *
 * Results:
 *	Information is copied to *dst in the form of a list element identical
 *	to src (i.e. if Tcl_SplitList is applied to dst it will produce a
 *	string identical to src). The return value is a count of the number of
 *	characters copied (not including the terminating NULL character).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ConvertCountedElement(
    register const char *src,	/* Source information for list element. */
    int length,			/* Number of bytes in src, or -1. */
    char *dst,			/* Place to put list-ified element. */
    int flags)			/* Flags produced by Tcl_ScanElement. */
{
    int numBytes = TclConvertElement(src, length, dst, flags);
    dst[numBytes] = '\0';
    return numBytes;
}

/*
 *----------------------------------------------------------------------
 *
 * TclConvertElement --
 *
 *	This is a companion function to TclScanElement. Given the information
 *	produced by TclScanElement, this function converts a string to a list
 *	element equal to that string.
 *
 * Results:
 *	Information is copied to *dst in the form of a list element identical
 *	to src (i.e. if Tcl_SplitList is applied to dst it will produce a
 *	string identical to src). The return value is a count of the number of
 *	characters copied (not including the terminating NULL character).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclConvertElement(
    register const char *src,	/* Source information for list element. */
    int length,			/* Number of bytes in src, or -1. */
    char *dst,			/* Place to put list-ified element. */
    int flags)			/* Flags produced by Tcl_ScanElement. */
{
    int conversion = flags & CONVERT_MASK;
    char *p = dst;

    /*
     * Let the caller demand we use escape sequences rather than braces.
     */

    if ((flags & TCL_DONT_USE_BRACES) && (conversion & CONVERT_BRACE)) {
	conversion = CONVERT_ESCAPE;
    }

    /*
     * No matter what the caller demands, empty string must be braced!
     */

    if ((src == NULL) || (length == 0) || (*src == '\0' && length == -1)) {
	src = tclEmptyStringRep;
	length = 0;
	conversion = CONVERT_BRACE;
    }

    /*
     * Escape leading hash as needed and requested.
     */

    if ((*src == '#') && !(flags & TCL_DONT_QUOTE_HASH)) {
	if (conversion == CONVERT_ESCAPE) {
	    p[0] = '\\';
	    p[1] = '#';
	    p += 2;
	    src++;
	    length -= (length > 0);
	} else {
	    conversion = CONVERT_BRACE;
	}
    }

    /*
     * No escape or quoting needed.  Copy the literal string value.
     */

    if (conversion == CONVERT_NONE) {
	if (length == -1) {
	    /* TODO: INT_MAX overflow? */
	    while (*src) {
		*p++ = *src++;
	    }
	    return p - dst;
	} else {
	    memcpy(dst, src, length);
	    return length;
	}
    }

    /*
     * Formatted string is original string enclosed in braces.
     */

    if (conversion == CONVERT_BRACE) {
	*p = '{';
	p++;
	if (length == -1) {
	    /* TODO: INT_MAX overflow? */
	    while (*src) {
		*p++ = *src++;
	    }
	} else {
	    memcpy(p, src, length);
	    p += length;
	}
	*p = '}';
	p++;
	return p - dst;
    }

    /* conversion == CONVERT_ESCAPE or CONVERT_MASK */

    /*
     * Formatted string is original string converted to escape sequences.
     */

    for ( ; length; src++, length -= (length > 0)) {
	switch (*src) {
	case ']':
	case '[':
	case '$':
	case ';':
	case ' ':
	case '\\':
	case '"':
	    *p = '\\';
	    p++;
	    break;
	case '{':
	case '}':
#if COMPAT
	    if (conversion == CONVERT_ESCAPE)
#endif /* COMPAT */
	    {
		*p = '\\';
		p++;
	    }
	    break;
	case '\f':
	    *p = '\\';
	    p++;
	    *p = 'f';
	    p++;
	    continue;
	case '\n':
	    *p = '\\';
	    p++;
	    *p = 'n';
	    p++;
	    continue;
	case '\r':
	    *p = '\\';
	    p++;
	    *p = 'r';
	    p++;
	    continue;
	case '\t':
	    *p = '\\';
	    p++;
	    *p = 't';
	    p++;
	    continue;
	case '\v':
	    *p = '\\';
	    p++;
	    *p = 'v';
	    p++;
	    continue;
	case '\0':
	    if (length == -1) {
		return p - dst;
	    }

	    /*
	     * If we reach this point, there's an embedded NULL in the string
	     * range being processed, which should not happen when the
	     * encoding rules for Tcl strings are properly followed.  If the
	     * day ever comes when we stop tolerating such things, this is
	     * where to put the Tcl_Panic().
	     */

	    break;
	}
	*p = *src;
	p++;
    }
    return p - dst;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Merge --
 *
 *	Given a collection of strings, merge them together into a single
 *	string that has proper Tcl list structured (i.e. Tcl_SplitList may be
 *	used to retrieve strings equal to the original elements, and Tcl_Eval
 *	will parse the string back into its original elements).
 *
 * Results:
 *	The return value is the address of a dynamically-allocated string
 *	containing the merged list.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_Merge(
    int argc,			/* How many strings to merge. */
    const char *const *argv)	/* Array of string values. */
{
#define LOCAL_SIZE 64
    char localFlags[LOCAL_SIZE], *flagPtr = NULL;
    int i, bytesNeeded = 0;
    char *result, *dst;

    /*
     * Handle empty list case first, so logic of the general case can be
     * simpler.
     */

    if (argc == 0) {
	result = ckalloc(1);
	result[0] = '\0';
	return result;
    }

    /*
     * Pass 1: estimate space, gather flags.
     */

    if (argc <= LOCAL_SIZE) {
	flagPtr = localFlags;
    } else {
	flagPtr = ckalloc(argc);
    }
    for (i = 0; i < argc; i++) {
	flagPtr[i] = ( i ? TCL_DONT_QUOTE_HASH : 0 );
	bytesNeeded += TclScanElement(argv[i], -1, &flagPtr[i]);
	if (bytesNeeded < 0) {
	    Tcl_Panic("max size for a Tcl value (%d bytes) exceeded", INT_MAX);
	}
    }
    if (bytesNeeded > INT_MAX - argc + 1) {
	Tcl_Panic("max size for a Tcl value (%d bytes) exceeded", INT_MAX);
    }
    bytesNeeded += argc;

    /*
     * Pass two: copy into the result area.
     */

    result = ckalloc(bytesNeeded);
    dst = result;
    for (i = 0; i < argc; i++) {
	flagPtr[i] |= ( i ? TCL_DONT_QUOTE_HASH : 0 );
	dst += TclConvertElement(argv[i], -1, dst, flagPtr[i]);
	*dst = ' ';
	dst++;
    }
    dst[-1] = 0;

    if (flagPtr != localFlags) {
	ckfree(flagPtr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Backslash --
 *
 *	Figure out how to handle a backslash sequence.
 *
 * Results:
 *	The return value is the character that should be substituted in place
 *	of the backslash sequence that starts at src. If readPtr isn't NULL
 *	then it is filled in with a count of the number of characters in the
 *	backslash sequence.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char
Tcl_Backslash(
    const char *src,		/* Points to the backslash character of a
				 * backslash sequence. */
    int *readPtr)		/* Fill in with number of characters read from
				 * src, unless NULL. */
{
    char buf[TCL_UTF_MAX];
    Tcl_UniChar ch = 0;

    Tcl_UtfBackslash(src, readPtr, buf);
    TclUtfToUniChar(buf, &ch);
    return (char) ch;
}

/*
 *----------------------------------------------------------------------
 *
 * UtfWellFormedEnd --
 *	Checks the end of utf string is malformed, if yes - wraps bytes
 *	to the given buffer (as well-formed NTS string).  The buffer
 *	argument should be initialized by the caller and ready to use.
 *
 * Results:
 *	The bytes with well-formed end of the string.
 *
 * Side effects:
 *	Buffer (DString) may be allocated, so must be released.
 *
 *----------------------------------------------------------------------
 */

static inline const char*
UtfWellFormedEnd(
    Tcl_DString *buffer,	/* Buffer used to hold well-formed string. */
    const char *bytes,		/* Pointer to the beginning of the string. */
    int length)			/* Length of the string. */
{
    const char *l = bytes + length;
    const char *p = Tcl_UtfPrev(l, bytes);

    if (Tcl_UtfCharComplete(p, l - p)) {
	return bytes;
    }
    /* 
     * Malformed utf-8 end, be sure we've NTS to safe compare of end-character,
     * avoid segfault by access violation out of range.
     */
    Tcl_DStringAppend(buffer, bytes, length);
    return Tcl_DStringValue(buffer);
}
/*
 *----------------------------------------------------------------------
 *
 * TclTrimRight --
 *	Takes two counted strings in the Tcl encoding.  Conceptually
 *	finds the sub string (offset) to trim from the right side of the
 *	first string all characters found in the second string.
 *
 * Results:
 *	The number of bytes to be removed from the end of the string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static inline int
TrimRight(
    const char *bytes,		/* String to be trimmed... */
    int numBytes,		/* ...and its length in bytes */
    const char *trim,		/* String of trim characters... */
    int numTrim)		/* ...and its length in bytes */
{
    const char *p = bytes + numBytes;
    int pInc;

    /*
     * Outer loop: iterate over string to be trimmed.
     */

    do {
	Tcl_UniChar ch1;
	const char *q = trim;
	int bytesLeft = numTrim;

	p = Tcl_UtfPrev(p, bytes);
 	pInc = TclUtfToUniChar(p, &ch1);

	/*
	 * Inner loop: scan trim string for match to current character.
	 */

	do {
	    Tcl_UniChar ch2;
	    int qInc = TclUtfToUniChar(q, &ch2);

	    if (ch1 == ch2) {
		break;
	    }

	    q += qInc;
	    bytesLeft -= qInc;
	} while (bytesLeft);

	if (bytesLeft == 0) {
	    /*
	     * No match; trim task done; *p is last non-trimmed char.
	     */

	    p += pInc;
	    break;
	}
    } while (p > bytes);

    return numBytes - (p - bytes);
}

int
TclTrimRight(
    const char *bytes,	/* String to be trimmed... */
    int numBytes,	/* ...and its length in bytes */
    const char *trim,	/* String of trim characters... */
    int numTrim)	/* ...and its length in bytes */
{
    int res;
    Tcl_DString bytesBuf, trimBuf;

    /* Empty strings -> nothing to do */
    if ((numBytes == 0) || (numTrim == 0)) {
	return 0;
    }

    Tcl_DStringInit(&bytesBuf);
    Tcl_DStringInit(&trimBuf);
    bytes = UtfWellFormedEnd(&bytesBuf, bytes, numBytes);
    trim = UtfWellFormedEnd(&trimBuf, trim, numTrim);

    res = TrimRight(bytes, numBytes, trim, numTrim);
    if (res > numBytes) {
	res = numBytes;
    }

    Tcl_DStringFree(&bytesBuf);
    Tcl_DStringFree(&trimBuf);

    return res;
}

/*
 *----------------------------------------------------------------------
 *
 * TclTrimLeft --
 *
 *	Takes two counted strings in the Tcl encoding.  Conceptually
 *	finds the sub string (offset) to trim from the left side of the
 *	first string all characters found in the second string.
 *
 * Results:
 *	The number of bytes to be removed from the start of the string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static inline int
TrimLeft(
    const char *bytes,		/* String to be trimmed... */
    int numBytes,		/* ...and its length in bytes */
    const char *trim,		/* String of trim characters... */
    int numTrim)		/* ...and its length in bytes */
{
    const char *p = bytes;

    /*
     * Outer loop: iterate over string to be trimmed.
     */

    do {
	Tcl_UniChar ch1;
	int pInc = TclUtfToUniChar(p, &ch1);
	const char *q = trim;
	int bytesLeft = numTrim;

	/*
	 * Inner loop: scan trim string for match to current character.
	 */

	do {
	    Tcl_UniChar ch2;
	    int qInc = TclUtfToUniChar(q, &ch2);

	    if (ch1 == ch2) {
		break;
	    }

	    q += qInc;
	    bytesLeft -= qInc;
	} while (bytesLeft);

	if (bytesLeft == 0) {
	    /*
	     * No match; trim task done; *p is first non-trimmed char.
	     */

	    break;
	}

	p += pInc;
	numBytes -= pInc;
    } while (numBytes > 0);

    return p - bytes;
}

int
TclTrimLeft(
    const char *bytes,	/* String to be trimmed... */
    int numBytes,	/* ...and its length in bytes */
    const char *trim,	/* String of trim characters... */
    int numTrim)	/* ...and its length in bytes */
{
    int res;
    Tcl_DString bytesBuf, trimBuf;

    /* Empty strings -> nothing to do */
    if ((numBytes == 0) || (numTrim == 0)) {
	return 0;
    }

    Tcl_DStringInit(&bytesBuf);
    Tcl_DStringInit(&trimBuf);
    bytes = UtfWellFormedEnd(&bytesBuf, bytes, numBytes);
    trim = UtfWellFormedEnd(&trimBuf, trim, numTrim);

    res = TrimLeft(bytes, numBytes, trim, numTrim);
    if (res > numBytes) {
	res = numBytes;
    }

    Tcl_DStringFree(&bytesBuf);
    Tcl_DStringFree(&trimBuf);

    return res;
}

/*
 *----------------------------------------------------------------------
 *
 * TclTrim --
 *	Finds the sub string (offset) to trim from both sides of the
 *	first string all characters found in the second string.
 *
 * Results:
 *	The number of bytes to be removed from the start of the string
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclTrim(
    const char *bytes,	/* String to be trimmed... */
    int numBytes,	/* ...and its length in bytes */
    const char *trim,	/* String of trim characters... */
    int numTrim,	/* ...and its length in bytes */
    int *trimRight)		/* Offset from the end of the string. */
{
    int trimLeft;
    Tcl_DString bytesBuf, trimBuf;

    *trimRight = 0;
    /* Empty strings -> nothing to do */
    if ((numBytes == 0) || (numTrim == 0)) {
	return 0;
    }

    Tcl_DStringInit(&bytesBuf);
    Tcl_DStringInit(&trimBuf);
    bytes = UtfWellFormedEnd(&bytesBuf, bytes, numBytes);
    trim = UtfWellFormedEnd(&trimBuf, trim, numTrim);

    trimLeft = TrimLeft(bytes, numBytes, trim, numTrim);
    if (trimLeft > numBytes) {
	trimLeft = numBytes;
    }
    numBytes -= trimLeft;
    /* have to trim yet (first char was already verified within TrimLeft) */
    if (numBytes > 1) {
	bytes += trimLeft;
	*trimRight = TrimRight(bytes, numBytes, trim, numTrim);
	if (*trimRight > numBytes) {
	    *trimRight = numBytes;
	}
    }

    Tcl_DStringFree(&bytesBuf);
    Tcl_DStringFree(&trimBuf);

    return trimLeft;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Concat --
 *
 *	Concatenate a set of strings into a single large string.
 *
 * Results:
 *	The return value is dynamically-allocated string containing a
 *	concatenation of all the strings in argv, with spaces between the
 *	original argv elements.
 *
 * Side effects:
 *	Memory is allocated for the result; the caller is responsible for
 *	freeing the memory.
 *
 *----------------------------------------------------------------------
 */

/* The whitespace characters trimmed during [concat] operations */
#define CONCAT_WS_SIZE (int) (sizeof(CONCAT_TRIM_SET "") - 1)

char *
Tcl_Concat(
    int argc,			/* Number of strings to concatenate. */
    const char *const *argv)	/* Array of strings to concatenate. */
{
    int i, needSpace = 0, bytesNeeded = 0;
    char *result, *p;

    /*
     * Dispose of the empty result corner case first to simplify later code.
     */

    if (argc == 0) {
	result = (char *) ckalloc(1);
	result[0] = '\0';
	return result;
    }

    /*
     * First allocate the result buffer at the size required.
     */

    for (i = 0;  i < argc;  i++) {
	bytesNeeded += strlen(argv[i]);
	if (bytesNeeded < 0) {
	    Tcl_Panic("Tcl_Concat: max size of Tcl value exceeded");
	}
    }
    if (bytesNeeded + argc - 1 < 0) {
	/*
	 * Panic test could be tighter, but not going to bother for this
	 * legacy routine.
	 */

	Tcl_Panic("Tcl_Concat: max size of Tcl value exceeded");
    }

    /*
     * All element bytes + (argc - 1) spaces + 1 terminating NULL.
     */

    result = ckalloc((unsigned) (bytesNeeded + argc));

    for (p = result, i = 0;  i < argc;  i++) {
	int triml, trimr, elemLength;
	const char *element;

	element = argv[i];
	elemLength = strlen(argv[i]);

	/* Trim away the leading/trailing whitespace. */
	triml = TclTrim(element, elemLength, CONCAT_TRIM_SET,
		CONCAT_WS_SIZE, &trimr);
	element += triml;
	elemLength -= triml + trimr;

	/* Do not permit trimming to expose a final backslash character. */
	elemLength += trimr && (element[elemLength - 1] == '\\');

	/*
	 * If we're left with empty element after trimming, do nothing.
	 */

	if (elemLength == 0) {
	    continue;
	}

	/*
	 * Append to the result with space if needed.
	 */

	if (needSpace) {
	    *p++ = ' ';
	}
	memcpy(p, element, (size_t) elemLength);
	p += elemLength;
	needSpace = 1;
    }
    *p = '\0';
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConcatObj --
 *
 *	Concatenate the strings from a set of objects into a single string
 *	object with spaces between the original strings.
 *
 * Results:
 *	The return value is a new string object containing a concatenation of
 *	the strings in objv. Its ref count is zero.
 *
 * Side effects:
 *	A new object is created.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_ConcatObj(
    int objc,			/* Number of objects to concatenate. */
    Tcl_Obj *const objv[])	/* Array of objects to concatenate. */
{
    int i, elemLength, needSpace = 0, bytesNeeded = 0;
    const char *element;
    Tcl_Obj *objPtr, *resPtr;

    /*
     * Check first to see if all the items are of list type or empty. If so,
     * we will concat them together as lists, and return a list object. This
     * is only valid when the lists are in canonical form.
     */

    for (i = 0;  i < objc;  i++) {
	int length;

	objPtr = objv[i];
	if (TclListObjIsCanonical(objPtr)) {
	    continue;
	}
	Tcl_GetStringFromObj(objPtr, &length);
	if (length > 0) {
	    break;
	}
    }
    if (i == objc) {
	resPtr = NULL;
	for (i = 0;  i < objc;  i++) {
	    objPtr = objv[i];
	    if (objPtr->bytes && objPtr->length == 0) {
		continue;
	    }
	    if (resPtr) {
		if (TCL_OK != Tcl_ListObjAppendList(NULL, resPtr, objPtr)) {
		    /* Abandon ship! */
		    Tcl_DecrRefCount(resPtr);
		    goto slow;
		}
	    } else {
		resPtr = TclListObjCopy(NULL, objPtr);
	    }
	}
	if (!resPtr) {
	    resPtr = Tcl_NewObj();
	}
	return resPtr;
    }

  slow:
    /*
     * Something cannot be determined to be safe, so build the concatenation
     * the slow way, using the string representations.
     *
     * First try to pre-allocate the size required.
     */

    for (i = 0;  i < objc;  i++) {
	element = TclGetStringFromObj(objv[i], &elemLength);
	bytesNeeded += elemLength;
	if (bytesNeeded < 0) {
	    break;
	}
    }

    /*
     * Does not matter if this fails, will simply try later to build up the
     * string with each Append reallocating as needed with the usual string
     * append algorithm.  When that fails it will report the error.
     */

    TclNewObj(resPtr);
    (void) Tcl_AttemptSetObjLength(resPtr, bytesNeeded + objc - 1);
    Tcl_SetObjLength(resPtr, 0);

    for (i = 0;  i < objc;  i++) {
	int triml, trimr;

	element = TclGetStringFromObj(objv[i], &elemLength);

	/* Trim away the leading/trailing whitespace. */
	triml = TclTrim(element, elemLength, CONCAT_TRIM_SET,
		CONCAT_WS_SIZE, &trimr);
	element += triml;
	elemLength -= triml + trimr;

	/* Do not permit trimming to expose a final backslash character. */
	elemLength += trimr && (element[elemLength - 1] == '\\');

	/*
	 * If we're left with empty element after trimming, do nothing.
	 */

	if (elemLength == 0) {
	    continue;
	}

	/*
	 * Append to the result with space if needed.
	 */

	if (needSpace) {
	    Tcl_AppendToObj(resPtr, " ", 1);
	}
	Tcl_AppendToObj(resPtr, element, elemLength);
	needSpace = 1;
    }
    return resPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_StringMatch --
 *
 *	See if a particular string matches a particular pattern.
 *
 * Results:
 *	The return value is 1 if string matches pattern, and 0 otherwise. The
 *	matching operation permits the following special characters in the
 *	pattern: *?\[] (see the manual entry for details on what these mean).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_StringMatch(
    const char *str,		/* String. */
    const char *pattern)	/* Pattern, which may contain special
				 * characters. */
{
    return Tcl_StringCaseMatch(str, pattern, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_StringCaseMatch --
 *
 *	See if a particular string matches a particular pattern. Allows case
 *	insensitivity.
 *
 * Results:
 *	The return value is 1 if string matches pattern, and 0 otherwise. The
 *	matching operation permits the following special characters in the
 *	pattern: *?\[] (see the manual entry for details on what these mean).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_StringCaseMatch(
    const char *str,		/* String. */
    const char *pattern,	/* Pattern, which may contain special
				 * characters. */
    int nocase)			/* 0 for case sensitive, 1 for insensitive */
{
    int p, charLen;
    const char *pstart = pattern;
    Tcl_UniChar ch1, ch2;

    while (1) {
	p = *pattern;

	/*
	 * See if we're at the end of both the pattern and the string. If so,
	 * we succeeded. If we're at the end of the pattern but not at the end
	 * of the string, we failed.
	 */

	if (p == '\0') {
	    return (*str == '\0');
	}
	if ((*str == '\0') && (p != '*')) {
	    return 0;
	}

	/*
	 * Check for a "*" as the next pattern character. It matches any
	 * substring. We handle this by calling ourselves recursively for each
	 * postfix of string, until either we match or we reach the end of the
	 * string.
	 */

	if (p == '*') {
	    /*
	     * Skip all successive *'s in the pattern
	     */

	    while (*(++pattern) == '*') {}
	    p = *pattern;
	    if (p == '\0') {
		return 1;
	    }

	    /*
	     * This is a special case optimization for single-byte utf.
	     */

	    if (UCHAR(*pattern) < 0x80) {
		ch2 = (Tcl_UniChar)
			(nocase ? tolower(UCHAR(*pattern)) : UCHAR(*pattern));
	    } else {
		Tcl_UtfToUniChar(pattern, &ch2);
		if (nocase) {
		    ch2 = Tcl_UniCharToLower(ch2);
		}
	    }

	    while (1) {
		/*
		 * Optimization for matching - cruise through the string
		 * quickly if the next char in the pattern isn't a special
		 * character
		 */

		if ((p != '[') && (p != '?') && (p != '\\')) {
		    if (nocase) {
			while (*str) {
			    charLen = TclUtfToUniChar(str, &ch1);
			    if (ch2==ch1 || ch2==Tcl_UniCharToLower(ch1)) {
				break;
			    }
			    str += charLen;
			}
		    } else {
			/*
			 * There's no point in trying to make this code
			 * shorter, as the number of bytes you want to compare
			 * each time is non-constant.
			 */

			while (*str) {
			    charLen = TclUtfToUniChar(str, &ch1);
			    if (ch2 == ch1) {
				break;
			    }
			    str += charLen;
			}
		    }
		}
		if (Tcl_StringCaseMatch(str, pattern, nocase)) {
		    return 1;
		}
		if (*str == '\0') {
		    return 0;
		}
		str += TclUtfToUniChar(str, &ch1);
	    }
	}

	/*
	 * Check for a "?" as the next pattern character. It matches any
	 * single character.
	 */

	if (p == '?') {
	    pattern++;
	    str += TclUtfToUniChar(str, &ch1);
	    continue;
	}

	/*
	 * Check for a "[" as the next pattern character. It is followed by a
	 * list of characters that are acceptable, or by a range (two
	 * characters separated by "-").
	 */

	if (p == '[') {
	    Tcl_UniChar startChar, endChar;

	    pattern++;
	    if (UCHAR(*str) < 0x80) {
		ch1 = (Tcl_UniChar)
			(nocase ? tolower(UCHAR(*str)) : UCHAR(*str));
		str++;
	    } else {
		str += Tcl_UtfToUniChar(str, &ch1);
		if (nocase) {
		    ch1 = Tcl_UniCharToLower(ch1);
		}
	    }
	    while (1) {
		if ((*pattern == ']') || (*pattern == '\0')) {
		    return 0;
		}
		if (UCHAR(*pattern) < 0x80) {
		    startChar = (Tcl_UniChar) (nocase
			    ? tolower(UCHAR(*pattern)) : UCHAR(*pattern));
		    pattern++;
		} else {
		    pattern += Tcl_UtfToUniChar(pattern, &startChar);
		    if (nocase) {
			startChar = Tcl_UniCharToLower(startChar);
		    }
		}
		if (*pattern == '-') {
		    pattern++;
		    if (*pattern == '\0') {
			return 0;
		    }
		    if (UCHAR(*pattern) < 0x80) {
			endChar = (Tcl_UniChar) (nocase
				? tolower(UCHAR(*pattern)) : UCHAR(*pattern));
			pattern++;
		    } else {
			pattern += Tcl_UtfToUniChar(pattern, &endChar);
			if (nocase) {
			    endChar = Tcl_UniCharToLower(endChar);
			}
		    }
		    if (((startChar <= ch1) && (ch1 <= endChar))
			    || ((endChar <= ch1) && (ch1 <= startChar))) {
			/*
			 * Matches ranges of form [a-z] or [z-a].
			 */

			break;
		    }
		} else if (startChar == ch1) {
		    break;
		}
	    }
	    while (*pattern != ']') {
		if (*pattern == '\0') {
		    pattern = Tcl_UtfPrev(pattern, pstart);
		    break;
		}
		pattern++;
	    }
	    pattern++;
	    continue;
	}

	/*
	 * If the next pattern character is '\', just strip off the '\' so we
	 * do exact matching on the character that follows.
	 */

	if (p == '\\') {
	    pattern++;
	    if (*pattern == '\0') {
		return 0;
	    }
	}

	/*
	 * There's no special character. Just make sure that the next bytes of
	 * each string match.
	 */

	str += TclUtfToUniChar(str, &ch1);
	pattern += TclUtfToUniChar(pattern, &ch2);
	if (nocase) {
	    if (Tcl_UniCharToLower(ch1) != Tcl_UniCharToLower(ch2)) {
		return 0;
	    }
	} else if (ch1 != ch2) {
	    return 0;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclByteArrayMatch --
 *
 *	See if a particular string matches a particular pattern.  Does not
 *	allow for case insensitivity.
 *	Parallels tclUtf.c:TclUniCharMatch, adjusted for char* and sans nocase.
 *
 * Results:
 *	The return value is 1 if string matches pattern, and 0 otherwise. The
 *	matching operation permits the following special characters in the
 *	pattern: *?\[] (see the manual entry for details on what these mean).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclByteArrayMatch(
    const unsigned char *string,/* String. */
    int strLen,			/* Length of String */
    const unsigned char *pattern,
				/* Pattern, which may contain special
				 * characters. */
    int ptnLen,			/* Length of Pattern */
    int flags)
{
    const unsigned char *stringEnd, *patternEnd;
    unsigned char p;

    stringEnd = string + strLen;
    patternEnd = pattern + ptnLen;

    while (1) {
	/*
	 * See if we're at the end of both the pattern and the string. If so,
	 * we succeeded. If we're at the end of the pattern but not at the end
	 * of the string, we failed.
	 */

	if (pattern == patternEnd) {
	    return (string == stringEnd);
	}
	p = *pattern;
	if ((string == stringEnd) && (p != '*')) {
	    return 0;
	}

	/*
	 * Check for a "*" as the next pattern character. It matches any
	 * substring. We handle this by skipping all the characters up to the
	 * next matching one in the pattern, and then calling ourselves
	 * recursively for each postfix of string, until either we match or we
	 * reach the end of the string.
	 */

	if (p == '*') {
	    /*
	     * Skip all successive *'s in the pattern.
	     */

	    while ((++pattern < patternEnd) && (*pattern == '*')) {
		/* empty body */
	    }
	    if (pattern == patternEnd) {
		return 1;
	    }
	    p = *pattern;
	    while (1) {
		/*
		 * Optimization for matching - cruise through the string
		 * quickly if the next char in the pattern isn't a special
		 * character.
		 */

		if ((p != '[') && (p != '?') && (p != '\\')) {
		    while ((string < stringEnd) && (p != *string)) {
			string++;
		    }
		}
		if (TclByteArrayMatch(string, stringEnd - string,
				pattern, patternEnd - pattern, 0)) {
		    return 1;
		}
		if (string == stringEnd) {
		    return 0;
		}
		string++;
	    }
	}

	/*
	 * Check for a "?" as the next pattern character. It matches any
	 * single character.
	 */

	if (p == '?') {
	    pattern++;
	    string++;
	    continue;
	}

	/*
	 * Check for a "[" as the next pattern character. It is followed by a
	 * list of characters that are acceptable, or by a range (two
	 * characters separated by "-").
	 */

	if (p == '[') {
	    unsigned char ch1, startChar, endChar;

	    pattern++;
	    ch1 = *string;
	    string++;
	    while (1) {
		if ((*pattern == ']') || (pattern == patternEnd)) {
		    return 0;
		}
		startChar = *pattern;
		pattern++;
		if (*pattern == '-') {
		    pattern++;
		    if (pattern == patternEnd) {
			return 0;
		    }
		    endChar = *pattern;
		    pattern++;
		    if (((startChar <= ch1) && (ch1 <= endChar))
			    || ((endChar <= ch1) && (ch1 <= startChar))) {
			/*
			 * Matches ranges of form [a-z] or [z-a].
			 */

			break;
		    }
		} else if (startChar == ch1) {
		    break;
		}
	    }
	    while (*pattern != ']') {
		if (pattern == patternEnd) {
		    pattern--;
		    break;
		}
		pattern++;
	    }
	    pattern++;
	    continue;
	}

	/*
	 * If the next pattern character is '\', just strip off the '\' so we
	 * do exact matching on the character that follows.
	 */

	if (p == '\\') {
	    if (++pattern == patternEnd) {
		return 0;
	    }
	}

	/*
	 * There's no special character. Just make sure that the next bytes of
	 * each string match.
	 */

	if (*string != *pattern) {
	    return 0;
	}
	string++;
	pattern++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclStringMatchObj --
 *
 *	See if a particular string matches a particular pattern. Allows case
 *	insensitivity. This is the generic multi-type handler for the various
 *	matching algorithms.
 *
 * Results:
 *	The return value is 1 if string matches pattern, and 0 otherwise. The
 *	matching operation permits the following special characters in the
 *	pattern: *?\[] (see the manual entry for details on what these mean).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclStringMatchObj(
    Tcl_Obj *strObj,		/* string object. */
    Tcl_Obj *ptnObj,		/* pattern object. */
    int flags)			/* Only TCL_MATCH_NOCASE should be passed, or
				 * 0. */
{
    int match, length, plen;

    /*
     * Promote based on the type of incoming object.
     * XXX: Currently doesn't take advantage of exact-ness that
     * XXX: TclReToGlob tells us about
    trivial = nocase ? 0 : TclMatchIsTrivial(TclGetString(ptnObj));
     */

    if ((strObj->typePtr == &tclStringType) || (strObj->typePtr == NULL)) {
	Tcl_UniChar *udata, *uptn;

	udata = Tcl_GetUnicodeFromObj(strObj, &length);
	uptn  = Tcl_GetUnicodeFromObj(ptnObj, &plen);
	match = TclUniCharMatch(udata, length, uptn, plen, flags);
    } else if (TclIsPureByteArray(strObj) && TclIsPureByteArray(ptnObj)
		&& !flags) {
	unsigned char *data, *ptn;

	data = Tcl_GetByteArrayFromObj(strObj, &length);
	ptn  = Tcl_GetByteArrayFromObj(ptnObj, &plen);
	match = TclByteArrayMatch(data, length, ptn, plen, 0);
    } else {
	match = Tcl_StringCaseMatch(TclGetString(strObj),
		TclGetString(ptnObj), flags);
    }
    return match;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringInit --
 *
 *	Initializes a dynamic string, discarding any previous contents of the
 *	string (Tcl_DStringFree should have been called already if the dynamic
 *	string was previously in use).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The dynamic string is initialized to be empty.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringInit(
    Tcl_DString *dsPtr)		/* Pointer to structure for dynamic string. */
{
    dsPtr->string = dsPtr->staticSpace;
    dsPtr->length = 0;
    dsPtr->spaceAvl = TCL_DSTRING_STATIC_SIZE;
    dsPtr->staticSpace[0] = '\0';
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringAppend --
 *
 *	Append more bytes to the current value of a dynamic string.
 *
 * Results:
 *	The return value is a pointer to the dynamic string's new value.
 *
 * Side effects:
 *	Length bytes from "bytes" (or all of "bytes" if length is less than
 *	zero) are added to the current value of the string. Memory gets
 *	reallocated if needed to accomodate the string's new size.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_DStringAppend(
    Tcl_DString *dsPtr,		/* Structure describing dynamic string. */
    const char *bytes,		/* String to append. If length is -1 then this
				 * must be null-terminated. */
    int length)			/* Number of bytes from "bytes" to append. If
				 * < 0, then append all of bytes, up to null
				 * at end. */
{
    int newSize;

    if (length < 0) {
	length = strlen(bytes);
    }
    newSize = length + dsPtr->length;

    /*
     * Allocate a larger buffer for the string if the current one isn't large
     * enough. Allocate extra space in the new buffer so that there will be
     * room to grow before we have to allocate again.
     */

    if (newSize >= dsPtr->spaceAvl) {
	dsPtr->spaceAvl = newSize * 2;
	if (dsPtr->string == dsPtr->staticSpace) {
	    char *newString = ckalloc(dsPtr->spaceAvl);

	    memcpy(newString, dsPtr->string, (size_t) dsPtr->length);
	    dsPtr->string = newString;
	} else {
	    int offset = -1;

	    /* See [16896d49fd] */
	    if (bytes >= dsPtr->string
		    && bytes <= dsPtr->string + dsPtr->length) {
		offset = bytes - dsPtr->string;
	    }

	    dsPtr->string = ckrealloc(dsPtr->string, dsPtr->spaceAvl);

	    if (offset >= 0) {
		bytes = dsPtr->string + offset;
	    }
	}
    }

    /*
     * Copy the new string into the buffer at the end of the old one.
     */

    memcpy(dsPtr->string + dsPtr->length, bytes, length);
    dsPtr->length += length;
    dsPtr->string[dsPtr->length] = '\0';
    return dsPtr->string;
}

/*
 *----------------------------------------------------------------------
 *
 * TclDStringAppendObj, TclDStringAppendDString --
 *
 *	Simple wrappers round Tcl_DStringAppend that make it easier to append
 *	from particular sources of strings.
 *
 *----------------------------------------------------------------------
 */

char *
TclDStringAppendObj(
    Tcl_DString *dsPtr,
    Tcl_Obj *objPtr)
{
    int length;
    char *bytes = Tcl_GetStringFromObj(objPtr, &length);

    return Tcl_DStringAppend(dsPtr, bytes, length);
}

char *
TclDStringAppendDString(
    Tcl_DString *dsPtr,
    Tcl_DString *toAppendPtr)
{
    return Tcl_DStringAppend(dsPtr, Tcl_DStringValue(toAppendPtr),
	    Tcl_DStringLength(toAppendPtr));
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringAppendElement --
 *
 *	Append a list element to the current value of a dynamic string.
 *
 * Results:
 *	The return value is a pointer to the dynamic string's new value.
 *
 * Side effects:
 *	String is reformatted as a list element and added to the current value
 *	of the string. Memory gets reallocated if needed to accomodate the
 *	string's new size.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_DStringAppendElement(
    Tcl_DString *dsPtr,		/* Structure describing dynamic string. */
    const char *element)	/* String to append. Must be
				 * null-terminated. */
{
    char *dst = dsPtr->string + dsPtr->length;
    int needSpace = TclNeedSpace(dsPtr->string, dst);
    char flags = needSpace ? TCL_DONT_QUOTE_HASH : 0;
    int newSize = dsPtr->length + needSpace
	    + TclScanElement(element, -1, &flags);

    /*
     * Allocate a larger buffer for the string if the current one isn't large
     * enough. Allocate extra space in the new buffer so that there will be
     * room to grow before we have to allocate again. SPECIAL NOTE: must use
     * memcpy, not strcpy, to copy the string to a larger buffer, since there
     * may be embedded NULLs in the string in some cases.
     */

    if (newSize >= dsPtr->spaceAvl) {
	dsPtr->spaceAvl = newSize * 2;
	if (dsPtr->string == dsPtr->staticSpace) {
	    char *newString = ckalloc(dsPtr->spaceAvl);

	    memcpy(newString, dsPtr->string, (size_t) dsPtr->length);
	    dsPtr->string = newString;
	} else {
	    int offset = -1;

	    /* See [16896d49fd] */
	    if (element >= dsPtr->string
		    && element <= dsPtr->string + dsPtr->length) {
		offset = element - dsPtr->string;
	    }

	    dsPtr->string = ckrealloc(dsPtr->string, dsPtr->spaceAvl);

	    if (offset >= 0) {
		element = dsPtr->string + offset;
	    }
	}
	dst = dsPtr->string + dsPtr->length;
    }

    /*
     * Convert the new string to a list element and copy it into the buffer at
     * the end, with a space, if needed.
     */

    if (needSpace) {
	*dst = ' ';
	dst++;
	dsPtr->length++;

	/*
	 * If we need a space to separate this element from preceding stuff,
	 * then this element will not lead a list, and need not have it's
	 * leading '#' quoted.
	 */

	flags |= TCL_DONT_QUOTE_HASH;
    }
    dsPtr->length += TclConvertElement(element, -1, dst, flags);
    dsPtr->string[dsPtr->length] = '\0';
    return dsPtr->string;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringSetLength --
 *
 *	Change the length of a dynamic string. This can cause the string to
 *	either grow or shrink, depending on the value of length.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The length of dsPtr is changed to length and a null byte is stored at
 *	that position in the string. If length is larger than the space
 *	allocated for dsPtr, then a panic occurs.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringSetLength(
    Tcl_DString *dsPtr,		/* Structure describing dynamic string. */
    int length)			/* New length for dynamic string. */
{
    int newsize;

    if (length < 0) {
	length = 0;
    }
    if (length >= dsPtr->spaceAvl) {
	/*
	 * There are two interesting cases here. In the first case, the user
	 * may be trying to allocate a large buffer of a specific size. It
	 * would be wasteful to overallocate that buffer, so we just allocate
	 * enough for the requested size plus the trailing null byte. In the
	 * second case, we are growing the buffer incrementally, so we need
	 * behavior similar to Tcl_DStringAppend. The requested length will
	 * usually be a small delta above the current spaceAvl, so we'll end
	 * up doubling the old size. This won't grow the buffer quite as
	 * quickly, but it should be close enough.
	 */

	newsize = dsPtr->spaceAvl * 2;
	if (length < newsize) {
	    dsPtr->spaceAvl = newsize;
	} else {
	    dsPtr->spaceAvl = length + 1;
	}
	if (dsPtr->string == dsPtr->staticSpace) {
	    char *newString = ckalloc(dsPtr->spaceAvl);

	    memcpy(newString, dsPtr->string, (size_t) dsPtr->length);
	    dsPtr->string = newString;
	} else {
	    dsPtr->string = ckrealloc(dsPtr->string, dsPtr->spaceAvl);
	}
    }
    dsPtr->length = length;
    dsPtr->string[length] = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringFree --
 *
 *	Frees up any memory allocated for the dynamic string and reinitializes
 *	the string to an empty state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The previous contents of the dynamic string are lost, and the new
 *	value is an empty string.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringFree(
    Tcl_DString *dsPtr)		/* Structure describing dynamic string. */
{
    if (dsPtr->string != dsPtr->staticSpace) {
	ckfree(dsPtr->string);
    }
    dsPtr->string = dsPtr->staticSpace;
    dsPtr->length = 0;
    dsPtr->spaceAvl = TCL_DSTRING_STATIC_SIZE;
    dsPtr->staticSpace[0] = '\0';
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringResult --
 *
 *	This function moves the value of a dynamic string into an interpreter
 *	as its string result. Afterwards, the dynamic string is reset to an
 *	empty string.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The string is "moved" to interp's result, and any existing string
 *	result for interp is freed. dsPtr is reinitialized to an empty string.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringResult(
    Tcl_Interp *interp,		/* Interpreter whose result is to be reset. */
    Tcl_DString *dsPtr)		/* Dynamic string that is to become the
				 * result of interp. */
{
    Tcl_ResetResult(interp);
    Tcl_SetObjResult(interp, TclDStringToObj(dsPtr));
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringGetResult --
 *
 *	This function moves an interpreter's result into a dynamic string.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The interpreter's string result is cleared, and the previous contents
 *	of dsPtr are freed.
 *
 *	If the string result is empty, the object result is moved to the
 *	string result, then the object result is reset.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringGetResult(
    Tcl_Interp *interp,		/* Interpreter whose result is to be reset. */
    Tcl_DString *dsPtr)		/* Dynamic string that is to become the result
				 * of interp. */
{
    Interp *iPtr = (Interp *) interp;

    if (dsPtr->string != dsPtr->staticSpace) {
	ckfree(dsPtr->string);
    }

    /*
     * Do more efficient transfer when we know the result is a Tcl_Obj. When
     * there's no st`ring result, we only have to deal with two cases:
     *
     *  1. When the string rep is the empty string, when we don't copy but
     *     instead use the staticSpace in the DString to hold an empty string.

     *  2. When the string rep is not there or there's a real string rep, when
     *     we use Tcl_GetString to fetch (or generate) the string rep - which
     *     we know to have been allocated with ckalloc() - and use it to
     *     populate the DString space. Then, we free the internal rep. and set
     *     the object's string representation back to the canonical empty
     *     string.
     */

    if (!iPtr->result[0] && iPtr->objResultPtr
	    && !Tcl_IsShared(iPtr->objResultPtr)) {
	if (iPtr->objResultPtr->bytes == tclEmptyStringRep) {
	    dsPtr->string = dsPtr->staticSpace;
	    dsPtr->string[0] = 0;
	    dsPtr->length = 0;
	    dsPtr->spaceAvl = TCL_DSTRING_STATIC_SIZE;
	} else {
	    dsPtr->string = Tcl_GetString(iPtr->objResultPtr);
	    dsPtr->length = iPtr->objResultPtr->length;
	    dsPtr->spaceAvl = dsPtr->length + 1;
	    TclFreeIntRep(iPtr->objResultPtr);
	    iPtr->objResultPtr->bytes = tclEmptyStringRep;
	    iPtr->objResultPtr->length = 0;
	}
	return;
    }

    /*
     * If the string result is empty, move the object result to the string
     * result, then reset the object result.
     */

    (void) Tcl_GetStringResult(interp);

    dsPtr->length = strlen(iPtr->result);
    if (iPtr->freeProc != NULL) {
	if (iPtr->freeProc == TCL_DYNAMIC) {
	    dsPtr->string = iPtr->result;
	    dsPtr->spaceAvl = dsPtr->length+1;
	} else {
	    dsPtr->string = ckalloc(dsPtr->length+1);
	    memcpy(dsPtr->string, iPtr->result, (unsigned) dsPtr->length+1);
	    iPtr->freeProc(iPtr->result);
	}
	dsPtr->spaceAvl = dsPtr->length+1;
	iPtr->freeProc = NULL;
    } else {
	if (dsPtr->length < TCL_DSTRING_STATIC_SIZE) {
	    dsPtr->string = dsPtr->staticSpace;
	    dsPtr->spaceAvl = TCL_DSTRING_STATIC_SIZE;
	} else {
	    dsPtr->string = ckalloc(dsPtr->length+1);
	    dsPtr->spaceAvl = dsPtr->length + 1;
	}
	memcpy(dsPtr->string, iPtr->result, (unsigned) dsPtr->length+1);
    }

    iPtr->result = iPtr->resultSpace;
    iPtr->resultSpace[0] = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclDStringToObj --
 *
 *	This function moves a dynamic string's contents to a new Tcl_Obj. Be
 *	aware that this function does *not* check that the encoding of the
 *	contents of the dynamic string is correct; this is the caller's
 *	responsibility to enforce.
 *
 * Results:
 *	The newly-allocated untyped (i.e., typePtr==NULL) Tcl_Obj with a
 *	reference count of zero.
 *
 * Side effects:
 *	The string is "moved" to the object. dsPtr is reinitialized to an
 *	empty string; it does not need to be Tcl_DStringFree'd after this if
 *	not used further.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclDStringToObj(
    Tcl_DString *dsPtr)
{
    Tcl_Obj *result;

    if (dsPtr->string == dsPtr->staticSpace) {
	if (dsPtr->length == 0) {
	    TclNewObj(result);
	} else {
	    /*
	     * Static buffer, so must copy.
	     */

	    TclNewStringObj(result, dsPtr->string, dsPtr->length);
	}
    } else {
	/*
	 * Dynamic buffer, so transfer ownership and reset.
	 */

	TclNewObj(result);
	result->bytes = dsPtr->string;
	result->length = dsPtr->length;
    }

    /*
     * Re-establish the DString as empty with no buffer allocated.
     */

    dsPtr->string = dsPtr->staticSpace;
    dsPtr->spaceAvl = TCL_DSTRING_STATIC_SIZE;
    dsPtr->length = 0;
    dsPtr->staticSpace[0] = '\0';

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringStartSublist --
 *
 *	This function adds the necessary information to a dynamic string
 *	(e.g. " {") to start a sublist. Future element appends will be in the
 *	sublist rather than the main list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters get added to the dynamic string.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringStartSublist(
    Tcl_DString *dsPtr)		/* Dynamic string. */
{
    if (TclNeedSpace(dsPtr->string, dsPtr->string + dsPtr->length)) {
	TclDStringAppendLiteral(dsPtr, " {");
    } else {
	TclDStringAppendLiteral(dsPtr, "{");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DStringEndSublist --
 *
 *	This function adds the necessary characters to a dynamic string to end
 *	a sublist (e.g. "}"). Future element appends will be in the enclosing
 *	(sub)list rather than the current sublist.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DStringEndSublist(
    Tcl_DString *dsPtr)		/* Dynamic string. */
{
    TclDStringAppendLiteral(dsPtr, "}");
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PrintDouble --
 *
 *	Given a floating-point value, this function converts it to an ASCII
 *	string using.
 *
 * Results:
 *	The ASCII equivalent of "value" is written at "dst". It is written
 *	using the current precision, and it is guaranteed to contain a decimal
 *	point or exponent, so that it looks like a floating-point value and
 *	not an integer.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_PrintDouble(
    Tcl_Interp *interp,		/* Interpreter whose tcl_precision variable
				 * used to be used to control printing. It's
				 * ignored now. */
    double value,		/* Value to print as string. */
    char *dst)			/* Where to store converted value; must have
				 * at least TCL_DOUBLE_SPACE characters. */
{
    char *p, c;
    int exponent;
    int signum;
    char *digits;
    char *end;
    int *precisionPtr = Tcl_GetThreadData(&precisionKey, (int) sizeof(int));

    /*
     * Handle NaN.
     */

    if (TclIsNaN(value)) {
	TclFormatNaN(value, dst);
	return;
    }

    /*
     * Handle infinities.
     */

    if (TclIsInfinite(value)) {
	/*
	 * Remember to copy the terminating NUL too.
	 */

	if (value < 0) {
	    memcpy(dst, "-Inf", 5);
	} else {
	    memcpy(dst, "Inf", 4);
	}
	return;
    }

    /*
     * Ordinary (normal and denormal) values.
     */

    if (*precisionPtr == 0) {
	digits = TclDoubleDigits(value, -1, TCL_DD_SHORTEST,
		&exponent, &signum, &end);
    } else {
	/*
	 * There are at least two possible interpretations for tcl_precision.
	 *
	 * The first is, "choose the decimal representation having
	 * $tcl_precision digits of significance that is nearest to the given
	 * number, breaking ties by rounding to even, and then trimming
	 * trailing zeros." This gives the greatest possible precision in the
	 * decimal string, but offers the anomaly that [expr 0.1] will be
	 * "0.10000000000000001".
	 *
	 * The second is "choose the decimal representation having at most
	 * $tcl_precision digits of significance that is nearest to the given
	 * number. If no such representation converts exactly to the given
	 * number, choose the one that is closest, breaking ties by rounding
	 * to even. If more than one such representation converts exactly to
	 * the given number, choose the shortest, breaking ties in favour of
	 * the nearest, breaking remaining ties in favour of the one ending in
	 * an even digit."
	 *
	 * Tcl 8.4 implements the first of these, which gives rise to
	 * anomalies in formatting:
	 *
	 *	% expr 0.1
	 *	0.10000000000000001
	 *	% expr 0.01
	 *	0.01
	 *	% expr 1e-7
	 *	9.9999999999999995e-08
	 *
	 * For human readability, it appears better to choose the second rule,
	 * and let [expr 0.1] return 0.1. But for 8.4 compatibility, we prefer
	 * the first (the recommended zero value for tcl_precision avoids the
	 * problem entirely).
	 *
	 * Uncomment TCL_DD_SHORTEN_FLAG in the next call to prefer the method
	 * that allows floating point values to be shortened if it can be done
	 * without loss of precision.
	 */

	digits = TclDoubleDigits(value, *precisionPtr,
		TCL_DD_E_FORMAT /* | TCL_DD_SHORTEN_FLAG */,
		&exponent, &signum, &end);
    }
    if (signum) {
	*dst++ = '-';
    }
    p = digits;
    if (exponent < -4 || exponent > 16) {
	/*
	 * E format for numbers < 1e-3 or >= 1e17.
	 */

	*dst++ = *p++;
	c = *p;
	if (c != '\0') {
	    *dst++ = '.';
	    while (c != '\0') {
		*dst++ = c;
		c = *++p;
	    }
	}

	/*
	 * Tcl 8.4 appears to format with at least a two-digit exponent;
	 * preserve that behaviour when tcl_precision != 0
	 */

	if (*precisionPtr == 0) {
	    sprintf(dst, "e%+d", exponent);
	} else {
	    sprintf(dst, "e%+03d", exponent);
	}
    } else {
	/*
	 * F format for others.
	 */

	if (exponent < 0) {
	    *dst++ = '0';
	}
	c = *p;
	while (exponent-- >= 0) {
	    if (c != '\0') {
		*dst++ = c;
		c = *++p;
	    } else {
		*dst++ = '0';
	    }
	}
	*dst++ = '.';
	if (c == '\0') {
	    *dst++ = '0';
	} else {
	    while (++exponent < -1) {
		*dst++ = '0';
	    }
	    while (c != '\0') {
		*dst++ = c;
		c = *++p;
	    }
	}
	*dst++ = '\0';
    }
    ckfree(digits);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPrecTraceProc --
 *
 *	This function is invoked whenever the variable "tcl_precision" is
 *	written.
 *
 * Results:
 *	Returns NULL if all went well, or an error message if the new value
 *	for the variable doesn't make sense.
 *
 * Side effects:
 *	If the new value doesn't make sense then this function undoes the
 *	effect of the variable modification. Otherwise it modifies the format
 *	string that's used by Tcl_PrintDouble.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
char *
TclPrecTraceProc(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *name1,		/* Name of variable. */
    const char *name2,		/* Second part of variable name. */
    int flags)			/* Information about what happened. */
{
    Tcl_Obj *value;
    int prec;
    int *precisionPtr = Tcl_GetThreadData(&precisionKey, (int) sizeof(int));

    /*
     * If the variable is unset, then recreate the trace.
     */

    if (flags & TCL_TRACE_UNSETS) {
	if ((flags & TCL_TRACE_DESTROYED) && !Tcl_InterpDeleted(interp)) {
	    Tcl_TraceVar2(interp, name1, name2,
		    TCL_GLOBAL_ONLY|TCL_TRACE_READS|TCL_TRACE_WRITES
		    |TCL_TRACE_UNSETS, TclPrecTraceProc, clientData);
	}
	return NULL;
    }

    /*
     * When the variable is read, reset its value from our shared value. This
     * is needed in case the variable was modified in some other interpreter
     * so that this interpreter's value is out of date.
     */


    if (flags & TCL_TRACE_READS) {
	Tcl_SetVar2Ex(interp, name1, name2, Tcl_NewIntObj(*precisionPtr),
		flags & TCL_GLOBAL_ONLY);
	return NULL;
    }

    /*
     * The variable is being written. Check the new value and disallow it if
     * it isn't reasonable or if this is a safe interpreter (we don't want
     * safe interpreters messing up the precision of other interpreters).
     */

    if (Tcl_IsSafe(interp)) {
	return (char *) "can't modify precision from a safe interpreter";
    }
    value = Tcl_GetVar2Ex(interp, name1, name2, flags & TCL_GLOBAL_ONLY);
    if (value == NULL
	    || Tcl_GetIntFromObj(NULL, value, &prec) != TCL_OK
	    || prec < 0 || prec > TCL_MAX_PREC) {
	return (char *) "improper value for precision";
    }
    *precisionPtr = prec;
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TclNeedSpace --
 *
 *	This function checks to see whether it is appropriate to add a space
 *	before appending a new list element to an existing string.
 *
 * Results:
 *	The return value is 1 if a space is appropriate, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclNeedSpace(
    const char *start,		/* First character in string. */
    const char *end)		/* End of string (place where space will be
				 * added, if appropriate). */
{
    /*
     * A space is needed unless either:
     * (a) we're at the start of the string, or
     */

    if (end == start) {
	return 0;
    }

    /*
     * (b) we're at the start of a nested list-element, quoted with an open
     *	   curly brace; we can be nested arbitrarily deep, so long as the
     *	   first curly brace starts an element, so backtrack over open curly
     *	   braces that are trailing characters of the string; and
     */

    end = Tcl_UtfPrev(end, start);
    while (*end == '{') {
	if (end == start) {
	    return 0;
	}
	end = Tcl_UtfPrev(end, start);
    }

    /*
     * (c) the trailing character of the string is already a list-element
     *	   separator (according to TclFindElement); that is, one of these
     *	   characters:
     *		\u0009	\t	TAB
     *		\u000A	\n	NEWLINE
     *		\u000B	\v	VERTICAL TAB
     *		\u000C	\f	FORM FEED
     *		\u000D	\r	CARRIAGE RETURN
     *		\u0020		SPACE
     *	   with the condition that the penultimate character is not a
     *	   backslash.
     */

    if (*end > 0x20) {
	/*
	 * Performance tweak. All ASCII spaces are <= 0x20. So get a quick
	 * answer for most characters before comparing against all spaces in
	 * the switch below.
	 *
	 * NOTE: Remove this if other Unicode spaces ever get accepted as
	 * list-element separators.
	 */

	return 1;
    }
    switch (*end) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '\v':
    case '\f':
	if ((end == start) || (end[-1] != '\\')) {
	    return 0;
	}
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFormatInt --
 *
 *	This procedure formats an integer into a sequence of decimal digit
 *	characters in a buffer. If the integer is negative, a minus sign is
 *	inserted at the start of the buffer. A null character is inserted at
 *	the end of the formatted characters. It is the caller's responsibility
 *	to ensure that enough storage is available. This procedure has the
 *	effect of sprintf(buffer, "%ld", n) but is faster as proven in
 *	benchmarks.  This is key to UpdateStringOfInt, which is a common path
 *	for a lot of code (e.g. int-indexed arrays).
 *
 * Results:
 *	An integer representing the number of characters formatted, not
 *	including the terminating \0.
 *
 * Side effects:
 *	The formatted characters are written into the storage pointer to by
 *	the "buffer" argument.
 *
 *----------------------------------------------------------------------
 */

int
TclFormatInt(
    char *buffer,		/* Points to the storage into which the
				 * formatted characters are written. */
    long n)			/* The integer to format. */
{
    long intVal;
    int i;
    int numFormatted, j;
    const char *digits = "0123456789";

    /*
     * Check first whether "n" is zero.
     */

    if (n == 0) {
	buffer[0] = '0';
	buffer[1] = 0;
	return 1;
    }

    /*
     * Check whether "n" is the maximum negative value. This is -2^(m-1) for
     * an m-bit word, and has no positive equivalent; negating it produces the
     * same value.
     */

    intVal = -n;			/* [Bug 3390638] Workaround for*/
    if (n == -n || intVal == n) {	/* broken compiler optimizers. */
	return sprintf(buffer, "%ld", n);
    }

    /*
     * Generate the characters of the result backwards in the buffer.
     */

    intVal = (n < 0? -n : n);
    i = 0;
    buffer[0] = '\0';
    do {
	i++;
	buffer[i] = digits[intVal % 10];
	intVal = intVal/10;
    } while (intVal > 0);
    if (n < 0) {
	i++;
	buffer[i] = '-';
    }
    numFormatted = i;

    /*
     * Now reverse the characters.
     */

    for (j = 0;  j < i;  j++, i--) {
	char tmp = buffer[i];

	buffer[i] = buffer[j];
	buffer[j] = tmp;
    }
    return numFormatted;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetIntForIndex --
 *
 *	Provides an integer corresponding to the list index held in a Tcl
 *	object. The string value 'objPtr' is expected have the format
 *	integer([+-]integer)? or end([+-]integer)?.
 *
 * Value
 * 	TCL_OK
 *      
 * 	    The index is stored at the address given by by 'indexPtr'. If
 * 	    'objPtr' has the value "end", the value stored is 'endValue'.
 * 
 * 	TCL_ERROR
 *      
 * 	    The value of 'objPtr' does not have one of the expected formats. If
 * 	    'interp' is non-NULL, an error message is left in the interpreter's
 * 	    result object.
 * 
 * Effect
 * 
 * 	The object referenced by 'objPtr' is converted, as needed, to an
 * 	integer, wide integer, or end-based-index object.
 * 
 *----------------------------------------------------------------------
 */

int
TclGetIntForIndex(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. If
				 * NULL, then no error message is left after
				 * errors. */
    Tcl_Obj *objPtr,		/* Points to an object containing either "end"
				 * or an integer. */
    int endValue,		/* The value to be stored at "indexPtr" if
				 * "objPtr" holds "end". */
    int *indexPtr)		/* Location filled in with an integer
				 * representing an index. */
{
    int length;
    char *opPtr;
    const char *bytes;

    if (TclGetIntFromObj(NULL, objPtr, indexPtr) == TCL_OK) {
	return TCL_OK;
    }

    if (GetEndOffsetFromObj(objPtr, endValue, indexPtr) == TCL_OK) {
	return TCL_OK;
    }

    bytes = TclGetStringFromObj(objPtr, &length);

    /*
     * Leading whitespace is acceptable in an index.
     */

    while (length && TclIsSpaceProc(*bytes)) {
	bytes++;
	length--;
    }

    if (TclParseNumber(NULL, NULL, NULL, bytes, length, (const char **)&opPtr,
	    TCL_PARSE_INTEGER_ONLY | TCL_PARSE_NO_WHITESPACE) == TCL_OK) {
	int code, first, second;
	char savedOp = *opPtr;

	if ((savedOp != '+') && (savedOp != '-')) {
	    goto parseError;
	}
	if (TclIsSpaceProc(opPtr[1])) {
	    goto parseError;
	}
	*opPtr = '\0';
	code = Tcl_GetInt(interp, bytes, &first);
	*opPtr = savedOp;
	if (code == TCL_ERROR) {
	    goto parseError;
	}
	if (TCL_ERROR == Tcl_GetInt(interp, opPtr+1, &second)) {
	    goto parseError;
	}
	if (savedOp == '+') {
	    *indexPtr = first + second;
	} else {
	    *indexPtr = first - second;
	}
	return TCL_OK;
    }

    /*
     * Report a parse error.
     */

  parseError:
    if (interp != NULL) {
	bytes = Tcl_GetString(objPtr);
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad index \"%s\": must be integer?[+-]integer? or"
		" end?[+-]integer?", bytes));
	if (!strncmp(bytes, "end-", 4)) {
	    bytes += 4;
	}
	TclCheckBadOctal(interp, bytes);
	Tcl_SetErrorCode(interp, "TCL", "VALUE", "INDEX", NULL);
    }

    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateStringOfEndOffset --
 *
 *	Update the string rep of a Tcl object holding an "end-offset"
 *	expression.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stores a valid string in the object's string rep.
 *
 * This function does NOT free any earlier string rep. If it is called on an
 * object that already has a valid string rep, it will leak memory.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateStringOfEndOffset(
    register Tcl_Obj *objPtr)
{
    char buffer[TCL_INTEGER_SPACE + 5];
    register int len = 3;

    memcpy(buffer, "end", 4);
    if (objPtr->internalRep.longValue != 0) {
	buffer[len++] = '-';
	len += TclFormatInt(buffer+len, -(objPtr->internalRep.longValue));
    }
    objPtr->bytes = ckalloc((unsigned) len+1);
    memcpy(objPtr->bytes, buffer, (unsigned) len+1);
    objPtr->length = len;
}

/*
 *----------------------------------------------------------------------
 *
 * GetEndOffsetFromObj --
 *
 *      Look for a string of the form "end[+-]offset" and convert it to an
 *      internal representation holding the offset.
 *
 * Results:
 *      Tcl return code.
 *
 * Side effects:
 *      May store a Tcl_ObjType.
 *
 *----------------------------------------------------------------------
 */

static int
GetEndOffsetFromObj(
    Tcl_Obj *objPtr,            /* Pointer to the object to parse */
    int endValue,               /* The value to be stored at "indexPtr" if
                                 * "objPtr" holds "end". */
    int *indexPtr)              /* Location filled in with an integer
                                 * representing an index. */
{
    if (SetEndOffsetFromAny(NULL, objPtr) != TCL_OK) {
	return TCL_ERROR;
    }

    /* TODO: Handle overflow cases sensibly */
    *indexPtr = endValue + (int)objPtr->internalRep.longValue;
    return TCL_OK;
}

    
/*
 *----------------------------------------------------------------------
 *
 * SetEndOffsetFromAny --
 *
 *	Look for a string of the form "end[+-]offset" and convert it to an
 *	internal representation holding the offset.
 *
 * Results:
 *	Returns TCL_OK if ok, TCL_ERROR if the string was badly formed.
 *
 * Side effects:
 *	If interp is not NULL, stores an error message in the interpreter
 *	result.
 *
 *----------------------------------------------------------------------
 */

static int
SetEndOffsetFromAny(
    Tcl_Interp *interp,		/* Tcl interpreter or NULL */
    Tcl_Obj *objPtr)		/* Pointer to the object to parse */
{
    int offset;			/* Offset in the "end-offset" expression */
    register const char *bytes;	/* String rep of the object */
    int length;			/* Length of the object's string rep */

    /*
     * If it's already the right type, we're fine.
     */

    if (objPtr->typePtr == &tclEndOffsetType) {
	return TCL_OK;
    }

    /*
     * Check for a string rep of the right form.
     */

    bytes = TclGetStringFromObj(objPtr, &length);
    if ((*bytes != 'e') || (strncmp(bytes, "end",
	    (size_t)((length > 3) ? 3 : length)) != 0)) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad index \"%s\": must be end?[+-]integer?", bytes));
	    Tcl_SetErrorCode(interp, "TCL", "VALUE", "INDEX", NULL);
	}
	return TCL_ERROR;
    }

    /*
     * Convert the string rep.
     */

    if (length <= 3) {
	offset = 0;
    } else if ((length > 4) && ((bytes[3] == '-') || (bytes[3] == '+'))) {
	/*
	 * This is our limited string expression evaluator. Pass everything
	 * after "end-" to Tcl_GetInt, then reverse for offset.
	 */

	if (TclIsSpaceProc(bytes[4])) {
	    goto badIndexFormat;
	}
	if (Tcl_GetInt(interp, bytes+4, &offset) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (bytes[3] == '-') {

	    /* TODO: Review overflow concerns here! */
	    offset = -offset;
	}
    } else {
	/*
	 * Conversion failed. Report the error.
	 */

    badIndexFormat:
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "bad index \"%s\": must be end?[+-]integer?", bytes));
	    Tcl_SetErrorCode(interp, "TCL", "VALUE", "INDEX", NULL);
	}
	return TCL_ERROR;
    }

    /*
     * The conversion succeeded. Free the old internal rep and set the new
     * one.
     */

    TclFreeIntRep(objPtr);
    objPtr->internalRep.longValue = offset;
    objPtr->typePtr = &tclEndOffsetType;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclIndexEncode --
 *
 *      Parse objPtr to determine if it is an index value. Two cases
 *	are possible.  The value objPtr might be parsed as an absolute
 *	index value in the C signed int range.  Note that this includes
 *	index values that are integers as presented and it includes index
 *      arithmetic expressions. The absolute index values that can be
 *	directly meaningful as an index into either a list or a string are
 *	those integer values >= TCL_INDEX_START (0)
 *	and < TCL_INDEX_AFTER (INT_MAX).
 *      The largest string supported in Tcl 8 has bytelength INT_MAX.
 *      This means the largest supported character length is also INT_MAX,
 *      and the index of the last character in a string of length INT_MAX
 *      is INT_MAX-1.
 *
 *      Any absolute index value parsed outside that range is encoded
 *      using the before and after values passed in by the
 *      caller as the encoding to use for indices that are either
 *      less than or greater than the usable index range. TCL_INDEX_AFTER
 *      is available as a good choice for most callers to use for
 *      after. Likewise, the value TCL_INDEX_BEFORE is good for
 *      most callers to use for before.  Other values are possible
 *      when the caller knows it is helpful in producing its own behavior
 *      for indices before and after the indexed item.
 *
 *      A token can also be parsed as an end-relative index expression.
 *      All end-relative expressions that indicate an index larger
 *      than end (end+2, end--5) point beyond the end of the indexed
 *      collection, and can be encoded as after.  The end-relative
 *      expressions that indicate an index less than or equal to end
 *      are encoded relative to the value TCL_INDEX_END (-2).  The
 *      index "end" is encoded as -2, down to the index "end-0x7ffffffe"
 *      which is encoded as INT_MIN. Since the largest index into a
 *      string possible in Tcl 8 is 0x7ffffffe, the interpretation of
 *      "end-0x7ffffffe" for that largest string would be 0.  Thus,
 *      if the tokens "end-0x7fffffff" or "end+-0x80000000" are parsed,
 *      they can be encoded with the before value.
 *
 *      These details will require re-examination whenever string and
 *      list length limits are increased, but that will likely also
 *      mean a revised routine capable of returning Tcl_WideInt values.
 *
 * Returns:
 *      TCL_OK if parsing succeeded, and TCL_ERROR if it failed.
 *
 * Side effects:
 *      When TCL_OK is returned, the encoded index value is written
 *      to *indexPtr.
 *
 *----------------------------------------------------------------------
 */

int
TclIndexEncode(
    Tcl_Interp *interp,	/* For error reporting, may be NULL */
    Tcl_Obj *objPtr,	/* Index value to parse */
    int before,		/* Value to return for index before beginning */
    int after,		/* Value to return for index after end */
    int *indexPtr)	/* Where to write the encoded answer, not NULL */
{
    int idx;

    if (TCL_OK == TclGetIntFromObj(NULL, objPtr, &idx)) {
        /* We parsed a value in the range INT_MIN...INT_MAX */
    integerEncode:
        if (idx < TCL_INDEX_START) {
            /* All negative absolute indices are "before the beginning" */
            idx = before;
        } else if (idx == INT_MAX) {
            /* This index value is always "after the end" */
            idx = after;
        }
        /* usual case, the absolute index value encodes itself */
    } else if (TCL_OK == GetEndOffsetFromObj(objPtr, 0, &idx)) {
        /*
         * We parsed an end+offset index value. 
         * idx holds the offset value in the range INT_MIN...INT_MAX.
         */
        if (idx > 0) {
            /*
             * All end+postive or end-negative expressions 
             * always indicate "after the end".
             */
            idx = after;
        } else if (idx < INT_MIN - TCL_INDEX_END) {
            /* These indices always indicate "before the beginning */
            idx = before;
        } else {
            /* Encoded end-positive (or end+negative) are offset */
            idx += TCL_INDEX_END;
        }

    /* TODO: Consider flag to suppress repeated end-offset parse. */
    } else if (TCL_OK == TclGetIntForIndexM(interp, objPtr, 0, &idx)) {
        /*
         * Only reach this case when the index value is a
         * constant index arithmetic expression, and idx
         * holds the result. Treat it the same as if it were
         * parsed as an absolute integer value.
         */
        goto integerEncode;
    } else {
	return TCL_ERROR;
    }
    *indexPtr = idx;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclIndexDecode --
 *
 *	Decodes a value previously encoded by TclIndexEncode.  The argument
 *	endValue indicates what value of "end" should be used in the
 *	decoding.
 *
 * Results:
 *	The decoded index value.
 *
 *----------------------------------------------------------------------
 */

int
TclIndexDecode(
    int encoded,	/* Value to decode */
    int endValue)	/* Meaning of "end" to use, > TCL_INDEX_END */
{
    if (encoded <= TCL_INDEX_END) {
	return (encoded - TCL_INDEX_END) + endValue;
    }
    return encoded;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCheckBadOctal --
 *
 *	This function checks for a bad octal value and appends a meaningful
 *	error to the interp's result.
 *
 * Results:
 *	1 if the argument was a bad octal, else 0.
 *
 * Side effects:
 *	The interpreter's result is modified.
 *
 *----------------------------------------------------------------------
 */

int
TclCheckBadOctal(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting. If
				 * NULL, then no error message is left after
				 * errors. */
    const char *value)		/* String to check. */
{
    register const char *p = value;

    /*
     * A frequent mistake is invalid octal values due to an unwanted leading
     * zero. Try to generate a meaningful error message.
     */

    while (TclIsSpaceProc(*p)) {
	p++;
    }
    if (*p == '+' || *p == '-') {
	p++;
    }
    if (*p == '0') {
	if ((p[1] == 'o') || p[1] == 'O') {
	    p += 2;
	}
	while (isdigit(UCHAR(*p))) {	/* INTL: digit. */
	    p++;
	}
	while (TclIsSpaceProc(*p)) {
	    p++;
	}
	if (*p == '\0') {
	    /*
	     * Reached end of string.
	     */

	    if (interp != NULL) {
		/*
		 * Don't reset the result here because we want this result to
		 * be added to an existing error message as extra info.
		 */

		Tcl_AppendToObj(Tcl_GetObjResult(interp),
			" (looks like invalid octal number)", -1);
	    }
	    return 1;
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ClearHash --
 *
 *	Remove all the entries in the hash table *tablePtr.
 *
 *----------------------------------------------------------------------
 */

static void
ClearHash(
    Tcl_HashTable *tablePtr)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;

    for (hPtr = Tcl_FirstHashEntry(tablePtr, &search); hPtr != NULL;
	    hPtr = Tcl_NextHashEntry(&search)) {
	Tcl_Obj *objPtr = Tcl_GetHashValue(hPtr);

	Tcl_DecrRefCount(objPtr);
	Tcl_DeleteHashEntry(hPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetThreadHash --
 *
 *	Get a thread-specific (Tcl_HashTable *) associated with a thread data
 *	key.
 *
 * Results:
 *	The Tcl_HashTable * corresponding to *keyPtr.
 *
 * Side effects:
 *	The first call on a keyPtr in each thread creates a new Tcl_HashTable,
 *	and registers a thread exit handler to dispose of it.
 *
 *----------------------------------------------------------------------
 */

static Tcl_HashTable *
GetThreadHash(
    Tcl_ThreadDataKey *keyPtr)
{
    Tcl_HashTable **tablePtrPtr =
	    Tcl_GetThreadData(keyPtr, sizeof(Tcl_HashTable *));

    if (NULL == *tablePtrPtr) {
	*tablePtrPtr = ckalloc(sizeof(Tcl_HashTable));
	Tcl_CreateThreadExitHandler(FreeThreadHash, *tablePtrPtr);
	Tcl_InitHashTable(*tablePtrPtr, TCL_ONE_WORD_KEYS);
    }
    return *tablePtrPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeThreadHash --
 *
 *	Thread exit handler used by GetThreadHash to dispose of a thread hash
 *	table.
 *
 * Side effects:
 *	Frees a Tcl_HashTable.
 *
 *----------------------------------------------------------------------
 */

static void
FreeThreadHash(
    ClientData clientData)
{
    Tcl_HashTable *tablePtr = clientData;

    ClearHash(tablePtr);
    Tcl_DeleteHashTable(tablePtr);
    ckfree(tablePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FreeProcessGlobalValue --
 *
 *	Exit handler used by Tcl(Set|Get)ProcessGlobalValue to cleanup a
 *	ProcessGlobalValue at exit.
 *
 *----------------------------------------------------------------------
 */

static void
FreeProcessGlobalValue(
    ClientData clientData)
{
    ProcessGlobalValue *pgvPtr = clientData;

    pgvPtr->epoch++;
    pgvPtr->numBytes = 0;
    ckfree(pgvPtr->value);
    pgvPtr->value = NULL;
    if (pgvPtr->encoding) {
	Tcl_FreeEncoding(pgvPtr->encoding);
	pgvPtr->encoding = NULL;
    }
    Tcl_MutexFinalize(&pgvPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * TclSetProcessGlobalValue --
 *
 *	Utility routine to set a global value shared by all threads in the
 *	process while keeping a thread-local copy as well.
 *
 *----------------------------------------------------------------------
 */

void
TclSetProcessGlobalValue(
    ProcessGlobalValue *pgvPtr,
    Tcl_Obj *newValue,
    Tcl_Encoding encoding)
{
    const char *bytes;
    Tcl_HashTable *cacheMap;
    Tcl_HashEntry *hPtr;
    int dummy;

    Tcl_MutexLock(&pgvPtr->mutex);

    /*
     * Fill the global string value.
     */

    pgvPtr->epoch++;
    if (NULL != pgvPtr->value) {
	ckfree(pgvPtr->value);
    } else {
	Tcl_CreateExitHandler(FreeProcessGlobalValue, pgvPtr);
    }
    bytes = Tcl_GetStringFromObj(newValue, &pgvPtr->numBytes);
    pgvPtr->value = ckalloc(pgvPtr->numBytes + 1);
    memcpy(pgvPtr->value, bytes, (unsigned) pgvPtr->numBytes + 1);
    if (pgvPtr->encoding) {
	Tcl_FreeEncoding(pgvPtr->encoding);
    }
    pgvPtr->encoding = encoding;

    /*
     * Fill the local thread copy directly with the Tcl_Obj value to avoid
     * loss of the intrep. Increment newValue refCount early to handle case
     * where we set a PGV to itself.
     */

    Tcl_IncrRefCount(newValue);
    cacheMap = GetThreadHash(&pgvPtr->key);
    ClearHash(cacheMap);
    hPtr = Tcl_CreateHashEntry(cacheMap, INT2PTR(pgvPtr->epoch), &dummy);
    Tcl_SetHashValue(hPtr, newValue);
    Tcl_MutexUnlock(&pgvPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetProcessGlobalValue --
 *
 *	Retrieve a global value shared among all threads of the process,
 *	preferring a thread-local copy as long as it remains valid.
 *
 * Results:
 *	Returns a (Tcl_Obj *) that holds a copy of the global value.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclGetProcessGlobalValue(
    ProcessGlobalValue *pgvPtr)
{
    Tcl_Obj *value = NULL;
    Tcl_HashTable *cacheMap;
    Tcl_HashEntry *hPtr;
    int epoch = pgvPtr->epoch;

    if (pgvPtr->encoding) {
	Tcl_Encoding current = Tcl_GetEncoding(NULL, NULL);

	if (pgvPtr->encoding != current) {
	    /*
	     * The system encoding has changed since the master string value
	     * was saved. Convert the master value to be based on the new
	     * system encoding.
	     */

	    Tcl_DString native, newValue;

	    Tcl_MutexLock(&pgvPtr->mutex);
	    pgvPtr->epoch++;
	    epoch = pgvPtr->epoch;
	    Tcl_UtfToExternalDString(pgvPtr->encoding, pgvPtr->value,
		    pgvPtr->numBytes, &native);
	    Tcl_ExternalToUtfDString(current, Tcl_DStringValue(&native),
	    Tcl_DStringLength(&native), &newValue);
	    Tcl_DStringFree(&native);
	    ckfree(pgvPtr->value);
	    pgvPtr->value = ckalloc(Tcl_DStringLength(&newValue) + 1);
	    memcpy(pgvPtr->value, Tcl_DStringValue(&newValue),
		    (size_t) Tcl_DStringLength(&newValue) + 1);
	    Tcl_DStringFree(&newValue);
	    Tcl_FreeEncoding(pgvPtr->encoding);
	    pgvPtr->encoding = current;
	    Tcl_MutexUnlock(&pgvPtr->mutex);
	} else {
	    Tcl_FreeEncoding(current);
	}
    }
    cacheMap = GetThreadHash(&pgvPtr->key);
    hPtr = Tcl_FindHashEntry(cacheMap, (char *) INT2PTR(epoch));
    if (NULL == hPtr) {
	int dummy;

	/*
	 * No cache for the current epoch - must be a new one.
	 *
	 * First, clear the cacheMap, as anything in it must refer to some
	 * expired epoch.
	 */

	ClearHash(cacheMap);

	/*
	 * If no thread has set the shared value, call the initializer.
	 */

	Tcl_MutexLock(&pgvPtr->mutex);
	if ((NULL == pgvPtr->value) && (pgvPtr->proc)) {
	    pgvPtr->epoch++;
	    pgvPtr->proc(&pgvPtr->value,&pgvPtr->numBytes,&pgvPtr->encoding);
	    if (pgvPtr->value == NULL) {
		Tcl_Panic("PGV Initializer did not initialize");
	    }
	    Tcl_CreateExitHandler(FreeProcessGlobalValue, pgvPtr);
	}

	/*
	 * Store a copy of the shared value in our epoch-indexed cache.
	 */

	value = Tcl_NewStringObj(pgvPtr->value, pgvPtr->numBytes);
	hPtr = Tcl_CreateHashEntry(cacheMap,
		INT2PTR(pgvPtr->epoch), &dummy);
	Tcl_MutexUnlock(&pgvPtr->mutex);
	Tcl_SetHashValue(hPtr, value);
	Tcl_IncrRefCount(value);
    }
    return Tcl_GetHashValue(hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclSetObjNameOfExecutable --
 *
 *	This function stores the absolute pathname of the executable file
 *	(normally as computed by TclpFindExecutable).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stores the executable name.
 *
 *----------------------------------------------------------------------
 */

void
TclSetObjNameOfExecutable(
    Tcl_Obj *name,
    Tcl_Encoding encoding)
{
    TclSetProcessGlobalValue(&executableName, name, encoding);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetObjNameOfExecutable --
 *
 *	This function retrieves the absolute pathname of the application in
 *	which the Tcl library is running, usually as previously stored by
 *	TclpFindExecutable(). This function call is the C API equivalent to
 *	the "info nameofexecutable" command.
 *
 * Results:
 *	A pointer to an "fsPath" Tcl_Obj, or to an empty Tcl_Obj if the
 *	pathname of the application is unknown.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclGetObjNameOfExecutable(void)
{
    return TclGetProcessGlobalValue(&executableName);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetNameOfExecutable --
 *
 *	This function retrieves the absolute pathname of the application in
 *	which the Tcl library is running, and returns it in string form.
 *
 *	The returned string belongs to Tcl and should be copied if the caller
 *	plans to keep it, to guard against it becoming invalid.
 *
 * Results:
 *	A pointer to the internal string or NULL if the internal full path
 *	name has not been computed or unknown.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
Tcl_GetNameOfExecutable(void)
{
    int numBytes;
    const char *bytes =
	    Tcl_GetStringFromObj(TclGetObjNameOfExecutable(), &numBytes);

    if (numBytes == 0) {
	return NULL;
    }
    return bytes;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetTime --
 *
 *	Deprecated synonym for Tcl_GetTime. This function is provided for the
 *	benefit of extensions written before Tcl_GetTime was exported from the
 *	library.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stores current time in the buffer designated by "timePtr"
 *
 *----------------------------------------------------------------------
 */

void
TclpGetTime(
    Tcl_Time *timePtr)
{
    Tcl_GetTime(timePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetPlatform --
 *
 *	This is a kludge that allows the test library to get access the
 *	internal tclPlatform variable.
 *
 * Results:
 *	Returns a pointer to the tclPlatform variable.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TclPlatformType *
TclGetPlatform(void)
{
    return &tclPlatform;
}

/*
 *----------------------------------------------------------------------
 *
 * TclReToGlob --
 *
 *	Attempt to convert a regular expression to an equivalent glob pattern.
 *
 * Results:
 *	Returns TCL_OK on success, TCL_ERROR on failure. If interp is not
 *	NULL, an error message is placed in the result. On success, the
 *	DString will contain an exact equivalent glob pattern. The caller is
 *	responsible for calling Tcl_DStringFree on success. If exactPtr is not
 *	NULL, it will be 1 if an exact match qualifies.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclReToGlob(
    Tcl_Interp *interp,
    const char *reStr,
    int reStrLen,
    Tcl_DString *dsPtr,
    int *exactPtr,
    int *quantifiersFoundPtr)
{
    int anchorLeft, anchorRight, lastIsStar, numStars;
    char *dsStr, *dsStrStart;
    const char *msg, *p, *strEnd, *code;

    strEnd = reStr + reStrLen;
    Tcl_DStringInit(dsPtr);
    if (quantifiersFoundPtr != NULL) {
	*quantifiersFoundPtr = 0;
    }

    /*
     * "***=xxx" == "*xxx*", watch for glob-sensitive chars.
     */

    if ((reStrLen >= 4) && (memcmp("***=", reStr, 4) == 0)) {
	/*
	 * At most, the glob pattern has length 2*reStrLen + 2 to backslash
	 * escape every character and have * at each end.
	 */

	Tcl_DStringSetLength(dsPtr, reStrLen + 2);
	dsStr = dsStrStart = Tcl_DStringValue(dsPtr);
	*dsStr++ = '*';
	for (p = reStr + 4; p < strEnd; p++) {
	    switch (*p) {
	    case '\\': case '*': case '[': case ']': case '?':
		/* Only add \ where necessary for glob */
		*dsStr++ = '\\';
		/* fall through */
	    default:
		*dsStr++ = *p;
		break;
	    }
	}
	*dsStr++ = '*';
	Tcl_DStringSetLength(dsPtr, dsStr - dsStrStart);
	if (exactPtr) {
	    *exactPtr = 0;
	}
	return TCL_OK;
    }

    /*
     * At most, the glob pattern has length reStrLen + 2 to account for
     * possible * at each end.
     */

    Tcl_DStringSetLength(dsPtr, reStrLen + 2);
    dsStr = dsStrStart = Tcl_DStringValue(dsPtr);

    /*
     * Check for anchored REs (ie ^foo$), so we can use string equal if
     * possible. Do not alter the start of str so we can free it correctly.
     *
     * Keep track of the last char being an unescaped star to prevent multiple
     * instances.  Simpler than checking that the last star may be escaped.
     */

    msg = NULL;
    code = NULL;
    p = reStr;
    anchorRight = 0;
    lastIsStar = 0;
    numStars = 0;

    if (*p == '^') {
	anchorLeft = 1;
	p++;
    } else {
	anchorLeft = 0;
	*dsStr++ = '*';
	lastIsStar = 1;
    }

    for ( ; p < strEnd; p++) {
	switch (*p) {
	case '\\':
	    p++;
	    switch (*p) {
	    case 'a':
		*dsStr++ = '\a';
		break;
	    case 'b':
		*dsStr++ = '\b';
		break;
	    case 'f':
		*dsStr++ = '\f';
		break;
	    case 'n':
		*dsStr++ = '\n';
		break;
	    case 'r':
		*dsStr++ = '\r';
		break;
	    case 't':
		*dsStr++ = '\t';
		break;
	    case 'v':
		*dsStr++ = '\v';
		break;
	    case 'B': case '\\':
		*dsStr++ = '\\';
		*dsStr++ = '\\';
		anchorLeft = 0; /* prevent exact match */
		break;
	    case '*': case '[': case ']': case '?':
		/* Only add \ where necessary for glob */
		*dsStr++ = '\\';
		anchorLeft = 0; /* prevent exact match */
		/* fall through */
	    case '{': case '}': case '(': case ')': case '+':
	    case '.': case '|': case '^': case '$':
		*dsStr++ = *p;
		break;
	    default:
		msg = "invalid escape sequence";
		code = "BADESCAPE";
		goto invalidGlob;
	    }
	    break;
	case '.':
	    if (quantifiersFoundPtr != NULL) {
		*quantifiersFoundPtr = 1;
	    }
	    anchorLeft = 0; /* prevent exact match */
	    if (p+1 < strEnd) {
		if (p[1] == '*') {
		    p++;
		    if (!lastIsStar) {
			*dsStr++ = '*';
			lastIsStar = 1;
			numStars++;
		    }
		    continue;
		} else if (p[1] == '+') {
		    p++;
		    *dsStr++ = '?';
		    *dsStr++ = '*';
		    lastIsStar = 1;
		    numStars++;
		    continue;
		}
	    }
	    *dsStr++ = '?';
	    break;
	case '$':
	    if (p+1 != strEnd) {
		msg = "$ not anchor";
		code = "NONANCHOR";
		goto invalidGlob;
	    }
	    anchorRight = 1;
	    break;
	case '*': case '+': case '?': case '|': case '^':
	case '{': case '}': case '(': case ')': case '[': case ']':
	    msg = "unhandled RE special char";
	    code = "UNHANDLED";
	    goto invalidGlob;
	default:
	    *dsStr++ = *p;
	    break;
	}
	lastIsStar = 0;
    }
    if (numStars > 1) {
	/*
	 * Heuristic: if >1 non-anchoring *, the risk is large that glob
	 * matching is slower than the RE engine, so report invalid.
	 */

	msg = "excessive recursive glob backtrack potential";
	code = "OVERCOMPLEX";
	goto invalidGlob;
    }

    if (!anchorRight && !lastIsStar) {
	*dsStr++ = '*';
    }
    Tcl_DStringSetLength(dsPtr, dsStr - dsStrStart);

    if (exactPtr) {
	*exactPtr = (anchorLeft && anchorRight);
    }

    return TCL_OK;

  invalidGlob:
    if (interp != NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(msg, -1));
	Tcl_SetErrorCode(interp, "TCL", "RE2GLOB", code, NULL);
    }
    Tcl_DStringFree(dsPtr);
    return TCL_ERROR;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
