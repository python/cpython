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
 *
 */

#include "Python.h"                     /* general Python API             */
#include "Python-ast.h"                 /* mod_ty */
#undef Yield   /* undefine macro conflicting with <winbase.h> */
#include "ast.h"
#include "graminit.h"                   /* symbols defined in the grammar */
#include "node.h"                       /* internal parser structure      */
#include "errcode.h"                    /* error codes for PyNode_*()     */
#include "token.h"                      /* token definitions              */
                                        /* ISTERMINAL() / ISNONTERMINAL() */
#include "grammar.h"
#include "parsetok.h"

extern grammar _PyParser_Grammar; /* From graminit.c */

#ifdef lint
#include <note.h>
#else
#define NOTE(x)
#endif

/*  String constants used to initialize module attributes.
 *
 */
static const char parser_copyright_string[] =
"Copyright 1995-1996 by Virginia Polytechnic Institute & State\n\
University, Blacksburg, Virginia, USA, and Fred L. Drake, Jr., Reston,\n\
Virginia, USA.  Portions copyright 1991-1995 by Stichting Mathematisch\n\
Centrum, Amsterdam, The Netherlands.";


PyDoc_STRVAR(parser_doc_string,
"This is an interface to Python's internal parser.");

static const char parser_version_string[] = "0.5";


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
    PyObject *result = NULL, *w;

    if (n == NULL) {
        Py_RETURN_NONE;
    }

    if (ISNONTERMINAL(TYPE(n))) {
        int i;

        result = mkseq(1 + NCH(n) + (TYPE(n) == encoding_decl));
        if (result == NULL)
            goto error;

        w = PyLong_FromLong(TYPE(n));
        if (w == NULL)
            goto error;
        (void) addelem(result, 0, w);

        for (i = 0; i < NCH(n); i++) {
            w = node2tuple(CHILD(n, i), mkseq, addelem, lineno, col_offset);
            if (w == NULL)
                goto error;
            (void) addelem(result, i+1, w);
        }

        if (TYPE(n) == encoding_decl) {
            w = PyUnicode_FromString(STR(n));
            if (w == NULL)
                goto error;
            (void) addelem(result, i+1, w);
        }
    }
    else if (ISTERMINAL(TYPE(n))) {
        result = mkseq(2 + lineno + col_offset);
        if (result == NULL)
            goto error;

        w = PyLong_FromLong(TYPE(n));
        if (w == NULL)
            goto error;
        (void) addelem(result, 0, w);

        w = PyUnicode_FromString(STR(n));
        if (w == NULL)
            goto error;
        (void) addelem(result, 1, w);

        if (lineno) {
            w = PyLong_FromLong(n->n_lineno);
            if (w == NULL)
                goto error;
            (void) addelem(result, 2, w);
        }

        if (col_offset) {
            w = PyLong_FromLong(n->n_col_offset);
            if (w == NULL)
                goto error;
            (void) addelem(result, 2 + lineno, w);
        }
    }
    else {
        PyErr_SetString(PyExc_SystemError,
                        "unrecognized parse tree node type");
        return ((PyObject*) NULL);
    }
    return result;

error:
    Py_XDECREF(result);
    return NULL;
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
static PyObject* parser_sizeof(PyST_Object *, void *);
static PyObject* parser_richcompare(PyObject *left, PyObject *right, int op);
static PyObject* parser_compilest(PyST_Object *, PyObject *, PyObject *);
static PyObject* parser_isexpr(PyST_Object *, PyObject *, PyObject *);
static PyObject* parser_issuite(PyST_Object *, PyObject *, PyObject *);
static PyObject* parser_st2list(PyST_Object *, PyObject *, PyObject *);
static PyObject* parser_st2tuple(PyST_Object *, PyObject *, PyObject *);

#define PUBLIC_METHOD_TYPE (METH_VARARGS|METH_KEYWORDS)

