/*
 * tclParse.c --
 *
 *	This file contains functions that parse Tcl scripts. They do so in a
 *	general-purpose fashion that can be used for many different purposes,
 *	including compilation, direct execution, code analysis, etc.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 Ajuba Solutions.
 * Contributions from Don Porter, NIST, 2002. (not subject to US copyright)
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclParse.h"
#include <assert.h>

/*
 * The following table provides parsing information about each possible 8-bit
 * character. The table is designed to be referenced with either signed or
 * unsigned characters, so it has 384 entries. The first 128 entries
 * correspond to negative character values, the next 256 correspond to
 * positive character values. The last 128 entries are identical to the first
 * 128. The table is always indexed with a 128-byte offset (the 128th entry
 * corresponds to a character value of 0).
 *
 * The macro CHAR_TYPE is used to index into the table and return information
 * about its character argument. The following return values are defined.
 *
 * TYPE_NORMAL -	All characters that don't have special significance to
 *			the Tcl parser.
 * TYPE_SPACE -		The character is a whitespace character other than
 *			newline.
 * TYPE_COMMAND_END -	Character is newline or semicolon.
 * TYPE_SUBS -		Character begins a substitution or has other special
 *			meaning in ParseTokens: backslash, dollar sign, or
 *			open bracket.
 * TYPE_QUOTE -		Character is a double quote.
 * TYPE_CLOSE_PAREN -	Character is a right parenthesis.
 * TYPE_CLOSE_BRACK -	Character is a right square bracket.
 * TYPE_BRACE -		Character is a curly brace (either left or right).
 */

const char tclCharTypeTable[] = {
    /*
     * Negative character values, from -128 to -1:
     */

    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,

    /*
     * Positive character values, from 0-127:
     */

    TYPE_SUBS,        TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_SPACE,       TYPE_COMMAND_END, TYPE_SPACE,
    TYPE_SPACE,       TYPE_SPACE,       TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_SPACE,       TYPE_NORMAL,      TYPE_QUOTE,       TYPE_NORMAL,
    TYPE_SUBS,        TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_CLOSE_PAREN, TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_COMMAND_END,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_SUBS,
    TYPE_SUBS,        TYPE_CLOSE_BRACK, TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_BRACE,
    TYPE_NORMAL,      TYPE_BRACE,       TYPE_NORMAL,      TYPE_NORMAL,

    /*
     * Large unsigned character values, from 128-255:
     */

    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
};

/*
 * Prototypes for local functions defined in this file:
 */

static inline int	CommandComplete(const char *script, int numBytes);
static int		ParseComment(const char *src, int numBytes,
			    Tcl_Parse *parsePtr);
static int		ParseTokens(const char *src, int numBytes, int mask,
			    int flags, Tcl_Parse *parsePtr);
static int		ParseWhiteSpace(const char *src, int numBytes,
			    int *incompletePtr, char *typePtr);

/*
 *----------------------------------------------------------------------
 *
 * TclParseInit --
 *
 *	Initialize the fields of a Tcl_Parse struct.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Tcl_Parse struct pointed to by parsePtr gets initialized.
 *
 *----------------------------------------------------------------------
 */

