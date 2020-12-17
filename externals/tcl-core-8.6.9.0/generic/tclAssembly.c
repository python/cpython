/*
 * tclAssembly.c --
 *
 *	Assembler for Tcl bytecodes.
 *
 * This file contains the procedures that convert Tcl Assembly Language (TAL)
 * to a sequence of bytecode instructions for the Tcl execution engine.
 *
 * Copyright (c) 2010 by Ozgur Dogan Ugurlu.
 * Copyright (c) 2010 by Kevin B. Kenny.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/*-
 *- THINGS TO DO:
 *- More instructions:
 *-   done - alternate exit point (affects stack and exception range checking)
 *-   break and continue - if exception ranges can be sorted out.
 *-   foreach_start4, foreach_step4
 *-   returnImm, returnStk
 *-   expandStart, expandStkTop, invokeExpanded, expandDrop
 *-   dictFirst, dictNext, dictDone
 *-   dictUpdateStart, dictUpdateEnd
 *-   jumpTable testing
 *-   syntax (?)
 *-   returnCodeBranch
 *-   tclooNext, tclooNextClass
 */

#include "tclInt.h"
#include "tclCompile.h"
#include "tclOOInt.h"

/*
 * Structure that represents a range of instructions in the bytecode.
 */

typedef struct CodeRange {
    int startOffset;		/* Start offset in the bytecode array */
    int endOffset;		/* End offset in the bytecode array */
} CodeRange;

/*
 * State identified for a basic block's catch context.
 */

typedef enum BasicBlockCatchState {
    BBCS_UNKNOWN = 0,		/* Catch context has not yet been identified */
    BBCS_NONE,			/* Block is outside of any catch */
    BBCS_INCATCH,		/* Block is within a catch context */
    BBCS_CAUGHT 		/* Block is within a catch context and
				 * may be executed after an exception fires */
} BasicBlockCatchState;

/*
 * Structure that defines a basic block - a linear sequence of bytecode
 * instructions with no jumps in or out (including not changing the
 * state of any exception range).
 */

typedef struct BasicBlock {
    int originalStartOffset;	/* Instruction offset before JUMP1s were
				 * substituted with JUMP4's */
    int startOffset;		/* Instruction offset of the start of the
				 * block */
    int startLine;		/* Line number in the input script of the
				 * instruction at the start of the block */
    int jumpOffset;		/* Bytecode offset of the 'jump' instruction
				 * that ends the block, or -1 if there is no
				 * jump. */
    int jumpLine;		/* Line number in the input script of the
				 * 'jump' instruction that ends the block, or
				 * -1 if there is no jump */
    struct BasicBlock* prevPtr;	/* Immediate predecessor of this block */
    struct BasicBlock* predecessor;
				/* Predecessor of this block in the spanning
				 * tree */
    struct BasicBlock* successor1;
				/* BasicBlock structure of the following
				 * block: NULL at the end of the bytecode
				 * sequence. */
    Tcl_Obj* jumpTarget;	/* Jump target label if the jump target is
				 * unresolved */
    int initialStackDepth;	/* Absolute stack depth on entry */
    int minStackDepth;		/* Low-water relative stack depth */
    int maxStackDepth;		/* High-water relative stack depth */
    int finalStackDepth;	/* Relative stack depth on exit */
    enum BasicBlockCatchState catchState;
				/* State of the block for 'catch' analysis */
    int catchDepth;		/* Number of nested catches in which the basic
				 * block appears */
    struct BasicBlock* enclosingCatch;
				/* BasicBlock structure of the last startCatch
				 * executed on a path to this block, or NULL
				 * if there is no enclosing catch */
    int foreignExceptionBase;	/* Base index of foreign exceptions */
    int foreignExceptionCount;	/* Count of foreign exceptions */
    ExceptionRange* foreignExceptions;
				/* ExceptionRange structures for exception
				 * ranges belonging to embedded scripts and
				 * expressions in this block */
    JumptableInfo* jtPtr;	/* Jump table at the end of this basic block */
    int flags;			/* Boolean flags */
} BasicBlock;

/*
 * Flags that pertain to a basic block.
 */

enum BasicBlockFlags {
    BB_VISITED = (1 << 0),	/* Block has been visited in the current
				 * traversal */
    BB_FALLTHRU = (1 << 1),	/* Control may pass from this block to a
				 * successor */
    BB_JUMP1 = (1 << 2),	/* Basic block ends with a 1-byte-offset jump
				 * and may need expansion */
    BB_JUMPTABLE = (1 << 3),	/* Basic block ends with a jump table */
    BB_BEGINCATCH = (1 << 4),	/* Block ends with a 'beginCatch' instruction,
				 * marking it as the start of a 'catch'
				 * sequence. The 'jumpTarget' is the exception
				 * exit from the catch block. */
    BB_ENDCATCH = (1 << 5)	/* Block ends with an 'endCatch' instruction,
				 * unwinding the catch from the exception
				 * stack. */
};

/*
 * Source instruction type recognized by the assembler.
 */

typedef enum TalInstType {
    ASSEM_1BYTE,		/* Fixed arity, 1-byte instruction */
    ASSEM_BEGIN_CATCH,		/* Begin catch: one 4-byte jump offset to be
				 * converted to appropriate exception
				 * ranges */
    ASSEM_BOOL,			/* One Boolean operand */
    ASSEM_BOOL_LVT4,		/* One Boolean, one 4-byte LVT ref. */
    ASSEM_CLOCK_READ,		/* 1-byte unsigned-integer case number, in the
				 * range 0-3 */
    ASSEM_CONCAT1,		/* 1-byte unsigned-integer operand count, must
				 * be strictly positive, consumes N, produces
				 * 1 */
    ASSEM_DICT_GET,		/* 'dict get' and related - consumes N+1
				 * operands, produces 1, N > 0 */
    ASSEM_DICT_SET,		/* specifies key count and LVT index, consumes
				 * N+1 operands, produces 1, N > 0 */
    ASSEM_DICT_UNSET,		/* specifies key count and LVT index, consumes
				 * N operands, produces 1, N > 0 */
    ASSEM_END_CATCH,		/* End catch. No args. Exception range popped
				 * from stack and stack pointer restored. */
    ASSEM_EVAL,			/* 'eval' - evaluate a constant script (by
				 * compiling it in line with the assembly
				 * code! I love Tcl!) */
    ASSEM_INDEX,		/* 4 byte operand, integer or end-integer */
    ASSEM_INVOKE,		/* 1- or 4-byte operand count, must be
				 * strictly positive, consumes N, produces
				 * 1. */
    ASSEM_JUMP,			/* Jump instructions */
    ASSEM_JUMP4,		/* Jump instructions forcing a 4-byte offset */
    ASSEM_JUMPTABLE,		/* Jumptable (switch -exact) */
    ASSEM_LABEL,		/* The assembly directive that defines a
				 * label */
    ASSEM_LINDEX_MULTI,		/* 4-byte operand count, must be strictly
				 * positive, consumes N, produces 1 */
    ASSEM_LIST,			/* 4-byte operand count, must be nonnegative,
				 * consumses N, produces 1 */
    ASSEM_LSET_FLAT,		/* 4-byte operand count, must be >= 3,
				 * consumes N, produces 1 */
    ASSEM_LVT,			/* One operand that references a local
				 * variable */
    ASSEM_LVT1,			/* One 1-byte operand that references a local
				 * variable */
    ASSEM_LVT1_SINT1,		/* One 1-byte operand that references a local
				 * variable, one signed-integer 1-byte
				 * operand */
    ASSEM_LVT4,			/* One 4-byte operand that references a local
				 * variable */
    ASSEM_OVER,			/* OVER: 4-byte operand count, consumes N+1,
				 * produces N+2 */
    ASSEM_PUSH,			/* one literal operand */
    ASSEM_REGEXP,		/* One Boolean operand, but weird mapping to
				 * call flags */
    ASSEM_REVERSE,		/* REVERSE: 4-byte operand count, consumes N,
				 * produces N */
    ASSEM_SINT1,		/* One 1-byte signed-integer operand
				 * (INCR_STK_IMM) */
    ASSEM_SINT4_LVT4		/* Signed 4-byte integer operand followed by
				 * LVT entry.  Fixed arity */
} TalInstType;

/*
 * Description of an instruction recognized by the assembler.
 */

typedef struct TalInstDesc {
    const char *name;		/* Name of instruction. */
    TalInstType instType;	/* The type of instruction */
    int tclInstCode;		/* Instruction code. For instructions having
				 * 1- and 4-byte variables, tclInstCode is
				 * ((1byte)<<8) || (4byte) */
    int operandsConsumed;	/* Number of operands consumed by the
				 * operation, or INT_MIN if the operation is
				 * variadic */
    int operandsProduced;	/* Number of operands produced by the
				 * operation. If negative, the operation has a
				 * net stack effect of -1-operandsProduced */
} TalInstDesc;

/*
 * Structure that holds the state of the assembler while generating code.
 */

typedef struct AssemblyEnv {
    CompileEnv* envPtr;		/* Compilation environment being used for code
				 * generation */
    Tcl_Parse* parsePtr;	/* Parse of the current line of source */
    Tcl_HashTable labelHash;	/* Hash table whose keys are labels and whose
				 * values are 'label' objects storing the code
				 * offsets of the labels. */
    int cmdLine;		/* Current line number within the assembly
				 * code */
    int* clNext;		/* Invisible continuation line for
				 * [info frame] */
    BasicBlock* head_bb;	/* First basic block in the code */
    BasicBlock* curr_bb;	/* Current basic block */
    int maxDepth;		/* Maximum stack depth encountered */
    int curCatchDepth;		/* Current depth of catches */
    int maxCatchDepth;		/* Maximum depth of catches encountered */
    int flags;			/* Compilation flags (TCL_EVAL_DIRECT) */
} AssemblyEnv;

/*
 * Static functions defined in this file.
 */

static void		AddBasicBlockRangeToErrorInfo(AssemblyEnv*,
			    BasicBlock*);
static BasicBlock *	AllocBB(AssemblyEnv*);
static int		AssembleOneLine(AssemblyEnv* envPtr);
static void		BBAdjustStackDepth(BasicBlock* bbPtr, int consumed,
			    int produced);
static void		BBUpdateStackReqs(BasicBlock* bbPtr, int tblIdx,
			    int count);
static void		BBEmitInstInt1(AssemblyEnv* assemEnvPtr, int tblIdx,
			    int opnd, int count);
static void		BBEmitInstInt4(AssemblyEnv* assemEnvPtr, int tblIdx,
			    int opnd, int count);
static void		BBEmitInst1or4(AssemblyEnv* assemEnvPtr, int tblIdx,
			    int param, int count);
static void		BBEmitOpcode(AssemblyEnv* assemEnvPtr, int tblIdx,
			    int count);
static int		BuildExceptionRanges(AssemblyEnv* assemEnvPtr);
static int		CalculateJumpRelocations(AssemblyEnv*, int*);
static int		CheckForUnclosedCatches(AssemblyEnv*);
static int		CheckForThrowInWrongContext(AssemblyEnv*);
static int		CheckNonThrowingBlock(AssemblyEnv*, BasicBlock*);
static int		BytecodeMightThrow(unsigned char);
static int		CheckJumpTableLabels(AssemblyEnv*, BasicBlock*);
static int		CheckNamespaceQualifiers(Tcl_Interp*, const char*,
			    int);
static int		CheckNonNegative(Tcl_Interp*, int);
static int		CheckOneByte(Tcl_Interp*, int);
static int		CheckSignedOneByte(Tcl_Interp*, int);
static int		CheckStack(AssemblyEnv*);
static int		CheckStrictlyPositive(Tcl_Interp*, int);
static ByteCode *	CompileAssembleObj(Tcl_Interp *interp,
			    Tcl_Obj *objPtr);
static void		CompileEmbeddedScript(AssemblyEnv*, Tcl_Token*,
			    const TalInstDesc*);
static int		DefineLabel(AssemblyEnv* envPtr, const char* label);
static void		DeleteMirrorJumpTable(JumptableInfo* jtPtr);
static void		DupAssembleCodeInternalRep(Tcl_Obj* src,
			    Tcl_Obj* dest);
static void		FillInJumpOffsets(AssemblyEnv*);
static int		CreateMirrorJumpTable(AssemblyEnv* assemEnvPtr,
			    Tcl_Obj* jumpTable);
static int		FindLocalVar(AssemblyEnv* envPtr,
			    Tcl_Token** tokenPtrPtr);
static int		FinishAssembly(AssemblyEnv*);
static void		FreeAssembleCodeInternalRep(Tcl_Obj *objPtr);
static void		FreeAssemblyEnv(AssemblyEnv*);
static int		GetBooleanOperand(AssemblyEnv*, Tcl_Token**, int*);
static int		GetListIndexOperand(AssemblyEnv*, Tcl_Token**, int*);
static int		GetIntegerOperand(AssemblyEnv*, Tcl_Token**, int*);
static int		GetNextOperand(AssemblyEnv*, Tcl_Token**, Tcl_Obj**);
static void		LookForFreshCatches(BasicBlock*, BasicBlock**);
static void		MoveCodeForJumps(AssemblyEnv*, int);
static void		MoveExceptionRangesToBasicBlock(AssemblyEnv*, int,
			    int);
static AssemblyEnv*	NewAssemblyEnv(CompileEnv*, int);
static int		ProcessCatches(AssemblyEnv*);
static int		ProcessCatchesInBasicBlock(AssemblyEnv*, BasicBlock*,
			    BasicBlock*, enum BasicBlockCatchState, int);
static void		ResetVisitedBasicBlocks(AssemblyEnv*);
static void		ResolveJumpTableTargets(AssemblyEnv*, BasicBlock*);
static void		ReportUndefinedLabel(AssemblyEnv*, BasicBlock*,
			    Tcl_Obj*);
static void		RestoreEmbeddedExceptionRanges(AssemblyEnv*);
static int		StackCheckBasicBlock(AssemblyEnv*, BasicBlock *,
			    BasicBlock *, int);
static BasicBlock*	StartBasicBlock(AssemblyEnv*, int fallthrough,
			    Tcl_Obj* jumpLabel);
/* static int		AdvanceIp(const unsigned char *pc); */
static int		StackCheckBasicBlock(AssemblyEnv*, BasicBlock *,
			    BasicBlock *, int);
static int		StackCheckExit(AssemblyEnv*);
static void		StackFreshCatches(AssemblyEnv*, BasicBlock*, int,
			    BasicBlock**, int*);
static void		SyncStackDepth(AssemblyEnv*);
static int		TclAssembleCode(CompileEnv* envPtr, const char* code,
			    int codeLen, int flags);
static void		UnstackExpiredCatches(CompileEnv*, BasicBlock*, int,
			    BasicBlock**, int*);

/*
 * Tcl_ObjType that describes bytecode emitted by the assembler.
 */

static const Tcl_ObjType assembleCodeType = {
    "assemblecode",
    FreeAssembleCodeInternalRep, /* freeIntRepProc */
    DupAssembleCodeInternalRep,	 /* dupIntRepProc */
    NULL,			 /* updateStringProc */
    NULL			 /* setFromAnyProc */
};

/*
 * Source instructions recognized in the Tcl Assembly Language (TAL)
 */

