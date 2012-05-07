/*  parsermodule.c
 *
 *  Copyright 1995-1996 by Fred L. Drake, Jr. and Virginia Polytechnic
 *  Institute and State University, Blacksburg, Virginia, USA.
 *  Portions copyright 1991-1995 by Stichting Mathematisch Centrum,
 *  Amsterdam, The Netherlands.  Copying is permitted under the terms
 *  associated with the main Python distribution, with the additional
 *  restriction that this additional notice be included and maintained
 *  on all distributed copies.
 *
 *  This module serves to replace the original parser module written
 *  by Guido.  The functionality is not matched precisely, but the
 *  original may be implemented on top of this.  This is desirable
 *  since the source of the text to be parsed is now divorced from
 *  this interface.
 *
 *  Unlike the prior interface, the ability to give a parse tree
 *  produced by Python code as a tuple to the compiler is enabled by
 *  this module.  See the documentation for more details.
 *
 *  I've added some annotations that help with the lint code-checking
 *  program, but they're not complete by a long shot.  The real errors
 *  that lint detects are gone, but there are still warnings with
 *  Py_[X]DECREF() and Py_[X]INCREF() macros.  The lint annotations
 *  look like "NOTE(...)".
 */

#include "Python.h"                     /* general Python API             */
#include "Python-ast.h"                 /* mod_ty */
#include "graminit.h"                   /* symbols defined in the grammar */
#include "node.h"                       /* internal parser structure      */
#include "errcode.h"                    /* error codes for PyNode_*()     */
#include "token.h"                      /* token definitions              */
#include "grammar.h"
#include "parsetok.h"
                                        /* ISTERMINAL() / ISNONTERMINAL() */
#undef Yield
#include "ast.h"

extern grammar _PyParser_Grammar; /* From graminit.c */

#ifdef lint
#include <note.h>
#else
#define NOTE(x)
#endif

/*  String constants used to initialize module attributes.
 *
 */
static char parser_copyright_string[] =
"Copyright 1995-1996 by Virginia Polytechnic Institute & State\n\
University, Blacksburg, Virginia, USA, and Fred L. Drake, Jr., Reston,\n\
Virginia, USA.  Portions copyright 1991-1995 by Stichting Mathematisch\n\
Centrum, Amsterdam, The Netherlands.";


PyDoc_STRVAR(parser_doc_string,
"This is an interface to Python's internal parser.");

static char parser_version_string[] = "0.5";


typedef PyObject* (*SeqMaker) (Py_ssize_t length);
typedef int (*SeqInserter) (PyObject* sequence,
                            Py_ssize_t index,
                            PyObject* element);

/*  The function below is copyrighted by Stichting Mathematisch Centrum.  The
 *  original copyright statement is included below, and continues to apply
 *  in full to the function immediately following.  All other material is
 *  original, copyrighted by Fred L. Drake, Jr. and Virginia Polytechnic
 *  Institute and State University.  Changes were made to comply with the
 *  new naming conventions.  Added arguments to provide support for creating
 *  lists as well as tuples, and optionally including the line numbers.
 */


static PyObject*
node2tuple(node *n,                     /* node to convert               */
           SeqMaker mkseq,              /* create sequence               */
           SeqInserter addelem,         /* func. to add elem. in seq.    */
           int lineno,                  /* include line numbers?         */
           int col_offset)              /* include column offsets?       */
{
    if (n == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    if (ISNONTERMINAL(TYPE(n))) {
        int i;
        PyObject *v;
        PyObject *w;

        v = mkseq(1 + NCH(n) + (TYPE(n) == encoding_decl));
        if (v == NULL)
            return (v);
        w = PyLong_FromLong(TYPE(n));
        if (w == NULL) {
            Py_DECREF(v);
            return ((PyObject*) NULL);
        }
        (void) addelem(v, 0, w);
        for (i = 0; i < NCH(n); i++) {
            w = node2tuple(CHILD(n, i), mkseq, addelem, lineno, col_offset);
            if (w == NULL) {
                Py_DECREF(v);
                return ((PyObject*) NULL);
            }
            (void) addelem(v, i+1, w);
        }

        if (TYPE(n) == encoding_decl)
            (void) addelem(v, i+1, PyUnicode_FromString(STR(n)));
        return (v);
    }
    else if (ISTERMINAL(TYPE(n))) {
        PyObject *result = mkseq(2 + lineno + col_offset);
        if (result != NULL) {
            (void) addelem(result, 0, PyLong_FromLong(TYPE(n)));
            (void) addelem(result, 1, PyUnicode_FromString(STR(n)));
            if (lineno == 1)
                (void) addelem(result, 2, PyLong_FromLong(n->n_lineno));
            if (col_offset == 1)
                (void) addelem(result, 3, PyLong_FromLong(n->n_col_offset));
        }
        return (result);
    }
    else {
        PyErr_SetString(PyExc_SystemError,
                        "unrecognized parse tree node type");
        return ((PyObject*) NULL);
    }
}
/*
 *  End of material copyrighted by Stichting Mathematisch Centrum.
 */



/*  There are two types of intermediate objects we're interested in:
 *  'eval' and 'exec' types.  These constants can be used in the st_type
 *  field of the object type to identify which any given object represents.
 *  These should probably go in an external header to allow other extensions
 *  to use them, but then, we really should be using C++ too.  ;-)
 */

#define PyST_EXPR  1
#define PyST_SUITE 2


/*  These are the internal objects and definitions required to implement the
 *  ST type.  Most of the internal names are more reminiscent of the 'old'
 *  naming style, but the code uses the new naming convention.
 */

static PyObject*
parser_error = 0;


typedef struct {
    PyObject_HEAD                       /* standard object header           */
    node* st_node;                      /* the node* returned by the parser */
    int   st_type;                      /* EXPR or SUITE ?                  */
    PyCompilerFlags st_flags;           /* Parser and compiler flags        */
} PyST_Object;


static void parser_free(PyST_Object *st);
static PyObject* parser_richcompare(PyObject *left, PyObject *right, int op);
static PyObject* parser_compilest(PyST_Object *, PyObject *, PyObject *);
static PyObject* parser_isexpr(PyST_Object *, PyObject *, PyObject *);
static PyObject* parser_issuite(PyST_Object *, PyObject *, PyObject *);
static PyObject* parser_st2list(PyST_Object *, PyObject *, PyObject *);
static PyObject* parser_st2tuple(PyST_Object *, PyObject *, PyObject *);

#define PUBLIC_METHOD_TYPE (METH_VARARGS|METH_KEYWORDS)

static PyMethodDef parser_methods[] = {
    {"compile",         (PyCFunction)parser_compilest,  PUBLIC_METHOD_TYPE,
        PyDoc_STR("Compile this ST object into a code object.")},
    {"isexpr",          (PyCFunction)parser_isexpr,     PUBLIC_METHOD_TYPE,
        PyDoc_STR("Determines if this ST object was created from an expression.")},
    {"issuite",         (PyCFunction)parser_issuite,    PUBLIC_METHOD_TYPE,
        PyDoc_STR("Determines if this ST object was created from a suite.")},
    {"tolist",          (PyCFunction)parser_st2list,    PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates a list-tree representation of this ST.")},
    {"totuple",         (PyCFunction)parser_st2tuple,   PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates a tuple-tree representation of this ST.")},

    {NULL, NULL, 0, NULL}
};

static
PyTypeObject PyST_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "parser.st",                        /* tp_name              */
    (int) sizeof(PyST_Object),          /* tp_basicsize         */
    0,                                  /* tp_itemsize          */
    (destructor)parser_free,            /* tp_dealloc           */
    0,                                  /* tp_print             */
    0,                                  /* tp_getattr           */
    0,                                  /* tp_setattr           */
    0,                                  /* tp_reserved          */
    0,                                  /* tp_repr              */
    0,                                  /* tp_as_number         */
    0,                                  /* tp_as_sequence       */
    0,                                  /* tp_as_mapping        */
    0,                                  /* tp_hash              */
    0,                                  /* tp_call              */
    0,                                  /* tp_str               */
    0,                                  /* tp_getattro          */
    0,                                  /* tp_setattro          */

    /* Functions to access object as input/output buffer */
    0,                                  /* tp_as_buffer         */

    Py_TPFLAGS_DEFAULT,                 /* tp_flags             */

    /* __doc__ */
    "Intermediate representation of a Python parse tree.",
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    parser_richcompare,                 /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    parser_methods,                     /* tp_methods */
};  /* PyST_Type */


/* PyST_Type isn't subclassable, so just check ob_type */
#define PyST_Object_Check(v) ((v)->ob_type == &PyST_Type)

static int
parser_compare_nodes(node *left, node *right)
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

        if (v != 0)
            return (v);
    }
    return (0);
}

/*  parser_richcompare(PyObject* left, PyObject* right, int op)
 *
 *  Comparison function used by the Python operators ==, !=, <, >, <=, >=
 *  This really just wraps a call to parser_compare_nodes() with some easy
 *  checks and protection code.
 *
 */

#define TEST_COND(cond) ((cond) ? Py_True : Py_False)