void
TclParseInit(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting */
    const char *start,		/* Start of string to be parsed. */
    int numBytes,		/* Total number of bytes in string. If < 0,
				 * the script consists of all bytes up to the
				 * first null character. */
    Tcl_Parse *parsePtr)	/* Points to struct to initialize */
{
    parsePtr->numWords = 0;
    parsePtr->tokenPtr = parsePtr->staticTokens;
    parsePtr->numTokens = 0;
    parsePtr->tokensAvailable = NUM_STATIC_TOKENS;
    parsePtr->string = start;
    parsePtr->end = start + numBytes;
    parsePtr->term = parsePtr->end;
    parsePtr->interp = interp;
    parsePtr->incomplete = 0;
    parsePtr->errorType = TCL_PARSE_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseCommand --
 *
 *	Given a string, this function parses the first Tcl command in the
 *	string and returns information about the structure of the command.
 *
 * Results:
 *	The return value is TCL_OK if the command was parsed successfully and
 *	TCL_ERROR otherwise. If an error occurs and interp isn't NULL then an
 *	error message is left in its result. On a successful return, parsePtr
 *	is filled in with information about the command that was parsed.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the information
 *	about the command, then additional space is malloc-ed. If the function
 *	returns TCL_OK then the caller must eventually invoke Tcl_FreeParse to
 *	release any additional space that was allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseCommand(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting; if
				 * NULL, then no error message is provided. */
    const char *start,		/* First character of string containing one or
				 * more Tcl commands. */
    register int numBytes,	/* Total number of bytes in string. If < 0,
				 * the script consists of all bytes up to the
				 * first null character. */
    int nested,			/* Non-zero means this is a nested command:
				 * close bracket should be considered a
				 * command terminator. If zero, then close
				 * bracket has no special meaning. */
    register Tcl_Parse *parsePtr)
				/* Structure to fill in with information about
				 * the parsed command; any previous
				 * information in the structure is ignored. */
{
    register const char *src;	/* Points to current character in the
				 * command. */
    char type;			/* Result returned by CHAR_TYPE(*src). */
    Tcl_Token *tokenPtr;	/* Pointer to token being filled in. */
    int wordIndex;		/* Index of word token for current word. */
    int terminators;		/* CHAR_TYPE bits that indicate the end of a
				 * command. */
    const char *termPtr;	/* Set by Tcl_ParseBraces/QuotedString to
				 * point to char after terminating one. */
    int scanned;

    if ((start == NULL) && (numBytes != 0)) {
	if (interp != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "can't parse a NULL pointer", -1));
	}
	return TCL_ERROR;
    }
    if (numBytes < 0) {
	numBytes = strlen(start);
    }
    TclParseInit(interp, start, numBytes, parsePtr);
    parsePtr->commentStart = NULL;
    parsePtr->commentSize = 0;
    parsePtr->commandStart = NULL;
    parsePtr->commandSize = 0;
    if (nested != 0) {
	terminators = TYPE_COMMAND_END | TYPE_CLOSE_BRACK;
    } else {
	terminators = TYPE_COMMAND_END;
    }

    /*
     * Parse any leading space and comments before the first word of the
     * command.
     */

    scanned = ParseComment(start, numBytes, parsePtr);
    src = (start + scanned);
    numBytes -= scanned;
    if (numBytes == 0) {
	if (nested) {
	    parsePtr->incomplete = nested;
	}
    }

    /*
     * The following loop parses the words of the command, one word in each
     * iteration through the loop.
     */

    parsePtr->commandStart = src;
    while (1) {
	int expandWord = 0;

	/*
	 * Create the token for the word.
	 */

	TclGrowParseTokenArray(parsePtr, 1);
	wordIndex = parsePtr->numTokens;
	tokenPtr = &parsePtr->tokenPtr[wordIndex];
	tokenPtr->type = TCL_TOKEN_WORD;

	/*
	 * Skip white space before the word. Also skip a backslash-newline
	 * sequence: it should be treated just like white space.
	 */

	scanned = ParseWhiteSpace(src,numBytes, &parsePtr->incomplete, &type);
	src += scanned;
	numBytes -= scanned;
	if (numBytes == 0) {
	    parsePtr->term = src;
	    break;
	}
	if ((type & terminators) != 0) {
	    parsePtr->term = src;
	    src++;
	    break;
	}
	tokenPtr->start = src;
	parsePtr->numTokens++;
	parsePtr->numWords++;

	/*
	 * At this point the word can have one of four forms: something
	 * enclosed in quotes, something enclosed in braces, and expanding
	 * word, or an unquoted word (anything else).
	 */

    parseWord:
	if (*src == '"') {
	    if (Tcl_ParseQuotedString(interp, src, numBytes, parsePtr, 1,
		    &termPtr) != TCL_OK) {
		goto error;
	    }
	    src = termPtr;
	    numBytes = parsePtr->end - src;
	} else if (*src == '{') {
	    int expIdx = wordIndex + 1;
	    Tcl_Token *expPtr;

	    if (Tcl_ParseBraces(interp, src, numBytes, parsePtr, 1,
		    &termPtr) != TCL_OK) {
		goto error;
	    }
	    src = termPtr;
	    numBytes = parsePtr->end - src;

	    /*
	     * Check whether the braces contained the word expansion prefix
	     * {*}
	     */

	    expPtr = &parsePtr->tokenPtr[expIdx];
	    if ((0 == expandWord)
		    /* Haven't seen prefix already */
		    && (1 == parsePtr->numTokens - expIdx)
		    /* Only one token */
		    && (((1 == (size_t) expPtr->size)
			    /* Same length as prefix */
			    && (expPtr->start[0] == '*')))
			    /* Is the prefix */
		    && (numBytes > 0) && (0 == ParseWhiteSpace(termPtr,
			    numBytes, &parsePtr->incomplete, &type))
		    && (type != TYPE_COMMAND_END)
		    /* Non-whitespace follows */) {
		expandWord = 1;
		parsePtr->numTokens--;
		goto parseWord;
	    }
	} else {
	    /*
	     * This is an unquoted word. Call ParseTokens and let it do all of
	     * the work.
	     */

	    if (ParseTokens(src, numBytes, TYPE_SPACE|terminators,
		    TCL_SUBST_ALL, parsePtr) != TCL_OK) {
		goto error;
	    }
	    src = parsePtr->term;
	    numBytes = parsePtr->end - src;
	}

	/*
	 * Finish filling in the token for the word and check for the special
	 * case of a word consisting of a single range of literal text.
	 */

	tokenPtr = &parsePtr->tokenPtr[wordIndex];
	tokenPtr->size = src - tokenPtr->start;
	tokenPtr->numComponents = parsePtr->numTokens - (wordIndex + 1);
	if (expandWord) {
	    int i, isLiteral = 1;

	    /*
	     * When a command includes a word that is an expanded literal; for
	     * example, {*}{1 2 3}, the parser performs that expansion
	     * immediately, generating several TCL_TOKEN_SIMPLE_WORDs instead
	     * of a single TCL_TOKEN_EXPAND_WORD that the Tcl_ParseCommand()
	     * caller might have to expand. This notably makes it simpler for
	     * those callers that wish to track line endings, such as those
	     * that implement key parts of TIP 280.
	     *
	     * First check whether the thing to be expanded is a literal,
	     * in the sense of being composed entirely of TCL_TOKEN_TEXT
	     * tokens.
	     */

	    for (i = 1; i <= tokenPtr->numComponents; i++) {
		if (tokenPtr[i].type != TCL_TOKEN_TEXT) {
		    isLiteral = 0;
		    break;
		}
	    }

	    if (isLiteral) {
		int elemCount = 0, code = TCL_OK, literal = 1;
		const char *nextElem, *listEnd, *elemStart;

		/*
		 * The word to be expanded is a literal, so determine the
		 * boundaries of the literal string to be treated as a list
		 * and expanded. That literal string starts at
		 * tokenPtr[1].start, and includes all bytes up to, but not
		 * including (tokenPtr[tokenPtr->numComponents].start +
		 * tokenPtr[tokenPtr->numComponents].size)
		 */

		listEnd = (tokenPtr[tokenPtr->numComponents].start +
			tokenPtr[tokenPtr->numComponents].size);
		nextElem = tokenPtr[1].start;

		/*
		 * Step through the literal string, parsing and counting list
		 * elements.
		 */

		while (nextElem < listEnd) {
		    int size;

		    code = TclFindElement(NULL, nextElem, listEnd - nextElem,
			    &elemStart, &nextElem, &size, &literal);
		    if ((code != TCL_OK) || !literal) {
			break;
		    }
		    if (elemStart < listEnd) {
			elemCount++;
		    }
		}

		if ((code != TCL_OK) || !literal) {
		    /*
		     * Some list element could not be parsed, or is not
		     * present as a literal substring of the script.  The
		     * compiler cannot handle list elements that get generated
		     * by a call to TclCopyAndCollapse(). Defer  the
		     * handling of  this to  compile/eval time, where  code is
		     * already  in place to  report the  "attempt to  expand a
		     * non-list" error or expand lists that require
		     * substitution.
		     */

		    tokenPtr->type = TCL_TOKEN_EXPAND_WORD;
		} else if (elemCount == 0) {
		    /*
		     * We are expanding a literal empty list. This means that
		     * the expanding word completely disappears, leaving no
		     * word generated this pass through the loop. Adjust
		     * accounting appropriately.
		     */

		    parsePtr->numWords--;
		    parsePtr->numTokens = wordIndex;
		} else {
		    /*
		     * Recalculate the number of Tcl_Tokens needed to store
		     * tokens representing the expanded list.
		     */

		    const char *listStart;
		    int growthNeeded = wordIndex + 2*elemCount
			    - parsePtr->numTokens;

		    parsePtr->numWords += elemCount - 1;
		    if (growthNeeded > 0) {
			TclGrowParseTokenArray(parsePtr, growthNeeded);
			tokenPtr = &parsePtr->tokenPtr[wordIndex];
		    }
		    parsePtr->numTokens = wordIndex + 2*elemCount;

		    /*
		     * Generate a TCL_TOKEN_SIMPLE_WORD token sequence for
		     * each element of the literal list we are expanding in
		     * place. Take care with the start and size fields of each
		     * token so they point to the right literal characters in
		     * the original script to represent the right expanded
		     * word value.
		     */

		    listStart = nextElem = tokenPtr[1].start;
		    while (nextElem < listEnd) {
			int quoted;

			tokenPtr->type = TCL_TOKEN_SIMPLE_WORD;
			tokenPtr->numComponents = 1;

			tokenPtr++;
			tokenPtr->type = TCL_TOKEN_TEXT;
			tokenPtr->numComponents = 0;
			TclFindElement(NULL, nextElem, listEnd - nextElem,
				&(tokenPtr->start), &nextElem,
				&(tokenPtr->size), NULL);

			quoted = (tokenPtr->start[-1] == '{'
				|| tokenPtr->start[-1] == '"')
				&& tokenPtr->start > listStart;
			tokenPtr[-1].start = tokenPtr->start - quoted;
			tokenPtr[-1].size = tokenPtr->start + tokenPtr->size
				- tokenPtr[-1].start + quoted;

			tokenPtr++;
		    }
		}
	    } else {
		/*
		 * The word to be expanded is not a literal, so defer
		 * expansion to compile/eval time by marking with a
		 * TCL_TOKEN_EXPAND_WORD token.
		 */

		tokenPtr->type = TCL_TOKEN_EXPAND_WORD;
	    }
	} else if ((tokenPtr->numComponents == 1)
		&& (tokenPtr[1].type == TCL_TOKEN_TEXT)) {
	    tokenPtr->type = TCL_TOKEN_SIMPLE_WORD;
	}

	/*
	 * Do two additional checks: (a) make sure we're really at the end of
	 * a word (there might have been garbage left after a quoted or braced
	 * word), and (b) check for the end of the command.
	 */

	scanned = ParseWhiteSpace(src,numBytes, &parsePtr->incomplete, &type);
	if (scanned) {
	    src += scanned;
	    numBytes -= scanned;
	    continue;
	}

	if (numBytes == 0) {
	    parsePtr->term = src;
	    break;
	}
	if ((type & terminators) != 0) {
	    parsePtr->term = src;
	    src++;
	    break;
	}
	if (src[-1] == '"') {
	    if (interp != NULL) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"extra characters after close-quote", -1));
	    }
	    parsePtr->errorType = TCL_PARSE_QUOTE_EXTRA;
	} else {
	    if (interp != NULL) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"extra characters after close-brace", -1));
	    }
	    parsePtr->errorType = TCL_PARSE_BRACE_EXTRA;
	}
	parsePtr->term = src;
	goto error;
    }

    parsePtr->commandSize = src - parsePtr->commandStart;
    return TCL_OK;

  error:
    Tcl_FreeParse(parsePtr);
    parsePtr->commandSize = parsePtr->end - parsePtr->commandStart;
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclIsSpaceProc --
 *
 *	Report whether byte is in the set of whitespace characters used by
 *	Tcl to separate words in scripts or elements in lists.
 *
 * Results:
 *	Returns 1, if byte is in the set, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclIsSpaceProc(
    int byte)
{
    return CHAR_TYPE(byte) & (TYPE_SPACE) || byte == '\n';
}

/*
 *----------------------------------------------------------------------
 *
 * TclIsBareword--
 *
 *	Report whether byte is one that can be part of a "bareword".
 *	This concept is named in expression parsing, where it determines
 *	what can be a legal function name, but is the same definition used
 *	in determining what variable names can be parsed as variable
 *	substitutions without the benefit of enclosing braces.  The set of
 *	ASCII chars that are accepted are the numeric chars ('0'-'9'),
 *	the alphabetic chars ('a'-'z', 'A'-'Z')	and underscore ('_').
 *
 * Results:
 *	Returns 1, if byte is in the accepted set of chars, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclIsBareword(
    int byte)
{
    if (byte < '0' || byte > 'z') {
	return 0;
    }
    if (byte <= '9' || byte >= 'a') {
	return 1;
    }
    if (byte == '_') {
	return 1;
    }
    if (byte < 'A' || byte > 'Z') {
	return 0;
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseWhiteSpace --
 *
 *	Scans up to numBytes bytes starting at src, consuming white space
 *	between words as defined by Tcl's parsing rules.
 *
 * Results:
 *	Returns the number of bytes recognized as white space. Records at
 *	parsePtr, information about the parse. Records at typePtr the
 *	character type of the non-whitespace character that terminated the
 *	scan.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ParseWhiteSpace(
    const char *src,		/* First character to parse. */
    register int numBytes,	/* Max number of bytes to scan. */
    int *incompletePtr,		/* Set this boolean memory to true if parsing
				 * indicates an incomplete command. */
    char *typePtr)		/* Points to location to store character type
				 * of character that ends run of whitespace */
{
    register char type = TYPE_NORMAL;
    register const char *p = src;

    while (1) {
	while (numBytes && ((type = CHAR_TYPE(*p)) & TYPE_SPACE)) {
	    numBytes--;
	    p++;
	}
	if (numBytes && (type & TYPE_SUBS)) {
	    if (*p != '\\') {
		break;
	    }
	    if (--numBytes == 0) {
		break;
	    }
	    if (p[1] != '\n') {
		break;
	    }
	    p += 2;
	    if (--numBytes == 0) {
		*incompletePtr = 1;
		break;
	    }
	    continue;
	}
	break;
    }
    *typePtr = type;
    return (p - src);
}

