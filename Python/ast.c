/*
 * This file includes functions to transform a concrete syntax tree (CST) to
 * an abstract syntax tree (AST).  The main function is PyAST_FromNode().
 *
 */
#include "Python.h"
#include "Python-ast.h"
#include "grammar.h"
#include "node.h"
#include "ast.h"
#include "token.h"
#include "parsetok.h"
#include "graminit.h"

#include <assert.h>

#if 0
#define fprintf if (0) fprintf
#endif

/* XXX TO DO
   - re-indent this file (should be done)
   - internal error checking (freeing memory, etc.)
   - syntax errors
*/


/* Data structure used internally */
struct compiling {
	char *c_encoding; /* source encoding */
};

static asdl_seq *seq_for_testlist(struct compiling *, const node *);
static expr_ty ast_for_expr(struct compiling *, const node *);
static stmt_ty ast_for_stmt(struct compiling *, const node *);
static asdl_seq *ast_for_suite(struct compiling *, const node *);
static asdl_seq *ast_for_exprlist(struct compiling *, const node *, int);
static expr_ty ast_for_testlist(struct compiling *, const node *, int);

/* Note different signature for ast_for_call */
static expr_ty ast_for_call(struct compiling *, const node *, expr_ty);

static PyObject *parsenumber(const char *);
static PyObject *parsestr(const char *s, const char *encoding);
static PyObject *parsestrplus(struct compiling *, const node *n);

extern grammar _PyParser_Grammar; /* From graminit.c */

#ifndef LINENO
#define LINENO(n)	((n)->n_lineno)
#endif

#define NEW_IDENTIFIER(n) PyString_InternFromString(STR(n))

static void
asdl_stmt_seq_free(asdl_seq* seq)
{
    int n, i;

    if (!seq)
	return;
             
    n = asdl_seq_LEN(seq);
    for (i = 0; i < n; i++)
	free_stmt(asdl_seq_GET(seq, i));
    asdl_seq_free(seq);
}

static void
asdl_expr_seq_free(asdl_seq* seq)
{
    int n, i;

    if (!seq)
	return;
             
    n = asdl_seq_LEN(seq);
    for (i = 0; i < n; i++)
	free_expr(asdl_seq_GET(seq, i));
    asdl_seq_free(seq);
}

/* This routine provides an invalid object for the syntax error.
   The outermost routine must unpack this error and create the
   proper object.  We do this so that we don't have to pass
   the filename to everything function.

   XXX Maybe we should just pass the filename...
*/

static int
ast_error(const node *n, const char *errstr)
{
    PyObject *u = Py_BuildValue("zi", errstr, LINENO(n));
    if (!u)
	return 0;
    PyErr_SetObject(PyExc_SyntaxError, u);
    Py_DECREF(u);
    return 0;
}

static void
ast_error_finish(const char *filename)
{
    PyObject *type, *value, *tback, *errstr, *loc, *tmp;
    int lineno;

    assert(PyErr_Occurred());
    if (!PyErr_ExceptionMatches(PyExc_SyntaxError))
	return;

    PyErr_Fetch(&type, &value, &tback);
    errstr = PyTuple_GetItem(value, 0);
    if (!errstr)
	return;
    Py_INCREF(errstr);
    lineno = PyInt_AsLong(PyTuple_GetItem(value, 1));
    if (lineno == -1)
	return;
    Py_DECREF(value);

    loc = PyErr_ProgramText(filename, lineno);
    if (!loc) {
	Py_INCREF(Py_None);
	loc = Py_None;
    }
    tmp = Py_BuildValue("(ziOO)", filename, lineno, Py_None, loc);
    Py_DECREF(loc);
    if (!tmp)
	return;
    value = Py_BuildValue("(OO)", errstr, tmp);
    Py_DECREF(errstr);
    Py_DECREF(tmp);
    if (!value)
	return;
    PyErr_Restore(type, value, tback);
}

/* num_stmts() returns number of contained statements.

   Use this routine to determine how big a sequence is needed for
   the statements in a parse tree.  Its raison d'etre is this bit of
   grammar:

   stmt: simple_stmt | compound_stmt
   simple_stmt: small_stmt (';' small_stmt)* [';'] NEWLINE

   A simple_stmt can contain multiple small_stmt elements joined
   by semicolons.  If the arg is a simple_stmt, the number of
   small_stmt elements is returned.
*/

static int
num_stmts(const node *n)
{
    int i, l;
    node *ch;

    switch (TYPE(n)) {
        case single_input:
            if (TYPE(CHILD(n, 0)) == NEWLINE)
                return 0;
            else
                return num_stmts(CHILD(n, 0));
        case file_input:
            l = 0;
            for (i = 0; i < NCH(n); i++) {
                ch = CHILD(n, i);
                if (TYPE(ch) == stmt)
                    l += num_stmts(ch);
            }
            return l;
        case stmt:
            return num_stmts(CHILD(n, 0));
        case compound_stmt:
            return 1;
        case simple_stmt:
            return NCH(n) / 2; /* Divide by 2 to remove count of semi-colons */
        case suite:
            if (NCH(n) == 1)
                return num_stmts(CHILD(n, 0));
            else {
                l = 0;
                for (i = 2; i < (NCH(n) - 1); i++)
                    l += num_stmts(CHILD(n, i));
                return l;
            }
        default: {
            char buf[128];

            sprintf(buf, "Non-statement found: %d %d\n",
                    TYPE(n), NCH(n));
            Py_FatalError(buf);
        }
    }
    assert(0);
    return 0;
}

/* Transform the CST rooted at node * to the appropriate AST
*/

mod_ty
PyAST_FromNode(const node *n, PyCompilerFlags *flags, const char *filename)
{
    int i, j, num;
    asdl_seq *stmts = NULL;
    stmt_ty s;
    node *ch;
    struct compiling c;

    if (flags && flags->cf_flags & PyCF_SOURCE_IS_UTF8) {
            c.c_encoding = "utf-8";
    } else if (TYPE(n) == encoding_decl) {
        c.c_encoding = STR(n);
        n = CHILD(n, 0);
    } else {
        c.c_encoding = NULL;
    }

    switch (TYPE(n)) {
        case file_input:
            stmts = asdl_seq_new(num_stmts(n));
            if (!stmts)
                    return NULL;
            for (i = 0; i < NCH(n) - 1; i++) {
                ch = CHILD(n, i);
                if (TYPE(ch) == NEWLINE)
                    continue;
                REQ(ch, stmt);
                num = num_stmts(ch);
                if (num == 1) {
                    s = ast_for_stmt(&c, ch);
                    if (!s)
                        goto error;
                    asdl_seq_APPEND(stmts, s);
                }
                else {
                    ch = CHILD(ch, 0);
                    REQ(ch, simple_stmt);
                    for (j = 0; j < num; j++) {
                        s = ast_for_stmt(&c, CHILD(ch, j * 2));
                        if (!s)
                            goto error;
                        asdl_seq_APPEND(stmts, s);
                    }
                }
            }
            return Module(stmts);
        case eval_input: {
            expr_ty testlist_ast;

            /* XXX Why not gen_for here? */
            testlist_ast = ast_for_testlist(&c, CHILD(n, 0), 0);
            if (!testlist_ast)
                goto error;
            return Expression(testlist_ast);
        }
        case single_input:
            if (TYPE(CHILD(n, 0)) == NEWLINE) {
                stmts = asdl_seq_new(1);
                if (!stmts)
		    goto error;
                asdl_seq_SET(stmts, 0, Pass(n->n_lineno));
                return Interactive(stmts);
            }
            else {
                n = CHILD(n, 0);
                num = num_stmts(n);
                stmts = asdl_seq_new(num);
                if (!stmts)
		    goto error;
                if (num == 1) {
		    stmt_ty s = ast_for_stmt(&c, n);
		    if (!s)
			goto error;
                    asdl_seq_SET(stmts, 0, s);
                }
                else {
                    /* Only a simple_stmt can contain multiple statements. */
                    REQ(n, simple_stmt);
                    for (i = 0; i < NCH(n); i += 2) {
                        stmt_ty s;
                        if (TYPE(CHILD(n, i)) == NEWLINE)
                            break;
                        s = ast_for_stmt(&c, CHILD(n, i));
                        if (!s)
                            goto error;
                        asdl_seq_SET(stmts, i / 2, s);
                    }
                }

                return Interactive(stmts);
            }
        default:
            goto error;
    }
 error:
    if (stmts)
	asdl_stmt_seq_free(stmts);
    ast_error_finish(filename);
    return NULL;
}

/* Return the AST repr. of the operator represented as syntax (|, ^, etc.)
*/

static operator_ty
get_operator(const node *n)
{
    switch (TYPE(n)) {
        case VBAR:
            return BitOr;
        case CIRCUMFLEX:
            return BitXor;
        case AMPER:
            return BitAnd;
        case LEFTSHIFT:
            return LShift;
        case RIGHTSHIFT:
            return RShift;
        case PLUS:
            return Add;
        case MINUS:
            return Sub;
        case STAR:
            return Mult;
        case SLASH:
            return Div;
        case DOUBLESLASH:
            return FloorDiv;
        case PERCENT:
            return Mod;
        default:
            return 0;
    }
}

/* Set the context ctx for expr_ty e returning 0 on success, -1 on error.

   Only sets context for expr kinds that "can appear in assignment context"
   (according to ../Parser/Python.asdl).  For other expr kinds, it sets
   an appropriate syntax error and returns false.

   If e is a sequential type, items in sequence will also have their context
   set.

*/

static int
set_context(expr_ty e, expr_context_ty ctx, const node *n)
{
    asdl_seq *s = NULL;

    switch (e->kind) {
        case Attribute_kind:
	    if (ctx == Store &&
		!strcmp(PyString_AS_STRING(e->v.Attribute.attr), "None")) {
		    return ast_error(n, "assignment to None");
	    }
	    e->v.Attribute.ctx = ctx;
	    break;
        case Subscript_kind:
	    e->v.Subscript.ctx = ctx;
	    break;
        case Name_kind:
	    if (ctx == Store &&
		!strcmp(PyString_AS_STRING(e->v.Name.id), "None")) {
		    return ast_error(n, "assignment to None");
	    }
	    e->v.Name.ctx = ctx;
	    break;
        case List_kind:
	    e->v.List.ctx = ctx;
	    s = e->v.List.elts;
	    break;
        case Tuple_kind:
            if (asdl_seq_LEN(e->v.Tuple.elts) == 0) 
                return ast_error(n, "can't assign to ()");
	    e->v.Tuple.ctx = ctx;
	    s = e->v.Tuple.elts;
	    break;
        case Call_kind:
	    if (ctx == Store)
		return ast_error(n, "can't assign to function call");
	    else if (ctx == Del)
		return ast_error(n, "can't delete function call");
	    else
		return ast_error(n, "unexpected operation on function call");
	    break;
        case BinOp_kind:
            return ast_error(n, "can't assign to operator");
        case GeneratorExp_kind:
            return ast_error(n, "assignment to generator expression "
                             "not possible");
        case Num_kind:
        case Str_kind:
	    return ast_error(n, "can't assign to literal");
        default: {
	   char buf[300];
	   PyOS_snprintf(buf, sizeof(buf), 
			 "unexpected expression in assignment %d (line %d)", 
			 e->kind, e->lineno);
	   return ast_error(n, buf);
       }
    }
    /* If the LHS is a list or tuple, we need to set the assignment
       context for all the tuple elements.  
    */
    if (s) {
	int i;

	for (i = 0; i < asdl_seq_LEN(s); i++) {
	    if (!set_context(asdl_seq_GET(s, i), ctx, n))
		return 0;
	}
    }
    return 1;
}