static PyObject *
parser_richcompare(PyObject *left, PyObject *right, int op)
{
    int result;
    PyObject *v;

    /* neither argument should be NULL, unless something's gone wrong */
    if (left == NULL || right == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    /* both arguments should be instances of PyST_Object */
    if (!PyST_Object_Check(left) || !PyST_Object_Check(right)) {
        v = Py_NotImplemented;
        goto finished;
    }

    if (left == right)
        /* if arguments are identical, they're equal */
        result = 0;
    else
        result = parser_compare_nodes(((PyST_Object *)left)->st_node,
                                      ((PyST_Object *)right)->st_node);

    /* Convert return value to a Boolean */
    switch (op) {
      case Py_EQ:
        v = TEST_COND(result == 0);
        break;
      case Py_NE:
        v = TEST_COND(result != 0);
        break;
      case Py_LE:
        v = TEST_COND(result <= 0);
        break;
      case Py_GE:
        v = TEST_COND(result >= 0);
        break;
      case Py_LT:
        v = TEST_COND(result < 0);
        break;
      case Py_GT:
        v = TEST_COND(result > 0);
        break;
      default:
        PyErr_BadArgument();
        return NULL;
    }
  finished:
    Py_INCREF(v);
    return v;
}

/*  parser_newstobject(node* st)
 *
 *  Allocates a new Python object representing an ST.  This is simply the
 *  'wrapper' object that holds a node* and allows it to be passed around in
 *  Python code.
 *
 */
static PyObject*
parser_newstobject(node *st, int type)
{
    PyST_Object* o = PyObject_New(PyST_Object, &PyST_Type);

    if (o != 0) {
        o->st_node = st;
        o->st_type = type;
        o->st_flags.cf_flags = 0;
    }
    else {
        PyNode_Free(st);
    }
    return ((PyObject*)o);
}


/*  void parser_free(PyST_Object* st)
 *
 *  This is called by a del statement that reduces the reference count to 0.
 *
 */
static void
parser_free(PyST_Object *st)
{
    PyNode_Free(st->st_node);
    PyObject_Del(st);
}


/*  parser_st2tuple(PyObject* self, PyObject* args, PyObject* kw)
 *
 *  This provides conversion from a node* to a tuple object that can be
 *  returned to the Python-level caller.  The ST object is not modified.
 *
 */
static PyObject*
parser_st2tuple(PyST_Object *self, PyObject *args, PyObject *kw)
{
    PyObject *line_option = 0;
    PyObject *col_option = 0;
    PyObject *res = 0;
    int ok;

    static char *keywords[] = {"st", "line_info", "col_info", NULL};

    if (self == NULL || PyModule_Check(self)) {
        ok = PyArg_ParseTupleAndKeywords(args, kw, "O!|OO:st2tuple", keywords,
                                         &PyST_Type, &self, &line_option,
                                         &col_option);
    }
    else
        ok = PyArg_ParseTupleAndKeywords(args, kw, "|OO:totuple", &keywords[1],
                                         &line_option, &col_option);
    if (ok != 0) {
        int lineno = 0;
        int col_offset = 0;
        if (line_option != NULL) {
            lineno = (PyObject_IsTrue(line_option) != 0) ? 1 : 0;
        }
        if (col_option != NULL) {
            col_offset = (PyObject_IsTrue(col_option) != 0) ? 1 : 0;
        }
        /*
         *  Convert ST into a tuple representation.  Use Guido's function,
         *  since it's known to work already.
         */
        res = node2tuple(((PyST_Object*)self)->st_node,
                         PyTuple_New, PyTuple_SetItem, lineno, col_offset);
    }
    return (res);
}


/*  parser_st2list(PyObject* self, PyObject* args, PyObject* kw)
 *
 *  This provides conversion from a node* to a list object that can be
 *  returned to the Python-level caller.  The ST object is not modified.
 *
 */
static PyObject*
parser_st2list(PyST_Object *self, PyObject *args, PyObject *kw)
{
    PyObject *line_option = 0;
    PyObject *col_option = 0;
    PyObject *res = 0;
    int ok;

    static char *keywords[] = {"st", "line_info", "col_info", NULL};

    if (self == NULL || PyModule_Check(self))
        ok = PyArg_ParseTupleAndKeywords(args, kw, "O!|OO:st2list", keywords,
                                         &PyST_Type, &self, &line_option,
                                         &col_option);
    else
        ok = PyArg_ParseTupleAndKeywords(args, kw, "|OO:tolist", &keywords[1],
                                         &line_option, &col_option);
    if (ok) {
        int lineno = 0;
        int col_offset = 0;
        if (line_option != 0) {
            lineno = PyObject_IsTrue(line_option) ? 1 : 0;
        }
        if (col_option != NULL) {
            col_offset = (PyObject_IsTrue(col_option) != 0) ? 1 : 0;
        }
        /*
         *  Convert ST into a tuple representation.  Use Guido's function,
         *  since it's known to work already.
         */
        res = node2tuple(self->st_node,
                         PyList_New, PyList_SetItem, lineno, col_offset);
    }
    return (res);
}


/*  parser_compilest(PyObject* self, PyObject* args)
 *
 *  This function creates code objects from the parse tree represented by
 *  the passed-in data object.  An optional file name is passed in as well.
 *
 */
static PyObject*
parser_compilest(PyST_Object *self, PyObject *args, PyObject *kw)
{
    PyObject*     res = 0;
    PyArena*      arena;
    mod_ty        mod;
    char*         str = "<syntax-tree>";
    int ok;

    static char *keywords[] = {"st", "filename", NULL};

    if (self == NULL || PyModule_Check(self))
        ok = PyArg_ParseTupleAndKeywords(args, kw, "O!|s:compilest", keywords,
                                         &PyST_Type, &self, &str);
    else
        ok = PyArg_ParseTupleAndKeywords(args, kw, "|s:compile", &keywords[1],
                                         &str);

    if (ok) {
        arena = PyArena_New();
        if (arena) {
           mod = PyAST_FromNode(self->st_node, &(self->st_flags), str, arena);
           if (mod) {
               res = (PyObject *)PyAST_Compile(mod, str, &(self->st_flags), arena);
           }
           PyArena_Free(arena);
        }
    }

    return (res);
}


/*  PyObject* parser_isexpr(PyObject* self, PyObject* args)
 *  PyObject* parser_issuite(PyObject* self, PyObject* args)
 *
 *  Checks the passed-in ST object to determine if it is an expression or
 *  a statement suite, respectively.  The return is a Python truth value.
 *
 */
static PyObject*
parser_isexpr(PyST_Object *self, PyObject *args, PyObject *kw)
{
    PyObject* res = 0;
    int ok;

    static char *keywords[] = {"st", NULL};

    if (self == NULL || PyModule_Check(self))
        ok = PyArg_ParseTupleAndKeywords(args, kw, "O!:isexpr", keywords,
                                         &PyST_Type, &self);
    else
        ok = PyArg_ParseTupleAndKeywords(args, kw, ":isexpr", &keywords[1]);

    if (ok) {
        /* Check to see if the ST represents an expression or not. */
        res = (self->st_type == PyST_EXPR) ? Py_True : Py_False;
        Py_INCREF(res);
    }
    return (res);
}


static PyObject*
parser_issuite(PyST_Object *self, PyObject *args, PyObject *kw)
{
    PyObject* res = 0;
    int ok;

    static char *keywords[] = {"st", NULL};

    if (self == NULL || PyModule_Check(self))
        ok = PyArg_ParseTupleAndKeywords(args, kw, "O!:issuite", keywords,
                                         &PyST_Type, &self);
    else
        ok = PyArg_ParseTupleAndKeywords(args, kw, ":issuite", &keywords[1]);

    if (ok) {
        /* Check to see if the ST represents an expression or not. */
        res = (self->st_type == PyST_EXPR) ? Py_False : Py_True;
        Py_INCREF(res);
    }
    return (res);
}


/*  err_string(char* message)
 *
 *  Sets the error string for an exception of type ParserError.
 *
 */
static void
err_string(char *message)
{
    PyErr_SetString(parser_error, message);
}


/*  PyObject* parser_do_parse(PyObject* args, int type)
 *
 *  Internal function to actually execute the parse and return the result if
 *  successful or set an exception if not.
 *
 */
static PyObject*
parser_do_parse(PyObject *args, PyObject *kw, char *argspec, int type)
{
    char*     string = 0;
    PyObject* res    = 0;
    int flags        = 0;
    perrdetail err;

    static char *keywords[] = {"source", NULL};

    if (PyArg_ParseTupleAndKeywords(args, kw, argspec, keywords, &string)) {
        node* n = PyParser_ParseStringFlagsFilenameEx(string, NULL,
                                                       &_PyParser_Grammar,
                                                      (type == PyST_EXPR)
                                                      ? eval_input : file_input,
                                                      &err, &flags);

        if (n) {
            res = parser_newstobject(n, type);
            if (res)
                ((PyST_Object *)res)->st_flags.cf_flags = flags & PyCF_MASK;
        }
        else {
            PyParser_SetError(&err);
        }
        PyParser_ClearError(&err);
    }
    return (res);
}


/*  PyObject* parser_expr(PyObject* self, PyObject* args)
 *  PyObject* parser_suite(PyObject* self, PyObject* args)
 *
 *  External interfaces to the parser itself.  Which is called determines if
 *  the parser attempts to recognize an expression ('eval' form) or statement
 *  suite ('exec' form).  The real work is done by parser_do_parse() above.
 *
 */
static PyObject*
parser_expr(PyST_Object *self, PyObject *args, PyObject *kw)
{
    NOTE(ARGUNUSED(self))
    return (parser_do_parse(args, kw, "s:expr", PyST_EXPR));
}


static PyObject*
parser_suite(PyST_Object *self, PyObject *args, PyObject *kw)
{
    NOTE(ARGUNUSED(self))
    return (parser_do_parse(args, kw, "s:suite", PyST_SUITE));
}



/*  This is the messy part of the code.  Conversion from a tuple to an ST
 *  object requires that the input tuple be valid without having to rely on
 *  catching an exception from the compiler.  This is done to allow the
 *  compiler itself to remain fast, since most of its input will come from
 *  the parser directly, and therefore be known to be syntactically correct.
 *  This validation is done to ensure that we don't core dump the compile
 *  phase, returning an exception instead.
 *
 *  Two aspects can be broken out in this code:  creating a node tree from
 *  the tuple passed in, and verifying that it is indeed valid.  It may be
 *  advantageous to expand the number of ST types to include funcdefs and
 *  lambdadefs to take advantage of the optimizer, recognizing those STs
 *  here.  They are not necessary, and not quite as useful in a raw form.
 *  For now, let's get expressions and suites working reliably.
 */


static node* build_node_tree(PyObject *tuple);
static int   validate_expr_tree(node *tree);
static int   validate_file_input(node *tree);
static int   validate_encoding_decl(node *tree);

/*  PyObject* parser_tuple2st(PyObject* self, PyObject* args)
 *
 *  This is the public function, called from the Python code.  It receives a
 *  single tuple object from the caller, and creates an ST object if the
 *  tuple can be validated.  It does this by checking the first code of the
 *  tuple, and, if acceptable, builds the internal representation.  If this
 *  step succeeds, the internal representation is validated as fully as
 *  possible with the various validate_*() routines defined below.
 *
 *  This function must be changed if support is to be added for PyST_FRAGMENT
 *  ST objects.
 *
 */
static PyObject*
parser_tuple2st(PyST_Object *self, PyObject *args, PyObject *kw)
{
    NOTE(ARGUNUSED(self))
    PyObject *st = 0;
    PyObject *tuple;
    node *tree;

    static char *keywords[] = {"sequence", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "O:sequence2st", keywords,
                                     &tuple))
        return (0);
    if (!PySequence_Check(tuple)) {
        PyErr_SetString(PyExc_ValueError,
                        "sequence2st() requires a single sequence argument");
        return (0);
    }
    /*
     *  Convert the tree to the internal form before checking it.
     */
    tree = build_node_tree(tuple);
    if (tree != 0) {
        int start_sym = TYPE(tree);
        if (start_sym == eval_input) {
            /*  Might be an eval form.  */
            if (validate_expr_tree(tree))
                st = parser_newstobject(tree, PyST_EXPR);
            else
                PyNode_Free(tree);
        }
        else if (start_sym == file_input) {
            /*  This looks like an exec form so far.  */
            if (validate_file_input(tree))
                st = parser_newstobject(tree, PyST_SUITE);
            else
                PyNode_Free(tree);
        }
        else if (start_sym == encoding_decl) {
            /* This looks like an encoding_decl so far. */
            if (validate_encoding_decl(tree))
                st = parser_newstobject(tree, PyST_SUITE);
            else
                PyNode_Free(tree);
        }
        else {
            /*  This is a fragment, at best. */
            PyNode_Free(tree);
            err_string("parse tree does not use a valid start symbol");
        }
    }
    /*  Make sure we throw an exception on all errors.  We should never
     *  get this, but we'd do well to be sure something is done.
     */
    if (st == NULL && !PyErr_Occurred())
        err_string("unspecified ST error occurred");

    return st;
}


/*  node* build_node_children()
 *
 *  Iterate across the children of the current non-terminal node and build
 *  their structures.  If successful, return the root of this portion of
 *  the tree, otherwise, 0.  Any required exception will be specified already,
 *  and no memory will have been deallocated.
 *
 */
