/*
 * tclCompCmdsSZ.c --
 *
 *	This file contains compilation procedures that compile various Tcl
 *	commands (beginning with the letters 's' through 'z', except for
 *	[upvar] and [variable]) into a sequence of instructions ("bytecodes").
 *	Also includes the operator command compilers.
 *
 * Copyright (c) 1997-1998 Sun Microsystems, Inc.
 * Copyright (c) 2001 by Kevin B. Kenny.  All rights reserved.
 * Copyright (c) 2002 ActiveState Corporation.
 * Copyright (c) 2004-2010 by Donal K. Fellows.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclCompile.h"
#include "tclStringTrim.h"

/*
 * Prototypes for procedures defined later in this file:
 */

static ClientData	DupJumptableInfo(ClientData clientData);
static void		FreeJumptableInfo(ClientData clientData);
static void		PrintJumptableInfo(ClientData clientData,
			    Tcl_Obj *appendObj, ByteCode *codePtr,
			    unsigned int pcOffset);
static void		DisassembleJumptableInfo(ClientData clientData,
			    Tcl_Obj *dictObj, ByteCode *codePtr,
			    unsigned int pcOffset);
static int		CompileAssociativeBinaryOpCmd(Tcl_Interp *interp,
			    Tcl_Parse *parsePtr, const char *identity,
			    int instruction, CompileEnv *envPtr);
static int		CompileComparisonOpCmd(Tcl_Interp *interp,
			    Tcl_Parse *parsePtr, int instruction,
			    CompileEnv *envPtr);
static int		CompileStrictlyBinaryOpCmd(Tcl_Interp *interp,
			    Tcl_Parse *parsePtr, int instruction,
			    CompileEnv *envPtr);
static int		CompileUnaryOpCmd(Tcl_Interp *interp,
			    Tcl_Parse *parsePtr, int instruction,
			    CompileEnv *envPtr);
static void		IssueSwitchChainedTests(Tcl_Interp *interp,
			    CompileEnv *envPtr, int mode, int noCase,
			    int valueIndex, int numWords,
			    Tcl_Token **bodyToken, int *bodyLines,
			    int **bodyNext);
static void		IssueSwitchJumpTable(Tcl_Interp *interp,
			    CompileEnv *envPtr, int valueIndex,
			    int numWords, Tcl_Token **bodyToken,
			    int *bodyLines, int **bodyContLines);
static int		IssueTryClausesInstructions(Tcl_Interp *interp,
			    CompileEnv *envPtr, Tcl_Token *bodyToken,
			    int numHandlers, int *matchCodes,
			    Tcl_Obj **matchClauses, int *resultVarIndices,
			    int *optionVarIndices, Tcl_Token **handlerTokens);
static int		IssueTryClausesFinallyInstructions(Tcl_Interp *interp,
			    CompileEnv *envPtr, Tcl_Token *bodyToken,
			    int numHandlers, int *matchCodes,
			    Tcl_Obj **matchClauses, int *resultVarIndices,
			    int *optionVarIndices, Tcl_Token **handlerTokens,
			    Tcl_Token *finallyToken);
static int		IssueTryFinallyInstructions(Tcl_Interp *interp,
			    CompileEnv *envPtr, Tcl_Token *bodyToken,
			    Tcl_Token *finallyToken);

/*
 * The structures below define the AuxData types defined in this file.
 */

const AuxDataType tclJumptableInfoType = {
    "JumptableInfo",		/* name */
    DupJumptableInfo,		/* dupProc */
    FreeJumptableInfo,		/* freeProc */
    PrintJumptableInfo,		/* printProc */
    DisassembleJumptableInfo	/* disassembleProc */
};

/*
 * Shorthand macros for instruction issuing.
 */

#define OP(name)	TclEmitOpcode(INST_##name, envPtr)
#define OP1(name,val)	TclEmitInstInt1(INST_##name,(val),envPtr)
#define OP4(name,val)	TclEmitInstInt4(INST_##name,(val),envPtr)
#define OP14(name,val1,val2) \
    TclEmitInstInt1(INST_##name,(val1),envPtr);TclEmitInt4((val2),envPtr)
#define OP44(name,val1,val2) \
    TclEmitInstInt4(INST_##name,(val1),envPtr);TclEmitInt4((val2),envPtr)
#define PUSH(str) \
    PushStringLiteral(envPtr, str)
#define JUMP4(name,var) \
    (var) = CurrentOffset(envPtr);TclEmitInstInt4(INST_##name##4,0,envPtr)
#define FIXJUMP4(var) \
    TclStoreInt4AtPtr(CurrentOffset(envPtr)-(var),envPtr->codeStart+(var)+1)
#define JUMP1(name,var) \
    (var) = CurrentOffset(envPtr);TclEmitInstInt1(INST_##name##1,0,envPtr)
#define FIXJUMP1(var) \
    TclStoreInt1AtPtr(CurrentOffset(envPtr)-(var),envPtr->codeStart+(var)+1)
#define LOAD(idx) \
    if ((idx)<256) {OP1(LOAD_SCALAR1,(idx));} else {OP4(LOAD_SCALAR4,(idx));}
#define STORE(idx) \
    if ((idx)<256) {OP1(STORE_SCALAR1,(idx));} else {OP4(STORE_SCALAR4,(idx));}
#define INVOKE(name) \
    TclEmitInvoke(envPtr,INST_##name)


/*
 *----------------------------------------------------------------------
 *
 * TclCompileSetCmd --
 *
 *	Procedure called to compile the "set" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "set" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileSetCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *varTokenPtr, *valueTokenPtr;
    int isAssignment, isScalar, localIndex, numWords;
    DefineLineInformation;	/* TIP #280 */

    numWords = parsePtr->numWords;
    if ((numWords != 2) && (numWords != 3)) {
	return TCL_ERROR;
    }
    isAssignment = (numWords == 3);

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
     * If we are doing an assignment, push the new value.
     */

    if (isAssignment) {
	valueTokenPtr = TokenAfter(varTokenPtr);
	CompileWord(envPtr, valueTokenPtr, interp, 2);
    }

    /*
     * Emit instructions to set/get the variable.
     */

	if (isScalar) {
	    if (localIndex < 0) {
		TclEmitOpcode((isAssignment?
			INST_STORE_STK : INST_LOAD_STK), envPtr);
	    } else if (localIndex <= 255) {
		TclEmitInstInt1((isAssignment?
			INST_STORE_SCALAR1 : INST_LOAD_SCALAR1),
			localIndex, envPtr);
	    } else {
		TclEmitInstInt4((isAssignment?
			INST_STORE_SCALAR4 : INST_LOAD_SCALAR4),
			localIndex, envPtr);
	    }
	} else {
	    if (localIndex < 0) {
		TclEmitOpcode((isAssignment?
			INST_STORE_ARRAY_STK : INST_LOAD_ARRAY_STK), envPtr);
	    } else if (localIndex <= 255) {
		TclEmitInstInt1((isAssignment?
			INST_STORE_ARRAY1 : INST_LOAD_ARRAY1),
			localIndex, envPtr);
	    } else {
		TclEmitInstInt4((isAssignment?
			INST_STORE_ARRAY4 : INST_LOAD_ARRAY4),
			localIndex, envPtr);
	    }
	}

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileString*Cmd --
 *
 *	Procedures called to compile various subcommands of the "string"
 *	command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "string" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileStringCatCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    int i, numWords = parsePtr->numWords, numArgs;
    Tcl_Token *wordTokenPtr;
    Tcl_Obj *obj, *folded;
    DefineLineInformation;	/* TIP #280 */

    /* Trivial case, no arg */

    if (numWords<2) {
	PushStringLiteral(envPtr, "");
	return TCL_OK;
    }

    /* General case: issue CONCAT1's (by chunks of 254 if needed), folding
       contiguous constants along the way */

    numArgs = 0;
    folded = NULL;
    wordTokenPtr = TokenAfter(parsePtr->tokenPtr);
    for (i = 1; i < numWords; i++) {
	obj = Tcl_NewObj();
	if (TclWordKnownAtCompileTime(wordTokenPtr, obj)) {
	    if (folded) {
		Tcl_AppendObjToObj(folded, obj);
		Tcl_DecrRefCount(obj);
	    } else {
		folded = obj;
	    }
	} else {
	    Tcl_DecrRefCount(obj);
	    if (folded) {
		int len;
		const char *bytes = Tcl_GetStringFromObj(folded, &len);

		PushLiteral(envPtr, bytes, len);
		Tcl_DecrRefCount(folded);
		folded = NULL;
		numArgs ++;
	    }
	    CompileWord(envPtr, wordTokenPtr, interp, i);
	    numArgs ++;
	    if (numArgs >= 254) { /* 254 to take care of the possible +1 of "folded" above */
		TclEmitInstInt1(INST_STR_CONCAT1, numArgs, envPtr);
		numArgs = 1;	/* concat pushes 1 obj, the result */
	    }
	}
	wordTokenPtr = TokenAfter(wordTokenPtr);
    }
    if (folded) {
	int len;
	const char *bytes = Tcl_GetStringFromObj(folded, &len);

	PushLiteral(envPtr, bytes, len);
	Tcl_DecrRefCount(folded);
	folded = NULL;
	numArgs ++;
    }
    if (numArgs > 1) {
	TclEmitInstInt1(INST_STR_CONCAT1, numArgs, envPtr);
    }

    return TCL_OK;
}

int
TclCompileStringCmpCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    /*
     * We don't support any flags; the bytecode isn't that sophisticated.
     */

    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    /*
     * Push the two operands onto the stack and then the test.
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 1);
    tokenPtr = TokenAfter(tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 2);
    TclEmitOpcode(INST_STR_CMP, envPtr);
    return TCL_OK;
}

int
TclCompileStringEqualCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    /*
     * We don't support any flags; the bytecode isn't that sophisticated.
     */

    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    /*
     * Push the two operands onto the stack and then the test.
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 1);
    tokenPtr = TokenAfter(tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 2);
    TclEmitOpcode(INST_STR_EQ, envPtr);
    return TCL_OK;
}

int
TclCompileStringFirstCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    /*
     * We don't support any flags; the bytecode isn't that sophisticated.
     */

    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    /*
     * Push the two operands onto the stack and then the test.
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 1);
    tokenPtr = TokenAfter(tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 2);
    OP(STR_FIND);
    return TCL_OK;
}

int
TclCompileStringLastCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    /*
     * We don't support any flags; the bytecode isn't that sophisticated.
     */

    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    /*
     * Push the two operands onto the stack and then the test.
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 1);
    tokenPtr = TokenAfter(tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 2);
    OP(STR_FIND_LAST);
    return TCL_OK;
}

int
TclCompileStringIndexCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    /*
     * Push the two operands onto the stack and then the index operation.
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 1);
    tokenPtr = TokenAfter(tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 2);
    TclEmitOpcode(INST_STR_INDEX, envPtr);
    return TCL_OK;
}

int
TclCompileStringIsCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr = TokenAfter(parsePtr->tokenPtr);
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
    int t, range, allowEmpty = 0, end;
    InstStringClassType strClassType;
    Tcl_Obj *isClass;

    if (parsePtr->numWords < 3 || parsePtr->numWords > 6) {
	return TCL_ERROR;
    }
    isClass = Tcl_NewObj();
    if (!TclWordKnownAtCompileTime(tokenPtr, isClass)) {
	Tcl_DecrRefCount(isClass);
	return TCL_ERROR;
    } else if (Tcl_GetIndexFromObj(interp, isClass, isClasses, "class", 0,
	    &t) != TCL_OK) {
	Tcl_DecrRefCount(isClass);
	TclCompileSyntaxError(interp, envPtr);
	return TCL_OK;
    }
    Tcl_DecrRefCount(isClass);

#define GotLiteral(tokenPtr, word) \
    ((tokenPtr)->type == TCL_TOKEN_SIMPLE_WORD &&			\
     (tokenPtr)[1].size > 1 &&						\
     (tokenPtr)[1].start[0] == word[0] &&				\
     strncmp((tokenPtr)[1].start, (word), (tokenPtr)[1].size) == 0)

    /*
     * Cannot handle the -failindex option at all, and that's the only legal
     * way to have more than 4 arguments.
     */

    if (parsePtr->numWords != 3 && parsePtr->numWords != 4) {
	return TCL_ERROR;
    }

    tokenPtr = TokenAfter(tokenPtr);
    if (parsePtr->numWords == 3) {
	allowEmpty = 1;
    } else {
	if (!GotLiteral(tokenPtr, "-strict")) {
	    return TCL_ERROR;
	}
	tokenPtr = TokenAfter(tokenPtr);
    }
#undef GotLiteral

    /*
     * Compile the code. There are several main classes of check here.
     *	1. Character classes
     *	2. Booleans
     *	3. Integers
     *	4. Floats
     *	5. Lists
     */

    CompileWord(envPtr, tokenPtr, interp, parsePtr->numWords-1);

    switch ((enum isClasses) t) {
    case STR_IS_ALNUM:
	strClassType = STR_CLASS_ALNUM;
	goto compileStrClass;
    case STR_IS_ALPHA:
	strClassType = STR_CLASS_ALPHA;
	goto compileStrClass;
    case STR_IS_ASCII:
	strClassType = STR_CLASS_ASCII;
	goto compileStrClass;
    case STR_IS_CONTROL:
	strClassType = STR_CLASS_CONTROL;
	goto compileStrClass;
    case STR_IS_DIGIT:
	strClassType = STR_CLASS_DIGIT;
	goto compileStrClass;
    case STR_IS_GRAPH:
	strClassType = STR_CLASS_GRAPH;
	goto compileStrClass;
    case STR_IS_LOWER:
	strClassType = STR_CLASS_LOWER;
	goto compileStrClass;
    case STR_IS_PRINT:
	strClassType = STR_CLASS_PRINT;
	goto compileStrClass;
    case STR_IS_PUNCT:
	strClassType = STR_CLASS_PUNCT;
	goto compileStrClass;
    case STR_IS_SPACE:
	strClassType = STR_CLASS_SPACE;
	goto compileStrClass;
    case STR_IS_UPPER:
	strClassType = STR_CLASS_UPPER;
	goto compileStrClass;
    case STR_IS_WORD:
	strClassType = STR_CLASS_WORD;
	goto compileStrClass;
    case STR_IS_XDIGIT:
	strClassType = STR_CLASS_XDIGIT;
    compileStrClass:
	if (allowEmpty) {
	    OP1(	STR_CLASS, strClassType);
	} else {
	    int over, over2;

	    OP(		DUP);
	    OP1(	STR_CLASS, strClassType);
	    JUMP1(	JUMP_TRUE, over);
	    OP(		POP);
	    PUSH(	"0");
	    JUMP1(	JUMP, over2);
	    FIXJUMP1(over);
	    PUSH(	"");
	    OP(		STR_NEQ);
	    FIXJUMP1(over2);
	}
	return TCL_OK;

    case STR_IS_BOOL:
    case STR_IS_FALSE:
    case STR_IS_TRUE:
	OP(		TRY_CVT_TO_BOOLEAN);
	switch (t) {
	    int over, over2;

	case STR_IS_BOOL:
	    if (allowEmpty) {
		JUMP1(	JUMP_TRUE, over);
		PUSH(	"");
		OP(	STR_EQ);
		JUMP1(	JUMP, over2);
		FIXJUMP1(over);
		OP(	POP);
		PUSH(	"1");
		FIXJUMP1(over2);
	    } else {
		OP4(	REVERSE, 2);
		OP(	POP);
	    }
	    return TCL_OK;
	case STR_IS_TRUE:
	    JUMP1(	JUMP_TRUE, over);
	    if (allowEmpty) {
		PUSH(	"");
		OP(	STR_EQ);
	    } else {
		OP(	POP);
		PUSH(	"0");
	    }
	    FIXJUMP1(	over);
	    OP(		LNOT);
	    OP(		LNOT);
	    return TCL_OK;
	case STR_IS_FALSE:
	    JUMP1(	JUMP_TRUE, over);
	    if (allowEmpty) {
		PUSH(	"");
		OP(	STR_NEQ);
	    } else {
		OP(	POP);
		PUSH(	"1");
	    }
	    FIXJUMP1(	over);
	    OP(		LNOT);
	    return TCL_OK;
	}

    case STR_IS_DOUBLE: {
	int satisfied, isEmpty;

	if (allowEmpty) {
	    OP(		DUP);
	    PUSH(	"");
	    OP(		STR_EQ);
	    JUMP1(	JUMP_TRUE, isEmpty);
	    OP(		NUM_TYPE);
	    JUMP1(	JUMP_TRUE, satisfied);
	    PUSH(	"0");
	    JUMP1(	JUMP, end);
	    FIXJUMP1(	isEmpty);
	    OP(		POP);
	    FIXJUMP1(	satisfied);
	} else {
	    OP(		NUM_TYPE);
	    JUMP1(	JUMP_TRUE, satisfied);
	    PUSH(	"0");
	    JUMP1(	JUMP, end);
	    TclAdjustStackDepth(-1, envPtr);
	    FIXJUMP1(	satisfied);
	}
	PUSH(		"1");
	FIXJUMP1(	end);
	return TCL_OK;
    }

    case STR_IS_INT:
    case STR_IS_WIDE:
    case STR_IS_ENTIER:
	if (allowEmpty) {
	    int testNumType;

	    OP(		DUP);
	    OP(		NUM_TYPE);
	    OP(		DUP);
	    JUMP1(	JUMP_TRUE, testNumType);
	    OP(		POP);
	    PUSH(	"");
	    OP(		STR_EQ);
	    JUMP1(	JUMP, end);
	    TclAdjustStackDepth(1, envPtr);
	    FIXJUMP1(	testNumType);
	    OP4(	REVERSE, 2);
	    OP(		POP);
	} else {
	    OP(		NUM_TYPE);
	    OP(		DUP);
	    JUMP1(	JUMP_FALSE, end);
	}

	switch (t) {
	case STR_IS_INT:
	    PUSH(	"1");
	    OP(		EQ);
	    break;
	case STR_IS_WIDE:
	    PUSH(	"2");
	    OP(		LE);
	    break;
	case STR_IS_ENTIER:
	    PUSH(	"3");
	    OP(		LE);
	    break;
	}
	FIXJUMP1(	end);
	return TCL_OK;

    case STR_IS_LIST:
	range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
	OP4(		BEGIN_CATCH4, range);
	ExceptionRangeStarts(envPtr, range);
	OP(		DUP);
	OP(		LIST_LENGTH);
	OP(		POP);
	ExceptionRangeEnds(envPtr, range);
	ExceptionRangeTarget(envPtr, range, catchOffset);
	OP(		POP);
	OP(		PUSH_RETURN_CODE);
	OP(		END_CATCH);
	OP(		LNOT);
	return TCL_OK;
    }

    return TclCompileBasicMin0ArgCmd(interp, parsePtr, cmdPtr, envPtr);
}

int
TclCompileStringMatchCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;
    int i, length, exactMatch = 0, nocase = 0;
    const char *str;

    if (parsePtr->numWords < 3 || parsePtr->numWords > 4) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(parsePtr->tokenPtr);

    /*
     * Check if we have a -nocase flag.
     */

    if (parsePtr->numWords == 4) {
	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    return TclCompileBasic3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
	}
	str = tokenPtr[1].start;
	length = tokenPtr[1].size;
	if ((length <= 1) || strncmp(str, "-nocase", (size_t) length)) {
	    /*
	     * Fail at run time, not in compilation.
	     */

	    return TclCompileBasic3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
	}
	nocase = 1;
	tokenPtr = TokenAfter(tokenPtr);
    }

    /*
     * Push the strings to match against each other.
     */

    for (i = 0; i < 2; i++) {
	if (tokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	    str = tokenPtr[1].start;
	    length = tokenPtr[1].size;
	    if (!nocase && (i == 0)) {
		/*
		 * Trivial matches can be done by 'string equal'. If -nocase
		 * was specified, we can't do this because INST_STR_EQ has no
		 * support for nocase.
		 */

		Tcl_Obj *copy = Tcl_NewStringObj(str, length);

		Tcl_IncrRefCount(copy);
		exactMatch = TclMatchIsTrivial(TclGetString(copy));
		TclDecrRefCount(copy);
	    }
	    PushLiteral(envPtr, str, length);
	} else {
	    SetLineInformation(i+1+nocase);
	    CompileTokens(envPtr, tokenPtr, interp);
	}
	tokenPtr = TokenAfter(tokenPtr);
    }

    /*
     * Push the matcher.
     */

    if (exactMatch) {
	TclEmitOpcode(INST_STR_EQ, envPtr);
    } else {
	TclEmitInstInt1(INST_STR_MATCH, nocase, envPtr);
    }
    return TCL_OK;
}

int
TclCompileStringLenCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;
    Tcl_Obj *objPtr;

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    TclNewObj(objPtr);
    if (TclWordKnownAtCompileTime(tokenPtr, objPtr)) {
	/*
	 * Here someone is asking for the length of a static string (or
	 * something with backslashes). Just push the actual character (not
	 * byte) length.
	 */

	char buf[TCL_INTEGER_SPACE];
	int len = Tcl_GetCharLength(objPtr);

	len = sprintf(buf, "%d", len);
	PushLiteral(envPtr, buf, len);
    } else {
	SetLineInformation(1);
	CompileTokens(envPtr, tokenPtr, interp);
	TclEmitOpcode(INST_STR_LEN, envPtr);
    }
    TclDecrRefCount(objPtr);
    return TCL_OK;
}