static operator_ty
ast_for_augassign(const node *n)
{
    REQ(n, augassign);
    n = CHILD(n, 0);
    switch (STR(n)[0]) {
        case '+':
            return Add;
        case '-':
            return Sub;
        case '/':
            if (STR(n)[1] == '/')
                return FloorDiv;
            else
                return Div;
        case '%':
            return Mod;
        case '<':
            return LShift;
        case '>':
            return RShift;
        case '&':
            return BitAnd;
        case '^':
            return BitXor;
        case '|':
            return BitOr;
        case '*':
            if (STR(n)[1] == '*')
                return Pow;
            else
                return Mult;
        default:
            PyErr_Format(PyExc_Exception, "invalid augassign: %s", STR(n));
            return 0;
    }
}

static cmpop_ty
ast_for_comp_op(const node *n)
{
    /* comp_op: '<'|'>'|'=='|'>='|'<='|'<>'|'!='|'in'|'not' 'in'|'is'
               |'is' 'not'
    */
    REQ(n, comp_op);
    if (NCH(n) == 1) {
	n = CHILD(n, 0);
	switch (TYPE(n)) {
            case LESS:
                return Lt;
            case GREATER:
                return Gt;
            case EQEQUAL:			/* == */
            case EQUAL:
                return Eq;
            case LESSEQUAL:
                return LtE;
            case GREATEREQUAL:
                return GtE;
            case NOTEQUAL:
                return NotEq;
            case NAME:
                if (strcmp(STR(n), "in") == 0)
                    return In;
                if (strcmp(STR(n), "is") == 0)
                    return Is;
            default:
                PyErr_Format(PyExc_Exception, "invalid comp_op: %s",
                             STR(n));
                return 0;
	}
    }
    else if (NCH(n) == 2) {
	/* handle "not in" and "is not" */
	switch (TYPE(CHILD(n, 0))) {
            case NAME:
                if (strcmp(STR(CHILD(n, 1)), "in") == 0)
                    return NotIn;
                if (strcmp(STR(CHILD(n, 0)), "is") == 0)
                    return IsNot;
            default:
                PyErr_Format(PyExc_Exception, "invalid comp_op: %s %s",
                             STR(CHILD(n, 0)), STR(CHILD(n, 1)));
                return 0;
	}
    }
    PyErr_Format(PyExc_Exception, "invalid comp_op: has %d children",
                 NCH(n));
    return 0;
}

static asdl_seq *
seq_for_testlist(struct compiling *c, const node *n)
{
    /* testlist: test (',' test)* [','] */
    assert(TYPE(n) == testlist
	   || TYPE(n) == listmaker
	   || TYPE(n) == testlist_gexp
	   || TYPE(n) == testlist_safe
	   );
    asdl_seq *seq;
    expr_ty expression;
    int i;

    seq = asdl_seq_new((NCH(n) + 1) / 2);
    if (!seq)
        return NULL;

    for (i = 0; i < NCH(n); i += 2) {
        REQ(CHILD(n, i), test);

        expression = ast_for_expr(c, CHILD(n, i));
        if (!expression) {
            asdl_seq_free(seq);
            return NULL;
        }

        assert(i / 2 < seq->size);
        asdl_seq_SET(seq, i / 2, expression);
    }
    return seq;
}

static expr_ty
compiler_complex_args(const node *n)
{
    int i, len = (NCH(n) + 1) / 2;
    expr_ty result;
    asdl_seq *args = asdl_seq_new(len);
    if (!args)
        return NULL;

    REQ(n, fplist);

    for (i = 0; i < len; i++) {
        const node *child = CHILD(CHILD(n, 2*i), 0);
        expr_ty arg;
        if (TYPE(child) == NAME) {
		if (!strcmp(STR(child), "None")) {
			ast_error(child, "assignment to None");
			return NULL;
		}
            arg = Name(NEW_IDENTIFIER(child), Store, LINENO(child));
	}
        else
            arg = compiler_complex_args(CHILD(CHILD(n, 2*i), 1));
	set_context(arg, Store, n);
        asdl_seq_SET(args, i, arg);
    }

    result = Tuple(args, Store, LINENO(n));
    set_context(result, Store, n);
    return result;
}

/* Create AST for argument list.

   XXX TO DO:
       - check for invalid argument lists like normal after default
*/

static arguments_ty
ast_for_arguments(struct compiling *c, const node *n)
{
    /* parameters: '(' [varargslist] ')'
       varargslist: (fpdef ['=' test] ',')* ('*' NAME [',' '**' NAME]
            | '**' NAME) | fpdef ['=' test] (',' fpdef ['=' test])* [',']
    */
    int i, n_args = 0, n_defaults = 0, found_default = 0;
    asdl_seq *args, *defaults;
    identifier vararg = NULL, kwarg = NULL;
    node *ch;

    if (TYPE(n) == parameters) {
	if (NCH(n) == 2) /* () as argument list */
	    return arguments(NULL, NULL, NULL, NULL);
	n = CHILD(n, 1);
    }
    REQ(n, varargslist);

    /* first count the number of normal args & defaults */
    for (i = 0; i < NCH(n); i++) {
	ch = CHILD(n, i);
	if (TYPE(ch) == fpdef) {
	    n_args++;
	}
	if (TYPE(ch) == EQUAL)
	    n_defaults++;
    }
    args = (n_args ? asdl_seq_new(n_args) : NULL);
    if (!args && n_args)
    	return NULL; /* Don't need to go to NULL; nothing allocated */
    defaults = (n_defaults ? asdl_seq_new(n_defaults) : NULL);
    if (!defaults && n_defaults)
        goto error;

    /* fpdef: NAME | '(' fplist ')'
       fplist: fpdef (',' fpdef)* [',']
    */
    i = 0;
    while (i < NCH(n)) {
	ch = CHILD(n, i);
	switch (TYPE(ch)) {
            case fpdef:
                /* XXX Need to worry about checking if TYPE(CHILD(n, i+1)) is
                   anything other than EQUAL or a comma? */
                /* XXX Should NCH(n) check be made a separate check? */
                if (i + 1 < NCH(n) && TYPE(CHILD(n, i + 1)) == EQUAL) {
                    asdl_seq_APPEND(defaults, 
				    ast_for_expr(c, CHILD(n, i + 2)));
                    i += 2;
		    found_default = 1;
                }
		else if (found_default) {
		    ast_error(n, 
			     "non-default argument follows default argument");
		    goto error;
		}

                if (NCH(ch) == 3) {
                    asdl_seq_APPEND(args, 
                                    compiler_complex_args(CHILD(ch, 1))); 
		}
                else if (TYPE(CHILD(ch, 0)) == NAME) {
		    if (!strcmp(STR(CHILD(ch, 0)), "None")) {
			    ast_error(CHILD(ch, 0), "assignment to None");
			    goto error;
		    }
                    expr_ty name = Name(NEW_IDENTIFIER(CHILD(ch, 0)),
                                        Param, LINENO(ch));
                    if (!name)
                        goto error;
                    asdl_seq_APPEND(args, name);
					 
		}
                i += 2; /* the name and the comma */
                break;
            case STAR:
		if (!strcmp(STR(CHILD(n, i+1)), "None")) {
			ast_error(CHILD(n, i+1), "assignment to None");
			goto error;
		}
                vararg = NEW_IDENTIFIER(CHILD(n, i+1));
                i += 3;
                break;
            case DOUBLESTAR:
		if (!strcmp(STR(CHILD(n, i+1)), "None")) {
			ast_error(CHILD(n, i+1), "assignment to None");
			goto error;
		}
                kwarg = NEW_IDENTIFIER(CHILD(n, i+1));
                i += 3;
                break;
            default:
                PyErr_Format(PyExc_Exception,
                             "unexpected node in varargslist: %d @ %d",
                             TYPE(ch), i);
                goto error;
	}
    }

    return arguments(args, vararg, kwarg, defaults);

 error:
    if (args)
        asdl_seq_free(args);
    if (defaults)
        asdl_seq_free(defaults);
    return NULL;
}

static expr_ty
ast_for_dotted_name(struct compiling *c, const node *n)
{
    expr_ty e = NULL;
    expr_ty attrib = NULL;
    identifier id = NULL;
    int i;

    REQ(n, dotted_name);
    
    id = NEW_IDENTIFIER(CHILD(n, 0));
    if (!id)
        goto error;
    e = Name(id, Load, LINENO(n));
    if (!e)
	goto error;
    id = NULL;

    for (i = 2; i < NCH(n); i+=2) {
        id = NEW_IDENTIFIER(CHILD(n, i));
	if (!id)
	    goto error;
	attrib = Attribute(e, id, Load, LINENO(CHILD(n, i)));
	if (!attrib)
	    goto error;
	e = attrib;
	attrib = NULL;
    }

    return e;
    
  error:
    Py_XDECREF(id);
    free_expr(e);
    return NULL;
}

static expr_ty
ast_for_decorator(struct compiling *c, const node *n)
{
    /* decorator: '@' dotted_name [ '(' [arglist] ')' ] NEWLINE */
    expr_ty d = NULL;
    expr_ty name_expr = NULL;
    
    REQ(n, decorator);
    
    if ((NCH(n) < 3 && NCH(n) != 5 && NCH(n) != 6)
	|| TYPE(CHILD(n, 0)) != AT || TYPE(RCHILD(n, -1)) != NEWLINE) {
	ast_error(n, "Invalid decorator node");
	goto error;
    }
    
    name_expr = ast_for_dotted_name(c, CHILD(n, 1));
    if (!name_expr)
	goto error;
	
    if (NCH(n) == 3) { /* No arguments */
	d = name_expr;
	name_expr = NULL;
    }
    else if (NCH(n) == 5) { /* Call with no arguments */
	d = Call(name_expr, NULL, NULL, NULL, NULL, LINENO(n));
	if (!d)
	    goto error;
	name_expr = NULL;
    }
    else {
	d = ast_for_call(c, CHILD(n, 3), name_expr);
	if (!d)
	    goto error;
	name_expr = NULL;
    }

    return d;
    
  error:
    free_expr(name_expr);
    free_expr(d);
    return NULL;
}

static asdl_seq*
ast_for_decorators(struct compiling *c, const node *n)
{
    asdl_seq* decorator_seq = NULL;
    expr_ty d = NULL;
    int i;
    
    REQ(n, decorators);

    decorator_seq = asdl_seq_new(NCH(n));
    if (!decorator_seq)
        return NULL;
	
    for (i = 0; i < NCH(n); i++) {
	d = ast_for_decorator(c, CHILD(n, i));
	if (!d)
	    goto error;
	asdl_seq_APPEND(decorator_seq, d);
	d = NULL;
    }
    return decorator_seq;
  error:
    asdl_expr_seq_free(decorator_seq);
    free_expr(d);
    return NULL;
}