static node*
build_node_children(PyObject *tuple, node *root, int *line_num)
{
    Py_ssize_t len = PyObject_Size(tuple);
    Py_ssize_t i;
    int  err;

    for (i = 1; i < len; ++i) {
        /* elem must always be a sequence, however simple */
        PyObject* elem = PySequence_GetItem(tuple, i);
        int ok = elem != NULL;
        long  type = 0;
        char *strn = 0;

        if (ok)
            ok = PySequence_Check(elem);
        if (ok) {
            PyObject *temp = PySequence_GetItem(elem, 0);
            if (temp == NULL)
                ok = 0;
            else {
                ok = PyLong_Check(temp);
                if (ok)
                    type = PyLong_AS_LONG(temp);
                Py_DECREF(temp);
            }
        }
        if (!ok) {
            PyObject *err = Py_BuildValue("os", elem,
                                          "Illegal node construct.");
            PyErr_SetObject(parser_error, err);
            Py_XDECREF(err);
            Py_XDECREF(elem);
            return (0);
        }
        if (ISTERMINAL(type)) {
            Py_ssize_t len = PyObject_Size(elem);
            PyObject *temp;
            const char *temp_str;

            if ((len != 2) && (len != 3)) {
                err_string("terminal nodes must have 2 or 3 entries");
                return 0;
            }
            temp = PySequence_GetItem(elem, 1);
            if (temp == NULL)
                return 0;
            if (!PyUnicode_Check(temp)) {
                PyErr_Format(parser_error,
                             "second item in terminal node must be a string,"
                             " found %s",
                             Py_TYPE(temp)->tp_name);
                Py_DECREF(temp);
                Py_DECREF(elem);
                return 0;
            }
            if (len == 3) {
                PyObject *o = PySequence_GetItem(elem, 2);
                if (o != NULL) {
                    if (PyLong_Check(o))
                        *line_num = PyLong_AS_LONG(o);
                    else {
                        PyErr_Format(parser_error,
                                     "third item in terminal node must be an"
                                     " integer, found %s",
                                     Py_TYPE(temp)->tp_name);
                        Py_DECREF(o);
                        Py_DECREF(temp);
                        Py_DECREF(elem);
                        return 0;
                    }
                    Py_DECREF(o);
                }
            }
            temp_str = _PyUnicode_AsStringAndSize(temp, &len);
            if (temp_str == NULL) {
                Py_DECREF(temp);
                Py_XDECREF(elem);
                return 0;
            }
            strn = (char *)PyObject_MALLOC(len + 1);
            if (strn != NULL)
                (void) memcpy(strn, temp_str, len + 1);
            Py_DECREF(temp);
        }
        else if (!ISNONTERMINAL(type)) {
            /*
             *  It has to be one or the other; this is an error.
             *  Throw an exception.
             */
            PyObject *err = Py_BuildValue("os", elem, "unknown node type.");
            PyErr_SetObject(parser_error, err);
            Py_XDECREF(err);
            Py_XDECREF(elem);
            return (0);
        }
        err = PyNode_AddChild(root, type, strn, *line_num, 0);
        if (err == E_NOMEM) {
            Py_XDECREF(elem);
            PyObject_FREE(strn);
            return (node *) PyErr_NoMemory();
        }
        if (err == E_OVERFLOW) {
            Py_XDECREF(elem);
            PyObject_FREE(strn);
            PyErr_SetString(PyExc_ValueError,
                            "unsupported number of child nodes");
            return NULL;
        }

        if (ISNONTERMINAL(type)) {
            node* new_child = CHILD(root, i - 1);

            if (new_child != build_node_children(elem, new_child, line_num)) {
                Py_XDECREF(elem);
                return (0);
            }
        }
        else if (type == NEWLINE) {     /* It's true:  we increment the     */
            ++(*line_num);              /* line number *after* the newline! */
        }
        Py_XDECREF(elem);
    }
    return root;
}


static node*
build_node_tree(PyObject *tuple)
{
    node* res = 0;
    PyObject *temp = PySequence_GetItem(tuple, 0);
    long num = -1;

    if (temp != NULL)
        num = PyLong_AsLong(temp);
    Py_XDECREF(temp);
    if (ISTERMINAL(num)) {
        /*
         *  The tuple is simple, but it doesn't start with a start symbol.
         *  Throw an exception now and be done with it.
         */
        tuple = Py_BuildValue("os", tuple,
                    "Illegal syntax-tree; cannot start with terminal symbol.");
        PyErr_SetObject(parser_error, tuple);
        Py_XDECREF(tuple);
    }
    else if (ISNONTERMINAL(num)) {
        /*
         *  Not efficient, but that can be handled later.
         */
        int line_num = 0;
        PyObject *encoding = NULL;

        if (num == encoding_decl) {
            encoding = PySequence_GetItem(tuple, 2);
            /* tuple isn't borrowed anymore here, need to DECREF */
            tuple = PySequence_GetSlice(tuple, 0, 2);
            if (tuple == NULL)
                return NULL;
        }
        res = PyNode_New(num);
        if (res != NULL) {
            if (res != build_node_children(tuple, res, &line_num)) {
                PyNode_Free(res);
                res = NULL;
            }
            if (res && encoding) {
                Py_ssize_t len;
                const char *temp;
                temp = _PyUnicode_AsStringAndSize(encoding, &len);
                if (temp == NULL) {
                    Py_DECREF(res);
                    Py_DECREF(encoding);
                    Py_DECREF(tuple);
                    return NULL;
                }
                res->n_str = (char *)PyObject_MALLOC(len + 1);
                if (res->n_str != NULL && temp != NULL)
                    (void) memcpy(res->n_str, temp, len + 1);
                Py_DECREF(encoding);
                Py_DECREF(tuple);
            }
        }
    }
    else {
        /*  The tuple is illegal -- if the number is neither TERMINAL nor
         *  NONTERMINAL, we can't use it.  Not sure the implementation
         *  allows this condition, but the API doesn't preclude it.
         */
        PyObject *err = Py_BuildValue("os", tuple,
                                      "Illegal component tuple.");
        PyErr_SetObject(parser_error, err);
        Py_XDECREF(err);
    }

    return (res);
}


/*
 *  Validation routines used within the validation section:
 */
static int validate_terminal(node *terminal, int type, char *string);

#define validate_ampersand(ch)  validate_terminal(ch,      AMPER, "&")
#define validate_circumflex(ch) validate_terminal(ch, CIRCUMFLEX, "^")
#define validate_colon(ch)      validate_terminal(ch,      COLON, ":")
#define validate_comma(ch)      validate_terminal(ch,      COMMA, ",")
#define validate_dedent(ch)     validate_terminal(ch,     DEDENT, "")
#define validate_equal(ch)      validate_terminal(ch,      EQUAL, "=")
#define validate_indent(ch)     validate_terminal(ch,     INDENT, (char*)NULL)
#define validate_lparen(ch)     validate_terminal(ch,       LPAR, "(")
#define validate_newline(ch)    validate_terminal(ch,    NEWLINE, (char*)NULL)
#define validate_rparen(ch)     validate_terminal(ch,       RPAR, ")")
#define validate_semi(ch)       validate_terminal(ch,       SEMI, ";")
#define validate_star(ch)       validate_terminal(ch,       STAR, "*")
#define validate_vbar(ch)       validate_terminal(ch,       VBAR, "|")
#define validate_doublestar(ch) validate_terminal(ch, DOUBLESTAR, "**")
#define validate_dot(ch)        validate_terminal(ch,        DOT, ".")
#define validate_at(ch)         validate_terminal(ch,         AT, "@")
#define validate_rarrow(ch)     validate_terminal(ch,     RARROW, "->")
#define validate_name(ch, str)  validate_terminal(ch,       NAME, str)

#define VALIDATER(n)    static int validate_##n(node *tree)

VALIDATER(node);                VALIDATER(small_stmt);
VALIDATER(class);               VALIDATER(node);
VALIDATER(parameters);          VALIDATER(suite);
VALIDATER(testlist);            VALIDATER(varargslist);
VALIDATER(vfpdef);
VALIDATER(stmt);                VALIDATER(simple_stmt);
VALIDATER(expr_stmt);           VALIDATER(power);
VALIDATER(del_stmt);
VALIDATER(return_stmt);         VALIDATER(raise_stmt);
VALIDATER(import_stmt);         VALIDATER(import_stmt);
VALIDATER(import_name);         VALIDATER(yield_stmt);
VALIDATER(global_stmt);         VALIDATER(nonlocal_stmt);
VALIDATER(assert_stmt);
VALIDATER(compound_stmt);       VALIDATER(test_or_star_expr);
VALIDATER(while);               VALIDATER(for);
VALIDATER(try);                 VALIDATER(except_clause);
VALIDATER(test);                VALIDATER(and_test);
VALIDATER(not_test);            VALIDATER(comparison);
VALIDATER(comp_op);
VALIDATER(star_expr);           VALIDATER(expr);
VALIDATER(xor_expr);            VALIDATER(and_expr);
VALIDATER(shift_expr);          VALIDATER(arith_expr);
VALIDATER(term);                VALIDATER(factor);
VALIDATER(atom);                VALIDATER(lambdef);
VALIDATER(trailer);             VALIDATER(subscript);
VALIDATER(subscriptlist);       VALIDATER(sliceop);
VALIDATER(exprlist);            VALIDATER(dictorsetmaker);
VALIDATER(arglist);             VALIDATER(argument);
VALIDATER(comp_for);
VALIDATER(comp_iter);           VALIDATER(comp_if);
VALIDATER(testlist_comp);       VALIDATER(yield_expr);
VALIDATER(or_test);
VALIDATER(test_nocond);         VALIDATER(lambdef_nocond);
VALIDATER(yield_arg);

#undef VALIDATER

#define is_even(n)      (((n) & 1) == 0)
#define is_odd(n)       (((n) & 1) == 1)


static int
validate_ntype(node *n, int t)
{
    if (TYPE(n) != t) {
        PyErr_Format(parser_error, "Expected node type %d, got %d.",
                     t, TYPE(n));
        return 0;
    }
    return 1;
}


/*  Verifies that the number of child nodes is exactly 'num', raising
 *  an exception if it isn't.  The exception message does not indicate
 *  the exact number of nodes, allowing this to be used to raise the
 *  "right" exception when the wrong number of nodes is present in a
 *  specific variant of a statement's syntax.  This is commonly used
 *  in that fashion.
 */
static int
validate_numnodes(node *n, int num, const char *const name)
{
    if (NCH(n) != num) {
        PyErr_Format(parser_error,
                     "Illegal number of children for %s node.", name);
        return 0;
    }
    return 1;
}


static int
validate_terminal(node *terminal, int type, char *string)
{
    int res = (validate_ntype(terminal, type)
               && ((string == 0) || (strcmp(string, STR(terminal)) == 0)));

    if (!res && !PyErr_Occurred()) {
        PyErr_Format(parser_error,
                     "Illegal terminal: expected \"%s\"", string);
    }
    return (res);
}


/*  X (',' X) [',']
 */
static int
validate_repeating_list(node *tree, int ntype, int (*vfunc)(node *),
                        const char *const name)
{
    int nch = NCH(tree);
    int res = (nch && validate_ntype(tree, ntype)
               && vfunc(CHILD(tree, 0)));

    if (!res && !PyErr_Occurred())
        (void) validate_numnodes(tree, 1, name);
    else {
        if (is_even(nch))
            res = validate_comma(CHILD(tree, --nch));
        if (res && nch > 1) {
            int pos = 1;
            for ( ; res && pos < nch; pos += 2)
                res = (validate_comma(CHILD(tree, pos))
                       && vfunc(CHILD(tree, pos + 1)));
        }
    }
    return (res);
}


/*  validate_class()
 *
 *  classdef:
 *      'class' NAME ['(' testlist ')'] ':' suite
 */
static int
validate_class(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, classdef) &&
                ((nch == 4) || (nch == 6) || (nch == 7)));

    if (res) {
        res = (validate_name(CHILD(tree, 0), "class")
               && validate_ntype(CHILD(tree, 1), NAME)
               && validate_colon(CHILD(tree, nch - 2))
               && validate_suite(CHILD(tree, nch - 1)));
    }
    else {
        (void) validate_numnodes(tree, 4, "class");
    }

    if (res) {
        if (nch == 7) {
                res = ((validate_lparen(CHILD(tree, 2)) &&
                        validate_arglist(CHILD(tree, 3)) &&
                        validate_rparen(CHILD(tree, 4))));
        }
        else if (nch == 6) {
                res = (validate_lparen(CHILD(tree,2)) &&
                        validate_rparen(CHILD(tree,3)));
        }
    }
    return (res);
}


/*  if_stmt:
 *      'if' test ':' suite ('elif' test ':' suite)* ['else' ':' suite]
 */
static int
validate_if(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, if_stmt)
               && (nch >= 4)
               && validate_name(CHILD(tree, 0), "if")
               && validate_test(CHILD(tree, 1))
               && validate_colon(CHILD(tree, 2))
               && validate_suite(CHILD(tree, 3)));

    if (res && ((nch % 4) == 3)) {
        /*  ... 'else' ':' suite  */
        res = (validate_name(CHILD(tree, nch - 3), "else")
               && validate_colon(CHILD(tree, nch - 2))
               && validate_suite(CHILD(tree, nch - 1)));
        nch -= 3;
    }
    else if (!res && !PyErr_Occurred())
        (void) validate_numnodes(tree, 4, "if");
    if ((nch % 4) != 0)
        /* Will catch the case for nch < 4 */
        res = validate_numnodes(tree, 0, "if");
    else if (res && (nch > 4)) {
        /*  ... ('elif' test ':' suite)+ ...  */
        int j = 4;
        while ((j < nch) && res) {
            res = (validate_name(CHILD(tree, j), "elif")
                   && validate_colon(CHILD(tree, j + 2))
                   && validate_test(CHILD(tree, j + 1))
                   && validate_suite(CHILD(tree, j + 3)));
            j += 4;
        }
    }
    return (res);
}