int
TclCompileStringMapCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *mapTokenPtr, *stringTokenPtr;
    Tcl_Obj *mapObj, **objv;
    char *bytes;
    int len;

    /*
     * We only handle the case:
     *
     *    string map {foo bar} $thing
     *
     * That is, a literal two-element list (doesn't need to be brace-quoted,
     * but does need to be compile-time knowable) and any old argument (the
     * thing to map).
     */

    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }
    mapTokenPtr = TokenAfter(parsePtr->tokenPtr);
    stringTokenPtr = TokenAfter(mapTokenPtr);
    mapObj = Tcl_NewObj();
    Tcl_IncrRefCount(mapObj);
    if (!TclWordKnownAtCompileTime(mapTokenPtr, mapObj)) {
	Tcl_DecrRefCount(mapObj);
	return TclCompileBasic2ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    } else if (Tcl_ListObjGetElements(NULL, mapObj, &len, &objv) != TCL_OK) {
	Tcl_DecrRefCount(mapObj);
	return TclCompileBasic2ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    } else if (len != 2) {
	Tcl_DecrRefCount(mapObj);
	return TclCompileBasic2ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    /*
     * Now issue the opcodes. Note that in the case that we know that the
     * first word is an empty word, we don't issue the map at all. That is the
     * correct semantics for mapping.
     */

    bytes = Tcl_GetStringFromObj(objv[0], &len);
    if (len == 0) {
	CompileWord(envPtr, stringTokenPtr, interp, 2);
    } else {
	PushLiteral(envPtr, bytes, len);
	bytes = Tcl_GetStringFromObj(objv[1], &len);
	PushLiteral(envPtr, bytes, len);
	CompileWord(envPtr, stringTokenPtr, interp, 2);
	OP(STR_MAP);
    }
    Tcl_DecrRefCount(mapObj);
    return TCL_OK;
}

int
TclCompileStringRangeCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *stringTokenPtr, *fromTokenPtr, *toTokenPtr;
    int idx1, idx2;

    if (parsePtr->numWords != 4) {
	return TCL_ERROR;
    }
    stringTokenPtr = TokenAfter(parsePtr->tokenPtr);
    fromTokenPtr = TokenAfter(stringTokenPtr);
    toTokenPtr = TokenAfter(fromTokenPtr);

    /* Every path must push the string argument */
    CompileWord(envPtr, stringTokenPtr,			interp, 1);

    /*
     * Parse the two indices.
     */

    if (TclGetIndexFromToken(fromTokenPtr, TCL_INDEX_START, TCL_INDEX_AFTER,
	    &idx1) != TCL_OK) {
	goto nonConstantIndices;
    }
    /*
     * Token parsed as an index expression. We treat all indices before
     * the string the same as the start of the string.
     */

    if (idx1 == TCL_INDEX_AFTER) {
	/* [string range $s end+1 $last] must be empty string */
	OP(		POP);
	PUSH(		"");
	return TCL_OK;
    }

    if (TclGetIndexFromToken(toTokenPtr, TCL_INDEX_BEFORE, TCL_INDEX_END,
	    &idx2) != TCL_OK) {
	goto nonConstantIndices;
    }
    /*
     * Token parsed as an index expression. We treat all indices after
     * the string the same as the end of the string.
     */
    if (idx2 == TCL_INDEX_BEFORE) {
	/* [string range $s $first -1] must be empty string */
	OP(		POP);
	PUSH(		"");
	return TCL_OK;
    }

    /*
     * Push the operand onto the stack and then the substring operation.
     */

    OP44(		STR_RANGE_IMM, idx1, idx2);
    return TCL_OK;

    /*
     * Push the operands onto the stack and then the substring operation.
     */

  nonConstantIndices:
    CompileWord(envPtr, fromTokenPtr,			interp, 2);
    CompileWord(envPtr, toTokenPtr,			interp, 3);
    OP(			STR_RANGE);
    return TCL_OK;
}

int
TclCompileStringReplaceCmd(
    Tcl_Interp *interp,		/* Tcl interpreter for context. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the
				 * command. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds the resulting instructions. */
{
    Tcl_Token *tokenPtr, *valueTokenPtr;
    DefineLineInformation;	/* TIP #280 */
    int first, last;

    if (parsePtr->numWords < 4 || parsePtr->numWords > 5) {
	return TCL_ERROR;
    }

    /* Bytecode to compute/push string argument being replaced */
    valueTokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, valueTokenPtr, interp, 1);

    /*
     * Check for first index known and useful at compile time.
     */
    tokenPtr = TokenAfter(valueTokenPtr);
    if (TclGetIndexFromToken(tokenPtr, TCL_INDEX_BEFORE, TCL_INDEX_AFTER,
	    &first) != TCL_OK) {
	goto genericReplace;
    }

    /*
     * Check for last index known and useful at compile time.
     */
    tokenPtr = TokenAfter(tokenPtr);
    if (TclGetIndexFromToken(tokenPtr, TCL_INDEX_BEFORE, TCL_INDEX_AFTER,
	    &last) != TCL_OK) {
	goto genericReplace;
    }

    /*
     * [string replace] is an odd bird.  For many arguments it is
     * a conventional substring replacer.  However it also goes out
     * of its way to become a no-op for many cases where it would be
     * replacing an empty substring.  Precisely, it is a no-op when
     *
     *		(last < first)		OR
     *		(last < 0)		OR
     *		(end < first)
     *
     * For some compile-time values we can detect these cases, and
     * compile direct to bytecode implementing the no-op.
     */

    if ((last == TCL_INDEX_BEFORE)		/* Know (last < 0) */
	    || (first == TCL_INDEX_AFTER)	/* Know (first > end) */

	/*
	 * Tricky to determine when runtime (last < first) can be
	 * certainly known based on the encoded values. Consider the
	 * cases...
	 *
	 * (first <= TCL_INDEX_END) &&
	 *	(last == TCL_INDEX_AFTER) => cannot tell REJECT
	 *	(last <= TCL_INDEX END) && (last < first) => ACCEPT
	 *	else => cannot tell REJECT
	 */
	    || ((first <= TCL_INDEX_END) && (last <= TCL_INDEX_END)
		&& (last < first))		/* Know (last < first) */
	/*
	 * (first == TCL_INDEX_BEFORE) &&
	 *	(last == TCL_INDEX_AFTER) => (first < last) REJECT
	 *	(last <= TCL_INDEX_END) => cannot tell REJECT
	 *	else		=> (first < last) REJECT
	 *
	 * else [[first >= TCL_INDEX_START]] &&
	 *	(last == TCL_INDEX_AFTER) => cannot tell REJECT
	 *	(last <= TCL_INDEX_END) => cannot tell REJECT
	 *	else [[last >= TCL_INDEX START]] && (last < first) => ACCEPT
	 */
	    || ((first >= TCL_INDEX_START) && (last >= TCL_INDEX_START)
		&& (last < first))) {		/* Know (last < first) */
	if (parsePtr->numWords == 5) {
	    tokenPtr = TokenAfter(tokenPtr);
	    CompileWord(envPtr, tokenPtr, interp, 4);
	    OP(		POP);		/* Pop newString */
	}
	/* Original string argument now on TOS as result */
	return TCL_OK;
    }

    if (parsePtr->numWords == 5) {
    /*
     * When we have a string replacement, we have to take care about
     * not replacing empty substrings that [string replace] promises
     * not to replace
     *
     * The remaining index values might be suitable for conventional
     * string replacement, but only if they cannot possibly meet the
     * conditions described above at runtime. If there's a chance they
     * might, we would have to emit bytecode to check and at that point
     * we're paying more in bytecode execution time than would make
     * things worthwhile. Trouble is we are very limited in
     * how much we can detect that at compile time. After decoding,
     * we need, first:
     *
     *		(first <= end)
     *
     * The encoded indices (first <= TCL_INDEX END) and
     * (first == TCL_INDEX_BEFORE) always meets this condition, but
     * any other encoded first index has some list for which it fails.
     *
     * We also need, second:
     *
     *		(last >= 0)
     *
     * The encoded indices (last >= TCL_INDEX_START) and
     * (last == TCL_INDEX_AFTER) always meet this condition but any
     * other encoded last index has some list for which it fails.
     *
     * Finally we need, third:
     *
     *		(first <= last)
     *
     * Considered in combination with the constraints we already have,
     * we see that we can proceed when (first == TCL_INDEX_BEFORE)
     * or (last == TCL_INDEX_AFTER). These also permit simplification
     * of the prefix|replace|suffix construction. The other constraints,
     * though, interfere with getting a guarantee that first <= last.
     */

    if ((first == TCL_INDEX_BEFORE) && (last >= TCL_INDEX_START)) {
	/* empty prefix */
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, 4);
	OP4(		REVERSE, 2);
	if (last == TCL_INDEX_AFTER) {
	    OP(		POP);		/* Pop  original */
	} else {
	    OP44(	STR_RANGE_IMM, last + 1, TCL_INDEX_END);
	    OP1(	STR_CONCAT1, 2);
	}
	return TCL_OK;
    }

    if ((last == TCL_INDEX_AFTER) && (first <= TCL_INDEX_END)) {
	OP44(		STR_RANGE_IMM, 0, first-1);
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, 4);
	OP1(		STR_CONCAT1, 2);
	return TCL_OK;
    }

	/* FLOW THROUGH TO genericReplace */

    } else {
	/*
	 * When we have no replacement string to worry about, we may
	 * have more luck, because the forbidden empty string replacements
	 * are harmless when they are replaced by another empty string.
	 */

	if ((first == TCL_INDEX_BEFORE) || (first == TCL_INDEX_START)) {
	    /* empty prefix - build suffix only */

	    if ((last == TCL_INDEX_END) || (last == TCL_INDEX_AFTER)) {
		/* empty suffix too => empty result */
		OP(	POP);		/* Pop  original */
		PUSH	(	"");
		return TCL_OK;
	    }
	    OP44(	STR_RANGE_IMM, last + 1, TCL_INDEX_END);
	    return TCL_OK;
	} else {
	    if ((last == TCL_INDEX_END) || (last == TCL_INDEX_AFTER)) {
		/* empty suffix - build prefix only */
		OP44(	STR_RANGE_IMM, 0, first-1);
		return TCL_OK;
	    }
	    OP(		DUP);
	    OP44(	STR_RANGE_IMM, 0, first-1);
	    OP4(	REVERSE, 2);
	    OP44(	STR_RANGE_IMM, last + 1, TCL_INDEX_END);
	    OP1(	STR_CONCAT1, 2);
	    return TCL_OK;
	}
    }

    genericReplace:
	tokenPtr = TokenAfter(valueTokenPtr);
	CompileWord(envPtr, tokenPtr, interp, 2);
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, 3);
	if (parsePtr->numWords == 5) {
	    tokenPtr = TokenAfter(tokenPtr);
	    CompileWord(envPtr, tokenPtr, interp, 4);
	} else {
	    PUSH(	"");
	}
	OP(		STR_REPLACE);
	return TCL_OK;
}

