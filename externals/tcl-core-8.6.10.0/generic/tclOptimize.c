/*
 * tclOptimize.c --
 *
 *	This file contains the bytecode optimizer.
 *
 * Copyright (c) 2013 by Donal Fellows.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclCompile.h"
#include <assert.h>

/*
 * Forward declarations.
 */

static void		AdvanceJumps(CompileEnv *envPtr);
static void		ConvertZeroEffectToNOP(CompileEnv *envPtr);
static void		LocateTargetAddresses(CompileEnv *envPtr,
			    Tcl_HashTable *tablePtr);
static void		TrimUnreachable(CompileEnv *envPtr);

/*
 * Helper macros.
 */

#define DefineTargetAddress(tablePtr, address) \
    ((void) Tcl_CreateHashEntry((tablePtr), (void *) (address), &isNew))
#define IsTargetAddress(tablePtr, address) \
    (Tcl_FindHashEntry((tablePtr), (void *) (address)) != NULL)
#define AddrLength(address) \
    (tclInstructionTable[*(unsigned char *)(address)].numBytes)
#define InstLength(instruction) \
    (tclInstructionTable[(unsigned char)(instruction)].numBytes)

/*
 * ----------------------------------------------------------------------
 *
 * LocateTargetAddresses --
 *
 *	Populate a hash table with places that we need to be careful around
 *	because they're the targets of various kinds of jumps and other
 *	non-local behavior.
 *
 * ----------------------------------------------------------------------
 */