/*  parameters:
 *      '(' [varargslist] ')'
 *
 */
static int
validate_parameters(node *tree)
{
    int nch = NCH(tree);
    int res = validate_ntype(tree, parameters) && ((nch == 2) || (nch == 3));

    if (res) {
        res = (validate_lparen(CHILD(tree, 0))
               && validate_rparen(CHILD(tree, nch - 1)));
        if (res && (nch == 3))
            res = validate_varargslist(CHILD(tree, 1));
    }
    else {
        (void) validate_numnodes(tree, 2, "parameters");
    }
    return (res);
}


/*  validate_suite()
 *
 *  suite:
 *      simple_stmt
 *    | NEWLINE INDENT stmt+ DEDENT
 */
static int
validate_suite(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, suite) && ((nch == 1) || (nch >= 4)));

    if (res && (nch == 1))
        res = validate_simple_stmt(CHILD(tree, 0));
    else if (res) {
        /*  NEWLINE INDENT stmt+ DEDENT  */
        res = (validate_newline(CHILD(tree, 0))
               && validate_indent(CHILD(tree, 1))
               && validate_stmt(CHILD(tree, 2))
               && validate_dedent(CHILD(tree, nch - 1)));

        if (res && (nch > 4)) {
            int i = 3;
            --nch;                      /* forget the DEDENT     */
            for ( ; res && (i < nch); ++i)
                res = validate_stmt(CHILD(tree, i));
        }
        else if (nch < 4)
            res = validate_numnodes(tree, 4, "suite");
    }
    return (res);
}


static int
validate_testlist(node *tree)
{
    return (validate_repeating_list(tree, testlist,
                                    validate_test, "testlist"));
}

static int
validate_testlist_star_expr(node *tl)
{
    return (validate_repeating_list(tl, testlist_star_expr, validate_test_or_star_expr,
                                    "testlist"));
}


/* validate either vfpdef or tfpdef.
 * vfpdef: NAME
 * tfpdef: NAME [':' test]
 */
static int
validate_vfpdef(node *tree)
{
    int nch = NCH(tree);
    if (TYPE(tree) == vfpdef) {
        return nch == 1 && validate_name(CHILD(tree, 0), NULL);
    }
    else if (TYPE(tree) == tfpdef) {
        if (nch == 1) {
            return validate_name(CHILD(tree, 0), NULL);
        }
        else if (nch == 3) {
            return validate_name(CHILD(tree, 0), NULL) &&
                   validate_colon(CHILD(tree, 1)) &&
                   validate_test(CHILD(tree, 2));
        }
    }
    return 0;
}

/* '*' [vfpdef] (',' vfpdef ['=' test])* [',' '**' vfpdef] | '**' vfpdef
 * ..or tfpdef in place of vfpdef. vfpdef: NAME; tfpdef: NAME [':' test]
 */
static int
validate_varargslist_trailer(node *tree, int start)
{
    int nch = NCH(tree);
    int res = 0;

    if (nch <= start) {
        err_string("expected variable argument trailer for varargslist");
        return 0;
    }
    if (TYPE(CHILD(tree, start)) == STAR) {
        /*
         * '*' [vfpdef]
         */
        res = validate_star(CHILD(tree, start++));
        if (res && start < nch && (TYPE(CHILD(tree, start)) == vfpdef ||
                                   TYPE(CHILD(tree, start)) == tfpdef))
            res = validate_vfpdef(CHILD(tree, start++));
        /*
         * (',' vfpdef ['=' test])*
         */
        while (res && start + 1 < nch && (
                   TYPE(CHILD(tree, start + 1)) == vfpdef ||
                   TYPE(CHILD(tree, start + 1)) == tfpdef)) {
            res = (validate_comma(CHILD(tree, start++))
                   && validate_vfpdef(CHILD(tree, start++)));
            if (res && start + 1 < nch && TYPE(CHILD(tree, start)) == EQUAL)
                res = (validate_equal(CHILD(tree, start++))
                       && validate_test(CHILD(tree, start++)));
        }
        /*
         * [',' '**' vfpdef]
         */
        if (res && start + 2 < nch && TYPE(CHILD(tree, start+1)) == DOUBLESTAR)
            res = (validate_comma(CHILD(tree, start++))
                   && validate_doublestar(CHILD(tree, start++))
                   && validate_vfpdef(CHILD(tree, start++)));
    }
    else if (TYPE(CHILD(tree, start)) == DOUBLESTAR) {
        /*
         * '**' vfpdef
         */
        if (start + 1 < nch)
            res = (validate_doublestar(CHILD(tree, start++))
                   && validate_vfpdef(CHILD(tree, start++)));
        else {
            res = 0;
            err_string("expected vfpdef after ** in varargslist trailer");
        }
    }
    else {
        res = 0;
        err_string("expected * or ** in varargslist trailer");
    }

    if (res && start != nch) {
        res = 0;
        err_string("unexpected extra children in varargslist trailer");
    }
    return res;
}


/* validate_varargslist()
 *
 * Validate typedargslist or varargslist.
 *
 * typedargslist: ((tfpdef ['=' test] ',')*
 *                 ('*' [tfpdef] (',' tfpdef ['=' test])* [',' '**' tfpdef] |
 *                  '**' tfpdef)
 *                 | tfpdef ['=' test] (',' tfpdef ['=' test])* [','])
 * tfpdef: NAME [':' test]
 * varargslist: ((vfpdef ['=' test] ',')*
 *               ('*' [vfpdef] (',' vfpdef ['=' test])*  [',' '**' vfpdef] |
 *                '**' vfpdef)
 *               | vfpdef ['=' test] (',' vfpdef ['=' test])* [','])
 * vfpdef: NAME
 *
 */
static int
validate_varargslist(node *tree)
{
    int nch = NCH(tree);
    int res = (TYPE(tree) == varargslist ||
               TYPE(tree) == typedargslist) &&
              (nch != 0);
    int sym;
    node *ch;
    int i = 0;

    if (!res)
        return 0;
    if (nch < 1) {
        err_string("varargslist missing child nodes");
        return 0;
    }
    while (i < nch) {
        ch = CHILD(tree, i);
        sym = TYPE(ch);
        if (sym == vfpdef || sym == tfpdef) {
            /* validate (vfpdef ['=' test] ',')+ */
            res = validate_vfpdef(ch);
            ++i;
            if (res && (i+2 <= nch) && TYPE(CHILD(tree, i)) == EQUAL) {
                res = (validate_equal(CHILD(tree, i))
                       && validate_test(CHILD(tree, i+1)));
                if (res)
                  i += 2;
            }
            if (res && i < nch) {
                res = validate_comma(CHILD(tree, i));
                ++i;
            }
        } else if (sym == DOUBLESTAR || sym == STAR) {
            res = validate_varargslist_trailer(tree, i);
            break;
        } else {
            res = 0;
            err_string("illegal formation for varargslist");
        }
    }
    return res;
}


/*  comp_iter:  comp_for | comp_if
 */
static int
validate_comp_iter(node *tree)
{
    int res = (validate_ntype(tree, comp_iter)
               && validate_numnodes(tree, 1, "comp_iter"));
    if (res && TYPE(CHILD(tree, 0)) == comp_for)
        res = validate_comp_for(CHILD(tree, 0));
    else
        res = validate_comp_if(CHILD(tree, 0));

    return res;
}

/*  comp_for:  'for' exprlist 'in' test [comp_iter]
 */
static int
validate_comp_for(node *tree)
{
    int nch = NCH(tree);
    int res;

    if (nch == 5)
        res = validate_comp_iter(CHILD(tree, 4));
    else
        res = validate_numnodes(tree, 4, "comp_for");

    if (res)
        res = (validate_name(CHILD(tree, 0), "for")
               && validate_exprlist(CHILD(tree, 1))
               && validate_name(CHILD(tree, 2), "in")
               && validate_or_test(CHILD(tree, 3)));

    return res;
}

/*  comp_if:  'if' test_nocond [comp_iter]
 */
static int
validate_comp_if(node *tree)
{
    int nch = NCH(tree);
    int res;

    if (nch == 3)
        res = validate_comp_iter(CHILD(tree, 2));
    else
        res = validate_numnodes(tree, 2, "comp_if");

    if (res)
        res = (validate_name(CHILD(tree, 0), "if")
               && validate_test_nocond(CHILD(tree, 1)));

    return res;
}


/*  simple_stmt | compound_stmt
 *
 */
static int
validate_stmt(node *tree)
{
    int res = (validate_ntype(tree, stmt)
               && validate_numnodes(tree, 1, "stmt"));

    if (res) {
        tree = CHILD(tree, 0);

        if (TYPE(tree) == simple_stmt)
            res = validate_simple_stmt(tree);
        else
            res = validate_compound_stmt(tree);
    }
    return (res);
}


/*  small_stmt (';' small_stmt)* [';'] NEWLINE
 *
 */
static int
validate_simple_stmt(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, simple_stmt)
               && (nch >= 2)
               && validate_small_stmt(CHILD(tree, 0))
               && validate_newline(CHILD(tree, nch - 1)));

    if (nch < 2)
        res = validate_numnodes(tree, 2, "simple_stmt");
    --nch;                              /* forget the NEWLINE    */
    if (res && is_even(nch))
        res = validate_semi(CHILD(tree, --nch));
    if (res && (nch > 2)) {
        int i;

        for (i = 1; res && (i < nch); i += 2)
            res = (validate_semi(CHILD(tree, i))
                   && validate_small_stmt(CHILD(tree, i + 1)));
    }
    return (res);
}


static int
validate_small_stmt(node *tree)
{
    int nch = NCH(tree);
    int res = validate_numnodes(tree, 1, "small_stmt");

    if (res) {
        int ntype = TYPE(CHILD(tree, 0));

        if (  (ntype == expr_stmt)
              || (ntype == del_stmt)
              || (ntype == pass_stmt)
              || (ntype == flow_stmt)
              || (ntype == import_stmt)
              || (ntype == global_stmt)
              || (ntype == nonlocal_stmt)
              || (ntype == assert_stmt))
            res = validate_node(CHILD(tree, 0));
        else {
            res = 0;
            err_string("illegal small_stmt child type");
        }
    }
    else if (nch == 1) {
        res = 0;
        PyErr_Format(parser_error,
                     "Unrecognized child node of small_stmt: %d.",
                     TYPE(CHILD(tree, 0)));
    }
    return (res);
}


/*  compound_stmt:
 *      if_stmt | while_stmt | for_stmt | try_stmt | with_stmt | funcdef | classdef | decorated
 */
static int
validate_compound_stmt(node *tree)
{
    int res = (validate_ntype(tree, compound_stmt)
               && validate_numnodes(tree, 1, "compound_stmt"));
    int ntype;

    if (!res)
        return (0);

    tree = CHILD(tree, 0);
    ntype = TYPE(tree);
    if (  (ntype == if_stmt)
          || (ntype == while_stmt)
          || (ntype == for_stmt)
          || (ntype == try_stmt)
          || (ntype == with_stmt)
          || (ntype == funcdef)
          || (ntype == classdef)
          || (ntype == decorated))
        res = validate_node(tree);
    else {
        res = 0;
        PyErr_Format(parser_error,
                     "Illegal compound statement type: %d.", TYPE(tree));
    }
    return (res);
}

static int
validate_yield_or_testlist(node *tree, int tse)
{
    if (TYPE(tree) == yield_expr) {
        return validate_yield_expr(tree);
    }
    else {
        if (tse)
            return validate_testlist_star_expr(tree);
        else
            return validate_testlist(tree);
    }
}

