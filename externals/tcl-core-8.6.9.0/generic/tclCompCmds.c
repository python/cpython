/*
 * tclCompCmds.c --
 *
 *	This file contains compilation procedures that compile various Tcl
 *	commands into a sequence of instructions ("bytecodes").
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

static ClientData	DupDictUpdateInfo(ClientData clientData);
static void		FreeDictUpdateInfo(ClientData clientData);
static void		PrintDictUpdateInfo(ClientData clientData,
			    Tcl_Obj *appendObj, ByteCode *codePtr,
			    unsigned int pcOffset);
static void		DisassembleDictUpdateInfo(ClientData clientData,
			    Tcl_Obj *dictObj, ByteCode *codePtr,
			    unsigned int pcOffset);
static ClientData	DupForeachInfo(ClientData clientData);
static void		FreeForeachInfo(ClientData clientData);
static void		PrintForeachInfo(ClientData clientData,
			    Tcl_Obj *appendObj, ByteCode *codePtr,
			    unsigned int pcOffset);
static void		DisassembleForeachInfo(ClientData clientData,
			    Tcl_Obj *dictObj, ByteCode *codePtr,
			    unsigned int pcOffset);
static void		PrintNewForeachInfo(ClientData clientData,
			    Tcl_Obj *appendObj, ByteCode *codePtr,
			    unsigned int pcOffset);
static void		DisassembleNewForeachInfo(ClientData clientData,
			    Tcl_Obj *dictObj, ByteCode *codePtr,
			    unsigned int pcOffset);
static int		CompileEachloopCmd(Tcl_Interp *interp,
			    Tcl_Parse *parsePtr, Command *cmdPtr,
			    CompileEnv *envPtr, int collect);
static int		CompileDictEachCmd(Tcl_Interp *interp,
			    Tcl_Parse *parsePtr, Command *cmdPtr,
			    struct CompileEnv *envPtr, int collect);

/*
 * The structures below define the AuxData types defined in this file.
 */

static const AuxDataType foreachInfoType = {
    "ForeachInfo",		/* name */
    DupForeachInfo,		/* dupProc */
    FreeForeachInfo,		/* freeProc */
    PrintForeachInfo,		/* printProc */
    DisassembleForeachInfo	/* disassembleProc */
};

static const AuxDataType newForeachInfoType = {
    "NewForeachInfo",		/* name */
    DupForeachInfo,		/* dupProc */
    FreeForeachInfo,		/* freeProc */
    PrintNewForeachInfo,	/* printProc */
    DisassembleNewForeachInfo	/* disassembleProc */
};

static const AuxDataType dictUpdateInfoType = {
    "DictUpdateInfo",		/* name */
    DupDictUpdateInfo,		/* dupProc */
    FreeDictUpdateInfo,		/* freeProc */
    PrintDictUpdateInfo,	/* printProc */
    DisassembleDictUpdateInfo	/* disassembleProc */
};