static const TalInstDesc TalInstructionTable[] = {
    /* PUSH must be first, see the code near the end of TclAssembleCode */
    {"push",		ASSEM_PUSH,	(INST_PUSH1<<8
					 | INST_PUSH4),		0,	1},

    {"add",		ASSEM_1BYTE,	INST_ADD,		2,	1},
    {"append",		ASSEM_LVT,	(INST_APPEND_SCALAR1<<8
					 | INST_APPEND_SCALAR4),1,	1},
    {"appendArray",	ASSEM_LVT,	(INST_APPEND_ARRAY1<<8
					 | INST_APPEND_ARRAY4),	2,	1},
    {"appendArrayStk",	ASSEM_1BYTE,	INST_APPEND_ARRAY_STK,	3,	1},
    {"appendStk",	ASSEM_1BYTE,	INST_APPEND_STK,	2,	1},
    {"arrayExistsImm",	ASSEM_LVT4,	INST_ARRAY_EXISTS_IMM,	0,	1},
    {"arrayExistsStk",	ASSEM_1BYTE,	INST_ARRAY_EXISTS_STK,	1,	1},
    {"arrayMakeImm",	ASSEM_LVT4,	INST_ARRAY_MAKE_IMM,	0,	0},
    {"arrayMakeStk",	ASSEM_1BYTE,	INST_ARRAY_MAKE_STK,	1,	0},
    {"beginCatch",	ASSEM_BEGIN_CATCH,
					INST_BEGIN_CATCH4,	0,	0},
    {"bitand",		ASSEM_1BYTE,	INST_BITAND,		2,	1},
    {"bitnot",		ASSEM_1BYTE,	INST_BITNOT,		1,	1},
    {"bitor",		ASSEM_1BYTE,	INST_BITOR,		2,	1},
    {"bitxor",		ASSEM_1BYTE,	INST_BITXOR,		2,	1},
    {"clockRead",	ASSEM_CLOCK_READ, INST_CLOCK_READ,	0,	1},
    {"concat",		ASSEM_CONCAT1,	INST_STR_CONCAT1,	INT_MIN,1},
    {"concatStk",	ASSEM_LIST,	INST_CONCAT_STK,	INT_MIN,1},
    {"coroName",	ASSEM_1BYTE,	INST_COROUTINE_NAME,	0,	1},
    {"currentNamespace",ASSEM_1BYTE,	INST_NS_CURRENT,	0,	1},
    {"dictAppend",	ASSEM_LVT4,	INST_DICT_APPEND,	2,	1},
    {"dictExists",	ASSEM_DICT_GET, INST_DICT_EXISTS,	INT_MIN,1},
    {"dictExpand",	ASSEM_1BYTE,	INST_DICT_EXPAND,	3,	1},
    {"dictGet",		ASSEM_DICT_GET, INST_DICT_GET,		INT_MIN,1},
    {"dictIncrImm",	ASSEM_SINT4_LVT4,
					INST_DICT_INCR_IMM,	1,	1},
    {"dictLappend",	ASSEM_LVT4,	INST_DICT_LAPPEND,	2,	1},
    {"dictRecombineStk",ASSEM_1BYTE,	INST_DICT_RECOMBINE_STK,3,	0},
    {"dictRecombineImm",ASSEM_LVT4,	INST_DICT_RECOMBINE_IMM,2,	0},
    {"dictSet",		ASSEM_DICT_SET, INST_DICT_SET,		INT_MIN,1},
    {"dictUnset",	ASSEM_DICT_UNSET,
					INST_DICT_UNSET,	INT_MIN,1},
    {"div",		ASSEM_1BYTE,	INST_DIV,		2,	1},
    {"dup",		ASSEM_1BYTE,	INST_DUP,		1,	2},
    {"endCatch",	ASSEM_END_CATCH,INST_END_CATCH,		0,	0},
    {"eq",		ASSEM_1BYTE,	INST_EQ,		2,	1},
    {"eval",		ASSEM_EVAL,	INST_EVAL_STK,		1,	1},
    {"evalStk",		ASSEM_1BYTE,	INST_EVAL_STK,		1,	1},
    {"exist",		ASSEM_LVT4,	INST_EXIST_SCALAR,	0,	1},
    {"existArray",	ASSEM_LVT4,	INST_EXIST_ARRAY,	1,	1},
    {"existArrayStk",	ASSEM_1BYTE,	INST_EXIST_ARRAY_STK,	2,	1},
    {"existStk",	ASSEM_1BYTE,	INST_EXIST_STK,		1,	1},
    {"expon",		ASSEM_1BYTE,	INST_EXPON,		2,	1},
    {"expr",		ASSEM_EVAL,	INST_EXPR_STK,		1,	1},
    {"exprStk",		ASSEM_1BYTE,	INST_EXPR_STK,		1,	1},
    {"ge",		ASSEM_1BYTE,	INST_GE,		2,	1},
    {"gt",		ASSEM_1BYTE,	INST_GT,		2,	1},
    {"incr",		ASSEM_LVT1,	INST_INCR_SCALAR1,	1,	1},
    {"incrArray",	ASSEM_LVT1,	INST_INCR_ARRAY1,	2,	1},
    {"incrArrayImm",	ASSEM_LVT1_SINT1,
					INST_INCR_ARRAY1_IMM,	1,	1},
    {"incrArrayStk",	ASSEM_1BYTE,	INST_INCR_ARRAY_STK,	3,	1},
    {"incrArrayStkImm", ASSEM_SINT1,	INST_INCR_ARRAY_STK_IMM,2,	1},
    {"incrImm",		ASSEM_LVT1_SINT1,
					INST_INCR_SCALAR1_IMM,	0,	1},
    {"incrStk",		ASSEM_1BYTE,	INST_INCR_STK,		2,	1},
    {"incrStkImm",	ASSEM_SINT1,	INST_INCR_STK_IMM,	1,	1},
    {"infoLevelArgs",	ASSEM_1BYTE,	INST_INFO_LEVEL_ARGS,	1,	1},
    {"infoLevelNumber",	ASSEM_1BYTE,	INST_INFO_LEVEL_NUM,	0,	1},
    {"invokeStk",	ASSEM_INVOKE,	(INST_INVOKE_STK1 << 8
					 | INST_INVOKE_STK4),	INT_MIN,1},
    {"jump",		ASSEM_JUMP,	INST_JUMP1,		0,	0},
    {"jump4",		ASSEM_JUMP4,	INST_JUMP4,		0,	0},
    {"jumpFalse",	ASSEM_JUMP,	INST_JUMP_FALSE1,	1,	0},
    {"jumpFalse4",	ASSEM_JUMP4,	INST_JUMP_FALSE4,	1,	0},
    {"jumpTable",	ASSEM_JUMPTABLE,INST_JUMP_TABLE,	1,	0},
    {"jumpTrue",	ASSEM_JUMP,	INST_JUMP_TRUE1,	1,	0},
    {"jumpTrue4",	ASSEM_JUMP4,	INST_JUMP_TRUE4,	1,	0},
    {"label",		ASSEM_LABEL,	0,			0,	0},
    {"land",		ASSEM_1BYTE,	INST_LAND,		2,	1},
    {"lappend",		ASSEM_LVT,	(INST_LAPPEND_SCALAR1<<8
					 | INST_LAPPEND_SCALAR4),
								1,	1},
    {"lappendArray",	ASSEM_LVT,	(INST_LAPPEND_ARRAY1<<8
					 | INST_LAPPEND_ARRAY4),2,	1},
    {"lappendArrayStk", ASSEM_1BYTE,	INST_LAPPEND_ARRAY_STK,	3,	1},
    {"lappendList",	ASSEM_LVT4,	INST_LAPPEND_LIST,	1,	1},
    {"lappendListArray",ASSEM_LVT4,	INST_LAPPEND_LIST_ARRAY,2,	1},
    {"lappendListArrayStk", ASSEM_1BYTE,INST_LAPPEND_LIST_ARRAY_STK, 3,	1},
    {"lappendListStk",	ASSEM_1BYTE,	INST_LAPPEND_LIST_STK,	2,	1},
    {"lappendStk",	ASSEM_1BYTE,	INST_LAPPEND_STK,	2,	1},
    {"le",		ASSEM_1BYTE,	INST_LE,		2,	1},
    {"lindexMulti",	ASSEM_LINDEX_MULTI,
					INST_LIST_INDEX_MULTI,	INT_MIN,1},
    {"list",		ASSEM_LIST,	INST_LIST,		INT_MIN,1},
    {"listConcat",	ASSEM_1BYTE,	INST_LIST_CONCAT,	2,	1},
    {"listIn",		ASSEM_1BYTE,	INST_LIST_IN,		2,	1},
    {"listIndex",	ASSEM_1BYTE,	INST_LIST_INDEX,	2,	1},
    {"listIndexImm",	ASSEM_INDEX,	INST_LIST_INDEX_IMM,	1,	1},
    {"listLength",	ASSEM_1BYTE,	INST_LIST_LENGTH,	1,	1},
    {"listNotIn",	ASSEM_1BYTE,	INST_LIST_NOT_IN,	2,	1},
    {"load",		ASSEM_LVT,	(INST_LOAD_SCALAR1 << 8
					 | INST_LOAD_SCALAR4),	0,	1},
    {"loadArray",	ASSEM_LVT,	(INST_LOAD_ARRAY1<<8
					 | INST_LOAD_ARRAY4),	1,	1},
    {"loadArrayStk",	ASSEM_1BYTE,	INST_LOAD_ARRAY_STK,	2,	1},
    {"loadStk",		ASSEM_1BYTE,	INST_LOAD_STK,		1,	1},
    {"lor",		ASSEM_1BYTE,	INST_LOR,		2,	1},
    {"lsetFlat",	ASSEM_LSET_FLAT,INST_LSET_FLAT,		INT_MIN,1},
    {"lsetList",	ASSEM_1BYTE,	INST_LSET_LIST,		3,	1},
    {"lshift",		ASSEM_1BYTE,	INST_LSHIFT,		2,	1},
    {"lt",		ASSEM_1BYTE,	INST_LT,		2,	1},
    {"mod",		ASSEM_1BYTE,	INST_MOD,		2,	1},
    {"mult",		ASSEM_1BYTE,	INST_MULT,		2,	1},
    {"neq",		ASSEM_1BYTE,	INST_NEQ,		2,	1},
    {"nop",		ASSEM_1BYTE,	INST_NOP,		0,	0},
    {"not",		ASSEM_1BYTE,	INST_LNOT,		1,	1},
    {"nsupvar",		ASSEM_LVT4,	INST_NSUPVAR,		2,	1},
    {"numericType",	ASSEM_1BYTE,	INST_NUM_TYPE,		1,	1},
    {"originCmd",	ASSEM_1BYTE,	INST_ORIGIN_COMMAND,	1,	1},
    {"over",		ASSEM_OVER,	INST_OVER,		INT_MIN,-1-1},
    {"pop",		ASSEM_1BYTE,	INST_POP,		1,	0},
    {"pushReturnCode",	ASSEM_1BYTE,	INST_PUSH_RETURN_CODE,	0,	1},
    {"pushReturnOpts",	ASSEM_1BYTE,	INST_PUSH_RETURN_OPTIONS,
								0,	1},
    {"pushResult",	ASSEM_1BYTE,	INST_PUSH_RESULT,	0,	1},
    {"regexp",		ASSEM_REGEXP,	INST_REGEXP,		2,	1},
    {"resolveCmd",	ASSEM_1BYTE,	INST_RESOLVE_COMMAND,	1,	1},
    {"reverse",		ASSEM_REVERSE,	INST_REVERSE,		INT_MIN,-1-0},
    {"rshift",		ASSEM_1BYTE,	INST_RSHIFT,		2,	1},
    {"store",		ASSEM_LVT,	(INST_STORE_SCALAR1<<8
					 | INST_STORE_SCALAR4),	1,	1},
    {"storeArray",	ASSEM_LVT,	(INST_STORE_ARRAY1<<8
					 | INST_STORE_ARRAY4),	2,	1},
    {"storeArrayStk",	ASSEM_1BYTE,	INST_STORE_ARRAY_STK,	3,	1},
    {"storeStk",	ASSEM_1BYTE,	INST_STORE_STK,		2,	1},
    {"strcaseLower",	ASSEM_1BYTE,	INST_STR_LOWER,		1,	1},
    {"strcaseTitle",	ASSEM_1BYTE,	INST_STR_TITLE,		1,	1},
    {"strcaseUpper",	ASSEM_1BYTE,	INST_STR_UPPER,		1,	1},
    {"strcmp",		ASSEM_1BYTE,	INST_STR_CMP,		2,	1},
    {"strcat",		ASSEM_CONCAT1,	INST_STR_CONCAT1,	INT_MIN,1},
    {"streq",		ASSEM_1BYTE,	INST_STR_EQ,		2,	1},
    {"strfind",		ASSEM_1BYTE,	INST_STR_FIND,		2,	1},
    {"strindex",	ASSEM_1BYTE,	INST_STR_INDEX,		2,	1},
    {"strlen",		ASSEM_1BYTE,	INST_STR_LEN,		1,	1},
    {"strmap",		ASSEM_1BYTE,	INST_STR_MAP,		3,	1},
    {"strmatch",	ASSEM_BOOL,	INST_STR_MATCH,		2,	1},
    {"strneq",		ASSEM_1BYTE,	INST_STR_NEQ,		2,	1},
    {"strrange",	ASSEM_1BYTE,	INST_STR_RANGE,		3,	1},
    {"strreplace",	ASSEM_1BYTE,	INST_STR_REPLACE,	4,	1},
    {"strrfind",	ASSEM_1BYTE,	INST_STR_FIND_LAST,	2,	1},
    {"strtrim",		ASSEM_1BYTE,	INST_STR_TRIM,		2,	1},
    {"strtrimLeft",	ASSEM_1BYTE,	INST_STR_TRIM_LEFT,	2,	1},
    {"strtrimRight",	ASSEM_1BYTE,	INST_STR_TRIM_RIGHT,	2,	1},
    {"sub",		ASSEM_1BYTE,	INST_SUB,		2,	1},
    {"tclooClass",	ASSEM_1BYTE,	INST_TCLOO_CLASS,	1,	1},
    {"tclooIsObject",	ASSEM_1BYTE,	INST_TCLOO_IS_OBJECT,	1,	1},
    {"tclooNamespace",	ASSEM_1BYTE,	INST_TCLOO_NS,		1,	1},
    {"tclooSelf",	ASSEM_1BYTE,	INST_TCLOO_SELF,	0,	1},
    {"tryCvtToBoolean",	ASSEM_1BYTE,	INST_TRY_CVT_TO_BOOLEAN,1,	2},
    {"tryCvtToNumeric",	ASSEM_1BYTE,	INST_TRY_CVT_TO_NUMERIC,1,	1},
    {"uminus",		ASSEM_1BYTE,	INST_UMINUS,		1,	1},
    {"unset",		ASSEM_BOOL_LVT4,INST_UNSET_SCALAR,	0,	0},
    {"unsetArray",	ASSEM_BOOL_LVT4,INST_UNSET_ARRAY,	1,	0},
    {"unsetArrayStk",	ASSEM_BOOL,	INST_UNSET_ARRAY_STK,	2,	0},
    {"unsetStk",	ASSEM_BOOL,	INST_UNSET_STK,		1,	0},
    {"uplus",		ASSEM_1BYTE,	INST_UPLUS,		1,	1},
    {"upvar",		ASSEM_LVT4,	INST_UPVAR,		2,	1},
    {"variable",	ASSEM_LVT4,	INST_VARIABLE,		1,	0},
    {"verifyDict",	ASSEM_1BYTE,	INST_DICT_VERIFY,	1,	0},
    {"yield",		ASSEM_1BYTE,	INST_YIELD,		1,	1},
    {NULL,		0,		0,			0,	0}
};

/*
 * List of instructions that cannot throw an exception under any
 * circumstances.  These instructions are the ones that are permissible after
 * an exception is caught but before the corresponding exception range is
 * popped from the stack.
 * The instructions must be in ascending order by numeric operation code.
 */

static const unsigned char NonThrowingByteCodes[] = {
    INST_PUSH1, INST_PUSH4, INST_POP, INST_DUP,			/* 1-4 */
    INST_JUMP1, INST_JUMP4,					/* 34-35 */
    INST_END_CATCH, INST_PUSH_RESULT, INST_PUSH_RETURN_CODE,	/* 70-72 */
    INST_LIST,							/* 79 */
    INST_OVER,							/* 95 */
    INST_PUSH_RETURN_OPTIONS,					/* 108 */
    INST_REVERSE,						/* 126 */
    INST_NOP,							/* 132 */
    INST_STR_MAP,						/* 143 */
    INST_STR_FIND,						/* 144 */
    INST_COROUTINE_NAME,					/* 149 */
    INST_NS_CURRENT,						/* 151 */
    INST_INFO_LEVEL_NUM,					/* 152 */
    INST_RESOLVE_COMMAND,					/* 154 */
    INST_STR_TRIM, INST_STR_TRIM_LEFT, INST_STR_TRIM_RIGHT,	/* 166-168 */
    INST_CONCAT_STK,						/* 169 */
    INST_STR_UPPER, INST_STR_LOWER, INST_STR_TITLE,		/* 170-172 */
    INST_NUM_TYPE						/* 180 */
};

/*
 * Helper macros.
 */

#if defined(TCL_DEBUG_ASSEMBLY) && defined(__GNUC__) && __GNUC__ > 2
#define DEBUG_PRINT(...)	fprintf(stderr, ##__VA_ARGS__);fflush(stderr)
#elif defined(__GNUC__) && __GNUC__ > 2
#define DEBUG_PRINT(...)	/* nothing */
#else
#define DEBUG_PRINT		/* nothing */
#endif

/*
 *-----------------------------------------------------------------------------
 *
 * BBAdjustStackDepth --
 *
 *	When an opcode is emitted, adjusts the stack information in the basic
 *	block to reflect the number of operands produced and consumed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates minimum, maximum and final stack requirements in the basic
 *	block.
 *
 *-----------------------------------------------------------------------------
 */

static void
BBAdjustStackDepth(
    BasicBlock *bbPtr,		/* Structure describing the basic block */
    int consumed,		/* Count of operands consumed by the
				 * operation */
    int produced)		/* Count of operands produced by the
				 * operation */
{
    int depth = bbPtr->finalStackDepth;

    depth -= consumed;
    if (depth < bbPtr->minStackDepth) {
	bbPtr->minStackDepth = depth;
    }
    depth += produced;
    if (depth > bbPtr->maxStackDepth) {
	bbPtr->maxStackDepth = depth;
    }
    bbPtr->finalStackDepth = depth;
}

/*
 *-----------------------------------------------------------------------------
 *
 * BBUpdateStackReqs --
 *
 *	Updates the stack requirements of a basic block, given the opcode
 *	being emitted and an operand count.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates min, max and final stack requirements in the basic block.
 *
 * Notes:
 *	This function must not be called for instructions such as REVERSE and
 *	OVER that are variadic but do not consume all their operands. Instead,
 *	BBAdjustStackDepth should be called directly.
 *
 *	count should be provided only for variadic operations. For operations
 *	with known arity, count should be 0.
 *
 *-----------------------------------------------------------------------------
 */