static int
validate_expr_stmt(node *tree)
{
    int j;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, expr_stmt)
               && is_odd(nch)
               && validate_testlist_star_expr(CHILD(tree, 0)));

    if (res && nch == 3
        && TYPE(CHILD(tree, 1)) == augassign) {
        res = validate_numnodes(CHILD(tree, 1), 1, "augassign")
            && validate_yield_or_testlist(CHILD(tree, 2), 0);

        if (res) {
            char *s = STR(CHILD(CHILD(tree, 1), 0));

            res = (strcmp(s, "+=") == 0
                   || strcmp(s, "-=") == 0
                   || strcmp(s, "*=") == 0
                   || strcmp(s, "/=") == 0
                   || strcmp(s, "//=") == 0
                   || strcmp(s, "%=") == 0
                   || strcmp(s, "&=") == 0
                   || strcmp(s, "|=") == 0
                   || strcmp(s, "^=") == 0
                   || strcmp(s, "<<=") == 0
                   || strcmp(s, ">>=") == 0
                   || strcmp(s, "**=") == 0);
            if (!res)
                err_string("illegal augmented assignment operator");
        }
    }
    else {
        for (j = 1; res && (j < nch); j += 2)
            res = validate_equal(CHILD(tree, j))
                && validate_yield_or_testlist(CHILD(tree, j + 1), 1);
    }
    return (res);
}


static int
validate_del_stmt(node *tree)
{
    return (validate_numnodes(tree, 2, "del_stmt")
            && validate_name(CHILD(tree, 0), "del")
            && validate_exprlist(CHILD(tree, 1)));
}


static int
validate_return_stmt(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, return_stmt)
               && ((nch == 1) || (nch == 2))
               && validate_name(CHILD(tree, 0), "return"));

    if (res && (nch == 2))
        res = validate_testlist(CHILD(tree, 1));

    return (res);
}


/*
 *  raise_stmt:
 *
 *  'raise' [test ['from' test]]
 */
static int
validate_raise_stmt(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, raise_stmt)
               && ((nch == 1) || (nch == 2) || (nch == 4)));

    if (!res && !PyErr_Occurred())
        (void) validate_numnodes(tree, 2, "raise");

    if (res) {
        res = validate_name(CHILD(tree, 0), "raise");
        if (res && (nch >= 2))
            res = validate_test(CHILD(tree, 1));
        if (res && (nch == 4)) {
            res = (validate_name(CHILD(tree, 2), "from")
                   && validate_test(CHILD(tree, 3)));
        }
    }
    return (res);
}


/* yield_expr: 'yield' [yield_arg]
 */
static int
validate_yield_expr(node *tree)
{
    int nch = NCH(tree);
    if (nch < 1 || nch > 2)
        return 0;
    if (!validate_ntype(tree, yield_expr))
        return 0;
    if (!validate_name(CHILD(tree, 0), "yield"))
        return 0;
    if (nch == 2) {
        if (!validate_yield_arg(CHILD(tree, 1)))
            return 0;
    }
    return 1;
}

/* yield_arg: 'from' test | testlist
 */
static int
validate_yield_arg(node *tree)
{
    int nch = NCH(tree);
    if (!validate_ntype(tree, yield_arg))
        return 0;
    switch (nch) {
      case 1:
        if (!validate_testlist(CHILD(tree, nch - 1)))
            return 0;
        break;
      case 2:
        if (!validate_name(CHILD(tree, 0), "from"))
            return 0;
        if (!validate_test(CHILD(tree, 1)))
            return 0;
        break;
      default:
        return 0;
    }
    return 1;
}

/* yield_stmt: yield_expr
 */
static int
validate_yield_stmt(node *tree)
{
    return (validate_ntype(tree, yield_stmt)
            && validate_numnodes(tree, 1, "yield_stmt")
            && validate_yield_expr(CHILD(tree, 0)));
}


static int
validate_import_as_name(node *tree)
{
    int nch = NCH(tree);
    int ok = validate_ntype(tree, import_as_name);

    if (ok) {
        if (nch == 1)
            ok = validate_name(CHILD(tree, 0), NULL);
        else if (nch == 3)
            ok = (validate_name(CHILD(tree, 0), NULL)
                  && validate_name(CHILD(tree, 1), "as")
                  && validate_name(CHILD(tree, 2), NULL));
        else
            ok = validate_numnodes(tree, 3, "import_as_name");
    }
    return ok;
}


/* dotted_name:  NAME ("." NAME)*
 */
static int
validate_dotted_name(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, dotted_name)
               && is_odd(nch)
               && validate_name(CHILD(tree, 0), NULL));
    int i;

    for (i = 1; res && (i < nch); i += 2) {
        res = (validate_dot(CHILD(tree, i))
               && validate_name(CHILD(tree, i+1), NULL));
    }
    return res;
}


/* dotted_as_name:  dotted_name [NAME NAME]
 */
static int
validate_dotted_as_name(node *tree)
{
    int nch = NCH(tree);
    int res = validate_ntype(tree, dotted_as_name);

    if (res) {
        if (nch == 1)
            res = validate_dotted_name(CHILD(tree, 0));
        else if (nch == 3)
            res = (validate_dotted_name(CHILD(tree, 0))
                   && validate_name(CHILD(tree, 1), "as")
                   && validate_name(CHILD(tree, 2), NULL));
        else {
            res = 0;
            err_string("illegal number of children for dotted_as_name");
        }
    }
    return res;
}


/* dotted_as_name (',' dotted_as_name)* */
static int
validate_dotted_as_names(node *tree)
{
        int nch = NCH(tree);
        int res = is_odd(nch) && validate_dotted_as_name(CHILD(tree, 0));
        int i;

        for (i = 1; res && (i < nch); i += 2)
            res = (validate_comma(CHILD(tree, i))
                   && validate_dotted_as_name(CHILD(tree, i + 1)));
        return (res);
}


/* import_as_name (',' import_as_name)* [','] */
static int
validate_import_as_names(node *tree)
{
    int nch = NCH(tree);
    int res = validate_import_as_name(CHILD(tree, 0));
    int i;

    for (i = 1; res && (i + 1 < nch); i += 2)
        res = (validate_comma(CHILD(tree, i))
               && validate_import_as_name(CHILD(tree, i + 1)));
    return (res);
}


/* 'import' dotted_as_names */
static int
validate_import_name(node *tree)
{
        return (validate_ntype(tree, import_name)
                && validate_numnodes(tree, 2, "import_name")
                && validate_name(CHILD(tree, 0), "import")
                && validate_dotted_as_names(CHILD(tree, 1)));
}

/* Helper function to count the number of leading dots (or ellipsis tokens) in
 * 'from ...module import name'
 */
static int
count_from_dots(node *tree)
{
    int i;
    for (i = 1; i < NCH(tree); i++)
        if (TYPE(CHILD(tree, i)) != DOT && TYPE(CHILD(tree, i)) != ELLIPSIS)
            break;
    return i - 1;
}

/* import_from: ('from' ('.'* dotted_name | '.'+)
 *               'import' ('*' | '(' import_as_names ')' | import_as_names))
 */
static int
validate_import_from(node *tree)
{
        int nch = NCH(tree);
        int ndots = count_from_dots(tree);
        int havename = (TYPE(CHILD(tree, ndots + 1)) == dotted_name);
        int offset = ndots + havename;
        int res = validate_ntype(tree, import_from)
                && (offset >= 1)
                && (nch >= 3 + offset)
                && validate_name(CHILD(tree, 0), "from")
                && (!havename || validate_dotted_name(CHILD(tree, ndots + 1)))
                && validate_name(CHILD(tree, offset + 1), "import");

        if (res && TYPE(CHILD(tree, offset + 2)) == LPAR)
            res = ((nch == offset + 5)
                   && validate_lparen(CHILD(tree, offset + 2))
                   && validate_import_as_names(CHILD(tree, offset + 3))
                   && validate_rparen(CHILD(tree, offset + 4)));
        else if (res && TYPE(CHILD(tree, offset + 2)) != STAR)
            res = validate_import_as_names(CHILD(tree, offset + 2));
        return (res);
}


/* import_stmt: import_name | import_from */
static int
validate_import_stmt(node *tree)
{
    int nch = NCH(tree);
    int res = validate_numnodes(tree, 1, "import_stmt");

    if (res) {
        int ntype = TYPE(CHILD(tree, 0));

        if (ntype == import_name || ntype == import_from)
            res = validate_node(CHILD(tree, 0));
        else {
            res = 0;
            err_string("illegal import_stmt child type");
        }
    }
    else if (nch == 1) {
        res = 0;
        PyErr_Format(parser_error,
                     "Unrecognized child node of import_stmt: %d.",
                     TYPE(CHILD(tree, 0)));
    }
    return (res);
}


/*  global_stmt:
 *
 *  'global' NAME (',' NAME)*
 */
static int
validate_global_stmt(node *tree)
{
    int j;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, global_stmt)
               && is_even(nch) && (nch >= 2));

    if (!res && !PyErr_Occurred())
        err_string("illegal global statement");

    if (res)
        res = (validate_name(CHILD(tree, 0), "global")
               && validate_ntype(CHILD(tree, 1), NAME));
    for (j = 2; res && (j < nch); j += 2)
        res = (validate_comma(CHILD(tree, j))
               && validate_ntype(CHILD(tree, j + 1), NAME));

    return (res);
}

/*  nonlocal_stmt:
 *
 *  'nonlocal' NAME (',' NAME)*
 */
static int
validate_nonlocal_stmt(node *tree)
{
    int j;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, nonlocal_stmt)
               && is_even(nch) && (nch >= 2));

    if (!res && !PyErr_Occurred())
        err_string("illegal nonlocal statement");

    if (res)
        res = (validate_name(CHILD(tree, 0), "nonlocal")
               && validate_ntype(CHILD(tree, 1), NAME));
    for (j = 2; res && (j < nch); j += 2)
        res = (validate_comma(CHILD(tree, j))
               && validate_ntype(CHILD(tree, j + 1), NAME));

    return res;
}

/*  assert_stmt:
 *
 *  'assert' test [',' test]
 */
static int
validate_assert_stmt(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, assert_stmt)
               && ((nch == 2) || (nch == 4))
               && (validate_name(CHILD(tree, 0), "assert"))
               && validate_test(CHILD(tree, 1)));

    if (!res && !PyErr_Occurred())
        err_string("illegal assert statement");
    if (res && (nch > 2))
        res = (validate_comma(CHILD(tree, 2))
               && validate_test(CHILD(tree, 3)));

    return (res);
}


static int
validate_while(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, while_stmt)
               && ((nch == 4) || (nch == 7))
               && validate_name(CHILD(tree, 0), "while")
               && validate_test(CHILD(tree, 1))
               && validate_colon(CHILD(tree, 2))
               && validate_suite(CHILD(tree, 3)));

    if (res && (nch == 7))
        res = (validate_name(CHILD(tree, 4), "else")
               && validate_colon(CHILD(tree, 5))
               && validate_suite(CHILD(tree, 6)));

    return (res);
}


static int
validate_for(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, for_stmt)
               && ((nch == 6) || (nch == 9))
               && validate_name(CHILD(tree, 0), "for")
               && validate_exprlist(CHILD(tree, 1))
               && validate_name(CHILD(tree, 2), "in")
               && validate_testlist(CHILD(tree, 3))
               && validate_colon(CHILD(tree, 4))
               && validate_suite(CHILD(tree, 5)));

    if (res && (nch == 9))
        res = (validate_name(CHILD(tree, 6), "else")
               && validate_colon(CHILD(tree, 7))
               && validate_suite(CHILD(tree, 8)));

    return (res);
}


/*  try_stmt:
 *      'try' ':' suite (except_clause ':' suite)+ ['else' ':' suite]
                                                   ['finally' ':' suite]
 *    | 'try' ':' suite 'finally' ':' suite
 *
 */
