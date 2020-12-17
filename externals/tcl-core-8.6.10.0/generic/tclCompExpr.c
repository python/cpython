/*
 * tclCompExpr.c --
 *
 *	This file contains the code to parse and compile Tcl expressions and
 *	implementations of the Tcl commands corresponding to expression
 *	operators, such as the command ::tcl::mathop::+ .
 *
 * Contributions from Don Porter, NIST, 2006-2007. (not subject to US copyright)
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclCompile.h"		/* CompileEnv */

/*
 * Expression parsing takes place in the routine ParseExpr(). It takes a
 * string as input, parses that string, and generates a representation of the
 * expression in the form of a tree of operators, a list of literals, a list
 * of function names, and an array of Tcl_Token's within a Tcl_Parse struct.
 * The tree is composed of OpNodes.
 */

typedef struct OpNode {
    int left;			/* "Pointer" to the left operand. */
    int right;			/* "Pointer" to the right operand. */
    union {
	int parent;		/* "Pointer" to the parent operand. */
	int prev;		/* "Pointer" joining incomplete tree stack */
    } p;
    unsigned char lexeme;	/* Code that identifies the operator. */
    unsigned char precedence;	/* Precedence of the operator */
    unsigned char mark;		/* Mark used to control traversal. */
    unsigned char constant;	/* Flag marking constant subexpressions. */
} OpNode;

/*
 * The storage for the tree is dynamically allocated array of OpNodes. The
 * array is grown as parsing needs dictate according to a scheme similar to
 * Tcl's string growth algorithm, so that the resizing costs are O(N) and so
 * that we use at least half the memory allocated as expressions get large.
 *
 * Each OpNode in the tree represents an operator in the expression, either
 * unary or binary. When parsing is completed successfully, a binary operator
 * OpNode will have its left and right fields filled with "pointers" to its
 * left and right operands. A unary operator OpNode will have its right field
 * filled with a pointer to its single operand. When an operand is a
 * subexpression the "pointer" takes the form of the index -- a non-negative
 * integer -- into the OpNode storage array where the root of that
 * subexpression parse tree is found.
 *
 * Non-operator elements of the expression do not get stored in the OpNode
 * tree. They are stored in the other structures according to their type.
 * Literal values get appended to the literal list. Elements that denote forms
 * of quoting or substitution known to the Tcl parser get stored as
 * Tcl_Tokens. These non-operator elements of the expression are the leaves of
 * the completed parse tree. When an operand of an OpNode is one of these leaf
 * elements, the following negative integer codes are used to indicate which
 * kind of elements it is.
 */

enum OperandTypes {
    OT_LITERAL = -3,	/* Operand is a literal in the literal list */
    OT_TOKENS = -2,	/* Operand is sequence of Tcl_Tokens */
    OT_EMPTY = -1	/* "Operand" is an empty string. This is a special
			 * case used only to represent the EMPTY lexeme. See
			 * below. */
};

/*
 * Readable macros to test whether a "pointer" value points to an operator.
 * They operate on the "non-negative integer -> operator; negative integer ->
 * a non-operator OperandType" distinction.
 */

#define IsOperator(l)	((l) >= 0)
#define NotOperator(l)	((l) < 0)

/*
 * Note that it is sufficient to store in the tree just the type of leaf
 * operand, without any explicit pointer to which leaf. This is true because
 * the traversals of the completed tree we perform are known to visit the
 * leaves in the same order as the original parse.
 *
 * In a completed parse tree, those OpNodes that are themselves (roots of
 * subexpression trees that are) operands of some operator store in their
 * p.parent field a "pointer" to the OpNode of that operator. The p.parent
 * field permits a traversal of the tree within a non-recursive routine
 * (ConvertTreeToTokens() and CompileExprTree()). This means that even
 * expression trees of great depth pose no risk of blowing the C stack.
 *
 * While the parse tree is being constructed, the same memory space is used to
 * hold the p.prev field which chains together a stack of incomplete trees
 * awaiting their right operands.
 *
 * The lexeme field is filled in with the lexeme of the operator that is
 * returned by the ParseLexeme() routine. Only lexemes for unary and binary
 * operators get stored in an OpNode. Other lexmes get different treatement.
 *
 * The precedence field provides a place to store the precedence of the
 * operator, so it need not be looked up again and again.
 *
 * The mark field is use to control the traversal of the tree, so that it can
 * be done non-recursively. The mark values are:
 */

enum Marks {
    MARK_LEFT,		/* Next step of traversal is to visit left subtree */
    MARK_RIGHT,		/* Next step of traversal is to visit right subtree */
    MARK_PARENT		/* Next step of traversal is to return to parent */
};

/*
 * The constant field is a boolean flag marking which subexpressions are
 * completely known at compile time, and are eligible for computing then
 * rather than waiting until run time.
 */

/*
 * Each lexeme belongs to one of four categories, which determine its place in
 * the parse tree. We use the two high bits of the (unsigned char) value to
 * store a NODE_TYPE code.
 */

#define NODE_TYPE	0xC0

/*
 * The four category values are LEAF, UNARY, and BINARY, explained below, and
 * "uncategorized", which is used either temporarily, until context determines
 * which of the other three categories is correct, or for lexemes like
 * INVALID, which aren't really lexemes at all, but indicators of a parsing
 * error. Note that the codes must be distinct to distinguish categories, but
 * need not take the form of a bit array.
 */

#define BINARY		0x40	/* This lexeme is a binary operator. An OpNode
				 * representing it should go into the parse
				 * tree, and two operands should be parsed for
				 * it in the expression. */
#define UNARY		0x80	/* This lexeme is a unary operator. An OpNode
				 * representing it should go into the parse
				 * tree, and one operand should be parsed for
				 * it in the expression. */
#define LEAF		0xC0	/* This lexeme is a leaf operand in the parse
				 * tree. No OpNode will be placed in the tree
				 * for it. Either a literal value will be
				 * appended to the list of literals in this
				 * expression, or appropriate Tcl_Tokens will
				 * be appended in a Tcl_Parse struct to
				 * represent those leaves that require some
				 * form of substitution. */

/* Uncategorized lexemes */

#define PLUS		1	/* Ambiguous. Resolves to UNARY_PLUS or
				 * BINARY_PLUS according to context. */
#define MINUS		2	/* Ambiguous. Resolves to UNARY_MINUS or
				 * BINARY_MINUS according to context. */
#define BAREWORD	3	/* Ambigous. Resolves to BOOLEAN or to
				 * FUNCTION or a parse error according to
				 * context and value. */
#define INCOMPLETE	4	/* A parse error. Used only when the single
				 * "=" is encountered.  */
#define INVALID		5	/* A parse error. Used when any punctuation
				 * appears that's not a supported operator. */

/* Leaf lexemes */

#define NUMBER		(LEAF | 1)
				/* For literal numbers */
#define SCRIPT		(LEAF | 2)
				/* Script substitution; [foo] */
#define BOOLEAN		(LEAF | BAREWORD)
				/* For literal booleans */
#define BRACED		(LEAF | 4)
				/* Braced string; {foo bar} */
#define VARIABLE	(LEAF | 5)
				/* Variable substitution; $x */
#define QUOTED		(LEAF | 6)
				/* Quoted string; "foo $bar [soom]" */
#define EMPTY		(LEAF | 7)
				/* Used only for an empty argument list to a
				 * function. Represents the empty string
				 * within parens in the expression: rand() */

/* Unary operator lexemes */

#define UNARY_PLUS	(UNARY | PLUS)
#define UNARY_MINUS	(UNARY | MINUS)
#define FUNCTION	(UNARY | BAREWORD)
				/* This is a bit of "creative interpretation"
				 * on the part of the parser. A function call
				 * is parsed into the parse tree according to
				 * the perspective that the function name is a
				 * unary operator and its argument list,
				 * enclosed in parens, is its operand. The
				 * additional requirements not implied
				 * generally by treatment as a unary operator
				 * -- for example, the requirement that the
				 * operand be enclosed in parens -- are hard
				 * coded in the relevant portions of
				 * ParseExpr(). We trade off the need to
				 * include such exceptional handling in the
				 * code against the need we would otherwise
				 * have for more lexeme categories. */
#define START		(UNARY | 4)
				/* This lexeme isn't parsed from the
				 * expression text at all. It represents the
				 * start of the expression and sits at the
				 * root of the parse tree where it serves as
				 * the start/end point of traversals. */
#define OPEN_PAREN	(UNARY | 5)
				/* Another bit of creative interpretation,
				 * where we treat "(" as a unary operator with
				 * the sub-expression between it and its
				 * matching ")" as its operand. See
				 * CLOSE_PAREN below. */
#define NOT		(UNARY | 6)
#define BIT_NOT		(UNARY | 7)

/* Binary operator lexemes */

#define BINARY_PLUS	(BINARY |  PLUS)
#define BINARY_MINUS	(BINARY |  MINUS)
#define COMMA		(BINARY |  3)
				/* The "," operator is a low precedence binary
				 * operator that separates the arguments in a
				 * function call. The additional constraint
				 * that this operator can only legally appear
				 * at the right places within a function call
				 * argument list are hard coded within
				 * ParseExpr().  */
#define MULT		(BINARY |  4)
#define DIVIDE		(BINARY |  5)
#define MOD		(BINARY |  6)
#define LESS		(BINARY |  7)
#define GREATER		(BINARY |  8)
#define BIT_AND		(BINARY |  9)
#define BIT_XOR		(BINARY | 10)
#define BIT_OR		(BINARY | 11)
#define QUESTION	(BINARY | 12)
				/* These two lexemes make up the */
#define COLON		(BINARY | 13)
				/* ternary conditional operator, $x ? $y : $z.
				 * We treat them as two binary operators to
				 * avoid another lexeme category, and code the
				 * additional constraints directly in
				 * ParseExpr(). For instance, the right
				 * operand of a "?" operator must be a ":"
				 * operator. */
#define LEFT_SHIFT	(BINARY | 14)
#define RIGHT_SHIFT	(BINARY | 15)
#define LEQ		(BINARY | 16)
#define GEQ		(BINARY | 17)
#define EQUAL		(BINARY | 18)
#define NEQ		(BINARY | 19)
#define AND		(BINARY | 20)
#define OR		(BINARY | 21)
#define STREQ		(BINARY | 22)
#define STRNEQ		(BINARY | 23)
#define EXPON		(BINARY | 24)
				/* Unlike the other binary operators, EXPON is
				 * right associative and this distinction is
				 * coded directly in ParseExpr(). */
#define IN_LIST		(BINARY | 25)
#define NOT_IN_LIST	(BINARY | 26)
#define CLOSE_PAREN	(BINARY | 27)
				/* By categorizing the CLOSE_PAREN lexeme as a
				 * BINARY operator, the normal parsing rules
				 * for binary operators assure that a close
				 * paren will not directly follow another
				 * operator, and the machinery already in
				 * place to connect operands to operators
				 * according to precedence performs most of
				 * the work of matching open and close parens
				 * for us. In the end though, a close paren is
				 * not really a binary operator, and some
				 * special coding in ParseExpr() make sure we
				 * never put an actual CLOSE_PAREN node in the
				 * parse tree. The sub-expression between
				 * parens becomes the single argument of the
				 * matching OPEN_PAREN unary operator. */
#define END		(BINARY | 28)
				/* This lexeme represents the end of the
				 * string being parsed. Treating it as a
				 * binary operator follows the same logic as
				 * the CLOSE_PAREN lexeme and END pairs with
				 * START, in the same way that CLOSE_PAREN
				 * pairs with OPEN_PAREN. */