static void
BBUpdateStackReqs(
    BasicBlock* bbPtr,		/* Structure describing the basic block */
    int tblIdx,			/* Index in TalInstructionTable of the
				 * operation being assembled */
    int count)			/* Count of operands for variadic insts */
{
    int consumed = TalInstructionTable[tblIdx].operandsConsumed;
    int produced = TalInstructionTable[tblIdx].operandsProduced;

    if (consumed == INT_MIN) {
	/*
	 * The instruction is variadic; it consumes 'count' operands.
	 */

	consumed = count;
    }
    if (produced < 0) {
	/*
	 * The instruction leaves some of its variadic operands on the stack,
	 * with net stack effect of '-1-produced'
	 */

	produced = consumed - produced - 1;
    }
    BBAdjustStackDepth(bbPtr, consumed, produced);
}

/*
 *-----------------------------------------------------------------------------
 *
 * BBEmitOpcode, BBEmitInstInt1, BBEmitInstInt4 --
 *
 *	Emit the opcode part of an instruction, or the entirety of an
 *	instruction with a 1- or 4-byte operand, and adjust stack
 *	requirements.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stores instruction and operand in the operand stream, and adjusts the
 *	stack.
 *
 *-----------------------------------------------------------------------------
 */

static void
BBEmitOpcode(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    int tblIdx,			/* Table index in TalInstructionTable of op */
    int count)			/* Operand count for variadic ops */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    BasicBlock* bbPtr = assemEnvPtr->curr_bb;
				/* Current basic block */
    int op = TalInstructionTable[tblIdx].tclInstCode & 0xff;

    /*
     * If this is the first instruction in a basic block, record its line
     * number.
     */

    if (bbPtr->startOffset == envPtr->codeNext - envPtr->codeStart) {
	bbPtr->startLine = assemEnvPtr->cmdLine;
    }

    TclEmitInt1(op, envPtr);
    TclUpdateAtCmdStart(op, envPtr);
    BBUpdateStackReqs(bbPtr, tblIdx, count);
}

static void
BBEmitInstInt1(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    int tblIdx,			/* Index in TalInstructionTable of op */
    int opnd,			/* 1-byte operand */
    int count)			/* Operand count for variadic ops */
{
    BBEmitOpcode(assemEnvPtr, tblIdx, count);
    TclEmitInt1(opnd, assemEnvPtr->envPtr);
}

static void
BBEmitInstInt4(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    int tblIdx,			/* Index in TalInstructionTable of op */
    int opnd,			/* 4-byte operand */
    int count)			/* Operand count for variadic ops */
{
    BBEmitOpcode(assemEnvPtr, tblIdx, count);
    TclEmitInt4(opnd, assemEnvPtr->envPtr);
}

/*
 *-----------------------------------------------------------------------------
 *
 * BBEmitInst1or4 --
 *
 *	Emits a 1- or 4-byte operation according to the magnitude of the
 *	operand.
 *
 *-----------------------------------------------------------------------------
 */

static void
BBEmitInst1or4(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    int tblIdx,			/* Index in TalInstructionTable of op */
    int param,			/* Variable-length parameter */
    int count)			/* Arity if variadic */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    BasicBlock* bbPtr = assemEnvPtr->curr_bb;
				/* Current basic block */
    int op = TalInstructionTable[tblIdx].tclInstCode;

    if (param <= 0xff) {
	op >>= 8;
    } else {
	op &= 0xff;
    }
    TclEmitInt1(op, envPtr);
    if (param <= 0xff) {
	TclEmitInt1(param, envPtr);
    } else {
	TclEmitInt4(param, envPtr);
    }
    TclUpdateAtCmdStart(op, envPtr);
    BBUpdateStackReqs(bbPtr, tblIdx, count);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_AssembleObjCmd, TclNRAssembleObjCmd --
 *
 *	Direct evaluation path for tcl::unsupported::assemble
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Assembles the code in objv[1], and executes it, so side effects
 *	include whatever the code does.
 *
 *-----------------------------------------------------------------------------
 */

int
Tcl_AssembleObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    /*
     * Boilerplate - make sure that there is an NRE trampoline on the C stack
     * because there needs to be one in place to execute bytecode.
     */

    return Tcl_NRCallObjProc(interp, TclNRAssembleObjCmd, dummy, objc, objv);
}

int
TclNRAssembleObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    ByteCode *codePtr;		/* Pointer to the bytecode to execute */
    Tcl_Obj* backtrace;		/* Object where extra error information is
				 * constructed. */

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "bytecodeList");
	return TCL_ERROR;
    }

    /*
     * Assemble the source to bytecode.
     */

    codePtr = CompileAssembleObj(interp, objv[1]);

    /*
     * On failure, report error line.
     */

    if (codePtr == NULL) {
	Tcl_AddErrorInfo(interp, "\n    (\"");
	Tcl_AppendObjToErrorInfo(interp, objv[0]);
	Tcl_AddErrorInfo(interp, "\" body, line ");
	backtrace = Tcl_NewIntObj(Tcl_GetErrorLine(interp));
	Tcl_AppendObjToErrorInfo(interp, backtrace);
	Tcl_AddErrorInfo(interp, ")");
	return TCL_ERROR;
    }

    /*
     * Use NRE to evaluate the bytecode from the trampoline.
     */

    return TclNRExecuteByteCode(interp, codePtr);
}

/*
 *-----------------------------------------------------------------------------
 *
 * CompileAssembleObj --
 *
 *	Sets up and assembles Tcl bytecode for the direct-execution path in
 *	the Tcl bytecode assembler.
 *
 * Results:
 *	Returns a pointer to the assembled code. Returns NULL if the assembly
 *	fails for any reason, with an appropriate error message in the
 *	interpreter.
 *
 *-----------------------------------------------------------------------------
 */

static ByteCode *
CompileAssembleObj(
    Tcl_Interp *interp,		/* Tcl interpreter */
    Tcl_Obj *objPtr)		/* Source code to assemble */
{
    Interp *iPtr = (Interp *) interp;
				/* Internals of the interpreter */
    CompileEnv compEnv;		/* Compilation environment structure */
    register ByteCode *codePtr = NULL;
				/* Bytecode resulting from the assembly */
    Namespace* namespacePtr;	/* Namespace in which variable and command
				 * names in the bytecode resolve */
    int status;			/* Status return from Tcl_AssembleCode */
    const char* source;		/* String representation of the source code */
    int sourceLen;		/* Length of the source code in bytes */


    /*
     * Get the expression ByteCode from the object. If it exists, make sure it
     * is valid in the current context.
     */

    if (objPtr->typePtr == &assembleCodeType) {
	namespacePtr = iPtr->varFramePtr->nsPtr;
	codePtr = objPtr->internalRep.twoPtrValue.ptr1;
	if (((Interp *) *codePtr->interpHandle == iPtr)
		&& (codePtr->compileEpoch == iPtr->compileEpoch)
		&& (codePtr->nsPtr == namespacePtr)
		&& (codePtr->nsEpoch == namespacePtr->resolverEpoch)
		&& (codePtr->localCachePtr
			== iPtr->varFramePtr->localCachePtr)) {
	    return codePtr;
	}

	/*
	 * Not valid, so free it and regenerate.
	 */

	FreeAssembleCodeInternalRep(objPtr);
    }

    /*
     * Set up the compilation environment, and assemble the code.
     */

    source = TclGetStringFromObj(objPtr, &sourceLen);
    TclInitCompileEnv(interp, &compEnv, source, sourceLen, NULL, 0);
    status = TclAssembleCode(&compEnv, source, sourceLen, TCL_EVAL_DIRECT);
    if (status != TCL_OK) {
	/*
	 * Assembly failed. Clean up and report the error.
	 */
	TclFreeCompileEnv(&compEnv);
	return NULL;
    }

    /*
     * Add a "done" instruction as the last instruction and change the object
     * into a ByteCode object. Ownership of the literal objects and aux data
     * items is given to the ByteCode object.
     */

    TclEmitOpcode(INST_DONE, &compEnv);
    TclInitByteCodeObj(objPtr, &compEnv);
    objPtr->typePtr = &assembleCodeType;
    TclFreeCompileEnv(&compEnv);

    /*
     * Record the local variable context to which the bytecode pertains
     */

    codePtr = objPtr->internalRep.twoPtrValue.ptr1;
    if (iPtr->varFramePtr->localCachePtr) {
	codePtr->localCachePtr = iPtr->varFramePtr->localCachePtr;
	codePtr->localCachePtr->refCount++;
    }

    /*
     * Report on what the assembler did.
     */

#ifdef TCL_COMPILE_DEBUG
    if (tclTraceCompile >= 2) {
	TclPrintByteCodeObj(interp, objPtr);
	fflush(stdout);
    }
#endif /* TCL_COMPILE_DEBUG */

    return codePtr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * TclCompileAssembleCmd --
 *
 *	Compilation procedure for the '::tcl::unsupported::assemble' command.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Puts the result of assembling the code into the bytecode stream in
 *	'compileEnv'.
 *
 * This procedure makes sure that the command has a single arg, which is
 * constant. If that condition is met, the procedure calls TclAssembleCode to
 * produce bytecode for the given assembly code, and returns any error
 * resulting from the assembly.
 *
 *-----------------------------------------------------------------------------
 */

int
TclCompileAssembleCmd(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;	/* Token in the input script */

    int numCommands = envPtr->numCommands;
    int offset = envPtr->codeNext - envPtr->codeStart;
    int depth = envPtr->currStackDepth;

    /*
     * Make sure that the command has a single arg that is a simple word.
     */

    if (parsePtr->numWords != 2) {
	return TCL_ERROR;
    }
    tokenPtr = TokenAfter(parsePtr->tokenPtr);
    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	return TCL_ERROR;
    }

    /*
     * Compile the code and convert any error from the compilation into
     * bytecode reporting the error;
     */

    if (TCL_ERROR == TclAssembleCode(envPtr, tokenPtr[1].start,
	    tokenPtr[1].size, TCL_EVAL_DIRECT)) {

	Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		"\n    (\"%.*s\" body, line %d)",
		parsePtr->tokenPtr->size, parsePtr->tokenPtr->start,
		Tcl_GetErrorLine(interp)));
	envPtr->numCommands = numCommands;
	envPtr->codeNext = envPtr->codeStart + offset;
	envPtr->currStackDepth = depth;
	TclCompileSyntaxError(interp, envPtr);
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * TclAssembleCode --
 *
 *	Take a list of instructions in a Tcl_Obj, and assemble them to Tcl
 *	bytecodes
 *
 * Results:
 *	Returns TCL_OK on success, TCL_ERROR on failure.  If 'flags' includes
 *	TCL_EVAL_DIRECT, places an error message in the interpreter result.
 *
 * Side effects:
 *	Adds byte codes to the compile environment, and updates the
 *	environment's stack depth.
 *
 *-----------------------------------------------------------------------------
 */

static int
TclAssembleCode(
    CompileEnv *envPtr,		/* Compilation environment that is to receive
				 * the generated bytecode */
    const char* codePtr,	/* Assembly-language code to be processed */
    int codeLen,		/* Length of the code */
    int flags)			/* OR'ed combination of flags */
{
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    /*
     * Walk through the assembly script using the Tcl parser.  Each 'command'
     * will be an instruction or assembly directive.
     */

    const char* instPtr = codePtr;
				/* Where to start looking for a line of code */
    const char* nextPtr;	/* Pointer to the end of the line of code */
    int bytesLeft = codeLen;	/* Number of bytes of source code remaining to
				 * be parsed */
    int status;			/* Tcl status return */
    AssemblyEnv* assemEnvPtr = NewAssemblyEnv(envPtr, flags);
    Tcl_Parse* parsePtr = assemEnvPtr->parsePtr;

    do {
	/*
	 * Parse out one command line from the assembly script.
	 */

	status = Tcl_ParseCommand(interp, instPtr, bytesLeft, 0, parsePtr);

	/*
	 * Report errors in the parse.
	 */

	if (status != TCL_OK) {
	    if (flags & TCL_EVAL_DIRECT) {
		Tcl_LogCommandInfo(interp, codePtr, parsePtr->commandStart,
			parsePtr->term + 1 - parsePtr->commandStart);
	    }
	    FreeAssemblyEnv(assemEnvPtr);
	    return TCL_ERROR;
	}

	/*
	 * Advance the pointers around any leading commentary.
	 */

	TclAdvanceLines(&assemEnvPtr->cmdLine, instPtr,
		parsePtr->commandStart);
	TclAdvanceContinuations(&assemEnvPtr->cmdLine, &assemEnvPtr->clNext,
		parsePtr->commandStart - envPtr->source);

	/*
	 * Process the line of code.
	 */

	if (parsePtr->numWords > 0) {
	    int instLen = parsePtr->commandSize;
		    /* Length in bytes of the current command */

	    if (parsePtr->term == parsePtr->commandStart + instLen - 1) {
		--instLen;
	    }

	    /*
	     * If tracing, show each line assembled as it happens.
	     */

#ifdef TCL_COMPILE_DEBUG
	    if ((tclTraceCompile >= 2) && (envPtr->procPtr == NULL)) {
		printf("  %4ld Assembling: ",
			(long)(envPtr->codeNext - envPtr->codeStart));
		TclPrintSource(stdout, parsePtr->commandStart,
			TclMin(instLen, 55));
		printf("\n");
	    }
#endif
	    if (AssembleOneLine(assemEnvPtr) != TCL_OK) {
		if (flags & TCL_EVAL_DIRECT) {
		    Tcl_LogCommandInfo(interp, codePtr,
			    parsePtr->commandStart, instLen);
		}
		Tcl_FreeParse(parsePtr);
		FreeAssemblyEnv(assemEnvPtr);
		return TCL_ERROR;
	    }
	}

	/*
	 * Advance to the next line of code.
	 */

	nextPtr = parsePtr->commandStart + parsePtr->commandSize;
	bytesLeft -= (nextPtr - instPtr);
	instPtr = nextPtr;
	TclAdvanceLines(&assemEnvPtr->cmdLine, parsePtr->commandStart,
		instPtr);
	TclAdvanceContinuations(&assemEnvPtr->cmdLine, &assemEnvPtr->clNext,
		instPtr - envPtr->source);
	Tcl_FreeParse(parsePtr);
    } while (bytesLeft > 0);

    /*
     * Done with parsing the code.
     */

    status = FinishAssembly(assemEnvPtr);
    FreeAssemblyEnv(assemEnvPtr);
    return status;
}

/*
 *-----------------------------------------------------------------------------
 *
 * NewAssemblyEnv --
 *
 *	Creates an environment for the assembler to run in.
 *
 * Results:
 *	Allocates, initialises and returns an assembler environment
 *
 *-----------------------------------------------------------------------------
 */

static AssemblyEnv*
NewAssemblyEnv(
    CompileEnv* envPtr,		/* Compilation environment being used for code
				 * generation*/
    int flags)			/* Compilation flags (TCL_EVAL_DIRECT) */
{
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    AssemblyEnv* assemEnvPtr = TclStackAlloc(interp, sizeof(AssemblyEnv));
				/* Assembler environment under construction */
    Tcl_Parse* parsePtr = TclStackAlloc(interp, sizeof(Tcl_Parse));
				/* Parse of one line of assembly code */

    assemEnvPtr->envPtr = envPtr;
    assemEnvPtr->parsePtr = parsePtr;
    assemEnvPtr->cmdLine = 1;
    assemEnvPtr->clNext = envPtr->clNext;

    /*
     * Make the hashtables that store symbol resolution.
     */

    Tcl_InitHashTable(&assemEnvPtr->labelHash, TCL_STRING_KEYS);

    /*
     * Start the first basic block.
     */

    assemEnvPtr->curr_bb = NULL;
    assemEnvPtr->head_bb = AllocBB(assemEnvPtr);
    assemEnvPtr->curr_bb = assemEnvPtr->head_bb;
    assemEnvPtr->head_bb->startLine = 1;

    /*
     * Stash compilation flags.
     */

    assemEnvPtr->flags = flags;
    return assemEnvPtr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * FreeAssemblyEnv --
 *
 *	Cleans up the assembler environment when assembly is complete.
 *
 *-----------------------------------------------------------------------------
 */

static void
FreeAssemblyEnv(
    AssemblyEnv* assemEnvPtr)	/* Environment to free */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment being used for code
				 * generation */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    BasicBlock* thisBB;		/* Pointer to a basic block being deleted */
    BasicBlock* nextBB;		/* Pointer to a deleted basic block's
				 * successor */

    /*
     * Free all the basic block structures.
     */

    for (thisBB = assemEnvPtr->head_bb; thisBB != NULL; thisBB = nextBB) {
	if (thisBB->jumpTarget != NULL) {
	    Tcl_DecrRefCount(thisBB->jumpTarget);
	}
	if (thisBB->foreignExceptions != NULL) {
	    ckfree(thisBB->foreignExceptions);
	}
	nextBB = thisBB->successor1;
	if (thisBB->jtPtr != NULL) {
	    DeleteMirrorJumpTable(thisBB->jtPtr);
	    thisBB->jtPtr = NULL;
	}
	ckfree(thisBB);
    }

    /*
     * Dispose what's left.
     */

    Tcl_DeleteHashTable(&assemEnvPtr->labelHash);
    TclStackFree(interp, assemEnvPtr->parsePtr);
    TclStackFree(interp, assemEnvPtr);
}

/*
 *-----------------------------------------------------------------------------
 *
 * AssembleOneLine --
 *
 *	Assembles a single command from an assembly language source.
 *
 * Results:
 *	Returns TCL_ERROR with an appropriate error message if the assembly
 *	fails. Returns TCL_OK if the assembly succeeds. Updates the assembly
 *	environment with the state of the assembly.
 *
 *-----------------------------------------------------------------------------
 */