int
TclCompileStringTrimLCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    if (parsePtr->numWords != 2 && parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr,			interp, 1);
    if (parsePtr->numWords == 3) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr,			interp, 2);
    } else {
	PushLiteral(envPtr, tclDefaultTrimSet, strlen(tclDefaultTrimSet));
    }
    OP(			STR_TRIM_LEFT);
    return TCL_OK;
}

int
TclCompileStringTrimRCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    if (parsePtr->numWords != 2 && parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr,			interp, 1);
    if (parsePtr->numWords == 3) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr,			interp, 2);
    } else {
	PushLiteral(envPtr, tclDefaultTrimSet, strlen(tclDefaultTrimSet));
    }
    OP(			STR_TRIM_RIGHT);
    return TCL_OK;
}

int
TclCompileStringTrimCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    if (parsePtr->numWords != 2 && parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr,			interp, 1);
    if (parsePtr->numWords == 3) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr,			interp, 2);
    } else {
	PushLiteral(envPtr, tclDefaultTrimSet, strlen(tclDefaultTrimSet));
    }
    OP(			STR_TRIM);
    return TCL_OK;
}

int
TclCompileStringToUpperCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    if (parsePtr->numWords != 2) {
	return TclCompileBasic1To3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr,			interp, 1);
    OP(			STR_UPPER);
    return TCL_OK;
}

int
TclCompileStringToLowerCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    if (parsePtr->numWords != 2) {
	return TclCompileBasic1To3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr,			interp, 1);
    OP(			STR_LOWER);
    return TCL_OK;
}

int
TclCompileStringToTitleCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr;

    if (parsePtr->numWords != 2) {
	return TclCompileBasic1To3ArgCmd(interp, parsePtr, cmdPtr, envPtr);
    }

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr,			interp, 1);
    OP(			STR_TITLE);
    return TCL_OK;
}

/*
 * Support definitions for the [string is] compilation.
 */

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

StringClassDesc const tclStringClassTable[] = {
    {"alnum",	Tcl_UniCharIsAlnum},
    {"alpha",	Tcl_UniCharIsAlpha},
    {"ascii",	UniCharIsAscii},
    {"control", Tcl_UniCharIsControl},
    {"digit",	Tcl_UniCharIsDigit},
    {"graph",	Tcl_UniCharIsGraph},
    {"lower",	Tcl_UniCharIsLower},
    {"print",	Tcl_UniCharIsPrint},
    {"punct",	Tcl_UniCharIsPunct},
    {"space",	Tcl_UniCharIsSpace},
    {"upper",	Tcl_UniCharIsUpper},
    {"word",	Tcl_UniCharIsWordChar},
    {"xdigit",	UniCharIsHexDigit},
    {NULL,	NULL}
};

/*
 *----------------------------------------------------------------------
 *
 * TclCompileSubstCmd --
 *
 *	Procedure called to compile the "subst" command.
 *
 * Results:
 *	Returns TCL_OK for successful compile, or TCL_ERROR to defer
 *	evaluation to runtime (either when it is too complex to get the
 *	semantics right, or when we know for sure that it is an error but need
 *	the error to happen at the right time).
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "subst" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileSubstCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    int numArgs = parsePtr->numWords - 1;
    int numOpts = numArgs - 1;
    int objc, flags = TCL_SUBST_ALL;
    Tcl_Obj **objv/*, *toSubst = NULL*/;
    Tcl_Token *wordTokenPtr = TokenAfter(parsePtr->tokenPtr);
    int code = TCL_ERROR;
    DefineLineInformation;	/* TIP #280 */

    if (numArgs == 0) {
	return TCL_ERROR;
    }

    objv = TclStackAlloc(interp, /*numArgs*/ numOpts * sizeof(Tcl_Obj *));

    for (objc = 0; objc < /*numArgs*/ numOpts; objc++) {
	objv[objc] = Tcl_NewObj();
	Tcl_IncrRefCount(objv[objc]);
	if (!TclWordKnownAtCompileTime(wordTokenPtr, objv[objc])) {
	    objc++;
	    goto cleanup;
	}
	wordTokenPtr = TokenAfter(wordTokenPtr);
    }

/*
    if (TclSubstOptions(NULL, numOpts, objv, &flags) == TCL_OK) {
	toSubst = objv[numOpts];
	Tcl_IncrRefCount(toSubst);
    }
*/

    /* TODO: Figure out expansion to cover WordKnownAtCompileTime
     *	The difficulty is that WKACT makes a copy, and if TclSubstParse
     *	below parses the copy of the original source string, some deep
     *	parts of the compile machinery get upset.  They want all pointers
     *	stored in Tcl_Tokens to point back to the same original string.
     */
    if (wordTokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	code = TclSubstOptions(NULL, numOpts, objv, &flags);
    }

  cleanup:
    while (--objc >= 0) {
	TclDecrRefCount(objv[objc]);
    }
    TclStackFree(interp, objv);
    if (/*toSubst == NULL*/ code != TCL_OK) {
	return TCL_ERROR;
    }

    SetLineInformation(numArgs);
    TclSubstCompile(interp, wordTokenPtr[1].start, wordTokenPtr[1].size,
	    flags, mapPtr->loc[eclIndex].line[numArgs], envPtr);

/*    TclDecrRefCount(toSubst);*/
    return TCL_OK;
}