static stmt_ty
ast_for_funcdef(struct compiling *c, const node *n)
{
    /* funcdef: 'def' [decorators] NAME parameters ':' suite */
    identifier name = NULL;
    arguments_ty args = NULL;
    asdl_seq *body = NULL;
    asdl_seq *decorator_seq = NULL;
    int name_i;

    REQ(n, funcdef);

    if (NCH(n) == 6) { /* decorators are present */
	decorator_seq = ast_for_decorators(c, CHILD(n, 0));
	if (!decorator_seq)
	    goto error;
	name_i = 2;
    }
    else {
	name_i = 1;
    }

    name = NEW_IDENTIFIER(CHILD(n, name_i));
    if (!name)
	goto error;
    else if (!strcmp(STR(CHILD(n, name_i)), "None")) {
	    ast_error(CHILD(n, name_i), "assignment to None");
	    goto error;
    }
    args = ast_for_arguments(c, CHILD(n, name_i + 1));
    if (!args)
	goto error;
    body = ast_for_suite(c, CHILD(n, name_i + 3));
    if (!body)
	goto error;

    return FunctionDef(name, args, body, decorator_seq, LINENO(n));

error:
    asdl_stmt_seq_free(body);
    asdl_expr_seq_free(decorator_seq);
    free_arguments(args);
    Py_XDECREF(name);
    return NULL;
}

static expr_ty
ast_for_lambdef(struct compiling *c, const node *n)
{
    /* lambdef: 'lambda' [varargslist] ':' test */
    arguments_ty args;
    expr_ty expression;

    if (NCH(n) == 3) {
        args = arguments(NULL, NULL, NULL, NULL);
        if (!args)
            return NULL;
        expression = ast_for_expr(c, CHILD(n, 2));
        if (!expression) {
            free_arguments(args);
            return NULL;
        }
    }
    else {
        args = ast_for_arguments(c, CHILD(n, 1));
        if (!args)
            return NULL;
        expression = ast_for_expr(c, CHILD(n, 3));
        if (!expression) {
            free_arguments(args);
            return NULL;
        }
    }

    return Lambda(args, expression, LINENO(n));
}

/* Count the number of 'for' loop in a list comprehension.

   Helper for ast_for_listcomp().
*/

static int
count_list_fors(const node *n)
{
    int n_fors = 0;
    node *ch = CHILD(n, 1);

 count_list_for:
    n_fors++;
    REQ(ch, list_for);
    if (NCH(ch) == 5)
	ch = CHILD(ch, 4);
    else
	return n_fors;
 count_list_iter:
    REQ(ch, list_iter);
    ch = CHILD(ch, 0);
    if (TYPE(ch) == list_for)
	goto count_list_for;
    else if (TYPE(ch) == list_if) {
        if (NCH(ch) == 3) {
            ch = CHILD(ch, 2);
            goto count_list_iter;
        }
        else
            return n_fors;
    }
    else {
        /* Should never be reached */
        PyErr_SetString(PyExc_Exception, "logic error in count_list_fors");
        return -1;
    }
}

/* Count the number of 'if' statements in a list comprehension.

   Helper for ast_for_listcomp().
*/

static int
count_list_ifs(const node *n)
{
    int n_ifs = 0;

 count_list_iter:
    REQ(n, list_iter);
    if (TYPE(CHILD(n, 0)) == list_for)
	return n_ifs;
    n = CHILD(n, 0);
    REQ(n, list_if);
    n_ifs++;
    if (NCH(n) == 2)
	return n_ifs;
    n = CHILD(n, 2);
    goto count_list_iter;
}

static expr_ty
ast_for_listcomp(struct compiling *c, const node *n)
{
    /* listmaker: test ( list_for | (',' test)* [','] )
       list_for: 'for' exprlist 'in' testlist_safe [list_iter]
       list_iter: list_for | list_if
       list_if: 'if' test [list_iter]
       testlist_safe: test [(',' test)+ [',']]
    */
    expr_ty elt;
    asdl_seq *listcomps;
    int i, n_fors;
    node *ch;

    REQ(n, listmaker);
    assert(NCH(n) > 1);

    elt = ast_for_expr(c, CHILD(n, 0));
    if (!elt)
        return NULL;

    n_fors = count_list_fors(n);
    if (n_fors == -1)
        return NULL;

    listcomps = asdl_seq_new(n_fors);
    if (!listcomps) {
        free_expr(elt);
    	return NULL;
    }
    
    ch = CHILD(n, 1);
    for (i = 0; i < n_fors; i++) {
	comprehension_ty lc;
	asdl_seq *t;
        expr_ty expression;

	REQ(ch, list_for);

	t = ast_for_exprlist(c, CHILD(ch, 1), Store);
        if (!t) {
            asdl_seq_free(listcomps);
            free_expr(elt);
            return NULL;
        }
        expression = ast_for_testlist(c, CHILD(ch, 3), 0);
        if (!expression) {
            asdl_seq_free(t);
            asdl_seq_free(listcomps);
            free_expr(elt);
            return NULL;
        }

	if (asdl_seq_LEN(t) == 1)
	    lc = comprehension(asdl_seq_GET(t, 0), expression, NULL);
	else
	    lc = comprehension(Tuple(t, Store, LINENO(ch)), expression, NULL);

        if (!lc) {
            asdl_seq_free(t);
            asdl_seq_free(listcomps);
            free_expr(expression);
            free_expr(elt);
            return NULL;
        }

	if (NCH(ch) == 5) {
	    int j, n_ifs;
	    asdl_seq *ifs;

	    ch = CHILD(ch, 4);
	    n_ifs = count_list_ifs(ch);
            if (n_ifs == -1) {
                asdl_seq_free(listcomps);
                free_expr(elt);
                return NULL;
            }

	    ifs = asdl_seq_new(n_ifs);
	    if (!ifs) {
		asdl_seq_free(listcomps);
                free_expr(elt);
		return NULL;
	    }

	    for (j = 0; j < n_ifs; j++) {
		REQ(ch, list_iter);

		ch = CHILD(ch, 0);
		REQ(ch, list_if);

		asdl_seq_APPEND(ifs, ast_for_expr(c, CHILD(ch, 1)));
		if (NCH(ch) == 3)
		    ch = CHILD(ch, 2);
	    }
	    /* on exit, must guarantee that ch is a list_for */
	    if (TYPE(ch) == list_iter)
		ch = CHILD(ch, 0);
            lc->ifs = ifs;
	}
	asdl_seq_APPEND(listcomps, lc);
    }

    return ListComp(elt, listcomps, LINENO(n));
}

/*
   Count the number of 'for' loops in a generator expression.

   Helper for ast_for_genexp().
*/

static int
count_gen_fors(const node *n)
{
	int n_fors = 0;
	node *ch = CHILD(n, 1);

 count_gen_for:
	n_fors++;
	REQ(ch, gen_for);
	if (NCH(ch) == 5)
		ch = CHILD(ch, 4);
	else
		return n_fors;
 count_gen_iter:
	REQ(ch, gen_iter);
	ch = CHILD(ch, 0);
	if (TYPE(ch) == gen_for)
		goto count_gen_for;
	else if (TYPE(ch) == gen_if) {
		if (NCH(ch) == 3) {
			ch = CHILD(ch, 2);
			goto count_gen_iter;
		}
		else
		    return n_fors;
	}
	else {
		/* Should never be reached */
		PyErr_SetString(PyExc_Exception, "logic error in count_gen_fors");
		return -1;
	}
}

/* Count the number of 'if' statements in a generator expression.

   Helper for ast_for_genexp().
*/

static int
count_gen_ifs(const node *n)
{
	int n_ifs = 0;

	while (1) {
		REQ(n, gen_iter);
		if (TYPE(CHILD(n, 0)) == gen_for)
			return n_ifs;
		n = CHILD(n, 0);
		REQ(n, gen_if);
		n_ifs++;
		if (NCH(n) == 2)
			return n_ifs;
		n = CHILD(n, 2);
	}
}

static expr_ty
ast_for_genexp(struct compiling *c, const node *n)
{
    /* testlist_gexp: test ( gen_for | (',' test)* [','] )
       argument: [test '='] test [gen_for]	 # Really [keyword '='] test */
    expr_ty elt;
    asdl_seq *genexps;
    int i, n_fors;
    node *ch;
    
    assert(TYPE(n) == (testlist_gexp) || TYPE(n) == (argument));
    assert(NCH(n) > 1);
    
    elt = ast_for_expr(c, CHILD(n, 0));
    if (!elt)
        return NULL;
    
    n_fors = count_gen_fors(n);
    if (n_fors == -1)
        return NULL;
    
    genexps = asdl_seq_new(n_fors);
    if (!genexps) {
        free_expr(elt);
        return NULL;
    }
    
    ch = CHILD(n, 1);
    for (i = 0; i < n_fors; i++) {
        comprehension_ty ge;
        asdl_seq *t;
        expr_ty expression;
        
        REQ(ch, gen_for);
        
        t = ast_for_exprlist(c, CHILD(ch, 1), Store);
        if (!t) {
            asdl_seq_free(genexps);
            free_expr(elt);
            return NULL;
        }
        expression = ast_for_testlist(c, CHILD(ch, 3), 1);
        if (!expression) {
            asdl_seq_free(genexps);
            free_expr(elt);
            return NULL;
        }
        
        if (asdl_seq_LEN(t) == 1)
            ge = comprehension(asdl_seq_GET(t, 0), expression,
                               NULL);
        else
            ge = comprehension(Tuple(t, Store, LINENO(ch)),
                               expression, NULL);
        
        if (!ge) {
            asdl_seq_free(genexps);
            free_expr(elt);
            return NULL;
        }
        
        if (NCH(ch) == 5) {
            int j, n_ifs;
            asdl_seq *ifs;
            
            ch = CHILD(ch, 4);
            n_ifs = count_gen_ifs(ch);
            if (n_ifs == -1) {
                asdl_seq_free(genexps);
                free_expr(elt);
                return NULL;
            }
            
            ifs = asdl_seq_new(n_ifs);
            if (!ifs) {
                asdl_seq_free(genexps);
                free_expr(elt);
                return NULL;
            }
            
            for (j = 0; j < n_ifs; j++) {
                REQ(ch, gen_iter);
                ch = CHILD(ch, 0);
                REQ(ch, gen_if);
                
                asdl_seq_APPEND(ifs, ast_for_expr(c, CHILD(ch, 1)));
                if (NCH(ch) == 3)
                    ch = CHILD(ch, 2);
            }
            /* on exit, must guarantee that ch is a gen_for */
            if (TYPE(ch) == gen_iter)
                ch = CHILD(ch, 0);
            ge->ifs = ifs;
        }
        asdl_seq_APPEND(genexps, ge);
    }
    
    return GeneratorExp(elt, genexps, LINENO(n));
}