static int
AssembleOneLine(
    AssemblyEnv* assemEnvPtr)	/* State of the assembly */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment being used for code
				 * gen */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    Tcl_Parse* parsePtr = assemEnvPtr->parsePtr;
				/* Parse of the line of code */
    Tcl_Token* tokenPtr;	/* Current token within the line of code */
    Tcl_Obj* instNameObj;	/* Name of the instruction */
    int tblIdx;			/* Index in TalInstructionTable of the
				 * instruction */
    enum TalInstType instType;	/* Type of the instruction */
    Tcl_Obj* operand1Obj = NULL;
				/* First operand to the instruction */
    const char* operand1;	/* String rep of the operand */
    int operand1Len;		/* String length of the operand */
    int opnd;			/* Integer representation of an operand */
    int litIndex;		/* Literal pool index of a constant */
    int localVar;		/* LVT index of a local variable */
    int flags;			/* Flags for a basic block */
    JumptableInfo* jtPtr;	/* Pointer to a jumptable */
    int infoIndex;		/* Index of the jumptable in auxdata */
    int status = TCL_ERROR;	/* Return value from this function */

    /*
     * Make sure that the instruction name is known at compile time.
     */

    tokenPtr = parsePtr->tokenPtr;
    if (GetNextOperand(assemEnvPtr, &tokenPtr, &instNameObj) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Look up the instruction name.
     */

    if (Tcl_GetIndexFromObjStruct(interp, instNameObj,
	    &TalInstructionTable[0].name, sizeof(TalInstDesc), "instruction",
	    TCL_EXACT, &tblIdx) != TCL_OK) {
	goto cleanup;
    }

    /*
     * Vector on the type of instruction being processed.
     */

    instType = TalInstructionTable[tblIdx].instType;
    switch (instType) {

    case ASSEM_PUSH:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "value");
	    goto cleanup;
	}
	if (GetNextOperand(assemEnvPtr, &tokenPtr, &operand1Obj) != TCL_OK) {
	    goto cleanup;
	}
	operand1 = Tcl_GetStringFromObj(operand1Obj, &operand1Len);
	litIndex = TclRegisterNewLiteral(envPtr, operand1, operand1Len);
	BBEmitInst1or4(assemEnvPtr, tblIdx, litIndex, 0);
	break;

    case ASSEM_1BYTE:
	if (parsePtr->numWords != 1) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "");
	    goto cleanup;
	}
	BBEmitOpcode(assemEnvPtr, tblIdx, 0);
	break;

    case ASSEM_BEGIN_CATCH:
	/*
	 * Emit the BEGIN_CATCH instruction with the code offset of the
	 * exception branch target instead of the exception range index. The
	 * correct index will be generated and inserted later, when catches
	 * are being resolved.
	 */

	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "label");
	    goto cleanup;
	}
	if (GetNextOperand(assemEnvPtr, &tokenPtr, &operand1Obj) != TCL_OK) {
	    goto cleanup;
	}
	assemEnvPtr->curr_bb->jumpLine = assemEnvPtr->cmdLine;
	assemEnvPtr->curr_bb->jumpOffset = envPtr->codeNext-envPtr->codeStart;
	BBEmitInstInt4(assemEnvPtr, tblIdx, 0, 0);
	assemEnvPtr->curr_bb->flags |= BB_BEGINCATCH;
	StartBasicBlock(assemEnvPtr, BB_FALLTHRU, operand1Obj);
	break;

    case ASSEM_BOOL:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "boolean");
	    goto cleanup;
	}
	if (GetBooleanOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK) {
	    goto cleanup;
	}
	BBEmitInstInt1(assemEnvPtr, tblIdx, opnd, 0);
	break;

    case ASSEM_BOOL_LVT4:
	if (parsePtr->numWords != 3) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "boolean varName");
	    goto cleanup;
	}
	if (GetBooleanOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK) {
	    goto cleanup;
	}
	localVar = FindLocalVar(assemEnvPtr, &tokenPtr);
	if (localVar < 0) {
	    goto cleanup;
	}
	BBEmitInstInt1(assemEnvPtr, tblIdx, opnd, 0);
	TclEmitInt4(localVar, envPtr);
	break;

    case ASSEM_CLOCK_READ:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "imm8");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK) {
	    goto cleanup;
	}
	if (opnd < 0 || opnd > 3) {
	    Tcl_SetObjResult(interp,
			     Tcl_NewStringObj("operand must be [0..3]", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "OPERAND<0,>3", NULL);
	    goto cleanup;
	}
	BBEmitInstInt1(assemEnvPtr, tblIdx, opnd, opnd);
	break;

    case ASSEM_CONCAT1:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "imm8");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckOneByte(interp, opnd) != TCL_OK
		|| CheckStrictlyPositive(interp, opnd) != TCL_OK) {
	    goto cleanup;
	}
	BBEmitInstInt1(assemEnvPtr, tblIdx, opnd, opnd);
	break;

    case ASSEM_DICT_GET:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckStrictlyPositive(interp, opnd) != TCL_OK) {
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, opnd, opnd+1);
	break;

    case ASSEM_DICT_SET:
	if (parsePtr->numWords != 3) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count varName");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckStrictlyPositive(interp, opnd) != TCL_OK) {
	    goto cleanup;
	}
	localVar = FindLocalVar(assemEnvPtr, &tokenPtr);
	if (localVar < 0) {
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, opnd, opnd+1);
	TclEmitInt4(localVar, envPtr);
	break;

    case ASSEM_DICT_UNSET:
	if (parsePtr->numWords != 3) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count varName");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckStrictlyPositive(interp, opnd) != TCL_OK) {
	    goto cleanup;
	}
	localVar = FindLocalVar(assemEnvPtr, &tokenPtr);
	if (localVar < 0) {
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, opnd, opnd);
	TclEmitInt4(localVar, envPtr);
	break;

    case ASSEM_END_CATCH:
	if (parsePtr->numWords != 1) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "");
	    goto cleanup;
	}
	assemEnvPtr->curr_bb->flags |= BB_ENDCATCH;
	BBEmitOpcode(assemEnvPtr, tblIdx, 0);
	StartBasicBlock(assemEnvPtr, BB_FALLTHRU, NULL);
	break;

    case ASSEM_EVAL:
	/* TODO - Refactor this stuff into a subroutine that takes the inst
	 * code, the message ("script" or "expression") and an evaluator
	 * callback that calls TclCompileScript or TclCompileExpr. */

	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj,
		    ((TalInstructionTable[tblIdx].tclInstCode
		    == INST_EVAL_STK) ? "script" : "expression"));
	    goto cleanup;
	}
	if (tokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	    CompileEmbeddedScript(assemEnvPtr, tokenPtr+1,
		    TalInstructionTable+tblIdx);
	} else if (GetNextOperand(assemEnvPtr, &tokenPtr,
		&operand1Obj) != TCL_OK) {
	    goto cleanup;
	} else {
	    operand1 = Tcl_GetStringFromObj(operand1Obj, &operand1Len);
	    litIndex = TclRegisterNewLiteral(envPtr, operand1, operand1Len);

	    /*
	     * Assumes that PUSH is the first slot!
	     */

	    BBEmitInst1or4(assemEnvPtr, 0, litIndex, 0);
	    BBEmitOpcode(assemEnvPtr, tblIdx, 0);
	}
	break;

    case ASSEM_INVOKE:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckStrictlyPositive(interp, opnd) != TCL_OK) {
	    goto cleanup;
	}

	BBEmitInst1or4(assemEnvPtr, tblIdx, opnd, opnd);
	break;

    case ASSEM_JUMP:
    case ASSEM_JUMP4:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "label");
	    goto cleanup;
	}
	if (GetNextOperand(assemEnvPtr, &tokenPtr, &operand1Obj) != TCL_OK) {
	    goto cleanup;
	}
	assemEnvPtr->curr_bb->jumpOffset = envPtr->codeNext-envPtr->codeStart;
	if (instType == ASSEM_JUMP) {
	    flags = BB_JUMP1;
	    BBEmitInstInt1(assemEnvPtr, tblIdx, 0, 0);
	} else {
	    flags = 0;
	    BBEmitInstInt4(assemEnvPtr, tblIdx, 0, 0);
	}

	/*
	 * Start a new basic block at the instruction following the jump.
	 */

	assemEnvPtr->curr_bb->jumpLine = assemEnvPtr->cmdLine;
	if (TalInstructionTable[tblIdx].operandsConsumed != 0) {
	    flags |= BB_FALLTHRU;
	}
	StartBasicBlock(assemEnvPtr, flags, operand1Obj);
	break;

    case ASSEM_JUMPTABLE:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "table");
	    goto cleanup;
	}
	if (GetNextOperand(assemEnvPtr, &tokenPtr, &operand1Obj) != TCL_OK) {
	    goto cleanup;
	}

	jtPtr = ckalloc(sizeof(JumptableInfo));

	Tcl_InitHashTable(&jtPtr->hashTable, TCL_STRING_KEYS);
	assemEnvPtr->curr_bb->jumpLine = assemEnvPtr->cmdLine;
	assemEnvPtr->curr_bb->jumpOffset = envPtr->codeNext-envPtr->codeStart;
	DEBUG_PRINT("bb %p jumpLine %d jumpOffset %d\n",
		assemEnvPtr->curr_bb, assemEnvPtr->cmdLine,
		envPtr->codeNext - envPtr->codeStart);

	infoIndex = TclCreateAuxData(jtPtr, &tclJumptableInfoType, envPtr);
	DEBUG_PRINT("auxdata index=%d\n", infoIndex);

	BBEmitInstInt4(assemEnvPtr, tblIdx, infoIndex, 0);
	if (CreateMirrorJumpTable(assemEnvPtr, operand1Obj) != TCL_OK) {
	    goto cleanup;
	}
	StartBasicBlock(assemEnvPtr, BB_JUMPTABLE|BB_FALLTHRU, NULL);
	break;

    case ASSEM_LABEL:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "name");
	    goto cleanup;
	}
	if (GetNextOperand(assemEnvPtr, &tokenPtr, &operand1Obj) != TCL_OK) {
	    goto cleanup;
	}

	/*
	 * Add the (label_name, address) pair to the hash table.
	 */

	if (DefineLabel(assemEnvPtr, Tcl_GetString(operand1Obj)) != TCL_OK) {
	    goto cleanup;
	}
	break;

    case ASSEM_LINDEX_MULTI:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckStrictlyPositive(interp, opnd) != TCL_OK) {
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, opnd, opnd);
	break;

    case ASSEM_LIST:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckNonNegative(interp, opnd) != TCL_OK) {
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, opnd, opnd);
	break;

    case ASSEM_INDEX:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count");
	    goto cleanup;
	}
	if (GetListIndexOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK) {
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, opnd, opnd);
	break;

    case ASSEM_LSET_FLAT:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK) {
	    goto cleanup;
	}
	if (opnd < 2) {
	    if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
		Tcl_SetObjResult(interp,
			Tcl_NewStringObj("operand must be >=2", -1));
		Tcl_SetErrorCode(interp, "TCL", "ASSEM", "OPERAND>=2", NULL);
	    }
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, opnd, opnd);
	break;

    case ASSEM_LVT:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "varname");
	    goto cleanup;
	}
	localVar = FindLocalVar(assemEnvPtr, &tokenPtr);
	if (localVar < 0) {
	    goto cleanup;
	}
	BBEmitInst1or4(assemEnvPtr, tblIdx, localVar, 0);
	break;

    case ASSEM_LVT1:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "varname");
	    goto cleanup;
	}
	localVar = FindLocalVar(assemEnvPtr, &tokenPtr);
	if (localVar < 0 || CheckOneByte(interp, localVar)) {
	    goto cleanup;
	}
	BBEmitInstInt1(assemEnvPtr, tblIdx, localVar, 0);
	break;

    case ASSEM_LVT1_SINT1:
	if (parsePtr->numWords != 3) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "varName imm8");
	    goto cleanup;
	}
	localVar = FindLocalVar(assemEnvPtr, &tokenPtr);
	if (localVar < 0 || CheckOneByte(interp, localVar)
		|| GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckSignedOneByte(interp, opnd)) {
	    goto cleanup;
	}
	BBEmitInstInt1(assemEnvPtr, tblIdx, localVar, 0);
	TclEmitInt1(opnd, envPtr);
	break;

    case ASSEM_LVT4:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "varname");
	    goto cleanup;
	}
	localVar = FindLocalVar(assemEnvPtr, &tokenPtr);
	if (localVar < 0) {
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, localVar, 0);
	break;

    case ASSEM_OVER:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckNonNegative(interp, opnd) != TCL_OK) {
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, opnd, opnd+1);
	break;

    case ASSEM_REGEXP:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "boolean");
	    goto cleanup;
	}
	if (GetBooleanOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK) {
	    goto cleanup;
	}
	{
	    int flags = TCL_REG_ADVANCED | (opnd ? TCL_REG_NOCASE : 0);

	    BBEmitInstInt1(assemEnvPtr, tblIdx, flags, 0);
	}
	break;

    case ASSEM_REVERSE:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckNonNegative(interp, opnd) != TCL_OK) {
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, opnd, opnd);
	break;

    case ASSEM_SINT1:
	if (parsePtr->numWords != 2) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "imm8");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK
		|| CheckSignedOneByte(interp, opnd) != TCL_OK) {
	    goto cleanup;
	}
	BBEmitInstInt1(assemEnvPtr, tblIdx, opnd, 0);
	break;

    case ASSEM_SINT4_LVT4:
	if (parsePtr->numWords != 3) {
	    Tcl_WrongNumArgs(interp, 1, &instNameObj, "count varName");
	    goto cleanup;
	}
	if (GetIntegerOperand(assemEnvPtr, &tokenPtr, &opnd) != TCL_OK) {
	    goto cleanup;
	}
	localVar = FindLocalVar(assemEnvPtr, &tokenPtr);
	if (localVar < 0) {
	    goto cleanup;
	}
	BBEmitInstInt4(assemEnvPtr, tblIdx, opnd, 0);
	TclEmitInt4(localVar, envPtr);
	break;

    default:
	Tcl_Panic("Instruction \"%s\" could not be found, can't happen\n",
		Tcl_GetString(instNameObj));
    }

    status = TCL_OK;
 cleanup:
    Tcl_DecrRefCount(instNameObj);
    if (operand1Obj) {
	Tcl_DecrRefCount(operand1Obj);
    }
    return status;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CompileEmbeddedScript --
 *
 *	Compile an embedded 'eval' or 'expr' that appears in assembly code.
 *
 * This procedure is called when the 'eval' or 'expr' assembly directive is
 * encountered, and the argument to the directive is a simple word that
 * requires no substitution. The appropriate compiler (TclCompileScript or
 * TclCompileExpr) is invoked recursively, and emits bytecode.
 *
 * Before the compiler is invoked, the compilation environment's stack
 * consumption is reset to zero. Upon return from the compilation, the net
 * stack effect of the compilation is in the compiler env, and this stack
 * effect is posted to the assembler environment. The compile environment's
 * stack consumption is then restored to what it was before (which is actually
 * the state of the stack on entry to the block of assembly code).
 *
 * Any exception ranges pushed by the compilation are copied to the basic
 * block and removed from the compiler environment. They will be rebuilt at
 * the end of assembly, when the exception stack depth is actually known.
 *
 *-----------------------------------------------------------------------------
 */

static void
CompileEmbeddedScript(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    Tcl_Token* tokenPtr,	/* Tcl_Token containing the script */
    const TalInstDesc* instPtr)	/* Instruction that determines whether
				 * the script is 'expr' or 'eval' */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */

    /*
     * The expression or script is not only known at compile time, but
     * actually a "simple word". It can be compiled inline by invoking the
     * compiler recursively.
     *
     * Save away the stack depth and reset it before compiling the script.
     * We'll record the stack usage of the script in the BasicBlock, and
     * accumulate it together with the stack usage of the enclosing assembly
     * code.
     */

    int savedStackDepth = envPtr->currStackDepth;
    int savedMaxStackDepth = envPtr->maxStackDepth;
    int savedCodeIndex = envPtr->codeNext - envPtr->codeStart;
    int savedExceptArrayNext = envPtr->exceptArrayNext;

    envPtr->currStackDepth = 0;
    envPtr->maxStackDepth = 0;

    StartBasicBlock(assemEnvPtr, BB_FALLTHRU, NULL);
    switch(instPtr->tclInstCode) {
    case INST_EVAL_STK:
	TclCompileScript(interp, tokenPtr->start, tokenPtr->size, envPtr);
	break;
    case INST_EXPR_STK:
	TclCompileExpr(interp, tokenPtr->start, tokenPtr->size, envPtr, 1);
	break;
    default:
	Tcl_Panic("no ASSEM_EVAL case for %s (%d), can't happen",
		instPtr->name, instPtr->tclInstCode);
    }

    /*
     * Roll up the stack usage of the embedded block into the assembler
     * environment.
     */

    SyncStackDepth(assemEnvPtr);
    envPtr->currStackDepth = savedStackDepth;
    envPtr->maxStackDepth = savedMaxStackDepth;

    /*
     * Save any exception ranges that were pushed by the compiler; they will
     * need to be fixed up once the stack depth is known.
     */

    MoveExceptionRangesToBasicBlock(assemEnvPtr, savedCodeIndex,
	    savedExceptArrayNext);

    /*
     * Flush the current basic block.
     */

    StartBasicBlock(assemEnvPtr, BB_FALLTHRU, NULL);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SyncStackDepth --
 *
 *	Copies the stack depth from the compile environment to a basic block.
 *
 * Side effects:
 *	Current and max stack depth in the current basic block are adjusted.
 *
 * This procedure is called on return from invoking the compiler for the
 * 'eval' and 'expr' operations. It adjusts the stack depth of the current
 * basic block to reflect the stack required by the just-compiled code.
 *
 *-----------------------------------------------------------------------------
 */

