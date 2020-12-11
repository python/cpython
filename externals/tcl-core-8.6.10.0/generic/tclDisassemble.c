/*
 * tclDisassemble.c --
 *
 *	This file contains procedures that disassemble bytecode into either
 *	human-readable or Tcl-processable forms.
 *
 * Copyright (c) 1996-1998 Sun Microsystems, Inc.
 * Copyright (c) 2001 by Kevin B. Kenny. All rights reserved.
 * Copyright (c) 2013-2016 Donal K. Fellows.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclCompile.h"
#include "tclOOInt.h"
#include <assert.h>

/*
 * Prototypes for procedures defined later in this file:
 */

static Tcl_Obj *	DisassembleByteCodeAsDicts(Tcl_Interp *interp,
			    Tcl_Obj *objPtr);
static Tcl_Obj *	DisassembleByteCodeObj(Tcl_Interp *interp,
			    Tcl_Obj *objPtr);
static int		FormatInstruction(ByteCode *codePtr,
			    const unsigned char *pc, Tcl_Obj *bufferObj);
static void		GetLocationInformation(Proc *procPtr,
			    Tcl_Obj **fileObjPtr, int *linePtr);
static void		PrintSourceToObj(Tcl_Obj *appendObj,
			    const char *stringPtr, int maxChars);
static void		UpdateStringOfInstName(Tcl_Obj *objPtr);

/*
 * The structure below defines an instruction name Tcl object to allow
 * reporting of inner contexts in errorstack without string allocation.
 */

static const Tcl_ObjType tclInstNameType = {
    "instname",			/* name */
    NULL,			/* freeIntRepProc */
    NULL,			/* dupIntRepProc */
    UpdateStringOfInstName,	/* updateStringProc */
    NULL,			/* setFromAnyProc */
};

/*
 * How to get the bytecode out of a Tcl_Obj.
 */

#define BYTECODE(objPtr)					\
    ((ByteCode *) (objPtr)->internalRep.twoPtrValue.ptr1)

/*
 *----------------------------------------------------------------------
 *
 * GetLocationInformation --
 *
 *	This procedure looks up the information about where a procedure was
 *	originally declared.
 *
 * Results:
 *	Writes to the variables pointed at by fileObjPtr and linePtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetLocationInformation(
    Proc *procPtr,		/* What to look up the information for. */
    Tcl_Obj **fileObjPtr,	/* Where to write the information about what
				 * file the code came from. Will be written
				 * to, either with the object (assume shared!)
				 * that describes what the file was, or with
				 * NULL if the information is not
				 * available. */
    int *linePtr)		/* Where to write the information about what
				 * line number represented the start of the
				 * code in question. Will be written to,
				 * either with the line number or with -1 if
				 * the information is not available. */
{
    CmdFrame *cfPtr = TclGetCmdFrameForProcedure(procPtr);

    *fileObjPtr = NULL;
    *linePtr = -1;
    if (cfPtr == NULL) {
	return;
    }

    /*
     * Get the source location data out of the CmdFrame.
     */

    *linePtr = cfPtr->line[0];
    if (cfPtr->type == TCL_LOCATION_SOURCE) {
	*fileObjPtr = cfPtr->data.eval.path;
    }
}