static expr_ty
ast_for_atom(struct compiling *c, const node *n)
{
    /* atom: '(' [yield_expr|testlist_gexp] ')' | '[' [listmaker] ']'
       | '{' [dictmaker] '}' | '`' testlist '`' | NAME | NUMBER | STRING+
    */
    node *ch = CHILD(n, 0);
    
    switch (TYPE(ch)) {
    case NAME:
	/* All names start in Load context, but may later be
	   changed. */
	return Name(NEW_IDENTIFIER(ch), Load, LINENO(n));
    case STRING: {
	PyObject *str = parsestrplus(c, n);
	
	if (!str)
	    return NULL;
	
	return Str(str, LINENO(n));
    }
    case NUMBER: {
	PyObject *pynum = parsenumber(STR(ch));
	
	if (!pynum)
	    return NULL;
	
	return Num(pynum, LINENO(n));
    }
    case LPAR: /* some parenthesized expressions */
	ch = CHILD(n, 1);
	
	if (TYPE(ch) == RPAR)
	    return Tuple(NULL, Load, LINENO(n));
	
	if (TYPE(ch) == yield_expr)
	    return ast_for_expr(c, ch);
	
	if ((NCH(ch) > 1) && (TYPE(CHILD(ch, 1)) == gen_for))
	    return ast_for_genexp(c, ch);
	
	return ast_for_testlist(c, ch, 1);
    case LSQB: /* list (or list comprehension) */
	ch = CHILD(n, 1);
	
	if (TYPE(ch) == RSQB)
	    return List(NULL, Load, LINENO(n));
	
	REQ(ch, listmaker);
	if (NCH(ch) == 1 || TYPE(CHILD(ch, 1)) == COMMA) {
	    asdl_seq *elts = seq_for_testlist(c, ch);
	    
	    if (!elts)
		return NULL;
	    
	    return List(elts, Load, LINENO(n));
	}
	else
	    return ast_for_listcomp(c, ch);
    case LBRACE: {
	/* dictmaker: test ':' test (',' test ':' test)* [','] */
	int i, size;
	asdl_seq *keys, *values;
	
	ch = CHILD(n, 1);
	size = (NCH(ch) + 1) / 4; /* +1 in case no trailing comma */
	keys = asdl_seq_new(size);
	if (!keys)
	    return NULL;
	
	values = asdl_seq_new(size);
	if (!values) {
	    asdl_seq_free(keys);
	    return NULL;
	}
	
	for (i = 0; i < NCH(ch); i += 4) {
	    expr_ty expression;
	    
	    expression = ast_for_expr(c, CHILD(ch, i));
	    if (!expression)
		return NULL;
	    
	    asdl_seq_SET(keys, i / 4, expression);
	    
	    expression = ast_for_expr(c, CHILD(ch, i + 2));
	    if (!expression)
		return NULL;
	    
	    asdl_seq_SET(values, i / 4, expression);
	}
	return Dict(keys, values, LINENO(n));
    }
    case BACKQUOTE: { /* repr */
	expr_ty expression = ast_for_testlist(c, CHILD(n, 1), 0);
	
	if (!expression)
	    return NULL;
	
	return Repr(expression, LINENO(n));
    }
    default:
	PyErr_Format(PyExc_Exception, "unhandled atom %d",
		     TYPE(ch));
	return NULL;
    }
}

static slice_ty
ast_for_slice(struct compiling *c, const node *n)
{
    node *ch;
    expr_ty lower = NULL, upper = NULL, step = NULL;

    REQ(n, subscript);

    /*
       subscript: '.' '.' '.' | test | [test] ':' [test] [sliceop]
       sliceop: ':' [test]
    */
    ch = CHILD(n, 0);
    if (TYPE(ch) == DOT)
	return Ellipsis();

    if (NCH(n) == 1 && TYPE(ch) == test) {
        /* 'step' variable hold no significance in terms of being used over
           other vars */
        step = ast_for_expr(c, ch); 
        if (!step)
            return NULL;
            
	return Index(step);
    }

    if (TYPE(ch) == test) {
	lower = ast_for_expr(c, ch);
        if (!lower)
            return NULL;
    }

    /* If there's an upper bound it's in the second or third position. */
    if (TYPE(ch) == COLON) {
	if (NCH(n) > 1) {
	    node *n2 = CHILD(n, 1);

	    if (TYPE(n2) == test) {
		upper = ast_for_expr(c, n2);
                if (!upper)
                    return NULL;
            }
	}
    } else if (NCH(n) > 2) {
	node *n2 = CHILD(n, 2);

	if (TYPE(n2) == test) {
	    upper = ast_for_expr(c, n2);
            if (!upper)
                return NULL;
        }
    }

    ch = CHILD(n, NCH(n) - 1);
    if (TYPE(ch) == sliceop) {
	if (NCH(ch) == 1)
            /* XXX: If only 1 child, then should just be a colon.  Should we
               just skip assigning and just get to the return? */
	    ch = CHILD(ch, 0);
	else
	    ch = CHILD(ch, 1);
	if (TYPE(ch) == test) {
	    step = ast_for_expr(c, ch);
            if (!step)
                return NULL;
        }
    }

    return Slice(lower, upper, step);
}

static expr_ty
ast_for_binop(struct compiling *c, const node *n)
{
	/* Must account for a sequence of expressions.
	   How should A op B op C by represented?  
	   BinOp(BinOp(A, op, B), op, C).
	*/

	int i, nops;
	expr_ty expr1, expr2, result;
        operator_ty operator;

        expr1 = ast_for_expr(c, CHILD(n, 0));
        if (!expr1)
            return NULL;

        expr2 = ast_for_expr(c, CHILD(n, 2));
        if (!expr2)
            return NULL;

        operator = get_operator(CHILD(n, 1));
        if (!operator)
            return NULL;

	result = BinOp(expr1, operator, expr2, LINENO(n));
	if (!result)
            return NULL;

	nops = (NCH(n) - 1) / 2;
	for (i = 1; i < nops; i++) {
		expr_ty tmp_result, tmp;
		const node* next_oper = CHILD(n, i * 2 + 1);

		operator = get_operator(next_oper);
                if (!operator)
                    return NULL;

                tmp = ast_for_expr(c, CHILD(n, i * 2 + 2));
                if (!tmp)
                    return NULL;

                tmp_result = BinOp(result, operator, tmp, 
				   LINENO(next_oper));
		if (!tmp) 
			return NULL;
		result = tmp_result;
	}
	return result;
}

/* Do not name a variable 'expr'!  Will cause a compile error.
*/

static expr_ty
ast_for_expr(struct compiling *c, const node *n)
{
    /* handle the full range of simple expressions
       test: and_test ('or' and_test)* | lambdef
       and_test: not_test ('and' not_test)*
       not_test: 'not' not_test | comparison
       comparison: expr (comp_op expr)*
       expr: xor_expr ('|' xor_expr)*
       xor_expr: and_expr ('^' and_expr)*
       and_expr: shift_expr ('&' shift_expr)*
       shift_expr: arith_expr (('<<'|'>>') arith_expr)*
       arith_expr: term (('+'|'-') term)*
       term: factor (('*'|'/'|'%'|'//') factor)*
       factor: ('+'|'-'|'~') factor | power
       power: atom trailer* ('**' factor)*
    */

    asdl_seq *seq;
    int i;

 loop:
    switch (TYPE(n)) {
        case test:
            if (TYPE(CHILD(n, 0)) == lambdef)
                return ast_for_lambdef(c, CHILD(n, 0));
            /* Fall through to and_test */
        case and_test:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            seq = asdl_seq_new((NCH(n) + 1) / 2);
            if (!seq)
                return NULL;
            for (i = 0; i < NCH(n); i += 2) {
                expr_ty e = ast_for_expr(c, CHILD(n, i));
                if (!e)
                    return NULL;
                asdl_seq_SET(seq, i / 2, e);
            }
            if (!strcmp(STR(CHILD(n, 1)), "and"))
                return BoolOp(And, seq, LINENO(n));
            else {
                assert(!strcmp(STR(CHILD(n, 1)), "or"));
                return BoolOp(Or, seq, LINENO(n));
            }
            break;
        case not_test:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            else {
                expr_ty expression = ast_for_expr(c, CHILD(n, 1));
                if (!expression)
                    return NULL;

                return UnaryOp(Not, expression, LINENO(n));
            }
        case comparison:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            else {
                expr_ty expression;
                asdl_seq *ops, *cmps;
                ops = asdl_seq_new(NCH(n) / 2);
                if (!ops)
                    return NULL;
                cmps = asdl_seq_new(NCH(n) / 2);
                if (!cmps) {
                    asdl_seq_free(ops);
                    return NULL;
                }
                for (i = 1; i < NCH(n); i += 2) {
                    /* XXX cmpop_ty is just an enum */
                    cmpop_ty operator;

                    operator = ast_for_comp_op(CHILD(n, i));
                    if (!operator)
                        return NULL;

                    expression = ast_for_expr(c, CHILD(n, i + 1));
                    if (!expression)
                        return NULL;
                        
                    asdl_seq_SET(ops, i / 2, (void *)operator);
                    asdl_seq_SET(cmps, i / 2, expression);
                }
                expression = ast_for_expr(c, CHILD(n, 0));
                if (!expression)
                    return NULL;
                    
                return Compare(expression, ops, cmps, LINENO(n));
            }
            break;

        /* The next five cases all handle BinOps.  The main body of code
           is the same in each case, but the switch turned inside out to
           reuse the code for each type of operator.
         */
        case expr:
        case xor_expr:
        case and_expr:
        case shift_expr:
        case arith_expr:
        case term:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            return ast_for_binop(c, n);
        case yield_expr: {
	    expr_ty exp = NULL;
	    if (NCH(n) == 2) {
		exp = ast_for_testlist(c, CHILD(n, 1), 0);
		if (!exp)
		    return NULL;
	    }
	    return Yield(exp, LINENO(n));
	}
        case factor: {
            expr_ty expression;
            
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }

            expression = ast_for_expr(c, CHILD(n, 1));
            if (!expression)
                return NULL;

            switch (TYPE(CHILD(n, 0))) {
                case PLUS:
                    return UnaryOp(UAdd, expression, LINENO(n));
                case MINUS:
                    return UnaryOp(USub, expression, LINENO(n));
                case TILDE:
                    return UnaryOp(Invert, expression, LINENO(n));
            }
            break;
        }
        case power: {
            expr_ty e = ast_for_atom(c, CHILD(n, 0));
	    if (!e)
		return NULL;
            if (NCH(n) == 1)
                return e;
            /* power: atom trailer* ('**' factor)*
               trailer: '(' [arglist] ')' | '[' subscriptlist ']' | '.' NAME 

               XXX What about atom trailer trailer ** factor?
            */
            for (i = 1; i < NCH(n); i++) {
                expr_ty new = e;
                node *ch = CHILD(n, i);
                if (ch->n_str && strcmp(ch->n_str, "**") == 0)
                    break;
                if (TYPE(CHILD(ch, 0)) == LPAR) {
                    if (NCH(ch) == 2)
                        new = Call(new, NULL, NULL, NULL, NULL, LINENO(ch));
                    else
                        new = ast_for_call(c, CHILD(ch, 1), new);

                    if (!new) {
		        free_expr(e);
                        return NULL;
		    }
                }
                else if (TYPE(CHILD(ch, 0)) == LSQB) {
                    REQ(CHILD(ch, 2), RSQB);
                    ch = CHILD(ch, 1);
                    if (NCH(ch) <= 2) {
                        slice_ty slc = ast_for_slice(c, CHILD(ch, 0));
                        if (!slc) {
		            free_expr(e);
                            return NULL;
			}

                        new = Subscript(e, slc, Load, LINENO(ch));
                        if (!new) {
		            free_expr(e);
                            free_slice(slc);
                            return NULL;
			}
                    }
                    else {
                        int j;
                        slice_ty slc;
                        asdl_seq *slices = asdl_seq_new((NCH(ch) + 1) / 2);
                        if (!slices) {
		            free_expr(e);
                            return NULL;
			}

                        for (j = 0; j < NCH(ch); j += 2) {
                            slc = ast_for_slice(c, CHILD(ch, j));
                            if (!slc) {
		                free_expr(e);
		                asdl_seq_free(slices);
                                return NULL;
			    }
                            asdl_seq_SET(slices, j / 2, slc);
                        }
                        new = Subscript(e, ExtSlice(slices), Load, LINENO(ch));
                        if (!new) {
		            free_expr(e);
		            asdl_seq_free(slices);
                            return NULL;
			}
                    }
                }
                else {
                    assert(TYPE(CHILD(ch, 0)) == DOT);
                    new = Attribute(e, NEW_IDENTIFIER(CHILD(ch, 1)), Load,
				    LINENO(ch));
                    if (!new) {
		        free_expr(e);
                        return NULL;
		    }
                }
                e = new;
            }
            if (TYPE(CHILD(n, NCH(n) - 1)) == factor) {
                expr_ty f = ast_for_expr(c, CHILD(n, NCH(n) - 1));
                if (!f) {
		    free_expr(e);
                    return NULL;
		}
                return BinOp(e, Pow, f, LINENO(n));
            }
            return e;
        }
        default:
	    abort();
            PyErr_Format(PyExc_Exception, "unhandled expr: %d", TYPE(n));
            return NULL;
    }
    /* should never get here */
    return NULL;
}

