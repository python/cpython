/*
 * tclBasic.c --
 *
 *	Contains the basic facilities for TCL command interpretation,
 *	including interpreter creation and deletion, command creation and
 *	deletion, and command/script execution.
 *
 * Copyright (c) 1987-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 2001, 2002 by Kevin B. Kenny.  All rights reserved.
 * Copyright (c) 2007 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright (c) 2006-2008 by Joe Mistachkin.  All rights reserved.
 * Copyright (c) 2008 Miguel Sofer <msofer@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclOOInt.h"
#include "tclCompile.h"
#include "tommath.h"
#include <math.h>
#include <assert.h>

#define INTERP_STACK_INITIAL_SIZE 2000
#define CORO_STACK_INITIAL_SIZE    200

/*
 * Determine whether we're using IEEE floating point
 */

#if (FLT_RADIX == 2) && (DBL_MANT_DIG == 53) && (DBL_MAX_EXP == 1024)
#   define IEEE_FLOATING_POINT
/* Largest odd integer that can be represented exactly in a double */
#   define MAX_EXACT 9007199254740991.0
#endif

/*
 * The following structure defines the client data for a math function
 * registered with Tcl_CreateMathFunc
 */

typedef struct OldMathFuncData {
    Tcl_MathProc *proc;		/* Handler function */
    int numArgs;		/* Number of args expected */
    Tcl_ValueType *argTypes;	/* Types of the args */
    ClientData clientData;	/* Client data for the handler function */
} OldMathFuncData;

/*
 * This is the script cancellation struct and hash table. The hash table is
 * used to keep track of the information necessary to process script
 * cancellation requests, including the original interp, asynchronous handler
 * tokens (created by Tcl_AsyncCreate), and the clientData and flags arguments
 * passed to Tcl_CancelEval on a per-interp basis. The cancelLock mutex is
 * used for protecting calls to Tcl_CancelEval as well as protecting access to
 * the hash table below.
 */

typedef struct {
    Tcl_Interp *interp;		/* Interp this struct belongs to. */
    Tcl_AsyncHandler async;	/* Async handler token for script
				 * cancellation. */
    char *result;		/* The script cancellation result or NULL for
				 * a default result. */
    int length;			/* Length of the above error message. */
    ClientData clientData;	/* Ignored */
    int flags;			/* Additional flags */
} CancelInfo;
static Tcl_HashTable cancelTable;
static int cancelTableInitialized = 0;	/* 0 means not yet initialized. */
TCL_DECLARE_MUTEX(cancelLock)

/*
 * Declarations for managing contexts for non-recursive coroutines. Contexts
 * are used to save the evaluation state between NR calls to each coro.
 */

#define SAVE_CONTEXT(context)				\
    (context).framePtr = iPtr->framePtr;		\
    (context).varFramePtr = iPtr->varFramePtr;		\
    (context).cmdFramePtr = iPtr->cmdFramePtr;		\
    (context).lineLABCPtr = iPtr->lineLABCPtr

#define RESTORE_CONTEXT(context)			\
    iPtr->framePtr = (context).framePtr;		\
    iPtr->varFramePtr = (context).varFramePtr;		\
    iPtr->cmdFramePtr = (context).cmdFramePtr;		\
    iPtr->lineLABCPtr = (context).lineLABCPtr

/*
 * Static functions in this file:
 */

static char *		CallCommandTraces(Interp *iPtr, Command *cmdPtr,
			    const char *oldName, const char *newName,
			    int flags);
static int		CancelEvalProc(ClientData clientData,
			    Tcl_Interp *interp, int code);
static int		CheckDoubleResult(Tcl_Interp *interp, double dResult);
static void		DeleteCoroutine(ClientData clientData);
static void		DeleteInterpProc(Tcl_Interp *interp);
static void		DeleteOpCmdClientData(ClientData clientData);
#ifdef USE_DTRACE
static Tcl_ObjCmdProc	DTraceObjCmd;
static Tcl_NRPostProc	DTraceCmdReturn;
#else
#   define DTraceCmdReturn	NULL
#endif /* USE_DTRACE */
static Tcl_ObjCmdProc	ExprAbsFunc;
static Tcl_ObjCmdProc	ExprBinaryFunc;
static Tcl_ObjCmdProc	ExprBoolFunc;
static Tcl_ObjCmdProc	ExprCeilFunc;
static Tcl_ObjCmdProc	ExprDoubleFunc;
static Tcl_ObjCmdProc	ExprEntierFunc;
static Tcl_ObjCmdProc	ExprFloorFunc;
static Tcl_ObjCmdProc	ExprIntFunc;
static Tcl_ObjCmdProc	ExprIsqrtFunc;
static Tcl_ObjCmdProc	ExprRandFunc;
static Tcl_ObjCmdProc	ExprRoundFunc;
static Tcl_ObjCmdProc	ExprSqrtFunc;
static Tcl_ObjCmdProc	ExprSrandFunc;
static Tcl_ObjCmdProc	ExprUnaryFunc;
static Tcl_ObjCmdProc	ExprWideFunc;
static void		MathFuncWrongNumArgs(Tcl_Interp *interp, int expected,
			    int actual, Tcl_Obj *const *objv);
static Tcl_NRPostProc	NRCoroutineCallerCallback;
static Tcl_NRPostProc	NRCoroutineExitCallback;
static Tcl_NRPostProc	NRCommand;

static Tcl_ObjCmdProc	OldMathFuncProc;
static void		OldMathFuncDeleteProc(ClientData clientData);
static void		ProcessUnexpectedResult(Tcl_Interp *interp,
			    int returnCode);
static int		RewindCoroutine(CoroutineData *corPtr, int result);
static void		TEOV_SwitchVarFrame(Tcl_Interp *interp);
static void		TEOV_PushExceptionHandlers(Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[], int flags);
static inline Command *	TEOV_LookupCmdFromObj(Tcl_Interp *interp,
			    Tcl_Obj *namePtr, Namespace *lookupNsPtr);
static int		TEOV_NotFound(Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[], Namespace *lookupNsPtr);
static int		TEOV_RunEnterTraces(Tcl_Interp *interp,
			    Command **cmdPtrPtr, Tcl_Obj *commandPtr, int objc,
			    Tcl_Obj *const objv[]);
static Tcl_NRPostProc	RewindCoroutineCallback;
static Tcl_NRPostProc	TEOEx_ByteCodeCallback;
static Tcl_NRPostProc	TEOEx_ListCallback;
static Tcl_NRPostProc	TEOV_Error;
static Tcl_NRPostProc	TEOV_Exception;
static Tcl_NRPostProc	TEOV_NotFoundCallback;
static Tcl_NRPostProc	TEOV_RestoreVarFrame;
static Tcl_NRPostProc	TEOV_RunLeaveTraces;
static Tcl_NRPostProc	EvalObjvCore;
static Tcl_NRPostProc	Dispatch;

static Tcl_ObjCmdProc NRCoroInjectObjCmd;
static Tcl_NRPostProc NRPostInvoke;

MODULE_SCOPE const TclStubs tclStubs;

/*
 * Magical counts for the number of arguments accepted by a coroutine command
 * after particular kinds of [yield].
 */

#define CORO_ACTIVATE_YIELD    PTR2INT(NULL)
#define CORO_ACTIVATE_YIELDM   PTR2INT(NULL)+1

#define COROUTINE_ARGUMENTS_SINGLE_OPTIONAL     (-1)
#define COROUTINE_ARGUMENTS_ARBITRARY           (-2)

/*
 * The following structure define the commands in the Tcl core.
 */

typedef struct {
    const char *name;		/* Name of object-based command. */
    Tcl_ObjCmdProc *objProc;	/* Object-based function for command. */
    CompileProc *compileProc;	/* Function called to compile command. */
    Tcl_ObjCmdProc *nreProc;	/* NR-based function for command */
    int flags;			/* Various flag bits, as defined below. */
} CmdInfo;

#define CMD_IS_SAFE         1   /* Whether this command is part of the set of
                                 * commands present by default in a safe
                                 * interpreter. */
/* CMD_COMPILES_EXPANDED - Whether the compiler for this command can handle
 * expansion for itself rather than needing the generic layer to take care of
 * it for it. Defined in tclInt.h. */

/*
 * The built-in commands, and the functions that implement them:
 */