/*
 *----------------------------------------------------------------------
 *
 * TclParseAllWhiteSpace --
 *
 *	Scans up to numBytes bytes starting at src, consuming all white space
 *	including the command-terminating newline characters.
 *
 * Results:
 *	Returns the number of bytes recognized as white space.
 *
 *----------------------------------------------------------------------
 */

int
TclParseAllWhiteSpace(
    const char *src,		/* First character to parse. */
    int numBytes)		/* Max number of byes to scan */
{
    int dummy;
    char type;
    const char *p = src;

    do {
	int scanned = ParseWhiteSpace(p, numBytes, &dummy, &type);

	p += scanned;
	numBytes -= scanned;
    } while (numBytes && (*p == '\n') && (p++, --numBytes));
    return (p-src);
}

/*
 *----------------------------------------------------------------------
 *
 * TclParseHex --
 *
 *	Scans a hexadecimal number as a Tcl_UniChar value (e.g., for parsing
 *	\x and \u escape sequences). At most numBytes bytes are scanned.
 *
 * Results:
 *	The numeric value is stored in *resultPtr. Returns the number of bytes
 *	consumed.
 *
 * Notes:
 *	Relies on the following properties of the ASCII character set, with
 *	which UTF-8 is compatible:
 *
 *	The digits '0' .. '9' and the letters 'A' .. 'Z' and 'a' .. 'z' occupy
 *	consecutive code points, and '0' < 'A' < 'a'.
 *
 *----------------------------------------------------------------------
 */

int
TclParseHex(
    const char *src,		/* First character to parse. */
    int numBytes,		/* Max number of byes to scan */
    int *resultPtr)	/* Points to storage provided by caller where
				 * the character resulting from the
				 * conversion is to be written. */
{
    int result = 0;
    register const char *p = src;

    while (numBytes--) {
	unsigned char digit = UCHAR(*p);

	if (!isxdigit(digit) || (result > 0x10fff)) {
	    break;
	}

	p++;
	result <<= 4;

	if (digit >= 'a') {
	    result |= (10 + digit - 'a');
	} else if (digit >= 'A') {
	    result |= (10 + digit - 'A');
	} else {
	    result |= (digit - '0');
	}
    }

    *resultPtr = result;
    return (p - src);
}