void
TclSubstCompile(
    Tcl_Interp *interp,
    const char *bytes,
    int numBytes,
    int flags,
    int line,
    CompileEnv *envPtr)
{
    Tcl_Token *endTokenPtr, *tokenPtr;
    int breakOffset = 0, count = 0, bline = line;
    Tcl_Parse parse;
    Tcl_InterpState state = NULL;

    TclSubstParse(interp, bytes, numBytes, flags, &parse, &state);
    if (state != NULL) {
	Tcl_ResetResult(interp);
    }

    /*
     * Tricky point! If the first token does not result in a *guaranteed* push
     * of a Tcl_Obj on the stack, we must push an empty object. Otherwise it
     * is possible to get to an INST_STR_CONCAT1 or INST_DONE without enough
     * values on the stack, resulting in a crash. Thanks to Joe Mistachkin for
     * identifying a script that could trigger this case.
     */

    tokenPtr = parse.tokenPtr;
    if (tokenPtr->type != TCL_TOKEN_TEXT && tokenPtr->type != TCL_TOKEN_BS) {
	PUSH("");
	count++;
    }

    for (endTokenPtr = tokenPtr + parse.numTokens;
	    tokenPtr < endTokenPtr; tokenPtr = TokenAfter(tokenPtr)) {
	int length, literal, catchRange, breakJump;
	char buf[TCL_UTF_MAX] = "";
	JumpFixup startFixup, okFixup, returnFixup, breakFixup;
	JumpFixup continueFixup, otherFixup, endFixup;

	switch (tokenPtr->type) {
	case TCL_TOKEN_TEXT:
	    literal = TclRegisterNewLiteral(envPtr,
		    tokenPtr->start, tokenPtr->size);
	    TclEmitPush(literal, envPtr);
	    TclAdvanceLines(&bline, tokenPtr->start,
		    tokenPtr->start + tokenPtr->size);
	    count++;
	    continue;
	case TCL_TOKEN_BS:
	    length = TclParseBackslash(tokenPtr->start, tokenPtr->size,
		    NULL, buf);
	    literal = TclRegisterNewLiteral(envPtr, buf, length);
	    TclEmitPush(literal, envPtr);
	    count++;
	    continue;
	case TCL_TOKEN_VARIABLE:
	    /*
	     * Check for simple variable access; see if we can only generate
	     * TCL_OK or TCL_ERROR from the substituted variable read; if so,
	     * there is no need to generate elaborate exception-management
	     * code. Note that the first component of TCL_TOKEN_VARIABLE is
	     * always TCL_TOKEN_TEXT...
	     */

	    if (tokenPtr->numComponents > 1) {
		int i, foundCommand = 0;

		for (i=2 ; i<=tokenPtr->numComponents ; i++) {
		    if (tokenPtr[i].type == TCL_TOKEN_COMMAND) {
			foundCommand = 1;
			break;
		    }
		}
		if (foundCommand) {
		    break;
		}
	    }

	    envPtr->line = bline;
	    TclCompileVarSubst(interp, tokenPtr, envPtr);
	    bline = envPtr->line;
	    count++;
	    continue;
	}

	while (count > 255) {
	    OP1(		STR_CONCAT1, 255);
	    count -= 254;
	}
	if (count > 1) {
	    OP1(		STR_CONCAT1, count);
	    count = 1;
	}

	if (breakOffset == 0) {
	    /* Jump to the start (jump over the jump to end) */
	    TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &startFixup);

	    /* Jump to the end (all BREAKs land here) */
	    breakOffset = CurrentOffset(envPtr);
	    TclEmitInstInt4(INST_JUMP4, 0, envPtr);

	    /* Start */
	    if (TclFixupForwardJumpToHere(envPtr, &startFixup, 127)) {
		Tcl_Panic("TclCompileSubstCmd: bad start jump distance %d",
			(int) (CurrentOffset(envPtr) - startFixup.codeOffset));
	    }
	}

	envPtr->line = bline;
	catchRange = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
	OP4(	BEGIN_CATCH4, catchRange);
	ExceptionRangeStarts(envPtr, catchRange);

	switch (tokenPtr->type) {
	case TCL_TOKEN_COMMAND:
	    TclCompileScript(interp, tokenPtr->start+1, tokenPtr->size-2,
		    envPtr);
	    count++;
	    break;
	case TCL_TOKEN_VARIABLE:
	    TclCompileVarSubst(interp, tokenPtr, envPtr);
	    count++;
	    break;
	default:
	    Tcl_Panic("unexpected token type in TclCompileSubstCmd: %d",
		    tokenPtr->type);
	}

	ExceptionRangeEnds(envPtr, catchRange);

	/* Substitution produced TCL_OK */
	OP(	END_CATCH);
	TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &okFixup);
	TclAdjustStackDepth(-1, envPtr);

	/* Exceptional return codes processed here */
	ExceptionRangeTarget(envPtr, catchRange, catchOffset);
	OP(	PUSH_RETURN_OPTIONS);
	OP(	PUSH_RESULT);
	OP(	PUSH_RETURN_CODE);
	OP(	END_CATCH);
	OP(	RETURN_CODE_BRANCH);

	/* ERROR -> reraise it; NB: can't require BREAK/CONTINUE handling */
	OP(	RETURN_STK);
	OP(	NOP);

	/* RETURN */
	TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &returnFixup);

	/* BREAK */
	TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &breakFixup);

	/* CONTINUE */
	TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &continueFixup);

	/* OTHER */
	TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &otherFixup);

	TclAdjustStackDepth(1, envPtr);
	/* BREAK destination */
	if (TclFixupForwardJumpToHere(envPtr, &breakFixup, 127)) {
	    Tcl_Panic("TclCompileSubstCmd: bad break jump distance %d",
		    (int) (CurrentOffset(envPtr) - breakFixup.codeOffset));
	}
	OP(	POP);
	OP(	POP);

	breakJump = CurrentOffset(envPtr) - breakOffset;
	if (breakJump > 127) {
	    OP4(JUMP4, -breakJump);
	} else {
	    OP1(JUMP1, -breakJump);
	}

	TclAdjustStackDepth(2, envPtr);
	/* CONTINUE destination */
	if (TclFixupForwardJumpToHere(envPtr, &continueFixup, 127)) {
	    Tcl_Panic("TclCompileSubstCmd: bad continue jump distance %d",
		    (int) (CurrentOffset(envPtr) - continueFixup.codeOffset));
	}
	OP(	POP);
	OP(	POP);
	TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &endFixup);

	TclAdjustStackDepth(2, envPtr);
	/* RETURN + other destination */
	if (TclFixupForwardJumpToHere(envPtr, &returnFixup, 127)) {
	    Tcl_Panic("TclCompileSubstCmd: bad return jump distance %d",
		    (int) (CurrentOffset(envPtr) - returnFixup.codeOffset));
	}
	if (TclFixupForwardJumpToHere(envPtr, &otherFixup, 127)) {
	    Tcl_Panic("TclCompileSubstCmd: bad other jump distance %d",
		    (int) (CurrentOffset(envPtr) - otherFixup.codeOffset));
	}

	/*
	 * Pull the result to top of stack, discard options dict.
	 */

	OP4(	REVERSE, 2);
	OP(	POP);

	/* OK destination */
	if (TclFixupForwardJumpToHere(envPtr, &okFixup, 127)) {
	    Tcl_Panic("TclCompileSubstCmd: bad ok jump distance %d",
		    (int) (CurrentOffset(envPtr) - okFixup.codeOffset));
	}
	if (count > 1) {
	    OP1(STR_CONCAT1, count);
	    count = 1;
	}

	/* CONTINUE jump to here */
	if (TclFixupForwardJumpToHere(envPtr, &endFixup, 127)) {
	    Tcl_Panic("TclCompileSubstCmd: bad end jump distance %d",
		    (int) (CurrentOffset(envPtr) - endFixup.codeOffset));
	}
	bline = envPtr->line;
    }

    while (count > 255) {
	OP1(	STR_CONCAT1, 255);
	count -= 254;
    }
    if (count > 1) {
	OP1(	STR_CONCAT1, count);
    }

    Tcl_FreeParse(&parse);

    if (state != NULL) {
	Tcl_RestoreInterpState(interp, state);
	TclCompileSyntaxError(interp, envPtr);
	TclAdjustStackDepth(-1, envPtr);
    }

    /* Final target of the multi-jump from all BREAKs */
    if (breakOffset > 0) {
	TclUpdateInstInt4AtPc(INST_JUMP4, CurrentOffset(envPtr) - breakOffset,
		envPtr->codeStart + breakOffset);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileSwitchCmd --
 *
 *	Procedure called to compile the "switch" command.
 *
 * Results:
 *	Returns TCL_OK for successful compile, or TCL_ERROR to defer
 *	evaluation to runtime (either when it is too complex to get the
 *	semantics right, or when we know for sure that it is an error but need
 *	the error to happen at the right time).
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "switch" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileSwitchCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;	/* Pointer to tokens in command. */
    int numWords;		/* Number of words in command. */

    Tcl_Token *valueTokenPtr;	/* Token for the value to switch on. */
    enum {Switch_Exact, Switch_Glob, Switch_Regexp} mode;
				/* What kind of switch are we doing? */

    Tcl_Token *bodyTokenArray;	/* Array of real pattern list items. */
    Tcl_Token **bodyToken;	/* Array of pointers to pattern list items. */
    int *bodyLines;		/* Array of line numbers for body list
				 * items. */
    int **bodyContLines;	/* Array of continuation line info. */
    int noCase;			/* Has the -nocase flag been given? */
    int foundMode = 0;		/* Have we seen a mode flag yet? */
    int i, valueIndex;
    int result = TCL_ERROR;
    DefineLineInformation;	/* TIP #280 */
    int *clNext = envPtr->clNext;

    /*
     * Only handle the following versions:
     *   switch         ?--? word {pattern body ...}
     *   switch -exact  ?--? word {pattern body ...}
     *   switch -glob   ?--? word {pattern body ...}
     *   switch -regexp ?--? word {pattern body ...}
     *   switch         --   word simpleWordPattern simpleWordBody ...
     *   switch -exact  --   word simpleWordPattern simpleWordBody ...
     *   switch -glob   --   word simpleWordPattern simpleWordBody ...
     *   switch -regexp --   word simpleWordPattern simpleWordBody ...
     * When the mode is -glob, can also handle a -nocase flag.
     *
     * First off, we don't care how the command's word was generated; we're
     * compiling it anyway! So skip it...
     */

    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    valueIndex = 1;
    numWords = parsePtr->numWords-1;

    /*
     * Check for options.
     */

    noCase = 0;
    mode = Switch_Exact;
    if (numWords == 2) {
	/*
	 * There's just the switch value and the bodies list. In that case, we
	 * can skip all option parsing and move on to consider switch values
	 * and the body list.
	 */

	goto finishedOptionParse;
    }

    /*
     * There must be at least one option, --, because without that there is no
     * way to statically avoid the problems you get from strings-to-be-matched
     * that start with a - (the interpreted code falls apart if it encounters
     * them, so we punt if we *might* encounter them as that is the easiest
     * way of emulating the behaviour).
     */

    for (; numWords>=3 ; tokenPtr=TokenAfter(tokenPtr),numWords--) {
	register unsigned size = tokenPtr[1].size;
	register const char *chrs = tokenPtr[1].start;

	/*
	 * We only process literal options, and we assume that -e, -g and -n
	 * are unique prefixes of -exact, -glob and -nocase respectively (true
	 * at time of writing). Note that -exact and -glob may only be given
	 * at most once or we bail out (error case).
	 */

	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD || size < 2) {
	    return TCL_ERROR;
	}

	if ((size <= 6) && !memcmp(chrs, "-exact", size)) {
	    if (foundMode) {
		return TCL_ERROR;
	    }
	    mode = Switch_Exact;
	    foundMode = 1;
	    valueIndex++;
	    continue;
	} else if ((size <= 5) && !memcmp(chrs, "-glob", size)) {
	    if (foundMode) {
		return TCL_ERROR;
	    }
	    mode = Switch_Glob;
	    foundMode = 1;
	    valueIndex++;
	    continue;
	} else if ((size <= 7) && !memcmp(chrs, "-regexp", size)) {
	    if (foundMode) {
		return TCL_ERROR;
	    }
	    mode = Switch_Regexp;
	    foundMode = 1;
	    valueIndex++;
	    continue;
	} else if ((size <= 7) && !memcmp(chrs, "-nocase", size)) {
	    noCase = 1;
	    valueIndex++;
	    continue;
	} else if ((size == 2) && !memcmp(chrs, "--", 2)) {
	    valueIndex++;
	    break;
	}

	/*
	 * The switch command has many flags we cannot compile at all (e.g.
	 * all the RE-related ones) which we must have encountered. Either
	 * that or we have run off the end. The action here is the same: punt
	 * to interpreted version.
	 */

	return TCL_ERROR;
    }
    if (numWords < 3) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(tokenPtr);
    numWords--;
    if (noCase && (mode == Switch_Exact)) {
	/*
	 * Can't compile this case; no opcode for case-insensitive equality!
	 */

	return TCL_ERROR;
    }

    /*
     * The value to test against is going to always get pushed on the stack.
     * But not yet; we need to verify that the rest of the command is
     * compilable too.
     */

  finishedOptionParse:
    valueTokenPtr = tokenPtr;
    /* For valueIndex, see previous loop. */
    tokenPtr = TokenAfter(tokenPtr);
    numWords--;

    /*
     * Build an array of tokens for the matcher terms and script bodies. Note
     * that in the case of the quoted bodies, this is tricky as we cannot use
     * copies of the string from the input token for the generated tokens (it
     * causes a crash during exception handling). When multiple tokens are
     * available at this point, this is pretty easy.
     */

    if (numWords == 1) {
	const char *bytes;
	int maxLen, numBytes;
	int bline;		/* TIP #280: line of the pattern/action list,
				 * and start of list for when tracking the
				 * location. This list comes immediately after
				 * the value we switch on. */

	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    return TCL_ERROR;
	}
	bytes = tokenPtr[1].start;
	numBytes = tokenPtr[1].size;

	/* Allocate enough space to work in. */
	maxLen = TclMaxListLength(bytes, numBytes, NULL);
	if (maxLen < 2)  {
	    return TCL_ERROR;
	}
	bodyTokenArray = ckalloc(sizeof(Tcl_Token) * maxLen);
	bodyToken = ckalloc(sizeof(Tcl_Token *) * maxLen);
	bodyLines = ckalloc(sizeof(int) * maxLen);
	bodyContLines = ckalloc(sizeof(int*) * maxLen);

	bline = mapPtr->loc[eclIndex].line[valueIndex+1];
	numWords = 0;

	while (numBytes > 0) {
	    const char *prevBytes = bytes;
	    int literal;

	    if (TCL_OK != TclFindElement(NULL, bytes, numBytes,
		    &(bodyTokenArray[numWords].start), &bytes,
		    &(bodyTokenArray[numWords].size), &literal) || !literal) {
		goto abort;
	    }

	    bodyTokenArray[numWords].type = TCL_TOKEN_TEXT;
	    bodyTokenArray[numWords].numComponents = 0;
	    bodyToken[numWords] = bodyTokenArray + numWords;

	    /*
	     * TIP #280: Now determine the line the list element starts on
	     * (there is no need to do it earlier, due to the possibility of
	     * aborting, see above).
	     */

	    TclAdvanceLines(&bline, prevBytes, bodyTokenArray[numWords].start);
	    TclAdvanceContinuations(&bline, &clNext,
		    bodyTokenArray[numWords].start - envPtr->source);
	    bodyLines[numWords] = bline;
	    bodyContLines[numWords] = clNext;
	    TclAdvanceLines(&bline, bodyTokenArray[numWords].start, bytes);
	    TclAdvanceContinuations(&bline, &clNext, bytes - envPtr->source);

	    numBytes -= (bytes - prevBytes);
	    numWords++;
	}
	if (numWords % 2) {
	abort:
	    ckfree((char *) bodyToken);
	    ckfree((char *) bodyTokenArray);
	    ckfree((char *) bodyLines);
	    ckfree((char *) bodyContLines);
	    return TCL_ERROR;
	}
    } else if (numWords % 2 || numWords == 0) {
	/*
	 * Odd number of words (>1) available, or no words at all available.
	 * Both are error cases, so punt and let the interpreted-version
	 * generate the error message. Note that the second case probably
	 * should get caught earlier, but it's easy to check here again anyway
	 * because it'd cause a nasty crash otherwise.
	 */

	return TCL_ERROR;
    } else {
	/*
	 * Multi-word definition of patterns & actions.
	 */

	bodyToken = ckalloc(sizeof(Tcl_Token *) * numWords);
	bodyLines = ckalloc(sizeof(int) * numWords);
	bodyContLines = ckalloc(sizeof(int*) * numWords);
	bodyTokenArray = NULL;
	for (i=0 ; i<numWords ; i++) {
	    /*
	     * We only handle the very simplest case. Anything more complex is
	     * a good reason to go to the interpreted case anyway due to
	     * traces, etc.
	     */

	    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
		goto freeTemporaries;
	    }
	    bodyToken[i] = tokenPtr+1;

	    /*
	     * TIP #280: Copy line information from regular cmd info.
	     */

	    bodyLines[i] = mapPtr->loc[eclIndex].line[valueIndex+1+i];
	    bodyContLines[i] = mapPtr->loc[eclIndex].next[valueIndex+1+i];
	    tokenPtr = TokenAfter(tokenPtr);
	}
    }

    /*
     * Fall back to interpreted if the last body is a continuation (it's
     * illegal, but this makes the error happen at the right time).
     */

    if (bodyToken[numWords-1]->size == 1 &&
	    bodyToken[numWords-1]->start[0] == '-') {
	goto freeTemporaries;
    }

    /*
     * Now we commit to generating code; the parsing stage per se is done.
     * Check if we can generate a jump table, since if so that's faster than
     * doing an explicit compare with each body. Note that we're definitely
     * over-conservative with determining whether we can do the jump table,
     * but it handles the most common case well enough.
     */

    /* Both methods push the value to match against onto the stack. */
    CompileWord(envPtr, valueTokenPtr, interp, valueIndex);

    if (mode == Switch_Exact) {
	IssueSwitchJumpTable(interp, envPtr, valueIndex, numWords, bodyToken,
		bodyLines, bodyContLines);
    } else {
	IssueSwitchChainedTests(interp, envPtr, mode, noCase, valueIndex,
		numWords, bodyToken, bodyLines, bodyContLines);
    }
    result = TCL_OK;

    /*
     * Clean up all our temporary space and return.
     */

  freeTemporaries:
    ckfree(bodyToken);
    ckfree(bodyLines);
    ckfree(bodyContLines);
    if (bodyTokenArray != NULL) {
	ckfree(bodyTokenArray);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * IssueSwitchChainedTests --
 *
 *	Generate instructions for a [switch] command that is to be compiled
 *	into a sequence of tests. This is the generic handle-everything mode
 *	that inherently has performance that is (on average) linear in the
 *	number of tests. It is the only mode that can handle -glob and -regexp
 *	matches, or anything that is case-insensitive. It does not handle the
 *	wild-and-wooly end of regexp matching (i.e., capture of match results)
 *	so that's when we spill to the interpreted version.
 *
 *----------------------------------------------------------------------
 */

static void
IssueSwitchChainedTests(
    Tcl_Interp *interp,		/* Context for compiling script bodies. */
    CompileEnv *envPtr,		/* Holds resulting instructions. */
    int mode,			/* Exact, Glob or Regexp */
    int noCase,			/* Case-insensitivity flag. */
    int valueIndex,		/* The value to match against. */
    int numBodyTokens,		/* Number of tokens describing things the
				 * switch can match against and bodies to
				 * execute when the match succeeds. */
    Tcl_Token **bodyToken,	/* Array of pointers to pattern list items. */
    int *bodyLines,		/* Array of line numbers for body list
				 * items. */
    int **bodyContLines)	/* Array of continuation line info. */
{
    enum {Switch_Exact, Switch_Glob, Switch_Regexp};
    int foundDefault;		/* Flag to indicate whether a "default" clause
				 * is present. */
    JumpFixup *fixupArray;	/* Array of forward-jump fixup records. */
    unsigned int *fixupTargetArray; /* Array of places for fixups to point at. */
    int fixupCount;		/* Number of places to fix up. */
    int contFixIndex;		/* Where the first of the jumps due to a group
				 * of continuation bodies starts, or -1 if
				 * there aren't any. */
    int contFixCount;		/* Number of continuation bodies pointing to
				 * the current (or next) real body. */
    int nextArmFixupIndex;
    int simple, exact;		/* For extracting the type of regexp. */
    int i;

    /*
     * Generate a test for each arm.
     */

    contFixIndex = -1;
    contFixCount = 0;
    fixupArray = TclStackAlloc(interp, sizeof(JumpFixup) * numBodyTokens);
    fixupTargetArray = TclStackAlloc(interp, sizeof(int) * numBodyTokens);
    memset(fixupTargetArray, 0, numBodyTokens * sizeof(int));
    fixupCount = 0;
    foundDefault = 0;
    for (i=0 ; i<numBodyTokens ; i+=2) {
	nextArmFixupIndex = -1;
	if (i!=numBodyTokens-2 || bodyToken[numBodyTokens-2]->size != 7 ||
		memcmp(bodyToken[numBodyTokens-2]->start, "default", 7)) {
	    /*
	     * Generate the test for the arm.
	     */

	    switch (mode) {
	    case Switch_Exact:
		OP(	DUP);
		TclCompileTokens(interp, bodyToken[i], 1,	envPtr);
		OP(	STR_EQ);
		break;
	    case Switch_Glob:
		TclCompileTokens(interp, bodyToken[i], 1,	envPtr);
		OP4(	OVER, 1);
		OP1(	STR_MATCH, noCase);
		break;
	    case Switch_Regexp:
		simple = exact = 0;

		/*
		 * Keep in sync with TclCompileRegexpCmd.
		 */

		if (bodyToken[i]->type == TCL_TOKEN_TEXT) {
		    Tcl_DString ds;

		    if (bodyToken[i]->size == 0) {
			/*
			 * The semantics of regexps are that they always match
			 * when the RE == "".
			 */

			PUSH("1");
			break;
		    }

		    /*
		     * Attempt to convert pattern to glob. If successful, push
		     * the converted pattern.
		     */

		    if (TclReToGlob(NULL, bodyToken[i]->start,
			    bodyToken[i]->size, &ds, &exact, NULL) == TCL_OK){
			simple = 1;
			PushLiteral(envPtr, Tcl_DStringValue(&ds),
				Tcl_DStringLength(&ds));
			Tcl_DStringFree(&ds);
		    }
		}
		if (!simple) {
		    TclCompileTokens(interp, bodyToken[i], 1, envPtr);
		}

		OP4(	OVER, 1);
		if (!simple) {
		    /*
		     * Pass correct RE compile flags. We use only Int1
		     * (8-bit), but that handles all the flags we want to
		     * pass. Don't use TCL_REG_NOSUB as we may have backrefs
		     * or capture vars.
		     */

		    int cflags = TCL_REG_ADVANCED
			    | (noCase ? TCL_REG_NOCASE : 0);

		    OP1(REGEXP, cflags);
		} else if (exact && !noCase) {
		    OP(	STR_EQ);
		} else {
		    OP1(STR_MATCH, noCase);
		}
		break;
	    default:
		Tcl_Panic("unknown switch mode: %d", mode);
	    }

	    /*
	     * In a fall-through case, we will jump on _true_ to the place
	     * where the body starts (generated later, with guarantee of this
	     * ensured earlier; the final body is never a fall-through).
	     */

	    if (bodyToken[i+1]->size==1 && bodyToken[i+1]->start[0]=='-') {
		if (contFixIndex == -1) {
		    contFixIndex = fixupCount;
		    contFixCount = 0;
		}
		TclEmitForwardJump(envPtr, TCL_TRUE_JUMP,
			&fixupArray[contFixIndex+contFixCount]);
		fixupCount++;
		contFixCount++;
		continue;
	    }

	    TclEmitForwardJump(envPtr, TCL_FALSE_JUMP,
		    &fixupArray[fixupCount]);
	    nextArmFixupIndex = fixupCount;
	    fixupCount++;
	} else {
	    /*
	     * Got a default clause; set a flag to inhibit the generation of
	     * the jump after the body and the cleanup of the intermediate
	     * value that we are switching against.
	     *
	     * Note that default clauses (which are always terminal clauses)
	     * cannot be fall-through clauses as well, since the last clause
	     * is never a fall-through clause (which we have already
	     * verified).
	     */

	    foundDefault = 1;
	}

	/*
	 * Generate the body for the arm. This is guaranteed not to be a
	 * fall-through case, but it might have preceding fall-through cases,
	 * so we must process those first.
	 */

	if (contFixIndex != -1) {
	    int j;

	    for (j=0 ; j<contFixCount ; j++) {
		fixupTargetArray[contFixIndex+j] = CurrentOffset(envPtr);
	    }
	    contFixIndex = -1;
	}

	/*
	 * Now do the actual compilation. Note that we do not use BODY()
	 * because we may have synthesized the tokens in a non-standard
	 * pattern.
	 */

	OP(	POP);
	envPtr->line = bodyLines[i+1];		/* TIP #280 */
	envPtr->clNext = bodyContLines[i+1];	/* TIP #280 */
	TclCompileCmdWord(interp, bodyToken[i+1], 1, envPtr);

	if (!foundDefault) {
	    TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP,
		    &fixupArray[fixupCount]);
	    fixupCount++;
	    fixupTargetArray[nextArmFixupIndex] = CurrentOffset(envPtr);
	}
    }

    /*
     * Discard the value we are matching against unless we've had a default
     * clause (in which case it will already be gone due to the code at the
     * start of processing an arm, guaranteed) and make the result of the
     * command an empty string.
     */

    if (!foundDefault) {
	OP(	POP);
	PUSH("");
    }

    /*
     * Do jump fixups for arms that were executed. First, fill in the jumps of
     * all jumps that don't point elsewhere to point to here.
     */

    for (i=0 ; i<fixupCount ; i++) {
	if (fixupTargetArray[i] == 0) {
	    fixupTargetArray[i] = envPtr->codeNext-envPtr->codeStart;
	}
    }

    /*
     * Now scan backwards over all the jumps (all of which are forward jumps)
     * doing each one. When we do one and there is a size changes, we must
     * scan back over all the previous ones and see if they need adjusting
     * before proceeding with further jump fixups (the interleaved nature of
     * all the jumps makes this impossible to do without nested loops).
     */

    for (i=fixupCount-1 ; i>=0 ; i--) {
	if (TclFixupForwardJump(envPtr, &fixupArray[i],
		fixupTargetArray[i] - fixupArray[i].codeOffset, 127)) {
	    int j;

	    for (j=i-1 ; j>=0 ; j--) {
		if (fixupTargetArray[j] > fixupArray[i].codeOffset) {
		    fixupTargetArray[j] += 3;
		}
	    }
	}
    }
    TclStackFree(interp, fixupTargetArray);
    TclStackFree(interp, fixupArray);
}