static const CmdInfo builtInCmds[] = {
    /*
     * Commands in the generic core.
     */

    {"append",		Tcl_AppendObjCmd,	TclCompileAppendCmd,	NULL,	CMD_IS_SAFE},
    {"apply",		Tcl_ApplyObjCmd,	NULL,			TclNRApplyObjCmd,	CMD_IS_SAFE},
    {"break",		Tcl_BreakObjCmd,	TclCompileBreakCmd,	NULL,	CMD_IS_SAFE},
#ifndef EXCLUDE_OBSOLETE_COMMANDS
    {"case",		Tcl_CaseObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
#endif
    {"catch",		Tcl_CatchObjCmd,	TclCompileCatchCmd,	TclNRCatchObjCmd,	CMD_IS_SAFE},
    {"concat",		Tcl_ConcatObjCmd,	TclCompileConcatCmd,	NULL,	CMD_IS_SAFE},
    {"continue",	Tcl_ContinueObjCmd,	TclCompileContinueCmd,	NULL,	CMD_IS_SAFE},
    {"coroutine",	NULL,			NULL,			TclNRCoroutineObjCmd,	CMD_IS_SAFE},
    {"error",		Tcl_ErrorObjCmd,	TclCompileErrorCmd,	NULL,	CMD_IS_SAFE},
    {"eval",		Tcl_EvalObjCmd,		NULL,			TclNREvalObjCmd,	CMD_IS_SAFE},
    {"expr",		Tcl_ExprObjCmd,		TclCompileExprCmd,	TclNRExprObjCmd,	CMD_IS_SAFE},
    {"for",		Tcl_ForObjCmd,		TclCompileForCmd,	TclNRForObjCmd,	CMD_IS_SAFE},
    {"foreach",		Tcl_ForeachObjCmd,	TclCompileForeachCmd,	TclNRForeachCmd,	CMD_IS_SAFE},
    {"format",		Tcl_FormatObjCmd,	TclCompileFormatCmd,	NULL,	CMD_IS_SAFE},
    {"global",		Tcl_GlobalObjCmd,	TclCompileGlobalCmd,	NULL,	CMD_IS_SAFE},
    {"if",		Tcl_IfObjCmd,		TclCompileIfCmd,	TclNRIfObjCmd,	CMD_IS_SAFE},
    {"incr",		Tcl_IncrObjCmd,		TclCompileIncrCmd,	NULL,	CMD_IS_SAFE},
    {"join",		Tcl_JoinObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"lappend",		Tcl_LappendObjCmd,	TclCompileLappendCmd,	NULL,	CMD_IS_SAFE},
    {"lassign",		Tcl_LassignObjCmd,	TclCompileLassignCmd,	NULL,	CMD_IS_SAFE},
    {"lindex",		Tcl_LindexObjCmd,	TclCompileLindexCmd,	NULL,	CMD_IS_SAFE},
    {"linsert",		Tcl_LinsertObjCmd,	TclCompileLinsertCmd,	NULL,	CMD_IS_SAFE},
    {"list",		Tcl_ListObjCmd,		TclCompileListCmd,	NULL,	CMD_IS_SAFE|CMD_COMPILES_EXPANDED},
    {"llength",		Tcl_LlengthObjCmd,	TclCompileLlengthCmd,	NULL,	CMD_IS_SAFE},
    {"lmap",		Tcl_LmapObjCmd,		TclCompileLmapCmd,	TclNRLmapCmd,	CMD_IS_SAFE},
    {"lrange",		Tcl_LrangeObjCmd,	TclCompileLrangeCmd,	NULL,	CMD_IS_SAFE},
    {"lrepeat",		Tcl_LrepeatObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"lreplace",	Tcl_LreplaceObjCmd,	TclCompileLreplaceCmd,	NULL,	CMD_IS_SAFE},
    {"lreverse",	Tcl_LreverseObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"lsearch",		Tcl_LsearchObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"lset",		Tcl_LsetObjCmd,		TclCompileLsetCmd,	NULL,	CMD_IS_SAFE},
    {"lsort",		Tcl_LsortObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"package",		Tcl_PackageObjCmd,	NULL,			TclNRPackageObjCmd,	CMD_IS_SAFE},
    {"proc",		Tcl_ProcObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"regexp",		Tcl_RegexpObjCmd,	TclCompileRegexpCmd,	NULL,	CMD_IS_SAFE},
    {"regsub",		Tcl_RegsubObjCmd,	TclCompileRegsubCmd,	NULL,	CMD_IS_SAFE},
    {"rename",		Tcl_RenameObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"return",		Tcl_ReturnObjCmd,	TclCompileReturnCmd,	NULL,	CMD_IS_SAFE},
    {"scan",		Tcl_ScanObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"set",		Tcl_SetObjCmd,		TclCompileSetCmd,	NULL,	CMD_IS_SAFE},
    {"split",		Tcl_SplitObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"subst",		Tcl_SubstObjCmd,	TclCompileSubstCmd,	TclNRSubstObjCmd,	CMD_IS_SAFE},
    {"switch",		Tcl_SwitchObjCmd,	TclCompileSwitchCmd,	TclNRSwitchObjCmd, CMD_IS_SAFE},
    {"tailcall",	NULL,			TclCompileTailcallCmd,	TclNRTailcallObjCmd,	CMD_IS_SAFE},
    {"throw",		Tcl_ThrowObjCmd,	TclCompileThrowCmd,	NULL,	CMD_IS_SAFE},
    {"trace",		Tcl_TraceObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"try",		Tcl_TryObjCmd,		TclCompileTryCmd,	TclNRTryObjCmd,	CMD_IS_SAFE},
    {"unset",		Tcl_UnsetObjCmd,	TclCompileUnsetCmd,	NULL,	CMD_IS_SAFE},
    {"uplevel",		Tcl_UplevelObjCmd,	NULL,			TclNRUplevelObjCmd,	CMD_IS_SAFE},
    {"upvar",		Tcl_UpvarObjCmd,	TclCompileUpvarCmd,	NULL,	CMD_IS_SAFE},
    {"variable",	Tcl_VariableObjCmd,	TclCompileVariableCmd,	NULL,	CMD_IS_SAFE},
    {"while",		Tcl_WhileObjCmd,	TclCompileWhileCmd,	TclNRWhileObjCmd,	CMD_IS_SAFE},
    {"yield",		NULL,			TclCompileYieldCmd,	TclNRYieldObjCmd,	CMD_IS_SAFE},
    {"yieldto",		NULL,			TclCompileYieldToCmd,	TclNRYieldToObjCmd,	CMD_IS_SAFE},

    /*
     * Commands in the OS-interface. Note that many of these are unsafe.
     */

    {"after",		Tcl_AfterObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"cd",		Tcl_CdObjCmd,		NULL,			NULL,	0},
    {"close",		Tcl_CloseObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"eof",		Tcl_EofObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"exec",		Tcl_ExecObjCmd,		NULL,			NULL,	0},
    {"exit",		Tcl_ExitObjCmd,		NULL,			NULL,	0},
    {"fblocked",	Tcl_FblockedObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"fconfigure",	Tcl_FconfigureObjCmd,	NULL,			NULL,	0},
    {"fcopy",		Tcl_FcopyObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"fileevent",	Tcl_FileEventObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"flush",		Tcl_FlushObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"gets",		Tcl_GetsObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"glob",		Tcl_GlobObjCmd,		NULL,			NULL,	0},
    {"load",		Tcl_LoadObjCmd,		NULL,			NULL,	0},
    {"open",		Tcl_OpenObjCmd,		NULL,			NULL,	0},
    {"pid",		Tcl_PidObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"puts",		Tcl_PutsObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"pwd",		Tcl_PwdObjCmd,		NULL,			NULL,	0},
    {"read",		Tcl_ReadObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"seek",		Tcl_SeekObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"socket",		Tcl_SocketObjCmd,	NULL,			NULL,	0},
    {"source",		Tcl_SourceObjCmd,	NULL,			TclNRSourceObjCmd,	0},
    {"tell",		Tcl_TellObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"time",		Tcl_TimeObjCmd,		NULL,			NULL,	CMD_IS_SAFE},
    {"unload",		Tcl_UnloadObjCmd,	NULL,			NULL,	0},
    {"update",		Tcl_UpdateObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {"vwait",		Tcl_VwaitObjCmd,	NULL,			NULL,	CMD_IS_SAFE},
    {NULL,		NULL,			NULL,			NULL,	0}
};

/*
 * Math functions. All are safe.
 */

typedef struct {
    const char *name;		/* Name of the function. The full name is
				 * "::tcl::mathfunc::<name>". */
    Tcl_ObjCmdProc *objCmdProc;	/* Function that evaluates the function */
    ClientData clientData;	/* Client data for the function */
} BuiltinFuncDef;
static const BuiltinFuncDef BuiltinFuncTable[] = {
    { "abs",	ExprAbsFunc,	NULL			},
    { "acos",	ExprUnaryFunc,	(ClientData) acos	},
    { "asin",	ExprUnaryFunc,	(ClientData) asin	},
    { "atan",	ExprUnaryFunc,	(ClientData) atan	},
    { "atan2",	ExprBinaryFunc,	(ClientData) atan2	},
    { "bool",	ExprBoolFunc,	NULL			},
    { "ceil",	ExprCeilFunc,	NULL			},
    { "cos",	ExprUnaryFunc,	(ClientData) cos	},
    { "cosh",	ExprUnaryFunc,	(ClientData) cosh	},
    { "double",	ExprDoubleFunc,	NULL			},
    { "entier",	ExprEntierFunc,	NULL			},
    { "exp",	ExprUnaryFunc,	(ClientData) exp	},
    { "floor",	ExprFloorFunc,	NULL			},
    { "fmod",	ExprBinaryFunc,	(ClientData) fmod	},
    { "hypot",	ExprBinaryFunc,	(ClientData) hypot	},
    { "int",	ExprIntFunc,	NULL			},
    { "isqrt",	ExprIsqrtFunc,	NULL			},
    { "log",	ExprUnaryFunc,	(ClientData) log	},
    { "log10",	ExprUnaryFunc,	(ClientData) log10	},
    { "pow",	ExprBinaryFunc,	(ClientData) pow	},
    { "rand",	ExprRandFunc,	NULL			},
    { "round",	ExprRoundFunc,	NULL			},
    { "sin",	ExprUnaryFunc,	(ClientData) sin	},
    { "sinh",	ExprUnaryFunc,	(ClientData) sinh	},
    { "sqrt",	ExprSqrtFunc,	NULL			},
    { "srand",	ExprSrandFunc,	NULL			},
    { "tan",	ExprUnaryFunc,	(ClientData) tan	},
    { "tanh",	ExprUnaryFunc,	(ClientData) tanh	},
    { "wide",	ExprWideFunc,	NULL			},
    { NULL, NULL, NULL }
};

/*
 * TIP#174's math operators. All are safe.
 */

typedef struct {
    const char *name;		/* Name of object-based command. */
    Tcl_ObjCmdProc *objProc;	/* Object-based function for command. */
    CompileProc *compileProc;	/* Function called to compile command. */
    union {
	int numArgs;
	int identity;
    } i;
    const char *expected;	/* For error message, what argument(s)
				 * were expected. */
} OpCmdInfo;
static const OpCmdInfo mathOpCmds[] = {
    { "~",	TclSingleOpCmd,		TclCompileInvertOpCmd,
		/* numArgs */ {1},	"integer"},
    { "!",	TclSingleOpCmd,		TclCompileNotOpCmd,
		/* numArgs */ {1},	"boolean"},
    { "+",	TclVariadicOpCmd,	TclCompileAddOpCmd,
		/* identity */ {0},	NULL},
    { "*",	TclVariadicOpCmd,	TclCompileMulOpCmd,
		/* identity */ {1},	NULL},
    { "&",	TclVariadicOpCmd,	TclCompileAndOpCmd,
		/* identity */ {-1},	NULL},
    { "|",	TclVariadicOpCmd,	TclCompileOrOpCmd,
		/* identity */ {0},	NULL},
    { "^",	TclVariadicOpCmd,	TclCompileXorOpCmd,
		/* identity */ {0},	NULL},
    { "**",	TclVariadicOpCmd,	TclCompilePowOpCmd,
		/* identity */ {1},	NULL},
    { "<<",	TclSingleOpCmd,		TclCompileLshiftOpCmd,
		/* numArgs */ {2},	"integer shift"},
    { ">>",	TclSingleOpCmd,		TclCompileRshiftOpCmd,
		/* numArgs */ {2},	"integer shift"},
    { "%",	TclSingleOpCmd,		TclCompileModOpCmd,
		/* numArgs */ {2},	"integer integer"},
    { "!=",	TclSingleOpCmd,		TclCompileNeqOpCmd,
		/* numArgs */ {2},	"value value"},
    { "ne",	TclSingleOpCmd,		TclCompileStrneqOpCmd,
		/* numArgs */ {2},	"value value"},
    { "in",	TclSingleOpCmd,		TclCompileInOpCmd,
		/* numArgs */ {2},	"value list"},
    { "ni",	TclSingleOpCmd,		TclCompileNiOpCmd,
		/* numArgs */ {2},	"value list"},
    { "-",	TclNoIdentOpCmd,	TclCompileMinusOpCmd,
		/* unused */ {0},	"value ?value ...?"},
    { "/",	TclNoIdentOpCmd,	TclCompileDivOpCmd,
		/* unused */ {0},	"value ?value ...?"},
    { "<",	TclSortingOpCmd,	TclCompileLessOpCmd,
		/* unused */ {0},	NULL},
    { "<=",	TclSortingOpCmd,	TclCompileLeqOpCmd,
		/* unused */ {0},	NULL},
    { ">",	TclSortingOpCmd,	TclCompileGreaterOpCmd,
		/* unused */ {0},	NULL},
    { ">=",	TclSortingOpCmd,	TclCompileGeqOpCmd,
		/* unused */ {0},	NULL},
    { "==",	TclSortingOpCmd,	TclCompileEqOpCmd,
		/* unused */ {0},	NULL},
    { "eq",	TclSortingOpCmd,	TclCompileStreqOpCmd,
		/* unused */ {0},	NULL},
    { NULL,	NULL,			NULL,
		{0},			NULL}
};

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeEvaluation --
 *
 *	Finalizes the script cancellation hash table.
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
TclFinalizeEvaluation(void)
{
    Tcl_MutexLock(&cancelLock);
    if (cancelTableInitialized == 1) {
	Tcl_DeleteHashTable(&cancelTable);
	cancelTableInitialized = 0;
    }
    Tcl_MutexUnlock(&cancelLock);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateInterp --
 *
 *	Create a new TCL command interpreter.
 *
 * Results:
 *	The return value is a token for the interpreter, which may be used in
 *	calls to functions like Tcl_CreateCmd, Tcl_Eval, or Tcl_DeleteInterp.
 *
 * Side effects:
 *	The command interpreter is initialized with the built-in commands and
 *	with the variables documented in tclvars(n).
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *
Tcl_CreateInterp(void)
{
    Interp *iPtr;
    Tcl_Interp *interp;
    Command *cmdPtr;
    const BuiltinFuncDef *builtinFuncPtr;
    const OpCmdInfo *opcmdInfoPtr;
    const CmdInfo *cmdInfoPtr;
    Tcl_Namespace *mathfuncNSPtr, *mathopNSPtr;
    Tcl_HashEntry *hPtr;
    int isNew;
    CancelInfo *cancelInfo;
    union {
	char c[sizeof(short)];
	short s;
    } order;
#ifdef TCL_COMPILE_STATS
    ByteCodeStats *statsPtr;
#endif /* TCL_COMPILE_STATS */
    char mathFuncName[32];
    CallFrame *framePtr;

    TclInitSubsystems();

    /*
     * Panic if someone updated the CallFrame structure without also updating
     * the Tcl_CallFrame structure (or vice versa).
     */

    if (sizeof(Tcl_CallFrame) < sizeof(CallFrame)) {
	/*NOTREACHED*/
	Tcl_Panic("Tcl_CallFrame must not be smaller than CallFrame");
    }

#if defined(_WIN32) && !defined(_WIN64)
    if (sizeof(time_t) != 4) {
	/*NOTREACHED*/
	Tcl_Panic("<time.h> is not compatible with MSVC");
    }
    if ((TclOffset(Tcl_StatBuf,st_atime) != 32)
	    || (TclOffset(Tcl_StatBuf,st_ctime) != 40)) {
	/*NOTREACHED*/
	Tcl_Panic("<sys/stat.h> is not compatible with MSVC");
    }
#endif

    if (cancelTableInitialized == 0) {
	Tcl_MutexLock(&cancelLock);
	if (cancelTableInitialized == 0) {
	    Tcl_InitHashTable(&cancelTable, TCL_ONE_WORD_KEYS);
	    cancelTableInitialized = 1;
	}
	Tcl_MutexUnlock(&cancelLock);
    }

    /*
     * Initialize support for namespaces and create the global namespace
     * (whose name is ""; an alias is "::"). This also initializes the Tcl
     * object type table and other object management code.
     */

    iPtr = ckalloc(sizeof(Interp));
    interp = (Tcl_Interp *) iPtr;

    iPtr->result = iPtr->resultSpace;
    iPtr->freeProc = NULL;
    iPtr->errorLine = 0;
    iPtr->objResultPtr = Tcl_NewObj();
    Tcl_IncrRefCount(iPtr->objResultPtr);
    iPtr->handle = TclHandleCreate(iPtr);
    iPtr->globalNsPtr = NULL;
    iPtr->hiddenCmdTablePtr = NULL;
    iPtr->interpInfo = NULL;

    TCL_CT_ASSERT(sizeof(iPtr->extra) <= sizeof(Tcl_HashTable));
    iPtr->extra.optimizer = TclOptimizeBytecode;

    iPtr->numLevels = 0;
    iPtr->maxNestingDepth = MAX_NESTING_DEPTH;
    iPtr->framePtr = NULL;	/* Initialise as soon as :: is available */
    iPtr->varFramePtr = NULL;	/* Initialise as soon as :: is available */

    /*
     * TIP #280 - Initialize the arrays used to extend the ByteCode and Proc
     * structures.
     */

    iPtr->cmdFramePtr = NULL;
    iPtr->linePBodyPtr = ckalloc(sizeof(Tcl_HashTable));
    iPtr->lineBCPtr = ckalloc(sizeof(Tcl_HashTable));
    iPtr->lineLAPtr = ckalloc(sizeof(Tcl_HashTable));
    iPtr->lineLABCPtr = ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(iPtr->linePBodyPtr, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(iPtr->lineBCPtr, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(iPtr->lineLAPtr, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(iPtr->lineLABCPtr, TCL_ONE_WORD_KEYS);
    iPtr->scriptCLLocPtr = NULL;

    iPtr->activeVarTracePtr = NULL;

    iPtr->returnOpts = NULL;
    iPtr->errorInfo = NULL;
    TclNewLiteralStringObj(iPtr->eiVar, "::errorInfo");
    Tcl_IncrRefCount(iPtr->eiVar);
    iPtr->errorStack = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(iPtr->errorStack);
    iPtr->resetErrorStack = 1;
    TclNewLiteralStringObj(iPtr->upLiteral,"UP");
    Tcl_IncrRefCount(iPtr->upLiteral);
    TclNewLiteralStringObj(iPtr->callLiteral,"CALL");
    Tcl_IncrRefCount(iPtr->callLiteral);
    TclNewLiteralStringObj(iPtr->innerLiteral,"INNER");
    Tcl_IncrRefCount(iPtr->innerLiteral);
    iPtr->innerContext = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(iPtr->innerContext);
    iPtr->errorCode = NULL;
    TclNewLiteralStringObj(iPtr->ecVar, "::errorCode");
    Tcl_IncrRefCount(iPtr->ecVar);
    iPtr->returnLevel = 1;
    iPtr->returnCode = TCL_OK;

    iPtr->rootFramePtr = NULL;	/* Initialise as soon as :: is available */
    iPtr->lookupNsPtr = NULL;

    iPtr->appendResult = NULL;
    iPtr->appendAvl = 0;
    iPtr->appendUsed = 0;

    Tcl_InitHashTable(&iPtr->packageTable, TCL_STRING_KEYS);
    iPtr->packageUnknown = NULL;

    /* TIP #268 */
    if (getenv("TCL_PKG_PREFER_LATEST") == NULL) {
	iPtr->packagePrefer = PKG_PREFER_STABLE;
    } else {
	iPtr->packagePrefer = PKG_PREFER_LATEST;
    }

    iPtr->cmdCount = 0;
    TclInitLiteralTable(&iPtr->literalTable);
    iPtr->compileEpoch = 0;
    iPtr->compiledProcPtr = NULL;
    iPtr->resolverPtr = NULL;
    iPtr->evalFlags = 0;
    iPtr->scriptFile = NULL;
    iPtr->flags = 0;
    iPtr->tracePtr = NULL;
    iPtr->tracesForbiddingInline = 0;
    iPtr->activeCmdTracePtr = NULL;
    iPtr->activeInterpTracePtr = NULL;
    iPtr->assocData = NULL;
    iPtr->execEnvPtr = NULL;	/* Set after namespaces initialized. */
    iPtr->emptyObjPtr = Tcl_NewObj();
				/* Another empty object. */
    Tcl_IncrRefCount(iPtr->emptyObjPtr);
    iPtr->resultSpace[0] = 0;
    iPtr->threadId = Tcl_GetCurrentThread();

    /* TIP #378 */
#ifdef TCL_INTERP_DEBUG_FRAME
    iPtr->flags |= INTERP_DEBUG_FRAME;
#else
    if (getenv("TCL_INTERP_DEBUG_FRAME") != NULL) {
        iPtr->flags |= INTERP_DEBUG_FRAME;
    }
#endif

    /*
     * Initialise the tables for variable traces and searches *before*
     * creating the global ns - so that the trace on errorInfo can be
     * recorded.
     */

    Tcl_InitHashTable(&iPtr->varTraces, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&iPtr->varSearches, TCL_ONE_WORD_KEYS);

    iPtr->globalNsPtr = NULL;	/* Force creation of global ns below. */
    iPtr->globalNsPtr = (Namespace *) Tcl_CreateNamespace(interp, "",
	    NULL, NULL);
    if (iPtr->globalNsPtr == NULL) {
	Tcl_Panic("Tcl_CreateInterp: can't create global namespace");
    }

    /*
     * Initialise the rootCallframe. It cannot be allocated on the stack, as
     * it has to be in place before TclCreateExecEnv tries to use a variable.
     */

    /* This is needed to satisfy GCC 3.3's strict aliasing rules */
    framePtr = ckalloc(sizeof(CallFrame));
    (void) Tcl_PushCallFrame(interp, (Tcl_CallFrame *) framePtr,
	    (Tcl_Namespace *) iPtr->globalNsPtr, /*isProcCallFrame*/ 0);
    framePtr->objc = 0;

    iPtr->framePtr = framePtr;
    iPtr->varFramePtr = framePtr;
    iPtr->rootFramePtr = framePtr;

    /*
     * Initialize support for code compilation and execution. We call
     * TclCreateExecEnv after initializing namespaces since it tries to
     * reference a Tcl variable (it links to the Tcl "tcl_traceExec"
     * variable).
     */

    iPtr->execEnvPtr = TclCreateExecEnv(interp, INTERP_STACK_INITIAL_SIZE);

    /*
     * TIP #219, Tcl Channel Reflection API support.
     */

    iPtr->chanMsg = NULL;

    /*
     * TIP #285, Script cancellation support.
     */

    iPtr->asyncCancelMsg = Tcl_NewObj();

    cancelInfo = ckalloc(sizeof(CancelInfo));
    cancelInfo->interp = interp;

    iPtr->asyncCancel = Tcl_AsyncCreate(CancelEvalProc, cancelInfo);
    cancelInfo->async = iPtr->asyncCancel;
    cancelInfo->result = NULL;
    cancelInfo->length = 0;

    Tcl_MutexLock(&cancelLock);
    hPtr = Tcl_CreateHashEntry(&cancelTable, iPtr, &isNew);
    Tcl_SetHashValue(hPtr, cancelInfo);
    Tcl_MutexUnlock(&cancelLock);

    /*
     * Initialize the compilation and execution statistics kept for this
     * interpreter.
     */

#ifdef TCL_COMPILE_STATS
    statsPtr = &iPtr->stats;
    statsPtr->numExecutions = 0;
    statsPtr->numCompilations = 0;
    statsPtr->numByteCodesFreed = 0;
    memset(statsPtr->instructionCount, 0,
	    sizeof(statsPtr->instructionCount));

    statsPtr->totalSrcBytes = 0.0;
    statsPtr->totalByteCodeBytes = 0.0;
    statsPtr->currentSrcBytes = 0.0;
    statsPtr->currentByteCodeBytes = 0.0;
    memset(statsPtr->srcCount, 0, sizeof(statsPtr->srcCount));
    memset(statsPtr->byteCodeCount, 0, sizeof(statsPtr->byteCodeCount));
    memset(statsPtr->lifetimeCount, 0, sizeof(statsPtr->lifetimeCount));

    statsPtr->currentInstBytes = 0.0;
    statsPtr->currentLitBytes = 0.0;
    statsPtr->currentExceptBytes = 0.0;
    statsPtr->currentAuxBytes = 0.0;
    statsPtr->currentCmdMapBytes = 0.0;

    statsPtr->numLiteralsCreated = 0;
    statsPtr->totalLitStringBytes = 0.0;
    statsPtr->currentLitStringBytes = 0.0;
    memset(statsPtr->literalCount, 0, sizeof(statsPtr->literalCount));
#endif /* TCL_COMPILE_STATS */

    /*
     * Initialise the stub table pointer.
     */

    iPtr->stubTable = &tclStubs;

    /*
     * Initialize the ensemble error message rewriting support.
     */

    TclResetRewriteEnsemble(interp, 1);

    /*
     * TIP#143: Initialise the resource limit support.
     */

    TclInitLimitSupport(interp);

    /*
     * Initialise the thread-specific data ekeko. Note that the thread's alloc
     * cache was already initialised by the call to alloc the interp struct.
     */

#if defined(TCL_THREADS) && defined(USE_THREAD_ALLOC)
    iPtr->allocCache = TclpGetAllocCache();
#else
    iPtr->allocCache = NULL;
#endif
    iPtr->pendingObjDataPtr = NULL;
    iPtr->asyncReadyPtr = TclGetAsyncReadyPtr();
    iPtr->deferredCallbacks = NULL;

    /*
     * Create the core commands. Do it here, rather than calling
     * Tcl_CreateCommand, because it's faster (there's no need to check for a
     * pre-existing command by the same name). If a command has a Tcl_CmdProc
     * but no Tcl_ObjCmdProc, set the Tcl_ObjCmdProc to
     * TclInvokeStringCommand. This is an object-based wrapper function that
     * extracts strings, calls the string function, and creates an object for
     * the result. Similarly, if a command has a Tcl_ObjCmdProc but no
     * Tcl_CmdProc, set the Tcl_CmdProc to TclInvokeObjectCommand.
     */

    for (cmdInfoPtr = builtInCmds; cmdInfoPtr->name != NULL; cmdInfoPtr++) {
	if ((cmdInfoPtr->objProc == NULL)
		&& (cmdInfoPtr->compileProc == NULL)
		&& (cmdInfoPtr->nreProc == NULL)) {
	    Tcl_Panic("builtin command with NULL object command proc and a NULL compile proc");
	}

	hPtr = Tcl_CreateHashEntry(&iPtr->globalNsPtr->cmdTable,
		cmdInfoPtr->name, &isNew);
	if (isNew) {
	    cmdPtr = ckalloc(sizeof(Command));
	    cmdPtr->hPtr = hPtr;
	    cmdPtr->nsPtr = iPtr->globalNsPtr;
	    cmdPtr->refCount = 1;
	    cmdPtr->cmdEpoch = 0;
	    cmdPtr->compileProc = cmdInfoPtr->compileProc;
	    cmdPtr->proc = TclInvokeObjectCommand;
	    cmdPtr->clientData = cmdPtr;
	    cmdPtr->objProc = cmdInfoPtr->objProc;
	    cmdPtr->objClientData = NULL;
	    cmdPtr->deleteProc = NULL;
	    cmdPtr->deleteData = NULL;
	    cmdPtr->flags = 0;
            if (cmdInfoPtr->flags & CMD_COMPILES_EXPANDED) {
                cmdPtr->flags |= CMD_COMPILES_EXPANDED;
            }
	    cmdPtr->importRefPtr = NULL;
	    cmdPtr->tracePtr = NULL;
	    cmdPtr->nreProc = cmdInfoPtr->nreProc;
	    Tcl_SetHashValue(hPtr, cmdPtr);
	}
    }

    /*
     * Create the "array", "binary", "chan", "clock", "dict", "encoding",
     * "file", "info", "namespace" and "string" ensembles. Note that all these
     * commands (and their subcommands that are not present in the global
     * namespace) are wholly safe *except* for "clock", "encoding" and "file".
     */

    TclInitArrayCmd(interp);
    TclInitBinaryCmd(interp);
    TclInitChanCmd(interp);
    TclInitDictCmd(interp);
    TclInitEncodingCmd(interp);
    TclInitFileCmd(interp);
    TclInitInfoCmd(interp);
    TclInitNamespaceCmd(interp);
    TclInitStringCmd(interp);
    TclInitPrefixCmd(interp);

    /*
     * Register "clock" subcommands. These *do* go through
     * Tcl_CreateObjCommand, since they aren't in the global namespace and
     * involve ensembles.
     */

    TclClockInit(interp);

    /*
     * Register the built-in functions. This is empty now that they are
     * implemented as commands in the ::tcl::mathfunc namespace.
     */

    /*
     * Register the default [interp bgerror] handler.
     */

    Tcl_CreateObjCommand(interp, "::tcl::Bgerror",
	    TclDefaultBgErrorHandlerObjCmd, NULL, NULL);

    /*
     * Create unsupported commands for debugging bytecode and objects.
     */

    Tcl_CreateObjCommand(interp, "::tcl::unsupported::disassemble",
	    Tcl_DisassembleObjCmd, INT2PTR(0), NULL);
    Tcl_CreateObjCommand(interp, "::tcl::unsupported::getbytecode",
	    Tcl_DisassembleObjCmd, INT2PTR(1), NULL);
    Tcl_CreateObjCommand(interp, "::tcl::unsupported::representation",
	    Tcl_RepresentationCmd, NULL, NULL);

    /* Adding the bytecode assembler command */
    cmdPtr = (Command *) Tcl_NRCreateCommand(interp,
            "::tcl::unsupported::assemble", Tcl_AssembleObjCmd,
            TclNRAssembleObjCmd, NULL, NULL);
    cmdPtr->compileProc = &TclCompileAssembleCmd;

    Tcl_NRCreateCommand(interp, "::tcl::unsupported::inject", NULL,
	    NRCoroInjectObjCmd, NULL, NULL);

#ifdef USE_DTRACE
    /*
     * Register the tcl::dtrace command.
     */

    Tcl_CreateObjCommand(interp, "::tcl::dtrace", DTraceObjCmd, NULL, NULL);
#endif /* USE_DTRACE */

    /*
     * Register the builtin math functions.
     */

    mathfuncNSPtr = Tcl_CreateNamespace(interp, "::tcl::mathfunc", NULL,NULL);
    if (mathfuncNSPtr == NULL) {
	Tcl_Panic("Can't create math function namespace");
    }
#define MATH_FUNC_PREFIX_LEN 17 /* == strlen("::tcl::mathfunc::") */
    memcpy(mathFuncName, "::tcl::mathfunc::", MATH_FUNC_PREFIX_LEN);
    for (builtinFuncPtr = BuiltinFuncTable; builtinFuncPtr->name != NULL;
	    builtinFuncPtr++) {
	strcpy(mathFuncName+MATH_FUNC_PREFIX_LEN, builtinFuncPtr->name);
	Tcl_CreateObjCommand(interp, mathFuncName,
		builtinFuncPtr->objCmdProc, builtinFuncPtr->clientData, NULL);
	Tcl_Export(interp, mathfuncNSPtr, builtinFuncPtr->name, 0);
    }

    /*
     * Register the mathematical "operator" commands. [TIP #174]
     */

    mathopNSPtr = Tcl_CreateNamespace(interp, "::tcl::mathop", NULL, NULL);
    if (mathopNSPtr == NULL) {
	Tcl_Panic("can't create math operator namespace");
    }
    Tcl_Export(interp, mathopNSPtr, "*", 1);
#define MATH_OP_PREFIX_LEN 15 /* == strlen("::tcl::mathop::") */
    memcpy(mathFuncName, "::tcl::mathop::", MATH_OP_PREFIX_LEN);
    for (opcmdInfoPtr=mathOpCmds ; opcmdInfoPtr->name!=NULL ; opcmdInfoPtr++){
	TclOpCmdClientData *occdPtr = ckalloc(sizeof(TclOpCmdClientData));

	occdPtr->op = opcmdInfoPtr->name;
	occdPtr->i.numArgs = opcmdInfoPtr->i.numArgs;
	occdPtr->expected = opcmdInfoPtr->expected;
	strcpy(mathFuncName + MATH_OP_PREFIX_LEN, opcmdInfoPtr->name);
	cmdPtr = (Command *) Tcl_CreateObjCommand(interp, mathFuncName,
		opcmdInfoPtr->objProc, occdPtr, DeleteOpCmdClientData);
	if (cmdPtr == NULL) {
	    Tcl_Panic("failed to create math operator %s",
		    opcmdInfoPtr->name);
	} else if (opcmdInfoPtr->compileProc != NULL) {
	    cmdPtr->compileProc = opcmdInfoPtr->compileProc;
	}
    }

    /*
     * Do Multiple/Safe Interps Tcl init stuff
     */

    TclInterpInit(interp);
    TclSetupEnv(interp);

    /*
     * TIP #59: Make embedded configuration information available.
     */

    TclInitEmbeddedConfigurationInformation(interp);

    /*
     * TIP #440: Declare the name of the script engine to be "Tcl".
     */

    Tcl_SetVar2(interp, "tcl_platform", "engine", "Tcl",
	    TCL_GLOBAL_ONLY);

    /*
     * Compute the byte order of this machine.
     */

    order.s = 1;
    Tcl_SetVar2(interp, "tcl_platform", "byteOrder",
	    ((order.c[0] == 1) ? "littleEndian" : "bigEndian"),
	    TCL_GLOBAL_ONLY);

    Tcl_SetVar2Ex(interp, "tcl_platform", "wordSize",
	    Tcl_NewLongObj((long) sizeof(long)), TCL_GLOBAL_ONLY);

    /* TIP #291 */
    Tcl_SetVar2Ex(interp, "tcl_platform", "pointerSize",
	    Tcl_NewLongObj((long) sizeof(void *)), TCL_GLOBAL_ONLY);

    /*
     * Set up other variables such as tcl_version and tcl_library
     */

    Tcl_SetVar(interp, "tcl_patchLevel", TCL_PATCH_LEVEL, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "tcl_version", TCL_VERSION, TCL_GLOBAL_ONLY);
    Tcl_TraceVar2(interp, "tcl_precision", NULL,
	    TCL_GLOBAL_ONLY|TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
	    TclPrecTraceProc, NULL);
    TclpSetVariables(interp);

#ifdef TCL_THREADS
    /*
     * The existence of the "threaded" element of the tcl_platform array
     * indicates that this particular Tcl shell has been compiled with threads
     * turned on. Using "info exists tcl_platform(threaded)" a Tcl script can
     * introspect on the interpreter level of thread safety.
     */

    Tcl_SetVar2(interp, "tcl_platform", "threaded", "1", TCL_GLOBAL_ONLY);
#endif

    /*
     * Register Tcl's version number.
     * TIP #268: Full patchlevel instead of just major.minor
     */

    Tcl_PkgProvideEx(interp, "Tcl", TCL_PATCH_LEVEL, &tclStubs);

    if (TclTommath_Init(interp) != TCL_OK) {
	Tcl_Panic("%s", Tcl_GetString(Tcl_GetObjResult(interp)));
    }

    if (TclOOInit(interp) != TCL_OK) {
	Tcl_Panic("%s", Tcl_GetString(Tcl_GetObjResult(interp)));
    }

    /*
     * Only build in zlib support if we've successfully detected a library to
     * compile and link against.
     */

#ifdef HAVE_ZLIB
    if (TclZlibInit(interp) != TCL_OK) {
	Tcl_Panic("%s", Tcl_GetString(Tcl_GetObjResult(interp)));
    }
#endif

    TOP_CB(iPtr) = NULL;
    return interp;
}

static void
DeleteOpCmdClientData(
    ClientData clientData)
{
    TclOpCmdClientData *occdPtr = clientData;

    ckfree(occdPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclHideUnsafeCommands --
 *
 *	Hides base commands that are not marked as safe from this interpreter.
 *
 * Results:
 *	TCL_OK if it succeeds, TCL_ERROR else.
 *
 * Side effects:
 *	Hides functionality in an interpreter.
 *
 *----------------------------------------------------------------------
 */

int
TclHideUnsafeCommands(
    Tcl_Interp *interp)		/* Hide commands in this interpreter. */
{
    register const CmdInfo *cmdInfoPtr;

    if (interp == NULL) {
	return TCL_ERROR;
    }
    for (cmdInfoPtr = builtInCmds; cmdInfoPtr->name != NULL; cmdInfoPtr++) {
	if (!(cmdInfoPtr->flags & CMD_IS_SAFE)) {
	    Tcl_HideCommand(interp, cmdInfoPtr->name, cmdInfoPtr->name);
	}
    }
    TclMakeEncodingCommandSafe(interp); /* Ugh! */
    TclMakeFileCommandSafe(interp);     /* Ugh! */
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tcl_CallWhenDeleted --
 *
 *	Arrange for a function to be called before a given interpreter is
 *	deleted. The function is called as soon as Tcl_DeleteInterp is called;
 *	if Tcl_CallWhenDeleted is called on an interpreter that has already
 *	been deleted, the function will be called when the last Tcl_Release is
 *	done on the interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When Tcl_DeleteInterp is invoked to delete interp, proc will be
 *	invoked. See the manual entry for details.
 *
 *--------------------------------------------------------------
 */

void
Tcl_CallWhenDeleted(
    Tcl_Interp *interp,		/* Interpreter to watch. */
    Tcl_InterpDeleteProc *proc,	/* Function to call when interpreter is about
				 * to be deleted. */
    ClientData clientData)	/* One-word value to pass to proc. */
{
    Interp *iPtr = (Interp *) interp;
    static Tcl_ThreadDataKey assocDataCounterKey;
    int *assocDataCounterPtr =
	    Tcl_GetThreadData(&assocDataCounterKey, (int)sizeof(int));
    int isNew;
    char buffer[32 + TCL_INTEGER_SPACE];
    AssocData *dPtr = ckalloc(sizeof(AssocData));
    Tcl_HashEntry *hPtr;

    sprintf(buffer, "Assoc Data Key #%d", *assocDataCounterPtr);
    (*assocDataCounterPtr)++;

    if (iPtr->assocData == NULL) {
	iPtr->assocData = ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(iPtr->assocData, TCL_STRING_KEYS);
    }
    hPtr = Tcl_CreateHashEntry(iPtr->assocData, buffer, &isNew);
    dPtr->proc = proc;
    dPtr->clientData = clientData;
    Tcl_SetHashValue(hPtr, dPtr);
}

/*
 *--------------------------------------------------------------
 *
 * Tcl_DontCallWhenDeleted --
 *
 *	Cancel the arrangement for a function to be called when a given
 *	interpreter is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If proc and clientData were previously registered as a callback via
 *	Tcl_CallWhenDeleted, they are unregistered. If they weren't previously
 *	registered then nothing happens.
 *
 *--------------------------------------------------------------
 */

void
Tcl_DontCallWhenDeleted(
    Tcl_Interp *interp,		/* Interpreter to watch. */
    Tcl_InterpDeleteProc *proc,	/* Function to call when interpreter is about
				 * to be deleted. */
    ClientData clientData)	/* One-word value to pass to proc. */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_HashTable *hTablePtr;
    Tcl_HashSearch hSearch;
    Tcl_HashEntry *hPtr;
    AssocData *dPtr;

    hTablePtr = iPtr->assocData;
    if (hTablePtr == NULL) {
	return;
    }
    for (hPtr = Tcl_FirstHashEntry(hTablePtr, &hSearch); hPtr != NULL;
	    hPtr = Tcl_NextHashEntry(&hSearch)) {
	dPtr = Tcl_GetHashValue(hPtr);
	if ((dPtr->proc == proc) && (dPtr->clientData == clientData)) {
	    ckfree(dPtr);
	    Tcl_DeleteHashEntry(hPtr);
	    return;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetAssocData --
 *
 *	Creates a named association between user-specified data, a delete
 *	function and this interpreter. If the association already exists the
 *	data is overwritten with the new data. The delete function will be
 *	invoked when the interpreter is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the associated data, creates the association if needed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetAssocData(
    Tcl_Interp *interp,		/* Interpreter to associate with. */
    const char *name,		/* Name for association. */
    Tcl_InterpDeleteProc *proc,	/* Proc to call when interpreter is about to
				 * be deleted. */
    ClientData clientData)	/* One-word value to pass to proc. */
{
    Interp *iPtr = (Interp *) interp;
    AssocData *dPtr;
    Tcl_HashEntry *hPtr;
    int isNew;

    if (iPtr->assocData == NULL) {
	iPtr->assocData = ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(iPtr->assocData, TCL_STRING_KEYS);
    }
    hPtr = Tcl_CreateHashEntry(iPtr->assocData, name, &isNew);
    if (isNew == 0) {
	dPtr = Tcl_GetHashValue(hPtr);
    } else {
	dPtr = ckalloc(sizeof(AssocData));
    }
    dPtr->proc = proc;
    dPtr->clientData = clientData;

    Tcl_SetHashValue(hPtr, dPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteAssocData --
 *
 *	Deletes a named association of user-specified data with the specified
 *	interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes the association.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteAssocData(
    Tcl_Interp *interp,		/* Interpreter to associate with. */
    const char *name)		/* Name of association. */
{
    Interp *iPtr = (Interp *) interp;
    AssocData *dPtr;
    Tcl_HashEntry *hPtr;

    if (iPtr->assocData == NULL) {
	return;
    }
    hPtr = Tcl_FindHashEntry(iPtr->assocData, name);
    if (hPtr == NULL) {
	return;
    }
    dPtr = Tcl_GetHashValue(hPtr);
    if (dPtr->proc != NULL) {
	dPtr->proc(dPtr->clientData, interp);
    }
    ckfree(dPtr);
    Tcl_DeleteHashEntry(hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetAssocData --
 *
 *	Returns the client data associated with this name in the specified
 *	interpreter.
 *
 * Results:
 *	The client data in the AssocData record denoted by the named
 *	association, or NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tcl_GetAssocData(
    Tcl_Interp *interp,		/* Interpreter associated with. */
    const char *name,		/* Name of association. */
    Tcl_InterpDeleteProc **procPtr)
				/* Pointer to place to store address of
				 * current deletion callback. */
{
    Interp *iPtr = (Interp *) interp;
    AssocData *dPtr;
    Tcl_HashEntry *hPtr;

    if (iPtr->assocData == NULL) {
	return NULL;
    }
    hPtr = Tcl_FindHashEntry(iPtr->assocData, name);
    if (hPtr == NULL) {
	return NULL;
    }
    dPtr = Tcl_GetHashValue(hPtr);
    if (procPtr != NULL) {
	*procPtr = dPtr->proc;
    }
    return dPtr->clientData;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InterpDeleted --
 *
 *	Returns nonzero if the interpreter has been deleted with a call to
 *	Tcl_DeleteInterp.
 *
 * Results:
 *	Nonzero if the interpreter is deleted, zero otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_InterpDeleted(
    Tcl_Interp *interp)
{
    return (((Interp *) interp)->flags & DELETED) ? 1 : 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteInterp --
 *
 *	Ensures that the interpreter will be deleted eventually. If there are
 *	no Tcl_Preserve calls in effect for this interpreter, it is deleted
 *	immediately, otherwise the interpreter is deleted when the last
 *	Tcl_Preserve is matched by a call to Tcl_Release. In either case, the
 *	function runs the currently registered deletion callbacks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The interpreter is marked as deleted. The caller may still use it
 *	safely if there are calls to Tcl_Preserve in effect for the
 *	interpreter, but further calls to Tcl_Eval etc in this interpreter
 *	will fail.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteInterp(
    Tcl_Interp *interp)		/* Token for command interpreter (returned by
				 * a previous call to Tcl_CreateInterp). */
{
    Interp *iPtr = (Interp *) interp;

    /*
     * If the interpreter has already been marked deleted, just punt.
     */

    if (iPtr->flags & DELETED) {
	return;
    }

    /*
     * Mark the interpreter as deleted. No further evals will be allowed.
     * Increase the compileEpoch as a signal to compiled bytecodes.
     */

    iPtr->flags |= DELETED;
    iPtr->compileEpoch++;

    /*
     * Ensure that the interpreter is eventually deleted.
     */

    Tcl_EventuallyFree(interp, (Tcl_FreeProc *) DeleteInterpProc);
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteInterpProc --
 *
 *	Helper function to delete an interpreter. This function is called when
 *	the last call to Tcl_Preserve on this interpreter is matched by a call
 *	to Tcl_Release. The function cleans up all resources used in the
 *	interpreter and calls all currently registered interpreter deletion
 *	callbacks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the interpreter deletion callbacks do. Frees resources used
 *	by the interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteInterpProc(
    Tcl_Interp *interp)		/* Interpreter to delete. */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;
    Tcl_HashTable *hTablePtr;
    ResolverScheme *resPtr, *nextResPtr;
    int i;

    /*
     * Punt if there is an error in the Tcl_Release/Tcl_Preserve matchup,
	 * unless we are exiting.
     */

    if ((iPtr->numLevels > 0) && !TclInExit()) {
	Tcl_Panic("DeleteInterpProc called with active evals");
    }

    /*
     * The interpreter should already be marked deleted; otherwise how did we
     * get here?
     */

    if (!(iPtr->flags & DELETED)) {
	Tcl_Panic("DeleteInterpProc called on interpreter not marked deleted");
    }

    /*
     * TIP #219, Tcl Channel Reflection API. Discard a leftover state.
     */

    if (iPtr->chanMsg != NULL) {
	Tcl_DecrRefCount(iPtr->chanMsg);
	iPtr->chanMsg = NULL;
    }

    /*
     * TIP #285, Script cancellation support. Delete this interp from the
     * global hash table of CancelInfo structs.
     */

    Tcl_MutexLock(&cancelLock);
    hPtr = Tcl_FindHashEntry(&cancelTable, (char *) iPtr);
    if (hPtr != NULL) {
	CancelInfo *cancelInfo = Tcl_GetHashValue(hPtr);

	if (cancelInfo != NULL) {
	    if (cancelInfo->result != NULL) {
		ckfree(cancelInfo->result);
	    }
	    ckfree(cancelInfo);
	}

	Tcl_DeleteHashEntry(hPtr);
    }

    if (iPtr->asyncCancel != NULL) {
	Tcl_AsyncDelete(iPtr->asyncCancel);
	iPtr->asyncCancel = NULL;
    }

    if (iPtr->asyncCancelMsg != NULL) {
	Tcl_DecrRefCount(iPtr->asyncCancelMsg);
	iPtr->asyncCancelMsg = NULL;
    }
    Tcl_MutexUnlock(&cancelLock);

    /*
     * Shut down all limit handler callback scripts that call back into this
     * interpreter. Then eliminate all limit handlers for this interpreter.
     */

    TclRemoveScriptLimitCallbacks(interp);
    TclLimitRemoveAllHandlers(interp);

    /*
     * Dismantle the namespace here, before we clear the assocData. If any
     * background errors occur here, they will be deleted below.
     *
     * Dismantle the namespace after freeing the iPtr->handle so that each
     * bytecode releases its literals without caring to update the literal
     * table, as it will be freed later in this function without further use.
     */

    TclHandleFree(iPtr->handle);
    TclTeardownNamespace(iPtr->globalNsPtr);

    /*
     * Delete all the hidden commands.
     */

    hTablePtr = iPtr->hiddenCmdTablePtr;
    if (hTablePtr != NULL) {
	/*
	 * Non-pernicious deletion. The deletion callbacks will not be allowed
	 * to create any new hidden or non-hidden commands.
	 * Tcl_DeleteCommandFromToken will remove the entry from the
	 * hiddenCmdTablePtr.
	 */

	hPtr = Tcl_FirstHashEntry(hTablePtr, &search);
	for (; hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    Tcl_DeleteCommandFromToken(interp, Tcl_GetHashValue(hPtr));
	}
	Tcl_DeleteHashTable(hTablePtr);
	ckfree(hTablePtr);
    }

    /*
     * Invoke deletion callbacks; note that a callback can create new
     * callbacks, so we iterate.
     */

    while (iPtr->assocData != NULL) {
	AssocData *dPtr;

	hTablePtr = iPtr->assocData;
	iPtr->assocData = NULL;
	for (hPtr = Tcl_FirstHashEntry(hTablePtr, &search);
		hPtr != NULL;
		hPtr = Tcl_FirstHashEntry(hTablePtr, &search)) {
	    dPtr = Tcl_GetHashValue(hPtr);
	    Tcl_DeleteHashEntry(hPtr);
	    if (dPtr->proc != NULL) {
		dPtr->proc(dPtr->clientData, interp);
	    }
	    ckfree(dPtr);
	}
	Tcl_DeleteHashTable(hTablePtr);
	ckfree(hTablePtr);
    }

    /*
     * Pop the root frame pointer and finish deleting the global
     * namespace. The order is important [Bug 1658572].
     */

    if ((iPtr->framePtr != iPtr->rootFramePtr) && !TclInExit()) {
	Tcl_Panic("DeleteInterpProc: popping rootCallFrame with other frames on top");
    }
    Tcl_PopCallFrame(interp);
    ckfree(iPtr->rootFramePtr);
    iPtr->rootFramePtr = NULL;
    Tcl_DeleteNamespace((Tcl_Namespace *) iPtr->globalNsPtr);

    /*
     * Free up the result *after* deleting variables, since variable deletion
     * could have transferred ownership of the result string to Tcl.
     */

    Tcl_FreeResult(interp);
    iPtr->result = NULL;
    Tcl_DecrRefCount(iPtr->objResultPtr);
    iPtr->objResultPtr = NULL;
    Tcl_DecrRefCount(iPtr->ecVar);
    if (iPtr->errorCode) {
	Tcl_DecrRefCount(iPtr->errorCode);
	iPtr->errorCode = NULL;
    }
    Tcl_DecrRefCount(iPtr->eiVar);
    if (iPtr->errorInfo) {
	Tcl_DecrRefCount(iPtr->errorInfo);
	iPtr->errorInfo = NULL;
    }
    Tcl_DecrRefCount(iPtr->errorStack);
    iPtr->errorStack = NULL;
    Tcl_DecrRefCount(iPtr->upLiteral);
    Tcl_DecrRefCount(iPtr->callLiteral);
    Tcl_DecrRefCount(iPtr->innerLiteral);
    Tcl_DecrRefCount(iPtr->innerContext);
    if (iPtr->returnOpts) {
	Tcl_DecrRefCount(iPtr->returnOpts);
    }
    if (iPtr->appendResult != NULL) {
	ckfree(iPtr->appendResult);
	iPtr->appendResult = NULL;
    }
    TclFreePackageInfo(iPtr);
    while (iPtr->tracePtr != NULL) {
	Tcl_DeleteTrace((Tcl_Interp *) iPtr, (Tcl_Trace) iPtr->tracePtr);
    }
    if (iPtr->execEnvPtr != NULL) {
	TclDeleteExecEnv(iPtr->execEnvPtr);
    }
    if (iPtr->scriptFile) {
	Tcl_DecrRefCount(iPtr->scriptFile);
	iPtr->scriptFile = NULL;
    }
    Tcl_DecrRefCount(iPtr->emptyObjPtr);
    iPtr->emptyObjPtr = NULL;

    resPtr = iPtr->resolverPtr;
    while (resPtr) {
	nextResPtr = resPtr->nextPtr;
	ckfree(resPtr->name);
	ckfree(resPtr);
	resPtr = nextResPtr;
    }

    /*
     * Free up literal objects created for scripts compiled by the
     * interpreter.
     */

    TclDeleteLiteralTable(interp, &iPtr->literalTable);

    /*
     * TIP #280 - Release the arrays for ByteCode/Proc extension, and
     * contents.
     */

    for (hPtr = Tcl_FirstHashEntry(iPtr->linePBodyPtr, &search);
	    hPtr != NULL;
	    hPtr = Tcl_NextHashEntry(&search)) {
	CmdFrame *cfPtr = Tcl_GetHashValue(hPtr);
	Proc *procPtr = (Proc *) Tcl_GetHashKey(iPtr->linePBodyPtr, hPtr);

	procPtr->iPtr = NULL;
	if (cfPtr) {
	    if (cfPtr->type == TCL_LOCATION_SOURCE) {
		Tcl_DecrRefCount(cfPtr->data.eval.path);
	    }
	    ckfree(cfPtr->line);
	    ckfree(cfPtr);
	}
	Tcl_DeleteHashEntry(hPtr);
    }
    Tcl_DeleteHashTable(iPtr->linePBodyPtr);
    ckfree(iPtr->linePBodyPtr);
    iPtr->linePBodyPtr = NULL;

    /*
     * See also tclCompile.c, TclCleanupByteCode
     */

    for (hPtr = Tcl_FirstHashEntry(iPtr->lineBCPtr, &search);
	    hPtr != NULL;
	    hPtr = Tcl_NextHashEntry(&search)) {
	ExtCmdLoc *eclPtr = Tcl_GetHashValue(hPtr);

	if (eclPtr->type == TCL_LOCATION_SOURCE) {
	    Tcl_DecrRefCount(eclPtr->path);
	}
	for (i=0; i< eclPtr->nuloc; i++) {
	    ckfree(eclPtr->loc[i].line);
	}

	if (eclPtr->loc != NULL) {
	    ckfree(eclPtr->loc);
	}

	ckfree(eclPtr);
	Tcl_DeleteHashEntry(hPtr);
    }
    Tcl_DeleteHashTable(iPtr->lineBCPtr);
    ckfree(iPtr->lineBCPtr);
    iPtr->lineBCPtr = NULL;

    /*
     * Location stack for uplevel/eval/... scripts which were passed through
     * proc arguments. Actually we track all arguments as we do not and cannot
     * know which arguments will be used as scripts and which will not.
     */

    if (iPtr->lineLAPtr->numEntries && !TclInExit()) {
	/*
	 * When the interp goes away we have nothing on the stack, so there
	 * are no arguments, so this table has to be empty.
	 */

	Tcl_Panic("Argument location tracking table not empty");
    }

    Tcl_DeleteHashTable(iPtr->lineLAPtr);
    ckfree((char *) iPtr->lineLAPtr);
    iPtr->lineLAPtr = NULL;

    if (iPtr->lineLABCPtr->numEntries && !TclInExit()) {
	/*
	 * When the interp goes away we have nothing on the stack, so there
	 * are no arguments, so this table has to be empty.
	 */

	Tcl_Panic("Argument location tracking table not empty");
    }

    Tcl_DeleteHashTable(iPtr->lineLABCPtr);
    ckfree(iPtr->lineLABCPtr);
    iPtr->lineLABCPtr = NULL;

    /*
     * Squelch the tables of traces on variables and searches over arrays in
     * the in the interpreter.
     */

    Tcl_DeleteHashTable(&iPtr->varTraces);
    Tcl_DeleteHashTable(&iPtr->varSearches);

    ckfree(iPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_HideCommand --
 *
 *	Makes a command hidden so that it cannot be invoked from within an
 *	interpreter, only from within an ancestor.
 *
 * Results:
 *	A standard Tcl result; also leaves a message in the interp's result if
 *	an error occurs.
 *
 * Side effects:
 *	Removes a command from the command table and create an entry into the
 *	hidden command table under the specified token name.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_HideCommand(
    Tcl_Interp *interp,		/* Interpreter in which to hide command. */
    const char *cmdName,	/* Name of command to hide. */
    const char *hiddenCmdToken)	/* Token name of the to-be-hidden command. */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Command cmd;
    Command *cmdPtr;
    Tcl_HashTable *hiddenCmdTablePtr;
    Tcl_HashEntry *hPtr;
    int isNew;

    if (iPtr->flags & DELETED) {
	/*
	 * The interpreter is being deleted. Do not create any new structures,
	 * because it is not safe to modify the interpreter.
	 */

	return TCL_ERROR;
    }

    /*
     * Disallow hiding of commands that are currently in a namespace or
     * renaming (as part of hiding) into a namespace (because the current
     * implementation with a single global table and the needed uniqueness of
     * names cause problems with namespaces).
     *
     * We don't need to check for "::" in cmdName because the real check is on
     * the nsPtr below.
     *
     * hiddenCmdToken is just a string which is not interpreted in any way. It
     * may contain :: but the string is not interpreted as a namespace
     * qualifier command name. Thus, hiding foo::bar to foo::bar and then
     * trying to expose or invoke ::foo::bar will NOT work; but if the
     * application always uses the same strings it will get consistent
     * behaviour.
     *
     * But as we currently limit ourselves to the global namespace only for
     * the source, in order to avoid potential confusion, lets prevent "::" in
     * the token too. - dl
     */

    if (strstr(hiddenCmdToken, "::") != NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"cannot use namespace qualifiers in hidden command"
		" token (rename)", -1));
        Tcl_SetErrorCode(interp, "TCL", "VALUE", "HIDDENTOKEN", NULL);
	return TCL_ERROR;
    }

    /*
     * Find the command to hide. An error is returned if cmdName can't be
     * found. Look up the command only from the global namespace. Full path of
     * the command must be given if using namespaces.
     */

    cmd = Tcl_FindCommand(interp, cmdName, NULL,
	    /*flags*/ TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
    if (cmd == (Tcl_Command) NULL) {
	return TCL_ERROR;
    }
    cmdPtr = (Command *) cmd;

    /*
     * Check that the command is really in global namespace
     */

    if (cmdPtr->nsPtr != iPtr->globalNsPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "can only hide global namespace commands (use rename then hide)",
                -1));
        Tcl_SetErrorCode(interp, "TCL", "HIDE", "NON_GLOBAL", NULL);
	return TCL_ERROR;
    }

    /*
     * Initialize the hidden command table if necessary.
     */

    hiddenCmdTablePtr = iPtr->hiddenCmdTablePtr;
    if (hiddenCmdTablePtr == NULL) {
	hiddenCmdTablePtr = ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(hiddenCmdTablePtr, TCL_STRING_KEYS);
	iPtr->hiddenCmdTablePtr = hiddenCmdTablePtr;
    }

    /*
     * It is an error to move an exposed command to a hidden command with
     * hiddenCmdToken if a hidden command with the name hiddenCmdToken already
     * exists.
     */

    hPtr = Tcl_CreateHashEntry(hiddenCmdTablePtr, hiddenCmdToken, &isNew);
    if (!isNew) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "hidden command named \"%s\" already exists",
                hiddenCmdToken));
        Tcl_SetErrorCode(interp, "TCL", "HIDE", "ALREADY_HIDDEN", NULL);
	return TCL_ERROR;
    }

    /*
     * NB: This code is currently 'like' a rename to a specialy set apart name
     * table. Changes here and in TclRenameCommand must be kept in synch until
     * the common parts are actually factorized out.
     */

    /*
     * Remove the hash entry for the command from the interpreter command
     * table. This is like deleting the command, so bump its command epoch;
     * this invalidates any cached references that point to the command.
     */

    if (cmdPtr->hPtr != NULL) {
	Tcl_DeleteHashEntry(cmdPtr->hPtr);
	cmdPtr->hPtr = NULL;
	cmdPtr->cmdEpoch++;
    }

    /*
     * The list of command exported from the namespace might have changed.
     * However, we do not need to recompute this just yet; next time we need
     * the info will be soon enough.
     */

    TclInvalidateNsCmdLookup(cmdPtr->nsPtr);

    /*
     * Now link the hash table entry with the command structure. We ensured
     * above that the nsPtr was right.
     */

    cmdPtr->hPtr = hPtr;
    Tcl_SetHashValue(hPtr, cmdPtr);

    /*
     * If the command being hidden has a compile function, increment the
     * interpreter's compileEpoch to invalidate its compiled code. This makes
     * sure that we don't later try to execute old code compiled with
     * command-specific (i.e., inline) bytecodes for the now-hidden command.
     * This field is checked in Tcl_EvalObj and ObjInterpProc, and code whose
     * compilation epoch doesn't match is recompiled.
     */

    if (cmdPtr->compileProc != NULL) {
	iPtr->compileEpoch++;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ExposeCommand --
 *
 *	Makes a previously hidden command callable from inside the interpreter
 *	instead of only by its ancestors.
 *
 * Results:
 *	A standard Tcl result. If an error occurs, a message is left in the
 *	interp's result.
 *
 * Side effects:
 *	Moves commands from one hash table to another.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ExposeCommand(
    Tcl_Interp *interp,		/* Interpreter in which to make command
				 * callable. */
    const char *hiddenCmdToken,	/* Name of hidden command. */
    const char *cmdName)	/* Name of to-be-exposed command. */
{
    Interp *iPtr = (Interp *) interp;
    Command *cmdPtr;
    Namespace *nsPtr;
    Tcl_HashEntry *hPtr;
    Tcl_HashTable *hiddenCmdTablePtr;
    int isNew;

    if (iPtr->flags & DELETED) {
	/*
	 * The interpreter is being deleted. Do not create any new structures,
	 * because it is not safe to modify the interpreter.
	 */

	return TCL_ERROR;
    }

    /*
     * Check that we have a regular name for the command (that the user is not
     * trying to do an expose and a rename (to another namespace) at the same
     * time).
     */

    if (strstr(cmdName, "::") != NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "cannot expose to a namespace (use expose to toplevel, then rename)",
                -1));
        Tcl_SetErrorCode(interp, "TCL", "EXPOSE", "NON_GLOBAL", NULL);
	return TCL_ERROR;
    }

    /*
     * Get the command from the hidden command table:
     */

    hPtr = NULL;
    hiddenCmdTablePtr = iPtr->hiddenCmdTablePtr;
    if (hiddenCmdTablePtr != NULL) {
	hPtr = Tcl_FindHashEntry(hiddenCmdTablePtr, hiddenCmdToken);
    }
    if (hPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "unknown hidden command \"%s\"", hiddenCmdToken));
        Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "HIDDENTOKEN",
                hiddenCmdToken, NULL);
	return TCL_ERROR;
    }
    cmdPtr = Tcl_GetHashValue(hPtr);

    /*
     * Check that we have a true global namespace command (enforced by
     * Tcl_HideCommand but let's double check. (If it was not, we would not
     * really know how to handle it).
     */

    if (cmdPtr->nsPtr != iPtr->globalNsPtr) {
	/*
	 * This case is theoritically impossible, we might rather Tcl_Panic
	 * than 'nicely' erroring out ?
	 */

	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"trying to expose a non-global command namespace command",
		-1));
	return TCL_ERROR;
    }

    /*
     * This is the global table.
     */

    nsPtr = cmdPtr->nsPtr;

    /*
     * It is an error to overwrite an existing exposed command as a result of
     * exposing a previously hidden command.
     */

    hPtr = Tcl_CreateHashEntry(&nsPtr->cmdTable, cmdName, &isNew);
    if (!isNew) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "exposed command \"%s\" already exists", cmdName));
        Tcl_SetErrorCode(interp, "TCL", "EXPOSE", "COMMAND_EXISTS", NULL);
	return TCL_ERROR;
    }

    /*
     * Command resolvers (per-interp, per-namespace) might have resolved to a
     * command for the given namespace scope with this command not being
     * registered with the namespace's command table. During BC compilation,
     * the so-resolved command turns into a CmdName literal. Without
     * invalidating a possible CmdName literal here explicitly, such literals
     * keep being reused while pointing to overhauled commands.
     */

    TclInvalidateCmdLiteral(interp, cmdName, nsPtr);

    /*
     * The list of command exported from the namespace might have changed.
     * However, we do not need to recompute this just yet; next time we need
     * the info will be soon enough.
     */

    TclInvalidateNsCmdLookup(nsPtr);

    /*
     * Remove the hash entry for the command from the interpreter hidden
     * command table.
     */

    if (cmdPtr->hPtr != NULL) {
	Tcl_DeleteHashEntry(cmdPtr->hPtr);
	cmdPtr->hPtr = NULL;
    }

    /*
     * Now link the hash table entry with the command structure. This is like
     * creating a new command, so deal with any shadowing of commands in the
     * global namespace.
     */

    cmdPtr->hPtr = hPtr;

    Tcl_SetHashValue(hPtr, cmdPtr);

    /*
     * Not needed as we are only in the global namespace (but would be needed
     * again if we supported namespace command hiding)
     *
     * TclResetShadowedCmdRefs(interp, cmdPtr);
     */

    /*
     * If the command being exposed has a compile function, increment
     * interpreter's compileEpoch to invalidate its compiled code. This makes
     * sure that we don't later try to execute old code compiled assuming the
     * command is hidden. This field is checked in Tcl_EvalObj and
     * ObjInterpProc, and code whose compilation epoch doesn't match is
     * recompiled.
     */

    if (cmdPtr->compileProc != NULL) {
	iPtr->compileEpoch++;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateCommand --
 *
 *	Define a new command in a command table.
 *
 * Results:
 *	The return value is a token for the command, which can be used in
 *	future calls to Tcl_GetCommandName.
 *
 * Side effects:
 *	If a command named cmdName already exists for interp, it is deleted.
 *	In the future, when cmdName is seen as the name of a command by
 *	Tcl_Eval, proc will be called. To support the bytecode interpreter,
 *	the command is created with a wrapper Tcl_ObjCmdProc
 *	(TclInvokeStringCommand) that eventially calls proc. When the command
 *	is deleted from the table, deleteProc will be called. See the manual
 *	entry for details on the calling sequence.
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
Tcl_CreateCommand(
    Tcl_Interp *interp,		/* Token for command interpreter returned by a
				 * previous call to Tcl_CreateInterp. */
    const char *cmdName,	/* Name of command. If it contains namespace
				 * qualifiers, the new command is put in the
				 * specified namespace; otherwise it is put in
				 * the global namespace. */
    Tcl_CmdProc *proc,		/* Function to associate with cmdName. */
    ClientData clientData,	/* Arbitrary value passed to string proc. */
    Tcl_CmdDeleteProc *deleteProc)
				/* If not NULL, gives a function to call when
				 * this command is deleted. */
{
    Interp *iPtr = (Interp *) interp;
    ImportRef *oldRefPtr = NULL;
    Namespace *nsPtr;
    Command *cmdPtr;
    Tcl_HashEntry *hPtr;
    const char *tail;
    int isNew = 0, deleted = 0;
    ImportedCmdData *dataPtr;

    if (iPtr->flags & DELETED) {
	/*
	 * The interpreter is being deleted. Don't create any new commands;
	 * it's not safe to muck with the interpreter anymore.
	 */

	return (Tcl_Command) NULL;
    }

    /*
     * If the command name we seek to create already exists, we need to
     * delete that first.  That can be tricky in the presence of traces.
     * Loop until we no longer find an existing command in the way, or
     * until we've deleted one command and that didn't finish the job.
     */

    while (1) {
        /*
         * Determine where the command should reside. If its name contains
         * namespace qualifiers, we put it in the specified namespace;
	 * otherwise, we always put it in the global namespace.
         */

        if (strstr(cmdName, "::") != NULL) {
	    Namespace *dummy1, *dummy2;

	    TclGetNamespaceForQualName(interp, cmdName, NULL,
		    TCL_CREATE_NS_IF_UNKNOWN, &nsPtr, &dummy1, &dummy2, &tail);
	    if ((nsPtr == NULL) || (tail == NULL)) {
	        return (Tcl_Command) NULL;
	    }
        } else {
	    nsPtr = iPtr->globalNsPtr;
	    tail = cmdName;
        }

        hPtr = Tcl_CreateHashEntry(&nsPtr->cmdTable, tail, &isNew);

	if (isNew || deleted) {
	    /*
	     * isNew - No conflict with existing command.
	     * deleted - We've already deleted a conflicting command
	     */
	    break;
	}

	/* An existing command conflicts. Try to delete it.. */
	cmdPtr = Tcl_GetHashValue(hPtr);

	/*
	 * Be careful to preserve
	 * any existing import links so we can restore them down below. That
	 * way, you can redefine a command and its import status will remain
	 * intact.
	 */

	cmdPtr->refCount++;
	if (cmdPtr->importRefPtr) {
	    cmdPtr->flags |= CMD_REDEF_IN_PROGRESS;
	}

	Tcl_DeleteCommandFromToken(interp, (Tcl_Command) cmdPtr);

	if (cmdPtr->flags & CMD_REDEF_IN_PROGRESS) {
	    oldRefPtr = cmdPtr->importRefPtr;
	    cmdPtr->importRefPtr = NULL;
	}
	TclCleanupCommandMacro(cmdPtr);
	deleted = 1;
    }

    if (!isNew) {
	/*
	 * If the deletion callback recreated the command, just throw away
	 * the new command (if we try to delete it again, we could get
	 * stuck in an infinite loop).
	 */

	ckfree(Tcl_GetHashValue(hPtr));
    }

    if (!deleted) {

	/*
	 * Command resolvers (per-interp, per-namespace) might have resolved
	 * to a command for the given namespace scope with this command not
	 * being registered with the namespace's command table. During BC
	 * compilation, the so-resolved command turns into a CmdName literal.
	 * Without invalidating a possible CmdName literal here explicitly,
	 * such literals keep being reused while pointing to overhauled
	 * commands.
	 */

	TclInvalidateCmdLiteral(interp, tail, nsPtr);

	/*
	 * The list of command exported from the namespace might have changed.
	 * However, we do not need to recompute this just yet; next time we
	 * need the info will be soon enough.
	 */

	TclInvalidateNsCmdLookup(nsPtr);
	TclInvalidateNsPath(nsPtr);
    }
    cmdPtr = ckalloc(sizeof(Command));
    Tcl_SetHashValue(hPtr, cmdPtr);
    cmdPtr->hPtr = hPtr;
    cmdPtr->nsPtr = nsPtr;
    cmdPtr->refCount = 1;
    cmdPtr->cmdEpoch = 0;
    cmdPtr->compileProc = NULL;
    cmdPtr->objProc = TclInvokeStringCommand;
    cmdPtr->objClientData = cmdPtr;
    cmdPtr->proc = proc;
    cmdPtr->clientData = clientData;
    cmdPtr->deleteProc = deleteProc;
    cmdPtr->deleteData = clientData;
    cmdPtr->flags = 0;
    cmdPtr->importRefPtr = NULL;
    cmdPtr->tracePtr = NULL;
    cmdPtr->nreProc = NULL;

    /*
     * Plug in any existing import references found above. Be sure to update
     * all of these references to point to the new command.
     */

    if (oldRefPtr != NULL) {
	cmdPtr->importRefPtr = oldRefPtr;
	while (oldRefPtr != NULL) {
	    Command *refCmdPtr = oldRefPtr->importedCmdPtr;
	    dataPtr = refCmdPtr->objClientData;
	    dataPtr->realCmdPtr = cmdPtr;
	    oldRefPtr = oldRefPtr->nextPtr;
	}
    }

    /*
     * We just created a command, so in its namespace and all of its parent
     * namespaces, it may shadow global commands with the same name. If any
     * shadowed commands are found, invalidate all cached command references
     * in the affected namespaces.
     */

    TclResetShadowedCmdRefs(interp, cmdPtr);
    return (Tcl_Command) cmdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateObjCommand --
 *
 *	Define a new object-based command in a command table.
 *
 * Results:
 *	The return value is a token for the command, which can be used in
 *	future calls to Tcl_GetCommandName.
 *
 * Side effects:
 *	If a command named "cmdName" already exists for interp, it is
 *	first deleted.  Then the new command is created from the arguments.
 *	[***] (See below for exception).
 *
 *	In the future, during bytecode evaluation when "cmdName" is seen as
 *	the name of a command by Tcl_EvalObj or Tcl_Eval, the object-based
 *	Tcl_ObjCmdProc proc will be called. When the command is deleted from
 *	the table, deleteProc will be called. See the manual entry for details
 *	on the calling sequence.
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
Tcl_CreateObjCommand(
    Tcl_Interp *interp,		/* Token for command interpreter (returned by
				 * previous call to Tcl_CreateInterp). */
    const char *cmdName,	/* Name of command. If it contains namespace
				 * qualifiers, the new command is put in the
				 * specified namespace; otherwise it is put in
				 * the global namespace. */
    Tcl_ObjCmdProc *proc,	/* Object-based function to associate with
				 * name. */
    ClientData clientData,	/* Arbitrary value to pass to object
				 * function. */
    Tcl_CmdDeleteProc *deleteProc
				/* If not NULL, gives a function to call when
				 * this command is deleted. */
)
{
    Interp *iPtr = (Interp *) interp;
    Namespace *nsPtr;
    const char *tail;

    if (iPtr->flags & DELETED) {
	/*
	 * The interpreter is being deleted. Don't create any new commands;
	 * it's not safe to muck with the interpreter anymore.
	 */
	return (Tcl_Command) NULL;
    }

    /*
     * Determine where the command should reside. If its name contains
     * namespace qualifiers, we put it in the specified namespace;
     * otherwise, we always put it in the global namespace.
     */

    if (strstr(cmdName, "::") != NULL) {
	Namespace *dummy1, *dummy2;

	TclGetNamespaceForQualName(interp, cmdName, NULL,
	    TCL_CREATE_NS_IF_UNKNOWN, &nsPtr, &dummy1, &dummy2, &tail);
	if ((nsPtr == NULL) || (tail == NULL)) {
	    return (Tcl_Command) NULL;
	}
    } else {
	nsPtr = iPtr->globalNsPtr;
	tail = cmdName;
    }

    return TclCreateObjCommandInNs(interp, tail, (Tcl_Namespace *) nsPtr,
	proc, clientData, deleteProc);
}

Tcl_Command
TclCreateObjCommandInNs (
    Tcl_Interp *interp,
    const char *cmdName,	/* Name of command, without any namespace components */
    Tcl_Namespace *namespace,   /* The namespace to create the command in */
    Tcl_ObjCmdProc *proc,	/* Object-based function to associate with
				 * name. */
    ClientData clientData,	/* Arbitrary value to pass to object
				 * function. */
    Tcl_CmdDeleteProc *deleteProc
				/* If not NULL, gives a function to call when
				 * this command is deleted. */
) {
    int deleted = 0, isNew = 0;
    Command *cmdPtr;
    ImportRef *oldRefPtr = NULL;
    ImportedCmdData *dataPtr;
    Tcl_HashEntry *hPtr;
    Namespace *nsPtr = (Namespace *) namespace;
    /*
     * If the command name we seek to create already exists, we need to
     * delete that first.  That can be tricky in the presence of traces.
     * Loop until we no longer find an existing command in the way, or
     * until we've deleted one command and that didn't finish the job.
     */
    while (1) {
	hPtr = Tcl_CreateHashEntry(&nsPtr->cmdTable, cmdName, &isNew);

	if (isNew || deleted) {
	    /*
	     * isNew - No conflict with existing command.
	     * deleted - We've already deleted a conflicting command
	     */
	    break;
	}


	/* An existing command conflicts. Try to delete it.. */
	cmdPtr = Tcl_GetHashValue(hPtr);

	/*
	 * [***] This is wrong.  See Tcl Bug a16752c252.
	 * However, this buggy behavior is kept under particular
	 * circumstances to accommodate deployed binaries of the
	 * "tclcompiler" program. http://sourceforge.net/projects/tclpro/
	 * that crash if the bug is fixed.
	 */

	if (cmdPtr->objProc == TclInvokeStringCommand
		&& cmdPtr->clientData == clientData
		&& cmdPtr->deleteData == clientData
		&& cmdPtr->deleteProc == deleteProc) {
	    cmdPtr->objProc = proc;
	    cmdPtr->objClientData = clientData;
	    return (Tcl_Command) cmdPtr;
	}

	/*
	 * Otherwise, we delete the old command. Be careful to preserve any
	 * existing import links so we can restore them down below. That way,
	 * you can redefine a command and its import status will remain
	 * intact.
	 */

	cmdPtr->refCount++;
	if (cmdPtr->importRefPtr) {
	    cmdPtr->flags |= CMD_REDEF_IN_PROGRESS;
	}

	/* Make sure namespace doesn't get deallocated. */
	cmdPtr->nsPtr->refCount++;

	Tcl_DeleteCommandFromToken(interp, (Tcl_Command) cmdPtr);
	nsPtr = (Namespace *) TclEnsureNamespace(interp,
	    (Tcl_Namespace *)cmdPtr->nsPtr);
	TclNsDecrRefCount(cmdPtr->nsPtr);

	if (cmdPtr->flags & CMD_REDEF_IN_PROGRESS) {
	    oldRefPtr = cmdPtr->importRefPtr;
	    cmdPtr->importRefPtr = NULL;
	}
	TclCleanupCommandMacro(cmdPtr);
	deleted = 1;
    }
    if (!isNew) {
	/*
	 * If the deletion callback recreated the command, just throw away
	 * the new command (if we try to delete it again, we could get
	 * stuck in an infinite loop).
	 */

	ckfree(Tcl_GetHashValue(hPtr));
    }

    if (!deleted) {
	/*
	 * Command resolvers (per-interp, per-namespace) might have resolved
	 * to a command for the given namespace scope with this command not
	 * being registered with the namespace's command table. During BC
	 * compilation, the so-resolved command turns into a CmdName literal.
	 * Without invalidating a possible CmdName literal here explicitly,
	 * such literals keep being reused while pointing to overhauled
	 * commands.
	 */

	TclInvalidateCmdLiteral(interp, cmdName, nsPtr);

	/*
	 * The list of command exported from the namespace might have changed.
	 * However, we do not need to recompute this just yet; next time we
	 * need the info will be soon enough.
	 */

	TclInvalidateNsCmdLookup(nsPtr);
	TclInvalidateNsPath(nsPtr);
    }
    cmdPtr = ckalloc(sizeof(Command));
    Tcl_SetHashValue(hPtr, cmdPtr);
    cmdPtr->hPtr = hPtr;
    cmdPtr->nsPtr = nsPtr;
    cmdPtr->refCount = 1;
    cmdPtr->cmdEpoch = 0;
    cmdPtr->compileProc = NULL;
    cmdPtr->objProc = proc;
    cmdPtr->objClientData = clientData;
    cmdPtr->proc = TclInvokeObjectCommand;
    cmdPtr->clientData = cmdPtr;
    cmdPtr->deleteProc = deleteProc;
    cmdPtr->deleteData = clientData;
    cmdPtr->flags = 0;
    cmdPtr->importRefPtr = NULL;
    cmdPtr->tracePtr = NULL;
    cmdPtr->nreProc = NULL;

    /*
     * Plug in any existing import references found above. Be sure to update
     * all of these references to point to the new command.
     */

    if (oldRefPtr != NULL) {
	cmdPtr->importRefPtr = oldRefPtr;
	while (oldRefPtr != NULL) {
	    Command *refCmdPtr = oldRefPtr->importedCmdPtr;
	    dataPtr = refCmdPtr->objClientData;
	    dataPtr->realCmdPtr = cmdPtr;
	    oldRefPtr = oldRefPtr->nextPtr;
	}
    }

    /*
     * We just created a command, so in its namespace and all of its parent
     * namespaces, it may shadow global commands with the same name. If any
     * shadowed commands are found, invalidate all cached command references
     * in the affected namespaces.
     */

    TclResetShadowedCmdRefs(interp, cmdPtr);
    return (Tcl_Command) cmdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInvokeStringCommand --
 *
 *	"Wrapper" Tcl_ObjCmdProc used to call an existing string-based
 *	Tcl_CmdProc if no object-based function exists for a command. A
 *	pointer to this function is stored as the Tcl_ObjCmdProc in a Command
 *	structure. It simply turns around and calls the string Tcl_CmdProc in
 *	the Command structure.
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	Besides those side effects of the called Tcl_CmdProc,
 *	TclInvokeStringCommand allocates and frees storage.
 *
 *----------------------------------------------------------------------
 */

int
TclInvokeStringCommand(
    ClientData clientData,	/* Points to command's Command structure. */
    Tcl_Interp *interp,		/* Current interpreter. */
    register int objc,		/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Command *cmdPtr = clientData;
    int i, result;
    const char **argv =
	    TclStackAlloc(interp, (unsigned)(objc + 1) * sizeof(char *));

    for (i = 0; i < objc; i++) {
	argv[i] = Tcl_GetString(objv[i]);
    }
    argv[objc] = 0;

    /*
     * Invoke the command's string-based Tcl_CmdProc.
     */

    result = cmdPtr->proc(cmdPtr->clientData, interp, objc, argv);

    TclStackFree(interp, (void *) argv);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInvokeObjectCommand --
 *
 *	"Wrapper" Tcl_CmdProc used to call an existing object-based
 *	Tcl_ObjCmdProc if no string-based function exists for a command. A
 *	pointer to this function is stored as the Tcl_CmdProc in a Command
 *	structure. It simply turns around and calls the object Tcl_ObjCmdProc
 *	in the Command structure.
 *
 * Results:
 *	A standard Tcl string result value.
 *
 * Side effects:
 *	Besides those side effects of the called Tcl_ObjCmdProc,
 *	TclInvokeObjectCommand allocates and frees storage.
 *
 *----------------------------------------------------------------------
 */

int
TclInvokeObjectCommand(
    ClientData clientData,	/* Points to command's Command structure. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    register const char **argv)	/* Argument strings. */
{
    Command *cmdPtr = clientData;
    Tcl_Obj *objPtr;
    int i, length, result;
    Tcl_Obj **objv =
	    TclStackAlloc(interp, (unsigned)(argc * sizeof(Tcl_Obj *)));

    for (i = 0; i < argc; i++) {
	length = strlen(argv[i]);
	TclNewStringObj(objPtr, argv[i], length);
	Tcl_IncrRefCount(objPtr);
	objv[i] = objPtr;
    }

    /*
     * Invoke the command's object-based Tcl_ObjCmdProc.
     */

    if (cmdPtr->objProc != NULL) {
	result = cmdPtr->objProc(cmdPtr->objClientData, interp, argc, objv);
    } else {
	result = Tcl_NRCallObjProc(interp, cmdPtr->nreProc,
		cmdPtr->objClientData, argc, objv);
    }

    /*
     * Move the interpreter's object result to the string result, then reset
     * the object result.
     */

    (void) Tcl_GetStringResult(interp);

    /*
     * Decrement the ref counts for the argument objects created above, then
     * free the objv array if malloc'ed storage was used.
     */

    for (i = 0; i < argc; i++) {
	objPtr = objv[i];
	Tcl_DecrRefCount(objPtr);
    }
    TclStackFree(interp, objv);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclRenameCommand --
 *
 *	Called to give an existing Tcl command a different name. Both the old
 *	command name and the new command name can have "::" namespace
 *	qualifiers. If the new command has a different namespace context, the
 *	command will be moved to that namespace and will execute in the
 *	context of that new namespace.
 *
 *	If the new command name is NULL or the null string, the command is
 *	deleted.
 *
 * Results:
 *	Returns TCL_OK if successful, and TCL_ERROR if anything goes wrong.
 *
 * Side effects:
 *	If anything goes wrong, an error message is returned in the
 *	interpreter's result object.
 *
 *----------------------------------------------------------------------
 */

int
TclRenameCommand(
    Tcl_Interp *interp,		/* Current interpreter. */
    const char *oldName,	/* Existing command name. */
    const char *newName)	/* New command name. */
{
    Interp *iPtr = (Interp *) interp;
    const char *newTail;
    Namespace *cmdNsPtr, *newNsPtr, *dummy1, *dummy2;
    Tcl_Command cmd;
    Command *cmdPtr;
    Tcl_HashEntry *hPtr, *oldHPtr;
    int isNew, result;
    Tcl_Obj *oldFullName;
    Tcl_DString newFullName;

    /*
     * Find the existing command. An error is returned if cmdName can't be
     * found.
     */

    cmd = Tcl_FindCommand(interp, oldName, NULL, /*flags*/ 0);
    cmdPtr = (Command *) cmd;
    if (cmdPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "can't %s \"%s\": command doesn't exist",
		((newName == NULL)||(*newName == '\0'))? "delete":"rename",
		oldName));
        Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "COMMAND", oldName, NULL);
	return TCL_ERROR;
    }

    /*
     * If the new command name is NULL or empty, delete the command. Do this
     * with Tcl_DeleteCommandFromToken, since we already have the command.
     */

    if ((newName == NULL) || (*newName == '\0')) {
	Tcl_DeleteCommandFromToken(interp, cmd);
	return TCL_OK;
    }

    cmdNsPtr = cmdPtr->nsPtr;
    oldFullName = Tcl_NewObj();
    Tcl_IncrRefCount(oldFullName);
    Tcl_GetCommandFullName(interp, cmd, oldFullName);

    /*
     * Make sure that the destination command does not already exist. The
     * rename operation is like creating a command, so we should automatically
     * create the containing namespaces just like Tcl_CreateCommand would.
     */

    TclGetNamespaceForQualName(interp, newName, NULL,
	    TCL_CREATE_NS_IF_UNKNOWN, &newNsPtr, &dummy1, &dummy2, &newTail);

    if ((newNsPtr == NULL) || (newTail == NULL)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "can't rename to \"%s\": bad command name", newName));
        Tcl_SetErrorCode(interp, "TCL", "VALUE", "COMMAND", NULL);
	result = TCL_ERROR;
	goto done;
    }
    if (Tcl_FindHashEntry(&newNsPtr->cmdTable, newTail) != NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "can't rename to \"%s\": command already exists", newName));
        Tcl_SetErrorCode(interp, "TCL", "OPERATION", "RENAME",
                "TARGET_EXISTS", NULL);
	result = TCL_ERROR;
	goto done;
    }

    /*
     * Warning: any changes done in the code here are likely to be needed in
     * Tcl_HideCommand code too (until the common parts are extracted out).
     * - dl
     */

    /*
     * Put the command in the new namespace so we can check for an alias loop.
     * Since we are adding a new command to a namespace, we must handle any
     * shadowing of the global commands that this might create.
     */

    oldHPtr = cmdPtr->hPtr;
    hPtr = Tcl_CreateHashEntry(&newNsPtr->cmdTable, newTail, &isNew);
    Tcl_SetHashValue(hPtr, cmdPtr);
    cmdPtr->hPtr = hPtr;
    cmdPtr->nsPtr = newNsPtr;
    TclResetShadowedCmdRefs(interp, cmdPtr);

    /*
     * Now check for an alias loop. If we detect one, put everything back the
     * way it was and report the error.
     */

    result = TclPreventAliasLoop(interp, interp, (Tcl_Command) cmdPtr);
    if (result != TCL_OK) {
	Tcl_DeleteHashEntry(cmdPtr->hPtr);
	cmdPtr->hPtr = oldHPtr;
	cmdPtr->nsPtr = cmdNsPtr;
	goto done;
    }

    /*
     * The list of command exported from the namespace might have changed.
     * However, we do not need to recompute this just yet; next time we need
     * the info will be soon enough. These might refer to the same variable,
     * but that's no big deal.
     */

    TclInvalidateNsCmdLookup(cmdNsPtr);
    TclInvalidateNsCmdLookup(cmdPtr->nsPtr);

    /*
     * Command resolvers (per-interp, per-namespace) might have resolved to a
     * command for the given namespace scope with this command not being
     * registered with the namespace's command table. During BC compilation,
     * the so-resolved command turns into a CmdName literal. Without
     * invalidating a possible CmdName literal here explicitly, such literals
     * keep being reused while pointing to overhauled commands.
     */

    TclInvalidateCmdLiteral(interp, newTail, cmdPtr->nsPtr);

    /*
     * Script for rename traces can delete the command "oldName". Therefore
     * increment the reference count for cmdPtr so that it's Command structure
     * is freed only towards the end of this function by calling
     * TclCleanupCommand.
     *
     * The trace function needs to get a fully qualified name for old and new
     * commands [Tcl bug #651271], or else there's no way for the trace
     * function to get the namespace from which the old command is being
     * renamed!
     */

    Tcl_DStringInit(&newFullName);
    Tcl_DStringAppend(&newFullName, newNsPtr->fullName, -1);
    if (newNsPtr != iPtr->globalNsPtr) {
	TclDStringAppendLiteral(&newFullName, "::");
    }
    Tcl_DStringAppend(&newFullName, newTail, -1);
    cmdPtr->refCount++;
    CallCommandTraces(iPtr, cmdPtr, Tcl_GetString(oldFullName),
	    Tcl_DStringValue(&newFullName), TCL_TRACE_RENAME);
    Tcl_DStringFree(&newFullName);

    /*
     * The new command name is okay, so remove the command from its current
     * namespace. This is like deleting the command, so bump the cmdEpoch to
     * invalidate any cached references to the command.
     */

    Tcl_DeleteHashEntry(oldHPtr);
    cmdPtr->cmdEpoch++;

    /*
     * If the command being renamed has a compile function, increment the
     * interpreter's compileEpoch to invalidate its compiled code. This makes
     * sure that we don't later try to execute old code compiled for the
     * now-renamed command.
     */

    if (cmdPtr->compileProc != NULL) {
	iPtr->compileEpoch++;
    }

    /*
     * Now free the Command structure, if the "oldName" command has been
     * deleted by invocation of rename traces.
     */

    TclCleanupCommandMacro(cmdPtr);
    result = TCL_OK;

  done:
    TclDecrRefCount(oldFullName);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetCommandInfo --
 *
 *	Modifies various information about a Tcl command. Note that this
 *	function will not change a command's namespace; use TclRenameCommand
 *	to do that. Also, the isNativeObjectProc member of *infoPtr is
 *	ignored.
 *
 * Results:
 *	If cmdName exists in interp, then the information at *infoPtr is
 *	stored with the command in place of the current information and 1 is
 *	returned. If the command doesn't exist then 0 is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetCommandInfo(
    Tcl_Interp *interp,		/* Interpreter in which to look for
				 * command. */
    const char *cmdName,	/* Name of desired command. */
    const Tcl_CmdInfo *infoPtr)	/* Where to find information to store in the
				 * command. */
{
    Tcl_Command cmd;

    cmd = Tcl_FindCommand(interp, cmdName, NULL, /*flags*/ 0);
    return Tcl_SetCommandInfoFromToken(cmd, infoPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetCommandInfoFromToken --
 *
 *	Modifies various information about a Tcl command. Note that this
 *	function will not change a command's namespace; use TclRenameCommand
 *	to do that. Also, the isNativeObjectProc member of *infoPtr is
 *	ignored.
 *
 * Results:
 *	If cmdName exists in interp, then the information at *infoPtr is
 *	stored with the command in place of the current information and 1 is
 *	returned. If the command doesn't exist then 0 is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetCommandInfoFromToken(
    Tcl_Command cmd,
    const Tcl_CmdInfo *infoPtr)
{
    Command *cmdPtr;		/* Internal representation of the command */

    if (cmd == NULL) {
	return 0;
    }

    /*
     * The isNativeObjectProc and nsPtr members of *infoPtr are ignored.
     */

    cmdPtr = (Command *) cmd;
    cmdPtr->proc = infoPtr->proc;
    cmdPtr->clientData = infoPtr->clientData;
    if (infoPtr->objProc == NULL) {
	cmdPtr->objProc = TclInvokeStringCommand;
	cmdPtr->objClientData = cmdPtr;
	cmdPtr->nreProc = NULL;
    } else {
	if (infoPtr->objProc != cmdPtr->objProc) {
	    cmdPtr->nreProc = NULL;
	    cmdPtr->objProc = infoPtr->objProc;
	}
	cmdPtr->objClientData = infoPtr->objClientData;
    }
    cmdPtr->deleteProc = infoPtr->deleteProc;
    cmdPtr->deleteData = infoPtr->deleteData;
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetCommandInfo --
 *
 *	Returns various information about a Tcl command.
 *
 * Results:
 *	If cmdName exists in interp, then *infoPtr is modified to hold
 *	information about cmdName and 1 is returned. If the command doesn't
 *	exist then 0 is returned and *infoPtr isn't modified.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetCommandInfo(
    Tcl_Interp *interp,		/* Interpreter in which to look for
				 * command. */
    const char *cmdName,	/* Name of desired command. */
    Tcl_CmdInfo *infoPtr)	/* Where to store information about
				 * command. */
{
    Tcl_Command cmd;

    cmd = Tcl_FindCommand(interp, cmdName, NULL, /*flags*/ 0);
    return Tcl_GetCommandInfoFromToken(cmd, infoPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetCommandInfoFromToken --
 *
 *	Returns various information about a Tcl command.
 *
 * Results:
 *	Copies information from the command identified by 'cmd' into a
 *	caller-supplied structure and returns 1. If the 'cmd' is NULL, leaves
 *	the structure untouched and returns 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetCommandInfoFromToken(
    Tcl_Command cmd,
    Tcl_CmdInfo *infoPtr)
{
    Command *cmdPtr;		/* Internal representation of the command */

    if (cmd == NULL) {
	return 0;
    }

    /*
     * Set isNativeObjectProc 1 if objProc was registered by a call to
     * Tcl_CreateObjCommand. Otherwise set it to 0.
     */

    cmdPtr = (Command *) cmd;
    infoPtr->isNativeObjectProc =
	    (cmdPtr->objProc != TclInvokeStringCommand);
    infoPtr->objProc = cmdPtr->objProc;
    infoPtr->objClientData = cmdPtr->objClientData;
    infoPtr->proc = cmdPtr->proc;
    infoPtr->clientData = cmdPtr->clientData;
    infoPtr->deleteProc = cmdPtr->deleteProc;
    infoPtr->deleteData = cmdPtr->deleteData;
    infoPtr->namespacePtr = (Tcl_Namespace *) cmdPtr->nsPtr;

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetCommandName --
 *
 *	Given a token returned by Tcl_CreateCommand, this function returns the
 *	current name of the command (which may have changed due to renaming).
 *
 * Results:
 *	The return value is the name of the given command.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
Tcl_GetCommandName(
    Tcl_Interp *interp,		/* Interpreter containing the command. */
    Tcl_Command command)	/* Token for command returned by a previous
				 * call to Tcl_CreateCommand. The command must
				 * not have been deleted. */
{
    Command *cmdPtr = (Command *) command;

    if ((cmdPtr == NULL) || (cmdPtr->hPtr == NULL)) {
	/*
	 * This should only happen if command was "created" after the
	 * interpreter began to be deleted, so there isn't really any command.
	 * Just return an empty string.
	 */

	return "";
    }

    return Tcl_GetHashKey(cmdPtr->hPtr->tablePtr, cmdPtr->hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetCommandFullName --
 *
 *	Given a token returned by, e.g., Tcl_CreateCommand or Tcl_FindCommand,
 *	this function appends to an object the command's full name, qualified
 *	by a sequence of parent namespace names. The command's fully-qualified
 *	name may have changed due to renaming.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The command's fully-qualified name is appended to the string
 *	representation of objPtr.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_GetCommandFullName(
    Tcl_Interp *interp,		/* Interpreter containing the command. */
    Tcl_Command command,	/* Token for command returned by a previous
				 * call to Tcl_CreateCommand. The command must
				 * not have been deleted. */
    Tcl_Obj *objPtr)		/* Points to the object onto which the
				 * command's full name is appended. */

{
    Interp *iPtr = (Interp *) interp;
    register Command *cmdPtr = (Command *) command;
    char *name;

    /*
     * Add the full name of the containing namespace, followed by the "::"
     * separator, and the command name.
     */

    if (cmdPtr != NULL) {
	if (cmdPtr->nsPtr != NULL) {
	    Tcl_AppendToObj(objPtr, cmdPtr->nsPtr->fullName, -1);
	    if (cmdPtr->nsPtr != iPtr->globalNsPtr) {
		Tcl_AppendToObj(objPtr, "::", 2);
	    }
	}
	if (cmdPtr->hPtr != NULL) {
	    name = Tcl_GetHashKey(cmdPtr->hPtr->tablePtr, cmdPtr->hPtr);
	    Tcl_AppendToObj(objPtr, name, -1);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteCommand --
 *
 *	Remove the given command from the given interpreter.
 *
 * Results:
 *	0 is returned if the command was deleted successfully. -1 is returned
 *	if there didn't exist a command by that name.
 *
 * Side effects:
 *	cmdName will no longer be recognized as a valid command for interp.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_DeleteCommand(
    Tcl_Interp *interp,		/* Token for command interpreter (returned by
				 * a previous Tcl_CreateInterp call). */
    const char *cmdName)	/* Name of command to remove. */
{
    Tcl_Command cmd;

    /*
     * Find the desired command and delete it.
     */

    cmd = Tcl_FindCommand(interp, cmdName, NULL, /*flags*/ 0);
    if (cmd == NULL) {
	return -1;
    }
    return Tcl_DeleteCommandFromToken(interp, cmd);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteCommandFromToken --
 *
 *	Removes the given command from the given interpreter. This function
 *	resembles Tcl_DeleteCommand, but takes a Tcl_Command token instead of
 *	a command name for efficiency.
 *
 * Results:
 *	0 is returned if the command was deleted successfully. -1 is returned
 *	if there didn't exist a command by that name.
 *
 * Side effects:
 *	The command specified by "cmd" will no longer be recognized as a valid
 *	command for "interp".
 *
 *----------------------------------------------------------------------
 */

int
Tcl_DeleteCommandFromToken(
    Tcl_Interp *interp,		/* Token for command interpreter returned by a
				 * previous call to Tcl_CreateInterp. */
    Tcl_Command cmd)		/* Token for command to delete. */
{
    Interp *iPtr = (Interp *) interp;
    Command *cmdPtr = (Command *) cmd;
    ImportRef *refPtr, *nextRefPtr;
    Tcl_Command importCmd;

    /*
     * Bump the command epoch counter. This will invalidate all cached
     * references that point to this command.
     */

    cmdPtr->cmdEpoch++;

    /*
     * The code here is tricky. We can't delete the hash table entry before
     * invoking the deletion callback because there are cases where the
     * deletion callback needs to invoke the command (e.g. object systems such
     * as OTcl). However, this means that the callback could try to delete or
     * rename the command. The deleted flag allows us to detect these cases
     * and skip nested deletes.
     */

    if (cmdPtr->flags & CMD_IS_DELETED) {
	/*
	 * Another deletion is already in progress. Remove the hash table
	 * entry now, but don't invoke a callback or free the command
	 * structure. Take care to only remove the hash entry if it has not
	 * already been removed; otherwise if we manage to hit this function
	 * three times, everything goes up in smoke. [Bug 1220058]
	 */

	if (cmdPtr->hPtr != NULL) {
	    Tcl_DeleteHashEntry(cmdPtr->hPtr);
	    cmdPtr->hPtr = NULL;
	}
	return 0;
    }

    /*
     * We must delete this command, even though both traces and delete procs
     * may try to avoid this (renaming the command etc). Also traces and
     * delete procs may try to delete the command themselves. This flag
     * declares that a delete is in progress and that recursive deletes should
     * be ignored.
     */

    cmdPtr->flags |= CMD_IS_DELETED;

    /*
     * Call trace functions for the command being deleted. Then delete its
     * traces.
     */

    cmdPtr->nsPtr->refCount++;

    if (cmdPtr->tracePtr != NULL) {
	CommandTrace *tracePtr;
	CallCommandTraces(iPtr,cmdPtr,NULL,NULL,TCL_TRACE_DELETE);

	/*
	 * Now delete these traces.
	 */

	tracePtr = cmdPtr->tracePtr;
	while (tracePtr != NULL) {
	    CommandTrace *nextPtr = tracePtr->nextPtr;

	    if (tracePtr->refCount-- <= 1) {
		ckfree(tracePtr);
	    }
	    tracePtr = nextPtr;
	}
	cmdPtr->tracePtr = NULL;
    }

    /*
     * The list of command exported from the namespace might have changed.
     * However, we do not need to recompute this just yet; next time we need
     * the info will be soon enough.
     */

    TclInvalidateNsCmdLookup(cmdPtr->nsPtr);
    TclNsDecrRefCount(cmdPtr->nsPtr);

    /*
     * If the command being deleted has a compile function, increment the
     * interpreter's compileEpoch to invalidate its compiled code. This makes
     * sure that we don't later try to execute old code compiled with
     * command-specific (i.e., inline) bytecodes for the now-deleted command.
     * This field is checked in Tcl_EvalObj and ObjInterpProc, and code whose
     * compilation epoch doesn't match is recompiled.
     */

    if (cmdPtr->compileProc != NULL) {
	iPtr->compileEpoch++;
    }

    if (cmdPtr->deleteProc != NULL) {
	/*
	 * Delete the command's client data. If this was an imported command
	 * created when a command was imported into a namespace, this client
	 * data will be a pointer to a ImportedCmdData structure describing
	 * the "real" command that this imported command refers to.
	 *
	 * If you are getting a crash during the call to deleteProc and
	 * cmdPtr->deleteProc is a pointer to the function free(), the most
	 * likely cause is that your extension allocated memory for the
	 * clientData argument to Tcl_CreateObjCommand with the ckalloc()
	 * macro and you are now trying to deallocate this memory with free()
	 * instead of ckfree(). You should pass a pointer to your own method
	 * that calls ckfree().
	 */

	cmdPtr->deleteProc(cmdPtr->deleteData);
    }

    /*
     * If this command was imported into other namespaces, then imported
     * commands were created that refer back to this command. Delete these
     * imported commands now.
     */
    if (!(cmdPtr->flags & CMD_REDEF_IN_PROGRESS)) {
	for (refPtr = cmdPtr->importRefPtr; refPtr != NULL;
		refPtr = nextRefPtr) {
	    nextRefPtr = refPtr->nextPtr;
	    importCmd = (Tcl_Command) refPtr->importedCmdPtr;
	    Tcl_DeleteCommandFromToken(interp, importCmd);
	}
    }

    /*
     * Don't use hPtr to delete the hash entry here, because it's possible
     * that the deletion callback renamed the command. Instead, use
     * cmdPtr->hptr, and make sure that no-one else has already deleted the
     * hash entry.
     */

    if (cmdPtr->hPtr != NULL) {
	Tcl_DeleteHashEntry(cmdPtr->hPtr);
	cmdPtr->hPtr = NULL;
    }

    /*
     * A number of tests for particular kinds of commands are done by checking
     * whether the objProc field holds a known value. Set the field to NULL so
     * that such tests won't have false positives when applied to deleted
     * commands.
     */

    cmdPtr->objProc = NULL;

    /*
     * Now free the Command structure, unless there is another reference to it
     * from a CmdName Tcl object in some ByteCode code sequence. In that case,
     * delay the cleanup until all references are either discarded (when a
     * ByteCode is freed) or replaced by a new reference (when a cached
     * CmdName Command reference is found to be invalid and
     * TclNRExecuteByteCode looks up the command in the command hashtable).
     */

    TclCleanupCommandMacro(cmdPtr);
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * CallCommandTraces --
 *
 *	Abstraction of the code to call traces on a command.
 *
 * Results:
 *	Currently always NULL.
 *
 * Side effects:
 *	Anything; this may recursively evaluate scripts and code exists to do
 *	just that.
 *
 *----------------------------------------------------------------------
 */

static char *
CallCommandTraces(
    Interp *iPtr,		/* Interpreter containing command. */
    Command *cmdPtr,		/* Command whose traces are to be invoked. */
    const char *oldName,	/* Command's old name, or NULL if we must get
				 * the name from cmdPtr */
    const char *newName,	/* Command's new name, or NULL if the command
				 * is not being renamed */
    int flags)			/* Flags indicating the type of traces to
				 * trigger, either TCL_TRACE_DELETE or
				 * TCL_TRACE_RENAME. */
{
    register CommandTrace *tracePtr;
    ActiveCommandTrace active;
    char *result;
    Tcl_Obj *oldNamePtr = NULL;
    Tcl_InterpState state = NULL;

    if (cmdPtr->flags & CMD_TRACE_ACTIVE) {
	/*
	 * While a rename trace is active, we will not process any more rename
	 * traces; while a delete trace is active we will never reach here -
	 * because Tcl_DeleteCommandFromToken checks for the condition
	 * (cmdPtr->flags & CMD_IS_DELETED) and returns immediately when a
	 * command deletion is in progress. For all other traces, delete
	 * traces will not be invoked but a call to TraceCommandProc will
	 * ensure that tracePtr->clientData is freed whenever the command
	 * "oldName" is deleted.
	 */

	if (cmdPtr->flags & TCL_TRACE_RENAME) {
	    flags &= ~TCL_TRACE_RENAME;
	}
	if (flags == 0) {
	    return NULL;
	}
    }
    cmdPtr->flags |= CMD_TRACE_ACTIVE;
    cmdPtr->refCount++;

    result = NULL;
    active.nextPtr = iPtr->activeCmdTracePtr;
    active.reverseScan = 0;
    iPtr->activeCmdTracePtr = &active;

    if (flags & TCL_TRACE_DELETE) {
	flags |= TCL_TRACE_DESTROYED;
    }
    active.cmdPtr = cmdPtr;

    Tcl_Preserve(iPtr);

    for (tracePtr = cmdPtr->tracePtr; tracePtr != NULL;
	    tracePtr = active.nextTracePtr) {
	active.nextTracePtr = tracePtr->nextPtr;
	if (!(tracePtr->flags & flags)) {
	    continue;
	}
	cmdPtr->flags |= tracePtr->flags;
	if (oldName == NULL) {
	    TclNewObj(oldNamePtr);
	    Tcl_IncrRefCount(oldNamePtr);
	    Tcl_GetCommandFullName((Tcl_Interp *) iPtr,
		    (Tcl_Command) cmdPtr, oldNamePtr);
	    oldName = TclGetString(oldNamePtr);
	}
	tracePtr->refCount++;
	if (state == NULL) {
	    state = Tcl_SaveInterpState((Tcl_Interp *) iPtr, TCL_OK);
	}
	tracePtr->traceProc(tracePtr->clientData, (Tcl_Interp *) iPtr,
		oldName, newName, flags);
	cmdPtr->flags &= ~tracePtr->flags;
	if (tracePtr->refCount-- <= 1) {
	    ckfree(tracePtr);
	}
    }

    if (state) {
	Tcl_RestoreInterpState((Tcl_Interp *) iPtr, state);
    }

    /*
     * If a new object was created to hold the full oldName, free it now.
     */

    if (oldNamePtr != NULL) {
	TclDecrRefCount(oldNamePtr);
    }

    /*
     * Restore the variable's flags, remove the record of our active traces,
     * and then return.
     */

    cmdPtr->flags &= ~CMD_TRACE_ACTIVE;
    cmdPtr->refCount--;
    iPtr->activeCmdTracePtr = active.nextPtr;
    Tcl_Release(iPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * CancelEvalProc --
 *
 *	Marks this interpreter as being canceled. This causes current
 *	executions to be unwound as the interpreter enters a state where it
 *	refuses to execute more commands or handle [catch] or [try], yet the
 *	interpreter is still able to execute further commands after the
 *	cancelation is cleared (unlike if it is deleted).
 *
 * Results:
 *	The value given for the code argument.
 *
 * Side effects:
 *	Transfers a message from the cancelation message to the interpreter.
 *
 *----------------------------------------------------------------------
 */

static int
CancelEvalProc(
    ClientData clientData,	/* Interp to cancel the script in progress. */
    Tcl_Interp *interp,		/* Ignored */
    int code)			/* Current return code from command. */
{
    CancelInfo *cancelInfo = clientData;
    Interp *iPtr;

    if (cancelInfo != NULL) {
	Tcl_MutexLock(&cancelLock);
	iPtr = (Interp *) cancelInfo->interp;

	if (iPtr != NULL) {
	    /*
	     * Setting the CANCELED flag will cause the script in progress to
	     * be canceled as soon as possible. The core honors this flag at
	     * all the necessary places to ensure script cancellation is
	     * responsive. Extensions can check for this flag by calling
	     * Tcl_Canceled and checking if TCL_ERROR is returned or they can
	     * choose to ignore the script cancellation flag and the
	     * associated functionality altogether. Currently, the only other
	     * flag we care about here is the TCL_CANCEL_UNWIND flag (from
	     * Tcl_CancelEval). We do not want to simply combine all the flags
	     * from original Tcl_CancelEval call with the interp flags here
	     * just in case the caller passed flags that might cause behaviour
	     * unrelated to script cancellation.
	     */

	    TclSetCancelFlags(iPtr, cancelInfo->flags | CANCELED);

	    /*
	     * Now, we must set the script cancellation flags on all the slave
	     * interpreters belonging to this one.
	     */

	    TclSetSlaveCancelFlags((Tcl_Interp *) iPtr,
		    cancelInfo->flags | CANCELED, 0);

	    /*
	     * Create the result object now so that Tcl_Canceled can avoid
	     * locking the cancelLock mutex.
	     */

	    if (cancelInfo->result != NULL) {
		Tcl_SetStringObj(iPtr->asyncCancelMsg, cancelInfo->result,
			cancelInfo->length);
	    } else {
		Tcl_SetObjLength(iPtr->asyncCancelMsg, 0);
	    }
	}
	Tcl_MutexUnlock(&cancelLock);
    }

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCleanupCommand --
 *
 *	This function frees up a Command structure unless it is still
 *	referenced from an interpreter's command hashtable or from a CmdName
 *	Tcl object representing the name of a command in a ByteCode
 *	instruction sequence.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets freed unless a reference to the Command structure still
 *	exists. In that case the cleanup is delayed until the command is
 *	deleted or when the last ByteCode referring to it is freed.
 *
 *----------------------------------------------------------------------
 */

void
TclCleanupCommand(
    register Command *cmdPtr)	/* Points to the Command structure to
				 * be freed. */
{
    cmdPtr->refCount--;
    if (cmdPtr->refCount <= 0) {
	ckfree(cmdPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateMathFunc --
 *
 *	Creates a new math function for expressions in a given interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Tcl function defined by "name" is created or redefined. If the
 *	function already exists then its definition is replaced; this includes
 *	the builtin functions. Redefining a builtin function forces all
 *	existing code to be invalidated since that code may be compiled using
 *	an instruction specific to the replaced function. In addition,
 *	redefioning a non-builtin function will force existing code to be
 *	invalidated if the number of arguments has changed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateMathFunc(
    Tcl_Interp *interp,		/* Interpreter in which function is to be
				 * available. */
    const char *name,		/* Name of function (e.g. "sin"). */
    int numArgs,		/* Nnumber of arguments required by
				 * function. */
    Tcl_ValueType *argTypes,	/* Array of types acceptable for each
				 * argument. */
    Tcl_MathProc *proc,		/* C function that implements the math
				 * function. */
    ClientData clientData)	/* Additional value to pass to the
				 * function. */
{
    Tcl_DString bigName;
    OldMathFuncData *data = ckalloc(sizeof(OldMathFuncData));

    data->proc = proc;
    data->numArgs = numArgs;
    data->argTypes = ckalloc(numArgs * sizeof(Tcl_ValueType));
    memcpy(data->argTypes, argTypes, numArgs * sizeof(Tcl_ValueType));
    data->clientData = clientData;

    Tcl_DStringInit(&bigName);
    TclDStringAppendLiteral(&bigName, "::tcl::mathfunc::");
    Tcl_DStringAppend(&bigName, name, -1);

    Tcl_CreateObjCommand(interp, Tcl_DStringValue(&bigName),
	    OldMathFuncProc, data, OldMathFuncDeleteProc);
    Tcl_DStringFree(&bigName);
}

/*
 *----------------------------------------------------------------------
 *
 * OldMathFuncProc --
 *
 *	Dispatch to a math function created with Tcl_CreateMathFunc
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Whatever the math function does.
 *
 *----------------------------------------------------------------------
 */

static int
OldMathFuncProc(
    ClientData clientData,	/* Ponter to OldMathFuncData describing the
				 * function being called */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int objc,			/* Actual parameter count */
    Tcl_Obj *const *objv)	/* Parameter vector */
{
    Tcl_Obj *valuePtr;
    OldMathFuncData *dataPtr = clientData;
    Tcl_Value funcResult, *args;
    int result;
    int j, k;
    double d;

    /*
     * Check argument count.
     */

    if (objc != dataPtr->numArgs + 1) {
	MathFuncWrongNumArgs(interp, dataPtr->numArgs+1, objc, objv);
	return TCL_ERROR;
    }

    /*
     * Convert arguments from Tcl_Obj's to Tcl_Value's.
     */

    args = ckalloc(dataPtr->numArgs * sizeof(Tcl_Value));
    for (j = 1, k = 0; j < objc; ++j, ++k) {
	/* TODO: Convert to TclGetNumberFromObj? */
	valuePtr = objv[j];
	result = Tcl_GetDoubleFromObj(NULL, valuePtr, &d);
#ifdef ACCEPT_NAN
	if ((result != TCL_OK) && (valuePtr->typePtr == &tclDoubleType)) {
	    d = valuePtr->internalRep.doubleValue;
	    result = TCL_OK;
	}
#endif
	if (result != TCL_OK) {
	    /*
	     * We have a non-numeric argument.
	     */

	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "argument to math function didn't have numeric value",
		    -1));
	    TclCheckBadOctal(interp, Tcl_GetString(valuePtr));
	    ckfree(args);
	    return TCL_ERROR;
	}

	/*
	 * Copy the object's numeric value to the argument record, converting
	 * it if necessary.
	 *
	 * NOTE: no bignum support; use the new mathfunc interface for that.
	 */

	args[k].type = dataPtr->argTypes[k];
	switch (args[k].type) {
	case TCL_EITHER:
	    if (Tcl_GetLongFromObj(NULL, valuePtr, &args[k].intValue)
		    == TCL_OK) {
		args[k].type = TCL_INT;
		break;
	    }
	    if (TclGetWideIntFromObj(interp, valuePtr, &args[k].wideValue)
		    == TCL_OK) {
		args[k].type = TCL_WIDE_INT;
		break;
	    }
	    args[k].type = TCL_DOUBLE;
	    /* FALLTHROUGH */

	case TCL_DOUBLE:
	    args[k].doubleValue = d;
	    break;
	case TCL_INT:
	    if (ExprIntFunc(NULL, interp, 2, &objv[j-1]) != TCL_OK) {
		ckfree(args);
		return TCL_ERROR;
	    }
	    valuePtr = Tcl_GetObjResult(interp);
	    Tcl_GetLongFromObj(NULL, valuePtr, &args[k].intValue);
	    Tcl_ResetResult(interp);
	    break;
	case TCL_WIDE_INT:
	    if (ExprWideFunc(NULL, interp, 2, &objv[j-1]) != TCL_OK) {
		ckfree(args);
		return TCL_ERROR;
	    }
	    valuePtr = Tcl_GetObjResult(interp);
	    TclGetWideIntFromObj(NULL, valuePtr, &args[k].wideValue);
	    Tcl_ResetResult(interp);
	    break;
	}
    }

    /*
     * Call the function.
     */

    errno = 0;
    result = dataPtr->proc(dataPtr->clientData, interp, args, &funcResult);
    ckfree(args);
    if (result != TCL_OK) {
	return result;
    }

    /*
     * Return the result of the call.
     */

    if (funcResult.type == TCL_INT) {
	TclNewLongObj(valuePtr, funcResult.intValue);
    } else if (funcResult.type == TCL_WIDE_INT) {
	valuePtr = Tcl_NewWideIntObj(funcResult.wideValue);
    } else {
	return CheckDoubleResult(interp, funcResult.doubleValue);
    }
    Tcl_SetObjResult(interp, valuePtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * OldMathFuncDeleteProc --
 *
 *	Cleans up after deleting a math function registered with
 *	Tcl_CreateMathFunc
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees allocated memory.
 *
 *----------------------------------------------------------------------
 */

static void
OldMathFuncDeleteProc(
    ClientData clientData)
{
    OldMathFuncData *dataPtr = clientData;

    ckfree(dataPtr->argTypes);
    ckfree(dataPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetMathFuncInfo --
 *
 *	Discovers how a particular math function was created in a given
 *	interpreter.
 *
 * Results:
 *	TCL_OK if it succeeds, TCL_ERROR else (leaving an error message in the
 *	interpreter result if that happens.)
 *
 * Side effects:
 *	If this function succeeds, the variables pointed to by the numArgsPtr
 *	and argTypePtr arguments will be updated to detail the arguments
 *	allowed by the function. The variable pointed to by the procPtr
 *	argument will be set to NULL if the function is a builtin function,
 *	and will be set to the address of the C function used to implement the
 *	math function otherwise (in which case the variable pointed to by the
 *	clientDataPtr argument will also be updated.)
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetMathFuncInfo(
    Tcl_Interp *interp,
    const char *name,
    int *numArgsPtr,
    Tcl_ValueType **argTypesPtr,
    Tcl_MathProc **procPtr,
    ClientData *clientDataPtr)
{
    Tcl_Obj *cmdNameObj;
    Command *cmdPtr;

    /*
     * Get the command that implements the math function.
     */

    TclNewLiteralStringObj(cmdNameObj, "tcl::mathfunc::");
    Tcl_AppendToObj(cmdNameObj, name, -1);
    Tcl_IncrRefCount(cmdNameObj);
    cmdPtr = (Command *) Tcl_GetCommandFromObj(interp, cmdNameObj);
    Tcl_DecrRefCount(cmdNameObj);

    /*
     * Report unknown functions.
     */

    if (cmdPtr == NULL) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "unknown math function \"%s\"", name));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "MATHFUNC", name, NULL);
	*numArgsPtr = -1;
	*argTypesPtr = NULL;
	*procPtr = NULL;
	*clientDataPtr = NULL;
	return TCL_ERROR;
    }

    /*
     * Retrieve function info for user defined functions; return dummy
     * information for builtins.
     */

    if (cmdPtr->objProc == &OldMathFuncProc) {
	OldMathFuncData *dataPtr = cmdPtr->clientData;

	*procPtr = dataPtr->proc;
	*numArgsPtr = dataPtr->numArgs;
	*argTypesPtr = dataPtr->argTypes;
	*clientDataPtr = dataPtr->clientData;
    } else {
	*procPtr = NULL;
	*numArgsPtr = -1;
	*argTypesPtr = NULL;
	*procPtr = NULL;
	*clientDataPtr = NULL;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ListMathFuncs --
 *
 *	Produces a list of all the math functions defined in a given
 *	interpreter.
 *
 * Results:
 *	A pointer to a Tcl_Obj structure with a reference count of zero, or
 *	NULL in the case of an error (in which case a suitable error message
 *	will be left in the interpreter result.)
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_ListMathFuncs(
    Tcl_Interp *interp,
    const char *pattern)
{
    Tcl_Obj *script = Tcl_NewStringObj("::info functions ", -1);
    Tcl_Obj *result;
    Tcl_InterpState state;

    if (pattern) {
	Tcl_Obj *patternObj = Tcl_NewStringObj(pattern, -1);
	Tcl_Obj *arg = Tcl_NewListObj(1, &patternObj);

	Tcl_AppendObjToObj(script, arg);
	Tcl_DecrRefCount(arg);	/* Should tear down patternObj too */
    }

    state = Tcl_SaveInterpState(interp, TCL_OK);
    Tcl_IncrRefCount(script);
    if (TCL_OK == Tcl_EvalObjEx(interp, script, 0)) {
	result = Tcl_DuplicateObj(Tcl_GetObjResult(interp));
    } else {
	result = Tcl_NewObj();
    }
    Tcl_DecrRefCount(script);
    Tcl_RestoreInterpState(interp, state);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInterpReady --
 *
 *	Check if an interpreter is ready to eval commands or scripts, i.e., if
 *	it was not deleted and if the nesting level is not too high.
 *
 * Results:
 *	The return value is TCL_OK if it the interpreter is ready, TCL_ERROR
 *	otherwise.
 *
 * Side effects:
 *	The interpreters object and string results are cleared.
 *
 *----------------------------------------------------------------------
 */

int
TclInterpReady(
    Tcl_Interp *interp)
{
    register Interp *iPtr = (Interp *) interp;

    /*
     * Reset both the interpreter's string and object results and clear out
     * any previous error information.
     */

    Tcl_ResetResult(interp);

    /*
     * If the interpreter has been deleted, return an error.
     */

    if (iPtr->flags & DELETED) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"attempt to call eval in deleted interpreter", -1));
	Tcl_SetErrorCode(interp, "TCL", "IDELETE",
		"attempt to call eval in deleted interpreter", NULL);
	return TCL_ERROR;
    }

    if (iPtr->execEnvPtr->rewind) {
	return TCL_ERROR;
    }

    /*
     * Make sure the script being evaluated (if any) has not been canceled.
     */

    if (TclCanceled(iPtr) &&
	    (TCL_OK != Tcl_Canceled(interp, TCL_LEAVE_ERR_MSG))) {
	return TCL_ERROR;
    }

    /*
     * Check depth of nested calls to Tcl_Eval: if this gets too large, it's
     * probably because of an infinite loop somewhere.
     */

    if (((iPtr->numLevels) <= iPtr->maxNestingDepth)) {
	return TCL_OK;
    }

    Tcl_SetObjResult(interp, Tcl_NewStringObj(
	    "too many nested evaluations (infinite loop?)", -1));
    Tcl_SetErrorCode(interp, "TCL", "LIMIT", "STACK", NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclResetCancellation --
 *
 *	Reset the script cancellation flags if the nesting level
 *	(iPtr->numLevels) for the interp is zero or argument force is
 *	non-zero.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	The script cancellation flags for the interp may be reset.
 *
 *----------------------------------------------------------------------
 */

int
TclResetCancellation(
    Tcl_Interp *interp,
    int force)
{
    register Interp *iPtr = (Interp *) interp;

    if (iPtr == NULL) {
	return TCL_ERROR;
    }

    if (force || (iPtr->numLevels == 0)) {
	TclUnsetCancelFlags(iPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Canceled --
 *
 *	Check if the script in progress has been canceled, i.e.,
 *	Tcl_CancelEval was called for this interpreter or any of its master
 *	interpreters.
 *
 * Results:
 *	The return value is TCL_OK if the script evaluation has not been
 *	canceled, TCL_ERROR otherwise.
 *
 *	If "flags" contains TCL_LEAVE_ERR_MSG, an error message is returned in
 *	the interpreter's result object. Otherwise, the interpreter's result
 *	object is left unchanged. If "flags" contains TCL_CANCEL_UNWIND,
 *	TCL_ERROR will only be returned if the script evaluation is being
 *	completely unwound.
 *
 * Side effects:
 *	The CANCELED flag for the interp will be reset if it is set.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Canceled(
    Tcl_Interp *interp,
    int flags)
{
    register Interp *iPtr = (Interp *) interp;

    /*
     * Has the current script in progress for this interpreter been canceled
     * or is the stack being unwound due to the previous script cancellation?
     */

    if (!TclCanceled(iPtr)) {
        return TCL_OK;
    }

    /*
     * The CANCELED flag is a one-shot flag that is reset immediately upon
     * being detected; however, if the TCL_CANCEL_UNWIND flag is set we will
     * continue to report that the script in progress has been canceled
     * thereby allowing the evaluation stack for the interp to be fully
     * unwound.
     */

    iPtr->flags &= ~CANCELED;

    /*
     * The CANCELED flag was detected and reset; however, if the caller
     * specified the TCL_CANCEL_UNWIND flag, we only return TCL_ERROR
     * (indicating that the script in progress has been canceled) if the
     * evaluation stack for the interp is being fully unwound.
     */

    if ((flags & TCL_CANCEL_UNWIND) && !(iPtr->flags & TCL_CANCEL_UNWIND)) {
        return TCL_OK;
    }

    /*
     * If the TCL_LEAVE_ERR_MSG flags bit is set, place an error in the
     * interp's result; otherwise, we leave it alone.
     */

    if (flags & TCL_LEAVE_ERR_MSG) {
        const char *id, *message = NULL;
        int length;

        /*
         * Setup errorCode variables so that we can differentiate between
         * being canceled and unwound.
         */

        if (iPtr->asyncCancelMsg != NULL) {
            message = Tcl_GetStringFromObj(iPtr->asyncCancelMsg, &length);
        } else {
            length = 0;
        }

        if (iPtr->flags & TCL_CANCEL_UNWIND) {
            id = "IUNWIND";
            if (length == 0) {
                message = "eval unwound";
            }
        } else {
            id = "ICANCEL";
            if (length == 0) {
                message = "eval canceled";
            }
        }

        Tcl_SetObjResult(interp, Tcl_NewStringObj(message, -1));
        Tcl_SetErrorCode(interp, "TCL", "CANCEL", id, message, NULL);
    }

    /*
     * Return TCL_ERROR to the caller (not necessarily just the Tcl core
     * itself) that indicates further processing of the script or command in
     * progress should halt gracefully and as soon as possible.
     */

    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CancelEval --
 *
 *	This function schedules the cancellation of the current script in the
 *	given interpreter.
 *
 * Results:
 *	The return value is a standard Tcl completion code such as TCL_OK or
 *	TCL_ERROR. Since the interp may belong to a different thread, no error
 *	message can be left in the interp's result.
 *
 * Side effects:
 *	The script in progress in the specified interpreter will be canceled
 *	with TCL_ERROR after asynchronous handlers are invoked at the next
 *	Tcl_Canceled check.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_CancelEval(
    Tcl_Interp *interp,		/* Interpreter in which to cancel the
				 * script. */
    Tcl_Obj *resultObjPtr,	/* The script cancellation error message or
				 * NULL for a default error message. */
    ClientData clientData,	/* Passed to CancelEvalProc. */
    int flags)			/* Collection of OR-ed bits that control
				 * the cancellation of the script. Only
				 * TCL_CANCEL_UNWIND is currently
				 * supported. */
{
    Tcl_HashEntry *hPtr;
    CancelInfo *cancelInfo;
    int code = TCL_ERROR;
    const char *result;

    if (interp == NULL) {
	return TCL_ERROR;
    }

    Tcl_MutexLock(&cancelLock);
    if (cancelTableInitialized != 1) {
	/*
	 * No CancelInfo hash table (Tcl_CreateInterp has never been called?)
	 */

	goto done;
    }
    hPtr = Tcl_FindHashEntry(&cancelTable, (char *) interp);
    if (hPtr == NULL) {
	/*
	 * No CancelInfo record for this interpreter.
	 */

	goto done;
    }
    cancelInfo = Tcl_GetHashValue(hPtr);

    /*
     * Populate information needed by the interpreter thread to fulfill the
     * cancellation request. Currently, clientData is ignored. If the
     * TCL_CANCEL_UNWIND flags bit is set, the script in progress is not
     * allowed to catch the script cancellation because the evaluation stack
     * for the interp is completely unwound.
     */

    if (resultObjPtr != NULL) {
	result = Tcl_GetStringFromObj(resultObjPtr, &cancelInfo->length);
	cancelInfo->result = ckrealloc(cancelInfo->result,cancelInfo->length);
	memcpy(cancelInfo->result, result, (size_t) cancelInfo->length);
	TclDecrRefCount(resultObjPtr);	/* Discard their result object. */
    } else {
	cancelInfo->result = NULL;
	cancelInfo->length = 0;
    }
    cancelInfo->clientData = clientData;
    cancelInfo->flags = flags;
    Tcl_AsyncMark(cancelInfo->async);
    code = TCL_OK;

  done:
    Tcl_MutexUnlock(&cancelLock);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InterpActive --
 *
 *	Returns non-zero if the specified interpreter is in use, i.e. if there
 *	is an evaluation currently active in the interpreter.
 *
 * Results:
 *	See above.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_InterpActive(
    Tcl_Interp *interp)
{
    return ((Interp *) interp)->numLevels > 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_EvalObjv --
 *
 *	This function evaluates a Tcl command that has already been parsed
 *	into words, with one Tcl_Obj holding each word.
 *
 * Results:
 *	The return value is a standard Tcl completion code such as TCL_OK or
 *	TCL_ERROR. A result or error message is left in interp's result.
 *
 * Side effects:
 *	Always pushes a callback. Other side effects depend on the command.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_EvalObjv(
    Tcl_Interp *interp,		/* Interpreter in which to evaluate the
				 * command. Also used for error reporting. */
    int objc,			/* Number of words in command. */
    Tcl_Obj *const objv[],	/* An array of pointers to objects that are
				 * the words that make up the command. */
    int flags)			/* Collection of OR-ed bits that control the
				 * evaluation of the script. Only
				 * TCL_EVAL_GLOBAL, TCL_EVAL_INVOKE and
				 * TCL_EVAL_NOERR are currently supported. */
{
    int result;
    NRE_callback *rootPtr = TOP_CB(interp);

    result = TclNREvalObjv(interp, objc, objv, flags, NULL);
    return TclNRRunCallbacks(interp, result, rootPtr);
}

int
TclNREvalObjv(
    Tcl_Interp *interp,		/* Interpreter in which to evaluate the
				 * command. Also used for error reporting. */
    int objc,			/* Number of words in command. */
    Tcl_Obj *const objv[],	/* An array of pointers to objects that are
				 * the words that make up the command. */
    int flags,			/* Collection of OR-ed bits that control the
				 * evaluation of the script. Only
				 * TCL_EVAL_GLOBAL, TCL_EVAL_INVOKE and
				 * TCL_EVAL_NOERR are currently supported. */
    Command *cmdPtr)		/* NULL if the Command is to be looked up
				 * here, otherwise the pointer to the
				 * requested Command struct to be invoked. */
{
    Interp *iPtr = (Interp *) interp;

    /*
     * data[1] stores a marker for use by tailcalls; it will be set to 1 by
     * command redirectors (imports, alias, ensembles) so that tailcall skips
     * this callback (that marks the end of the target command) and goes back
     * to the end of the source command.
     */

    if (iPtr->deferredCallbacks) {
        iPtr->deferredCallbacks = NULL;
    } else {
	TclNRAddCallback(interp, NRCommand, NULL, NULL, NULL, NULL);
    }

    iPtr->numLevels++;
    TclNRAddCallback(interp, EvalObjvCore, cmdPtr, INT2PTR(flags),
	    INT2PTR(objc), objv);
    return TCL_OK;
}

static int
EvalObjvCore(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Command *cmdPtr = NULL, *preCmdPtr = data[0];
    int flags = PTR2INT(data[1]);
    int objc = PTR2INT(data[2]);
    Tcl_Obj **objv = data[3];
    Interp *iPtr = (Interp *) interp;
    Namespace *lookupNsPtr = NULL;
    int enterTracesDone = 0;

    /*
     * Push records for task to be done on return, in INVERSE order. First, if
     * needed, the exception handlers (as they should happen last).
     */

    if (!(flags & TCL_EVAL_NOERR)) {
	TEOV_PushExceptionHandlers(interp, objc, objv, flags);
    }

    if (TCL_OK != TclInterpReady(interp)) {
	return TCL_ERROR;
    }

    if (objc == 0) {
	return TCL_OK;
    }

    if (TclLimitExceeded(iPtr->limit)) {
	return TCL_ERROR;
    }

    /*
     * Configure evaluation context to match the requested flags.
     */

    if (iPtr->lookupNsPtr) {

	/*
	 * Capture the namespace we should do command name resolution in, as
	 * instructed by our caller sneaking it in to us in a private interp
	 * field.  Clear that field right away so we cannot possibly have its
	 * use leak where it should not.  The sneaky message pass is done.
	 *
	 * Use of this mechanism overrides the TCL_EVAL_GLOBAL flag.
	 * TODO: Is that a bug?
	 */

	lookupNsPtr = iPtr->lookupNsPtr;
	iPtr->lookupNsPtr = NULL;
    } else if (flags & TCL_EVAL_INVOKE) {
	lookupNsPtr = iPtr->globalNsPtr;
    } else {

	/*
	 * TCL_EVAL_INVOKE was not set: clear rewrite rules
	 */

	TclResetRewriteEnsemble(interp, 1);

	if (flags & TCL_EVAL_GLOBAL) {
	    TEOV_SwitchVarFrame(interp);
	    lookupNsPtr = iPtr->globalNsPtr;
	}
    }

    /*
     * Lookup the Command to dispatch.
     */

    reresolve:
    assert(cmdPtr == NULL);
    if (preCmdPtr) {
	/* Caller gave it to us */
	if (!(preCmdPtr->flags & CMD_IS_DELETED)) {
	    /* So long as it exists, use it. */
	    cmdPtr = preCmdPtr;
	} else if (flags & TCL_EVAL_NORESOLVE) {
	    /*
	     * When it's been deleted, and we're told not to attempt
	     * resolving it ourselves, all we can do is raise an error.
	     */
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "attempt to invoke a deleted command"));
	    Tcl_SetErrorCode(interp, "TCL", "EVAL", "DELETEDCOMMAND", NULL);
	    return TCL_ERROR;
	}
    }
    if (cmdPtr == NULL) {
	cmdPtr = TEOV_LookupCmdFromObj(interp, objv[0], lookupNsPtr);
	if (!cmdPtr) {
	    return TEOV_NotFound(interp, objc, objv, lookupNsPtr);
	}
    }

    if (enterTracesDone || iPtr->tracePtr
	    || (cmdPtr->flags & CMD_HAS_EXEC_TRACES)) {

	Tcl_Obj *commandPtr = TclGetSourceFromFrame(
		flags & TCL_EVAL_SOURCE_IN_FRAME ?  iPtr->cmdFramePtr : NULL,
		objc, objv);
	Tcl_IncrRefCount(commandPtr);

	if (!enterTracesDone) {

	    int code = TEOV_RunEnterTraces(interp, &cmdPtr, commandPtr,
		    objc, objv);

	    /*
	     * Send any exception from enter traces back as an exception
	     * raised by the traced command.
	     * TODO: Is this a bug?  Letting an execution trace BREAK or
	     * CONTINUE or RETURN in the place of the traced command?
	     * Would either converting all exceptions to TCL_ERROR, or
	     * just swallowing them be better?  (Swallowing them has the
	     * problem of permanently hiding program errors.)
	     */

	    if (code != TCL_OK) {
		Tcl_DecrRefCount(commandPtr);
		return code;
	    }

	    /*
	     * If the enter traces made the resolved cmdPtr unusable, go
	     * back and resolve again, but next time don't run enter
	     * traces again.
	     */

	    if (cmdPtr == NULL) {
		enterTracesDone = 1;
		Tcl_DecrRefCount(commandPtr);
		goto reresolve;
	    }
	}

	/*
	 * Schedule leave traces.  Raise the refCount on the resolved
	 * cmdPtr, so that when it passes to the leave traces we know
	 * it's still valid.
	 */

	cmdPtr->refCount++;
	TclNRAddCallback(interp, TEOV_RunLeaveTraces, INT2PTR(objc),
		    commandPtr, cmdPtr, objv);
    }

    TclNRAddCallback(interp, Dispatch,
	    cmdPtr->nreProc ? cmdPtr->nreProc : cmdPtr->objProc,
	    cmdPtr->objClientData, INT2PTR(objc), objv);
    return TCL_OK;
}

static int
Dispatch(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_ObjCmdProc *objProc = data[0];
    ClientData clientData = data[1];
    int objc = PTR2INT(data[2]);
    Tcl_Obj **objv = data[3];
    Interp *iPtr = (Interp *) interp;

#ifdef USE_DTRACE
    if (TCL_DTRACE_CMD_ARGS_ENABLED()) {
	const char *a[10];
	int i = 0;

	while (i < 10) {
	    a[i] = i < objc ? TclGetString(objv[i]) : NULL; i++;
	}
	TCL_DTRACE_CMD_ARGS(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
		a[8], a[9]);
    }
    if (TCL_DTRACE_CMD_INFO_ENABLED() && iPtr->cmdFramePtr) {
	Tcl_Obj *info = TclInfoFrame(interp, iPtr->cmdFramePtr);
	const char *a[6]; int i[2];

	TclDTraceInfo(info, a, i);
	TCL_DTRACE_CMD_INFO(a[0], a[1], a[2], a[3], i[0], i[1], a[4], a[5]);
	TclDecrRefCount(info);
    }
    if ((TCL_DTRACE_CMD_RETURN_ENABLED() || TCL_DTRACE_CMD_RESULT_ENABLED())
	    && objc) {
	TclNRAddCallback(interp, DTraceCmdReturn, objv[0], NULL, NULL, NULL);
    }
    if (TCL_DTRACE_CMD_ENTRY_ENABLED() && objc) {
	TCL_DTRACE_CMD_ENTRY(TclGetString(objv[0]), objc - 1,
		(Tcl_Obj **)(objv + 1));
    }
#endif /* USE_DTRACE */

    iPtr->cmdCount++;
    return objProc(clientData, interp, objc, objv);
}

int
TclNRRunCallbacks(
    Tcl_Interp *interp,
    int result,
    struct NRE_callback *rootPtr)
				/* All callbacks down to rootPtr not inclusive
				 * are to be run. */
{
    Interp *iPtr = (Interp *) interp;
    NRE_callback *callbackPtr;
    Tcl_NRPostProc *procPtr;

    /*
     * If the interpreter has a non-empty string result, the result object is
     * either empty or stale because some function set interp->result
     * directly. If so, move the string result to the result object, then
     * reset the string result.
     *
     * This only needs to be done for the first item in the list: all other
     * are for NR function calls, and those are Tcl_Obj based.
     */

    if (*(iPtr->result) != 0) {
	(void) Tcl_GetObjResult(interp);
    }

    /* This is the trampoline. */

    while (TOP_CB(interp) != rootPtr) {
	callbackPtr = TOP_CB(interp);
	procPtr = callbackPtr->procPtr;
	TOP_CB(interp) = callbackPtr->nextPtr;
	result = procPtr(callbackPtr->data, interp, result);
	TCLNR_FREE(interp, callbackPtr);
    }
    return result;
}

static int
NRCommand(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Interp *iPtr = (Interp *) interp;

    iPtr->numLevels--;

     /*
      * If there is a tailcall, schedule it next
      */

    if (data[1] && (data[1] != INT2PTR(1))) {
        TclNRAddCallback(interp, TclNRTailcallEval, data[1], NULL, NULL, NULL);
    }

    /* OPT ??
     * Do not interrupt a series of cleanups with async or limit checks:
     * just check at the end?
     */

    if (TclAsyncReady(iPtr)) {
	result = Tcl_AsyncInvoke(interp, result);
    }
    if ((result == TCL_OK) && TclCanceled(iPtr)) {
	result = Tcl_Canceled(interp, TCL_LEAVE_ERR_MSG);
    }
    if (result == TCL_OK && TclLimitReady(iPtr->limit)) {
	result = Tcl_LimitCheck(interp);
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TEOV_Exception	 -
 * TEOV_LookupCmdFromObj -
 * TEOV_RunEnterTraces	 -
 * TEOV_RunLeaveTraces	 -
 * TEOV_NotFound	 -
 *
 *	These are helper functions for Tcl_EvalObjv.
 *
 *----------------------------------------------------------------------
 */

static void
TEOV_PushExceptionHandlers(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[],
    int flags)
{
    Interp *iPtr = (Interp *) interp;

    /*
     * If any error processing is necessary, push the appropriate records.
     * Note that we have to push them in the inverse order: first the one that
     * has to run last.
     */

    if (!(flags & TCL_EVAL_INVOKE)) {
	/*
	 * Error messages
	 */

	TclNRAddCallback(interp, TEOV_Error, INT2PTR(objc),
		(ClientData) objv, NULL, NULL);
    }

    if (iPtr->numLevels == 1) {
	/*
	 * No CONTINUE or BREAK at level 0, manage RETURN
	 */

	TclNRAddCallback(interp, TEOV_Exception, INT2PTR(iPtr->evalFlags),
		NULL, NULL, NULL);
    }
}

static void
TEOV_SwitchVarFrame(
    Tcl_Interp *interp)
{
    Interp *iPtr = (Interp *) interp;

    /*
     * Change the varFrame to be the rootVarFrame, and push a record to
     * restore things at the end.
     */

    TclNRAddCallback(interp, TEOV_RestoreVarFrame, iPtr->varFramePtr, NULL,
	    NULL, NULL);
    iPtr->varFramePtr = iPtr->rootFramePtr;
}

static int
TEOV_RestoreVarFrame(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    ((Interp *) interp)->varFramePtr = data[0];
    return result;
}

static int
TEOV_Exception(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Interp *iPtr = (Interp *) interp;
    int allowExceptions = (PTR2INT(data[0]) & TCL_ALLOW_EXCEPTIONS);

    if (result != TCL_OK) {
	if (result == TCL_RETURN) {
	    result = TclUpdateReturnInfo(iPtr);
	}
	if ((result != TCL_OK) && (result != TCL_ERROR) && !allowExceptions) {
	    ProcessUnexpectedResult(interp, result);
	    result = TCL_ERROR;
	}
    }

    /*
     * We are returning to level 0, so should process TclResetCancellation. As
     * numLevels has not *yet* been decreased, do not call it: do the thing
     * here directly.
     */

    TclUnsetCancelFlags(iPtr);
    return result;
}

static int
TEOV_Error(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *listPtr;
    const char *cmdString;
    int cmdLen;
    int objc = PTR2INT(data[0]);
    Tcl_Obj **objv = data[1];

    if ((result == TCL_ERROR) && !(iPtr->flags & ERR_ALREADY_LOGGED)){
	/*
	 * If there was an error, a command string will be needed for the
	 * error log: get it out of the itemPtr. The details depend on the
	 * type.
	 */

	listPtr = Tcl_NewListObj(objc, objv);
	cmdString = Tcl_GetStringFromObj(listPtr, &cmdLen);
	Tcl_LogCommandInfo(interp, cmdString, cmdString, cmdLen);
	Tcl_DecrRefCount(listPtr);
    }
    iPtr->flags &= ~ERR_ALREADY_LOGGED;
    return result;
}

static int
TEOV_NotFound(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[],
    Namespace *lookupNsPtr)
{
    Command * cmdPtr;
    Interp *iPtr = (Interp *) interp;
    int i, newObjc, handlerObjc;
    Tcl_Obj **newObjv, **handlerObjv;
    CallFrame *varFramePtr = iPtr->varFramePtr;
    Namespace *currNsPtr = NULL;/* Used to check for and invoke any registered
				 * unknown command handler for the current
				 * namespace (TIP 181). */
    Namespace *savedNsPtr = NULL;

    currNsPtr = varFramePtr->nsPtr;
    if ((currNsPtr == NULL) || (currNsPtr->unknownHandlerPtr == NULL)) {
	currNsPtr = iPtr->globalNsPtr;
	if (currNsPtr == NULL) {
	    Tcl_Panic("Tcl_EvalObjv: NULL global namespace pointer");
	}
    }

    /*
     * Check to see if the resolution namespace has lost its unknown handler.
     * If so, reset it to "::unknown".
     */

    if (currNsPtr->unknownHandlerPtr == NULL) {
	TclNewLiteralStringObj(currNsPtr->unknownHandlerPtr, "::unknown");
	Tcl_IncrRefCount(currNsPtr->unknownHandlerPtr);
    }

    /*
     * Get the list of words for the unknown handler and allocate enough space
     * to hold both the handler prefix and all words of the command invokation
     * itself.
     */

    Tcl_ListObjGetElements(NULL, currNsPtr->unknownHandlerPtr,
	    &handlerObjc, &handlerObjv);
    newObjc = objc + handlerObjc;
    newObjv = TclStackAlloc(interp, (int) sizeof(Tcl_Obj *) * newObjc);

    /*
     * Copy command prefix from unknown handler and add on the real command's
     * full argument list. Note that we only use memcpy() once because we have
     * to increment the reference count of all the handler arguments anyway.
     */

    for (i = 0; i < handlerObjc; ++i) {
	newObjv[i] = handlerObjv[i];
	Tcl_IncrRefCount(newObjv[i]);
    }
    memcpy(newObjv+handlerObjc, objv, sizeof(Tcl_Obj *) * (unsigned)objc);

    /*
     * Look up and invoke the handler (by recursive call to this function). If
     * there is no handler at all, instead of doing the recursive call we just
     * generate a generic error message; it would be an infinite-recursion
     * nightmare otherwise.
     *
     * In this case we worry a bit less about recursion for now, and call the
     * "blocking" interface.
     */

    cmdPtr = TEOV_LookupCmdFromObj(interp, newObjv[0], lookupNsPtr);
    if (cmdPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "invalid command name \"%s\"", TclGetString(objv[0])));
        Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "COMMAND",
                TclGetString(objv[0]), NULL);

	/*
	 * Release any resources we locked and allocated during the handler
	 * call.
	 */

	for (i = 0; i < handlerObjc; ++i) {
	    Tcl_DecrRefCount(newObjv[i]);
	}
	TclStackFree(interp, newObjv);
	return TCL_ERROR;
    }

    if (lookupNsPtr) {
	savedNsPtr = varFramePtr->nsPtr;
	varFramePtr->nsPtr = lookupNsPtr;
    }
    TclSkipTailcall(interp);
    TclNRAddCallback(interp, TEOV_NotFoundCallback, INT2PTR(handlerObjc),
	    newObjv, savedNsPtr, NULL);
    return TclNREvalObjv(interp, newObjc, newObjv, TCL_EVAL_NOERR, NULL);
}

static int
TEOV_NotFoundCallback(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Interp *iPtr = (Interp *) interp;
    int objc = PTR2INT(data[0]);
    Tcl_Obj **objv = data[1];
    Namespace *savedNsPtr = data[2];

    int i;

    if (savedNsPtr) {
	iPtr->varFramePtr->nsPtr = savedNsPtr;
    }

    /*
     * Release any resources we locked and allocated during the handler call.
     */

    for (i = 0; i < objc; ++i) {
	Tcl_DecrRefCount(objv[i]);
    }
    TclStackFree(interp, objv);

    return result;
}

static int
TEOV_RunEnterTraces(
    Tcl_Interp *interp,
    Command **cmdPtrPtr,
    Tcl_Obj *commandPtr,
    int objc,
    Tcl_Obj *const objv[])
{
    Interp *iPtr = (Interp *) interp;
    Command *cmdPtr = *cmdPtrPtr;
    int newEpoch, cmdEpoch = cmdPtr->cmdEpoch;
    int length, traceCode = TCL_OK;
    const char *command = Tcl_GetStringFromObj(commandPtr, &length);

    /*
     * Call trace functions.
     * Execute any command or execution traces. Note that we bump up the
     * command's reference count for the duration of the calling of the
     * traces so that the structure doesn't go away underneath our feet.
     */

    cmdPtr->refCount++;
    if (iPtr->tracePtr) {
	traceCode = TclCheckInterpTraces(interp, command, length,
		cmdPtr, TCL_OK, TCL_TRACE_ENTER_EXEC, objc, objv);
    }
    if ((cmdPtr->flags & CMD_HAS_EXEC_TRACES) && (traceCode == TCL_OK)) {
	traceCode = TclCheckExecutionTraces(interp, command, length,
		cmdPtr, TCL_OK, TCL_TRACE_ENTER_EXEC, objc, objv);
    }
    newEpoch = cmdPtr->cmdEpoch;
    TclCleanupCommandMacro(cmdPtr);

    if (traceCode != TCL_OK) {
	if (traceCode == TCL_ERROR) {
	    Tcl_Obj *info;

	    TclNewLiteralStringObj(info, "\n    (enter trace on \"");
	    Tcl_AppendLimitedToObj(info, command, length, 55, "...");
	    Tcl_AppendToObj(info, "\")", 2);
	    Tcl_AppendObjToErrorInfo(interp, info);
	    iPtr->flags |= ERR_ALREADY_LOGGED;
	}
	return traceCode;
    }
    if (cmdEpoch != newEpoch) {
	*cmdPtrPtr = NULL;
    }
    return TCL_OK;
}

static int
TEOV_RunLeaveTraces(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Interp *iPtr = (Interp *) interp;
    int traceCode = TCL_OK;
    int objc = PTR2INT(data[0]);
    Tcl_Obj *commandPtr = data[1];
    Command *cmdPtr = data[2];
    Tcl_Obj **objv = data[3];
    int length;
    const char *command = Tcl_GetStringFromObj(commandPtr, &length);

    if (!(cmdPtr->flags & CMD_IS_DELETED)) {
	if (cmdPtr->flags & CMD_HAS_EXEC_TRACES){
	    traceCode = TclCheckExecutionTraces(interp, command, length,
		    cmdPtr, result, TCL_TRACE_LEAVE_EXEC, objc, objv);
	}
	if (iPtr->tracePtr != NULL && traceCode == TCL_OK) {
	    traceCode = TclCheckInterpTraces(interp, command, length,
		    cmdPtr, result, TCL_TRACE_LEAVE_EXEC, objc, objv);
	}
    }

    /*
     * As cmdPtr is set, TclNRRunCallbacks is about to reduce the numlevels.
     * Prevent that by resetting the cmdPtr field and dealing right here with
     * cmdPtr->refCount.
     */

    TclCleanupCommandMacro(cmdPtr);

    if (traceCode != TCL_OK) {
	if (traceCode == TCL_ERROR) {
	    Tcl_Obj *info;

	    TclNewLiteralStringObj(info, "\n    (leave trace on \"");
	    Tcl_AppendLimitedToObj(info, command, length, 55, "...");
	    Tcl_AppendToObj(info, "\")", 2);
	    Tcl_AppendObjToErrorInfo(interp, info);
	    iPtr->flags |= ERR_ALREADY_LOGGED;
	}
	result = traceCode;
    }
    Tcl_DecrRefCount(commandPtr);
    return result;
}

static inline Command *
TEOV_LookupCmdFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *namePtr,
    Namespace *lookupNsPtr)
{
    Interp *iPtr = (Interp *) interp;
    Command *cmdPtr;
    Namespace *savedNsPtr = iPtr->varFramePtr->nsPtr;

    if (lookupNsPtr) {
	iPtr->varFramePtr->nsPtr = lookupNsPtr;
    }
    cmdPtr = (Command *) Tcl_GetCommandFromObj(interp, namePtr);
    iPtr->varFramePtr->nsPtr = savedNsPtr;
    return cmdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_EvalTokensStandard --
 *
 *	Given an array of tokens parsed from a Tcl command (e.g., the tokens
 *	that make up a word or the index for an array variable) this function
 *	evaluates the tokens and concatenates their values to form a single
 *	result value.
 *
 * Results:
 *	The return value is a standard Tcl completion code such as TCL_OK or
 *	TCL_ERROR. A result or error message is left in interp's result.
 *
 * Side effects:
 *	Depends on the array of tokens being evaled.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_EvalTokensStandard(
    Tcl_Interp *interp,		/* Interpreter in which to lookup variables,
				 * execute nested commands, and report
				 * errors. */
    Tcl_Token *tokenPtr,	/* Pointer to first in an array of tokens to
				 * evaluate and concatenate. */
    int count)			/* Number of tokens to consider at tokenPtr.
				 * Must be at least 1. */
{
    return TclSubstTokens(interp, tokenPtr, count, /* numLeftPtr */ NULL, 1,
	    NULL, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_EvalTokens --
 *
 *	Given an array of tokens parsed from a Tcl command (e.g., the tokens
 *	that make up a word or the index for an array variable) this function
 *	evaluates the tokens and concatenates their values to form a single
 *	result value.
 *
 * Results:
 *	The return value is a pointer to a newly allocated Tcl_Obj containing
 *	the value of the array of tokens. The reference count of the returned
 *	object has been incremented. If an error occurs in evaluating the
 *	tokens then a NULL value is returned and an error message is left in
 *	interp's result.
 *
 * Side effects:
 *	A new object is allocated to hold the result.
 *
 *----------------------------------------------------------------------
 *
 * This uses a non-standard return convention; its use is now deprecated. It
 * is a wrapper for the new function Tcl_EvalTokensStandard, and is not used
 * in the core any longer. It is only kept for backward compatibility.
 */

Tcl_Obj *
Tcl_EvalTokens(
    Tcl_Interp *interp,		/* Interpreter in which to lookup variables,
				 * execute nested commands, and report
				 * errors. */
    Tcl_Token *tokenPtr,	/* Pointer to first in an array of tokens to
				 * evaluate and concatenate. */
    int count)			/* Number of tokens to consider at tokenPtr.
				 * Must be at least 1. */
{
    Tcl_Obj *resPtr;

    if (Tcl_EvalTokensStandard(interp, tokenPtr, count) != TCL_OK) {
	return NULL;
    }
    resPtr = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(resPtr);
    Tcl_ResetResult(interp);
    return resPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_EvalEx, TclEvalEx --
 *
 *	This function evaluates a Tcl script without using the compiler or
 *	byte-code interpreter. It just parses the script, creates values for
 *	each word of each command, then calls EvalObjv to execute each
 *	command.
 *
 * Results:
 *	The return value is a standard Tcl completion code such as TCL_OK or
 *	TCL_ERROR. A result or error message is left in interp's result.
 *
 * Side effects:
 *	Depends on the script.
 *
 * TIP #280 : Keep public API, internally extended API.
 *----------------------------------------------------------------------
 */

int
Tcl_EvalEx(
    Tcl_Interp *interp,		/* Interpreter in which to evaluate the
				 * script. Also used for error reporting. */
    const char *script,		/* First character of script to evaluate. */
    int numBytes,		/* Number of bytes in script. If < 0, the
				 * script consists of all bytes up to the
				 * first null character. */
    int flags)			/* Collection of OR-ed bits that control the
				 * evaluation of the script. Only
				 * TCL_EVAL_GLOBAL is currently supported. */
{
    return TclEvalEx(interp, script, numBytes, flags, 1, NULL, script);
}

int
TclEvalEx(
    Tcl_Interp *interp,		/* Interpreter in which to evaluate the
				 * script. Also used for error reporting. */
    const char *script,		/* First character of script to evaluate. */
    int numBytes,		/* Number of bytes in script. If < 0, the
				 * script consists of all bytes up to the
				 * first NUL character. */
    int flags,			/* Collection of OR-ed bits that control the
				 * evaluation of the script. Only
				 * TCL_EVAL_GLOBAL is currently supported. */
    int line,			/* The line the script starts on. */
    int *clNextOuter,		/* Information about an outer context for */
    const char *outerScript)	/* continuation line data. This is set only in
				 * TclSubstTokens(), to properly handle
				 * [...]-nested commands. The 'outerScript'
				 * refers to the most-outer script containing
				 * the embedded command, which is refered to
				 * by 'script'. The 'clNextOuter' refers to
				 * the current entry in the table of
				 * continuation lines in this "master script",
				 * and the character offsets are relative to
				 * the 'outerScript' as well.
				 *
				 * If outerScript == script, then this call is
				 * for the outer-most script/command. See
				 * Tcl_EvalEx() and TclEvalObjEx() for places
				 * generating arguments for which this is
				 * true. */
{
    Interp *iPtr = (Interp *) interp;
    const char *p, *next;
    const unsigned int minObjs = 20;
    Tcl_Obj **objv, **objvSpace;
    int *expand, *lines, *lineSpace;
    Tcl_Token *tokenPtr;
    int commandLength, bytesLeft, expandRequested, code = TCL_OK;
    CallFrame *savedVarFramePtr;/* Saves old copy of iPtr->varFramePtr in case
				 * TCL_EVAL_GLOBAL was set. */
    int allowExceptions = (iPtr->evalFlags & TCL_ALLOW_EXCEPTIONS);
    int gotParse = 0;
    unsigned int i, objectsUsed = 0;
				/* These variables keep track of how much
				 * state has been allocated while evaluating
				 * the script, so that it can be freed
				 * properly if an error occurs. */
    Tcl_Parse *parsePtr = TclStackAlloc(interp, sizeof(Tcl_Parse));
    CmdFrame *eeFramePtr = TclStackAlloc(interp, sizeof(CmdFrame));
    Tcl_Obj **stackObjArray =
	    TclStackAlloc(interp, minObjs * sizeof(Tcl_Obj *));
    int *expandStack = TclStackAlloc(interp, minObjs * sizeof(int));
    int *linesStack = TclStackAlloc(interp, minObjs * sizeof(int));
				/* TIP #280 Structures for tracking of command
				 * locations. */
    int *clNext = NULL;		/* Pointer for the tracking of invisible
				 * continuation lines. Initialized only if the
				 * caller gave us a table of locations to
				 * track, via scriptCLLocPtr. It always refers
				 * to the table entry holding the location of
				 * the next invisible continuation line to
				 * look for, while parsing the script. */

    if (iPtr->scriptCLLocPtr) {
	if (clNextOuter) {
	    clNext = clNextOuter;
	} else {
	    clNext = &iPtr->scriptCLLocPtr->loc[0];
	}
    }

    if (numBytes < 0) {
	numBytes = strlen(script);
    }
    Tcl_ResetResult(interp);

    savedVarFramePtr = iPtr->varFramePtr;
    if (flags & TCL_EVAL_GLOBAL) {
	iPtr->varFramePtr = iPtr->rootFramePtr;
    }

    /*
     * Each iteration through the following loop parses the next command from
     * the script and then executes it.
     */

    objv = objvSpace = stackObjArray;
    lines = lineSpace = linesStack;
    expand = expandStack;
    p = script;
    bytesLeft = numBytes;

    /*
     * TIP #280 Initialize tracking. Do not push on the frame stack yet.
     *
     * We open a new context, either for a sourced script, or 'eval'.
     * For sourced files we always have a path object, even if nothing was
     * specified in the interp itself. That makes code using it simpler as
     * NULL checks can be left out. Sourced file without path in the
     * 'scriptFile' is possible during Tcl initialization.
     */

    eeFramePtr->level = iPtr->cmdFramePtr ? iPtr->cmdFramePtr->level + 1 : 1;
    eeFramePtr->framePtr = iPtr->framePtr;
    eeFramePtr->nextPtr = iPtr->cmdFramePtr;
    eeFramePtr->nline = 0;
    eeFramePtr->line = NULL;
    eeFramePtr->cmdObj = NULL;

    iPtr->cmdFramePtr = eeFramePtr;
    if (iPtr->evalFlags & TCL_EVAL_FILE) {
	/*
	 * Set up for a sourced file.
	 */

	eeFramePtr->type = TCL_LOCATION_SOURCE;

	if (iPtr->scriptFile) {
	    /*
	     * Normalization here, to have the correct pwd. Should have
	     * negligible impact on performance, as the norm should have been
	     * done already by the 'source' invoking us, and it caches the
	     * result.
	     */

	    Tcl_Obj *norm = Tcl_FSGetNormalizedPath(interp, iPtr->scriptFile);

	    if (norm == NULL) {
		/*
		 * Error message in the interp result.
		 */

		code = TCL_ERROR;
		goto error;
	    }
	    eeFramePtr->data.eval.path = norm;
	} else {
	    TclNewLiteralStringObj(eeFramePtr->data.eval.path, "");
	}
	Tcl_IncrRefCount(eeFramePtr->data.eval.path);
    } else {
	/*
	 * Set up for plain eval.
	 */

	eeFramePtr->type = TCL_LOCATION_EVAL;
	eeFramePtr->data.eval.path = NULL;
    }

    iPtr->evalFlags = 0;
    do {
	if (Tcl_ParseCommand(interp, p, bytesLeft, 0, parsePtr) != TCL_OK) {
	    code = TCL_ERROR;
	    Tcl_LogCommandInfo(interp, script, parsePtr->commandStart,
		    parsePtr->term + 1 - parsePtr->commandStart);
	    goto posterror;
	}

	/*
	 * TIP #280 Track lines. The parser may have skipped text till it
	 * found the command we are now at. We have to count the lines in this
	 * block, and do not forget invisible continuation lines.
	 */

	TclAdvanceLines(&line, p, parsePtr->commandStart);
	TclAdvanceContinuations(&line, &clNext,
		parsePtr->commandStart - outerScript);

	gotParse = 1;
	if (parsePtr->numWords > 0) {
	    /*
	     * TIP #280. Track lines within the words of the current
	     * command. We use a separate pointer into the table of
	     * continuation line locations to not lose our position for the
	     * per-command parsing.
	     */

	    int wordLine = line;
	    const char *wordStart = parsePtr->commandStart;
	    int *wordCLNext = clNext;
	    unsigned int objectsNeeded = 0;
	    unsigned int numWords = parsePtr->numWords;

	    /*
	     * Generate an array of objects for the words of the command.
	     */

	    if (numWords > minObjs) {
		expand =    ckalloc(numWords * sizeof(int));
		objvSpace = ckalloc(numWords * sizeof(Tcl_Obj *));
		lineSpace = ckalloc(numWords * sizeof(int));
	    }
	    expandRequested = 0;
	    objv = objvSpace;
	    lines = lineSpace;

	    iPtr->cmdFramePtr = eeFramePtr->nextPtr;
	    for (objectsUsed = 0, tokenPtr = parsePtr->tokenPtr;
		    objectsUsed < numWords;
		    objectsUsed++, tokenPtr += tokenPtr->numComponents+1) {
		/*
		 * TIP #280. Track lines to current word. Save the information
		 * on a per-word basis, signaling dynamic words as needed.
		 * Make the information available to the recursively called
		 * evaluator as well, including the type of context (source
		 * vs. eval).
		 */

		TclAdvanceLines(&wordLine, wordStart, tokenPtr->start);
		TclAdvanceContinuations(&wordLine, &wordCLNext,
			tokenPtr->start - outerScript);
		wordStart = tokenPtr->start;

		lines[objectsUsed] = TclWordKnownAtCompileTime(tokenPtr, NULL)
			? wordLine : -1;

		if (eeFramePtr->type == TCL_LOCATION_SOURCE) {
		    iPtr->evalFlags |= TCL_EVAL_FILE;
		}

		code = TclSubstTokens(interp, tokenPtr+1,
			tokenPtr->numComponents, NULL, wordLine,
			wordCLNext, outerScript);

		iPtr->evalFlags = 0;

		if (code != TCL_OK) {
		    break;
		}
		objv[objectsUsed] = Tcl_GetObjResult(interp);
		Tcl_IncrRefCount(objv[objectsUsed]);
		if (tokenPtr->type == TCL_TOKEN_EXPAND_WORD) {
		    int numElements;

		    code = TclListObjLength(interp, objv[objectsUsed],
			    &numElements);
		    if (code == TCL_ERROR) {
			/*
			 * Attempt to expand a non-list.
			 */

			Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
				"\n    (expanding word %d)", objectsUsed));
			Tcl_DecrRefCount(objv[objectsUsed]);
			break;
		    }
		    expandRequested = 1;
		    expand[objectsUsed] = 1;

		    objectsNeeded += (numElements ? numElements : 1);
		} else {
		    expand[objectsUsed] = 0;
		    objectsNeeded++;
		}

		if (wordCLNext) {
		    TclContinuationsEnterDerived(objv[objectsUsed],
			    wordStart - outerScript, wordCLNext);
		}
	    } /* for loop */
	    iPtr->cmdFramePtr = eeFramePtr;
	    if (code != TCL_OK) {
		goto error;
	    }
	    if (expandRequested) {
		/*
		 * Some word expansion was requested. Check for objv resize.
		 */

		Tcl_Obj **copy = objvSpace;
		int *lcopy = lineSpace;
		int wordIdx = numWords;
		int objIdx = objectsNeeded - 1;

		if ((numWords > minObjs) || (objectsNeeded > minObjs)) {
		    objv = objvSpace =
			    ckalloc(objectsNeeded * sizeof(Tcl_Obj *));
		    lines = lineSpace = ckalloc(objectsNeeded * sizeof(int));
		}

		objectsUsed = 0;
		while (wordIdx--) {
		    if (expand[wordIdx]) {
			int numElements;
			Tcl_Obj **elements, *temp = copy[wordIdx];

			Tcl_ListObjGetElements(NULL, temp, &numElements,
				&elements);
			objectsUsed += numElements;
			while (numElements--) {
			    lines[objIdx] = -1;
			    objv[objIdx--] = elements[numElements];
			    Tcl_IncrRefCount(elements[numElements]);
			}
			Tcl_DecrRefCount(temp);
		    } else {
			lines[objIdx] = lcopy[wordIdx];
			objv[objIdx--] = copy[wordIdx];
			objectsUsed++;
		    }
		}
		objv += objIdx+1;

		if (copy != stackObjArray) {
		    ckfree(copy);
		}
		if (lcopy != linesStack) {
		    ckfree(lcopy);
		}
	    }

	    /*
	     * Execute the command and free the objects for its words.
	     *
	     * TIP #280: Remember the command itself for 'info frame'. We
	     * shorten the visible command by one char to exclude the
	     * termination character, if necessary. Here is where we put our
	     * frame on the stack of frames too. _After_ the nested commands
	     * have been executed.
	     */

	    eeFramePtr->cmd = parsePtr->commandStart;
	    eeFramePtr->len = parsePtr->commandSize;

	    if (parsePtr->term ==
		    parsePtr->commandStart + parsePtr->commandSize - 1) {
		eeFramePtr->len--;
	    }

	    eeFramePtr->nline = objectsUsed;
	    eeFramePtr->line = lines;

	    TclArgumentEnter(interp, objv, objectsUsed, eeFramePtr);
	    code = Tcl_EvalObjv(interp, objectsUsed, objv,
		    TCL_EVAL_NOERR | TCL_EVAL_SOURCE_IN_FRAME);
	    TclArgumentRelease(interp, objv, objectsUsed);

	    eeFramePtr->line = NULL;
	    eeFramePtr->nline = 0;
	    if (eeFramePtr->cmdObj) {
		Tcl_DecrRefCount(eeFramePtr->cmdObj);
		eeFramePtr->cmdObj = NULL;
	    }

	    if (code != TCL_OK) {
		goto error;
	    }
	    for (i = 0; i < objectsUsed; i++) {
		Tcl_DecrRefCount(objv[i]);
	    }
	    objectsUsed = 0;
	    if (objvSpace != stackObjArray) {
		ckfree(objvSpace);
		objvSpace = stackObjArray;
		ckfree(lineSpace);
		lineSpace = linesStack;
	    }

	    /*
	     * Free expand separately since objvSpace could have been
	     * reallocated above.
	     */

	    if (expand != expandStack) {
		ckfree(expand);
		expand = expandStack;
	    }
	}

	/*
	 * Advance to the next command in the script.
	 *
	 * TIP #280 Track Lines. Now we track how many lines were in the
	 * executed command.
	 */

	next = parsePtr->commandStart + parsePtr->commandSize;
	bytesLeft -= next - p;
	p = next;
	TclAdvanceLines(&line, parsePtr->commandStart, p);
	Tcl_FreeParse(parsePtr);
	gotParse = 0;
    } while (bytesLeft > 0);
    iPtr->varFramePtr = savedVarFramePtr;
    code = TCL_OK;
    goto cleanup_return;

  error:
    /*
     * Generate and log various pieces of error information.
     */

    if (iPtr->numLevels == 0) {
	if (code == TCL_RETURN) {
	    code = TclUpdateReturnInfo(iPtr);
	}
	if ((code != TCL_OK) && (code != TCL_ERROR) && !allowExceptions) {
	    ProcessUnexpectedResult(interp, code);
	    code = TCL_ERROR;
	}
    }
    if ((code == TCL_ERROR) && !(iPtr->flags & ERR_ALREADY_LOGGED)) {
	commandLength = parsePtr->commandSize;
	if (parsePtr->term == parsePtr->commandStart + commandLength - 1) {
	    /*
	     * The terminator character (such as ; or ]) of the command where
	     * the error occurred is the last character in the parsed command.
	     * Reduce the length by one so that the error message doesn't
	     * include the terminator character.
	     */

	    commandLength -= 1;
	}
	Tcl_LogCommandInfo(interp, script, parsePtr->commandStart,
		commandLength);
    }
 posterror:
    iPtr->flags &= ~ERR_ALREADY_LOGGED;

    /*
     * Then free resources that had been allocated to the command.
     */

    for (i = 0; i < objectsUsed; i++) {
	Tcl_DecrRefCount(objv[i]);
    }
    if (gotParse) {
	Tcl_FreeParse(parsePtr);
    }
    if (objvSpace != stackObjArray) {
	ckfree(objvSpace);
	ckfree(lineSpace);
    }
    if (expand != expandStack) {
	ckfree(expand);
    }
    iPtr->varFramePtr = savedVarFramePtr;

 cleanup_return:
    /*
     * TIP #280. Release the local CmdFrame, and its contents.
     */

    iPtr->cmdFramePtr = iPtr->cmdFramePtr->nextPtr;
    if (eeFramePtr->type == TCL_LOCATION_SOURCE) {
	Tcl_DecrRefCount(eeFramePtr->data.eval.path);
    }
    TclStackFree(interp, linesStack);
    TclStackFree(interp, expandStack);
    TclStackFree(interp, stackObjArray);
    TclStackFree(interp, eeFramePtr);
    TclStackFree(interp, parsePtr);

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclAdvanceLines --
 *
 *	This function is a helper which counts the number of lines in a block
 *	of text and advances an external counter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The specified counter is advanced per the number of lines found.
 *
 * TIP #280
 *----------------------------------------------------------------------
 */

void
TclAdvanceLines(
    int *line,
    const char *start,
    const char *end)
{
    register const char *p;

    for (p = start; p < end; p++) {
	if (*p == '\n') {
	    (*line)++;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclAdvanceContinuations --
 *
 *	This procedure is a helper which counts the number of continuation
 *	lines (CL) in a block of text using a table of CL locations and
 *	advances an external counter, and the pointer into the table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The specified counter is advanced per the number of continuation lines
 *	found.
 *
 * TIP #280
 *----------------------------------------------------------------------
 */

void
TclAdvanceContinuations(
    int *line,
    int **clNextPtrPtr,
    int loc)
{
    /*
     * Track the invisible continuation lines embedded in a script, if any.
     * Here they are just spaces (already). They were removed by
     * TclSubstTokens via TclParseBackslash.
     *
     * *clNextPtrPtr         <=> We have continuation lines to track.
     * **clNextPtrPtr >= 0   <=> We are not beyond the last possible location.
     * loc >= **clNextPtrPtr <=> We stepped beyond the current cont. line.
     */

    while (*clNextPtrPtr && (**clNextPtrPtr >= 0)
	    && (loc >= **clNextPtrPtr)) {
	/*
	 * We just stepped over an invisible continuation line. Adjust the
	 * line counter and step to the table entry holding the location of
	 * the next continuation line to track.
	 */

	(*line)++;
	(*clNextPtrPtr)++;
    }
}

/*
 *----------------------------------------------------------------------
 * Note: The whole data structure access for argument location tracking is
 * hidden behind these three functions. The only parts open are the lineLAPtr
 * field in the Interp structure. The CFWord definition is internal to here.
 * Should make it easier to redo the data structures if we find something more
 * space/time efficient.
 */

/*
 *----------------------------------------------------------------------
 *
 * TclArgumentEnter --
 *
 *	This procedure is a helper for the TIP #280 uplevel extension. It
 *	enters location references for the arguments of a command to be
 *	invoked. Only the first entry has the actual data, further entries
 *	simply count the usage up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May allocate memory.
 *
 * TIP #280
 *----------------------------------------------------------------------
 */

void
TclArgumentEnter(
    Tcl_Interp *interp,
    Tcl_Obj **objv,
    int objc,
    CmdFrame *cfPtr)
{
    Interp *iPtr = (Interp *) interp;
    int new, i;
    Tcl_HashEntry *hPtr;
    CFWord *cfwPtr;

    for (i = 1; i < objc; i++) {
	/*
	 * Ignore argument words without line information (= dynamic). If they
	 * are variables they may have location information associated with
	 * that, either through globally recorded 'set' invokations, or
	 * literals in bytecode. Eitehr way there is no need to record
	 * something here.
	 */

	if (cfPtr->line[i] < 0) {
	    continue;
	}
	hPtr = Tcl_CreateHashEntry(iPtr->lineLAPtr, objv[i], &new);
	if (new) {
	    /*
	     * The word is not on the stack yet, remember the current location
	     * and initialize references.
	     */

	    cfwPtr = ckalloc(sizeof(CFWord));
	    cfwPtr->framePtr = cfPtr;
	    cfwPtr->word = i;
	    cfwPtr->refCount = 1;
	    Tcl_SetHashValue(hPtr, cfwPtr);
	} else {
	    /*
	     * The word is already on the stack, its current location is not
	     * relevant. Just remember the reference to prevent early removal.
	     */

	    cfwPtr = Tcl_GetHashValue(hPtr);
	    cfwPtr->refCount++;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclArgumentRelease --
 *
 *	This procedure is a helper for the TIP #280 uplevel extension. It
 *	removes the location references for the arguments of a command just
 *	done. Usage is counted down, the data is removed only when no user is
 *	left over.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May release memory.
 *
 * TIP #280
 *----------------------------------------------------------------------
 */

void
TclArgumentRelease(
    Tcl_Interp *interp,
    Tcl_Obj **objv,
    int objc)
{
    Interp *iPtr = (Interp *) interp;
    int i;

    for (i = 1; i < objc; i++) {
	CFWord *cfwPtr;
	Tcl_HashEntry *hPtr =
		Tcl_FindHashEntry(iPtr->lineLAPtr, (char *) objv[i]);

	if (!hPtr) {
	    continue;
	}
	cfwPtr = Tcl_GetHashValue(hPtr);

	cfwPtr->refCount--;
	if (cfwPtr->refCount > 0) {
	    continue;
	}

	ckfree(cfwPtr);
	Tcl_DeleteHashEntry(hPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclArgumentBCEnter --
 *
 *	This procedure is a helper for the TIP #280 uplevel extension. It
 *	enters location references for the literal arguments of commands in
 *	bytecode about to be invoked. Only the first entry has the actual
 *	data, further entries simply count the usage up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May allocate memory.
 *
 * TIP #280
 *----------------------------------------------------------------------
 */

void
TclArgumentBCEnter(
    Tcl_Interp *interp,
    Tcl_Obj *objv[],
    int objc,
    void *codePtr,
    CmdFrame *cfPtr,
    int cmd,
    int pc)
{
    ExtCmdLoc *eclPtr;
    int word;
    ECL *ePtr;
    CFWordBC *lastPtr = NULL;
    Interp *iPtr = (Interp *) interp;
    Tcl_HashEntry *hePtr =
	    Tcl_FindHashEntry(iPtr->lineBCPtr, (char *) codePtr);

    if (!hePtr) {
	return;
    }
    eclPtr = Tcl_GetHashValue(hePtr);
    ePtr = &eclPtr->loc[cmd];

    /*
     * ePtr->nline is the number of words originally parsed.
     *
     * objc is the number of elements getting invoked.
     *
     * If they are not the same, we arrived here by compiling an
     * ensemble dispatch.  Ensemble subcommands that lead to script
     * evaluation are not supposed to get compiled, because a command
     * such as [info level] in the script can expose some of the dispatch
     * shenanigans.  This means that we don't have to tend to the
     * housekeeping, and can escape now.
     */

    if (ePtr->nline != objc) {
        return;
    }

    /*
     * Having disposed of the ensemble cases, we can state...
     * A few truths ...
     * (1) ePtr->nline == objc
     * (2) (ePtr->line[word] < 0) => !literal, for all words
     * (3) (word == 0) => !literal
     *
     * Item (2) is why we can use objv to get the literals, and do not
     * have to save them at compile time.
     */

    for (word = 1; word < objc; word++) {
	if (ePtr->line[word] >= 0) {
	    int isnew;
	    Tcl_HashEntry *hPtr = Tcl_CreateHashEntry(iPtr->lineLABCPtr,
		objv[word], &isnew);
	    CFWordBC *cfwPtr = ckalloc(sizeof(CFWordBC));

	    cfwPtr->framePtr = cfPtr;
	    cfwPtr->obj = objv[word];
	    cfwPtr->pc = pc;
	    cfwPtr->word = word;
	    cfwPtr->nextPtr = lastPtr;
	    lastPtr = cfwPtr;

	    if (isnew) {
		/*
		 * The word is not on the stack yet, remember the current
		 * location and initialize references.
		 */

		cfwPtr->prevPtr = NULL;
	    } else {
		/*
		 * The object is already on the stack, however it may have
		 * a different location now (literal sharing may map
		 * multiple location to a single Tcl_Obj*. Save the old
		 * information in the new structure.
		 */

		cfwPtr->prevPtr = Tcl_GetHashValue(hPtr);
	    }

	    Tcl_SetHashValue(hPtr, cfwPtr);
	}
    } /* for */

    cfPtr->litarg = lastPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclArgumentBCRelease --
 *
 *	This procedure is a helper for the TIP #280 uplevel extension. It
 *	removes the location references for the literal arguments of commands
 *	in bytecode just done. Usage is counted down, the data is removed only
 *	when no user is left over.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May release memory.
 *
 * TIP #280
 *----------------------------------------------------------------------
 */

void
TclArgumentBCRelease(
    Tcl_Interp *interp,
    CmdFrame *cfPtr)
{
    Interp *iPtr = (Interp *) interp;
    CFWordBC *cfwPtr = (CFWordBC *) cfPtr->litarg;

    while (cfwPtr) {
	CFWordBC *nextPtr = cfwPtr->nextPtr;
	Tcl_HashEntry *hPtr =
		Tcl_FindHashEntry(iPtr->lineLABCPtr, (char *) cfwPtr->obj);
	CFWordBC *xPtr = Tcl_GetHashValue(hPtr);

	if (xPtr != cfwPtr) {
	    Tcl_Panic("TclArgumentBC Enter/Release Mismatch");
	}

	if (cfwPtr->prevPtr) {
	    Tcl_SetHashValue(hPtr, cfwPtr->prevPtr);
	} else {
	    Tcl_DeleteHashEntry(hPtr);
	}

	ckfree(cfwPtr);
	cfwPtr = nextPtr;
    }

    cfPtr->litarg = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TclArgumentGet --
 *
 *	This procedure is a helper for the TIP #280 uplevel extension. It
 *	finds the location references for a Tcl_Obj, if any.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes found location information into the result arguments.
 *
 * TIP #280
 *----------------------------------------------------------------------
 */

void
TclArgumentGet(
    Tcl_Interp *interp,
    Tcl_Obj *obj,
    CmdFrame **cfPtrPtr,
    int *wordPtr)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_HashEntry *hPtr;
    CmdFrame *framePtr;

    /*
     * An object which either has no string rep or else is a canonical list is
     * guaranteed to have been generated dynamically: bail out, this cannot
     * have a usable absolute location. _Do not touch_ the information the set
     * up by the caller. It knows better than us.
     */

    if ((obj->bytes == NULL) || TclListObjIsCanonical(obj)) {
	return;
    }

    /*
     * First look for location information recorded in the argument
     * stack. That is nearest.
     */

    hPtr = Tcl_FindHashEntry(iPtr->lineLAPtr, (char *) obj);
    if (hPtr) {
	CFWord *cfwPtr = Tcl_GetHashValue(hPtr);

	*wordPtr = cfwPtr->word;
	*cfPtrPtr = cfwPtr->framePtr;
	return;
    }

    /*
     * Check if the Tcl_Obj has location information as a bytecode literal, in
     * that stack.
     */

    hPtr = Tcl_FindHashEntry(iPtr->lineLABCPtr, (char *) obj);
    if (hPtr) {
	CFWordBC *cfwPtr = Tcl_GetHashValue(hPtr);

	framePtr = cfwPtr->framePtr;
	framePtr->data.tebc.pc = (char *) (((ByteCode *)
		framePtr->data.tebc.codePtr)->codeStart + cfwPtr->pc);
	*cfPtrPtr = cfwPtr->framePtr;
	*wordPtr = cfwPtr->word;
	return;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Eval --
 *
 *	Execute a Tcl command in a string. This function executes the script
 *	directly, rather than compiling it to bytecodes. Before the arrival of
 *	the bytecode compiler in Tcl 8.0 Tcl_Eval was the main function used
 *	for executing Tcl commands, but nowadays it isn't used much.
 *
 * Results:
 *	The return value is one of the return codes defined in tcl.h (such as
 *	TCL_OK), and interp's result contains a value to supplement the return
 *	code. The value of the result will persist only until the next call to
 *	Tcl_Eval or Tcl_EvalObj: you must copy it or lose it!
 *
 * Side effects:
 *	Can be almost arbitrary, depending on the commands in the script.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_Eval
int
Tcl_Eval(
    Tcl_Interp *interp,		/* Token for command interpreter (returned by
				 * previous call to Tcl_CreateInterp). */
    const char *script)		/* Pointer to TCL command to execute. */
{
    int code = Tcl_EvalEx(interp, script, -1, 0);

    /*
     * For backwards compatibility with old C code that predates the object
     * system in Tcl 8.0, we have to mirror the object result back into the
     * string result (some callers may expect it there).
     */

    (void) Tcl_GetStringResult(interp);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_EvalObj, Tcl_GlobalEvalObj --
 *
 *	These functions are deprecated but we keep them around for backwards
 *	compatibility reasons.
 *
 * Results:
 *	See the functions they call.
 *
 * Side effects:
 *	See the functions they call.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_EvalObj
int
Tcl_EvalObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    return Tcl_EvalObjEx(interp, objPtr, 0);
}
#undef Tcl_GlobalEvalObj
int
Tcl_GlobalEvalObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    return Tcl_EvalObjEx(interp, objPtr, TCL_EVAL_GLOBAL);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_EvalObjEx, TclEvalObjEx --
 *
 *	Execute Tcl commands stored in a Tcl object. These commands are
 *	compiled into bytecodes if necessary, unless TCL_EVAL_DIRECT is
 *	specified.
 *
 *	If the flag TCL_EVAL_DIRECT is passed in, the value of invoker
 *	must be NULL.  Support for non-NULL invokers in that mode has
 *	been removed since it was unused and untested.  Failure to
 *	follow this limitation will lead to an assertion panic.
 *
 * Results:
 *	The return value is one of the return codes defined in tcl.h (such as
 *	TCL_OK), and the interpreter's result contains a value to supplement
 *	the return code.
 *
 * Side effects:
 *	The object is converted, if necessary, to a ByteCode object that holds
 *	the bytecode instructions for the commands. Executing the commands
 *	will almost certainly have side effects that depend on those commands.
 *
 * TIP #280 : Keep public API, internally extended API.
 *----------------------------------------------------------------------
 */

int
Tcl_EvalObjEx(
    Tcl_Interp *interp,		/* Token for command interpreter (returned by
				 * a previous call to Tcl_CreateInterp). */
    register Tcl_Obj *objPtr,	/* Pointer to object containing commands to
				 * execute. */
    int flags)			/* Collection of OR-ed bits that control the
				 * evaluation of the script. Supported values
				 * are TCL_EVAL_GLOBAL and TCL_EVAL_DIRECT. */
{
    return TclEvalObjEx(interp, objPtr, flags, NULL, 0);
}

int
TclEvalObjEx(
    Tcl_Interp *interp,		/* Token for command interpreter (returned by
				 * a previous call to Tcl_CreateInterp). */
    register Tcl_Obj *objPtr,	/* Pointer to object containing commands to
				 * execute. */
    int flags,			/* Collection of OR-ed bits that control the
				 * evaluation of the script. Supported values
				 * are TCL_EVAL_GLOBAL and TCL_EVAL_DIRECT. */
    const CmdFrame *invoker,	/* Frame of the command doing the eval. */
    int word)			/* Index of the word which is in objPtr. */
{
    int result = TCL_OK;
    NRE_callback *rootPtr = TOP_CB(interp);

    result = TclNREvalObjEx(interp, objPtr, flags, invoker, word);
    return TclNRRunCallbacks(interp, result, rootPtr);
}

int
TclNREvalObjEx(
    Tcl_Interp *interp,		/* Token for command interpreter (returned by
				 * a previous call to Tcl_CreateInterp). */
    register Tcl_Obj *objPtr,	/* Pointer to object containing commands to
				 * execute. */
    int flags,			/* Collection of OR-ed bits that control the
				 * evaluation of the script. Supported values
				 * are TCL_EVAL_GLOBAL and TCL_EVAL_DIRECT. */
    const CmdFrame *invoker,	/* Frame of the command doing the eval. */
    int word)			/* Index of the word which is in objPtr. */
{
    Interp *iPtr = (Interp *) interp;
    int result;

    /*
     * This function consists of three independent blocks for: direct
     * evaluation of canonical lists, compilation and bytecode execution and
     * finally direct evaluation. Precisely one of these blocks will be run.
     */

    if (TclListObjIsCanonical(objPtr)) {
	CmdFrame *eoFramePtr = NULL;
	int objc;
	Tcl_Obj *listPtr, **objv;

	/*
	 * Canonical List Optimization:  In this case, we
	 * can safely use Tcl_EvalObjv instead and get an appreciable
	 * improvement in execution speed. This is because it allows us to
	 * avoid a setFromAny step that would just pack everything into a
	 * string and back out again.
	 *
	 * This also preserves any associations between list elements and
	 * location information for such elements.
	 */

	/*
	 * Shimmer protection! Always pass an unshared obj. The caller could
	 * incr the refCount of objPtr AFTER calling us! To be completely safe
	 * we always make a copy. The callback takes care od the refCounts for
	 * both listPtr and objPtr.
	 *
	 * TODO: Create a test to demo this need, or eliminate it.
	 * FIXME OPT: preserve just the internal rep?
	 */

	Tcl_IncrRefCount(objPtr);
	listPtr = TclListObjCopy(interp, objPtr);
	Tcl_IncrRefCount(listPtr);

	if (word != INT_MIN) {
	    /*
	     * TIP #280 Structures for tracking lines. As we know that this is
	     * dynamic execution we ignore the invoker, even if known.
	     *
	     * TIP #280. We do _not_ compute all the line numbers for the
	     * words in the command. For the eval of a pure list the most
	     * sensible choice is to put all words on line 1. Given that we
	     * neither need memory for them nor compute anything. 'line' is
	     * left NULL. The two places using this information (TclInfoFrame,
	     * and TclInitCompileEnv), are special-cased to use the proper
	     * line number directly instead of accessing the 'line' array.
	     *
	     * Note that we use (word==INTMIN) to signal that no command frame
	     * should be pushed, as needed by alias and ensemble redirections.
	     */

	    eoFramePtr = TclStackAlloc(interp, sizeof(CmdFrame));
	    eoFramePtr->nline = 0;
	    eoFramePtr->line = NULL;

	    eoFramePtr->type = TCL_LOCATION_EVAL;
	    eoFramePtr->level = (iPtr->cmdFramePtr == NULL?
		    1 : iPtr->cmdFramePtr->level + 1);
	    eoFramePtr->framePtr = iPtr->framePtr;
	    eoFramePtr->nextPtr = iPtr->cmdFramePtr;

	    eoFramePtr->cmdObj = objPtr;
	    eoFramePtr->cmd = NULL;
	    eoFramePtr->len = 0;
	    eoFramePtr->data.eval.path = NULL;

	    iPtr->cmdFramePtr = eoFramePtr;

	    flags |= TCL_EVAL_SOURCE_IN_FRAME;
	}

	TclMarkTailcall(interp);
        TclNRAddCallback(interp, TEOEx_ListCallback, listPtr, eoFramePtr,
		objPtr, NULL);

	ListObjGetElements(listPtr, objc, objv);
	return TclNREvalObjv(interp, objc, objv, flags, NULL);
    }

    if (!(flags & TCL_EVAL_DIRECT)) {
	/*
	 * Let the compiler/engine subsystem do the evaluation.
	 *
	 * TIP #280 The invoker provides us with the context for the script.
	 * We transfer this to the byte code compiler.
	 */

	int allowExceptions = (iPtr->evalFlags & TCL_ALLOW_EXCEPTIONS);
	ByteCode *codePtr;
	CallFrame *savedVarFramePtr = NULL;	/* Saves old copy of
						 * iPtr->varFramePtr in case
						 * TCL_EVAL_GLOBAL was set. */

        if (TclInterpReady(interp) != TCL_OK) {
            return TCL_ERROR;
        }
	if (flags & TCL_EVAL_GLOBAL) {
	    savedVarFramePtr = iPtr->varFramePtr;
	    iPtr->varFramePtr = iPtr->rootFramePtr;
	}
	Tcl_IncrRefCount(objPtr);
	codePtr = TclCompileObj(interp, objPtr, invoker, word);

	TclNRAddCallback(interp, TEOEx_ByteCodeCallback, savedVarFramePtr,
		objPtr, INT2PTR(allowExceptions), NULL);
        return TclNRExecuteByteCode(interp, codePtr);
    }

    {
	/*
	 * We're not supposed to use the compiler or byte-code
	 * interpreter. Let Tcl_EvalEx evaluate the command directly (and
	 * probably more slowly).
	 */

	const char *script;
	int numSrcBytes;

	/*
	 * Now we check if we have data about invisible continuation lines for
	 * the script, and make it available to the direct script parser and
	 * evaluator we are about to call, if so.
	 *
	 * It may be possible that the script Tcl_Obj* can be free'd while the
	 * evaluator is using it, leading to the release of the associated
	 * ContLineLoc structure as well. To ensure that the latter doesn't
	 * happen we set a lock on it. We release this lock later in this
	 * function, after the evaluator is done. The relevant "lineCLPtr"
	 * hashtable is managed in the file "tclObj.c".
	 *
	 * Another important action is to save (and later restore) the
	 * continuation line information of the caller, in case we are
	 * executing nested commands in the eval/direct path.
	 */

	ContLineLoc *saveCLLocPtr = iPtr->scriptCLLocPtr;

	assert(invoker == NULL);

	iPtr->scriptCLLocPtr = TclContinuationsGet(objPtr);

	Tcl_IncrRefCount(objPtr);

	script = Tcl_GetStringFromObj(objPtr, &numSrcBytes);
	result = Tcl_EvalEx(interp, script, numSrcBytes, flags);

	TclDecrRefCount(objPtr);

	iPtr->scriptCLLocPtr = saveCLLocPtr;
	return result;
    }
}

static int
TEOEx_ByteCodeCallback(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Interp *iPtr = (Interp *) interp;
    CallFrame *savedVarFramePtr = data[0];
    Tcl_Obj *objPtr = data[1];
    int allowExceptions = PTR2INT(data[2]);

    if (iPtr->numLevels == 0) {
	if (result == TCL_RETURN) {
	    result = TclUpdateReturnInfo(iPtr);
	}
	if ((result != TCL_OK) && (result != TCL_ERROR) && !allowExceptions) {
	    const char *script;
	    int numSrcBytes;

	    ProcessUnexpectedResult(interp, result);
	    result = TCL_ERROR;
	    script = Tcl_GetStringFromObj(objPtr, &numSrcBytes);
	    Tcl_LogCommandInfo(interp, script, script, numSrcBytes);
	}

	/*
	 * We are returning to level 0, so should call TclResetCancellation.
	 * Let us just unset the flags inline.
	 */

	TclUnsetCancelFlags(iPtr);
    }
    iPtr->evalFlags = 0;

    /*
     * Restore the callFrame if this was a TCL_EVAL_GLOBAL.
     */

    if (savedVarFramePtr) {
	iPtr->varFramePtr = savedVarFramePtr;
    }

    TclDecrRefCount(objPtr);
    return result;
}

static int
TEOEx_ListCallback(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *listPtr = data[0];
    CmdFrame *eoFramePtr = data[1];
    Tcl_Obj *objPtr = data[2];

    /*
     * Remove the cmdFrame
     */

    if (eoFramePtr) {
	iPtr->cmdFramePtr = eoFramePtr->nextPtr;
	TclStackFree(interp, eoFramePtr);
    }
    TclDecrRefCount(objPtr);
    TclDecrRefCount(listPtr);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ProcessUnexpectedResult --
 *
 *	Function called by Tcl_EvalObj to set the interpreter's result value
 *	to an appropriate error message when the code it evaluates returns an
 *	unexpected result code (not TCL_OK and not TCL_ERROR) to the topmost
 *	evaluation level.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The interpreter result is set to an error message appropriate to the
 *	result code.
 *
 *----------------------------------------------------------------------
 */

static void
ProcessUnexpectedResult(
    Tcl_Interp *interp,		/* The interpreter in which the unexpected
				 * result code was returned. */
    int returnCode)		/* The unexpected result code. */
{
    char buf[TCL_INTEGER_SPACE];

    Tcl_ResetResult(interp);
    if (returnCode == TCL_BREAK) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"invoked \"break\" outside of a loop", -1));
    } else if (returnCode == TCL_CONTINUE) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"invoked \"continue\" outside of a loop", -1));
    } else {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"command returned bad code: %d", returnCode));
    }
    sprintf(buf, "%d", returnCode);
    Tcl_SetErrorCode(interp, "TCL", "UNEXPECTED_RESULT_CODE", buf, NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_ExprLong, Tcl_ExprDouble, Tcl_ExprBoolean --
 *
 *	Functions to evaluate an expression and return its value in a
 *	particular form.
 *
 * Results:
 *	Each of the functions below returns a standard Tcl result. If an error
 *	occurs then an error message is left in the interp's result. Otherwise
 *	the value of the expression, in the appropriate form, is stored at
 *	*ptr. If the expression had a result that was incompatible with the
 *	desired form then an error is returned.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_ExprLong(
    Tcl_Interp *interp,		/* Context in which to evaluate the
				 * expression. */
    const char *exprstring,	/* Expression to evaluate. */
    long *ptr)			/* Where to store result. */
{
    register Tcl_Obj *exprPtr;
    int result = TCL_OK;
    if (*exprstring == '\0') {
	/*
	 * Legacy compatibility - return 0 for the zero-length string.
	 */

	*ptr = 0;
    } else {
	exprPtr = Tcl_NewStringObj(exprstring, -1);
	Tcl_IncrRefCount(exprPtr);
	result = Tcl_ExprLongObj(interp, exprPtr, ptr);
	Tcl_DecrRefCount(exprPtr);
	if (result != TCL_OK) {
	    (void) Tcl_GetStringResult(interp);
	}
    }
    return result;
}

int
Tcl_ExprDouble(
    Tcl_Interp *interp,		/* Context in which to evaluate the
				 * expression. */
    const char *exprstring,	/* Expression to evaluate. */
    double *ptr)		/* Where to store result. */
{
    register Tcl_Obj *exprPtr;
    int result = TCL_OK;

    if (*exprstring == '\0') {
	/*
	 * Legacy compatibility - return 0 for the zero-length string.
	 */

	*ptr = 0.0;
    } else {
	exprPtr = Tcl_NewStringObj(exprstring, -1);
	Tcl_IncrRefCount(exprPtr);
	result = Tcl_ExprDoubleObj(interp, exprPtr, ptr);
	Tcl_DecrRefCount(exprPtr);
				/* Discard the expression object. */
	if (result != TCL_OK) {
	    (void) Tcl_GetStringResult(interp);
	}
    }
    return result;
}

int
Tcl_ExprBoolean(
    Tcl_Interp *interp,		/* Context in which to evaluate the
				 * expression. */
    const char *exprstring,	/* Expression to evaluate. */
    int *ptr)			/* Where to store 0/1 result. */
{
    if (*exprstring == '\0') {
	/*
	 * An empty string. Just set the result boolean to 0 (false).
	 */

	*ptr = 0;
	return TCL_OK;
    } else {
	int result;
	Tcl_Obj *exprPtr = Tcl_NewStringObj(exprstring, -1);

	Tcl_IncrRefCount(exprPtr);
	result = Tcl_ExprBooleanObj(interp, exprPtr, ptr);
	Tcl_DecrRefCount(exprPtr);
	if (result != TCL_OK) {
	    /*
	     * Move the interpreter's object result to the string result, then
	     * reset the object result.
	     */

	    (void) Tcl_GetStringResult(interp);
	}
	return result;
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tcl_ExprLongObj, Tcl_ExprDoubleObj, Tcl_ExprBooleanObj --
 *
 *	Functions to evaluate an expression in an object and return its value
 *	in a particular form.
 *
 * Results:
 *	Each of the functions below returns a standard Tcl result object. If
 *	an error occurs then an error message is left in the interpreter's
 *	result. Otherwise the value of the expression, in the appropriate
 *	form, is stored at *ptr. If the expression had a result that was
 *	incompatible with the desired form then an error is returned.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tcl_ExprLongObj(
    Tcl_Interp *interp,		/* Context in which to evaluate the
				 * expression. */
    register Tcl_Obj *objPtr,	/* Expression to evaluate. */
    long *ptr)			/* Where to store long result. */
{
    Tcl_Obj *resultPtr;
    int result, type;
    double d;
    ClientData internalPtr;

    result = Tcl_ExprObj(interp, objPtr, &resultPtr);
    if (result != TCL_OK) {
	return TCL_ERROR;
    }

    if (TclGetNumberFromObj(interp, resultPtr, &internalPtr, &type)!=TCL_OK) {
	return TCL_ERROR;
    }

    switch (type) {
    case TCL_NUMBER_DOUBLE: {
	mp_int big;

	d = *((const double *) internalPtr);
	Tcl_DecrRefCount(resultPtr);
	if (Tcl_InitBignumFromDouble(interp, d, &big) != TCL_OK) {
	    return TCL_ERROR;
	}
	resultPtr = Tcl_NewBignumObj(&big);
	/* FALLTHROUGH */
    }
    case TCL_NUMBER_LONG:
    case TCL_NUMBER_WIDE:
    case TCL_NUMBER_BIG:
	result = TclGetLongFromObj(interp, resultPtr, ptr);
	break;

    case TCL_NUMBER_NAN:
	Tcl_GetDoubleFromObj(interp, resultPtr, &d);
	result = TCL_ERROR;
    }

    Tcl_DecrRefCount(resultPtr);/* Discard the result object. */
    return result;
}

int
Tcl_ExprDoubleObj(
    Tcl_Interp *interp,		/* Context in which to evaluate the
				 * expression. */
    register Tcl_Obj *objPtr,	/* Expression to evaluate. */
    double *ptr)		/* Where to store double result. */
{
    Tcl_Obj *resultPtr;
    int result, type;
    ClientData internalPtr;

    result = Tcl_ExprObj(interp, objPtr, &resultPtr);
    if (result != TCL_OK) {
	return TCL_ERROR;
    }

    result = TclGetNumberFromObj(interp, resultPtr, &internalPtr, &type);
    if (result == TCL_OK) {
	switch (type) {
	case TCL_NUMBER_NAN:
#ifndef ACCEPT_NAN
	    result = Tcl_GetDoubleFromObj(interp, resultPtr, ptr);
	    break;
#endif
	case TCL_NUMBER_DOUBLE:
	    *ptr = *((const double *) internalPtr);
	    result = TCL_OK;
	    break;
	default:
	    result = Tcl_GetDoubleFromObj(interp, resultPtr, ptr);
	}
    }
    Tcl_DecrRefCount(resultPtr);/* Discard the result object. */
    return result;
}

int
Tcl_ExprBooleanObj(
    Tcl_Interp *interp,		/* Context in which to evaluate the
				 * expression. */
    register Tcl_Obj *objPtr,	/* Expression to evaluate. */
    int *ptr)			/* Where to store 0/1 result. */
{
    Tcl_Obj *resultPtr;
    int result;

    result = Tcl_ExprObj(interp, objPtr, &resultPtr);
    if (result == TCL_OK) {
	result = Tcl_GetBooleanFromObj(interp, resultPtr, ptr);
	Tcl_DecrRefCount(resultPtr);
				/* Discard the result object. */
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclObjInvokeNamespace --
 *
 *	Object version: Invokes a Tcl command, given an objv/objc, from either
 *	the exposed or hidden set of commands in the given interpreter.
 *
 *	NOTE: The command is invoked in the global stack frame of the
 *	interpreter or namespace, thus it cannot see any current state on the
 *	stack of that interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Whatever the command does.
 *
 *----------------------------------------------------------------------
 */

int
TclObjInvokeNamespace(
    Tcl_Interp *interp,		/* Interpreter in which command is to be
				 * invoked. */
    int objc,			/* Count of arguments. */
    Tcl_Obj *const objv[],	/* Argument objects; objv[0] points to the
				 * name of the command to invoke. */
    Tcl_Namespace *nsPtr,	/* The namespace to use. */
    int flags)			/* Combination of flags controlling the call:
				 * TCL_INVOKE_HIDDEN, TCL_INVOKE_NO_UNKNOWN,
				 * or TCL_INVOKE_NO_TRACEBACK. */
{
    int result;
    Tcl_CallFrame *framePtr;

    /*
     * Make the specified namespace the current namespace and invoke the
     * command.
     */

    (void) TclPushStackFrame(interp, &framePtr, nsPtr, /*isProcFrame*/0);
    result = TclObjInvoke(interp, objc, objv, flags);

    TclPopStackFrame(interp);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclObjInvoke --
 *
 *	Invokes a Tcl command, given an objv/objc, from either the exposed or
 *	the hidden sets of commands in the given interpreter.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	Whatever the command does.
 *
 *----------------------------------------------------------------------
 */

int
TclObjInvoke(
    Tcl_Interp *interp,		/* Interpreter in which command is to be
				 * invoked. */
    int objc,			/* Count of arguments. */
    Tcl_Obj *const objv[],	/* Argument objects; objv[0] points to the
				 * name of the command to invoke. */
    int flags)			/* Combination of flags controlling the call:
				 * TCL_INVOKE_HIDDEN, TCL_INVOKE_NO_UNKNOWN,
				 * or TCL_INVOKE_NO_TRACEBACK. */
{
    if (interp == NULL) {
	return TCL_ERROR;
    }
    if ((objc < 1) || (objv == NULL)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "illegal argument vector", -1));
	return TCL_ERROR;
    }
    if ((flags & TCL_INVOKE_HIDDEN) == 0) {
	Tcl_Panic("TclObjInvoke: called without TCL_INVOKE_HIDDEN");
    }
    return Tcl_NRCallObjProc(interp, TclNRInvoke, NULL, objc, objv);
}

int
TclNRInvoke(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    register Interp *iPtr = (Interp *) interp;
    Tcl_HashTable *hTblPtr;	/* Table of hidden commands. */
    const char *cmdName;	/* Name of the command from objv[0]. */
    Tcl_HashEntry *hPtr = NULL;
    Command *cmdPtr;

    cmdName = TclGetString(objv[0]);
    hTblPtr = iPtr->hiddenCmdTablePtr;
    if (hTblPtr != NULL) {
	hPtr = Tcl_FindHashEntry(hTblPtr, cmdName);
    }
    if (hPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "invalid hidden command name \"%s\"", cmdName));
        Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "HIDDENTOKEN", cmdName,
                NULL);
	return TCL_ERROR;
    }
    cmdPtr = Tcl_GetHashValue(hPtr);

    /* Avoid the exception-handling brain damage when numLevels == 0 . */
    iPtr->numLevels++;
    Tcl_NRAddCallback(interp, NRPostInvoke, NULL, NULL, NULL, NULL);

    /*
     * Normal command resolution of objv[0] isn't going to find cmdPtr.
     * That's the whole point of **hidden** commands.  So tell the
     * Eval core machinery not to even try (and risk finding something wrong).
     */

    return TclNREvalObjv(interp, objc, objv, TCL_EVAL_NORESOLVE, cmdPtr);
}

static int
NRPostInvoke(
    ClientData clientData[],
    Tcl_Interp *interp,
    int result)
{
    Interp *iPtr = (Interp *)interp;
    iPtr->numLevels--;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_ExprString --
 *
 *	Evaluate an expression in a string and return its value in string
 *	form.
 *
 * Results:
 *	A standard Tcl result. If the result is TCL_OK, then the interp's
 *	result is set to the string value of the expression. If the result is
 *	TCL_ERROR, then the interp's result contains an error message.
 *
 * Side effects:
 *	A Tcl object is allocated to hold a copy of the expression string.
 *	This expression object is passed to Tcl_ExprObj and then deallocated.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_ExprString(
    Tcl_Interp *interp,		/* Context in which to evaluate the
				 * expression. */
    const char *expr)		/* Expression to evaluate. */
{
    int code = TCL_OK;

    if (expr[0] == '\0') {
	/*
	 * An empty string. Just set the interpreter's result to 0.
	 */

	Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
    } else {
	Tcl_Obj *resultPtr, *exprObj = Tcl_NewStringObj(expr, -1);

	Tcl_IncrRefCount(exprObj);
	code = Tcl_ExprObj(interp, exprObj, &resultPtr);
	Tcl_DecrRefCount(exprObj);
	if (code == TCL_OK) {
	    Tcl_SetObjResult(interp, resultPtr);
	    Tcl_DecrRefCount(resultPtr);
	}
    }

    /*
     * Force the string rep of the interp result.
     */

    (void) Tcl_GetStringResult(interp);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendObjToErrorInfo --
 *
 *	Add a Tcl_Obj value to the errorInfo field that describes the current
 *	error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The value of the Tcl_obj is appended to the errorInfo field. If we are
 *	just starting to log an error, errorInfo is initialized from the error
 *	message in the interpreter's result.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_AddObjErrorInfo
void
Tcl_AppendObjToErrorInfo(
    Tcl_Interp *interp,		/* Interpreter to which error information
				 * pertains. */
    Tcl_Obj *objPtr)		/* Message to record. */
{
    int length;
    const char *message = TclGetStringFromObj(objPtr, &length);

    Tcl_IncrRefCount(objPtr);
    Tcl_AddObjErrorInfo(interp, message, length);
    Tcl_DecrRefCount(objPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AddErrorInfo --
 *
 *	Add information to the errorInfo field that describes the current
 *	error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of message are appended to the errorInfo field. If we are
 *	just starting to log an error, errorInfo is initialized from the error
 *	message in the interpreter's result.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_AddErrorInfo
void
Tcl_AddErrorInfo(
    Tcl_Interp *interp,		/* Interpreter to which error information
				 * pertains. */
    const char *message)	/* Message to record. */
{
    Tcl_AddObjErrorInfo(interp, message, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AddObjErrorInfo --
 *
 *	Add information to the errorInfo field that describes the current
 *	error. This routine differs from Tcl_AddErrorInfo by taking a byte
 *	pointer and length.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	"length" bytes from "message" are appended to the errorInfo field. If
 *	"length" is negative, use bytes up to the first NULL byte. If we are
 *	just starting to log an error, errorInfo is initialized from the error
 *	message in the interpreter's result.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AddObjErrorInfo(
    Tcl_Interp *interp,		/* Interpreter to which error information
				 * pertains. */
    const char *message,	/* Points to the first byte of an array of
				 * bytes of the message. */
    int length)			/* The number of bytes in the message. If < 0,
				 * then append all bytes up to a NULL byte. */
{
    register Interp *iPtr = (Interp *) interp;

    /*
     * If we are just starting to log an error, errorInfo is initialized from
     * the error message in the interpreter's result.
     */

    iPtr->flags |= ERR_LEGACY_COPY;
    if (iPtr->errorInfo == NULL) {
	if (iPtr->result[0] != 0) {
	    /*
	     * The interp's string result is set, apparently by some extension
	     * making a deprecated direct write to it. That extension may
	     * expect interp->result to continue to be set, so we'll take
	     * special pains to avoid clearing it, until we drop support for
	     * interp->result completely.
	     */

	    iPtr->errorInfo = Tcl_NewStringObj(iPtr->result, -1);
	} else {
	    iPtr->errorInfo = iPtr->objResultPtr;
	}
	Tcl_IncrRefCount(iPtr->errorInfo);
	if (!iPtr->errorCode) {
	    Tcl_SetErrorCode(interp, "NONE", NULL);
	}
    }

    /*
     * Now append "message" to the end of errorInfo.
     */

    if (length != 0) {
	if (Tcl_IsShared(iPtr->errorInfo)) {
	    Tcl_DecrRefCount(iPtr->errorInfo);
	    iPtr->errorInfo = Tcl_DuplicateObj(iPtr->errorInfo);
	    Tcl_IncrRefCount(iPtr->errorInfo);
	}
	Tcl_AppendToObj(iPtr->errorInfo, message, length);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_VarEvalVA --
 *
 *	Given a variable number of string arguments, concatenate them all
 *	together and execute the result as a Tcl command.
 *
 * Results:
 *	A standard Tcl return result. An error message or other result may be
 *	left in the interp's result.
 *
 * Side effects:
 *	Depends on what was done by the command.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_VarEvalVA(
    Tcl_Interp *interp,		/* Interpreter in which to evaluate command */
    va_list argList)		/* Variable argument list. */
{
    Tcl_DString buf;
    char *string;
    int result;

    /*
     * Copy the strings one after the other into a single larger string. Use
     * stack-allocated space for small commands, but if the command gets too
     * large than call ckalloc to create the space.
     */

    Tcl_DStringInit(&buf);
    while (1) {
	string = va_arg(argList, char *);
	if (string == NULL) {
	    break;
	}
	Tcl_DStringAppend(&buf, string, -1);
    }

    result = Tcl_Eval(interp, Tcl_DStringValue(&buf));
    Tcl_DStringFree(&buf);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_VarEval --
 *
 *	Given a variable number of string arguments, concatenate them all
 *	together and execute the result as a Tcl command.
 *
 * Results:
 *	A standard Tcl return result. An error message or other result may be
 *	left in interp->result.
 *
 * Side effects:
 *	Depends on what was done by the command.
 *
 *----------------------------------------------------------------------
 */
	/* ARGSUSED */
int
Tcl_VarEval(
    Tcl_Interp *interp,
    ...)
{
    va_list argList;
    int result;

    va_start(argList, interp);
    result = Tcl_VarEvalVA(interp, argList);
    va_end(argList);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GlobalEval --
 *
 *	Evaluate a command at global level in an interpreter.
 *
 * Results:
 *	A standard Tcl result is returned, and the interp's result is modified
 *	accordingly.
 *
 * Side effects:
 *	The command string is executed in interp, and the execution is carried
 *	out in the variable context of global level (no functions active),
 *	just as if an "uplevel #0" command were being executed.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_GlobalEval
int
Tcl_GlobalEval(
    Tcl_Interp *interp,		/* Interpreter in which to evaluate
				 * command. */
    const char *command)	/* Command to evaluate. */
{
    register Interp *iPtr = (Interp *) interp;
    int result;
    CallFrame *savedVarFramePtr;

    savedVarFramePtr = iPtr->varFramePtr;
    iPtr->varFramePtr = iPtr->rootFramePtr;
    result = Tcl_Eval(interp, command);
    iPtr->varFramePtr = savedVarFramePtr;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetRecursionLimit --
 *
 *	Set the maximum number of recursive calls that may be active for an
 *	interpreter at once.
 *
 * Results:
 *	The return value is the old limit on nesting for interp.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_SetRecursionLimit(
    Tcl_Interp *interp,		/* Interpreter whose nesting limit is to be
				 * set. */
    int depth)			/* New value for maximimum depth. */
{
    Interp *iPtr = (Interp *) interp;
    int old;

    old = iPtr->maxNestingDepth;
    if (depth > 0) {
	iPtr->maxNestingDepth = depth;
    }
    return old;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AllowExceptions --
 *
 *	Sets a flag in an interpreter so that exceptions can occur in the next
 *	call to Tcl_Eval without them being turned into errors.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The TCL_ALLOW_EXCEPTIONS flag gets set in the interpreter's evalFlags
 *	structure. See the reference documentation for more details.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AllowExceptions(
    Tcl_Interp *interp)		/* Interpreter in which to set flag. */
{
    Interp *iPtr = (Interp *) interp;

    iPtr->evalFlags |= TCL_ALLOW_EXCEPTIONS;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetVersion --
 *
 *	Get the Tcl major, minor, and patchlevel version numbers and the
 *	release type. A patch is a release type TCL_FINAL_RELEASE with a
 *	patchLevel > 0.
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
Tcl_GetVersion(
    int *majorV,
    int *minorV,
    int *patchLevelV,
    int *type)
{
    if (majorV != NULL) {
	*majorV = TCL_MAJOR_VERSION;
    }
    if (minorV != NULL) {
	*minorV = TCL_MINOR_VERSION;
    }
    if (patchLevelV != NULL) {
	*patchLevelV = TCL_RELEASE_SERIAL;
    }
    if (type != NULL) {
	*type = TCL_RELEASE_LEVEL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Math Functions --
 *
 *	This page contains the functions that implement all of the built-in
 *	math functions for expressions.
 *
 * Results:
 *	Each function returns TCL_OK if it succeeds and pushes an Tcl object
 *	holding the result. If it fails it returns TCL_ERROR and leaves an
 *	error message in the interpreter's result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ExprCeilFunc(
    ClientData clientData,	/* Ignored */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter list. */
{
    int code;
    double d;
    mp_int big;

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }
    code = Tcl_GetDoubleFromObj(interp, objv[1], &d);
#ifdef ACCEPT_NAN
    if ((code != TCL_OK) && (objv[1]->typePtr == &tclDoubleType)) {
	Tcl_SetObjResult(interp, objv[1]);
	return TCL_OK;
    }
#endif
    if (code != TCL_OK) {
	return TCL_ERROR;
    }

    if (Tcl_GetBignumFromObj(NULL, objv[1], &big) == TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(TclCeil(&big)));
	mp_clear(&big);
    } else {
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(ceil(d)));
    }
    return TCL_OK;
}

static int
ExprFloorFunc(
    ClientData clientData,	/* Ignored */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter list. */
{
    int code;
    double d;
    mp_int big;

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }
    code = Tcl_GetDoubleFromObj(interp, objv[1], &d);
#ifdef ACCEPT_NAN
    if ((code != TCL_OK) && (objv[1]->typePtr == &tclDoubleType)) {
	Tcl_SetObjResult(interp, objv[1]);
	return TCL_OK;
    }
#endif
    if (code != TCL_OK) {
	return TCL_ERROR;
    }

    if (Tcl_GetBignumFromObj(NULL, objv[1], &big) == TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(TclFloor(&big)));
	mp_clear(&big);
    } else {
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(floor(d)));
    }
    return TCL_OK;
}

static int
ExprIsqrtFunc(
    ClientData clientData,	/* Ignored */
    Tcl_Interp *interp,		/* The interpreter in which to execute. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter list. */
{
    ClientData ptr;
    int type;
    double d;
    Tcl_WideInt w;
    mp_int big;
    int exact = 0;		/* Flag ==1 if the argument can be represented
				 * in a double as an exact integer. */

    /*
     * Check syntax.
     */

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }

    /*
     * Make sure that the arg is a number.
     */

    if (TclGetNumberFromObj(interp, objv[1], &ptr, &type) != TCL_OK) {
	return TCL_ERROR;
    }

    switch (type) {
    case TCL_NUMBER_NAN:
	Tcl_GetDoubleFromObj(interp, objv[1], &d);
	return TCL_ERROR;
    case TCL_NUMBER_DOUBLE:
	d = *((const double *) ptr);
	if (d < 0) {
	    goto negarg;
	}
#ifdef IEEE_FLOATING_POINT
	if (d <= MAX_EXACT) {
	    exact = 1;
	}
#endif
	if (!exact) {
	    if (Tcl_InitBignumFromDouble(interp, d, &big) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	break;
    case TCL_NUMBER_BIG:
	if (Tcl_GetBignumFromObj(interp, objv[1], &big) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (SIGN(&big) == MP_NEG) {
	    mp_clear(&big);
	    goto negarg;
	}
	break;
    default:
	if (TclGetWideIntFromObj(interp, objv[1], &w) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (w < 0) {
	    goto negarg;
	}
	d = (double) w;
#ifdef IEEE_FLOATING_POINT
	if (d < MAX_EXACT) {
	    exact = 1;
	}
#endif
	if (!exact) {
	    Tcl_GetBignumFromObj(interp, objv[1], &big);
	}
	break;
    }

    if (exact) {
	Tcl_SetObjResult(interp, Tcl_NewWideIntObj((Tcl_WideInt) sqrt(d)));
    } else {
	mp_int root;

	mp_init(&root);
	mp_sqrt(&big, &root);
	mp_clear(&big);
	Tcl_SetObjResult(interp, Tcl_NewBignumObj(&root));
    }
    return TCL_OK;

  negarg:
    Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "square root of negative argument", -1));
    Tcl_SetErrorCode(interp, "ARITH", "DOMAIN",
	    "domain error: argument not in valid range", NULL);
    return TCL_ERROR;
}

static int
ExprSqrtFunc(
    ClientData clientData,	/* Ignored */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter list. */
{
    int code;
    double d;
    mp_int big;

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }
    code = Tcl_GetDoubleFromObj(interp, objv[1], &d);
#ifdef ACCEPT_NAN
    if ((code != TCL_OK) && (objv[1]->typePtr == &tclDoubleType)) {
	Tcl_SetObjResult(interp, objv[1]);
	return TCL_OK;
    }
#endif
    if (code != TCL_OK) {
	return TCL_ERROR;
    }
    if ((d >= 0.0) && TclIsInfinite(d)
	    && (Tcl_GetBignumFromObj(NULL, objv[1], &big) == TCL_OK)) {
	mp_int root;

	mp_init(&root);
	mp_sqrt(&big, &root);
	mp_clear(&big);
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(TclBignumToDouble(&root)));
	mp_clear(&root);
    } else {
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(sqrt(d)));
    }
    return TCL_OK;
}

static int
ExprUnaryFunc(
    ClientData clientData,	/* Contains the address of a function that
				 * takes one double argument and returns a
				 * double result. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count */
    Tcl_Obj *const *objv)	/* Actual parameter list */
{
    int code;
    double d;
    double (*func)(double) = (double (*)(double)) clientData;

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }
    code = Tcl_GetDoubleFromObj(interp, objv[1], &d);
#ifdef ACCEPT_NAN
    if ((code != TCL_OK) && (objv[1]->typePtr == &tclDoubleType)) {
	d = objv[1]->internalRep.doubleValue;
	Tcl_ResetResult(interp);
	code = TCL_OK;
    }
#endif
    if (code != TCL_OK) {
	return TCL_ERROR;
    }
    errno = 0;
    return CheckDoubleResult(interp, func(d));
}

static int
CheckDoubleResult(
    Tcl_Interp *interp,
    double dResult)
{
#ifndef ACCEPT_NAN
    if (TclIsNaN(dResult)) {
	TclExprFloatError(interp, dResult);
	return TCL_ERROR;
    }
#endif
    if ((errno == ERANGE) && ((dResult == 0.0) || TclIsInfinite(dResult))) {
	/*
	 * When ERANGE signals under/overflow, just accept 0.0 or +/-Inf
	 */
    } else if (errno != 0) {
	/*
	 * Report other errno values as errors.
	 */

	TclExprFloatError(interp, dResult);
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(dResult));
    return TCL_OK;
}

static int
ExprBinaryFunc(
    ClientData clientData,	/* Contains the address of a function that
				 * takes two double arguments and returns a
				 * double result. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Parameter vector. */
{
    int code;
    double d1, d2;
    double (*func)(double, double) = (double (*)(double, double)) clientData;

    if (objc != 3) {
	MathFuncWrongNumArgs(interp, 3, objc, objv);
	return TCL_ERROR;
    }
    code = Tcl_GetDoubleFromObj(interp, objv[1], &d1);
#ifdef ACCEPT_NAN
    if ((code != TCL_OK) && (objv[1]->typePtr == &tclDoubleType)) {
	d1 = objv[1]->internalRep.doubleValue;
	Tcl_ResetResult(interp);
	code = TCL_OK;
    }
#endif
    if (code != TCL_OK) {
	return TCL_ERROR;
    }
    code = Tcl_GetDoubleFromObj(interp, objv[2], &d2);
#ifdef ACCEPT_NAN
    if ((code != TCL_OK) && (objv[2]->typePtr == &tclDoubleType)) {
	d2 = objv[2]->internalRep.doubleValue;
	Tcl_ResetResult(interp);
	code = TCL_OK;
    }
#endif
    if (code != TCL_OK) {
	return TCL_ERROR;
    }
    errno = 0;
    return CheckDoubleResult(interp, func(d1, d2));
}

static int
ExprAbsFunc(
    ClientData clientData,	/* Ignored. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Parameter vector. */
{
    ClientData ptr;
    int type;
    mp_int big;

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }

    if (TclGetNumberFromObj(interp, objv[1], &ptr, &type) != TCL_OK) {
	return TCL_ERROR;
    }

    if (type == TCL_NUMBER_LONG) {
	long l = *((const long *) ptr);

	if (l > (long)0) {
	    goto unChanged;
	} else if (l == (long)0) {
	    const char *string = objv[1]->bytes;
	    if (string) {
		while (*string != '0') {
		    if (*string == '-') {
			Tcl_SetObjResult(interp, Tcl_NewLongObj(0));
			return TCL_OK;
		    }
		    string++;
		}
	    }
	    goto unChanged;
	} else if (l == LONG_MIN) {
	    TclBNInitBignumFromLong(&big, l);
	    goto tooLarge;
	}
	Tcl_SetObjResult(interp, Tcl_NewLongObj(-l));
	return TCL_OK;
    }

    if (type == TCL_NUMBER_DOUBLE) {
	double d = *((const double *) ptr);
	static const double poszero = 0.0;

	/*
	 * We need to distinguish here between positive 0.0 and negative -0.0.
	 * [Bug 2954959]
	 */

	if (d == -0.0) {
	    if (!memcmp(&d, &poszero, sizeof(double))) {
		goto unChanged;
	    }
	} else if (d > -0.0) {
	    goto unChanged;
	}
	Tcl_SetObjResult(interp, Tcl_NewDoubleObj(-d));
	return TCL_OK;
    }

#ifndef TCL_WIDE_INT_IS_LONG
    if (type == TCL_NUMBER_WIDE) {
	Tcl_WideInt w = *((const Tcl_WideInt *) ptr);

	if (w >= (Tcl_WideInt)0) {
	    goto unChanged;
	}
	if (w == LLONG_MIN) {
	    TclBNInitBignumFromWideInt(&big, w);
	    goto tooLarge;
	}
	Tcl_SetObjResult(interp, Tcl_NewWideIntObj(-w));
	return TCL_OK;
    }
#endif

    if (type == TCL_NUMBER_BIG) {
	if (mp_cmp_d((const mp_int *) ptr, 0) == MP_LT) {
	    Tcl_GetBignumFromObj(NULL, objv[1], &big);
	tooLarge:
	    mp_neg(&big, &big);
	    Tcl_SetObjResult(interp, Tcl_NewBignumObj(&big));
	} else {
	unChanged:
	    Tcl_SetObjResult(interp, objv[1]);
	}
	return TCL_OK;
    }

    if (type == TCL_NUMBER_NAN) {
#ifdef ACCEPT_NAN
	Tcl_SetObjResult(interp, objv[1]);
	return TCL_OK;
#else
	double d;

	Tcl_GetDoubleFromObj(interp, objv[1], &d);
	return TCL_ERROR;
#endif
    }
    return TCL_OK;
}

static int
ExprBoolFunc(
    ClientData clientData,	/* Ignored. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter vector. */
{
    int value;

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }
    if (Tcl_GetBooleanFromObj(interp, objv[1], &value) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(value));
    return TCL_OK;
}

static int
ExprDoubleFunc(
    ClientData clientData,	/* Ignored. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter vector. */
{
    double dResult;

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[1], &dResult) != TCL_OK) {
#ifdef ACCEPT_NAN
	if (objv[1]->typePtr == &tclDoubleType) {
	    Tcl_SetObjResult(interp, objv[1]);
	    return TCL_OK;
	}
#endif
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(dResult));
    return TCL_OK;
}

static int
ExprEntierFunc(
    ClientData clientData,	/* Ignored. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter vector. */
{
    double d;
    int type;
    ClientData ptr;

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }
    if (TclGetNumberFromObj(interp, objv[1], &ptr, &type) != TCL_OK) {
	return TCL_ERROR;
    }

    if (type == TCL_NUMBER_DOUBLE) {
	d = *((const double *) ptr);
	if ((d < (double)LONG_MAX) && (d > (double)LONG_MIN)) {
	    long result = (long) d;

	    Tcl_SetObjResult(interp, Tcl_NewLongObj(result));
	    return TCL_OK;
#ifndef TCL_WIDE_INT_IS_LONG
	} else if ((d < (double)LLONG_MAX) && (d > (double)LLONG_MIN)) {
	    Tcl_WideInt result = (Tcl_WideInt) d;

	    Tcl_SetObjResult(interp, Tcl_NewWideIntObj(result));
	    return TCL_OK;
#endif
	} else {
	    mp_int big;

	    if (Tcl_InitBignumFromDouble(interp, d, &big) != TCL_OK) {
		/* Infinity */
		return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp, Tcl_NewBignumObj(&big));
	    return TCL_OK;
	}
    }

    if (type != TCL_NUMBER_NAN) {
	/*
	 * All integers are already of integer type.
	 */

	Tcl_SetObjResult(interp, objv[1]);
	return TCL_OK;
    }

    /*
     * Get the error message for NaN.
     */

    Tcl_GetDoubleFromObj(interp, objv[1], &d);
    return TCL_ERROR;
}

static int
ExprIntFunc(
    ClientData clientData,	/* Ignored. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter vector. */
{
    long iResult;
    Tcl_Obj *objPtr;
    if (ExprEntierFunc(NULL, interp, objc, objv) != TCL_OK) {
	return TCL_ERROR;
    }
    objPtr = Tcl_GetObjResult(interp);
    if (TclGetLongFromObj(NULL, objPtr, &iResult) != TCL_OK) {
	/*
	 * Truncate the bignum; keep only bits in long range.
	 */

	mp_int big;

	Tcl_GetBignumFromObj(NULL, objPtr, &big);
	mp_mod_2d(&big, (int) CHAR_BIT * sizeof(long), &big);
	objPtr = Tcl_NewBignumObj(&big);
	Tcl_IncrRefCount(objPtr);
	TclGetLongFromObj(NULL, objPtr, &iResult);
	Tcl_DecrRefCount(objPtr);
    }
    Tcl_SetObjResult(interp, Tcl_NewLongObj(iResult));
    return TCL_OK;
}

static int
ExprWideFunc(
    ClientData clientData,	/* Ignored. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter vector. */
{
    Tcl_WideInt wResult;
    Tcl_Obj *objPtr;

    if (ExprEntierFunc(NULL, interp, objc, objv) != TCL_OK) {
	return TCL_ERROR;
    }
    objPtr = Tcl_GetObjResult(interp);
    if (TclGetWideIntFromObj(NULL, objPtr, &wResult) != TCL_OK) {
	/*
	 * Truncate the bignum; keep only bits in wide int range.
	 */

	mp_int big;

	Tcl_GetBignumFromObj(NULL, objPtr, &big);
	mp_mod_2d(&big, (int) CHAR_BIT * sizeof(Tcl_WideInt), &big);
	objPtr = Tcl_NewBignumObj(&big);
	Tcl_IncrRefCount(objPtr);
	TclGetWideIntFromObj(NULL, objPtr, &wResult);
	Tcl_DecrRefCount(objPtr);
    }
    Tcl_SetObjResult(interp, Tcl_NewWideIntObj(wResult));
    return TCL_OK;
}

static int
ExprRandFunc(
    ClientData clientData,	/* Ignored. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter vector. */
{
    Interp *iPtr = (Interp *) interp;
    double dResult;
    long tmp;			/* Algorithm assumes at least 32 bits. Only
				 * long guarantees that. See below. */
    Tcl_Obj *oResult;

    if (objc != 1) {
	MathFuncWrongNumArgs(interp, 1, objc, objv);
	return TCL_ERROR;
    }

    if (!(iPtr->flags & RAND_SEED_INITIALIZED)) {
	iPtr->flags |= RAND_SEED_INITIALIZED;

	/*
	 * To ensure different seeds in different threads (bug #416643), 
	 * take into consideration the thread this interp is running in.
	 */

	iPtr->randSeed = TclpGetClicks() + (PTR2INT(Tcl_GetCurrentThread())<<12);

	/*
	 * Make sure 1 <= randSeed <= (2^31) - 2. See below.
	 */

	iPtr->randSeed &= (unsigned long) 0x7fffffff;
	if ((iPtr->randSeed == 0) || (iPtr->randSeed == 0x7fffffff)) {
	    iPtr->randSeed ^= 123459876;
	}
    }

    /*
     * Generate the random number using the linear congruential generator
     * defined by the following recurrence:
     *		seed = ( IA * seed ) mod IM
     * where IA is 16807 and IM is (2^31) - 1. The recurrence maps a seed in
     * the range [1, IM - 1] to a new seed in that same range. The recurrence
     * maps IM to 0, and maps 0 back to 0, so those two values must not be
     * allowed as initial values of seed.
     *
     * In order to avoid potential problems with integer overflow, the
     * recurrence is implemented in terms of additional constants IQ and IR
     * such that
     *		IM = IA*IQ + IR
     * None of the operations in the implementation overflows a 32-bit signed
     * integer, and the C type long is guaranteed to be at least 32 bits wide.
     *
     * For more details on how this algorithm works, refer to the following
     * papers:
     *
     *	S.K. Park & K.W. Miller, "Random number generators: good ones are hard
     *	to find," Comm ACM 31(10):1192-1201, Oct 1988
     *
     *	W.H. Press & S.A. Teukolsky, "Portable random number generators,"
     *	Computers in Physics 6(5):522-524, Sep/Oct 1992.
     */

#define RAND_IA		16807
#define RAND_IM		2147483647
#define RAND_IQ		127773
#define RAND_IR		2836
#define RAND_MASK	123459876

    tmp = iPtr->randSeed/RAND_IQ;
    iPtr->randSeed = RAND_IA*(iPtr->randSeed - tmp*RAND_IQ) - RAND_IR*tmp;
    if (iPtr->randSeed < 0) {
	iPtr->randSeed += RAND_IM;
    }

    /*
     * Since the recurrence keeps seed values in the range [1, RAND_IM - 1],
     * dividing by RAND_IM yields a double in the range (0, 1).
     */

    dResult = iPtr->randSeed * (1.0/RAND_IM);

    /*
     * Push a Tcl object with the result.
     */

    TclNewDoubleObj(oResult, dResult);
    Tcl_SetObjResult(interp, oResult);
    return TCL_OK;
}

static int
ExprRoundFunc(
    ClientData clientData,	/* Ignored. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Parameter vector. */
{
    double d;
    ClientData ptr;
    int type;

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }

    if (TclGetNumberFromObj(interp, objv[1], &ptr, &type) != TCL_OK) {
	return TCL_ERROR;
    }

    if (type == TCL_NUMBER_DOUBLE) {
	double fractPart, intPart;
	long max = LONG_MAX, min = LONG_MIN;

	fractPart = modf(*((const double *) ptr), &intPart);
	if (fractPart <= -0.5) {
	    min++;
	} else if (fractPart >= 0.5) {
	    max--;
	}
	if ((intPart >= (double)max) || (intPart <= (double)min)) {
	    mp_int big;

	    if (Tcl_InitBignumFromDouble(interp, intPart, &big) != TCL_OK) {
		/* Infinity */
		return TCL_ERROR;
	    }
	    if (fractPart <= -0.5) {
		mp_sub_d(&big, 1, &big);
	    } else if (fractPart >= 0.5) {
		mp_add_d(&big, 1, &big);
	    }
	    Tcl_SetObjResult(interp, Tcl_NewBignumObj(&big));
	    return TCL_OK;
	} else {
	    long result = (long)intPart;

	    if (fractPart <= -0.5) {
		result--;
	    } else if (fractPart >= 0.5) {
		result++;
	    }
	    Tcl_SetObjResult(interp, Tcl_NewLongObj(result));
	    return TCL_OK;
	}
    }

    if (type != TCL_NUMBER_NAN) {
	/*
	 * All integers are already rounded
	 */

	Tcl_SetObjResult(interp, objv[1]);
	return TCL_OK;
    }

    /*
     * Get the error message for NaN.
     */

    Tcl_GetDoubleFromObj(interp, objv[1], &d);
    return TCL_ERROR;
}

static int
ExprSrandFunc(
    ClientData clientData,	/* Ignored. */
    Tcl_Interp *interp,		/* The interpreter in which to execute the
				 * function. */
    int objc,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Parameter vector. */
{
    Interp *iPtr = (Interp *) interp;
    long i = 0;			/* Initialized to avoid compiler warning. */

    /*
     * Convert argument and use it to reset the seed.
     */

    if (objc != 2) {
	MathFuncWrongNumArgs(interp, 2, objc, objv);
	return TCL_ERROR;
    }

    if (TclGetLongFromObj(NULL, objv[1], &i) != TCL_OK) {
	Tcl_Obj *objPtr;
	mp_int big;

	if (Tcl_GetBignumFromObj(interp, objv[1], &big) != TCL_OK) {
	    /* TODO: more ::errorInfo here? or in caller? */
	    return TCL_ERROR;
	}

	mp_mod_2d(&big, (int) CHAR_BIT * sizeof(long), &big);
	objPtr = Tcl_NewBignumObj(&big);
	Tcl_IncrRefCount(objPtr);
	TclGetLongFromObj(NULL, objPtr, &i);
	Tcl_DecrRefCount(objPtr);
    }

    /*
     * Reset the seed. Make sure 1 <= randSeed <= 2^31 - 2. See comments in
     * ExprRandFunc for more details.
     */

    iPtr->flags |= RAND_SEED_INITIALIZED;
    iPtr->randSeed = i;
    iPtr->randSeed &= (unsigned long) 0x7fffffff;
    if ((iPtr->randSeed == 0) || (iPtr->randSeed == 0x7fffffff)) {
	iPtr->randSeed ^= 123459876;
    }

    /*
     * To avoid duplicating the random number generation code we simply clean
     * up our state and call the real random number function. That function
     * will always succeed.
     */

    return ExprRandFunc(clientData, interp, 1, objv);
}

/*
 *----------------------------------------------------------------------
 *
 * MathFuncWrongNumArgs --
 *
 *	Generate an error message when a math function presents the wrong
 *	number of arguments.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An error message is stored in the interpreter result.
 *
 *----------------------------------------------------------------------
 */

static void
MathFuncWrongNumArgs(
    Tcl_Interp *interp,		/* Tcl interpreter */
    int expected,		/* Formal parameter count. */
    int found,			/* Actual parameter count. */
    Tcl_Obj *const *objv)	/* Actual parameter vector. */
{
    const char *name = Tcl_GetString(objv[0]);
    const char *tail = name + strlen(name);

    while (tail > name+1) {
	tail--;
	if (*tail == ':' && tail[-1] == ':') {
	    name = tail+1;
	    break;
	}
    }
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "too %s arguments for math function \"%s\"",
	    (found < expected ? "few" : "many"), name));
    Tcl_SetErrorCode(interp, "TCL", "WRONGARGS", NULL);
}

#ifdef USE_DTRACE
/*
 *----------------------------------------------------------------------
 *
 * DTraceObjCmd --
 *
 *	This function is invoked to process the "::tcl::dtrace" Tcl command.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	The 'tcl-probe' DTrace probe is triggered (if it is enabled).
 *
 *----------------------------------------------------------------------
 */

static int
DTraceObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    if (TCL_DTRACE_TCL_PROBE_ENABLED()) {
	char *a[10];
	int i = 0;

	while (i++ < 10) {
	    a[i-1] = i < objc ? TclGetString(objv[i]) : NULL;
	}
	TCL_DTRACE_TCL_PROBE(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
		a[8], a[9]);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclDTraceInfo --
 *
 *	Extract information from a TIP280 dict for use by DTrace probes.
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
TclDTraceInfo(
    Tcl_Obj *info,
    const char **args,
    int *argsi)
{
    static Tcl_Obj *keys[10] = { NULL };
    Tcl_Obj **k = keys, *val;
    int i = 0;

    if (!*k) {
#define kini(s) TclNewLiteralStringObj(keys[i], s); i++
	kini("cmd");	kini("type");	kini("proc");	kini("file");
	kini("method");	kini("class");	kini("lambda");	kini("object");
	kini("line");	kini("level");
#undef kini
    }
    for (i = 0; i < 6; i++) {
	Tcl_DictObjGet(NULL, info, *k++, &val);
	args[i] = val ? TclGetString(val) : NULL;
    }
    /* no "proc" -> use "lambda" */
    if (!args[2]) {
	Tcl_DictObjGet(NULL, info, *k, &val);
	args[2] = val ? TclGetString(val) : NULL;
    }
    k++;
    /* no "class" -> use "object" */
    if (!args[5]) {
	Tcl_DictObjGet(NULL, info, *k, &val);
	args[5] = val ? TclGetString(val) : NULL;
    }
    k++;
    for (i = 0; i < 2; i++) {
	Tcl_DictObjGet(NULL, info, *k++, &val);
	if (val) {
	    TclGetIntFromObj(NULL, val, &argsi[i]);
	} else {
	    argsi[i] = 0;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DTraceCmdReturn --
 *
 *	NR callback for DTrace command return probes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
DTraceCmdReturn(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    char *cmdName = TclGetString((Tcl_Obj *) data[0]);

    if (TCL_DTRACE_CMD_RETURN_ENABLED()) {
	TCL_DTRACE_CMD_RETURN(cmdName, result);
    }
    if (TCL_DTRACE_CMD_RESULT_ENABLED()) {
	Tcl_Obj *r = Tcl_GetObjResult(interp);

	TCL_DTRACE_CMD_RESULT(cmdName, result, TclGetString(r), r);
    }
    return result;
}

TCL_DTRACE_DEBUG_LOG()

#endif /* USE_DTRACE */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_NRCallObjProc --
 *
 *	This function calls an objProc directly while managing things properly
 *	if it happens to be an NR objProc. It is meant to be used by extenders
 *	that provide an NR implementation of a command, as this function
 *	permits a trivial coding of the non-NR objProc.
 *
 * Results:
 *	The return value is a standard Tcl completion code such as TCL_OK or
 *	TCL_ERROR. A result or error message is left in interp's result.
 *
 * Side effects:
 *	Depends on the objProc.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_NRCallObjProc(
    Tcl_Interp *interp,
    Tcl_ObjCmdProc *objProc,
    ClientData clientData,
    int objc,
    Tcl_Obj *const objv[])
{
    NRE_callback *rootPtr = TOP_CB(interp);

    TclNRAddCallback(interp, Dispatch, objProc, clientData,
	    INT2PTR(objc), objv);
    return TclNRRunCallbacks(interp, TCL_OK, rootPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_NRCreateCommand --
 *
 *	Define a new NRE-enabled object-based command in a command table.
 *
 * Results:
 *	The return value is a token for the command, which can be used in
 *	future calls to Tcl_GetCommandName.
 *
 * Side effects:
 *	If no command named "cmdName" already exists for interp, one is
 *	created. Otherwise, if a command does exist, then if the object-based
 *	Tcl_ObjCmdProc is TclInvokeStringCommand, we assume Tcl_CreateCommand
 *	was called previously for the same command and just set its
 *	Tcl_ObjCmdProc to the argument "proc"; otherwise, we delete the old
 *	command.
 *
 *	In the future, during bytecode evaluation when "cmdName" is seen as
 *	the name of a command by Tcl_EvalObj or Tcl_Eval, the object-based
 *	Tcl_ObjCmdProc proc will be called. When the command is deleted from
 *	the table, deleteProc will be called. See the manual entry for details
 *	on the calling sequence.
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
Tcl_NRCreateCommand(
    Tcl_Interp *interp,		/* Token for command interpreter (returned by
				 * previous call to Tcl_CreateInterp). */
    const char *cmdName,	/* Name of command. If it contains namespace
				 * qualifiers, the new command is put in the
				 * specified namespace; otherwise it is put in
				 * the global namespace. */
    Tcl_ObjCmdProc *proc,	/* Object-based function to associate with
				 * name, provides direct access for direct
				 * calls. */
    Tcl_ObjCmdProc *nreProc,	/* Object-based function to associate with
				 * name, provides NR implementation */
    ClientData clientData,	/* Arbitrary value to pass to object
				 * function. */
    Tcl_CmdDeleteProc *deleteProc)
				/* If not NULL, gives a function to call when
				 * this command is deleted. */
{
    Command *cmdPtr = (Command *)
	    Tcl_CreateObjCommand(interp,cmdName,proc,clientData,deleteProc);

    cmdPtr->nreProc = nreProc;
    return (Tcl_Command) cmdPtr;
}

Tcl_Command
TclNRCreateCommandInNs (
    Tcl_Interp *interp,
    const char *cmdName,
    Tcl_Namespace *nsPtr,
    Tcl_ObjCmdProc *proc,
    Tcl_ObjCmdProc *nreProc,
    ClientData clientData,
    Tcl_CmdDeleteProc *deleteProc) {
    Command *cmdPtr = (Command *)
	TclCreateObjCommandInNs(interp,cmdName,nsPtr,proc,clientData,deleteProc);

    cmdPtr->nreProc = nreProc;
    return (Tcl_Command) cmdPtr;
}

/****************************************************************************
 * Stuff for the public api
 ****************************************************************************/

int
Tcl_NREvalObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    int flags)
{
    return TclNREvalObjEx(interp, objPtr, flags, NULL, INT_MIN);
}

int
Tcl_NREvalObjv(
    Tcl_Interp *interp,		/* Interpreter in which to evaluate the
				 * command. Also used for error reporting. */
    int objc,			/* Number of words in command. */
    Tcl_Obj *const objv[],	/* An array of pointers to objects that are
				 * the words that make up the command. */
    int flags)			/* Collection of OR-ed bits that control the
				 * evaluation of the script. Only
				 * TCL_EVAL_GLOBAL, TCL_EVAL_INVOKE and
				 * TCL_EVAL_NOERR are currently supported. */
{
    return TclNREvalObjv(interp, objc, objv, flags, NULL);
}

int
Tcl_NRCmdSwap(
    Tcl_Interp *interp,
    Tcl_Command cmd,
    int objc,
    Tcl_Obj *const objv[],
    int flags)
{
    return TclNREvalObjv(interp, objc, objv, flags|TCL_EVAL_NOERR,
	    (Command *) cmd);
}

/*****************************************************************************
 * Tailcall related code
 *****************************************************************************
 *
 * The steps of the tailcall dance are as follows:
 *
 *   1. when [tailcall] is invoked, it stores the corresponding callback in
 *      the current CallFrame and returns TCL_RETURN
 *   2. when the CallFrame is popped, it calls TclSetTailcall to store the
 *      callback in the proper NRCommand callback - the spot where the command
 *      that pushed the CallFrame is completely cleaned up
 *   3. when the NRCommand callback runs, it schedules the tailcall callback
 *      to run immediately after it returns
 *
 *   One delicate point is to properly define the NRCommand where the tailcall
 *   will execute. There are functions whose purpose is to help define the
 *   precise spot:
 *     TclMarkTailcall: if the NEXT command to be pushed tailcalls, execution
 *         should continue right here
 *     TclSkipTailcall:  if the NEXT command to be pushed tailcalls, execution
 *         should continue after the CURRENT command is fully returned ("skip
 *         the next command: we are redirecting to it, tailcalls should run
 *         after WE return")
 *     TclPushTailcallPoint: the search for a tailcalling spot cannot traverse
 *         this point. This is special for OO, as some of the oo constructs
 *         that behave like commands may not push an NRCommand callback.
 */

void
TclMarkTailcall(
    Tcl_Interp *interp)
{
    Interp *iPtr = (Interp *) interp;

    if (iPtr->deferredCallbacks == NULL) {
	TclNRAddCallback(interp, NRCommand, NULL, NULL,
                NULL, NULL);
        iPtr->deferredCallbacks = TOP_CB(interp);
    }
}

void
TclSkipTailcall(
    Tcl_Interp *interp)
{
    Interp *iPtr = (Interp *) interp;

    TclMarkTailcall(interp);
    iPtr->deferredCallbacks->data[1] = INT2PTR(1);
}

void
TclPushTailcallPoint(
    Tcl_Interp *interp)
{
    TclNRAddCallback(interp, NRCommand, NULL, NULL, NULL, NULL);
    ((Interp *) interp)->numLevels++;
}


/*
 *----------------------------------------------------------------------
 *
 * TclSetTailcall --
 *
 *	Splice a tailcall command in the proper spot of the NRE callback
 *	stack, so that it runs at the right time.
 *
 *----------------------------------------------------------------------
 */

void
TclSetTailcall(
    Tcl_Interp *interp,
    Tcl_Obj *listPtr)
{
    /*
     * Find the splicing spot: right before the NRCommand of the thing
     * being tailcalled. Note that we skip NRCommands marked by a 1 in data[1]
     * (used by command redirectors).
     */

    NRE_callback *runPtr;

    for (runPtr = TOP_CB(interp); runPtr; runPtr = runPtr->nextPtr) {
        if (((runPtr->procPtr) == NRCommand) && !runPtr->data[1]) {
            break;
        }
    }
    if (!runPtr) {
        Tcl_Panic("tailcall cannot find the right splicing spot: should not happen!");
    }
    runPtr->data[1] = listPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * TclNRTailcallObjCmd --
 *
 *	Prepare the tailcall as a list and store it in the current
 *	varFrame. When the frame is later popped the tailcall will be spliced
 *	at the proper place.
 *
 * Results:
 *	The first NRCommand callback that is not marked to be skipped is
 *	updated so that its data[1] field contains the tailcall list.
 *
 *----------------------------------------------------------------------
 */

int
TclNRTailcallObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Interp *iPtr = (Interp *) interp;

    if (objc < 1) {
	Tcl_WrongNumArgs(interp, 1, objv, "?command? ?arg ...?");
	return TCL_ERROR;
    }

    if (!(iPtr->varFramePtr->isProcCallFrame & 1)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "tailcall can only be called from a proc, lambda or method", -1));
        Tcl_SetErrorCode(interp, "TCL", "TAILCALL", "ILLEGAL", NULL);
	return TCL_ERROR;
    }

    /*
     * Invocation without args just clears a scheduled tailcall; invocation
     * with an argument replaces any previously scheduled tailcall.
     */

    if (iPtr->varFramePtr->tailcallPtr) {
        Tcl_DecrRefCount(iPtr->varFramePtr->tailcallPtr);
        iPtr->varFramePtr->tailcallPtr = NULL;
    }

    /*
     * Create the callback to actually evaluate the tailcalled
     * command, then set it in the varFrame so that PopCallFrame can use it
     * at the proper time.
     */

    if (objc > 1) {
        Tcl_Obj *listPtr, *nsObjPtr;
        Tcl_Namespace *nsPtr = (Tcl_Namespace *) iPtr->varFramePtr->nsPtr;

        /* The tailcall data is in a Tcl list: the first element is the
         * namespace, the rest the command to be tailcalled. */

        nsObjPtr = Tcl_NewStringObj(nsPtr->fullName, -1);
        listPtr = Tcl_NewListObj(objc, objv);
 	TclListObjSetElement(interp, listPtr, 0, nsObjPtr);

        iPtr->varFramePtr->tailcallPtr = listPtr;
    }
    return TCL_RETURN;
}


/*
 *----------------------------------------------------------------------
 *
 * TclNRTailcallEval --
 *
 *	This NREcallback actually causes the tailcall to be evaluated.
 *
 *----------------------------------------------------------------------
 */

int
TclNRTailcallEval(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *listPtr = data[0], *nsObjPtr;
    Tcl_Namespace *nsPtr;
    int objc;
    Tcl_Obj **objv;

    Tcl_ListObjGetElements(interp, listPtr, &objc, &objv);
    nsObjPtr = objv[0];

    if (result == TCL_OK) {
	result = TclGetNamespaceFromObj(interp, nsObjPtr, &nsPtr);
    }

    if (result != TCL_OK) {
        /*
         * Tailcall execution was preempted, eg by an intervening catch or by
         * a now-gone namespace: cleanup and return.
         */

	Tcl_DecrRefCount(listPtr);
        return result;
    }

    /*
     * Perform the tailcall
     */

    TclMarkTailcall(interp);
    TclNRAddCallback(interp, TclNRReleaseValues, listPtr, NULL, NULL,NULL);
    iPtr->lookupNsPtr = (Namespace *) nsPtr;
    return TclNREvalObjv(interp, objc-1, objv+1, 0, NULL);
}

int
TclNRReleaseValues(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    int i = 0;
    while (i < 4) {
	if (data[i]) {
	    Tcl_DecrRefCount((Tcl_Obj *) data[i]);
	} else {
	    break;
	}
	i++;
    }
    return result;
}


void
Tcl_NRAddCallback(
    Tcl_Interp *interp,
    Tcl_NRPostProc *postProcPtr,
    ClientData data0,
    ClientData data1,
    ClientData data2,
    ClientData data3)
{
    if (!(postProcPtr)) {
	Tcl_Panic("Adding a callback without an objProc?!");
    }
    TclNRAddCallback(interp, postProcPtr, data0, data1, data2, data3);
}

/*
 *----------------------------------------------------------------------
 *
 * TclNRCoroutineObjCmd -- (and friends)
 *
 *	This object-based function is invoked to process the "coroutine" Tcl
 *	command. It is heavily based on "apply".
 *
 * Results:
 *	A standard Tcl object result value.
 *
 * Side effects:
 *	A new procedure gets created.
 *
 * ** FIRST EXPERIMENTAL IMPLEMENTATION **
 *
 * It is fairly amateurish and not up to our standards - mainly in terms of
 * error messages and [info] interaction. Just to test the infrastructure in
 * teov and tebc.
 *----------------------------------------------------------------------
 */

#define iPtr ((Interp *) interp)

int
TclNRYieldObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    CoroutineData *corPtr = iPtr->execEnvPtr->corPtr;

    if (objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?returnValue?");
	return TCL_ERROR;
    }

    if (!corPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "yield can only be called in a coroutine", -1));
	Tcl_SetErrorCode(interp, "TCL", "COROUTINE", "ILLEGAL_YIELD", NULL);
	return TCL_ERROR;
    }

    if (objc == 2) {
	Tcl_SetObjResult(interp, objv[1]);
    }

    NRE_ASSERT(!COR_IS_SUSPENDED(corPtr));
    TclNRAddCallback(interp, TclNRCoroutineActivateCallback, corPtr,
            clientData, NULL, NULL);
    return TCL_OK;
}

int
TclNRYieldToObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    CoroutineData *corPtr = iPtr->execEnvPtr->corPtr;
    Tcl_Obj *listPtr, *nsObjPtr;
    Tcl_Namespace *nsPtr = TclGetCurrentNamespace(interp);

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "command ?arg ...?");
	return TCL_ERROR;
    }

    if (!corPtr) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "yieldto can only be called in a coroutine", -1));
	Tcl_SetErrorCode(interp, "TCL", "COROUTINE", "ILLEGAL_YIELD", NULL);
	return TCL_ERROR;
    }

    if (((Namespace *) nsPtr)->flags & NS_DYING) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"yieldto called in deleted namespace", -1));
        Tcl_SetErrorCode(interp, "TCL", "COROUTINE", "YIELDTO_IN_DELETED",
		NULL);
        return TCL_ERROR;
    }

    /*
     * Add the tailcall in the caller env, then just yield.
     *
     * This is essentially code from TclNRTailcallObjCmd
     */

    listPtr = Tcl_NewListObj(objc, objv);
    nsObjPtr = Tcl_NewStringObj(nsPtr->fullName, -1);
    TclListObjSetElement(interp, listPtr, 0, nsObjPtr);

    /*
     * Add the callback in the caller's env, then instruct TEBC to yield.
     */

    iPtr->execEnvPtr = corPtr->callerEEPtr;
    TclSetTailcall(interp, listPtr);
    iPtr->execEnvPtr = corPtr->eePtr;

    return TclNRYieldObjCmd(INT2PTR(CORO_ACTIVATE_YIELDM), interp, 1, objv);
}

static int
RewindCoroutineCallback(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    return Tcl_RestoreInterpState(interp, data[0]);
}

static int
RewindCoroutine(
    CoroutineData *corPtr,
    int result)
{
    Tcl_Interp *interp = corPtr->eePtr->interp;
    Tcl_InterpState state = Tcl_SaveInterpState(interp, result);

    NRE_ASSERT(COR_IS_SUSPENDED(corPtr));
    NRE_ASSERT(corPtr->eePtr != NULL);
    NRE_ASSERT(corPtr->eePtr != iPtr->execEnvPtr);

    corPtr->eePtr->rewind = 1;
    TclNRAddCallback(interp, RewindCoroutineCallback, state,
	    NULL, NULL, NULL);
    return TclNRInterpCoroutine(corPtr, interp, 0, NULL);
}

static void
DeleteCoroutine(
    ClientData clientData)
{
    CoroutineData *corPtr = clientData;
    Tcl_Interp *interp = corPtr->eePtr->interp;
    NRE_callback *rootPtr = TOP_CB(interp);

    if (COR_IS_SUSPENDED(corPtr)) {
	TclNRRunCallbacks(interp, RewindCoroutine(corPtr,TCL_OK), rootPtr);
    }
}

static int
NRCoroutineCallerCallback(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    CoroutineData *corPtr = data[0];
    Command *cmdPtr = corPtr->cmdPtr;

    /*
     * This is the last callback in the caller execEnv, right before switching
     * to the coroutine's
     */

    NRE_ASSERT(iPtr->execEnvPtr == corPtr->callerEEPtr);

    if (!corPtr->eePtr) {
	/*
	 * The execEnv was wound down but not deleted for our sake. We finish
	 * the job here. The caller context has already been restored.
	 */

	NRE_ASSERT(iPtr->varFramePtr == corPtr->caller.varFramePtr);
	NRE_ASSERT(iPtr->framePtr == corPtr->caller.framePtr);
	NRE_ASSERT(iPtr->cmdFramePtr == corPtr->caller.cmdFramePtr);
	ckfree(corPtr);
	return result;
    }

    NRE_ASSERT(COR_IS_SUSPENDED(corPtr));
    SAVE_CONTEXT(corPtr->running);
    RESTORE_CONTEXT(corPtr->caller);

    if (cmdPtr->flags & CMD_IS_DELETED) {
	/*
	 * The command was deleted while it was running: wind down the
	 * execEnv, this will do the complete cleanup. RewindCoroutine will
	 * restore both the caller's context and interp state.
	 */

	return RewindCoroutine(corPtr, result);
    }

    return result;
}

static int
NRCoroutineExitCallback(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    CoroutineData *corPtr = data[0];
    Command *cmdPtr = corPtr->cmdPtr;

    /*
     * This runs at the bottom of the Coroutine's execEnv: it will be executed
     * when the coroutine returns or is wound down, but not when it yields. It
     * deletes the coroutine and restores the caller's environment.
     */

    NRE_ASSERT(interp == corPtr->eePtr->interp);
    NRE_ASSERT(TOP_CB(interp) == NULL);
    NRE_ASSERT(iPtr->execEnvPtr == corPtr->eePtr);
    NRE_ASSERT(!COR_IS_SUSPENDED(corPtr));
    NRE_ASSERT((corPtr->callerEEPtr->callbackPtr->procPtr == NRCoroutineCallerCallback));

    cmdPtr->deleteProc = NULL;
    Tcl_DeleteCommandFromToken(interp, (Tcl_Command) cmdPtr);
    TclCleanupCommandMacro(cmdPtr);

    corPtr->eePtr->corPtr = NULL;
    TclDeleteExecEnv(corPtr->eePtr);
    corPtr->eePtr = NULL;

    corPtr->stackLevel = NULL;

    /*
     * #280.
     * Drop the coroutine-owned copy of the lineLABCPtr hashtable for literal
     * command arguments in bytecode.
     */

    Tcl_DeleteHashTable(corPtr->lineLABCPtr);
    ckfree(corPtr->lineLABCPtr);
    corPtr->lineLABCPtr = NULL;

    RESTORE_CONTEXT(corPtr->caller);
    iPtr->execEnvPtr = corPtr->callerEEPtr;
    iPtr->numLevels++;

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclNRCoroutineActivateCallback --
 *
 *      This is the workhorse for coroutines: it implements both yield and
 *      resume.
 *
 *      It is important that both be implemented in the same callback: the
 *      detection of the impossibility to suspend due to a busy C-stack relies
 *      on the precise position of a local variable in the stack. We do not
 *      want the compiler to play tricks on us, either by moving things around
 *      or inlining.
 *
 *----------------------------------------------------------------------
 */

int
TclNRCoroutineActivateCallback(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    CoroutineData *corPtr = data[0];
    int type = PTR2INT(data[1]);
    int numLevels, unused;
    int *stackLevel = &unused;

    if (!corPtr->stackLevel) {
        /*
         * -- Coroutine is suspended --
         * Push the callback to restore the caller's context on yield or
         * return.
         */

        TclNRAddCallback(interp, NRCoroutineCallerCallback, corPtr,
                NULL, NULL, NULL);

        /*
         * Record the stackLevel at which the resume is happening, then swap
         * the interp's environment to make it suitable to run this coroutine.
         */

        corPtr->stackLevel = stackLevel;
        numLevels = corPtr->auxNumLevels;
        corPtr->auxNumLevels = iPtr->numLevels;

        SAVE_CONTEXT(corPtr->caller);
        corPtr->callerEEPtr = iPtr->execEnvPtr;
        RESTORE_CONTEXT(corPtr->running);
        iPtr->execEnvPtr = corPtr->eePtr;
        iPtr->numLevels += numLevels;
    } else {
        /*
         * Coroutine is active: yield
         */

        if (corPtr->stackLevel != stackLevel) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(
                    "cannot yield: C stack busy", -1));
            Tcl_SetErrorCode(interp, "TCL", "COROUTINE", "CANT_YIELD",
                    NULL);
            return TCL_ERROR;
        }

        if (type == CORO_ACTIVATE_YIELD) {
            corPtr->nargs = COROUTINE_ARGUMENTS_SINGLE_OPTIONAL;
        } else if (type == CORO_ACTIVATE_YIELDM) {
            corPtr->nargs = COROUTINE_ARGUMENTS_ARBITRARY;
        } else {
            Tcl_Panic("Yield received an option which is not implemented");
        }

        corPtr->stackLevel = NULL;

        numLevels = iPtr->numLevels;
        iPtr->numLevels = corPtr->auxNumLevels;
        corPtr->auxNumLevels = numLevels - corPtr->auxNumLevels;

        iPtr->execEnvPtr = corPtr->callerEEPtr;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclNREvalList --
 *
 *      Callback to invoke command as list, used in order to delayed
 *	processing of canonical list command in sane environment.
 *
 *----------------------------------------------------------------------
 */

static int
TclNREvalList(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    int objc;
    Tcl_Obj **objv;
    Tcl_Obj *listPtr = data[0];

    Tcl_IncrRefCount(listPtr);

    TclMarkTailcall(interp);
    TclNRAddCallback(interp, TclNRReleaseValues, listPtr, NULL, NULL,NULL);
    TclListObjGetElements(NULL, listPtr, &objc, &objv);
    return TclNREvalObjv(interp, objc, objv, 0, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * NRCoroInjectObjCmd --
 *
 *      Implementation of [::tcl::unsupported::inject] command.
 *
 *----------------------------------------------------------------------
 */

static int
NRCoroInjectObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Command *cmdPtr;
    CoroutineData *corPtr;
    ExecEnv *savedEEPtr = iPtr->execEnvPtr;

    /*
     * Usage more or less like tailcall:
     *   inject coroName cmd ?arg1 arg2 ...?
     */

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "coroName cmd ?arg1 arg2 ...?");
	return TCL_ERROR;
    }

    cmdPtr = (Command *) Tcl_GetCommandFromObj(interp, objv[1]);
    if ((!cmdPtr) || (cmdPtr->nreProc != TclNRInterpCoroutine)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "can only inject a command into a coroutine", -1));
        Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "COROUTINE",
                TclGetString(objv[1]), NULL);
        return TCL_ERROR;
    }

    corPtr = cmdPtr->objClientData;
    if (!COR_IS_SUSPENDED(corPtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "can only inject a command into a suspended coroutine", -1));
        Tcl_SetErrorCode(interp, "TCL", "COROUTINE", "ACTIVE", NULL);
        return TCL_ERROR;
    }

    /*
     * Add the callback to the coro's execEnv, so that it is the first thing
     * to happen when the coro is resumed.
     */

    iPtr->execEnvPtr = corPtr->eePtr;
    TclNRAddCallback(interp, TclNREvalList, Tcl_NewListObj(objc-2, objv+2),
	NULL, NULL, NULL);
    iPtr->execEnvPtr = savedEEPtr;

    return TCL_OK;
}

int
TclNRInterpCoroutine(
    ClientData clientData,
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    CoroutineData *corPtr = clientData;

    if (!COR_IS_SUSPENDED(corPtr)) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "coroutine \"%s\" is already running",
                Tcl_GetString(objv[0])));
	Tcl_SetErrorCode(interp, "TCL", "COROUTINE", "BUSY", NULL);
	return TCL_ERROR;
    }

    /*
     * Parse all the arguments to work out what to feed as the result of the
     * [yield]. TRICKY POINT: objc==0 happens here! It occurs when a coroutine
     * is deleted!
     */

    switch (corPtr->nargs) {
    case COROUTINE_ARGUMENTS_SINGLE_OPTIONAL:
        if (objc == 2) {
            Tcl_SetObjResult(interp, objv[1]);
        } else if (objc > 2) {
            Tcl_WrongNumArgs(interp, 1, objv, "?arg?");
            return TCL_ERROR;
        }
        break;
    default:
        if (corPtr->nargs != objc-1) {
            Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong coro nargs; how did we get here? "
                    "not implemented!", -1));
            Tcl_SetErrorCode(interp, "TCL", "WRONGARGS", NULL);
            return TCL_ERROR;
        }
        /* fallthrough */
    case COROUTINE_ARGUMENTS_ARBITRARY:
        if (objc > 1) {
            Tcl_SetObjResult(interp, Tcl_NewListObj(objc-1, objv+1));
        }
        break;
    }

    TclNRAddCallback(interp, TclNRCoroutineActivateCallback, corPtr,
            NULL, NULL, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclNRCoroutineObjCmd --
 *
 *      Implementation of [coroutine] command; see documentation for
 *      description of what this does.
 *
 *----------------------------------------------------------------------
 */

int
TclNRCoroutineObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Command *cmdPtr;
    CoroutineData *corPtr;
    const char *procName, *simpleName;
    Namespace *nsPtr, *altNsPtr, *cxtNsPtr,
	*inNsPtr = (Namespace *)TclGetCurrentNamespace(interp);
    Namespace *lookupNsPtr = iPtr->varFramePtr->nsPtr;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "name cmd ?arg ...?");
	return TCL_ERROR;
    }

    procName = TclGetString(objv[1]);
    TclGetNamespaceForQualName(interp, procName, inNsPtr, 0,
	    &nsPtr, &altNsPtr, &cxtNsPtr, &simpleName);

    if (nsPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "can't create procedure \"%s\": unknown namespace",
                procName));
        Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "NAMESPACE", NULL);
	return TCL_ERROR;
    }
    if (simpleName == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "can't create procedure \"%s\": bad procedure name",
                procName));
        Tcl_SetErrorCode(interp, "TCL", "VALUE", "COMMAND", procName, NULL);
	return TCL_ERROR;
    }

    /*
     * We ARE creating the coroutine command: allocate the corresponding
     * struct and create the corresponding command.
     */

    corPtr = ckalloc(sizeof(CoroutineData));

    cmdPtr = (Command *) TclNRCreateCommandInNs(interp, simpleName,
	    (Tcl_Namespace *)nsPtr, /*objProc*/ NULL, TclNRInterpCoroutine,
	    corPtr, DeleteCoroutine);

    corPtr->cmdPtr = cmdPtr;
    cmdPtr->refCount++;

    /*
     * #280.
     * Provide the new coroutine with its own copy of the lineLABCPtr
     * hashtable for literal command arguments in bytecode. Note that that
     * CFWordBC chains are not duplicated, only the entrypoints to them. This
     * means that in the presence of coroutines each chain is potentially a
     * tree. Like the chain -> tree conversion of the CmdFrame stack.
     */

    {
	Tcl_HashSearch hSearch;
	Tcl_HashEntry *hePtr;

	corPtr->lineLABCPtr = ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(corPtr->lineLABCPtr, TCL_ONE_WORD_KEYS);

	for (hePtr = Tcl_FirstHashEntry(iPtr->lineLABCPtr,&hSearch);
		hePtr; hePtr = Tcl_NextHashEntry(&hSearch)) {
	    int isNew;
	    Tcl_HashEntry *newPtr =
		    Tcl_CreateHashEntry(corPtr->lineLABCPtr,
		    Tcl_GetHashKey(iPtr->lineLABCPtr, hePtr),
		    &isNew);

	    Tcl_SetHashValue(newPtr, Tcl_GetHashValue(hePtr));
	}
    }

    /*
     * Create the base context.
     */

    corPtr->running.framePtr = iPtr->rootFramePtr;
    corPtr->running.varFramePtr = iPtr->rootFramePtr;
    corPtr->running.cmdFramePtr = NULL;
    corPtr->running.lineLABCPtr = corPtr->lineLABCPtr;
    corPtr->stackLevel = NULL;
    corPtr->auxNumLevels = 0;

    /*
     * Create the coro's execEnv, switch to it to push the exit and coro
     * command callbacks, then switch back.
     */

    corPtr->eePtr = TclCreateExecEnv(interp, CORO_STACK_INITIAL_SIZE);
    corPtr->callerEEPtr = iPtr->execEnvPtr;
    corPtr->eePtr->corPtr = corPtr;

    SAVE_CONTEXT(corPtr->caller);
    corPtr->callerEEPtr = iPtr->execEnvPtr;
    RESTORE_CONTEXT(corPtr->running);
    iPtr->execEnvPtr = corPtr->eePtr;

    TclNRAddCallback(interp, NRCoroutineExitCallback, corPtr,
	    NULL, NULL, NULL);

    /* ensure that the command is looked up in the correct namespace */
    iPtr->lookupNsPtr = lookupNsPtr;
    Tcl_NREvalObj(interp, Tcl_NewListObj(objc-2, objv+2), 0);
    iPtr->numLevels--;

    SAVE_CONTEXT(corPtr->running);
    RESTORE_CONTEXT(corPtr->caller);
    iPtr->execEnvPtr = corPtr->callerEEPtr;

    /*
     * Now just resume the coroutine.
     */

    TclNRAddCallback(interp, TclNRCoroutineActivateCallback, corPtr,
            NULL, NULL, NULL);
    return TCL_OK;
}

/*
 * This is used in the [info] ensemble
 */

int
TclInfoCoroutineCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    CoroutineData *corPtr = iPtr->execEnvPtr->corPtr;

    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
	return TCL_ERROR;
    }

    if (corPtr && !(corPtr->cmdPtr->flags & CMD_IS_DELETED)) {
	Tcl_Obj *namePtr;

	TclNewObj(namePtr);
	Tcl_GetCommandFullName(interp, (Tcl_Command) corPtr->cmdPtr, namePtr);
	Tcl_SetObjResult(interp, namePtr);
    }
    return TCL_OK;
}

#undef iPtr

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 */