/*
 *----------------------------------------------------------------------
 *
 * TclGetAuxDataType --
 *
 *	This procedure looks up an Auxdata type by name.
 *
 * Results:
 *	If an AuxData type with name matching "typeName" is found, a pointer
 *	to its AuxDataType structure is returned; otherwise, NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const AuxDataType *
TclGetAuxDataType(
    const char *typeName)	/* Name of AuxData type to look up. */
{
    if (!strcmp(typeName, foreachInfoType.name)) {
	return &foreachInfoType;
    } else if (!strcmp(typeName, newForeachInfoType.name)) {
	return &newForeachInfoType;
    } else if (!strcmp(typeName, dictUpdateInfoType.name)) {
	return &dictUpdateInfoType;
    } else if (!strcmp(typeName, tclJumptableInfoType.name)) {
	return &tclJumptableInfoType;
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileAppendCmd --
 *
 *	Procedure called to compile the "append" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "append" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileAppendCmd(
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
    if (numWords == 1) {
	return TCL_ERROR;
    } else if (numWords == 2) {
	/*
	 * append varName == set varName
	 */

	return TclCompileSetCmd(interp, parsePtr, cmdPtr, envPtr);
    } else if (numWords > 3) {
	/*
	 * APPEND instructions currently only handle one value, but we can
	 * handle some multi-value cases by stringing them together.
	 */

	goto appendMultiple;
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
     * We are doing an assignment, otherwise TclCompileSetCmd was called, so
     * push the new value. This will need to be extended to push a value for
     * each argument.
     */

	valueTokenPtr = TokenAfter(varTokenPtr);
	CompileWord(envPtr, valueTokenPtr, interp, 2);

    /*
     * Emit instructions to set/get the variable.
     */

	if (isScalar) {
	    if (localIndex < 0) {
		TclEmitOpcode(INST_APPEND_STK, envPtr);
	    } else {
		Emit14Inst(INST_APPEND_SCALAR, localIndex, envPtr);
	    }
	} else {
	    if (localIndex < 0) {
		TclEmitOpcode(INST_APPEND_ARRAY_STK, envPtr);
	    } else {
		Emit14Inst(INST_APPEND_ARRAY, localIndex, envPtr);
	    }
	}

    return TCL_OK;

  appendMultiple:
    /*
     * Can only handle the case where we are appending to a local scalar when
     * there are multiple values to append.  Fortunately, this is common.
     */

    varTokenPtr = TokenAfter(parsePtr->tokenPtr);

    localIndex = LocalScalarFromToken(varTokenPtr, envPtr);
    if (localIndex < 0) {
	return TCL_ERROR;
    }

    /*
     * Definitely appending to a local scalar; generate the words and append
     * them.
     */

    valueTokenPtr = TokenAfter(varTokenPtr);
    for (i = 2 ; i < numWords ; i++) {
	CompileWord(envPtr, valueTokenPtr, interp, i);
	valueTokenPtr = TokenAfter(valueTokenPtr);
    }
    TclEmitInstInt4(	  INST_REVERSE, numWords-2,		envPtr);
    for (i = 2 ; i < numWords ;) {
	Emit14Inst(	  INST_APPEND_SCALAR, localIndex,	envPtr);
	if (++i < numWords) {
	    TclEmitOpcode(INST_POP,				envPtr);
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileArray*Cmd --
 *
 *	Functions called to compile "array" sucommands.
 *
 * Results:
 *	All return TCL_OK for a successful compile, and TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "array" subcommand at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileArrayExistsCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;
    int isScalar, localIndex;

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    PushVarNameWord(interp, tokenPtr, envPtr, TCL_NO_ELEMENT,
	    &localIndex, &isScalar, 1);
    if (!isScalar) {
	return TCL_ERROR;
    }

    if (localIndex >= 0) {
	TclEmitInstInt4(INST_ARRAY_EXISTS_IMM, localIndex,	envPtr);
    } else {
	TclEmitOpcode(	INST_ARRAY_EXISTS_STK,			envPtr);
    }
    return TCL_OK;
}

int
TclCompileArraySetCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *varTokenPtr, *dataTokenPtr;
    int isScalar, localIndex, code = TCL_OK;
    int isDataLiteral, isDataValid, isDataEven, len;
    int keyVar, valVar, infoIndex;
    int fwd, offsetBack, offsetFwd;
    Tcl_Obj *literalObj;
    ForeachInfo *infoPtr;

    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    varTokenPtr = TokenAfter(parsePtr->tokenPtr);
    dataTokenPtr = TokenAfter(varTokenPtr);
    literalObj = Tcl_NewObj();
    isDataLiteral = TclWordKnownAtCompileTime(dataTokenPtr, literalObj);
    isDataValid = (isDataLiteral
	    && Tcl_ListObjLength(NULL, literalObj, &len) == TCL_OK);
    isDataEven = (isDataValid && (len & 1) == 0);

    /*
     * Special case: literal odd-length argument is always an error.
     */

    if (isDataValid && !isDataEven) {
	/* Abandon custom compile and let invocation raise the error */
	code = TclCompileBasic2ArgCmd(interp, parsePtr, cmdPtr, envPtr);
	goto done;

	/*
	 * We used to compile to the bytecode that would throw the error,
	 * but that was wrong because it would not invoke the array trace
	 * on the variable.
	 *
	PushStringLiteral(envPtr, "list must have an even number of elements");
	PushStringLiteral(envPtr, "-errorcode {TCL ARGUMENT FORMAT}");
	TclEmitInstInt4(INST_RETURN_IMM, TCL_ERROR,		envPtr);
	TclEmitInt4(		0,				envPtr);
	goto done;
	 *
	 */
    }

    /*
     * Except for the special "ensure array" case below, when we're not in
     * a proc, we cannot do a better compile than generic.
     */

    if ((varTokenPtr->type != TCL_TOKEN_SIMPLE_WORD) ||
	    (envPtr->procPtr == NULL && !(isDataEven && len == 0))) {
	code = TclCompileBasic2ArgCmd(interp, parsePtr, cmdPtr, envPtr);
	goto done;
    }

    PushVarNameWord(interp, varTokenPtr, envPtr, TCL_NO_ELEMENT,
	    &localIndex, &isScalar, 1);
    if (!isScalar) {
	code = TCL_ERROR;
	goto done;
    }

    /*
     * Special case: literal empty value argument is just an "ensure array"
     * operation.
     */

    if (isDataEven && len == 0) {
	if (localIndex >= 0) {
	    TclEmitInstInt4(INST_ARRAY_EXISTS_IMM, localIndex,	envPtr);
	    TclEmitInstInt1(INST_JUMP_TRUE1, 7,			envPtr);
	    TclEmitInstInt4(INST_ARRAY_MAKE_IMM, localIndex,	envPtr);
	} else {
	    TclEmitOpcode(  INST_DUP,				envPtr);
	    TclEmitOpcode(  INST_ARRAY_EXISTS_STK,		envPtr);
	    TclEmitInstInt1(INST_JUMP_TRUE1, 5,			envPtr);
	    TclEmitOpcode(  INST_ARRAY_MAKE_STK,		envPtr);
	    TclEmitInstInt1(INST_JUMP1, 3,			envPtr);
	    /* Each branch decrements stack depth, but we only take one. */
	    TclAdjustStackDepth(1, envPtr);
	    TclEmitOpcode(  INST_POP,				envPtr);
	}
	PushStringLiteral(envPtr, "");
	goto done;
    }

    if (localIndex < 0) {
	/*
	 * a non-local variable: upvar from a local one! This consumes the
	 * variable name that was left at stacktop.
	 */

	localIndex = TclFindCompiledLocal(varTokenPtr->start,
		varTokenPtr->size, 1, envPtr);
	PushStringLiteral(envPtr, "0");
	TclEmitInstInt4(INST_REVERSE, 2,        		envPtr);
	TclEmitInstInt4(INST_UPVAR, localIndex, 		envPtr);
	TclEmitOpcode(INST_POP,          			envPtr);
    }

    /*
     * Prepare for the internal foreach.
     */

    keyVar = AnonymousLocal(envPtr);
    valVar = AnonymousLocal(envPtr);

    infoPtr = ckalloc(sizeof(ForeachInfo));
    infoPtr->numLists = 1;
    infoPtr->varLists[0] = ckalloc(sizeof(ForeachVarList) + sizeof(int));
    infoPtr->varLists[0]->numVars = 2;
    infoPtr->varLists[0]->varIndexes[0] = keyVar;
    infoPtr->varLists[0]->varIndexes[1] = valVar;
    infoIndex = TclCreateAuxData(infoPtr, &newForeachInfoType, envPtr);

    /*
     * Start issuing instructions to write to the array.
     */

    TclEmitInstInt4(INST_ARRAY_EXISTS_IMM, localIndex,	envPtr);
    TclEmitInstInt1(INST_JUMP_TRUE1, 7,			envPtr);
    TclEmitInstInt4(INST_ARRAY_MAKE_IMM, localIndex,	envPtr);

    CompileWord(envPtr, dataTokenPtr, interp, 2);
    if (!isDataLiteral || !isDataValid) {
	/*
	 * Only need this safety check if we're handling a non-literal or list
	 * containing an invalid literal; with valid list literals, we've
	 * already checked (worth it because literals are a very common
	 * use-case with [array set]).
	 */

	TclEmitOpcode(	INST_DUP,				envPtr);
	TclEmitOpcode(	INST_LIST_LENGTH,			envPtr);
	PushStringLiteral(envPtr, "1");
	TclEmitOpcode(	INST_BITAND,				envPtr);
	offsetFwd = CurrentOffset(envPtr);
	TclEmitInstInt1(INST_JUMP_FALSE1, 0,			envPtr);
	PushStringLiteral(envPtr, "list must have an even number of elements");
	PushStringLiteral(envPtr, "-errorcode {TCL ARGUMENT FORMAT}");
	TclEmitInstInt4(INST_RETURN_IMM, TCL_ERROR,		envPtr);
	TclEmitInt4(		0,				envPtr);
	TclAdjustStackDepth(-1, envPtr);
	fwd = CurrentOffset(envPtr) - offsetFwd;
	TclStoreInt1AtPtr(fwd, envPtr->codeStart+offsetFwd+1);
    }

    TclEmitInstInt4(INST_FOREACH_START, infoIndex,	envPtr);
    offsetBack = CurrentOffset(envPtr);
    Emit14Inst(	INST_LOAD_SCALAR, keyVar,		envPtr);
    Emit14Inst(	INST_LOAD_SCALAR, valVar,		envPtr);
    Emit14Inst(	INST_STORE_ARRAY, localIndex,		envPtr);
    TclEmitOpcode(	INST_POP,			envPtr);
    infoPtr->loopCtTemp = offsetBack - CurrentOffset(envPtr); /*misuse */
    TclEmitOpcode( INST_FOREACH_STEP,			envPtr);
    TclEmitOpcode( INST_FOREACH_END,			envPtr);
    TclAdjustStackDepth(-3, envPtr);
    PushStringLiteral(envPtr,	"");

    done:
    Tcl_DecrRefCount(literalObj);
    return code;
}

int
TclCompileArrayUnsetCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr = TokenAfter(parsePtr->tokenPtr);
    int isScalar, localIndex;

    if (parsePtr->numWords != 2) {
	return TclCompileBasic2ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    PushVarNameWord(interp, tokenPtr, envPtr, TCL_NO_ELEMENT,
	    &localIndex, &isScalar, 1);
    if (!isScalar) {
	return TCL_ERROR;
    }

    if (localIndex >= 0) {
	TclEmitInstInt4(INST_ARRAY_EXISTS_IMM, localIndex,	envPtr);
	TclEmitInstInt1(INST_JUMP_FALSE1, 8,			envPtr);
	TclEmitInstInt1(INST_UNSET_SCALAR, 1,			envPtr);
	TclEmitInt4(		localIndex,			envPtr);
    } else {
	TclEmitOpcode(	INST_DUP,				envPtr);
	TclEmitOpcode(	INST_ARRAY_EXISTS_STK,			envPtr);
	TclEmitInstInt1(INST_JUMP_FALSE1, 6,			envPtr);
	TclEmitInstInt1(INST_UNSET_STK, 1,			envPtr);
	TclEmitInstInt1(INST_JUMP1, 3,				envPtr);
	/* Each branch decrements stack depth, but we only take one. */
	TclAdjustStackDepth(1, envPtr);
	TclEmitOpcode(	INST_POP,				envPtr);
    }
    PushStringLiteral(envPtr,	"");
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileBreakCmd --
 *
 *	Procedure called to compile the "break" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "break" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileBreakCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    ExceptionRange *rangePtr;
    ExceptionAux *auxPtr;

    if (parsePtr->numWords != 1) {
	return TCL_ERROR;
    }

    /*
     * Find the innermost exception range that contains this command.
     */

    rangePtr = TclGetInnermostExceptionRange(envPtr, TCL_BREAK, &auxPtr);
    if (rangePtr && rangePtr->type == LOOP_EXCEPTION_RANGE) {
	/*
	 * Found the target! No need for a nasty INST_BREAK here.
	 */

	TclCleanupStackForBreakContinue(envPtr, auxPtr);
	TclAddLoopBreakFixup(envPtr, auxPtr);
    } else {
	/*
	 * Emit a real break.
	 */

	TclEmitOpcode(INST_BREAK, envPtr);
    }
    TclAdjustStackDepth(1, envPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileCatchCmd --
 *
 *	Procedure called to compile the "catch" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "catch" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileCatchCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    JumpFixup jumpFixup;
    Tcl_Token *cmdTokenPtr, *resultNameTokenPtr, *optsNameTokenPtr;
    int resultIndex, optsIndex, range, dropScript = 0;
    DefineLineInformation;	/* TIP #280 */
    int depth = TclGetStackDepth(envPtr);

    /*
     * If syntax does not match what we expect for [catch], do not compile.
     * Let runtime checks determine if syntax has changed.
     */

    if ((parsePtr->numWords < 2) || (parsePtr->numWords > 4)) {
	return TCL_ERROR;
    }

    /*
     * If variables were specified and the catch command is at global level
     * (not in a procedure), don't compile it inline: the payoff is too small.
     */

    if ((parsePtr->numWords >= 3) && !EnvHasLVT(envPtr)) {
	return TCL_ERROR;
    }

    /*
     * Make sure the variable names, if any, have no substitutions and just
     * refer to local scalars.
     */

    resultIndex = optsIndex = -1;
    cmdTokenPtr = TokenAfter(parsePtr->tokenPtr);
    if (parsePtr->numWords >= 3) {
	resultNameTokenPtr = TokenAfter(cmdTokenPtr);
	/* DGP */
	resultIndex = LocalScalarFromToken(resultNameTokenPtr, envPtr);
	if (resultIndex < 0) {
	    return TCL_ERROR;
	}

	/* DKF */
	if (parsePtr->numWords == 4) {
	    optsNameTokenPtr = TokenAfter(resultNameTokenPtr);
	    optsIndex = LocalScalarFromToken(optsNameTokenPtr, envPtr);
	    if (optsIndex < 0) {
		return TCL_ERROR;
	    }
	}
    }

    /*
     * We will compile the catch command. Declare the exception range that it
     * uses.
     *
     * If the body is a simple word, compile a BEGIN_CATCH instruction,
     * followed by the instructions to eval the body.
     * Otherwise, compile instructions to substitute the body text before
     * starting the catch, then BEGIN_CATCH, and then EVAL_STK to evaluate the
     * substituted body.
     * Care has to be taken to make sure that substitution happens outside the
     * catch range so that errors in the substitution are not caught.
     * [Bug 219184]
     * The reason for duplicating the script is that EVAL_STK would otherwise
     * begin by undeflowing the stack below the mark set by BEGIN_CATCH4.
     */

    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    if (cmdTokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	TclEmitInstInt4(	INST_BEGIN_CATCH4, range,	envPtr);
	ExceptionRangeStarts(envPtr, range);
	BODY(cmdTokenPtr, 1);
    } else {
	SetLineInformation(1);
	CompileTokens(envPtr, cmdTokenPtr, interp);
	TclEmitInstInt4(	INST_BEGIN_CATCH4, range,	envPtr);
	ExceptionRangeStarts(envPtr, range);
	TclEmitOpcode(		INST_DUP,			envPtr);
	TclEmitInvoke(envPtr,	INST_EVAL_STK);
	/* drop the script */
	dropScript = 1;
	TclEmitInstInt4(	INST_REVERSE, 2,		envPtr);
	TclEmitOpcode(		INST_POP,			envPtr);
    }
    ExceptionRangeEnds(envPtr, range);


    /*
     * Emit the "no errors" epilogue: push "0" (TCL_OK) as the catch result,
     * and jump around the "error case" code.
     */

    TclCheckStackDepth(depth+1, envPtr);
    PushStringLiteral(envPtr, "0");
    TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &jumpFixup);

    /*
     * Emit the "error case" epilogue. Push the interpreter result and the
     * return code.
     */

    ExceptionRangeTarget(envPtr, range, catchOffset);
    TclSetStackDepth(depth + dropScript, envPtr);

    if (dropScript) {
	TclEmitOpcode(		INST_POP,			envPtr);
    }


    /* Stack at this point is empty */
    TclEmitOpcode(		INST_PUSH_RESULT,		envPtr);
    TclEmitOpcode(		INST_PUSH_RETURN_CODE,		envPtr);

    /* Stack at this point on both branches: result returnCode */

    if (TclFixupForwardJumpToHere(envPtr, &jumpFixup, 127)) {
	Tcl_Panic("TclCompileCatchCmd: bad jump distance %d",
		(int)(CurrentOffset(envPtr) - jumpFixup.codeOffset));
    }

    /*
     * Push the return options if the caller wants them. This needs to happen
     * before INST_END_CATCH
     */

    if (optsIndex != -1) {
	TclEmitOpcode(		INST_PUSH_RETURN_OPTIONS,	envPtr);
    }

    /*
     * End the catch
     */

    TclEmitOpcode(		INST_END_CATCH,			envPtr);

    /*
     * Save the result and return options if the caller wants them. This needs
     * to happen after INST_END_CATCH (compile-3.6/7).
     */

    if (optsIndex != -1) {
	Emit14Inst(		INST_STORE_SCALAR, optsIndex,	envPtr);
	TclEmitOpcode(		INST_POP,			envPtr);
    }

    /*
     * At this point, the top of the stack is inconveniently ordered:
     *		result returnCode
     * Reverse the stack to store the result.
     */

    TclEmitInstInt4(	INST_REVERSE, 2,		envPtr);
    if (resultIndex != -1) {
	Emit14Inst(	INST_STORE_SCALAR, resultIndex,	envPtr);
    }
    TclEmitOpcode(	INST_POP,			envPtr);

    TclCheckStackDepth(depth+1, envPtr);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 *
 * TclCompileClockClicksCmd --
 *
 *	Procedure called to compile the "tcl::clock::clicks" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to run time.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "clock clicks"
 *	command at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileClockClicksCmd(
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token* tokenPtr;

    switch (parsePtr->numWords) {
    case 1:
	/*
	 * No args
	 */
	TclEmitInstInt1(INST_CLOCK_READ, 0, envPtr);
	break;
    case 2:
	/*
	 * -milliseconds or -microseconds
	 */
	tokenPtr = TokenAfter(parsePtr->tokenPtr);
	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD
	    || tokenPtr[1].size < 4
	    || tokenPtr[1].size > 13) {
	    return TCL_ERROR;
	} else if (!strncmp(tokenPtr[1].start, "-microseconds",
			    tokenPtr[1].size)) {
	    TclEmitInstInt1(INST_CLOCK_READ, 1, envPtr);
	    break;
	} else if (!strncmp(tokenPtr[1].start, "-milliseconds",
			    tokenPtr[1].size)) {
	    TclEmitInstInt1(INST_CLOCK_READ, 2, envPtr);
	    break;
	} else {
	    return TCL_ERROR;
	}
    default:
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*----------------------------------------------------------------------
 *
 * TclCompileClockReadingCmd --
 *
 *	Procedure called to compile the "tcl::clock::microseconds",
 *	"tcl::clock::milliseconds" and "tcl::clock::seconds" commands.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to run time.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "clock clicks"
 *	command at runtime.
 *
 * Client data is 1 for microseconds, 2 for milliseconds, 3 for seconds.
 *----------------------------------------------------------------------
 */

int
TclCompileClockReadingCmd(
    Tcl_Interp* interp,		/* Tcl interpreter */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    if (parsePtr->numWords != 1) {
	return TCL_ERROR;
    }

    TclEmitInstInt1(INST_CLOCK_READ, PTR2INT(cmdPtr->objClientData), envPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileConcatCmd --
 *
 *	Procedure called to compile the "concat" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "concat" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileConcatCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Obj *objPtr, *listObj;
    Tcl_Token *tokenPtr;
    int i;

    /* TODO: Consider compiling expansion case. */
    if (parsePtr->numWords == 1) {
	/*
	 * [concat] without arguments just pushes an empty object.
	 */

	PushStringLiteral(envPtr, "");
	return TCL_OK;
    }

    /*
     * Test if all arguments are compile-time known. If they are, we can
     * implement with a simple push.
     */

    listObj = Tcl_NewObj();
    for (i = 1, tokenPtr = parsePtr->tokenPtr; i < parsePtr->numWords; i++) {
	tokenPtr = TokenAfter(tokenPtr);
	objPtr = Tcl_NewObj();
	if (!TclWordKnownAtCompileTime(tokenPtr, objPtr)) {
	    Tcl_DecrRefCount(objPtr);
	    Tcl_DecrRefCount(listObj);
	    listObj = NULL;
	    break;
	}
	(void) Tcl_ListObjAppendElement(NULL, listObj, objPtr);
    }
    if (listObj != NULL) {
	Tcl_Obj **objs;
	const char *bytes;
	int len;

	Tcl_ListObjGetElements(NULL, listObj, &len, &objs);
	objPtr = Tcl_ConcatObj(len, objs);
	Tcl_DecrRefCount(listObj);
	bytes = Tcl_GetStringFromObj(objPtr, &len);
	PushLiteral(envPtr, bytes, len);
	Tcl_DecrRefCount(objPtr);
	return TCL_OK;
    }

    /*
     * General case: runtime concat.
     */

    for (i = 1, tokenPtr = parsePtr->tokenPtr; i < parsePtr->numWords; i++) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, i);
    }

    TclEmitInstInt4(	INST_CONCAT_STK, i-1,		envPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileContinueCmd --
 *
 *	Procedure called to compile the "continue" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "continue" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileContinueCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    ExceptionRange *rangePtr;
    ExceptionAux *auxPtr;

    /*
     * There should be no argument after the "continue".
     */

    if (parsePtr->numWords != 1) {
	return TCL_ERROR;
    }

    /*
     * See if we can find a valid continueOffset (i.e., not -1) in the
     * innermost containing exception range.
     */

    rangePtr = TclGetInnermostExceptionRange(envPtr, TCL_CONTINUE, &auxPtr);
    if (rangePtr && rangePtr->type == LOOP_EXCEPTION_RANGE) {
	/*
	 * Found the target! No need for a nasty INST_CONTINUE here.
	 */

	TclCleanupStackForBreakContinue(envPtr, auxPtr);
	TclAddLoopContinueFixup(envPtr, auxPtr);
    } else {
	/*
	 * Emit a real continue.
	 */

	TclEmitOpcode(INST_CONTINUE, envPtr);
    }
    TclAdjustStackDepth(1, envPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileDict*Cmd --
 *
 *	Functions called to compile "dict" sucommands.
 *
 * Results:
 *	All return TCL_OK for a successful compile, and TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "dict" subcommand at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileDictSetCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;
    int i, dictVarIndex;
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *varTokenPtr;

    /*
     * There must be at least one argument after the command.
     */

    if (parsePtr->numWords < 4) {
	return TCL_ERROR;
    }

    /*
     * The dictionary variable must be a local scalar that is knowable at
     * compile time; anything else exceeds the complexity of the opcode. So
     * discover what the index is.
     */

    varTokenPtr = TokenAfter(parsePtr->tokenPtr);
    dictVarIndex = LocalScalarFromToken(varTokenPtr, envPtr);
    if (dictVarIndex < 0) {
	return TCL_ERROR;
    }

    /*
     * Remaining words (key path and value to set) can be handled normally.
     */

    tokenPtr = TokenAfter(varTokenPtr);
    for (i=2 ; i< parsePtr->numWords ; i++) {
	CompileWord(envPtr, tokenPtr, interp, i);
	tokenPtr = TokenAfter(tokenPtr);
    }

    /*
     * Now emit the instruction to do the dict manipulation.
     */

    TclEmitInstInt4( INST_DICT_SET, parsePtr->numWords-3,	envPtr);
    TclEmitInt4(     dictVarIndex,			envPtr);
    TclAdjustStackDepth(-1, envPtr);
    return TCL_OK;
}

int
TclCompileDictIncrCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *varTokenPtr, *keyTokenPtr;
    int dictVarIndex, incrAmount;

    /*
     * There must be at least two arguments after the command.
     */

    if (parsePtr->numWords < 3 || parsePtr->numWords > 4) {
	return TCL_ERROR;
    }
    varTokenPtr = TokenAfter(parsePtr->tokenPtr);
    keyTokenPtr = TokenAfter(varTokenPtr);

    /*
     * Parse the increment amount, if present.
     */

    if (parsePtr->numWords == 4) {
	const char *word;
	int numBytes, code;
	Tcl_Token *incrTokenPtr;
	Tcl_Obj *intObj;

	incrTokenPtr = TokenAfter(keyTokenPtr);
	if (incrTokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    return TclCompileBasic2Or3ArgCmd(interp, parsePtr,cmdPtr, envPtr);
	}
	word = incrTokenPtr[1].start;
	numBytes = incrTokenPtr[1].size;

	intObj = Tcl_NewStringObj(word, numBytes);
	Tcl_IncrRefCount(intObj);
	code = TclGetIntFromObj(NULL, intObj, &incrAmount);
	TclDecrRefCount(intObj);
	if (code != TCL_OK) {
	    return TclCompileBasic2Or3ArgCmd(interp, parsePtr,cmdPtr, envPtr);
	}
    } else {
	incrAmount = 1;
    }

    /*
     * The dictionary variable must be a local scalar that is knowable at
     * compile time; anything else exceeds the complexity of the opcode. So
     * discover what the index is.
     */

    dictVarIndex = LocalScalarFromToken(varTokenPtr, envPtr);
    if (dictVarIndex < 0) {
	return TclCompileBasic2Or3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    /*
     * Emit the key and the code to actually do the increment.
     */

    CompileWord(envPtr, keyTokenPtr, interp, 2);
    TclEmitInstInt4( INST_DICT_INCR_IMM, incrAmount,	envPtr);
    TclEmitInt4(     dictVarIndex,			envPtr);
    return TCL_OK;
}

int
TclCompileDictGetCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;
    int i;
    DefineLineInformation;	/* TIP #280 */

    /*
     * There must be at least two arguments after the command (the single-arg
     * case is legal, but too special and magic for us to deal with here).
     */

    /* TODO: Consider support for compiling expanded args. */
    if (parsePtr->numWords < 3) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(parsePtr->tokenPtr);

    /*
     * Only compile this because we need INST_DICT_GET anyway.
     */

    for (i=1 ; i<parsePtr->numWords ; i++) {
	CompileWord(envPtr, tokenPtr, interp, i);
	tokenPtr = TokenAfter(tokenPtr);
    }
    TclEmitInstInt4(INST_DICT_GET, parsePtr->numWords-2, envPtr);
    TclAdjustStackDepth(-1, envPtr);
    return TCL_OK;
}

int
TclCompileDictExistsCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;
    int i;
    DefineLineInformation;	/* TIP #280 */

    /*
     * There must be at least two arguments after the command (the single-arg
     * case is legal, but too special and magic for us to deal with here).
     */

    /* TODO: Consider support for compiling expanded args. */
    if (parsePtr->numWords < 3) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(parsePtr->tokenPtr);

    /*
     * Now we do the code generation.
     */

    for (i=1 ; i<parsePtr->numWords ; i++) {
	CompileWord(envPtr, tokenPtr, interp, i);
	tokenPtr = TokenAfter(tokenPtr);
    }
    TclEmitInstInt4(INST_DICT_EXISTS, parsePtr->numWords-2, envPtr);
    TclAdjustStackDepth(-1, envPtr);
    return TCL_OK;
}

int
TclCompileDictUnsetCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;
    DefineLineInformation;	/* TIP #280 */
    int i, dictVarIndex;

    /*
     * There must be at least one argument after the variable name for us to
     * compile to bytecode.
     */

    /* TODO: Consider support for compiling expanded args. */
    if (parsePtr->numWords < 3) {
	return TCL_ERROR;
    }

    /*
     * The dictionary variable must be a local scalar that is knowable at
     * compile time; anything else exceeds the complexity of the opcode. So
     * discover what the index is.
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    dictVarIndex = LocalScalarFromToken(tokenPtr, envPtr);
    if (dictVarIndex < 0) {
	return TclCompileBasicMin2ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    /*
     * Remaining words (the key path) can be handled normally.
     */

    for (i=2 ; i<parsePtr->numWords ; i++) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, i);
    }

    /*
     * Now emit the instruction to do the dict manipulation.
     */

    TclEmitInstInt4( INST_DICT_UNSET, parsePtr->numWords-2,	envPtr);
    TclEmitInt4(	dictVarIndex,				envPtr);
    return TCL_OK;
}

int
TclCompileDictCreateCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    int worker;			/* Temp var for building the value in. */
    Tcl_Token *tokenPtr;
    Tcl_Obj *keyObj, *valueObj, *dictObj;
    const char *bytes;
    int i, len;

    if ((parsePtr->numWords & 1) == 0) {
	return TCL_ERROR;
    }

    /*
     * See if we can build the value at compile time...
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    dictObj = Tcl_NewObj();
    Tcl_IncrRefCount(dictObj);
    for (i=1 ; i<parsePtr->numWords ; i+=2) {
	keyObj = Tcl_NewObj();
	Tcl_IncrRefCount(keyObj);
	if (!TclWordKnownAtCompileTime(tokenPtr, keyObj)) {
	    Tcl_DecrRefCount(keyObj);
	    Tcl_DecrRefCount(dictObj);
	    goto nonConstant;
	}
	tokenPtr = TokenAfter(tokenPtr);
	valueObj = Tcl_NewObj();
	Tcl_IncrRefCount(valueObj);
	if (!TclWordKnownAtCompileTime(tokenPtr, valueObj)) {
	    Tcl_DecrRefCount(keyObj);
	    Tcl_DecrRefCount(valueObj);
	    Tcl_DecrRefCount(dictObj);
	    goto nonConstant;
	}
	tokenPtr = TokenAfter(tokenPtr);
	Tcl_DictObjPut(NULL, dictObj, keyObj, valueObj);
	Tcl_DecrRefCount(keyObj);
	Tcl_DecrRefCount(valueObj);
    }

    /*
     * We did! Excellent. The "verifyDict" is to do type forcing.
     */

    bytes = Tcl_GetStringFromObj(dictObj, &len);
    PushLiteral(envPtr, bytes, len);
    TclEmitOpcode(		INST_DUP,			envPtr);
    TclEmitOpcode(		INST_DICT_VERIFY,		envPtr);
    Tcl_DecrRefCount(dictObj);
    return TCL_OK;

    /*
     * Otherwise, we've got to issue runtime code to do the building, which we
     * do by [dict set]ting into an unnamed local variable. This requires that
     * we are in a context with an LVT.
     */

  nonConstant:
    worker = AnonymousLocal(envPtr);
    if (worker < 0) {
	return TclCompileBasicMin0ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    PushStringLiteral(envPtr,		"");
    Emit14Inst(			INST_STORE_SCALAR, worker,	envPtr);
    TclEmitOpcode(		INST_POP,			envPtr);
    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    for (i=1 ; i<parsePtr->numWords ; i+=2) {
	CompileWord(envPtr, tokenPtr, interp, i);
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, i+1);
	tokenPtr = TokenAfter(tokenPtr);
	TclEmitInstInt4(	INST_DICT_SET, 1,		envPtr);
	TclEmitInt4(			worker,			envPtr);
	TclAdjustStackDepth(-1, envPtr);
	TclEmitOpcode(		INST_POP,			envPtr);
    }
    Emit14Inst(			INST_LOAD_SCALAR, worker,	envPtr);
    TclEmitInstInt1(		INST_UNSET_SCALAR, 0,		envPtr);
    TclEmitInt4(			worker,			envPtr);
    return TCL_OK;
}

int
TclCompileDictMergeCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;
    int i, workerIndex, infoIndex, outLoop;

    /*
     * Deal with some special edge cases. Note that in the case with one
     * argument, the only thing to do is to verify the dict-ness.
     */

    /* TODO: Consider support for compiling expanded args. (less likely) */
    if (parsePtr->numWords < 2) {
	PushStringLiteral(envPtr, "");
	return TCL_OK;
    } else if (parsePtr->numWords == 2) {
	tokenPtr = TokenAfter(parsePtr->tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, 1);
	TclEmitOpcode(		INST_DUP,			envPtr);
	TclEmitOpcode(		INST_DICT_VERIFY,		envPtr);
	return TCL_OK;
    }

    /*
     * There's real merging work to do.
     *
     * Allocate some working space. This means we'll only ever compile this
     * command when there's an LVT present.
     */

    workerIndex = AnonymousLocal(envPtr);
    if (workerIndex < 0) {
	return TclCompileBasicMin2ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }
    infoIndex = AnonymousLocal(envPtr);

    /*
     * Get the first dictionary and verify that it is so.
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 1);
    TclEmitOpcode(		INST_DUP,			envPtr);
    TclEmitOpcode(		INST_DICT_VERIFY,		envPtr);
    Emit14Inst(			INST_STORE_SCALAR, workerIndex,	envPtr);
    TclEmitOpcode(		INST_POP,			envPtr);

    /*
     * For each of the remaining dictionaries...
     */

    outLoop = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    TclEmitInstInt4(		INST_BEGIN_CATCH4, outLoop,	envPtr);
    ExceptionRangeStarts(envPtr, outLoop);
    for (i=2 ; i<parsePtr->numWords ; i++) {
	/*
	 * Get the dictionary, and merge its pairs into the first dict (using
	 * a small loop).
	 */

	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, i);
	TclEmitInstInt4(	INST_DICT_FIRST, infoIndex,	envPtr);
	TclEmitInstInt1(	INST_JUMP_TRUE1, 24,		envPtr);
	TclEmitInstInt4(	INST_REVERSE, 2,		envPtr);
	TclEmitInstInt4(	INST_DICT_SET, 1,		envPtr);
	TclEmitInt4(			workerIndex,		envPtr);
	TclAdjustStackDepth(-1, envPtr);
	TclEmitOpcode(		INST_POP,			envPtr);
	TclEmitInstInt4(	INST_DICT_NEXT, infoIndex,	envPtr);
	TclEmitInstInt1(	INST_JUMP_FALSE1, -20,		envPtr);
	TclEmitOpcode(		INST_POP,			envPtr);
	TclEmitOpcode(		INST_POP,			envPtr);
	TclEmitInstInt1(	INST_UNSET_SCALAR, 0,		envPtr);
	TclEmitInt4(			infoIndex,		envPtr);
    }
    ExceptionRangeEnds(envPtr, outLoop);
    TclEmitOpcode(		INST_END_CATCH,			envPtr);

    /*
     * Clean up any state left over.
     */

    Emit14Inst(			INST_LOAD_SCALAR, workerIndex,	envPtr);
    TclEmitInstInt1(		INST_UNSET_SCALAR, 0,		envPtr);
    TclEmitInt4(			workerIndex,		envPtr);
    TclEmitInstInt1(		INST_JUMP1, 18,			envPtr);

    /*
     * If an exception happens when starting to iterate over the second (and
     * subsequent) dicts. This is strictly not necessary, but it is nice.
     */

    TclAdjustStackDepth(-1, envPtr);
    ExceptionRangeTarget(envPtr, outLoop, catchOffset);
    TclEmitOpcode(		INST_PUSH_RETURN_OPTIONS,	envPtr);
    TclEmitOpcode(		INST_PUSH_RESULT,		envPtr);
    TclEmitOpcode(		INST_END_CATCH,			envPtr);
    TclEmitInstInt1(		INST_UNSET_SCALAR, 0,		envPtr);
    TclEmitInt4(			workerIndex,		envPtr);
    TclEmitInstInt1(		INST_UNSET_SCALAR, 0,		envPtr);
    TclEmitInt4(			infoIndex,		envPtr);
    TclEmitOpcode(		INST_RETURN_STK,		envPtr);

    return TCL_OK;
}