static int
validate_try(node *tree)
{
    int nch = NCH(tree);
    int pos = 3;
    int res = (validate_ntype(tree, try_stmt)
               && (nch >= 6) && ((nch % 3) == 0));

    if (res)
        res = (validate_name(CHILD(tree, 0), "try")
               && validate_colon(CHILD(tree, 1))
               && validate_suite(CHILD(tree, 2))
               && validate_colon(CHILD(tree, nch - 2))
               && validate_suite(CHILD(tree, nch - 1)));
    else if (!PyErr_Occurred()) {
        const char* name = "except";
        if (TYPE(CHILD(tree, nch - 3)) != except_clause)
            name = STR(CHILD(tree, nch - 3));

        PyErr_Format(parser_error,
                     "Illegal number of children for try/%s node.", name);
    }
    /* Handle try/finally statement */
    if (res && (TYPE(CHILD(tree, pos)) == NAME) &&
        (strcmp(STR(CHILD(tree, pos)), "finally") == 0)) {
        res = (validate_numnodes(tree, 6, "try/finally")
               && validate_colon(CHILD(tree, 4))
               && validate_suite(CHILD(tree, 5)));
        return (res);
    }
    /* try/except statement: skip past except_clause sections */
    while (res && pos < nch && (TYPE(CHILD(tree, pos)) == except_clause)) {
        res = (validate_except_clause(CHILD(tree, pos))
               && validate_colon(CHILD(tree, pos + 1))
               && validate_suite(CHILD(tree, pos + 2)));
        pos += 3;
    }
    /* skip else clause */
    if (res && pos < nch && (TYPE(CHILD(tree, pos)) == NAME) &&
        (strcmp(STR(CHILD(tree, pos)), "else") == 0)) {
        res = (validate_colon(CHILD(tree, pos + 1))
               && validate_suite(CHILD(tree, pos + 2)));
        pos += 3;
    }
    if (res && pos < nch) {
        /* last clause must be a finally */
        res = (validate_name(CHILD(tree, pos), "finally")
               && validate_numnodes(tree, pos + 3, "try/except/finally")
               && validate_colon(CHILD(tree, pos + 1))
               && validate_suite(CHILD(tree, pos + 2)));
    }
    return (res);
}


static int
validate_except_clause(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, except_clause)
               && ((nch == 1) || (nch == 2) || (nch == 4))
               && validate_name(CHILD(tree, 0), "except"));

    if (res && (nch > 1))
        res = validate_test(CHILD(tree, 1));
    if (res && (nch == 4))
        res = (validate_name(CHILD(tree, 2), "as")
               && validate_ntype(CHILD(tree, 3), NAME));

    return (res);
}


static int
validate_test(node *tree)
{
    int nch = NCH(tree);
    int res = validate_ntype(tree, test) && is_odd(nch);

    if (res && (TYPE(CHILD(tree, 0)) == lambdef))
        res = ((nch == 1)
               && validate_lambdef(CHILD(tree, 0)));
    else if (res) {
        res = validate_or_test(CHILD(tree, 0));
        res = (res && (nch == 1 || (nch == 5 &&
            validate_name(CHILD(tree, 1), "if") &&
            validate_or_test(CHILD(tree, 2)) &&
            validate_name(CHILD(tree, 3), "else") &&
            validate_test(CHILD(tree, 4)))));
    }
    return (res);
}

static int
validate_test_nocond(node *tree)
{
    int nch = NCH(tree);
    int res = validate_ntype(tree, test_nocond) && (nch == 1);

    if (res && (TYPE(CHILD(tree, 0)) == lambdef_nocond))
        res = (validate_lambdef_nocond(CHILD(tree, 0)));
    else if (res) {
        res = (validate_or_test(CHILD(tree, 0)));
    }
    return (res);
}

static int
validate_or_test(node *tree)
{
    int nch = NCH(tree);
    int res = validate_ntype(tree, or_test) && is_odd(nch);

    if (res) {
        int pos;
        res = validate_and_test(CHILD(tree, 0));
        for (pos = 1; res && (pos < nch); pos += 2)
            res = (validate_name(CHILD(tree, pos), "or")
                   && validate_and_test(CHILD(tree, pos + 1)));
    }
    return (res);
}


static int
validate_and_test(node *tree)
{
    int pos;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, and_test)
               && is_odd(nch)
               && validate_not_test(CHILD(tree, 0)));

    for (pos = 1; res && (pos < nch); pos += 2)
        res = (validate_name(CHILD(tree, pos), "and")
               && validate_not_test(CHILD(tree, 0)));

    return (res);
}


static int
validate_not_test(node *tree)
{
    int nch = NCH(tree);
    int res = validate_ntype(tree, not_test) && ((nch == 1) || (nch == 2));

    if (res) {
        if (nch == 2)
            res = (validate_name(CHILD(tree, 0), "not")
                   && validate_not_test(CHILD(tree, 1)));
        else if (nch == 1)
            res = validate_comparison(CHILD(tree, 0));
    }
    return (res);
}


static int
validate_comparison(node *tree)
{
    int pos;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, comparison)
               && is_odd(nch)
               && validate_expr(CHILD(tree, 0)));

    for (pos = 1; res && (pos < nch); pos += 2)
        res = (validate_comp_op(CHILD(tree, pos))
               && validate_expr(CHILD(tree, pos + 1)));

    return (res);
}


static int
validate_comp_op(node *tree)
{
    int res = 0;
    int nch = NCH(tree);

    if (!validate_ntype(tree, comp_op))
        return (0);
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
                  PyErr_Format(parser_error,
                               "illegal operator '%s'", STR(tree));
              }
              break;
          default:
              err_string("illegal comparison operator type");
              break;
        }
    }
    else if ((res = validate_numnodes(tree, 2, "comp_op")) != 0) {
        res = (validate_ntype(CHILD(tree, 0), NAME)
               && validate_ntype(CHILD(tree, 1), NAME)
               && (((strcmp(STR(CHILD(tree, 0)), "is") == 0)
                    && (strcmp(STR(CHILD(tree, 1)), "not") == 0))
                   || ((strcmp(STR(CHILD(tree, 0)), "not") == 0)
                       && (strcmp(STR(CHILD(tree, 1)), "in") == 0))));
        if (!res && !PyErr_Occurred())
            err_string("unknown comparison operator");
    }
    return (res);
}


static int
validate_star_expr(node *tree)
{
    int res = validate_ntype(tree, star_expr);
    if (!res) return res;
    if (!validate_numnodes(tree, 2, "star_expr"))
        return 0;
    return validate_ntype(CHILD(tree, 0), STAR) &&      \
        validate_expr(CHILD(tree, 1));
}


static int
validate_expr(node *tree)
{
    int j;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, expr)
               && is_odd(nch)
               && validate_xor_expr(CHILD(tree, 0)));

    for (j = 2; res && (j < nch); j += 2)
        res = (validate_xor_expr(CHILD(tree, j))
               && validate_vbar(CHILD(tree, j - 1)));

    return (res);
}


static int
validate_xor_expr(node *tree)
{
    int j;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, xor_expr)
               && is_odd(nch)
               && validate_and_expr(CHILD(tree, 0)));

    for (j = 2; res && (j < nch); j += 2)
        res = (validate_circumflex(CHILD(tree, j - 1))
               && validate_and_expr(CHILD(tree, j)));

    return (res);
}


static int
validate_and_expr(node *tree)
{
    int pos;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, and_expr)
               && is_odd(nch)
               && validate_shift_expr(CHILD(tree, 0)));

    for (pos = 1; res && (pos < nch); pos += 2)
        res = (validate_ampersand(CHILD(tree, pos))
               && validate_shift_expr(CHILD(tree, pos + 1)));

    return (res);
}


static int
validate_chain_two_ops(node *tree, int (*termvalid)(node *), int op1, int op2)
 {
    int pos = 1;
    int nch = NCH(tree);
    int res = (is_odd(nch)
               && (*termvalid)(CHILD(tree, 0)));

    for ( ; res && (pos < nch); pos += 2) {
        if (TYPE(CHILD(tree, pos)) != op1)
            res = validate_ntype(CHILD(tree, pos), op2);
        if (res)
            res = (*termvalid)(CHILD(tree, pos + 1));
    }
    return (res);
}


static int
validate_shift_expr(node *tree)
{
    return (validate_ntype(tree, shift_expr)
            && validate_chain_two_ops(tree, validate_arith_expr,
                                      LEFTSHIFT, RIGHTSHIFT));
}


static int
validate_arith_expr(node *tree)
{
    return (validate_ntype(tree, arith_expr)
            && validate_chain_two_ops(tree, validate_term, PLUS, MINUS));
}


static int
validate_term(node *tree)
{
    int pos = 1;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, term)
               && is_odd(nch)
               && validate_factor(CHILD(tree, 0)));

    for ( ; res && (pos < nch); pos += 2)
        res = (((TYPE(CHILD(tree, pos)) == STAR)
               || (TYPE(CHILD(tree, pos)) == SLASH)
               || (TYPE(CHILD(tree, pos)) == DOUBLESLASH)
               || (TYPE(CHILD(tree, pos)) == PERCENT))
               && validate_factor(CHILD(tree, pos + 1)));

    return (res);
}


/*  factor:
 *
 *  factor: ('+'|'-'|'~') factor | power
 */
static int
validate_factor(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, factor)
               && (((nch == 2)
                    && ((TYPE(CHILD(tree, 0)) == PLUS)
                        || (TYPE(CHILD(tree, 0)) == MINUS)
                        || (TYPE(CHILD(tree, 0)) == TILDE))
                    && validate_factor(CHILD(tree, 1)))
                   || ((nch == 1)
                       && validate_power(CHILD(tree, 0)))));
    return (res);
}


/*  power:
 *
 *  power: atom trailer* ('**' factor)*
 */
static int
validate_power(node *tree)
{
    int pos = 1;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, power) && (nch >= 1)
               && validate_atom(CHILD(tree, 0)));

    while (res && (pos < nch) && (TYPE(CHILD(tree, pos)) == trailer))
        res = validate_trailer(CHILD(tree, pos++));
    if (res && (pos < nch)) {
        if (!is_even(nch - pos)) {
            err_string("illegal number of nodes for 'power'");
            return (0);
        }
        for ( ; res && (pos < (nch - 1)); pos += 2)
            res = (validate_doublestar(CHILD(tree, pos))
                   && validate_factor(CHILD(tree, pos + 1)));
    }
    return (res);
}


static int
validate_atom(node *tree)
{
    int pos;
    int nch = NCH(tree);
    int res = validate_ntype(tree, atom);

    if (res && nch < 1)
        res = validate_numnodes(tree, nch+1, "atom");
    if (res) {
        switch (TYPE(CHILD(tree, 0))) {
          case LPAR:
            res = ((nch <= 3)
                   && (validate_rparen(CHILD(tree, nch - 1))));

            if (res && (nch == 3)) {
                if (TYPE(CHILD(tree, 1))==yield_expr)
                        res = validate_yield_expr(CHILD(tree, 1));
                else
                        res = validate_testlist_comp(CHILD(tree, 1));
            }
            break;
          case LSQB:
            if (nch == 2)
                res = validate_ntype(CHILD(tree, 1), RSQB);
            else if (nch == 3)
                res = (validate_testlist_comp(CHILD(tree, 1))
                       && validate_ntype(CHILD(tree, 2), RSQB));
            else {
                res = 0;
                err_string("illegal list display atom");
            }
            break;
          case LBRACE:
            res = ((nch <= 3)
                   && validate_ntype(CHILD(tree, nch - 1), RBRACE));

            if (res && (nch == 3))
                res = validate_dictorsetmaker(CHILD(tree, 1));
            break;
          case NAME:
          case NUMBER:
          case ELLIPSIS:
            res = (nch == 1);
            break;
          case STRING:
            for (pos = 1; res && (pos < nch); ++pos)
                res = validate_ntype(CHILD(tree, pos), STRING);
            break;
          default:
            res = 0;
            break;
        }
    }
    return (res);
}


/*  testlist_comp:
 *    test ( comp_for | (',' test)* [','] )
 */
static int
validate_testlist_comp(node *tree)
{
    int nch = NCH(tree);
    int ok = nch;

    if (nch == 0)
        err_string("missing child nodes of testlist_comp");
    else {
        ok = validate_test_or_star_expr(CHILD(tree, 0));
    }

    /*
     *  comp_for | (',' test)* [',']
     */
    if (nch == 2 && TYPE(CHILD(tree, 1)) == comp_for)
        ok = validate_comp_for(CHILD(tree, 1));
    else {
        /*  (',' test)* [',']  */
        int i = 1;
        while (ok && nch - i >= 2) {
            ok = (validate_comma(CHILD(tree, i))
                  && validate_test_or_star_expr(CHILD(tree, i+1)));
            i += 2;
        }
        if (ok && i == nch-1)
            ok = validate_comma(CHILD(tree, i));
        else if (i != nch) {
            ok = 0;
            err_string("illegal trailing nodes for testlist_comp");
        }
    }
    return ok;
}