/*
 *----------------------------------------------------------------------
 *
 * IssueSwitchJumpTable --
 *
 *	Generate instructions for a [switch] command that is to be compiled
 *	into a jump table. This only handles the case where case-sensitive,
 *	exact matching is used, but this is actually the most common case in
 *	real code.
 *
 *----------------------------------------------------------------------
 */

static void
IssueSwitchJumpTable(
    Tcl_Interp *interp,		/* Context for compiling script bodies. */
    CompileEnv *envPtr,		/* Holds resulting instructions. */
    int valueIndex,		/* The value to match against. */
    int numBodyTokens,		/* Number of tokens describing things the
				 * switch can match against and bodies to
				 * execute when the match succeeds. */
    Tcl_Token **bodyToken,	/* Array of pointers to pattern list items. */
    int *bodyLines,		/* Array of line numbers for body list
				 * items. */
    int **bodyContLines)	/* Array of continuation line info. */
{
    JumptableInfo *jtPtr;
    int infoIndex, isNew, *finalFixups, numRealBodies = 0, jumpLocation;
    int mustGenerate, foundDefault, jumpToDefault, i;
    Tcl_DString buffer;
    Tcl_HashEntry *hPtr;

    /*
     * Compile the switch by using a jump table, which is basically a
     * hashtable that maps from literal values to match against to the offset
     * (relative to the INST_JUMP_TABLE instruction) to jump to. The jump
     * table itself is independent of any invokation of the bytecode, and as
     * such is stored in an auxData block.
     *
     * Start by allocating the jump table itself, plus some workspace.
     */

    jtPtr = ckalloc(sizeof(JumptableInfo));
    Tcl_InitHashTable(&jtPtr->hashTable, TCL_STRING_KEYS);
    infoIndex = TclCreateAuxData(jtPtr, &tclJumptableInfoType, envPtr);
    finalFixups = TclStackAlloc(interp, sizeof(int) * (numBodyTokens/2));
    foundDefault = 0;
    mustGenerate = 1;

    /*
     * Next, issue the instruction to do the jump, together with what we want
     * to do if things do not work out (jump to either the default clause or
     * the "default" default, which just sets the result to empty). Note that
     * we will come back and rewrite the jump's offset parameter when we know
     * what it should be, and that all jumps we issue are of the wide kind
     * because that makes the code much easier to debug!
     */

    jumpLocation = CurrentOffset(envPtr);
    OP4(	JUMP_TABLE, infoIndex);
    jumpToDefault = CurrentOffset(envPtr);
    OP4(	JUMP4, 0);

    for (i=0 ; i<numBodyTokens ; i+=2) {
	/*
	 * For each arm, we must first work out what to do with the match
	 * term.
	 */

	if (i!=numBodyTokens-2 || bodyToken[numBodyTokens-2]->size != 7 ||
		memcmp(bodyToken[numBodyTokens-2]->start, "default", 7)) {
	    /*
	     * This is not a default clause, so insert the current location as
	     * a target in the jump table (assuming it isn't already there,
	     * which would indicate that this clause is probably masked by an
	     * earlier one). Note that we use a Tcl_DString here simply
	     * because the hash API does not let us specify the string length.
	     */

	    Tcl_DStringInit(&buffer);
	    TclDStringAppendToken(&buffer, bodyToken[i]);
	    hPtr = Tcl_CreateHashEntry(&jtPtr->hashTable,
		    Tcl_DStringValue(&buffer), &isNew);
	    if (isNew) {
		/*
		 * First time we've encountered this match clause, so it must
		 * point to here.
		 */

		Tcl_SetHashValue(hPtr, CurrentOffset(envPtr) - jumpLocation);
	    }
	    Tcl_DStringFree(&buffer);
	} else {
	    /*
	     * This is a default clause, so patch up the fallthrough from the
	     * INST_JUMP_TABLE instruction to here.
	     */

	    foundDefault = 1;
	    isNew = 1;
	    TclStoreInt4AtPtr(CurrentOffset(envPtr)-jumpToDefault,
		    envPtr->codeStart+jumpToDefault+1);
	}

	/*
	 * Now, for each arm we must deal with the body of the clause.
	 *
	 * If this is a continuation body (never true of a final clause,
	 * whether default or not) we're done because the next jump target
	 * will also point here, so we advance to the next clause.
	 */

	if (bodyToken[i+1]->size == 1 && bodyToken[i+1]->start[0] == '-') {
	    mustGenerate = 1;
	    continue;
	}

	/*
	 * Also skip this arm if its only match clause is masked. (We could
	 * probably be more aggressive about this, but that would be much more
	 * difficult to get right.)
	 */

	if (!isNew && !mustGenerate) {
	    continue;
	}
	mustGenerate = 0;

	/*
	 * Compile the body of the arm.
	 */

	envPtr->line = bodyLines[i+1];		/* TIP #280 */
	envPtr->clNext = bodyContLines[i+1];	/* TIP #280 */
	TclCompileCmdWord(interp, bodyToken[i+1], 1, envPtr);

	/*
	 * Compile a jump in to the end of the command if this body is
	 * anything other than a user-supplied default arm (to either skip
	 * over the remaining bodies or the code that generates an empty
	 * result).
	 */

	if (i+2 < numBodyTokens || !foundDefault) {
	    finalFixups[numRealBodies++] = CurrentOffset(envPtr);

	    /*
	     * Easier by far to issue this jump as a fixed-width jump, since
	     * otherwise we'd need to do a lot more (and more awkward)
	     * rewriting when we fixed this all up.
	     */

	    OP4(	JUMP4, 0);
	    TclAdjustStackDepth(-1, envPtr);
	}
    }

    /*
     * We're at the end. If we've not already done so through the processing
     * of a user-supplied default clause, add in a "default" default clause
     * now.
     */

    if (!foundDefault) {
	TclStoreInt4AtPtr(CurrentOffset(envPtr)-jumpToDefault,
		envPtr->codeStart+jumpToDefault+1);
	PUSH("");
    }

    /*
     * No more instructions to be issued; everything that needs to jump to the
     * end of the command is fixed up at this point.
     */

    for (i=0 ; i<numRealBodies ; i++) {
	TclStoreInt4AtPtr(CurrentOffset(envPtr)-finalFixups[i],
		envPtr->codeStart+finalFixups[i]+1);
    }

    /*
     * Clean up all our temporary space and return.
     */

    TclStackFree(interp, finalFixups);
}

/*
 *----------------------------------------------------------------------
 *
 * DupJumptableInfo, FreeJumptableInfo --
 *
 *	Functions to duplicate, release and print a jump-table created for use
 *	with the INST_JUMP_TABLE instruction.
 *
 * Results:
 *	DupJumptableInfo: a copy of the jump-table
 *	FreeJumptableInfo: none
 *	PrintJumptableInfo: none
 *	DisassembleJumptableInfo: none
 *
 * Side effects:
 *	DupJumptableInfo: allocates memory
 *	FreeJumptableInfo: releases memory
 *	PrintJumptableInfo: none
 *	DisassembleJumptableInfo: none
 *
 *----------------------------------------------------------------------
 */

static ClientData
DupJumptableInfo(
    ClientData clientData)
{
    JumptableInfo *jtPtr = clientData;
    JumptableInfo *newJtPtr = ckalloc(sizeof(JumptableInfo));
    Tcl_HashEntry *hPtr, *newHPtr;
    Tcl_HashSearch search;
    int isNew;

    Tcl_InitHashTable(&newJtPtr->hashTable, TCL_STRING_KEYS);
    hPtr = Tcl_FirstHashEntry(&jtPtr->hashTable, &search);
    while (hPtr != NULL) {
	newHPtr = Tcl_CreateHashEntry(&newJtPtr->hashTable,
		Tcl_GetHashKey(&jtPtr->hashTable, hPtr), &isNew);
	Tcl_SetHashValue(newHPtr, Tcl_GetHashValue(hPtr));
    }
    return newJtPtr;
}

static void
FreeJumptableInfo(
    ClientData clientData)
{
    JumptableInfo *jtPtr = clientData;

    Tcl_DeleteHashTable(&jtPtr->hashTable);
    ckfree(jtPtr);
}

static void
PrintJumptableInfo(
    ClientData clientData,
    Tcl_Obj *appendObj,
    ByteCode *codePtr,
    unsigned int pcOffset)
{
    register JumptableInfo *jtPtr = clientData;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;
    const char *keyPtr;
    int offset, i = 0;

    hPtr = Tcl_FirstHashEntry(&jtPtr->hashTable, &search);
    for (; hPtr ; hPtr = Tcl_NextHashEntry(&search)) {
	keyPtr = Tcl_GetHashKey(&jtPtr->hashTable, hPtr);
	offset = PTR2INT(Tcl_GetHashValue(hPtr));

	if (i++) {
	    Tcl_AppendToObj(appendObj, ", ", -1);
	    if (i%4==0) {
		Tcl_AppendToObj(appendObj, "\n\t\t", -1);
	    }
	}
	Tcl_AppendPrintfToObj(appendObj, "\"%s\"->pc %d",
		keyPtr, pcOffset + offset);
    }
}