static void
LocateTargetAddresses(
    CompileEnv *envPtr,
    Tcl_HashTable *tablePtr)
{
    unsigned char *currentInstPtr, *targetInstPtr;
    int isNew, i;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch hSearch;

    Tcl_InitHashTable(tablePtr, TCL_ONE_WORD_KEYS);

    /*
     * The starts of commands represent target addresses.
     */

    for (i=0 ; i<envPtr->numCommands ; i++) {
	DefineTargetAddress(tablePtr,
		envPtr->codeStart + envPtr->cmdMapPtr[i].codeOffset);
    }

    /*
     * Find places where we should be careful about replacing instructions
     * because they are the targets of various types of jumps.
     */

    for (currentInstPtr = envPtr->codeStart ;
	    currentInstPtr < envPtr->codeNext ;
	    currentInstPtr += AddrLength(currentInstPtr)) {
	switch (*currentInstPtr) {
	case INST_JUMP1:
	case INST_JUMP_TRUE1:
	case INST_JUMP_FALSE1:
	    targetInstPtr = currentInstPtr+TclGetInt1AtPtr(currentInstPtr+1);
	    goto storeTarget;
	case INST_JUMP4:
	case INST_JUMP_TRUE4:
	case INST_JUMP_FALSE4:
	case INST_START_CMD:
	    targetInstPtr = currentInstPtr+TclGetInt4AtPtr(currentInstPtr+1);
	    goto storeTarget;
	case INST_BEGIN_CATCH4:
	    targetInstPtr = envPtr->codeStart + envPtr->exceptArrayPtr[
		    TclGetUInt4AtPtr(currentInstPtr+1)].codeOffset;
	storeTarget:
	    DefineTargetAddress(tablePtr, targetInstPtr);
	    break;
	case INST_JUMP_TABLE:
	    hPtr = Tcl_FirstHashEntry(
		    &JUMPTABLEINFO(envPtr, currentInstPtr+1)->hashTable,
		    &hSearch);
	    for (; hPtr ; hPtr = Tcl_NextHashEntry(&hSearch)) {
		targetInstPtr = currentInstPtr +
			PTR2INT(Tcl_GetHashValue(hPtr));
		DefineTargetAddress(tablePtr, targetInstPtr);
	    }
	    break;
	case INST_RETURN_CODE_BRANCH:
	    for (i=TCL_ERROR ; i<TCL_CONTINUE+1 ; i++) {
		DefineTargetAddress(tablePtr, currentInstPtr + 2*i - 1);
	    }
	    break;
	}
    }

    /*
     * Add a marker *after* the last bytecode instruction. WARNING: points to
     * one past the end!
     */

    DefineTargetAddress(tablePtr, currentInstPtr);

    /*
     * Enter in the targets of exception ranges.
     */

    for (i=0 ; i<envPtr->exceptArrayNext ; i++) {
	ExceptionRange *rangePtr = &envPtr->exceptArrayPtr[i];

	if (rangePtr->type == CATCH_EXCEPTION_RANGE) {
	    targetInstPtr = envPtr->codeStart + rangePtr->catchOffset;
	    DefineTargetAddress(tablePtr, targetInstPtr);
	} else {
	    targetInstPtr = envPtr->codeStart + rangePtr->breakOffset;
	    DefineTargetAddress(tablePtr, targetInstPtr);
	    if (rangePtr->continueOffset >= 0) {
		targetInstPtr = envPtr->codeStart + rangePtr->continueOffset;
		DefineTargetAddress(tablePtr, targetInstPtr);
	    }
	}
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * TrimUnreachable --
 *
 *	Converts code that provably can't be executed into NOPs and reduces
 *	the overall reported length of the bytecode where that is possible.
 *
 * ----------------------------------------------------------------------
 */

static void
TrimUnreachable(
    CompileEnv *envPtr)
{
    unsigned char *currentInstPtr;
    Tcl_HashTable targets;

    LocateTargetAddresses(envPtr, &targets);

    for (currentInstPtr = envPtr->codeStart ;
	    currentInstPtr < envPtr->codeNext-1 ;
	    currentInstPtr += AddrLength(currentInstPtr)) {
	int clear = 0;

	if (*currentInstPtr != INST_DONE) {
	    continue;
	}

	while (!IsTargetAddress(&targets, currentInstPtr + 1 + clear)) {
	    clear += AddrLength(currentInstPtr + 1 + clear);
	}
	if (currentInstPtr + 1 + clear == envPtr->codeNext) {
	    envPtr->codeNext -= clear;
	} else {
	    while (clear --> 0) {
		*(currentInstPtr + 1 + clear) = INST_NOP;
	    }
	}
    }

    Tcl_DeleteHashTable(&targets);
}

/*
 * ----------------------------------------------------------------------
 *
 * ConvertZeroEffectToNOP --
 *
 *	Replace PUSH/POP sequences (when non-hazardous) with NOPs. Also
 *	replace PUSH empty/STR_CONCAT and TRY_CVT_NUMERIC (when followed by an
 *	operation that guarantees the check for arithmeticity) and eliminate
 *	LNOT when we can invert the following JUMP condition.
 *
 * ----------------------------------------------------------------------
 */

static void
ConvertZeroEffectToNOP(
    CompileEnv *envPtr)
{
    unsigned char *currentInstPtr;
    int size;
    Tcl_HashTable targets;

    LocateTargetAddresses(envPtr, &targets);
    for (currentInstPtr = envPtr->codeStart ;
	    currentInstPtr < envPtr->codeNext ; currentInstPtr += size) {
	int blank = 0, i, nextInst;

	size = AddrLength(currentInstPtr);
	while ((currentInstPtr + size < envPtr->codeNext)
		&& *(currentInstPtr+size) == INST_NOP) {
	    if (IsTargetAddress(&targets, currentInstPtr + size)) {
		break;
	    }
	    size += InstLength(INST_NOP);
	}
	if (IsTargetAddress(&targets, currentInstPtr + size)) {
	    continue;
	}
	nextInst = *(currentInstPtr + size);
	switch (*currentInstPtr) {
	case INST_PUSH1:
	    if (nextInst == INST_POP) {
		blank = size + InstLength(nextInst);
	    } else if (nextInst == INST_STR_CONCAT1
		    && TclGetUInt1AtPtr(currentInstPtr + size + 1) == 2) {
		Tcl_Obj *litPtr = TclFetchLiteral(envPtr,
			TclGetUInt1AtPtr(currentInstPtr + 1));
		int numBytes;

		(void) Tcl_GetStringFromObj(litPtr, &numBytes);
		if (numBytes == 0) {
		    blank = size + InstLength(nextInst);
		}
	    }
	    break;
	case INST_PUSH4:
	    if (nextInst == INST_POP) {
		blank = size + 1;
	    } else if (nextInst == INST_STR_CONCAT1
		    && TclGetUInt1AtPtr(currentInstPtr + size + 1) == 2) {
		Tcl_Obj *litPtr = TclFetchLiteral(envPtr,
			TclGetUInt4AtPtr(currentInstPtr + 1));
		int numBytes;

		(void) Tcl_GetStringFromObj(litPtr, &numBytes);
		if (numBytes == 0) {
		    blank = size + InstLength(nextInst);
		}
	    }
	    break;

	case INST_LNOT:
	    switch (nextInst) {
	    case INST_JUMP_TRUE1:
		blank = size;
		*(currentInstPtr + size) = INST_JUMP_FALSE1;
		break;
	    case INST_JUMP_FALSE1:
		blank = size;
		*(currentInstPtr + size) = INST_JUMP_TRUE1;
		break;
	    case INST_JUMP_TRUE4:
		blank = size;
		*(currentInstPtr + size) = INST_JUMP_FALSE4;
		break;
	    case INST_JUMP_FALSE4:
		blank = size;
		*(currentInstPtr + size) = INST_JUMP_TRUE4;
		break;
	    }
	    break;

	case INST_TRY_CVT_TO_NUMERIC:
	    switch (nextInst) {
	    case INST_JUMP_TRUE1:
	    case INST_JUMP_TRUE4:
	    case INST_JUMP_FALSE1:
	    case INST_JUMP_FALSE4:
	    case INST_INCR_SCALAR1:
	    case INST_INCR_ARRAY1:
	    case INST_INCR_ARRAY_STK:
	    case INST_INCR_SCALAR_STK:
	    case INST_INCR_STK:
	    case INST_LOR:
	    case INST_LAND:
	    case INST_EQ:
	    case INST_NEQ:
	    case INST_LT:
	    case INST_LE:
	    case INST_GT:
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
	    case INST_LNOT:
	    case INST_BITNOT:
	    case INST_UMINUS:
	    case INST_UPLUS:
	    case INST_TRY_CVT_TO_NUMERIC:
		blank = size;
		break;
	    }
	    break;
	}

	if (blank > 0) {
	    for (i=0 ; i<blank ; i++) {
		*(currentInstPtr + i) = INST_NOP;
	    }
	    size = blank;
	}
    }
    Tcl_DeleteHashTable(&targets);
}

/*
 * ----------------------------------------------------------------------
 *
 * AdvanceJumps --
 *
 *	Advance jumps past NOPs and chained JUMPs. After this runs, the only
 *	JUMPs that jump to a NOP or a JUMP will be length-1 ones that run out
 *	of room in their opcode to be targeted to where they really belong.
 *
 * ----------------------------------------------------------------------
 */

static void
AdvanceJumps(
    CompileEnv *envPtr)
{
    unsigned char *currentInstPtr;
    Tcl_HashTable jumps;

    for (currentInstPtr = envPtr->codeStart ;
	    currentInstPtr < envPtr->codeNext-1 ;
	    currentInstPtr += AddrLength(currentInstPtr)) {
	int offset, delta, isNew;

	switch (*currentInstPtr) {
	case INST_JUMP1:
	case INST_JUMP_TRUE1:
	case INST_JUMP_FALSE1:
	    offset = TclGetInt1AtPtr(currentInstPtr + 1);
	    Tcl_InitHashTable(&jumps, TCL_ONE_WORD_KEYS);
	    for (delta=0 ; offset+delta != 0 ;) {
		if (offset + delta < -128 || offset + delta > 127) {
		    break;
		}
		Tcl_CreateHashEntry(&jumps, INT2PTR(offset), &isNew);
		if (!isNew) {
		    offset = TclGetInt1AtPtr(currentInstPtr + 1);
		    break;
		}
		offset += delta;
		switch (*(currentInstPtr + offset)) {
		case INST_NOP:
		    delta = InstLength(INST_NOP);
		    continue;
		case INST_JUMP1:
		    delta = TclGetInt1AtPtr(currentInstPtr + offset + 1);
		    continue;
		case INST_JUMP4:
		    delta = TclGetInt4AtPtr(currentInstPtr + offset + 1);
		    continue;
		}
		break;
	    }
	    Tcl_DeleteHashTable(&jumps);
	    TclStoreInt1AtPtr(offset, currentInstPtr + 1);
	    continue;

	case INST_JUMP4:
	case INST_JUMP_TRUE4:
	case INST_JUMP_FALSE4:
	    Tcl_InitHashTable(&jumps, TCL_ONE_WORD_KEYS);
	    Tcl_CreateHashEntry(&jumps, INT2PTR(0), &isNew);
	    for (offset = TclGetInt4AtPtr(currentInstPtr + 1); offset!=0 ;) {
		Tcl_CreateHashEntry(&jumps, INT2PTR(offset), &isNew);
		if (!isNew) {
		    offset = TclGetInt4AtPtr(currentInstPtr + 1);
		    break;
		}
		switch (*(currentInstPtr + offset)) {
		case INST_NOP:
		    offset += InstLength(INST_NOP);
		    continue;
		case INST_JUMP1:
		    offset += TclGetInt1AtPtr(currentInstPtr + offset + 1);
		    continue;
		case INST_JUMP4:
		    offset += TclGetInt4AtPtr(currentInstPtr + offset + 1);
		    continue;
		}
		break;
	    }
	    Tcl_DeleteHashTable(&jumps);
	    TclStoreInt4AtPtr(offset, currentInstPtr + 1);
	    continue;
	}
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOptimizeBytecode --
 *
 *	A very simple peephole optimizer for bytecode.
 *
 * ----------------------------------------------------------------------
 */

void
TclOptimizeBytecode(
    void *envPtr)
{
    ConvertZeroEffectToNOP(envPtr);
    AdvanceJumps(envPtr);
    TrimUnreachable(envPtr);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