/*
 * When ParseExpr() builds the parse tree it must choose which operands to
 * connect to which operators.  This is done according to operator precedence.
 * The greater an operator's precedence the greater claim it has to link to an
 * available operand.  The Precedence enumeration lists the precedence values
 * used by Tcl expression operators, from lowest to highest claim.  Each
 * precedence level is commented with the operators that hold that precedence.
 */

enum Precedence {
    PREC_END = 1,	/* END */
    PREC_START,		/* START */
    PREC_CLOSE_PAREN,	/* ")" */
    PREC_OPEN_PAREN,	/* "(" */
    PREC_COMMA,		/* "," */
    PREC_CONDITIONAL,	/* "?", ":" */
    PREC_OR,		/* "||" */
    PREC_AND,		/* "&&" */
    PREC_BIT_OR,	/* "|" */
    PREC_BIT_XOR,	/* "^" */
    PREC_BIT_AND,	/* "&" */
    PREC_EQUAL,		/* "==", "!=", "eq", "ne", "in", "ni" */
    PREC_COMPARE,	/* "<", ">", "<=", ">=" */
    PREC_SHIFT,		/* "<<", ">>" */
    PREC_ADD,		/* "+", "-" */
    PREC_MULT,		/* "*", "/", "%" */
    PREC_EXPON,		/* "**" */
    PREC_UNARY		/* "+", "-", FUNCTION, "!", "~" */
};

/*
 * Here the same information contained in the comments above is stored in
 * inverted form, so that given a lexeme, one can quickly look up its
 * precedence value.
 */

static const unsigned char prec[] = {
    /* Non-operator lexemes */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,
    /* Binary operator lexemes */
    PREC_ADD,		/* BINARY_PLUS */
    PREC_ADD,		/* BINARY_MINUS */
    PREC_COMMA,		/* COMMA */
    PREC_MULT,		/* MULT */
    PREC_MULT,		/* DIVIDE */
    PREC_MULT,		/* MOD */
    PREC_COMPARE,	/* LESS */
    PREC_COMPARE,	/* GREATER */
    PREC_BIT_AND,	/* BIT_AND */
    PREC_BIT_XOR,	/* BIT_XOR */
    PREC_BIT_OR,	/* BIT_OR */
    PREC_CONDITIONAL,	/* QUESTION */
    PREC_CONDITIONAL,	/* COLON */
    PREC_SHIFT,		/* LEFT_SHIFT */
    PREC_SHIFT,		/* RIGHT_SHIFT */
    PREC_COMPARE,	/* LEQ */
    PREC_COMPARE,	/* GEQ */
    PREC_EQUAL,		/* EQUAL */
    PREC_EQUAL,		/* NEQ */
    PREC_AND,		/* AND */
    PREC_OR,		/* OR */
    PREC_EQUAL,		/* STREQ */
    PREC_EQUAL,		/* STRNEQ */
    PREC_EXPON,		/* EXPON */
    PREC_EQUAL,		/* IN_LIST */
    PREC_EQUAL,		/* NOT_IN_LIST */
    PREC_CLOSE_PAREN,	/* CLOSE_PAREN */
    PREC_END,		/* END */
    /* Expansion room for more binary operators */
    0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,
    /* Unary operator lexemes */
    PREC_UNARY,		/* UNARY_PLUS */
    PREC_UNARY,		/* UNARY_MINUS */
    PREC_UNARY,		/* FUNCTION */
    PREC_START,		/* START */
    PREC_OPEN_PAREN,	/* OPEN_PAREN */
    PREC_UNARY,		/* NOT*/
    PREC_UNARY,		/* BIT_NOT*/
};

/*
 * A table mapping lexemes to bytecode instructions, used by CompileExprTree().
 */

static const unsigned char instruction[] = {
    /* Non-operator lexemes */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,
    /* Binary operator lexemes */
    INST_ADD,		/* BINARY_PLUS */
    INST_SUB,		/* BINARY_MINUS */
    0,			/* COMMA */
    INST_MULT,		/* MULT */
    INST_DIV,		/* DIVIDE */
    INST_MOD,		/* MOD */
    INST_LT,		/* LESS */
    INST_GT,		/* GREATER */
    INST_BITAND,	/* BIT_AND */
    INST_BITXOR,	/* BIT_XOR */
    INST_BITOR,		/* BIT_OR */
    0,			/* QUESTION */
    0,			/* COLON */
    INST_LSHIFT,	/* LEFT_SHIFT */
    INST_RSHIFT,	/* RIGHT_SHIFT */
    INST_LE,		/* LEQ */
    INST_GE,		/* GEQ */
    INST_EQ,		/* EQUAL */
    INST_NEQ,		/* NEQ */
    0,			/* AND */
    0,			/* OR */
    INST_STR_EQ,	/* STREQ */
    INST_STR_NEQ,	/* STRNEQ */
    INST_EXPON,		/* EXPON */
    INST_LIST_IN,	/* IN_LIST */
    INST_LIST_NOT_IN,	/* NOT_IN_LIST */
    0,			/* CLOSE_PAREN */
    0,			/* END */
    /* Expansion room for more binary operators */
    0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,
    /* Unary operator lexemes */
    INST_UPLUS,		/* UNARY_PLUS */
    INST_UMINUS,	/* UNARY_MINUS */
    0,			/* FUNCTION */
    0,			/* START */
    0,			/* OPEN_PAREN */
    INST_LNOT,		/* NOT*/
    INST_BITNOT,	/* BIT_NOT*/
};

/*
 * A table mapping a byte value to the corresponding lexeme for use by
 * ParseLexeme().
 */

static const unsigned char Lexeme[] = {
	INVALID		/* NUL */,	INVALID		/* SOH */,
	INVALID		/* STX */,	INVALID		/* ETX */,
	INVALID		/* EOT */,	INVALID		/* ENQ */,
	INVALID		/* ACK */,	INVALID		/* BEL */,
	INVALID		/* BS */,	INVALID		/* HT */,
	INVALID		/* LF */,	INVALID		/* VT */,
	INVALID		/* FF */,	INVALID		/* CR */,
	INVALID		/* SO */,	INVALID		/* SI */,
	INVALID		/* DLE */,	INVALID		/* DC1 */,
	INVALID		/* DC2 */,	INVALID		/* DC3 */,
	INVALID		/* DC4 */,	INVALID		/* NAK */,
	INVALID		/* SYN */,	INVALID		/* ETB */,
	INVALID		/* CAN */,	INVALID		/* EM */,
	INVALID		/* SUB */,	INVALID		/* ESC */,
	INVALID		/* FS */,	INVALID		/* GS */,
	INVALID		/* RS */,	INVALID		/* US */,
	INVALID		/* SPACE */,	0		/* ! or != */,
	QUOTED		/* " */,	INVALID		/* # */,
	VARIABLE	/* $ */,	MOD		/* % */,
	0		/* & or && */,	INVALID		/* ' */,
	OPEN_PAREN	/* ( */,	CLOSE_PAREN	/* ) */,
	0		/* * or ** */,	PLUS		/* + */,
	COMMA		/* , */,	MINUS		/* - */,
	0		/* . */,	DIVIDE		/* / */,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 0-9 */
	COLON		/* : */,	INVALID		/* ; */,
	0		/* < or << or <= */,
	0		/* == or INVALID */,
	0		/* > or >> or >= */,
	QUESTION	/* ? */,	INVALID		/* @ */,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		/* A-M */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		/* N-Z */
	SCRIPT		/* [ */,	INVALID		/* \ */,
	INVALID		/* ] */,	BIT_XOR		/* ^ */,
	INVALID		/* _ */,	INVALID		/* ` */,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		/* a-m */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		/* n-z */
	BRACED		/* { */,	0		/* | or || */,
	INVALID		/* } */,	BIT_NOT		/* ~ */,
	INVALID		/* DEL */
};

/*
 * The JumpList struct is used to create a stack of data needed for the
 * TclEmitForwardJump() and TclFixupForwardJump() calls that are performed
 * when compiling the short-circuiting operators QUESTION/COLON, AND, and OR.
 * Keeping a stack permits the CompileExprTree() routine to be non-recursive.
 */

typedef struct JumpList {
    JumpFixup jump;		/* Pass this argument to matching calls of
				 * TclEmitForwardJump() and
				 * TclFixupForwardJump(). */
    struct JumpList *next;	/* Point to next item on the stack */
} JumpList;

/*
 * Declarations for local functions to this file:
 */

static void		CompileExprTree(Tcl_Interp *interp, OpNode *nodes,
			    int index, Tcl_Obj *const **litObjvPtr,
			    Tcl_Obj *const *funcObjv, Tcl_Token *tokenPtr,
			    CompileEnv *envPtr, int optimize);
static void		ConvertTreeToTokens(const char *start, int numBytes,
			    OpNode *nodes, Tcl_Token *tokenPtr,
			    Tcl_Parse *parsePtr);
static int		ExecConstantExprTree(Tcl_Interp *interp, OpNode *nodes,
			    int index, Tcl_Obj * const **litObjvPtr);
static int		ParseExpr(Tcl_Interp *interp, const char *start,
			    int numBytes, OpNode **opTreePtr,
			    Tcl_Obj *litList, Tcl_Obj *funcList,
			    Tcl_Parse *parsePtr, int parseOnly);
static int		ParseLexeme(const char *start, int numBytes,
			    unsigned char *lexemePtr, Tcl_Obj **literalPtr);