static void
DisassembleJumptableInfo(
    ClientData clientData,
    Tcl_Obj *dictObj,
    ByteCode *codePtr,
    unsigned int pcOffset)
{
    register JumptableInfo *jtPtr = clientData;
    Tcl_Obj *mapping = Tcl_NewObj();
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;
    const char *keyPtr;
    int offset;

    hPtr = Tcl_FirstHashEntry(&jtPtr->hashTable, &search);
    for (; hPtr ; hPtr = Tcl_NextHashEntry(&search)) {
	keyPtr = Tcl_GetHashKey(&jtPtr->hashTable, hPtr);
	offset = PTR2INT(Tcl_GetHashValue(hPtr));
	Tcl_DictObjPut(NULL, mapping, Tcl_NewStringObj(keyPtr, -1),
		Tcl_NewIntObj(offset));
    }
    Tcl_DictObjPut(NULL, dictObj, Tcl_NewStringObj("mapping", -1), mapping);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileTailcallCmd --
 *
 *	Procedure called to compile the "tailcall" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "tailcall" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileTailcallCmd(
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

    if (parsePtr->numWords < 2 || parsePtr->numWords > 256
	    || envPtr->procPtr == NULL) {
	return TCL_ERROR;
    }

    /* make room for the nsObjPtr */
    /* TODO: Doesn't this have to be a known value? */
    CompileWord(envPtr, tokenPtr, interp, 0);
    for (i=1 ; i<parsePtr->numWords ; i++) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, i);
    }
    TclEmitInstInt1(	INST_TAILCALL, parsePtr->numWords,	envPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileThrowCmd --
 *
 *	Procedure called to compile the "throw" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "throw" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileThrowCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    int numWords = parsePtr->numWords;
    Tcl_Token *codeToken, *msgToken;
    Tcl_Obj *objPtr;
    int codeKnown, codeIsList, codeIsValid, len;

    if (numWords != 3) {
	return TCL_ERROR;
    }
    codeToken = TokenAfter(parsePtr->tokenPtr);
    msgToken = TokenAfter(codeToken);

    TclNewObj(objPtr);
    Tcl_IncrRefCount(objPtr);

    codeKnown = TclWordKnownAtCompileTime(codeToken, objPtr);

    /*
     * First we must emit the code to substitute the arguments.  This
     * must come first in case substitution raises errors.
     */
    if (!codeKnown) {
	CompileWord(envPtr, codeToken, interp, 1);
	PUSH(			"-errorcode");
    }
    CompileWord(envPtr, msgToken, interp, 2);

    codeIsList = codeKnown && (TCL_OK ==
	    Tcl_ListObjLength(interp, objPtr, &len));
    codeIsValid = codeIsList && (len != 0);

    if (codeIsValid) {
	Tcl_Obj *errPtr, *dictPtr;

	TclNewLiteralStringObj(errPtr, "-errorcode");
	TclNewObj(dictPtr);
	Tcl_DictObjPut(NULL, dictPtr, errPtr, objPtr);
	TclEmitPush(TclAddLiteralObj(envPtr, dictPtr, NULL), envPtr);
    }
    TclDecrRefCount(objPtr);

    /*
     * Simpler bytecodes when we detect invalid arguments at compile time.
     */
    if (codeKnown && !codeIsValid) {
	OP(			POP);
	if (codeIsList) {
	    /* Must be an empty list */
	    goto issueErrorForEmptyCode;
	}
	TclCompileSyntaxError(interp, envPtr);
	return TCL_OK;
    }

    if (!codeKnown) {
	/*
	 * Argument validity checking has to be done by bytecode at
	 * run time.
	 */
	OP4(			REVERSE, 3);
	OP(			DUP);
	OP(			LIST_LENGTH);
	OP1(			JUMP_FALSE1, 16);
	OP4(			LIST, 2);
	OP44(			RETURN_IMM, TCL_ERROR, 0);
	TclAdjustStackDepth(2, envPtr);
	OP(			POP);
	OP(			POP);
	OP(			POP);
    issueErrorForEmptyCode:
	PUSH(			"type must be non-empty list");
	PUSH(			"-errorcode {TCL OPERATION THROW BADEXCEPTION}");
    }
    OP44(			RETURN_IMM, TCL_ERROR, 0);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileTryCmd --
 *
 *	Procedure called to compile the "try" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "try" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileTryCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    int numWords = parsePtr->numWords, numHandlers, result = TCL_ERROR;
    Tcl_Token *bodyToken, *finallyToken, *tokenPtr;
    Tcl_Token **handlerTokens = NULL;
    Tcl_Obj **matchClauses = NULL;
    int *matchCodes=NULL, *resultVarIndices=NULL, *optionVarIndices=NULL;
    int i;

    if (numWords < 2) {
	return TCL_ERROR;
    }

    bodyToken = TokenAfter(parsePtr->tokenPtr);

    if (numWords == 2) {
	/*
	 * No handlers or finally; do nothing beyond evaluating the body.
	 */

	DefineLineInformation;	/* TIP #280 */
	BODY(bodyToken, 1);
	return TCL_OK;
    }

    numWords -= 2;
    tokenPtr = TokenAfter(bodyToken);

    /*
     * Extract information about what handlers there are.
     */

    numHandlers = numWords >> 2;
    numWords -= numHandlers * 4;
    if (numHandlers > 0) {
	handlerTokens = TclStackAlloc(interp, sizeof(Tcl_Token*)*numHandlers);
	matchClauses = TclStackAlloc(interp, sizeof(Tcl_Obj *) * numHandlers);
	memset(matchClauses, 0, sizeof(Tcl_Obj *) * numHandlers);
	matchCodes = TclStackAlloc(interp, sizeof(int) * numHandlers);
	resultVarIndices = TclStackAlloc(interp, sizeof(int) * numHandlers);
	optionVarIndices = TclStackAlloc(interp, sizeof(int) * numHandlers);

	for (i=0 ; i<numHandlers ; i++) {
	    Tcl_Obj *tmpObj, **objv;
	    int objc;

	    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
		goto failedToCompile;
	    }
	    if (tokenPtr[1].size == 4
		    && !strncmp(tokenPtr[1].start, "trap", 4)) {
		/*
		 * Parse the list of errorCode words to match against.
		 */

		matchCodes[i] = TCL_ERROR;
		tokenPtr = TokenAfter(tokenPtr);
		TclNewObj(tmpObj);
		Tcl_IncrRefCount(tmpObj);
		if (!TclWordKnownAtCompileTime(tokenPtr, tmpObj)
			|| Tcl_ListObjLength(NULL, tmpObj, &objc) != TCL_OK
			|| (objc == 0)) {
		    TclDecrRefCount(tmpObj);
		    goto failedToCompile;
		}
		Tcl_ListObjReplace(NULL, tmpObj, 0, 0, 0, NULL);
		matchClauses[i] = tmpObj;
	    } else if (tokenPtr[1].size == 2
		    && !strncmp(tokenPtr[1].start, "on", 2)) {
		int code;

		/*
		 * Parse the result code to look for.
		 */

		tokenPtr = TokenAfter(tokenPtr);
		TclNewObj(tmpObj);
		Tcl_IncrRefCount(tmpObj);
		if (!TclWordKnownAtCompileTime(tokenPtr, tmpObj)) {
		    TclDecrRefCount(tmpObj);
		    goto failedToCompile;
		}
		if (TCL_ERROR == TclGetCompletionCodeFromObj(NULL, tmpObj, &code)) {
		    TclDecrRefCount(tmpObj);
		    goto failedToCompile;
		}
		matchCodes[i] = code;
		TclDecrRefCount(tmpObj);
	    } else {
		goto failedToCompile;
	    }

	    /*
	     * Parse the variable binding.
	     */

	    tokenPtr = TokenAfter(tokenPtr);
	    TclNewObj(tmpObj);
	    Tcl_IncrRefCount(tmpObj);
	    if (!TclWordKnownAtCompileTime(tokenPtr, tmpObj)) {
		TclDecrRefCount(tmpObj);
		goto failedToCompile;
	    }
	    if (Tcl_ListObjGetElements(NULL, tmpObj, &objc, &objv) != TCL_OK
		    || (objc > 2)) {
		TclDecrRefCount(tmpObj);
		goto failedToCompile;
	    }
	    if (objc > 0) {
		int len;
		const char *varname = Tcl_GetStringFromObj(objv[0], &len);

		resultVarIndices[i] = LocalScalar(varname, len, envPtr);
		if (resultVarIndices[i] < 0) {
		    TclDecrRefCount(tmpObj);
		    goto failedToCompile;
		}
	    } else {
		resultVarIndices[i] = -1;
	    }
	    if (objc == 2) {
		int len;
		const char *varname = Tcl_GetStringFromObj(objv[1], &len);

		optionVarIndices[i] = LocalScalar(varname, len, envPtr);
		if (optionVarIndices[i] < 0) {
		    TclDecrRefCount(tmpObj);
		    goto failedToCompile;
		}
	    } else {
		optionVarIndices[i] = -1;
	    }
	    TclDecrRefCount(tmpObj);

	    /*
	     * Extract the body for this handler.
	     */

	    tokenPtr = TokenAfter(tokenPtr);
	    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
		goto failedToCompile;
	    }
	    if (tokenPtr[1].size == 1 && tokenPtr[1].start[0] == '-') {
		handlerTokens[i] = NULL;
	    } else {
		handlerTokens[i] = tokenPtr;
	    }

	    tokenPtr = TokenAfter(tokenPtr);
	}

	if (handlerTokens[numHandlers-1] == NULL) {
	    goto failedToCompile;
	}
    }

    /*
     * Parse the finally clause
     */

    if (numWords == 0) {
	finallyToken = NULL;
    } else if (numWords == 2) {
	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD || tokenPtr[1].size != 7
		|| strncmp(tokenPtr[1].start, "finally", 7)) {
	    goto failedToCompile;
	}
	finallyToken = TokenAfter(tokenPtr);
    } else {
	goto failedToCompile;
    }

    /*
     * Issue the bytecode.
     */

    if (!finallyToken) {
	result = IssueTryClausesInstructions(interp, envPtr, bodyToken,
		numHandlers, matchCodes, matchClauses, resultVarIndices,
		optionVarIndices, handlerTokens);
    } else if (numHandlers == 0) {
	result = IssueTryFinallyInstructions(interp, envPtr, bodyToken,
		finallyToken);
    } else {
	result = IssueTryClausesFinallyInstructions(interp, envPtr, bodyToken,
		numHandlers, matchCodes, matchClauses, resultVarIndices,
		optionVarIndices, handlerTokens, finallyToken);
    }

    /*
     * Delete any temporary state and finish off.
     */

  failedToCompile:
    if (numHandlers > 0) {
	for (i=0 ; i<numHandlers ; i++) {
	    if (matchClauses[i]) {
		TclDecrRefCount(matchClauses[i]);
	    }
	}
	TclStackFree(interp, optionVarIndices);
	TclStackFree(interp, resultVarIndices);
	TclStackFree(interp, matchCodes);
	TclStackFree(interp, matchClauses);
	TclStackFree(interp, handlerTokens);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * IssueTryClausesInstructions, IssueTryClausesFinallyInstructions,
 * IssueTryFinallyInstructions --
 *
 *	The code generators for [try]. Split from the parsing engine for
 *	reasons of developer sanity, and also split between no-finally,
 *	just-finally and with-finally cases because so many of the details of
 *	generation vary between the three.
 *
 *	The macros below make the instruction issuing easier to follow.
 *
 *----------------------------------------------------------------------
 */

static int
IssueTryClausesInstructions(
    Tcl_Interp *interp,
    CompileEnv *envPtr,
    Tcl_Token *bodyToken,
    int numHandlers,
    int *matchCodes,
    Tcl_Obj **matchClauses,
    int *resultVars,
    int *optionVars,
    Tcl_Token **handlerTokens)
{
    DefineLineInformation;	/* TIP #280 */
    int range, resultVar, optionsVar;
    int i, j, len, forwardsNeedFixing = 0, trapZero = 0, afterBody = 0;
    int *addrsToFix, *forwardsToFix, notCodeJumpSource, notECJumpSource;
    int *noError;
    char buf[TCL_INTEGER_SPACE];

    resultVar = AnonymousLocal(envPtr);
    optionsVar = AnonymousLocal(envPtr);
    if (resultVar < 0 || optionsVar < 0) {
	return TCL_ERROR;
    }

    /*
     * Check if we're supposed to trap a normal TCL_OK completion of the body.
     * If not, we can handle that case much more efficiently.
     */

    for (i=0 ; i<numHandlers ; i++) {
	if (matchCodes[i] == 0) {
	    trapZero = 1;
	    break;
	}
    }

    /*
     * Compile the body, trapping any error in it so that we can trap on it
     * and/or run a finally clause. Note that there must be at least one
     * on/trap clause; when none is present, this whole function is not called
     * (and it's never called when there's a finally clause).
     */

    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    OP4(				BEGIN_CATCH4, range);
    ExceptionRangeStarts(envPtr, range);
    BODY(				bodyToken, 1);
    ExceptionRangeEnds(envPtr, range);
    if (!trapZero) {
	OP(				END_CATCH);
	JUMP4(				JUMP, afterBody);
	TclAdjustStackDepth(-1, envPtr);
    } else {
	PUSH(				"0");
	OP4(				REVERSE, 2);
	OP1(				JUMP1, 4);
	TclAdjustStackDepth(-2, envPtr);
    }
    ExceptionRangeTarget(envPtr, range, catchOffset);
    OP(					PUSH_RETURN_CODE);
    OP(					PUSH_RESULT);
    OP(					PUSH_RETURN_OPTIONS);
    OP(					END_CATCH);
    STORE(				optionsVar);
    OP(					POP);
    STORE(				resultVar);
    OP(					POP);

    /*
     * Now we handle all the registered 'on' and 'trap' handlers in order.
     * For us to be here, there must be at least one handler.
     *
     * Slight overallocation, but reduces size of this function.
     */

    addrsToFix = TclStackAlloc(interp, sizeof(int)*numHandlers);
    forwardsToFix = TclStackAlloc(interp, sizeof(int)*numHandlers);
    noError = TclStackAlloc(interp, sizeof(int)*numHandlers);

    for (i=0 ; i<numHandlers ; i++) {
	noError[i] = -1;
	sprintf(buf, "%d", matchCodes[i]);
	OP(				DUP);
	PushLiteral(envPtr, buf, strlen(buf));
	OP(				EQ);
	JUMP4(				JUMP_FALSE, notCodeJumpSource);
	if (matchClauses[i]) {
	    const char *p;
	    Tcl_ListObjLength(NULL, matchClauses[i], &len);

	    /*
	     * Match the errorcode according to try/trap rules.
	     */

	    LOAD(			optionsVar);
	    PUSH(			"-errorcode");
	    OP4(			DICT_GET, 1);
	    TclAdjustStackDepth(-1, envPtr);
	    OP44(			LIST_RANGE_IMM, 0, len-1);
	    p = Tcl_GetStringFromObj(matchClauses[i], &len);
	    PushLiteral(envPtr, p, len);
	    OP(				STR_EQ);
	    JUMP4(			JUMP_FALSE, notECJumpSource);
	} else {
	    notECJumpSource = -1; /* LINT */
	}
	OP(				POP);

	/*
	 * There is no finally clause, so we can avoid wrapping a catch
	 * context around the handler. That simplifies what instructions need
	 * to be issued a lot since we can let errors just fall through.
	 */

	if (resultVars[i] >= 0) {
	    LOAD(			resultVar);
	    STORE(			resultVars[i]);
	    OP(				POP);
	    if (optionVars[i] >= 0) {
		LOAD(			optionsVar);
		STORE(			optionVars[i]);
		OP(			POP);
	    }
	}
	if (!handlerTokens[i]) {
	    forwardsNeedFixing = 1;
	    JUMP4(			JUMP, forwardsToFix[i]);
	    TclAdjustStackDepth(1, envPtr);
	} else {
	    int dontChangeOptions;

	    forwardsToFix[i] = -1;
	    if (forwardsNeedFixing) {
		forwardsNeedFixing = 0;
		for (j=0 ; j<i ; j++) {
		    if (forwardsToFix[j] == -1) {
			continue;
		    }
		    FIXJUMP4(forwardsToFix[j]);
		    forwardsToFix[j] = -1;
		}
	    }
	    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
	    OP4(			BEGIN_CATCH4, range);
	    ExceptionRangeStarts(envPtr, range);
	    BODY(			handlerTokens[i], 5+i*4);
	    ExceptionRangeEnds(envPtr, range);
	    OP(				END_CATCH);
	    JUMP4(			JUMP, noError[i]);
	    ExceptionRangeTarget(envPtr, range, catchOffset);
	    TclAdjustStackDepth(-1, envPtr);
	    OP(				PUSH_RESULT);
	    OP(				PUSH_RETURN_OPTIONS);
	    OP(				PUSH_RETURN_CODE);
	    OP(				END_CATCH);
	    PUSH(			"1");
	    OP(				EQ);
	    JUMP1(			JUMP_FALSE, dontChangeOptions);
	    LOAD(			optionsVar);
	    OP4(			REVERSE, 2);
	    STORE(			optionsVar);
	    OP(				POP);
	    PUSH(			"-during");
	    OP4(			REVERSE, 2);
	    OP44(			DICT_SET, 1, optionsVar);
	    TclAdjustStackDepth(-1, envPtr);
	    FIXJUMP1(		dontChangeOptions);
	    OP4(			REVERSE, 2);
	    INVOKE(			RETURN_STK);
	}

	JUMP4(				JUMP, addrsToFix[i]);
	if (matchClauses[i]) {
	    FIXJUMP4(	notECJumpSource);
	}
	FIXJUMP4(	notCodeJumpSource);
    }

    /*
     * Drop the result code since it didn't match any clause, and reissue the
     * exception. Note also that INST_RETURN_STK can proceed to the next
     * instruction.
     */

    OP(					POP);
    LOAD(				optionsVar);
    LOAD(				resultVar);
    INVOKE(				RETURN_STK);

    /*
     * Fix all the jumps from taken clauses to here (which is the end of the
     * [try]).
     */

    if (!trapZero) {
	FIXJUMP4(afterBody);
    }
    for (i=0 ; i<numHandlers ; i++) {
	FIXJUMP4(addrsToFix[i]);
	if (noError[i] != -1) {
	    FIXJUMP4(noError[i]);
	}
    }
    TclStackFree(interp, noError);
    TclStackFree(interp, forwardsToFix);
    TclStackFree(interp, addrsToFix);
    return TCL_OK;
}

static int
IssueTryClausesFinallyInstructions(
    Tcl_Interp *interp,
    CompileEnv *envPtr,
    Tcl_Token *bodyToken,
    int numHandlers,
    int *matchCodes,
    Tcl_Obj **matchClauses,
    int *resultVars,
    int *optionVars,
    Tcl_Token **handlerTokens,
    Tcl_Token *finallyToken)	/* Not NULL */
{
    DefineLineInformation;	/* TIP #280 */
    int range, resultVar, optionsVar, i, j, len, forwardsNeedFixing = 0;
    int trapZero = 0, afterBody = 0, finalOK, finalError, noFinalError;
    int *addrsToFix, *forwardsToFix, notCodeJumpSource, notECJumpSource;
    char buf[TCL_INTEGER_SPACE];

    resultVar = AnonymousLocal(envPtr);
    optionsVar = AnonymousLocal(envPtr);
    if (resultVar < 0 || optionsVar < 0) {
	return TCL_ERROR;
    }

    /*
     * Check if we're supposed to trap a normal TCL_OK completion of the body.
     * If not, we can handle that case much more efficiently.
     */

    for (i=0 ; i<numHandlers ; i++) {
	if (matchCodes[i] == 0) {
	    trapZero = 1;
	    break;
	}
    }

    /*
     * Compile the body, trapping any error in it so that we can trap on it
     * (if any trap matches) and run a finally clause.
     */

    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    OP4(				BEGIN_CATCH4, range);
    ExceptionRangeStarts(envPtr, range);
    BODY(				bodyToken, 1);
    ExceptionRangeEnds(envPtr, range);
    if (!trapZero) {
	OP(				END_CATCH);
	STORE(				resultVar);
	OP(				POP);
	PUSH(				"-level 0 -code 0");
	STORE(				optionsVar);
	OP(				POP);
	JUMP4(				JUMP, afterBody);
    } else {
	PUSH(				"0");
	OP4(				REVERSE, 2);
	OP1(				JUMP1, 4);
	TclAdjustStackDepth(-2, envPtr);
    }
    ExceptionRangeTarget(envPtr, range, catchOffset);
    OP(					PUSH_RETURN_CODE);
    OP(					PUSH_RESULT);
    OP(					PUSH_RETURN_OPTIONS);
    OP(					END_CATCH);
    STORE(				optionsVar);
    OP(					POP);
    STORE(				resultVar);
    OP(					POP);

    /*
     * Now we handle all the registered 'on' and 'trap' handlers in order.
     *
     * Slight overallocation, but reduces size of this function.
     */

    addrsToFix = TclStackAlloc(interp, sizeof(int)*numHandlers);
    forwardsToFix = TclStackAlloc(interp, sizeof(int)*numHandlers);

    for (i=0 ; i<numHandlers ; i++) {
	int noTrapError, trapError;
	const char *p;

	sprintf(buf, "%d", matchCodes[i]);
	OP(				DUP);
	PushLiteral(envPtr, buf, strlen(buf));
	OP(				EQ);
	JUMP4(				JUMP_FALSE, notCodeJumpSource);
	if (matchClauses[i]) {
	    Tcl_ListObjLength(NULL, matchClauses[i], &len);

	    /*
	     * Match the errorcode according to try/trap rules.
	     */

	    LOAD(			optionsVar);
	    PUSH(			"-errorcode");
	    OP4(			DICT_GET, 1);
	    TclAdjustStackDepth(-1, envPtr);
	    OP44(			LIST_RANGE_IMM, 0, len-1);
	    p = Tcl_GetStringFromObj(matchClauses[i], &len);
	    PushLiteral(envPtr, p, len);
	    OP(				STR_EQ);
	    JUMP4(			JUMP_FALSE, notECJumpSource);
	} else {
	    notECJumpSource = -1; /* LINT */
	}
	OP(				POP);

	/*
	 * There is a finally clause, so we need a fairly complex sequence of
	 * instructions to deal with an on/trap handler because we must call
	 * the finally handler *and* we need to substitute the result from a
	 * failed trap for the result from the main script.
	 */

	if (resultVars[i] >= 0 || handlerTokens[i]) {
	    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
	    OP4(			BEGIN_CATCH4, range);
	    ExceptionRangeStarts(envPtr, range);
	}
	if (resultVars[i] >= 0) {
	    LOAD(			resultVar);
	    STORE(			resultVars[i]);
	    OP(				POP);
	    if (optionVars[i] >= 0) {
		LOAD(			optionsVar);
		STORE(			optionVars[i]);
		OP(			POP);
	    }

	    if (!handlerTokens[i]) {
		/*
		 * No handler. Will not be the last handler (that is a
		 * condition that is checked by the caller). Chain to the next
		 * one.
		 */

		ExceptionRangeEnds(envPtr, range);
		OP(			END_CATCH);
		forwardsNeedFixing = 1;
		JUMP4(			JUMP, forwardsToFix[i]);
		goto finishTrapCatchHandling;
	    }
	} else if (!handlerTokens[i]) {
	    /*
	     * No handler. Will not be the last handler (that condition is
	     * checked by the caller). Chain to the next one.
	     */

	    forwardsNeedFixing = 1;
	    JUMP4(			JUMP, forwardsToFix[i]);
	    goto endOfThisArm;
	}

	/*
	 * Got a handler. Make sure that any pending patch-up actions from
	 * previous unprocessed handlers are dealt with now that we know where
	 * they are to jump to.
	 */

	if (forwardsNeedFixing) {
	    forwardsNeedFixing = 0;
	    OP1(			JUMP1, 7);
	    for (j=0 ; j<i ; j++) {
		if (forwardsToFix[j] == -1) {
		    continue;
		}
		FIXJUMP4(	forwardsToFix[j]);
		forwardsToFix[j] = -1;
	    }
	    OP4(			BEGIN_CATCH4, range);
	}
	BODY(				handlerTokens[i], 5+i*4);
	ExceptionRangeEnds(envPtr, range);
	PUSH(				"0");
	OP(				PUSH_RETURN_OPTIONS);
	OP4(				REVERSE, 3);
	OP1(				JUMP1, 5);
	TclAdjustStackDepth(-3, envPtr);
	forwardsToFix[i] = -1;

	/*
	 * Error in handler or setting of variables; replace the stored
	 * exception with the new one. Note that we only push this if we have
	 * either a body or some variable setting here. Otherwise this code is
	 * unreachable.
	 */

    finishTrapCatchHandling:
	ExceptionRangeTarget(envPtr, range, catchOffset);
	OP(				PUSH_RETURN_OPTIONS);
	OP(				PUSH_RETURN_CODE);
	OP(				PUSH_RESULT);
	OP(				END_CATCH);
	STORE(				resultVar);
	OP(				POP);
	PUSH(				"1");
	OP(				EQ);
	JUMP1(				JUMP_FALSE, noTrapError);
	LOAD(				optionsVar);
	PUSH(				"-during");
	OP4(				REVERSE, 3);
	STORE(				optionsVar);
	OP(				POP);
	OP44(				DICT_SET, 1, optionsVar);
	TclAdjustStackDepth(-1, envPtr);
	JUMP1(				JUMP, trapError);
	FIXJUMP1(		noTrapError);
	STORE(				optionsVar);
	FIXJUMP1(		trapError);
	/* Skip POP at end; can clean up with subsequent POP */
	if (i+1 < numHandlers) {
	    OP(				POP);
	}

    endOfThisArm:
	if (i+1 < numHandlers) {
	    JUMP4(			JUMP, addrsToFix[i]);
	    TclAdjustStackDepth(1, envPtr);
	}
	if (matchClauses[i]) {
	    FIXJUMP4(		notECJumpSource);
	}
	FIXJUMP4(		notCodeJumpSource);
    }

    /*
     * Drop the result code, and fix all the jumps from taken clauses - which
     * drop the result code as their first action - to point straight after
     * (i.e., to the start of the finally clause).
     */

    OP(					POP);
    for (i=0 ; i<numHandlers-1 ; i++) {
	FIXJUMP4(		addrsToFix[i]);
    }
    TclStackFree(interp, forwardsToFix);
    TclStackFree(interp, addrsToFix);

    /*
     * Process the finally clause (at last!) Note that we do not wrap this in
     * error handlers because we would just rethrow immediately anyway. Then
     * (on normal success) we reissue the exception. Note also that
     * INST_RETURN_STK can proceed to the next instruction; that'll be the
     * next command (or some inter-command manipulation).
     */

    if (!trapZero) {
	FIXJUMP4(		afterBody);
    }
    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    OP4(				BEGIN_CATCH4, range);
    ExceptionRangeStarts(envPtr, range);
    BODY(				finallyToken, 3 + 4*numHandlers);
    ExceptionRangeEnds(envPtr, range);
    OP(					END_CATCH);
    OP(					POP);
    JUMP1(				JUMP, finalOK);
    ExceptionRangeTarget(envPtr, range, catchOffset);
    OP(					PUSH_RESULT);
    OP(					PUSH_RETURN_OPTIONS);
    OP(					PUSH_RETURN_CODE);
    OP(					END_CATCH);
    PUSH(				"1");
    OP(					EQ);
    JUMP1(				JUMP_FALSE, noFinalError);
    LOAD(				optionsVar);
    PUSH(				"-during");
    OP4(				REVERSE, 3);
    STORE(				optionsVar);
    OP(					POP);
    OP44(				DICT_SET, 1, optionsVar);
    TclAdjustStackDepth(-1, envPtr);
    OP(					POP);
    JUMP1(				JUMP, finalError);
    TclAdjustStackDepth(1, envPtr);
    FIXJUMP1(			noFinalError);
    STORE(				optionsVar);
    OP(					POP);
    FIXJUMP1(			finalError);
    STORE(				resultVar);
    OP(					POP);
    FIXJUMP1(			finalOK);
    LOAD(				optionsVar);
    LOAD(				resultVar);
    INVOKE(				RETURN_STK);

    return TCL_OK;
}

static int
IssueTryFinallyInstructions(
    Tcl_Interp *interp,
    CompileEnv *envPtr,
    Tcl_Token *bodyToken,
    Tcl_Token *finallyToken)
{
    DefineLineInformation;	/* TIP #280 */
    int range, jumpOK, jumpSplice;

    /*
     * Note that this one is simple enough that we can issue it without
     * needing a local variable table, making it a universal compilation.
     */

    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    OP4(				BEGIN_CATCH4, range);
    ExceptionRangeStarts(envPtr, range);
    BODY(				bodyToken, 1);
    ExceptionRangeEnds(envPtr, range);
    OP1(				JUMP1, 3);
    TclAdjustStackDepth(-1, envPtr);
    ExceptionRangeTarget(envPtr, range, catchOffset);
    OP(					PUSH_RESULT);
    OP(					PUSH_RETURN_OPTIONS);
    OP(					END_CATCH);

    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    OP4(				BEGIN_CATCH4, range);
    ExceptionRangeStarts(envPtr, range);
    BODY(				finallyToken, 3);
    ExceptionRangeEnds(envPtr, range);
    OP(					END_CATCH);
    OP(					POP);
    JUMP1(				JUMP, jumpOK);
    ExceptionRangeTarget(envPtr, range, catchOffset);
    OP(					PUSH_RESULT);
    OP(					PUSH_RETURN_OPTIONS);
    OP(					PUSH_RETURN_CODE);
    OP(					END_CATCH);
    PUSH(				"1");
    OP(					EQ);
    JUMP1(				JUMP_FALSE, jumpSplice);
    PUSH(				"-during");
    OP4(				OVER, 3);
    OP4(				LIST, 2);
    OP(					LIST_CONCAT);
    FIXJUMP1(		jumpSplice);
    OP4(				REVERSE, 4);
    OP(					POP);
    OP(					POP);
    OP1(				JUMP1, 7);
    FIXJUMP1(		jumpOK);
    OP4(				REVERSE, 2);
    INVOKE(				RETURN_STK);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileUnsetCmd --
 *
 *	Procedure called to compile the "unset" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "unset" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileUnsetCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *varTokenPtr;
    int isScalar, localIndex, flags = 1, i, varCount = 0, haveFlags = 0;
    DefineLineInformation;	/* TIP #280 */

    /* TODO: Consider support for compiling expanded args. */

    /*
     * Verify that all words - except the first non-option one - are known at
     * compile time so that we can handle them without needing to do a nasty
     * push/rotate. [Bug 3970f54c4e]
     */

    for (i=1,varTokenPtr=parsePtr->tokenPtr ; i<parsePtr->numWords ; i++) {
	Tcl_Obj *leadingWord = Tcl_NewObj();

	varTokenPtr = TokenAfter(varTokenPtr);
	if (!TclWordKnownAtCompileTime(varTokenPtr, leadingWord)) {
	    TclDecrRefCount(leadingWord);

	    /*
	     * We can tolerate non-trivial substitutions in the first variable
	     * to be unset. If a '--' or '-nocomplain' was present, anything
	     * goes in that one place! (All subsequent variable names must be
	     * constants since we don't want to have to push them all first.)
	     */

	    if (varCount == 0) {
		if (haveFlags) {
		    continue;
		}

		/*
		 * In fact, we're OK as long as we're the first argument *and*
		 * we provably don't start with a '-'. If that is true, then
		 * even if everything else is varying, we still can't be a
		 * flag. Otherwise we'll spill to runtime to place a limit on
		 * the trickiness.
		 */

		if (varTokenPtr->type == TCL_TOKEN_WORD
			&& varTokenPtr[1].type == TCL_TOKEN_TEXT
			&& varTokenPtr[1].size > 0
			&& varTokenPtr[1].start[0] != '-') {
		    continue;
		}
	    }
	    return TCL_ERROR;
	}
	if (varCount == 0) {
	    const char *bytes;
	    int len;

	    bytes = Tcl_GetStringFromObj(leadingWord, &len);
	    if (i == 1 && len == 11 && !strncmp("-nocomplain", bytes, 11)) {
		flags = 0;
		haveFlags++;
	    } else if (i == (2 - flags) && len == 2 && !strncmp("--", bytes, 2)) {
		haveFlags++;
	    } else {
		varCount++;
	    }
	} else {
	    varCount++;
	}
	TclDecrRefCount(leadingWord);
    }

    /*
     * Issue instructions to unset each of the named variables.
     */

    varTokenPtr = TokenAfter(parsePtr->tokenPtr);
    for (i=0; i<haveFlags;i++) {
	varTokenPtr = TokenAfter(varTokenPtr);
    }
    for (i=1+haveFlags ; i<parsePtr->numWords ; i++) {
	/*
	 * Decide if we can use a frame slot for the var/array name or if we
	 * need to emit code to compute and push the name at runtime. We use a
	 * frame slot (entry in the array of local vars) if we are compiling a
	 * procedure body and if the name is simple text that does not include
	 * namespace qualifiers.
	 */

	PushVarNameWord(interp, varTokenPtr, envPtr, 0,
		&localIndex, &isScalar, i);

	/*
	 * Emit instructions to unset the variable.
	 */

	if (isScalar) {
	    if (localIndex < 0) {
		OP1(	UNSET_STK, flags);
	    } else {
		OP14(	UNSET_SCALAR, flags, localIndex);
	    }
	} else {
	    if (localIndex < 0) {
		OP1(	UNSET_ARRAY_STK, flags);
	    } else {
		OP14(	UNSET_ARRAY, flags, localIndex);
	    }
	}

	varTokenPtr = TokenAfter(varTokenPtr);
    }
    PUSH("");
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileWhileCmd --
 *
 *	Procedure called to compile the "while" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "while" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileWhileCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *testTokenPtr, *bodyTokenPtr;
    JumpFixup jumpEvalCondFixup;
    int testCodeOffset, bodyCodeOffset, jumpDist, range, code, boolVal;
    int loopMayEnd = 1;		/* This is set to 0 if it is recognized as an
				 * infinite loop. */
    Tcl_Obj *boolObj;
    DefineLineInformation;	/* TIP #280 */

    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }

    /*
     * If the test expression requires substitutions, don't compile the while
     * command inline. E.g., the expression might cause the loop to never
     * execute or execute forever, as in "while "$x < 5" {}".
     *
     * Bail out also if the body expression requires substitutions in order to
     * insure correct behaviour [Bug 219166]
     */

    testTokenPtr = TokenAfter(parsePtr->tokenPtr);
    bodyTokenPtr = TokenAfter(testTokenPtr);

    if ((testTokenPtr->type != TCL_TOKEN_SIMPLE_WORD)
	    || (bodyTokenPtr->type != TCL_TOKEN_SIMPLE_WORD)) {
	return TCL_ERROR;
    }

    /*
     * Find out if the condition is a constant.
     */

    boolObj = Tcl_NewStringObj(testTokenPtr[1].start, testTokenPtr[1].size);
    Tcl_IncrRefCount(boolObj);
    code = Tcl_GetBooleanFromObj(NULL, boolObj, &boolVal);
    TclDecrRefCount(boolObj);
    if (code == TCL_OK) {
	if (boolVal) {
	    /*
	     * It is an infinite loop; flag it so that we generate a more
	     * efficient body.
	     */

	    loopMayEnd = 0;
	} else {
	    /*
	     * This is an empty loop: "while 0 {...}" or such. Compile no
	     * bytecodes.
	     */

	    goto pushResult;
	}
    }

    /*
     * Create a ExceptionRange record for the loop body. This is used to
     * implement break and continue.
     */

    range = TclCreateExceptRange(LOOP_EXCEPTION_RANGE, envPtr);

    /*
     * Jump to the evaluation of the condition. This code uses the "loop
     * rotation" optimisation (which eliminates one branch from the loop).
     * "while cond body" produces then:
     *       goto A
     *    B: body                : bodyCodeOffset
     *    A: cond -> result      : testCodeOffset, continueOffset
     *       if (result) goto B
     *
     * The infinite loop "while 1 body" produces:
     *    B: body                : all three offsets here
     *       goto B
     */

    if (loopMayEnd) {
	TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP,
		&jumpEvalCondFixup);
	testCodeOffset = 0;	/* Avoid compiler warning. */
    } else {
	/*
	 * Make sure that the first command in the body is preceded by an
	 * INST_START_CMD, and hence counted properly. [Bug 1752146]
	 */

	envPtr->atCmdStart &= ~1;
	testCodeOffset = CurrentOffset(envPtr);
    }

    /*
     * Compile the loop body.
     */

    bodyCodeOffset = ExceptionRangeStarts(envPtr, range);
    if (!loopMayEnd) {
	envPtr->exceptArrayPtr[range].continueOffset = testCodeOffset;
	envPtr->exceptArrayPtr[range].codeOffset = bodyCodeOffset;
    }
    BODY(bodyTokenPtr, 2);
    ExceptionRangeEnds(envPtr, range);
    OP(		POP);

    /*
     * Compile the test expression then emit the conditional jump that
     * terminates the while. We already know it's a simple word.
     */

    if (loopMayEnd) {
	testCodeOffset = CurrentOffset(envPtr);
	jumpDist = testCodeOffset - jumpEvalCondFixup.codeOffset;
	if (TclFixupForwardJump(envPtr, &jumpEvalCondFixup, jumpDist, 127)) {
	    bodyCodeOffset += 3;
	    testCodeOffset += 3;
	}
	SetLineInformation(1);
	TclCompileExprWords(interp, testTokenPtr, 1, envPtr);

	jumpDist = CurrentOffset(envPtr) - bodyCodeOffset;
	if (jumpDist > 127) {
	    TclEmitInstInt4(INST_JUMP_TRUE4, -jumpDist, envPtr);
	} else {
	    TclEmitInstInt1(INST_JUMP_TRUE1, -jumpDist, envPtr);
	}
    } else {
	jumpDist = CurrentOffset(envPtr) - bodyCodeOffset;
	if (jumpDist > 127) {
	    TclEmitInstInt4(INST_JUMP4, -jumpDist, envPtr);
	} else {
	    TclEmitInstInt1(INST_JUMP1, -jumpDist, envPtr);
	}
    }

    /*
     * Set the loop's body, continue and break offsets.
     */

    envPtr->exceptArrayPtr[range].continueOffset = testCodeOffset;
    envPtr->exceptArrayPtr[range].codeOffset = bodyCodeOffset;
    ExceptionRangeTarget(envPtr, range, breakOffset);
    TclFinalizeLoopExceptionRange(envPtr, range);

    /*
     * The while command's result is an empty string.
     */

  pushResult:
    PUSH("");
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileYieldCmd --
 *
 *	Procedure called to compile the "yield" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "yield" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileYieldCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    if (parsePtr->numWords < 1 || parsePtr->numWords > 2) {
	return TCL_ERROR;
    }

    if (parsePtr->numWords == 1) {
	PUSH("");
    } else {
	DefineLineInformation;	/* TIP #280 */
	Tcl_Token *valueTokenPtr = TokenAfter(parsePtr->tokenPtr);

	CompileWord(envPtr, valueTokenPtr, interp, 1);
    }
    OP(		YIELD);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileYieldToCmd --
 *
 *	Procedure called to compile the "yieldto" command.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "yieldto" command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileYieldToCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    DefineLineInformation;	/* TIP #280 */
    Tcl_Token *tokenPtr = TokenAfter(parsePtr->tokenPtr);
    int i;

    if (parsePtr->numWords < 2) {
	return TCL_ERROR;
    }

    OP(		NS_CURRENT);
    for (i = 1 ; i < parsePtr->numWords ; i++) {
	CompileWord(envPtr, tokenPtr, interp, i);
	tokenPtr = TokenAfter(tokenPtr);
    }
    OP4(	LIST, i);
    OP(		YIELD_TO_INVOKE);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CompileUnaryOpCmd --
 *
 *	Utility routine to compile the unary operator commands.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the compiled command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