static PyMethodDef parser_methods[] = {
    {"compile",         (PyCFunction)(void(*)(void))parser_compilest,  PUBLIC_METHOD_TYPE,
        PyDoc_STR("Compile this ST object into a code object.")},
    {"isexpr",          (PyCFunction)(void(*)(void))parser_isexpr,     PUBLIC_METHOD_TYPE,
        PyDoc_STR("Determines if this ST object was created from an expression.")},
    {"issuite",         (PyCFunction)(void(*)(void))parser_issuite,    PUBLIC_METHOD_TYPE,
        PyDoc_STR("Determines if this ST object was created from a suite.")},
    {"tolist",          (PyCFunction)(void(*)(void))parser_st2list,    PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates a list-tree representation of this ST.")},
    {"totuple",         (PyCFunction)(void(*)(void))parser_st2tuple,   PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates a tuple-tree representation of this ST.")},
    {"__sizeof__",      (PyCFunction)parser_sizeof,     METH_NOARGS,
        PyDoc_STR("Returns size in memory, in bytes.")},
    {NULL, NULL, 0, NULL}
};

static
PyTypeObject PyST_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "parser.st",                        /* tp_name              */
    (int) sizeof(PyST_Object),          /* tp_basicsize         */
    0,                                  /* tp_itemsize          */
    (destructor)parser_free,            /* tp_dealloc           */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr           */
    0,                                  /* tp_setattr           */
    0,                                  /* tp_as_async          */
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
#define PyST_Object_Check(v) (Py_TYPE(v) == &PyST_Type)

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

static PyObject *
parser_richcompare(PyObject *left, PyObject *right, int op)
{
    int result;

    /* neither argument should be NULL, unless something's gone wrong */
    if (left == NULL || right == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    /* both arguments should be instances of PyST_Object */
    if (!PyST_Object_Check(left) || !PyST_Object_Check(right)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (left == right)
        /* if arguments are identical, they're equal */
        result = 0;
    else
        result = parser_compare_nodes(((PyST_Object *)left)->st_node,
                                      ((PyST_Object *)right)->st_node);

    Py_RETURN_RICHCOMPARE(result, 0, op);
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
        o->st_flags = _PyCompilerFlags_INIT;
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

static PyObject *
parser_sizeof(PyST_Object *st, void *unused)
{
    Py_ssize_t res;

    res = _PyObject_SIZE(Py_TYPE(st)) + _PyNode_SizeOf(st->st_node);
    return PyLong_FromSsize_t(res);
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
    int line_info = 0;
    int col_info = 0;
    PyObject *res = 0;
    int ok;

    static char *keywords[] = {"st", "line_info", "col_info", NULL};

    if (self == NULL || PyModule_Check(self)) {
        ok = PyArg_ParseTupleAndKeywords(args, kw, "O!|pp:st2tuple", keywords,
                                         &PyST_Type, &self, &line_info,
                                         &col_info);
    }
    else
        ok = PyArg_ParseTupleAndKeywords(args, kw, "|pp:totuple", &keywords[1],
                                         &line_info, &col_info);
    if (ok != 0) {
        /*
         *  Convert ST into a tuple representation.  Use Guido's function,
         *  since it's known to work already.
         */
        res = node2tuple(((PyST_Object*)self)->st_node,
                         PyTuple_New, PyTuple_SetItem, line_info, col_info);
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
    int line_info = 0;
    int col_info = 0;
    PyObject *res = 0;
    int ok;

    static char *keywords[] = {"st", "line_info", "col_info", NULL};

    if (self == NULL || PyModule_Check(self))
        ok = PyArg_ParseTupleAndKeywords(args, kw, "O!|pp:st2list", keywords,
                                         &PyST_Type, &self, &line_info,
                                         &col_info);
    else
        ok = PyArg_ParseTupleAndKeywords(args, kw, "|pp:tolist", &keywords[1],
                                         &line_info, &col_info);
    if (ok) {
        /*
         *  Convert ST into a tuple representation.  Use Guido's function,
         *  since it's known to work already.
         */
        res = node2tuple(self->st_node,
                         PyList_New, PyList_SetItem, line_info, col_info);
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
    PyObject*     res = NULL;
    PyArena*      arena = NULL;
    mod_ty        mod;
    PyObject*     filename = NULL;
    int ok;

    static char *keywords[] = {"st", "filename", NULL};

    if (self == NULL || PyModule_Check(self))
        ok = PyArg_ParseTupleAndKeywords(args, kw, "O!|O&:compilest", keywords,
                                         &PyST_Type, &self,
                                         PyUnicode_FSDecoder, &filename);
    else
        ok = PyArg_ParseTupleAndKeywords(args, kw, "|O&:compile", &keywords[1],
                                         PyUnicode_FSDecoder, &filename);
    if (!ok)
        goto error;

    if (filename == NULL) {
        filename = PyUnicode_FromString("<syntax-tree>");
        if (filename == NULL)
            goto error;
    }

    arena = PyArena_New();
    if (!arena)
        goto error;

    mod = PyAST_FromNodeObject(self->st_node, &self->st_flags,
                               filename, arena);
    if (!mod)
        goto error;

    res = (PyObject *)PyAST_CompileObject(mod, filename,
                                          &self->st_flags, -1, arena);
error:
    Py_XDECREF(filename);
    if (arena != NULL)
        PyArena_Free(arena);
    return res;
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


/*  err_string(const char* message)
 *
 *  Sets the error string for an exception of type ParserError.
 *
 */
static void
err_string(const char *message)
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
parser_do_parse(PyObject *args, PyObject *kw, const char *argspec, int type)
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
            if (res) {
                ((PyST_Object *)res)->st_flags.cf_flags = flags & PyCF_MASK;
                ((PyST_Object *)res)->st_flags.cf_feature_version = PY_MINOR_VERSION;
            }
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

static int
validate_node(node *tree)
{
    int type = TYPE(tree);
    int nch = NCH(tree);
    state *dfa_state;
    int pos, arc;

    assert(ISNONTERMINAL(type));
    type -= NT_OFFSET;
    if (type >= _PyParser_Grammar.g_ndfas) {
        PyErr_Format(parser_error, "Unrecognized node type %d.", TYPE(tree));
        return 0;
    }
    const dfa *nt_dfa = &_PyParser_Grammar.g_dfa[type];
    REQ(tree, nt_dfa->d_type);

    /* Run the DFA for this nonterminal. */
    dfa_state = nt_dfa->d_state;
    for (pos = 0; pos < nch; ++pos) {
        node *ch = CHILD(tree, pos);
        int ch_type = TYPE(ch);
        if ((ch_type >= NT_OFFSET + _PyParser_Grammar.g_ndfas)
            || (ISTERMINAL(ch_type) && (ch_type >= N_TOKENS))
            || (ch_type < 0)
           ) {
            PyErr_Format(parser_error, "Unrecognized node type %d.", ch_type);
            return 0;
        }
        if (ch_type == suite && TYPE(tree) == funcdef) {
            /* This is the opposite hack of what we do in parser.c
               (search for func_body_suite), except we don't ever
               support type comments here. */
            ch_type = func_body_suite;
        }
        for (arc = 0; arc < dfa_state->s_narcs; ++arc) {
            short a_label = dfa_state->s_arc[arc].a_lbl;
            assert(a_label < _PyParser_Grammar.g_ll.ll_nlabels);

            const char *label_str = _PyParser_Grammar.g_ll.ll_label[a_label].lb_str;
            if ((_PyParser_Grammar.g_ll.ll_label[a_label].lb_type == ch_type)
                && ((ch->n_str == NULL) || (label_str == NULL)
                     || (strcmp(ch->n_str, label_str) == 0))
               ) {
                /* The child is acceptable; if non-terminal, validate it recursively. */
                if (ISNONTERMINAL(ch_type) && !validate_node(ch))
                    return 0;

                /* Update the state, and move on to the next child. */
                dfa_state = &nt_dfa->d_state[dfa_state->s_arc[arc].a_arrow];
                goto arc_found;
            }
        }
        /* What would this state have accepted? */
        {
            short a_label = dfa_state->s_arc->a_lbl;
            if (!a_label) /* Wouldn't accept any more children */
                goto illegal_num_children;

            int next_type = _PyParser_Grammar.g_ll.ll_label[a_label].lb_type;
            const char *expected_str = _PyParser_Grammar.g_ll.ll_label[a_label].lb_str;

            if (ISNONTERMINAL(next_type)) {
                PyErr_Format(parser_error, "Expected %s, got %s.",
                             _PyParser_Grammar.g_dfa[next_type - NT_OFFSET].d_name,
                             ISTERMINAL(ch_type) ? _PyParser_TokenNames[ch_type] :
                             _PyParser_Grammar.g_dfa[ch_type - NT_OFFSET].d_name);
            }
            else if (expected_str != NULL) {
                PyErr_Format(parser_error, "Illegal terminal: expected '%s'.",
                             expected_str);
            }
            else {
                PyErr_Format(parser_error, "Illegal terminal: expected %s.",
                             _PyParser_TokenNames[next_type]);
            }
            return 0;
        }

arc_found:
        continue;
    }
    /* Are we in a final state? If so, return 1 for successful validation. */
    for (arc = 0; arc < dfa_state->s_narcs; ++arc) {
        if (!dfa_state->s_arc[arc].a_lbl) {
            return 1;
        }
    }

illegal_num_children:
    PyErr_Format(parser_error,
                 "Illegal number of children for %s node.", nt_dfa->d_name);
    return 0;
}

/*  PyObject* parser_tuple2st(PyObject* self, PyObject* args)
 *
 *  This is the public function, called from the Python code.  It receives a
 *  single tuple object from the caller, and creates an ST object if the
 *  tuple can be validated.  It does this by checking the first code of the
 *  tuple, and, if acceptable, builds the internal representation.  If this
 *  step succeeds, the internal representation is validated as fully as
 *  possible with the recursive validate_node() routine defined above.
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
        node *validation_root = NULL;
        int tree_type = 0;
        switch (TYPE(tree)) {
        case eval_input:
            /*  Might be an eval form.  */
            tree_type = PyST_EXPR;
            validation_root = tree;
            break;
        case encoding_decl:
            /* This looks like an encoding_decl so far. */
            if (NCH(tree) == 1) {
                tree_type = PyST_SUITE;
                validation_root = CHILD(tree, 0);
            }
            else {
                err_string("Error Parsing encoding_decl");
            }
            break;
        case file_input:
            /*  This looks like an exec form so far.  */
            tree_type = PyST_SUITE;
            validation_root = tree;
            break;
        default:
            /*  This is a fragment, at best. */
            err_string("parse tree does not use a valid start symbol");
        }

        if (validation_root != NULL && validate_node(validation_root))
            st = parser_newstobject(tree, tree_type);
        else
            PyNode_Free(tree);
    }
    /*  Make sure we raise an exception on all errors.  We should never
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

    if (len < 0) {
        return NULL;
    }
    for (i = 1; i < len; ++i) {
        /* elem must always be a sequence, however simple */
        PyObject* elem = PySequence_GetItem(tuple, i);
        int ok = elem != NULL;
        int type = 0;
        char *strn = 0;

        if (ok)
            ok = PySequence_Check(elem);
        if (ok) {
            PyObject *temp = PySequence_GetItem(elem, 0);
            if (temp == NULL)
                ok = 0;
            else {
                ok = PyLong_Check(temp);
                if (ok) {
                    type = _PyLong_AsInt(temp);
                    if (type == -1 && PyErr_Occurred()) {
                        Py_DECREF(temp);
                        Py_DECREF(elem);
                        return NULL;
                    }
                }
                Py_DECREF(temp);
            }
        }
        if (!ok) {
            PyObject *err = Py_BuildValue("Os", elem,
                                          "Illegal node construct.");
            PyErr_SetObject(parser_error, err);
            Py_XDECREF(err);
            Py_XDECREF(elem);
            return NULL;
        }
        if (ISTERMINAL(type)) {
            Py_ssize_t len = PyObject_Size(elem);
            PyObject *temp;
            const char *temp_str;

            if ((len != 2) && (len != 3)) {
                err_string("terminal nodes must have 2 or 3 entries");
                Py_DECREF(elem);
                return NULL;
            }
            temp = PySequence_GetItem(elem, 1);
            if (temp == NULL) {
                Py_DECREF(elem);
                return NULL;
            }
            if (!PyUnicode_Check(temp)) {
                PyErr_Format(parser_error,
                             "second item in terminal node must be a string,"
                             " found %s",
                             Py_TYPE(temp)->tp_name);
                Py_DECREF(temp);
                Py_DECREF(elem);
                return NULL;
            }
            if (len == 3) {
                PyObject *o = PySequence_GetItem(elem, 2);
                if (o == NULL) {
                    Py_DECREF(temp);
                    Py_DECREF(elem);
                    return NULL;
                }
                if (PyLong_Check(o)) {
                    int num = _PyLong_AsInt(o);
                    if (num == -1 && PyErr_Occurred()) {
                        Py_DECREF(o);
                        Py_DECREF(temp);
                        Py_DECREF(elem);
                        return NULL;
                    }
                    *line_num = num;
                }
                else {
                    PyErr_Format(parser_error,
                                 "third item in terminal node must be an"
                                 " integer, found %s",
                                 Py_TYPE(temp)->tp_name);
                    Py_DECREF(o);
                    Py_DECREF(temp);
                    Py_DECREF(elem);
                    return NULL;
                }
                Py_DECREF(o);
            }
            temp_str = PyUnicode_AsUTF8AndSize(temp, &len);
            if (temp_str == NULL) {
                Py_DECREF(temp);
                Py_DECREF(elem);
                return NULL;
            }
            strn = (char *)PyObject_MALLOC(len + 1);
            if (strn == NULL) {
                Py_DECREF(temp);
                Py_DECREF(elem);
                PyErr_NoMemory();
                return NULL;
            }
            (void) memcpy(strn, temp_str, len + 1);
            Py_DECREF(temp);
        }
        else if (!ISNONTERMINAL(type)) {
            /*
             *  It has to be one or the other; this is an error.
             *  Raise an exception.
             */
            PyObject *err = Py_BuildValue("Os", elem, "unknown node type.");
            PyErr_SetObject(parser_error, err);
            Py_XDECREF(err);
            Py_DECREF(elem);
            return NULL;
        }
        err = PyNode_AddChild(root, type, strn, *line_num, 0, *line_num, 0);
        if (err == E_NOMEM) {
            Py_DECREF(elem);
            PyObject_FREE(strn);
            PyErr_NoMemory();
            return NULL;
        }
        if (err == E_OVERFLOW) {
            Py_DECREF(elem);
            PyObject_FREE(strn);
            PyErr_SetString(PyExc_ValueError,
                            "unsupported number of child nodes");
            return NULL;
        }

        if (ISNONTERMINAL(type)) {
            node* new_child = CHILD(root, i - 1);

            if (new_child != build_node_children(elem, new_child, line_num)) {
                Py_DECREF(elem);
                return NULL;
            }
        }
        else if (type == NEWLINE) {     /* It's true:  we increment the     */
            ++(*line_num);              /* line number *after* the newline! */
        }
        Py_DECREF(elem);
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
         *  Raise an exception now and be done with it.
         */
        tuple = Py_BuildValue("Os", tuple,
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
            if (encoding == NULL) {
                PyErr_SetString(parser_error, "missed encoding");
                return NULL;
            }
            if (!PyUnicode_Check(encoding)) {
                PyErr_Format(parser_error,
                             "encoding must be a string, found %.200s",
                             Py_TYPE(encoding)->tp_name);
                Py_DECREF(encoding);
                return NULL;
            }
            /* tuple isn't borrowed anymore here, need to DECREF */
            tuple = PySequence_GetSlice(tuple, 0, 2);
            if (tuple == NULL) {
                Py_DECREF(encoding);
                return NULL;
            }
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
                temp = PyUnicode_AsUTF8AndSize(encoding, &len);
                if (temp == NULL) {
                    PyNode_Free(res);
                    Py_DECREF(encoding);
                    Py_DECREF(tuple);
                    return NULL;
                }
                res->n_str = (char *)PyObject_MALLOC(len + 1);
                if (res->n_str == NULL) {
                    PyNode_Free(res);
                    Py_DECREF(encoding);
                    Py_DECREF(tuple);
                    PyErr_NoMemory();
                    return NULL;
                }
                (void) memcpy(res->n_str, temp, len + 1);
            }
        }
        if (encoding != NULL) {
            Py_DECREF(encoding);
            Py_DECREF(tuple);
        }
    }
    else {
        /*  The tuple is illegal -- if the number is neither TERMINAL nor
         *  NONTERMINAL, we can't use it.  Not sure the implementation
         *  allows this condition, but the API doesn't preclude it.
         */
        PyObject *err = Py_BuildValue("Os", tuple,
                                      "Illegal component tuple.");
        PyErr_SetObject(parser_error, err);
        Py_XDECREF(err);
    }

    return (res);
}


static PyObject*
pickle_constructor = NULL;


static PyObject*
parser__pickler(PyObject *self, PyObject *args)
{
    NOTE(ARGUNUSED(self))
    PyObject *result = NULL;
    PyObject *st = NULL;

    if (PyArg_ParseTuple(args, "O!:_pickler", &PyST_Type, &st)) {
        PyObject *newargs;
        PyObject *tuple;

        if ((newargs = PyTuple_Pack(2, st, Py_True)) == NULL)
            return NULL;
        tuple = parser_st2tuple((PyST_Object*)NULL, newargs, NULL);
        if (tuple != NULL) {
            result = Py_BuildValue("O(O)", pickle_constructor, tuple);
            Py_DECREF(tuple);
        }
        Py_DECREF(newargs);
    }

    return (result);
}


/*  Functions exported by this module.  Most of this should probably
 *  be converted into an ST object with methods, but that is better
 *  done directly in Python, allowing subclasses to be created directly.
 *  We'd really have to write a wrapper around it all anyway to allow
 *  inheritance.
 */
static PyMethodDef parser_functions[] =  {
    {"compilest",      (PyCFunction)(void(*)(void))parser_compilest,  PUBLIC_METHOD_TYPE,
        PyDoc_STR("Compiles an ST object into a code object.")},
    {"expr",            (PyCFunction)(void(*)(void))parser_expr,      PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates an ST object from an expression.")},
    {"isexpr",          (PyCFunction)(void(*)(void))parser_isexpr,    PUBLIC_METHOD_TYPE,
        PyDoc_STR("Determines if an ST object was created from an expression.")},
    {"issuite",         (PyCFunction)(void(*)(void))parser_issuite,   PUBLIC_METHOD_TYPE,
        PyDoc_STR("Determines if an ST object was created from a suite.")},
    {"suite",           (PyCFunction)(void(*)(void))parser_suite,     PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates an ST object from a suite.")},
    {"sequence2st",     (PyCFunction)(void(*)(void))parser_tuple2st,  PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates an ST object from a tree representation.")},
    {"st2tuple",        (PyCFunction)(void(*)(void))parser_st2tuple,  PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates a tuple-tree representation of an ST.")},
    {"st2list",         (PyCFunction)(void(*)(void))parser_st2list,   PUBLIC_METHOD_TYPE,
        PyDoc_STR("Creates a list-tree representation of an ST.")},
    {"tuple2st",        (PyCFunction)(void(*)(void))parser_tuple2st,  PUBLIC_METHOD_TYPE,
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

    if (PyErr_WarnEx(PyExc_DeprecationWarning,
            "The parser module is deprecated and will be removed "
            "in future versions of Python", 7) != 0) {
        return NULL;
    }

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
