/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Instruction opcodes for compiled code */

#define STOP_CODE	0
#define POP_TOP		1
#define ROT_TWO		2
#define ROT_THREE	3
#define DUP_TOP		4

#define UNARY_POSITIVE	10
#define UNARY_NEGATIVE	11
#define UNARY_NOT	12
#define UNARY_CONVERT	13
#define UNARY_CALL	14

#define BINARY_MULTIPLY	20
#define BINARY_DIVIDE	21
#define BINARY_MODULO	22
#define BINARY_ADD	23
#define BINARY_SUBTRACT	24
#define BINARY_SUBSCR	25
#define BINARY_CALL	26

#define SLICE		30
/* Also uses 31-33 */

#define STORE_SLICE	40
/* Also uses 41-43 */

#define DELETE_SLICE	50
/* Also uses 51-53 */

#define STORE_SUBSCR	60
#define DELETE_SUBSCR	61

#define PRINT_EXPR	70
#define PRINT_ITEM	71
#define PRINT_NEWLINE	72

#define BREAK_LOOP	80
#define RAISE_EXCEPTION	81
#define LOAD_LOCALS	82
#define RETURN_VALUE	83
#define REQUIRE_ARGS	84
#define REFUSE_ARGS	85
#define BUILD_FUNCTION	86
#define POP_BLOCK	87
#define END_FINALLY	88
#define BUILD_CLASS	89

#define HAVE_ARGUMENT	90	/* Opcodes from here have an argument: */

#define STORE_NAME	90	/* Index in name list */
#define DELETE_NAME	91	/* "" */
#define UNPACK_TUPLE	92	/* Number of tuple items */
#define UNPACK_LIST	93	/* Number of list items */
/* unused:		94 */
#define STORE_ATTR	95	/* Index in name list */
#define DELETE_ATTR	96	/* "" */

#define LOAD_CONST	100	/* Index in const list */
#define LOAD_NAME	101	/* Index in name list */
#define BUILD_TUPLE	102	/* Number of tuple items */
#define BUILD_LIST	103	/* Number of list items */
#define BUILD_MAP	104	/* Always zero for now */
#define LOAD_ATTR	105	/* Index in name list */
#define COMPARE_OP	106	/* Comparison operator */
#define IMPORT_NAME	107	/* Index in name list */
#define IMPORT_FROM	108	/* Index in name list */

#define JUMP_FORWARD	110	/* Number of bytes to skip */
#define JUMP_IF_FALSE	111	/* "" */
#define JUMP_IF_TRUE	112	/* "" */
#define JUMP_ABSOLUTE	113	/* Target byte offset from beginning of code */
#define FOR_LOOP	114	/* Number of bytes to skip */

#define SETUP_LOOP	120	/* Target address (absolute) */
#define SETUP_EXCEPT	121	/* "" */
#define SETUP_FINALLY	122	/* "" */

#define SET_LINENO	127	/* Current line number */

/* Comparison operator codes (argument to COMPARE_OP) */
enum cmp_op {LT, LE, EQ, NE, GT, GE, IN, NOT_IN, IS, IS_NOT, EXC_MATCH, BAD};

#define HAS_ARG(op) ((op) >= HAVE_ARGUMENT)