int
TclCompileDictForCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    return CompileDictEachCmd(interp, parsePtr, cmdPtr, envPtr,
	    TCL_EACH_KEEP_NONE);
}

int
TclCompileDictMapCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    return CompileDictEachCmd(interp, parsePtr, cmdPtr, envPtr,
	    TCL_EACH_COLLECT);
}

int
CompileDictEachCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr,		/* Holds resulting instructions. */
    int collect)		/* Flag == TCL_EACH_COLLECT to collect and
				 * construct a new dictionary with the loop
				 * body result. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *varsTokenPtr, *dictTokenPtr, *bodyTokenPtr;
    int keyVarIndex, valueVarIndex, nameChars, loopRange, catchRange;
    int infoIndex, jumpDisplacement, bodyTargetOffset, emptyTargetOffset;
    int numVars, endTargetOffset;
    int collectVar = -1;	/* Index of temp var holding the result
				 * dict. */
    const char **argv;
    Tcl_DString buffer;

    /*
     * There must be three arguments after the command.
     */

    if (parsePtr->numWords != 4) {
	return TclCompileBasic3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    varsTokenPtr = TokenAfter(parsePtr->tokenPtr);
    dictTokenPtr = TokenAfter(varsTokenPtr);
    bodyTokenPtr = TokenAfter(dictTokenPtr);
    if (varsTokenPtr->type != TCL_TOKEN_SIMPLE_WORD ||
	    bodyTokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	return TclCompileBasic3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    /*
     * Create temporary variable to capture return values from loop body when
     * we're collecting results.
     */

    if (collect == TCL_EACH_COLLECT) {
	collectVar = AnonymousLocal(envPtr);
	if (collectVar < 0) {
	    return TclCompileBasic3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
	}
    }

    /*
     * Check we've got a pair of variables and that they are local variables.
     * Then extract their indices in the LVT.
     */

    Tcl_DStringInit(&buffer);
    TclDStringAppendToken(&buffer, &varsTokenPtr[1]);
    if (Tcl_SplitList(NULL, Tcl_DStringValue(&buffer), &numVars,
	    &argv) != TCL_OK) {
	Tcl_DStringFree(&buffer);
	return TclCompileBasic3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }
    Tcl_DStringFree(&buffer);
    if (numVars != 2) {
	ckfree(argv);
	return TclCompileBasic3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    nameChars = strlen(argv[0]);
    keyVarIndex = LocalScalar(argv[0], nameChars, envPtr);
    nameChars = strlen(argv[1]);
    valueVarIndex = LocalScalar(argv[1], nameChars, envPtr);
    ckfree(argv);

    if ((keyVarIndex < 0) || (valueVarIndex < 0)) {
	return TclCompileBasic3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    /*
     * Allocate a temporary variable to store the iterator reference. The
     * variable will contain a Tcl_DictSearch reference which will be
     * allocated by INST_DICT_FIRST and disposed when the variable is unset
     * (at which point it should also have been finished with).
     */

    infoIndex = AnonymousLocal(envPtr);
    if (infoIndex < 0) {
	return TclCompileBasic3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    /*
     * Preparation complete; issue instructions. Note that this code issues
     * fixed-sized jumps. That simplifies things a lot!
     *
     * First up, initialize the accumulator dictionary if needed.
     */

    if (collect == TCL_EACH_COLLECT) {
	PushStringLiteral(envPtr, "");
	Emit14Inst(	INST_STORE_SCALAR, collectVar,		envPtr);
	TclEmitOpcode(	INST_POP,				envPtr);
    }

    /*
     * Get the dictionary and start the iteration. No catching of errors at
     * this point.
     */

    CompileWord(envPtr, dictTokenPtr, interp, 2);

    /*
     * Now we catch errors from here on so that we can finalize the search
     * started by Tcl_DictObjFirst above.
     */

    catchRange = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    TclEmitInstInt4(	INST_BEGIN_CATCH4, catchRange,		envPtr);
    ExceptionRangeStarts(envPtr, catchRange);

    TclEmitInstInt4(	INST_DICT_FIRST, infoIndex,		envPtr);
    emptyTargetOffset = CurrentOffset(envPtr);
    TclEmitInstInt4(	INST_JUMP_TRUE4, 0,			envPtr);

    /*
     * Inside the iteration, write the loop variables.
     */

    bodyTargetOffset = CurrentOffset(envPtr);
    Emit14Inst(		INST_STORE_SCALAR, keyVarIndex,		envPtr);
    TclEmitOpcode(	INST_POP,				envPtr);
    Emit14Inst(		INST_STORE_SCALAR, valueVarIndex,	envPtr);
    TclEmitOpcode(	INST_POP,				envPtr);

    /*
     * Set up the loop exception targets.
     */

    loopRange = TclCreateExceptRange(LOOP_EXCEPTION_RANGE, envPtr);
    ExceptionRangeStarts(envPtr, loopRange);

    /*
     * Compile the loop body itself. It should be stack-neutral.
     */

    BODY(bodyTokenPtr, 3);
    if (collect == TCL_EACH_COLLECT) {
	Emit14Inst(	INST_LOAD_SCALAR, keyVarIndex,		envPtr);
	TclEmitInstInt4(INST_OVER, 1,				envPtr);
	TclEmitInstInt4(INST_DICT_SET, 1,			envPtr);
	TclEmitInt4(		collectVar,			envPtr);
	TclAdjustStackDepth(-1, envPtr);
	TclEmitOpcode(	INST_POP,				envPtr);
    }
    TclEmitOpcode(	INST_POP,				envPtr);

    /*
     * Both exception target ranges (error and loop) end here.
     */

    ExceptionRangeEnds(envPtr, loopRange);
    ExceptionRangeEnds(envPtr, catchRange);

    /*
     * Continue (or just normally process) by getting the next pair of items
     * from the dictionary and jumping back to the code to write them into
     * variables if there is another pair.
     */

    ExceptionRangeTarget(envPtr, loopRange, continueOffset);
    TclEmitInstInt4(	INST_DICT_NEXT, infoIndex,		envPtr);
    jumpDisplacement = bodyTargetOffset - CurrentOffset(envPtr);
    TclEmitInstInt4(	INST_JUMP_FALSE4, jumpDisplacement,	envPtr);
    endTargetOffset = CurrentOffset(envPtr);
    TclEmitInstInt1(	INST_JUMP1, 0,				envPtr);

    /*
     * Error handler "finally" clause, which force-terminates the iteration
     * and rethrows the error.
     */

    TclAdjustStackDepth(-1, envPtr);
    ExceptionRangeTarget(envPtr, catchRange, catchOffset);
    TclEmitOpcode(	INST_PUSH_RETURN_OPTIONS,		envPtr);
    TclEmitOpcode(	INST_PUSH_RESULT,			envPtr);
    TclEmitOpcode(	INST_END_CATCH,				envPtr);
    TclEmitInstInt1(	INST_UNSET_SCALAR, 0,			envPtr);
    TclEmitInt4(		infoIndex,			envPtr);
    if (collect == TCL_EACH_COLLECT) {
	TclEmitInstInt1(INST_UNSET_SCALAR, 0,			envPtr);
	TclEmitInt4(		collectVar,			envPtr);
    }
    TclEmitOpcode(	INST_RETURN_STK,			envPtr);

    /*
     * Otherwise we're done (the jump after the DICT_FIRST points here) and we
     * need to pop the bogus key/value pair (pushed to keep stack calculations
     * easy!) Note that we skip the END_CATCH. [Bug 1382528]
     */

    jumpDisplacement = CurrentOffset(envPtr) - emptyTargetOffset;
    TclUpdateInstInt4AtPc(INST_JUMP_TRUE4, jumpDisplacement,
	    envPtr->codeStart + emptyTargetOffset);
    jumpDisplacement = CurrentOffset(envPtr) - endTargetOffset;
    TclUpdateInstInt1AtPc(INST_JUMP1, jumpDisplacement,
	    envPtr->codeStart + endTargetOffset);
    TclEmitOpcode(	INST_POP,				envPtr);
    TclEmitOpcode(	INST_POP,				envPtr);
    ExceptionRangeTarget(envPtr, loopRange, breakOffset);
    TclFinalizeLoopExceptionRange(envPtr, loopRange);
    TclEmitOpcode(	INST_END_CATCH,				envPtr);

    /*
     * Final stage of the command (normal case) is that we push an empty
     * object (or push the accumulator as the result object). This is done
     * last to promote peephole optimization when it's dropped immediately.
     */

    TclEmitInstInt1(	INST_UNSET_SCALAR, 0,			envPtr);
    TclEmitInt4(		infoIndex,			envPtr);
    if (collect == TCL_EACH_COLLECT) {
	Emit14Inst(	INST_LOAD_SCALAR, collectVar,		envPtr);
	TclEmitInstInt1(INST_UNSET_SCALAR, 0,			envPtr);
	TclEmitInt4(		collectVar,			envPtr);
    } else {
	PushStringLiteral(envPtr, "");
    }
    return TCL_OK;
}

int
TclCompileDictUpdateCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    int i, dictIndex, numVars, range, infoIndex;
    Tcl_Token **keyTokenPtrs, *dictVarTokenPtr, *bodyTokenPtr, *tokenPtr;
    DictUpdateInfo *duiPtr;
    JumpFixup jumpFixup;

    /*
     * There must be at least one argument after the command.
     */

    if (parsePtr->numWords < 5) {
	return TCL_ERROR;
    }

    /*
     * Parse the command. Expect the following:
     *   dict update <lit(eral)> <any> <lit> ?<any> <lit> ...? <lit>
     */

    if ((parsePtr->numWords - 1) & 1) {
	return TCL_ERROR;
    }
    numVars = (parsePtr->numWords - 3) / 2;

    /*
     * The dictionary variable must be a local scalar that is knowable at
     * compile time; anything else exceeds the complexity of the opcode. So
     * discover what the index is.
     */

    dictVarTokenPtr = TokenAfter(parsePtr->tokenPtr);
    dictIndex = LocalScalarFromToken(dictVarTokenPtr, envPtr);
    if (dictIndex < 0) {
	goto issueFallback;
    }

    /*
     * Assemble the instruction metadata. This is complex enough that it is
     * represented as auxData; it holds an ordered list of variable indices
     * that are to be used.
     */

    duiPtr = ckalloc(sizeof(DictUpdateInfo) + sizeof(int) * (numVars - 1));
    duiPtr->length = numVars;
    keyTokenPtrs = TclStackAlloc(interp, sizeof(Tcl_Token *) * numVars);
    tokenPtr = TokenAfter(dictVarTokenPtr);

    for (i=0 ; i<numVars ; i++) {
	/*
	 * Put keys to one side for later compilation to bytecode.
	 */

	keyTokenPtrs[i] = tokenPtr;
	tokenPtr = TokenAfter(tokenPtr);

	/*
	 * Stash the index in the auxiliary data (if it is indeed a local
	 * scalar that is resolvable at compile-time).
	 */

	duiPtr->varIndices[i] = LocalScalarFromToken(tokenPtr, envPtr);
	if (duiPtr->varIndices[i] < 0) {
	    goto failedUpdateInfoAssembly;
	}
	tokenPtr = TokenAfter(tokenPtr);
    }
    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	goto failedUpdateInfoAssembly;
    }
    bodyTokenPtr = tokenPtr;

    /*
     * The list of variables to bind is stored in auxiliary data so that it
     * can't be snagged by literal sharing and forced to shimmer dangerously.
     */

    infoIndex = TclCreateAuxData(duiPtr, &dictUpdateInfoType, envPtr);

    for (i=0 ; i<numVars ; i++) {
	CompileWord(envPtr, keyTokenPtrs[i], interp, 2*i+2);
    }
    TclEmitInstInt4(	INST_LIST, numVars,			envPtr);
    TclEmitInstInt4(	INST_DICT_UPDATE_START, dictIndex,	envPtr);
    TclEmitInt4(		infoIndex,			envPtr);

    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    TclEmitInstInt4(	INST_BEGIN_CATCH4, range,		envPtr);

    ExceptionRangeStarts(envPtr, range);
    BODY(bodyTokenPtr, parsePtr->numWords - 1);
    ExceptionRangeEnds(envPtr, range);

    /*
     * Normal termination code: the stack has the key list below the result of
     * the body evaluation: swap them and finish the update code.
     */

    TclEmitOpcode(	INST_END_CATCH,				envPtr);
    TclEmitInstInt4(	INST_REVERSE, 2,			envPtr);
    TclEmitInstInt4(	INST_DICT_UPDATE_END, dictIndex,	envPtr);
    TclEmitInt4(		infoIndex,			envPtr);

    /*
     * Jump around the exceptional termination code.
     */

    TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &jumpFixup);

    /*
     * Termination code for non-ok returns: stash the result and return
     * options in the stack, bring up the key list, finish the update code,
     * and finally return with the catched return data
     */

    ExceptionRangeTarget(envPtr, range, catchOffset);
    TclEmitOpcode(	INST_PUSH_RESULT,			envPtr);
    TclEmitOpcode(	INST_PUSH_RETURN_OPTIONS,		envPtr);
    TclEmitOpcode(	INST_END_CATCH,				envPtr);
    TclEmitInstInt4(	INST_REVERSE, 3,			envPtr);

    TclEmitInstInt4(	INST_DICT_UPDATE_END, dictIndex,	envPtr);
    TclEmitInt4(		infoIndex,			envPtr);
    TclEmitInvoke(envPtr,INST_RETURN_STK);

    if (TclFixupForwardJumpToHere(envPtr, &jumpFixup, 127)) {
	Tcl_Panic("TclCompileDictCmd(update): bad jump distance %d",
		(int) (CurrentOffset(envPtr) - jumpFixup.codeOffset));
    }
    TclStackFree(interp, keyTokenPtrs);
    return TCL_OK;

    /*
     * Clean up after a failure to create the DictUpdateInfo structure.
     */

  failedUpdateInfoAssembly:
    ckfree(duiPtr);
    TclStackFree(interp, keyTokenPtrs);
  issueFallback:
    return TclCompileBasicMin2ArgCmd(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileDictAppendCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;
    int i, dictVarIndex;

    /*
     * There must be at least two argument after the command. And we impose an
     * (arbirary) safe limit; anyone exceeding it should stop worrying about
     * speed quite so much. ;-)
     */

    /* TODO: Consider support for compiling expanded args. */
    if (parsePtr->numWords<4 || parsePtr->numWords>100) {
	return TCL_ERROR;
    }

    /*
     * Get the index of the local variable that we will be working with.
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    dictVarIndex = LocalScalarFromToken(tokenPtr, envPtr);
    if (dictVarIndex < 0) {
	return TclCompileBasicMin2ArgCmd(interp, parsePtr,cmdPtr, envPtr);
    }

    /*
     * Produce the string to concatenate onto the dictionary entry.
     */

    tokenPtr = TokenAfter(tokenPtr);
    for (i=2 ; i<parsePtr->numWords ; i++) {
	CompileWord(envPtr, tokenPtr, interp, i);
	tokenPtr = TokenAfter(tokenPtr);
    }
    if (parsePtr->numWords > 4) {
	TclEmitInstInt1(INST_STR_CONCAT1, parsePtr->numWords-3, envPtr);
    }

    /*
     * Do the concatenation.
     */

    TclEmitInstInt4(INST_DICT_APPEND, dictVarIndex, envPtr);
    return TCL_OK;
}

int
TclCompileDictLappendCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *varTokenPtr, *keyTokenPtr, *valueTokenPtr;
    int dictVarIndex;

    /*
     * There must be three arguments after the command.
     */

    /* TODO: Consider support for compiling expanded args. */
    /* Probably not.  Why is INST_DICT_LAPPEND limited to one value? */
    if (parsePtr->numWords != 4) {
	return TCL_ERROR;
    }

    /*
     * Parse the arguments.
     */

    varTokenPtr = TokenAfter(parsePtr->tokenPtr);
    keyTokenPtr = TokenAfter(varTokenPtr);
    valueTokenPtr = TokenAfter(keyTokenPtr);
    dictVarIndex = LocalScalarFromToken(varTokenPtr, envPtr);
    if (dictVarIndex < 0) {
	return TclCompileBasic3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    /*
     * Issue the implementation.
     */

    CompileWord(envPtr, keyTokenPtr, interp, 2);
    CompileWord(envPtr, valueTokenPtr, interp, 3);
    TclEmitInstInt4(	INST_DICT_LAPPEND, dictVarIndex,	envPtr);
    return TCL_OK;
}

int
TclCompileDictWithCmd(
    Tcl_Interp *interp,		/* Used for looking up stuff. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    int i, range, varNameTmp = -1, pathTmp = -1, keysTmp, gotPath;
    int dictVar, bodyIsEmpty = 1;
    Tcl_Token *varTokenPtr, *tokenPtr;
    JumpFixup jumpFixup;
    const char *ptr, *end;

    /*
     * There must be at least one argument after the command.
     */

    /* TODO: Consider support for compiling expanded args. */
    if (parsePtr->numWords < 3) {
	return TCL_ERROR;
    }

    /*
     * Parse the command (trivially). Expect the following:
     *   dict with <any (varName)> ?<any> ...? <literal>
     */

    varTokenPtr = TokenAfter(parsePtr->tokenPtr);
    tokenPtr = TokenAfter(varTokenPtr);
    for (i=3 ; i<parsePtr->numWords ; i++) {
	tokenPtr = TokenAfter(tokenPtr);
    }
    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	return TclCompileBasicMin2ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    /*
     * Test if the last word is an empty script; if so, we can compile it in
     * all cases, but if it is non-empty we need local variable table entries
     * to hold the temporary variables (used to keep stack usage simple).
     */

    for (ptr=tokenPtr[1].start,end=ptr+tokenPtr[1].size ; ptr!=end ; ptr++) {
	if (*ptr!=' ' && *ptr!='\t' && *ptr!='\n' && *ptr!='\r') {
	    if (envPtr->procPtr == NULL) {
		return TclCompileBasicMin2ArgCmd(interp, parsePtr, cmdPtr,
			envPtr);
	    }
	    bodyIsEmpty = 0;
	    break;
	}
    }

    /*
     * Determine if we're manipulating a dict in a simple local variable.
     */

    gotPath = (parsePtr->numWords > 3);
    dictVar = LocalScalarFromToken(varTokenPtr, envPtr);

    /*
     * Special case: an empty body means we definitely have no need to issue
     * try-finally style code or to allocate local variable table entries for
     * storing temporaries. Still need to do both INST_DICT_EXPAND and
     * INST_DICT_RECOMBINE_* though, because we can't determine if we're free
     * of traces.
     */

    if (bodyIsEmpty) {
	if (dictVar >= 0) {
	    if (gotPath) {
		/*
		 * Case: Path into dict in LVT with empty body.
		 */

		tokenPtr = TokenAfter(varTokenPtr);
		for (i=2 ; i<parsePtr->numWords-1 ; i++) {
		    CompileWord(envPtr, tokenPtr, interp, i);
		    tokenPtr = TokenAfter(tokenPtr);
		}
		TclEmitInstInt4(INST_LIST, parsePtr->numWords-3,envPtr);
		Emit14Inst(	INST_LOAD_SCALAR, dictVar,	envPtr);
		TclEmitInstInt4(INST_OVER, 1,			envPtr);
		TclEmitOpcode(	INST_DICT_EXPAND,		envPtr);
		TclEmitInstInt4(INST_DICT_RECOMBINE_IMM, dictVar, envPtr);
	    } else {
		/*
		 * Case: Direct dict in LVT with empty body.
		 */

		PushStringLiteral(envPtr, "");
		Emit14Inst(	INST_LOAD_SCALAR, dictVar,	envPtr);
		PushStringLiteral(envPtr, "");
		TclEmitOpcode(	INST_DICT_EXPAND,		envPtr);
		TclEmitInstInt4(INST_DICT_RECOMBINE_IMM, dictVar, envPtr);
	    }
	} else {
	    if (gotPath) {
		/*
		 * Case: Path into dict in non-simple var with empty body.
		 */

		tokenPtr = varTokenPtr;
		for (i=1 ; i<parsePtr->numWords-1 ; i++) {
		    CompileWord(envPtr, tokenPtr, interp, i);
		    tokenPtr = TokenAfter(tokenPtr);
		}
		TclEmitInstInt4(INST_LIST, parsePtr->numWords-3,envPtr);
		TclEmitInstInt4(INST_OVER, 1,			envPtr);
		TclEmitOpcode(	INST_LOAD_STK,			envPtr);
		TclEmitInstInt4(INST_OVER, 1,			envPtr);
		TclEmitOpcode(	INST_DICT_EXPAND,		envPtr);
		TclEmitOpcode(	INST_DICT_RECOMBINE_STK,	envPtr);
	    } else {
		/*
		 * Case: Direct dict in non-simple var with empty body.
		 */

		CompileWord(envPtr, varTokenPtr, interp, 1);
		TclEmitOpcode(	INST_DUP,			envPtr);
		TclEmitOpcode(	INST_LOAD_STK,			envPtr);
		PushStringLiteral(envPtr, "");
		TclEmitOpcode(	INST_DICT_EXPAND,		envPtr);
		PushStringLiteral(envPtr, "");
		TclEmitInstInt4(INST_REVERSE, 2,		envPtr);
		TclEmitOpcode(	INST_DICT_RECOMBINE_STK,	envPtr);
	    }
	}
	PushStringLiteral(envPtr, "");
	return TCL_OK;
    }

    /*
     * OK, we have a non-trivial body. This means that the focus is on
     * generating a try-finally structure where the INST_DICT_RECOMBINE_* goes
     * in the 'finally' clause.
     *
     * Start by allocating local (unnamed, untraced) working variables.
     */

    if (dictVar == -1) {
	varNameTmp = AnonymousLocal(envPtr);
    }
    if (gotPath) {
	pathTmp = AnonymousLocal(envPtr);
    }
    keysTmp = AnonymousLocal(envPtr);

    /*
     * Issue instructions. First, the part to expand the dictionary.
     */

    if (dictVar == -1) {
	CompileWord(envPtr, varTokenPtr, interp, 1);
	Emit14Inst(		INST_STORE_SCALAR, varNameTmp,	envPtr);
    }
    tokenPtr = TokenAfter(varTokenPtr);
    if (gotPath) {
	for (i=2 ; i<parsePtr->numWords-1 ; i++) {
	    CompileWord(envPtr, tokenPtr, interp, i);
	    tokenPtr = TokenAfter(tokenPtr);
	}
	TclEmitInstInt4(	INST_LIST, parsePtr->numWords-3,envPtr);
	Emit14Inst(		INST_STORE_SCALAR, pathTmp,	envPtr);
	TclEmitOpcode(		INST_POP,			envPtr);
    }
    if (dictVar == -1) {
	TclEmitOpcode(		INST_LOAD_STK,			envPtr);
    } else {
	Emit14Inst(		INST_LOAD_SCALAR, dictVar,	envPtr);
    }
    if (gotPath) {
	Emit14Inst(		INST_LOAD_SCALAR, pathTmp,	envPtr);
    } else {
	PushStringLiteral(envPtr, "");
    }
    TclEmitOpcode(		INST_DICT_EXPAND,		envPtr);
    Emit14Inst(			INST_STORE_SCALAR, keysTmp,	envPtr);
    TclEmitOpcode(		INST_POP,			envPtr);

    /*
     * Now the body of the [dict with].
     */

    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    TclEmitInstInt4(		INST_BEGIN_CATCH4, range,	envPtr);

    ExceptionRangeStarts(envPtr, range);
    BODY(tokenPtr, parsePtr->numWords - 1);
    ExceptionRangeEnds(envPtr, range);

    /*
     * Now fold the results back into the dictionary in the OK case.
     */

    TclEmitOpcode(		INST_END_CATCH,			envPtr);
    if (dictVar == -1) {
	Emit14Inst(		INST_LOAD_SCALAR, varNameTmp,	envPtr);
    }
    if (gotPath) {
	Emit14Inst(		INST_LOAD_SCALAR, pathTmp,	envPtr);
    } else {
	PushStringLiteral(envPtr, "");
    }
    Emit14Inst(			INST_LOAD_SCALAR, keysTmp,	envPtr);
    if (dictVar == -1) {
	TclEmitOpcode(		INST_DICT_RECOMBINE_STK,	envPtr);
    } else {
	TclEmitInstInt4(	INST_DICT_RECOMBINE_IMM, dictVar, envPtr);
    }
    TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &jumpFixup);

    /*
     * Now fold the results back into the dictionary in the exception case.
     */

    TclAdjustStackDepth(-1, envPtr);
    ExceptionRangeTarget(envPtr, range, catchOffset);
    TclEmitOpcode(		INST_PUSH_RETURN_OPTIONS,	envPtr);
    TclEmitOpcode(		INST_PUSH_RESULT,		envPtr);
    TclEmitOpcode(		INST_END_CATCH,			envPtr);
    if (dictVar == -1) {
	Emit14Inst(		INST_LOAD_SCALAR, varNameTmp,	envPtr);
    }
    if (parsePtr->numWords > 3) {
	Emit14Inst(		INST_LOAD_SCALAR, pathTmp,	envPtr);
    } else {
	PushStringLiteral(envPtr, "");
    }
    Emit14Inst(			INST_LOAD_SCALAR, keysTmp,	envPtr);
    if (dictVar == -1) {
	TclEmitOpcode(		INST_DICT_RECOMBINE_STK,	envPtr);
    } else {
	TclEmitInstInt4(	INST_DICT_RECOMBINE_IMM, dictVar, envPtr);
    }
    TclEmitInvoke(envPtr,	INST_RETURN_STK);

    /*
     * Prepare for the start of the next command.
     */

    if (TclFixupForwardJumpToHere(envPtr, &jumpFixup, 127)) {
	Tcl_Panic("TclCompileDictCmd(update): bad jump distance %d",
		(int) (CurrentOffset(envPtr) - jumpFixup.codeOffset));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DupDictUpdateInfo, FreeDictUpdateInfo --
 *
 *	Functions to duplicate, release and print the aux data created for use
 *	with the INST_DICT_UPDATE_START and INST_DICT_UPDATE_END instructions.
 *
 * Results:
 *	DupDictUpdateInfo: a copy of the auxiliary data
 *	FreeDictUpdateInfo: none
 *	PrintDictUpdateInfo: none
 *	DisassembleDictUpdateInfo: none
 *
 * Side effects:
 *	DupDictUpdateInfo: allocates memory
 *	FreeDictUpdateInfo: releases memory
 *	PrintDictUpdateInfo: none
 *	DisassembleDictUpdateInfo: none
 *
 *----------------------------------------------------------------------
 */

static ClientData
DupDictUpdateInfo(
    ClientData clientData)
{
    DictUpdateInfo *dui1Ptr, *dui2Ptr;
    unsigned len;

    dui1Ptr = clientData;
    len = sizeof(DictUpdateInfo) + sizeof(int) * (dui1Ptr->length - 1);
    dui2Ptr = ckalloc(len);
    memcpy(dui2Ptr, dui1Ptr, len);
    return dui2Ptr;
}

static void
FreeDictUpdateInfo(
    ClientData clientData)
{
    ckfree(clientData);
}

static void
PrintDictUpdateInfo(
    ClientData clientData,
    Tcl_Obj *appendObj,
    ByteCode *codePtr,
    unsigned int pcOffset)
{
    DictUpdateInfo *duiPtr = clientData;
    int i;

    for (i=0 ; i<duiPtr->length ; i++) {
	if (i) {
	    Tcl_AppendToObj(appendObj, ", ", -1);
	}
	Tcl_AppendPrintfToObj(appendObj, "%%v%u", duiPtr->varIndices[i]);
    }
}

static void
DisassembleDictUpdateInfo(
    ClientData clientData,
    Tcl_Obj *dictObj,
    ByteCode *codePtr,
    unsigned int pcOffset)
{
    DictUpdateInfo *duiPtr = clientData;
    int i;
    Tcl_Obj *variables = Tcl_NewObj();

    for (i=0 ; i<duiPtr->length ; i++) {
	Tcl_ListObjAppendElement(NULL, variables,
		Tcl_NewIntObj(duiPtr->varIndices[i]));
    }
    Tcl_DictObjPut(NULL, dictObj, Tcl_NewStringObj("variables", -1),
	    variables);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileErrorCmd --
 *
 *	Procedure called to compile the "error" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "error" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileErrorCmd(
    Tcl_Interp *interp,		/* Used for context. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    /*
     * General syntax: [error message ?errorInfo? ?errorCode?]
     */

    Tcl_Token *tokenPtr;
    DefineLineInformation;	/* TIP #280 */

    if (parsePtr->numWords < 2 || parsePtr->numWords > 4) {
	return TCL_ERROR;
    }

    /*
     * Handle the message.
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 1);

    /*
     * Construct the options. Note that -code and -level are not here.
     */

    if (parsePtr->numWords == 2) {
	PushStringLiteral(envPtr, "");
    } else {
	PushStringLiteral(envPtr, "-errorinfo");
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, 2);
	if (parsePtr->numWords == 3) {
	    TclEmitInstInt4(	INST_LIST, 2,			envPtr);
	} else {
	    PushStringLiteral(envPtr, "-errorcode");
	    tokenPtr = TokenAfter(tokenPtr);
	    CompileWord(envPtr, tokenPtr, interp, 3);
	    TclEmitInstInt4(	INST_LIST, 4,			envPtr);
	}
    }

    /*
     * Issue the error via 'returnImm error 0'.
     */

    TclEmitInstInt4(		INST_RETURN_IMM, TCL_ERROR,	envPtr);
    TclEmitInt4(			0,			envPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileExprCmd --
 *
 *	Procedure called to compile the "expr" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "expr" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileExprCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *firstWordPtr;

    if (parsePtr->numWords == 1) {
	return TCL_ERROR;
    }

    /*
     * TIP #280: Use the per-word line information of the current command.
     */

    envPtr->line = envPtr->extCmdMapPtr->loc[
	    envPtr->extCmdMapPtr->nuloc-1].line[1];

    firstWordPtr = TokenAfter(parsePtr->tokenPtr);
    TclCompileExprWords(interp, firstWordPtr, parsePtr->numWords-1, envPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileForCmd --
 *
 *	Procedure called to compile the "for" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "for" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileForCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *startTokenPtr, *testTokenPtr, *nextTokenPtr, *bodyTokenPtr;
    JumpFixup jumpEvalCondFixup;
    int bodyCodeOffset, nextCodeOffset, jumpDist;
    int bodyRange, nextRange;
    DefineLineInformation;	/* TIP #280 */

    if (parsePtr->numWords != 5) {
	return TCL_ERROR;
    }

    /*
     * If the test expression requires substitutions, don't compile the for
     * command inline. E.g., the expression might cause the loop to never
     * execute or execute forever, as in "for {} "$x > 5" {incr x} {}".
     */

    startTokenPtr = TokenAfter(parsePtr->tokenPtr);
    testTokenPtr = TokenAfter(startTokenPtr);
    if (testTokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	return TCL_ERROR;
    }

    /*
     * Bail out also if the body or the next expression require substitutions
     * in order to insure correct behaviour [Bug 219166]
     */

    nextTokenPtr = TokenAfter(testTokenPtr);
    bodyTokenPtr = TokenAfter(nextTokenPtr);
    if ((nextTokenPtr->type != TCL_TOKEN_SIMPLE_WORD)
	    || (bodyTokenPtr->type != TCL_TOKEN_SIMPLE_WORD)) {
	return TCL_ERROR;
    }

    /*
     * Inline compile the initial command.
     */

    BODY(startTokenPtr, 1);
    TclEmitOpcode(INST_POP, envPtr);

    /*
     * Jump to the evaluation of the condition. This code uses the "loop
     * rotation" optimisation (which eliminates one branch from the loop).
     * "for start cond next body" produces then:
     *       start
     *       goto A
     *    B: body                : bodyCodeOffset
     *       next                : nextCodeOffset, continueOffset
     *    A: cond -> result      : testCodeOffset
     *       if (result) goto B
     */

    TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &jumpEvalCondFixup);

    /*
     * Compile the loop body.
     */

    bodyRange = TclCreateExceptRange(LOOP_EXCEPTION_RANGE, envPtr);
    bodyCodeOffset = ExceptionRangeStarts(envPtr, bodyRange);
    BODY(bodyTokenPtr, 4);
    ExceptionRangeEnds(envPtr, bodyRange);
    TclEmitOpcode(INST_POP, envPtr);

    /*
     * Compile the "next" subcommand. Note that this exception range will not
     * have a continueOffset (other than -1) connected to it; it won't trap
     * TCL_CONTINUE but rather just TCL_BREAK.
     */

    nextRange = TclCreateExceptRange(LOOP_EXCEPTION_RANGE, envPtr);
    envPtr->exceptAuxArrayPtr[nextRange].supportsContinue = 0;
    nextCodeOffset = ExceptionRangeStarts(envPtr, nextRange);
    BODY(nextTokenPtr, 3);
    ExceptionRangeEnds(envPtr, nextRange);
    TclEmitOpcode(INST_POP, envPtr);

    /*
     * Compile the test expression then emit the conditional jump that
     * terminates the for.
     */

    if (TclFixupForwardJumpToHere(envPtr, &jumpEvalCondFixup, 127)) {
	bodyCodeOffset += 3;
	nextCodeOffset += 3;
    }

    SetLineInformation(2);
    TclCompileExprWords(interp, testTokenPtr, 1, envPtr);

    jumpDist = CurrentOffset(envPtr) - bodyCodeOffset;
    if (jumpDist > 127) {
	TclEmitInstInt4(INST_JUMP_TRUE4, -jumpDist, envPtr);
    } else {
	TclEmitInstInt1(INST_JUMP_TRUE1, -jumpDist, envPtr);
    }

    /*
     * Fix the starting points of the exception ranges (may have moved due to
     * jump type modification) and set where the exceptions target.
     */

    envPtr->exceptArrayPtr[bodyRange].codeOffset = bodyCodeOffset;
    envPtr->exceptArrayPtr[bodyRange].continueOffset = nextCodeOffset;

    envPtr->exceptArrayPtr[nextRange].codeOffset = nextCodeOffset;

    ExceptionRangeTarget(envPtr, bodyRange, breakOffset);
    ExceptionRangeTarget(envPtr, nextRange, breakOffset);
    TclFinalizeLoopExceptionRange(envPtr, bodyRange);
    TclFinalizeLoopExceptionRange(envPtr, nextRange);

    /*
     * The for command's result is an empty string.
     */

    PushStringLiteral(envPtr, "");

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileForeachCmd --
 *
 *	Procedure called to compile the "foreach" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "foreach" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileForeachCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    return CompileEachloopCmd(interp, parsePtr, cmdPtr, envPtr,
	    TCL_EACH_KEEP_NONE);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileLmapCmd --
 *
 *	Procedure called to compile the "lmap" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "lmap" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileLmapCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    return CompileEachloopCmd(interp, parsePtr, cmdPtr, envPtr,
	    TCL_EACH_COLLECT);
}

/*
 *----------------------------------------------------------------------
 *
 * CompileEachloopCmd --
 *
 *	Procedure called to compile the "foreach" and "lmap" commands.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "foreach" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

static int
CompileEachloopCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr,		/* Holds resulting instructions. */
    int collect)		/* Select collecting or accumulating mode
				 * (TCL_EACH_*) */
{
    Proc *procPtr = envPtr->procPtr;
    ForeachInfo *infoPtr=NULL;	/* Points to the structure describing this
				 * foreach command. Stored in a AuxData
				 * record in the ByteCode. */

    Tcl_Token *tokenPtr, *bodyTokenPtr;
    int jumpBackOffset, infoIndex, range;
    int numWords, numLists, i, j, code = TCL_OK;
    Tcl_Obj *varListObj = NULL;
    DefineLineInformation;	/* TIP #280 */

    /*
     * If the foreach command isn't in a procedure, don't compile it inline:
     * the payoff is too small.
     */

    if (procPtr == NULL) {
	return TCL_ERROR;
    }

    numWords = parsePtr->numWords;
    if ((numWords < 4) || (numWords%2 != 0)) {
	return TCL_ERROR;
    }

    /*
     * Bail out if the body requires substitutions in order to insure correct
     * behaviour. [Bug 219166]
     */

    for (i = 0, tokenPtr = parsePtr->tokenPtr; i < numWords-1; i++) {
	tokenPtr = TokenAfter(tokenPtr);
    }
    bodyTokenPtr = tokenPtr;
    if (bodyTokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	return TCL_ERROR;
    }

    /*
     * Create and initialize the ForeachInfo and ForeachVarList data
     * structures describing this command. Then create a AuxData record
     * pointing to the ForeachInfo structure.
     */

    numLists = (numWords - 2)/2;
    infoPtr = ckalloc(sizeof(ForeachInfo)
	    + (numLists - 1) * sizeof(ForeachVarList *));
    infoPtr->numLists = 0;	/* Count this up as we go */

    /*
     * Parse each var list into sequence of var names.  Don't
     * compile the foreach inline if any var name needs substitutions or isn't
     * a scalar, or if any var list needs substitutions.
     */

    varListObj = Tcl_NewObj();
    for (i = 0, tokenPtr = parsePtr->tokenPtr;
	    i < numWords-1;
	    i++, tokenPtr = TokenAfter(tokenPtr)) {
	ForeachVarList *varListPtr;
	int numVars;

	if (i%2 != 1) {
	    continue;
	}

	/*
	 * If the variable list is empty, we can enter an infinite loop when
	 * the interpreted version would not.  Take care to ensure this does
	 * not happen.  [Bug 1671138]
	 */

	if (!TclWordKnownAtCompileTime(tokenPtr, varListObj) ||
		TCL_OK != Tcl_ListObjLength(NULL, varListObj, &numVars) ||
		numVars == 0) {
	    code = TCL_ERROR;
	    goto done;
	}

	varListPtr = ckalloc(sizeof(ForeachVarList)
		+ (numVars - 1) * sizeof(int));
	varListPtr->numVars = numVars;
	infoPtr->varLists[i/2] = varListPtr;
	infoPtr->numLists++;

	for (j = 0;  j < numVars;  j++) {
	    Tcl_Obj *varNameObj;
	    const char *bytes;
	    int numBytes, varIndex;

	    Tcl_ListObjIndex(NULL, varListObj, j, &varNameObj);
	    bytes = Tcl_GetStringFromObj(varNameObj, &numBytes);
	    varIndex = LocalScalar(bytes, numBytes, envPtr);
	    if (varIndex < 0) {
		code = TCL_ERROR;
		goto done;
	    }
	    varListPtr->varIndexes[j] = varIndex;
	}
	Tcl_SetObjLength(varListObj, 0);
    }

    /*
     * We will compile the foreach command.
     */

    infoIndex = TclCreateAuxData(infoPtr, &newForeachInfoType, envPtr);

    /*
     * Create the collecting object, unshared.
     */

    if (collect == TCL_EACH_COLLECT) {
	TclEmitInstInt4(INST_LIST, 0, envPtr);
    }

    /*
     * Evaluate each value list and leave it on stack.
     */

    for (i = 0, tokenPtr = parsePtr->tokenPtr;
	    i < numWords-1;
	    i++, tokenPtr = TokenAfter(tokenPtr)) {
	if ((i%2 == 0) && (i > 0)) {
	    CompileWord(envPtr, tokenPtr, interp, i);
	}
    }

    TclEmitInstInt4(INST_FOREACH_START, infoIndex, envPtr);

    /*
     * Inline compile the loop body.
     */

    range = TclCreateExceptRange(LOOP_EXCEPTION_RANGE, envPtr);

    ExceptionRangeStarts(envPtr, range);
    BODY(bodyTokenPtr, numWords - 1);
    ExceptionRangeEnds(envPtr, range);

    if (collect == TCL_EACH_COLLECT) {
	TclEmitOpcode(INST_LMAP_COLLECT, envPtr);
    } else {
	TclEmitOpcode(		INST_POP,			envPtr);
    }

    /*
     * Bottom of loop code: assign each loop variable and check whether
     * to terminate the loop. Set the loop's break target.
     */

    ExceptionRangeTarget(envPtr, range, continueOffset);
    TclEmitOpcode(INST_FOREACH_STEP, envPtr);
    ExceptionRangeTarget(envPtr, range, breakOffset);
    TclFinalizeLoopExceptionRange(envPtr, range);
    TclEmitOpcode(INST_FOREACH_END, envPtr);
    TclAdjustStackDepth(-(numLists+2), envPtr);

    /*
     * Set the jumpback distance from INST_FOREACH_STEP to the start of the
     * body's code. Misuse loopCtTemp for storing the jump size.
     */

    jumpBackOffset = envPtr->exceptArrayPtr[range].continueOffset -
	    envPtr->exceptArrayPtr[range].codeOffset;
    infoPtr->loopCtTemp = -jumpBackOffset;

    /*
     * The command's result is an empty string if not collecting. If
     * collecting, it is automatically left on stack after FOREACH_END.
     */

    if (collect != TCL_EACH_COLLECT) {
	PushStringLiteral(envPtr, "");
    }

    done:
    if (code == TCL_ERROR) {
	FreeForeachInfo(infoPtr);
    }
    Tcl_DecrRefCount(varListObj);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * DupForeachInfo --
 *
 *	This procedure duplicates a ForeachInfo structure created as auxiliary
 *	data during the compilation of a foreach command.
 *
 * Results:
 *	A pointer to a newly allocated copy of the existing ForeachInfo
 *	structure is returned.
 *
 * Side effects:
 *	Storage for the copied ForeachInfo record is allocated. If the
 *	original ForeachInfo structure pointed to any ForeachVarList records,
 *	these structures are also copied and pointers to them are stored in
 *	the new ForeachInfo record.
 *
 *----------------------------------------------------------------------
 */

static ClientData
DupForeachInfo(
    ClientData clientData)	/* The foreach command's compilation auxiliary
				 * data to duplicate. */
{
    register ForeachInfo *srcPtr = clientData;
    ForeachInfo *dupPtr;
    register ForeachVarList *srcListPtr, *dupListPtr;
    int numVars, i, j, numLists = srcPtr->numLists;

    dupPtr = ckalloc(sizeof(ForeachInfo)
	    + numLists * sizeof(ForeachVarList *));
    dupPtr->numLists = numLists;
    dupPtr->firstValueTemp = srcPtr->firstValueTemp;
    dupPtr->loopCtTemp = srcPtr->loopCtTemp;

    for (i = 0;  i < numLists;  i++) {
	srcListPtr = srcPtr->varLists[i];
	numVars = srcListPtr->numVars;
	dupListPtr = ckalloc(sizeof(ForeachVarList)
		+ numVars * sizeof(int));
	dupListPtr->numVars = numVars;
	for (j = 0;  j < numVars;  j++) {
	    dupListPtr->varIndexes[j] =	srcListPtr->varIndexes[j];
	}
	dupPtr->varLists[i] = dupListPtr;
    }
    return dupPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeForeachInfo --
 *
 *	Procedure to free a ForeachInfo structure created as auxiliary data
 *	during the compilation of a foreach command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Storage for the ForeachInfo structure pointed to by the ClientData
 *	argument is freed as is any ForeachVarList record pointed to by the
 *	ForeachInfo structure.
 *
 *----------------------------------------------------------------------
 */

static void
FreeForeachInfo(
    ClientData clientData)	/* The foreach command's compilation auxiliary
				 * data to free. */
{
    register ForeachInfo *infoPtr = clientData;
    register ForeachVarList *listPtr;
    int numLists = infoPtr->numLists;
    register int i;

    for (i = 0;  i < numLists;  i++) {
	listPtr = infoPtr->varLists[i];
	ckfree(listPtr);
    }
    ckfree(infoPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintForeachInfo, DisassembleForeachInfo --
 *
 *	Functions to write a human-readable or script-readablerepresentation
 *	of a ForeachInfo structure to a Tcl_Obj for debugging.
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
PrintForeachInfo(
    ClientData clientData,
    Tcl_Obj *appendObj,
    ByteCode *codePtr,
    unsigned int pcOffset)
{
    register ForeachInfo *infoPtr = clientData;
    register ForeachVarList *varsPtr;
    int i, j;

    Tcl_AppendToObj(appendObj, "data=[", -1);

    for (i=0 ; i<infoPtr->numLists ; i++) {
	if (i) {
	    Tcl_AppendToObj(appendObj, ", ", -1);
	}
	Tcl_AppendPrintfToObj(appendObj, "%%v%u",
		(unsigned) (infoPtr->firstValueTemp + i));
    }
    Tcl_AppendPrintfToObj(appendObj, "], loop=%%v%u",
	    (unsigned) infoPtr->loopCtTemp);
    for (i=0 ; i<infoPtr->numLists ; i++) {
	if (i) {
	    Tcl_AppendToObj(appendObj, ",", -1);
	}
	Tcl_AppendPrintfToObj(appendObj, "\n\t\t it%%v%u\t[",
		(unsigned) (infoPtr->firstValueTemp + i));
	varsPtr = infoPtr->varLists[i];
	for (j=0 ; j<varsPtr->numVars ; j++) {
	    if (j) {
		Tcl_AppendToObj(appendObj, ", ", -1);
	    }
	    Tcl_AppendPrintfToObj(appendObj, "%%v%u",
		    (unsigned) varsPtr->varIndexes[j]);
	}
	Tcl_AppendToObj(appendObj, "]", -1);
    }
}

static void
PrintNewForeachInfo(
    ClientData clientData,
    Tcl_Obj *appendObj,
    ByteCode *codePtr,
    unsigned int pcOffset)
{
    register ForeachInfo *infoPtr = clientData;
    register ForeachVarList *varsPtr;
    int i, j;

    Tcl_AppendPrintfToObj(appendObj, "jumpOffset=%+d, vars=",
	    infoPtr->loopCtTemp);
    for (i=0 ; i<infoPtr->numLists ; i++) {
	if (i) {
	    Tcl_AppendToObj(appendObj, ",", -1);
	}
	Tcl_AppendToObj(appendObj, "[", -1);
	varsPtr = infoPtr->varLists[i];
	for (j=0 ; j<varsPtr->numVars ; j++) {
	    if (j) {
		Tcl_AppendToObj(appendObj, ",", -1);
	    }
	    Tcl_AppendPrintfToObj(appendObj, "%%v%u",
		    (unsigned) varsPtr->varIndexes[j]);
	}
	Tcl_AppendToObj(appendObj, "]", -1);
    }
}

static void
DisassembleForeachInfo(
    ClientData clientData,
    Tcl_Obj *dictObj,
    ByteCode *codePtr,
    unsigned int pcOffset)
{
    register ForeachInfo *infoPtr = clientData;
    register ForeachVarList *varsPtr;
    int i, j;
    Tcl_Obj *objPtr, *innerPtr;

    /*
     * Data stores.
     */

    objPtr = Tcl_NewObj();
    for (i=0 ; i<infoPtr->numLists ; i++) {
	Tcl_ListObjAppendElement(NULL, objPtr,
		Tcl_NewIntObj(infoPtr->firstValueTemp + i));
    }
    Tcl_DictObjPut(NULL, dictObj, Tcl_NewStringObj("data", -1), objPtr);

    /*
     * Loop counter.
     */

    Tcl_DictObjPut(NULL, dictObj, Tcl_NewStringObj("loop", -1),
	   Tcl_NewIntObj(infoPtr->loopCtTemp));

    /*
     * Assignment targets.
     */

    objPtr = Tcl_NewObj();
    for (i=0 ; i<infoPtr->numLists ; i++) {
	innerPtr = Tcl_NewObj();
	varsPtr = infoPtr->varLists[i];
	for (j=0 ; j<varsPtr->numVars ; j++) {
	    Tcl_ListObjAppendElement(NULL, innerPtr,
		    Tcl_NewIntObj(varsPtr->varIndexes[j]));
	}
	Tcl_ListObjAppendElement(NULL, objPtr, innerPtr);
    }
    Tcl_DictObjPut(NULL, dictObj, Tcl_NewStringObj("assign", -1), objPtr);
}

static void
DisassembleNewForeachInfo(
    ClientData clientData,
    Tcl_Obj *dictObj,
    ByteCode *codePtr,
    unsigned int pcOffset)
{
    register ForeachInfo *infoPtr = clientData;
    register ForeachVarList *varsPtr;
    int i, j;
    Tcl_Obj *objPtr, *innerPtr;

    /*
     * Jump offset.
     */

    Tcl_DictObjPut(NULL, dictObj, Tcl_NewStringObj("jumpOffset", -1),
	   Tcl_NewIntObj(infoPtr->loopCtTemp));

    /*
     * Assignment targets.
     */

    objPtr = Tcl_NewObj();
    for (i=0 ; i<infoPtr->numLists ; i++) {
	innerPtr = Tcl_NewObj();
	varsPtr = infoPtr->varLists[i];
	for (j=0 ; j<varsPtr->numVars ; j++) {
	    Tcl_ListObjAppendElement(NULL, innerPtr,
		    Tcl_NewIntObj(varsPtr->varIndexes[j]));
	}
	Tcl_ListObjAppendElement(NULL, objPtr, innerPtr);
    }
    Tcl_DictObjPut(NULL, dictObj, Tcl_NewStringObj("assign", -1), objPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileFormatCmd --
 *
 *	Procedure called to compile the "format" command. Handles cases that
 *	can be done as constants or simple string concatenation only.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "format" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileFormatCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr = parsePtr->tokenPtr;
    Tcl_Obj **objv, *formatObj, *tmpObj;
    char *bytes, *start;
    int i, j, len;

    /*
     * Don't handle any guaranteed-error cases.
     */

    if (parsePtr->numWords < 2) {
	return TCL_ERROR;
    }

    /*
     * Check if the argument words are all compile-time-known literals; that's
     * a case we can handle by compiling to a constant.
     */

    formatObj = Tcl_NewObj();
    Tcl_IncrRefCount(formatObj);
    tokenPtr = TokenAfter(tokenPtr);
    if (!TclWordKnownAtCompileTime(tokenPtr, formatObj)) {
	Tcl_DecrRefCount(formatObj);
	return TCL_ERROR;
    }

    objv = ckalloc((parsePtr->numWords-2) * sizeof(Tcl_Obj *));
    for (i=0 ; i+2 < parsePtr->numWords ; i++) {
	tokenPtr = TokenAfter(tokenPtr);
	objv[i] = Tcl_NewObj();
	Tcl_IncrRefCount(objv[i]);
	if (!TclWordKnownAtCompileTime(tokenPtr, objv[i])) {
	    goto checkForStringConcatCase;
	}
    }

    /*
     * Everything is a literal, so the result is constant too (or an error if
     * the format is broken). Do the format now.
     */

    tmpObj = Tcl_Format(interp, Tcl_GetString(formatObj),
	    parsePtr->numWords-2, objv);
    for (; --i>=0 ;) {
	Tcl_DecrRefCount(objv[i]);
    }
    ckfree(objv);
    Tcl_DecrRefCount(formatObj);
    if (tmpObj == NULL) {
	TclCompileSyntaxError(interp, envPtr);
	return TCL_OK;
    }

    /*
     * Not an error, always a constant result, so just push the result as a
     * literal. Job done.
     */

    bytes = Tcl_GetStringFromObj(tmpObj, &len);
    PushLiteral(envPtr, bytes, len);
    Tcl_DecrRefCount(tmpObj);
    return TCL_OK;

  checkForStringConcatCase:
    /*
     * See if we can generate a sequence of things to concatenate. This
     * requires that all the % sequences be %s or %%, as everything else is
     * sufficiently complex that we don't bother.
     *
     * First, get the state of the system relatively sensible (cleaning up
     * after our attempt to spot a literal).
     */

    for (; i>=0 ; i--) {
	Tcl_DecrRefCount(objv[i]);
    }
    ckfree(objv);
    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    tokenPtr = TokenAfter(tokenPtr);
    i = 0;

    /*
     * Now scan through and check for non-%s and non-%% substitutions.
     */

    for (bytes = Tcl_GetString(formatObj) ; *bytes ; bytes++) {
	if (*bytes == '%') {
	    bytes++;
	    if (*bytes == 's') {
		i++;
		continue;
	    } else if (*bytes == '%') {
		continue;
	    }
	    Tcl_DecrRefCount(formatObj);
	    return TCL_ERROR;
	}
    }

    /*
     * Check if the number of things to concatenate will fit in a byte.
     */

    if (i+2 != parsePtr->numWords || i > 125) {
	Tcl_DecrRefCount(formatObj);
	return TCL_ERROR;
    }

    /*
     * Generate the pushes of the things to concatenate, a sequence of
     * literals and compiled tokens (of which at least one is non-literal or
     * we'd have the case in the first half of this function) which we will
     * concatenate.
     */

    i = 0;			/* The count of things to concat. */
    j = 2;			/* The index into the argument tokens, for
				 * TIP#280 handling. */
    start = Tcl_GetString(formatObj);
				/* The start of the currently-scanned literal
				 * in the format string. */
    tmpObj = Tcl_NewObj();	/* The buffer used to accumulate the literal
				 * being built. */
    for (bytes = start ; *bytes ; bytes++) {
	if (*bytes == '%') {
	    Tcl_AppendToObj(tmpObj, start, bytes - start);
	    if (*++bytes == '%') {
		Tcl_AppendToObj(tmpObj, "%", 1);
	    } else {
		char *b = Tcl_GetStringFromObj(tmpObj, &len);

		/*
		 * If there is a non-empty literal from the format string,
		 * push it and reset.
		 */

		if (len > 0) {
		    PushLiteral(envPtr, b, len);
		    Tcl_DecrRefCount(tmpObj);
		    tmpObj = Tcl_NewObj();
		    i++;
		}

		/*
		 * Push the code to produce the string that would be
		 * substituted with %s, except we'll be concatenating
		 * directly.
		 */

		CompileWord(envPtr, tokenPtr, interp, j);
		tokenPtr = TokenAfter(tokenPtr);
		j++;
		i++;
	    }
	    start = bytes + 1;
	}
    }

    /*
     * Handle the case of a trailing literal.
     */

    Tcl_AppendToObj(tmpObj, start, bytes - start);
    bytes = Tcl_GetStringFromObj(tmpObj, &len);
    if (len > 0) {
	PushLiteral(envPtr, bytes, len);
	i++;
    }
    Tcl_DecrRefCount(tmpObj);
    Tcl_DecrRefCount(formatObj);

    if (i > 1) {
	/*
	 * Do the concatenation, which produces the result.
	 */

	TclEmitInstInt1(INST_STR_CONCAT1, i, envPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclLocalScalarFromToken --
 *
 *	Get the index into the table of compiled locals that corresponds
 *	to a local scalar variable name.
 *
 * Results:
 * 	Returns the non-negative integer index value into the table of
 * 	compiled locals corresponding to a local scalar variable name.
 * 	If the arguments passed in do not identify a local scalar variable
 * 	then return -1.
 *
 * Side effects:
 *	May add an entery into the table of compiled locals.
 *
 *----------------------------------------------------------------------
 */

int
TclLocalScalarFromToken(
    Tcl_Token *tokenPtr,
    CompileEnv *envPtr)
{
    int isScalar, index;

    TclPushVarName(NULL, tokenPtr, envPtr, TCL_NO_ELEMENT, &index, &isScalar);
    if (!isScalar) {
	index = -1;
    }
    return index;
}

int
TclLocalScalar(
    const char *bytes,
    int numBytes,
    CompileEnv *envPtr)
{
    Tcl_Token token[2] =        {{TCL_TOKEN_SIMPLE_WORD, NULL, 0, 1},
                                 {TCL_TOKEN_TEXT, NULL, 0, 0}};

    token[1].start = bytes;
    token[1].size = numBytes;
    return TclLocalScalarFromToken(token, envPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPushVarName --
 *
 *	Procedure used in the compiling where pushing a variable name is
 *	necessary (append, lappend, set).
 *
 * Results:
 *	The values written to *localIndexPtr and *isScalarPtr signal to
 *	the caller what the instructions emitted by this routine will do:
 *
 *	*isScalarPtr	(*localIndexPtr < 0)
 *	1		1	Push the varname on the stack. (Stack +1)
 *	1		0	*localIndexPtr is the index of the compiled
 *				local for this varname.  No instructions
 *				emitted.	(Stack +0)
 *	0		1	Push part1 and part2 names of array element
 *				on the stack.	(Stack +2)
 *	0		0	*localIndexPtr is the index of the compiled
 *				local for this array.  Element name is pushed
 *				on the stack.	(Stack +1)
 *
 * Side effects:
 *	Instructions are added to envPtr.
 *
 *----------------------------------------------------------------------
 */

void
TclPushVarName(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Token *varTokenPtr,	/* Points to a variable token. */
    CompileEnv *envPtr,		/* Holds resulting instructions. */
    int flags,			/* TCL_NO_LARGE_INDEX | TCL_NO_ELEMENT. */
    int *localIndexPtr,		/* Must not be NULL. */
    int *isScalarPtr)		/* Must not be NULL. */
{
    register const char *p;
    const char *name, *elName;
    register int i, n;
    Tcl_Token *elemTokenPtr = NULL;
    int nameChars, elNameChars, simpleVarName, localIndex;
    int elemTokenCount = 0, allocedTokens = 0, removedParen = 0;

    /*
     * Decide if we can use a frame slot for the var/array name or if we need
     * to emit code to compute and push the name at runtime. We use a frame
     * slot (entry in the array of local vars) if we are compiling a procedure
     * body and if the name is simple text that does not include namespace
     * qualifiers.
     */

    simpleVarName = 0;
    name = elName = NULL;
    nameChars = elNameChars = 0;
    localIndex = -1;

    if (varTokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	/*
	 * A simple variable name. Divide it up into "name" and "elName"
	 * strings. If it is not a local variable, look it up at runtime.
	 */

	simpleVarName = 1;

	name = varTokenPtr[1].start;
	nameChars = varTokenPtr[1].size;
	if (name[nameChars-1] == ')') {
	    /*
	     * last char is ')' => potential array reference.
	     */

	    for (i=0,p=name ; i<nameChars ; i++,p++) {
		if (*p == '(') {
		    elName = p + 1;
		    elNameChars = nameChars - i - 2;
		    nameChars = i;
		    break;
		}
	    }

	    if (!(flags & TCL_NO_ELEMENT) && (elName != NULL) && elNameChars) {
		/*
		 * An array element, the element name is a simple string:
		 * assemble the corresponding token.
		 */

		elemTokenPtr = TclStackAlloc(interp, sizeof(Tcl_Token));
		allocedTokens = 1;
		elemTokenPtr->type = TCL_TOKEN_TEXT;
		elemTokenPtr->start = elName;
		elemTokenPtr->size = elNameChars;
		elemTokenPtr->numComponents = 0;
		elemTokenCount = 1;
	    }
	}
    } else if (interp && ((n = varTokenPtr->numComponents) > 1)
	    && (varTokenPtr[1].type == TCL_TOKEN_TEXT)
	    && (varTokenPtr[n].type == TCL_TOKEN_TEXT)
	    && (varTokenPtr[n].start[varTokenPtr[n].size - 1] == ')')) {
	/*
	 * Check for parentheses inside first token.
	 */

	simpleVarName = 0;
	for (i = 0, p = varTokenPtr[1].start;
		i < varTokenPtr[1].size; i++, p++) {
	    if (*p == '(') {
		simpleVarName = 1;
		break;
	    }
	}
	if (simpleVarName) {
	    int remainingChars;

	    /*
	     * Check the last token: if it is just ')', do not count it.
	     * Otherwise, remove the ')' and flag so that it is restored at
	     * the end.
	     */

	    if (varTokenPtr[n].size == 1) {
		n--;
	    } else {
		varTokenPtr[n].size--;
		removedParen = n;
	    }

	    name = varTokenPtr[1].start;
	    nameChars = p - varTokenPtr[1].start;
	    elName = p + 1;
	    remainingChars = (varTokenPtr[2].start - p) - 1;
	    elNameChars = (varTokenPtr[n].start-p) + varTokenPtr[n].size - 1;

	    if (!(flags & TCL_NO_ELEMENT)) {
	      if (remainingChars) {
		/*
		 * Make a first token with the extra characters in the first
		 * token.
		 */

		elemTokenPtr = TclStackAlloc(interp, n * sizeof(Tcl_Token));
		allocedTokens = 1;
		elemTokenPtr->type = TCL_TOKEN_TEXT;
		elemTokenPtr->start = elName;
		elemTokenPtr->size = remainingChars;
		elemTokenPtr->numComponents = 0;
		elemTokenCount = n;

		/*
		 * Copy the remaining tokens.
		 */

		memcpy(elemTokenPtr+1, varTokenPtr+2,
			(n-1) * sizeof(Tcl_Token));
	      } else {
		/*
		 * Use the already available tokens.
		 */

		elemTokenPtr = &varTokenPtr[2];
		elemTokenCount = n - 1;
	      }
	    }
	}
    }

    if (simpleVarName) {
	/*
	 * See whether name has any namespace separators (::'s).
	 */

	int hasNsQualifiers = 0;

	for (i = 0, p = name;  i < nameChars;  i++, p++) {
	    if ((*p == ':') && ((i+1) < nameChars) && (*(p+1) == ':')) {
		hasNsQualifiers = 1;
		break;
	    }
	}

	/*
	 * Look up the var name's index in the array of local vars in the proc
	 * frame. If retrieving the var's value and it doesn't already exist,
	 * push its name and look it up at runtime.
	 */

	if (!hasNsQualifiers) {
	    localIndex = TclFindCompiledLocal(name, nameChars, 1, envPtr);
	    if ((flags & TCL_NO_LARGE_INDEX) && (localIndex > 255)) {
		/*
		 * We'll push the name.
		 */

		localIndex = -1;
	    }
	}
	if (interp && localIndex < 0) {
	    PushLiteral(envPtr, name, nameChars);
	}

	/*
	 * Compile the element script, if any, and only if not inhibited. [Bug
	 * 3600328]
	 */

	if (elName != NULL && !(flags & TCL_NO_ELEMENT)) {
	    if (elNameChars) {
		TclCompileTokens(interp, elemTokenPtr, elemTokenCount,
			envPtr);
	    } else {
		PushStringLiteral(envPtr, "");
	    }
	}
    } else if (interp) {
	/*
	 * The var name isn't simple: compile and push it.
	 */

	CompileTokens(envPtr, varTokenPtr, interp);
    }

    if (removedParen) {
	varTokenPtr[removedParen].size++;
    }
    if (allocedTokens) {
	TclStackFree(interp, elemTokenPtr);
    }
    *localIndexPtr = localIndex;
    *isScalarPtr = (elName == NULL);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