static void
SyncStackDepth(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    BasicBlock* curr_bb = assemEnvPtr->curr_bb;
				/* Current basic block */
    int maxStackDepth = curr_bb->finalStackDepth + envPtr->maxStackDepth;
				/* Max stack depth in the basic block */

    if (maxStackDepth > curr_bb->maxStackDepth) {
	curr_bb->maxStackDepth = maxStackDepth;
    }
    curr_bb->finalStackDepth += envPtr->currStackDepth;
}

/*
 *-----------------------------------------------------------------------------
 *
 * MoveExceptionRangesToBasicBlock --
 *
 *	Removes exception ranges that were created by compiling an embedded
 *	script from the CompileEnv, and stores them in the BasicBlock. They
 *	will be reinstalled, at the correct stack depth, after control flow
 *	analysis is complete on the assembly code.
 *
 *-----------------------------------------------------------------------------
 */

static void
MoveExceptionRangesToBasicBlock(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    int savedCodeIndex,		/* Start of the embedded code */
    int savedExceptArrayNext)	/* Saved index of the end of the exception
				 * range array */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    BasicBlock* curr_bb = assemEnvPtr->curr_bb;
				/* Current basic block */
    int exceptionCount = envPtr->exceptArrayNext - savedExceptArrayNext;
				/* Number of ranges that must be moved */
    int i;

    if (exceptionCount == 0) {
	/* Nothing to do */
	return;
    }

    /*
     * Save the exception ranges in the basic block. They will be re-added at
     * the conclusion of assembly; at this time, the INST_BEGIN_CATCH
     * instructions in the block will be adjusted from whatever range indices
     * they have [savedExceptArrayNext .. envPtr->exceptArrayNext) to the
     * indices that the exceptions acquire. The saved exception ranges are
     * converted to a relative nesting depth. The depth will be recomputed
     * once flow analysis has determined the actual stack depth of the block.
     */

    DEBUG_PRINT("basic block %p has %d exceptions starting at %d\n",
	    curr_bb, exceptionCount, savedExceptArrayNext);
    curr_bb->foreignExceptionBase = savedExceptArrayNext;
    curr_bb->foreignExceptionCount = exceptionCount;
    curr_bb->foreignExceptions =
	    ckalloc(exceptionCount * sizeof(ExceptionRange));
    memcpy(curr_bb->foreignExceptions,
	    envPtr->exceptArrayPtr + savedExceptArrayNext,
	    exceptionCount * sizeof(ExceptionRange));
    for (i = 0; i < exceptionCount; ++i) {
	curr_bb->foreignExceptions[i].nestingLevel -= envPtr->exceptDepth;
    }
    envPtr->exceptArrayNext = savedExceptArrayNext;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CreateMirrorJumpTable --
 *
 *	Makes a jump table with comparison values and assembly code labels.
 *
 * Results:
 *	Returns a standard Tcl status, with an error message in the
 *	interpreter on error.
 *
 * Side effects:
 *	Initializes the jump table pointer in the current basic block to a
 *	JumptableInfo. The keys in the JumptableInfo are the comparison
 *	strings. The values, instead of being jump displacements, are
 *	Tcl_Obj's with the code labels.
 */

static int
CreateMirrorJumpTable(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    Tcl_Obj* jumps)		/* List of alternating keywords and labels */
{
    int objc;			/* Number of elements in the 'jumps' list */
    Tcl_Obj** objv;		/* Pointers to the elements in the list */
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    BasicBlock* bbPtr = assemEnvPtr->curr_bb;
				/* Current basic block */
    JumptableInfo* jtPtr;
    Tcl_HashTable* jtHashPtr;	/* Hashtable in the JumptableInfo */
    Tcl_HashEntry* hashEntry;	/* Entry for a key in the hashtable */
    int isNew;			/* Flag==1 if the key is not yet in the
				 * table. */
    int i;

    if (Tcl_ListObjGetElements(interp, jumps, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc % 2 != 0) {
	if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "jump table must have an even number of list elements",
		    -1));
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "BADJUMPTABLE", NULL);
	}
	return TCL_ERROR;
    }

    /*
     * Allocate the jumptable.
     */

    jtPtr = ckalloc(sizeof(JumptableInfo));
    jtHashPtr = &jtPtr->hashTable;
    Tcl_InitHashTable(jtHashPtr, TCL_STRING_KEYS);

    /*
     * Fill the keys and labels into the table.
     */

    DEBUG_PRINT("jump table {\n");
    for (i = 0; i < objc; i+=2) {
	DEBUG_PRINT("  %s -> %s\n", Tcl_GetString(objv[i]),
		Tcl_GetString(objv[i+1]));
	hashEntry = Tcl_CreateHashEntry(jtHashPtr, Tcl_GetString(objv[i]),
		&isNew);
	if (!isNew) {
	    if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"duplicate entry in jump table for \"%s\"",
			Tcl_GetString(objv[i])));
		Tcl_SetErrorCode(interp, "TCL", "ASSEM", "DUPJUMPTABLEENTRY");
		DeleteMirrorJumpTable(jtPtr);
		return TCL_ERROR;
	    }
	}
	Tcl_SetHashValue(hashEntry, objv[i+1]);
	Tcl_IncrRefCount(objv[i+1]);
    }
    DEBUG_PRINT("}\n");

    /*
     * Put the mirror jumptable in the basic block struct.
     */

    bbPtr->jtPtr = jtPtr;
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DeleteMirrorJumpTable --
 *
 *	Cleans up a jump table when the basic block is deleted.
 *
 *-----------------------------------------------------------------------------
 */

static void
DeleteMirrorJumpTable(
    JumptableInfo* jtPtr)
{
    Tcl_HashTable* jtHashPtr = &jtPtr->hashTable;
				/* Hash table pointer */
    Tcl_HashSearch search;	/* Hash search control */
    Tcl_HashEntry* entry;	/* Hash table entry containing a jump label */
    Tcl_Obj* label;		/* Jump label from the hash table */

    for (entry = Tcl_FirstHashEntry(jtHashPtr, &search);
	    entry != NULL;
	    entry = Tcl_NextHashEntry(&search)) {
	label = Tcl_GetHashValue(entry);
	Tcl_DecrRefCount(label);
	Tcl_SetHashValue(entry, NULL);
    }
    Tcl_DeleteHashTable(jtHashPtr);
    ckfree(jtPtr);
}

/*
 *-----------------------------------------------------------------------------
 *
 * GetNextOperand --
 *
 *	Retrieves the next operand in sequence from an assembly instruction,
 *	and makes sure that its value is known at compile time.
 *
 * Results:
 *	If successful, returns TCL_OK and leaves a Tcl_Obj with the operand
 *	text in *operandObjPtr. In case of failure, returns TCL_ERROR and
 *	leaves *operandObjPtr untouched.
 *
 * Side effects:
 *	Advances *tokenPtrPtr around the token just processed.
 *
 *-----------------------------------------------------------------------------
 */

static int
GetNextOperand(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    Tcl_Token** tokenPtrPtr,	/* INPUT/OUTPUT: Pointer to the token holding
				 * the operand */
    Tcl_Obj** operandObjPtr)	/* OUTPUT: Tcl object holding the operand text
				 * with \-substitutions done. */
{
    Tcl_Interp* interp = (Tcl_Interp*) assemEnvPtr->envPtr->iPtr;
    Tcl_Obj* operandObj = Tcl_NewObj();

    if (!TclWordKnownAtCompileTime(*tokenPtrPtr, operandObj)) {
	Tcl_DecrRefCount(operandObj);
	if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "assembly code may not contain substitutions", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "NOSUBST", NULL);
	}
	return TCL_ERROR;
    }
    *tokenPtrPtr = TokenAfter(*tokenPtrPtr);
    Tcl_IncrRefCount(operandObj);
    *operandObjPtr = operandObj;
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * GetBooleanOperand --
 *
 *	Retrieves a Boolean operand from the input stream and advances
 *	the token pointer.
 *
 * Results:
 *	Returns a standard Tcl result (with an error message in the
 *	interpreter on failure).
 *
 * Side effects:
 *	Stores the Boolean value in (*result) and advances (*tokenPtrPtr)
 *	to the next token.
 *
 *-----------------------------------------------------------------------------
 */

static int
GetBooleanOperand(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    Tcl_Token** tokenPtrPtr,	/* Current token from the parser */
    int* result)		/* OUTPUT: Integer extracted from the token */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    Tcl_Token* tokenPtr = *tokenPtrPtr;
				/* INOUT: Pointer to the next token in the
				 * source code */
    Tcl_Obj* intObj;		/* Integer from the source code */
    int status;			/* Tcl status return */

    /*
     * Extract the next token as a string.
     */

    if (GetNextOperand(assemEnvPtr, tokenPtrPtr, &intObj) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Convert to an integer, advance to the next token and return.
     */

    status = Tcl_GetBooleanFromObj(interp, intObj, result);
    Tcl_DecrRefCount(intObj);
    *tokenPtrPtr = TokenAfter(tokenPtr);
    return status;
}

/*
 *-----------------------------------------------------------------------------
 *
 * GetIntegerOperand --
 *
 *	Retrieves an integer operand from the input stream and advances the
 *	token pointer.
 *
 * Results:
 *	Returns a standard Tcl result (with an error message in the
 *	interpreter on failure).
 *
 * Side effects:
 *	Stores the integer value in (*result) and advances (*tokenPtrPtr) to
 *	the next token.
 *
 *-----------------------------------------------------------------------------
 */

static int
GetIntegerOperand(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    Tcl_Token** tokenPtrPtr,	/* Current token from the parser */
    int* result)		/* OUTPUT: Integer extracted from the token */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    Tcl_Token* tokenPtr = *tokenPtrPtr;
				/* INOUT: Pointer to the next token in the
				 * source code */
    Tcl_Obj* intObj;		/* Integer from the source code */
    int status;			/* Tcl status return */

    /*
     * Extract the next token as a string.
     */

    if (GetNextOperand(assemEnvPtr, tokenPtrPtr, &intObj) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Convert to an integer, advance to the next token and return.
     */

    status = Tcl_GetIntFromObj(interp, intObj, result);
    Tcl_DecrRefCount(intObj);
    *tokenPtrPtr = TokenAfter(tokenPtr);
    return status;
}

/*
 *-----------------------------------------------------------------------------
 *
 * GetListIndexOperand --
 *
 *	Gets the value of an operand intended to serve as a list index.
 *
 * Results:
 *	Returns a standard Tcl result: TCL_OK if the parse is successful and
 *	TCL_ERROR (with an appropriate error message) if the parse fails.
 *
 * Side effects:
 *	Stores the list index at '*index'. Values between -1 and 0x7fffffff
 *	have their natural meaning; values between -2 and -0x80000000
 *	represent 'end-2-N'.
 *
 *-----------------------------------------------------------------------------
 */

static int
GetListIndexOperand(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    Tcl_Token** tokenPtrPtr,	/* Current token from the parser */
    int* result)		/* OUTPUT: Integer extracted from the token */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    Tcl_Token* tokenPtr = *tokenPtrPtr;
				/* INOUT: Pointer to the next token in the
				 * source code */
    Tcl_Obj *value;
    int status;

    /* General operand validity check */
    if (GetNextOperand(assemEnvPtr, tokenPtrPtr, &value) != TCL_OK) {
	return TCL_ERROR;
    }
     
    /* Convert to an integer, advance to the next token and return. */
    /*
     * NOTE: Indexing a list with an index before it yields the
     * same result as indexing after it, and might be more easily portable
     * when list size limits grow.
     */
    status = TclIndexEncode(interp, value,
	    TCL_INDEX_BEFORE,TCL_INDEX_BEFORE, result);

    Tcl_DecrRefCount(value);
    *tokenPtrPtr = TokenAfter(tokenPtr);
    return status;
}

/*
 *-----------------------------------------------------------------------------
 *
 * FindLocalVar --
 *
 *	Gets the name of a local variable from the input stream and advances
 *	the token pointer.
 *
 * Results:
 *	Returns the LVT index of the local variable.  Returns -1 if the
 *	variable is non-local, not known at compile time, or cannot be
 *	installed in the LVT (leaving an error message in the interpreter
 *	result if necessary).
 *
 * Side effects:
 *	Advances the token pointer.  May define a new LVT slot if the variable
 *	has not yet been seen and the execution context allows for it.
 *
 *-----------------------------------------------------------------------------
 */

static int
FindLocalVar(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    Tcl_Token** tokenPtrPtr)
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    Tcl_Token* tokenPtr = *tokenPtrPtr;
				/* INOUT: Pointer to the next token in the
				 * source code. */
    Tcl_Obj* varNameObj;	/* Name of the variable */
    const char* varNameStr;
    int varNameLen;
    int localVar;		/* Index of the variable in the LVT */

    if (GetNextOperand(assemEnvPtr, tokenPtrPtr, &varNameObj) != TCL_OK) {
	return -1;
    }
    varNameStr = Tcl_GetStringFromObj(varNameObj, &varNameLen);
    if (CheckNamespaceQualifiers(interp, varNameStr, varNameLen)) {
	Tcl_DecrRefCount(varNameObj);
	return -1;
    }
    localVar = TclFindCompiledLocal(varNameStr, varNameLen, 1, envPtr);
    Tcl_DecrRefCount(varNameObj);
    if (localVar == -1) {
	if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "cannot use this instruction to create a variable"
		    " in a non-proc context", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "LVT", NULL);
	}
	return -1;
    }
    *tokenPtrPtr = TokenAfter(tokenPtr);
    return localVar;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CheckNamespaceQualifiers --
 *
 *	Verify that a variable name has no namespace qualifiers before
 *	attempting to install it in the LVT.
 *
 * Results:
 *	On success, returns TCL_OK. On failure, returns TCL_ERROR and stores
 *	an error message in the interpreter result.
 *
 *-----------------------------------------------------------------------------
 */

static int
CheckNamespaceQualifiers(
    Tcl_Interp* interp,		/* Tcl interpreter for error reporting */
    const char* name,		/* Variable name to check */
    int nameLen)		/* Length of the variable */
{
    const char* p;

    for (p = name; p+2 < name+nameLen;  p++) {
	if ((*p == ':') && (p[1] == ':')) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "variable \"%s\" is not local", name));
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "NONLOCAL", name, NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CheckOneByte --
 *
 *	Verify that a constant fits in a single byte in the instruction
 *	stream.
 *
 * Results:
 *	On success, returns TCL_OK. On failure, returns TCL_ERROR and stores
 *	an error message in the interpreter result.
 *
 * This code is here primarily to verify that instructions like INCR_SCALAR1
 * are possible on a given local variable. The fact that there is no
 * INCR_SCALAR4 is puzzling.
 *
 *-----------------------------------------------------------------------------
 */