static expr_ty
ast_for_call(struct compiling *c, const node *n, expr_ty func)
{
    /*
      arglist: (argument ',')* (argument [',']| '*' test [',' '**' test]
               | '**' test)
      argument: [test '='] test [gen_for]	 # Really [keyword '='] test
    */

    int i, nargs, nkeywords, ngens;
    asdl_seq *args = NULL;
    asdl_seq *keywords = NULL;
    expr_ty vararg = NULL, kwarg = NULL;

    REQ(n, arglist);

    nargs = 0;
    nkeywords = 0;
    ngens = 0;
    for (i = 0; i < NCH(n); i++) {
	node *ch = CHILD(n, i);
	if (TYPE(ch) == argument) {
	    if (NCH(ch) == 1)
		nargs++;
	    else if (TYPE(CHILD(ch, 1)) == gen_for)
		ngens++;
            else
		nkeywords++;
	}
    }
    if (ngens > 1 || (ngens && (nargs || nkeywords))) {
        ast_error(n, "Generator expression must be parenthesised "
		  "if not sole argument");
	return NULL;
    }

    if (nargs + nkeywords + ngens > 255) {
      ast_error(n, "more than 255 arguments");
      return NULL;
    }

    args = asdl_seq_new(nargs + ngens);
    if (!args)
        goto error;
    keywords = asdl_seq_new(nkeywords);
    if (!keywords)
        goto error;
    nargs = 0;
    nkeywords = 0;
    for (i = 0; i < NCH(n); i++) {
	node *ch = CHILD(n, i);
	if (TYPE(ch) == argument) {
	    expr_ty e;
	    if (NCH(ch) == 1) {
		e = ast_for_expr(c, CHILD(ch, 0));
                if (!e)
                    goto error;
		asdl_seq_SET(args, nargs++, e);
	    }  
	    else if (TYPE(CHILD(ch, 1)) == gen_for) {
        	e = ast_for_genexp(c, ch);
                if (!e)
                    goto error;
		asdl_seq_SET(args, nargs++, e);
            }
	    else {
		keyword_ty kw;
		identifier key;

		/* CHILD(ch, 0) is test, but must be an identifier? */ 
		e = ast_for_expr(c, CHILD(ch, 0));
                if (!e)
                    goto error;
                /* f(lambda x: x[0] = 3) ends up getting parsed with
                 * LHS test = lambda x: x[0], and RHS test = 3.
                 * SF bug 132313 points out that complaining about a keyword
                 * then is very confusing.
                 */
                if (e->kind == Lambda_kind) {
                  ast_error(CHILD(ch, 0), "lambda cannot contain assignment");
                  goto error;
                } else if (e->kind != Name_kind) {
                  ast_error(CHILD(ch, 0), "keyword can't be an expression");
                  goto error;
                }
		key = e->v.Name.id;
		free(e);
		e = ast_for_expr(c, CHILD(ch, 2));
                if (!e)
                    goto error;
		kw = keyword(key, e);
                if (!kw)
                    goto error;
		asdl_seq_SET(keywords, nkeywords++, kw);
	    }
	}
	else if (TYPE(ch) == STAR) {
	    vararg = ast_for_expr(c, CHILD(n, i+1));
	    i++;
	}
	else if (TYPE(ch) == DOUBLESTAR) {
	    kwarg = ast_for_expr(c, CHILD(n, i+1));
	    i++;
	}
    }

    return Call(func, args, keywords, vararg, kwarg, LINENO(n));

 error:
    if (args)
        asdl_seq_free(args);
    if (keywords)
        asdl_seq_free(keywords);
    return NULL;
}

/* Unlike other ast_for_XXX() functions, this takes a flag that
   indicates whether generator expressions are allowed.  If gexp is
   non-zero, check for testlist_gexp instead of plain testlist.
*/

static expr_ty
ast_for_testlist(struct compiling *c, const node* n, int gexp)
{
  /* testlist_gexp: test ( gen_for | (',' test)* [','] )
     testlist: test (',' test)* [',']
  */

    assert(NCH(n) > 0);
    if (NCH(n) == 1)
	return ast_for_expr(c, CHILD(n, 0));
    if (TYPE(CHILD(n, 1)) == gen_for) {
	if (!gexp) {
	    ast_error(n, "illegal generator expression");
	    return NULL;
	}
	return ast_for_genexp(c, n);
    }
    else {
        asdl_seq *tmp = seq_for_testlist(c, n);
        if (!tmp)
            return NULL;

	return Tuple(tmp, Load, LINENO(n));
    }
    return NULL;  /* unreachable */
}

static stmt_ty
ast_for_expr_stmt(struct compiling *c, const node *n)
{
    REQ(n, expr_stmt);
    /* expr_stmt: testlist (augassign (yield_expr|testlist) 
                | ('=' (yield_expr|testlist))*)
       testlist: test (',' test)* [',']
       augassign: '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^='
	        | '<<=' | '>>=' | '**=' | '//='
       test: ... here starts the operator precendence dance
     */

    if (NCH(n) == 1) {
	expr_ty e = ast_for_testlist(c, CHILD(n, 0), 0);
        if (!e)
            return NULL;

	return Expr(e, LINENO(n));
    }
    else if (TYPE(CHILD(n, 1)) == augassign) {
        expr_ty expr1, expr2;
        operator_ty operator;
	node *ch = CHILD(n, 0);

	if (TYPE(ch) == testlist)
	    expr1 = ast_for_testlist(c, ch, 0);
	else
	    expr1 = Yield(ast_for_expr(c, CHILD(ch, 0)), LINENO(ch));

        if (!expr1)
            return NULL;
        if (expr1->kind == GeneratorExp_kind) {
	    ast_error(ch, "augmented assignment to generator "
		      "expression not possible");
	    return NULL;
        }
	if (expr1->kind == Name_kind) {
		char *var_name = PyString_AS_STRING(expr1->v.Name.id);
		if (var_name[0] == 'N' && !strcmp(var_name, "None")) {
			ast_error(ch, "assignment to None");
			return NULL;
		}
	}

	ch = CHILD(n, 2);
	if (TYPE(ch) == testlist)
	    expr2 = ast_for_testlist(c, ch, 0);
	else
	    expr2 = Yield(ast_for_expr(c, ch), LINENO(ch));
        if (!expr2)
            return NULL;

        operator = ast_for_augassign(CHILD(n, 1));
        if (!operator)
            return NULL;

	return AugAssign(expr1, operator, expr2, LINENO(n));
    }
    else {
	int i;
	asdl_seq *targets;
	node *value;
        expr_ty expression;

	/* a normal assignment */
	REQ(CHILD(n, 1), EQUAL);
	targets = asdl_seq_new(NCH(n) / 2);
	if (!targets)
	    return NULL;
	for (i = 0; i < NCH(n) - 2; i += 2) {
	    node *ch = CHILD(n, i);
	    if (TYPE(ch) == yield_expr) {
		ast_error(ch, "assignment to yield expression not possible");
		goto error;
	    }
	    expr_ty e = ast_for_testlist(c, ch, 0);

	    /* set context to assign */
	    if (!e) 
	      goto error;

	    if (!set_context(e, Store, CHILD(n, i))) {
              free_expr(e);
	      goto error;
            }

	    asdl_seq_SET(targets, i / 2, e);
	}
	value = CHILD(n, NCH(n) - 1);
	if (TYPE(value) == testlist)
	    expression = ast_for_testlist(c, value, 0);
	else
	    expression = ast_for_expr(c, value);
	if (!expression)
	    return NULL;
	return Assign(targets, expression, LINENO(n));
    error:
        for (i = i / 2; i >= 0; i--)
            free_expr((expr_ty)asdl_seq_GET(targets, i));
        asdl_seq_free(targets);
        return NULL;
    }
    return NULL;
}

static stmt_ty
ast_for_print_stmt(struct compiling *c, const node *n)
{
    /* print_stmt: 'print' ( [ test (',' test)* [','] ]
                             | '>>' test [ (',' test)+ [','] ] )
     */
    expr_ty dest = NULL, expression;
    asdl_seq *seq;
    bool nl;
    int i, start = 1;

    REQ(n, print_stmt);
    if (NCH(n) >= 2 && TYPE(CHILD(n, 1)) == RIGHTSHIFT) {
	dest = ast_for_expr(c, CHILD(n, 2));
        if (!dest)
            return NULL;
	start = 4;
    }
    seq = asdl_seq_new((NCH(n) + 1 - start) / 2);
    if (!seq)
	return NULL;
    for (i = start; i < NCH(n); i += 2) {
        expression = ast_for_expr(c, CHILD(n, i));
        if (!expression) {
	    asdl_seq_free(seq);
            return NULL;
	}

	asdl_seq_APPEND(seq, expression);
    }
    nl = (TYPE(CHILD(n, NCH(n) - 1)) == COMMA) ? false : true;
    return Print(dest, seq, nl, LINENO(n));
}

static asdl_seq *
ast_for_exprlist(struct compiling *c, const node *n, int context)
{
    asdl_seq *seq;
    int i;
    expr_ty e;

    REQ(n, exprlist);

    seq = asdl_seq_new((NCH(n) + 1) / 2);
    if (!seq)
	return NULL;
    for (i = 0; i < NCH(n); i += 2) {
	e = ast_for_expr(c, CHILD(n, i));
	if (!e) {
	    asdl_seq_free(seq);
	    return NULL;
	}
	if (context) {
	    if (!set_context(e, context, CHILD(n, i)))
                return NULL;
        }
	asdl_seq_SET(seq, i / 2, e);
    }
    return seq;
}