#ifdef TCL_COMPILE_DEBUG
/*
 *----------------------------------------------------------------------
 *
 * TclPrintByteCodeObj --
 *
 *	This procedure prints ("disassembles") the instructions of a bytecode
 *	object to stdout.
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
TclPrintByteCodeObj(
    Tcl_Interp *interp,		/* Used only for getting location info. */
    Tcl_Obj *objPtr)		/* The bytecode object to disassemble. */
{
    Tcl_Obj *bufPtr = DisassembleByteCodeObj(interp, objPtr);

    fprintf(stdout, "\n%s", TclGetString(bufPtr));
    Tcl_DecrRefCount(bufPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclPrintInstruction --
 *
 *	This procedure prints ("disassembles") one instruction from a bytecode
 *	object to stdout.
 *
 * Results:
 *	Returns the length in bytes of the current instruiction.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclPrintInstruction(
    ByteCode *codePtr,		/* Bytecode containing the instruction. */
    const unsigned char *pc)	/* Points to first byte of instruction. */
{
    Tcl_Obj *bufferObj;
    int numBytes;

    TclNewObj(bufferObj);
    numBytes = FormatInstruction(codePtr, pc, bufferObj);
    fprintf(stdout, "%s", TclGetString(bufferObj));
    Tcl_DecrRefCount(bufferObj);
    return numBytes;
}

/*
 *----------------------------------------------------------------------
 *
 * TclPrintObject --
 *
 *	This procedure prints up to a specified number of characters from the
 *	argument Tcl object's string representation to a specified file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Outputs characters to the specified file.
 *
 *----------------------------------------------------------------------
 */

void
TclPrintObject(
    FILE *outFile,		/* The file to print the source to. */
    Tcl_Obj *objPtr,		/* Points to the Tcl object whose string
				 * representation should be printed. */
    int maxChars)		/* Maximum number of chars to print. */
{
    char *bytes;
    int length;

    bytes = Tcl_GetStringFromObj(objPtr, &length);
    TclPrintSource(outFile, bytes, TclMin(length, maxChars));
}

/*
 *----------------------------------------------------------------------
 *
 * TclPrintSource --
 *
 *	This procedure prints up to a specified number of characters from the
 *	argument string to a specified file. It tries to produce legible
 *	output by adding backslashes as necessary.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Outputs characters to the specified file.
 *
 *----------------------------------------------------------------------
 */

void
TclPrintSource(
    FILE *outFile,		/* The file to print the source to. */
    const char *stringPtr,	/* The string to print. */
    int maxChars)		/* Maximum number of chars to print. */
{
    Tcl_Obj *bufferObj;

    TclNewObj(bufferObj);
    PrintSourceToObj(bufferObj, stringPtr, maxChars);
    fprintf(outFile, "%s", TclGetString(bufferObj));
    Tcl_DecrRefCount(bufferObj);
}
#endif /* TCL_COMPILE_DEBUG */

/*
 *----------------------------------------------------------------------
 *
 * DisassembleByteCodeObj --
 *
 *	Given an object which is of bytecode type, return a disassembled
 *	version of the bytecode (in a new refcount 0 object). No guarantees
 *	are made about the details of the contents of the result.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
DisassembleByteCodeObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)		/* The bytecode object to disassemble. */
{
    ByteCode *codePtr = BYTECODE(objPtr);
    unsigned char *codeStart, *codeLimit, *pc;
    unsigned char *codeDeltaNext, *codeLengthNext;
    unsigned char *srcDeltaNext, *srcLengthNext;
    int codeOffset, codeLen, srcOffset, srcLen, numCmds, delta, i, line;
    Interp *iPtr = (Interp *) *codePtr->interpHandle;
    Tcl_Obj *bufferObj, *fileObj;
    char ptrBuf1[20], ptrBuf2[20];

    TclNewObj(bufferObj);
    if (codePtr->refCount <= 0) {
	return bufferObj;	/* Already freed. */
    }

    codeStart = codePtr->codeStart;
    codeLimit = codeStart + codePtr->numCodeBytes;
    numCmds = codePtr->numCommands;

    /*
     * Print header lines describing the ByteCode.
     */

    sprintf(ptrBuf1, "%p", codePtr);
    sprintf(ptrBuf2, "%p", iPtr);
    Tcl_AppendPrintfToObj(bufferObj,
	    "ByteCode 0x%s, refCt %u, epoch %u, interp 0x%s (epoch %u)\n",
	    ptrBuf1, codePtr->refCount, codePtr->compileEpoch, ptrBuf2,
	    iPtr->compileEpoch);
    Tcl_AppendToObj(bufferObj, "  Source ", -1);
    PrintSourceToObj(bufferObj, codePtr->source,
	    TclMin(codePtr->numSrcBytes, 55));
    GetLocationInformation(codePtr->procPtr, &fileObj, &line);
    if (line > -1 && fileObj != NULL) {
	Tcl_AppendPrintfToObj(bufferObj, "\n  File \"%s\" Line %d",
		Tcl_GetString(fileObj), line);
    }
    Tcl_AppendPrintfToObj(bufferObj,
	    "\n  Cmds %d, src %d, inst %d, litObjs %u, aux %d, stkDepth %u, code/src %.2f\n",
	    numCmds, codePtr->numSrcBytes, codePtr->numCodeBytes,
	    codePtr->numLitObjects, codePtr->numAuxDataItems,
	    codePtr->maxStackDepth,
#ifdef TCL_COMPILE_STATS
	    codePtr->numSrcBytes?
		    codePtr->structureSize/(float)codePtr->numSrcBytes :
#endif
	    0.0);

#ifdef TCL_COMPILE_STATS
    Tcl_AppendPrintfToObj(bufferObj,
	    "  Code %lu = header %lu+inst %d+litObj %lu+exc %lu+aux %lu+cmdMap %d\n",
	    (unsigned long) codePtr->structureSize,
	    (unsigned long) (sizeof(ByteCode) - sizeof(size_t) - sizeof(Tcl_Time)),
	    codePtr->numCodeBytes,
	    (unsigned long) (codePtr->numLitObjects * sizeof(Tcl_Obj *)),
	    (unsigned long) (codePtr->numExceptRanges*sizeof(ExceptionRange)),
	    (unsigned long) (codePtr->numAuxDataItems * sizeof(AuxData)),
	    codePtr->numCmdLocBytes);
#endif /* TCL_COMPILE_STATS */

    /*
     * If the ByteCode is the compiled body of a Tcl procedure, print
     * information about that procedure. Note that we don't know the
     * procedure's name since ByteCode's can be shared among procedures.
     */

    if (codePtr->procPtr != NULL) {
	Proc *procPtr = codePtr->procPtr;
	int numCompiledLocals = procPtr->numCompiledLocals;

	sprintf(ptrBuf1, "%p", procPtr);
	Tcl_AppendPrintfToObj(bufferObj,
		"  Proc 0x%s, refCt %d, args %d, compiled locals %d\n",
		ptrBuf1, procPtr->refCount, procPtr->numArgs,
		numCompiledLocals);
	if (numCompiledLocals > 0) {
	    CompiledLocal *localPtr = procPtr->firstLocalPtr;

	    for (i = 0;  i < numCompiledLocals;  i++) {
		Tcl_AppendPrintfToObj(bufferObj,
			"      slot %d%s%s%s%s%s%s", i,
			(localPtr->flags & (VAR_ARRAY|VAR_LINK)) ? "" : ", scalar",
			(localPtr->flags & VAR_ARRAY) ? ", array" : "",
			(localPtr->flags & VAR_LINK) ? ", link" : "",
			(localPtr->flags & VAR_ARGUMENT) ? ", arg" : "",
			(localPtr->flags & VAR_TEMPORARY) ? ", temp" : "",
			(localPtr->flags & VAR_RESOLVED) ? ", resolved" : "");
		if (TclIsVarTemporary(localPtr)) {
		    Tcl_AppendToObj(bufferObj, "\n", -1);
		} else {
		    Tcl_AppendPrintfToObj(bufferObj, ", \"%s\"\n",
			    localPtr->name);
		}
		localPtr = localPtr->nextPtr;
	    }
	}
    }

    /*
     * Print the ExceptionRange array.
     */

    if (codePtr->numExceptRanges > 0) {
	Tcl_AppendPrintfToObj(bufferObj, "  Exception ranges %d, depth %d:\n",
		codePtr->numExceptRanges, codePtr->maxExceptDepth);
	for (i = 0;  i < codePtr->numExceptRanges;  i++) {
	    ExceptionRange *rangePtr = &codePtr->exceptArrayPtr[i];

	    Tcl_AppendPrintfToObj(bufferObj,
		    "      %d: level %d, %s, pc %d-%d, ",
		    i, rangePtr->nestingLevel,
		    (rangePtr->type==LOOP_EXCEPTION_RANGE ? "loop" : "catch"),
		    rangePtr->codeOffset,
		    (rangePtr->codeOffset + rangePtr->numCodeBytes - 1));
	    switch (rangePtr->type) {
	    case LOOP_EXCEPTION_RANGE:
		Tcl_AppendPrintfToObj(bufferObj, "continue %d, break %d\n",
			rangePtr->continueOffset, rangePtr->breakOffset);
		break;
	    case CATCH_EXCEPTION_RANGE:
		Tcl_AppendPrintfToObj(bufferObj, "catch %d\n",
			rangePtr->catchOffset);
		break;
	    default:
		Tcl_Panic("DisassembleByteCodeObj: bad ExceptionRange type %d",
			rangePtr->type);
	    }
	}
    }

    /*
     * If there were no commands (e.g., an expression or an empty string was
     * compiled), just print all instructions and return.
     */

    if (numCmds == 0) {
	pc = codeStart;
	while (pc < codeLimit) {
	    Tcl_AppendToObj(bufferObj, "    ", -1);
	    pc += FormatInstruction(codePtr, pc, bufferObj);
	}
	return bufferObj;
    }

    /*
     * Print table showing the code offset, source offset, and source length
     * for each command. These are encoded as a sequence of bytes.
     */

    Tcl_AppendPrintfToObj(bufferObj, "  Commands %d:", numCmds);
    codeDeltaNext = codePtr->codeDeltaStart;
    codeLengthNext = codePtr->codeLengthStart;
    srcDeltaNext = codePtr->srcDeltaStart;
    srcLengthNext = codePtr->srcLengthStart;
    codeOffset = srcOffset = 0;
    for (i = 0;  i < numCmds;  i++) {
	if ((unsigned) *codeDeltaNext == (unsigned) 0xFF) {
	    codeDeltaNext++;
	    delta = TclGetInt4AtPtr(codeDeltaNext);
	    codeDeltaNext += 4;
	} else {
	    delta = TclGetInt1AtPtr(codeDeltaNext);
	    codeDeltaNext++;
	}
	codeOffset += delta;

	if ((unsigned) *codeLengthNext == (unsigned) 0xFF) {
	    codeLengthNext++;
	    codeLen = TclGetInt4AtPtr(codeLengthNext);
	    codeLengthNext += 4;
	} else {
	    codeLen = TclGetInt1AtPtr(codeLengthNext);
	    codeLengthNext++;
	}

	if ((unsigned) *srcDeltaNext == (unsigned) 0xFF) {
	    srcDeltaNext++;
	    delta = TclGetInt4AtPtr(srcDeltaNext);
	    srcDeltaNext += 4;
	} else {
	    delta = TclGetInt1AtPtr(srcDeltaNext);
	    srcDeltaNext++;
	}
	srcOffset += delta;

	if ((unsigned) *srcLengthNext == (unsigned) 0xFF) {
	    srcLengthNext++;
	    srcLen = TclGetInt4AtPtr(srcLengthNext);
	    srcLengthNext += 4;
	} else {
	    srcLen = TclGetInt1AtPtr(srcLengthNext);
	    srcLengthNext++;
	}

	Tcl_AppendPrintfToObj(bufferObj, "%s%4d: pc %d-%d, src %d-%d",
		((i % 2)? "     " : "\n   "),
		(i+1), codeOffset, (codeOffset + codeLen - 1),
		srcOffset, (srcOffset + srcLen - 1));
    }
    if (numCmds > 0) {
	Tcl_AppendToObj(bufferObj, "\n", -1);
    }

    /*
     * Print each instruction. If the instruction corresponds to the start of
     * a command, print the command's source. Note that we don't need the code
     * length here.
     */

    codeDeltaNext = codePtr->codeDeltaStart;
    srcDeltaNext = codePtr->srcDeltaStart;
    srcLengthNext = codePtr->srcLengthStart;
    codeOffset = srcOffset = 0;
    pc = codeStart;
    for (i = 0;  i < numCmds;  i++) {
	if ((unsigned) *codeDeltaNext == (unsigned) 0xFF) {
	    codeDeltaNext++;
	    delta = TclGetInt4AtPtr(codeDeltaNext);
	    codeDeltaNext += 4;
	} else {
	    delta = TclGetInt1AtPtr(codeDeltaNext);
	    codeDeltaNext++;
	}
	codeOffset += delta;

	if ((unsigned) *srcDeltaNext == (unsigned) 0xFF) {
	    srcDeltaNext++;
	    delta = TclGetInt4AtPtr(srcDeltaNext);
	    srcDeltaNext += 4;
	} else {
	    delta = TclGetInt1AtPtr(srcDeltaNext);
	    srcDeltaNext++;
	}
	srcOffset += delta;

	if ((unsigned) *srcLengthNext == (unsigned) 0xFF) {
	    srcLengthNext++;
	    srcLen = TclGetInt4AtPtr(srcLengthNext);
	    srcLengthNext += 4;
	} else {
	    srcLen = TclGetInt1AtPtr(srcLengthNext);
	    srcLengthNext++;
	}

	/*
	 * Print instructions before command i.
	 */

	while ((pc-codeStart) < codeOffset) {
	    Tcl_AppendToObj(bufferObj, "    ", -1);
	    pc += FormatInstruction(codePtr, pc, bufferObj);
	}

	Tcl_AppendPrintfToObj(bufferObj, "  Command %d: ", i+1);
	PrintSourceToObj(bufferObj, (codePtr->source + srcOffset),
		TclMin(srcLen, 55));
	Tcl_AppendToObj(bufferObj, "\n", -1);
    }
    if (pc < codeLimit) {
	/*
	 * Print instructions after the last command.
	 */

	while (pc < codeLimit) {
	    Tcl_AppendToObj(bufferObj, "    ", -1);
	    pc += FormatInstruction(codePtr, pc, bufferObj);
	}
    }
    return bufferObj;
}

/*
 *----------------------------------------------------------------------
 *
 * FormatInstruction --
 *
 *	Appends a representation of a bytecode instruction to a Tcl_Obj.
 *
 *----------------------------------------------------------------------
 */

static int
FormatInstruction(
    ByteCode *codePtr,		/* Bytecode containing the instruction. */
    const unsigned char *pc,	/* Points to first byte of instruction. */
    Tcl_Obj *bufferObj)		/* Object to append instruction info to. */
{
    Proc *procPtr = codePtr->procPtr;
    unsigned char opCode = *pc;
    register const InstructionDesc *instDesc = &tclInstructionTable[opCode];
    unsigned char *codeStart = codePtr->codeStart;
    unsigned pcOffset = pc - codeStart;
    int opnd = 0, i, j, numBytes = 1;
    int localCt = procPtr ? procPtr->numCompiledLocals : 0;
    CompiledLocal *localPtr = procPtr ? procPtr->firstLocalPtr : NULL;
    char suffixBuffer[128];	/* Additional info to print after main opcode
				 * and immediates. */
    char *suffixSrc = NULL;
    Tcl_Obj *suffixObj = NULL;
    AuxData *auxPtr = NULL;

    suffixBuffer[0] = '\0';
    Tcl_AppendPrintfToObj(bufferObj, "(%u) %s ", pcOffset, instDesc->name);
    for (i = 0;  i < instDesc->numOperands;  i++) {
	switch (instDesc->opTypes[i]) {
	case OPERAND_INT1:
	    opnd = TclGetInt1AtPtr(pc+numBytes); numBytes++;
	    Tcl_AppendPrintfToObj(bufferObj, "%+d ", opnd);
	    break;
	case OPERAND_INT4:
	    opnd = TclGetInt4AtPtr(pc+numBytes); numBytes += 4;
	    Tcl_AppendPrintfToObj(bufferObj, "%+d ", opnd);
	    break;
	case OPERAND_UINT1:
	    opnd = TclGetUInt1AtPtr(pc+numBytes); numBytes++;
	    Tcl_AppendPrintfToObj(bufferObj, "%u ", (unsigned) opnd);
	    break;
	case OPERAND_UINT4:
	    opnd = TclGetUInt4AtPtr(pc+numBytes); numBytes += 4;
	    if (opCode == INST_START_CMD) {
		sprintf(suffixBuffer+strlen(suffixBuffer),
			", %u cmds start here", opnd);
	    }
	    Tcl_AppendPrintfToObj(bufferObj, "%u ", (unsigned) opnd);
	    break;
	case OPERAND_OFFSET1:
	    opnd = TclGetInt1AtPtr(pc+numBytes); numBytes++;
	    sprintf(suffixBuffer, "pc %u", pcOffset+opnd);
	    Tcl_AppendPrintfToObj(bufferObj, "%+d ", opnd);
	    break;
	case OPERAND_OFFSET4:
	    opnd = TclGetInt4AtPtr(pc+numBytes); numBytes += 4;
	    if (opCode == INST_START_CMD) {
		sprintf(suffixBuffer, "next cmd at pc %u", pcOffset+opnd);
	    } else {
		sprintf(suffixBuffer, "pc %u", pcOffset+opnd);
	    }
	    Tcl_AppendPrintfToObj(bufferObj, "%+d ", opnd);
	    break;
	case OPERAND_LIT1:
	    opnd = TclGetUInt1AtPtr(pc+numBytes); numBytes++;
	    suffixObj = codePtr->objArrayPtr[opnd];
	    Tcl_AppendPrintfToObj(bufferObj, "%u ", (unsigned) opnd);
	    break;
	case OPERAND_LIT4:
	    opnd = TclGetUInt4AtPtr(pc+numBytes); numBytes += 4;
	    suffixObj = codePtr->objArrayPtr[opnd];
	    Tcl_AppendPrintfToObj(bufferObj, "%u ", (unsigned) opnd);
	    break;
	case OPERAND_AUX4:
	    opnd = TclGetUInt4AtPtr(pc+numBytes); numBytes += 4;
	    Tcl_AppendPrintfToObj(bufferObj, "%u ", (unsigned) opnd);
	    auxPtr = &codePtr->auxDataArrayPtr[opnd];
	    break;
	case OPERAND_IDX4:
	    opnd = TclGetInt4AtPtr(pc+numBytes); numBytes += 4;
	    if (opnd >= -1) {
		Tcl_AppendPrintfToObj(bufferObj, "%d ", opnd);
	    } else if (opnd == -2) {
		Tcl_AppendPrintfToObj(bufferObj, "end ");
	    } else {
		Tcl_AppendPrintfToObj(bufferObj, "end-%d ", -2-opnd);
	    }
	    break;
	case OPERAND_LVT1:
	    opnd = TclGetUInt1AtPtr(pc+numBytes);
	    numBytes++;
	    goto printLVTindex;
	case OPERAND_LVT4:
	    opnd = TclGetUInt4AtPtr(pc+numBytes);
	    numBytes += 4;
	printLVTindex:
	    if (localPtr != NULL) {
		if (opnd >= localCt) {
		    Tcl_Panic("FormatInstruction: bad local var index %u (%u locals)",
			    (unsigned) opnd, localCt);
		}
		for (j = 0;  j < opnd;  j++) {
		    localPtr = localPtr->nextPtr;
		}
		if (TclIsVarTemporary(localPtr)) {
		    sprintf(suffixBuffer, "temp var %u", (unsigned) opnd);
		} else {
		    sprintf(suffixBuffer, "var ");
		    suffixSrc = localPtr->name;
		}
	    }
	    Tcl_AppendPrintfToObj(bufferObj, "%%v%u ", (unsigned) opnd);
	    break;
	case OPERAND_SCLS1:
	    opnd = TclGetUInt1AtPtr(pc+numBytes); numBytes++;
	    Tcl_AppendPrintfToObj(bufferObj, "%s ",
		    tclStringClassTable[opnd].name);
	    break;
	case OPERAND_NONE:
	default:
	    break;
	}
    }
    if (suffixObj) {
	const char *bytes;
	int length;

	Tcl_AppendToObj(bufferObj, "\t# ", -1);
	bytes = Tcl_GetStringFromObj(codePtr->objArrayPtr[opnd], &length);
	PrintSourceToObj(bufferObj, bytes, TclMin(length, 40));
    } else if (suffixBuffer[0]) {
	Tcl_AppendPrintfToObj(bufferObj, "\t# %s", suffixBuffer);
	if (suffixSrc) {
	    PrintSourceToObj(bufferObj, suffixSrc, 40);
	}
    }
    Tcl_AppendToObj(bufferObj, "\n", -1);
    if (auxPtr && auxPtr->type->printProc) {
	Tcl_AppendToObj(bufferObj, "\t\t[", -1);
	auxPtr->type->printProc(auxPtr->clientData, bufferObj, codePtr,
		pcOffset);
	Tcl_AppendToObj(bufferObj, "]\n", -1);
    }
    return numBytes;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetInnerContext --
 *
 *	If possible, returns a list capturing the inner context. Otherwise
 *	return NULL.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclGetInnerContext(
    Tcl_Interp *interp,
    const unsigned char *pc,
    Tcl_Obj **tosPtr)
{
    int objc = 0, off = 0;
    Tcl_Obj *result;
    Interp *iPtr = (Interp *) interp;

    switch (*pc) {
    case INST_STR_LEN:
    case INST_LNOT:
    case INST_BITNOT:
    case INST_UMINUS:
    case INST_UPLUS:
    case INST_TRY_CVT_TO_NUMERIC:
    case INST_EXPAND_STKTOP:
    case INST_EXPR_STK:
        objc = 1;
        break;

    case INST_LIST_IN:
    case INST_LIST_NOT_IN:	/* Basic list containment operators. */
    case INST_STR_EQ:
    case INST_STR_NEQ:		/* String (in)equality check */
    case INST_STR_CMP:		/* String compare. */
    case INST_STR_INDEX:
    case INST_STR_MATCH:
    case INST_REGEXP:
    case INST_EQ:
    case INST_NEQ:
    case INST_LT:
    case INST_GT:
    case INST_LE:
    case INST_GE:
    case INST_MOD:
    case INST_LSHIFT:
    case INST_RSHIFT:
    case INST_BITOR:
    case INST_BITXOR:
    case INST_BITAND:
    case INST_EXPON:
    case INST_ADD:
    case INST_SUB:
    case INST_DIV:
    case INST_MULT:
        objc = 2;
        break;

    case INST_RETURN_STK:
        /* early pop. TODO: dig out opt dict too :/ */
        objc = 1;
        break;

    case INST_SYNTAX:
    case INST_RETURN_IMM:
        objc = 2;
        break;

    case INST_INVOKE_STK4:
	objc = TclGetUInt4AtPtr(pc+1);
        break;

    case INST_INVOKE_STK1:
	objc = TclGetUInt1AtPtr(pc+1);
	break;
    }

    result = iPtr->innerContext;
    if (Tcl_IsShared(result)) {
        Tcl_DecrRefCount(result);
        iPtr->innerContext = result = Tcl_NewListObj(objc + 1, NULL);
        Tcl_IncrRefCount(result);
    } else {
        int len;

        /*
         * Reset while keeping the list intrep as much as possible.
         */

	Tcl_ListObjLength(interp, result, &len);
        Tcl_ListObjReplace(interp, result, 0, len, 0, NULL);
    }
    Tcl_ListObjAppendElement(NULL, result, TclNewInstNameObj(*pc));

    for (; objc>0 ; objc--) {
        Tcl_Obj *objPtr;

        objPtr = tosPtr[1 - objc + off];
        if (!objPtr) {
            Tcl_Panic("InnerContext: bad tos -- appending null object");
        }
        if ((objPtr->refCount<=0)
#ifdef TCL_MEM_DEBUG
                || (objPtr->refCount==0x61616161)
#endif
        ) {
            Tcl_Panic("InnerContext: bad tos -- appending freed object %p",
                    objPtr);
        }
        Tcl_ListObjAppendElement(NULL, result, objPtr);
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclNewInstNameObj --
 *
 *	Creates a new InstName Tcl_Obj based on the given instruction
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclNewInstNameObj(
    unsigned char inst)
{
    Tcl_Obj *objPtr = Tcl_NewObj();

    objPtr->typePtr = &tclInstNameType;
    objPtr->internalRep.longValue = (long) inst;
    objPtr->bytes = NULL;

    return objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateStringOfInstName --
 *
 *	Update the string representation for an instruction name object.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateStringOfInstName(
    Tcl_Obj *objPtr)
{
    int inst = objPtr->internalRep.longValue;
    char *s, buf[20];
    int len;

    if ((inst < 0) || (inst > LAST_INST_OPCODE)) {
        sprintf(buf, "inst_%d", inst);
        s = buf;
    } else {
        s = (char *) tclInstructionTable[objPtr->internalRep.longValue].name;
    }
    len = strlen(s);
    objPtr->bytes = ckalloc(len + 1);
    memcpy(objPtr->bytes, s, len + 1);
    objPtr->length = len;
}

/*
 *----------------------------------------------------------------------
 *
 * PrintSourceToObj --
 *
 *	Appends a quoted representation of a string to a Tcl_Obj.
 *
 *----------------------------------------------------------------------
 */

static void
PrintSourceToObj(
    Tcl_Obj *appendObj,		/* The object to print the source to. */
    const char *stringPtr,	/* The string to print. */
    int maxChars)		/* Maximum number of chars to print. */
{
    register const char *p;
    register int i = 0, len;
    Tcl_UniChar ch = 0;

    if (stringPtr == NULL) {
	Tcl_AppendToObj(appendObj, "\"\"", -1);
	return;
    }

    Tcl_AppendToObj(appendObj, "\"", -1);
    p = stringPtr;
    for (;  (*p != '\0') && (i < maxChars);  p+=len) {

	len = TclUtfToUniChar(p, &ch);
	switch (ch) {
	case '"':
	    Tcl_AppendToObj(appendObj, "\\\"", -1);
	    i += 2;
	    continue;
	case '\f':
	    Tcl_AppendToObj(appendObj, "\\f", -1);
	    i += 2;
	    continue;
	case '\n':
	    Tcl_AppendToObj(appendObj, "\\n", -1);
	    i += 2;
	    continue;
	case '\r':
	    Tcl_AppendToObj(appendObj, "\\r", -1);
	    i += 2;
	    continue;
	case '\t':
	    Tcl_AppendToObj(appendObj, "\\t", -1);
	    i += 2;
	    continue;
	case '\v':
	    Tcl_AppendToObj(appendObj, "\\v", -1);
	    i += 2;
	    continue;
	default:
#if TCL_UTF_MAX > 4
	    if (ch > 0xffff) {
		Tcl_AppendPrintfToObj(appendObj, "\\U%08x", ch);
		i += 10;
	    } else
#elif TCL_UTF_MAX > 3
	    /* If len == 0, this means we have a char > 0xffff, resulting in
	     * TclUtfToUniChar producing a surrogate pair. We want to output
	     * this pair as a single Unicode character.
	     */
	    if (len == 0) {
		int upper = ((ch & 0x3ff) + 1) << 10;
		len = TclUtfToUniChar(p, &ch);
		Tcl_AppendPrintfToObj(appendObj, "\\U%08x", upper + (ch & 0x3ff));
		i += 10;
	    } else
#endif
	    if (ch < 0x20 || ch >= 0x7f) {
		Tcl_AppendPrintfToObj(appendObj, "\\u%04x", ch);
		i += 6;
	    } else {
		Tcl_AppendPrintfToObj(appendObj, "%c", ch);
		i++;
	    }
	    continue;
	}
    }
    if (*p != '\0') {
	Tcl_AppendToObj(appendObj, "...", -1);
    }
    Tcl_AppendToObj(appendObj, "\"", -1);
}

/*
 *----------------------------------------------------------------------
 *
 * DisassembleByteCodeAsDicts --
 *
 *	Given an object which is of bytecode type, return a disassembled
 *	version of the bytecode (in a new refcount 0 object) in a dictionary.
 *	No guarantees are made about the details of the contents of the
 *	result, but it is intended to be more readable than the old output
 *	format.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
DisassembleByteCodeAsDicts(
    Tcl_Interp *interp,		/* Used for looking up the CmdFrame for the
				 * procedure, if one exists. */
    Tcl_Obj *objPtr)		/* The bytecode-holding value to take apart */
{
    ByteCode *codePtr = BYTECODE(objPtr);
    Tcl_Obj *description, *literals, *variables, *instructions, *inst;
    Tcl_Obj *aux, *exn, *commands, *file;
    unsigned char *pc, *opnd, *codeOffPtr, *codeLenPtr, *srcOffPtr, *srcLenPtr;
    int codeOffset, codeLength, sourceOffset, sourceLength;
    int i, val, line;

    /*
     * Get the literals from the bytecode.
     */

    literals = Tcl_NewObj();
    for (i=0 ; i<codePtr->numLitObjects ; i++) {
	Tcl_ListObjAppendElement(NULL, literals, codePtr->objArrayPtr[i]);
    }

    /*
     * Get the variables from the bytecode.
     */

    variables = Tcl_NewObj();
    if (codePtr->procPtr) {
	int localCount = codePtr->procPtr->numCompiledLocals;
	CompiledLocal *localPtr = codePtr->procPtr->firstLocalPtr;

	for (i=0 ; i<localCount ; i++,localPtr=localPtr->nextPtr) {
	    Tcl_Obj *descriptor[2];

	    descriptor[0] = Tcl_NewObj();
	    if (!(localPtr->flags & (VAR_ARRAY|VAR_LINK))) {
		Tcl_ListObjAppendElement(NULL, descriptor[0],
			Tcl_NewStringObj("scalar", -1));
	    }
	    if (localPtr->flags & VAR_ARRAY) {
		Tcl_ListObjAppendElement(NULL, descriptor[0],
			Tcl_NewStringObj("array", -1));
	    }
	    if (localPtr->flags & VAR_LINK) {
		Tcl_ListObjAppendElement(NULL, descriptor[0],
			Tcl_NewStringObj("link", -1));
	    }
	    if (localPtr->flags & VAR_ARGUMENT) {
		Tcl_ListObjAppendElement(NULL, descriptor[0],
			Tcl_NewStringObj("arg", -1));
	    }
	    if (localPtr->flags & VAR_TEMPORARY) {
		Tcl_ListObjAppendElement(NULL, descriptor[0],
			Tcl_NewStringObj("temp", -1));
	    }
	    if (localPtr->flags & VAR_RESOLVED) {
		Tcl_ListObjAppendElement(NULL, descriptor[0],
			Tcl_NewStringObj("resolved", -1));
	    }
	    if (localPtr->flags & VAR_TEMPORARY) {
		Tcl_ListObjAppendElement(NULL, variables,
			Tcl_NewListObj(1, descriptor));
	    } else {
		descriptor[1] = Tcl_NewStringObj(localPtr->name, -1);
		Tcl_ListObjAppendElement(NULL, variables,
			Tcl_NewListObj(2, descriptor));
	    }
	}
    }

    /*
     * Get the instructions from the bytecode.
     */

    instructions = Tcl_NewObj();
    for (pc=codePtr->codeStart; pc<codePtr->codeStart+codePtr->numCodeBytes;){
	const InstructionDesc *instDesc = &tclInstructionTable[*pc];
	int address = pc - codePtr->codeStart;

	inst = Tcl_NewObj();
	Tcl_ListObjAppendElement(NULL, inst, Tcl_NewStringObj(
		instDesc->name, -1));
	opnd = pc + 1;
	for (i=0 ; i<instDesc->numOperands ; i++) {
	    switch (instDesc->opTypes[i]) {
	    case OPERAND_INT1:
		val = TclGetInt1AtPtr(opnd);
		opnd += 1;
		goto formatNumber;
	    case OPERAND_UINT1:
		val = TclGetUInt1AtPtr(opnd);
		opnd += 1;
		goto formatNumber;
	    case OPERAND_INT4:
		val = TclGetInt4AtPtr(opnd);
		opnd += 4;
		goto formatNumber;
	    case OPERAND_UINT4:
		val = TclGetUInt4AtPtr(opnd);
		opnd += 4;
	    formatNumber:
		Tcl_ListObjAppendElement(NULL, inst, Tcl_NewIntObj(val));
		break;

	    case OPERAND_OFFSET1:
		val = TclGetInt1AtPtr(opnd);
		opnd += 1;
		goto formatAddress;
	    case OPERAND_OFFSET4:
		val = TclGetInt4AtPtr(opnd);
		opnd += 4;
	    formatAddress:
		Tcl_ListObjAppendElement(NULL, inst, Tcl_ObjPrintf(
			"pc %d", address + val));
		break;

	    case OPERAND_LIT1:
		val = TclGetUInt1AtPtr(opnd);
		opnd += 1;
		goto formatLiteral;
	    case OPERAND_LIT4:
		val = TclGetUInt4AtPtr(opnd);
		opnd += 4;
	    formatLiteral:
		Tcl_ListObjAppendElement(NULL, inst, Tcl_ObjPrintf(
			"@%d", val));
		break;

	    case OPERAND_LVT1:
		val = TclGetUInt1AtPtr(opnd);
		opnd += 1;
		goto formatVariable;
	    case OPERAND_LVT4:
		val = TclGetUInt4AtPtr(opnd);
		opnd += 4;
	    formatVariable:
		Tcl_ListObjAppendElement(NULL, inst, Tcl_ObjPrintf(
			"%%%d", val));
		break;
	    case OPERAND_IDX4:
		val = TclGetInt4AtPtr(opnd);
		opnd += 4;
		if (val >= -1) {
		    Tcl_ListObjAppendElement(NULL, inst, Tcl_ObjPrintf(
			    ".%d", val));
		} else if (val == -2) {
		    Tcl_ListObjAppendElement(NULL, inst, Tcl_NewStringObj(
			    ".end", -1));
		} else {
		    Tcl_ListObjAppendElement(NULL, inst, Tcl_ObjPrintf(
			    ".end-%d", -2-val));
		}
		break;
	    case OPERAND_AUX4:
		val = TclGetInt4AtPtr(opnd);
		opnd += 4;
		Tcl_ListObjAppendElement(NULL, inst, Tcl_ObjPrintf(
			"?%d", val));
		break;
	    case OPERAND_SCLS1:
		val = TclGetUInt1AtPtr(opnd);
		opnd++;
		Tcl_ListObjAppendElement(NULL, inst, Tcl_ObjPrintf(
			"=%s", tclStringClassTable[val].name));
		break;
	    case OPERAND_NONE:
		Tcl_Panic("opcode %d with more than zero 'no' operands", *pc);
	    }
	}
	Tcl_DictObjPut(NULL, instructions, Tcl_NewIntObj(address), inst);
	pc += instDesc->numBytes;
    }

    /*
     * Get the auxiliary data from the bytecode.
     */

    aux = Tcl_NewObj();
    for (i=0 ; i<codePtr->numAuxDataItems ; i++) {
	AuxData *auxData = &codePtr->auxDataArrayPtr[i];
	Tcl_Obj *auxDesc = Tcl_NewStringObj(auxData->type->name, -1);

	if (auxData->type->disassembleProc) {
	    Tcl_Obj *desc = Tcl_NewObj();

	    Tcl_DictObjPut(NULL, desc, Tcl_NewStringObj("name", -1), auxDesc);
	    auxDesc = desc;
	    auxData->type->disassembleProc(auxData->clientData, auxDesc,
		    codePtr, 0);
	} else if (auxData->type->printProc) {
	    Tcl_Obj *desc = Tcl_NewObj();

	    auxData->type->printProc(auxData->clientData, desc, codePtr, 0);
	    Tcl_ListObjAppendElement(NULL, auxDesc, desc);
	}
	Tcl_ListObjAppendElement(NULL, aux, auxDesc);
    }

    /*
     * Get the exception ranges from the bytecode.
     */

    exn = Tcl_NewObj();
    for (i=0 ; i<codePtr->numExceptRanges ; i++) {
	ExceptionRange *rangePtr = &codePtr->exceptArrayPtr[i];

	switch (rangePtr->type) {
	case LOOP_EXCEPTION_RANGE:
	    Tcl_ListObjAppendElement(NULL, exn, Tcl_ObjPrintf(
		    "type %s level %d from %d to %d break %d continue %d",
		    "loop", rangePtr->nestingLevel, rangePtr->codeOffset,
		    rangePtr->codeOffset + rangePtr->numCodeBytes - 1,
		    rangePtr->breakOffset, rangePtr->continueOffset));
	    break;
	case CATCH_EXCEPTION_RANGE:
	    Tcl_ListObjAppendElement(NULL, exn, Tcl_ObjPrintf(
		    "type %s level %d from %d to %d catch %d",
		    "catch", rangePtr->nestingLevel, rangePtr->codeOffset,
		    rangePtr->codeOffset + rangePtr->numCodeBytes - 1,
		    rangePtr->catchOffset));
	    break;
	}
    }

    /*
     * Get the command information from the bytecode.
     *
     * The way these are encoded in the bytecode is non-trivial; the Decode
     * macro (which updates its argument and returns the next decoded value)
     * handles this so that the rest of the code does not.
     */

#define Decode(ptr) \
    ((TclGetUInt1AtPtr(ptr) == 0xFF)			\
	? ((ptr)+=5 , TclGetInt4AtPtr((ptr)-4))		\
	: ((ptr)+=1 , TclGetInt1AtPtr((ptr)-1)))

    commands = Tcl_NewObj();
    codeOffPtr = codePtr->codeDeltaStart;
    codeLenPtr = codePtr->codeLengthStart;
    srcOffPtr = codePtr->srcDeltaStart;
    srcLenPtr = codePtr->srcLengthStart;
    codeOffset = sourceOffset = 0;
    for (i=0 ; i<codePtr->numCommands ; i++) {
	Tcl_Obj *cmd;

	codeOffset += Decode(codeOffPtr);
	codeLength = Decode(codeLenPtr);
	sourceOffset += Decode(srcOffPtr);
	sourceLength = Decode(srcLenPtr);
	cmd = Tcl_NewObj();
	Tcl_DictObjPut(NULL, cmd, Tcl_NewStringObj("codefrom", -1),
		Tcl_NewIntObj(codeOffset));
	Tcl_DictObjPut(NULL, cmd, Tcl_NewStringObj("codeto", -1),
		Tcl_NewIntObj(codeOffset + codeLength - 1));

	/*
	 * Convert byte offsets to character offsets; important if multibyte
	 * characters are present in the source!
	 */

	Tcl_DictObjPut(NULL, cmd, Tcl_NewStringObj("scriptfrom", -1),
		Tcl_NewIntObj(Tcl_NumUtfChars(codePtr->source,
			sourceOffset)));
	Tcl_DictObjPut(NULL, cmd, Tcl_NewStringObj("scriptto", -1),
		Tcl_NewIntObj(Tcl_NumUtfChars(codePtr->source,
			sourceOffset + sourceLength - 1)));
	Tcl_DictObjPut(NULL, cmd, Tcl_NewStringObj("script", -1),
		Tcl_NewStringObj(codePtr->source+sourceOffset, sourceLength));
	Tcl_ListObjAppendElement(NULL, commands, cmd);
    }

#undef Decode

    /*
     * Get the source file and line number information from the CmdFrame
     * system if it is available.
     */

    GetLocationInformation(codePtr->procPtr, &file, &line);

    /*
     * Build the overall result.
     */

    description = Tcl_NewObj();
    Tcl_DictObjPut(NULL, description, Tcl_NewStringObj("literals", -1),
	    literals);
    Tcl_DictObjPut(NULL, description, Tcl_NewStringObj("variables", -1),
	    variables);
    Tcl_DictObjPut(NULL, description, Tcl_NewStringObj("exception", -1), exn);
    Tcl_DictObjPut(NULL, description, Tcl_NewStringObj("instructions", -1),
	    instructions);
    Tcl_DictObjPut(NULL, description, Tcl_NewStringObj("auxiliary", -1), aux);
    Tcl_DictObjPut(NULL, description, Tcl_NewStringObj("commands", -1),
	    commands);
    Tcl_DictObjPut(NULL, description, Tcl_NewStringObj("script", -1),
	    Tcl_NewStringObj(codePtr->source, codePtr->numSrcBytes));
    Tcl_DictObjPut(NULL, description, Tcl_NewStringObj("namespace", -1),
	    Tcl_NewStringObj(codePtr->nsPtr->fullName, -1));
    Tcl_DictObjPut(NULL, description, Tcl_NewStringObj("stackdepth", -1),
	    Tcl_NewIntObj(codePtr->maxStackDepth));
    Tcl_DictObjPut(NULL, description, Tcl_NewStringObj("exceptdepth", -1),
	    Tcl_NewIntObj(codePtr->maxExceptDepth));
    if (line > -1) {
	Tcl_DictObjPut(NULL, description,
		Tcl_NewStringObj("initiallinenumber", -1),
		Tcl_NewIntObj(line));
    }
    if (file) {
	Tcl_DictObjPut(NULL, description,
		Tcl_NewStringObj("sourcefile", -1), file);
    }
    return description;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DisassembleObjCmd --
 *
 *	Implementation of the "::tcl::unsupported::disassemble" command. This
 *	command is not documented, but will disassemble procedures, lambda
 *	terms and general scripts. Note that will compile terms if necessary
 *	in order to disassemble them.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_DisassembleObjCmd(
    ClientData clientData,	/* What type of operation. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const types[] = {
	"constructor", "destructor",
	"lambda", "method", "objmethod", "proc", "script", NULL
    };
    enum Types {
	DISAS_CLASS_CONSTRUCTOR, DISAS_CLASS_DESTRUCTOR,
	DISAS_LAMBDA, DISAS_CLASS_METHOD, DISAS_OBJECT_METHOD, DISAS_PROC,
	DISAS_SCRIPT
    };
    int idx, result;
    Tcl_Obj *codeObjPtr = NULL;
    Proc *procPtr = NULL;
    Tcl_HashEntry *hPtr;
    Object *oPtr;
    Method *methodPtr;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "type ...");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], types, "type", 0, &idx)!=TCL_OK){
	return TCL_ERROR;
    }

    switch ((enum Types) idx) {
    case DISAS_LAMBDA: {
	Command cmd;
	Tcl_Obj *nsObjPtr;
	Tcl_Namespace *nsPtr;

	/*
	 * Compile (if uncompiled) and disassemble a lambda term.
	 *
	 * WARNING! Pokes inside the lambda objtype.
	 */

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "lambdaTerm");
	    return TCL_ERROR;
	}
	if (objv[2]->typePtr == &tclLambdaType) {
	    procPtr = objv[2]->internalRep.twoPtrValue.ptr1;
	}
	if (procPtr == NULL || procPtr->iPtr != (Interp *) interp) {
	    result = tclLambdaType.setFromAnyProc(interp, objv[2]);
	    if (result != TCL_OK) {
		return result;
	    }
	    procPtr = objv[2]->internalRep.twoPtrValue.ptr1;
	}

	memset(&cmd, 0, sizeof(Command));
	nsObjPtr = objv[2]->internalRep.twoPtrValue.ptr2;
	result = TclGetNamespaceFromObj(interp, nsObjPtr, &nsPtr);
	if (result != TCL_OK) {
	    return result;
	}
	cmd.nsPtr = (Namespace *) nsPtr;
	procPtr->cmdPtr = &cmd;
	result = TclPushProcCallFrame(procPtr, interp, objc, objv, 1);
	if (result != TCL_OK) {
	    return result;
	}
	TclPopStackFrame(interp);
	codeObjPtr = procPtr->bodyPtr;
	break;
    }
    case DISAS_PROC:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "procName");
	    return TCL_ERROR;
	}

	procPtr = TclFindProc((Interp *) interp, TclGetString(objv[2]));
	if (procPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "\"%s\" isn't a procedure", TclGetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "PROC",
		    TclGetString(objv[2]), NULL);
	    return TCL_ERROR;
	}

	/*
	 * Compile (if uncompiled) and disassemble a procedure.
	 */

	result = TclPushProcCallFrame(procPtr, interp, 2, objv+1, 1);
	if (result != TCL_OK) {
	    return result;
	}
	TclPopStackFrame(interp);
	codeObjPtr = procPtr->bodyPtr;
	break;
    case DISAS_SCRIPT:
	/*
	 * Compile and disassemble a script.
	 */

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "script");
	    return TCL_ERROR;
	}
	if ((objv[2]->typePtr != &tclByteCodeType)
		&& (TclSetByteCodeFromAny(interp, objv[2], NULL, NULL) != TCL_OK)) {
	    return TCL_ERROR;
	}
	codeObjPtr = objv[2];
	break;

    case DISAS_CLASS_CONSTRUCTOR:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "className");
	    return TCL_ERROR;
	}

	/*
	 * Look up the body of a constructor.
	 */

	oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[2]);
	if (oPtr == NULL) {
	    return TCL_ERROR;
	}
	if (oPtr->classPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "\"%s\" is not a class", TclGetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "CLASS",
		    TclGetString(objv[2]), NULL);
	    return TCL_ERROR;
	}

	methodPtr = oPtr->classPtr->constructorPtr;
	if (methodPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "\"%s\" has no defined constructor",
		    TclGetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "DISASSEMBLE",
		    "CONSRUCTOR", NULL);
	    return TCL_ERROR;
	}
	procPtr = TclOOGetProcFromMethod(methodPtr);
	if (procPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "body not available for this kind of constructor", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "DISASSEMBLE",
		    "METHODTYPE", NULL);
	    return TCL_ERROR;
	}

	/*
	 * Compile if necessary.
	 */

	if (procPtr->bodyPtr->typePtr != &tclByteCodeType) {
	    Command cmd;

	    /*
	     * Yes, this is ugly, but we need to pass the namespace in to the
	     * compiler in two places.
	     */

	    cmd.nsPtr = (Namespace *) oPtr->namespacePtr;
	    procPtr->cmdPtr = &cmd;
	    result = TclProcCompileProc(interp, procPtr, procPtr->bodyPtr,
		    (Namespace *) oPtr->namespacePtr, "body of constructor",
		    TclGetString(objv[2]));
	    procPtr->cmdPtr = NULL;
	    if (result != TCL_OK) {
		return result;
	    }
	}
	codeObjPtr = procPtr->bodyPtr;
	break;

    case DISAS_CLASS_DESTRUCTOR:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "className");
	    return TCL_ERROR;
	}

	/*
	 * Look up the body of a destructor.
	 */

	oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[2]);
	if (oPtr == NULL) {
	    return TCL_ERROR;
	}
	if (oPtr->classPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "\"%s\" is not a class", TclGetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "CLASS",
		    TclGetString(objv[2]), NULL);
	    return TCL_ERROR;
	}

	methodPtr = oPtr->classPtr->destructorPtr;
	if (methodPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "\"%s\" has no defined destructor",
		    TclGetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "DISASSEMBLE",
		    "DESRUCTOR", NULL);
	    return TCL_ERROR;
	}
	procPtr = TclOOGetProcFromMethod(methodPtr);
	if (procPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "body not available for this kind of destructor", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "DISASSEMBLE",
		    "METHODTYPE", NULL);
	    return TCL_ERROR;
	}

	/*
	 * Compile if necessary.
	 */

	if (procPtr->bodyPtr->typePtr != &tclByteCodeType) {
	    Command cmd;

	    /*
	     * Yes, this is ugly, but we need to pass the namespace in to the
	     * compiler in two places.
	     */

	    cmd.nsPtr = (Namespace *) oPtr->namespacePtr;
	    procPtr->cmdPtr = &cmd;
	    result = TclProcCompileProc(interp, procPtr, procPtr->bodyPtr,
		    (Namespace *) oPtr->namespacePtr, "body of destructor",
		    TclGetString(objv[2]));
	    procPtr->cmdPtr = NULL;
	    if (result != TCL_OK) {
		return result;
	    }
	}
	codeObjPtr = procPtr->bodyPtr;
	break;

    case DISAS_CLASS_METHOD:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "className methodName");
	    return TCL_ERROR;
	}

	/*
	 * Look up the body of a class method.
	 */

	oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[2]);
	if (oPtr == NULL) {
	    return TCL_ERROR;
	}
	if (oPtr->classPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "\"%s\" is not a class", TclGetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "CLASS",
		    TclGetString(objv[2]), NULL);
	    return TCL_ERROR;
	}
	hPtr = Tcl_FindHashEntry(&oPtr->classPtr->classMethods,
		(char *) objv[3]);
	goto methodBody;
    case DISAS_OBJECT_METHOD:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "objectName methodName");
	    return TCL_ERROR;
	}

	/*
	 * Look up the body of an instance method.
	 */

	oPtr = (Object *) Tcl_GetObjectFromObj(interp, objv[2]);
	if (oPtr == NULL) {
	    return TCL_ERROR;
	}
	if (oPtr->methodsPtr == NULL) {
	    goto unknownMethod;
	}
	hPtr = Tcl_FindHashEntry(oPtr->methodsPtr, (char *) objv[3]);

	/*
	 * Compile (if necessary) and disassemble a method body.
	 */

    methodBody:
	if (hPtr == NULL) {
	unknownMethod:
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "unknown method \"%s\"", TclGetString(objv[3])));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		    TclGetString(objv[3]), NULL);
	    return TCL_ERROR;
	}
	procPtr = TclOOGetProcFromMethod(Tcl_GetHashValue(hPtr));
	if (procPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "body not available for this kind of method", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "DISASSEMBLE",
		    "METHODTYPE", NULL);
	    return TCL_ERROR;
	}
	if (procPtr->bodyPtr->typePtr != &tclByteCodeType) {
	    Command cmd;

	    /*
	     * Yes, this is ugly, but we need to pass the namespace in to the
	     * compiler in two places.
	     */

	    cmd.nsPtr = (Namespace *) oPtr->namespacePtr;
	    procPtr->cmdPtr = &cmd;
	    result = TclProcCompileProc(interp, procPtr, procPtr->bodyPtr,
		    (Namespace *) oPtr->namespacePtr, "body of method",
		    TclGetString(objv[3]));
	    procPtr->cmdPtr = NULL;
	    if (result != TCL_OK) {
		return result;
	    }
	}
	codeObjPtr = procPtr->bodyPtr;
	break;
    default:
	CLANG_ASSERT(0);
    }

    /*
     * Do the actual disassembly.
     */

    if (BYTECODE(codeObjPtr)->flags & TCL_BYTECODE_PRECOMPILED) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"may not disassemble prebuilt bytecode", -1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "DISASSEMBLE",
		"BYTECODE", NULL);
	return TCL_ERROR;
    }
    if (PTR2INT(clientData)) {
	Tcl_SetObjResult(interp,
		DisassembleByteCodeAsDicts(interp, codeObjPtr));
    } else {
	Tcl_SetObjResult(interp,
		DisassembleByteCodeObj(interp, codeObjPtr));
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
