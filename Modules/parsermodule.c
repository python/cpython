/*  Parser.c
 *
 *  Copyright 1995 by Fred L. Drake, Jr. and Virginia Polytechnic Institute
 *  and State University, Blacksburg, Virginia, USA.  Portions copyright
 *  1991-1995 by Stichting Mathematisch Centrum, Amsterdam, The Netherlands.
 *  Copying is permitted under the terms associated with the main Python
 *  distribution, with the additional restriction that this additional notice
 *  be included and maintained on all distributed copies.
 *
 *  This module serves to replace the original parser module written by
 *  Guido.  The functionality is not matched precisely, but the original
 *  may be implemented on top of this.  This is desirable since the source
 *  of the text to be parsed is now divorced from this interface.
 *
 *  Unlike the prior interface, the ability to give a parse tree produced
 *  by Python code as a tuple to the compiler is enabled by this module.
 *  See the documentation for more details.
 *
 */

#include "Python.h"			/* general Python API		  */
#include "graminit.h"			/* symbols defined in the grammar */
#include "node.h"			/* internal parser structure	  */
#include "token.h"			/* token definitions		  */
					/* ISTERMINAL() / ISNONTERMINAL() */

/*
 *  All the "fudge" declarations are here:
 */


/*  These appearantly aren't prototyped in any of the standard Python headers,
 *  either by this name or as 'parse_string()/compile().'  This works at
 *  cutting out the warning, but needs to be done as part of the mainstream
 *  Python headers if this is going to be supported.  It is being handled as
 *  part of the Great Renaming.
 */
extern node*	 PyParser_SimpleParseString(char*, int);
extern PyObject* PyNode_Compile(node*, char*);


/*  This isn't part of the Python runtime, but it's in the library somewhere.
 *  Where it is varies a bit, so just declare it.
 */
extern char* strdup(const char*);


/*
 *  That's it!  Now, on to the module....
 */



/*  String constants used to initialize module attributes.
 *
 */
static char*
parser_copyright_string
= "Copyright 1995 by Virginia Polytechnic Institute & State University and\n"
  "Fred L. Drake, Jr., Blacksburg, Virginia, USA.  Portions copyright\n"
  "1991-1995 by Stichting Mathematisch Centrum, Amsterdam, The Netherlands.";


static char*
parser_doc_string
= "This is an interface to Python's internal parser.";

static char*
parser_version_string = "0.1";


/*  The function below is copyrigthed by Stichting Mathematisch Centrum.
 *  original copyright statement is included below, and continues to apply
 *  in full to the function immediately following.  All other material is
 *  original, copyrighted by Fred L. Drake, Jr. and Virginia Polytechnic
 *  Institute and State University.  Changes were made to comply with the
 *  new naming conventions.
 */

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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