static stmt_ty
ast_for_del_stmt(struct compiling *c, const node *n)
{
    asdl_seq *expr_list;
    
    /* del_stmt: 'del' exprlist */
    REQ(n, del_stmt);

    expr_list = ast_for_exprlist(c, CHILD(n, 1), Del);
    if (!expr_list)
        return NULL;
    return Delete(expr_list, LINENO(n));
}

static stmt_ty
ast_for_flow_stmt(struct compiling *c, const node *n)
{
    /*
      flow_stmt: break_stmt | continue_stmt | return_stmt | raise_stmt
                 | yield_stmt
      break_stmt: 'break'
      continue_stmt: 'continue'
      return_stmt: 'return' [testlist]
      yield_stmt: yield_expr
      yield_expr: 'yield' testlist
      raise_stmt: 'raise' [test [',' test [',' test]]]
    */
    node *ch;

    REQ(n, flow_stmt);
    ch = CHILD(n, 0);
    switch (TYPE(ch)) {
        case break_stmt:
            return Break(LINENO(n));
        case continue_stmt:
            return Continue(LINENO(n));
        case yield_stmt: { /* will reduce to yield_expr */
	    expr_ty exp = ast_for_expr(c, CHILD(ch, 0));
	    if (!exp)
		return NULL;
            return Expr(exp, LINENO(n));
        }
        case return_stmt:
            if (NCH(ch) == 1)
                return Return(NULL, LINENO(n));
            else {
                expr_ty expression = ast_for_testlist(c, CHILD(ch, 1), 0);
                if (!expression)
                    return NULL;
                return Return(expression, LINENO(n));
            }
        case raise_stmt:
            if (NCH(ch) == 1)
                return Raise(NULL, NULL, NULL, LINENO(n));
            else if (NCH(ch) == 2) {
                expr_ty expression = ast_for_expr(c, CHILD(ch, 1));
                if (!expression)
                    return NULL;
                return Raise(expression, NULL, NULL, LINENO(n));
            }
            else if (NCH(ch) == 4) {
                expr_ty expr1, expr2;

                expr1 = ast_for_expr(c, CHILD(ch, 1));
                if (!expr1)
                    return NULL;
                expr2 = ast_for_expr(c, CHILD(ch, 3));
                if (!expr2)
                    return NULL;

                return Raise(expr1, expr2, NULL, LINENO(n));
            }
            else if (NCH(ch) == 6) {
                expr_ty expr1, expr2, expr3;

                expr1 = ast_for_expr(c, CHILD(ch, 1));
                if (!expr1)
                    return NULL;
                expr2 = ast_for_expr(c, CHILD(ch, 3));
                if (!expr2)
                    return NULL;
                expr3 = ast_for_expr(c, CHILD(ch, 5));
                if (!expr3)
                    return NULL;
                    
                return Raise(expr1, expr2, expr3, LINENO(n));
            }
        default:
            PyErr_Format(PyExc_Exception,
                         "unexpected flow_stmt: %d", TYPE(ch));
            return NULL;
    }
}

static alias_ty
alias_for_import_name(const node *n)
{
    /*
      import_as_name: NAME [NAME NAME]
      dotted_as_name: dotted_name [NAME NAME]
      dotted_name: NAME ('.' NAME)*
    */
 loop:
    switch (TYPE(n)) {
        case import_as_name:
            if (NCH(n) == 3)
                return alias(NEW_IDENTIFIER(CHILD(n, 0)),
                             NEW_IDENTIFIER(CHILD(n, 2)));
            else
                return alias(NEW_IDENTIFIER(CHILD(n, 0)),
                             NULL);
            break;
        case dotted_as_name:
            if (NCH(n) == 1) {
                n = CHILD(n, 0);
                goto loop;
            }
            else {
                alias_ty a = alias_for_import_name(CHILD(n, 0));
                assert(!a->asname);
                a->asname = NEW_IDENTIFIER(CHILD(n, 2));
                return a;
            }
            break;
        case dotted_name:
            if (NCH(n) == 1)
                return alias(NEW_IDENTIFIER(CHILD(n, 0)), NULL);
            else {
                /* Create a string of the form "a.b.c" */
                int i, len;
                PyObject *str;
                char *s;

                len = 0;
                for (i = 0; i < NCH(n); i += 2)
                    /* length of string plus one for the dot */
                    len += strlen(STR(CHILD(n, i))) + 1;
                len--; /* the last name doesn't have a dot */
                str = PyString_FromStringAndSize(NULL, len);
                if (!str)
                    return NULL;
                s = PyString_AS_STRING(str);
                if (!s)
                    return NULL;
                for (i = 0; i < NCH(n); i += 2) {
                    char *sch = STR(CHILD(n, i));
                    strcpy(s, STR(CHILD(n, i)));
                    s += strlen(sch);
                    *s++ = '.';
                }
                --s;
                *s = '\0';
                PyString_InternInPlace(&str);
                return alias(str, NULL);
            }
            break;
        case STAR:
            return alias(PyString_InternFromString("*"), NULL);
        default:
            PyErr_Format(PyExc_Exception,
                         "unexpected import name: %d", TYPE(n));
            return NULL;
    }
    return NULL;
}

static stmt_ty
ast_for_import_stmt(struct compiling *c, const node *n)
{
    /*
      import_stmt: import_name | import_from
      import_name: 'import' dotted_as_names
      import_from: 'from' dotted_name 'import' ('*' | 
                                                '(' import_as_names ')' | 
                                                import_as_names)
    */
    int i;
    asdl_seq *aliases;

    REQ(n, import_stmt);
    n = CHILD(n, 0);
    if (STR(CHILD(n, 0))[0] == 'i') { /* import */
        n = CHILD(n, 1);
	aliases = asdl_seq_new((NCH(n) + 1) / 2);
	if (!aliases)
		return NULL;
	for (i = 0; i < NCH(n); i += 2) {
            alias_ty import_alias = alias_for_import_name(CHILD(n, i));
            if (!import_alias) {
                asdl_seq_free(aliases);
                return NULL;
            }
	    asdl_seq_SET(aliases, i / 2, import_alias);
        }
	return Import(aliases, LINENO(n));
    }
    else if (STR(CHILD(n, 0))[0] == 'f') { /* from */
	stmt_ty import;
        int n_children;
        const char *from_modules;
	int lineno = LINENO(n);
	alias_ty mod = alias_for_import_name(CHILD(n, 1));
	if (!mod)
            return NULL;

        /* XXX this needs to be cleaned up */

        from_modules = STR(CHILD(n, 3));
        if (!from_modules) {
            n = CHILD(n, 3);                  /* from ... import x, y, z */
            if (NCH(n) % 2 == 0) {
                /* it ends with a comma, not valid but the parser allows it */
                ast_error(n, "trailing comma not allowed without"
                             " surrounding parentheses");
                return NULL;
            }
        }
        else if (from_modules[0] == '*') {
            n = CHILD(n, 3); /* from ... import * */
        }
        else if (from_modules[0] == '(')
            n = CHILD(n, 4);                  /* from ... import (x, y, z) */
        else
            return NULL;

        n_children = NCH(n);
        if (from_modules && from_modules[0] == '*')
            n_children = 1;

	aliases = asdl_seq_new((n_children + 1) / 2);
	if (!aliases) {
            free_alias(mod);
            return NULL;
	}

        /* handle "from ... import *" special b/c there's no children */
        if (from_modules && from_modules[0] == '*') {
            alias_ty import_alias = alias_for_import_name(n);
            if (!import_alias) {
                asdl_seq_free(aliases);
                free_alias(mod);
                return NULL;
            }
	    asdl_seq_APPEND(aliases, import_alias);
        }

	for (i = 0; i < NCH(n); i += 2) {
            alias_ty import_alias = alias_for_import_name(CHILD(n, i));
            if (!import_alias) {
                asdl_seq_free(aliases);
                free_alias(mod);
                return NULL;
            }
	    asdl_seq_APPEND(aliases, import_alias);
        }
        Py_INCREF(mod->name);
	import = ImportFrom(mod->name, aliases, lineno);
	free_alias(mod);
	return import;
    }
    PyErr_Format(PyExc_Exception,
                 "unknown import statement: starts with command '%s'",
                 STR(CHILD(n, 0)));
    return NULL;
}

static stmt_ty
ast_for_global_stmt(struct compiling *c, const node *n)
{
    /* global_stmt: 'global' NAME (',' NAME)* */
    identifier name;
    asdl_seq *s;
    int i;

    REQ(n, global_stmt);
    s = asdl_seq_new(NCH(n) / 2);
    if (!s)
    	return NULL;
    for (i = 1; i < NCH(n); i += 2) {
	name = NEW_IDENTIFIER(CHILD(n, i));
	if (!name) {
	    asdl_seq_free(s);
	    return NULL;
	}
	asdl_seq_SET(s, i / 2, name);
    }
    return Global(s, LINENO(n));
}

static stmt_ty
ast_for_exec_stmt(struct compiling *c, const node *n)
{
    expr_ty expr1, globals = NULL, locals = NULL;
    int n_children = NCH(n);
    if (n_children != 2 && n_children != 4 && n_children != 6) {
        PyErr_Format(PyExc_Exception,
                     "poorly formed 'exec' statement: %d parts to statement",
                     n_children);
        return NULL;
    }

    /* exec_stmt: 'exec' expr ['in' test [',' test]] */
    REQ(n, exec_stmt);
    expr1 = ast_for_expr(c, CHILD(n, 1));
    if (!expr1)
        return NULL;
    if (n_children >= 4) {
        globals = ast_for_expr(c, CHILD(n, 3));
        if (!globals)
            return NULL;
    }
    if (n_children == 6) {
        locals = ast_for_expr(c, CHILD(n, 5));
        if (!locals)
            return NULL;
    }

    return Exec(expr1, globals, locals, LINENO(n));
}

static stmt_ty
ast_for_assert_stmt(struct compiling *c, const node *n)
{
    /* assert_stmt: 'assert' test [',' test] */
    REQ(n, assert_stmt);
    if (NCH(n) == 2) {
        expr_ty expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
	return Assert(expression, NULL, LINENO(n));
    }
    else if (NCH(n) == 4) {
        expr_ty expr1, expr2;

        expr1 = ast_for_expr(c, CHILD(n, 1));
        if (!expr1)
            return NULL;
        expr2 = ast_for_expr(c, CHILD(n, 3));
        if (!expr2)
            return NULL;
            
	return Assert(expr1, expr2, LINENO(n));
    }
    PyErr_Format(PyExc_Exception,
                 "improper number of parts to 'assert' statement: %d",
                 NCH(n));
    return NULL;
}