/*
 *----------------------------------------------------------------------
 *
 * ParseExpr --
 *
 *	Given a string, the numBytes bytes starting at start, this function
 *	parses it as a Tcl expression and constructs a tree representing the
 *	structure of the expression. The caller must pass in empty lists as
 *	the funcList and litList arguments. The elements of the parsed
 *	expression are returned to the caller as that tree, a list of literal
 *	values, a list of function names, and in Tcl_Tokens added to a
 *	Tcl_Parse struct passed in by the caller.
 *
 * Results:
 *	If the string is successfully parsed as a valid Tcl expression, TCL_OK
 *	is returned, and data about the expression structure is written to the
 *	last four arguments. If the string cannot be parsed as a valid Tcl
 *	expression, TCL_ERROR is returned, and if interp is non-NULL, an error
 *	message is written to interp.
 *
 * Side effects:
 *	Memory will be allocated. If TCL_OK is returned, the caller must clean
 *	up the returned data structures. The (OpNode *) value written to
 *	opTreePtr should be passed to ckfree() and the parsePtr argument
 *	should be passed to Tcl_FreeParse(). The elements appended to the
 *	litList and funcList will automatically be freed whenever the refcount
 *	on those lists indicates they can be freed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseExpr(
    Tcl_Interp *interp,		/* Used for error reporting. */
    const char *start,		/* Start of source string to parse. */
    int numBytes,		/* Number of bytes in string. */
    OpNode **opTreePtr,		/* Points to space where a pointer to the
				 * allocated OpNode tree should go. */
    Tcl_Obj *litList,		/* List to append literals to. */
    Tcl_Obj *funcList,		/* List to append function names to. */
    Tcl_Parse *parsePtr,	/* Structure to fill with tokens representing
				 * those operands that require run time
				 * substitutions. */
    int parseOnly)		/* A boolean indicating whether the caller's
				 * aim is just a parse, or whether it will go
				 * on to compile the expression. Different
				 * optimizations are appropriate for the two
				 * scenarios. */
{
    OpNode *nodes = NULL;	/* Pointer to the OpNode storage array where
				 * we build the parse tree. */
    unsigned int nodesAvailable = 64; /* Initial size of the storage array. This
				 * value establishes a minimum tree memory
				 * cost of only about 1 kibyte, and is large
				 * enough for most expressions to parse with
				 * no need for array growth and
				 * reallocation. */
    unsigned int nodesUsed = 0;	/* Number of OpNodes filled. */
    int scanned = 0;		/* Capture number of byte scanned by parsing
				 * routines. */
    int lastParsed;		/* Stores info about what the lexeme parsed
				 * the previous pass through the parsing loop
				 * was. If it was an operator, lastParsed is
				 * the index of the OpNode for that operator.
				 * If it was not an operator, lastParsed holds
				 * an OperandTypes value encoding what we need
				 * to know about it. */
    int incomplete;		/* Index of the most recent incomplete tree in
				 * the OpNode array. Heads a stack of
				 * incomplete trees linked by p.prev. */
    int complete = OT_EMPTY;	/* "Index" of the complete tree (that is, a
				 * complete subexpression) determined at the
				 * moment. OT_EMPTY is a nonsense value used
				 * only to silence compiler warnings. During a
				 * parse, complete will always hold an index
				 * or an OperandTypes value pointing to an
				 * actual leaf at the time the complete tree
				 * is needed. */

    /*
     * These variables control generation of the error message.
     */

    Tcl_Obj *msg = NULL;	/* The error message. */
    Tcl_Obj *post = NULL;	/* In a few cases, an additional postscript
				 * for the error message, supplying more
				 * information after the error msg and
				 * location have been reported. */
    const char *errCode = NULL;	/* The detail word of the errorCode list, or
				 * NULL to indicate that no changes to the
				 * errorCode are to be done. */
    const char *subErrCode = NULL;
				/* Extra information for use in generating the
				 * errorCode. */
    const char *mark = "_@_";	/* In the portion of the complete error
				 * message where the error location is
				 * reported, this "mark" substring is inserted
				 * into the string being parsed to aid in
				 * pinpointing the location of the syntax
				 * error in the expression. */
    int insertMark = 0;		/* A boolean controlling whether the "mark"
				 * should be inserted. */
    const int limit = 25;	/* Portions of the error message are
				 * constructed out of substrings of the
				 * original expression. In order to keep the
				 * error message readable, we impose this
				 * limit on the substring size we extract. */

    TclParseInit(interp, start, numBytes, parsePtr);

    nodes = attemptckalloc(nodesAvailable * sizeof(OpNode));
    if (nodes == NULL) {
	TclNewLiteralStringObj(msg, "not enough memory to parse expression");
	errCode = "NOMEM";
	goto error;
    }

    /*
     * Initialize the parse tree with the special "START" node.
     */

    nodes->lexeme = START;
    nodes->precedence = prec[START];
    nodes->mark = MARK_RIGHT;
    nodes->constant = 1;
    incomplete = lastParsed = nodesUsed;
    nodesUsed++;

    /*
     * Main parsing loop parses one lexeme per iteration. We exit the loop
     * only when there's a syntax error with a "goto error" which takes us to
     * the error handling code following the loop, or when we've successfully
     * completed the parse and we return to the caller.
     */

    while (1) {
	OpNode *nodePtr;	/* Points to the OpNode we may fill this pass
				 * through the loop. */
	unsigned char lexeme;	/* The lexeme we parse this iteration. */
	Tcl_Obj *literal;	/* Filled by the ParseLexeme() call when a
				 * literal is parsed that has a Tcl_Obj rep
				 * worth preserving. */

	/*
	 * Each pass through this loop adds up to one more OpNode. Allocate
	 * space for one if required.
	 */

	if (nodesUsed >= nodesAvailable) {
	    unsigned int size = nodesUsed * 2;
	    OpNode *newPtr = NULL;

	    do {
	      if (size <= UINT_MAX/sizeof(OpNode)) {
		newPtr = attemptckrealloc(nodes, size * sizeof(OpNode));
	      }
	    } while ((newPtr == NULL)
		    && ((size -= (size - nodesUsed) / 2) > nodesUsed));
	    if (newPtr == NULL) {
		TclNewLiteralStringObj(msg,
			"not enough memory to parse expression");
		errCode = "NOMEM";
		goto error;
	    }
	    nodesAvailable = size;
	    nodes = newPtr;
	}
	nodePtr = nodes + nodesUsed;

	/*
	 * Skip white space between lexemes.
	 */

	scanned = TclParseAllWhiteSpace(start, numBytes);
	start += scanned;
	numBytes -= scanned;

	scanned = ParseLexeme(start, numBytes, &lexeme, &literal);

	/*
	 * Use context to categorize the lexemes that are ambiguous.
	 */

	if ((NODE_TYPE & lexeme) == 0) {
	    int b;

	    switch (lexeme) {
	    case INVALID:
		msg = Tcl_ObjPrintf("invalid character \"%.*s\"",
			scanned, start);
		errCode = "BADCHAR";
		goto error;
	    case INCOMPLETE:
		msg = Tcl_ObjPrintf("incomplete operator \"%.*s\"",
			scanned, start);
		errCode = "PARTOP";
		goto error;
	    case BAREWORD:

		/*
		 * Most barewords in an expression are a syntax error. The
		 * exceptions are that when a bareword is followed by an open
		 * paren, it might be a function call, and when the bareword
		 * is a legal literal boolean value, we accept that as well.
		 */

		if (start[scanned+TclParseAllWhiteSpace(
			start+scanned, numBytes-scanned)] == '(') {
		    lexeme = FUNCTION;

		    /*
		     * When we compile the expression we'll need the function
		     * name, and there's no place in the parse tree to store
		     * it, so we keep a separate list of all the function
		     * names we've parsed in the order we found them.
		     */

		    Tcl_ListObjAppendElement(NULL, funcList, literal);
		} else if (Tcl_GetBooleanFromObj(NULL,literal,&b) == TCL_OK) {
		    lexeme = BOOLEAN;
		} else {
		    Tcl_DecrRefCount(literal);
		    msg = Tcl_ObjPrintf("invalid bareword \"%.*s%s\"",
			    (scanned < limit) ? scanned : limit - 3, start,
			    (scanned < limit) ? "" : "...");
		    post = Tcl_ObjPrintf(
			    "should be \"$%.*s%s\" or \"{%.*s%s}\"",
			    (scanned < limit) ? scanned : limit - 3,
			    start, (scanned < limit) ? "" : "...",
			    (scanned < limit) ? scanned : limit - 3,
			    start, (scanned < limit) ? "" : "...");
		    Tcl_AppendPrintfToObj(post, " or \"%.*s%s(...)\" or ...",
			    (scanned < limit) ? scanned : limit - 3,
			    start, (scanned < limit) ? "" : "...");
		    errCode = "BAREWORD";
		    if (start[0] == '0') {
			const char *stop;
			TclParseNumber(NULL, NULL, NULL, start, scanned,
				&stop, TCL_PARSE_NO_WHITESPACE);

			if (isdigit(UCHAR(*stop)) || (stop == start + 1)) {
			    switch (start[1]) {
			    case 'b':
				Tcl_AppendToObj(post,
					" (invalid binary number?)", -1);
				parsePtr->errorType = TCL_PARSE_BAD_NUMBER;
				errCode = "BADNUMBER";
				subErrCode = "BINARY";
				break;
			    case 'o':
				Tcl_AppendToObj(post,
					" (invalid octal number?)", -1);
				parsePtr->errorType = TCL_PARSE_BAD_NUMBER;
				errCode = "BADNUMBER";
				subErrCode = "OCTAL";
				break;
			    default:
				if (isdigit(UCHAR(start[1]))) {
				    Tcl_AppendToObj(post,
					    " (invalid octal number?)", -1);
				    parsePtr->errorType = TCL_PARSE_BAD_NUMBER;
				    errCode = "BADNUMBER";
				    subErrCode = "OCTAL";
				}
				break;
			    }
			}
		    }
		    goto error;
		}
		break;
	    case PLUS:
	    case MINUS:
		if (IsOperator(lastParsed)) {
		    /*
		     * A "+" or "-" coming just after another operator must be
		     * interpreted as a unary operator.
		     */

		    lexeme |= UNARY;
		} else {
		    lexeme |= BINARY;
		}
	    }
	}	/* Uncategorized lexemes */

	/*
	 * Handle lexeme based on its category.
	 */

	switch (NODE_TYPE & lexeme) {
	case LEAF: {
	    /*
	     * Each LEAF results in either a literal getting appended to the
	     * litList, or a sequence of Tcl_Tokens representing a Tcl word
	     * getting appended to the parsePtr->tokens. No OpNode is filled
	     * for this lexeme.
	     */

	    Tcl_Token *tokenPtr;
	    const char *end = start;
	    int wordIndex;
	    int code = TCL_OK;

	    /*
	     * A leaf operand appearing just after something that's not an
	     * operator is a syntax error.
	     */

	    if (NotOperator(lastParsed)) {
		msg = Tcl_ObjPrintf("missing operator at %s", mark);
		errCode = "MISSING";
		scanned = 0;
		insertMark = 1;

		/*
		 * Free any literal to avoid a memleak.
		 */

		if ((lexeme == NUMBER) || (lexeme == BOOLEAN)) {
		    Tcl_DecrRefCount(literal);
		}
		goto error;
	    }

	    switch (lexeme) {
	    case NUMBER:
	    case BOOLEAN:
		/*
		 * TODO: Consider using a dict or hash to collapse all
		 * duplicate literals into a single representative value.
		 * (Like what is done with [split $s {}]).
		 * Pro:	~75% memory saving on expressions like
		 *	{1+1+1+1+1+.....+1} (Convert "pointer + Tcl_Obj" cost
		 *	to "pointer" cost only)
		 * Con:	Cost of the dict store/retrieve on every literal in
		 *	every expression when expressions like the above tend
		 *	to be uncommon.
		 *	The memory savings is temporary; Compiling to bytecode
		 *	will collapse things as literals are registered
		 *	anyway, so the savings applies only to the time
		 *	between parsing and compiling. Possibly important due
		 *	to high-water mark nature of memory allocation.
		 */

		Tcl_ListObjAppendElement(NULL, litList, literal);
		complete = lastParsed = OT_LITERAL;
		start += scanned;
		numBytes -= scanned;
		continue;

	    default:
		break;
	    }

	    /*
	     * Remaining LEAF cases may involve filling Tcl_Tokens, so make
	     * room for at least 2 more tokens.
	     */

	    TclGrowParseTokenArray(parsePtr, 2);
	    wordIndex = parsePtr->numTokens;
	    tokenPtr = parsePtr->tokenPtr + wordIndex;
	    tokenPtr->type = TCL_TOKEN_WORD;
	    tokenPtr->start = start;
	    parsePtr->numTokens++;

	    switch (lexeme) {
	    case QUOTED:
		code = Tcl_ParseQuotedString(NULL, start, numBytes,
			parsePtr, 1, &end);
		scanned = end - start;
		break;

	    case BRACED:
		code = Tcl_ParseBraces(NULL, start, numBytes,
			parsePtr, 1, &end);
		scanned = end - start;
		break;

	    case VARIABLE:
		code = Tcl_ParseVarName(NULL, start, numBytes, parsePtr, 1);

		/*
		 * Handle the quirk that Tcl_ParseVarName reports a successful
		 * parse even when it gets only a "$" with no variable name.
		 */

		tokenPtr = parsePtr->tokenPtr + wordIndex + 1;
		if (code == TCL_OK && tokenPtr->type != TCL_TOKEN_VARIABLE) {
		    TclNewLiteralStringObj(msg, "invalid character \"$\"");
		    errCode = "BADCHAR";
		    goto error;
		}
		scanned = tokenPtr->size;
		break;

	    case SCRIPT: {
		Tcl_Parse *nestedPtr =
			TclStackAlloc(interp, sizeof(Tcl_Parse));

		tokenPtr = parsePtr->tokenPtr + parsePtr->numTokens;
		tokenPtr->type = TCL_TOKEN_COMMAND;
		tokenPtr->start = start;
		tokenPtr->numComponents = 0;

		end = start + numBytes;
		start++;
		while (1) {
		    code = Tcl_ParseCommand(interp, start, end - start, 1,
			    nestedPtr);
		    if (code != TCL_OK) {
			parsePtr->term = nestedPtr->term;
			parsePtr->errorType = nestedPtr->errorType;
			parsePtr->incomplete = nestedPtr->incomplete;
			break;
		    }
		    start = nestedPtr->commandStart + nestedPtr->commandSize;
		    Tcl_FreeParse(nestedPtr);
		    if ((nestedPtr->term < end) && (nestedPtr->term[0] == ']')
			    && !nestedPtr->incomplete) {
			break;
		    }

		    if (start == end) {
			TclNewLiteralStringObj(msg, "missing close-bracket");
			parsePtr->term = tokenPtr->start;
			parsePtr->errorType = TCL_PARSE_MISSING_BRACKET;
			parsePtr->incomplete = 1;
			code = TCL_ERROR;
			errCode = "UNBALANCED";
			break;
		    }
		}
		TclStackFree(interp, nestedPtr);
		end = start;
		start = tokenPtr->start;
		scanned = end - start;
		tokenPtr->size = scanned;
		parsePtr->numTokens++;
		break;
	    }			/* SCRIPT case */
	    }
	    if (code != TCL_OK) {
		/*
		 * Here we handle all the syntax errors generated by the
		 * Tcl_Token generating parsing routines called in the switch
		 * just above. If the value of parsePtr->incomplete is 1, then
		 * the error was an unbalanced '[', '(', '{', or '"' and
		 * parsePtr->term is pointing to that unbalanced character. If
		 * the value of parsePtr->incomplete is 0, then the error is
		 * one of lacking whitespace following a quoted word, for
		 * example: expr {[an error {foo}bar]}, and parsePtr->term
		 * points to where the whitespace is missing. We reset our
		 * values of start and scanned so that when our error message
		 * is constructed, the location of the syntax error is sure to
		 * appear in it, even if the quoted expression is truncated.
		 */

		start = parsePtr->term;
		scanned = parsePtr->incomplete;
		if (parsePtr->incomplete) {
		    errCode = "UNBALANCED";
		}
		goto error;
	    }

	    tokenPtr = parsePtr->tokenPtr + wordIndex;
	    tokenPtr->size = scanned;
	    tokenPtr->numComponents = parsePtr->numTokens - wordIndex - 1;
	    if (!parseOnly && ((lexeme == QUOTED) || (lexeme == BRACED))) {
		/*
		 * When this expression is destined to be compiled, and a
		 * braced or quoted word within an expression is known at
		 * compile time (no runtime substitutions in it), we can store
		 * it as a literal rather than in its tokenized form. This is
		 * an advantage since the compiled bytecode is going to need
		 * the argument in Tcl_Obj form eventually, so it's just as
		 * well to get there now. Another advantage is that with this
		 * conversion, larger constant expressions might be grown and
		 * optimized.
		 *
		 * On the contrary, if the end goal of this parse is to fill a
		 * Tcl_Parse for a caller of Tcl_ParseExpr(), then it's
		 * wasteful to convert to a literal only to convert back again
		 * later.
		 */

		literal = Tcl_NewObj();
		if (TclWordKnownAtCompileTime(tokenPtr, literal)) {
		    Tcl_ListObjAppendElement(NULL, litList, literal);
		    complete = lastParsed = OT_LITERAL;
		    parsePtr->numTokens = wordIndex;
		    break;
		}
		Tcl_DecrRefCount(literal);
	    }
	    complete = lastParsed = OT_TOKENS;
	    break;
	} /* case LEAF */

	case UNARY:

	    /*
	     * A unary operator appearing just after something that's not an
	     * operator is a syntax error -- something trying to be the left
	     * operand of an operator that doesn't take one.
	     */

	    if (NotOperator(lastParsed)) {
		msg = Tcl_ObjPrintf("missing operator at %s", mark);
		scanned = 0;
		insertMark = 1;
		errCode = "MISSING";
		goto error;
	    }

	    /*
	     * Create an OpNode for the unary operator.
	     */

	    nodePtr->lexeme = lexeme;
	    nodePtr->precedence = prec[lexeme];
	    nodePtr->mark = MARK_RIGHT;

	    /*
	     * A FUNCTION cannot be a constant expression, because Tcl allows
	     * functions to return variable results with the same arguments;
	     * for example, rand(). Other unary operators can root a constant
	     * expression, so long as the argument is a constant expression.
	     */

	    nodePtr->constant = (lexeme != FUNCTION);

	    /*
	     * This unary operator is a new incomplete tree, so push it onto
	     * our stack of incomplete trees. Also remember it as the last
	     * lexeme we parsed.
	     */

	    nodePtr->p.prev = incomplete;
	    incomplete = lastParsed = nodesUsed;
	    nodesUsed++;
	    break;

	case BINARY: {
	    OpNode *incompletePtr;
	    unsigned char precedence = prec[lexeme];

	    /*
	     * A binary operator appearing just after another operator is a
	     * syntax error -- one of the two operators is missing an operand.
	     */

	    if (IsOperator(lastParsed)) {
		if ((lexeme == CLOSE_PAREN)
			&& (nodePtr[-1].lexeme == OPEN_PAREN)) {
		    if (nodePtr[-2].lexeme == FUNCTION) {
			/*
			 * Normally, "()" is a syntax error, but as a special
			 * case accept it as an argument list for a function.
			 * Treat this as a special LEAF lexeme, and restart
			 * the parsing loop with zero characters scanned. We
			 * will parse the ")" again the next time through, but
			 * with the OT_EMPTY leaf as the subexpression between
			 * the parens.
			 */

			scanned = 0;
			complete = lastParsed = OT_EMPTY;
			break;
		    }
		    msg = Tcl_ObjPrintf("empty subexpression at %s", mark);
		    scanned = 0;
		    insertMark = 1;
		    errCode = "EMPTY";
		    goto error;
		}

		if (nodePtr[-1].precedence > precedence) {
		    if (nodePtr[-1].lexeme == OPEN_PAREN) {
			TclNewLiteralStringObj(msg, "unbalanced open paren");
			parsePtr->errorType = TCL_PARSE_MISSING_PAREN;
			errCode = "UNBALANCED";
		    } else if (nodePtr[-1].lexeme == COMMA) {
			msg = Tcl_ObjPrintf(
				"missing function argument at %s", mark);
			scanned = 0;
			insertMark = 1;
			errCode = "MISSING";
		    } else if (nodePtr[-1].lexeme == START) {
			TclNewLiteralStringObj(msg, "empty expression");
			errCode = "EMPTY";
		    }
		} else if (lexeme == CLOSE_PAREN) {
		    TclNewLiteralStringObj(msg, "unbalanced close paren");
		    errCode = "UNBALANCED";
		} else if ((lexeme == COMMA)
			&& (nodePtr[-1].lexeme == OPEN_PAREN)
			&& (nodePtr[-2].lexeme == FUNCTION)) {
		    msg = Tcl_ObjPrintf("missing function argument at %s",
			    mark);
		    scanned = 0;
		    insertMark = 1;
		    errCode = "UNBALANCED";
		}
		if (msg == NULL) {
		    msg = Tcl_ObjPrintf("missing operand at %s", mark);
		    scanned = 0;
		    insertMark = 1;
		    errCode = "MISSING";
		}
		goto error;
	    }

	    /*
	     * Here is where the tree comes together. At this point, we have a
	     * stack of incomplete trees corresponding to substrings that are
	     * incomplete expressions, followed by a complete tree
	     * corresponding to a substring that is itself a complete
	     * expression, followed by the binary operator we have just
	     * parsed. The incomplete trees can each be completed by adding a
	     * right operand.
	     *
	     * To illustrate with an example, when we parse the expression
	     * "1+2*3-4" and we reach this point having just parsed the "-"
	     * operator, we have these incomplete trees: START, "1+", and
	     * "2*". Next we have the complete subexpression "3". Last is the
	     * "-" we've just parsed.
	     *
	     * The next step is to join our complete tree to an operator. The
	     * choice is governed by the precedence and associativity of the
	     * competing operators. If we connect it as the right operand of
	     * our most recent incomplete tree, we get a new complete tree,
	     * and we can repeat the process. The while loop following repeats
	     * this until precedence indicates it is time to join the complete
	     * tree as the left operand of the just parsed binary operator.
	     *
	     * Continuing the example, the first pass through the loop will
	     * join "3" to "2*"; the next pass will join "2*3" to "1+". Then
	     * we'll exit the loop and join "1+2*3" to "-". When we return to
	     * parse another lexeme, our stack of incomplete trees is START
	     * and "1+2*3-".
	     */

	    while (1) {
		incompletePtr = nodes + incomplete;

		if (incompletePtr->precedence < precedence) {
		    break;
		}

		if (incompletePtr->precedence == precedence) {
		    /*
		     * Right association rules for exponentiation.
		     */

		    if (lexeme == EXPON) {
			break;
		    }

		    /*
		     * Special association rules for the conditional
		     * operators. The "?" and ":" operators have equal
		     * precedence, but must be linked up in sensible pairs.
		     */

		    if ((incompletePtr->lexeme == QUESTION)
			    && (NotOperator(complete)
			    || (nodes[complete].lexeme != COLON))) {
			break;
		    }
		    if ((incompletePtr->lexeme == COLON)
			    && (lexeme == QUESTION)) {
			break;
		    }
		}

		/*
		 * Some special syntax checks...
		 */

		/* Parens must balance */
		if ((incompletePtr->lexeme == OPEN_PAREN)
			&& (lexeme != CLOSE_PAREN)) {
		    TclNewLiteralStringObj(msg, "unbalanced open paren");
		    parsePtr->errorType = TCL_PARSE_MISSING_PAREN;
		    errCode = "UNBALANCED";
		    goto error;
		}

		/* Right operand of "?" must be ":" */
		if ((incompletePtr->lexeme == QUESTION)
			&& (NotOperator(complete)
			|| (nodes[complete].lexeme != COLON))) {
		    msg = Tcl_ObjPrintf("missing operator \":\" at %s", mark);
		    scanned = 0;
		    insertMark = 1;
		    errCode = "MISSING";
		    goto error;
		}

		/* Operator ":" may only be right operand of "?" */
		if (IsOperator(complete)
			&& (nodes[complete].lexeme == COLON)
			&& (incompletePtr->lexeme != QUESTION)) {
		    TclNewLiteralStringObj(msg,
			    "unexpected operator \":\" "
			    "without preceding \"?\"");
		    errCode = "SURPRISE";
		    goto error;
		}

		/*
		 * Attach complete tree as right operand of most recent
		 * incomplete tree.
		 */

		incompletePtr->right = complete;
		if (IsOperator(complete)) {
		    nodes[complete].p.parent = incomplete;
		    incompletePtr->constant = incompletePtr->constant
			    && nodes[complete].constant;
		} else {
		    incompletePtr->constant = incompletePtr->constant
			    && (complete == OT_LITERAL);
		}

		/*
		 * The QUESTION/COLON and FUNCTION/OPEN_PAREN combinations
		 * each make up a single operator. Force them to agree whether
		 * they have a constant expression.
		 */

		if ((incompletePtr->lexeme == QUESTION)
			|| (incompletePtr->lexeme == FUNCTION)) {
		    nodes[complete].constant = incompletePtr->constant;
		}

		if (incompletePtr->lexeme == START) {
		    /*
		     * Completing the START tree indicates we're done.
		     * Transfer the parse tree to the caller and return.
		     */

		    *opTreePtr = nodes;
		    return TCL_OK;
		}

		/*
		 * With a right operand attached, last incomplete tree has
		 * become the complete tree. Pop it from the incomplete tree
		 * stack.
		 */

		complete = incomplete;
		incomplete = incompletePtr->p.prev;

		/* CLOSE_PAREN can only close one OPEN_PAREN. */
		if (incompletePtr->lexeme == OPEN_PAREN) {
		    break;
		}
	    }

	    /*
	     * More syntax checks...
	     */

	    /* Parens must balance. */
	    if (lexeme == CLOSE_PAREN) {
		if (incompletePtr->lexeme != OPEN_PAREN) {
		    TclNewLiteralStringObj(msg, "unbalanced close paren");
		    errCode = "UNBALANCED";
		    goto error;
		}
	    }

	    /* Commas must appear only in function argument lists. */
	    if (lexeme == COMMA) {
		if  ((incompletePtr->lexeme != OPEN_PAREN)
			|| (incompletePtr[-1].lexeme != FUNCTION)) {
		    TclNewLiteralStringObj(msg,
			    "unexpected \",\" outside function argument list");
		    errCode = "SURPRISE";
		    goto error;
		}
	    }

	    /* Operator ":" may only be right operand of "?" */
	    if (IsOperator(complete) && (nodes[complete].lexeme == COLON)) {
		TclNewLiteralStringObj(msg,
			"unexpected operator \":\" without preceding \"?\"");
		errCode = "SURPRISE";
		goto error;
	    }

	    /*
	     * Create no node for a CLOSE_PAREN lexeme.
	     */

	    if (lexeme == CLOSE_PAREN) {
		break;
	    }

	    /*
	     * Link complete tree as left operand of new node.
	     */

	    nodePtr->lexeme = lexeme;
	    nodePtr->precedence = precedence;
	    nodePtr->mark = MARK_LEFT;
	    nodePtr->left = complete;

	    /*
	     * The COMMA operator cannot be optimized, since the function
	     * needs all of its arguments, and optimization would reduce the
	     * number. Other binary operators root constant expressions when
	     * both arguments are constant expressions.
	     */

	    nodePtr->constant = (lexeme != COMMA);

	    if (IsOperator(complete)) {
		nodes[complete].p.parent = nodesUsed;
		nodePtr->constant = nodePtr->constant
			&& nodes[complete].constant;
	    } else {
		nodePtr->constant = nodePtr->constant
			&& (complete == OT_LITERAL);
	    }

	    /*
	     * With a left operand attached and a right operand missing, the
	     * just-parsed binary operator is root of a new incomplete tree.
	     * Push it onto the stack of incomplete trees.
	     */

	    nodePtr->p.prev = incomplete;
	    incomplete = lastParsed = nodesUsed;
	    nodesUsed++;
	    break;
	}	/* case BINARY */
	}	/* lexeme handler */

	/* Advance past the just-parsed lexeme */
	start += scanned;
	numBytes -= scanned;
    }	/* main parsing loop */

    /*
     * We only get here if there's been an error. Any errors that didn't get a
     * suitable parsePtr->errorType, get recorded as syntax errors.
     */

  error:
    if (parsePtr->errorType == TCL_PARSE_SUCCESS) {
	parsePtr->errorType = TCL_PARSE_SYNTAX;
    }

    /*
     * Free any partial parse tree we've built.
     */

    if (nodes != NULL) {
	ckfree(nodes);
    }

    if (interp == NULL) {
	/*
	 * Nowhere to report an error message, so just free it.
	 */

	if (msg) {
	    Tcl_DecrRefCount(msg);
	}
    } else {
	/*
	 * Construct the complete error message. Start with the simple error
	 * message, pulled from the interp result if necessary...
	 */

	if (msg == NULL) {
	    msg = Tcl_GetObjResult(interp);
	}

	/*
	 * Add a detailed quote from the bad expression, displaying and
	 * sometimes marking the precise location of the syntax error.
	 */

	Tcl_AppendPrintfToObj(msg, "\nin expression \"%s%.*s%.*s%s%s%.*s%s\"",
		((start - limit) < parsePtr->string) ? "" : "...",
		((start - limit) < parsePtr->string)
			? (int) (start - parsePtr->string) : limit - 3,
		((start - limit) < parsePtr->string)
			? parsePtr->string : start - limit + 3,
		(scanned < limit) ? scanned : limit - 3, start,
		(scanned < limit) ? "" : "...", insertMark ? mark : "",
		(start + scanned + limit > parsePtr->end)
			? (int) (parsePtr->end - start) - scanned : limit-3,
		start + scanned,
		(start + scanned + limit > parsePtr->end) ? "" : "...");

	/*
	 * Next, append any postscript message.
	 */

	if (post != NULL) {
	    Tcl_AppendToObj(msg, ";\n", -1);
	    Tcl_AppendObjToObj(msg, post);
	    Tcl_DecrRefCount(post);
	}
	Tcl_SetObjResult(interp, msg);

	/*
	 * Finally, place context information in the errorInfo.
	 */

	numBytes = parsePtr->end - parsePtr->string;
	Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		"\n    (parsing expression \"%.*s%s\")",
		(numBytes < limit) ? numBytes : limit - 3,
		parsePtr->string, (numBytes < limit) ? "" : "..."));
	if (errCode) {
	    Tcl_SetErrorCode(interp, "TCL", "PARSE", "EXPR", errCode,
		    subErrCode, NULL);
	}
    }

    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ConvertTreeToTokens --
 *
 *	Given a string, the numBytes bytes starting at start, and an OpNode
 *	tree and Tcl_Token array created by passing that same string to
 *	ParseExpr(), this function writes into *parsePtr the sequence of
 *	Tcl_Tokens needed so to satisfy the historical interface provided by
 *	Tcl_ParseExpr(). Note that this routine exists only for the sake of
 *	the public Tcl_ParseExpr() routine. It is not used by Tcl itself at
 *	all.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Tcl_Parse *parsePtr is filled with Tcl_Tokens representing the
 *	parsed expression.
 *
 *----------------------------------------------------------------------
 */