/*
 *----------------------------------------------------------------------
 *
 * TclParseBackslash --
 *
 *	Scans up to numBytes bytes starting at src, consuming a backslash
 *	sequence as defined by Tcl's parsing rules.
 *
 * Results:
 *	Records at readPtr the number of bytes making up the backslash
 *	sequence. Records at dst the UTF-8 encoded equivalent of that
 *	backslash sequence. Returns the number of bytes written to dst, at
 *	most TCL_UTF_MAX. Either readPtr or dst may be NULL, if the results
 *	are not needed, but the return value is the same either way.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclParseBackslash(
    const char *src,		/* Points to the backslash character of a a
				 * backslash sequence. */
    int numBytes,		/* Max number of bytes to scan. */
    int *readPtr,		/* NULL, or points to storage where the number
				 * of bytes scanned should be written. */
    char *dst)			/* NULL, or points to buffer where the UTF-8
				 * encoding of the backslash sequence is to be
				 * written. At most TCL_UTF_MAX bytes will be
				 * written there. */
{
    register const char *p = src+1;
    Tcl_UniChar unichar = 0;
    int result;
    int count;
    char buf[TCL_UTF_MAX] = "";

    if (numBytes == 0) {
	if (readPtr != NULL) {
	    *readPtr = 0;
	}
	return 0;
    }

    if (dst == NULL) {
	dst = buf;
    }

    if (numBytes == 1) {
	/*
	 * Can only scan the backslash, so return it.
	 */

	result = '\\';
	count = 1;
	goto done;
    }

    count = 2;
    switch (*p) {
	/*
	 * Note: in the conversions below, use absolute values (e.g., 0xa)
	 * rather than symbolic values (e.g. \n) that get converted by the
	 * compiler. It's possible that compilers on some platforms will do
	 * the symbolic conversions differently, which could result in
	 * non-portable Tcl scripts.
	 */

    case 'a':
	result = 0x7;
	break;
    case 'b':
	result = 0x8;
	break;
    case 'f':
	result = 0xc;
	break;
    case 'n':
	result = 0xa;
	break;
    case 'r':
	result = 0xd;
	break;
    case 't':
	result = 0x9;
	break;
    case 'v':
	result = 0xb;
	break;
    case 'x':
	count += TclParseHex(p+1, (numBytes > 3) ? 2 : numBytes-2, &result);
	if (count == 2) {
	    /*
	     * No hexadigits -> This is just "x".
	     */

	    result = 'x';
	} else {
	    /*
	     * Keep only the last byte (2 hex digits).
	     */
	    result = (unsigned char) result;
	}
	break;
    case 'u':
	count += TclParseHex(p+1, (numBytes > 5) ? 4 : numBytes-2, &result);
	if (count == 2) {
	    /*
	     * No hexadigits -> This is just "u".
	     */
	    result = 'u';
	}
	break;
    case 'U':
	count += TclParseHex(p+1, (numBytes > 9) ? 8 : numBytes-2, &result);
	if (count == 2) {
	    /*
	     * No hexadigits -> This is just "U".
	     */
	    result = 'U';
	}
	break;
    case '\n':
	count--;
	do {
	    p++;
	    count++;
	} while ((count < numBytes) && ((*p == ' ') || (*p == '\t')));
	result = ' ';
	break;
    case 0:
	result = '\\';
	count = 1;
	break;
    default:
	/*
	 * Check for an octal number \oo?o?
	 */

	if (isdigit(UCHAR(*p)) && (UCHAR(*p) < '8')) {	/* INTL: digit */
	    result = *p - '0';
	    p++;
	    if ((numBytes == 2) || !isdigit(UCHAR(*p))	/* INTL: digit */
		    || (UCHAR(*p) >= '8')) {
		break;
	    }
	    count = 3;
	    result = (result << 3) + (*p - '0');
	    p++;
	    if ((numBytes == 3) || !isdigit(UCHAR(*p))	/* INTL: digit */
		    || (UCHAR(*p) >= '8') || (result >= 0x20)) {
		break;
	    }
	    count = 4;
	    result = UCHAR((result << 3) + (*p - '0'));
	    break;
	}

	/*
	 * We have to convert here in case the user has put a backslash in
	 * front of a multi-byte utf-8 character. While this means nothing
	 * special, we shouldn't break up a correct utf-8 character. [Bug
	 * #217987] test subst-3.2
	 */

	if (Tcl_UtfCharComplete(p, numBytes - 1)) {
	    count = TclUtfToUniChar(p, &unichar) + 1;	/* +1 for '\' */
	} else {
	    char utfBytes[TCL_UTF_MAX];

	    memcpy(utfBytes, p, (size_t) (numBytes - 1));
	    utfBytes[numBytes - 1] = '\0';
	    count = TclUtfToUniChar(utfBytes, &unichar) + 1;
	}
	result = unichar;
	break;
    }

  done:
    if (readPtr != NULL) {
	*readPtr = count;
    }
    count = Tcl_UniCharToUtf(result, dst);
#if TCL_UTF_MAX > 3
     if ((result >= 0xD800) && (count < 3)) {
	count += Tcl_UniCharToUtf(-1, dst + count);
    }
#endif
    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseComment --
 *
 *	Scans up to numBytes bytes starting at src, consuming a Tcl comment as
 *	defined by Tcl's parsing rules.
 *
 * Results:
 *	Records in parsePtr information about the parse. Returns the number of
 *	bytes consumed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ParseComment(
    const char *src,		/* First character to parse. */
    register int numBytes,	/* Max number of bytes to scan. */
    Tcl_Parse *parsePtr)	/* Information about parse in progress.
				 * Updated if parsing indicates an incomplete
				 * command. */
{
    register const char *p = src;

    while (numBytes) {
	char type;
	int scanned;

	do {
	    scanned = ParseWhiteSpace(p, numBytes,
		    &parsePtr->incomplete, &type);
	    p += scanned;
	    numBytes -= scanned;
	} while (numBytes && (*p == '\n') && (p++,numBytes--));

	if ((numBytes == 0) || (*p != '#')) {
	    break;
	}
	if (parsePtr->commentStart == NULL) {
	    parsePtr->commentStart = p;
	}

	while (numBytes) {
	    if (*p == '\\') {
		scanned = ParseWhiteSpace(p, numBytes, &parsePtr->incomplete,
			&type);
		if (scanned) {
		    p += scanned;
		    numBytes -= scanned;
		} else {
		    /*
		     * General backslash substitution in comments isn't part
		     * of the formal spec, but test parse-15.47 and history
		     * indicate that it has been the de facto rule. Don't
		     * change it now.
		     */

		    TclParseBackslash(p, numBytes, &scanned, NULL);
		    p += scanned;
		    numBytes -= scanned;
		}
	    } else {
		p++;
		numBytes--;
		if (p[-1] == '\n') {
		    break;
		}
	    }
	}
	parsePtr->commentSize = p - parsePtr->commentStart;
    }
    return (p - src);
}

/*
 *----------------------------------------------------------------------
 *
 * ParseTokens --
 *
 *	This function forms the heart of the Tcl parser. It parses one or more
 *	tokens from a string, up to a termination point specified by the
 *	caller. This function is used to parse unquoted command words (those
 *	not in quotes or braces), words in quotes, and array indices for
 *	variables. No more than numBytes bytes will be scanned.
 *
 * Results:
 *	Tokens are added to parsePtr and parsePtr->term is filled in with the
 *	address of the character that terminated the parse (the first one
 *	whose CHAR_TYPE matched mask or the character at parsePtr->end). The
 *	return value is TCL_OK if the parse completed successfully and
 *	TCL_ERROR otherwise. If a parse error occurs and parsePtr->interp is
 *	not NULL, then an error message is left in the interpreter's result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ParseTokens(
    register const char *src,	/* First character to parse. */
    register int numBytes,	/* Max number of bytes to scan. */
    int mask,			/* Specifies when to stop parsing. The parse
				 * stops at the first unquoted character whose
				 * CHAR_TYPE contains any of the bits in
				 * mask. */
    int flags,			/* OR-ed bits indicating what substitutions to
				 * perform: TCL_SUBST_COMMANDS,
				 * TCL_SUBST_VARIABLES, and
				 * TCL_SUBST_BACKSLASHES */
    Tcl_Parse *parsePtr)	/* Information about parse in progress.
				 * Updated with additional tokens and
				 * termination information. */
{
    char type;
    int originalTokens;
    int noSubstCmds = !(flags & TCL_SUBST_COMMANDS);
    int noSubstVars = !(flags & TCL_SUBST_VARIABLES);
    int noSubstBS = !(flags & TCL_SUBST_BACKSLASHES);
    Tcl_Token *tokenPtr;

    /*
     * Each iteration through the following loop adds one token of type
     * TCL_TOKEN_TEXT, TCL_TOKEN_BS, TCL_TOKEN_COMMAND, or TCL_TOKEN_VARIABLE
     * to parsePtr. For TCL_TOKEN_VARIABLE tokens, additional tokens are added
     * for the parsed variable name.
     */

    originalTokens = parsePtr->numTokens;
    while (numBytes && !((type = CHAR_TYPE(*src)) & mask)) {
	TclGrowParseTokenArray(parsePtr, 1);
	tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
	tokenPtr->start = src;
	tokenPtr->numComponents = 0;

	if ((type & TYPE_SUBS) == 0) {
	    /*
	     * This is a simple range of characters. Scan to find the end of
	     * the range.
	     */

	    while ((++src, --numBytes)
		    && !(CHAR_TYPE(*src) & (mask | TYPE_SUBS))) {
		/* empty loop */
	    }
	    tokenPtr->type = TCL_TOKEN_TEXT;
	    tokenPtr->size = src - tokenPtr->start;
	    parsePtr->numTokens++;
	} else if (*src == '$') {
	    int varToken;

	    if (noSubstVars) {
		tokenPtr->type = TCL_TOKEN_TEXT;
		tokenPtr->size = 1;
		parsePtr->numTokens++;
		src++;
		numBytes--;
		continue;
	    }

	    /*
	     * This is a variable reference. Call Tcl_ParseVarName to do all
	     * the dirty work of parsing the name.
	     */

	    varToken = parsePtr->numTokens;
	    if (Tcl_ParseVarName(parsePtr->interp, src, numBytes, parsePtr,
		    1) != TCL_OK) {
		return TCL_ERROR;
	    }
	    src += parsePtr->tokenPtr[varToken].size;
	    numBytes -= parsePtr->tokenPtr[varToken].size;
	} else if (*src == '[') {
	    Tcl_Parse *nestedPtr;

	    if (noSubstCmds) {
		tokenPtr->type = TCL_TOKEN_TEXT;
		tokenPtr->size = 1;
		parsePtr->numTokens++;
		src++;
		numBytes--;
		continue;
	    }

	    /*
	     * Command substitution. Call Tcl_ParseCommand recursively (and
	     * repeatedly) to parse the nested command(s), then throw away the
	     * parse information.
	     */

	    src++;
	    numBytes--;
	    nestedPtr = TclStackAlloc(parsePtr->interp, sizeof(Tcl_Parse));
	    while (1) {
		const char *curEnd;

		if (Tcl_ParseCommand(parsePtr->interp, src, numBytes, 1,
			nestedPtr) != TCL_OK) {
		    parsePtr->errorType = nestedPtr->errorType;
		    parsePtr->term = nestedPtr->term;
		    parsePtr->incomplete = nestedPtr->incomplete;
		    TclStackFree(parsePtr->interp, nestedPtr);
		    return TCL_ERROR;
		}
		curEnd = src + numBytes;
		src = nestedPtr->commandStart + nestedPtr->commandSize;
		numBytes = curEnd - src;
		Tcl_FreeParse(nestedPtr);

		/*
		 * Check for the closing ']' that ends the command
		 * substitution. It must have been the last character of the
		 * parsed command.
		 */

		if ((nestedPtr->term < parsePtr->end)
			&& (*(nestedPtr->term) == ']')
			&& !(nestedPtr->incomplete)) {
		    break;
		}
		if (numBytes == 0) {
		    if (parsePtr->interp != NULL) {
			Tcl_SetObjResult(parsePtr->interp, Tcl_NewStringObj(
				"missing close-bracket", -1));
		    }
		    parsePtr->errorType = TCL_PARSE_MISSING_BRACKET;
		    parsePtr->term = tokenPtr->start;
		    parsePtr->incomplete = 1;
		    TclStackFree(parsePtr->interp, nestedPtr);
		    return TCL_ERROR;
		}
	    }
	    TclStackFree(parsePtr->interp, nestedPtr);
	    tokenPtr->type = TCL_TOKEN_COMMAND;
	    tokenPtr->size = src - tokenPtr->start;
	    parsePtr->numTokens++;
	} else if (*src == '\\') {
	    if (noSubstBS) {
		tokenPtr->type = TCL_TOKEN_TEXT;
		tokenPtr->size = 1;
		parsePtr->numTokens++;
		src++;
		numBytes--;
		continue;
	    }

	    /*
	     * Backslash substitution.
	     */

	    TclParseBackslash(src, numBytes, &tokenPtr->size, NULL);

	    if (tokenPtr->size == 1) {
		/*
		 * Just a backslash, due to end of string.
		 */

		tokenPtr->type = TCL_TOKEN_TEXT;
		parsePtr->numTokens++;
		src++;
		numBytes--;
		continue;
	    }

	    if (src[1] == '\n') {
		if (numBytes == 2) {
		    parsePtr->incomplete = 1;
		}

		/*
		 * Note: backslash-newline is special in that it is treated
		 * the same as a space character would be. This means that it
		 * could terminate the token.
		 */

		if (mask & TYPE_SPACE) {
		    if (parsePtr->numTokens == originalTokens) {
			goto finishToken;
		    }
		    break;
		}
	    }

	    tokenPtr->type = TCL_TOKEN_BS;
	    parsePtr->numTokens++;
	    src += tokenPtr->size;
	    numBytes -= tokenPtr->size;
	} else if (*src == 0) {
	    tokenPtr->type = TCL_TOKEN_TEXT;
	    tokenPtr->size = 1;
	    parsePtr->numTokens++;
	    src++;
	    numBytes--;
	} else {
	    Tcl_Panic("ParseTokens encountered unknown character");
	}
    }
    if (parsePtr->numTokens == originalTokens) {
	/*
	 * There was nothing in this range of text. Add an empty token for the
	 * empty range, so that there is always at least one token added.
	 */

	TclGrowParseTokenArray(parsePtr, 1);
	tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
	tokenPtr->start = src;
	tokenPtr->numComponents = 0;

    finishToken:
	tokenPtr->type = TCL_TOKEN_TEXT;
	tokenPtr->size = 0;
	parsePtr->numTokens++;
    }
    parsePtr->term = src;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FreeParse --
 *
 *	This function is invoked to free any dynamic storage that may have
 *	been allocated by a previous call to Tcl_ParseCommand.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is any dynamically allocated memory in *parsePtr, it is
 *	freed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_FreeParse(
    Tcl_Parse *parsePtr)	/* Structure that was filled in by a previous
				 * call to Tcl_ParseCommand. */
{
    if (parsePtr->tokenPtr != parsePtr->staticTokens) {
	ckfree(parsePtr->tokenPtr);
	parsePtr->tokenPtr = parsePtr->staticTokens;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseVarName --
 *
 *	Given a string starting with a $ sign, parse off a variable name and
 *	return information about the parse. No more than numBytes bytes will
 *	be scanned.
 *
 * Results:
 *	The return value is TCL_OK if the command was parsed successfully and
 *	TCL_ERROR otherwise. If an error occurs and interp isn't NULL then an
 *	error message is left in its result. On a successful return, tokenPtr
 *	and numTokens fields of parsePtr are filled in with information about
 *	the variable name that was parsed. The "size" field of the first new
 *	token gives the total number of bytes in the variable name. Other
 *	fields in parsePtr are undefined.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the information
 *	about the command, then additional space is malloc-ed. If the function
 *	returns TCL_OK then the caller must eventually invoke Tcl_FreeParse to
 *	release any additional space that was allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseVarName(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting; if
				 * NULL, then no error message is provided. */
    const char *start,		/* Start of variable substitution string.
				 * First character must be "$". */
    register int numBytes,	/* Total number of bytes in string. If < 0,
				 * the string consists of all bytes up to the
				 * first null character. */
    Tcl_Parse *parsePtr,	/* Structure to fill in with information about
				 * the variable name. */
    int append)			/* Non-zero means append tokens to existing
				 * information in parsePtr; zero means ignore
				 * existing tokens in parsePtr and
				 * reinitialize it. */
{
    Tcl_Token *tokenPtr;
    register const char *src;
    int varIndex;
    unsigned array;

    if ((numBytes == 0) || (start == NULL)) {
	return TCL_ERROR;
    }
    if (numBytes < 0) {
	numBytes = strlen(start);
    }

    if (!append) {
	TclParseInit(interp, start, numBytes, parsePtr);
    }

    /*
     * Generate one token for the variable, an additional token for the name,
     * plus any number of additional tokens for the index, if there is one.
     */

    src = start;
    TclGrowParseTokenArray(parsePtr, 2);
    tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
    tokenPtr->type = TCL_TOKEN_VARIABLE;
    tokenPtr->start = src;
    varIndex = parsePtr->numTokens;
    parsePtr->numTokens++;
    tokenPtr++;
    src++;
    numBytes--;
    if (numBytes == 0) {
	goto justADollarSign;
    }
    tokenPtr->type = TCL_TOKEN_TEXT;
    tokenPtr->start = src;
    tokenPtr->numComponents = 0;

    /*
     * The name of the variable can have three forms:
     * 1. The $ sign is followed by an open curly brace. Then the variable
     *	  name is everything up to the next close curly brace, and the
     *	  variable is a scalar variable.
     * 2. The $ sign is not followed by an open curly brace. Then the variable
     *	  name is everything up to the next character that isn't a letter,
     *	  digit, or underscore. :: sequences are also considered part of the
     *	  variable name, in order to support namespaces. If the following
     *	  character is an open parenthesis, then the information between
     *	  parentheses is the array element name.
     * 3. The $ sign is followed by something that isn't a letter, digit, or
     *	  underscore: in this case, there is no variable name and the token is
     *	  just "$".
     */

    if (*src == '{') {
	src++;
	numBytes--;
	tokenPtr->type = TCL_TOKEN_TEXT;
	tokenPtr->start = src;
	tokenPtr->numComponents = 0;

	while (numBytes && (*src != '}')) {
	    numBytes--;
	    src++;
	}
	if (numBytes == 0) {
	    if (parsePtr->interp != NULL) {
		Tcl_SetObjResult(parsePtr->interp, Tcl_NewStringObj(
			"missing close-brace for variable name", -1));
	    }
	    parsePtr->errorType = TCL_PARSE_MISSING_VAR_BRACE;
	    parsePtr->term = tokenPtr->start-1;
	    parsePtr->incomplete = 1;
	    goto error;
	}
	tokenPtr->size = src - tokenPtr->start;
	tokenPtr[-1].size = src - tokenPtr[-1].start;
	parsePtr->numTokens++;
	src++;
    } else {
	tokenPtr->type = TCL_TOKEN_TEXT;
	tokenPtr->start = src;
	tokenPtr->numComponents = 0;

	while (numBytes) {
	    if (TclIsBareword(*src)) {
		src += 1;
		numBytes -= 1;
		continue;
	    }
	    if ((src[0] == ':') && (numBytes != 1) && (src[1] == ':')) {
		src += 2;
		numBytes -= 2;
		while (numBytes && (*src == ':')) {
		    src++;
		    numBytes--;
		}
		continue;
	    }
	    break;
	}

	/*
	 * Support for empty array names here.
	 */

	array = (numBytes && (*src == '('));
	tokenPtr->size = src - tokenPtr->start;
	if ((tokenPtr->size == 0) && !array) {
	    goto justADollarSign;
	}
	parsePtr->numTokens++;
	if (array) {
	    /*
	     * This is a reference to an array element. Call ParseTokens
	     * recursively to parse the element name, since it could contain
	     * any number of substitutions.
	     */

	    if (TCL_OK != ParseTokens(src+1, numBytes-1, TYPE_CLOSE_PAREN,
		    TCL_SUBST_ALL, parsePtr)) {
		goto error;
	    }
	    if ((parsePtr->term == src+numBytes) || (*parsePtr->term != ')')){
		if (parsePtr->interp != NULL) {
		    Tcl_SetObjResult(parsePtr->interp, Tcl_NewStringObj(
			    "missing )", -1));
		}
		parsePtr->errorType = TCL_PARSE_MISSING_PAREN;
		parsePtr->term = src;
		parsePtr->incomplete = 1;
		goto error;
	    }
	    src = parsePtr->term + 1;
	}
    }
    tokenPtr = &parsePtr->tokenPtr[varIndex];
    tokenPtr->size = src - tokenPtr->start;
    tokenPtr->numComponents = parsePtr->numTokens - (varIndex + 1);
    return TCL_OK;

    /*
     * The dollar sign isn't followed by a variable name. Replace the
     * TCL_TOKEN_VARIABLE token with a TCL_TOKEN_TEXT token for the dollar
     * sign.
     */

  justADollarSign:
    tokenPtr = &parsePtr->tokenPtr[varIndex];
    tokenPtr->type = TCL_TOKEN_TEXT;
    tokenPtr->size = 1;
    tokenPtr->numComponents = 0;
    return TCL_OK;

  error:
    Tcl_FreeParse(parsePtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseVar --
 *
 *	Given a string starting with a $ sign, parse off a variable name and
 *	return its value.
 *
 * Results:
 *	The return value is the contents of the variable given by the leading
 *	characters of string. If termPtr isn't NULL, *termPtr gets filled in
 *	with the address of the character just after the last one in the
 *	variable specifier. If the variable doesn't exist, then the return
 *	value is NULL and an error message will be left in interp's result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
Tcl_ParseVar(
    Tcl_Interp *interp,		/* Context for looking up variable. */
    register const char *start,	/* Start of variable substitution. First
				 * character must be "$". */
    const char **termPtr)	/* If non-NULL, points to word to fill in with
				 * character just after last one in the
				 * variable specifier. */
{
    register Tcl_Obj *objPtr;
    int code;
    Tcl_Parse *parsePtr = TclStackAlloc(interp, sizeof(Tcl_Parse));

    if (Tcl_ParseVarName(interp, start, -1, parsePtr, 0) != TCL_OK) {
	TclStackFree(interp, parsePtr);
	return NULL;
    }

    if (termPtr != NULL) {
	*termPtr = start + parsePtr->tokenPtr->size;
    }
    if (parsePtr->numTokens == 1) {
	/*
	 * There isn't a variable name after all: the $ is just a $.
	 */

	TclStackFree(interp, parsePtr);
	return "$";
    }

    code = TclSubstTokens(interp, parsePtr->tokenPtr, parsePtr->numTokens,
	    NULL, 1, NULL, NULL);
    Tcl_FreeParse(parsePtr);
    TclStackFree(interp, parsePtr);
    if (code != TCL_OK) {
	return NULL;
    }
    objPtr = Tcl_GetObjResult(interp);

    /*
     * At this point we should have an object containing the value of a
     * variable. Just return the string from that object.
     *
     * Since TclSubstTokens above returned TCL_OK, we know that objPtr
     * is shared.  It is in both the interp result and the value of the
     * variable.  Returning the string relies on that to be true.
     */

    assert( Tcl_IsShared(objPtr) );

    Tcl_ResetResult(interp);
    return TclGetString(objPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseBraces --
 *
 *	Given a string in braces such as a Tcl command argument or a string
 *	value in a Tcl expression, this function parses the string and returns
 *	information about the parse. No more than numBytes bytes will be
 *	scanned.
 *
 * Results:
 *	The return value is TCL_OK if the string was parsed successfully and
 *	TCL_ERROR otherwise. If an error occurs and interp isn't NULL then an
 *	error message is left in its result. On a successful return, tokenPtr
 *	and numTokens fields of parsePtr are filled in with information about
 *	the string that was parsed. Other fields in parsePtr are undefined.
 *	termPtr is set to point to the character just after the last one in
 *	the braced string.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the information
 *	about the command, then additional space is malloc-ed. If the function
 *	returns TCL_OK then the caller must eventually invoke Tcl_FreeParse to
 *	release any additional space that was allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseBraces(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting; if
				 * NULL, then no error message is provided. */
    const char *start,		/* Start of string enclosed in braces. The
				 * first character must be {'. */
    register int numBytes,	/* Total number of bytes in string. If < 0,
				 * the string consists of all bytes up to the
				 * first null character. */
    register Tcl_Parse *parsePtr,
				/* Structure to fill in with information about
				 * the string. */
    int append,			/* Non-zero means append tokens to existing
				 * information in parsePtr; zero means ignore
				 * existing tokens in parsePtr and
				 * reinitialize it. */
    const char **termPtr)	/* If non-NULL, points to word in which to
				 * store a pointer to the character just after
				 * the terminating '}' if the parse was
				 * successful. */
{
    Tcl_Token *tokenPtr;
    register const char *src;
    int startIndex, level, length;

    if ((numBytes == 0) || (start == NULL)) {
	return TCL_ERROR;
    }
    if (numBytes < 0) {
	numBytes = strlen(start);
    }

    if (!append) {
	TclParseInit(interp, start, numBytes, parsePtr);
    }

    src = start;
    startIndex = parsePtr->numTokens;

    TclGrowParseTokenArray(parsePtr, 1);
    tokenPtr = &parsePtr->tokenPtr[startIndex];
    tokenPtr->type = TCL_TOKEN_TEXT;
    tokenPtr->start = src+1;
    tokenPtr->numComponents = 0;
    level = 1;
    while (1) {
	while (++src, --numBytes) {
	    if (CHAR_TYPE(*src) != TYPE_NORMAL) {
		break;
	    }
	}
	if (numBytes == 0) {
	    goto missingBraceError;
	}

	switch (*src) {
	case '{':
	    level++;
	    break;
	case '}':
	    if (--level == 0) {
		/*
		 * Decide if we need to finish emitting a partially-finished
		 * token. There are 3 cases:
		 *     {abc \newline xyz} or {xyz}
		 *		- finish emitting "xyz" token
		 *     {abc \newline}
		 *		- don't emit token after \newline
		 *     {}	- finish emitting zero-sized token
		 *
		 * The last case ensures that there is a token (even if empty)
		 * that describes the braced string.
		 */

		if ((src != tokenPtr->start)
			|| (parsePtr->numTokens == startIndex)) {
		    tokenPtr->size = (src - tokenPtr->start);
		    parsePtr->numTokens++;
		}
		if (termPtr != NULL) {
		    *termPtr = src+1;
		}
		return TCL_OK;
	    }
	    break;
	case '\\':
	    TclParseBackslash(src, numBytes, &length, NULL);
	    if ((length > 1) && (src[1] == '\n')) {
		/*
		 * A backslash-newline sequence must be collapsed, even inside
		 * braces, so we have to split the word into multiple tokens
		 * so that the backslash-newline can be represented
		 * explicitly.
		 */

		if (numBytes == 2) {
		    parsePtr->incomplete = 1;
		}
		tokenPtr->size = (src - tokenPtr->start);
		if (tokenPtr->size != 0) {
		    parsePtr->numTokens++;
		}
		TclGrowParseTokenArray(parsePtr, 2);
		tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
		tokenPtr->type = TCL_TOKEN_BS;
		tokenPtr->start = src;
		tokenPtr->size = length;
		tokenPtr->numComponents = 0;
		parsePtr->numTokens++;

		src += length - 1;
		numBytes -= length - 1;
		tokenPtr++;
		tokenPtr->type = TCL_TOKEN_TEXT;
		tokenPtr->start = src + 1;
		tokenPtr->numComponents = 0;
	    } else {
		src += length - 1;
		numBytes -= length - 1;
	    }
	    break;
	}
    }

  missingBraceError:
    parsePtr->errorType = TCL_PARSE_MISSING_BRACE;
    parsePtr->term = start;
    parsePtr->incomplete = 1;
    if (parsePtr->interp == NULL) {
	/*
	 * Skip straight to the exit code since we have no interpreter to put
	 * error message in.
	 */

	goto error;
    }

    Tcl_SetObjResult(parsePtr->interp, Tcl_NewStringObj(
	    "missing close-brace", -1));

    /*
     * Guess if the problem is due to comments by searching the source string
     * for a possible open brace within the context of a comment. Since we
     * aren't performing a full Tcl parse, just look for an open brace
     * preceded by a '<whitespace>#' on the same line.
     */

    {
	register int openBrace = 0;

	while (--src > start) {
	    switch (*src) {
	    case '{':
		openBrace = 1;
		break;
	    case '\n':
		openBrace = 0;
		break;
	    case '#' :
		if (openBrace && TclIsSpaceProc(src[-1])) {
		    Tcl_AppendToObj(Tcl_GetObjResult(parsePtr->interp),
			    ": possible unbalanced brace in comment", -1);
		    goto error;
		}
		break;
	    }
	}
    }

  error:
    Tcl_FreeParse(parsePtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseQuotedString --
 *
 *	Given a double-quoted string such as a quoted Tcl command argument or
 *	a quoted value in a Tcl expression, this function parses the string
 *	and returns information about the parse. No more than numBytes bytes
 *	will be scanned.
 *
 * Results:
 *	The return value is TCL_OK if the string was parsed successfully and
 *	TCL_ERROR otherwise. If an error occurs and interp isn't NULL then an
 *	error message is left in its result. On a successful return, tokenPtr
 *	and numTokens fields of parsePtr are filled in with information about
 *	the string that was parsed. Other fields in parsePtr are undefined.
 *	termPtr is set to point to the character just after the quoted
 *	string's terminating close-quote.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the information
 *	about the command, then additional space is malloc-ed. If the function
 *	returns TCL_OK then the caller must eventually invoke Tcl_FreeParse to
 *	release any additional space that was allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseQuotedString(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting; if
				 * NULL, then no error message is provided. */
    const char *start,		/* Start of the quoted string. The first
				 * character must be '"'. */
    register int numBytes,	/* Total number of bytes in string. If < 0,
				 * the string consists of all bytes up to the
				 * first null character. */
    register Tcl_Parse *parsePtr,
				/* Structure to fill in with information about
				 * the string. */
    int append,			/* Non-zero means append tokens to existing
				 * information in parsePtr; zero means ignore
				 * existing tokens in parsePtr and
				 * reinitialize it. */
    const char **termPtr)	/* If non-NULL, points to word in which to
				 * store a pointer to the character just after
				 * the quoted string's terminating close-quote
				 * if the parse succeeds. */
{
    if ((numBytes == 0) || (start == NULL)) {
	return TCL_ERROR;
    }
    if (numBytes < 0) {
	numBytes = strlen(start);
    }

    if (!append) {
	TclParseInit(interp, start, numBytes, parsePtr);
    }

    if (TCL_OK != ParseTokens(start+1, numBytes-1, TYPE_QUOTE, TCL_SUBST_ALL,
	    parsePtr)) {
	goto error;
    }
    if (*parsePtr->term != '"') {
	if (parsePtr->interp != NULL) {
	    Tcl_SetObjResult(parsePtr->interp, Tcl_NewStringObj(
		    "missing \"", -1));
	}
	parsePtr->errorType = TCL_PARSE_MISSING_QUOTE;
	parsePtr->term = start;
	parsePtr->incomplete = 1;
	goto error;
    }
    if (termPtr != NULL) {
	*termPtr = (parsePtr->term + 1);
    }
    return TCL_OK;

  error:
    Tcl_FreeParse(parsePtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclSubstParse --
 *
 *	Token parser used by the [subst] command. Parses the string made up of
 *	'numBytes' bytes starting at 'bytes'. Parsing is controlled by the
 *	flags argument to provide support for the -nobackslashes, -nocommands,
 *	and -novariables options, as represented by the flag values
 *	TCL_SUBST_BACKSLASHES, TCL_SUBST_COMMANDS, TCL_SUBST_VARIABLES.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Tcl_Parse struct '*parsePtr' is filled with parse results.
 *	The caller is expected to eventually call Tcl_FreeParse() to properly
 *	cleanup the value written there.
 *
 *	If a parse error occurs, the Tcl_InterpState value '*statePtr' is
 *	filled with the state created by that error. When *statePtr is written
 *	to, the caller is expected to make the required calls to either
 *	Tcl_RestoreInterpState() or Tcl_DiscardInterpState() to dispose of the
 *	value written there.
 *
 *----------------------------------------------------------------------
 */

void
TclSubstParse(
    Tcl_Interp *interp,
    const char *bytes,
    int numBytes,
    int flags,
    Tcl_Parse *parsePtr,
    Tcl_InterpState *statePtr)
{
    int length = numBytes;
    const char *p = bytes;

    TclParseInit(interp, p, length, parsePtr);

    /*
     * First parse the string rep of objPtr, as if it were enclosed as a
     * "-quoted word in a normal Tcl command. Honor flags that selectively
     * inhibit types of substitution.
     */

    if (TCL_OK != ParseTokens(p, length, /* mask */ 0, flags, parsePtr)) {
	/*
	 * There was a parse error. Save the interpreter state for possible
	 * error reporting later.
	 */

	*statePtr = Tcl_SaveInterpState(interp, TCL_ERROR);

	/*
	 * We need to re-parse to get the portion of the string we can [subst]
	 * before the parse error. Sadly, all the Tcl_Token's created by the
	 * first parse attempt are gone, freed according to the public spec
	 * for the Tcl_Parse* routines. The only clue we have is parse.term,
	 * which points to either the unmatched opener, or to characters that
	 * follow a close brace or close quote.
	 *
	 * Call ParseTokens again, working on the string up to parse.term.
	 * Keep repeating until we get a good parse on a prefix.
	 */

	do {
	    parsePtr->numTokens = 0;
	    parsePtr->tokensAvailable = NUM_STATIC_TOKENS;
	    parsePtr->end = parsePtr->term;
	    parsePtr->incomplete = 0;
	    parsePtr->errorType = TCL_PARSE_SUCCESS;
	} while (TCL_OK !=
		ParseTokens(p, parsePtr->end - p, 0, flags, parsePtr));

	/*
	 * The good parse will have to be followed by {, (, or [.
	 */

	switch (*(parsePtr->term)) {
	case '{':
	    /*
	     * Parse error was a missing } in a ${varname} variable
	     * substitution at the toplevel. We will subst everything up to
	     * that broken variable substitution before reporting the parse
	     * error. Substituting the leftover '$' will have no side-effects,
	     * so the current token stream is fine.
	     */
	    break;

	case '(':
	    /*
	     * Parse error was during the parsing of the index part of an
	     * array variable substitution at the toplevel.
	     */

	    if (*(parsePtr->term - 1) == '$') {
		/*
		 * Special case where removing the array index left us with
		 * just a dollar sign (array variable with name the empty
		 * string as its name), instead of with a scalar variable
		 * reference.
		 *
		 * As in the previous case, existing token stream is OK.
		 */
	    } else {
		/*
		 * The current parse includes a successful parse of a scalar
		 * variable substitution where there should have been an array
		 * variable substitution. We remove that mistaken part of the
		 * parse before moving on. A scalar variable substitution is
		 * two tokens.
		 */

		Tcl_Token *varTokenPtr =
			parsePtr->tokenPtr + parsePtr->numTokens - 2;

		if (varTokenPtr->type != TCL_TOKEN_VARIABLE) {
		    Tcl_Panic("TclSubstParse: programming error");
		}
		if (varTokenPtr[1].type != TCL_TOKEN_TEXT) {
		    Tcl_Panic("TclSubstParse: programming error");
		}
		parsePtr->numTokens -= 2;
	    }
	    break;
	case '[':
	    /*
	     * Parse error occurred during parsing of a toplevel command
	     * substitution.
	     */

	    parsePtr->end = p + length;
	    p = parsePtr->term + 1;
	    length = parsePtr->end - p;
	    if (length == 0) {
		/*
		 * No commands, just an unmatched [. As in previous cases,
		 * existing token stream is OK.
		 */
	    } else {
		/*
		 * We want to add the parsing of as many commands as we can
		 * within that substitution until we reach the actual parse
		 * error. We'll do additional parsing to determine what length
		 * to claim for the final TCL_TOKEN_COMMAND token.
		 */

		Tcl_Token *tokenPtr;
		const char *lastTerm = parsePtr->term;
		Tcl_Parse *nestedPtr =
			TclStackAlloc(interp, sizeof(Tcl_Parse));

		while (TCL_OK ==
			Tcl_ParseCommand(NULL, p, length, 0, nestedPtr)) {
		    Tcl_FreeParse(nestedPtr);
		    p = nestedPtr->term + (nestedPtr->term < nestedPtr->end);
		    length = nestedPtr->end - p;
		    if ((length == 0) && (nestedPtr->term == nestedPtr->end)) {
			/*
			 * If we run out of string, blame the missing close
			 * bracket on the last command, and do not evaluate it
			 * during substitution.
			 */

			break;
		    }
		    lastTerm = nestedPtr->term;
		}
		TclStackFree(interp, nestedPtr);

		if (lastTerm == parsePtr->term) {
		    /*
		     * Parse error in first command. No commands to subst, add
		     * no more tokens.
		     */
		    break;
		}

		/*
		 * Create a command substitution token for whatever commands
		 * got parsed.
		 */

		TclGrowParseTokenArray(parsePtr, 1);
		tokenPtr = &(parsePtr->tokenPtr[parsePtr->numTokens]);
		tokenPtr->start = parsePtr->term;
		tokenPtr->numComponents = 0;
		tokenPtr->type = TCL_TOKEN_COMMAND;
		tokenPtr->size = lastTerm - tokenPtr->start + 1;
		parsePtr->numTokens++;
	    }
	    break;

	default:
	    Tcl_Panic("bad parse in TclSubstParse: %c", p[length]);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclSubstTokens --
 *
 *	Accepts an array of count Tcl_Token's, and creates a result value in
 *	the interp from concatenating the results of performing Tcl
 *	substitution on each Tcl_Token. Substitution is interrupted if any
 *	non-TCL_OK completion code arises.
 *
 * Results:
 *	The return value is a standard Tcl completion code. The result in
 *	interp is the substituted value, or an error message if TCL_ERROR is
 *	returned. If tokensLeftPtr is not NULL, then it points to an int where
 *	the number of tokens remaining to be processed is written.
 *
 * Side effects:
 *	Can be anything, depending on the types of substitution done.
 *
 *----------------------------------------------------------------------
 */

int
TclSubstTokens(
    Tcl_Interp *interp,		/* Interpreter in which to lookup variables,
				 * execute nested commands, and report
				 * errors. */
    Tcl_Token *tokenPtr,	/* Pointer to first in an array of tokens to
				 * evaluate and concatenate. */
    int count,			/* Number of tokens to consider at tokenPtr.
				 * Must be at least 1. */
    int *tokensLeftPtr,		/* If not NULL, points to memory where an
				 * integer representing the number of tokens
				 * left to be substituted will be written */
    int line,			/* The line the script starts on. */
    int *clNextOuter,		/* Information about an outer context for */
    const char *outerScript)	/* continuation line data. This is set by
				 * EvalEx() to properly handle [...]-nested
				 * commands. The 'outerScript' refers to the
				 * most-outer script containing the embedded
				 * command, which is refered to by 'script'.
				 * The 'clNextOuter' refers to the current
				 * entry in the table of continuation lines in
				 * this "master script", and the character
				 * offsets are relative to the 'outerScript'
				 * as well.
				 *
				 * If outerScript == script, then this call is
				 * for words in the outer-most script or
				 * command. See Tcl_EvalEx and TclEvalObjEx
				 * for the places generating arguments for
				 * which this is true. */
{
    Tcl_Obj *result;
    int code = TCL_OK;
#define NUM_STATIC_POS 20
    int isLiteral, maxNumCL, numCL, i, adjust;
    int *clPosition = NULL;
    Interp *iPtr = (Interp *) interp;
    int inFile = iPtr->evalFlags & TCL_EVAL_FILE;

    /*
     * Each pass through this loop will substitute one token, and its
     * components, if any. The only thing tricky here is that we go to some
     * effort to pass Tcl_Obj's through untouched, to avoid string copying and
     * Tcl_Obj creation if possible, to aid performance and limit shimmering.
     *
     * Further optimization opportunities might be to check for the equivalent
     * of Tcl_SetObjResult(interp, Tcl_GetObjResult(interp)) and omit them.
     */

    /*
     * For the handling of continuation lines in literals we first check if
     * this is actually a literal. For if not we can forego the additional
     * processing. Otherwise we pre-allocate a small table to store the
     * locations of all continuation lines we find in this literal, if any.
     * The table is extended if needed.
     */

    numCL = 0;
    maxNumCL = 0;
    isLiteral = 1;
    for (i=0 ; i < count; i++) {
	if ((tokenPtr[i].type != TCL_TOKEN_TEXT)
		&& (tokenPtr[i].type != TCL_TOKEN_BS)) {
	    isLiteral = 0;
	    break;
	}
    }

    if (isLiteral) {
	maxNumCL = NUM_STATIC_POS;
	clPosition = ckalloc(maxNumCL * sizeof(int));
    }

    adjust = 0;
    result = NULL;
    for (; count>0 && code==TCL_OK ; count--, tokenPtr++) {
	Tcl_Obj *appendObj = NULL;
	const char *append = NULL;
	int appendByteLength = 0;
	char utfCharBytes[TCL_UTF_MAX] = "";

	switch (tokenPtr->type) {
	case TCL_TOKEN_TEXT:
	    append = tokenPtr->start;
	    appendByteLength = tokenPtr->size;
	    break;

	case TCL_TOKEN_BS:
	    appendByteLength = TclParseBackslash(tokenPtr->start,
		    tokenPtr->size, NULL, utfCharBytes);
	    append = utfCharBytes;

	    /*
	     * If the backslash sequence we found is in a literal, and
	     * represented a continuation line, we compute and store its
	     * location (as char offset to the beginning of the _result_
	     * script). We may have to extend the table of locations.
	     *
	     * Note that the continuation line information is relevant even if
	     * the word we are processing is not a literal, as it can affect
	     * nested commands. See the branch for TCL_TOKEN_COMMAND below,
	     * where the adjustment we are tracking here is taken into
	     * account. The good thing is that we do not need a table of
	     * everything, just the number of lines we have to add as
	     * correction.
	     */

	    if ((appendByteLength == 1) && (utfCharBytes[0] == ' ')
		    && (tokenPtr->start[1] == '\n')) {
		if (isLiteral) {
		    int clPos;

		    if (result == 0) {
			clPos = 0;
		    } else {
			Tcl_GetStringFromObj(result, &clPos);
		    }

		    if (numCL >= maxNumCL) {
			maxNumCL *= 2;
			clPosition = ckrealloc(clPosition,
				maxNumCL * sizeof(int));
		    }
		    clPosition[numCL] = clPos;
		    numCL++;
		}
		adjust++;
	    }
	    break;

	case TCL_TOKEN_COMMAND: {
	    /* TIP #280: Transfer line information to nested command */
	    iPtr->numLevels++;
	    code = TclInterpReady(interp);
	    if (code == TCL_OK) {
		/*
		 * Test cases: info-30.{6,8,9}
		 */

		int theline;

		TclAdvanceContinuations(&line, &clNextOuter,
			tokenPtr->start - outerScript);
		theline = line + adjust;
		code = TclEvalEx(interp, tokenPtr->start+1, tokenPtr->size-2,
			0, theline, clNextOuter, outerScript);

		TclAdvanceLines(&line, tokenPtr->start+1,
			tokenPtr->start + tokenPtr->size - 1);

		/*
		 * Restore flag reset by nested eval for future bracketed
		 * commands and their cmdframe setup
		 */

		if (inFile) {
		    iPtr->evalFlags |= TCL_EVAL_FILE;
		}
	    }
	    iPtr->numLevels--;
	    TclResetCancellation(interp, 0);
	    appendObj = Tcl_GetObjResult(interp);
	    break;
	}

	case TCL_TOKEN_VARIABLE: {
	    Tcl_Obj *arrayIndex = NULL;
	    Tcl_Obj *varName = NULL;

	    if (tokenPtr->numComponents > 1) {
		/*
		 * Subst the index part of an array variable reference.
		 */

		code = TclSubstTokens(interp, tokenPtr+2,
			tokenPtr->numComponents - 1, NULL, line, NULL, NULL);
		arrayIndex = Tcl_GetObjResult(interp);
		Tcl_IncrRefCount(arrayIndex);
	    }

	    if (code == TCL_OK) {
		varName = Tcl_NewStringObj(tokenPtr[1].start,
			tokenPtr[1].size);
		appendObj = Tcl_ObjGetVar2(interp, varName, arrayIndex,
			TCL_LEAVE_ERR_MSG);
		Tcl_DecrRefCount(varName);
		if (appendObj == NULL) {
		    code = TCL_ERROR;
		}
	    }

	    switch (code) {
	    case TCL_OK:	/* Got value */
	    case TCL_ERROR:	/* Already have error message */
	    case TCL_BREAK:	/* Will not substitute anyway */
	    case TCL_CONTINUE:	/* Will not substitute anyway */
		break;
	    default:
		/*
		 * All other return codes, we will subst the result from the
		 * code-throwing evaluation.
		 */

		appendObj = Tcl_GetObjResult(interp);
	    }

	    if (arrayIndex != NULL) {
		Tcl_DecrRefCount(arrayIndex);
	    }
	    count -= tokenPtr->numComponents;
	    tokenPtr += tokenPtr->numComponents;
	    break;
	}

	default:
	    Tcl_Panic("unexpected token type in TclSubstTokens: %d",
		    tokenPtr->type);
	}

	if ((code == TCL_BREAK) || (code == TCL_CONTINUE)) {
	    /*
	     * Inhibit substitution.
	     */
	    continue;
	}

	if (result == NULL) {
	    /*
	     * First pass through. If we have a Tcl_Obj, just use it. If not,
	     * create one from our string.
	     */

	    if (appendObj != NULL) {
		result = appendObj;
	    } else {
		result = Tcl_NewStringObj(append, appendByteLength);
	    }
	    Tcl_IncrRefCount(result);
	} else {
	    /*
	     * Subsequent passes. Append to result.
	     */

	    if (Tcl_IsShared(result)) {
		Tcl_DecrRefCount(result);
		result = Tcl_DuplicateObj(result);
		Tcl_IncrRefCount(result);
	    }
	    if (appendObj != NULL) {
		Tcl_AppendObjToObj(result, appendObj);
	    } else {
		Tcl_AppendToObj(result, append, appendByteLength);
	    }
	}
    }

    if (code != TCL_ERROR) {		/* Keep error message in result! */
	if (result != NULL) {
	    Tcl_SetObjResult(interp, result);

	    /*
	     * If the code found continuation lines (which implies that this
	     * word is a literal), then we store the accumulated table of
	     * locations in the thread-global data structure for the bytecode
	     * compiler to find later, assuming that the literal is a script
	     * which will be compiled.
	     */

	    if (numCL) {
		TclContinuationsEnter(result, numCL, clPosition);
	    }

	    /*
	     * Release the temp table we used to collect the locations of
	     * continuation lines, if any.
	     */

	    if (maxNumCL) {
		ckfree(clPosition);
	    }
	} else {
	    Tcl_ResetResult(interp);
	}
    }
    if (tokensLeftPtr != NULL) {
	*tokensLeftPtr = count;
    }
    if (result != NULL) {
	Tcl_DecrRefCount(result);
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * CommandComplete --
 *
 *	This function is shared by TclCommandComplete and
 *	Tcl_ObjCommandComplete; it does all the real work of seeing whether a
 *	script is complete
 *
 * Results:
 *	1 is returned if the script is complete, 0 if there are open
 *	delimiters such as " or (. 1 is also returned if there is a parse
 *	error in the script other than unmatched delimiters.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static inline int
CommandComplete(
    const char *script,		/* Script to check. */
    int numBytes)		/* Number of bytes in script. */
{
    Tcl_Parse parse;
    const char *p, *end;
    int result;

    p = script;
    end = p + numBytes;
    while (Tcl_ParseCommand(NULL, p, end - p, 0, &parse) == TCL_OK) {
	p = parse.commandStart + parse.commandSize;
	if (p >= end) {
	    break;
	}
	Tcl_FreeParse(&parse);
    }
    if (parse.incomplete) {
	result = 0;
    } else {
	result = 1;
    }
    Tcl_FreeParse(&parse);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CommandComplete --
 *
 *	Given a partial or complete Tcl script, this function determines
 *	whether the script is complete in the sense of having matched braces
 *	and quotes and brackets.
 *
 * Results:
 *	1 is returned if the script is complete, 0 otherwise. 1 is also
 *	returned if there is a parse error in the script other than unmatched
 *	delimiters.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_CommandComplete(
    const char *script)		/* Script to check. */
{
    return CommandComplete(script, (int) strlen(script));
}

/*
 *----------------------------------------------------------------------
 *
 * TclObjCommandComplete --
 *
 *	Given a partial or complete Tcl command in a Tcl object, this function
 *	determines whether the command is complete in the sense of having
 *	matched braces and quotes and brackets.
 *
 * Results:
 *	1 is returned if the command is complete, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclObjCommandComplete(
    Tcl_Obj *objPtr)		/* Points to object holding script to
				 * check. */
{
    int length;
    const char *script = Tcl_GetStringFromObj(objPtr, &length);

    return CommandComplete(script, length);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