static asdl_seq *
ast_for_suite(struct compiling *c, const node *n)
{
    /* suite: simple_stmt | NEWLINE INDENT stmt+ DEDENT */
    asdl_seq *seq = NULL;
    stmt_ty s;
    int i, total, num, end, pos = 0;
    node *ch;

    REQ(n, suite);

    total = num_stmts(n);
    seq = asdl_seq_new(total);
    if (!seq)
    	return NULL;
    if (TYPE(CHILD(n, 0)) == simple_stmt) {
	n = CHILD(n, 0);
	/* simple_stmt always ends with a NEWLINE,
	   and may have a trailing SEMI 
	*/
	end = NCH(n) - 1;
	if (TYPE(CHILD(n, end - 1)) == SEMI)
	    end--;
        /* loop by 2 to skip semi-colons */
	for (i = 0; i < end; i += 2) {
	    ch = CHILD(n, i);
	    s = ast_for_stmt(c, ch);
	    if (!s)
		goto error;
	    asdl_seq_SET(seq, pos++, s);
	}
    }
    else {
	for (i = 2; i < (NCH(n) - 1); i++) {
	    ch = CHILD(n, i);
	    REQ(ch, stmt);
	    num = num_stmts(ch);
	    if (num == 1) {
		/* small_stmt or compound_stmt with only one child */
		s = ast_for_stmt(c, ch);
		if (!s)
		    goto error;
		asdl_seq_SET(seq, pos++, s);
	    }
	    else {
		int j;
		ch = CHILD(ch, 0);
		REQ(ch, simple_stmt);
		for (j = 0; j < NCH(ch); j += 2) {
		    s = ast_for_stmt(c, CHILD(ch, j));
		    if (!s)
			goto error;
		    asdl_seq_SET(seq, pos++, s);
		}
	    }
	}
    }
    assert(pos == seq->size);
    return seq;
 error:
    if (seq)
	asdl_seq_free(seq);
    return NULL;
}

static stmt_ty
ast_for_if_stmt(struct compiling *c, const node *n)
{
    /* if_stmt: 'if' test ':' suite ('elif' test ':' suite)*
       ['else' ':' suite]
    */
    char *s;

    REQ(n, if_stmt);

    if (NCH(n) == 4) {
        expr_ty expression;
        asdl_seq *suite_seq;

        expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
        suite_seq = ast_for_suite(c, CHILD(n, 3)); 
        if (!suite_seq)
            return NULL;
            
	return If(expression, suite_seq, NULL, LINENO(n));
    }
    s = STR(CHILD(n, 4));
    /* s[2], the third character in the string, will be
       's' for el_s_e, or
       'i' for el_i_f
    */
    if (s[2] == 's') {
        expr_ty expression;
        asdl_seq *seq1, *seq2;

        expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
        seq1 = ast_for_suite(c, CHILD(n, 3));
        if (!seq1)
            return NULL;
        seq2 = ast_for_suite(c, CHILD(n, 6));
        if (!seq2)
            return NULL;

	return If(expression, seq1, seq2, LINENO(n));
    }
    else if (s[2] == 'i') {
	int i, n_elif, has_else = 0;
	asdl_seq *orelse = NULL;
	n_elif = NCH(n) - 4;
        /* must reference the child n_elif+1 since 'else' token is third,
           not fourth, child from the end. */
	if (TYPE(CHILD(n, (n_elif + 1))) == NAME
	    && STR(CHILD(n, (n_elif + 1)))[2] == 's') {
	    has_else = 1;
	    n_elif -= 3;
	}
	n_elif /= 4;

	if (has_else) {
            expr_ty expression;
            asdl_seq *seq1, *seq2;

	    orelse = asdl_seq_new(1);
	    if (!orelse)
		return NULL;
            expression = ast_for_expr(c, CHILD(n, NCH(n) - 6));
            if (!expression) {
                asdl_seq_free(orelse);
                return NULL;
            }
            seq1 = ast_for_suite(c, CHILD(n, NCH(n) - 4));
            if (!seq1) {
                asdl_seq_free(orelse);
                return NULL;
            }
            seq2 = ast_for_suite(c, CHILD(n, NCH(n) - 1));
            if (!seq2) {
                asdl_seq_free(orelse);
                return NULL;
            }

	    asdl_seq_SET(orelse, 0, If(expression, seq1, seq2, 
				       LINENO(CHILD(n, NCH(n) - 6))));
	    /* the just-created orelse handled the last elif */
	    n_elif--;
	}
        else
            orelse  = NULL;

	for (i = 0; i < n_elif; i++) {
	    int off = 5 + (n_elif - i - 1) * 4;
            expr_ty expression;
            asdl_seq *suite_seq;
	    asdl_seq *new = asdl_seq_new(1);
	    if (!new)
		return NULL;
            expression = ast_for_expr(c, CHILD(n, off));
            if (!expression) {
                asdl_seq_free(new);
                return NULL;
            }
            suite_seq = ast_for_suite(c, CHILD(n, off + 2));
            if (!suite_seq) {
                asdl_seq_free(new);
                return NULL;
            }

	    asdl_seq_SET(new, 0,
			 If(expression, suite_seq, orelse, 
			    LINENO(CHILD(n, off))));
	    orelse = new;
	}
	return If(ast_for_expr(c, CHILD(n, 1)),
		  ast_for_suite(c, CHILD(n, 3)),
		  orelse, LINENO(n));
    }
    else {
        PyErr_Format(PyExc_Exception,
                     "unexpected token in 'if' statement: %s", s);
        return NULL;
    }
}

static stmt_ty
ast_for_while_stmt(struct compiling *c, const node *n)
{
    /* while_stmt: 'while' test ':' suite ['else' ':' suite] */
    REQ(n, while_stmt);

    if (NCH(n) == 4) {
        expr_ty expression;
        asdl_seq *suite_seq;

        expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
        suite_seq = ast_for_suite(c, CHILD(n, 3));
        if (!suite_seq)
            return NULL;
	return While(expression, suite_seq, NULL, LINENO(n));
    }
    else if (NCH(n) == 7) {
        expr_ty expression;
        asdl_seq *seq1, *seq2;

        expression = ast_for_expr(c, CHILD(n, 1));
        if (!expression)
            return NULL;
        seq1 = ast_for_suite(c, CHILD(n, 3));
        if (!seq1)
            return NULL;
        seq2 = ast_for_suite(c, CHILD(n, 6));
        if (!seq2)
            return NULL;

	return While(expression, seq1, seq2, LINENO(n));
    }
    else {
        PyErr_Format(PyExc_Exception,
                     "wrong number of tokens for 'while' statement: %d",
                     NCH(n));
        return NULL;
    }
}

static stmt_ty
ast_for_for_stmt(struct compiling *c, const node *n)
{
    asdl_seq *_target = NULL, *seq = NULL, *suite_seq = NULL;
    expr_ty expression;
    expr_ty target;
    /* for_stmt: 'for' exprlist 'in' testlist ':' suite ['else' ':' suite] */
    REQ(n, for_stmt);

    if (NCH(n) == 9) {
	seq = ast_for_suite(c, CHILD(n, 8));
        if (!seq)
            return NULL;
    }

    _target = ast_for_exprlist(c, CHILD(n, 1), Store);
    if (!_target)
        return NULL;
    if (asdl_seq_LEN(_target) == 1) {
	target = asdl_seq_GET(_target, 0);
	asdl_seq_free(_target);
    }
    else
	target = Tuple(_target, Store, LINENO(n));

    expression = ast_for_testlist(c, CHILD(n, 3), 0);
    if (!expression)
        return NULL;
    suite_seq = ast_for_suite(c, CHILD(n, 5));
    if (!suite_seq)
        return NULL;

    return For(target, expression, suite_seq, seq, LINENO(n));
}

static excepthandler_ty
ast_for_except_clause(struct compiling *c, const node *exc, node *body)
{
    /* except_clause: 'except' [test [',' test]] */
    REQ(exc, except_clause);
    REQ(body, suite);

    if (NCH(exc) == 1) {
        asdl_seq *suite_seq = ast_for_suite(c, body);
        if (!suite_seq)
            return NULL;

	return excepthandler(NULL, NULL, suite_seq);
    }
    else if (NCH(exc) == 2) {
        expr_ty expression;
        asdl_seq *suite_seq;

        expression = ast_for_expr(c, CHILD(exc, 1));
        if (!expression)
            return NULL;
        suite_seq = ast_for_suite(c, body);
        if (!suite_seq)
            return NULL;

	return excepthandler(expression, NULL, suite_seq);
    }
    else if (NCH(exc) == 4) {
        asdl_seq *suite_seq;
        expr_ty expression;
	expr_ty e = ast_for_expr(c, CHILD(exc, 3));
	if (!e)
            return NULL;
	if (!set_context(e, Store, CHILD(exc, 3)))
            return NULL;
        expression = ast_for_expr(c, CHILD(exc, 1));
        if (!expression)
            return NULL;
        suite_seq = ast_for_suite(c, body);
        if (!suite_seq)
            return NULL;

	return excepthandler(expression, e, suite_seq);
    }
    else {
        PyErr_Format(PyExc_Exception,
                     "wrong number of children for 'except' clause: %d",
                     NCH(exc));
        return NULL;
    }
}

static stmt_ty
ast_for_try_stmt(struct compiling *c, const node *n)
{
    REQ(n, try_stmt);

    if (TYPE(CHILD(n, 3)) == NAME) {/* must be 'finally' */
	/* try_stmt: 'try' ':' suite 'finally' ':' suite) */
        asdl_seq *s1, *s2;
        s1 = ast_for_suite(c, CHILD(n, 2));
        if (!s1)
            return NULL;
        s2 = ast_for_suite(c, CHILD(n, 5));
        if (!s2)
            return NULL;
            
	return TryFinally(s1, s2, LINENO(n));
    }
    else if (TYPE(CHILD(n, 3)) == except_clause) {
	/* try_stmt: ('try' ':' suite (except_clause ':' suite)+
           ['else' ':' suite]
	*/
        asdl_seq *suite_seq1, *suite_seq2;
	asdl_seq *handlers;
	int i, has_else = 0, n_except = NCH(n) - 3;
	if (TYPE(CHILD(n, NCH(n) - 3)) == NAME) {
	    has_else = 1;
	    n_except -= 3;
	}
	n_except /= 3;
	handlers = asdl_seq_new(n_except);
	if (!handlers)
		return NULL;
	for (i = 0; i < n_except; i++) {
            excepthandler_ty e = ast_for_except_clause(c,
                                                       CHILD(n, 3 + i * 3),
                                                       CHILD(n, 5 + i * 3));
            if (!e)
                return NULL;
	    asdl_seq_SET(handlers, i, e);
        }

        suite_seq1 = ast_for_suite(c, CHILD(n, 2));
        if (!suite_seq1)
            return NULL;
        if (has_else) {
            suite_seq2 = ast_for_suite(c, CHILD(n, NCH(n) - 1));
            if (!suite_seq2)
                return NULL;
        }
        else
            suite_seq2 = NULL;

	return TryExcept(suite_seq1, handlers, suite_seq2, LINENO(n));
    }
    else {
        PyErr_SetString(PyExc_Exception, "malformed 'try' statement");
        return NULL;
    }
}