static void
ConvertTreeToTokens(
    const char *start,
    int numBytes,
    OpNode *nodes,
    Tcl_Token *tokenPtr,
    Tcl_Parse *parsePtr)
{
    int subExprTokenIdx = 0;
    OpNode *nodePtr = nodes;
    int next = nodePtr->right;

    while (1) {
	Tcl_Token *subExprTokenPtr;
	int scanned, parentIdx;
	unsigned char lexeme;

	/*
	 * Advance the mark so the next exit from this node won't retrace
	 * steps over ground already covered.
	 */

	nodePtr->mark++;

	/*
	 * Handle next child node or leaf.
	 */

	switch (next) {
	case OT_EMPTY:

	    /* No tokens and no characters for the OT_EMPTY leaf. */
	    break;

	case OT_LITERAL:

	    /*
	     * Skip any white space that comes before the literal.
	     */

	    scanned = TclParseAllWhiteSpace(start, numBytes);
	    start += scanned;
	    numBytes -= scanned;

	    /*
	     * Reparse the literal to get pointers into source string.
	     */

	    scanned = ParseLexeme(start, numBytes, &lexeme, NULL);

	    TclGrowParseTokenArray(parsePtr, 2);
	    subExprTokenPtr = parsePtr->tokenPtr + parsePtr->numTokens;
	    subExprTokenPtr->type = TCL_TOKEN_SUB_EXPR;
	    subExprTokenPtr->start = start;
	    subExprTokenPtr->size = scanned;
	    subExprTokenPtr->numComponents = 1;
	    subExprTokenPtr[1].type = TCL_TOKEN_TEXT;
	    subExprTokenPtr[1].start = start;
	    subExprTokenPtr[1].size = scanned;
	    subExprTokenPtr[1].numComponents = 0;

	    parsePtr->numTokens += 2;
	    start += scanned;
	    numBytes -= scanned;
	    break;

	case OT_TOKENS: {
	    /*
	     * tokenPtr points to a token sequence that came from parsing a
	     * Tcl word. A Tcl word is made up of a sequence of one or more
	     * elements. When the word is only a single element, it's been the
	     * historical practice to replace the TCL_TOKEN_WORD token
	     * directly with a TCL_TOKEN_SUB_EXPR token. However, when the
	     * word has multiple elements, a TCL_TOKEN_WORD token is kept as a
	     * grouping device so that TCL_TOKEN_SUB_EXPR always has only one
	     * element. Wise or not, these are the rules the Tcl expr parser
	     * has followed, and for the sake of those few callers of
	     * Tcl_ParseExpr() we do not change them now. Internally, we can
	     * do better.
	     */

	    int toCopy = tokenPtr->numComponents + 1;

	    if (tokenPtr->numComponents == tokenPtr[1].numComponents + 1) {
		/*
		 * Single element word. Copy tokens and convert the leading
		 * token to TCL_TOKEN_SUB_EXPR.
		 */

		TclGrowParseTokenArray(parsePtr, toCopy);
		subExprTokenPtr = parsePtr->tokenPtr + parsePtr->numTokens;
		memcpy(subExprTokenPtr, tokenPtr,
			(size_t) toCopy * sizeof(Tcl_Token));
		subExprTokenPtr->type = TCL_TOKEN_SUB_EXPR;
		parsePtr->numTokens += toCopy;
	    } else {
		/*
		 * Multiple element word. Create a TCL_TOKEN_SUB_EXPR token to
		 * lead, with fields initialized from the leading token, then
		 * copy entire set of word tokens.
		 */

		TclGrowParseTokenArray(parsePtr, toCopy+1);
		subExprTokenPtr = parsePtr->tokenPtr + parsePtr->numTokens;
		*subExprTokenPtr = *tokenPtr;
		subExprTokenPtr->type = TCL_TOKEN_SUB_EXPR;
		subExprTokenPtr->numComponents++;
		subExprTokenPtr++;
		memcpy(subExprTokenPtr, tokenPtr,
			(size_t) toCopy * sizeof(Tcl_Token));
		parsePtr->numTokens += toCopy + 1;
	    }

	    scanned = tokenPtr->start + tokenPtr->size - start;
	    start += scanned;
	    numBytes -= scanned;
	    tokenPtr += toCopy;
	    break;
	}

	default:

	    /*
	     * Advance to the child node, which is an operator.
	     */

	    nodePtr = nodes + next;

	    /*
	     * Skip any white space that comes before the subexpression.
	     */

	    scanned = TclParseAllWhiteSpace(start, numBytes);
	    start += scanned;
	    numBytes -= scanned;

	    /*
	     * Generate tokens for the operator / subexpression...
	     */

	    switch (nodePtr->lexeme) {
	    case OPEN_PAREN:
	    case COMMA:
	    case COLON:

		/*
		 * Historical practice has been to have no Tcl_Tokens for
		 * these operators.
		 */

		break;

	    default: {

		/*
		 * Remember the index of the last subexpression we were
		 * working on -- that of our parent. We'll stack it later.
		 */

		parentIdx = subExprTokenIdx;

		/*
		 * Verify space for the two leading Tcl_Tokens representing
		 * the subexpression rooted by this operator. The first
		 * Tcl_Token will be of type TCL_TOKEN_SUB_EXPR; the second of
		 * type TCL_TOKEN_OPERATOR.
		 */

		TclGrowParseTokenArray(parsePtr, 2);
		subExprTokenIdx = parsePtr->numTokens;
		subExprTokenPtr = parsePtr->tokenPtr + subExprTokenIdx;
		parsePtr->numTokens += 2;
		subExprTokenPtr->type = TCL_TOKEN_SUB_EXPR;
		subExprTokenPtr[1].type = TCL_TOKEN_OPERATOR;

		/*
		 * Our current position scanning the string is the starting
		 * point for this subexpression.
		 */

		subExprTokenPtr->start = start;

		/*
		 * Eventually, we know that the numComponents field of the
		 * Tcl_Token of type TCL_TOKEN_OPERATOR will be 0. This means
		 * we can make other use of this field for now to track the
		 * stack of subexpressions we have pending.
		 */

		subExprTokenPtr[1].numComponents = parentIdx;
		break;
	    }
	    }
	    break;
	}

	/* Determine which way to exit the node on this pass. */
    router:
	switch (nodePtr->mark) {
	case MARK_LEFT:
	    next = nodePtr->left;
	    break;

	case MARK_RIGHT:
	    next = nodePtr->right;

	    /*
	     * Skip any white space that comes before the operator.
	     */

	    scanned = TclParseAllWhiteSpace(start, numBytes);
	    start += scanned;
	    numBytes -= scanned;

	    /*
	     * Here we scan from the string the operator corresponding to
	     * nodePtr->lexeme.
	     */

	    scanned = ParseLexeme(start, numBytes, &lexeme, NULL);

	    switch(nodePtr->lexeme) {
	    case OPEN_PAREN:
	    case COMMA:
	    case COLON:

		/*
		 * No tokens for these lexemes -> nothing to do.
		 */

		break;

	    default:

		/*
		 * Record in the TCL_TOKEN_OPERATOR token the pointers into
		 * the string marking where the operator is.
		 */

		subExprTokenPtr = parsePtr->tokenPtr + subExprTokenIdx;
		subExprTokenPtr[1].start = start;
		subExprTokenPtr[1].size = scanned;
		break;
	    }

	    start += scanned;
	    numBytes -= scanned;
	    break;

	case MARK_PARENT:
	    switch (nodePtr->lexeme) {
	    case START:

		/* When we get back to the START node, we're done. */
		return;

	    case COMMA:
	    case COLON:

		/* No tokens for these lexemes -> nothing to do. */
		break;

	    case OPEN_PAREN:

		/*
		 * Skip past matching close paren.
		 */

		scanned = TclParseAllWhiteSpace(start, numBytes);
		start += scanned;
		numBytes -= scanned;
		scanned = ParseLexeme(start, numBytes, &lexeme, NULL);
		start += scanned;
		numBytes -= scanned;
		break;

	    default:

		/*
		 * Before we leave this node/operator/subexpression for the
		 * last time, finish up its tokens....
		 *
		 * Our current position scanning the string is where the
		 * substring for the subexpression ends.
		 */

		subExprTokenPtr = parsePtr->tokenPtr + subExprTokenIdx;
		subExprTokenPtr->size = start - subExprTokenPtr->start;

		/*
		 * All the Tcl_Tokens allocated and filled belong to
		 * this subexpresion. The first token is the leading
		 * TCL_TOKEN_SUB_EXPR token, and all the rest (one fewer)
		 * are its components.
		 */

		subExprTokenPtr->numComponents =
			(parsePtr->numTokens - subExprTokenIdx) - 1;

		/*
		 * Finally, as we return up the tree to our parent, pop the
		 * parent subexpression off our subexpression stack, and
		 * fill in the zero numComponents for the operator Tcl_Token.
		 */

		parentIdx = subExprTokenPtr[1].numComponents;
		subExprTokenPtr[1].numComponents = 0;
		subExprTokenIdx = parentIdx;
		break;
	    }

	    /*
	     * Since we're returning to parent, skip child handling code.
	     */

	    nodePtr = nodes + nodePtr->p.parent;
	    goto router;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseExpr --
 *
 *	Given a string, the numBytes bytes starting at start, this function
 *	parses it as a Tcl expression and stores information about the
 *	structure of the expression in the Tcl_Parse struct indicated by the
 *	caller.
 *
 * Results:
 *	If the string is successfully parsed as a valid Tcl expression, TCL_OK
 *	is returned, and data about the expression structure is written to
 *	*parsePtr. If the string cannot be parsed as a valid Tcl expression,
 *	TCL_ERROR is returned, and if interp is non-NULL, an error message is
 *	written to interp.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the information
 *	about the expression, then additional space is malloc-ed. If the
 *	function returns TCL_OK then the caller must eventually invoke
 *	Tcl_FreeParse to release any additional space that was allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseExpr(
    Tcl_Interp *interp,		/* Used for error reporting. */
    const char *start,		/* Start of source string to parse. */
    int numBytes,		/* Number of bytes in string. If < 0, the
				 * string consists of all bytes up to the
				 * first null character. */
    Tcl_Parse *parsePtr)	/* Structure to fill with information about
				 * the parsed expression; any previous
				 * information in the structure is ignored. */
{
    int code;
    OpNode *opTree = NULL;	/* Will point to the tree of operators. */
    Tcl_Obj *litList = Tcl_NewObj();	/* List to hold the literals. */
    Tcl_Obj *funcList = Tcl_NewObj();	/* List to hold the functon names. */
    Tcl_Parse *exprParsePtr = TclStackAlloc(interp, sizeof(Tcl_Parse));
				/* Holds the Tcl_Tokens of substitutions. */

    if (numBytes < 0) {
	numBytes = (start ? strlen(start) : 0);
    }

    code = ParseExpr(interp, start, numBytes, &opTree, litList, funcList,
	    exprParsePtr, 1 /* parseOnly */);
    Tcl_DecrRefCount(funcList);
    Tcl_DecrRefCount(litList);

    TclParseInit(interp, start, numBytes, parsePtr);
    if (code == TCL_OK) {
	ConvertTreeToTokens(start, numBytes,
		opTree, exprParsePtr->tokenPtr, parsePtr);
    } else {
	parsePtr->term = exprParsePtr->term;
	parsePtr->errorType = exprParsePtr->errorType;
    }

    Tcl_FreeParse(exprParsePtr);
    TclStackFree(interp, exprParsePtr);
    ckfree(opTree);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseLexeme --
 *
 *	Parse a single lexeme from the start of a string, scanning no more
 *	than numBytes bytes.
 *
 * Results:
 *	Returns the number of bytes scanned to produce the lexeme.
 *
 * Side effects:
 *	Code identifying lexeme parsed is writen to *lexemePtr.
 *
 *----------------------------------------------------------------------
 */

static int
ParseLexeme(
    const char *start,		/* Start of lexeme to parse. */
    int numBytes,		/* Number of bytes in string. */
    unsigned char *lexemePtr,	/* Write code of parsed lexeme to this
				 * storage. */
    Tcl_Obj **literalPtr)	/* Write corresponding literal value to this
				   storage, if non-NULL. */
{
    const char *end;
    int scanned;
    Tcl_UniChar ch = 0;
    Tcl_Obj *literal = NULL;
    unsigned char byte;

    if (numBytes == 0) {
	*lexemePtr = END;
	return 0;
    }
    byte = UCHAR(*start);
    if (byte < sizeof(Lexeme) && Lexeme[byte] != 0) {
	*lexemePtr = Lexeme[byte];
	return 1;
    }
    switch (byte) {
    case '*':
	if ((numBytes > 1) && (start[1] == '*')) {
	    *lexemePtr = EXPON;
	    return 2;
	}
	*lexemePtr = MULT;
	return 1;

    case '=':
	if ((numBytes > 1) && (start[1] == '=')) {
	    *lexemePtr = EQUAL;
	    return 2;
	}
	*lexemePtr = INCOMPLETE;
	return 1;

    case '!':
	if ((numBytes > 1) && (start[1] == '=')) {
	    *lexemePtr = NEQ;
	    return 2;
	}
	*lexemePtr = NOT;
	return 1;

    case '&':
	if ((numBytes > 1) && (start[1] == '&')) {
	    *lexemePtr = AND;
	    return 2;
	}
	*lexemePtr = BIT_AND;
	return 1;

    case '|':
	if ((numBytes > 1) && (start[1] == '|')) {
	    *lexemePtr = OR;
	    return 2;
	}
	*lexemePtr = BIT_OR;
	return 1;

    case '<':
	if (numBytes > 1) {
	    switch (start[1]) {
	    case '<':
		*lexemePtr = LEFT_SHIFT;
		return 2;
	    case '=':
		*lexemePtr = LEQ;
		return 2;
	    }
	}
	*lexemePtr = LESS;
	return 1;

    case '>':
	if (numBytes > 1) {
	    switch (start[1]) {
	    case '>':
		*lexemePtr = RIGHT_SHIFT;
		return 2;
	    case '=':
		*lexemePtr = GEQ;
		return 2;
	    }
	}
	*lexemePtr = GREATER;
	return 1;

    case 'i':
	if ((numBytes > 1) && (start[1] == 'n')
		&& ((numBytes == 2) || start[2] & 0x80 || !isalpha(UCHAR(start[2])))) {
	    /*
	     * Must make this check so we can tell the difference between the
	     * "in" operator and the "int" function name and the "infinity"
	     * numeric value.
	     */

	    *lexemePtr = IN_LIST;
	    return 2;
	}
	break;

    case 'e':
	if ((numBytes > 1) && (start[1] == 'q')
		&& ((numBytes == 2) || start[2] & 0x80 || !isalpha(UCHAR(start[2])))) {
	    *lexemePtr = STREQ;
	    return 2;
	}
	break;

    case 'n':
	if ((numBytes > 1)
		&& ((numBytes == 2) || start[2] & 0x80 || !isalpha(UCHAR(start[2])))) {
	    switch (start[1]) {
	    case 'e':
		*lexemePtr = STRNEQ;
		return 2;
	    case 'i':
		*lexemePtr = NOT_IN_LIST;
		return 2;
	    }
	}
    }

    literal = Tcl_NewObj();
    if (TclParseNumber(NULL, literal, NULL, start, numBytes, &end,
	    TCL_PARSE_NO_WHITESPACE) == TCL_OK) {
	if (end < start + numBytes && !TclIsBareword(*end)) {

	number:
	    TclInitStringRep(literal, start, end-start);
	    *lexemePtr = NUMBER;
	    if (literalPtr) {
		*literalPtr = literal;
	    } else {
		Tcl_DecrRefCount(literal);
	    }
	    return (end-start);
	} else {
	    unsigned char lexeme;

	    /*
	     * We have a number followed directly by bareword characters
	     * (alpha, digit, underscore).  Is this a number followed by
	     * bareword syntax error?  Or should we join into one bareword?
	     * Example: Inf + luence + () becomes a valid function call.
	     * [Bug 3401704]
	     */
	    if (literal->typePtr == &tclDoubleType) {
		const char *p = start;

		while (p < end) {
		    if (!TclIsBareword(*p++)) {
			/*
			 * The number has non-bareword characters, so we
			 * must treat it as a number.
			 */
			goto number;
		    }
		}
	    }
	    ParseLexeme(end, numBytes-(end-start), &lexeme, NULL);
	    if ((NODE_TYPE & lexeme) == BINARY) {
		/*
		 * The bareword characters following the number take the
		 * form of an operator (eq, ne, in, ni, ...) so we treat
		 * as number + operator.
		 */
		goto number;
	    }

	    /*
	     * Otherwise, fall through and parse the whole as a bareword.
	     */
	}
    }

    /*
     * We reject leading underscores in bareword.  No sensible reason why.
     * Might be inspired by reserved identifier rules in C, which of course
     * have no direct relevance here.
     */

    if (!TclIsBareword(*start) || *start == '_') {
	if (Tcl_UtfCharComplete(start, numBytes)) {
	    scanned = TclUtfToUniChar(start, &ch);
	} else {
	    char utfBytes[TCL_UTF_MAX];

	    memcpy(utfBytes, start, (size_t) numBytes);
	    utfBytes[numBytes] = '\0';
	    scanned = TclUtfToUniChar(utfBytes, &ch);
	}
	*lexemePtr = INVALID;
	Tcl_DecrRefCount(literal);
	return scanned;
    }
    end = start;
    while (numBytes && TclIsBareword(*end)) {
	end += 1;
	numBytes -= 1;
    }
    *lexemePtr = BAREWORD;
    if (literalPtr) {
	Tcl_SetStringObj(literal, start, (int) (end-start));
	*literalPtr = literal;
    } else {
	Tcl_DecrRefCount(literal);
    }
    return (end-start);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileExpr --
 *
 *	This procedure compiles a string containing a Tcl expression into Tcl
 *	bytecodes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds instructions to envPtr to evaluate the expression at runtime.
 *
 *----------------------------------------------------------------------
 */

void
TclCompileExpr(
    Tcl_Interp *interp,		/* Used for error reporting. */
    const char *script,		/* The source script to compile. */
    int numBytes,		/* Number of bytes in script. */
    CompileEnv *envPtr,		/* Holds resulting instructions. */
    int optimize)		/* 0 for one-off expressions. */
{
    OpNode *opTree = NULL;	/* Will point to the tree of operators */
    Tcl_Obj *litList = Tcl_NewObj();	/* List to hold the literals */
    Tcl_Obj *funcList = Tcl_NewObj();	/* List to hold the functon names*/
    Tcl_Parse *parsePtr = TclStackAlloc(interp, sizeof(Tcl_Parse));
				/* Holds the Tcl_Tokens of substitutions */

    int code = ParseExpr(interp, script, numBytes, &opTree, litList,
	    funcList, parsePtr, 0 /* parseOnly */);

    if (code == TCL_OK) {
	/*
	 * Valid parse; compile the tree.
	 */

	int objc;
	Tcl_Obj *const *litObjv;
	Tcl_Obj **funcObjv;

	/* TIP #280 : Track Lines within the expression */
	TclAdvanceLines(&envPtr->line, script,
		script + TclParseAllWhiteSpace(script, numBytes));

	TclListObjGetElements(NULL, litList, &objc, (Tcl_Obj ***)&litObjv);
	TclListObjGetElements(NULL, funcList, &objc, &funcObjv);
	CompileExprTree(interp, opTree, 0, &litObjv, funcObjv,
		parsePtr->tokenPtr, envPtr, optimize);
    } else {
	TclCompileSyntaxError(interp, envPtr);
    }

    Tcl_FreeParse(parsePtr);
    TclStackFree(interp, parsePtr);
    Tcl_DecrRefCount(funcList);
    Tcl_DecrRefCount(litList);
    ckfree(opTree);
}

/*
 *----------------------------------------------------------------------
 *
 * ExecConstantExprTree --
 *	Compiles and executes bytecode for the subexpression tree at index
 *	in the nodes array.  This subexpression must be constant, made up
 *	of only constant operators (not functions) and literals.
 *
 * Results:
 *	A standard Tcl return code and result left in interp.
 *
 * Side effects:
 *	Consumes subtree of nodes rooted at index.  Advances the pointer
 *	*litObjvPtr.
 *
 *----------------------------------------------------------------------
 */

static int
ExecConstantExprTree(
    Tcl_Interp *interp,
    OpNode *nodes,
    int index,
    Tcl_Obj *const **litObjvPtr)
{
    CompileEnv *envPtr;
    ByteCode *byteCodePtr;
    int code;
    Tcl_Obj *byteCodeObj = Tcl_NewObj();
    NRE_callback *rootPtr = TOP_CB(interp);

    /*
     * Note we are compiling an expression with literal arguments. This means
     * there can be no [info frame] calls when we execute the resulting
     * bytecode, so there's no need to tend to TIP 280 issues.
     */

    envPtr = TclStackAlloc(interp, sizeof(CompileEnv));
    TclInitCompileEnv(interp, envPtr, NULL, 0, NULL, 0);
    CompileExprTree(interp, nodes, index, litObjvPtr, NULL, NULL, envPtr,
	    0 /* optimize */);
    TclEmitOpcode(INST_DONE, envPtr);
    Tcl_IncrRefCount(byteCodeObj);
    TclInitByteCodeObj(byteCodeObj, envPtr);
    TclFreeCompileEnv(envPtr);
    TclStackFree(interp, envPtr);
    byteCodePtr = byteCodeObj->internalRep.twoPtrValue.ptr1;
    TclNRExecuteByteCode(interp, byteCodePtr);
    code = TclNRRunCallbacks(interp, TCL_OK, rootPtr);
    Tcl_DecrRefCount(byteCodeObj);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * CompileExprTree --
 *
 *	Compiles and writes to envPtr instructions for the subexpression tree
 *	at index in the nodes array. (*litObjvPtr) must point to the proper
 *	location in a corresponding literals list. Likewise, when non-NULL,
 *	funcObjv and tokenPtr must point into matching arrays of function
 *	names and Tcl_Token's derived from earlier call to ParseExpr(). When
 *	optimize is true, any constant subexpressions will be precomputed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds instructions to envPtr to evaluate the expression at runtime.
 *	Consumes subtree of nodes rooted at index. Advances the pointer
 *	*litObjvPtr.
 *
 *----------------------------------------------------------------------
 */

static void
CompileExprTree(
    Tcl_Interp *interp,
    OpNode *nodes,
    int index,
    Tcl_Obj *const **litObjvPtr,
    Tcl_Obj *const *funcObjv,
    Tcl_Token *tokenPtr,
    CompileEnv *envPtr,
    int optimize)
{
    OpNode *nodePtr = nodes + index;
    OpNode *rootPtr = nodePtr;
    int numWords = 0;
    JumpList *jumpPtr = NULL;
    int convert = 1;

    while (1) {
	int next;
	JumpList *freePtr, *newJump;

	if (nodePtr->mark == MARK_LEFT) {
	    next = nodePtr->left;

	    if (nodePtr->lexeme == QUESTION) {
		convert = 1;
	    }
	} else if (nodePtr->mark == MARK_RIGHT) {
	    next = nodePtr->right;

	    switch (nodePtr->lexeme) {
	    case FUNCTION: {
		Tcl_DString cmdName;
		const char *p;
		int length;

		Tcl_DStringInit(&cmdName);
		TclDStringAppendLiteral(&cmdName, "tcl::mathfunc::");
		p = TclGetStringFromObj(*funcObjv, &length);
		funcObjv++;
		Tcl_DStringAppend(&cmdName, p, length);
		TclEmitPush(TclRegisterNewCmdLiteral(envPtr,
			Tcl_DStringValue(&cmdName),
			Tcl_DStringLength(&cmdName)), envPtr);
		Tcl_DStringFree(&cmdName);

		/*
		 * Start a count of the number of words in this function
		 * command invocation. In case there's already a count in
		 * progress (nested functions), save it in our unused "left"
		 * field for restoring later.
		 */

		nodePtr->left = numWords;
		numWords = 2;	/* Command plus one argument */
		break;
	    }
	    case QUESTION:
		newJump = TclStackAlloc(interp, sizeof(JumpList));
		newJump->next = jumpPtr;
		jumpPtr = newJump;
		TclEmitForwardJump(envPtr, TCL_FALSE_JUMP, &jumpPtr->jump);
		break;
	    case COLON:
		newJump = TclStackAlloc(interp, sizeof(JumpList));
		newJump->next = jumpPtr;
		jumpPtr = newJump;
		TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP,
			&jumpPtr->jump);
		TclAdjustStackDepth(-1, envPtr);
		if (convert) {
		    jumpPtr->jump.jumpType = TCL_TRUE_JUMP;
		}
		convert = 1;
		break;
	    case AND:
	    case OR:
		newJump = TclStackAlloc(interp, sizeof(JumpList));
		newJump->next = jumpPtr;
		jumpPtr = newJump;
		TclEmitForwardJump(envPtr, (nodePtr->lexeme == AND)
			?  TCL_FALSE_JUMP : TCL_TRUE_JUMP, &jumpPtr->jump);
		break;
	    }
	} else {
	    int pc1, pc2, target;

	    switch (nodePtr->lexeme) {
	    case START:
	    case QUESTION:
		if (convert && (nodePtr == rootPtr)) {
		    TclEmitOpcode(INST_TRY_CVT_TO_NUMERIC, envPtr);
		}
		break;
	    case OPEN_PAREN:

		/* do nothing */
		break;
	    case FUNCTION:
		/*
		 * Use the numWords count we've kept to invoke the function
		 * command with the correct number of arguments.
		 */

		if (numWords < 255) {
		    TclEmitInvoke(envPtr, INST_INVOKE_STK1, numWords);
		} else {
		    TclEmitInvoke(envPtr, INST_INVOKE_STK4, numWords);
		}

		/*
		 * Restore any saved numWords value.
		 */

		numWords = nodePtr->left;
		convert = 1;
		break;
	    case COMMA:
		/*
		 * Each comma implies another function argument.
		 */

		numWords++;
		break;
	    case COLON:
		CLANG_ASSERT(jumpPtr);
		if (jumpPtr->jump.jumpType == TCL_TRUE_JUMP) {
		    jumpPtr->jump.jumpType = TCL_UNCONDITIONAL_JUMP;
		    convert = 1;
		}
		target = jumpPtr->jump.codeOffset + 2;
		if (TclFixupForwardJumpToHere(envPtr, &jumpPtr->jump, 127)) {
		    target += 3;
		}
		freePtr = jumpPtr;
		jumpPtr = jumpPtr->next;
		TclStackFree(interp, freePtr);
		TclFixupForwardJump(envPtr, &jumpPtr->jump,
			target - jumpPtr->jump.codeOffset, 127);

		freePtr = jumpPtr;
		jumpPtr = jumpPtr->next;
		TclStackFree(interp, freePtr);
		break;
	    case AND:
	    case OR:
		CLANG_ASSERT(jumpPtr);
		pc1 = CurrentOffset(envPtr);
		TclEmitInstInt1((nodePtr->lexeme == AND) ? INST_JUMP_FALSE1
			: INST_JUMP_TRUE1, 0, envPtr);
		TclEmitPush(TclRegisterNewLiteral(envPtr,
			(nodePtr->lexeme == AND) ? "1" : "0", 1), envPtr);
		pc2 = CurrentOffset(envPtr);
		TclEmitInstInt1(INST_JUMP1, 0, envPtr);
		TclAdjustStackDepth(-1, envPtr);
		TclStoreInt1AtPtr(CurrentOffset(envPtr) - pc1,
			envPtr->codeStart + pc1 + 1);
		if (TclFixupForwardJumpToHere(envPtr, &jumpPtr->jump, 127)) {
		    pc2 += 3;
		}
		TclEmitPush(TclRegisterNewLiteral(envPtr,
			(nodePtr->lexeme == AND) ? "0" : "1", 1), envPtr);
		TclStoreInt1AtPtr(CurrentOffset(envPtr) - pc2,
			envPtr->codeStart + pc2 + 1);
		convert = 0;
		freePtr = jumpPtr;
		jumpPtr = jumpPtr->next;
		TclStackFree(interp, freePtr);
		break;
	    default:
		TclEmitOpcode(instruction[nodePtr->lexeme], envPtr);
		convert = 0;
		break;
	    }
	    if (nodePtr == rootPtr) {
		/* We're done */

		return;
	    }
	    nodePtr = nodes + nodePtr->p.parent;
	    continue;
	}

	nodePtr->mark++;
	switch (next) {
	case OT_EMPTY:
	    numWords = 1;	/* No arguments, so just the command */
	    break;
	case OT_LITERAL: {
	    Tcl_Obj *const *litObjv = *litObjvPtr;
	    Tcl_Obj *literal = *litObjv;

	    if (optimize) {
		int length;
		const char *bytes = TclGetStringFromObj(literal, &length);
		int index = TclRegisterNewLiteral(envPtr, bytes, length);
		Tcl_Obj *objPtr = TclFetchLiteral(envPtr, index);

		if ((objPtr->typePtr == NULL) && (literal->typePtr != NULL)) {
		    /*
		     * Would like to do this:
		     *
		     * lePtr->objPtr = literal;
		     * Tcl_IncrRefCount(literal);
		     * Tcl_DecrRefCount(objPtr);
		     *
		     * However, the design of the "global" and "local"
		     * LiteralTable does not permit the value of lePtr->objPtr
		     * to change. So rather than replace lePtr->objPtr, we do
		     * surgery to transfer our desired intrep into it.
		     */

		    objPtr->typePtr = literal->typePtr;
		    objPtr->internalRep = literal->internalRep;
		    literal->typePtr = NULL;
		}
		TclEmitPush(index, envPtr);
	    } else {
		/*
		 * When optimize==0, we know the expression is a one-off and
		 * there's nothing to be gained from sharing literals when
		 * they won't live long, and the copies we have already have
		 * an appropriate intrep. In this case, skip literal
		 * registration that would enable sharing, and use the routine
		 * that preserves intreps.
		 */

		TclEmitPush(TclAddLiteralObj(envPtr, literal, NULL), envPtr);
	    }
	    (*litObjvPtr)++;
	    break;
	}
	case OT_TOKENS:
	    CompileTokens(envPtr, tokenPtr, interp);
	    tokenPtr += tokenPtr->numComponents + 1;
	    break;
	default:
	    if (optimize && nodes[next].constant) {
		Tcl_InterpState save = Tcl_SaveInterpState(interp, TCL_OK);

		if (ExecConstantExprTree(interp, nodes, next, litObjvPtr)
			== TCL_OK) {
		    int index;
		    Tcl_Obj *objPtr = Tcl_GetObjResult(interp);

		    /*
		     * Don't generate a string rep, but if we have one
		     * already, then use it to share via the literal table.
		     */

		    if (objPtr->bytes) {
			Tcl_Obj *tableValue;

			index = TclRegisterNewLiteral(envPtr, objPtr->bytes,
				objPtr->length);
			tableValue = TclFetchLiteral(envPtr, index);
			if ((tableValue->typePtr == NULL) &&
				(objPtr->typePtr != NULL)) {
			    /*
			     * Same intrep surgery as for OT_LITERAL.
			     */

			    tableValue->typePtr = objPtr->typePtr;
			    tableValue->internalRep = objPtr->internalRep;
			    objPtr->typePtr = NULL;
			}
		    } else {
			index = TclAddLiteralObj(envPtr, objPtr, NULL);
		    }
		    TclEmitPush(index, envPtr);
		} else {
		    TclCompileSyntaxError(interp, envPtr);
		}
		Tcl_RestoreInterpState(interp, save);
		convert = 0;
	    } else {
		nodePtr = nodes + next;
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclSingleOpCmd --
 *
 *	Implements the commands: ~, !, <<, >>, %, !=, ne, in, ni
 *	in the ::tcl::mathop namespace.  These commands have no
 *	extension to arbitrary arguments; they accept only exactly one
 *	or exactly two arguments as suitable for the operator.
 *
 * Results:
 *	A standard Tcl return code and result left in interp.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclSingleOpCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    TclOpCmdClientData *occdPtr = clientData;
    unsigned char lexeme;
    OpNode nodes[2];
    Tcl_Obj *const *litObjv = objv + 1;

    if (objc != 1 + occdPtr->i.numArgs) {
	Tcl_WrongNumArgs(interp, 1, objv, occdPtr->expected);
	return TCL_ERROR;
    }

    ParseLexeme(occdPtr->op, strlen(occdPtr->op), &lexeme, NULL);
    nodes[0].lexeme = START;
    nodes[0].mark = MARK_RIGHT;
    nodes[0].right = 1;
    nodes[1].lexeme = lexeme;
    if (objc == 2) {
	nodes[1].mark = MARK_RIGHT;
    } else {
	nodes[1].mark = MARK_LEFT;
	nodes[1].left = OT_LITERAL;
    }
    nodes[1].right = OT_LITERAL;
    nodes[1].p.parent = 0;

    return ExecConstantExprTree(interp, nodes, 0, &litObjv);
}

/*
 *----------------------------------------------------------------------
 *
 * TclSortingOpCmd --
 *	Implements the commands:
 *		<, <=, >, >=, ==, eq
 *	in the ::tcl::mathop namespace. These commands are defined for
 *	arbitrary number of arguments by computing the AND of the base
 *	operator applied to all neighbor argument pairs.
 *
 * Results:
 *	A standard Tcl return code and result left in interp.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclSortingOpCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    int code = TCL_OK;

    if (objc < 3) {
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(1));
    } else {
	TclOpCmdClientData *occdPtr = clientData;
	Tcl_Obj **litObjv = TclStackAlloc(interp,
		2 * (objc-2) * sizeof(Tcl_Obj *));
	OpNode *nodes = TclStackAlloc(interp, 2 * (objc-2) * sizeof(OpNode));
	unsigned char lexeme;
	int i, lastAnd = 1;
	Tcl_Obj *const *litObjPtrPtr = litObjv;

	ParseLexeme(occdPtr->op, strlen(occdPtr->op), &lexeme, NULL);

	litObjv[0] = objv[1];
	nodes[0].lexeme = START;
	nodes[0].mark = MARK_RIGHT;
	for (i=2; i<objc-1; i++) {
	    litObjv[2*(i-1)-1] = objv[i];
	    nodes[2*(i-1)-1].lexeme = lexeme;
	    nodes[2*(i-1)-1].mark = MARK_LEFT;
	    nodes[2*(i-1)-1].left = OT_LITERAL;
	    nodes[2*(i-1)-1].right = OT_LITERAL;

	    litObjv[2*(i-1)] = objv[i];
	    nodes[2*(i-1)].lexeme = AND;
	    nodes[2*(i-1)].mark = MARK_LEFT;
	    nodes[2*(i-1)].left = lastAnd;
	    nodes[lastAnd].p.parent = 2*(i-1);

	    nodes[2*(i-1)].right = 2*(i-1)+1;
	    nodes[2*(i-1)+1].p.parent= 2*(i-1);

	    lastAnd = 2*(i-1);
	}
	litObjv[2*(objc-2)-1] = objv[objc-1];

	nodes[2*(objc-2)-1].lexeme = lexeme;
	nodes[2*(objc-2)-1].mark = MARK_LEFT;
	nodes[2*(objc-2)-1].left = OT_LITERAL;
	nodes[2*(objc-2)-1].right = OT_LITERAL;

	nodes[0].right = lastAnd;
	nodes[lastAnd].p.parent = 0;

	code = ExecConstantExprTree(interp, nodes, 0, &litObjPtrPtr);

	TclStackFree(interp, nodes);
	TclStackFree(interp, litObjv);
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclVariadicOpCmd --
 *	Implements the commands: +, *, &, |, ^, **
 *	in the ::tcl::mathop namespace. These commands are defined for
 *	arbitrary number of arguments by repeatedly applying the base
 *	operator with suitable associative rules. When fewer than two
 *	arguments are provided, suitable identity values are returned.
 *
 * Results:
 *	A standard Tcl return code and result left in interp.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclVariadicOpCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    TclOpCmdClientData *occdPtr = clientData;
    unsigned char lexeme;
    int code;

    if (objc < 2) {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(occdPtr->i.identity));
	return TCL_OK;
    }

    ParseLexeme(occdPtr->op, strlen(occdPtr->op), &lexeme, NULL);
    lexeme |= BINARY;

    if (objc == 2) {
	Tcl_Obj *litObjv[2];
	OpNode nodes[2];
	int decrMe = 0;
	Tcl_Obj *const *litObjPtrPtr = litObjv;

	if (lexeme == EXPON) {
	    litObjv[1] = Tcl_NewIntObj(occdPtr->i.identity);
	    Tcl_IncrRefCount(litObjv[1]);
	    decrMe = 1;
	    litObjv[0] = objv[1];
	    nodes[0].lexeme = START;
	    nodes[0].mark = MARK_RIGHT;
	    nodes[0].right = 1;
	    nodes[1].lexeme = lexeme;
	    nodes[1].mark = MARK_LEFT;
	    nodes[1].left = OT_LITERAL;
	    nodes[1].right = OT_LITERAL;
	    nodes[1].p.parent = 0;
	} else {
	    if (lexeme == DIVIDE) {
		litObjv[0] = Tcl_NewDoubleObj(1.0);
	    } else {
		litObjv[0] = Tcl_NewIntObj(occdPtr->i.identity);
	    }
	    Tcl_IncrRefCount(litObjv[0]);
	    litObjv[1] = objv[1];
	    nodes[0].lexeme = START;
	    nodes[0].mark = MARK_RIGHT;
	    nodes[0].right = 1;
	    nodes[1].lexeme = lexeme;
	    nodes[1].mark = MARK_LEFT;
	    nodes[1].left = OT_LITERAL;
	    nodes[1].right = OT_LITERAL;
	    nodes[1].p.parent = 0;
	}

	code = ExecConstantExprTree(interp, nodes, 0, &litObjPtrPtr);

	Tcl_DecrRefCount(litObjv[decrMe]);
	return code;
    } else {
	Tcl_Obj *const *litObjv = objv + 1;
	OpNode *nodes = TclStackAlloc(interp, (objc-1) * sizeof(OpNode));
	int i, lastOp = OT_LITERAL;

	nodes[0].lexeme = START;
	nodes[0].mark = MARK_RIGHT;
	if (lexeme == EXPON) {
	    for (i=objc-2; i>0; i--) {
		nodes[i].lexeme = lexeme;
		nodes[i].mark = MARK_LEFT;
		nodes[i].left = OT_LITERAL;
		nodes[i].right = lastOp;
		if (lastOp >= 0) {
		    nodes[lastOp].p.parent = i;
		}
		lastOp = i;
	    }
	} else {
	    for (i=1; i<objc-1; i++) {
		nodes[i].lexeme = lexeme;
		nodes[i].mark = MARK_LEFT;
		nodes[i].left = lastOp;
		if (lastOp >= 0) {
		    nodes[lastOp].p.parent = i;
		}
		nodes[i].right = OT_LITERAL;
		lastOp = i;
	    }
	}
	nodes[0].right = lastOp;
	nodes[lastOp].p.parent = 0;

	code = ExecConstantExprTree(interp, nodes, 0, &litObjv);

	TclStackFree(interp, nodes);
	return code;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclNoIdentOpCmd --
 *	Implements the commands: -, /
 *	in the ::tcl::mathop namespace. These commands are defined for
 *	arbitrary non-zero number of arguments by repeatedly applying the base
 *	operator with suitable associative rules. When no arguments are
 *	provided, an error is raised.
 *
 * Results:
 *	A standard Tcl return code and result left in interp.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclNoIdentOpCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    TclOpCmdClientData *occdPtr = clientData;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, occdPtr->expected);
	return TCL_ERROR;
    }
    return TclVariadicOpCmd(clientData, interp, objc, objv);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
