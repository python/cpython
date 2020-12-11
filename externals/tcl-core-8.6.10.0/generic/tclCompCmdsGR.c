/*
 * tclCompCmdsGR.c --
 *
 *	This file contains compilation procedures that compile various Tcl
 *	commands (beginning with the letters 'g' through 'r') into a sequence
 *	of instructions ("bytecodes").
 *
 * Copyright (c) 1997-1998 Sun Microsystems, Inc.
 * Copyright (c) 2001 by Kevin B. Kenny.  All rights reserved.
 * Copyright (c) 2002 ActiveState Corporation.
 * Copyright (c) 2004-2013 by Donal K. Fellows.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclCompile.h"
#include <assert.h>

/*
 * Prototypes for procedures defined later in this file:
 */

static void		CompileReturnInternal(CompileEnv *envPtr,
			    unsigned char op, int code, int level,
			    Tcl_Obj *returnOpts);
static int		IndexTailVarIfKnown(Tcl_Interp *interp,
			    Tcl_Token *varTokenPtr, CompileEnv *envPtr);


/*
 *----------------------------------------------------------------------
 *
 * TclGetIndexFromToken --
 *
 *	Parse a token to determine if an index value is known at
 *	compile time.
 *
 * Returns:
 *	TCL_OK if parsing succeeded, and TCL_ERROR if it failed.
 *
 * Side effects:
 *	When TCL_OK is returned, the encoded index value is written
 *	to *index.
 *
 *----------------------------------------------------------------------
 */