static stmt_ty
ast_for_classdef(struct compiling *c, const node *n)
{
    /* classdef: 'class' NAME ['(' testlist ')'] ':' suite */
    expr_ty _bases;
    asdl_seq *bases, *s;
    
    REQ(n, classdef);

    if (!strcmp(STR(CHILD(n, 1)), "None")) {
	    ast_error(n, "assignment to None");
	    return NULL;
    }

    if (NCH(n) == 4) {
        s = ast_for_suite(c, CHILD(n, 3));
        if (!s)
            return NULL;
	return ClassDef(NEW_IDENTIFIER(CHILD(n, 1)), NULL, s, LINENO(n));
    }
    /* check for empty base list */
    if (TYPE(CHILD(n,3)) == RPAR) {
	s = ast_for_suite(c, CHILD(n,5));
	if (!s)
		return NULL;
	return ClassDef(NEW_IDENTIFIER(CHILD(n, 1)), NULL, s, LINENO(n));
    }

    /* else handle the base class list */
    _bases = ast_for_testlist(c, CHILD(n, 3), 0);
    if (!_bases)
        return NULL;
    /* XXX: I don't think we can set to diff types here, how to free???

	Here's the allocation chain:
    		Tuple (Python-ast.c:907)
    		ast_for_testlist (ast.c:1782)
    		ast_for_classdef (ast.c:2677)
     */
    if (_bases->kind == Tuple_kind)
	bases = _bases->v.Tuple.elts;
    else {
	bases = asdl_seq_new(1);
	if (!bases) {
            free_expr(_bases);
	    /* XXX: free _bases */
            return NULL;
	}
	asdl_seq_SET(bases, 0, _bases);
    }

    s = ast_for_suite(c, CHILD(n, 6));
    if (!s) {
	/* XXX: I think this free is correct, but needs to change see above */
        if (_bases->kind == Tuple_kind)
		free_expr(_bases);
	else {
		free_expr(_bases);
        	asdl_seq_free(bases);
	}
        return NULL;
    }
    return ClassDef(NEW_IDENTIFIER(CHILD(n, 1)), bases, s, LINENO(n));
}

static stmt_ty
ast_for_stmt(struct compiling *c, const node *n)
{
    if (TYPE(n) == stmt) {
	assert(NCH(n) == 1);
	n = CHILD(n, 0);
    }
    if (TYPE(n) == simple_stmt) {
	assert(num_stmts(n) == 1);
	n = CHILD(n, 0);
    }
    if (TYPE(n) == small_stmt) {
	REQ(n, small_stmt);
	n = CHILD(n, 0);
	/* small_stmt: expr_stmt | print_stmt  | del_stmt | pass_stmt
	             | flow_stmt | import_stmt | global_stmt | exec_stmt
                     | assert_stmt
	*/
	switch (TYPE(n)) {
            case expr_stmt:
                return ast_for_expr_stmt(c, n);
            case print_stmt:
                return ast_for_print_stmt(c, n);
            case del_stmt:
                return ast_for_del_stmt(c, n);
            case pass_stmt:
                return Pass(LINENO(n));
            case flow_stmt:
                return ast_for_flow_stmt(c, n);
            case import_stmt:
                return ast_for_import_stmt(c, n);
            case global_stmt:
                return ast_for_global_stmt(c, n);
            case exec_stmt:
                return ast_for_exec_stmt(c, n);
            case assert_stmt:
                return ast_for_assert_stmt(c, n);
            default:
                PyErr_Format(PyExc_Exception,
                             "unhandled small_stmt: TYPE=%d NCH=%d\n",
                             TYPE(n), NCH(n));
                return NULL;
        }
    }
    else {
        /* compound_stmt: if_stmt | while_stmt | for_stmt | try_stmt
	                | funcdef | classdef
	*/
	node *ch = CHILD(n, 0);
	REQ(n, compound_stmt);
	switch (TYPE(ch)) {
            case if_stmt:
                return ast_for_if_stmt(c, ch);
            case while_stmt:
                return ast_for_while_stmt(c, ch);
            case for_stmt:
                return ast_for_for_stmt(c, ch);
            case try_stmt:
                return ast_for_try_stmt(c, ch);
            case funcdef:
                return ast_for_funcdef(c, ch);
            case classdef:
                return ast_for_classdef(c, ch);
            default:
                PyErr_Format(PyExc_Exception,
                             "unhandled small_stmt: TYPE=%d NCH=%d\n",
                             TYPE(n), NCH(n));
                return NULL;
	}
    }
}

static PyObject *
parsenumber(const char *s)
{
	const char *end;
	long x;
	double dx;
#ifndef WITHOUT_COMPLEX
	Py_complex c;
	int imflag;
#endif

	errno = 0;
	end = s + strlen(s) - 1;
#ifndef WITHOUT_COMPLEX
	imflag = *end == 'j' || *end == 'J';
#endif
	if (*end == 'l' || *end == 'L')
		return PyLong_FromString((char *)s, (char **)0, 0);
	if (s[0] == '0') {
		x = (long) PyOS_strtoul((char *)s, (char **)&end, 0);
 		if (x < 0 && errno == 0) {
	 			return PyLong_FromString((char *)s,
							 (char **)0,
							 0);
		}
	}
	else
		x = PyOS_strtol((char *)s, (char **)&end, 0);
	if (*end == '\0') {
		if (errno != 0)
			return PyLong_FromString((char *)s, (char **)0, 0);
		return PyInt_FromLong(x);
	}
	/* XXX Huge floats may silently fail */
#ifndef WITHOUT_COMPLEX
	if (imflag) {
		c.real = 0.;
		PyFPE_START_PROTECT("atof", return 0)
		c.imag = atof(s);
		PyFPE_END_PROTECT(c)
		return PyComplex_FromCComplex(c);
	}
	else
#endif
	{
		PyFPE_START_PROTECT("atof", return 0)
		dx = atof(s);
		PyFPE_END_PROTECT(dx)
		return PyFloat_FromDouble(dx);
	}
}

static PyObject *
decode_utf8(const char **sPtr, const char *end, char* encoding)
{
#ifndef Py_USING_UNICODE
	Py_FatalError("decode_utf8 should not be called in this build.");
        return NULL;
#else
	PyObject *u, *v;
	char *s, *t;
	t = s = (char *)*sPtr;
	/* while (s < end && *s != '\\') s++; */ /* inefficient for u".." */
	while (s < end && (*s & 0x80)) s++;
	*sPtr = s;
	u = PyUnicode_DecodeUTF8(t, s - t, NULL);
	if (u == NULL)
		return NULL;
	v = PyUnicode_AsEncodedString(u, encoding, NULL);
	Py_DECREF(u);
	return v;
#endif
}

static PyObject *
decode_unicode(const char *s, size_t len, int rawmode, const char *encoding)
{
	PyObject *v, *u;
	char *buf;
	char *p;
	const char *end;
	if (encoding == NULL) {
	     	buf = (char *)s;
		u = NULL;
	} else if (strcmp(encoding, "iso-8859-1") == 0) {
	     	buf = (char *)s;
		u = NULL;
	} else {
		/* "\XX" may become "\u005c\uHHLL" (12 bytes) */
		u = PyString_FromStringAndSize((char *)NULL, len * 4);
		if (u == NULL)
			return NULL;
		p = buf = PyString_AsString(u);
		end = s + len;
		while (s < end) {
			if (*s == '\\') {
				*p++ = *s++;
				if (*s & 0x80) {
					strcpy(p, "u005c");
					p += 5;
				}
			}
			if (*s & 0x80) { /* XXX inefficient */
				PyObject *w;
				char *r;
				int rn, i;
				w = decode_utf8(&s, end, "utf-16-be");
				if (w == NULL) {
					Py_DECREF(u);
					return NULL;
				}
				r = PyString_AsString(w);
				rn = PyString_Size(w);
				assert(rn % 2 == 0);
				for (i = 0; i < rn; i += 2) {
					sprintf(p, "\\u%02x%02x",
						r[i + 0] & 0xFF,
						r[i + 1] & 0xFF);
					p += 6;
				}
				Py_DECREF(w);
			} else {
				*p++ = *s++;
			}
		}
		len = p - buf;
		s = buf;
	}
	if (rawmode)
		v = PyUnicode_DecodeRawUnicodeEscape(s, len, NULL);
	else
		v = PyUnicode_DecodeUnicodeEscape(s, len, NULL);
	Py_XDECREF(u);
	return v;
}

/* s is a Python string literal, including the bracketing quote characters,
 * and r &/or u prefixes (if any), and embedded escape sequences (if any).
 * parsestr parses it, and returns the decoded Python string object.
 */
static PyObject *
parsestr(const char *s, const char *encoding)
{
	PyObject *v;
	size_t len;
	int quote = *s;
	int rawmode = 0;
	int need_encoding;
	int unicode = 0;

	if (isalpha(quote) || quote == '_') {
		if (quote == 'u' || quote == 'U') {
			quote = *++s;
			unicode = 1;
		}
		if (quote == 'r' || quote == 'R') {
			quote = *++s;
			rawmode = 1;
		}
	}
	if (quote != '\'' && quote != '\"') {
		PyErr_BadInternalCall();
		return NULL;
	}
	s++;
	len = strlen(s);
	if (len > INT_MAX) {
		PyErr_SetString(PyExc_OverflowError, 
				"string to parse is too long");
		return NULL;
	}
	if (s[--len] != quote) {
		PyErr_BadInternalCall();
		return NULL;
	}
	if (len >= 4 && s[0] == quote && s[1] == quote) {
		s += 2;
		len -= 2;
		if (s[--len] != quote || s[--len] != quote) {
			PyErr_BadInternalCall();
			return NULL;
		}
	}
#ifdef Py_USING_UNICODE
	if (unicode || Py_UnicodeFlag) {
		return decode_unicode(s, len, rawmode, encoding);
	}
#endif
	need_encoding = (encoding != NULL &&
			 strcmp(encoding, "utf-8") != 0 &&
			 strcmp(encoding, "iso-8859-1") != 0);
	if (rawmode || strchr(s, '\\') == NULL) {
		if (need_encoding) {
#ifndef Py_USING_UNICODE
			/* This should not happen - we never see any other
			   encoding. */
			Py_FatalError("cannot deal with encodings in this build.");
#else
			PyObject* u = PyUnicode_DecodeUTF8(s, len, NULL);
			if (u == NULL)
				return NULL;
			v = PyUnicode_AsEncodedString(u, encoding, NULL);
			Py_DECREF(u);
			return v;
#endif
		} else {
			return PyString_FromStringAndSize(s, len);
		}
	}

	v = PyString_DecodeEscape(s, len, NULL, unicode,
				  need_encoding ? encoding : NULL);
	return v;
}

/* Build a Python string object out of a STRING atom.  This takes care of
 * compile-time literal catenation, calling parsestr() on each piece, and
 * pasting the intermediate results together.
 */
static PyObject *
parsestrplus(struct compiling *c, const node *n)
{
	PyObject *v;
	int i;
	REQ(CHILD(n, 0), STRING);
	if ((v = parsestr(STR(CHILD(n, 0)), c->c_encoding)) != NULL) {
		/* String literal concatenation */
		for (i = 1; i < NCH(n); i++) {
			PyObject *s;
			s = parsestr(STR(CHILD(n, i)), c->c_encoding);
			if (s == NULL)
				goto onError;
			if (PyString_Check(v) && PyString_Check(s)) {
				PyString_ConcatAndDel(&v, s);
				if (v == NULL)
				    goto onError;
			}
#ifdef Py_USING_UNICODE
			else {
				PyObject *temp;
				temp = PyUnicode_Concat(v, s);
				Py_DECREF(s);
				if (temp == NULL)
					goto onError;
				Py_DECREF(v);
				v = temp;
			}
#endif
		}
	}
	return v;

 onError:
	Py_XDECREF(v);
	return NULL;
}