static int
CompileUnaryOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    int instruction,
    CompileEnv *envPtr)
{
    Tcl_Token *tokenPtr;
    DefineLineInformation;	/* TIP #280 */

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    CompileWord(envPtr, tokenPtr, interp, 1);
    TclEmitOpcode(instruction, envPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CompileAssociativeBinaryOpCmd --
 *
 *	Utility routine to compile the binary operator commands that accept an
 *	arbitrary number of arguments, and that are associative operations.
 *	Because of the associativity, we may combine operations from right to
 *	left, saving us any effort of re-ordering the arguments on the stack
 *	after substitutions are completed.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the compiled command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

static int
CompileAssociativeBinaryOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    const char *identity,
    int instruction,
    CompileEnv *envPtr)
{
    Tcl_Token *tokenPtr = parsePtr->tokenPtr;
    DefineLineInformation;	/* TIP #280 */
    int words;

    /* TODO: Consider support for compiling expanded args. */
    for (words=1 ; words<parsePtr->numWords ; words++) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, words);
    }
    if (parsePtr->numWords <= 2) {
	PushLiteral(envPtr, identity, -1);
	words++;
    }
    if (words > 3) {
	/*
	 * Reverse order of arguments to get precise agreement with [expr] in
	 * calcuations, including roundoff errors.
	 */

	OP4(	REVERSE, words-1);
    }
    while (--words > 1) {
	TclEmitOpcode(instruction, envPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CompileStrictlyBinaryOpCmd --
 *
 *	Utility routine to compile the binary operator commands, that strictly
 *	accept exactly two arguments.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the compiled command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

static int
CompileStrictlyBinaryOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    int instruction,
    CompileEnv *envPtr)
{
    if (parsePtr->numWords != 3) {
	return TCL_ERROR;
    }
    return CompileAssociativeBinaryOpCmd(interp, parsePtr,
	    NULL, instruction, envPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * CompileComparisonOpCmd --
 *
 *	Utility routine to compile the n-ary comparison operator commands.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the compiled command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

static int
CompileComparisonOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    int instruction,
    CompileEnv *envPtr)
{
    Tcl_Token *tokenPtr;
    DefineLineInformation;	/* TIP #280 */

    /* TODO: Consider support for compiling expanded args. */
    if (parsePtr->numWords < 3) {
	PUSH("1");
    } else if (parsePtr->numWords == 3) {
	tokenPtr = TokenAfter(parsePtr->tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, 1);
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, 2);
	TclEmitOpcode(instruction, envPtr);
    } else if (envPtr->procPtr == NULL) {
	/*
	 * No local variable space!
	 */

	return TCL_ERROR;
    } else {
	int tmpIndex = AnonymousLocal(envPtr);
	int words;

	tokenPtr = TokenAfter(parsePtr->tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, 1);
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, 2);
	STORE(tmpIndex);
	TclEmitOpcode(instruction, envPtr);
	for (words=3 ; words<parsePtr->numWords ;) {
	    LOAD(tmpIndex);
	    tokenPtr = TokenAfter(tokenPtr);
	    CompileWord(envPtr, tokenPtr, interp, words);
	    if (++words < parsePtr->numWords) {
		STORE(tmpIndex);
	    }
	    TclEmitOpcode(instruction, envPtr);
	}
	for (; words>3 ; words--) {
	    OP(	BITAND);
	}

	/*
	 * Drop the value from the temp variable; retaining that reference
	 * might be expensive elsewhere.
	 */

	OP14(	UNSET_SCALAR, 0, tmpIndex);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompile*OpCmd --
 *
 *	Procedures called to compile the corresponding "::tcl::mathop::*"
 *	commands. These are all wrappers around the utility operator command
 *	compiler functions, except for the compilers for subtraction and
 *	division, which are special.
 *
 * Results:
 *	Returns TCL_OK for a successful compile. Returns TCL_ERROR to defer
 *	evaluation to runtime.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the compiled command at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileInvertOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileUnaryOpCmd(interp, parsePtr, INST_BITNOT, envPtr);
}

int
TclCompileNotOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileUnaryOpCmd(interp, parsePtr, INST_LNOT, envPtr);
}