int
TclGetIndexFromToken(
    Tcl_Token *tokenPtr,
    int before,
    int after,
    int *indexPtr)
{
    Tcl_Obj *tmpObj = Tcl_NewObj();
    int result = TCL_ERROR;

    if (TclWordKnownAtCompileTime(tokenPtr, tmpObj)) {
	result = TclIndexEncode(NULL, tmpObj, before, after, indexPtr);
    }
    Tcl_DecrRefCount(tmpObj);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileGlobalCmd --
 *
 *	Procedure called to compile the "global" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "global" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileGlobalCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *varTokenPtr;
    int localIndex, numWords, i;
    DefineLineInformation;	/* TIP #280 */

    /* TODO: Consider support for compiling expanded args. */
    numWords = parsePtr->numWords;
    if (numWords < 2) {
	return TCL_ERROR;
    }

    /*
     * 'global' has no effect outside of proc bodies; handle that at runtime
     */

    if (envPtr->procPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Push the namespace
     */

    PushStringLiteral(envPtr, "::");

    /*
     * Loop over the variables.
     */

    varTokenPtr = TokenAfter(parsePtr->tokenPtr);
    for (i=1; i<numWords; varTokenPtr = TokenAfter(varTokenPtr),i++) {
	localIndex = IndexTailVarIfKnown(interp, varTokenPtr, envPtr);

	if (localIndex < 0) {
	    return TCL_ERROR;
	}

	/* TODO: Consider what value can pass through the
	 * IndexTailVarIfKnown() screen.  Full CompileWord()
	 * likely does not apply here.  Push known value instead. */
	CompileWord(envPtr, varTokenPtr, interp, i);
	TclEmitInstInt4(	INST_NSUPVAR, localIndex,	envPtr);
    }

    /*
     * Pop the namespace, and set the result to empty
     */

    TclEmitOpcode(		INST_POP,			envPtr);
    PushStringLiteral(envPtr, "");
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileIfCmd --
 *
 *	Procedure called to compile the "if" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "if" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileIfCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    JumpFixupArray jumpFalseFixupArray;
				/* Used to fix the ifFalse jump after each
				 * test when its target PC is determined. */
    JumpFixupArray jumpEndFixupArray;
				/* Used to fix the jump after each "then" body
				 * to the end of the "if" when that PC is
				 * determined. */
    Tcl_Token *tokenPtr, *testTokenPtr;
    int jumpIndex = 0;		/* Avoid compiler warning. */
    int jumpFalseDist, numWords, wordIdx, numBytes, j, code;
    const char *word;
    int realCond = 1;		/* Set to 0 for static conditions:
				 * "if 0 {..}" */
    int boolVal;		/* Value of static condition. */
    int compileScripts = 1;
    DefineLineInformation;	/* TIP #280 */

    /*
     * Only compile the "if" command if all arguments are simple words, in
     * order to insure correct substitution [Bug 219166]
     */

    tokenPtr = parsePtr->tokenPtr;
    wordIdx = 0;
    numWords = parsePtr->numWords;

    for (wordIdx = 0; wordIdx < numWords; wordIdx++) {
	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    return TCL_ERROR;
	}
	tokenPtr = TokenAfter(tokenPtr);
    }

    TclInitJumpFixupArray(&jumpFalseFixupArray);
    TclInitJumpFixupArray(&jumpEndFixupArray);
    code = TCL_OK;

    /*
     * Each iteration of this loop compiles one "if expr ?then? body" or
     * "elseif expr ?then? body" clause.
     */

    tokenPtr = parsePtr->tokenPtr;
    wordIdx = 0;
    while (wordIdx < numWords) {
	/*
	 * Stop looping if the token isn't "if" or "elseif".
	 */

	word = tokenPtr[1].start;
	numBytes = tokenPtr[1].size;
	if ((tokenPtr == parsePtr->tokenPtr)
		|| ((numBytes == 6) && (strncmp(word, "elseif", 6) == 0))) {
	    tokenPtr = TokenAfter(tokenPtr);
	    wordIdx++;
	} else {
	    break;
	}
	if (wordIdx >= numWords) {
	    code = TCL_ERROR;
	    goto done;
	}

	/*
	 * Compile the test expression then emit the conditional jump around
	 * the "then" part.
	 */

	testTokenPtr = tokenPtr;

	if (realCond) {
	    /*
	     * Find out if the condition is a constant.
	     */

	    Tcl_Obj *boolObj = Tcl_NewStringObj(testTokenPtr[1].start,
		    testTokenPtr[1].size);

	    Tcl_IncrRefCount(boolObj);
	    code = Tcl_GetBooleanFromObj(NULL, boolObj, &boolVal);
	    TclDecrRefCount(boolObj);
	    if (code == TCL_OK) {
		/*
		 * A static condition.
		 */

		realCond = 0;
		if (!boolVal) {
		    compileScripts = 0;
		}
	    } else {
		SetLineInformation(wordIdx);
		Tcl_ResetResult(interp);
		TclCompileExprWords(interp, testTokenPtr, 1, envPtr);
		if (jumpFalseFixupArray.next >= jumpFalseFixupArray.end) {
		    TclExpandJumpFixupArray(&jumpFalseFixupArray);
		}
		jumpIndex = jumpFalseFixupArray.next;
		jumpFalseFixupArray.next++;
		TclEmitForwardJump(envPtr, TCL_FALSE_JUMP,
			jumpFalseFixupArray.fixup+jumpIndex);
	    }
	    code = TCL_OK;
	}

	/*
	 * Skip over the optional "then" before the then clause.
	 */

	tokenPtr = TokenAfter(testTokenPtr);
	wordIdx++;
	if (wordIdx >= numWords) {
	    code = TCL_ERROR;
	    goto done;
	}
	if (tokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	    word = tokenPtr[1].start;
	    numBytes = tokenPtr[1].size;
	    if ((numBytes == 4) && (strncmp(word, "then", 4) == 0)) {
		tokenPtr = TokenAfter(tokenPtr);
		wordIdx++;
		if (wordIdx >= numWords) {
		    code = TCL_ERROR;
		    goto done;
		}
	    }
	}

	/*
	 * Compile the "then" command body.
	 */

	if (compileScripts) {
	    BODY(tokenPtr, wordIdx);
	}

	if (realCond) {
	    /*
	     * Jump to the end of the "if" command. Both jumpFalseFixupArray
	     * and jumpEndFixupArray are indexed by "jumpIndex".
	     */

	    if (jumpEndFixupArray.next >= jumpEndFixupArray.end) {
		TclExpandJumpFixupArray(&jumpEndFixupArray);
	    }
	    jumpEndFixupArray.next++;
	    TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP,
		    jumpEndFixupArray.fixup+jumpIndex);

	    /*
	     * Fix the target of the jumpFalse after the test. Generate a 4
	     * byte jump if the distance is > 120 bytes. This is conservative,
	     * and ensures that we won't have to replace this jump if we later
	     * also need to replace the proceeding jump to the end of the "if"
	     * with a 4 byte jump.
	     */

	    TclAdjustStackDepth(-1, envPtr);
	    if (TclFixupForwardJumpToHere(envPtr,
		    jumpFalseFixupArray.fixup+jumpIndex, 120)) {
		/*
		 * Adjust the code offset for the proceeding jump to the end
		 * of the "if" command.
		 */

		jumpEndFixupArray.fixup[jumpIndex].codeOffset += 3;
	    }
	} else if (boolVal) {
	    /*
	     * We were processing an "if 1 {...}"; stop compiling scripts.
	     */

	    compileScripts = 0;
	} else {
	    /*
	     * We were processing an "if 0 {...}"; reset so that the rest
	     * (elseif, else) is compiled correctly.
	     */

	    realCond = 1;
	    compileScripts = 1;
	}

	tokenPtr = TokenAfter(tokenPtr);
	wordIdx++;
    }

    /*
     * Check for the optional else clause. Do not compile anything if this was
     * an "if 1 {...}" case.
     */

    if ((wordIdx < numWords) && (tokenPtr->type == TCL_TOKEN_SIMPLE_WORD)) {
	/*
	 * There is an else clause. Skip over the optional "else" word.
	 */

	word = tokenPtr[1].start;
	numBytes = tokenPtr[1].size;
	if ((numBytes == 4) && (strncmp(word, "else", 4) == 0)) {
	    tokenPtr = TokenAfter(tokenPtr);
	    wordIdx++;
	    if (wordIdx >= numWords) {
		code = TCL_ERROR;
		goto done;
	    }
	}

	if (compileScripts) {
	    /*
	     * Compile the else command body.
	     */

	    BODY(tokenPtr, wordIdx);
	}

	/*
	 * Make sure there are no words after the else clause.
	 */

	wordIdx++;
	if (wordIdx < numWords) {
	    code = TCL_ERROR;
	    goto done;
	}
    } else {
	/*
	 * No else clause: the "if" command's result is an empty string.
	 */

	if (compileScripts) {
	    PushStringLiteral(envPtr, "");
	}
    }

    /*
     * Fix the unconditional jumps to the end of the "if" command.
     */

    for (j = jumpEndFixupArray.next;  j > 0;  j--) {
	jumpIndex = (j - 1);	/* i.e. process the closest jump first. */
	if (TclFixupForwardJumpToHere(envPtr,
		jumpEndFixupArray.fixup+jumpIndex, 127)) {
	    /*
	     * Adjust the immediately preceeding "ifFalse" jump. We moved it's
	     * target (just after this jump) down three bytes.
	     */

	    unsigned char *ifFalsePc = envPtr->codeStart
		    + jumpFalseFixupArray.fixup[jumpIndex].codeOffset;
	    unsigned char opCode = *ifFalsePc;

	    if (opCode == INST_JUMP_FALSE1) {
		jumpFalseDist = TclGetInt1AtPtr(ifFalsePc + 1);
		jumpFalseDist += 3;
		TclStoreInt1AtPtr(jumpFalseDist, (ifFalsePc + 1));
	    } else if (opCode == INST_JUMP_FALSE4) {
		jumpFalseDist = TclGetInt4AtPtr(ifFalsePc + 1);
		jumpFalseDist += 3;
		TclStoreInt4AtPtr(jumpFalseDist, (ifFalsePc + 1));
	    } else {
		Tcl_Panic("TclCompileIfCmd: unexpected opcode \"%d\" updating ifFalse jump", (int) opCode);
	    }
	}
    }

    /*
     * Free the jumpFixupArray array if malloc'ed storage was used.
     */

  done:
    TclFreeJumpFixupArray(&jumpFalseFixupArray);
    TclFreeJumpFixupArray(&jumpEndFixupArray);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileIncrCmd --
 *
 *	Procedure called to compile the "incr" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "incr" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileIncrCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *varTokenPtr, *incrTokenPtr;
    int isScalar, localIndex, haveImmValue, immValue;
    DefineLineInformation;	/* TIP #280 */

    if ((parsePtr->numWords != 2) && (parsePtr->numWords != 3)) {
	return TCL_ERROR;
    }

    varTokenPtr = TokenAfter(parsePtr->tokenPtr);

    PushVarNameWord(interp, varTokenPtr, envPtr, TCL_NO_LARGE_INDEX,
	    &localIndex, &isScalar, 1);

    /*
     * If an increment is given, push it, but see first if it's a small
     * integer.
     */

    haveImmValue = 0;
    immValue = 1;
    if (parsePtr->numWords == 3) {
	incrTokenPtr = TokenAfter(varTokenPtr);
	if (incrTokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	    const char *word = incrTokenPtr[1].start;
	    int numBytes = incrTokenPtr[1].size;
	    int code;
	    Tcl_Obj *intObj = Tcl_NewStringObj(word, numBytes);

	    Tcl_IncrRefCount(intObj);
	    code = TclGetIntFromObj(NULL, intObj, &immValue);
	    TclDecrRefCount(intObj);
	    if ((code == TCL_OK) && (-127 <= immValue) && (immValue <= 127)) {
		haveImmValue = 1;
	    }
	    if (!haveImmValue) {
		PushLiteral(envPtr, word, numBytes);
	    }
	} else {
	    SetLineInformation(2);
	    CompileTokens(envPtr, incrTokenPtr, interp);
	}
    } else {			/* No incr amount given so use 1. */
	haveImmValue = 1;
    }

    /*
     * Emit the instruction to increment the variable.
     */

    if (isScalar) {	/* Simple scalar variable. */
	if (localIndex >= 0) {
	    if (haveImmValue) {
		TclEmitInstInt1(INST_INCR_SCALAR1_IMM, localIndex, envPtr);
		TclEmitInt1(immValue, envPtr);
	    } else {
		TclEmitInstInt1(INST_INCR_SCALAR1, localIndex,	envPtr);
	    }
	} else {
	    if (haveImmValue) {
		TclEmitInstInt1(INST_INCR_STK_IMM, immValue, envPtr);
	    } else {
		TclEmitOpcode(	INST_INCR_STK,		envPtr);
	    }
	}
    } else {			/* Simple array variable. */
	if (localIndex >= 0) {
	    if (haveImmValue) {
		TclEmitInstInt1(INST_INCR_ARRAY1_IMM, localIndex, envPtr);
		TclEmitInt1(immValue, envPtr);
	    } else {
		TclEmitInstInt1(INST_INCR_ARRAY1, localIndex,	envPtr);
	    }
	} else {
	    if (haveImmValue) {
		TclEmitInstInt1(INST_INCR_ARRAY_STK_IMM, immValue, envPtr);
	    } else {
		TclEmitOpcode(	INST_INCR_ARRAY_STK,		envPtr);
	    }
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileInfo*Cmd --
 *
 *	Procedures called to compile "info" subcommands.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "info" subcommand at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileInfoCommandsCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;
    Tcl_Obj *objPtr;
    char *bytes;

    /*
     * We require one compile-time known argument for the case we can compile.
     */

    if (parsePtr->numWords == 1) {
	return TclCompileBasic0ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    } else if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    objPtr = Tcl_NewObj();
    Tcl_IncrRefCount(objPtr);
    if (!TclWordKnownAtCompileTime(tokenPtr, objPtr)) {
	goto notCompilable;
    }
    bytes = Tcl_GetString(objPtr);

    /*
     * We require that the argument start with "::" and not have any of "*\[?"
     * in it. (Theoretically, we should look in only the final component, but
     * the difference is so slight given current naming practices.)
     */

    if (bytes[0] != ':' || bytes[1] != ':' || !TclMatchIsTrivial(bytes)) {
	goto notCompilable;
    }
    Tcl_DecrRefCount(objPtr);

    /*
     * Confirmed as a literal that will not frighten the horses. Compile. Note
     * that the result needs to be list-ified.
     */

    /* TODO: Just push the known value */
    CompileWord(envPtr, tokenPtr,		interp, 1);
    TclEmitOpcode(	INST_RESOLVE_COMMAND,	envPtr);
    TclEmitOpcode(	INST_DUP,		envPtr);
    TclEmitOpcode(	INST_STR_LEN,		envPtr);
    TclEmitInstInt1(	INST_JUMP_FALSE1, 7,	envPtr);
    TclEmitInstInt4(	INST_LIST, 1,		envPtr);
    return TCL_OK;

  notCompilable:
    Tcl_DecrRefCount(objPtr);
    return TclCompileBasic1ArgCmd(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileInfoCoroutineCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Only compile [info coroutine] without arguments.
     */

    if (parsePtr->numWords != 1) {
	return TCL_ERROR;
    }

    /*
     * Not much to do; we compile to a single instruction...
     */

    TclEmitOpcode(		INST_COROUTINE_NAME,		envPtr);
    return TCL_OK;
}

int
TclCompileInfoExistsCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;
    int isScalar, localIndex;
    DefineLineInformation;	/* TIP #280 */

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }

    /*
     * Decide if we can use a frame slot for the var/array name or if we need
     * to emit code to compute and push the name at runtime. We use a frame
     * slot (entry in the array of local vars) if we are compiling a procedure
     * body and if the name is simple text that does not include namespace
     * qualifiers.
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    PushVarNameWord(interp, tokenPtr, envPtr, 0, &localIndex, &isScalar, 1);

    /*
     * Emit instruction to check the variable for existence.
     */

    if (isScalar) {
	if (localIndex < 0) {
	    TclEmitOpcode(	INST_EXIST_STK,			envPtr);
	} else {
	    TclEmitInstInt4(	INST_EXIST_SCALAR, localIndex,	envPtr);
	}
    } else {
	if (localIndex < 0) {
	    TclEmitOpcode(	INST_EXIST_ARRAY_STK,		envPtr);
	} else {
	    TclEmitInstInt4(	INST_EXIST_ARRAY, localIndex,	envPtr);
	}
    }

    return TCL_OK;
}

int
TclCompileInfoLevelCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Only compile [info level] without arguments or with a single argument.
     */

    if (parsePtr->numWords == 1) {
	/*
	 * Not much to do; we compile to a single instruction...
	 */

	TclEmitOpcode(		INST_INFO_LEVEL_NUM,		envPtr);
    } else if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    } else {
	DefineLineInformation;	/* TIP #280 */

	/*
	 * Compile the argument, then add the instruction to convert it into a
	 * list of arguments.
	 */

	CompileWord(envPtr, TokenAfter(parsePtr->tokenPtr), interp, 1);
	TclEmitOpcode(		INST_INFO_LEVEL_ARGS,		envPtr);
    }
    return TCL_OK;
}

int
TclCompileInfoObjectClassCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr = TokenAfter(parsePtr->tokenPtr);

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }
    CompileWord(envPtr,		tokenPtr,		interp, 1);
    TclEmitOpcode(		INST_TCLOO_CLASS,	envPtr);
    return TCL_OK;
}

int
TclCompileInfoObjectIsACmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr = TokenAfter(parsePtr->tokenPtr);

    /*
     * We only handle [info object isa object <somevalue>]. The first three
     * words are compressed to a single token by the ensemble compilation
     * engine.
     */

    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }
    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD || tokenPtr[1].size < 1
	    || strncmp(tokenPtr[1].start, "object", tokenPtr[1].size)) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(tokenPtr);

    /*
     * Issue the code.
     */

    CompileWord(envPtr,		tokenPtr,		interp, 2);
    TclEmitOpcode(		INST_TCLOO_IS_OBJECT,	envPtr);
    return TCL_OK;
}