/*  decorator:
 *    '@' dotted_name [ '(' [arglist] ')' ] NEWLINE
 */
static int
validate_decorator(node *tree)
{
    int ok;
    int nch = NCH(tree);
    ok = (validate_ntype(tree, decorator) &&
          (nch == 3 || nch == 5 || nch == 6) &&
          validate_at(CHILD(tree, 0)) &&
          validate_dotted_name(CHILD(tree, 1)) &&
          validate_newline(RCHILD(tree, -1)));

    if (ok && nch != 3) {
        ok = (validate_lparen(CHILD(tree, 2)) &&
              validate_rparen(RCHILD(tree, -2)));

        if (ok && nch == 6)
            ok = validate_arglist(CHILD(tree, 3));
    }

    return ok;
}

/*  decorators:
 *    decorator+
 */
static int
validate_decorators(node *tree)
{
    int i, nch, ok;
    nch = NCH(tree);
    ok = validate_ntype(tree, decorators) && nch >= 1;

    for (i = 0; ok && i < nch; ++i)
        ok = validate_decorator(CHILD(tree, i));

    return ok;
}

/*  with_item:
 *   test ['as' expr]
 */
static int
validate_with_item(node *tree)
{
    int nch = NCH(tree);
    int ok = (validate_ntype(tree, with_item)
              && (nch == 1 || nch == 3)
              && validate_test(CHILD(tree, 0)));
    if (ok && nch == 3)
        ok = (validate_name(CHILD(tree, 1), "as")
              && validate_expr(CHILD(tree, 2)));
    return ok;
}

/*  with_stmt:
 *    0      1          ...             -2   -1
 *   'with' with_item (',' with_item)* ':' suite
 */
static int
validate_with_stmt(node *tree)
{
    int i;
    int nch = NCH(tree);
    int ok = (validate_ntype(tree, with_stmt)
        && (nch % 2 == 0)
        && validate_name(CHILD(tree, 0), "with")
        && validate_colon(RCHILD(tree, -2))
        && validate_suite(RCHILD(tree, -1)));
    for (i = 1; ok && i < nch - 2; i += 2)
        ok = validate_with_item(CHILD(tree, i));
    return ok;
}

/* funcdef: 'def' NAME parameters ['->' test] ':' suite */

static int
validate_funcdef(node *tree)
{
    int nch = NCH(tree);
    int res = validate_ntype(tree, funcdef);
    if (res) {
        if (nch == 5) {
            res = (validate_name(CHILD(tree, 0), "def")
                   && validate_ntype(CHILD(tree, 1), NAME)
                   && validate_parameters(CHILD(tree, 2))
                   && validate_colon(CHILD(tree, 3))
                   && validate_suite(CHILD(tree, 4)));
        }
        else if (nch == 7) {
            res = (validate_name(CHILD(tree, 0), "def")
                   && validate_ntype(CHILD(tree, 1), NAME)
                   && validate_parameters(CHILD(tree, 2))
                   && validate_rarrow(CHILD(tree, 3))
                   && validate_test(CHILD(tree, 4))
                   && validate_colon(CHILD(tree, 5))
                   && validate_suite(CHILD(tree, 6)));
        }
        else {
            res = 0;
            err_string("illegal number of children for funcdef");
        }
    }
    return res;
}


/* decorated
 *   decorators (classdef | funcdef)
 */
static int
validate_decorated(node *tree)
{
    int nch = NCH(tree);
    int ok = (validate_ntype(tree, decorated)
              && (nch == 2)
              && validate_decorators(RCHILD(tree, -2)));
    if (TYPE(RCHILD(tree, -1)) == funcdef)
        ok = ok && validate_funcdef(RCHILD(tree, -1));
    else
        ok = ok && validate_class(RCHILD(tree, -1));
    return ok;
}

static int
validate_lambdef(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, lambdef)
               && ((nch == 3) || (nch == 4))
               && validate_name(CHILD(tree, 0), "lambda")
               && validate_colon(CHILD(tree, nch - 2))
               && validate_test(CHILD(tree, nch - 1)));

    if (res && (nch == 4))
        res = validate_varargslist(CHILD(tree, 1));
    else if (!res && !PyErr_Occurred())
        (void) validate_numnodes(tree, 3, "lambdef");

    return (res);
}


static int
validate_lambdef_nocond(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, lambdef_nocond)
               && ((nch == 3) || (nch == 4))
               && validate_name(CHILD(tree, 0), "lambda")
               && validate_colon(CHILD(tree, nch - 2))
               && validate_test(CHILD(tree, nch - 1)));

    if (res && (nch == 4))
        res = validate_varargslist(CHILD(tree, 1));
    else if (!res && !PyErr_Occurred())
        (void) validate_numnodes(tree, 3, "lambdef_nocond");

    return (res);
}


/*  arglist:
 *
 *  (argument ',')* (argument [','] | '*' test [',' '**' test] | '**' test)
 */
static int
validate_arglist(node *tree)
{
    int nch = NCH(tree);
    int i = 0;
    int ok = 1;

    if (nch <= 0)
        /* raise the right error from having an invalid number of children */
        return validate_numnodes(tree, nch + 1, "arglist");

    if (nch > 1) {
        for (i=0; i<nch; i++) {
            if (TYPE(CHILD(tree, i)) == argument) {
                node *ch = CHILD(tree, i);
                if (NCH(ch) == 2 && TYPE(CHILD(ch, 1)) == comp_for) {
                    err_string("need '(', ')' for generator expression");
                    return 0;
                }
            }
        }
    }

    while (ok && nch-i >= 2) {
        /* skip leading (argument ',') */
        ok = (validate_argument(CHILD(tree, i))
              && validate_comma(CHILD(tree, i+1)));
        if (ok)
            i += 2;
        else
            PyErr_Clear();
    }
    ok = 1;
    if (nch-i > 0) {
        /*
         * argument | '*' test [',' '**' test] | '**' test
         */
        int sym = TYPE(CHILD(tree, i));

        if (sym == argument) {
            ok = validate_argument(CHILD(tree, i));
            if (ok && i+1 != nch) {
                err_string("illegal arglist specification"
                           " (extra stuff on end)");
                ok = 0;
            }
        }
        else if (sym == STAR) {
            ok = validate_star(CHILD(tree, i));
            if (ok && (nch-i == 2))
                ok = validate_test(CHILD(tree, i+1));
            else if (ok && (nch-i == 5))
                ok = (validate_test(CHILD(tree, i+1))
                      && validate_comma(CHILD(tree, i+2))
                      && validate_doublestar(CHILD(tree, i+3))
                      && validate_test(CHILD(tree, i+4)));
            else {
                err_string("illegal use of '*' in arglist");
                ok = 0;
            }
        }
        else if (sym == DOUBLESTAR) {
            if (nch-i == 2)
                ok = (validate_doublestar(CHILD(tree, i))
                      && validate_test(CHILD(tree, i+1)));
            else {
                err_string("illegal use of '**' in arglist");
                ok = 0;
            }
        }
        else {
            err_string("illegal arglist specification");
            ok = 0;
        }
    }
    return (ok);
}



/*  argument:
 *
 *  [test '='] test [comp_for]
 */
static int
validate_argument(node *tree)
{
    int nch = NCH(tree);
    int res = (validate_ntype(tree, argument)
               && ((nch == 1) || (nch == 2) || (nch == 3)));
    if (res) 
        res = validate_test(CHILD(tree, 0));
    if (res && (nch == 2))
        res = validate_comp_for(CHILD(tree, 1));
    else if (res && (nch == 3))
        res = (validate_equal(CHILD(tree, 1))
               && validate_test(CHILD(tree, 2)));

    return (res);
}



/*  trailer:
 *
 *  '(' [arglist] ')' | '[' subscriptlist ']' | '.' NAME
 */
static int
validate_trailer(node *tree)
{
    int nch = NCH(tree);
    int res = validate_ntype(tree, trailer) && ((nch == 2) || (nch == 3));

    if (res) {
        switch (TYPE(CHILD(tree, 0))) {
          case LPAR:
            res = validate_rparen(CHILD(tree, nch - 1));
            if (res && (nch == 3))
                res = validate_arglist(CHILD(tree, 1));
            break;
          case LSQB:
            res = (validate_numnodes(tree, 3, "trailer")
                   && validate_subscriptlist(CHILD(tree, 1))
                   && validate_ntype(CHILD(tree, 2), RSQB));
            break;
          case DOT:
            res = (validate_numnodes(tree, 2, "trailer")
                   && validate_ntype(CHILD(tree, 1), NAME));
            break;
          default:
            res = 0;
            break;
        }
    }
    else {
        (void) validate_numnodes(tree, 2, "trailer");
    }
    return (res);
}


/*  subscriptlist:
 *
 *  subscript (',' subscript)* [',']
 */
static int
validate_subscriptlist(node *tree)
{
    return (validate_repeating_list(tree, subscriptlist,
                                    validate_subscript, "subscriptlist"));
}


/*  subscript:
 *
 *  '.' '.' '.' | test | [test] ':' [test] [sliceop]
 */
static int
validate_subscript(node *tree)
{
    int offset = 0;
    int nch = NCH(tree);
    int res = validate_ntype(tree, subscript) && (nch >= 1) && (nch <= 4);

    if (!res) {
        if (!PyErr_Occurred())
            err_string("invalid number of arguments for subscript node");
        return (0);
    }
    if (TYPE(CHILD(tree, 0)) == DOT)
        /* take care of ('.' '.' '.') possibility */
        return (validate_numnodes(tree, 3, "subscript")
                && validate_dot(CHILD(tree, 0))
                && validate_dot(CHILD(tree, 1))
                && validate_dot(CHILD(tree, 2)));
    if (nch == 1) {
        if (TYPE(CHILD(tree, 0)) == test)
            res = validate_test(CHILD(tree, 0));
        else
            res = validate_colon(CHILD(tree, 0));
        return (res);
    }
    /*  Must be [test] ':' [test] [sliceop],
     *  but at least one of the optional components will
     *  be present, but we don't know which yet.
     */
    if ((TYPE(CHILD(tree, 0)) != COLON) || (nch == 4)) {
        res = validate_test(CHILD(tree, 0));
        offset = 1;
    }
    if (res)
        res = validate_colon(CHILD(tree, offset));
    if (res) {
        int rem = nch - ++offset;
        if (rem) {
            if (TYPE(CHILD(tree, offset)) == test) {
                res = validate_test(CHILD(tree, offset));
                ++offset;
                --rem;
            }
            if (res && rem)
                res = validate_sliceop(CHILD(tree, offset));
        }
    }
    return (res);
}


static int
validate_sliceop(node *tree)
{
    int nch = NCH(tree);
    int res = ((nch == 1) || validate_numnodes(tree, 2, "sliceop"))
              && validate_ntype(tree, sliceop);
    if (!res && !PyErr_Occurred()) {
        res = validate_numnodes(tree, 1, "sliceop");
    }
    if (res)
        res = validate_colon(CHILD(tree, 0));
    if (res && (nch == 2))
        res = validate_test(CHILD(tree, 1));

    return (res);
}


static int
validate_test_or_star_expr(node *n)
{
    if (TYPE(n) == test)
        return validate_test(n);
    return validate_star_expr(n);
}

static int
validate_expr_or_star_expr(node *n)
{
    if (TYPE(n) == expr)
        return validate_expr(n);
    return validate_star_expr(n);
}


static int
validate_exprlist(node *tree)
{
    return (validate_repeating_list(tree, exprlist,
                                    validate_expr_or_star_expr, "exprlist"));
}

/*
 *  dictorsetmaker:
 *
 *  (test ':' test (comp_for | (',' test ':' test)* [','])) |
 *  (test (comp_for | (',' test)* [',']))
 */