static PyObject*
node2tuple(n)
	node *n;
{
	if (n == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (ISNONTERMINAL(TYPE(n))) {
		int i;
		PyObject *v, *w;
		v = PyTuple_New(1 + NCH(n));
		if (v == NULL)
			return v;
		w = PyInt_FromLong(TYPE(n));
		if (w == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		PyTuple_SetItem(v, 0, w);
		for (i = 0; i < NCH(n); i++) {
			w = node2tuple(CHILD(n, i));
			if (w == NULL) {
				Py_DECREF(v);
				return NULL;
			}
			PyTuple_SetItem(v, i+1, w);
		}
		return v;
	}
	else if (ISTERMINAL(TYPE(n))) {
		return Py_BuildValue("(is)", TYPE(n), STR(n));
	}
	else {
		PyErr_SetString(PyExc_SystemError,
			   "unrecognized parse tree node type");
		return NULL;
	}
}
/*
 *  End of material copyrighted by Stichting Mathematisch Centrum.
 */



/*  There are two types of intermediate objects we're interested in:
 *  'eval' and 'exec' types.  These constants can be used in the ast_type
 *  field of the object type to identify which any given object represents.
 *  These should probably go in an external header to allow other extensions
 *  to use them, but then, we really should be using C++ too.  ;-)
 *
 *  The PyAST_FRAGMENT type is not currently supported.
 */

#define PyAST_EXPR	1
#define PyAST_SUITE	2
#define PyAST_FRAGMENT	3


/*  These are the internal objects and definitions required to implement the
 *  AST type.  Most of the internal names are more reminiscent of the 'old'
 *  naming style, but the code uses the new naming convention.
 */

static PyObject*
parser_error = 0;


typedef struct _PyAST_Object {

    PyObject_HEAD			/* standard object header	    */
    node* ast_node;			/* the node* returned by the parser */
    int	  ast_type;			/* EXPR or SUITE ?		    */

} PyAST_Object;


staticforward void parser_free(PyAST_Object* ast);
staticforward int  parser_compare(PyAST_Object* left, PyAST_Object* right);
staticforward long parser_hash(PyAST_Object* ast);


/* static */
PyTypeObject PyAST_Type = {

    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "ast",				/* tp_name		*/
    sizeof(PyAST_Object),		/* tp_basicsize		*/
    0,					/* tp_itemsize		*/
    (destructor)parser_free,		/* tp_dealloc		*/
    0,					/* tp_print		*/
    0,					/* tp_getattr		*/
    0,					/* tp_setattr		*/
    (cmpfunc)parser_compare,		/* tp_compare		*/
    0,					/* tp_repr		*/
    0,					/* tp_as_number		*/
    0,					/* tp_as_sequence	*/
    0,					/* tp_as_mapping	*/
    0,					/* tp_hash		*/
    0,					/* tp_call		*/
    0					/* tp_str		*/

};  /* PyAST_Type */


static int
parser_compare_nodes(left, right)
     node* left;
     node* right;
{
    int j;

    if (TYPE(left) < TYPE(right))
	return (-1);

    if (TYPE(right) < TYPE(left))
	return (1);

    if (ISTERMINAL(TYPE(left)))
	return (strcmp(STR(left), STR(right)));

    if (NCH(left) < NCH(right))
	return (-1);

    if (NCH(right) < NCH(left))
	return (1);

    for (j = 0; j < NCH(left); ++j) {
	int v = parser_compare_nodes(CHILD(left, j), CHILD(right, j));

	if (v)
	    return (v);
    }
    return (0);

}   /* parser_compare_nodes() */


/*  int parser_compare(PyAST_Object* left, PyAST_Object* right)
 *
 *  Comparison function used by the Python operators ==, !=, <, >, <=, >=
 *  This really just wraps a call to parser_compare_nodes() with some easy
 *  checks and protection code.
 *
 */
static int
parser_compare(left, right)
     PyAST_Object* left;
     PyAST_Object* right;
{
    if (left == right)
	return (0);

    if ((left == 0) || (right == 0))
	return (-1);

    return (parser_compare_nodes(left->ast_node, right->ast_node));

}   /* parser_compare() */


/*  parser_newastobject(node* ast)
 *
 *  Allocates a new Python object representing an AST.  This is simply the
 *  'wrapper' object that holds a node* and allows it to be passed around in
 *  Python code.
 *
 */
static PyObject*
parser_newastobject(ast, type)
     node* ast;
     int   type;
{
    PyAST_Object* o = PyObject_NEW(PyAST_Object, &PyAST_Type);

    if (o != 0) {
	o->ast_node = ast;
	o->ast_type = type;
    }
    return ((PyObject*)o);

}   /* parser_newastobject() */


/*  void parser_free(PyAST_Object* ast)
 *
 *  This is called by a del statement that reduces the reference count to 0.
 *
 */
static void
parser_free(ast)
     PyAST_Object* ast;
{
    PyNode_Free(ast->ast_node);
    PyMem_DEL(ast);

}   /* parser_free() */


/*  parser_ast2tuple(PyObject* self, PyObject* args)
 *
 *  This provides conversion from a node* to a tuple object that can be
 *  returned to the Python-level caller.  The AST object is not modified.
 *
 */
static PyObject*
parser_ast2tuple(self, args)
     PyObject* self;
     PyObject* args;
{
    PyObject* ast;
    PyObject* res = 0;

    if (PyArg_ParseTuple(args, "O!:ast2tuple", &PyAST_Type, &ast)) {
	/*
	 *  Convert AST into a tuple representation.  Use Guido's function,
	 *  since it's known to work already.
	 */
	res = node2tuple(((PyAST_Object*)ast)->ast_node);
    }
    return (res);

}   /* parser_ast2tuple() */


/*  parser_compileast(PyObject* self, PyObject* args)
 *
 *  This function creates code objects from the parse tree represented by
 *  the passed-in data object.  An optional file name is passed in as well.
 *
 */
static PyObject*
parser_compileast(self, args)
     PyObject* self;
     PyObject* args;
{
    PyAST_Object* ast;
    PyObject*     res = 0;
    char*	  str = "<ast>";

    if (PyArg_ParseTuple(args, "O!|s", &PyAST_Type, &ast, &str))
	res = PyNode_Compile(ast->ast_node, str);

    return (res);

}   /* parser_compileast() */


/*  PyObject* parser_isexpr(PyObject* self, PyObject* args)
 *  PyObject* parser_issuite(PyObject* self, PyObject* args)
 *
 *  Checks the passed-in AST object to determine if it is an expression or
 *  a statement suite, respectively.  The return is a Python truth value.
 *
 */
static PyObject*
parser_isexpr(self, args)
     PyObject* self;
     PyObject* args;
{
    PyAST_Object* ast;
    PyObject* res = 0;

    if (PyArg_ParseTuple(args, "O!:isexpr", &PyAST_Type, &ast)) {
	/*
	 *  Check to see if the AST represents an expression or not.
	 */
	res = (ast->ast_type == PyAST_EXPR) ? Py_True : Py_False;
	Py_INCREF(res);
    }
    return (res);

}   /* parser_isexpr() */


static PyObject*
parser_issuite(self, args)
     PyObject* self;
     PyObject* args;
{
    PyAST_Object* ast;
    PyObject* res = 0;

    if (PyArg_ParseTuple(args, "O!:isexpr", &PyAST_Type, &ast)) {
	/*
	 *  Check to see if the AST represents an expression or not.
	 */
	res = (ast->ast_type == PyAST_EXPR) ? Py_False : Py_True;
	Py_INCREF(res);
    }
    return (res);

}   /* parser_issuite() */


/*  PyObject* parser_do_parse(PyObject* args, int type)
 *
 *  Internal function to actually execute the parse and return the result if
 *  successful, or set an exception if not.
 *
 */
static PyObject*
parser_do_parse(args, type)
     PyObject *args;
     int      type;
{
    char*     string = 0;
    PyObject* res    = 0;

    if (PyArg_ParseTuple(args, "s", &string)) {
	node* n = PyParser_SimpleParseString(string,
					     (type == PyAST_EXPR)
					     ? eval_input : file_input);

	if (n != 0)
	    res = parser_newastobject(n, type);
	else
	    PyErr_SetString(parser_error, "Could not parse string.");
    }
    return (res);

}  /* parser_do_parse() */


/*  PyObject* parser_expr(PyObject* self, PyObject* args)
 *  PyObject* parser_suite(PyObject* self, PyObject* args)
 *
 *  External interfaces to the parser itself.  Which is called determines if
 *  the parser attempts to recognize an expression ('eval' form) or statement
 *  suite ('exec' form).  The real work is done by parser_do_parse() above.
 *
 */
static PyObject*
parser_expr(self, args)
     PyObject* self;
     PyObject* args;
{
    return (parser_do_parse(args, PyAST_EXPR));

}   /* parser_expr() */


static PyObject*
parser_suite(self, args)
     PyObject* self;
     PyObject* args;
{
    return (parser_do_parse(args, PyAST_SUITE));

}   /* parser_suite() */



/*  This is the messy part of the code.  Conversion from a tuple to an AST
 *  object requires that the input tuple be valid without having to rely on
 *  catching an exception from the compiler.  This is done to allow the
 *  compiler itself to remain fast, since most of its input will come from
 *  the parser directly, and therefore be known to be syntactically correct.
 *  This validation is done to ensure that we don't core dump the compile
 *  phase, returning an exception instead.
 *
 *  Two aspects can be broken out in this code:  creating a node tree from
 *  the tuple passed in, and verifying that it is indeed valid.  It may be
 *  advantageous to expand the number of AST types to include funcdefs and
 *  lambdadefs to take advantage of the optimizer, recognizing those ASTs
 *  here.  They are not necessary, and not quite as useful in a raw form.
 *  For now, let's get expressions and suites working reliably.
 */


staticforward node* build_node_tree(PyObject*);
staticforward int   validate_expr_tree(node*);
staticforward int   validate_suite_tree(node*);


/*  PyObject* parser_tuple2ast(PyObject* self, PyObject* args)
 *
 *  This is the public function, called from the Python code.  It receives a
 *  single tuple object from the caller, and creates an AST object if the
 *  tuple can be validated.  It does this by checking the first code of the
 *  tuple, and, if acceptable, builds the internal representation.  If this
 *  step succeeds, the internal representation is validated as fully as
 *  possible with the various validate_*() routines defined below.
 *
 *  This function must be changed if support is to be added for PyAST_FRAGMENT
 *  AST objects.
 *
 */
static PyObject*
parser_tuple2ast(self, args)
     PyObject* self;
     PyObject* args;
{
    PyObject* ast	= 0;
    PyObject* tuple	= 0;
    int	      start_sym;
    int	      next_sym;

    if ((PyTuple_Size(args) == 1)
	&& (tuple = PyTuple_GetItem(args, 0))
	&& PyTuple_Check(tuple)
	&& (PyTuple_Size(tuple) >= 2)
	&& PyInt_Check(PyTuple_GetItem(tuple, 0))
	&& PyTuple_Check(PyTuple_GetItem(tuple, 1))
	&& (PyTuple_Size(PyTuple_GetItem(tuple, 1)) >= 2)
	&& PyInt_Check(PyTuple_GetItem(PyTuple_GetItem(tuple, 1), 0))) {

	/*
	 *  This might be a valid parse tree, but let's do a quick check
	 *  before we jump the gun.
	 */

	start_sym = PyInt_AsLong(PyTuple_GetItem(tuple, 0));
	next_sym = PyInt_AsLong(PyTuple_GetItem(PyTuple_GetItem(tuple, 1), 0));

	if ((start_sym == eval_input) && (next_sym == testlist)) {
	    /*
	     *  Might be an expression.
	     */
	    node* expression = build_node_tree(PyTuple_GetItem(args, 0));

	    puts("Parser.tuple2ast: built eval input tree.");
	    if ((expression != 0) && validate_expr_tree(expression))
		ast = parser_newastobject(expression, PyAST_EXPR);
	}
	else if ((start_sym == file_input) && (next_sym == stmt)) {
	    /*
	     *  This looks like a suite so far.
	     */
	    node* suite_tree = build_node_tree(PyTuple_GetItem(args, 0));

	    puts("Parser.tuple2ast: built file input tree.");
	    if ((suite_tree != 0) && validate_suite_tree(suite_tree))
		ast = parser_newastobject(suite_tree, PyAST_SUITE);
	}
	/*
	 *  Make sure we throw an exception on all errors.  We should never
	 *  get this, but we'd do well to be sure something is done.
	 */
	if ((ast == 0) && !PyErr_Occurred()) {
	    PyErr_SetString(parser_error, "Unspecified ast error occurred.");
	}
    }
    else {
	PyErr_SetString(PyExc_TypeError,
			"parser.tuple2ast(): expected single tuple.");
    }
    return (ast);

}   /* parser_tuple2ast() */


/*  int check_terminal_tuple()
 *
 *  Check a tuple to determine that it is indeed a valid terminal node.  The
 *  node is known to be required as a terminal, so we throw an exception if
 *  there is a failure.  The portion of the resulting node tree already built
 *  is passed in so we can deallocate it in the event of a failure.
 *
 *  The format of an acceptable terminal tuple is "(is)":  the fact that elem
 *  is a tuple and the integer is a valid terminal symbol has been established
 *  before this function is called.  We must check the length of the tuple and
 *  the type of the second element.  We do *NOT* check the actual text of the
 *  string element, which we could do in many cases.  This is done by the
 *  validate_*() functions which operate on the internal representation.
 *
 */
static int
check_terminal_tuple(elem, result)
     PyObject* elem;
     node*     result;
{
    int   res = 0;
    char* str = 0;

    if (PyTuple_Size(elem) != 2) {
	str = "Illegal terminal symbol; node too long.";
    }
    else if (!PyString_Check(PyTuple_GetItem(elem, 1))) {
	str = "Illegal terminal symbol; expected a string.";
    }
    else
	res = 1;

    if ((res == 0) && (result != 0)) {
	elem = Py_BuildValue("(os)", elem, str);
	PyErr_SetObject(parser_error, elem);
    }
    return (res);

}   /* check_terminal_tuple() */


/*  node* build_node_children()
 *
 *  Iterate across the children of the current non-terminal node and build
 *  their structures.  If successful, return the root of this portion of
 *  the tree, otherwise, 0.  Any required exception will be specified already,
 *  and no memory will have been deallocated.
 *
 */
static node*
build_node_children(tuple, root, line_num)
     PyObject* tuple;
     node*     root;
     int*      line_num;
{
    int len = PyTuple_Size(tuple);
    int i;

    for (i = 1; i < len; ++i) {
	/* elem must always be a tuple, however simple */
	PyObject* elem = PyTuple_GetItem(tuple, i);
	long      type = 0;
	char*     strn = 0;

	if ((!PyTuple_Check(elem)) || !PyInt_Check(PyTuple_GetItem(elem, 0))) {
	    PyErr_SetObject(parser_error,
			    Py_BuildValue("(os)", elem,
					  "Illegal node construct."));
	    return (0);
	}
	type = PyInt_AsLong(PyTuple_GetItem(elem, 0));

	if (ISTERMINAL(type)) {
	    if (check_terminal_tuple(elem, root))
		strn = strdup(PyString_AsString(PyTuple_GetItem(elem, 1)));
	    else
		return (0);
	}
	else if (!ISNONTERMINAL(type)) {
	    /*
	     *  It has to be one or the other; this is an error.
	     *  Throw an exception.
	     */
	    PyErr_SetObject(parser_error,
			    Py_BuildValue("(os)", elem,
					  "Unknown node type."));
	    return (0);
	}
	PyNode_AddChild(root, type, strn, *line_num);

	if (ISNONTERMINAL(type)) {
	    node* new_child = CHILD(root, i - 1);

	    if (new_child != build_node_children(elem, new_child))
		return (0);
	}
	else if (type == NEWLINE)	/* It's true:  we increment the     */
	    ++(*line_num);		/* line number *after* the newline! */
    }
    return (root);

}   /* build_node_children() */


static node*
build_node_tree(tuple)
     PyObject* tuple;
{
    node* res = 0;
    long  num = PyInt_AsLong(PyTuple_GetItem(tuple, 0));

    if (ISTERMINAL(num)) {
	/*
	 *  The tuple is simple, but it doesn't start with a start symbol.
	 *  Throw an exception now and be done with it.
	 */
	tuple = Py_BuildValue("(os)", tuple,
			      "Illegal ast tuple; cannot start with terminal symbol.");
	PyErr_SetObject(parser_error, tuple);
    }
    else if (ISNONTERMINAL(num)) {
	/*
	 *  Not efficient, but that can be handled later.
	 */
	int line_num = 0;

	res = PyNode_New(num);
	if (res != build_node_children(tuple, res, &line_num)) {
	    PyNode_Free(res);
	    res = 0;
	}
    }
    else {
	/*
	 *  The tuple is illegal -- if the number is neither TERMINAL nor
	 *  NONTERMINAL, we can't use it.
	 */
	PyErr_SetObject(parser_error,
			Py_BuildValue("(os)", tuple,
				      "Illegal component tuple."));
    }
    return (res);

}   /* build_node_tree() */


#define	VALIDATER(n)	static int validate_##n(node*)
#define	VALIDATE(n)	static int validate_##n(node* tree)


/*
 *  Validation for the code above:
 */
VALIDATER(expr_tree);
VALIDATER(suite_tree);


/*
 *  Validation routines used within the validation section:
 */
staticforward int validate_terminal(node*, int, char*);

#define	validate_ampersand(ch)	validate_terminal(ch,	   AMPER, "&")
#define	validate_circumflex(ch)	validate_terminal(ch, CIRCUMFLEX, "^")
#define validate_colon(ch)	validate_terminal(ch,	   COLON, ":")
#define validate_comma(ch)	validate_terminal(ch,	   COMMA, ",")
#define validate_dedent(ch)	validate_terminal(ch,	  DEDENT, "")
#define	validate_equal(ch)	validate_terminal(ch,	   EQUAL, "=")
#define validate_indent(ch)	validate_terminal(ch,	  INDENT, "")
#define validate_lparen(ch)	validate_terminal(ch,	    LPAR, "(")
#define	validate_newline(ch)	validate_terminal(ch,	 NEWLINE, "")
#define validate_rparen(ch)	validate_terminal(ch,	    RPAR, ")")
#define validate_semi(ch)	validate_terminal(ch,	    SEMI, ";")
#define	validate_star(ch)	validate_terminal(ch,	    STAR, "*")
#define	validate_vbar(ch)	validate_terminal(ch,	    VBAR, "|")

#define	validate_compound_stmt(ch) validate_node(ch)
#define	validate_name(ch, str)	validate_terminal(ch,    NAME, str)
#define validate_small_stmt(ch)	validate_node(ch)

VALIDATER(class);		VALIDATER(node);
VALIDATER(parameters);		VALIDATER(suite);
VALIDATER(testlist);		VALIDATER(varargslist);
VALIDATER(fpdef);		VALIDATER(fplist);
VALIDATER(stmt);		VALIDATER(simple_stmt);
VALIDATER(expr_stmt);
VALIDATER(print_stmt);		VALIDATER(del_stmt);
VALIDATER(return_stmt);
VALIDATER(raise_stmt);		VALIDATER(import_stmt);
VALIDATER(global_stmt);
VALIDATER(access_stmt);		VALIDATER(accesstype);
VALIDATER(exec_stmt);		VALIDATER(compound_stmt);
VALIDATER(while);		VALIDATER(for);
VALIDATER(try);			VALIDATER(except_clause);
VALIDATER(test);		VALIDATER(and_test);
VALIDATER(not_test);		VALIDATER(comparison);
VALIDATER(comp_op);		VALIDATER(expr);
VALIDATER(xor_expr);		VALIDATER(and_expr);
VALIDATER(shift_expr);		VALIDATER(arith_expr);
VALIDATER(term);		VALIDATER(factor);
VALIDATER(atom);		VALIDATER(lambdef);
VALIDATER(trailer);		VALIDATER(subscript);
VALIDATER(exprlist);		VALIDATER(dictmaker);


#define	is_even(n)	(((n) & 1) == 0)
#define	is_odd(n)	(((n) & 1) == 1)


static int
validate_ntype(n, t)
     node* n;
     int   t;
{
    int res = (TYPE(n) == t);

    if (!res) {
	char buffer[128];

	sprintf(buffer, "Expected node type %d, got %d.", t, TYPE(n));
	PyErr_SetString(parser_error, buffer);
    }
    return (res);

}   /* validate_ntype() */


static int
validate_terminal(terminal, type, string)
     node* terminal;
     int   type;
     char* string;
{
    static char buffer[60];
    int res = ((TYPE(terminal) == type)
	       && (strcmp(string, STR(terminal)) == 0));

    if (!res) {
	sprintf(buffer, "Illegal NAME: expected \"%s\"", string);
	PyErr_SetString(parser_error, buffer);
    }
    return (res);

}   /* validate_terminal() */


VALIDATE(class) {
    int nch = NCH(tree);
    int res = (((nch == 4)
		|| ((nch == 7)
		    && validate_lparen(CHILD(tree, 2))
		    && validate_ntype(CHILD(tree, 3), testlist)
		    && validate_testlist(CHILD(tree, 3))
		    && validate_rparen(CHILD(tree, 4))))
		&& validate_terminal(CHILD(tree, 0),	NAME, "class")
		&& validate_ntype(CHILD(tree, 1), NAME)
		&& validate_colon(CHILD(tree, nch - 2))
		&& validate_ntype(CHILD(tree, nch - 1), suite)
		&& validate_suite(CHILD(tree, nch - 1)));

    if (!res) {
	if ((nch >= 2)
	    && validate_ntype(CHILD(tree, 1), NAME)) {
	    char buffer[128];

	    sprintf(buffer, "Illegal classdef tuple for %s",
		    STR(CHILD(tree, 1)));
	    PyErr_SetString(parser_error, buffer);
	}
	else {
	    PyErr_SetString(parser_error, "Illegal classdef tuple.");
	}
    }
    return (res);

}   /* validate_class() */


static int
validate_elif(elif_node, test_node, colon_node, suite_node)
     node* elif_node;
     node* test_node;
     node* colon_node;
     node* suite_node;
{
    return (validate_ntype(test_node, test)
	    && validate_ntype(suite_node, suite)
	    && validate_name(elif_node, "elif")
	    && validate_colon(colon_node)
	    && validate_node(test_node)
	    && validate_suite(suite_node));

}   /* validate_elif() */


static int
validate_else(else_node, colon_node, suite_node)
     node* else_node;
     node* colon_node;
     node* suite_node;
{
    return (validate_ntype(suite_node, suite)
	    && validate_name(else_node, "else")
	    && validate_colon(colon_node)
	    && validate_suite(suite_node));

}   /* validate_else() */


VALIDATE(if) {
    int nch = NCH(tree);
    int res = ((nch >= 4)
	       && validate_ntype(CHILD(tree, 1), test)
	       && validate_ntype(CHILD(tree, 3), suite)
	       && validate_name(CHILD(tree, 0), "if")
	       && validate_colon(CHILD(tree, 2))
	       && validate_parameters(CHILD(tree, 1))
	       && validate_suite(CHILD(tree, 3)));

    if (res && ((nch % 4) == 3)) {
	/*
	 *  There must be a single 'else' clause, and maybe a series
	 *  of 'elif' clauses.
	 */
	res = validate_else(CHILD(tree, nch-3), CHILD(tree, nch-2),
			    CHILD(tree, nch-1));
	nch -= 3;
    }
    if ((nch % 4) != 0)
	res = 0;
    else if (res && (nch > 4)) {
	/*
	 *  There might be a series of 'elif' clauses.
	 */
	int j = 4;
	while ((j < nch) && res) {
	    res = validate_elif(CHILD(tree, j),   CHILD(tree, j+1),
				CHILD(tree, j+2), CHILD(tree, j+3));
	    j += 4;
	}
    }
    if (!res && !PyErr_Occurred()) {
	PyErr_SetString(parser_error, "Illegal 'if' statement found.");
    }
    return (res);

}   /* validate_if() */


VALIDATE(parameters) {
    int res = 1;
    int nch = NCH(tree);

    res = (((nch == 2)
	    || ((nch == 3)
		&& validate_varargslist(CHILD(tree, 1))))
	   && validate_lparen(CHILD(tree, 0))
	   && validate_rparen(CHILD(tree, nch - 1)));

    return (res);

}   /* validate_parameters() */


VALIDATE(suite) {
    int res = 1;
    int nch = NCH(tree);

    if (nch == 1) {
	res = (validate_ntype(CHILD(tree, 0), simple_stmt)
	       && validate_simple_stmt(CHILD(tree, 0)));
    }
    else {
	res = ((nch >= 5)
	       && validate_newline(CHILD(tree, 0))
	       && validate_indent(CHILD(tree, 1))
	       && validate_dedent(CHILD(tree, nch - 1)));

	if (res) {
	    int i = 2;

	    while (TYPE(CHILD(tree, i)) == NEWLINE)
		++i;
	    res = (validate_ntype(CHILD(tree, i), stmt)
		   && validate_stmt(CHILD(tree, i)));

	    if (res) {
		++i;
		while (TYPE(CHILD(tree, i)) == NEWLINE)
		    ++i;

		while (res && (TYPE(CHILD(tree, i)) != DEDENT)) {
		    res = (validate_ntype(CHILD(tree, i), stmt)
			   && validate_stmt(CHILD(tree, i)));

		    if (res) {
			++i;
			while (TYPE(CHILD(tree, i)) == NEWLINE)
			    ++i;
		    }
		}
	    }
	}
    }
    return (res);

}   /* validate_suite() */


VALIDATE(testlist) {
    int i;
    int nch = NCH(tree);
    int res = ((nch >= 1)
	       && (is_odd(nch)
		   || validate_comma(CHILD(tree, nch - 1))));

    /*
     *	If there are an even, non-zero number of children, the last one
     *	absolutely must be a comma.  Why the trailing comma is allowed,
     *	I have no idea!
     */
    if ((res) && is_odd(nch)) {
	/*
	 *  If the number is odd, the last is a test, and can be
	 *  verified.  What's left, if anything, can be verified
	 *  as a list of [test, comma] pairs.
	 */
	--nch;
	res = (validate_ntype(CHILD(tree, nch), test)
	       && validate_test(CHILD(tree, nch)));
    }
    for (i = 0; res && (i < nch); i += 2) {
	res = (validate_ntype(CHILD(tree, i), test)
	       && validate_test(CHILD(tree, i))
	       && validate_comma(CHILD(tree, i + 1)));
    }
    return (res);

}   /* validate_testlist() */


VALIDATE(varargslist) {
    int nch = NCH(tree);
    int res = (nch != 0);

    if (res && (TYPE(CHILD(tree, 0)) == fpdef)) {
	int pos = 0;

	while (res && (pos < nch)) {
	    res = (validate_ntype(CHILD(tree, pos), fpdef)
		   && validate_fpdef(CHILD(tree, pos)));
	    ++pos;
	    if (res && (pos < nch) && (TYPE(CHILD(tree, pos)) == EQUAL)) {
		res = ((pos + 1 < nch)
		       && validate_ntype(CHILD(tree, pos + 1), test)
		       && validate_test(CHILD(tree, pos + 1)));
		pos += 2;
	    }
	    if (res && (pos < nch)) {
		res = validate_comma(CHILD(tree, pos));
		++pos;
	    }
	}
    }
    else {
	int pos = 0;

	res = ((nch > 1)
	       && ((nch & 1) == 0)
	       && validate_star(CHILD(tree, nch - 2))
	       && validate_ntype(CHILD(tree, nch - 1), NAME));

	nch -= 2;
	while (res && (pos < nch)) {
	    /*
	     *  Sequence of:	fpdef ['=' test] ','
	     */
	    res = (validate_ntype(CHILD(tree, pos), fpdef)
		   && validate_fpdef(CHILD(tree, pos))
		   && ((TYPE(CHILD(tree, pos + 1)) == COMMA)
		       || (((pos + 2) < nch)
			   && validate_equal(CHILD(tree, pos + 1))
			   && validate_ntype(CHILD(tree, pos + 2), test)
			   && validate_test(CHILD(tree, pos + 2))
			   && validate_comma(CHILD(tree, pos + 3)))));
	}
    }
    return (res);

}   /* validate_varargslist() */


VALIDATE(fpdef) {
    int nch = NCH(tree);

    return (((nch == 1)
	     && validate_ntype(CHILD(tree, 0), NAME))
	    || ((nch == 3)
		&& validate_lparen(CHILD(tree, 0))
		&& validate_fplist(CHILD(tree, 1))
		&& validate_rparen(CHILD(tree, 2))));

} /* validate_fpdef() */


VALIDATE(fplist) {
    int j;
    int nch = NCH(tree);
    int res = ((nch != 0) && validate_fpdef(CHILD(tree, 0)));

    if (res && is_even(nch)) {
	res = validate_comma(CHILD(tree, nch - 1));
	--nch;
    }
    for (j = 1; res && (j < nch); j += 2) {
	res = (validate_comma(CHILD(tree, j))
	       && validate_fpdef(CHILD(tree, j + 1)));
    }
    return (res);

}   /* validate_fplist() */


VALIDATE(stmt) {
    int nch = NCH(tree);

    return ((nch == 1)
	    && (((TYPE(CHILD(tree, 0)) == simple_stmt)
		 && validate_simple_stmt(CHILD(tree, 0)))
		|| (validate_ntype(CHILD(tree, 0), compound_stmt)
		    && validate_compound_stmt(CHILD(tree, 0)))));

}   /* validate_stmt() */


VALIDATE(simple_stmt) {
    int nch = NCH(tree);
    int res = ((nch >= 2)
	       && validate_ntype(CHILD(tree, 0), small_stmt)
	       && validate_small_stmt(CHILD(tree, 0))
	       && validate_newline(CHILD(tree, nch - 1)));

    --nch;			/* forget the NEWLINE */
    if (res && (nch >= 2)) {
	if (TYPE(CHILD(tree, nch - 1)) == SEMI)
	    --nch;
    }
    if (res && (nch > 2)) {
	int i;

	for (i = 1; res && (i < nch); i += 2) {
	    res = (validate_semi(CHILD(tree, i))
		   && validate_ntype(CHILD(tree, i + 1), small_stmt)
		   && validate_small_stmt(CHILD(tree, i + 1)));
	}
    }
    return (res);

}   /* validate_simple_stmt() */


VALIDATE(expr_stmt) {
    int j;
    int nch = NCH(tree);
    int res = (is_odd(nch)
	       && (validate_testlist(CHILD(tree, 0))));

    for (j = 1; res && (j < nch); j += 2) {
	res = (validate_equal(CHILD(tree, j))
	       && validate_ntype(CHILD(tree, j + 1), testlist)
	       && validate_testlist(CHILD(tree, j + 1)));
    }
    return (res);

}   /* validate_expr_stmt() */


VALIDATE(print_stmt) {
    int j;
    int nch = NCH(tree);
    int res = ((nch != 0)
	       && is_even(nch)
	       && validate_name(CHILD(tree, 0), "print")
	       && validate_ntype(CHILD(tree, 1), test)
	       && validate_test(CHILD(tree, 1)));
    
    for (j = 2; res && (j < nch); j += 2) {
	res = (validate_comma(CHILD(tree, j))
	       && validate_ntype(CHILD(tree, j + 1), test)
	       && validate_test(CHILD(tree, 1)));
    }
    return (res);

}   /* validate_print_stmt() */


VALIDATE(del_stmt) {

    return ((NCH(tree) == 2)
	    && validate_name(CHILD(tree, 0), "del")
	    && validate_ntype(CHILD(tree, 1), exprlist)
	    && validate_exprlist(CHILD(tree, 1)));

}   /* validate_del_stmt() */


VALIDATE(return_stmt) {
    int nch = NCH(tree);
    int res = (((nch == 1)
		|| (nch == 2))
	       && validate_name(CHILD(tree, 0), "return"));

    if (res && (nch == 2)) {
	res = (validate_ntype(CHILD(tree, 1), testlist)
	       && validate_testlist(CHILD(tree, 1)));
    }
    return (res);

}   /* validate_return_stmt() */


VALIDATE(raise_stmt) {
    int nch = NCH(tree);
    int res = (((nch == 2) || (nch == 4))
	       && validate_name(CHILD(tree, 0), "raise")
	       && validate_ntype(CHILD(tree, 1), test)
	       && validate_test(CHILD(tree, 1)));

    if (res && (nch == 4)) {
	res = (validate_comma(CHILD(tree, 2))
	       && (TYPE(CHILD(tree, 3)) == test)
	       && validate_test(CHILD(tree, 3)));
    }
    return (res);

}   /* validate_raise_stmt() */


VALIDATE(import_stmt) {
    int nch = NCH(tree);
    int res = ((nch >= 2)
	       && validate_ntype(CHILD(tree, 0), NAME)
	       && validate_ntype(CHILD(tree, 1), NAME));

    if (res && (strcmp(STR(CHILD(tree, 0)), "import") == 0)) {
	res = is_even(nch);
	if (res) {
	    int j;

	    for (j = 2; res && (j < nch); j += 2) {
		res = (validate_comma(CHILD(tree, j))
		       && validate_ntype(CHILD(tree, j + 1), NAME));
	    }
	}
    }
    else if (res && validate_name(CHILD(tree, 0), "from")) {
	res = ((nch >= 4)
	       && is_even(nch)
	       && validate_name(CHILD(tree, 2), "import"));
	if (nch == 4) {
	    res = ((TYPE(CHILD(tree, 3)) == NAME)
		   || validate_ntype(CHILD(tree, 3), STAR));
	}
	else {
	    /* 'from' NAME 'import' NAME (',' NAME)* */
	    int j;

	    res = validate_ntype(CHILD(tree, 3), NAME);
	    for (j = 4; res && (j < nch); j += 2) {
		res = (validate_comma(CHILD(tree, j))
		       && validate_ntype(CHILD(tree, j + 1), NAME));
	    }
	}
    }
    else {
	res = 0;
    }
    return (res);

}   /* validate_import_stmt() */


VALIDATE(global_stmt) {
    int j;
    int nch = NCH(tree);
    int res = (is_even(nch)
	       && validate_name(CHILD(tree, 0), "global")
	       && validate_ntype(CHILD(tree, 1), NAME));

    for (j = 2; res && (j < nch); j += 2) {
	res = (validate_comma(CHILD(tree, j))
	       && validate_ntype(CHILD(tree, j + 1), NAME));
    }
    return (res);

}   /* validate_global_stmt() */


VALIDATE(access_stmt) {
    int pos = 3;
    int nch = NCH(tree);
    int res = ((nch >= 4)
	       && is_even(nch)
	       && validate_name(CHILD(tree, 0), "access")
	       && validate_accesstype(CHILD(tree, nch - 1)));

    if (res && (TYPE(CHILD(tree, 1)) != STAR)) {
	int j;

	res = validate_ntype(CHILD(tree, 1), NAME);
	for (j = 2; res && (j < (nch - 2)); j += 2) {
	    if (TYPE(CHILD(tree, j)) == COLON)
		break;
	    res = (validate_comma(CHILD(tree, j))
		   && validate_ntype(CHILD(tree, j + 1), NAME)
		   && (pos += 2));
	}
    }
    else {
	res = validate_star(CHILD(tree, 1));
    }
    res = (res && validate_colon(CHILD(tree, pos - 1)));

    for (; res && (pos < (nch - 1)); pos += 2) {
	res = (validate_accesstype(CHILD(tree, pos))
	       && validate_comma(CHILD(tree, pos + 1)));
    }
    return (res && (pos == (nch - 1)));

}   /* validate_access_stmt() */


VALIDATE(accesstype) {
    int nch = NCH(tree);
    int res = (nch >= 1);
    int i;

    for (i = 0; res && (i < nch); ++i) {
	res = validate_ntype(CHILD(tree, i), NAME);
    }
    return (res);

}   /* validate_accesstype() */


VALIDATE(exec_stmt) {
    int nch = NCH(tree);
    int res = (((nch == 2) || (nch == 4) || (nch == 6))
	       && validate_name(CHILD(tree, 0), "exec")
	       && validate_expr(CHILD(tree, 1)));

    if (res && (nch > 2)) {
	res = (validate_name(CHILD(tree, 2), "in")
	       && validate_test(CHILD(tree, 3)));
    }
    if (res && (nch > 4)) {
	res = (validate_comma(CHILD(tree, 4))
	       && validate_test(CHILD(tree, 5)));
    }
    return (res);

}   /* validate_exec_stmt() */


VALIDATE(while) {
    int nch = NCH(tree);
    int res = (((nch == 4) || (nch == 7))
	       && validate_name(CHILD(tree, 0), "while")
	       && validate_ntype(CHILD(tree, 1), test)
	       && validate_test(CHILD(tree, 1))
	       && validate_colon(CHILD(tree, 2))
	       && validate_ntype(CHILD(tree, 3), suite)
	       && validate_suite(CHILD(tree, 3)));

    if (res && (nch == 7)) {
	res = (validate_name(CHILD(tree, 4), "else")
	       && validate_colon(CHILD(tree, 5))
	       && validate_ntype(CHILD(tree, 6), suite)
	       && validate_suite(CHILD(tree, 6)));
    }
    return (res);

}   /* validate_while() */


VALIDATE(for) {
    int nch = NCH(tree);
    int res = (((nch == 6) || (nch == 9))
	       && validate_name(CHILD(tree, 0), "for")
	       && validate_ntype(CHILD(tree, 1), exprlist)
	       && validate_exprlist(CHILD(tree, 1))
	       && validate_name(CHILD(tree, 2), "in")
	       && validate_ntype(CHILD(tree, 3), testlist)
	       && validate_testlist(CHILD(tree, 3))
	       && validate_colon(CHILD(tree, 4))
	       && validate_ntype(CHILD(tree, 5), suite)
	       && validate_suite(CHILD(tree, 5)));

    if (res && (nch == 9)) {
	res = (validate_name(CHILD(tree, 6), "else")
	       && validate_colon(CHILD(tree, 7))
	       && validate_ntype(CHILD(tree, 8), suite)
	       && validate_suite(CHILD(tree, 8)));
    }
    return (res);

}   /* validate_for() */


VALIDATE(try) {
    int nch = NCH(tree);
    int res = ((nch >= 6)
	       && ((nch % 3) == 0)
	       && validate_name(CHILD(tree, 0), "try")
	       && validate_colon(CHILD(tree, 1))
	       && validate_ntype(CHILD(tree, 2), suite)
	       && validate_suite(CHILD(tree, 2))
	       && validate_colon(CHILD(tree, nch - 2))
	       && validate_ntype(CHILD(tree, nch - 1), suite)
	       && validate_suite(CHILD(tree, nch - 1)));

    if (res && (TYPE(CHILD(tree, 3)) == except_clause)) {
	int groups = (nch / 3) - 2;

	res = validate_except_clause(CHILD(tree, 3));

	if (res && (groups != 0)) {
	    int cln_pos = 4;
	    int sui_pos = 5;
	    int nxt_pos = 6;

	    while (res && groups--) {
		res = (validate_colon(CHILD(tree, cln_pos))
		       && validate_ntype(CHILD(tree, sui_pos), suite)
		       && validate_suite(CHILD(tree, sui_pos)));

		if (res && (TYPE(CHILD(tree, nxt_pos)) == NAME)) {
		    res = ((groups == 0)
			   && validate_name(CHILD(tree, nxt_pos), "else"));
		}
		else if (res) {
		    res = (validate_ntype(CHILD(tree, nxt_pos), except_clause)
			   && validate_except_clause(CHILD(tree, nxt_pos)));
		}
		/* Update for next group. */
		cln_pos += 3;
		sui_pos += 3;
		nxt_pos += 3;
	    }
	}
    }
    else if (res) {
	res = ((nch == 6)
	       && validate_name(CHILD(tree, 3), "finally"));
    }
    return (res);

}   /* validate_try() */


VALIDATE(except_clause) {
    int nch = NCH(tree);
    int res = (((nch == 1) || (nch == 2) || (nch == 4))
	       && validate_name(CHILD(tree, 0), "except"));

    if (res && (nch > 1)) {
	res = (validate_ntype(CHILD(tree, 1), test)
	       && validate_test(CHILD(tree, 1)));
    }
    if (res && (nch == 4)) {
	res = (validate_comma(CHILD(tree, 2))
	       && validate_ntype(CHILD(tree, 3), test)
	       && validate_test(CHILD(tree, 3)));
    }
    return (res);

}   /* validate_except_clause() */


VALIDATE(test) {
    int nch = NCH(tree);
    int res = is_odd(nch);

    if (res && (TYPE(CHILD(tree, 0)) == lambdef)) {
	res = ((nch == 1)
	       && validate_lambdef(CHILD(tree, 0)));
    }
    else if (res) {
	int pos;

	res = (validate_ntype(CHILD(tree, 0), and_test)
	       && validate_and_test(CHILD(tree, 0)));

	for (pos = 1; res && (pos < nch); pos += 2) {
	    res = (validate_comma(CHILD(tree, pos))
		   && validate_ntype(CHILD(tree, pos + 1), and_test)
		   && validate_and_test(CHILD(tree, pos + 1)));
	}
    }
    return (res);

}   /* validate_test() */


VALIDATE(and_test) {
    int pos;
    int nch = NCH(tree);
    int res = (is_odd(nch)
	       && validate_ntype(CHILD(tree, 0), not_test)
	       && validate_not_test(CHILD(tree, 0)));

    for (pos = 1; res && (pos < nch); pos += 2) {
	res = (validate_name(CHILD(tree, pos), "and")
	       && validate_ntype(CHILD(tree, 0), not_test)
	       && validate_not_test(CHILD(tree, 0)));
    }
    return (res);

}   /* validate_and_test() */


VALIDATE(not_test) {
    int nch = NCH(tree);

    return (((nch == 2)
	     && validate_name(CHILD(tree, 0), "not")
	     && validate_ntype(CHILD(tree, 1), not_test)
	     && validate_not_test(CHILD(tree, 1)))
	    || ((nch == 1)
		&& validate_ntype(CHILD(tree, 0), comparison)
		&& validate_comparison(CHILD(tree, 0))));

}   /* validate_not_test() */


VALIDATE(comparison) {
    int pos;
    int nch = NCH(tree);
    int res = (is_odd(nch)
	       && validate_ntype(CHILD(tree, 0), expr)
	       && validate_expr(CHILD(tree, 0)));

    for (pos = 1; res && (pos < nch); pos += 2) {
	res = (validate_ntype(CHILD(tree, pos), comp_op)
	       && validate_comp_op(CHILD(tree, pos))
	       && validate_ntype(CHILD(tree, pos + 1), expr)
	       && validate_expr(CHILD(tree, 1)));
    }
    return (res);

}   /* validate_comparison() */


VALIDATE(comp_op) {
    int res = 0;
    int nch = NCH(tree);

    if (nch == 1) {
	/*
	 *  Only child will be a terminal with a well-defined symbolic name
	 *  or a NAME with a string of either 'is' or 'in'
	 */
	tree = CHILD(tree, 0);
	switch (TYPE(tree)) {
	    case LESS:
	    case GREATER:
	    case EQEQUAL:
	    case EQUAL:
	    case LESSEQUAL:
	    case GREATEREQUAL:
	    case NOTEQUAL:
	      res = 1;
	      break;
	    case NAME:
	      res = ((strcmp(STR(tree), "in") == 0)
		     || (strcmp(STR(tree), "is") == 0));
	      if (!res) {
		  char buffer[128];

		  sprintf(buffer, "Illegal comparison operator: '%s'.",
			  STR(tree));
		  PyErr_SetString(parser_error, buffer);
	      }
	      break;
	  default:
	      PyErr_SetString(parser_error,
			      "Illegal comparison operator type.");
	      break;
	}
    }
    else if (nch == 2) {
	res = (validate_ntype(CHILD(tree, 0), NAME)
	       && validate_ntype(CHILD(tree, 1), NAME)
	       && (((strcmp(STR(CHILD(tree, 0)), "is") == 0)
		    && (strcmp(STR(CHILD(tree, 1)), "not") == 0))
		   || ((strcmp(STR(CHILD(tree, 0)), "not") == 0)
		       && (strcmp(STR(CHILD(tree, 1)), "in") == 0))));
    }

    if (!res && !PyErr_Occurred()) {
	PyErr_SetString(parser_error, "Unknown comparison operator.");
    }
    return (res);

}   /* validate_comp_op() */


VALIDATE(expr) {
    int j;
    int nch = NCH(tree);
    int res = (is_odd(nch)
	       && validate_ntype(CHILD(tree, 0), xor_expr)
	       && validate_xor_expr(CHILD(tree, 0)));

    for (j = 2; res && (j < nch); j += 2) {
	res = (validate_ntype(CHILD(tree, j), xor_expr)
	       && validate_xor_expr(CHILD(tree, j))
	       && validate_vbar(CHILD(tree, j - 1)));
    }
    return (res);

}   /* validate_expr() */


VALIDATE(xor_expr) {
    int j;
    int nch = NCH(tree);
    int res = (is_odd(nch)
	       && validate_ntype(CHILD(tree, 0), and_expr)
	       && validate_and_expr(CHILD(tree, 0)));

    for (j = 2; res && (j < nch); j += 2) {
	res = (validate_circumflex(CHILD(tree, j - 1))
	       && validate_ntype(CHILD(tree, j), and_expr)
	       && validate_and_expr(CHILD(tree, j)));
    }
    return (res);

}   /* validate_xor_expr() */


VALIDATE(and_expr) {
    int pos;
    int nch = NCH(tree);
    int res = (is_odd(nch)
	       && validate_ntype(CHILD(tree, 0), shift_expr)
	       && validate_shift_expr(CHILD(tree, 0)));

    for (pos = 1; res && (pos < nch); pos += 2) {
	res = (validate_ampersand(CHILD(tree, pos))
	       && validate_ntype(CHILD(tree, pos + 1), shift_expr)
	       && validate_shift_expr(CHILD(tree, pos + 1)));
    }
    return (res);

}   /* validate_and_expr() */


static int
validate_chain_two_ops(tree, termtype, termvalid, op1, op2)
     node*	tree;
     int	termtype;
     int (*termvalid)(node*);
     int	op1, op2;
{
    int pos;
    int nch = NCH(tree);
    int res = (is_odd(nch)
	       && validate_ntype(CHILD(tree, 0), termtype)
	       && (*termvalid)(CHILD(tree, 0)));

    for (pos = 1; res && (pos < nch); pos += 2) {
	res = (((TYPE(CHILD(tree, pos)) == op1)
		|| validate_ntype(CHILD(tree, pos), op2))
	       && validate_ntype(CHILD(tree, pos + 1), termtype)
	       && (*termvalid)(CHILD(tree, pos + 1)));
    }
    return (res);

}   /* validate_chain_two_ops() */


VALIDATE(shift_expr) {

    return (validate_chain_two_ops(tree, arith_expr,
				   validate_arith_expr,
				   LEFTSHIFT, RIGHTSHIFT));

}   /* validate_shift_expr() */


VALIDATE(arith_expr) {

    return (validate_chain_two_ops(tree, term,
				   validate_term,
				   PLUS, MINUS));

}   /* validate_arith_expr() */


VALIDATE(term) {
    int pos;
    int nch = NCH(tree);
    int res = (is_odd(nch)
	       && validate_ntype(CHILD(tree, 0), factor)
	       && validate_factor(CHILD(tree, 0)));

    for (pos = 1; res && (pos < nch); pos += 2) {
	res= (((TYPE(CHILD(tree, pos)) == STAR)
	       || (TYPE(CHILD(tree, pos)) == SLASH)
	       || validate_ntype(CHILD(tree, pos), PERCENT))
	      && validate_ntype(CHILD(tree, pos + 1), factor)
	      && validate_factor(CHILD(tree, pos + 1)));
    }
    return (res);

}   /* validate_term() */


VALIDATE(factor) {
    int nch = NCH(tree);
    int res = (((nch == 2)
		&& ((TYPE(CHILD(tree, 0)) == PLUS)
		    || (TYPE(CHILD(tree, 0)) == MINUS)
		    || validate_ntype(CHILD(tree, 0), TILDE))
		&& validate_ntype(CHILD(tree, 1), factor)
		&& validate_factor(CHILD(tree, 1)))
	       || ((nch >= 1)
		   && validate_ntype(CHILD(tree, 0), atom)
		   && validate_atom(CHILD(tree, 0))));

    if (res && (TYPE(CHILD(tree, 0)) == atom)) {
	int pos;

	for (pos = 1; res && (pos < nch); ++pos) {
	    res = (validate_ntype(CHILD(tree, pos), trailer)
		   && validate_trailer(CHILD(tree, pos)));
	}
    }
    return (res);

}   /* validate_factor() */


VALIDATE(atom) {
    int pos;
    int nch = NCH(tree);
    int res = (nch >= 1);

    if (res) {
	switch (TYPE(CHILD(tree, 0))) {
	  case LPAR:
	    res = ((nch <= 3)
		   && (validate_rparen(CHILD(tree, nch - 1))));

	    if (res && (nch == 3)) {
		res = (validate_ntype(CHILD(tree, 1), testlist)
		       && validate_testlist(CHILD(tree, 1)));
	    }
	    break;
	  case LSQB:
	    res = ((nch <= 3)
		   && validate_ntype(CHILD(tree, nch - 1), RSQB));

	    if (res && (nch == 3)) {
		res = (validate_ntype(CHILD(tree, 1), testlist)
		       && validate_testlist(CHILD(tree, 1)));
	    }
	    break;
	  case LBRACE:
	    res = ((nch <= 3)
		   && validate_ntype(CHILD(tree, nch - 1), RBRACE));

	    if (res && (nch == 3)) {
		res = (validate_ntype(CHILD(tree, 1), dictmaker)
		       && validate_dictmaker(CHILD(tree, 1)));
	    }
	    break;
	  case BACKQUOTE:
	    res = ((nch == 3)
		   && validate_ntype(CHILD(tree, 1), testlist)
		   && validate_testlist(CHILD(tree, 1))
		   && validate_ntype(CHILD(tree, 2), BACKQUOTE));
	    break;
	  case NAME:
	  case NUMBER:
	    res = (nch == 1);
	    break;
	  case STRING:
	    for (pos = 1; res && (pos < nch); ++pos) {
		res = validate_ntype(CHILD(tree, pos), STRING);
	    }
	    break;
	  default:
	    res = 0;
	    break;
	}
    }
    return (res);

}   /* validate_atom() */


VALIDATE(funcdef) {

    return ((NCH(tree) == 5)
	    && validate_name(CHILD(tree, 0), "def")
	    && validate_ntype(CHILD(tree, 1), NAME)
	    && validate_ntype(CHILD(tree, 2), parameters)
	    && validate_colon(CHILD(tree, 3))
	    && validate_ntype(CHILD(tree, 4), suite)
	    && validate_parameters(CHILD(tree, 2))
	    && validate_suite(CHILD(tree, 4)));

}   /* validate_funcdef() */


VALIDATE(lambdef) {
    int nch = NCH(tree);
    int res = (((nch == 3) || (nch == 4))
	       && validate_name(CHILD(tree, 0), "lambda")
	       && validate_colon(CHILD(tree, nch - 2))
	       && validate_ntype(CHILD(tree, nch - 1), test)
	       && validate_testlist(CHILD(tree, nch - 1)));

    if (res && (nch == 4)) {
	res = (validate_ntype(CHILD(tree, 1), varargslist)
	       && validate_varargslist(CHILD(tree, 1)));
    }
    return (res);

}   /* validate_lambdef() */


VALIDATE(trailer) {
    int nch = NCH(tree);
    int res = ((nch == 2) || (nch == 3));

    if (res) {
	switch (TYPE(CHILD(tree, 0))) {
	  case LPAR:
	    res = validate_rparen(CHILD(tree, nch - 1));
	    if (res && (nch == 3)) {
		res = (validate_ntype(CHILD(tree, 1), testlist)
		       && validate_testlist(CHILD(tree, 1)));
	    }
	    break;
	  case LSQB:
	    res = ((nch == 3)
		   && validate_ntype(CHILD(tree, 1), subscript)
		   && validate_subscript(CHILD(tree, 1))
		   && validate_ntype(CHILD(tree, 2), RSQB));
	    break;
	  case DOT:
	    res = ((nch == 2)
		   && validate_ntype(CHILD(tree, 1), NAME));
	    break;
	  default:
	    res = 0;
	    break;
	}
    }
    return (res);

}   /* validate_trailer() */


VALIDATE(subscript) {
    int nch = NCH(tree);
    int res = ((nch >= 1) && (nch <= 3));

    if (res && is_odd(nch)) {
	res = (validate_ntype(CHILD(tree, 0), test)
	       && validate_test(CHILD(tree, 0)));

	if (res && (nch == 3)) {
	    res = (validate_colon(CHILD(tree, 1))
		   && validate_ntype(CHILD(tree, 2), test)
		   && validate_test(CHILD(tree, 2)));
	}
    }
    else if (res == 2) {
	if (TYPE(CHILD(tree, 0)) == COLON) {
	    res = (validate_ntype(CHILD(tree, 1), test)
		   && validate_test(CHILD(tree, 1)));
	}
	else {
	    res = (validate_ntype(CHILD(tree, 0), test)
		   && validate_test(CHILD(tree, 0))
		   && validate_colon(CHILD(tree, 1)));
	}
    }
    return (res);

}   /* validate_subscript() */


VALIDATE(exprlist) {
    int nch = NCH(tree);
    int res = ((nch >= 1)
	       && validate_ntype(CHILD(tree, 0), expr)
	       && validate_expr(CHILD(tree, 0)));

    if (res && is_even(nch)) {
	res = validate_comma(CHILD(tree, --nch));
    }
    if (res && (nch > 1)) {
	int pos;

	for (pos = 1; res && (pos < nch); pos += 2) {
	    res = (validate_comma(CHILD(tree, pos))
		   && validate_ntype(CHILD(tree, pos + 1), expr)
		   && validate_expr(CHILD(tree, pos + 1)));
	}
    }
    return (res);

}   /* validate_exprlist() */


VALIDATE(dictmaker) {
    int nch = NCH(tree);
    int res = ((nch >= 3)
	       && validate_ntype(CHILD(tree, 0), test)
	       && validate_test(CHILD(tree, 0))
	       && validate_colon(CHILD(tree, 1))
	       && validate_ntype(CHILD(tree, 2), test)
	       && validate_test(CHILD(tree, 2)));

    if (res && ((nch % 4) == 0)) {
	res = validate_comma(CHILD(tree, --nch));
    }
    else if (res) {
	res = ((nch % 4) == 3);
    }
    if (res && (nch > 3)) {
	int pos = 3;

	/* What's left are groups of:  ',' test ':' test  */
	while (res && (pos < nch)) {
	    res = (validate_comma(CHILD(tree, pos))
		   && validate_ntype(CHILD(tree, pos + 1), test)
		   && validate_test(CHILD(tree, pos + 1))
		   && validate_colon(CHILD(tree, pos + 2))
		   && validate_ntype(CHILD(tree, pos + 3), test)
		   && validate_test(CHILD(tree, pos + 3)));
	    pos += 4;
	}
    }
    return (res);

}   /* validate_dictmaker() */


VALIDATE(eval_input) {
    int pos;
    int nch = NCH(tree);
    int res = ((nch >= 2)
	       && validate_testlist(CHILD(tree, 0))
	       && validate_ntype(CHILD(tree, nch - 1), ENDMARKER));

    for (pos = 1; res && (pos < (nch - 1)); ++pos) {
	res = validate_ntype(CHILD(tree, pos), NEWLINE);
    }
    return (res);

}   /* validate_eval_input() */


VALIDATE(node) {
    int   nch  = 0;			/* num. children on current node  */
    int   res  = 1;			/* result value			  */
    node* next = 0;			/* node to process after this one */
    
    while (res & (tree != 0)) {
	nch  = NCH(tree);
	next = 0;
	switch (TYPE(tree)) {
	    /*
	     *  Definition nodes.
	     */
	  case funcdef:
	    res = validate_funcdef(tree);
	    break;
	  case classdef:
	    res = validate_class(tree);
	    break;
	    /*
	     *  "Trivial" parse tree nodes.
	     */
	  case stmt:
	    res = validate_stmt(tree);
	    break;
	  case small_stmt:
	    res = ((nch == 1)
		   && ((TYPE(CHILD(tree, 0)) == expr_stmt)
		       || (TYPE(CHILD(tree, 0)) == print_stmt)
		       || (TYPE(CHILD(tree, 0)) == del_stmt)
		       || (TYPE(CHILD(tree, 0)) == pass_stmt)
		       || (TYPE(CHILD(tree, 0)) == flow_stmt)
		       || (TYPE(CHILD(tree, 0)) == import_stmt)
		       || (TYPE(CHILD(tree, 0)) == global_stmt)
		       || (TYPE(CHILD(tree, 0)) == access_stmt)
		       || validate_ntype(CHILD(tree, 0), exec_stmt))
		   && (next = CHILD(tree, 0)));
	    break;
	  case flow_stmt:
	    res  = ((nch == 1)
		    && ((TYPE(CHILD(tree, 0)) == break_stmt)
			|| (TYPE(CHILD(tree, 0)) == continue_stmt)
			|| (TYPE(CHILD(tree, 0)) == return_stmt)
			|| validate_ntype(CHILD(tree, 0), raise_stmt))
		    && (next = CHILD(tree, 0)));
	    break;
	    /*
	     *  Compound statements.
	     */
	  case simple_stmt:
	    res = validate_simple_stmt(tree);
	    break;
	  case compound_stmt:
	    res = ((NCH(tree) == 1)
		   && ((TYPE(CHILD(tree, 0)) == if_stmt)
		       || (TYPE(CHILD(tree, 0)) == while_stmt)
		       || (TYPE(CHILD(tree, 0)) == for_stmt)
		       || (TYPE(CHILD(tree, 0)) == try_stmt)
		       || (TYPE(CHILD(tree, 0)) == funcdef)
		       || validate_ntype(CHILD(tree, 0), classdef))
		   && (next = CHILD(tree, 0)));
	    break;
	    /*
	     *  Fundemental statements.
	     */
	  case expr_stmt:
	    res = validate_expr_stmt(tree);
	    break;
	  case print_stmt:
	    res = validate_print_stmt(tree);
	    break;
	  case del_stmt:
	    res = validate_del_stmt(tree);
	    break;
	  case pass_stmt:
	    res = ((nch == 1)
		   && validate_name(CHILD(tree, 0), "pass"));
	    break;
	  case break_stmt:
	    res = ((nch == 1)
		   && validate_name(CHILD(tree, 0), "break"));
	    break;
	  case continue_stmt:
	    res = ((nch == 1)
		   && validate_name(CHILD(tree, 0), "continue"));
	    break;
	  case return_stmt:
	    res = validate_return_stmt(tree);
	    break;
	  case raise_stmt:
	    res = validate_raise_stmt(tree);
	    break;
	  case import_stmt:
	    res = validate_import_stmt(tree);
	    break;
	  case global_stmt:
	    res = validate_global_stmt(tree);
	    break;
	  case access_stmt:
	    res = validate_access_stmt(tree);
	    break;
	  case exec_stmt:
	    res = validate_exec_stmt(tree);
	    break;
	  case if_stmt:
	    res = validate_if(tree);
	    break;
	  case while_stmt:
	    res = validate_while(tree);
	    break;
	  case for_stmt:
	    res = validate_for(tree);
	    break;
	  case try_stmt:
	    res = validate_try(tree);
	    break;
	  case suite:
	    res = validate_suite(tree);
	    break;
	    /*
	     *  Expression nodes.
	     */
	  case testlist:
	    res = validate_testlist(tree);
	    break;
	  case test:
	    res = validate_test(tree);
	    break;
	  case and_test:
	    res = validate_and_test(tree);
	    break;
	  case not_test:
	    res = validate_not_test(tree);
	    break;
	  case comparison:
	    res = validate_comparison(tree);
	    break;
	  case exprlist:
	    res = validate_exprlist(tree);
	    break;
	  case expr:
	    res = validate_expr(tree);
	    break;
	  case xor_expr:
	    res = validate_xor_expr(tree);
	    break;
	  case and_expr:
	    res = validate_and_expr(tree);
	    break;
	  case shift_expr:
	    res = validate_shift_expr(tree);
	    break;
	  case arith_expr:
	    res = validate_arith_expr(tree);
	    break;
	  case term:
	    res = validate_term(tree);
	    break;
	  case factor:
	    res = validate_factor(tree);
	    break;
	  case atom:
	    res = validate_atom(tree);
	    break;
	    
	  default:
	    /* Hopefully never reached! */
	    res = 0;
	    break;
	}
	tree = next;
    }
    return (res);

}   /* validate_node() */


VALIDATE(expr_tree) {
    return (validate_ntype(tree, eval_input)
	    && validate_eval_input(tree));

}   /* validate_expr_tree() */


VALIDATE(suite_tree) {
    int j;
    int nch = NCH(tree);
    int res = ((nch >= 1)
	       && validate_ntype(CHILD(tree, nch - 1), ENDMARKER)
	       && nch--);

    for (j = 0; res && (j < nch); ++j) {
	res = ((TYPE(CHILD(tree, j)) == NEWLINE)
	       || (validate_ntype(CHILD(tree, j), stmt)
		   && validate_stmt(CHILD(tree, j))));
    }
    return (res);

}   /* validate_suite_tree() */



/*  Functions exported by this module.  Most of this should probably
 *  be converted into an AST object with methods, but that is better
 *  done directly in Python, allowing subclasses to be created directly.
 *  We'd really have to write a wrapper around it all anyway.
 *
 */
static PyMethodDef parser_functions[] =  {
    {"ast2tuple",	parser_ast2tuple,	1},
    {"compileast",	parser_compileast,	1},
    {"expr",		parser_expr,		1},
    {"isexpr",		parser_isexpr,		1},
    {"issuite",		parser_issuite,		1},
    {"suite",		parser_suite,		1},
    {"tuple2ast",	parser_tuple2ast,	1},

    {0, 0, 0}
    };



void
initparser() {
    PyObject* module = Py_InitModule("parser", parser_functions);
    PyObject* dict   = PyModule_GetDict(module);

    parser_error = PyString_FromString("parser.ParserError");

    if ((parser_error == 0)
	|| (PyDict_SetItemString(dict, "ParserError", parser_error) != 0)) {
	/*
	 *  This is serious.
	 */
	Py_FatalError("can't define parser.error");
    }
    /*
     *  Nice to have, but don't cry if we fail.
     */
    PyDict_SetItemString(dict, "__copyright__",
			 PyString_FromString(parser_copyright_string));
    PyDict_SetItemString(dict, "__doc__",
			 PyString_FromString(parser_doc_string));
    PyDict_SetItemString(dict, "__version__",
			 PyString_FromString(parser_version_string));

}   /* initparser() */


/*
 *  end of Parser.c
 */