int
TclCompileInfoObjectNamespaceCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr = TokenAfter(parsePtr->tokenPtr);

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }
    CompileWord(envPtr,		tokenPtr,		interp, 1);
    TclEmitOpcode(		INST_TCLOO_NS,		envPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileLappendCmd --
 *
 *	Procedure called to compile the "lappend" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "lappend" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileLappendCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *varTokenPtr, *valueTokenPtr;
    int isScalar, localIndex, numWords, i;
    DefineLineInformation;	/* TIP #280 */

    /* TODO: Consider support for compiling expanded args. */
    numWords = parsePtr->numWords;
    if (numWords < 3) {
	return TCL_ERROR;
    }

    if (numWords != 3 || envPtr->procPtr == NULL) {
	goto lappendMultiple;
    }

    /*
     * Decide if we can use a frame slot for the var/array name or if we
     * need to emit code to compute and push the name at runtime. We use a
     * frame slot (entry in the array of local vars) if we are compiling a
     * procedure body and if the name is simple text that does not include
     * namespace qualifiers.
     */

    varTokenPtr = TokenAfter(parsePtr->tokenPtr);

    PushVarNameWord(interp, varTokenPtr, envPtr, 0,
	    &localIndex, &isScalar, 1);

    /*
     * If we are doing an assignment, push the new value. In the no values
     * case, create an empty object.
     */

    if (numWords > 2) {
	Tcl_Token *valueTokenPtr = TokenAfter(varTokenPtr);

	CompileWord(envPtr, valueTokenPtr, interp, 2);
    }

    /*
     * Emit instructions to set/get the variable.
     */

    /*
     * The *_STK opcodes should be refactored to make better use of existing
     * LOAD/STORE instructions.
     */

    if (isScalar) {
	if (localIndex < 0) {
	    TclEmitOpcode(	INST_LAPPEND_STK,		envPtr);
	} else {
	    Emit14Inst(		INST_LAPPEND_SCALAR, localIndex, envPtr);
	}
    } else {
	if (localIndex < 0) {
	    TclEmitOpcode(	INST_LAPPEND_ARRAY_STK,		envPtr);
	} else {
	    Emit14Inst(		INST_LAPPEND_ARRAY, localIndex,	envPtr);
	}
    }

    return TCL_OK;

  lappendMultiple:
    varTokenPtr = TokenAfter(parsePtr->tokenPtr);
    PushVarNameWord(interp, varTokenPtr, envPtr, 0,
	    &localIndex, &isScalar, 1);
    valueTokenPtr = TokenAfter(varTokenPtr);
    for (i = 2 ; i < numWords ; i++) {
	CompileWord(envPtr, valueTokenPtr, interp, i);
	valueTokenPtr = TokenAfter(valueTokenPtr);
    }
    TclEmitInstInt4(	    INST_LIST, numWords-2,		envPtr);
    if (isScalar) {
	if (localIndex < 0) {
	    TclEmitOpcode(  INST_LAPPEND_LIST_STK,		envPtr);
	} else {
	    TclEmitInstInt4(INST_LAPPEND_LIST, localIndex,	envPtr);
	}
    } else {
	if (localIndex < 0) {
	    TclEmitOpcode(  INST_LAPPEND_LIST_ARRAY_STK,	envPtr);
	} else {
	    TclEmitInstInt4(INST_LAPPEND_LIST_ARRAY, localIndex,envPtr);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileLassignCmd --
 *
 *	Procedure called to compile the "lassign" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "lassign" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileLassignCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;
    int isScalar, localIndex, numWords, idx;
    DefineLineInformation;	/* TIP #280 */

    numWords = parsePtr->numWords;

    /*
     * Check for command syntax error, but we'll punt that to runtime.
     */

    if (numWords < 3) {
	return TCL_ERROR;
    }

    /*
     * Generate code to push list being taken apart by [lassign].
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 1);

    /*
     * Generate code to assign values from the list to variables.
     */

    for (idx=0 ; idx<numWords-2 ; idx++) {
	tokenPtr = TokenAfter(tokenPtr);

	/*
	 * Generate the next variable name.
	 */

	PushVarNameWord(interp, tokenPtr, envPtr, 0, &localIndex,
		&isScalar, idx+2);

	/*
	 * Emit instructions to get the idx'th item out of the list value on
	 * the stack and assign it to the variable.
	 */

	if (isScalar) {
	    if (localIndex >= 0) {
		TclEmitOpcode(	INST_DUP,			envPtr);
		TclEmitInstInt4(INST_LIST_INDEX_IMM, idx,	envPtr);
		Emit14Inst(	INST_STORE_SCALAR, localIndex,	envPtr);
		TclEmitOpcode(	INST_POP,			envPtr);
	    } else {
		TclEmitInstInt4(INST_OVER, 1,			envPtr);
		TclEmitInstInt4(INST_LIST_INDEX_IMM, idx,	envPtr);
		TclEmitOpcode(	INST_STORE_STK,			envPtr);
		TclEmitOpcode(	INST_POP,			envPtr);
	    }
	} else {
	    if (localIndex >= 0) {
		TclEmitInstInt4(INST_OVER, 1,			envPtr);
		TclEmitInstInt4(INST_LIST_INDEX_IMM, idx,	envPtr);
		Emit14Inst(	INST_STORE_ARRAY, localIndex,	envPtr);
		TclEmitOpcode(	INST_POP,			envPtr);
	    } else {
		TclEmitInstInt4(INST_OVER, 2,			envPtr);
		TclEmitInstInt4(INST_LIST_INDEX_IMM, idx,	envPtr);
		TclEmitOpcode(	INST_STORE_ARRAY_STK,		envPtr);
		TclEmitOpcode(	INST_POP,			envPtr);
	    }
	}
    }

    /*
     * Generate code to leave the rest of the list on the stack.
     */

    TclEmitInstInt4(		INST_LIST_RANGE_IMM, idx,	envPtr);
    TclEmitInt4(			TCL_INDEX_END,		envPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileLindexCmd --
 *
 *	Procedure called to compile the "lindex" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "lindex" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileLindexCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *idxTokenPtr, *valTokenPtr;
    int i, idx, numWords = parsePtr->numWords;
    DefineLineInformation;	/* TIP #280 */

    /*
     * Quit if too few args.
     */

    /* TODO: Consider support for compiling expanded args. */
    if (numWords <= 1) {
	return TCL_ERROR;
    }

    valTokenPtr = TokenAfter(parsePtr->tokenPtr);
    if (numWords != 3) {
	goto emitComplexLindex;
    }

    idxTokenPtr = TokenAfter(valTokenPtr);
    if (TclGetIndexFromToken(idxTokenPtr, TCL_INDEX_BEFORE, TCL_INDEX_BEFORE,
	    &idx) == TCL_OK) {
	/*
	 * The idxTokenPtr parsed as a valid index value and was
	 * encoded as expected by INST_LIST_INDEX_IMM.
	 *
	 * NOTE: that we rely on indexing before a list producing the
	 * same result as indexing after a list.
	 */

	CompileWord(envPtr, valTokenPtr, interp, 1);
	TclEmitInstInt4(	INST_LIST_INDEX_IMM, idx,	envPtr);
	return TCL_OK;
    }

    /*
     * If the value was not known at compile time, the conversion failed or
     * the value was negative, we just keep on going with the more complex
     * compilation.
     */

    /*
     * Push the operands onto the stack.
     */

  emitComplexLindex:
    for (i=1 ; i<numWords ; i++) {
	CompileWord(envPtr, valTokenPtr, interp, i);
	valTokenPtr = TokenAfter(valTokenPtr);
    }

    /*
     * Emit INST_LIST_INDEX if objc==3, or INST_LIST_INDEX_MULTI if there are
     * multiple index args.
     */

    if (numWords == 3) {
	TclEmitOpcode(		INST_LIST_INDEX,		envPtr);
    } else {
	TclEmitInstInt4(	INST_LIST_INDEX_MULTI, numWords-1, envPtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileListCmd --
 *
 *	Procedure called to compile the "list" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "list" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileListCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *valueTokenPtr;
    int i, numWords, concat, build;
    Tcl_Obj *listObj, *objPtr;

    if (parsePtr->numWords == 1) {
	/*
	 * [list] without arguments just pushes an empty object.
	 */

	PushStringLiteral(envPtr, "");
	return TCL_OK;
    }

    /*
     * Test if all arguments are compile-time known. If they are, we can
     * implement with a simple push.
     */

    numWords = parsePtr->numWords;
    valueTokenPtr = TokenAfter(parsePtr->tokenPtr);
    listObj = Tcl_NewObj();
    for (i = 1; i < numWords && listObj != NULL; i++) {
	objPtr = Tcl_NewObj();
	if (TclWordKnownAtCompileTime(valueTokenPtr, objPtr)) {
	    (void) Tcl_ListObjAppendElement(NULL, listObj, objPtr);
	} else {
	    Tcl_DecrRefCount(objPtr);
	    Tcl_DecrRefCount(listObj);
	    listObj = NULL;
	}
	valueTokenPtr = TokenAfter(valueTokenPtr);
    }
    if (listObj != NULL) {
	TclEmitPush(TclAddLiteralObj(envPtr, listObj, NULL), envPtr);
	return TCL_OK;
    }

    /*
     * Push the all values onto the stack.
     */

    numWords = parsePtr->numWords;
    valueTokenPtr = TokenAfter(parsePtr->tokenPtr);
    concat = build = 0;
    for (i = 1; i < numWords; i++) {
	if (valueTokenPtr->type == TCL_TOKEN_EXPAND_WORD && build > 0) {
	    TclEmitInstInt4(	INST_LIST, build,	envPtr);
	    if (concat) {
		TclEmitOpcode(	INST_LIST_CONCAT,	envPtr);
	    }
	    build = 0;
	    concat = 1;
	}
	CompileWord(envPtr, valueTokenPtr, interp, i);
	if (valueTokenPtr->type == TCL_TOKEN_EXPAND_WORD) {
	    if (concat) {
		TclEmitOpcode(	INST_LIST_CONCAT,	envPtr);
	    } else {
		concat = 1;
	    }
	} else {
	    build++;
	}
	valueTokenPtr = TokenAfter(valueTokenPtr);
    }
    if (build > 0) {
	TclEmitInstInt4(	INST_LIST, build,	envPtr);
	if (concat) {
	    TclEmitOpcode(	INST_LIST_CONCAT,	envPtr);
	}
    }

    /*
     * If there was just one expanded word, we must ensure that it is a list
     * at this point. We use an [lrange ... 0 end] for this (instead of
     * [llength], as with literals) as we must drop any string representation
     * that might be hanging around.
     */

    if (concat && numWords == 2) {
	TclEmitInstInt4(	INST_LIST_RANGE_IMM, 0,	envPtr);
	TclEmitInt4(			TCL_INDEX_END,	envPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileLlengthCmd --
 *
 *	Procedure called to compile the "llength" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "llength" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileLlengthCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *varTokenPtr;
    DefineLineInformation;	/* TIP #280 */

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }
    varTokenPtr = TokenAfter(parsePtr->tokenPtr);

    CompileWord(envPtr, varTokenPtr, interp, 1);
    TclEmitOpcode(		INST_LIST_LENGTH,		envPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileLrangeCmd --
 *
 *	How to compile the "lrange" command. We only bother because we needed
 *	the opcode anyway for "lassign".
 *
 *----------------------------------------------------------------------
 */

int
TclCompileLrangeCmd(
    Tcl_Interp *interp,		/* Tcl interpreter for context. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the
				 * command. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds the resulting instructions. */
{
    Tcl_Token *tokenPtr, *listTokenPtr;
    DefineLineInformation;	/* TIP #280 */
    int idx1, idx2;

    if (parsePtr->numWords != 4) {
	return TCL_ERROR;
    }
    listTokenPtr = TokenAfter(parsePtr->tokenPtr);

    tokenPtr = TokenAfter(listTokenPtr);
    if (TclGetIndexFromToken(tokenPtr, TCL_INDEX_START, TCL_INDEX_AFTER,
	    &idx1) != TCL_OK) {
	return TCL_ERROR;
    }
    /*
     * Token was an index value, and we treat all "first" indices
     * before the list same as the start of the list.
     */

    tokenPtr = TokenAfter(tokenPtr);
    if (TclGetIndexFromToken(tokenPtr, TCL_INDEX_BEFORE, TCL_INDEX_END,
	    &idx2) != TCL_OK) {
	return TCL_ERROR;
    }
    /*
     * Token was an index value, and we treat all "last" indices
     * after the list same as the end of the list.
     */

    /*
     * Issue instructions. It's not safe to skip doing the LIST_RANGE, as
     * we've not proved that the 'list' argument is really a list. Not that it
     * is worth trying to do that given current knowledge.
     */

    CompileWord(envPtr, listTokenPtr, interp, 1);
    TclEmitInstInt4(		INST_LIST_RANGE_IMM, idx1,	envPtr);
    TclEmitInt4(		idx2,				envPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileLinsertCmd --
 *
 *	How to compile the "linsert" command. We only bother with the case
 *	where the index is constant.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileLinsertCmd(
    Tcl_Interp *interp,		/* Tcl interpreter for context. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the
				 * command. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds the resulting instructions. */
{
    Tcl_Token *tokenPtr, *listTokenPtr;
    DefineLineInformation;	/* TIP #280 */
    int idx, i;

    if (parsePtr->numWords < 3) {
	return TCL_ERROR;
    }
    listTokenPtr = TokenAfter(parsePtr->tokenPtr);

    /*
     * Parse the index. Will only compile if it is constant and not an
     * _integer_ less than zero (since we reserve negative indices here for
     * end-relative indexing) or an end-based index greater than 'end' itself.
     */

    tokenPtr = TokenAfter(listTokenPtr);

    /*
     * NOTE: This command treats all inserts at indices before the list
     * the same as inserts at the start of the list, and all inserts
     * after the list the same as inserts at the end of the list. We
     * make that transformation here so we can use the optimized bytecode
     * as much as possible.
     */
    if (TclGetIndexFromToken(tokenPtr, TCL_INDEX_START, TCL_INDEX_END,
	    &idx) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * There are four main cases. If there are no values to insert, this is
     * just a confirm-listiness check. If the index is '0', this is a prepend.
     * If the index is 'end' (== TCL_INDEX_END), this is an append. Otherwise,
     * this is a splice (== split, insert values as list, concat-3).
     */

    CompileWord(envPtr, listTokenPtr, interp, 1);
    if (parsePtr->numWords == 3) {
	TclEmitInstInt4(	INST_LIST_RANGE_IMM, 0,		envPtr);
	TclEmitInt4(			TCL_INDEX_END,		envPtr);
	return TCL_OK;
    }

    for (i=3 ; i<parsePtr->numWords ; i++) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, i);
    }
    TclEmitInstInt4(		INST_LIST, i-3,			envPtr);

    if (idx == TCL_INDEX_START) {
	TclEmitInstInt4(	INST_REVERSE, 2,		envPtr);
	TclEmitOpcode(		INST_LIST_CONCAT,		envPtr);
    } else if (idx == TCL_INDEX_END) {
	TclEmitOpcode(		INST_LIST_CONCAT,		envPtr);
    } else {
	/*
	 * Here we handle two ranges for idx. First when idx > 0, we
	 * want the first half of the split to end at index idx-1 and
	 * the second half to start at index idx.
	 * Second when idx < TCL_INDEX_END, indicating "end-N" indexing,
	 * we want the first half of the split to end at index end-N and
	 * the second half to start at index end-N+1. We accomplish this
	 * with a pre-adjustment of the end-N value.
	 * The root of this is that the commands [lrange] and [linsert]
	 * differ in their interpretation of the "end" index.
	 */

	if (idx < TCL_INDEX_END) {
	    idx++;
	}
	TclEmitInstInt4(	INST_OVER, 1,			envPtr);
	TclEmitInstInt4(	INST_LIST_RANGE_IMM, 0,		envPtr);
	TclEmitInt4(			idx-1,			envPtr);
	TclEmitInstInt4(	INST_REVERSE, 3,		envPtr);
	TclEmitInstInt4(	INST_LIST_RANGE_IMM, idx,	envPtr);
	TclEmitInt4(			TCL_INDEX_END,		envPtr);
	TclEmitOpcode(		INST_LIST_CONCAT,		envPtr);
	TclEmitOpcode(		INST_LIST_CONCAT,		envPtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileLreplaceCmd --
 *
 *	How to compile the "lreplace" command. We only bother with the case
 *	where the indices are constant.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileLreplaceCmd(
    Tcl_Interp *interp,		/* Tcl interpreter for context. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the
				 * command. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds the resulting instructions. */
{
    Tcl_Token *tokenPtr, *listTokenPtr;
    DefineLineInformation;	/* TIP #280 */
    int idx1, idx2, i;
    int emptyPrefix=1, suffixStart = 0;

    if (parsePtr->numWords < 4) {
	return TCL_ERROR;
    }
    listTokenPtr = TokenAfter(parsePtr->tokenPtr);

    tokenPtr = TokenAfter(listTokenPtr);
    if (TclGetIndexFromToken(tokenPtr, TCL_INDEX_START, TCL_INDEX_AFTER,
	    &idx1) != TCL_OK) {
	return TCL_ERROR;
    }

    tokenPtr = TokenAfter(tokenPtr);
    if (TclGetIndexFromToken(tokenPtr, TCL_INDEX_BEFORE, TCL_INDEX_END,
	    &idx2) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * General structure of the [lreplace] result is
     *		prefix replacement suffix
     * In a few cases we can predict various parts will be empty and
     * take advantage.
     *
     * The proper suffix begins with the greater of indices idx1 or
     * idx2 + 1. If we cannot tell at compile time which is greater,
     * we must defer to direct evaluation.
     */

    if (idx1 == TCL_INDEX_AFTER) {
	suffixStart = idx1;
    } else if (idx2 == TCL_INDEX_BEFORE) {
	suffixStart = idx1;
    } else if (idx2 == TCL_INDEX_END) {
	suffixStart = TCL_INDEX_AFTER;
    } else if (((idx2 < TCL_INDEX_END) && (idx1 <= TCL_INDEX_END))
	    || ((idx2 >= TCL_INDEX_START) && (idx1 >= TCL_INDEX_START))) {
	suffixStart = (idx1 > idx2 + 1) ? idx1 : idx2 + 1;
    } else {
	return TCL_ERROR;
    }

    /* All paths start with computing/pushing the original value. */
    CompileWord(envPtr, listTokenPtr, interp, 1);

    /*
     * Push all the replacement values next so any errors raised in
     * creating them get raised first.
     */
    if (parsePtr->numWords > 4) {
	/* Push the replacement arguments */
	tokenPtr = TokenAfter(tokenPtr);
	for (i=4 ; i<parsePtr->numWords ; i++) {
	    CompileWord(envPtr, tokenPtr, interp, i);
	    tokenPtr = TokenAfter(tokenPtr);
	}

	/* Make a list of them... */
	TclEmitInstInt4(	INST_LIST, i - 4,		envPtr);

	emptyPrefix = 0;
    }

    if ((idx1 == suffixStart) && (parsePtr->numWords == 4)) {
	/*
	 * This is a "no-op". Example: [lreplace {a b c} 2 0]
	 * We still do a list operation to get list-verification
	 * and canonicalization side effects.
	 */
	TclEmitInstInt4(	INST_LIST_RANGE_IMM, 0,		envPtr);
	TclEmitInt4(			TCL_INDEX_END,		envPtr);
	return TCL_OK;
    }

    if (idx1 != TCL_INDEX_START) {
	/* Prefix may not be empty; generate bytecode to push it */
	if (emptyPrefix) {
	    TclEmitOpcode(	INST_DUP,			envPtr);
	} else {
	    TclEmitInstInt4(	INST_OVER, 1,			envPtr);
	}
	TclEmitInstInt4(	INST_LIST_RANGE_IMM, 0,		envPtr);
	TclEmitInt4(			idx1 - 1,		envPtr);
	if (!emptyPrefix) {
	    TclEmitInstInt4(	INST_REVERSE, 2,		envPtr);
	    TclEmitOpcode(	INST_LIST_CONCAT,		envPtr);
	}
	emptyPrefix = 0;
    }

    if (!emptyPrefix) {
	TclEmitInstInt4(	INST_REVERSE, 2,		envPtr);
    }

    if (suffixStart == TCL_INDEX_AFTER) {
	TclEmitOpcode(		INST_POP,			envPtr);
	if (emptyPrefix) {
	    PushStringLiteral(envPtr, "");
	}
    } else {
	/* Suffix may not be empty; generate bytecode to push it */
	TclEmitInstInt4(	INST_LIST_RANGE_IMM, suffixStart, envPtr);
	TclEmitInt4(			TCL_INDEX_END,		envPtr);
	if (!emptyPrefix) {
	    TclEmitOpcode(	INST_LIST_CONCAT,		envPtr);
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileLsetCmd --
 *
 *	Procedure called to compile the "lset" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "lset" command at
 *	runtime.
 *
 * The general template for execution of the "lset" command is:
 *	(1) Instructions to push the variable name, unless the variable is
 *	    local to the stack frame.
 *	(2) If the variable is an array element, instructions to push the
 *	    array element name.
 *	(3) Instructions to push each of zero or more "index" arguments to the
 *	    stack, followed with the "newValue" element.
 *	(4) Instructions to duplicate the variable name and/or array element
 *	    name onto the top of the stack, if either was pushed at steps (1)
 *	    and (2).
 *	(5) The appropriate INST_LOAD_* instruction to place the original
 *	    value of the list variable at top of stack.
 *	(6) At this point, the stack contains:
 *		varName? arrayElementName? index1 index2 ... newValue oldList
 *	    The compiler emits one of INST_LSET_FLAT or INST_LSET_LIST
 *	    according as whether there is exactly one index element (LIST) or
 *	    either zero or else two or more (FLAT). This instruction removes
 *	    everything from the stack except for the two names and pushes the
 *	    new value of the variable.
 *	(7) Finally, INST_STORE_* stores the new value in the variable and
 *	    cleans up the stack.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileLsetCmd(
    Tcl_Interp *interp,		/* Tcl interpreter for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the
				 * command. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds the resulting instructions. */
{
    int tempDepth;		/* Depth used for emitting one part of the
				 * code burst. */
    Tcl_Token *varTokenPtr;	/* Pointer to the Tcl_Token representing the
				 * parse of the variable name. */
    int localIndex;		/* Index of var in local var table. */
    int isScalar;		/* Flag == 1 if scalar, 0 if array. */
    int i;
    DefineLineInformation;	/* TIP #280 */

    /*
     * Check argument count.
     */

    /* TODO: Consider support for compiling expanded args. */
    if (parsePtr->numWords < 3) {
	/*
	 * Fail at run time, not in compilation.
	 */

	return TCL_ERROR;
    }

    /*
     * Decide if we can use a frame slot for the var/array name or if we need
     * to emit code to compute and push the name at runtime. We use a frame
     * slot (entry in the array of local vars) if we are compiling a procedure
     * body and if the name is simple text that does not include namespace
     * qualifiers.
     */

    varTokenPtr = TokenAfter(parsePtr->tokenPtr);
    PushVarNameWord(interp, varTokenPtr, envPtr, 0,
	    &localIndex, &isScalar, 1);

    /*
     * Push the "index" args and the new element value.
     */

    for (i=2 ; i<parsePtr->numWords ; ++i) {
	varTokenPtr = TokenAfter(varTokenPtr);
	CompileWord(envPtr, varTokenPtr, interp, i);
    }

    /*
     * Duplicate the variable name if it's been pushed.
     */

    if (localIndex < 0) {
	if (isScalar) {
	    tempDepth = parsePtr->numWords - 2;
	} else {
	    tempDepth = parsePtr->numWords - 1;
	}
	TclEmitInstInt4(	INST_OVER, tempDepth,		envPtr);
    }

    /*
     * Duplicate an array index if one's been pushed.
     */

    if (!isScalar) {
	if (localIndex < 0) {
	    tempDepth = parsePtr->numWords - 1;
	} else {
	    tempDepth = parsePtr->numWords - 2;
	}
	TclEmitInstInt4(	INST_OVER, tempDepth,		envPtr);
    }

    /*
     * Emit code to load the variable's value.
     */

    if (isScalar) {
	if (localIndex < 0) {
	    TclEmitOpcode(	INST_LOAD_STK,			envPtr);
	} else {
	    Emit14Inst(		INST_LOAD_SCALAR, localIndex,	envPtr);
	}
    } else {
	if (localIndex < 0) {
	    TclEmitOpcode(	INST_LOAD_ARRAY_STK,		envPtr);
	} else {
	    Emit14Inst(		INST_LOAD_ARRAY, localIndex,	envPtr);
	}
    }

    /*
     * Emit the correct variety of 'lset' instruction.
     */

    if (parsePtr->numWords == 4) {
	TclEmitOpcode(		INST_LSET_LIST,			envPtr);
    } else {
	TclEmitInstInt4(	INST_LSET_FLAT, parsePtr->numWords-1, envPtr);
    }

    /*
     * Emit code to put the value back in the variable.
     */

    if (isScalar) {
	if (localIndex < 0) {
	    TclEmitOpcode(	INST_STORE_STK,			envPtr);
	} else {
	    Emit14Inst(		INST_STORE_SCALAR, localIndex,	envPtr);
	}
    } else {
	if (localIndex < 0) {
	    TclEmitOpcode(	INST_STORE_ARRAY_STK,		envPtr);
	} else {
	    Emit14Inst(		INST_STORE_ARRAY, localIndex,	envPtr);
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileNamespace*Cmd --
 *
 *	Procedures called to compile the "namespace" command; currently, only
 *	the subcommands "namespace current" and "namespace upvar" are compiled
 *	to bytecodes, and the latter only inside a procedure(-like) context.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "namespace upvar"
 *	command at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileNamespaceCurrentCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * Only compile [namespace current] without arguments.
     */

    if (parsePtr->numWords != 1) {
	return TCL_ERROR;
    }

    /*
     * Not much to do; we compile to a single instruction...
     */

    TclEmitOpcode(		INST_NS_CURRENT,		envPtr);
    return TCL_OK;
}

int
TclCompileNamespaceCodeCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;
    DefineLineInformation;	/* TIP #280 */

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(parsePtr->tokenPtr);

    /*
     * The specification of [namespace code] is rather shocking, in that it is
     * supposed to check if the argument is itself the result of [namespace
     * code] and not apply itself in that case. Which is excessively cautious,
     * but what the test suite checks for.
     */

    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD || (tokenPtr[1].size > 20
	    && strncmp(tokenPtr[1].start, "::namespace inscope ", 20) == 0)) {
	/*
	 * Technically, we could just pass a literal '::namespace inscope '
	 * term through, but that's something which really shouldn't be
	 * occurring as something that the user writes so we'll just punt it.
	 */

	return TCL_ERROR;
    }

    /*
     * Now we can compile using the same strategy as [namespace code]'s normal
     * implementation does internally. Note that we can't bind the namespace
     * name directly here, because TclOO plays complex games with namespaces;
     * the value needs to be determined at runtime for safety.
     */

    PushStringLiteral(envPtr,		"::namespace");
    PushStringLiteral(envPtr,		"inscope");
    TclEmitOpcode(		INST_NS_CURRENT,	envPtr);
    CompileWord(envPtr,		tokenPtr,		interp, 1);
    TclEmitInstInt4(		INST_LIST, 4,		envPtr);
    return TCL_OK;
}

int
TclCompileNamespaceOriginCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;
    DefineLineInformation;	/* TIP #280 */

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(parsePtr->tokenPtr);

    CompileWord(envPtr,	tokenPtr,			interp, 1);
    TclEmitOpcode(	INST_ORIGIN_COMMAND,		envPtr);
    return TCL_OK;
}

int
TclCompileNamespaceQualifiersCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr = TokenAfter(parsePtr->tokenPtr);
    DefineLineInformation;	/* TIP #280 */
    int off;

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }

    CompileWord(envPtr, tokenPtr, interp, 1);
    PushStringLiteral(envPtr, "0");
    PushStringLiteral(envPtr, "::");
    TclEmitInstInt4(	INST_OVER, 2,			envPtr);
    TclEmitOpcode(	INST_STR_FIND_LAST,		envPtr);
    off = CurrentOffset(envPtr);
    PushStringLiteral(envPtr, "1");
    TclEmitOpcode(	INST_SUB,			envPtr);
    TclEmitInstInt4(	INST_OVER, 2,			envPtr);
    TclEmitInstInt4(	INST_OVER, 1,			envPtr);
    TclEmitOpcode(	INST_STR_INDEX,			envPtr);
    PushStringLiteral(envPtr, ":");
    TclEmitOpcode(	INST_STR_EQ,			envPtr);
    off = off - CurrentOffset(envPtr);
    TclEmitInstInt1(	INST_JUMP_TRUE1, off,		envPtr);
    TclEmitOpcode(	INST_STR_RANGE,			envPtr);
    return TCL_OK;
}

int
TclCompileNamespaceTailCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr = TokenAfter(parsePtr->tokenPtr);
    DefineLineInformation;	/* TIP #280 */
    JumpFixup jumpFixup;

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }

    /*
     * Take care; only add 2 to found index if the string was actually found.
     */

    CompileWord(envPtr, tokenPtr, interp, 1);
    PushStringLiteral(envPtr, "::");
    TclEmitInstInt4(	INST_OVER, 1,			envPtr);
    TclEmitOpcode(	INST_STR_FIND_LAST,		envPtr);
    TclEmitOpcode(	INST_DUP,			envPtr);
    PushStringLiteral(envPtr, "0");
    TclEmitOpcode(	INST_GE,			envPtr);
    TclEmitForwardJump(envPtr, TCL_FALSE_JUMP, &jumpFixup);
    PushStringLiteral(envPtr, "2");
    TclEmitOpcode(	INST_ADD,			envPtr);
    TclFixupForwardJumpToHere(envPtr, &jumpFixup, 127);
    PushStringLiteral(envPtr, "end");
    TclEmitOpcode(	INST_STR_RANGE,			envPtr);
    return TCL_OK;
}

int
TclCompileNamespaceUpvarCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr, *otherTokenPtr, *localTokenPtr;
    int localIndex, numWords, i;
    DefineLineInformation;	/* TIP #280 */

    if (envPtr->procPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Only compile [namespace upvar ...]: needs an even number of args, >=4
     */

    numWords = parsePtr->numWords;
    if ((numWords % 2) || (numWords < 4)) {
	return TCL_ERROR;
    }

    /*
     * Push the namespace
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 1);

    /*
     * Loop over the (otherVar, thisVar) pairs. If any of the thisVar is not a
     * local variable, return an error so that the non-compiled command will
     * be called at runtime.
     */

    localTokenPtr = tokenPtr;
    for (i=2; i<numWords; i+=2) {
	otherTokenPtr = TokenAfter(localTokenPtr);
	localTokenPtr = TokenAfter(otherTokenPtr);

	CompileWord(envPtr, otherTokenPtr, interp, i);
	localIndex = LocalScalarFromToken(localTokenPtr, envPtr);
	if (localIndex < 0) {
	    return TCL_ERROR;
	}
	TclEmitInstInt4(	INST_NSUPVAR, localIndex,	envPtr);
    }

    /*
     * Pop the namespace, and set the result to empty
     */

    TclEmitOpcode(		INST_POP,			envPtr);
    PushStringLiteral(envPtr, "");
    return TCL_OK;
}

int
TclCompileNamespaceWhichCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr, *opt;
    int idx;

    if (parsePtr->numWords < 2 || parsePtr->numWords > 3) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    idx = 1;

    /*
     * If there's an option, check that it's "-command". We don't handle
     * "-variable" (currently) and anything else is an error.
     */

    if (parsePtr->numWords == 3) {
	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    return TCL_ERROR;
	}
	opt = tokenPtr + 1;
	if (opt->size < 2 || opt->size > 8
		|| strncmp(opt->start, "-command", opt->size) != 0) {
	    return TCL_ERROR;
	}
	tokenPtr = TokenAfter(tokenPtr);
	idx++;
    }

    /*
     * Issue the bytecode.
     */

    CompileWord(envPtr,		tokenPtr,		interp, idx);
    TclEmitOpcode(		INST_RESOLVE_COMMAND,	envPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileRegexpCmd --
 *
 *	Procedure called to compile the "regexp" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "regexp" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileRegexpCmd(
    Tcl_Interp *interp,		/* Tcl interpreter for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the
				 * command. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds the resulting instructions. */
{
    Tcl_Token *varTokenPtr;	/* Pointer to the Tcl_Token representing the
				 * parse of the RE or string. */
    int i, len, nocase, exact, sawLast, simple;
    const char *str;
    DefineLineInformation;	/* TIP #280 */

    /*
     * We are only interested in compiling simple regexp cases. Currently
     * supported compile cases are:
     *   regexp ?-nocase? ?--? staticString $var
     *   regexp ?-nocase? ?--? {^staticString$} $var
     */

    if (parsePtr->numWords < 3) {
	return TCL_ERROR;
    }

    simple = 0;
    nocase = 0;
    sawLast = 0;
    varTokenPtr = parsePtr->tokenPtr;

    /*
     * We only look for -nocase and -- as options. Everything else gets pushed
     * to runtime execution. This is different than regexp's runtime option
     * handling, but satisfies our stricter needs.
     */

    for (i = 1; i < parsePtr->numWords - 2; i++) {
	varTokenPtr = TokenAfter(varTokenPtr);
	if (varTokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    /*
	     * Not a simple string, so punt to runtime.
	     */

	    return TCL_ERROR;
	}
	str = varTokenPtr[1].start;
	len = varTokenPtr[1].size;
	if ((len == 2) && (str[0] == '-') && (str[1] == '-')) {
	    sawLast++;
	    i++;
	    break;
	} else if ((len > 1) && (strncmp(str,"-nocase",(unsigned)len) == 0)) {
	    nocase = 1;
	} else {
	    /*
	     * Not an option we recognize.
	     */

	    return TCL_ERROR;
	}
    }

    if ((parsePtr->numWords - i) != 2) {
	/*
	 * We don't support capturing to variables.
	 */

	return TCL_ERROR;
    }

    /*
     * Get the regexp string. If it is not a simple string or can't be
     * converted to a glob pattern, push the word for the INST_REGEXP.
     * Keep changes here in sync with TclCompileSwitchCmd Switch_Regexp.
     */

    varTokenPtr = TokenAfter(varTokenPtr);

    if (varTokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	Tcl_DString ds;

	str = varTokenPtr[1].start;
	len = varTokenPtr[1].size;

	/*
	 * If it has a '-', it could be an incorrectly formed regexp command.
	 */

	if ((*str == '-') && !sawLast) {
	    return TCL_ERROR;
	}

	if (len == 0) {
	    /*
	     * The semantics of regexp are always match on re == "".
	     */

	    PushStringLiteral(envPtr, "1");
	    return TCL_OK;
	}

	/*
	 * Attempt to convert pattern to glob.  If successful, push the
	 * converted pattern as a literal.
	 */

	if (TclReToGlob(NULL, varTokenPtr[1].start, len, &ds, &exact, NULL)
		== TCL_OK) {
	    simple = 1;
	    PushLiteral(envPtr, Tcl_DStringValue(&ds),Tcl_DStringLength(&ds));
	    Tcl_DStringFree(&ds);
	}
    }

    if (!simple) {
	CompileWord(envPtr, varTokenPtr, interp, parsePtr->numWords-2);
    }

    /*
     * Push the string arg.
     */

    varTokenPtr = TokenAfter(varTokenPtr);
    CompileWord(envPtr, varTokenPtr, interp, parsePtr->numWords-1);

    if (simple) {
	if (exact && !nocase) {
	    TclEmitOpcode(	INST_STR_EQ,			envPtr);
	} else {
	    TclEmitInstInt1(	INST_STR_MATCH, nocase,		envPtr);
	}
    } else {
	/*
	 * Pass correct RE compile flags.  We use only Int1 (8-bit), but
	 * that handles all the flags we want to pass.
	 * Don't use TCL_REG_NOSUB as we may have backrefs.
	 */

	int cflags = TCL_REG_ADVANCED | (nocase ? TCL_REG_NOCASE : 0);

	TclEmitInstInt1(	INST_REGEXP, cflags,		envPtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileRegsubCmd --
 *
 *	Procedure called to compile the "regsub" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "regsub" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileRegsubCmd(
    Tcl_Interp *interp,		/* Tcl interpreter for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the
				 * command. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds the resulting instructions. */
{
    /*
     * We only compile the case with [regsub -all] where the pattern is both
     * known at compile time and simple (i.e., no RE metacharacters). That is,
     * the pattern must be translatable into a glob like "*foo*" with no other
     * glob metacharacters inside it; there must be some "foo" in there too.
     * The substitution string must also be known at compile time and free of
     * metacharacters ("\digit" and "&"). Finally, there must not be a
     * variable mentioned in the [regsub] to write the result back to (because
     * we can't get the count of substitutions that would be the result in
     * that case). The key is that these are the conditions under which a
     * [string map] could be used instead, in particular a [string map] of the
     * form we can compile to bytecode.
     *
     * In short, we look for:
     *
     *   regsub -all [--] simpleRE string simpleReplacement
     *
     * The only optional part is the "--", and no other options are handled.
     */

    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr, *stringTokenPtr;
    Tcl_Obj *patternObj = NULL, *replacementObj = NULL;
    Tcl_DString pattern;
    const char *bytes;
    int len, exact, quantified, result = TCL_ERROR;

    if (parsePtr->numWords < 5 || parsePtr->numWords > 6) {
	return TCL_ERROR;
    }

    /*
     * Parse the "-all", which must be the first argument (other options not
     * supported, non-"-all" substitution we can't compile).
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD || tokenPtr[1].size != 4
	    || strncmp(tokenPtr[1].start, "-all", 4)) {
	return TCL_ERROR;
    }

    /*
     * Get the pattern into patternObj, checking for "--" in the process.
     */

    Tcl_DStringInit(&pattern);
    tokenPtr = TokenAfter(tokenPtr);
    patternObj = Tcl_NewObj();
    if (!TclWordKnownAtCompileTime(tokenPtr, patternObj)) {
	goto done;
    }
    if (Tcl_GetString(patternObj)[0] == '-') {
	if (strcmp(Tcl_GetString(patternObj), "--") != 0
		|| parsePtr->numWords == 5) {
	    goto done;
	}
	tokenPtr = TokenAfter(tokenPtr);
	Tcl_DecrRefCount(patternObj);
	patternObj = Tcl_NewObj();
	if (!TclWordKnownAtCompileTime(tokenPtr, patternObj)) {
	    goto done;
	}
    } else if (parsePtr->numWords == 6) {
	goto done;
    }

    /*
     * Identify the code which produces the string to apply the substitution
     * to (stringTokenPtr), and the replacement string (into replacementObj).
     */

    stringTokenPtr = TokenAfter(tokenPtr);
    tokenPtr = TokenAfter(stringTokenPtr);
    replacementObj = Tcl_NewObj();
    if (!TclWordKnownAtCompileTime(tokenPtr, replacementObj)) {
	goto done;
    }

    /*
     * Next, higher-level checks. Is the RE a very simple glob? Is the
     * replacement "simple"?
     */

    bytes = Tcl_GetStringFromObj(patternObj, &len);
    if (TclReToGlob(NULL, bytes, len, &pattern, &exact, &quantified)
	    != TCL_OK || exact || quantified) {
	goto done;
    }
    bytes = Tcl_DStringValue(&pattern);
    if (*bytes++ != '*') {
	goto done;
    }
    while (1) {
	switch (*bytes) {
	case '*':
	    if (bytes[1] == '\0') {
		/*
		 * OK, we've proved there are no metacharacters except for the
		 * '*' at each end.
		 */

		len = Tcl_DStringLength(&pattern) - 2;
		if (len > 0) {
		    goto isSimpleGlob;
		}

		/*
		 * The pattern is "**"! I believe that should be impossible,
		 * but we definitely can't handle that at all.
		 */
	    }
	case '\0': case '?': case '[': case '\\':
	    goto done;
	}
	bytes++;
    }
  isSimpleGlob:
    for (bytes = Tcl_GetString(replacementObj); *bytes; bytes++) {
	switch (*bytes) {
	case '\\': case '&':
	    goto done;
	}
    }

    /*
     * Proved the simplicity constraints! Time to issue the code.
     */

    result = TCL_OK;
    bytes = Tcl_DStringValue(&pattern) + 1;
    PushLiteral(envPtr,	bytes, len);
    bytes = Tcl_GetStringFromObj(replacementObj, &len);
    PushLiteral(envPtr,	bytes, len);
    CompileWord(envPtr,	stringTokenPtr, interp, parsePtr->numWords-2);
    TclEmitOpcode(	INST_STR_MAP,	envPtr);

  done:
    Tcl_DStringFree(&pattern);
    if (patternObj) {
	Tcl_DecrRefCount(patternObj);
    }
    if (replacementObj) {
	Tcl_DecrRefCount(replacementObj);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileReturnCmd --
 *
 *	Procedure called to compile the "return" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "return" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileReturnCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * General syntax: [return ?-option value ...? ?result?]
     * An even number of words means an explicit result argument is present.
     */
    int level, code, objc, size, status = TCL_OK;
    int numWords = parsePtr->numWords;
    int explicitResult = (0 == (numWords % 2));
    int numOptionWords = numWords - 1 - explicitResult;
    Tcl_Obj *returnOpts, **objv;
    Tcl_Token *wordTokenPtr = TokenAfter(parsePtr->tokenPtr);
    DefineLineInformation;	/* TIP #280 */

    /*
     * Check for special case which can always be compiled:
     *	    return -options <opts> <msg>
     * Unlike the normal [return] compilation, this version does everything at
     * runtime so it can handle arbitrary words and not just literals. Note
     * that if INST_RETURN_STK wasn't already needed for something else
     * ('finally' clause processing) this piece of code would not be present.
     */

    if ((numWords == 4) && (wordTokenPtr->type == TCL_TOKEN_SIMPLE_WORD)
	    && (wordTokenPtr[1].size == 8)
	    && (strncmp(wordTokenPtr[1].start, "-options", 8) == 0)) {
	Tcl_Token *optsTokenPtr = TokenAfter(wordTokenPtr);
	Tcl_Token *msgTokenPtr = TokenAfter(optsTokenPtr);

	CompileWord(envPtr, optsTokenPtr, interp, 2);
	CompileWord(envPtr, msgTokenPtr,  interp, 3);
	TclEmitInvoke(envPtr, INST_RETURN_STK);
	return TCL_OK;
    }

    /*
     * Allocate some working space.
     */

    objv = TclStackAlloc(interp, numOptionWords * sizeof(Tcl_Obj *));

    /*
     * Scan through the return options. If any are unknown at compile time,
     * there is no value in bytecompiling. Save the option values known in an
     * objv array for merging into a return options dictionary.
     *
     * TODO: There is potential for improvement if all option keys are known
     * at compile time and all option values relating to '-code' and '-level'
     * are known at compile time.
     */

    for (objc = 0; objc < numOptionWords; objc++) {
	objv[objc] = Tcl_NewObj();
	Tcl_IncrRefCount(objv[objc]);
	if (!TclWordKnownAtCompileTime(wordTokenPtr, objv[objc])) {
	    /*
	     * Non-literal, so punt to run-time assembly of the dictionary.
	     */

	    for (; objc>=0 ; objc--) {
		TclDecrRefCount(objv[objc]);
	    }
	    TclStackFree(interp, objv);
	    goto issueRuntimeReturn;
	}
	wordTokenPtr = TokenAfter(wordTokenPtr);
    }
    status = TclMergeReturnOptions(interp, objc, objv,
	    &returnOpts, &code, &level);
    while (--objc >= 0) {
	TclDecrRefCount(objv[objc]);
    }
    TclStackFree(interp, objv);
    if (TCL_ERROR == status) {
	/*
	 * Something was bogus in the return options. Clear the error message,
	 * and report back to the compiler that this must be interpreted at
	 * runtime.
	 */

	Tcl_ResetResult(interp);
	return TCL_ERROR;
    }

    /*
     * All options are known at compile time, so we're going to bytecompile.
     * Emit instructions to push the result on the stack.
     */

    if (explicitResult) {
	 CompileWord(envPtr, wordTokenPtr, interp, numWords-1);
    } else {
	/*
	 * No explict result argument, so default result is empty string.
	 */

	PushStringLiteral(envPtr, "");
    }

    /*
     * Check for optimization: When [return] is in a proc, and there's no
     * enclosing [catch], and there are no return options, then the INST_DONE
     * instruction is equivalent, and may be more efficient.
     */

    if (numOptionWords == 0 && envPtr->procPtr != NULL) {
	/*
	 * We have default return options and we're in a proc ...
	 */

	int index = envPtr->exceptArrayNext - 1;
	int enclosingCatch = 0;

	while (index >= 0) {
	    ExceptionRange range = envPtr->exceptArrayPtr[index];

	    if ((range.type == CATCH_EXCEPTION_RANGE)
		    && (range.catchOffset == -1)) {
		enclosingCatch = 1;
		break;
	    }
	    index--;
	}
	if (!enclosingCatch) {
	    /*
	     * ... and there is no enclosing catch. Issue the maximally
	     * efficient exit instruction.
	     */

	    Tcl_DecrRefCount(returnOpts);
	    TclEmitOpcode(INST_DONE, envPtr);
	    TclAdjustStackDepth(1, envPtr);
	    return TCL_OK;
	}
    }

    /* Optimize [return -level 0 $x]. */
    Tcl_DictObjSize(NULL, returnOpts, &size);
    if (size == 0 && level == 0 && code == TCL_OK) {
	Tcl_DecrRefCount(returnOpts);
	return TCL_OK;
    }

    /*
     * Could not use the optimization, so we push the return options dict, and
     * emit the INST_RETURN_IMM instruction with code and level as operands.
     */

    CompileReturnInternal(envPtr, INST_RETURN_IMM, code, level, returnOpts);
    return TCL_OK;

  issueRuntimeReturn:
    /*
     * Assemble the option dictionary (as a list as that's good enough).
     */

    wordTokenPtr = TokenAfter(parsePtr->tokenPtr);
    for (objc=1 ; objc<=numOptionWords ; objc++) {
	CompileWord(envPtr, wordTokenPtr, interp, objc);
	wordTokenPtr = TokenAfter(wordTokenPtr);
    }
    TclEmitInstInt4(INST_LIST, numOptionWords, envPtr);

    /*
     * Push the result.
     */

    if (explicitResult) {
	CompileWord(envPtr, wordTokenPtr, interp, numWords-1);
    } else {
	PushStringLiteral(envPtr, "");
    }

    /*
     * Issue the RETURN itself.
     */

    TclEmitInvoke(envPtr, INST_RETURN_STK);
    return TCL_OK;
}

static void
CompileReturnInternal(
    CompileEnv *envPtr,
    unsigned char op,
    int code,
    int level,
    Tcl_Obj *returnOpts)
{
    if (level == 0 && (code == TCL_BREAK || code == TCL_CONTINUE)) {
	ExceptionRange *rangePtr;
	ExceptionAux *exceptAux;

	rangePtr = TclGetInnermostExceptionRange(envPtr, code, &exceptAux);
	if (rangePtr && rangePtr->type == LOOP_EXCEPTION_RANGE) {
	    TclCleanupStackForBreakContinue(envPtr, exceptAux);
	    if (code == TCL_BREAK) {
		TclAddLoopBreakFixup(envPtr, exceptAux);
	    } else {
		TclAddLoopContinueFixup(envPtr, exceptAux);
	    }
	    Tcl_DecrRefCount(returnOpts);
	    return;
	}
    }

    TclEmitPush(TclAddLiteralObj(envPtr, returnOpts, NULL), envPtr);
    TclEmitInstInt4(op, code, envPtr);
    TclEmitInt4(level, envPtr);
}

void
TclCompileSyntaxError(
    Tcl_Interp *interp,
    CompileEnv *envPtr)
{
    Tcl_Obj *msg = Tcl_GetObjResult(interp);
    int numBytes;
    const char *bytes = TclGetStringFromObj(msg, &numBytes);

    TclErrorStackResetIf(interp, bytes, numBytes);
    TclEmitPush(TclRegisterNewLiteral(envPtr, bytes, numBytes), envPtr);
    CompileReturnInternal(envPtr, INST_SYNTAX, TCL_ERROR, 0,
	    TclNoErrorStack(interp, Tcl_GetReturnOptions(interp, TCL_ERROR)));
    Tcl_ResetResult(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileUpvarCmd --
 *
 *	Procedure called to compile the "upvar" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "upvar" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileUpvarCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr, *otherTokenPtr, *localTokenPtr;
    int localIndex, numWords, i;
    DefineLineInformation;	/* TIP #280 */
    Tcl_Obj *objPtr;

    if (envPtr->procPtr == NULL) {
	return TCL_ERROR;
    }

    numWords = parsePtr->numWords;
    if (numWords < 3) {
	return TCL_ERROR;
    }

    /*
     * Push the frame index if it is known at compile time
     */

    objPtr = Tcl_NewObj();
    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    if (TclWordKnownAtCompileTime(tokenPtr, objPtr)) {
	CallFrame *framePtr;
	const Tcl_ObjType *newTypePtr, *typePtr = objPtr->typePtr;

	/*
	 * Attempt to convert to a level reference. Note that TclObjGetFrame
	 * only changes the obj type when a conversion was successful.
	 */

	TclObjGetFrame(interp, objPtr, &framePtr);
	newTypePtr = objPtr->typePtr;
	Tcl_DecrRefCount(objPtr);

	if (newTypePtr != typePtr) {
	    if (numWords%2) {
		return TCL_ERROR;
	    }
	    /* TODO: Push the known value instead? */
	    CompileWord(envPtr, tokenPtr, interp, 1);
	    otherTokenPtr = TokenAfter(tokenPtr);
	    i = 2;
	} else {
	    if (!(numWords%2)) {
		return TCL_ERROR;
	    }
	    PushStringLiteral(envPtr, "1");
	    otherTokenPtr = tokenPtr;
	    i = 1;
	}
    } else {
	Tcl_DecrRefCount(objPtr);
	return TCL_ERROR;
    }

    /*
     * Loop over the (otherVar, thisVar) pairs. If any of the thisVar is not a
     * local variable, return an error so that the non-compiled command will
     * be called at runtime.
     */

    for (; i<numWords; i+=2, otherTokenPtr = TokenAfter(localTokenPtr)) {
	localTokenPtr = TokenAfter(otherTokenPtr);

	CompileWord(envPtr, otherTokenPtr, interp, i);
	localIndex = LocalScalarFromToken(localTokenPtr, envPtr);
	if (localIndex < 0) {
	    return TCL_ERROR;
	}
	TclEmitInstInt4(	INST_UPVAR, localIndex,		envPtr);
    }

    /*
     * Pop the frame index, and set the result to empty
     */

    TclEmitOpcode(		INST_POP,			envPtr);
    PushStringLiteral(envPtr, "");
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileVariableCmd --
 *
 *	Procedure called to compile the "variable" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "variable" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileVariableCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *varTokenPtr, *valueTokenPtr;
    int localIndex, numWords, i;
    DefineLineInformation;	/* TIP #280 */

    numWords = parsePtr->numWords;
    if (numWords < 2) {
	return TCL_ERROR;
    }

    /*
     * Bail out if not compiling a proc body
     */

    if (envPtr->procPtr == NULL) {
	return TCL_ERROR;
    }

    /*
     * Loop over the (var, value) pairs.
     */

    valueTokenPtr = parsePtr->tokenPtr;
    for (i=1; i<numWords; i+=2) {
	varTokenPtr = TokenAfter(valueTokenPtr);
	valueTokenPtr = TokenAfter(varTokenPtr);

	localIndex = IndexTailVarIfKnown(interp, varTokenPtr, envPtr);

	if (localIndex < 0) {
	    return TCL_ERROR;
	}

	/* TODO: Consider what value can pass through the
	 * IndexTailVarIfKnown() screen.  Full CompileWord()
	 * likely does not apply here.  Push known value instead. */
	CompileWord(envPtr, varTokenPtr, interp, i);
	TclEmitInstInt4(	INST_VARIABLE, localIndex,	envPtr);

	if (i+1 < numWords) {
	    /*
	     * A value has been given: set the variable, pop the value
	     */

	    CompileWord(envPtr, valueTokenPtr, interp, i+1);
	    Emit14Inst(		INST_STORE_SCALAR, localIndex,	envPtr);
	    TclEmitOpcode(	INST_POP,			envPtr);
	}
    }

    /*
     * Set the result to empty
     */

    PushStringLiteral(envPtr, "");
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IndexTailVarIfKnown --
 *
 *	Procedure used in compiling [global] and [variable] commands. It
 *	inspects the variable name described by varTokenPtr and, if the tail
 *	is known at compile time, defines a corresponding local variable.
 *
 * Results:
 *	Returns the variable's index in the table of compiled locals if the
 *	tail is known at compile time, or -1 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
IndexTailVarIfKnown(
    Tcl_Interp *interp,
    Tcl_Token *varTokenPtr,	/* Token representing the variable name */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Obj *tailPtr;
    const char *tailName, *p;
    int len, n = varTokenPtr->numComponents;
    Tcl_Token *lastTokenPtr;
    int full, localIndex;

    /*
     * Determine if the tail is (a) known at compile time, and (b) not an
     * array element. Should any of these fail, return an error so that the
     * non-compiled command will be called at runtime.
     *
     * In order for the tail to be known at compile time, the last token in
     * the word has to be constant and contain "::" if it is not the only one.
     */

    if (!EnvHasLVT(envPtr)) {
	return -1;
    }

    TclNewObj(tailPtr);
    if (TclWordKnownAtCompileTime(varTokenPtr, tailPtr)) {
	full = 1;
	lastTokenPtr = varTokenPtr;
    } else {
	full = 0;
	lastTokenPtr = varTokenPtr + n;

	if (lastTokenPtr->type != TCL_TOKEN_TEXT) {
	    Tcl_DecrRefCount(tailPtr);
	    return -1;
	}
	Tcl_SetStringObj(tailPtr, lastTokenPtr->start, lastTokenPtr->size);
    }

    tailName = TclGetStringFromObj(tailPtr, &len);

    if (len) {
	if (*(tailName+len-1) == ')') {
	    /*
	     * Possible array: bail out
	     */

	    Tcl_DecrRefCount(tailPtr);
	    return -1;
	}

	/*
	 * Get the tail: immediately after the last '::'
	 */

	for (p = tailName + len -1; p > tailName; p--) {
	    if ((*p == ':') && (*(p-1) == ':')) {
		p++;
		break;
	    }
	}
	if (!full && (p == tailName)) {
	    /*
	     * No :: in the last component.
	     */

	    Tcl_DecrRefCount(tailPtr);
	    return -1;
	}
	len -= p - tailName;
	tailName = p;
    }

    localIndex = TclFindCompiledLocal(tailName, len, 1, envPtr);
    Tcl_DecrRefCount(tailPtr);
    return localIndex;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclCompileObjectNextCmd, TclCompileObjectSelfCmd --
 *
 *	Compilations of the TclOO utility commands [next] and [self].
 *
 * ----------------------------------------------------------------------
 */

int
TclCompileObjectNextCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr = parsePtr->tokenPtr;
    int i;

    if (parsePtr->numWords > 255) {
	return TCL_ERROR;
    }

    for (i=0 ; i<parsePtr->numWords ; i++) {
	CompileWord(envPtr, tokenPtr, interp, i);
	tokenPtr = TokenAfter(tokenPtr);
    }
    TclEmitInstInt1(	INST_TCLOO_NEXT, i,		envPtr);
    return TCL_OK;
}

int
TclCompileObjectNextToCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr = parsePtr->tokenPtr;
    int i;

    if (parsePtr->numWords < 2 || parsePtr->numWords > 255) {
	return TCL_ERROR;
    }

    for (i=0 ; i<parsePtr->numWords ; i++) {
	CompileWord(envPtr, tokenPtr, interp, i);
	tokenPtr = TokenAfter(tokenPtr);
    }
    TclEmitInstInt1(	INST_TCLOO_NEXT_CLASS, i,	envPtr);
    return TCL_OK;
}

int
TclCompileObjectSelfCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * We only handle [self] and [self object] (which is the same operation).
     * These are the only very common operations on [self] for which
     * bytecoding is at all reasonable.
     */

    if (parsePtr->numWords == 1) {
	goto compileSelfObject;
    } else if (parsePtr->numWords == 2) {
	Tcl_Token *tokenPtr = TokenAfter(parsePtr->tokenPtr), *subcmd;

	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD || tokenPtr[1].size==0) {
	    return TCL_ERROR;
	}

	subcmd = tokenPtr + 1;
	if (strncmp(subcmd->start, "object", subcmd->size) == 0) {
	    goto compileSelfObject;
	} else if (strncmp(subcmd->start, "namespace", subcmd->size) == 0) {
	    goto compileSelfNamespace;
	}
    }

    /*
     * Can't compile; handle with runtime call.
     */

    return TCL_ERROR;

  compileSelfObject:

    /*
     * This delegates the entire problem to a single opcode.
     */

    TclEmitOpcode(		INST_TCLOO_SELF,		envPtr);
    return TCL_OK;

  compileSelfNamespace:

    /*
     * This is formally only correct with TclOO methods as they are currently
     * implemented; it assumes that the current namespace is invariably when a
     * TclOO context is present is the object's namespace, and that's
     * technically only something that's a matter of current policy. But it
     * avoids creating another opcode, so that's all good!
     */

    TclEmitOpcode(		INST_TCLOO_SELF,		envPtr);
    TclEmitOpcode(		INST_POP,			envPtr);
    TclEmitOpcode(		INST_NS_CURRENT,		envPtr);
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