static int
CheckOneByte(
    Tcl_Interp* interp,		/* Tcl interpreter for error reporting */
    int value)			/* Value to check */
{
    Tcl_Obj* result;		/* Error message */

    if (value < 0 || value > 0xff) {
	result = Tcl_NewStringObj("operand does not fit in one byte", -1);
	Tcl_SetObjResult(interp, result);
	Tcl_SetErrorCode(interp, "TCL", "ASSEM", "1BYTE", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CheckSignedOneByte --
 *
 *	Verify that a constant fits in a single signed byte in the instruction
 *	stream.
 *
 * Results:
 *	On success, returns TCL_OK. On failure, returns TCL_ERROR and stores
 *	an error message in the interpreter result.
 *
 * This code is here primarily to verify that instructions like INCR_SCALAR1
 * are possible on a given local variable. The fact that there is no
 * INCR_SCALAR4 is puzzling.
 *
 *-----------------------------------------------------------------------------
 */

static int
CheckSignedOneByte(
    Tcl_Interp* interp,		/* Tcl interpreter for error reporting */
    int value)			/* Value to check */
{
    Tcl_Obj* result;		/* Error message */

    if (value > 0x7f || value < -0x80) {
	result = Tcl_NewStringObj("operand does not fit in one byte", -1);
	Tcl_SetObjResult(interp, result);
	Tcl_SetErrorCode(interp, "TCL", "ASSEM", "1BYTE", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CheckNonNegative --
 *
 *	Verify that a constant is nonnegative
 *
 * Results:
 *	On success, returns TCL_OK. On failure, returns TCL_ERROR and stores
 *	an error message in the interpreter result.
 *
 * This code is here primarily to verify that instructions like INCR_INVOKE
 * are consuming a positive number of operands
 *
 *-----------------------------------------------------------------------------
 */

static int
CheckNonNegative(
    Tcl_Interp* interp,		/* Tcl interpreter for error reporting */
    int value)			/* Value to check */
{
    Tcl_Obj* result;		/* Error message */

    if (value < 0) {
	result = Tcl_NewStringObj("operand must be nonnegative", -1);
	Tcl_SetObjResult(interp, result);
	Tcl_SetErrorCode(interp, "TCL", "ASSEM", "NONNEGATIVE", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CheckStrictlyPositive --
 *
 *	Verify that a constant is positive
 *
 * Results:
 *	On success, returns TCL_OK. On failure, returns TCL_ERROR and
 *	stores an error message in the interpreter result.
 *
 * This code is here primarily to verify that instructions like INCR_INVOKE
 * are consuming a positive number of operands
 *
 *-----------------------------------------------------------------------------
 */

static int
CheckStrictlyPositive(
    Tcl_Interp* interp,		/* Tcl interpreter for error reporting */
    int value)			/* Value to check */
{
    Tcl_Obj* result;		/* Error message */

    if (value <= 0) {
	result = Tcl_NewStringObj("operand must be positive", -1);
	Tcl_SetObjResult(interp, result);
	Tcl_SetErrorCode(interp, "TCL", "ASSEM", "POSITIVE", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DefineLabel --
 *
 *	Defines a label appearing in the assembly sequence.
 *
 * Results:
 *	Returns a standard Tcl result. Returns TCL_OK and an empty result if
 *	the definition succeeds; returns TCL_ERROR and an appropriate message
 *	if a duplicate definition is found.
 *
 *-----------------------------------------------------------------------------
 */

static int
DefineLabel(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    const char* labelName)	/* Label being defined */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    Tcl_HashEntry* entry;	/* Label's entry in the symbol table */
    int isNew;			/* Flag == 1 iff the label was previously
				 * undefined */

    /* TODO - This can now be simplified! */

    StartBasicBlock(assemEnvPtr, BB_FALLTHRU, NULL);

    /*
     * Look up the newly-defined label in the symbol table.
     */

    entry = Tcl_CreateHashEntry(&assemEnvPtr->labelHash, labelName, &isNew);
    if (!isNew) {
	/*
	 * This is a duplicate label.
	 */

	if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "duplicate definition of label \"%s\"", labelName));
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "DUPLABEL", labelName,
		    NULL);
	}
	return TCL_ERROR;
    }

    /*
     * This is the first appearance of the label in the code.
     */

    Tcl_SetHashValue(entry, assemEnvPtr->curr_bb);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * StartBasicBlock --
 *
 *	Starts a new basic block when a label or jump is encountered.
 *
 * Results:
 *	Returns a pointer to the BasicBlock structure of the new
 *	basic block.
 *
 *-----------------------------------------------------------------------------
 */

static BasicBlock*
StartBasicBlock(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    int flags,			/* Flags to apply to the basic block being
				 * closed, if there is one. */
    Tcl_Obj* jumpLabel)		/* Label of the location that the block jumps
				 * to, or NULL if the block does not jump */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    BasicBlock* newBB;		/* BasicBlock structure for the new block */
    BasicBlock* currBB = assemEnvPtr->curr_bb;

    /*
     * Coalesce zero-length blocks.
     */

    if (currBB->startOffset == envPtr->codeNext - envPtr->codeStart) {
	currBB->startLine = assemEnvPtr->cmdLine;
	return currBB;
    }

    /*
     * Make the new basic block.
     */

    newBB = AllocBB(assemEnvPtr);

    /*
     * Record the jump target if there is one.
     */

    currBB->jumpTarget = jumpLabel;
    if (jumpLabel != NULL) {
	Tcl_IncrRefCount(currBB->jumpTarget);
    }

    /*
     * Record the fallthrough if there is one.
     */

    currBB->flags |= flags;

    /*
     * Record the successor block.
     */

    currBB->successor1 = newBB;
    assemEnvPtr->curr_bb = newBB;
    return newBB;
}

/*
 *-----------------------------------------------------------------------------
 *
 * AllocBB --
 *
 *	Allocates a new basic block
 *
 * Results:
 *	Returns a pointer to the newly allocated block, which is initialized
 *	to contain no code and begin at the current instruction pointer.
 *
 *-----------------------------------------------------------------------------
 */

static BasicBlock *
AllocBB(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
    BasicBlock *bb = ckalloc(sizeof(BasicBlock));

    bb->originalStartOffset =
	    bb->startOffset = envPtr->codeNext - envPtr->codeStart;
    bb->startLine = assemEnvPtr->cmdLine + 1;
    bb->jumpOffset = -1;
    bb->jumpLine = -1;
    bb->prevPtr = assemEnvPtr->curr_bb;
    bb->predecessor = NULL;
    bb->successor1 = NULL;
    bb->jumpTarget = NULL;
    bb->initialStackDepth = 0;
    bb->minStackDepth = 0;
    bb->maxStackDepth = 0;
    bb->finalStackDepth = 0;
    bb->catchDepth = 0;
    bb->enclosingCatch = NULL;
    bb->foreignExceptionBase = -1;
    bb->foreignExceptionCount = 0;
    bb->foreignExceptions = NULL;
    bb->jtPtr = NULL;
    bb->flags = 0;

    return bb;
}

/*
 *-----------------------------------------------------------------------------
 *
 * FinishAssembly --
 *
 *	Postprocessing after all bytecode has been generated for a block of
 *	assembly code.
 *
 * Results:
 *	Returns a standard Tcl result, with an error message left in the
 *	interpreter if appropriate.
 *
 * Side effects:
 *	The program is checked to see if any undefined labels remain.  The
 *	initial stack depth of all the basic blocks in the flow graph is
 *	calculated and saved.  The stack balance on exit is computed, checked
 *	and saved.
 *
 *-----------------------------------------------------------------------------
 */

static int
FinishAssembly(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    int mustMove;		/* Amount by which the code needs to be grown
				 * because of expanding jumps */

    /*
     * Resolve the targets of all jumps and determine whether code needs to be
     * moved around.
     */

    if (CalculateJumpRelocations(assemEnvPtr, &mustMove)) {
	return TCL_ERROR;
    }

    /*
     * Move the code if necessary.
     */

    if (mustMove) {
	MoveCodeForJumps(assemEnvPtr, mustMove);
    }

    /*
     * Resolve jump target labels to bytecode offsets.
     */

    FillInJumpOffsets(assemEnvPtr);

    /*
     * Label each basic block with its catch context. Quit on inconsistency.
     */

    if (ProcessCatches(assemEnvPtr) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Make sure that no block accessible from a catch's error exit that hasn't
     * popped the exception stack can throw an exception.
     */

    if (CheckForThrowInWrongContext(assemEnvPtr) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Compute stack balance throughout the program.
     */

    if (CheckStack(assemEnvPtr) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * TODO - Check for unreachable code. Or maybe not; unreachable code is
     * Mostly Harmless.
     */

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CalculateJumpRelocations --
 *
 *	Calculate any movement that has to be done in the assembly code to
 *	expand JUMP1 instructions to JUMP4 (because they jump more than a
 *	1-byte range).
 *
 * Results:
 *	Returns a standard Tcl result, with an appropriate error message if
 *	anything fails.
 *
 * Side effects:
 *	Sets the 'startOffset' pointer in every basic block to the new origin
 *	of the block, and turns off JUMP1 flags on instructions that must be
 *	expanded (and adjusts them to the corresponding JUMP4's).  Does *not*
 *	store the jump offsets at this point.
 *
 *	Sets *mustMove to 1 if and only if at least one instruction changed
 *	size so the code must be moved.
 *
 *	As a side effect, also checks for undefined labels and reports them.
 *
 *-----------------------------------------------------------------------------
 */

static int
CalculateJumpRelocations(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    int* mustMove)		/* OUTPUT: Number of bytes that have been
				 * added to the code */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    BasicBlock* bbPtr;		/* Pointer to a basic block being checked */
    Tcl_HashEntry* entry;	/* Exit label's entry in the symbol table */
    BasicBlock* jumpTarget;	/* Basic block where the jump goes */
    int motion;			/* Amount by which the code has expanded */
    int offset;			/* Offset in the bytecode from a jump
				 * instruction to its target */
    unsigned opcode;		/* Opcode in the bytecode being adjusted */

    /*
     * Iterate through basic blocks as long as a change results in code
     * expansion.
     */

    *mustMove = 0;
    do {
	motion = 0;
	for (bbPtr = assemEnvPtr->head_bb;
		bbPtr != NULL;
		bbPtr = bbPtr->successor1) {
	    /*
	     * Advance the basic block start offset by however many bytes we
	     * have inserted in the code up to this point
	     */

	    bbPtr->startOffset += motion;

	    /*
	     * If the basic block references a label (and hence performs a
	     * jump), find the location of the label. Report an error if the
	     * label is missing.
	     */

	    if (bbPtr->jumpTarget != NULL) {
		entry = Tcl_FindHashEntry(&assemEnvPtr->labelHash,
			Tcl_GetString(bbPtr->jumpTarget));
		if (entry == NULL) {
		    ReportUndefinedLabel(assemEnvPtr, bbPtr,
			    bbPtr->jumpTarget);
		    return TCL_ERROR;
		}

		/*
		 * If the instruction is a JUMP1, turn it into a JUMP4 if its
		 * target is out of range.
		 */

		jumpTarget = Tcl_GetHashValue(entry);
		if (bbPtr->flags & BB_JUMP1) {
		    offset = jumpTarget->startOffset
			    - (bbPtr->jumpOffset + motion);
		    if (offset < -0x80 || offset > 0x7f) {
			opcode = TclGetUInt1AtPtr(envPtr->codeStart
				+ bbPtr->jumpOffset);
			++opcode;
			TclStoreInt1AtPtr(opcode,
				envPtr->codeStart + bbPtr->jumpOffset);
			motion += 3;
			bbPtr->flags &= ~BB_JUMP1;
		    }
		}
	    }

	    /*
	     * If the basic block references a jump table, that doesn't affect
	     * the code locations, but resolve the labels now, and store basic
	     * block pointers in the jumptable hash.
	     */

	    if (bbPtr->flags & BB_JUMPTABLE) {
		if (CheckJumpTableLabels(assemEnvPtr, bbPtr) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	}
	*mustMove += motion;
    } while (motion != 0);

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CheckJumpTableLabels --
 *
 *	Make sure that all the labels in a jump table are defined.
 *
 * Results:
 *	Returns TCL_OK if they are, TCL_ERROR if they aren't.
 *
 *-----------------------------------------------------------------------------
 */

static int
CheckJumpTableLabels(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    BasicBlock* bbPtr)		/* Basic block that ends in a jump table */
{
    Tcl_HashTable* symHash = &bbPtr->jtPtr->hashTable;
				/* Hash table with the symbols */
    Tcl_HashSearch search;	/* Hash table iterator */
    Tcl_HashEntry* symEntryPtr;	/* Hash entry for the symbols */
    Tcl_Obj* symbolObj;		/* Jump target */
    Tcl_HashEntry* valEntryPtr;	/* Hash entry for the resolutions */

    /*
     * Look up every jump target in the jump hash.
     */

    DEBUG_PRINT("check jump table labels %p {\n", bbPtr);
    for (symEntryPtr = Tcl_FirstHashEntry(symHash, &search);
	    symEntryPtr != NULL;
	    symEntryPtr = Tcl_NextHashEntry(&search)) {
	symbolObj = Tcl_GetHashValue(symEntryPtr);
	valEntryPtr = Tcl_FindHashEntry(&assemEnvPtr->labelHash,
		Tcl_GetString(symbolObj));
	DEBUG_PRINT("  %s -> %s (%d)\n",
		(char*) Tcl_GetHashKey(symHash, symEntryPtr),
		Tcl_GetString(symbolObj), (valEntryPtr != NULL));
	if (valEntryPtr == NULL) {
	    ReportUndefinedLabel(assemEnvPtr, bbPtr, symbolObj);
	    return TCL_ERROR;
	}
    }
    DEBUG_PRINT("}\n");
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ReportUndefinedLabel --
 *
 *	Report that a basic block refers to an undefined jump label
 *
 * Side effects:
 *	Stores an error message, error code, and line number information in
 *	the assembler's Tcl interpreter.
 *
 *-----------------------------------------------------------------------------
 */

static void
ReportUndefinedLabel(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    BasicBlock* bbPtr,		/* Basic block that contains the undefined
				 * label */
    Tcl_Obj* jumpTarget)	/* Label of a jump target */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */

    if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"undefined label \"%s\"", Tcl_GetString(jumpTarget)));
	Tcl_SetErrorCode(interp, "TCL", "ASSEM", "NOLABEL",
		Tcl_GetString(jumpTarget), NULL);
	Tcl_SetErrorLine(interp, bbPtr->jumpLine);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * MoveCodeForJumps --
 *
 *	Move bytecodes in memory to accommodate JUMP1 instructions that have
 *	expanded to become JUMP4's.
 *
 *-----------------------------------------------------------------------------
 */

static void
MoveCodeForJumps(
    AssemblyEnv* assemEnvPtr,	/* Assembler environment */
    int mustMove)		/* Number of bytes of added code */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    BasicBlock* bbPtr;		/* Pointer to a basic block being checked */
    int topOffset;		/* Bytecode offset of the following basic
				 * block before code motion */

    /*
     * Make sure that there is enough space in the bytecode array to
     * accommodate the expanded code.
     */

    while (envPtr->codeEnd < envPtr->codeNext + mustMove) {
	TclExpandCodeArray(envPtr);
    }

    /*
     * Iterate through the bytecodes in reverse order, and move them upward to
     * their new homes.
     */

    topOffset = envPtr->codeNext - envPtr->codeStart;
    for (bbPtr = assemEnvPtr->curr_bb; bbPtr != NULL; bbPtr = bbPtr->prevPtr) {
	DEBUG_PRINT("move code from %d to %d\n",
		bbPtr->originalStartOffset, bbPtr->startOffset);
	memmove(envPtr->codeStart + bbPtr->startOffset,
		envPtr->codeStart + bbPtr->originalStartOffset,
		topOffset - bbPtr->originalStartOffset);
	topOffset = bbPtr->originalStartOffset;
	bbPtr->jumpOffset += (bbPtr->startOffset - bbPtr->originalStartOffset);
    }
    envPtr->codeNext += mustMove;
}

/*
 *-----------------------------------------------------------------------------
 *
 * FillInJumpOffsets --
 *
 *	Fill in the final offsets of all jump instructions once bytecode
 *	locations have been completely determined.
 *
 *-----------------------------------------------------------------------------
 */

static void
FillInJumpOffsets(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    BasicBlock* bbPtr;		/* Pointer to a basic block being checked */
    Tcl_HashEntry* entry;	/* Hashtable entry for a jump target label */
    BasicBlock* jumpTarget;	/* Basic block where a jump goes */
    int fromOffset;		/* Bytecode location of a jump instruction */
    int targetOffset;		/* Bytecode location of a jump instruction's
				 * target */

    for (bbPtr = assemEnvPtr->head_bb;
	    bbPtr != NULL;
	    bbPtr = bbPtr->successor1) {
	if (bbPtr->jumpTarget != NULL) {
	    entry = Tcl_FindHashEntry(&assemEnvPtr->labelHash,
		    Tcl_GetString(bbPtr->jumpTarget));
	    jumpTarget = Tcl_GetHashValue(entry);
	    fromOffset = bbPtr->jumpOffset;
	    targetOffset = jumpTarget->startOffset;
	    if (bbPtr->flags & BB_JUMP1) {
		TclStoreInt1AtPtr(targetOffset - fromOffset,
			envPtr->codeStart + fromOffset + 1);
	    } else {
		TclStoreInt4AtPtr(targetOffset - fromOffset,
			envPtr->codeStart + fromOffset + 1);
	    }
	}
	if (bbPtr->flags & BB_JUMPTABLE) {
	    ResolveJumpTableTargets(assemEnvPtr, bbPtr);
	}
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * ResolveJumpTableTargets --
 *
 *	Puts bytecode addresses for the targets of a jumptable into the
 *	table
 *
 * Results:
 *	Returns TCL_OK if they are, TCL_ERROR if they aren't.
 *
 *-----------------------------------------------------------------------------
 */

static void
ResolveJumpTableTargets(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    BasicBlock* bbPtr)		/* Basic block that ends in a jump table */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_HashTable* symHash = &bbPtr->jtPtr->hashTable;
				/* Hash table with the symbols */
    Tcl_HashSearch search;	/* Hash table iterator */
    Tcl_HashEntry* symEntryPtr;	/* Hash entry for the symbols */
    Tcl_Obj* symbolObj;		/* Jump target */
    Tcl_HashEntry* valEntryPtr;	/* Hash entry for the resolutions */
    int auxDataIndex;		/* Index of the auxdata */
    JumptableInfo* realJumpTablePtr;
				/* Jump table in the actual code */
    Tcl_HashTable* realJumpHashPtr;
				/* Jump table hash in the actual code */
    Tcl_HashEntry* realJumpEntryPtr;
				/* Entry in the jump table hash in
				 * the actual code */
    BasicBlock* jumpTargetBBPtr;
				/* Basic block that the jump proceeds to */
    int junk;

    auxDataIndex = TclGetInt4AtPtr(envPtr->codeStart + bbPtr->jumpOffset + 1);
    DEBUG_PRINT("bbPtr = %p jumpOffset = %d auxDataIndex = %d\n",
	    bbPtr, bbPtr->jumpOffset, auxDataIndex);
    realJumpTablePtr = TclFetchAuxData(envPtr, auxDataIndex);
    realJumpHashPtr = &realJumpTablePtr->hashTable;

    /*
     * Look up every jump target in the jump hash.
     */

    DEBUG_PRINT("resolve jump table {\n");
    for (symEntryPtr = Tcl_FirstHashEntry(symHash, &search);
	    symEntryPtr != NULL;
	    symEntryPtr = Tcl_NextHashEntry(&search)) {
	symbolObj = Tcl_GetHashValue(symEntryPtr);
	DEBUG_PRINT("     symbol %s\n", Tcl_GetString(symbolObj));

	valEntryPtr = Tcl_FindHashEntry(&assemEnvPtr->labelHash,
		Tcl_GetString(symbolObj));
	jumpTargetBBPtr = Tcl_GetHashValue(valEntryPtr);

	realJumpEntryPtr = Tcl_CreateHashEntry(realJumpHashPtr,
		Tcl_GetHashKey(symHash, symEntryPtr), &junk);
	DEBUG_PRINT("  %s -> %s -> bb %p (pc %d)    hash entry %p\n",
		(char*) Tcl_GetHashKey(symHash, symEntryPtr),
		Tcl_GetString(symbolObj), jumpTargetBBPtr,
		jumpTargetBBPtr->startOffset, realJumpEntryPtr);

	Tcl_SetHashValue(realJumpEntryPtr,
		INT2PTR(jumpTargetBBPtr->startOffset - bbPtr->jumpOffset));
    }
    DEBUG_PRINT("}\n");
}

/*
 *-----------------------------------------------------------------------------
 *
 * CheckForThrowInWrongContext --
 *
 *	Verify that no beginCatch/endCatch sequence can throw an exception
 *	after an original exception is caught and before its exception context
 *	is removed from the stack.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Stores an appropriate error message in the interpreter as needed.
 *
 *-----------------------------------------------------------------------------
 */

static int
CheckForThrowInWrongContext(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    BasicBlock* blockPtr;	/* Current basic block */

    /*
     * Walk through the basic blocks in turn, checking all the ones that have
     * caught an exception and not disposed of it properly.
     */

    for (blockPtr = assemEnvPtr->head_bb;
	    blockPtr != NULL;
	    blockPtr = blockPtr->successor1) {
	if (blockPtr->catchState == BBCS_CAUGHT) {
	    /*
	     * Walk through the instructions in the basic block.
	     */

	    if (CheckNonThrowingBlock(assemEnvPtr, blockPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CheckNonThrowingBlock --
 *
 *	Check that a basic block cannot throw an exception.
 *
 * Results:
 *	Returns TCL_ERROR if the block cannot be proven to be nonthrowing.
 *
 * Side effects:
 *	Stashes an error message in the interpreter result.
 *
 *-----------------------------------------------------------------------------
 */

static int
CheckNonThrowingBlock(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    BasicBlock* blockPtr)	/* Basic block where exceptions are not
				 * allowed */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    BasicBlock* nextPtr;	/* Pointer to the succeeding basic block */
    int offset;			/* Bytecode offset of the current
				 * instruction */
    int bound;			/* Bytecode offset following the last
				 * instruction of the block. */
    unsigned char opcode;	/* Current bytecode instruction */

    /*
     * Determine where in the code array the basic block ends.
     */

    nextPtr = blockPtr->successor1;
    if (nextPtr == NULL) {
	bound = envPtr->codeNext - envPtr->codeStart;
    } else {
	bound = nextPtr->startOffset;
    }

    /*
     * Walk through the instructions of the block.
     */

    offset = blockPtr->startOffset;
    while (offset < bound) {
	/*
	 * Determine whether an instruction is nonthrowing.
	 */

	opcode = (envPtr->codeStart)[offset];
	if (BytecodeMightThrow(opcode)) {
	    /*
	     * Report an error for a throw in the wrong context.
	     */

	    if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"\"%s\" instruction may not appear in "
			"a context where an exception has been "
			"caught and not disposed of.",
			tclInstructionTable[opcode].name));
		Tcl_SetErrorCode(interp, "TCL", "ASSEM", "BADTHROW", NULL);
		AddBasicBlockRangeToErrorInfo(assemEnvPtr, blockPtr);
	    }
	    return TCL_ERROR;
	}
	offset += tclInstructionTable[opcode].numBytes;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * BytecodeMightThrow --
 *
 *	Tests if a given bytecode instruction might throw an exception.
 *
 * Results:
 *	Returns 1 if the bytecode might throw an exception, 0 if the
 *	instruction is known never to throw.
 *
 *-----------------------------------------------------------------------------
 */

static int
BytecodeMightThrow(
    unsigned char opcode)
{
    /*
     * Binary search on the non-throwing bytecode list.
     */

    int min = 0;
    int max = sizeof(NonThrowingByteCodes) - 1;
    int mid;
    unsigned char c;

    while (max >= min) {
	mid = (min + max) / 2;
	c = NonThrowingByteCodes[mid];
	if (opcode < c) {
	    max = mid-1;
	} else if (opcode > c) {
	    min = mid+1;
	} else {
	    /*
	     * Opcode is nonthrowing.
	     */

	    return 0;
	}
    }

    return 1;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CheckStack --
 *
 *	Audit stack usage in a block of assembly code.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Updates stack depth on entry for all basic blocks in the flowgraph.
 *	Calculates the max stack depth used in the program, and updates the
 *	compilation environment to reflect it.
 *
 *-----------------------------------------------------------------------------
 */

static int
CheckStack(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    int maxDepth;		/* Maximum stack depth overall */

    /*
     * Checking the head block will check all the other blocks recursively.
     */

    assemEnvPtr->maxDepth = 0;
    if (StackCheckBasicBlock(assemEnvPtr, assemEnvPtr->head_bb, NULL,
	    0) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Post the max stack depth back to the compilation environment.
     */

    maxDepth = assemEnvPtr->maxDepth + envPtr->currStackDepth;
    if (maxDepth > envPtr->maxStackDepth) {
	envPtr->maxStackDepth = maxDepth;
    }

    /*
     * If the exit is reachable, make sure that the program exits with 1
     * operand on the stack.
     */

    if (StackCheckExit(assemEnvPtr) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Reset the visited state on all basic blocks.
     */

    ResetVisitedBasicBlocks(assemEnvPtr);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * StackCheckBasicBlock --
 *
 *	Checks stack consumption for a basic block (and recursively for its
 *	successors).
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Updates initial stack depth for the basic block and its successors.
 *	(Final and maximum stack depth are relative to initial, and are not
 *	touched).
 *
 * This procedure eventually checks, for the entire flow graph, whether stack
 * balance is consistent.  It is an error for a given basic block to be
 * reachable along multiple flow paths with different stack depths.
 *
 *-----------------------------------------------------------------------------
 */

static int
StackCheckBasicBlock(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    BasicBlock* blockPtr,	/* Pointer to the basic block being checked */
    BasicBlock* predecessor,	/* Pointer to the block that passed control to
				 * this one. */
    int initialStackDepth)	/* Stack depth on entry to the block */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    BasicBlock* jumpTarget;	/* Basic block where a jump goes */
    int stackDepth;		/* Current stack depth */
    int maxDepth;		/* Maximum stack depth so far */
    int result;			/* Tcl status return */
    Tcl_HashSearch jtSearch;	/* Search structure for the jump table */
    Tcl_HashEntry* jtEntry;	/* Hash entry in the jump table */
    Tcl_Obj* targetLabel;	/* Target label from the jump table */
    Tcl_HashEntry* entry;	/* Hash entry in the label table */

    if (blockPtr->flags & BB_VISITED) {
	/*
	 * If the block is already visited, check stack depth for consistency
	 * among the paths that reach it.
	 */

	if (blockPtr->initialStackDepth == initialStackDepth) {
	    return TCL_OK;
	}
	if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "inconsistent stack depths on two execution paths", -1));

	    /*
	     * TODO - add execution trace of both paths
	     */

	    Tcl_SetErrorLine(interp, blockPtr->startLine);
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "BADSTACK", NULL);
	}
	return TCL_ERROR;
    }

    /*
     * If the block is not already visited, set the 'predecessor' link to
     * indicate how control got to it. Set the initial stack depth to the
     * current stack depth in the flow of control.
     */

    blockPtr->flags |= BB_VISITED;
    blockPtr->predecessor = predecessor;
    blockPtr->initialStackDepth = initialStackDepth;

    /*
     * Calculate minimum stack depth, and flag an error if the block
     * underflows the stack.
     */

    if (initialStackDepth + blockPtr->minStackDepth < 0) {
	if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("stack underflow", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "BADSTACK", NULL);
	    AddBasicBlockRangeToErrorInfo(assemEnvPtr, blockPtr);
	    Tcl_SetErrorLine(interp, blockPtr->startLine);
	}
	return TCL_ERROR;
    }

    /*
     * Make sure that the block doesn't try to pop below the stack level of an
     * enclosing catch.
     */

    if (blockPtr->enclosingCatch != 0 &&
	    initialStackDepth + blockPtr->minStackDepth
	    < (blockPtr->enclosingCatch->initialStackDepth
		+ blockPtr->enclosingCatch->finalStackDepth)) {
	if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "code pops stack below level of enclosing catch", -1));
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "BADSTACKINCATCH", -1);
	    AddBasicBlockRangeToErrorInfo(assemEnvPtr, blockPtr);
	    Tcl_SetErrorLine(interp, blockPtr->startLine);
	}
	return TCL_ERROR;
    }

    /*
     * Update maximum stgack depth.
     */

    maxDepth = initialStackDepth + blockPtr->maxStackDepth;
    if (maxDepth > assemEnvPtr->maxDepth) {
	assemEnvPtr->maxDepth = maxDepth;
    }

    /*
     * Calculate stack depth on exit from the block, and invoke this procedure
     * recursively to check successor blocks.
     */

    stackDepth = initialStackDepth + blockPtr->finalStackDepth;
    result = TCL_OK;
    if (blockPtr->flags & BB_FALLTHRU) {
	result = StackCheckBasicBlock(assemEnvPtr, blockPtr->successor1,
		blockPtr, stackDepth);
    }

    if (result == TCL_OK && blockPtr->jumpTarget != NULL) {
	entry = Tcl_FindHashEntry(&assemEnvPtr->labelHash,
		Tcl_GetString(blockPtr->jumpTarget));
	jumpTarget = Tcl_GetHashValue(entry);
	result = StackCheckBasicBlock(assemEnvPtr, jumpTarget, blockPtr,
		stackDepth);
    }

    /*
     * All blocks referenced in a jump table are successors.
     */

    if (blockPtr->flags & BB_JUMPTABLE) {
	for (jtEntry = Tcl_FirstHashEntry(&blockPtr->jtPtr->hashTable,
		    &jtSearch);
		result == TCL_OK && jtEntry != NULL;
		jtEntry = Tcl_NextHashEntry(&jtSearch)) {
	    targetLabel = Tcl_GetHashValue(jtEntry);
	    entry = Tcl_FindHashEntry(&assemEnvPtr->labelHash,
		    Tcl_GetString(targetLabel));
	    jumpTarget = Tcl_GetHashValue(entry);
	    result = StackCheckBasicBlock(assemEnvPtr, jumpTarget,
		    blockPtr, stackDepth);
	}
    }

    return result;
}

/*
 *-----------------------------------------------------------------------------
 *
 * StackCheckExit --
 *
 *	Makes sure that the net stack effect of an entire assembly language
 *	script is to push 1 result.
 *
 * Results:
 *	Returns a standard Tcl result, with an error message in the
 *	interpreter result if the stack is wrong.
 *
 * Side effects:
 *	If the assembly code had a net stack effect of zero, emits code to the
 *	concluding block to push a null result. In any case, updates the stack
 *	depth in the compile environment to reflect the net effect of the
 *	assembly code.
 *
 *-----------------------------------------------------------------------------
 */

static int
StackCheckExit(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    int depth;			/* Net stack effect */
    int litIndex;		/* Index in the literal pool of the empty
				 * string */
    BasicBlock* curr_bb = assemEnvPtr->curr_bb;
				/* Final basic block in the assembly */

    /*
     * Don't perform these checks if execution doesn't reach the exit (either
     * because of an infinite loop or because the only return is from the
     * middle.
     */

    if (curr_bb->flags & BB_VISITED) {
	/*
	 * Exit with no operands; push an empty one.
	 */

	depth = curr_bb->finalStackDepth + curr_bb->initialStackDepth;
	if (depth == 0) {
	    /*
	     * Emit a 'push' of the empty literal.
	     */

	    litIndex = TclRegisterNewLiteral(envPtr, "", 0);

	    /*
	     * Assumes that 'push' is at slot 0 in TalInstructionTable.
	     */

	    BBEmitInst1or4(assemEnvPtr, 0, litIndex, 0);
	    ++depth;
	}

	/*
	 * Exit with unbalanced stack.
	 */

	if (depth != 1) {
	    if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"stack is unbalanced on exit from the code (depth=%d)",
			depth));
		Tcl_SetErrorCode(interp, "TCL", "ASSEM", "BADSTACK", NULL);
	    }
	    return TCL_ERROR;
	}

	/*
	 * Record stack usage.
	 */

	envPtr->currStackDepth += depth;
    }

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ProcessCatches --
 *
 *	First pass of 'catch' processing.
 *
 * Results:
 *	Returns a standard Tcl result, with an appropriate error message if
 *	the result is TCL_ERROR.
 *
 * Side effects:
 *	Labels all basic blocks with their enclosing catches.
 *
 *-----------------------------------------------------------------------------
 */

static int
ProcessCatches(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    BasicBlock* blockPtr;	/* Pointer to a basic block */

    /*
     * Clear the catch state of all basic blocks.
     */

    for (blockPtr = assemEnvPtr->head_bb;
	    blockPtr != NULL;
	    blockPtr = blockPtr->successor1) {
	blockPtr->catchState = BBCS_UNKNOWN;
	blockPtr->enclosingCatch = NULL;
    }

    /*
     * Start the check recursively from the first basic block, which is
     * outside any exception context
     */

    if (ProcessCatchesInBasicBlock(assemEnvPtr, assemEnvPtr->head_bb,
	    NULL, BBCS_NONE, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Check for unclosed catch on exit.
     */

    if (CheckForUnclosedCatches(assemEnvPtr) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Now there's enough information to build the exception ranges.
     */

    if (BuildExceptionRanges(assemEnvPtr) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Finally, restore any exception ranges from embedded scripts.
     */

    RestoreEmbeddedExceptionRanges(assemEnvPtr);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ProcessCatchesInBasicBlock --
 *
 *	First-pass catch processing for one basic block.
 *
 * Results:
 *	Returns a standard Tcl result, with error message in the interpreter
 *	result if an error occurs.
 *
 * This procedure checks consistency of the exception context through the
 * assembler program, and records the enclosing 'catch' for every basic block.
 *
 *-----------------------------------------------------------------------------
 */

static int
ProcessCatchesInBasicBlock(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    BasicBlock* bbPtr,		/* Basic block being processed */
    BasicBlock* enclosing,	/* Start basic block of the enclosing catch */
    enum BasicBlockCatchState state,
				/* BBCS_NONE, BBCS_INCATCH, or BBCS_CAUGHT */
    int catchDepth)		/* Depth of nesting of catches */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    int result;			/* Return value from this procedure */
    BasicBlock* fallThruEnclosing;
				/* Enclosing catch if execution falls thru */
    enum BasicBlockCatchState fallThruState;
				/* Catch state of the successor block */
    BasicBlock* jumpEnclosing;	/* Enclosing catch if execution goes to jump
				 * target */
    enum BasicBlockCatchState jumpState;
				/* Catch state of the jump target */
    int changed = 0;		/* Flag == 1 iff successor blocks need to be
				 * checked because the state of this block has
				 * changed. */
    BasicBlock* jumpTarget;	/* Basic block where a jump goes */
    Tcl_HashSearch jtSearch;	/* Hash search control for a jumptable */
    Tcl_HashEntry* jtEntry;	/* Entry in a jumptable */
    Tcl_Obj* targetLabel;	/* Target label from a jumptable */
    Tcl_HashEntry* entry;	/* Entry from the label table */

    /*
     * Update the state of the current block, checking for consistency.  Set
     * 'changed' to 1 if the state changes and successor blocks need to be
     * rechecked.
     */

    if (bbPtr->catchState == BBCS_UNKNOWN) {
	bbPtr->enclosingCatch = enclosing;
    } else if (bbPtr->enclosingCatch != enclosing) {
	if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "execution reaches an instruction in inconsistent "
		    "exception contexts", -1));
	    Tcl_SetErrorLine(interp, bbPtr->startLine);
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "BADCATCH", NULL);
	}
	return TCL_ERROR;
    }
    if (state > bbPtr->catchState) {
	bbPtr->catchState = state;
	changed = 1;
    }

    /*
     * If this block has been visited before, and its state hasn't changed,
     * we're done with it for now.
     */

    if (!changed) {
	return TCL_OK;
    }
    bbPtr->catchDepth = catchDepth;

    /*
     * Determine enclosing catch and 'caught' state for the fallthrough and
     * the jump target. Default for both is the state of the current block.
     */

    fallThruEnclosing = enclosing;
    fallThruState = state;
    jumpEnclosing = enclosing;
    jumpState = state;

    /*
     * TODO: Make sure that the test cases include validating that a natural
     * loop can't include 'beginCatch' or 'endCatch'
     */

    if (bbPtr->flags & BB_BEGINCATCH) {
	/*
	 * If the block begins a catch, the state for the successor is 'in
	 * catch'. The jump target is the exception exit, and the state of the
	 * jump target is 'caught.'
	 */

	fallThruEnclosing = bbPtr;
	fallThruState = BBCS_INCATCH;
	jumpEnclosing = bbPtr;
	jumpState = BBCS_CAUGHT;
	++catchDepth;
    }

    if (bbPtr->flags & BB_ENDCATCH) {
	/*
	 * If the block ends a catch, the state for the successor is whatever
	 * the state was on entry to the catch.
	 */

	if (enclosing == NULL) {
	    if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"endCatch without a corresponding beginCatch", -1));
		Tcl_SetErrorLine(interp, bbPtr->startLine);
		Tcl_SetErrorCode(interp, "TCL", "ASSEM", "BADENDCATCH", NULL);
	    }
	    return TCL_ERROR;
	}
	fallThruEnclosing = enclosing->enclosingCatch;
	fallThruState = enclosing->catchState;
	--catchDepth;
    }

    /*
     * Visit any successor blocks with the appropriate exception context
     */

    result = TCL_OK;
    if (bbPtr->flags & BB_FALLTHRU) {
	result = ProcessCatchesInBasicBlock(assemEnvPtr, bbPtr->successor1,
		fallThruEnclosing, fallThruState, catchDepth);
    }
    if (result == TCL_OK && bbPtr->jumpTarget != NULL) {
	entry = Tcl_FindHashEntry(&assemEnvPtr->labelHash,
		Tcl_GetString(bbPtr->jumpTarget));
	jumpTarget = Tcl_GetHashValue(entry);
	result = ProcessCatchesInBasicBlock(assemEnvPtr, jumpTarget,
		jumpEnclosing, jumpState, catchDepth);
    }

    /*
     * All blocks referenced in a jump table are successors.
     */

    if (bbPtr->flags & BB_JUMPTABLE) {
	for (jtEntry = Tcl_FirstHashEntry(&bbPtr->jtPtr->hashTable,&jtSearch);
		result == TCL_OK && jtEntry != NULL;
		jtEntry = Tcl_NextHashEntry(&jtSearch)) {
	    targetLabel = Tcl_GetHashValue(jtEntry);
	    entry = Tcl_FindHashEntry(&assemEnvPtr->labelHash,
		    Tcl_GetString(targetLabel));
	    jumpTarget = Tcl_GetHashValue(entry);
	    result = ProcessCatchesInBasicBlock(assemEnvPtr, jumpTarget,
		    jumpEnclosing, jumpState, catchDepth);
	}
    }

    return result;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CheckForUnclosedCatches --
 *
 *	Checks that a sequence of assembly code has no unclosed catches on
 *	exit.
 *
 * Results:
 *	Returns a standard Tcl result, with an error message for unclosed
 *	catches.
 *
 *-----------------------------------------------------------------------------
 */

static int
CheckForUnclosedCatches(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */

    if (assemEnvPtr->curr_bb->catchState >= BBCS_INCATCH) {
	if (assemEnvPtr->flags & TCL_EVAL_DIRECT) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "catch still active on exit from assembly code", -1));
	    Tcl_SetErrorLine(interp,
		    assemEnvPtr->curr_bb->enclosingCatch->startLine);
	    Tcl_SetErrorCode(interp, "TCL", "ASSEM", "UNCLOSEDCATCH", NULL);
	}
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * BuildExceptionRanges --
 *
 *	Walks through the assembly code and builds exception ranges for the
 *	catches embedded therein.
 *
 * Results:
 *	Returns a standard Tcl result with an error message in the interpreter
 *	if anything is unsuccessful.
 *
 * Side effects:
 *	Each contiguous block of code with a given catch exit is assigned an
 *	exception range at the appropriate level.
 *	Exception ranges in embedded blocks have their levels corrected and
 *	collated into the table.
 *	Blocks that end with 'beginCatch' are associated with the innermost
 *	exception range of the following block.
 *
 *-----------------------------------------------------------------------------
 */

static int
BuildExceptionRanges(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    BasicBlock* bbPtr;		/* Current basic block */
    BasicBlock* prevPtr = NULL;	/* Previous basic block */
    int catchDepth = 0;		/* Current catch depth */
    int maxCatchDepth = 0;	/* Maximum catch depth in the program */
    BasicBlock** catches;	/* Stack of catches in progress */
    int* catchIndices;		/* Indices of the exception ranges of catches
				 * in progress */
    int i;

    /*
     * Determine the max catch depth for the entire assembly script
     * (excluding embedded eval's and expr's, which will be handled later).
     */

    for (bbPtr=assemEnvPtr->head_bb; bbPtr != NULL; bbPtr=bbPtr->successor1) {
	if (bbPtr->catchDepth > maxCatchDepth) {
	    maxCatchDepth = bbPtr->catchDepth;
	}
    }

    /*
     * Allocate memory for a stack of active catches.
     */

    catches = ckalloc(maxCatchDepth * sizeof(BasicBlock*));
    catchIndices = ckalloc(maxCatchDepth * sizeof(int));
    for (i = 0; i < maxCatchDepth; ++i) {
	catches[i] = NULL;
	catchIndices[i] = -1;
    }

    /*
     * Walk through the basic blocks and manage exception ranges.
     */

    for (bbPtr=assemEnvPtr->head_bb; bbPtr != NULL; bbPtr=bbPtr->successor1) {
	UnstackExpiredCatches(envPtr, bbPtr, catchDepth, catches,
		catchIndices);
	LookForFreshCatches(bbPtr, catches);
	StackFreshCatches(assemEnvPtr, bbPtr, catchDepth, catches,
		catchIndices);

	/*
	 * If the last block was a 'begin catch', fill in the exception range.
	 */

	catchDepth = bbPtr->catchDepth;
	if (prevPtr != NULL && (prevPtr->flags & BB_BEGINCATCH)) {
	    TclStoreInt4AtPtr(catchIndices[catchDepth-1],
		    envPtr->codeStart + bbPtr->startOffset - 4);
	}

	prevPtr = bbPtr;
    }

    /* Make sure that all catches are closed */

    if (catchDepth != 0) {
	Tcl_Panic("unclosed catch at end of code in "
		"tclAssembly.c:BuildExceptionRanges, can't happen");
    }

    /* Free temp storage */

    ckfree(catchIndices);
    ckfree(catches);

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * UnstackExpiredCatches --
 *
 *	Unstacks and closes the exception ranges for any catch contexts that
 *	were active in the previous basic block but are inactive in the
 *	current one.
 *
 *-----------------------------------------------------------------------------
 */

static void
UnstackExpiredCatches(
    CompileEnv* envPtr,		/* Compilation environment */
    BasicBlock* bbPtr,		/* Basic block being processed */
    int catchDepth,		/* Depth of nesting of catches prior to entry
				 * to this block */
    BasicBlock** catches,	/* Array of catch contexts */
    int* catchIndices)		/* Indices of the exception ranges
				 * corresponding to the catch contexts */
{
    ExceptionRange* range;	/* Exception range for a specific catch */
    BasicBlock* catch;		/* Catch block being examined */
    BasicBlockCatchState catchState;
				/* State of the code relative to the catch
				 * block being examined ("in catch" or
				 * "caught"). */

    /*
     * Unstack any catches that are deeper than the nesting level of the basic
     * block being entered.
     */

    while (catchDepth > bbPtr->catchDepth) {
	--catchDepth;
	if (catches[catchDepth] != NULL) {
	    range = envPtr->exceptArrayPtr + catchIndices[catchDepth];
	    range->numCodeBytes = bbPtr->startOffset - range->codeOffset;
	    catches[catchDepth] = NULL;
	    catchIndices[catchDepth] = -1;
	}
    }

    /*
     * Unstack any catches that don't match the basic block being entered,
     * either because they are no longer part of the context, or because the
     * context has changed from INCATCH to CAUGHT.
     */

    catchState = bbPtr->catchState;
    catch = bbPtr->enclosingCatch;
    while (catchDepth > 0) {
	--catchDepth;
	if (catches[catchDepth] != NULL) {
	    if (catches[catchDepth] != catch || catchState >= BBCS_CAUGHT) {
		range = envPtr->exceptArrayPtr + catchIndices[catchDepth];
		range->numCodeBytes = bbPtr->startOffset - range->codeOffset;
		catches[catchDepth] = NULL;
		catchIndices[catchDepth] = -1;
	    }
	    catchState = catch->catchState;
	    catch = catch->enclosingCatch;
	}
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * LookForFreshCatches --
 *
 *	Determines whether a basic block being entered needs any exception
 *	ranges that are not already stacked.
 *
 * Does not create the ranges: this procedure iterates from the innermost
 * catch outward, but exception ranges must be created from the outermost
 * catch inward.
 *
 *-----------------------------------------------------------------------------
 */

static void
LookForFreshCatches(
    BasicBlock* bbPtr,		/* Basic block being entered */
    BasicBlock** catches)	/* Array of catch contexts that are already
				 * entered */
{
    BasicBlockCatchState catchState;
				/* State ("in catch" or "caught") of the
				 * current catch. */
    BasicBlock* catch;		/* Current enclosing catch */
    int catchDepth;		/* Nesting depth of the current catch */

    catchState = bbPtr->catchState;
    catch = bbPtr->enclosingCatch;
    catchDepth = bbPtr->catchDepth;
    while (catchDepth > 0) {
	--catchDepth;
	if (catches[catchDepth] != catch && catchState < BBCS_CAUGHT) {
	    catches[catchDepth] = catch;
	}
	catchState = catch->catchState;
	catch = catch->enclosingCatch;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * StackFreshCatches --
 *
 *	Make ExceptionRange records for any catches that are in the basic
 *	block being entered and were not in the previous basic block.
 *
 *-----------------------------------------------------------------------------
 */

static void
StackFreshCatches(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    BasicBlock* bbPtr,		/* Basic block being processed */
    int catchDepth,		/* Depth of nesting of catches prior to entry
				 * to this block */
    BasicBlock** catches,	/* Array of catch contexts */
    int* catchIndices)		/* Indices of the exception ranges
				 * corresponding to the catch contexts */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    ExceptionRange* range;	/* Exception range for a specific catch */
    BasicBlock* catch;		/* Catch block being examined */
    BasicBlock* errorExit;	/* Error exit from the catch block */
    Tcl_HashEntry* entryPtr;

    catchDepth = 0;

    /*
     * Iterate through the enclosing catch blocks from the outside in,
     * looking for ones that don't have exception ranges (and are uncaught)
     */

    for (catchDepth = 0; catchDepth < bbPtr->catchDepth; ++catchDepth) {
	if (catchIndices[catchDepth] == -1 && catches[catchDepth] != NULL) {
	    /*
	     * Create an exception range for a block that needs one.
	     */

	    catch = catches[catchDepth];
	    catchIndices[catchDepth] =
		    TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
	    range = envPtr->exceptArrayPtr + catchIndices[catchDepth];
	    range->nestingLevel = envPtr->exceptDepth + catchDepth;
	    envPtr->maxExceptDepth =
		    TclMax(range->nestingLevel + 1, envPtr->maxExceptDepth);
	    range->codeOffset = bbPtr->startOffset;

	    entryPtr = Tcl_FindHashEntry(&assemEnvPtr->labelHash,
		    Tcl_GetString(catch->jumpTarget));
	    if (entryPtr == NULL) {
		Tcl_Panic("undefined label in tclAssembly.c:"
			"BuildExceptionRanges, can't happen");
	    }

	    errorExit = Tcl_GetHashValue(entryPtr);
	    range->catchOffset = errorExit->startOffset;
	}
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * RestoreEmbeddedExceptionRanges --
 *
 *	Processes an assembly script, replacing any exception ranges that
 *	were present in embedded code.
 *
 *-----------------------------------------------------------------------------
 */

static void
RestoreEmbeddedExceptionRanges(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    BasicBlock* bbPtr;		/* Current basic block */
    int rangeBase;		/* Base of the foreign exception ranges when
				 * they are reinstalled */
    int rangeIndex;		/* Index of the current foreign exception
				 * range as reinstalled */
    ExceptionRange* range;	/* Current foreign exception range */
    unsigned char opcode;	/* Current instruction's opcode */
    int catchIndex;		/* Index of the exception range to which the
				 * current instruction refers */
    int i;

    /*
     * Walk the basic blocks looking for exceptions in embedded scripts.
     */

    for (bbPtr = assemEnvPtr->head_bb;
	    bbPtr != NULL;
	    bbPtr = bbPtr->successor1) {
	if (bbPtr->foreignExceptionCount != 0) {
	    /*
	     * Reinstall the embedded exceptions and track their nesting level
	     */

	    rangeBase = envPtr->exceptArrayNext;
	    for (i = 0; i < bbPtr->foreignExceptionCount; ++i) {
		range = bbPtr->foreignExceptions + i;
		rangeIndex = TclCreateExceptRange(range->type, envPtr);
		range->nestingLevel += envPtr->exceptDepth + bbPtr->catchDepth;
		memcpy(envPtr->exceptArrayPtr + rangeIndex, range,
			sizeof(ExceptionRange));
		if (range->nestingLevel >= envPtr->maxExceptDepth) {
		    envPtr->maxExceptDepth = range->nestingLevel + 1;
		}
	    }

	    /*
	     * Walk through the bytecode of the basic block, and relocate
	     * INST_BEGIN_CATCH4 instructions to the new locations
	     */

	    i = bbPtr->startOffset;
	    while (i < bbPtr->successor1->startOffset) {
		opcode = envPtr->codeStart[i];
		if (opcode == INST_BEGIN_CATCH4) {
		    catchIndex = TclGetUInt4AtPtr(envPtr->codeStart + i + 1);
		    if (catchIndex >= bbPtr->foreignExceptionBase
			    && catchIndex < (bbPtr->foreignExceptionBase +
			    bbPtr->foreignExceptionCount)) {
			catchIndex -= bbPtr->foreignExceptionBase;
			catchIndex += rangeBase;
			TclStoreInt4AtPtr(catchIndex, envPtr->codeStart+i+1);
		    }
		}
		i += tclInstructionTable[opcode].numBytes;
	    }
	}
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * ResetVisitedBasicBlocks --
 *
 *	Turns off the 'visited' flag in all basic blocks at the conclusion
 *	of a pass.
 *
 *-----------------------------------------------------------------------------
 */

static void
ResetVisitedBasicBlocks(
    AssemblyEnv* assemEnvPtr)	/* Assembly environment */
{
    BasicBlock* block;

    for (block = assemEnvPtr->head_bb; block != NULL;
	    block = block->successor1) {
	block->flags &= ~BB_VISITED;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * AddBasicBlockRangeToErrorInfo --
 *
 *	Updates the error info of the Tcl interpreter to show a given basic
 *	block in the code.
 *
 * This procedure is used to label the callstack with source location
 * information when reporting an error in stack checking.
 *
 *-----------------------------------------------------------------------------
 */

static void
AddBasicBlockRangeToErrorInfo(
    AssemblyEnv* assemEnvPtr,	/* Assembly environment */
    BasicBlock* bbPtr)		/* Basic block in which the error is found */
{
    CompileEnv* envPtr = assemEnvPtr->envPtr;
				/* Compilation environment */
    Tcl_Interp* interp = (Tcl_Interp*) envPtr->iPtr;
				/* Tcl interpreter */
    Tcl_Obj* lineNo;		/* Line number in the source */

    Tcl_AddErrorInfo(interp, "\n    in assembly code between lines ");
    lineNo = Tcl_NewIntObj(bbPtr->startLine);
    Tcl_IncrRefCount(lineNo);
    Tcl_AppendObjToErrorInfo(interp, lineNo);
    Tcl_AddErrorInfo(interp, " and ");
    if (bbPtr->successor1 != NULL) {
	Tcl_SetIntObj(lineNo, bbPtr->successor1->startLine);
	Tcl_AppendObjToErrorInfo(interp, lineNo);
    } else {
	Tcl_AddErrorInfo(interp, "end of assembly code");
    }
    Tcl_DecrRefCount(lineNo);
}

/*
 *-----------------------------------------------------------------------------
 *
 * DupAssembleCodeInternalRep --
 *
 *	Part of the Tcl object type implementation for Tcl assembly language
 *	bytecode. We do not copy the bytecode intrep. Instead, we return
 *	without setting copyPtr->typePtr, so the copy is a plain string copy
 *	of the assembly source, and if it is to be used as a compiled
 *	expression, it will need to be reprocessed.
 *
 *	This makes sense, because with Tcl's copy-on-write practices, the
 *	usual (only?) time Tcl_DuplicateObj() will be called is when the copy
 *	is about to be modified, which would invalidate any copied bytecode
 *	anyway. The only reason it might make sense to copy the bytecode is if
 *	we had some modifying routines that operated directly on the intrep,
 *	as we do for lists and dicts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

static void
DupAssembleCodeInternalRep(
    Tcl_Obj *srcPtr,
    Tcl_Obj *copyPtr)
{
    return;
}

/*
 *-----------------------------------------------------------------------------
 *
 * FreeAssembleCodeInternalRep --
 *
 *	Part of the Tcl object type implementation for Tcl expression
 *	bytecode. Frees the storage allocated to hold the internal rep, unless
 *	ref counts indicate bytecode execution is still in progress.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May free allocated memory. Leaves objPtr untyped.
 *
 *-----------------------------------------------------------------------------
 */

static void
FreeAssembleCodeInternalRep(
    Tcl_Obj *objPtr)
{
    ByteCode *codePtr = objPtr->internalRep.twoPtrValue.ptr1;

    codePtr->refCount--;
    if (codePtr->refCount <= 0) {
	TclCleanupByteCode(codePtr);
    }
    objPtr->typePtr = NULL;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
