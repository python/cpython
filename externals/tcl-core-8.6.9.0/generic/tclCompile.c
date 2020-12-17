/*
 * tclCompile.c --
 *
 *	This file contains procedures that compile Tcl commands or parts of
 *	commands (like quoted strings or nested sub-commands) into a sequence
 *	of instructions ("bytecodes").
 *
 * Copyright (c) 1996-1998 Sun Microsystems, Inc.
 * Copyright (c) 2001 by Kevin B. Kenny. All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclCompile.h"
#include <assert.h>

/*
 * Variable that controls whether compilation tracing is enabled and, if so,
 * what level of tracing is desired:
 *    0: no compilation tracing
 *    1: summarize compilation of top level cmds and proc bodies
 *    2: display all instructions of each ByteCode compiled
 * This variable is linked to the Tcl variable "tcl_traceCompile".
 */

#ifdef TCL_COMPILE_DEBUG
int tclTraceCompile = 0;
static int traceInitialized = 0;
#endif

/*
 * A table describing the Tcl bytecode instructions. Entries in this table
 * must correspond to the instruction opcode definitions in tclCompile.h. The
 * names "op1" and "op4" refer to an instruction's one or four byte first
 * operand. Similarly, "stktop" and "stknext" refer to the topmost and next to
 * topmost stack elements.
 *
 * Note that the load, store, and incr instructions do not distinguish local
 * from global variables; the bytecode interpreter at runtime uses the
 * existence of a procedure call frame to distinguish these.
 */