static int
validate_dictorsetmaker(node *tree)
{
    int nch = NCH(tree);
    int res;
    int i = 0;

    res = validate_ntype(tree, dictorsetmaker);
    if (!res)
        return 0;

    if (nch - i < 1) {
        (void) validate_numnodes(tree, 1, "dictorsetmaker");
        return 0;
    }

    res = validate_test(CHILD(tree, i++));
    if (!res)
        return 0;

    if (nch - i >= 2 && TYPE(CHILD(tree, i)) == COLON) {
        /* Dictionary display or dictionary comprehension. */
        res = (validate_colon(CHILD(tree, i++))
               && validate_test(CHILD(tree, i++)));
        if (!res)
            return 0;

        if (nch - i >= 1 && TYPE(CHILD(tree, i)) == comp_for) {
            /* Dictionary comprehension. */
            res = validate_comp_for(CHILD(tree, i++));
            if (!res)
                return 0;
        }
        else {
            /* Dictionary display. */
            while (nch - i >= 4) {
                res = (validate_comma(CHILD(tree, i++))
                       && validate_test(CHILD(tree, i++))
                       && validate_colon(CHILD(tree, i++))
                       && validate_test(CHILD(tree, i++)));
                if (!res)
                    return 0;
            }
            if (nch - i == 1) {
                res = validate_comma(CHILD(tree, i++));
                if (!res)
                    return 0;
            }
        }
    }
    else {
        /* Set display or set comprehension. */
        if (nch - i >= 1 && TYPE(CHILD(tree, i)) == comp_for) {
            /* Set comprehension. */
            res = validate_comp_for(CHILD(tree, i++));
            if (!res)
                return 0;
        }
        else {
            /* Set display. */
            while (nch - i >= 2) {
                res = (validate_comma(CHILD(tree, i++))
                       && validate_test(CHILD(tree, i++)));
                if (!res)
                    return 0;
            }
            if (nch - i == 1) {
                res = validate_comma(CHILD(tree, i++));
                if (!res)
                    return 0;
            }
        }
    }

    if (nch - i > 0) {
        err_string("Illegal trailing nodes for dictorsetmaker.");
        return 0;
    }

    return 1;
}


static int
validate_eval_input(node *tree)
{
    int pos;
    int nch = NCH(tree);
    int res = (validate_ntype(tree, eval_input)
               && (nch >= 2)
               && validate_testlist(CHILD(tree, 0))
               && validate_ntype(CHILD(tree, nch - 1), ENDMARKER));

    for (pos = 1; res && (pos < (nch - 1)); ++pos)
        res = validate_ntype(CHILD(tree, pos), NEWLINE);

    return (res);
}


static int
validate_node(node *tree)
{
    int   nch  = 0;                     /* num. children on current node  */
    int   res  = 1;                     /* result value                   */
    node* next = 0;                     /* node to process after this one */

    while (res && (tree != 0)) {
        nch  = NCH(tree);
        next = 0;
        switch (TYPE(tree)) {
            /*
             *  Definition nodes.
             */
          case funcdef:
            res = validate_funcdef(tree);
            break;
          case with_stmt:
            res = validate_with_stmt(tree);
            break;
          case classdef:
            res = validate_class(tree);
            break;
          case decorated:
            res = validate_decorated(tree);
            break;
            /*
             *  "Trivial" parse tree nodes.
             *  (Why did I call these trivial?)
             */
          case stmt:
            res = validate_stmt(tree);
            break;
          case small_stmt:
            /*
             *  expr_stmt | del_stmt | pass_stmt | flow_stmt |
             *  import_stmt | global_stmt | nonlocal_stmt | assert_stmt
             */
            res = validate_small_stmt(tree);
            break;
          case flow_stmt:
            res  = (validate_numnodes(tree, 1, "flow_stmt")
                    && ((TYPE(CHILD(tree, 0)) == break_stmt)
                        || (TYPE(CHILD(tree, 0)) == continue_stmt)
                        || (TYPE(CHILD(tree, 0)) == yield_stmt)
                        || (TYPE(CHILD(tree, 0)) == return_stmt)
                        || (TYPE(CHILD(tree, 0)) == raise_stmt)));
            if (res)
                next = CHILD(tree, 0);
            else if (nch == 1)
                err_string("illegal flow_stmt type");
            break;
          case yield_stmt:
            res = validate_yield_stmt(tree);
            break;
            /*
             *  Compound statements.
             */
          case simple_stmt:
            res = validate_simple_stmt(tree);
            break;
          case compound_stmt:
            res = validate_compound_stmt(tree);
            break;
            /*
             *  Fundamental statements.
             */
          case expr_stmt:
            res = validate_expr_stmt(tree);
            break;
          case del_stmt:
            res = validate_del_stmt(tree);
            break;
          case pass_stmt:
            res = (validate_numnodes(tree, 1, "pass")
                   && validate_name(CHILD(tree, 0), "pass"));
            break;
          case break_stmt:
            res = (validate_numnodes(tree, 1, "break")
                   && validate_name(CHILD(tree, 0), "break"));
            break;
          case continue_stmt:
            res = (validate_numnodes(tree, 1, "continue")
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
          case import_name:
            res = validate_import_name(tree);
            break;
          case import_from:
            res = validate_import_from(tree);
            break;
          case global_stmt:
            res = validate_global_stmt(tree);
            break;
          case nonlocal_stmt:
            res = validate_nonlocal_stmt(tree);
            break;
          case assert_stmt:
            res = validate_assert_stmt(tree);
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
          case yield_expr:
            res = validate_yield_expr(tree);
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
          case comp_op:
            res = validate_comp_op(tree);
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
          case power:
            res = validate_power(tree);
            break;
          case atom:
            res = validate_atom(tree);
            break;

          default:
            /* Hopefully never reached! */
            err_string("unrecognized node type");
            res = 0;
            break;
        }
        tree = next;
    }
    return (res);
}


static int
validate_expr_tree(node *tree)
{
    int res = validate_eval_input(tree);

    if (!res && !PyErr_Occurred())
        err_string("could not validate expression tuple");

    return (res);
}


/*  file_input:
 *      (NEWLINE | stmt)* ENDMARKER
 */
static int
validate_file_input(node *tree)
{
    int j;
    int nch = NCH(tree) - 1;
    int res = ((nch >= 0)
               && validate_ntype(CHILD(tree, nch), ENDMARKER));

    for (j = 0; res && (j < nch); ++j) {
        if (TYPE(CHILD(tree, j)) == stmt)
            res = validate_stmt(CHILD(tree, j));
        else
            res = validate_newline(CHILD(tree, j));
    }
    /*  This stays in to prevent any internal failures from getting to the
     *  user.  Hopefully, this won't be needed.  If a user reports getting
     *  this, we have some debugging to do.
     */
    if (!res && !PyErr_Occurred())
        err_string("VALIDATION FAILURE: report this to the maintainer!");

    return (res);
}

static int
validate_encoding_decl(node *tree)
{
    int nch = NCH(tree);
    int res = ((nch == 1)
        && validate_file_input(CHILD(tree, 0)));

    if (!res && !PyErr_Occurred())
        err_string("Error Parsing encoding_decl");

    return res;
}

static PyObject*
pickle_constructor = NULL;


static PyObject*
parser__pickler(PyObject *self, PyObject *args)
{
    NOTE(ARGUNUSED(self))
    PyObject *result = NULL;
    PyObject *st = NULL;
    PyObject *empty_dict = NULL;

    if (PyArg_ParseTuple(args, "O!:_pickler", &PyST_Type, &st)) {
        PyObject *newargs;
        PyObject *tuple;

        if ((empty_dict = PyDict_New()) == NULL)
            goto finally;
        if ((newargs = Py_BuildValue("Oi", st, 1)) == NULL)
            goto finally;
        tuple = parser_st2tuple((PyST_Object*)NULL, newargs, empty_dict);
        if (tuple != NULL) {
            result = Py_BuildValue("O(O)", pickle_constructor, tuple);
            Py_DECREF(tuple);
        }
        Py_DECREF(empty_dict);
        Py_DECREF(newargs);
    }
  finally:
    Py_XDECREF(empty_dict);

    return (result);
}


/*  Functions exported by this module.  Most of this should probably
 *  be converted into an ST object with methods, but that is better
 *  done directly in Python, allowing subclasses to be created directly.
 *  We'd really have to write a wrapper around it all anyway to allow
 *  inheritance.
 */
static PyMethodDef parser_functions[] =  {
    {"compilest",      (PyCFunction)parser_compilest,  PUBLIC_METHOD_TYPE,
        PyDoc_STR("Compiles an ST object into a code object.")},
    {"expr",            (PyCFunction)parser_expr,      PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates an ST object from an expression.")},
    {"isexpr",          (PyCFunction)parser_isexpr,    PUBLIC_METHOD_TYPE,
        PyDoc_STR("Determines if an ST object was created from an expression.")},
    {"issuite",         (PyCFunction)parser_issuite,   PUBLIC_METHOD_TYPE,
        PyDoc_STR("Determines if an ST object was created from a suite.")},
    {"suite",           (PyCFunction)parser_suite,     PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates an ST object from a suite.")},
    {"sequence2st",     (PyCFunction)parser_tuple2st,  PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates an ST object from a tree representation.")},
    {"st2tuple",        (PyCFunction)parser_st2tuple,  PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates a tuple-tree representation of an ST.")},
    {"st2list",         (PyCFunction)parser_st2list,   PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates a list-tree representation of an ST.")},
    {"tuple2st",        (PyCFunction)parser_tuple2st,  PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates an ST object from a tree representation.")},

    /* private stuff: support pickle module */
    {"_pickler",        (PyCFunction)parser__pickler,  METH_VARARGS,
        PyDoc_STR("Returns the pickle magic to allow ST objects to be pickled.")},

    {NULL, NULL, 0, NULL}
    };



static struct PyModuleDef parsermodule = {
        PyModuleDef_HEAD_INIT,
        "parser",
        NULL,
        -1,
        parser_functions,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC PyInit_parser(void);  /* supply a prototype */

PyMODINIT_FUNC
PyInit_parser(void)
{
    PyObject *module, *copyreg;

    if (PyType_Ready(&PyST_Type) < 0)
        return NULL;
    module = PyModule_Create(&parsermodule);
    if (module == NULL)
        return NULL;

    if (parser_error == 0)
        parser_error = PyErr_NewException("parser.ParserError", NULL, NULL);

    if (parser_error == 0)
        return NULL;
    /* CAUTION:  The code next used to skip bumping the refcount on
     * parser_error.  That's a disaster if PyInit_parser() gets called more
     * than once.  By incref'ing, we ensure that each module dict that
     * gets created owns its reference to the shared parser_error object,
     * and the file static parser_error vrbl owns a reference too.
     */
    Py_INCREF(parser_error);
    if (PyModule_AddObject(module, "ParserError", parser_error) != 0)
        return NULL;

    Py_INCREF(&PyST_Type);
    PyModule_AddObject(module, "STType", (PyObject*)&PyST_Type);

    PyModule_AddStringConstant(module, "__copyright__",
                               parser_copyright_string);
    PyModule_AddStringConstant(module, "__doc__",
                               parser_doc_string);
    PyModule_AddStringConstant(module, "__version__",
                               parser_version_string);

    /* Register to support pickling.
     * If this fails, the import of this module will fail because an
     * exception will be raised here; should we clear the exception?
     */
    copyreg = PyImport_ImportModuleNoBlock("copyreg");
    if (copyreg != NULL) {
        PyObject *func, *pickler;
        _Py_IDENTIFIER(pickle);
        _Py_IDENTIFIER(sequence2st);
        _Py_IDENTIFIER(_pickler);

        func = _PyObject_GetAttrId(copyreg, &PyId_pickle);
        pickle_constructor = _PyObject_GetAttrId(module, &PyId_sequence2st);
        pickler = _PyObject_GetAttrId(module, &PyId__pickler);
        Py_XINCREF(pickle_constructor);
        if ((func != NULL) && (pickle_constructor != NULL)
            && (pickler != NULL)) {
            PyObject *res;

            res = PyObject_CallFunctionObjArgs(func, &PyST_Type, pickler,
                                               pickle_constructor, NULL);
            Py_XDECREF(res);
        }
        Py_XDECREF(func);
        Py_XDECREF(pickle_constructor);
        Py_XDECREF(pickler);
        Py_DECREF(copyreg);
    }
    return module;
}