int
TclCompileAddOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileAssociativeBinaryOpCmd(interp, parsePtr, "0", INST_ADD,
	    envPtr);
}

int
TclCompileMulOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileAssociativeBinaryOpCmd(interp, parsePtr, "1", INST_MULT,
	    envPtr);
}

int
TclCompileAndOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileAssociativeBinaryOpCmd(interp, parsePtr, "-1", INST_BITAND,
	    envPtr);
}

int
TclCompileOrOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileAssociativeBinaryOpCmd(interp, parsePtr, "0", INST_BITOR,
	    envPtr);
}

int
TclCompileXorOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileAssociativeBinaryOpCmd(interp, parsePtr, "0", INST_BITXOR,
	    envPtr);
}

int
TclCompilePowOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    /*
     * This one has its own implementation because the ** operator is the only
     * one with right associativity.
     */

    Tcl_Token *tokenPtr = parsePtr->tokenPtr;
    DefineLineInformation;	/* TIP #280 */
    int words;

    for (words=1 ; words<parsePtr->numWords ; words++) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, words);
    }
    if (parsePtr->numWords <= 2) {
	PUSH("1");
	words++;
    }
    while (--words > 1) {
	TclEmitOpcode(INST_EXPON, envPtr);
    }
    return TCL_OK;
}

int
TclCompileLshiftOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileStrictlyBinaryOpCmd(interp, parsePtr, INST_LSHIFT, envPtr);
}

int
TclCompileRshiftOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileStrictlyBinaryOpCmd(interp, parsePtr, INST_RSHIFT, envPtr);
}

int
TclCompileModOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileStrictlyBinaryOpCmd(interp, parsePtr, INST_MOD, envPtr);
}

int
TclCompileNeqOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileStrictlyBinaryOpCmd(interp, parsePtr, INST_NEQ, envPtr);
}

int
TclCompileStrneqOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileStrictlyBinaryOpCmd(interp, parsePtr, INST_STR_NEQ, envPtr);
}

int
TclCompileInOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileStrictlyBinaryOpCmd(interp, parsePtr, INST_LIST_IN, envPtr);
}

int
TclCompileNiOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileStrictlyBinaryOpCmd(interp, parsePtr, INST_LIST_NOT_IN,
	    envPtr);
}

int
TclCompileLessOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileComparisonOpCmd(interp, parsePtr, INST_LT, envPtr);
}

int
TclCompileLeqOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileComparisonOpCmd(interp, parsePtr, INST_LE, envPtr);
}

int
TclCompileGreaterOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileComparisonOpCmd(interp, parsePtr, INST_GT, envPtr);
}

int
TclCompileGeqOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileComparisonOpCmd(interp, parsePtr, INST_GE, envPtr);
}

int
TclCompileEqOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileComparisonOpCmd(interp, parsePtr, INST_EQ, envPtr);
}

int
TclCompileStreqOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    return CompileComparisonOpCmd(interp, parsePtr, INST_STR_EQ, envPtr);
}

int
TclCompileMinusOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    Tcl_Token *tokenPtr = parsePtr->tokenPtr;
    DefineLineInformation;	/* TIP #280 */
    int words;

    /* TODO: Consider support for compiling expanded args. */
    if (parsePtr->numWords == 1) {
	/*
	 * Fallback to direct eval to report syntax error.
	 */

	return TCL_ERROR;
    }
    for (words=1 ; words<parsePtr->numWords ; words++) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, words);
    }
    if (words == 2) {
	TclEmitOpcode(INST_UMINUS, envPtr);
	return TCL_OK;
    }
    if (words == 3) {
	TclEmitOpcode(INST_SUB, envPtr);
	return TCL_OK;
    }

    /*
     * Reverse order of arguments to get precise agreement with [expr] in
     * calcuations, including roundoff errors.
     */

    TclEmitInstInt4(INST_REVERSE, words-1, envPtr);
    while (--words > 1) {
	TclEmitInstInt4(INST_REVERSE, 2, envPtr);
	TclEmitOpcode(INST_SUB, envPtr);
    }
    return TCL_OK;
}

int
TclCompileDivOpCmd(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)
{
    Tcl_Token *tokenPtr = parsePtr->tokenPtr;
    DefineLineInformation;	/* TIP #280 */
    int words;

    /* TODO: Consider support for compiling expanded args. */
    if (parsePtr->numWords == 1) {
	/*
	 * Fallback to direct eval to report syntax error.
	 */

	return TCL_ERROR;
    }
    if (parsePtr->numWords == 2) {
	PUSH("1.0");
    }
    for (words=1 ; words<parsePtr->numWords ; words++) {
	tokenPtr = TokenAfter(tokenPtr);
	CompileWord(envPtr, tokenPtr, interp, words);
    }
    if (words <= 3) {
	TclEmitOpcode(INST_DIV, envPtr);
	return TCL_OK;
    }

    /*
     * Reverse order of arguments to get precise agreement with [expr] in
     * calcuations, including roundoff errors.
     */

    TclEmitInstInt4(INST_REVERSE, words-1, envPtr);
    while (--words > 1) {
	TclEmitInstInt4(INST_REVERSE, 2, envPtr);
	TclEmitOpcode(INST_DIV, envPtr);
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