InstructionDesc const tclInstructionTable[] = {
    /* Name	      Bytes stackEffect #Opnds  Operand types */
    {"done",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Finish ByteCode execution and return stktop (top stack item) */
    {"push1",		  2,   +1,         1,	{OPERAND_LIT1}},
	/* Push object at ByteCode objArray[op1] */
    {"push4",		  5,   +1,         1,	{OPERAND_LIT4}},
	/* Push object at ByteCode objArray[op4] */
    {"pop",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Pop the topmost stack object */
    {"dup",		  1,   +1,         0,	{OPERAND_NONE}},
	/* Duplicate the topmost stack object and push the result */
    {"strcat",		  2,   INT_MIN,    1,	{OPERAND_UINT1}},
	/* Concatenate the top op1 items and push result */
    {"invokeStk1",	  2,   INT_MIN,    1,	{OPERAND_UINT1}},
	/* Invoke command named objv[0]; <objc,objv> = <op1,top op1> */
    {"invokeStk4",	  5,   INT_MIN,    1,	{OPERAND_UINT4}},
	/* Invoke command named objv[0]; <objc,objv> = <op4,top op4> */
    {"evalStk",		  1,   0,          0,	{OPERAND_NONE}},
	/* Evaluate command in stktop using Tcl_EvalObj. */
    {"exprStk",		  1,   0,          0,	{OPERAND_NONE}},
	/* Execute expression in stktop using Tcl_ExprStringObj. */

    {"loadScalar1",	  2,   1,          1,	{OPERAND_LVT1}},
	/* Load scalar variable at index op1 <= 255 in call frame */
    {"loadScalar4",	  5,   1,          1,	{OPERAND_LVT4}},
	/* Load scalar variable at index op1 >= 256 in call frame */
    {"loadScalarStk",	  1,   0,          0,	{OPERAND_NONE}},
	/* Load scalar variable; scalar's name is stktop */
    {"loadArray1",	  2,   0,          1,	{OPERAND_LVT1}},
	/* Load array element; array at slot op1<=255, element is stktop */
    {"loadArray4",	  5,   0,          1,	{OPERAND_LVT4}},
	/* Load array element; array at slot op1 > 255, element is stktop */
    {"loadArrayStk",	  1,   -1,         0,	{OPERAND_NONE}},
	/* Load array element; element is stktop, array name is stknext */
    {"loadStk",		  1,   0,          0,	{OPERAND_NONE}},
	/* Load general variable; unparsed variable name is stktop */
    {"storeScalar1",	  2,   0,          1,	{OPERAND_LVT1}},
	/* Store scalar variable at op1<=255 in frame; value is stktop */
    {"storeScalar4",	  5,   0,          1,	{OPERAND_LVT4}},
	/* Store scalar variable at op1 > 255 in frame; value is stktop */
    {"storeScalarStk",	  1,   -1,         0,	{OPERAND_NONE}},
	/* Store scalar; value is stktop, scalar name is stknext */
    {"storeArray1",	  2,   -1,         1,	{OPERAND_LVT1}},
	/* Store array element; array at op1<=255, value is top then elem */
    {"storeArray4",	  5,   -1,         1,	{OPERAND_LVT4}},
	/* Store array element; array at op1>=256, value is top then elem */
    {"storeArrayStk",	  1,   -2,         0,	{OPERAND_NONE}},
	/* Store array element; value is stktop, then elem, array names */
    {"storeStk",	  1,   -1,         0,	{OPERAND_NONE}},
	/* Store general variable; value is stktop, then unparsed name */

    {"incrScalar1",	  2,   0,          1,	{OPERAND_LVT1}},
	/* Incr scalar at index op1<=255 in frame; incr amount is stktop */
    {"incrScalarStk",	  1,   -1,         0,	{OPERAND_NONE}},
	/* Incr scalar; incr amount is stktop, scalar's name is stknext */
    {"incrArray1",	  2,   -1,         1,	{OPERAND_LVT1}},
	/* Incr array elem; arr at slot op1<=255, amount is top then elem */
    {"incrArrayStk",	  1,   -2,         0,	{OPERAND_NONE}},
	/* Incr array element; amount is top then elem then array names */
    {"incrStk",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Incr general variable; amount is stktop then unparsed var name */
    {"incrScalar1Imm",	  3,   +1,         2,	{OPERAND_LVT1, OPERAND_INT1}},
	/* Incr scalar at slot op1 <= 255; amount is 2nd operand byte */
    {"incrScalarStkImm",  2,   0,          1,	{OPERAND_INT1}},
	/* Incr scalar; scalar name is stktop; incr amount is op1 */
    {"incrArray1Imm",	  3,   0,          2,	{OPERAND_LVT1, OPERAND_INT1}},
	/* Incr array elem; array at slot op1 <= 255, elem is stktop,
	 * amount is 2nd operand byte */
    {"incrArrayStkImm",	  2,   -1,         1,	{OPERAND_INT1}},
	/* Incr array element; elem is top then array name, amount is op1 */
    {"incrStkImm",	  2,   0,	   1,	{OPERAND_INT1}},
	/* Incr general variable; unparsed name is top, amount is op1 */

    {"jump1",		  2,   0,          1,	{OPERAND_OFFSET1}},
	/* Jump relative to (pc + op1) */
    {"jump4",		  5,   0,          1,	{OPERAND_OFFSET4}},
	/* Jump relative to (pc + op4) */
    {"jumpTrue1",	  2,   -1,         1,	{OPERAND_OFFSET1}},
	/* Jump relative to (pc + op1) if stktop expr object is true */
    {"jumpTrue4",	  5,   -1,         1,	{OPERAND_OFFSET4}},
	/* Jump relative to (pc + op4) if stktop expr object is true */
    {"jumpFalse1",	  2,   -1,         1,	{OPERAND_OFFSET1}},
	/* Jump relative to (pc + op1) if stktop expr object is false */
    {"jumpFalse4",	  5,   -1,         1,	{OPERAND_OFFSET4}},
	/* Jump relative to (pc + op4) if stktop expr object is false */

    {"lor",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Logical or:	push (stknext || stktop) */
    {"land",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Logical and:	push (stknext && stktop) */
    {"bitor",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Bitwise or:	push (stknext | stktop) */
    {"bitxor",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Bitwise xor	push (stknext ^ stktop) */
    {"bitand",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Bitwise and:	push (stknext & stktop) */
    {"eq",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Equal:	push (stknext == stktop) */
    {"neq",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Not equal:	push (stknext != stktop) */
    {"lt",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Less:	push (stknext < stktop) */
    {"gt",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Greater:	push (stknext > stktop) */
    {"le",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Less or equal: push (stknext <= stktop) */
    {"ge",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Greater or equal: push (stknext >= stktop) */
    {"lshift",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Left shift:	push (stknext << stktop) */
    {"rshift",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Right shift:	push (stknext >> stktop) */
    {"add",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Add:		push (stknext + stktop) */
    {"sub",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Sub:		push (stkext - stktop) */
    {"mult",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Multiply:	push (stknext * stktop) */
    {"div",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Divide:	push (stknext / stktop) */
    {"mod",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Mod:		push (stknext % stktop) */
    {"uplus",		  1,   0,          0,	{OPERAND_NONE}},
	/* Unary plus:	push +stktop */
    {"uminus",		  1,   0,          0,	{OPERAND_NONE}},
	/* Unary minus:	push -stktop */
    {"bitnot",		  1,   0,          0,	{OPERAND_NONE}},
	/* Bitwise not:	push ~stktop */
    {"not",		  1,   0,          0,	{OPERAND_NONE}},
	/* Logical not:	push !stktop */
    {"callBuiltinFunc1",  2,   1,          1,	{OPERAND_UINT1}},
	/* Call builtin math function with index op1; any args are on stk */
    {"callFunc1",	  2,   INT_MIN,    1,	{OPERAND_UINT1}},
	/* Call non-builtin func objv[0]; <objc,objv>=<op1,top op1> */
    {"tryCvtToNumeric",	  1,   0,          0,	{OPERAND_NONE}},
	/* Try converting stktop to first int then double if possible. */

    {"break",		  1,   0,          0,	{OPERAND_NONE}},
	/* Abort closest enclosing loop; if none, return TCL_BREAK code. */
    {"continue",	  1,   0,          0,	{OPERAND_NONE}},
	/* Skip to next iteration of closest enclosing loop; if none, return
	 * TCL_CONTINUE code. */

    {"foreach_start4",	  5,   0,          1,	{OPERAND_AUX4}},
	/* Initialize execution of a foreach loop. Operand is aux data index
	 * of the ForeachInfo structure for the foreach command. */
    {"foreach_step4",	  5,   +1,         1,	{OPERAND_AUX4}},
	/* "Step" or begin next iteration of foreach loop. Push 0 if to
	 * terminate loop, else push 1. */

    {"beginCatch4",	  5,   0,          1,	{OPERAND_UINT4}},
	/* Record start of catch with the operand's exception index. Push the
	 * current stack depth onto a special catch stack. */
    {"endCatch",	  1,   0,          0,	{OPERAND_NONE}},
	/* End of last catch. Pop the bytecode interpreter's catch stack. */
    {"pushResult",	  1,   +1,         0,	{OPERAND_NONE}},
	/* Push the interpreter's object result onto the stack. */
    {"pushReturnCode",	  1,   +1,         0,	{OPERAND_NONE}},
	/* Push interpreter's return code (e.g. TCL_OK or TCL_ERROR) as a new
	 * object onto the stack. */

    {"streq",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Str Equal:	push (stknext eq stktop) */
    {"strneq",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Str !Equal:	push (stknext neq stktop) */
    {"strcmp",		  1,   -1,         0,	{OPERAND_NONE}},
	/* Str Compare:	push (stknext cmp stktop) */
    {"strlen",		  1,   0,          0,	{OPERAND_NONE}},
	/* Str Length:	push (strlen stktop) */
    {"strindex",	  1,   -1,         0,	{OPERAND_NONE}},
	/* Str Index:	push (strindex stknext stktop) */
    {"strmatch",	  2,   -1,         1,	{OPERAND_INT1}},
	/* Str Match:	push (strmatch stknext stktop) opnd == nocase */

    {"list",		  5,   INT_MIN,    1,	{OPERAND_UINT4}},
	/* List:	push (stk1 stk2 ... stktop) */
    {"listIndex",	  1,   -1,         0,	{OPERAND_NONE}},
	/* List Index:	push (listindex stknext stktop) */
    {"listLength",	  1,   0,          0,	{OPERAND_NONE}},
	/* List Len:	push (listlength stktop) */

    {"appendScalar1",	  2,   0,          1,	{OPERAND_LVT1}},
	/* Append scalar variable at op1<=255 in frame; value is stktop */
    {"appendScalar4",	  5,   0,          1,	{OPERAND_LVT4}},
	/* Append scalar variable at op1 > 255 in frame; value is stktop */
    {"appendArray1",	  2,   -1,         1,	{OPERAND_LVT1}},
	/* Append array element; array at op1<=255, value is top then elem */
    {"appendArray4",	  5,   -1,         1,	{OPERAND_LVT4}},
	/* Append array element; array at op1>=256, value is top then elem */
    {"appendArrayStk",	  1,   -2,         0,	{OPERAND_NONE}},
	/* Append array element; value is stktop, then elem, array names */
    {"appendStk",	  1,   -1,         0,	{OPERAND_NONE}},
	/* Append general variable; value is stktop, then unparsed name */
    {"lappendScalar1",	  2,   0,          1,	{OPERAND_LVT1}},
	/* Lappend scalar variable at op1<=255 in frame; value is stktop */
    {"lappendScalar4",	  5,   0,          1,	{OPERAND_LVT4}},
	/* Lappend scalar variable at op1 > 255 in frame; value is stktop */
    {"lappendArray1",	  2,   -1,         1,	{OPERAND_LVT1}},
	/* Lappend array element; array at op1<=255, value is top then elem */
    {"lappendArray4",	  5,   -1,         1,	{OPERAND_LVT4}},
	/* Lappend array element; array at op1>=256, value is top then elem */
    {"lappendArrayStk",	  1,   -2,         0,	{OPERAND_NONE}},
	/* Lappend array element; value is stktop, then elem, array names */
    {"lappendStk",	  1,   -1,         0,	{OPERAND_NONE}},
	/* Lappend general variable; value is stktop, then unparsed name */

    {"lindexMulti",	  5,   INT_MIN,    1,	{OPERAND_UINT4}},
	/* Lindex with generalized args, operand is number of stacked objs
	 * used: (operand-1) entries from stktop are the indices; then list to
	 * process. */
    {"over",		  5,   +1,         1,	{OPERAND_UINT4}},
	/* Duplicate the arg-th element from top of stack (TOS=0) */
    {"lsetList",          1,   -2,         0,	{OPERAND_NONE}},
	/* Four-arg version of 'lset'. stktop is old value; next is new
	 * element value, next is the index list; pushes new value */
    {"lsetFlat",          5,   INT_MIN,    1,	{OPERAND_UINT4}},
	/* Three- or >=5-arg version of 'lset', operand is number of stacked
	 * objs: stktop is old value, next is new element value, next come
	 * (operand-2) indices; pushes the new value.
	 */

    {"returnImm",	  9,   -1,         2,	{OPERAND_INT4, OPERAND_UINT4}},
	/* Compiled [return], code, level are operands; options and result
	 * are on the stack. */
    {"expon",		  1,   -1,	   0,	{OPERAND_NONE}},
	/* Binary exponentiation operator: push (stknext ** stktop) */

    /*
     * NOTE: the stack effects of expandStkTop and invokeExpanded are wrong -
     * but it cannot be done right at compile time, the stack effect is only
     * known at run time. The value for invokeExpanded is estimated better at
     * compile time.
     * See the comments further down in this file, where INST_INVOKE_EXPANDED
     * is emitted.
     */
    {"expandStart",       1,    0,          0,	{OPERAND_NONE}},
	/* Start of command with {*} (expanded) arguments */
    {"expandStkTop",      5,    0,          1,	{OPERAND_UINT4}},
	/* Expand the list at stacktop: push its elements on the stack */
    {"invokeExpanded",    1,    0,          0,	{OPERAND_NONE}},
	/* Invoke the command marked by the last 'expandStart' */

    {"listIndexImm",	  5,	0,	   1,	{OPERAND_IDX4}},
	/* List Index:	push (lindex stktop op4) */
    {"listRangeImm",	  9,	0,	   2,	{OPERAND_IDX4, OPERAND_IDX4}},
	/* List Range:	push (lrange stktop op4 op4) */
    {"startCommand",	  9,	0,	   2,	{OPERAND_OFFSET4, OPERAND_UINT4}},
	/* Start of bytecoded command: op is the length of the cmd's code, op2
	 * is number of commands here */

    {"listIn",		  1,	-1,	   0,	{OPERAND_NONE}},
	/* List containment: push [lsearch stktop stknext]>=0) */
    {"listNotIn",	  1,	-1,	   0,	{OPERAND_NONE}},
	/* List negated containment: push [lsearch stktop stknext]<0) */

    {"pushReturnOpts",	  1,	+1,	   0,	{OPERAND_NONE}},
	/* Push the interpreter's return option dictionary as an object on the
	 * stack. */
    {"returnStk",	  1,	-1,	   0,	{OPERAND_NONE}},
	/* Compiled [return]; options and result are on the stack, code and
	 * level are in the options. */

    {"dictGet",		  5,	INT_MIN,   1,	{OPERAND_UINT4}},
	/* The top op4 words (min 1) are a key path into the dictionary just
	 * below the keys on the stack, and all those values are replaced by
	 * the value read out of that key-path (like [dict get]).
	 * Stack:  ... dict key1 ... keyN => ... value */
    {"dictSet",		  9,	INT_MIN,   2,	{OPERAND_UINT4, OPERAND_LVT4}},
	/* Update a dictionary value such that the keys are a path pointing to
	 * the value. op4#1 = numKeys, op4#2 = LVTindex
	 * Stack:  ... key1 ... keyN value => ... newDict */
    {"dictUnset",	  9,	INT_MIN,   2,	{OPERAND_UINT4, OPERAND_LVT4}},
	/* Update a dictionary value such that the keys are not a path pointing
	 * to any value. op4#1 = numKeys, op4#2 = LVTindex
	 * Stack:  ... key1 ... keyN => ... newDict */
    {"dictIncrImm",	  9,	0,	   2,	{OPERAND_INT4, OPERAND_LVT4}},
	/* Update a dictionary value such that the value pointed to by key is
	 * incremented by some value (or set to it if the key isn't in the
	 * dictionary at all). op4#1 = incrAmount, op4#2 = LVTindex
	 * Stack:  ... key => ... newDict */
    {"dictAppend",	  5,	-1,	   1,	{OPERAND_LVT4}},
	/* Update a dictionary value such that the value pointed to by key has
	 * some value string-concatenated onto it. op4 = LVTindex
	 * Stack:  ... key valueToAppend => ... newDict */
    {"dictLappend",	  5,	-1,	   1,	{OPERAND_LVT4}},
	/* Update a dictionary value such that the value pointed to by key has
	 * some value list-appended onto it. op4 = LVTindex
	 * Stack:  ... key valueToAppend => ... newDict */
    {"dictFirst",	  5,	+2,	   1,	{OPERAND_LVT4}},
	/* Begin iterating over the dictionary, using the local scalar
	 * indicated by op4 to hold the iterator state. The local scalar
	 * should not refer to a named variable as the value is not wholly
	 * managed correctly.
	 * Stack:  ... dict => ... value key doneBool */
    {"dictNext",	  5,	+3,	   1,	{OPERAND_LVT4}},
	/* Get the next iteration from the iterator in op4's local scalar.
	 * Stack:  ... => ... value key doneBool */
    {"dictDone",	  5,	0,	   1,	{OPERAND_LVT4}},
	/* Terminate the iterator in op4's local scalar. Use unsetScalar
	 * instead (with 0 for flags). */
    {"dictUpdateStart",   9,    0,	   2,	{OPERAND_LVT4, OPERAND_AUX4}},
	/* Create the variables (described in the aux data referred to by the
	 * second immediate argument) to mirror the state of the dictionary in
	 * the variable referred to by the first immediate argument. The list
	 * of keys (top of the stack, not popped) must be the same length as
	 * the list of variables.
	 * Stack:  ... keyList => ... keyList */
    {"dictUpdateEnd",	  9,    -1,	   2,	{OPERAND_LVT4, OPERAND_AUX4}},
	/* Reflect the state of local variables (described in the aux data
	 * referred to by the second immediate argument) back to the state of
	 * the dictionary in the variable referred to by the first immediate
	 * argument. The list of keys (popped from the stack) must be the same
	 * length as the list of variables.
	 * Stack:  ... keyList => ... */
    {"jumpTable",	 5,	-1,	   1,	{OPERAND_AUX4}},
	/* Jump according to the jump-table (in AuxData as indicated by the
	 * operand) and the argument popped from the list. Always executes the
	 * next instruction if no match against the table's entries was found.
	 * Stack:  ... value => ...
	 * Note that the jump table contains offsets relative to the PC when
	 * it points to this instruction; the code is relocatable. */
    {"upvar",            5,    -1,        1,   {OPERAND_LVT4}},
	/* finds level and otherName in stack, links to local variable at
	 * index op1. Leaves the level on stack. */
    {"nsupvar",          5,    -1,        1,   {OPERAND_LVT4}},
	/* finds namespace and otherName in stack, links to local variable at
	 * index op1. Leaves the namespace on stack. */
    {"variable",         5,    -1,        1,   {OPERAND_LVT4}},
	/* finds namespace and otherName in stack, links to local variable at
	 * index op1. Leaves the namespace on stack. */
    {"syntax",		 9,   -1,         2,	{OPERAND_INT4, OPERAND_UINT4}},
	/* Compiled bytecodes to signal syntax error. Equivalent to returnImm
	 * except for the ERR_ALREADY_LOGGED flag in the interpreter. */
    {"reverse",		 5,    0,         1,	{OPERAND_UINT4}},
	/* Reverse the order of the arg elements at the top of stack */

    {"regexp",		 2,   -1,         1,	{OPERAND_INT1}},
	/* Regexp:	push (regexp stknext stktop) opnd == nocase */

    {"existScalar",	 5,    1,         1,	{OPERAND_LVT4}},
	/* Test if scalar variable at index op1 in call frame exists */
    {"existArray",	 5,    0,         1,	{OPERAND_LVT4}},
	/* Test if array element exists; array at slot op1, element is
	 * stktop */
    {"existArrayStk",	 1,    -1,        0,	{OPERAND_NONE}},
	/* Test if array element exists; element is stktop, array name is
	 * stknext */
    {"existStk",	 1,    0,         0,	{OPERAND_NONE}},
	/* Test if general variable exists; unparsed variable name is stktop*/

    {"nop",		 1,    0,         0,	{OPERAND_NONE}},
	/* Do nothing */
    {"returnCodeBranch", 1,   -1,	  0,	{OPERAND_NONE}},
	/* Jump to next instruction based on the return code on top of stack
	 * ERROR: +1;	RETURN: +3;	BREAK: +5;	CONTINUE: +7;
	 * Other non-OK: +9
	 */

    {"unsetScalar",	 6,    0,         2,	{OPERAND_UINT1, OPERAND_LVT4}},
	/* Make scalar variable at index op2 in call frame cease to exist;
	 * op1 is 1 for errors on problems, 0 otherwise */
    {"unsetArray",	 6,    -1,        2,	{OPERAND_UINT1, OPERAND_LVT4}},
	/* Make array element cease to exist; array at slot op2, element is
	 * stktop; op1 is 1 for errors on problems, 0 otherwise */
    {"unsetArrayStk",	 2,    -2,        1,	{OPERAND_UINT1}},
	/* Make array element cease to exist; element is stktop, array name is
	 * stknext; op1 is 1 for errors on problems, 0 otherwise */
    {"unsetStk",	 2,    -1,        1,	{OPERAND_UINT1}},
	/* Make general variable cease to exist; unparsed variable name is
	 * stktop; op1 is 1 for errors on problems, 0 otherwise */

    {"dictExpand",       1,    -1,        0,    {OPERAND_NONE}},
        /* Probe into a dict and extract it (or a subdict of it) into
         * variables with matched names. Produces list of keys bound as
         * result. Part of [dict with].
	 * Stack:  ... dict path => ... keyList */
    {"dictRecombineStk", 1,    -3,        0,    {OPERAND_NONE}},
        /* Map variable contents back into a dictionary in a variable. Part of
         * [dict with].
	 * Stack:  ... dictVarName path keyList => ... */
    {"dictRecombineImm", 5,    -2,        1,    {OPERAND_LVT4}},
        /* Map variable contents back into a dictionary in the local variable
         * indicated by the LVT index. Part of [dict with].
	 * Stack:  ... path keyList => ... */
    {"dictExists",	 5,	INT_MIN,  1,	{OPERAND_UINT4}},
	/* The top op4 words (min 1) are a key path into the dictionary just
	 * below the keys on the stack, and all those values are replaced by a
	 * boolean indicating whether it is possible to read out a value from
	 * that key-path (like [dict exists]).
	 * Stack:  ... dict key1 ... keyN => ... boolean */
    {"verifyDict",	 1,    -1,	  0,	{OPERAND_NONE}},
	/* Verifies that the word on the top of the stack is a dictionary,
	 * popping it if it is and throwing an error if it is not.
	 * Stack:  ... value => ... */

    {"strmap",		 1,    -2,	  0,	{OPERAND_NONE}},
	/* Simplified version of [string map] that only applies one change
	 * string, and only case-sensitively.
	 * Stack:  ... from to string => ... changedString */
    {"strfind",		 1,    -1,	  0,	{OPERAND_NONE}},
	/* Find the first index of a needle string in a haystack string,
	 * producing the index (integer) or -1 if nothing found.
	 * Stack:  ... needle haystack => ... index */
    {"strrfind",	 1,    -1,	  0,	{OPERAND_NONE}},
	/* Find the last index of a needle string in a haystack string,
	 * producing the index (integer) or -1 if nothing found.
	 * Stack:  ... needle haystack => ... index */
    {"strrangeImm",	 9,	0,	  2,	{OPERAND_IDX4, OPERAND_IDX4}},
	/* String Range: push (string range stktop op4 op4) */
    {"strrange",	 1,    -2,	  0,	{OPERAND_NONE}},
	/* String Range with non-constant arguments.
	 * Stack:  ... string idxA idxB => ... substring */

    {"yield",		 1,	0,	  0,	{OPERAND_NONE}},
	/* Makes the current coroutine yield the value at the top of the
	 * stack, and places the response back on top of the stack when it
	 * resumes.
	 * Stack:  ... valueToYield => ... resumeValue */
    {"coroName",         1,    +1,	  0,	{OPERAND_NONE}},
	/* Push the name of the interpreter's current coroutine as an object
	 * on the stack. */
    {"tailcall",	 2,    INT_MIN,	  1,	{OPERAND_UINT1}},
	/* Do a tailcall with the opnd items on the stack as the thing to
	 * tailcall to; opnd must be greater than 0 for the semantics to work
	 * right. */

    {"currentNamespace", 1,    +1,	  0,	{OPERAND_NONE}},
	/* Push the name of the interpreter's current namespace as an object
	 * on the stack. */
    {"infoLevelNumber",  1,    +1,	  0,	{OPERAND_NONE}},
	/* Push the stack depth (i.e., [info level]) of the interpreter as an
	 * object on the stack. */
    {"infoLevelArgs",	 1,	0,	  0,	{OPERAND_NONE}},
	/* Push the argument words to a stack depth (i.e., [info level <n>])
	 * of the interpreter as an object on the stack.
	 * Stack:  ... depth => ... argList */
    {"resolveCmd",	 1,	0,	  0,	{OPERAND_NONE}},
	/* Resolves the command named on the top of the stack to its fully
	 * qualified version, or produces the empty string if no such command
	 * exists. Never generates errors.
	 * Stack:  ... cmdName => ... fullCmdName */

    {"tclooSelf",	 1,	+1,	  0,	{OPERAND_NONE}},
	/* Push the identity of the current TclOO object (i.e., the name of
	 * its current public access command) on the stack. */
    {"tclooClass",	 1,	0,	  0,	{OPERAND_NONE}},
	/* Push the class of the TclOO object named at the top of the stack
	 * onto the stack.
	 * Stack:  ... object => ... class */
    {"tclooNamespace",	 1,	0,	  0,	{OPERAND_NONE}},
	/* Push the namespace of the TclOO object named at the top of the
	 * stack onto the stack.
	 * Stack:  ... object => ... namespace */
    {"tclooIsObject",	 1,	0,	  0,	{OPERAND_NONE}},
	/* Push whether the value named at the top of the stack is a TclOO
	 * object (i.e., a boolean). Can corrupt the interpreter result
	 * despite not throwing, so not safe for use in a post-exception
	 * context.
	 * Stack:  ... value => ... boolean */

    {"arrayExistsStk",	 1,	0,	  0,	{OPERAND_NONE}},
	/* Looks up the element on the top of the stack and tests whether it
	 * is an array. Pushes a boolean describing whether this is the
	 * case. Also runs the whole-array trace on the named variable, so can
	 * throw anything.
	 * Stack:  ... varName => ... boolean */
    {"arrayExistsImm",	 5,	+1,	  1,	{OPERAND_LVT4}},
	/* Looks up the variable indexed by opnd and tests whether it is an
	 * array. Pushes a boolean describing whether this is the case. Also
	 * runs the whole-array trace on the named variable, so can throw
	 * anything.
	 * Stack:  ... => ... boolean */
    {"arrayMakeStk",	 1,	-1,	  0,	{OPERAND_NONE}},
	/* Forces the element on the top of the stack to be the name of an
	 * array.
	 * Stack:  ... varName => ... */
    {"arrayMakeImm",	 5,	0,	  1,	{OPERAND_LVT4}},
	/* Forces the variable indexed by opnd to be an array. Does not touch
	 * the stack. */

    {"invokeReplace",	 6,	INT_MIN,  2,	{OPERAND_UINT4,OPERAND_UINT1}},
	/* Invoke command named objv[0], replacing the first two words with
	 * the word at the top of the stack;
	 * <objc,objv> = <op4,top op4 after popping 1> */

    {"listConcat",	 1,	-1,	  0,	{OPERAND_NONE}},
	/* Concatenates the two lists at the top of the stack into a single
	 * list and pushes that resulting list onto the stack.
	 * Stack: ... list1 list2 => ... [lconcat list1 list2] */

    {"expandDrop",       1,    0,          0,	{OPERAND_NONE}},
	/* Drops an element from the auxiliary stack, popping stack elements
	 * until the matching stack depth is reached. */

    /* New foreach implementation */
    {"foreach_start",	 5,	+2,	  1,	{OPERAND_AUX4}},
	/* Initialize execution of a foreach loop. Operand is aux data index
	 * of the ForeachInfo structure for the foreach command. It pushes 2
	 * elements which hold runtime params for foreach_step, they are later
	 * dropped by foreach_end together with the value lists. NOTE that the
	 * iterator-tracker and info reference must not be passed to bytecodes
	 * that handle normal Tcl values. NOTE that this instruction jumps to
	 * the foreach_step instruction paired with it; the stack info below
	 * is only nominal.
	 * Stack: ... listObjs... => ... listObjs... iterTracker info */
    {"foreach_step",	 1,	 0,	  0,	{OPERAND_NONE}},
	/* "Step" or begin next iteration of foreach loop. Assigns to foreach
	 * iteration variables. May jump to straight after the foreach_start
	 * that pushed the iterTracker and info values. MUST be followed
	 * immediately by a foreach_end.
	 * Stack: ... listObjs... iterTracker info =>
	 *				... listObjs... iterTracker info */
    {"foreach_end",	 1,	 0,	  0,	{OPERAND_NONE}},
	/* Clean up a foreach loop by dropping the info value, the tracker
	 * value and the lists that were being iterated over.
	 * Stack: ... listObjs... iterTracker info => ... */
    {"lmap_collect",	 1,	-1,	  0,	{OPERAND_NONE}},
	/* Appends the value at the top of the stack to the list located on
	 * the stack the "other side" of the foreach-related values.
	 * Stack: ... collector listObjs... iterTracker info value =>
	 *			... collector listObjs... iterTracker info */

    {"strtrim",		 1,	-1,	  0,	{OPERAND_NONE}},
	/* [string trim] core: removes the characters (designated by the value
	 * at the top of the stack) from both ends of the string and pushes
	 * the resulting string.
	 * Stack: ... string charset => ... trimmedString */
    {"strtrimLeft",	 1,	-1,	  0,	{OPERAND_NONE}},
	/* [string trimleft] core: removes the characters (designated by the
	 * value at the top of the stack) from the left of the string and
	 * pushes the resulting string.
	 * Stack: ... string charset => ... trimmedString */
    {"strtrimRight",	 1,	-1,	  0,	{OPERAND_NONE}},
	/* [string trimright] core: removes the characters (designated by the
	 * value at the top of the stack) from the right of the string and
	 * pushes the resulting string.
	 * Stack: ... string charset => ... trimmedString */

    {"concatStk",	 5,	INT_MIN,  1,	{OPERAND_UINT4}},
	/* Wrapper round Tcl_ConcatObj(), used for [concat] and [eval]. opnd
	 * is number of values to concatenate.
	 * Operation:	push concat(stk1 stk2 ... stktop) */

    {"strcaseUpper",	 1,	0,	  0,	{OPERAND_NONE}},
	/* [string toupper] core: converts whole string to upper case using
	 * the default (extended "C" locale) rules.
	 * Stack: ... string => ... newString */
    {"strcaseLower",	 1,	0,	  0,	{OPERAND_NONE}},
	/* [string tolower] core: converts whole string to upper case using
	 * the default (extended "C" locale) rules.
	 * Stack: ... string => ... newString */
    {"strcaseTitle",	 1,	0,	  0,	{OPERAND_NONE}},
	/* [string totitle] core: converts whole string to upper case using
	 * the default (extended "C" locale) rules.
	 * Stack: ... string => ... newString */
    {"strreplace",	 1,	-3,	  0,	{OPERAND_NONE}},
	/* [string replace] core: replaces a non-empty range of one string
	 * with the contents of another.
	 * Stack: ... string fromIdx toIdx replacement => ... newString */

    {"originCmd",	 1,	0,	  0,	{OPERAND_NONE}},
	/* Reports which command was the origin (via namespace import chain)
	 * of the command named on the top of the stack.
	 * Stack:  ... cmdName => ... fullOriginalCmdName */

    {"tclooNext",	 2,	INT_MIN,  1,	{OPERAND_UINT1}},
	/* Call the next item on the TclOO call chain, passing opnd arguments
	 * (min 1, max 255, *includes* "next").  The result of the invoked
	 * method implementation will be pushed on the stack in place of the
	 * arguments (similar to invokeStk).
	 * Stack:  ... "next" arg2 arg3 -- argN => ... result */
    {"tclooNextClass",	 2,	INT_MIN,  1,	{OPERAND_UINT1}},
	/* Call the following item on the TclOO call chain defined by class
	 * className, passing opnd arguments (min 2, max 255, *includes*
	 * "nextto" and the class name). The result of the invoked method
	 * implementation will be pushed on the stack in place of the
	 * arguments (similar to invokeStk).
	 * Stack:  ... "nextto" className arg3 arg4 -- argN => ... result */

    {"yieldToInvoke",	 1,	0,	  0,	{OPERAND_NONE}},
	/* Makes the current coroutine yield the value at the top of the
	 * stack, invoking the given command/args with resolution in the given
	 * namespace (all packed into a list), and places the list of values
	 * that are the response back on top of the stack when it resumes.
	 * Stack:  ... [list ns cmd arg1 ... argN] => ... resumeList */

    {"numericType",	 1,	0,	  0,	{OPERAND_NONE}},
	/* Pushes the numeric type code of the word at the top of the stack.
	 * Stack:  ... value => ... typeCode */
    {"tryCvtToBoolean",	 1,	+1,	  0,	{OPERAND_NONE}},
	/* Try converting stktop to boolean if possible. No errors.
	 * Stack:  ... value => ... value isStrictBool */
    {"strclass",	 2,	0,	  1,	{OPERAND_SCLS1}},
	/* See if all the characters of the given string are a member of the
	 * specified (by opnd) character class. Note that an empty string will
	 * satisfy the class check (standard definition of "all").
	 * Stack:  ... stringValue => ... boolean */

    {"lappendList",	 5,	0,	1,	{OPERAND_LVT4}},
	/* Lappend list to scalar variable at op4 in frame.
	 * Stack:  ... list => ... listVarContents */
    {"lappendListArray", 5,	-1,	1,	{OPERAND_LVT4}},
	/* Lappend list to array element; array at op4.
	 * Stack:  ... elem list => ... listVarContents */
    {"lappendListArrayStk", 1,	-2,	0,	{OPERAND_NONE}},
	/* Lappend list to array element.
	 * Stack:  ... arrayName elem list => ... listVarContents */
    {"lappendListStk",	 1,	-1,	0,	{OPERAND_NONE}},
	/* Lappend list to general variable.
	 * Stack:  ... varName list => ... listVarContents */

    {"clockRead",	 2,	+1,	1,	{OPERAND_UINT1}},
        /* Read clock out to the stack. Operand is which clock to read
	 * 0=clicks, 1=microseconds, 2=milliseconds, 3=seconds.
	 * Stack: ... => ... time */

    {NULL, 0, 0, 0, {OPERAND_NONE}}
};

/*
 * Prototypes for procedures defined later in this file:
 */

static ByteCode *	CompileSubstObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
			    int flags);
static void		DupByteCodeInternalRep(Tcl_Obj *srcPtr,
			    Tcl_Obj *copyPtr);
static unsigned char *	EncodeCmdLocMap(CompileEnv *envPtr,
			    ByteCode *codePtr, unsigned char *startPtr);
static void		EnterCmdExtentData(CompileEnv *envPtr,
			    int cmdNumber, int numSrcBytes, int numCodeBytes);
static void		EnterCmdStartData(CompileEnv *envPtr,
			    int cmdNumber, int srcOffset, int codeOffset);
static void		FreeByteCodeInternalRep(Tcl_Obj *objPtr);
static void		FreeSubstCodeInternalRep(Tcl_Obj *objPtr);
static int		GetCmdLocEncodingSize(CompileEnv *envPtr);
static int		IsCompactibleCompileEnv(Tcl_Interp *interp,
			    CompileEnv *envPtr);
#ifdef TCL_COMPILE_STATS
static void		RecordByteCodeStats(ByteCode *codePtr);
#endif /* TCL_COMPILE_STATS */
static int		SetByteCodeFromAny(Tcl_Interp *interp,
			    Tcl_Obj *objPtr);
static void		StartExpanding(CompileEnv *envPtr);

/*
 * TIP #280: Helper for building the per-word line information of all compiled
 * commands.
 */
static void		EnterCmdWordData(ExtCmdLoc *eclPtr, int srcOffset,
			    Tcl_Token *tokenPtr, const char *cmd, int len,
			    int numWords, int line, int *clNext, int **lines,
			    CompileEnv *envPtr);
static void		ReleaseCmdWordData(ExtCmdLoc *eclPtr);

/*
 * The structure below defines the bytecode Tcl object type by means of
 * procedures that can be invoked by generic object code.
 */

const Tcl_ObjType tclByteCodeType = {
    "bytecode",			/* name */
    FreeByteCodeInternalRep,	/* freeIntRepProc */
    DupByteCodeInternalRep,	/* dupIntRepProc */
    NULL,			/* updateStringProc */
    SetByteCodeFromAny		/* setFromAnyProc */
};

/*
 * The structure below defines a bytecode Tcl object type to hold the
 * compiled bytecode for the [subst]itution of Tcl values.
 */

static const Tcl_ObjType substCodeType = {
    "substcode",		/* name */
    FreeSubstCodeInternalRep,	/* freeIntRepProc */
    DupByteCodeInternalRep,	/* dupIntRepProc - shared with bytecode */
    NULL,			/* updateStringProc */
    NULL,			/* setFromAnyProc */
};

/*
 * Helper macros.
 */

#define TclIncrUInt4AtPtr(ptr, delta) \
    TclStoreInt4AtPtr(TclGetUInt4AtPtr(ptr)+(delta), (ptr));

/*
 *----------------------------------------------------------------------
 *
 * TclSetByteCodeFromAny --
 *
 *	Part of the bytecode Tcl object type implementation. Attempts to
 *	generate an byte code internal form for the Tcl object "objPtr" by
 *	compiling its string representation. This function also takes a hook
 *	procedure that will be invoked to perform any needed post processing
 *	on the compilation results before generating byte codes. interp is
 *	compilation context and may not be NULL.
 *
 * Results:
 *	The return value is a standard Tcl object result. If an error occurs
 *	during compilation, an error message is left in the interpreter's
 *	result.
 *
 * Side effects:
 *	Frees the old internal representation. If no error occurs, then the
 *	compiled code is stored as "objPtr"s bytecode representation. Also, if
 *	debugging, initializes the "tcl_traceCompile" Tcl variable used to
 *	trace compilations.
 *
 *----------------------------------------------------------------------
 */

int
TclSetByteCodeFromAny(
    Tcl_Interp *interp,		/* The interpreter for which the code is being
				 * compiled. Must not be NULL. */
    Tcl_Obj *objPtr,		/* The object to make a ByteCode object. */
    CompileHookProc *hookProc,	/* Procedure to invoke after compilation. */
    ClientData clientData)	/* Hook procedure private data. */
{
    Interp *iPtr = (Interp *) interp;
    CompileEnv compEnv;		/* Compilation environment structure allocated
				 * in frame. */
    int length, result = TCL_OK;
    const char *stringPtr;
    Proc *procPtr = iPtr->compiledProcPtr;
    ContLineLoc *clLocPtr;

#ifdef TCL_COMPILE_DEBUG
    if (!traceInitialized) {
	if (Tcl_LinkVar(interp, "tcl_traceCompile",
		(char *) &tclTraceCompile, TCL_LINK_INT) != TCL_OK) {
	    Tcl_Panic("SetByteCodeFromAny: unable to create link for tcl_traceCompile variable");
	}
	traceInitialized = 1;
    }
#endif

    stringPtr = TclGetStringFromObj(objPtr, &length);

    /*
     * TIP #280: Pick up the CmdFrame in which the BC compiler was invoked and
     * use to initialize the tracking in the compiler. This information was
     * stored by TclCompEvalObj and ProcCompileProc.
     */

    TclInitCompileEnv(interp, &compEnv, stringPtr, length,
	    iPtr->invokeCmdFramePtr, iPtr->invokeWord);

    /*
     * Now we check if we have data about invisible continuation lines for the
     * script, and make it available to the compile environment, if so.
     *
     * It is not clear if the script Tcl_Obj* can be free'd while the compiler
     * is using it, leading to the release of the associated ContLineLoc
     * structure as well. To ensure that the latter doesn't happen we set a
     * lock on it. We release this lock in the function TclFreeCompileEnv(),
     * found in this file. The "lineCLPtr" hashtable is managed in the file
     * "tclObj.c".
     */

    clLocPtr = TclContinuationsGet(objPtr);
    if (clLocPtr) {
	compEnv.clNext = &clLocPtr->loc[0];
    }

    TclCompileScript(interp, stringPtr, length, &compEnv);

    /*
     * Successful compilation. Add a "done" instruction at the end.
     */

    TclEmitOpcode(INST_DONE, &compEnv);

    /*
     * Check for optimizations!
     *
     * Test if the generated code is free of most hazards; if so, recompile
     * but with generation of INST_START_CMD disabled. This produces somewhat
     * faster code in some cases, and more compact code in more.
     */

    if (Tcl_GetMaster(interp) == NULL &&
	    !Tcl_LimitTypeEnabled(interp, TCL_LIMIT_COMMANDS|TCL_LIMIT_TIME)
	    && IsCompactibleCompileEnv(interp, &compEnv)) {
	TclFreeCompileEnv(&compEnv);
	iPtr->compiledProcPtr = procPtr;
	TclInitCompileEnv(interp, &compEnv, stringPtr, length,
		iPtr->invokeCmdFramePtr, iPtr->invokeWord);
	if (clLocPtr) {
	    compEnv.clNext = &clLocPtr->loc[0];
	}
	compEnv.atCmdStart = 2;		/* The disabling magic. */
	TclCompileScript(interp, stringPtr, length, &compEnv);
	assert (compEnv.atCmdStart > 1);
	TclEmitOpcode(INST_DONE, &compEnv);
	assert (compEnv.atCmdStart > 1);
    }

    /*
     * Apply some peephole optimizations that can cross specific/generic
     * instruction generator boundaries.
     */

    if (iPtr->extra.optimizer) {
	(iPtr->extra.optimizer)(&compEnv);
    }

    /*
     * Invoke the compilation hook procedure if one exists.
     */

    if (hookProc) {
	result = hookProc(interp, &compEnv, clientData);
    }

    /*
     * Change the object into a ByteCode object. Ownership of the literal
     * objects and aux data items is given to the ByteCode object.
     */

#ifdef TCL_COMPILE_DEBUG
    TclVerifyLocalLiteralTable(&compEnv);
#endif /*TCL_COMPILE_DEBUG*/

    if (result == TCL_OK) {
	TclInitByteCodeObj(objPtr, &compEnv);
#ifdef TCL_COMPILE_DEBUG
	if (tclTraceCompile >= 2) {
	    TclPrintByteCodeObj(interp, objPtr);
	    fflush(stdout);
	}
#endif /* TCL_COMPILE_DEBUG */
    }

    TclFreeCompileEnv(&compEnv);
    return result;
}

/*
 *-----------------------------------------------------------------------
 *
 * SetByteCodeFromAny --
 *
 *	Part of the bytecode Tcl object type implementation. Attempts to
 *	generate an byte code internal form for the Tcl object "objPtr" by
 *	compiling its string representation.
 *
 * Results:
 *	The return value is a standard Tcl object result. If an error occurs
 *	during compilation, an error message is left in the interpreter's
 *	result unless "interp" is NULL.
 *
 * Side effects:
 *	Frees the old internal representation. If no error occurs, then the
 *	compiled code is stored as "objPtr"s bytecode representation. Also, if
 *	debugging, initializes the "tcl_traceCompile" Tcl variable used to
 *	trace compilations.
 *
 *----------------------------------------------------------------------
 */

static int
SetByteCodeFromAny(
    Tcl_Interp *interp,		/* The interpreter for which the code is being
				 * compiled. Must not be NULL. */
    Tcl_Obj *objPtr)		/* The object to make a ByteCode object. */
{
    if (interp == NULL) {
	return TCL_ERROR;
    }
    return TclSetByteCodeFromAny(interp, objPtr, NULL, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * DupByteCodeInternalRep --
 *
 *	Part of the bytecode Tcl object type implementation. However, it does
 *	not copy the internal representation of a bytecode Tcl_Obj, but
 *	instead leaves the new object untyped (with a NULL type pointer).
 *	Code will be compiled for the new object only if necessary.
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
DupByteCodeInternalRep(
    Tcl_Obj *srcPtr,		/* Object with internal rep to copy. */
    Tcl_Obj *copyPtr)		/* Object with internal rep to set. */
{
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeByteCodeInternalRep --
 *
 *	Part of the bytecode Tcl object type implementation. Frees the storage
 *	associated with a bytecode object's internal representation unless its
 *	code is actively being executed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The bytecode object's internal rep is marked invalid and its code gets
 *	freed unless the code is actively being executed. In that case the
 *	cleanup is delayed until the last execution of the code completes.
 *
 *----------------------------------------------------------------------
 */

static void
FreeByteCodeInternalRep(
    register Tcl_Obj *objPtr)	/* Object whose internal rep to free. */
{
    register ByteCode *codePtr = objPtr->internalRep.twoPtrValue.ptr1;

    objPtr->typePtr = NULL;
    if (codePtr->refCount-- <= 1) {
	TclCleanupByteCode(codePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclCleanupByteCode --
 *
 *	This procedure does all the real work of freeing up a bytecode
 *	object's ByteCode structure. It's called only when the structure's
 *	reference count becomes zero.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees objPtr's bytecode internal representation and sets its type NULL
 *	Also releases its literals and frees its auxiliary data items.
 *
 *----------------------------------------------------------------------
 */

void
TclCleanupByteCode(
    register ByteCode *codePtr)	/* Points to the ByteCode to free. */
{
    Tcl_Interp *interp = (Tcl_Interp *) *codePtr->interpHandle;
    Interp *iPtr = (Interp *) interp;
    int numLitObjects = codePtr->numLitObjects;
    int numAuxDataItems = codePtr->numAuxDataItems;
    register Tcl_Obj **objArrayPtr, *objPtr;
    register const AuxData *auxDataPtr;
    int i;
#ifdef TCL_COMPILE_STATS

    if (interp != NULL) {
	ByteCodeStats *statsPtr;
	Tcl_Time destroyTime;
	int lifetimeSec, lifetimeMicroSec, log2;

	statsPtr = &iPtr->stats;

	statsPtr->numByteCodesFreed++;
	statsPtr->currentSrcBytes -= (double) codePtr->numSrcBytes;
	statsPtr->currentByteCodeBytes -= (double) codePtr->structureSize;

	statsPtr->currentInstBytes -= (double) codePtr->numCodeBytes;
	statsPtr->currentLitBytes -= (double)
		codePtr->numLitObjects * sizeof(Tcl_Obj *);
	statsPtr->currentExceptBytes -= (double)
		codePtr->numExceptRanges * sizeof(ExceptionRange);
	statsPtr->currentAuxBytes -= (double)
		codePtr->numAuxDataItems * sizeof(AuxData);
	statsPtr->currentCmdMapBytes -= (double) codePtr->numCmdLocBytes;

	Tcl_GetTime(&destroyTime);
	lifetimeSec = destroyTime.sec - codePtr->createTime.sec;
	if (lifetimeSec > 2000) {	/* avoid overflow */
	    lifetimeSec = 2000;
	}
	lifetimeMicroSec = 1000000 * lifetimeSec +
		(destroyTime.usec - codePtr->createTime.usec);

	log2 = TclLog2(lifetimeMicroSec);
	if (log2 > 31) {
	    log2 = 31;
	}
	statsPtr->lifetimeCount[log2]++;
    }
#endif /* TCL_COMPILE_STATS */

    /*
     * A single heap object holds the ByteCode structure and its code, object,
     * command location, and auxiliary data arrays. This means we only need to
     * 1) decrement the ref counts of the LiteralEntry's in its literal array,
     * 2) call the free procs for the auxiliary data items, 3) free the
     * localCache if it is unused, and finally 4) free the ByteCode
     * structure's heap object.
     *
     * The case for TCL_BYTECODE_PRECOMPILED (precompiled ByteCodes, like
     * those generated from tbcload) is special, as they doesn't make use of
     * the global literal table. They instead maintain private references to
     * their literals which must be decremented.
     *
     * In order to insure a proper and efficient cleanup of the literal array
     * when it contains non-shared literals [Bug 983660], we also distinguish
     * the case of an interpreter being deleted (signaled by interp == NULL).
     * Also, as the interp deletion will remove the global literal table
     * anyway, we avoid the extra cost of updating it for each literal being
     * released.
     */

    if (codePtr->flags & TCL_BYTECODE_PRECOMPILED) {

	objArrayPtr = codePtr->objArrayPtr;
	for (i = 0;  i < numLitObjects;  i++) {
	    objPtr = *objArrayPtr;
	    if (objPtr) {
		Tcl_DecrRefCount(objPtr);
	    }
	    objArrayPtr++;
	}
	codePtr->numLitObjects = 0;
    } else {
	objArrayPtr = codePtr->objArrayPtr;
	while (numLitObjects--) {
	    /* TclReleaseLiteral calls Tcl_DecrRefCount() for us */
	    TclReleaseLiteral(interp, *objArrayPtr++);
	}
    }

    auxDataPtr = codePtr->auxDataArrayPtr;
    for (i = 0;  i < numAuxDataItems;  i++) {
	if (auxDataPtr->type->freeProc != NULL) {
	    auxDataPtr->type->freeProc(auxDataPtr->clientData);
	}
	auxDataPtr++;
    }

    /*
     * TIP #280. Release the location data associated with this byte code
     * structure, if any. NOTE: The interp we belong to may be gone already,
     * and the data with it.
     *
     * See also tclBasic.c, DeleteInterpProc
     */

    if (iPtr) {
	Tcl_HashEntry *hePtr = Tcl_FindHashEntry(iPtr->lineBCPtr,
		(char *) codePtr);

	if (hePtr) {
	    ReleaseCmdWordData(Tcl_GetHashValue(hePtr));
	    Tcl_DeleteHashEntry(hePtr);
	}
    }

    if (codePtr->localCachePtr && (--codePtr->localCachePtr->refCount == 0)) {
	TclFreeLocalCache(interp, codePtr->localCachePtr);
    }

    TclHandleRelease(codePtr->interpHandle);
    ckfree(codePtr);
}

/*
 * ---------------------------------------------------------------------
 *
 * IsCompactibleCompileEnv --
 *
 *	Checks to see if we may apply some basic compaction optimizations to a
 *	piece of bytecode. Idempotent.
 *
 * ---------------------------------------------------------------------
 */

static int
IsCompactibleCompileEnv(
    Tcl_Interp *interp,
    CompileEnv *envPtr)
{
    unsigned char *pc;
    int size;

    /*
     * Special: procedures in the '::tcl' namespace (or its children) are
     * considered to be well-behaved and so can have compaction applied even
     * if it would otherwise be invalid.
     */

    if (envPtr->procPtr != NULL && envPtr->procPtr->cmdPtr != NULL
	    && envPtr->procPtr->cmdPtr->nsPtr != NULL) {
	Namespace *nsPtr = envPtr->procPtr->cmdPtr->nsPtr;

	if (strcmp(nsPtr->fullName, "::tcl") == 0
		|| strncmp(nsPtr->fullName, "::tcl::", 7) == 0) {
	    return 1;
	}
    }

    /*
     * Go through and ensure that no operation involved can cause a desired
     * change of bytecode sequence during running. This comes down to ensuring
     * that there are no mapped variables (due to traces) or calls to external
     * commands (traces, [uplevel] trickery). This is actually a very
     * conservative check; it turns down a lot of code that is OK in practice.
     */

    for (pc = envPtr->codeStart ; pc < envPtr->codeNext ; pc += size) {
	switch (*pc) {
	    /* Invokes */
	case INST_INVOKE_STK1:
	case INST_INVOKE_STK4:
	case INST_INVOKE_EXPANDED:
	case INST_INVOKE_REPLACE:
	    return 0;
	    /* Runtime evals */
	case INST_EVAL_STK:
	case INST_EXPR_STK:
	case INST_YIELD:
	    return 0;
	    /* Upvars */
	case INST_UPVAR:
	case INST_NSUPVAR:
	case INST_VARIABLE:
	    return 0;
	default:
	    size = tclInstructionTable[*pc].numBytes;
	    assert (size > 0);
	    break;
	}
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SubstObj --
 *
 *	This function performs the substitutions specified on the given string
 *	as described in the user documentation for the "subst" Tcl command.
 *
 * Results:
 *	A Tcl_Obj* containing the substituted string, or NULL to indicate that
 *	an error occurred.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_SubstObj(
    Tcl_Interp *interp,		/* Interpreter in which substitution occurs */
    Tcl_Obj *objPtr,		/* The value to be substituted. */
    int flags)			/* What substitutions to do. */
{
    NRE_callback *rootPtr = TOP_CB(interp);

    if (TclNRRunCallbacks(interp, Tcl_NRSubstObj(interp, objPtr, flags),
	    rootPtr) != TCL_OK) {
	return NULL;
    }
    return Tcl_GetObjResult(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_NRSubstObj --
 *
 *	Request substitution of a Tcl value by the NR stack.
 *
 * Results:
 *	Returns TCL_OK.
 *
 * Side effects:
 *	Compiles objPtr into bytecode that performs the substitutions as
 *	governed by flags and places callbacks on the NR stack to execute
 *	the bytecode and store the result in the interp.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_NRSubstObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    int flags)
{
    ByteCode *codePtr = CompileSubstObj(interp, objPtr, flags);

    /* TODO: Confirm we do not need this. */
    /* Tcl_ResetResult(interp); */
    return TclNRExecuteByteCode(interp, codePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * CompileSubstObj --
 *
 *	Compile a Tcl value into ByteCode implementing its substitution, as
 *	governed by flags.
 *
 * Results:
 *	A (ByteCode *) is returned pointing to the resulting ByteCode.
 *	The caller must manage its refCount and arrange for a call to
 *	TclCleanupByteCode() when the last reference disappears.
 *
 * Side effects:
 *	The Tcl_ObjType of objPtr is changed to the "substcode" type, and the
 *	ByteCode and governing flags value are kept in the internal rep for
 *	faster operations the next time CompileSubstObj is called on the same
 *	value.
 *
 *----------------------------------------------------------------------
 */

static ByteCode *
CompileSubstObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    int flags)
{
    Interp *iPtr = (Interp *) interp;
    ByteCode *codePtr = NULL;

    if (objPtr->typePtr == &substCodeType) {
	Namespace *nsPtr = iPtr->varFramePtr->nsPtr;

	codePtr = objPtr->internalRep.twoPtrValue.ptr1;
	if (flags != PTR2INT(objPtr->internalRep.twoPtrValue.ptr2)
		|| ((Interp *) *codePtr->interpHandle != iPtr)
		|| (codePtr->compileEpoch != iPtr->compileEpoch)
		|| (codePtr->nsPtr != nsPtr)
		|| (codePtr->nsEpoch != nsPtr->resolverEpoch)
		|| (codePtr->localCachePtr !=
		iPtr->varFramePtr->localCachePtr)) {
	    FreeSubstCodeInternalRep(objPtr);
	}
    }
    if (objPtr->typePtr != &substCodeType) {
	CompileEnv compEnv;
	int numBytes;
	const char *bytes = Tcl_GetStringFromObj(objPtr, &numBytes);

	/* TODO: Check for more TIP 280 */
	TclInitCompileEnv(interp, &compEnv, bytes, numBytes, NULL, 0);

	TclSubstCompile(interp, bytes, numBytes, flags, 1, &compEnv);

	TclEmitOpcode(INST_DONE, &compEnv);
	TclInitByteCodeObj(objPtr, &compEnv);
	objPtr->typePtr = &substCodeType;
	TclFreeCompileEnv(&compEnv);

	codePtr = objPtr->internalRep.twoPtrValue.ptr1;
	objPtr->internalRep.twoPtrValue.ptr1 = codePtr;
	objPtr->internalRep.twoPtrValue.ptr2 = INT2PTR(flags);
	if (iPtr->varFramePtr->localCachePtr) {
	    codePtr->localCachePtr = iPtr->varFramePtr->localCachePtr;
	    codePtr->localCachePtr->refCount++;
	}
#ifdef TCL_COMPILE_DEBUG
	if (tclTraceCompile >= 2) {
	    TclPrintByteCodeObj(interp, objPtr);
	    fflush(stdout);
	}
#endif /* TCL_COMPILE_DEBUG */
    }
    return codePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeSubstCodeInternalRep --
 *
 *	Part of the substcode Tcl object type implementation. Frees the
 *	storage associated with a substcode object's internal representation
 *	unless its code is actively being executed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The substcode object's internal rep is marked invalid and its code
 *	gets freed unless the code is actively being executed. In that case
 *	the cleanup is delayed until the last execution of the code completes.
 *
 *----------------------------------------------------------------------
 */

static void
FreeSubstCodeInternalRep(
    register Tcl_Obj *objPtr)	/* Object whose internal rep to free. */
{
    register ByteCode *codePtr = objPtr->internalRep.twoPtrValue.ptr1;

    objPtr->typePtr = NULL;
    if (codePtr->refCount-- <= 1) {
	TclCleanupByteCode(codePtr);
    }
}

static void
ReleaseCmdWordData(
    ExtCmdLoc *eclPtr)
{
    int i;

    if (eclPtr->type == TCL_LOCATION_SOURCE) {
	Tcl_DecrRefCount(eclPtr->path);
    }
    for (i=0 ; i<eclPtr->nuloc ; i++) {
	ckfree((char *) eclPtr->loc[i].line);
    }

    if (eclPtr->loc != NULL) {
	ckfree((char *) eclPtr->loc);
    }

    ckfree((char *) eclPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitCompileEnv --
 *
 *	Initializes a CompileEnv compilation environment structure for the
 *	compilation of a string in an interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The CompileEnv structure is initialized.
 *
 *----------------------------------------------------------------------
 */

void
TclInitCompileEnv(
    Tcl_Interp *interp,		/* The interpreter for which a CompileEnv
				 * structure is initialized. */
    register CompileEnv *envPtr,/* Points to the CompileEnv structure to
				 * initialize. */
    const char *stringPtr,	/* The source string to be compiled. */
    int numBytes,		/* Number of bytes in source string. */
    const CmdFrame *invoker,	/* Location context invoking the bcc */
    int word)			/* Index of the word in that context getting
				 * compiled */
{
    Interp *iPtr = (Interp *) interp;

    assert(tclInstructionTable[LAST_INST_OPCODE+1].name == NULL);

    envPtr->iPtr = iPtr;
    envPtr->source = stringPtr;
    envPtr->numSrcBytes = numBytes;
    envPtr->procPtr = iPtr->compiledProcPtr;
    iPtr->compiledProcPtr = NULL;
    envPtr->numCommands = 0;
    envPtr->exceptDepth = 0;
    envPtr->maxExceptDepth = 0;
    envPtr->maxStackDepth = 0;
    envPtr->currStackDepth = 0;
    TclInitLiteralTable(&envPtr->localLitTable);

    envPtr->codeStart = envPtr->staticCodeSpace;
    envPtr->codeNext = envPtr->codeStart;
    envPtr->codeEnd = envPtr->codeStart + COMPILEENV_INIT_CODE_BYTES;
    envPtr->mallocedCodeArray = 0;

    envPtr->literalArrayPtr = envPtr->staticLiteralSpace;
    envPtr->literalArrayNext = 0;
    envPtr->literalArrayEnd = COMPILEENV_INIT_NUM_OBJECTS;
    envPtr->mallocedLiteralArray = 0;

    envPtr->exceptArrayPtr = envPtr->staticExceptArraySpace;
    envPtr->exceptAuxArrayPtr = envPtr->staticExAuxArraySpace;
    envPtr->exceptArrayNext = 0;
    envPtr->exceptArrayEnd = COMPILEENV_INIT_EXCEPT_RANGES;
    envPtr->mallocedExceptArray = 0;

    envPtr->cmdMapPtr = envPtr->staticCmdMapSpace;
    envPtr->cmdMapEnd = COMPILEENV_INIT_CMD_MAP_SIZE;
    envPtr->mallocedCmdMap = 0;
    envPtr->atCmdStart = 1;
    envPtr->expandCount = 0;

    /*
     * TIP #280: Set up the extended command location information, based on
     * the context invoking the byte code compiler. This structure is used to
     * keep the per-word line information for all compiled commands.
     *
     * See also tclBasic.c, TclEvalObjEx, for the equivalent code in the
     * non-compiling evaluator
     */

    envPtr->extCmdMapPtr = ckalloc(sizeof(ExtCmdLoc));
    envPtr->extCmdMapPtr->loc = NULL;
    envPtr->extCmdMapPtr->nloc = 0;
    envPtr->extCmdMapPtr->nuloc = 0;
    envPtr->extCmdMapPtr->path = NULL;

    if (invoker == NULL) {
	/*
	 * Initialize the compiler for relative counting in case of a
	 * dynamic context.
	 */

	envPtr->line = 1;
	if (iPtr->evalFlags & TCL_EVAL_FILE) {
	    iPtr->evalFlags &= ~TCL_EVAL_FILE;
	    envPtr->extCmdMapPtr->type = TCL_LOCATION_SOURCE;

	    if (iPtr->scriptFile) {
		/*
		 * Normalization here, to have the correct pwd. Should have
		 * negligible impact on performance, as the norm should have
		 * been done already by the 'source' invoking us, and it
		 * caches the result.
		 */

		Tcl_Obj *norm =
			Tcl_FSGetNormalizedPath(interp, iPtr->scriptFile);

		if (norm == NULL) {
		    /*
		     * Error message in the interp result. No place to put it.
		     * And no place to serve the error itself to either. Fake
		     * a path, empty string.
		     */

		    TclNewLiteralStringObj(envPtr->extCmdMapPtr->path, "");
		} else {
		    envPtr->extCmdMapPtr->path = norm;
		}
	    } else {
		TclNewLiteralStringObj(envPtr->extCmdMapPtr->path, "");
	    }

	    Tcl_IncrRefCount(envPtr->extCmdMapPtr->path);
	} else {
	    envPtr->extCmdMapPtr->type =
		(envPtr->procPtr ? TCL_LOCATION_PROC : TCL_LOCATION_BC);
	}
    } else {
	/*
	 * Initialize the compiler using the context, making counting absolute
	 * to that context. Note that the context can be byte code execution.
	 * In that case we have to fill out the missing pieces (line, path,
	 * ...) which may make change the type as well.
	 */

	CmdFrame *ctxPtr = TclStackAlloc(interp, sizeof(CmdFrame));
	int pc = 0;

	*ctxPtr = *invoker;
	if (invoker->type == TCL_LOCATION_BC) {
	    /*
	     * Note: Type BC => ctx.data.eval.path    is not used.
	     *			ctx.data.tebc.codePtr is used instead.
	     */

	    TclGetSrcInfoForPc(ctxPtr);
	    pc = 1;
	}

	if ((ctxPtr->nline <= word) || (ctxPtr->line[word] < 0)) {
	    /*
	     * Word is not a literal, relative counting.
	     */

	    envPtr->line = 1;
	    envPtr->extCmdMapPtr->type =
		    (envPtr->procPtr ? TCL_LOCATION_PROC : TCL_LOCATION_BC);

	    if (pc && (ctxPtr->type == TCL_LOCATION_SOURCE)) {
		/*
		 * The reference made by 'TclGetSrcInfoForPc' is dead.
		 */

		Tcl_DecrRefCount(ctxPtr->data.eval.path);
	    }
	} else {
	    envPtr->line = ctxPtr->line[word];
	    envPtr->extCmdMapPtr->type = ctxPtr->type;

	    if (ctxPtr->type == TCL_LOCATION_SOURCE) {
		envPtr->extCmdMapPtr->path = ctxPtr->data.eval.path;

		if (pc) {
		    /*
		     * The reference 'TclGetSrcInfoForPc' made is transfered.
		     */

		    ctxPtr->data.eval.path = NULL;
		} else {
		    /*
		     * We have a new reference here.
		     */

		    Tcl_IncrRefCount(envPtr->extCmdMapPtr->path);
		}
	    }
	}

	TclStackFree(interp, ctxPtr);
    }

    envPtr->extCmdMapPtr->start = envPtr->line;

    /*
     * Initialize the data about invisible continuation lines as empty, i.e.
     * not used. The caller (TclSetByteCodeFromAny) will set this up, if such
     * data is available.
     */

    envPtr->clNext = NULL;

    envPtr->auxDataArrayPtr = envPtr->staticAuxDataArraySpace;
    envPtr->auxDataArrayNext = 0;
    envPtr->auxDataArrayEnd = COMPILEENV_INIT_AUX_DATA_SIZE;
    envPtr->mallocedAuxDataArray = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFreeCompileEnv --
 *
 *	Free the storage allocated in a CompileEnv compilation environment
 *	structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocated storage in the CompileEnv structure is freed. Note that its
 *	local literal table is not deleted and its literal objects are not
 *	released. In addition, storage referenced by its auxiliary data items
 *	is not freed. This is done so that, when compilation is successful,
 *	"ownership" of these objects and aux data items is handed over to the
 *	corresponding ByteCode structure.
 *
 *----------------------------------------------------------------------
 */

void
TclFreeCompileEnv(
    register CompileEnv *envPtr)/* Points to the CompileEnv structure. */
{
    if (envPtr->localLitTable.buckets != envPtr->localLitTable.staticBuckets){
	ckfree(envPtr->localLitTable.buckets);
	envPtr->localLitTable.buckets = envPtr->localLitTable.staticBuckets;
    }
    if (envPtr->iPtr) {
	/*
	 * We never converted to Bytecode, so free the things we would
	 * have transferred to it.
	 */

	int i;
	LiteralEntry *entryPtr = envPtr->literalArrayPtr;
	AuxData *auxDataPtr = envPtr->auxDataArrayPtr;

	for (i = 0;  i < envPtr->literalArrayNext;  i++) {
	    TclReleaseLiteral((Tcl_Interp *)envPtr->iPtr, entryPtr->objPtr);
	    entryPtr++;
	}

#ifdef TCL_COMPILE_DEBUG
	TclVerifyGlobalLiteralTable(envPtr->iPtr);
#endif /*TCL_COMPILE_DEBUG*/

	for (i = 0;  i < envPtr->auxDataArrayNext;  i++) {
	    if (auxDataPtr->type->freeProc != NULL) {
		auxDataPtr->type->freeProc(auxDataPtr->clientData);
	    }
	    auxDataPtr++;
	}
    }
    if (envPtr->mallocedCodeArray) {
	ckfree(envPtr->codeStart);
    }
    if (envPtr->mallocedLiteralArray) {
	ckfree(envPtr->literalArrayPtr);
    }
    if (envPtr->mallocedExceptArray) {
	ckfree(envPtr->exceptArrayPtr);
	ckfree(envPtr->exceptAuxArrayPtr);
    }
    if (envPtr->mallocedCmdMap) {
	ckfree(envPtr->cmdMapPtr);
    }
    if (envPtr->mallocedAuxDataArray) {
	ckfree(envPtr->auxDataArrayPtr);
    }
    if (envPtr->extCmdMapPtr) {
	ReleaseCmdWordData(envPtr->extCmdMapPtr);
	envPtr->extCmdMapPtr = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclWordKnownAtCompileTime --
 *
 *	Test whether the value of a token is completely known at compile time.
 *
 * Results:
 *	Returns true if the tokenPtr argument points to a word value that is
 *	completely known at compile time. Generally, values that are known at
 *	compile time can be compiled to their values, while values that cannot
 *	be known until substitution at runtime must be compiled to bytecode
 *	instructions that perform that substitution. For several commands,
 *	whether or not arguments are known at compile time determine whether
 *	it is worthwhile to compile at all.
 *
 * Side effects:
 *	When returning true, appends the known value of the word to the
 *	unshared Tcl_Obj (*valuePtr), unless valuePtr is NULL.
 *
 *----------------------------------------------------------------------
 */

int
TclWordKnownAtCompileTime(
    Tcl_Token *tokenPtr,	/* Points to Tcl_Token we should check */
    Tcl_Obj *valuePtr)		/* If not NULL, points to an unshared Tcl_Obj
				 * to which we should append the known value
				 * of the word. */
{
    int numComponents = tokenPtr->numComponents;
    Tcl_Obj *tempPtr = NULL;

    if (tokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	if (valuePtr != NULL) {
	    Tcl_AppendToObj(valuePtr, tokenPtr[1].start, tokenPtr[1].size);
	}
	return 1;
    }
    if (tokenPtr->type != TCL_TOKEN_WORD) {
	return 0;
    }
    tokenPtr++;
    if (valuePtr != NULL) {
	tempPtr = Tcl_NewObj();
	Tcl_IncrRefCount(tempPtr);
    }
    while (numComponents--) {
	switch (tokenPtr->type) {
	case TCL_TOKEN_TEXT:
	    if (tempPtr != NULL) {
		Tcl_AppendToObj(tempPtr, tokenPtr->start, tokenPtr->size);
	    }
	    break;

	case TCL_TOKEN_BS:
	    if (tempPtr != NULL) {
		char utfBuf[TCL_UTF_MAX];
		int length = TclParseBackslash(tokenPtr->start,
			tokenPtr->size, NULL, utfBuf);

		Tcl_AppendToObj(tempPtr, utfBuf, length);
	    }
	    break;

	default:
	    if (tempPtr != NULL) {
		Tcl_DecrRefCount(tempPtr);
	    }
	    return 0;
	}
	tokenPtr++;
    }
    if (valuePtr != NULL) {
	Tcl_AppendObjToObj(valuePtr, tempPtr);
	Tcl_DecrRefCount(tempPtr);
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileScript --
 *
 *	Compile a Tcl script in a string.
 *
 * Results:
 *	The return value is TCL_OK on a successful compilation and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	Adds instructions to envPtr to evaluate the script at runtime.
 *
 *----------------------------------------------------------------------
 */

static int
ExpandRequested(
    Tcl_Token *tokenPtr,
    int numWords)
{
    /* Determine whether any words of the command require expansion */
    while (numWords--) {
	if (tokenPtr->type == TCL_TOKEN_EXPAND_WORD) {
	    return 1;
	}
	tokenPtr = TokenAfter(tokenPtr);
    }
    return 0;
}

static void
CompileCmdLiteral(
    Tcl_Interp *interp,
    Tcl_Obj *cmdObj,
    CompileEnv *envPtr)
{
    int numBytes;
    const char *bytes;
    Command *cmdPtr;
    int cmdLitIdx, extraLiteralFlags = LITERAL_CMD_NAME;

    cmdPtr = (Command *) Tcl_GetCommandFromObj(interp, cmdObj);
    if ((cmdPtr != NULL) && (cmdPtr->flags & CMD_VIA_RESOLVER)) {
	extraLiteralFlags |= LITERAL_UNSHARED;
    }

    bytes = Tcl_GetStringFromObj(cmdObj, &numBytes);
    cmdLitIdx = TclRegisterLiteral(envPtr, (char *)bytes, numBytes, extraLiteralFlags);

    if (cmdPtr) {
	TclSetCmdNameObj(interp, TclFetchLiteral(envPtr, cmdLitIdx), cmdPtr);
    }
    TclEmitPush(cmdLitIdx, envPtr);
}

void
TclCompileInvocation(
    Tcl_Interp *interp,
    Tcl_Token *tokenPtr,
    Tcl_Obj *cmdObj,
    int numWords,
    CompileEnv *envPtr)
{
    int wordIdx = 0, depth = TclGetStackDepth(envPtr);
    DefineLineInformation;

    if (cmdObj) {
	CompileCmdLiteral(interp, cmdObj, envPtr);
	wordIdx = 1;
	tokenPtr = TokenAfter(tokenPtr);
    }

    for (; wordIdx < numWords; wordIdx++, tokenPtr = TokenAfter(tokenPtr)) {
	int objIdx;

	SetLineInformation(wordIdx);

	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    CompileTokens(envPtr, tokenPtr, interp);
	    continue;
	}

	objIdx = TclRegisterNewLiteral(envPtr,
		tokenPtr[1].start, tokenPtr[1].size);
	if (envPtr->clNext) {
	    TclContinuationsEnterDerived(TclFetchLiteral(envPtr, objIdx),
		    tokenPtr[1].start - envPtr->source, envPtr->clNext);
	}
	TclEmitPush(objIdx, envPtr);
    }

    if (wordIdx <= 255) {
	TclEmitInvoke(envPtr, INST_INVOKE_STK1, wordIdx);
    } else {
	TclEmitInvoke(envPtr, INST_INVOKE_STK4, wordIdx);
    }
    TclCheckStackDepth(depth+1, envPtr);
}

static void
CompileExpanded(
    Tcl_Interp *interp,
    Tcl_Token *tokenPtr,
    Tcl_Obj *cmdObj,
    int numWords,
    CompileEnv *envPtr)
{
    int wordIdx = 0;
    DefineLineInformation;
    int depth = TclGetStackDepth(envPtr);

    StartExpanding(envPtr);
    if (cmdObj) {
	CompileCmdLiteral(interp, cmdObj, envPtr);
	wordIdx = 1;
	tokenPtr = TokenAfter(tokenPtr);
    }

    for (; wordIdx < numWords; wordIdx++, tokenPtr = TokenAfter(tokenPtr)) {
	int objIdx;

	SetLineInformation(wordIdx);

	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    CompileTokens(envPtr, tokenPtr, interp);
	    if (tokenPtr->type == TCL_TOKEN_EXPAND_WORD) {
		TclEmitInstInt4(INST_EXPAND_STKTOP,
			envPtr->currStackDepth, envPtr);
	    }
	    continue;
	}

	objIdx = TclRegisterNewLiteral(envPtr,
		tokenPtr[1].start, tokenPtr[1].size);
	if (envPtr->clNext) {
	    TclContinuationsEnterDerived(TclFetchLiteral(envPtr, objIdx),
		    tokenPtr[1].start - envPtr->source, envPtr->clNext);
	}
	TclEmitPush(objIdx, envPtr);
    }

    /*
     * The stack depth during argument expansion can only be managed at
     * runtime, as the number of elements in the expanded lists is not known
     * at compile time. We adjust here the stack depth estimate so that it is
     * correct after the command with expanded arguments returns.
     *
     * The end effect of this command's invocation is that all the words of
     * the command are popped from the stack, and the result is pushed: the
     * stack top changes by (1-wordIdx).
     *
     * Note that the estimates are not correct while the command is being
     * prepared and run, INST_EXPAND_STKTOP is not stack-neutral in general.
     */

    TclEmitInvoke(envPtr, INST_INVOKE_EXPANDED, wordIdx);
    TclCheckStackDepth(depth+1, envPtr);
}

static int
CompileCmdCompileProc(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    Command *cmdPtr,
    CompileEnv *envPtr)
{
    int unwind = 0, incrOffset = -1;
    DefineLineInformation;
    int depth = TclGetStackDepth(envPtr);

    /*
     * Emit of the INST_START_CMD instruction is controlled by the value of
     * envPtr->atCmdStart:
     *
     * atCmdStart == 2	: We are not using the INST_START_CMD instruction.
     * atCmdStart == 1	: INST_START_CMD was the last instruction emitted.
     *			: We do not need to emit another.  Instead we
     *			: increment the number of cmds started at it (except
     *			: for the special case at the start of a script.)
     * atCmdStart == 0	: The last instruction was something else.  We need
     *			: to emit INST_START_CMD here.
     */

    switch (envPtr->atCmdStart) {
    case 0:
	unwind = tclInstructionTable[INST_START_CMD].numBytes;
	TclEmitInstInt4(INST_START_CMD, 0, envPtr);
	incrOffset = envPtr->codeNext - envPtr->codeStart;
	TclEmitInt4(0, envPtr);
	break;
    case 1:
	if (envPtr->codeNext > envPtr->codeStart) {
	    incrOffset = envPtr->codeNext - 4 - envPtr->codeStart;
	}
	break;
    case 2:
	/* Nothing to do */
	;
    }

    if (TCL_OK == TclAttemptCompileProc(interp, parsePtr, 1, cmdPtr, envPtr)) {
	if (incrOffset >= 0) {
	    /*
	     * We successfully compiled a command.  Increment the number of
	     * commands that start at the currently active INST_START_CMD.
	     */

	    unsigned char *incrPtr = envPtr->codeStart + incrOffset;
	    unsigned char *startPtr = incrPtr - 5;

	    TclIncrUInt4AtPtr(incrPtr, 1);
	    if (unwind) {
		/* We started the INST_START_CMD.  Record the code length. */
		TclStoreInt4AtPtr(envPtr->codeNext - startPtr, startPtr + 1);
	    }
	}
	TclCheckStackDepth(depth+1, envPtr);
	return TCL_OK;
    }

    envPtr->codeNext -= unwind; /* Unwind INST_START_CMD */

    /*
     * Throw out any line information generated by the failed compile attempt.
     */

    while (mapPtr->nuloc - 1 > eclIndex) {
	mapPtr->nuloc--;
	ckfree(mapPtr->loc[mapPtr->nuloc].line);
	mapPtr->loc[mapPtr->nuloc].line = NULL;
    }

    /*
     * Reset the index of next command.  Toss out any from failed nested
     * partial compiles.
     */

    envPtr->numCommands = mapPtr->nuloc;
    return TCL_ERROR;
}

static int
CompileCommandTokens(
    Tcl_Interp *interp,
    Tcl_Parse *parsePtr,
    CompileEnv *envPtr)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Token *tokenPtr = parsePtr->tokenPtr;
    ExtCmdLoc *eclPtr = envPtr->extCmdMapPtr;
    Tcl_Obj *cmdObj = Tcl_NewObj();
    Command *cmdPtr = NULL;
    int code = TCL_ERROR;
    int cmdKnown, expand = -1;
    int *wlines, wlineat;
    int cmdLine = envPtr->line;
    int *clNext = envPtr->clNext;
    int cmdIdx = envPtr->numCommands;
    int startCodeOffset = envPtr->codeNext - envPtr->codeStart;
    int depth = TclGetStackDepth(envPtr);

    assert (parsePtr->numWords > 0);

    /* Pre-Compile */

    envPtr->numCommands++;
    EnterCmdStartData(envPtr, cmdIdx,
	    parsePtr->commandStart - envPtr->source, startCodeOffset);

    /*
     * TIP #280. Scan the words and compute the extended location information.
     * The map first contain full per-word line information for use by the
     * compiler. This is later replaced by a reduced form which signals
     * non-literal words, stored in 'wlines'.
     */

    EnterCmdWordData(eclPtr, parsePtr->commandStart - envPtr->source,
	    parsePtr->tokenPtr, parsePtr->commandStart,
	    parsePtr->commandSize, parsePtr->numWords, cmdLine,
	    clNext, &wlines, envPtr);
    wlineat = eclPtr->nuloc - 1;

    envPtr->line = eclPtr->loc[wlineat].line[0];
    envPtr->clNext = eclPtr->loc[wlineat].next[0];

    /* Do we know the command word? */
    Tcl_IncrRefCount(cmdObj);
    tokenPtr = parsePtr->tokenPtr;
    cmdKnown = TclWordKnownAtCompileTime(tokenPtr, cmdObj);

    /* Is this a command we should (try to) compile with a compileProc ? */
    if (cmdKnown && !(iPtr->flags & DONT_COMPILE_CMDS_INLINE)) {
	cmdPtr = (Command *) Tcl_GetCommandFromObj(interp, cmdObj);
	if (cmdPtr) {
	    /*
	     * Found a command.  Test the ways we can be told not to attempt
	     * to compile it.
	     */
	    if ((cmdPtr->compileProc == NULL)
		    || (cmdPtr->nsPtr->flags & NS_SUPPRESS_COMPILATION)
		    || (cmdPtr->flags & CMD_HAS_EXEC_TRACES)) {
		cmdPtr = NULL;
	    }
	}
	if (cmdPtr && !(cmdPtr->flags & CMD_COMPILES_EXPANDED)) {
	    expand = ExpandRequested(parsePtr->tokenPtr, parsePtr->numWords);
	    if (expand) {
		/* We need to expand, but compileProc cannot. */
		cmdPtr = NULL;
	    }
	}
    }

    /* If cmdPtr != NULL, we will try to call cmdPtr->compileProc */
    if (cmdPtr) {
	code = CompileCmdCompileProc(interp, parsePtr, cmdPtr, envPtr);
    }

    if (code == TCL_ERROR) {
	if (expand < 0) {
	    expand = ExpandRequested(parsePtr->tokenPtr, parsePtr->numWords);
	}

	if (expand) {
	    CompileExpanded(interp, parsePtr->tokenPtr,
		    cmdKnown ? cmdObj : NULL, parsePtr->numWords, envPtr);
	} else {
	    TclCompileInvocation(interp, parsePtr->tokenPtr,
		    cmdKnown ? cmdObj : NULL, parsePtr->numWords, envPtr);
	}
    }

    Tcl_DecrRefCount(cmdObj);

    TclEmitOpcode(INST_POP, envPtr);
    EnterCmdExtentData(envPtr, cmdIdx,
	    parsePtr->term - parsePtr->commandStart,
	    (envPtr->codeNext-envPtr->codeStart) - startCodeOffset);

    /*
     * TIP #280: Free full form of per-word line data and insert the reduced
     * form now
     */

    envPtr->line = cmdLine;
    envPtr->clNext = clNext;
    ckfree(eclPtr->loc[wlineat].line);
    ckfree(eclPtr->loc[wlineat].next);
    eclPtr->loc[wlineat].line = wlines;
    eclPtr->loc[wlineat].next = NULL;

    TclCheckStackDepth(depth, envPtr);
    return cmdIdx;
}

void
TclCompileScript(
    Tcl_Interp *interp,		/* Used for error and status reporting. Also
				 * serves as context for finding and compiling
				 * commands. May not be NULL. */
    const char *script,		/* The source script to compile. */
    int numBytes,		/* Number of bytes in script. If < 0, the
				 * script consists of all bytes up to the
				 * first null character. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    int lastCmdIdx = -1;	/* Index into envPtr->cmdMapPtr of the last
				 * command this routine compiles into bytecode.
				 * Initial value of -1 indicates this routine
				 * has not yet generated any bytecode. */
    const char *p = script;	/* Where we are in our compile. */
    int depth = TclGetStackDepth(envPtr);

    if (envPtr->iPtr == NULL) {
	Tcl_Panic("TclCompileScript() called on uninitialized CompileEnv");
    }

    /* Each iteration compiles one command from the script. */

    while (numBytes > 0) {
	Tcl_Parse parse;
	const char *next;

	if (TCL_OK != Tcl_ParseCommand(interp, p, numBytes, 0, &parse)) {
	    /*
	     * Compile bytecodes to report the parse error at runtime.
	     */

	    Tcl_LogCommandInfo(interp, script, parse.commandStart,
		    parse.term + 1 - parse.commandStart);
	    TclCompileSyntaxError(interp, envPtr);
	    return;
	}

#ifdef TCL_COMPILE_DEBUG
	/*
	 * If tracing, print a line for each top level command compiled.
	 * TODO: Suppress when numWords == 0 ?
	 */

	if ((tclTraceCompile >= 1) && (envPtr->procPtr == NULL)) {
	    int commandLength = parse.term - parse.commandStart;
	    fprintf(stdout, "  Compiling: ");
	    TclPrintSource(stdout, parse.commandStart,
		    TclMin(commandLength, 55));
	    fprintf(stdout, "\n");
	}
#endif

	/*
	 * TIP #280: Count newlines before the command start.
	 * (See test info-30.33).
	 */

	TclAdvanceLines(&envPtr->line, p, parse.commandStart);
	TclAdvanceContinuations(&envPtr->line, &envPtr->clNext,
		parse.commandStart - envPtr->source);

	/*
	 * Advance parser to the next command in the script.
	 */

	next = parse.commandStart + parse.commandSize;
	numBytes -= next - p;
	p = next;

	if (parse.numWords == 0) {
	    /*
	     * The "command" parsed has no words.  In this case we can skip
	     * the rest of the loop body.  With no words, clearly
	     * CompileCommandTokens() has nothing to do.  Since the parser
	     * aggressively sucks up leading comment and white space,
	     * including newlines, parse.commandStart must be pointing at
	     * either the end of script, or a command-terminating semi-colon.
	     * In either case, the TclAdvance*() calls have nothing to do.
	     * Finally, when no words are parsed, no tokens have been
	     * allocated at parse.tokenPtr so there's also nothing for
	     * Tcl_FreeParse() to do.
	     *
	     * The advantage of this shortcut is that CompileCommandTokens()
	     * can be written with an assumption that parse.numWords > 0, with
	     * the implication the CCT() always generates bytecode.
	     */
	    continue;
	}

	lastCmdIdx = CompileCommandTokens(interp, &parse, envPtr);

	/*
	 * TIP #280: Track lines in the just compiled command.
	 */

	TclAdvanceLines(&envPtr->line, parse.commandStart, p);
	TclAdvanceContinuations(&envPtr->line, &envPtr->clNext,
		p - envPtr->source);
	Tcl_FreeParse(&parse);
    }

    if (lastCmdIdx == -1) {
	/*
	 * Compiling the script yielded no bytecode.  The script must be all
	 * whitespace, comments, and empty commands.  Such scripts are defined
	 * to successfully produce the empty string result, so we emit the
	 * simple bytecode that makes that happen.
	 */

	PushStringLiteral(envPtr, "");
    } else {
	/*
	 * We compiled at least one command to bytecode.  The routine
	 * CompileCommandTokens() follows the bytecode of each compiled
	 * command with an INST_POP, so that stack balance is maintained when
	 * several commands are in sequence.  (The result of each command is
	 * thrown away before moving on to the next command).  For the last
	 * command compiled, we need to undo that INST_POP so that the result
	 * of the last command becomes the result of the script.  The code
	 * here removes that trailing INST_POP.
	 */

	envPtr->cmdMapPtr[lastCmdIdx].numCodeBytes--;
	envPtr->codeNext--;
	envPtr->currStackDepth++;
    }
    TclCheckStackDepth(depth+1, envPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileTokens --
 *
 *	Given an array of tokens parsed from a Tcl command (e.g., the tokens
 *	that make up a word) this procedure emits instructions to evaluate the
 *	tokens and concatenate their values to form a single result value on
 *	the interpreter's runtime evaluation stack.
 *
 * Results:
 *	The return value is a standard Tcl result. If an error occurs, an
 *	error message is left in the interpreter's result.
 *
 * Side effects:
 *	Instructions are added to envPtr to push and evaluate the tokens at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

void
TclCompileVarSubst(
    Tcl_Interp *interp,
    Tcl_Token *tokenPtr,
    CompileEnv *envPtr)
{
    const char *p, *name = tokenPtr[1].start;
    int nameBytes = tokenPtr[1].size;
    int i, localVar, localVarName = 1;

    /*
     * Determine how the variable name should be handled: if it contains any
     * namespace qualifiers it is not a local variable (localVarName=-1); if
     * it looks like an array element and the token has a single component, it
     * should not be created here [Bug 569438] (localVarName=0); otherwise,
     * the local variable can safely be created (localVarName=1).
     */

    for (i = 0, p = name;  i < nameBytes;  i++, p++) {
	if ((*p == ':') && (i < nameBytes-1) && (*(p+1) == ':')) {
	    localVarName = -1;
	    break;
	} else if ((*p == '(')
		&& (tokenPtr->numComponents == 1)
		&& (*(name + nameBytes - 1) == ')')) {
	    localVarName = 0;
	    break;
	}
    }

    /*
     * Either push the variable's name, or find its index in the array
     * of local variables in a procedure frame.
     */

    localVar = -1;
    if (localVarName != -1) {
	localVar = TclFindCompiledLocal(name, nameBytes, localVarName, envPtr);
    }
    if (localVar < 0) {
	PushLiteral(envPtr, name, nameBytes);
    }

    /*
     * Emit instructions to load the variable.
     */

    TclAdvanceLines(&envPtr->line, tokenPtr[1].start,
	    tokenPtr[1].start + tokenPtr[1].size);

    if (tokenPtr->numComponents == 1) {
	if (localVar < 0) {
	    TclEmitOpcode(INST_LOAD_STK, envPtr);
	} else if (localVar <= 255) {
	    TclEmitInstInt1(INST_LOAD_SCALAR1, localVar, envPtr);
	} else {
	    TclEmitInstInt4(INST_LOAD_SCALAR4, localVar, envPtr);
	}
    } else {
	TclCompileTokens(interp, tokenPtr+2, tokenPtr->numComponents-1, envPtr);
	if (localVar < 0) {
	    TclEmitOpcode(INST_LOAD_ARRAY_STK, envPtr);
	} else if (localVar <= 255) {
	    TclEmitInstInt1(INST_LOAD_ARRAY1, localVar, envPtr);
	} else {
	    TclEmitInstInt4(INST_LOAD_ARRAY4, localVar, envPtr);
	}
    }
}

void
TclCompileTokens(
    Tcl_Interp *interp,		/* Used for error and status reporting. */
    Tcl_Token *tokenPtr,	/* Pointer to first in an array of tokens to
				 * compile. */
    int count,			/* Number of tokens to consider at tokenPtr.
				 * Must be at least 1. */
    CompileEnv *envPtr)		/* Holds the resulting instructions. */
{
    Tcl_DString textBuffer;	/* Holds concatenated chars from adjacent
				 * TCL_TOKEN_TEXT, TCL_TOKEN_BS tokens. */
    char buffer[TCL_UTF_MAX];
    int i, numObjsToConcat, length, adjust;
    unsigned char *entryCodeNext = envPtr->codeNext;
#define NUM_STATIC_POS 20
    int isLiteral, maxNumCL, numCL;
    int *clPosition = NULL;
    int depth = TclGetStackDepth(envPtr);

    /*
     * For the handling of continuation lines in literals we first check if
     * this is actually a literal. For if not we can forego the additional
     * processing. Otherwise we pre-allocate a small table to store the
     * locations of all continuation lines we find in this literal, if any.
     * The table is extended if needed.
     *
     * Note: Different to the equivalent code in function 'TclSubstTokens()'
     * (see file "tclParse.c") we do not seem to need the 'adjust' variable.
     * We also do not seem to need code which merges continuation line
     * information of multiple words which concat'd at runtime. Either that or
     * I have not managed to find a test case for these two possibilities yet.
     * It might be a difference between compile- versus run-time processing.
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
    Tcl_DStringInit(&textBuffer);
    numObjsToConcat = 0;
    for ( ;  count > 0;  count--, tokenPtr++) {
	switch (tokenPtr->type) {
	case TCL_TOKEN_TEXT:
	    TclDStringAppendToken(&textBuffer, tokenPtr);
	    TclAdvanceLines(&envPtr->line, tokenPtr->start,
		    tokenPtr->start + tokenPtr->size);
	    break;

	case TCL_TOKEN_BS:
	    length = TclParseBackslash(tokenPtr->start, tokenPtr->size,
		    NULL, buffer);
	    Tcl_DStringAppend(&textBuffer, buffer, length);

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

	    if ((length == 1) && (buffer[0] == ' ') &&
		(tokenPtr->start[1] == '\n')) {
		if (isLiteral) {
		    int clPos = Tcl_DStringLength(&textBuffer);

		    if (numCL >= maxNumCL) {
			maxNumCL *= 2;
			clPosition = ckrealloc(clPosition,
                                maxNumCL * sizeof(int));
		    }
		    clPosition[numCL] = clPos;
		    numCL ++;
		}
		adjust++;
	    }
	    break;

	case TCL_TOKEN_COMMAND:
	    /*
	     * Push any accumulated chars appearing before the command.
	     */

	    if (Tcl_DStringLength(&textBuffer) > 0) {
		int literal = TclRegisterDStringLiteral(envPtr, &textBuffer);

		TclEmitPush(literal, envPtr);
		numObjsToConcat++;
		Tcl_DStringFree(&textBuffer);

		if (numCL) {
		    TclContinuationsEnter(TclFetchLiteral(envPtr, literal),
			    numCL, clPosition);
		}
		numCL = 0;
	    }

	    envPtr->line += adjust;
	    TclCompileScript(interp, tokenPtr->start+1,
		    tokenPtr->size-2, envPtr);
	    envPtr->line -= adjust;
	    numObjsToConcat++;
	    break;

	case TCL_TOKEN_VARIABLE:
	    /*
	     * Push any accumulated chars appearing before the $<var>.
	     */

	    if (Tcl_DStringLength(&textBuffer) > 0) {
		int literal;

		literal = TclRegisterDStringLiteral(envPtr, &textBuffer);
		TclEmitPush(literal, envPtr);
		numObjsToConcat++;
		Tcl_DStringFree(&textBuffer);
	    }

	    TclCompileVarSubst(interp, tokenPtr, envPtr);
	    numObjsToConcat++;
	    count -= tokenPtr->numComponents;
	    tokenPtr += tokenPtr->numComponents;
	    break;

	default:
	    Tcl_Panic("Unexpected token type in TclCompileTokens: %d; %.*s",
		    tokenPtr->type, tokenPtr->size, tokenPtr->start);
	}
    }

    /*
     * Push any accumulated characters appearing at the end.
     */

    if (Tcl_DStringLength(&textBuffer) > 0) {
	int literal = TclRegisterDStringLiteral(envPtr, &textBuffer);

	TclEmitPush(literal, envPtr);
	numObjsToConcat++;
	if (numCL) {
	    TclContinuationsEnter(TclFetchLiteral(envPtr, literal),
		    numCL, clPosition);
	}
	numCL = 0;
    }

    /*
     * If necessary, concatenate the parts of the word.
     */

    while (numObjsToConcat > 255) {
	TclEmitInstInt1(INST_STR_CONCAT1, 255, envPtr);
	numObjsToConcat -= 254;	/* concat pushes 1 obj, the result */
    }
    if (numObjsToConcat > 1) {
	TclEmitInstInt1(INST_STR_CONCAT1, numObjsToConcat, envPtr);
    }

    /*
     * If the tokens yielded no instructions, push an empty string.
     */

    if (envPtr->codeNext == entryCodeNext) {
	PushStringLiteral(envPtr, "");
    }
    Tcl_DStringFree(&textBuffer);

    /*
     * Release the temp table we used to collect the locations of continuation
     * lines, if any.
     */

    if (maxNumCL) {
	ckfree(clPosition);
    }
    TclCheckStackDepth(depth+1, envPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileCmdWord --
 *
 *	Given an array of parse tokens for a word containing one or more Tcl
 *	commands, emit inline instructions to execute them. This procedure
 *	differs from TclCompileTokens in that a simple word such as a loop
 *	body enclosed in braces is not just pushed as a string, but is itself
 *	parsed into tokens and compiled.
 *
 * Results:
 *	The return value is a standard Tcl result. If an error occurs, an
 *	error message is left in the interpreter's result.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the tokens at runtime.
 *
 *----------------------------------------------------------------------
 */

void
TclCompileCmdWord(
    Tcl_Interp *interp,		/* Used for error and status reporting. */
    Tcl_Token *tokenPtr,	/* Pointer to first in an array of tokens for
				 * a command word to compile inline. */
    int count,			/* Number of tokens to consider at tokenPtr.
				 * Must be at least 1. */
    CompileEnv *envPtr)		/* Holds the resulting instructions. */
{
    if ((count == 1) && (tokenPtr->type == TCL_TOKEN_TEXT)) {
	/*
	 * Handle the common case: if there is a single text token, compile it
	 * into an inline sequence of instructions.
	 */

	TclCompileScript(interp, tokenPtr->start, tokenPtr->size, envPtr);
    } else {
	/*
	 * Multiple tokens or the single token involves substitutions. Emit
	 * instructions to invoke the eval command procedure at runtime on the
	 * result of evaluating the tokens.
	 */

	TclCompileTokens(interp, tokenPtr, count, envPtr);
	TclEmitInvoke(envPtr, INST_EVAL_STK);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileExprWords --
 *
 *	Given an array of parse tokens representing one or more words that
 *	contain a Tcl expression, emit inline instructions to execute the
 *	expression. This procedure differs from TclCompileExpr in that it
 *	supports Tcl's two-level substitution semantics for expressions that
 *	appear as command words.
 *
 * Results:
 *	The return value is a standard Tcl result. If an error occurs, an
 *	error message is left in the interpreter's result.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the expression.
 *
 *----------------------------------------------------------------------
 */

void
TclCompileExprWords(
    Tcl_Interp *interp,		/* Used for error and status reporting. */
    Tcl_Token *tokenPtr,	/* Points to first in an array of word tokens
				 * tokens for the expression to compile
				 * inline. */
    int numWords,		/* Number of word tokens starting at tokenPtr.
				 * Must be at least 1. Each word token
				 * contains one or more subtokens. */
    CompileEnv *envPtr)		/* Holds the resulting instructions. */
{
    Tcl_Token *wordPtr;
    int i, concatItems;

    /*
     * If the expression is a single word that doesn't require substitutions,
     * just compile its string into inline instructions.
     */

    if ((numWords == 1) && (tokenPtr->type == TCL_TOKEN_SIMPLE_WORD)) {
	TclCompileExpr(interp, tokenPtr[1].start,tokenPtr[1].size, envPtr, 1);
	return;
    }

    /*
     * Emit code to call the expr command proc at runtime. Concatenate the
     * (already substituted once) expr tokens with a space between each.
     */

    wordPtr = tokenPtr;
    for (i = 0;  i < numWords;  i++) {
	CompileTokens(envPtr, wordPtr, interp);
	if (i < (numWords - 1)) {
	    PushStringLiteral(envPtr, " ");
	}
	wordPtr += wordPtr->numComponents + 1;
    }
    concatItems = 2*numWords - 1;
    while (concatItems > 255) {
	TclEmitInstInt1(INST_STR_CONCAT1, 255, envPtr);
	concatItems -= 254;
    }
    if (concatItems > 1) {
	TclEmitInstInt1(INST_STR_CONCAT1, concatItems, envPtr);
    }
    TclEmitOpcode(INST_EXPR_STK, envPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileNoOp --
 *
 *	Function called to compile no-op's
 *
 * Results:
 *	The return value is TCL_OK, indicating successful compilation.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute a no-op at runtime. No
 *	result is pushed onto the stack: the compiler has to take care of this
 *	itself if the last compiled command is a NoOp.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileNoOp(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tcl_Parse *parsePtr,	/* Points to a parse structure for the command
				 * created by Tcl_ParseCommand. */
    Command *cmdPtr,		/* Points to defintion of command being
				 * compiled. */
    CompileEnv *envPtr)		/* Holds resulting instructions. */
{
    Tcl_Token *tokenPtr;
    int i;

    tokenPtr = parsePtr->tokenPtr;
    for (i = 1; i < parsePtr->numWords; i++) {
	tokenPtr = tokenPtr + tokenPtr->numComponents + 1;

	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    CompileTokens(envPtr, tokenPtr, interp);
	    TclEmitOpcode(INST_POP, envPtr);
	}
    }
    PushStringLiteral(envPtr, "");
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitByteCodeObj --
 *
 *	Create a ByteCode structure and initialize it from a CompileEnv
 *	compilation environment structure. The ByteCode structure is smaller
 *	and contains just that information needed to execute the bytecode
 *	instructions resulting from compiling a Tcl script. The resulting
 *	structure is placed in the specified object.
 *
 * Results:
 *	A newly constructed ByteCode object is stored in the internal
 *	representation of the objPtr.
 *
 * Side effects:
 *	A single heap object is allocated to hold the new ByteCode structure
 *	and its code, object, command location, and aux data arrays. Note that
 *	"ownership" (i.e., the pointers to) the Tcl objects and aux data items
 *	will be handed over to the new ByteCode structure from the CompileEnv
 *	structure.
 *
 *----------------------------------------------------------------------
 */

void
TclInitByteCodeObj(
    Tcl_Obj *objPtr,		/* Points object that should be initialized,
				 * and whose string rep contains the source
				 * code. */
    register CompileEnv *envPtr)/* Points to the CompileEnv structure from
				 * which to create a ByteCode structure. */
{
    register ByteCode *codePtr;
    size_t codeBytes, objArrayBytes, exceptArrayBytes, cmdLocBytes;
    size_t auxDataArrayBytes, structureSize;
    register unsigned char *p;
#ifdef TCL_COMPILE_DEBUG
    unsigned char *nextPtr;
#endif
    int numLitObjects = envPtr->literalArrayNext;
    Namespace *namespacePtr;
    int i, isNew;
    Interp *iPtr;

    if (envPtr->iPtr == NULL) {
	Tcl_Panic("TclInitByteCodeObj() called on uninitialized CompileEnv");
    }

    iPtr = envPtr->iPtr;

    codeBytes = envPtr->codeNext - envPtr->codeStart;
    objArrayBytes = envPtr->literalArrayNext * sizeof(Tcl_Obj *);
    exceptArrayBytes = envPtr->exceptArrayNext * sizeof(ExceptionRange);
    auxDataArrayBytes = envPtr->auxDataArrayNext * sizeof(AuxData);
    cmdLocBytes = GetCmdLocEncodingSize(envPtr);

    /*
     * Compute the total number of bytes needed for this bytecode.
     */

    structureSize = sizeof(ByteCode);
    structureSize += TCL_ALIGN(codeBytes);	  /* align object array */
    structureSize += TCL_ALIGN(objArrayBytes);	  /* align exc range arr */
    structureSize += TCL_ALIGN(exceptArrayBytes); /* align AuxData array */
    structureSize += auxDataArrayBytes;
    structureSize += cmdLocBytes;

    if (envPtr->iPtr->varFramePtr != NULL) {
	namespacePtr = envPtr->iPtr->varFramePtr->nsPtr;
    } else {
	namespacePtr = envPtr->iPtr->globalNsPtr;
    }

    p = ckalloc(structureSize);
    codePtr = (ByteCode *) p;
    codePtr->interpHandle = TclHandlePreserve(iPtr->handle);
    codePtr->compileEpoch = iPtr->compileEpoch;
    codePtr->nsPtr = namespacePtr;
    codePtr->nsEpoch = namespacePtr->resolverEpoch;
    codePtr->refCount = 1;
    if (namespacePtr->compiledVarResProc || iPtr->resolverPtr) {
	codePtr->flags = TCL_BYTECODE_RESOLVE_VARS;
    } else {
	codePtr->flags = 0;
    }
    codePtr->source = envPtr->source;
    codePtr->procPtr = envPtr->procPtr;

    codePtr->numCommands = envPtr->numCommands;
    codePtr->numSrcBytes = envPtr->numSrcBytes;
    codePtr->numCodeBytes = codeBytes;
    codePtr->numLitObjects = numLitObjects;
    codePtr->numExceptRanges = envPtr->exceptArrayNext;
    codePtr->numAuxDataItems = envPtr->auxDataArrayNext;
    codePtr->numCmdLocBytes = cmdLocBytes;
    codePtr->maxExceptDepth = envPtr->maxExceptDepth;
    codePtr->maxStackDepth = envPtr->maxStackDepth;

    p += sizeof(ByteCode);
    codePtr->codeStart = p;
    memcpy(p, envPtr->codeStart, (size_t) codeBytes);

    p += TCL_ALIGN(codeBytes);		/* align object array */
    codePtr->objArrayPtr = (Tcl_Obj **) p;
    for (i = 0;  i < numLitObjects;  i++) {
	Tcl_Obj *fetched = TclFetchLiteral(envPtr, i);

	if (objPtr == fetched) {
	    /*
	     * Prevent circular reference where the bytecode intrep of
	     * a value contains a literal which is that same value.
	     * If this is allowed to happen, refcount decrements may not
	     * reach zero, and memory may leak.  Bugs 467523, 3357771
	     *
	     * NOTE:  [Bugs 3392070, 3389764] We make a copy based completely
	     * on the string value, and do not call Tcl_DuplicateObj() so we
             * can be sure we do not have any lingering cycles hiding in
	     * the intrep.
	     */
	    int numBytes;
	    const char *bytes = Tcl_GetStringFromObj(objPtr, &numBytes);

	    codePtr->objArrayPtr[i] = Tcl_NewStringObj(bytes, numBytes);
	    Tcl_IncrRefCount(codePtr->objArrayPtr[i]);
	    TclReleaseLiteral((Tcl_Interp *)iPtr, objPtr);
	} else {
	    codePtr->objArrayPtr[i] = fetched;
	}
    }

    p += TCL_ALIGN(objArrayBytes);	/* align exception range array */
    if (exceptArrayBytes > 0) {
	codePtr->exceptArrayPtr = (ExceptionRange *) p;
	memcpy(p, envPtr->exceptArrayPtr, (size_t) exceptArrayBytes);
    } else {
	codePtr->exceptArrayPtr = NULL;
    }

    p += TCL_ALIGN(exceptArrayBytes);	/* align AuxData array */
    if (auxDataArrayBytes > 0) {
	codePtr->auxDataArrayPtr = (AuxData *) p;
	memcpy(p, envPtr->auxDataArrayPtr, (size_t) auxDataArrayBytes);
    } else {
	codePtr->auxDataArrayPtr = NULL;
    }

    p += auxDataArrayBytes;
#ifndef TCL_COMPILE_DEBUG
    EncodeCmdLocMap(envPtr, codePtr, (unsigned char *) p);
#else
    nextPtr = EncodeCmdLocMap(envPtr, codePtr, (unsigned char *) p);
    if (((size_t)(nextPtr - p)) != cmdLocBytes) {
	Tcl_Panic("TclInitByteCodeObj: encoded cmd location bytes %lu != expected size %lu", (unsigned long)(nextPtr - p), (unsigned long)cmdLocBytes);
    }
#endif

    /*
     * Record various compilation-related statistics about the new ByteCode
     * structure. Don't include overhead for statistics-related fields.
     */

#ifdef TCL_COMPILE_STATS
    codePtr->structureSize = structureSize
	    - (sizeof(size_t) + sizeof(Tcl_Time));
    Tcl_GetTime(&codePtr->createTime);

    RecordByteCodeStats(codePtr);
#endif /* TCL_COMPILE_STATS */

    /*
     * Free the old internal rep then convert the object to a bytecode object
     * by making its internal rep point to the just compiled ByteCode.
     */

    TclFreeIntRep(objPtr);
    objPtr->internalRep.twoPtrValue.ptr1 = codePtr;
    objPtr->typePtr = &tclByteCodeType;

    /*
     * TIP #280. Associate the extended per-word line information with the
     * byte code object (internal rep), for use with the bc compiler.
     */

    Tcl_SetHashValue(Tcl_CreateHashEntry(iPtr->lineBCPtr, codePtr,
	    &isNew), envPtr->extCmdMapPtr);
    envPtr->extCmdMapPtr = NULL;

    /* We've used up the CompileEnv.  Mark as uninitialized. */
    envPtr->iPtr = NULL;

    codePtr->localCachePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFindCompiledLocal --
 *
 *	This procedure is called at compile time to look up and optionally
 *	allocate an entry ("slot") for a variable in a procedure's array of
 *	local variables. If the variable's name is NULL, a new temporary
 *	variable is always created. (Such temporary variables can only be
 *	referenced using their slot index.)
 *
 * Results:
 *	If create is 0 and the name is non-NULL, then if the variable is
 *	found, the index of its entry in the procedure's array of local
 *	variables is returned; otherwise -1 is returned. If name is NULL, the
 *	index of a new temporary variable is returned. Finally, if create is 1
 *	and name is non-NULL, the index of a new entry is returned.
 *
 * Side effects:
 *	Creates and registers a new local variable if create is 1 and the
 *	variable is unknown, or if the name is NULL.
 *
 *----------------------------------------------------------------------
 */

int
TclFindCompiledLocal(
    register const char *name,	/* Points to first character of the name of a
				 * scalar or array variable. If NULL, a
				 * temporary var should be created. */
    int nameBytes,		/* Number of bytes in the name. */
    int create,			/* If 1, allocate a local frame entry for the
				 * variable if it is new. */
    CompileEnv *envPtr)		/* Points to the current compile environment*/
{
    register CompiledLocal *localPtr;
    int localVar = -1;
    register int i;
    Proc *procPtr;

    /*
     * If not creating a temporary, does a local variable of the specified
     * name already exist?
     */

    procPtr = envPtr->procPtr;

    if (procPtr == NULL) {
	/*
	 * Compiling a non-body script: give it read access to the LVT in the
	 * current localCache
	 */

	LocalCache *cachePtr = envPtr->iPtr->varFramePtr->localCachePtr;
	const char *localName;
	Tcl_Obj **varNamePtr;
	int len;

	if (!cachePtr || !name) {
	    return -1;
	}

	varNamePtr = &cachePtr->varName0;
	for (i=0; i < cachePtr->numVars; varNamePtr++, i++) {
	    if (*varNamePtr) {
		localName = Tcl_GetStringFromObj(*varNamePtr, &len);
		if ((len == nameBytes) && !strncmp(name, localName, len)) {
		    return i;
		}
	    }
	}
	return -1;
    }

    if (name != NULL) {
	int localCt = procPtr->numCompiledLocals;

	localPtr = procPtr->firstLocalPtr;
	for (i = 0;  i < localCt;  i++) {
	    if (!TclIsVarTemporary(localPtr)) {
		char *localName = localPtr->name;

		if ((nameBytes == localPtr->nameLength) &&
			(strncmp(name,localName,(unsigned)nameBytes) == 0)) {
		    return i;
		}
	    }
	    localPtr = localPtr->nextPtr;
	}
    }

    /*
     * Create a new variable if appropriate.
     */

    if (create || (name == NULL)) {
	localVar = procPtr->numCompiledLocals;
	localPtr = ckalloc(TclOffset(CompiledLocal, name) + nameBytes + 1);
	if (procPtr->firstLocalPtr == NULL) {
	    procPtr->firstLocalPtr = procPtr->lastLocalPtr = localPtr;
	} else {
	    procPtr->lastLocalPtr->nextPtr = localPtr;
	    procPtr->lastLocalPtr = localPtr;
	}
	localPtr->nextPtr = NULL;
	localPtr->nameLength = nameBytes;
	localPtr->frameIndex = localVar;
	localPtr->flags = 0;
	if (name == NULL) {
	    localPtr->flags |= VAR_TEMPORARY;
	}
	localPtr->defValuePtr = NULL;
	localPtr->resolveInfo = NULL;

	if (name != NULL) {
	    memcpy(localPtr->name, name, (size_t) nameBytes);
	}
	localPtr->name[nameBytes] = '\0';
	procPtr->numCompiledLocals++;
    }
    return localVar;
}

/*
 *----------------------------------------------------------------------
 *
 * TclExpandCodeArray --
 *
 *	Procedure that uses malloc to allocate more storage for a CompileEnv's
 *	code array.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The byte code array in *envPtr is reallocated to a new array of double
 *	the size, and if envPtr->mallocedCodeArray is non-zero the old array
 *	is freed. Byte codes are copied from the old array to the new one.
 *
 *----------------------------------------------------------------------
 */

void
TclExpandCodeArray(
    void *envArgPtr)		/* Points to the CompileEnv whose code array
				 * must be enlarged. */
{
    CompileEnv *envPtr = envArgPtr;
				/* The CompileEnv containing the code array to
				 * be doubled in size. */

    /*
     * envPtr->codeNext is equal to envPtr->codeEnd. The currently defined
     * code bytes are stored between envPtr->codeStart and envPtr->codeNext-1
     * [inclusive].
     */

    size_t currBytes = envPtr->codeNext - envPtr->codeStart;
    size_t newBytes = 2 * (envPtr->codeEnd - envPtr->codeStart);

    if (envPtr->mallocedCodeArray) {
	envPtr->codeStart = ckrealloc(envPtr->codeStart, newBytes);
    } else {
	/*
	 * envPtr->codeStart isn't a ckalloc'd pointer, so we must code a
	 * ckrealloc equivalent for ourselves.
	 */

	unsigned char *newPtr = ckalloc(newBytes);

	memcpy(newPtr, envPtr->codeStart, currBytes);
	envPtr->codeStart = newPtr;
	envPtr->mallocedCodeArray = 1;
    }

    envPtr->codeNext = envPtr->codeStart + currBytes;
    envPtr->codeEnd = envPtr->codeStart + newBytes;
}

/*
 *----------------------------------------------------------------------
 *
 * EnterCmdStartData --
 *
 *	Registers the starting source and bytecode location of a command. This
 *	information is used at runtime to map between instruction pc and
 *	source locations.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Inserts source and code location information into the compilation
 *	environment envPtr for the command at index cmdIndex. The compilation
 *	environment's CmdLocation array is grown if necessary.
 *
 *----------------------------------------------------------------------
 */

static void
EnterCmdStartData(
    CompileEnv *envPtr,		/* Points to the compilation environment
				 * structure in which to enter command
				 * location information. */
    int cmdIndex,		/* Index of the command whose start data is
				 * being set. */
    int srcOffset,		/* Offset of first char of the command. */
    int codeOffset)		/* Offset of first byte of command code. */
{
    CmdLocation *cmdLocPtr;

    if ((cmdIndex < 0) || (cmdIndex >= envPtr->numCommands)) {
	Tcl_Panic("EnterCmdStartData: bad command index %d", cmdIndex);
    }

    if (cmdIndex >= envPtr->cmdMapEnd) {
	/*
	 * Expand the command location array by allocating more storage from
	 * the heap. The currently allocated CmdLocation entries are stored
	 * from cmdMapPtr[0] up to cmdMapPtr[envPtr->cmdMapEnd] (inclusive).
	 */

	size_t currElems = envPtr->cmdMapEnd;
	size_t newElems = 2 * currElems;
	size_t currBytes = currElems * sizeof(CmdLocation);
	size_t newBytes = newElems * sizeof(CmdLocation);

	if (envPtr->mallocedCmdMap) {
	    envPtr->cmdMapPtr = ckrealloc(envPtr->cmdMapPtr, newBytes);
	} else {
	    /*
	     * envPtr->cmdMapPtr isn't a ckalloc'd pointer, so we must code a
	     * ckrealloc equivalent for ourselves.
	     */

	    CmdLocation *newPtr = ckalloc(newBytes);

	    memcpy(newPtr, envPtr->cmdMapPtr, currBytes);
	    envPtr->cmdMapPtr = newPtr;
	    envPtr->mallocedCmdMap = 1;
	}
	envPtr->cmdMapEnd = newElems;
    }

    if (cmdIndex > 0) {
	if (codeOffset < envPtr->cmdMapPtr[cmdIndex-1].codeOffset) {
	    Tcl_Panic("EnterCmdStartData: cmd map not sorted by code offset");
	}
    }

    cmdLocPtr = &envPtr->cmdMapPtr[cmdIndex];
    cmdLocPtr->codeOffset = codeOffset;
    cmdLocPtr->srcOffset = srcOffset;
    cmdLocPtr->numSrcBytes = -1;
    cmdLocPtr->numCodeBytes = -1;
}

/*
 *----------------------------------------------------------------------
 *
 * EnterCmdExtentData --
 *
 *	Registers the source and bytecode length for a command. This
 *	information is used at runtime to map between instruction pc and
 *	source locations.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Inserts source and code length information into the compilation
 *	environment envPtr for the command at index cmdIndex. Starting source
 *	and bytecode information for the command must already have been
 *	registered.
 *
 *----------------------------------------------------------------------
 */

static void
EnterCmdExtentData(
    CompileEnv *envPtr,		/* Points to the compilation environment
				 * structure in which to enter command
				 * location information. */
    int cmdIndex,		/* Index of the command whose source and code
				 * length data is being set. */
    int numSrcBytes,		/* Number of command source chars. */
    int numCodeBytes)		/* Offset of last byte of command code. */
{
    CmdLocation *cmdLocPtr;

    if ((cmdIndex < 0) || (cmdIndex >= envPtr->numCommands)) {
	Tcl_Panic("EnterCmdExtentData: bad command index %d", cmdIndex);
    }

    if (cmdIndex > envPtr->cmdMapEnd) {
	Tcl_Panic("EnterCmdExtentData: missing start data for command %d",
		cmdIndex);
    }

    cmdLocPtr = &envPtr->cmdMapPtr[cmdIndex];
    cmdLocPtr->numSrcBytes = numSrcBytes;
    cmdLocPtr->numCodeBytes = numCodeBytes;
}

/*
 *----------------------------------------------------------------------
 * TIP #280
 *
 * EnterCmdWordData --
 *
 *	Registers the lines for the words of a command. This information is
 *	used at runtime by 'info frame'.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Inserts word location information into the compilation environment
 *	envPtr for the command at index cmdIndex. The compilation
 *	environment's ExtCmdLoc.ECL array is grown if necessary.
 *
 *----------------------------------------------------------------------
 */

static void
EnterCmdWordData(
    ExtCmdLoc *eclPtr,		/* Points to the map environment structure in
				 * which to enter command location
				 * information. */
    int srcOffset,		/* Offset of first char of the command. */
    Tcl_Token *tokenPtr,
    const char *cmd,
    int len,
    int numWords,
    int line,
    int *clNext,
    int **wlines,
    CompileEnv *envPtr)
{
    ECL *ePtr;
    const char *last;
    int wordIdx, wordLine, *wwlines, *wordNext;

    if (eclPtr->nuloc >= eclPtr->nloc) {
	/*
	 * Expand the ECL array by allocating more storage from the heap. The
	 * currently allocated ECL entries are stored from eclPtr->loc[0] up
	 * to eclPtr->loc[eclPtr->nuloc-1] (inclusive).
	 */

	size_t currElems = eclPtr->nloc;
	size_t newElems = (currElems ? 2*currElems : 1);
	size_t newBytes = newElems * sizeof(ECL);

	eclPtr->loc = ckrealloc(eclPtr->loc, newBytes);
	eclPtr->nloc = newElems;
    }

    ePtr = &eclPtr->loc[eclPtr->nuloc];
    ePtr->srcOffset = srcOffset;
    ePtr->line = ckalloc(numWords * sizeof(int));
    ePtr->next = ckalloc(numWords * sizeof(int *));
    ePtr->nline = numWords;
    wwlines = ckalloc(numWords * sizeof(int));

    last = cmd;
    wordLine = line;
    wordNext = clNext;
    for (wordIdx=0 ; wordIdx<numWords;
	    wordIdx++, tokenPtr += tokenPtr->numComponents + 1) {
	TclAdvanceLines(&wordLine, last, tokenPtr->start);
	TclAdvanceContinuations(&wordLine, &wordNext,
		tokenPtr->start - envPtr->source);
	/* See Ticket 4b61afd660 */
	wwlines[wordIdx] =
		((wordIdx == 0) || TclWordKnownAtCompileTime(tokenPtr, NULL))
		? wordLine : -1;
	ePtr->line[wordIdx] = wordLine;
	ePtr->next[wordIdx] = wordNext;
	last = tokenPtr->start;
    }

    *wlines = wwlines;
    eclPtr->nuloc ++;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCreateExceptRange --
 *
 *	Procedure that allocates and initializes a new ExceptionRange
 *	structure of the specified kind in a CompileEnv.
 *
 * Results:
 *	Returns the index for the newly created ExceptionRange.
 *
 * Side effects:
 *	If there is not enough room in the CompileEnv's ExceptionRange array,
 *	the array in expanded: a new array of double the size is allocated, if
 *	envPtr->mallocedExceptArray is non-zero the old array is freed, and
 *	ExceptionRange entries are copied from the old array to the new one.
 *
 *----------------------------------------------------------------------
 */

int
TclCreateExceptRange(
    ExceptionRangeType type,	/* The kind of ExceptionRange desired. */
    register CompileEnv *envPtr)/* Points to CompileEnv for which to create a
				 * new ExceptionRange structure. */
{
    register ExceptionRange *rangePtr;
    register ExceptionAux *auxPtr;
    int index = envPtr->exceptArrayNext;

    if (index >= envPtr->exceptArrayEnd) {
	/*
	 * Expand the ExceptionRange array. The currently allocated entries
	 * are stored between elements 0 and (envPtr->exceptArrayNext - 1)
	 * [inclusive].
	 */

	size_t currBytes =
		envPtr->exceptArrayNext * sizeof(ExceptionRange);
	size_t currBytes2 = envPtr->exceptArrayNext * sizeof(ExceptionAux);
	int newElems = 2*envPtr->exceptArrayEnd;
	size_t newBytes = newElems * sizeof(ExceptionRange);
	size_t newBytes2 = newElems * sizeof(ExceptionAux);

	if (envPtr->mallocedExceptArray) {
	    envPtr->exceptArrayPtr =
		    ckrealloc(envPtr->exceptArrayPtr, newBytes);
	    envPtr->exceptAuxArrayPtr =
		    ckrealloc(envPtr->exceptAuxArrayPtr, newBytes2);
	} else {
	    /*
	     * envPtr->exceptArrayPtr isn't a ckalloc'd pointer, so we must
	     * code a ckrealloc equivalent for ourselves.
	     */

	    ExceptionRange *newPtr = ckalloc(newBytes);
	    ExceptionAux *newPtr2 = ckalloc(newBytes2);

	    memcpy(newPtr, envPtr->exceptArrayPtr, currBytes);
	    memcpy(newPtr2, envPtr->exceptAuxArrayPtr, currBytes2);
	    envPtr->exceptArrayPtr = newPtr;
	    envPtr->exceptAuxArrayPtr = newPtr2;
	    envPtr->mallocedExceptArray = 1;
	}
	envPtr->exceptArrayEnd = newElems;
    }
    envPtr->exceptArrayNext++;

    rangePtr = &envPtr->exceptArrayPtr[index];
    rangePtr->type = type;
    rangePtr->nestingLevel = envPtr->exceptDepth;
    rangePtr->codeOffset = -1;
    rangePtr->numCodeBytes = -1;
    rangePtr->breakOffset = -1;
    rangePtr->continueOffset = -1;
    rangePtr->catchOffset = -1;
    auxPtr = &envPtr->exceptAuxArrayPtr[index];
    auxPtr->supportsContinue = 1;
    auxPtr->stackDepth = envPtr->currStackDepth;
    auxPtr->expandTarget = envPtr->expandCount;
    auxPtr->expandTargetDepth = -1;
    auxPtr->numBreakTargets = 0;
    auxPtr->breakTargets = NULL;
    auxPtr->allocBreakTargets = 0;
    auxPtr->numContinueTargets = 0;
    auxPtr->continueTargets = NULL;
    auxPtr->allocContinueTargets = 0;
    return index;
}

/*
 * ---------------------------------------------------------------------
 *
 * TclGetInnermostExceptionRange --
 *
 *	Returns the innermost exception range that covers the current code
 *	creation point, and (optionally) the stack depth that is expected at
 *	that point. Relies on the fact that the range has a numCodeBytes = -1
 *	when it is being populated and that inner ranges come after outer
 *	ranges.
 *
 * ---------------------------------------------------------------------
 */

ExceptionRange *
TclGetInnermostExceptionRange(
    CompileEnv *envPtr,
    int returnCode,
    ExceptionAux **auxPtrPtr)
{
    int i = envPtr->exceptArrayNext;
    ExceptionRange *rangePtr = envPtr->exceptArrayPtr + i;

    while (i > 0) {
	rangePtr--; i--;

	if (CurrentOffset(envPtr) >= rangePtr->codeOffset &&
		(rangePtr->numCodeBytes == -1 || CurrentOffset(envPtr) <
			rangePtr->codeOffset+rangePtr->numCodeBytes) &&
		(returnCode != TCL_CONTINUE ||
			envPtr->exceptAuxArrayPtr[i].supportsContinue)) {

	    if (auxPtrPtr) {
		*auxPtrPtr = envPtr->exceptAuxArrayPtr + i;
	    }
	    return rangePtr;
	}
    }
    return NULL;
}

/*
 * ---------------------------------------------------------------------
 *
 * TclAddLoopBreakFixup, TclAddLoopContinueFixup --
 *
 *	Adds a place that wants to break/continue to the loop exception range
 *	tracking that will be fixed up once the loop can be finalized. These
 *	functions will generate an INST_JUMP4 that will be fixed up during the
 *	loop finalization.
 *
 * ---------------------------------------------------------------------
 */

void
TclAddLoopBreakFixup(
    CompileEnv *envPtr,
    ExceptionAux *auxPtr)
{
    int range = auxPtr - envPtr->exceptAuxArrayPtr;

    if (envPtr->exceptArrayPtr[range].type != LOOP_EXCEPTION_RANGE) {
	Tcl_Panic("trying to add 'break' fixup to full exception range");
    }

    if (++auxPtr->numBreakTargets > auxPtr->allocBreakTargets) {
	auxPtr->allocBreakTargets *= 2;
	auxPtr->allocBreakTargets += 2;
	if (auxPtr->breakTargets) {
	    auxPtr->breakTargets = ckrealloc(auxPtr->breakTargets,
		    sizeof(int) * auxPtr->allocBreakTargets);
	} else {
	    auxPtr->breakTargets =
		    ckalloc(sizeof(int) * auxPtr->allocBreakTargets);
	}
    }
    auxPtr->breakTargets[auxPtr->numBreakTargets - 1] = CurrentOffset(envPtr);
    TclEmitInstInt4(INST_JUMP4, 0, envPtr);
}

void
TclAddLoopContinueFixup(
    CompileEnv *envPtr,
    ExceptionAux *auxPtr)
{
    int range = auxPtr - envPtr->exceptAuxArrayPtr;

    if (envPtr->exceptArrayPtr[range].type != LOOP_EXCEPTION_RANGE) {
	Tcl_Panic("trying to add 'continue' fixup to full exception range");
    }

    if (++auxPtr->numContinueTargets > auxPtr->allocContinueTargets) {
	auxPtr->allocContinueTargets *= 2;
	auxPtr->allocContinueTargets += 2;
	if (auxPtr->continueTargets) {
	    auxPtr->continueTargets = ckrealloc(auxPtr->continueTargets,
		    sizeof(int) * auxPtr->allocContinueTargets);
	} else {
	    auxPtr->continueTargets =
		    ckalloc(sizeof(int) * auxPtr->allocContinueTargets);
	}
    }
    auxPtr->continueTargets[auxPtr->numContinueTargets - 1] =
	    CurrentOffset(envPtr);
    TclEmitInstInt4(INST_JUMP4, 0, envPtr);
}

/*
 * ---------------------------------------------------------------------
 *
 * TclCleanupStackForBreakContinue --
 *
 *	Ditch the extra elements from the auxiliary stack and the main stack.
 *	How to do this exactly depends on whether there are any elements on
 *	the auxiliary stack to pop.
 *
 * ---------------------------------------------------------------------
 */

void
TclCleanupStackForBreakContinue(
    CompileEnv *envPtr,
    ExceptionAux *auxPtr)
{
    int savedStackDepth = envPtr->currStackDepth;
    int toPop = envPtr->expandCount - auxPtr->expandTarget;

    if (toPop > 0) {
	while (toPop --> 0) {
	    TclEmitOpcode(INST_EXPAND_DROP, envPtr);
	}
	TclAdjustStackDepth(auxPtr->expandTargetDepth - envPtr->currStackDepth,
		envPtr);
	envPtr->currStackDepth = auxPtr->expandTargetDepth;
    }
    toPop = envPtr->currStackDepth - auxPtr->stackDepth;
    while (toPop --> 0) {
	TclEmitOpcode(INST_POP, envPtr);
    }
    envPtr->currStackDepth = savedStackDepth;
}

/*
 * ---------------------------------------------------------------------
 *
 * StartExpanding --
 *
 *	Pushes an INST_EXPAND_START and does some additional housekeeping so
 *	that the [break] and [continue] compilers can use an exception-free
 *	issue to discard it.
 *
 * ---------------------------------------------------------------------
 */

static void
StartExpanding(
    CompileEnv *envPtr)
{
    int i;

    TclEmitOpcode(INST_EXPAND_START, envPtr);

    /*
     * Update inner exception ranges with information about the environment
     * where this expansion started.
     */

    for (i=0 ; i<envPtr->exceptArrayNext ; i++) {
	ExceptionRange *rangePtr = &envPtr->exceptArrayPtr[i];
	ExceptionAux *auxPtr = &envPtr->exceptAuxArrayPtr[i];

	/*
	 * Ignore loops unless they're still being built.
	 */

	if (rangePtr->codeOffset > CurrentOffset(envPtr)) {
	    continue;
	}
	if (rangePtr->numCodeBytes != -1) {
	    continue;
	}

	/*
	 * Adequate condition: further out loops and further in exceptions
	 * don't actually need this information.
	 */

	if (auxPtr->expandTarget == envPtr->expandCount) {
	    auxPtr->expandTargetDepth = envPtr->currStackDepth;
	}
    }

    /*
     * There's now one more expansion being processed on the auxiliary stack.
     */

    envPtr->expandCount++;
}

/*
 * ---------------------------------------------------------------------
 *
 * TclFinalizeLoopExceptionRange --
 *
 *	Finalizes a loop exception range, binding the registered [break] and
 *	[continue] implementations so that they jump to the correct place.
 *	Note that this must only be called after *all* the exception range
 *	target offsets have been set.
 *
 * ---------------------------------------------------------------------
 */

void
TclFinalizeLoopExceptionRange(
    CompileEnv *envPtr,
    int range)
{
    ExceptionRange *rangePtr = &envPtr->exceptArrayPtr[range];
    ExceptionAux *auxPtr = &envPtr->exceptAuxArrayPtr[range];
    int i, offset;
    unsigned char *site;

    if (rangePtr->type != LOOP_EXCEPTION_RANGE) {
	Tcl_Panic("trying to finalize a loop exception range");
    }

    /*
     * Do the jump fixups. Note that these are always issued as INST_JUMP4 so
     * there is no need to fuss around with updating code offsets.
     */

    for (i=0 ; i<auxPtr->numBreakTargets ; i++) {
	site = envPtr->codeStart + auxPtr->breakTargets[i];
	offset = rangePtr->breakOffset - auxPtr->breakTargets[i];
	TclUpdateInstInt4AtPc(INST_JUMP4, offset, site);
    }
    for (i=0 ; i<auxPtr->numContinueTargets ; i++) {
	site = envPtr->codeStart + auxPtr->continueTargets[i];
	if (rangePtr->continueOffset == -1) {
	    int j;

	    /*
	     * WTF? Can't bind, so revert to an INST_CONTINUE. Not enough
	     * space to do anything else.
	     */

	    *site = INST_CONTINUE;
	    for (j=0 ; j<4 ; j++) {
		*++site = INST_NOP;
	    }
	} else {
	    offset = rangePtr->continueOffset - auxPtr->continueTargets[i];
	    TclUpdateInstInt4AtPc(INST_JUMP4, offset, site);
	}
    }

    /*
     * Drop the arrays we were holding the only reference to.
     */

    if (auxPtr->breakTargets) {
	ckfree(auxPtr->breakTargets);
	auxPtr->breakTargets = NULL;
	auxPtr->numBreakTargets = 0;
    }
    if (auxPtr->continueTargets) {
	ckfree(auxPtr->continueTargets);
	auxPtr->continueTargets = NULL;
	auxPtr->numContinueTargets = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclCreateAuxData --
 *
 *	Procedure that allocates and initializes a new AuxData structure in a
 *	CompileEnv's array of compilation auxiliary data records. These
 *	AuxData records hold information created during compilation by
 *	CompileProcs and used by instructions during execution.
 *
 * Results:
 *	Returns the index for the newly created AuxData structure.
 *
 * Side effects:
 *	If there is not enough room in the CompileEnv's AuxData array, the
 *	AuxData array in expanded: a new array of double the size is
 *	allocated, if envPtr->mallocedAuxDataArray is non-zero the old array
 *	is freed, and AuxData entries are copied from the old array to the new
 *	one.
 *
 *----------------------------------------------------------------------
 */

int
TclCreateAuxData(
    ClientData clientData,	/* The compilation auxiliary data to store in
				 * the new aux data record. */
    const AuxDataType *typePtr,	/* Pointer to the type to attach to this
				 * AuxData */
    register CompileEnv *envPtr)/* Points to the CompileEnv for which a new
				 * aux data structure is to be allocated. */
{
    int index;			/* Index for the new AuxData structure. */
    register AuxData *auxDataPtr;
				/* Points to the new AuxData structure */

    index = envPtr->auxDataArrayNext;
    if (index >= envPtr->auxDataArrayEnd) {
	/*
	 * Expand the AuxData array. The currently allocated entries are
	 * stored between elements 0 and (envPtr->auxDataArrayNext - 1)
	 * [inclusive].
	 */

	size_t currBytes = envPtr->auxDataArrayNext * sizeof(AuxData);
	int newElems = 2*envPtr->auxDataArrayEnd;
	size_t newBytes = newElems * sizeof(AuxData);

	if (envPtr->mallocedAuxDataArray) {
	    envPtr->auxDataArrayPtr =
		    ckrealloc(envPtr->auxDataArrayPtr, newBytes);
	} else {
	    /*
	     * envPtr->auxDataArrayPtr isn't a ckalloc'd pointer, so we must
	     * code a ckrealloc equivalent for ourselves.
	     */

	    AuxData *newPtr = ckalloc(newBytes);

	    memcpy(newPtr, envPtr->auxDataArrayPtr, currBytes);
	    envPtr->auxDataArrayPtr = newPtr;
	    envPtr->mallocedAuxDataArray = 1;
	}
	envPtr->auxDataArrayEnd = newElems;
    }
    envPtr->auxDataArrayNext++;

    auxDataPtr = &envPtr->auxDataArrayPtr[index];
    auxDataPtr->clientData = clientData;
    auxDataPtr->type = typePtr;
    return index;
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitJumpFixupArray --
 *
 *	Initializes a JumpFixupArray structure to hold some number of jump
 *	fixup entries.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The JumpFixupArray structure is initialized.
 *
 *----------------------------------------------------------------------
 */

void
TclInitJumpFixupArray(
    register JumpFixupArray *fixupArrayPtr)
				/* Points to the JumpFixupArray structure to
				 * initialize. */
{
    fixupArrayPtr->fixup = fixupArrayPtr->staticFixupSpace;
    fixupArrayPtr->next = 0;
    fixupArrayPtr->end = JUMPFIXUP_INIT_ENTRIES - 1;
    fixupArrayPtr->mallocedArray = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclExpandJumpFixupArray --
 *
 *	Procedure that uses malloc to allocate more storage for a jump fixup
 *	array.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The jump fixup array in *fixupArrayPtr is reallocated to a new array
 *	of double the size, and if fixupArrayPtr->mallocedArray is non-zero
 *	the old array is freed. Jump fixup structures are copied from the old
 *	array to the new one.
 *
 *----------------------------------------------------------------------
 */

void
TclExpandJumpFixupArray(
    register JumpFixupArray *fixupArrayPtr)
				/* Points to the JumpFixupArray structure to
				 * enlarge. */
{
    /*
     * The currently allocated jump fixup entries are stored from fixup[0] up
     * to fixup[fixupArrayPtr->fixupNext] (*not* inclusive). We assume
     * fixupArrayPtr->fixupNext is equal to fixupArrayPtr->fixupEnd.
     */

    size_t currBytes = fixupArrayPtr->next * sizeof(JumpFixup);
    int newElems = 2*(fixupArrayPtr->end + 1);
    size_t newBytes = newElems * sizeof(JumpFixup);

    if (fixupArrayPtr->mallocedArray) {
	fixupArrayPtr->fixup = ckrealloc(fixupArrayPtr->fixup, newBytes);
    } else {
	/*
	 * fixupArrayPtr->fixup isn't a ckalloc'd pointer, so we must code a
	 * ckrealloc equivalent for ourselves.
	 */

	JumpFixup *newPtr = ckalloc(newBytes);

	memcpy(newPtr, fixupArrayPtr->fixup, currBytes);
	fixupArrayPtr->fixup = newPtr;
	fixupArrayPtr->mallocedArray = 1;
    }
    fixupArrayPtr->end = newElems;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFreeJumpFixupArray --
 *
 *	Free any storage allocated in a jump fixup array structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocated storage in the JumpFixupArray structure is freed.
 *
 *----------------------------------------------------------------------
 */

void
TclFreeJumpFixupArray(
    register JumpFixupArray *fixupArrayPtr)
				/* Points to the JumpFixupArray structure to
				 * free. */
{
    if (fixupArrayPtr->mallocedArray) {
	ckfree(fixupArrayPtr->fixup);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclEmitForwardJump --
 *
 *	Procedure to emit a two-byte forward jump of kind "jumpType". Since
 *	the jump may later have to be grown to five bytes if the jump target
 *	is more than, say, 127 bytes away, this procedure also initializes a
 *	JumpFixup record with information about the jump.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The JumpFixup record pointed to by "jumpFixupPtr" is initialized with
 *	information needed later if the jump is to be grown. Also, a two byte
 *	jump of the designated type is emitted at the current point in the
 *	bytecode stream.
 *
 *----------------------------------------------------------------------
 */

void
TclEmitForwardJump(
    CompileEnv *envPtr,		/* Points to the CompileEnv structure that
				 * holds the resulting instruction. */
    TclJumpType jumpType,	/* Indicates the kind of jump: if true or
				 * false or unconditional. */
    JumpFixup *jumpFixupPtr)	/* Points to the JumpFixup structure to
				 * initialize with information about this
				 * forward jump. */
{
    /*
     * Initialize the JumpFixup structure:
     *    - codeOffset is offset of first byte of jump below
     *    - cmdIndex is index of the command after the current one
     *    - exceptIndex is the index of the first ExceptionRange after the
     *	    current one.
     */

    jumpFixupPtr->jumpType = jumpType;
    jumpFixupPtr->codeOffset = envPtr->codeNext - envPtr->codeStart;
    jumpFixupPtr->cmdIndex = envPtr->numCommands;
    jumpFixupPtr->exceptIndex = envPtr->exceptArrayNext;

    switch (jumpType) {
    case TCL_UNCONDITIONAL_JUMP:
	TclEmitInstInt1(INST_JUMP1, 0, envPtr);
	break;
    case TCL_TRUE_JUMP:
	TclEmitInstInt1(INST_JUMP_TRUE1, 0, envPtr);
	break;
    default:
	TclEmitInstInt1(INST_JUMP_FALSE1, 0, envPtr);
	break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclFixupForwardJump --
 *
 *	Procedure that updates a previously-emitted forward jump to jump a
 *	specified number of bytes, "jumpDist". If necessary, the jump is grown
 *	from two to five bytes; this is done if the jump distance is greater
 *	than "distThreshold" (normally 127 bytes). The jump is described by a
 *	JumpFixup record previously initialized by TclEmitForwardJump.
 *
 * Results:
 *	1 if the jump was grown and subsequent instructions had to be moved;
 *	otherwise 0. This result is returned to allow callers to update any
 *	additional code offsets they may hold.
 *
 * Side effects:
 *	The jump may be grown and subsequent instructions moved. If this
 *	happens, the code offsets for any commands and any ExceptionRange
 *	records between the jump and the current code address will be updated
 *	to reflect the moved code. Also, the bytecode instruction array in the
 *	CompileEnv structure may be grown and reallocated.
 *
 *----------------------------------------------------------------------
 */

int
TclFixupForwardJump(
    CompileEnv *envPtr,		/* Points to the CompileEnv structure that
				 * holds the resulting instruction. */
    JumpFixup *jumpFixupPtr,	/* Points to the JumpFixup structure that
				 * describes the forward jump. */
    int jumpDist,		/* Jump distance to set in jump instr. */
    int distThreshold)		/* Maximum distance before the two byte jump
				 * is grown to five bytes. */
{
    unsigned char *jumpPc, *p;
    int firstCmd, lastCmd, firstRange, lastRange, k;
    unsigned numBytes;

    if (jumpDist <= distThreshold) {
	jumpPc = envPtr->codeStart + jumpFixupPtr->codeOffset;
	switch (jumpFixupPtr->jumpType) {
	case TCL_UNCONDITIONAL_JUMP:
	    TclUpdateInstInt1AtPc(INST_JUMP1, jumpDist, jumpPc);
	    break;
	case TCL_TRUE_JUMP:
	    TclUpdateInstInt1AtPc(INST_JUMP_TRUE1, jumpDist, jumpPc);
	    break;
	default:
	    TclUpdateInstInt1AtPc(INST_JUMP_FALSE1, jumpDist, jumpPc);
	    break;
	}
	return 0;
    }

    /*
     * We must grow the jump then move subsequent instructions down. Note that
     * if we expand the space for generated instructions, code addresses might
     * change; be careful about updating any of these addresses held in
     * variables.
     */

    if ((envPtr->codeNext + 3) > envPtr->codeEnd) {
	TclExpandCodeArray(envPtr);
    }
    jumpPc = envPtr->codeStart + jumpFixupPtr->codeOffset;
    numBytes = envPtr->codeNext-jumpPc-2;
    p = jumpPc+2;
    memmove(p+3, p, numBytes);

    envPtr->codeNext += 3;
    jumpDist += 3;
    switch (jumpFixupPtr->jumpType) {
    case TCL_UNCONDITIONAL_JUMP:
	TclUpdateInstInt4AtPc(INST_JUMP4, jumpDist, jumpPc);
	break;
    case TCL_TRUE_JUMP:
	TclUpdateInstInt4AtPc(INST_JUMP_TRUE4, jumpDist, jumpPc);
	break;
    default:
	TclUpdateInstInt4AtPc(INST_JUMP_FALSE4, jumpDist, jumpPc);
	break;
    }

    /*
     * Adjust the code offsets for any commands and any ExceptionRange records
     * between the jump and the current code address.
     */

    firstCmd = jumpFixupPtr->cmdIndex;
    lastCmd = envPtr->numCommands - 1;
    if (firstCmd < lastCmd) {
	for (k = firstCmd;  k <= lastCmd;  k++) {
	    envPtr->cmdMapPtr[k].codeOffset += 3;
	}
    }

    firstRange = jumpFixupPtr->exceptIndex;
    lastRange = envPtr->exceptArrayNext - 1;
    for (k = firstRange;  k <= lastRange;  k++) {
	ExceptionRange *rangePtr = &envPtr->exceptArrayPtr[k];

	rangePtr->codeOffset += 3;
	switch (rangePtr->type) {
	case LOOP_EXCEPTION_RANGE:
	    rangePtr->breakOffset += 3;
	    if (rangePtr->continueOffset != -1) {
		rangePtr->continueOffset += 3;
	    }
	    break;
	case CATCH_EXCEPTION_RANGE:
	    rangePtr->catchOffset += 3;
	    break;
	default:
	    Tcl_Panic("TclFixupForwardJump: bad ExceptionRange type %d",
		    rangePtr->type);
	}
    }

    for (k = 0 ; k < envPtr->exceptArrayNext ; k++) {
	ExceptionAux *auxPtr = &envPtr->exceptAuxArrayPtr[k];
	int i;

	for (i=0 ; i<auxPtr->numBreakTargets ; i++) {
	    if (jumpFixupPtr->codeOffset < auxPtr->breakTargets[i]) {
		auxPtr->breakTargets[i] += 3;
	    }
	}
	for (i=0 ; i<auxPtr->numContinueTargets ; i++) {
	    if (jumpFixupPtr->codeOffset < auxPtr->continueTargets[i]) {
		auxPtr->continueTargets[i] += 3;
	    }
	}
    }

    return 1;			/* the jump was grown */
}

/*
 *----------------------------------------------------------------------
 *
 * TclEmitInvoke --
 *
 *	Emit one of the invoke-related instructions, wrapping it if necessary
 *	in code that ensures that any break or continue operation passing
 *	through it gets the stack unwinding correct, converting it into an
 *	internal jump if in an appropriate context.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Issues the jump with all correct stack management. May create another
 *	loop exception range; pointers to ExceptionRange and ExceptionAux
 *	structures should not be held across this call.
 *
 *----------------------------------------------------------------------
 */

void
TclEmitInvoke(
    CompileEnv *envPtr,
    int opcode,
    ...)
{
    va_list argList;
    ExceptionRange *rangePtr;
    ExceptionAux *auxBreakPtr, *auxContinuePtr;
    int arg1, arg2, wordCount = 0, expandCount = 0;
    int loopRange = 0, breakRange = 0, continueRange = 0;
    int cleanup, depth = TclGetStackDepth(envPtr);

    /*
     * Parse the arguments.
     */

    va_start(argList, opcode);
    switch (opcode) {
    case INST_INVOKE_STK1:
	wordCount = arg1 = cleanup = va_arg(argList, int);
	arg2 = 0;
	break;
    case INST_INVOKE_STK4:
	wordCount = arg1 = cleanup = va_arg(argList, int);
	arg2 = 0;
	break;
    case INST_INVOKE_REPLACE:
	arg1 = va_arg(argList, int);
	arg2 = va_arg(argList, int);
	wordCount = arg1 + arg2 - 1;
	cleanup = arg1 + 1;
	break;
    default:
	Tcl_Panic("unexpected opcode");
    case INST_EVAL_STK:
	wordCount = cleanup = 1;
	arg1 = arg2 = 0;
	break;
    case INST_RETURN_STK:
	wordCount = cleanup = 2;
	arg1 = arg2 = 0;
	break;
    case INST_INVOKE_EXPANDED:
	wordCount = arg1 = cleanup = va_arg(argList, int);
	arg2 = 0;
	expandCount = 1;
	break;
    }
    va_end(argList);

    /*
     * Determine if we need to handle break and continue exceptions with a
     * special handling exception range (so that we can correctly unwind the
     * stack).
     *
     * These must be done separately; they can be different (especially for
     * calls from inside a [for] increment clause).
     */

    rangePtr = TclGetInnermostExceptionRange(envPtr, TCL_CONTINUE,
	    &auxContinuePtr);
    if (rangePtr == NULL || rangePtr->type != LOOP_EXCEPTION_RANGE) {
	auxContinuePtr = NULL;
    } else if (auxContinuePtr->stackDepth == envPtr->currStackDepth-wordCount
	    && auxContinuePtr->expandTarget == envPtr->expandCount-expandCount) {
	auxContinuePtr = NULL;
    } else {
	continueRange = auxContinuePtr - envPtr->exceptAuxArrayPtr;
    }

    rangePtr = TclGetInnermostExceptionRange(envPtr, TCL_BREAK, &auxBreakPtr);
    if (rangePtr == NULL || rangePtr->type != LOOP_EXCEPTION_RANGE) {
	auxBreakPtr = NULL;
    } else if (auxContinuePtr == NULL
	    && auxBreakPtr->stackDepth == envPtr->currStackDepth-wordCount
	    && auxBreakPtr->expandTarget == envPtr->expandCount-expandCount) {
	auxBreakPtr = NULL;
    } else {
	breakRange = auxBreakPtr - envPtr->exceptAuxArrayPtr;
    }

    if (auxBreakPtr != NULL || auxContinuePtr != NULL) {
	loopRange = TclCreateExceptRange(LOOP_EXCEPTION_RANGE, envPtr);
	ExceptionRangeStarts(envPtr, loopRange);
    }

    /*
     * Issue the invoke itself.
     */

    switch (opcode) {
    case INST_INVOKE_STK1:
	TclEmitInstInt1(INST_INVOKE_STK1, arg1, envPtr);
	break;
    case INST_INVOKE_STK4:
	TclEmitInstInt4(INST_INVOKE_STK4, arg1, envPtr);
	break;
    case INST_INVOKE_EXPANDED:
	TclEmitOpcode(INST_INVOKE_EXPANDED, envPtr);
	envPtr->expandCount--;
	TclAdjustStackDepth(1 - arg1, envPtr);
	break;
    case INST_EVAL_STK:
	TclEmitOpcode(INST_EVAL_STK, envPtr);
	break;
    case INST_RETURN_STK:
	TclEmitOpcode(INST_RETURN_STK, envPtr);
	break;
    case INST_INVOKE_REPLACE:
	TclEmitInstInt4(INST_INVOKE_REPLACE, arg1, envPtr);
	TclEmitInt1(arg2, envPtr);
	TclAdjustStackDepth(-1, envPtr); /* Correction to stack depth calcs */
	break;
    }

    /*
     * If we're generating a special wrapper exception range, we need to
     * finish that up now.
     */

    if (auxBreakPtr != NULL || auxContinuePtr != NULL) {
	int savedStackDepth = envPtr->currStackDepth;
	int savedExpandCount = envPtr->expandCount;
	JumpFixup nonTrapFixup;

	if (auxBreakPtr != NULL) {
	    auxBreakPtr = envPtr->exceptAuxArrayPtr + breakRange;
	}
	if (auxContinuePtr != NULL) {
	    auxContinuePtr = envPtr->exceptAuxArrayPtr + continueRange;
	}

	ExceptionRangeEnds(envPtr, loopRange);
	TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &nonTrapFixup);

	/*
	 * Careful! When generating these stack unwinding sequences, the depth
	 * of stack in the cases where they are taken is not the same as if
	 * the exception is not taken.
	 */

	if (auxBreakPtr != NULL) {
	    TclAdjustStackDepth(-1, envPtr);

	    ExceptionRangeTarget(envPtr, loopRange, breakOffset);
	    TclCleanupStackForBreakContinue(envPtr, auxBreakPtr);
	    TclAddLoopBreakFixup(envPtr, auxBreakPtr);
	    TclAdjustStackDepth(1, envPtr);

	    envPtr->currStackDepth = savedStackDepth;
	    envPtr->expandCount = savedExpandCount;
	}

	if (auxContinuePtr != NULL) {
	    TclAdjustStackDepth(-1, envPtr);

	    ExceptionRangeTarget(envPtr, loopRange, continueOffset);
	    TclCleanupStackForBreakContinue(envPtr, auxContinuePtr);
	    TclAddLoopContinueFixup(envPtr, auxContinuePtr);
	    TclAdjustStackDepth(1, envPtr);

	    envPtr->currStackDepth = savedStackDepth;
	    envPtr->expandCount = savedExpandCount;
	}

	TclFinalizeLoopExceptionRange(envPtr, loopRange);
	TclFixupForwardJumpToHere(envPtr, &nonTrapFixup, 127);
    }
    TclCheckStackDepth(depth+1-cleanup, envPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetInstructionTable --
 *
 *	Returns a pointer to the table describing Tcl bytecode instructions.
 *	This procedure is defined so that clients can access the pointer from
 *	outside the TCL DLLs.
 *
 * Results:
 *	Returns a pointer to the global instruction table, same as the
 *	expression (&tclInstructionTable[0]).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const void * /* == InstructionDesc* == */
TclGetInstructionTable(void)
{
    return &tclInstructionTable[0];
}

/*
 *----------------------------------------------------------------------
 *
 * GetCmdLocEncodingSize --
 *
 *	Computes the total number of bytes needed to encode the command
 *	location information for some compiled code.
 *
 * Results:
 *	The byte count needed to encode the compiled location information.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
GetCmdLocEncodingSize(
    CompileEnv *envPtr)		/* Points to compilation environment structure
				 * containing the CmdLocation structure to
				 * encode. */
{
    register CmdLocation *mapPtr = envPtr->cmdMapPtr;
    int numCmds = envPtr->numCommands;
    int codeDelta, codeLen, srcDelta, srcLen;
    int codeDeltaNext, codeLengthNext, srcDeltaNext, srcLengthNext;
				/* The offsets in their respective byte
				 * sequences where the next encoded offset or
				 * length should go. */
    int prevCodeOffset, prevSrcOffset, i;

    codeDeltaNext = codeLengthNext = srcDeltaNext = srcLengthNext = 0;
    prevCodeOffset = prevSrcOffset = 0;
    for (i = 0;  i < numCmds;  i++) {
	codeDelta = mapPtr[i].codeOffset - prevCodeOffset;
	if (codeDelta < 0) {
	    Tcl_Panic("GetCmdLocEncodingSize: bad code offset");
	} else if (codeDelta <= 127) {
	    codeDeltaNext++;
	} else {
	    codeDeltaNext += 5;	/* 1 byte for 0xFF, 4 for positive delta */
	}
	prevCodeOffset = mapPtr[i].codeOffset;

	codeLen = mapPtr[i].numCodeBytes;
	if (codeLen < 0) {
	    Tcl_Panic("GetCmdLocEncodingSize: bad code length");
	} else if (codeLen <= 127) {
	    codeLengthNext++;
	} else {
	    codeLengthNext += 5;/* 1 byte for 0xFF, 4 for length */
	}

	srcDelta = mapPtr[i].srcOffset - prevSrcOffset;
	if ((-127 <= srcDelta) && (srcDelta <= 127) && (srcDelta != -1)) {
	    srcDeltaNext++;
	} else {
	    srcDeltaNext += 5;	/* 1 byte for 0xFF, 4 for delta */
	}
	prevSrcOffset = mapPtr[i].srcOffset;

	srcLen = mapPtr[i].numSrcBytes;
	if (srcLen < 0) {
	    Tcl_Panic("GetCmdLocEncodingSize: bad source length");
	} else if (srcLen <= 127) {
	    srcLengthNext++;
	} else {
	    srcLengthNext += 5;	/* 1 byte for 0xFF, 4 for length */
	}
    }

    return (codeDeltaNext + codeLengthNext + srcDeltaNext + srcLengthNext);
}

/*
 *----------------------------------------------------------------------
 *
 * EncodeCmdLocMap --
 *
 *	Encode the command location information for some compiled code into a
 *	ByteCode structure. The encoded command location map is stored as
 *	three adjacent byte sequences.
 *
 * Results:
 *	Pointer to the first byte after the encoded command location
 *	information.
 *
 * Side effects:
 *	The encoded information is stored into the block of memory headed by
 *	codePtr. Also records pointers to the start of the four byte sequences
 *	in fields in codePtr's ByteCode header structure.
 *
 *----------------------------------------------------------------------
 */

static unsigned char *
EncodeCmdLocMap(
    CompileEnv *envPtr,		/* Points to compilation environment structure
				 * containing the CmdLocation structure to
				 * encode. */
    ByteCode *codePtr,		/* ByteCode in which to encode envPtr's
				 * command location information. */
    unsigned char *startPtr)	/* Points to the first byte in codePtr's
				 * memory block where the location information
				 * is to be stored. */
{
    register CmdLocation *mapPtr = envPtr->cmdMapPtr;
    int numCmds = envPtr->numCommands;
    register unsigned char *p = startPtr;
    int codeDelta, codeLen, srcDelta, srcLen, prevOffset;
    register int i;

    /*
     * Encode the code offset for each command as a sequence of deltas.
     */

    codePtr->codeDeltaStart = p;
    prevOffset = 0;
    for (i = 0;  i < numCmds;  i++) {
	codeDelta = mapPtr[i].codeOffset - prevOffset;
	if (codeDelta < 0) {
	    Tcl_Panic("EncodeCmdLocMap: bad code offset");
	} else if (codeDelta <= 127) {
	    TclStoreInt1AtPtr(codeDelta, p);
	    p++;
	} else {
	    TclStoreInt1AtPtr(0xFF, p);
	    p++;
	    TclStoreInt4AtPtr(codeDelta, p);
	    p += 4;
	}
	prevOffset = mapPtr[i].codeOffset;
    }

    /*
     * Encode the code length for each command.
     */

    codePtr->codeLengthStart = p;
    for (i = 0;  i < numCmds;  i++) {
	codeLen = mapPtr[i].numCodeBytes;
	if (codeLen < 0) {
	    Tcl_Panic("EncodeCmdLocMap: bad code length");
	} else if (codeLen <= 127) {
	    TclStoreInt1AtPtr(codeLen, p);
	    p++;
	} else {
	    TclStoreInt1AtPtr(0xFF, p);
	    p++;
	    TclStoreInt4AtPtr(codeLen, p);
	    p += 4;
	}
    }

    /*
     * Encode the source offset for each command as a sequence of deltas.
     */

    codePtr->srcDeltaStart = p;
    prevOffset = 0;
    for (i = 0;  i < numCmds;  i++) {
	srcDelta = mapPtr[i].srcOffset - prevOffset;
	if ((-127 <= srcDelta) && (srcDelta <= 127) && (srcDelta != -1)) {
	    TclStoreInt1AtPtr(srcDelta, p);
	    p++;
	} else {
	    TclStoreInt1AtPtr(0xFF, p);
	    p++;
	    TclStoreInt4AtPtr(srcDelta, p);
	    p += 4;
	}
	prevOffset = mapPtr[i].srcOffset;
    }

    /*
     * Encode the source length for each command.
     */

    codePtr->srcLengthStart = p;
    for (i = 0;  i < numCmds;  i++) {
	srcLen = mapPtr[i].numSrcBytes;
	if (srcLen < 0) {
	    Tcl_Panic("EncodeCmdLocMap: bad source length");
	} else if (srcLen <= 127) {
	    TclStoreInt1AtPtr(srcLen, p);
	    p++;
	} else {
	    TclStoreInt1AtPtr(0xFF, p);
	    p++;
	    TclStoreInt4AtPtr(srcLen, p);
	    p += 4;
	}
    }

    return p;
}

#ifdef TCL_COMPILE_STATS
/*
 *----------------------------------------------------------------------
 *
 * RecordByteCodeStats --
 *
 *	Accumulates various compilation-related statistics for each newly
 *	compiled ByteCode. Called by the TclInitByteCodeObj when Tcl is
 *	compiled with the -DTCL_COMPILE_STATS flag
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Accumulates aggregate code-related statistics in the interpreter's
 *	ByteCodeStats structure. Records statistics specific to a ByteCode in
 *	its ByteCode structure.
 *
 *----------------------------------------------------------------------
 */

void
RecordByteCodeStats(
    ByteCode *codePtr)		/* Points to ByteCode structure with info
				 * to add to accumulated statistics. */
{
    Interp *iPtr = (Interp *) *codePtr->interpHandle;
    register ByteCodeStats *statsPtr;

    if (iPtr == NULL) {
	/* Avoid segfaulting in case we're called in a deleted interp */
	return;
    }
    statsPtr = &(iPtr->stats);

    statsPtr->numCompilations++;
    statsPtr->totalSrcBytes += (double) codePtr->numSrcBytes;
    statsPtr->totalByteCodeBytes += (double) codePtr->structureSize;
    statsPtr->currentSrcBytes += (double) codePtr->numSrcBytes;
    statsPtr->currentByteCodeBytes += (double) codePtr->structureSize;

    statsPtr->srcCount[TclLog2(codePtr->numSrcBytes)]++;
    statsPtr->byteCodeCount[TclLog2((int) codePtr->structureSize)]++;

    statsPtr->currentInstBytes += (double) codePtr->numCodeBytes;
    statsPtr->currentLitBytes += (double)
	    codePtr->numLitObjects * sizeof(Tcl_Obj *);
    statsPtr->currentExceptBytes += (double)
	    codePtr->numExceptRanges * sizeof(ExceptionRange);
    statsPtr->currentAuxBytes += (double)
	    codePtr->numAuxDataItems * sizeof(AuxData);
    statsPtr->currentCmdMapBytes += (double) codePtr->numCmdLocBytes;
}
#endif /* TCL_COMPILE_STATS */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * tab-width: 8
 * End:
 */
