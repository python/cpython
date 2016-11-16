#include "Python.h"
#include "Python-ast.h"
#include "code.h"
#include "symtable.h"
#include "structmember.h"

/* error strings used for warnings */
#define GLOBAL_AFTER_ASSIGN \
"name '%U' is assigned to before global declaration"

#define NONLOCAL_AFTER_ASSIGN \
"name '%U' is assigned to before nonlocal declaration"

#define GLOBAL_AFTER_USE \
"name '%U' is used prior to global declaration"

#define NONLOCAL_AFTER_USE \
"name '%U' is used prior to nonlocal declaration"

#define GLOBAL_ANNOT \
"annotated name '%U' can't be global"

#define NONLOCAL_ANNOT \
"annotated name '%U' can't be nonlocal"

#define IMPORT_STAR_WARNING "import * only allowed at module level"

static PySTEntryObject *
ste_new(struct symtable *st, identifier name, _Py_block_ty block,
        void *key, int lineno, int col_offset)
{
    PySTEntryObject *ste = NULL;
    PyObject *k = NULL;

    k = PyLong_FromVoidPtr(key);
    if (k == NULL)
        goto fail;
    ste = PyObject_New(PySTEntryObject, &PySTEntry_Type);
    if (ste == NULL) {
        Py_DECREF(k);
        goto fail;
    }
    ste->ste_table = st;
    ste->ste_id = k; /* ste owns reference to k */

    Py_INCREF(name);
    ste->ste_name = name;

    ste->ste_symbols = NULL;
    ste->ste_varnames = NULL;
    ste->ste_children = NULL;

    ste->ste_directives = NULL;

    ste->ste_type = block;
    ste->ste_nested = 0;
    ste->ste_free = 0;
    ste->ste_varargs = 0;
    ste->ste_varkeywords = 0;
    ste->ste_opt_lineno = 0;
    ste->ste_opt_col_offset = 0;
    ste->ste_tmpname = 0;
    ste->ste_lineno = lineno;
    ste->ste_col_offset = col_offset;

    if (st->st_cur != NULL &&
        (st->st_cur->ste_nested ||
         st->st_cur->ste_type == FunctionBlock))
        ste->ste_nested = 1;
    ste->ste_child_free = 0;
    ste->ste_generator = 0;
    ste->ste_coroutine = 0;
    ste->ste_returns_value = 0;
    ste->ste_needs_class_closure = 0;

    ste->ste_symbols = PyDict_New();
    ste->ste_varnames = PyList_New(0);
    ste->ste_children = PyList_New(0);
    if (ste->ste_symbols == NULL
        || ste->ste_varnames == NULL
        || ste->ste_children == NULL)
        goto fail;

    if (PyDict_SetItem(st->st_blocks, ste->ste_id, (PyObject *)ste) < 0)
        goto fail;

    return ste;
 fail:
    Py_XDECREF(ste);
    return NULL;
}

static PyObject *
ste_repr(PySTEntryObject *ste)
{
    return PyUnicode_FromFormat("<symtable entry %U(%ld), line %d>",
                                ste->ste_name,
                                PyLong_AS_LONG(ste->ste_id), ste->ste_lineno);
}

static void
ste_dealloc(PySTEntryObject *ste)
{
    ste->ste_table = NULL;
    Py_XDECREF(ste->ste_id);
    Py_XDECREF(ste->ste_name);
    Py_XDECREF(ste->ste_symbols);
    Py_XDECREF(ste->ste_varnames);
    Py_XDECREF(ste->ste_children);
    Py_XDECREF(ste->ste_directives);
    PyObject_Del(ste);
}

#define OFF(x) offsetof(PySTEntryObject, x)

static PyMemberDef ste_memberlist[] = {
    {"id",       T_OBJECT, OFF(ste_id), READONLY},
    {"name",     T_OBJECT, OFF(ste_name), READONLY},
    {"symbols",  T_OBJECT, OFF(ste_symbols), READONLY},
    {"varnames", T_OBJECT, OFF(ste_varnames), READONLY},
    {"children", T_OBJECT, OFF(ste_children), READONLY},
    {"nested",   T_INT,    OFF(ste_nested), READONLY},
    {"type",     T_INT,    OFF(ste_type), READONLY},
    {"lineno",   T_INT,    OFF(ste_lineno), READONLY},
    {NULL}
};

PyTypeObject PySTEntry_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "symtable entry",
    sizeof(PySTEntryObject),
    0,
    (destructor)ste_dealloc,                /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                         /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)ste_repr,                         /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    ste_memberlist,                             /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
};

static int symtable_analyze(struct symtable *st);
static int symtable_enter_block(struct symtable *st, identifier name,
                                _Py_block_ty block, void *ast, int lineno,
                                int col_offset);
static int symtable_exit_block(struct symtable *st, void *ast);
static int symtable_visit_stmt(struct symtable *st, stmt_ty s);
static int symtable_visit_expr(struct symtable *st, expr_ty s);
static int symtable_visit_genexp(struct symtable *st, expr_ty s);
static int symtable_visit_listcomp(struct symtable *st, expr_ty s);
static int symtable_visit_setcomp(struct symtable *st, expr_ty s);
static int symtable_visit_dictcomp(struct symtable *st, expr_ty s);
static int symtable_visit_arguments(struct symtable *st, arguments_ty);
static int symtable_visit_excepthandler(struct symtable *st, excepthandler_ty);
static int symtable_visit_alias(struct symtable *st, alias_ty);
static int symtable_visit_comprehension(struct symtable *st, comprehension_ty);
static int symtable_visit_keyword(struct symtable *st, keyword_ty);
static int symtable_visit_slice(struct symtable *st, slice_ty);
static int symtable_visit_params(struct symtable *st, asdl_seq *args);
static int symtable_visit_argannotations(struct symtable *st, asdl_seq *args);
static int symtable_implicit_arg(struct symtable *st, int pos);
static int symtable_visit_annotations(struct symtable *st, stmt_ty s, arguments_ty, expr_ty);
static int symtable_visit_withitem(struct symtable *st, withitem_ty item);


static identifier top = NULL, lambda = NULL, genexpr = NULL,
    listcomp = NULL, setcomp = NULL, dictcomp = NULL,
    __class__ = NULL;

#define GET_IDENTIFIER(VAR) \
    ((VAR) ? (VAR) : ((VAR) = PyUnicode_InternFromString(# VAR)))

#define DUPLICATE_ARGUMENT \
"duplicate argument '%U' in function definition"

static struct symtable *
symtable_new(void)
{
    struct symtable *st;

    st = (struct symtable *)PyMem_Malloc(sizeof(struct symtable));
    if (st == NULL)
        return NULL;

    st->st_filename = NULL;
    st->st_blocks = NULL;

    if ((st->st_stack = PyList_New(0)) == NULL)
        goto fail;
    if ((st->st_blocks = PyDict_New()) == NULL)
        goto fail;
    st->st_cur = NULL;
    st->st_private = NULL;
    return st;
 fail:
    PySymtable_Free(st);
    return NULL;
}

/* When compiling the use of C stack is probably going to be a lot
   lighter than when executing Python code but still can overflow
   and causing a Python crash if not checked (e.g. eval("()"*300000)).
   Using the current recursion limit for the compiler seems too
   restrictive (it caused at least one test to fail) so a factor is
   used to allow deeper recursion when compiling an expression.

   Using a scaling factor means this should automatically adjust when
   the recursion limit is adjusted for small or large C stack allocations.
*/
#define COMPILER_STACK_FRAME_SCALE 3

struct symtable *
PySymtable_BuildObject(mod_ty mod, PyObject *filename, PyFutureFeatures *future)
{
    struct symtable *st = symtable_new();
    asdl_seq *seq;
    int i;
    PyThreadState *tstate;
    int recursion_limit = Py_GetRecursionLimit();

    if (st == NULL)
        return NULL;
    if (filename == NULL) {
        PySymtable_Free(st);
        return NULL;
    }
    Py_INCREF(filename);
    st->st_filename = filename;
    st->st_future = future;

    /* Setup recursion depth check counters */
    tstate = PyThreadState_GET();
    if (!tstate) {
        PySymtable_Free(st);
        return NULL;
    }
    /* Be careful here to prevent overflow. */
    st->recursion_depth = (tstate->recursion_depth < INT_MAX / COMPILER_STACK_FRAME_SCALE) ?
        tstate->recursion_depth * COMPILER_STACK_FRAME_SCALE : tstate->recursion_depth;
    st->recursion_limit = (recursion_limit < INT_MAX / COMPILER_STACK_FRAME_SCALE) ?
        recursion_limit * COMPILER_STACK_FRAME_SCALE : recursion_limit;

    /* Make the initial symbol information gathering pass */
    if (!GET_IDENTIFIER(top) ||
        !symtable_enter_block(st, top, ModuleBlock, (void *)mod, 0, 0)) {
        PySymtable_Free(st);
        return NULL;
    }

    st->st_top = st->st_cur;
    switch (mod->kind) {
    case Module_kind:
        seq = mod->v.Module.body;
        for (i = 0; i < asdl_seq_LEN(seq); i++)
            if (!symtable_visit_stmt(st,
                        (stmt_ty)asdl_seq_GET(seq, i)))
                goto error;
        break;
    case Expression_kind:
        if (!symtable_visit_expr(st, mod->v.Expression.body))
            goto error;
        break;
    case Interactive_kind:
        seq = mod->v.Interactive.body;
        for (i = 0; i < asdl_seq_LEN(seq); i++)
            if (!symtable_visit_stmt(st,
                        (stmt_ty)asdl_seq_GET(seq, i)))
                goto error;
        break;
    case Suite_kind:
        PyErr_SetString(PyExc_RuntimeError,
                        "this compiler does not handle Suites");
        goto error;
    }
    if (!symtable_exit_block(st, (void *)mod)) {
        PySymtable_Free(st);
        return NULL;
    }
    /* Make the second symbol analysis pass */
    if (symtable_analyze(st))
        return st;
    PySymtable_Free(st);
    return NULL;
 error:
    (void) symtable_exit_block(st, (void *)mod);
    PySymtable_Free(st);
    return NULL;
}

struct symtable *
PySymtable_Build(mod_ty mod, const char *filename_str, PyFutureFeatures *future)
{
    PyObject *filename;
    struct symtable *st;
    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    st = PySymtable_BuildObject(mod, filename, future);
    Py_DECREF(filename);
    return st;
}

void
PySymtable_Free(struct symtable *st)
{
    Py_XDECREF(st->st_filename);
    Py_XDECREF(st->st_blocks);
    Py_XDECREF(st->st_stack);
    PyMem_Free((void *)st);
}

PySTEntryObject *
PySymtable_Lookup(struct symtable *st, void *key)
{
    PyObject *k, *v;

    k = PyLong_FromVoidPtr(key);
    if (k == NULL)
        return NULL;
    v = PyDict_GetItem(st->st_blocks, k);
    if (v) {
        assert(PySTEntry_Check(v));
        Py_INCREF(v);
    }
    else {
        PyErr_SetString(PyExc_KeyError,
                        "unknown symbol table entry");
    }

    Py_DECREF(k);
    return (PySTEntryObject *)v;
}

int
PyST_GetScope(PySTEntryObject *ste, PyObject *name)
{
    PyObject *v = PyDict_GetItem(ste->ste_symbols, name);
    if (!v)
        return 0;
    assert(PyLong_Check(v));
    return (PyLong_AS_LONG(v) >> SCOPE_OFFSET) & SCOPE_MASK;
}

static int
error_at_directive(PySTEntryObject *ste, PyObject *name)
{
    Py_ssize_t i;
    PyObject *data;
    assert(ste->ste_directives);
    for (i = 0; i < PyList_GET_SIZE(ste->ste_directives); i++) {
        data = PyList_GET_ITEM(ste->ste_directives, i);
        assert(PyTuple_CheckExact(data));
        assert(PyUnicode_CheckExact(PyTuple_GET_ITEM(data, 0)));
        if (PyUnicode_Compare(PyTuple_GET_ITEM(data, 0), name) == 0) {
            PyErr_SyntaxLocationObject(ste->ste_table->st_filename,
                                       PyLong_AsLong(PyTuple_GET_ITEM(data, 1)),
                                       PyLong_AsLong(PyTuple_GET_ITEM(data, 2)));

            return 0;
        }
    }
    PyErr_SetString(PyExc_RuntimeError,
                    "BUG: internal directive bookkeeping broken");
    return 0;
}


/* Analyze raw symbol information to determine scope of each name.

   The next several functions are helpers for symtable_analyze(),
   which determines whether a name is local, global, or free.  In addition,
   it determines which local variables are cell variables; they provide
   bindings that are used for free variables in enclosed blocks.

   There are also two kinds of global variables, implicit and explicit.  An
   explicit global is declared with the global statement.  An implicit
   global is a free variable for which the compiler has found no binding
   in an enclosing function scope.  The implicit global is either a global
   or a builtin.  Python's module and class blocks use the xxx_NAME opcodes
   to handle these names to implement slightly odd semantics.  In such a
   block, the name is treated as global until it is assigned to; then it
   is treated as a local.

   The symbol table requires two passes to determine the scope of each name.
   The first pass collects raw facts from the AST via the symtable_visit_*
   functions: the name is a parameter here, the name is used but not defined
   here, etc.  The second pass analyzes these facts during a pass over the
   PySTEntryObjects created during pass 1.

   When a function is entered during the second pass, the parent passes
   the set of all name bindings visible to its children.  These bindings
   are used to determine if non-local variables are free or implicit globals.
   Names which are explicitly declared nonlocal must exist in this set of
   visible names - if they do not, a syntax error is raised. After doing
   the local analysis, it analyzes each of its child blocks using an
   updated set of name bindings.

   The children update the free variable set.  If a local variable is added to
   the free variable set by the child, the variable is marked as a cell.  The
   function object being defined must provide runtime storage for the variable
   that may outlive the function's frame.  Cell variables are removed from the
   free set before the analyze function returns to its parent.

   During analysis, the names are:
      symbols: dict mapping from symbol names to flag values (including offset scope values)
      scopes: dict mapping from symbol names to scope values (no offset)
      local: set of all symbol names local to the current scope
      bound: set of all symbol names local to a containing function scope
      free: set of all symbol names referenced but not bound in child scopes
      global: set of all symbol names explicitly declared as global
*/

#define SET_SCOPE(DICT, NAME, I) { \
    PyObject *o = PyLong_FromLong(I); \
    if (!o) \
        return 0; \
    if (PyDict_SetItem((DICT), (NAME), o) < 0) { \
        Py_DECREF(o); \
        return 0; \
    } \
    Py_DECREF(o); \
}

/* Decide on scope of name, given flags.

   The namespace dictionaries may be modified to record information
   about the new name.  For example, a new global will add an entry to
   global.  A name that was global can be changed to local.
*/

static int
analyze_name(PySTEntryObject *ste, PyObject *scopes, PyObject *name, long flags,
             PyObject *bound, PyObject *local, PyObject *free,
             PyObject *global)
{
    if (flags & DEF_GLOBAL) {
        if (flags & DEF_PARAM) {
            PyErr_Format(PyExc_SyntaxError,
                        "name '%U' is parameter and global",
                        name);
            return error_at_directive(ste, name);
        }
        if (flags & DEF_NONLOCAL) {
            PyErr_Format(PyExc_SyntaxError,
                         "name '%U' is nonlocal and global",
                         name);
            return error_at_directive(ste, name);
        }
        SET_SCOPE(scopes, name, GLOBAL_EXPLICIT);
        if (PySet_Add(global, name) < 0)
            return 0;
        if (bound && (PySet_Discard(bound, name) < 0))
            return 0;
        return 1;
    }
    if (flags & DEF_NONLOCAL) {
        if (flags & DEF_PARAM) {
            PyErr_Format(PyExc_SyntaxError,
                         "name '%U' is parameter and nonlocal",
                         name);
            return error_at_directive(ste, name);
        }
        if (!bound) {
            PyErr_Format(PyExc_SyntaxError,
                         "nonlocal declaration not allowed at module level");
            return error_at_directive(ste, name);
        }
        if (!PySet_Contains(bound, name)) {
            PyErr_Format(PyExc_SyntaxError,
                         "no binding for nonlocal '%U' found",
                         name);

            return error_at_directive(ste, name);
        }
        SET_SCOPE(scopes, name, FREE);
        ste->ste_free = 1;
        return PySet_Add(free, name) >= 0;
    }
    if (flags & DEF_BOUND) {
        SET_SCOPE(scopes, name, LOCAL);
        if (PySet_Add(local, name) < 0)
            return 0;
        if (PySet_Discard(global, name) < 0)
            return 0;
        return 1;
    }
    /* If an enclosing block has a binding for this name, it
       is a free variable rather than a global variable.
       Note that having a non-NULL bound implies that the block
       is nested.
    */
    if (bound && PySet_Contains(bound, name)) {
        SET_SCOPE(scopes, name, FREE);
        ste->ste_free = 1;
        return PySet_Add(free, name) >= 0;
    }
    /* If a parent has a global statement, then call it global
       explicit?  It could also be global implicit.
     */
    if (global && PySet_Contains(global, name)) {
        SET_SCOPE(scopes, name, GLOBAL_IMPLICIT);
        return 1;
    }
    if (ste->ste_nested)
        ste->ste_free = 1;
    SET_SCOPE(scopes, name, GLOBAL_IMPLICIT);
    return 1;
}

#undef SET_SCOPE

/* If a name is defined in free and also in locals, then this block
   provides the binding for the free variable.  The name should be
   marked CELL in this block and removed from the free list.

   Note that the current block's free variables are included in free.
   That's safe because no name can be free and local in the same scope.
*/

static int
analyze_cells(PyObject *scopes, PyObject *free)
{
    PyObject *name, *v, *v_cell;
    int success = 0;
    Py_ssize_t pos = 0;

    v_cell = PyLong_FromLong(CELL);
    if (!v_cell)
        return 0;
    while (PyDict_Next(scopes, &pos, &name, &v)) {
        long scope;
        assert(PyLong_Check(v));
        scope = PyLong_AS_LONG(v);
        if (scope != LOCAL)
            continue;
        if (!PySet_Contains(free, name))
            continue;
        /* Replace LOCAL with CELL for this name, and remove
           from free. It is safe to replace the value of name
           in the dict, because it will not cause a resize.
         */
        if (PyDict_SetItem(scopes, name, v_cell) < 0)
            goto error;
        if (PySet_Discard(free, name) < 0)
            goto error;
    }
    success = 1;
 error:
    Py_DECREF(v_cell);
    return success;
}

static int
drop_class_free(PySTEntryObject *ste, PyObject *free)
{
    int res;
    if (!GET_IDENTIFIER(__class__))
        return 0;
    res = PySet_Discard(free, __class__);
    if (res < 0)
        return 0;
    if (res)
        ste->ste_needs_class_closure = 1;
    return 1;
}

/* Enter the final scope information into the ste_symbols dict.
 *
 * All arguments are dicts.  Modifies symbols, others are read-only.
*/
static int
update_symbols(PyObject *symbols, PyObject *scopes,
               PyObject *bound, PyObject *free, int classflag)
{
    PyObject *name = NULL, *itr = NULL;
    PyObject *v = NULL, *v_scope = NULL, *v_new = NULL, *v_free = NULL;
    Py_ssize_t pos = 0;

    /* Update scope information for all symbols in this scope */
    while (PyDict_Next(symbols, &pos, &name, &v)) {
        long scope, flags;
        assert(PyLong_Check(v));
        flags = PyLong_AS_LONG(v);
        v_scope = PyDict_GetItem(scopes, name);
        assert(v_scope && PyLong_Check(v_scope));
        scope = PyLong_AS_LONG(v_scope);
        flags |= (scope << SCOPE_OFFSET);
        v_new = PyLong_FromLong(flags);
        if (!v_new)
            return 0;
        if (PyDict_SetItem(symbols, name, v_new) < 0) {
            Py_DECREF(v_new);
            return 0;
        }
        Py_DECREF(v_new);
    }

    /* Record not yet resolved free variables from children (if any) */
    v_free = PyLong_FromLong(FREE << SCOPE_OFFSET);
    if (!v_free)
        return 0;

    itr = PyObject_GetIter(free);
    if (!itr)
        goto error;

    while ((name = PyIter_Next(itr))) {
        v = PyDict_GetItem(symbols, name);

        /* Handle symbol that already exists in this scope */
        if (v) {
            /* Handle a free variable in a method of
               the class that has the same name as a local
               or global in the class scope.
            */
            if  (classflag &&
                 PyLong_AS_LONG(v) & (DEF_BOUND | DEF_GLOBAL)) {
                long flags = PyLong_AS_LONG(v) | DEF_FREE_CLASS;
                v_new = PyLong_FromLong(flags);
                if (!v_new) {
                    goto error;
                }
                if (PyDict_SetItem(symbols, name, v_new) < 0) {
                    Py_DECREF(v_new);
                    goto error;
                }
                Py_DECREF(v_new);
            }
            /* It's a cell, or already free in this scope */
            Py_DECREF(name);
            continue;
        }
        /* Handle global symbol */
        if (bound && !PySet_Contains(bound, name)) {
            Py_DECREF(name);
            continue;       /* it's a global */
        }
        /* Propagate new free symbol up the lexical stack */
        if (PyDict_SetItem(symbols, name, v_free) < 0) {
            goto error;
        }
        Py_DECREF(name);
    }
    Py_DECREF(itr);
    Py_DECREF(v_free);
    return 1;
error:
    Py_XDECREF(v_free);
    Py_XDECREF(itr);
    Py_XDECREF(name);
    return 0;
}

/* Make final symbol table decisions for block of ste.

   Arguments:
   ste -- current symtable entry (input/output)
   bound -- set of variables bound in enclosing scopes (input).  bound
       is NULL for module blocks.
   free -- set of free variables in enclosed scopes (output)
   globals -- set of declared global variables in enclosing scopes (input)

   The implementation uses two mutually recursive functions,
   analyze_block() and analyze_child_block().  analyze_block() is
   responsible for analyzing the individual names defined in a block.
   analyze_child_block() prepares temporary namespace dictionaries
   used to evaluated nested blocks.

   The two functions exist because a child block should see the name
   bindings of its enclosing blocks, but those bindings should not
   propagate back to a parent block.
*/

static int
analyze_child_block(PySTEntryObject *entry, PyObject *bound, PyObject *free,
                    PyObject *global, PyObject* child_free);

static int
analyze_block(PySTEntryObject *ste, PyObject *bound, PyObject *free,
              PyObject *global)
{
    PyObject *name, *v, *local = NULL, *scopes = NULL, *newbound = NULL;
    PyObject *newglobal = NULL, *newfree = NULL, *allfree = NULL;
    PyObject *temp;
    int i, success = 0;
    Py_ssize_t pos = 0;

    local = PySet_New(NULL);  /* collect new names bound in block */
    if (!local)
        goto error;
    scopes = PyDict_New();  /* collect scopes defined for each name */
    if (!scopes)
        goto error;

    /* Allocate new global and bound variable dictionaries.  These
       dictionaries hold the names visible in nested blocks.  For
       ClassBlocks, the bound and global names are initialized
       before analyzing names, because class bindings aren't
       visible in methods.  For other blocks, they are initialized
       after names are analyzed.
     */

    /* TODO(jhylton): Package these dicts in a struct so that we
       can write reasonable helper functions?
    */
    newglobal = PySet_New(NULL);
    if (!newglobal)
        goto error;
    newfree = PySet_New(NULL);
    if (!newfree)
        goto error;
    newbound = PySet_New(NULL);
    if (!newbound)
        goto error;

    /* Class namespace has no effect on names visible in
       nested functions, so populate the global and bound
       sets to be passed to child blocks before analyzing
       this one.
     */
    if (ste->ste_type == ClassBlock) {
        /* Pass down known globals */
        temp = PyNumber_InPlaceOr(newglobal, global);
        if (!temp)
            goto error;
        Py_DECREF(temp);
        /* Pass down previously bound symbols */
        if (bound) {
            temp = PyNumber_InPlaceOr(newbound, bound);
            if (!temp)
                goto error;
            Py_DECREF(temp);
        }
    }

    while (PyDict_Next(ste->ste_symbols, &pos, &name, &v)) {
        long flags = PyLong_AS_LONG(v);
        if (!analyze_name(ste, scopes, name, flags,
                          bound, local, free, global))
            goto error;
    }

    /* Populate global and bound sets to be passed to children. */
    if (ste->ste_type != ClassBlock) {
        /* Add function locals to bound set */
        if (ste->ste_type == FunctionBlock) {
            temp = PyNumber_InPlaceOr(newbound, local);
            if (!temp)
                goto error;
            Py_DECREF(temp);
        }
        /* Pass down previously bound symbols */
        if (bound) {
            temp = PyNumber_InPlaceOr(newbound, bound);
            if (!temp)
                goto error;
            Py_DECREF(temp);
        }
        /* Pass down known globals */
        temp = PyNumber_InPlaceOr(newglobal, global);
        if (!temp)
            goto error;
        Py_DECREF(temp);
    }
    else {
        /* Special-case __class__ */
        if (!GET_IDENTIFIER(__class__))
            goto error;
        if (PySet_Add(newbound, __class__) < 0)
            goto error;
    }

    /* Recursively call analyze_child_block() on each child block.

       newbound, newglobal now contain the names visible in
       nested blocks.  The free variables in the children will
       be collected in allfree.
    */
    allfree = PySet_New(NULL);
    if (!allfree)
        goto error;
    for (i = 0; i < PyList_GET_SIZE(ste->ste_children); ++i) {
        PyObject *c = PyList_GET_ITEM(ste->ste_children, i);
        PySTEntryObject* entry;
        assert(c && PySTEntry_Check(c));
        entry = (PySTEntryObject*)c;
        if (!analyze_child_block(entry, newbound, newfree, newglobal,
                                 allfree))
            goto error;
        /* Check if any children have free variables */
        if (entry->ste_free || entry->ste_child_free)
            ste->ste_child_free = 1;
    }

    temp = PyNumber_InPlaceOr(newfree, allfree);
    if (!temp)
        goto error;
    Py_DECREF(temp);

    /* Check if any local variables must be converted to cell variables */
    if (ste->ste_type == FunctionBlock && !analyze_cells(scopes, newfree))
        goto error;
    else if (ste->ste_type == ClassBlock && !drop_class_free(ste, newfree))
        goto error;
    /* Records the results of the analysis in the symbol table entry */
    if (!update_symbols(ste->ste_symbols, scopes, bound, newfree,
                        ste->ste_type == ClassBlock))
        goto error;

    temp = PyNumber_InPlaceOr(free, newfree);
    if (!temp)
        goto error;
    Py_DECREF(temp);
    success = 1;
 error:
    Py_XDECREF(scopes);
    Py_XDECREF(local);
    Py_XDECREF(newbound);
    Py_XDECREF(newglobal);
    Py_XDECREF(newfree);
    Py_XDECREF(allfree);
    if (!success)
        assert(PyErr_Occurred());
    return success;
}

static int
analyze_child_block(PySTEntryObject *entry, PyObject *bound, PyObject *free,
                    PyObject *global, PyObject* child_free)
{
    PyObject *temp_bound = NULL, *temp_global = NULL, *temp_free = NULL;
    PyObject *temp;

    /* Copy the bound and global dictionaries.

       These dictionaries are used by all blocks enclosed by the
       current block.  The analyze_block() call modifies these
       dictionaries.

    */
    temp_bound = PySet_New(bound);
    if (!temp_bound)
        goto error;
    temp_free = PySet_New(free);
    if (!temp_free)
        goto error;
    temp_global = PySet_New(global);
    if (!temp_global)
        goto error;

    if (!analyze_block(entry, temp_bound, temp_free, temp_global))
        goto error;
    temp = PyNumber_InPlaceOr(child_free, temp_free);
    if (!temp)
        goto error;
    Py_DECREF(temp);
    Py_DECREF(temp_bound);
    Py_DECREF(temp_free);
    Py_DECREF(temp_global);
    return 1;
 error:
    Py_XDECREF(temp_bound);
    Py_XDECREF(temp_free);
    Py_XDECREF(temp_global);
    return 0;
}

static int
symtable_analyze(struct symtable *st)
{
    PyObject *free, *global;
    int r;

    free = PySet_New(NULL);
    if (!free)
        return 0;
    global = PySet_New(NULL);
    if (!global) {
        Py_DECREF(free);
        return 0;
    }
    r = analyze_block(st->st_top, NULL, free, global);
    Py_DECREF(free);
    Py_DECREF(global);
    return r;
}

/* symtable_enter_block() gets a reference via ste_new.
   This reference is released when the block is exited, via the DECREF
   in symtable_exit_block().
*/

static int
symtable_exit_block(struct symtable *st, void *ast)
{
    Py_ssize_t size;

    st->st_cur = NULL;
    size = PyList_GET_SIZE(st->st_stack);
    if (size) {
        if (PyList_SetSlice(st->st_stack, size - 1, size, NULL) < 0)
            return 0;
        if (--size)
            st->st_cur = (PySTEntryObject *)PyList_GET_ITEM(st->st_stack, size - 1);
    }
    return 1;
}

static int
symtable_enter_block(struct symtable *st, identifier name, _Py_block_ty block,
                     void *ast, int lineno, int col_offset)
{
    PySTEntryObject *prev = NULL, *ste;

    ste = ste_new(st, name, block, ast, lineno, col_offset);
    if (ste == NULL)
        return 0;
    if (PyList_Append(st->st_stack, (PyObject *)ste) < 0) {
        Py_DECREF(ste);
        return 0;
    }
    prev = st->st_cur;
    /* The entry is owned by the stack. Borrow it for st_cur. */
    Py_DECREF(ste);
    st->st_cur = ste;
    if (block == ModuleBlock)
        st->st_global = st->st_cur->ste_symbols;
    if (prev) {
        if (PyList_Append(prev->ste_children, (PyObject *)ste) < 0) {
            return 0;
        }
    }
    return 1;
}

static long
symtable_lookup(struct symtable *st, PyObject *name)
{
    PyObject *o;
    PyObject *mangled = _Py_Mangle(st->st_private, name);
    if (!mangled)
        return 0;
    o = PyDict_GetItem(st->st_cur->ste_symbols, mangled);
    Py_DECREF(mangled);
    if (!o)
        return 0;
    return PyLong_AsLong(o);
}

static int
symtable_add_def(struct symtable *st, PyObject *name, int flag)
{
    PyObject *o;
    PyObject *dict;
    long val;
    PyObject *mangled = _Py_Mangle(st->st_private, name);


    if (!mangled)
        return 0;
    dict = st->st_cur->ste_symbols;
    if ((o = PyDict_GetItem(dict, mangled))) {
        val = PyLong_AS_LONG(o);
        if ((flag & DEF_PARAM) && (val & DEF_PARAM)) {
            /* Is it better to use 'mangled' or 'name' here? */
            PyErr_Format(PyExc_SyntaxError, DUPLICATE_ARGUMENT, name);
            PyErr_SyntaxLocationObject(st->st_filename,
                                       st->st_cur->ste_lineno,
                                       st->st_cur->ste_col_offset);
            goto error;
        }
        val |= flag;
    } else
        val = flag;
    o = PyLong_FromLong(val);
    if (o == NULL)
        goto error;
    if (PyDict_SetItem(dict, mangled, o) < 0) {
        Py_DECREF(o);
        goto error;
    }
    Py_DECREF(o);

    if (flag & DEF_PARAM) {
        if (PyList_Append(st->st_cur->ste_varnames, mangled) < 0)
            goto error;
    } else      if (flag & DEF_GLOBAL) {
        /* XXX need to update DEF_GLOBAL for other flags too;
           perhaps only DEF_FREE_GLOBAL */
        val = flag;
        if ((o = PyDict_GetItem(st->st_global, mangled))) {
            val |= PyLong_AS_LONG(o);
        }
        o = PyLong_FromLong(val);
        if (o == NULL)
            goto error;
        if (PyDict_SetItem(st->st_global, mangled, o) < 0) {
            Py_DECREF(o);
            goto error;
        }
        Py_DECREF(o);
    }
    Py_DECREF(mangled);
    return 1;

error:
    Py_DECREF(mangled);
    return 0;
}

/* VISIT, VISIT_SEQ and VIST_SEQ_TAIL take an ASDL type as their second argument.
   They use the ASDL name to synthesize the name of the C type and the visit
   function.

   VISIT_SEQ_TAIL permits the start of an ASDL sequence to be skipped, which is
   useful if the first node in the sequence requires special treatment.

   VISIT_QUIT macro returns the specified value exiting from the function but
   first adjusts current recursion counter depth.
*/

#define VISIT_QUIT(ST, X) \
    return --(ST)->recursion_depth,(X)

#define VISIT(ST, TYPE, V) \
    if (!symtable_visit_ ## TYPE((ST), (V))) \
        VISIT_QUIT((ST), 0);

#define VISIT_SEQ(ST, TYPE, SEQ) { \
    int i; \
    asdl_seq *seq = (SEQ); /* avoid variable capture */ \
    for (i = 0; i < asdl_seq_LEN(seq); i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, i); \
        if (!symtable_visit_ ## TYPE((ST), elt)) \
            VISIT_QUIT((ST), 0);                 \
    } \
}

#define VISIT_SEQ_TAIL(ST, TYPE, SEQ, START) { \
    int i; \
    asdl_seq *seq = (SEQ); /* avoid variable capture */ \
    for (i = (START); i < asdl_seq_LEN(seq); i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, i); \
        if (!symtable_visit_ ## TYPE((ST), elt)) \
            VISIT_QUIT((ST), 0);                 \
    } \
}

#define VISIT_SEQ_WITH_NULL(ST, TYPE, SEQ) {     \
    int i = 0; \
    asdl_seq *seq = (SEQ); /* avoid variable capture */ \
    for (i = 0; i < asdl_seq_LEN(seq); i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, i); \
        if (!elt) continue; /* can be NULL */ \
        if (!symtable_visit_ ## TYPE((ST), elt)) \
            VISIT_QUIT((ST), 0);             \
    } \
}

static int
symtable_new_tmpname(struct symtable *st)
{
    char tmpname[256];
    identifier tmp;

    PyOS_snprintf(tmpname, sizeof(tmpname), "_[%d]",
                  ++st->st_cur->ste_tmpname);
    tmp = PyUnicode_InternFromString(tmpname);
    if (!tmp)
        return 0;
    if (!symtable_add_def(st, tmp, DEF_LOCAL))
        return 0;
    Py_DECREF(tmp);
    return 1;
}


static int
symtable_record_directive(struct symtable *st, identifier name, stmt_ty s)
{
    PyObject *data, *mangled;
    int res;
    if (!st->st_cur->ste_directives) {
        st->st_cur->ste_directives = PyList_New(0);
        if (!st->st_cur->ste_directives)
            return 0;
    }
    mangled = _Py_Mangle(st->st_private, name);
    if (!mangled)
        return 0;
    data = Py_BuildValue("(Nii)", mangled, s->lineno, s->col_offset);
    if (!data)
        return 0;
    res = PyList_Append(st->st_cur->ste_directives, data);
    Py_DECREF(data);
    return res == 0;
}


static int
symtable_visit_stmt(struct symtable *st, stmt_ty s)
{
    if (++st->recursion_depth > st->recursion_limit) {
        PyErr_SetString(PyExc_RecursionError,
                        "maximum recursion depth exceeded during compilation");
        VISIT_QUIT(st, 0);
    }
    switch (s->kind) {
    case FunctionDef_kind:
        if (!symtable_add_def(st, s->v.FunctionDef.name, DEF_LOCAL))
            VISIT_QUIT(st, 0);
        if (s->v.FunctionDef.args->defaults)
            VISIT_SEQ(st, expr, s->v.FunctionDef.args->defaults);
        if (s->v.FunctionDef.args->kw_defaults)
            VISIT_SEQ_WITH_NULL(st, expr, s->v.FunctionDef.args->kw_defaults);
        if (!symtable_visit_annotations(st, s, s->v.FunctionDef.args,
                                        s->v.FunctionDef.returns))
            VISIT_QUIT(st, 0);
        if (s->v.FunctionDef.decorator_list)
            VISIT_SEQ(st, expr, s->v.FunctionDef.decorator_list);
        if (!symtable_enter_block(st, s->v.FunctionDef.name,
                                  FunctionBlock, (void *)s, s->lineno,
                                  s->col_offset))
            VISIT_QUIT(st, 0);
        VISIT(st, arguments, s->v.FunctionDef.args);
        VISIT_SEQ(st, stmt, s->v.FunctionDef.body);
        if (!symtable_exit_block(st, s))
            VISIT_QUIT(st, 0);
        break;
    case ClassDef_kind: {
        PyObject *tmp;
        if (!symtable_add_def(st, s->v.ClassDef.name, DEF_LOCAL))
            VISIT_QUIT(st, 0);
        VISIT_SEQ(st, expr, s->v.ClassDef.bases);
        VISIT_SEQ(st, keyword, s->v.ClassDef.keywords);
        if (s->v.ClassDef.decorator_list)
            VISIT_SEQ(st, expr, s->v.ClassDef.decorator_list);
        if (!symtable_enter_block(st, s->v.ClassDef.name, ClassBlock,
                                  (void *)s, s->lineno, s->col_offset))
            VISIT_QUIT(st, 0);
        tmp = st->st_private;
        st->st_private = s->v.ClassDef.name;
        VISIT_SEQ(st, stmt, s->v.ClassDef.body);
        st->st_private = tmp;
        if (!symtable_exit_block(st, s))
            VISIT_QUIT(st, 0);
        break;
    }
    case Return_kind:
        if (s->v.Return.value) {
            VISIT(st, expr, s->v.Return.value);
            st->st_cur->ste_returns_value = 1;
        }
        break;
    case Delete_kind:
        VISIT_SEQ(st, expr, s->v.Delete.targets);
        break;
    case Assign_kind:
        VISIT_SEQ(st, expr, s->v.Assign.targets);
        VISIT(st, expr, s->v.Assign.value);
        break;
    case AnnAssign_kind:
        if (s->v.AnnAssign.target->kind == Name_kind) {
            expr_ty e_name = s->v.AnnAssign.target;
            long cur = symtable_lookup(st, e_name->v.Name.id);
            if (cur < 0) {
                VISIT_QUIT(st, 0);
            }
            if ((cur & (DEF_GLOBAL | DEF_NONLOCAL))
                && s->v.AnnAssign.simple) {
                PyErr_Format(PyExc_SyntaxError,
                             cur & DEF_GLOBAL ? GLOBAL_ANNOT : NONLOCAL_ANNOT,
                             e_name->v.Name.id);
                PyErr_SyntaxLocationObject(st->st_filename,
                                           s->lineno,
                                           s->col_offset);
                VISIT_QUIT(st, 0);
            }
            if (s->v.AnnAssign.simple &&
                !symtable_add_def(st, e_name->v.Name.id,
                                  DEF_ANNOT | DEF_LOCAL)) {
                VISIT_QUIT(st, 0);
            }
            else {
                if (s->v.AnnAssign.value
                    && !symtable_add_def(st, e_name->v.Name.id, DEF_LOCAL)) {
                    VISIT_QUIT(st, 0);
                }
            }
        }
        else {
            VISIT(st, expr, s->v.AnnAssign.target);
        }
        VISIT(st, expr, s->v.AnnAssign.annotation);
        if (s->v.AnnAssign.value) {
            VISIT(st, expr, s->v.AnnAssign.value);
        }
        break;
    case AugAssign_kind:
        VISIT(st, expr, s->v.AugAssign.target);
        VISIT(st, expr, s->v.AugAssign.value);
        break;
    case For_kind:
        VISIT(st, expr, s->v.For.target);
        VISIT(st, expr, s->v.For.iter);
        VISIT_SEQ(st, stmt, s->v.For.body);
        if (s->v.For.orelse)
            VISIT_SEQ(st, stmt, s->v.For.orelse);
        break;
    case While_kind:
        VISIT(st, expr, s->v.While.test);
        VISIT_SEQ(st, stmt, s->v.While.body);
        if (s->v.While.orelse)
            VISIT_SEQ(st, stmt, s->v.While.orelse);
        break;
    case If_kind:
        /* XXX if 0: and lookup_yield() hacks */
        VISIT(st, expr, s->v.If.test);
        VISIT_SEQ(st, stmt, s->v.If.body);
        if (s->v.If.orelse)
            VISIT_SEQ(st, stmt, s->v.If.orelse);
        break;
    case Raise_kind:
        if (s->v.Raise.exc) {
            VISIT(st, expr, s->v.Raise.exc);
            if (s->v.Raise.cause) {
                VISIT(st, expr, s->v.Raise.cause);
            }
        }
        break;
    case Try_kind:
        VISIT_SEQ(st, stmt, s->v.Try.body);
        VISIT_SEQ(st, stmt, s->v.Try.orelse);
        VISIT_SEQ(st, excepthandler, s->v.Try.handlers);
        VISIT_SEQ(st, stmt, s->v.Try.finalbody);
        break;
    case Assert_kind:
        VISIT(st, expr, s->v.Assert.test);
        if (s->v.Assert.msg)
            VISIT(st, expr, s->v.Assert.msg);
        break;
    case Import_kind:
        VISIT_SEQ(st, alias, s->v.Import.names);
        break;
    case ImportFrom_kind:
        VISIT_SEQ(st, alias, s->v.ImportFrom.names);
        break;
    case Global_kind: {
        int i;
        asdl_seq *seq = s->v.Global.names;
        for (i = 0; i < asdl_seq_LEN(seq); i++) {
            identifier name = (identifier)asdl_seq_GET(seq, i);
            long cur = symtable_lookup(st, name);
            if (cur < 0)
                VISIT_QUIT(st, 0);
            if (cur & (DEF_LOCAL | USE | DEF_ANNOT)) {
                char* msg;
                if (cur & USE) {
                    msg = GLOBAL_AFTER_USE;
                } else if (cur & DEF_ANNOT) {
                    msg = GLOBAL_ANNOT;
                } else {  /* DEF_LOCAL */
                    msg = GLOBAL_AFTER_ASSIGN;
                }
                PyErr_Format(PyExc_SyntaxError,
                             msg, name);
                PyErr_SyntaxLocationObject(st->st_filename,
                                           s->lineno,
                                           s->col_offset);
                VISIT_QUIT(st, 0);
            }
            if (!symtable_add_def(st, name, DEF_GLOBAL))
                VISIT_QUIT(st, 0);
            if (!symtable_record_directive(st, name, s))
                VISIT_QUIT(st, 0);
        }
        break;
    }
    case Nonlocal_kind: {
        int i;
        asdl_seq *seq = s->v.Nonlocal.names;
        for (i = 0; i < asdl_seq_LEN(seq); i++) {
            identifier name = (identifier)asdl_seq_GET(seq, i);
            long cur = symtable_lookup(st, name);
            if (cur < 0)
                VISIT_QUIT(st, 0);
            if (cur & (DEF_LOCAL | USE | DEF_ANNOT)) {
                char* msg;
                if (cur & USE) {
                    msg = NONLOCAL_AFTER_USE;
                } else if (cur & DEF_ANNOT) {
                    msg = NONLOCAL_ANNOT;
                } else {  /* DEF_LOCAL */
                    msg = NONLOCAL_AFTER_ASSIGN;
                }
                PyErr_Format(PyExc_SyntaxError, msg, name);
                PyErr_SyntaxLocationObject(st->st_filename,
                                           s->lineno,
                                           s->col_offset);
                VISIT_QUIT(st, 0);
            }
            if (!symtable_add_def(st, name, DEF_NONLOCAL))
                VISIT_QUIT(st, 0);
            if (!symtable_record_directive(st, name, s))
                VISIT_QUIT(st, 0);
        }
        break;
    }
    case Expr_kind:
        VISIT(st, expr, s->v.Expr.value);
        break;
    case Pass_kind:
    case Break_kind:
    case Continue_kind:
        /* nothing to do here */
        break;
    case With_kind:
        VISIT_SEQ(st, withitem, s->v.With.items);
        VISIT_SEQ(st, stmt, s->v.With.body);
        break;
    case AsyncFunctionDef_kind:
        if (!symtable_add_def(st, s->v.AsyncFunctionDef.name, DEF_LOCAL))
            VISIT_QUIT(st, 0);
        if (s->v.AsyncFunctionDef.args->defaults)
            VISIT_SEQ(st, expr, s->v.AsyncFunctionDef.args->defaults);
        if (s->v.AsyncFunctionDef.args->kw_defaults)
            VISIT_SEQ_WITH_NULL(st, expr,
                                s->v.AsyncFunctionDef.args->kw_defaults);
        if (!symtable_visit_annotations(st, s, s->v.AsyncFunctionDef.args,
                                        s->v.AsyncFunctionDef.returns))
            VISIT_QUIT(st, 0);
        if (s->v.AsyncFunctionDef.decorator_list)
            VISIT_SEQ(st, expr, s->v.AsyncFunctionDef.decorator_list);
        if (!symtable_enter_block(st, s->v.AsyncFunctionDef.name,
                                  FunctionBlock, (void *)s, s->lineno,
                                  s->col_offset))
            VISIT_QUIT(st, 0);
        st->st_cur->ste_coroutine = 1;
        VISIT(st, arguments, s->v.AsyncFunctionDef.args);
        VISIT_SEQ(st, stmt, s->v.AsyncFunctionDef.body);
        if (!symtable_exit_block(st, s))
            VISIT_QUIT(st, 0);
        break;
    case AsyncWith_kind:
        VISIT_SEQ(st, withitem, s->v.AsyncWith.items);
        VISIT_SEQ(st, stmt, s->v.AsyncWith.body);
        break;
    case AsyncFor_kind:
        VISIT(st, expr, s->v.AsyncFor.target);
        VISIT(st, expr, s->v.AsyncFor.iter);
        VISIT_SEQ(st, stmt, s->v.AsyncFor.body);
        if (s->v.AsyncFor.orelse)
            VISIT_SEQ(st, stmt, s->v.AsyncFor.orelse);
        break;
    }
    VISIT_QUIT(st, 1);
}

static int
symtable_visit_expr(struct symtable *st, expr_ty e)
{
    if (++st->recursion_depth > st->recursion_limit) {
        PyErr_SetString(PyExc_RecursionError,
                        "maximum recursion depth exceeded during compilation");
        VISIT_QUIT(st, 0);
    }
    switch (e->kind) {
    case BoolOp_kind:
        VISIT_SEQ(st, expr, e->v.BoolOp.values);
        break;
    case BinOp_kind:
        VISIT(st, expr, e->v.BinOp.left);
        VISIT(st, expr, e->v.BinOp.right);
        break;
    case UnaryOp_kind:
        VISIT(st, expr, e->v.UnaryOp.operand);
        break;
    case Lambda_kind: {
        if (!GET_IDENTIFIER(lambda))
            VISIT_QUIT(st, 0);
        if (e->v.Lambda.args->defaults)
            VISIT_SEQ(st, expr, e->v.Lambda.args->defaults);
        if (e->v.Lambda.args->kw_defaults)
            VISIT_SEQ_WITH_NULL(st, expr, e->v.Lambda.args->kw_defaults);
        if (!symtable_enter_block(st, lambda,
                                  FunctionBlock, (void *)e, e->lineno,
                                  e->col_offset))
            VISIT_QUIT(st, 0);
        VISIT(st, arguments, e->v.Lambda.args);
        VISIT(st, expr, e->v.Lambda.body);
        if (!symtable_exit_block(st, (void *)e))
            VISIT_QUIT(st, 0);
        break;
    }
    case IfExp_kind:
        VISIT(st, expr, e->v.IfExp.test);
        VISIT(st, expr, e->v.IfExp.body);
        VISIT(st, expr, e->v.IfExp.orelse);
        break;
    case Dict_kind:
        VISIT_SEQ_WITH_NULL(st, expr, e->v.Dict.keys);
        VISIT_SEQ(st, expr, e->v.Dict.values);
        break;
    case Set_kind:
        VISIT_SEQ(st, expr, e->v.Set.elts);
        break;
    case GeneratorExp_kind:
        if (!symtable_visit_genexp(st, e))
            VISIT_QUIT(st, 0);
        break;
    case ListComp_kind:
        if (!symtable_visit_listcomp(st, e))
            VISIT_QUIT(st, 0);
        break;
    case SetComp_kind:
        if (!symtable_visit_setcomp(st, e))
            VISIT_QUIT(st, 0);
        break;
    case DictComp_kind:
        if (!symtable_visit_dictcomp(st, e))
            VISIT_QUIT(st, 0);
        break;
    case Yield_kind:
        if (e->v.Yield.value)
            VISIT(st, expr, e->v.Yield.value);
        st->st_cur->ste_generator = 1;
        break;
    case YieldFrom_kind:
        VISIT(st, expr, e->v.YieldFrom.value);
        st->st_cur->ste_generator = 1;
        break;
    case Await_kind:
        VISIT(st, expr, e->v.Await.value);
        st->st_cur->ste_coroutine = 1;
        break;
    case Compare_kind:
        VISIT(st, expr, e->v.Compare.left);
        VISIT_SEQ(st, expr, e->v.Compare.comparators);
        break;
    case Call_kind:
        VISIT(st, expr, e->v.Call.func);
        VISIT_SEQ(st, expr, e->v.Call.args);
        VISIT_SEQ_WITH_NULL(st, keyword, e->v.Call.keywords);
        break;
    case FormattedValue_kind:
        VISIT(st, expr, e->v.FormattedValue.value);
        if (e->v.FormattedValue.format_spec)
            VISIT(st, expr, e->v.FormattedValue.format_spec);
        break;
    case JoinedStr_kind:
        VISIT_SEQ(st, expr, e->v.JoinedStr.values);
        break;
    case Constant_kind:
    case Num_kind:
    case Str_kind:
    case Bytes_kind:
    case Ellipsis_kind:
    case NameConstant_kind:
        /* Nothing to do here. */
        break;
    /* The following exprs can be assignment targets. */
    case Attribute_kind:
        VISIT(st, expr, e->v.Attribute.value);
        break;
    case Subscript_kind:
        VISIT(st, expr, e->v.Subscript.value);
        VISIT(st, slice, e->v.Subscript.slice);
        break;
    case Starred_kind:
        VISIT(st, expr, e->v.Starred.value);
        break;
    case Name_kind:
        if (!symtable_add_def(st, e->v.Name.id,
                              e->v.Name.ctx == Load ? USE : DEF_LOCAL))
            VISIT_QUIT(st, 0);
        /* Special-case super: it counts as a use of __class__ */
        if (e->v.Name.ctx == Load &&
            st->st_cur->ste_type == FunctionBlock &&
            _PyUnicode_EqualToASCIIString(e->v.Name.id, "super")) {
            if (!GET_IDENTIFIER(__class__) ||
                !symtable_add_def(st, __class__, USE))
                VISIT_QUIT(st, 0);
        }
        break;
    /* child nodes of List and Tuple will have expr_context set */
    case List_kind:
        VISIT_SEQ(st, expr, e->v.List.elts);
        break;
    case Tuple_kind:
        VISIT_SEQ(st, expr, e->v.Tuple.elts);
        break;
    }
    VISIT_QUIT(st, 1);
}

static int
symtable_implicit_arg(struct symtable *st, int pos)
{
    PyObject *id = PyUnicode_FromFormat(".%d", pos);
    if (id == NULL)
        return 0;
    if (!symtable_add_def(st, id, DEF_PARAM)) {
        Py_DECREF(id);
        return 0;
    }
    Py_DECREF(id);
    return 1;
}

static int
symtable_visit_params(struct symtable *st, asdl_seq *args)
{
    int i;

    if (!args)
        return -1;

    for (i = 0; i < asdl_seq_LEN(args); i++) {
        arg_ty arg = (arg_ty)asdl_seq_GET(args, i);
        if (!symtable_add_def(st, arg->arg, DEF_PARAM))
            return 0;
    }

    return 1;
}

static int
symtable_visit_argannotations(struct symtable *st, asdl_seq *args)
{
    int i;

    if (!args)
        return -1;

    for (i = 0; i < asdl_seq_LEN(args); i++) {
        arg_ty arg = (arg_ty)asdl_seq_GET(args, i);
        if (arg->annotation)
            VISIT(st, expr, arg->annotation);
    }

    return 1;
}

static int
symtable_visit_annotations(struct symtable *st, stmt_ty s,
                           arguments_ty a, expr_ty returns)
{
    if (a->args && !symtable_visit_argannotations(st, a->args))
        return 0;
    if (a->vararg && a->vararg->annotation)
        VISIT(st, expr, a->vararg->annotation);
    if (a->kwarg && a->kwarg->annotation)
        VISIT(st, expr, a->kwarg->annotation);
    if (a->kwonlyargs && !symtable_visit_argannotations(st, a->kwonlyargs))
        return 0;
    if (returns)
        VISIT(st, expr, returns);
    return 1;
}

static int
symtable_visit_arguments(struct symtable *st, arguments_ty a)
{
    /* skip default arguments inside function block
       XXX should ast be different?
    */
    if (a->args && !symtable_visit_params(st, a->args))
        return 0;
    if (a->kwonlyargs && !symtable_visit_params(st, a->kwonlyargs))
        return 0;
    if (a->vararg) {
        if (!symtable_add_def(st, a->vararg->arg, DEF_PARAM))
            return 0;
        st->st_cur->ste_varargs = 1;
    }
    if (a->kwarg) {
        if (!symtable_add_def(st, a->kwarg->arg, DEF_PARAM))
            return 0;
        st->st_cur->ste_varkeywords = 1;
    }
    return 1;
}


static int
symtable_visit_excepthandler(struct symtable *st, excepthandler_ty eh)
{
    if (eh->v.ExceptHandler.type)
        VISIT(st, expr, eh->v.ExceptHandler.type);
    if (eh->v.ExceptHandler.name)
        if (!symtable_add_def(st, eh->v.ExceptHandler.name, DEF_LOCAL))
            return 0;
    VISIT_SEQ(st, stmt, eh->v.ExceptHandler.body);
    return 1;
}

static int
symtable_visit_withitem(struct symtable *st, withitem_ty item)
{
    VISIT(st, expr, item->context_expr);
    if (item->optional_vars) {
        VISIT(st, expr, item->optional_vars);
    }
    return 1;
}


static int
symtable_visit_alias(struct symtable *st, alias_ty a)
{
    /* Compute store_name, the name actually bound by the import
       operation.  It is different than a->name when a->name is a
       dotted package name (e.g. spam.eggs)
    */
    PyObject *store_name;
    PyObject *name = (a->asname == NULL) ? a->name : a->asname;
    Py_ssize_t dot = PyUnicode_FindChar(name, '.', 0,
                                        PyUnicode_GET_LENGTH(name), 1);
    if (dot != -1) {
        store_name = PyUnicode_Substring(name, 0, dot);
        if (!store_name)
            return 0;
    }
    else {
        store_name = name;
        Py_INCREF(store_name);
    }
    if (!_PyUnicode_EqualToASCIIString(name, "*")) {
        int r = symtable_add_def(st, store_name, DEF_IMPORT);
        Py_DECREF(store_name);
        return r;
    }
    else {
        if (st->st_cur->ste_type != ModuleBlock) {
            int lineno = st->st_cur->ste_lineno;
            int col_offset = st->st_cur->ste_col_offset;
            PyErr_SetString(PyExc_SyntaxError, IMPORT_STAR_WARNING);
            PyErr_SyntaxLocationObject(st->st_filename, lineno, col_offset);
            Py_DECREF(store_name);
            return 0;
        }
        Py_DECREF(store_name);
        return 1;
    }
}


static int
symtable_visit_comprehension(struct symtable *st, comprehension_ty lc)
{
    VISIT(st, expr, lc->target);
    VISIT(st, expr, lc->iter);
    VISIT_SEQ(st, expr, lc->ifs);
    if (lc->is_async) {
        st->st_cur->ste_coroutine = 1;
    }
    return 1;
}


static int
symtable_visit_keyword(struct symtable *st, keyword_ty k)
{
    VISIT(st, expr, k->value);
    return 1;
}


static int
symtable_visit_slice(struct symtable *st, slice_ty s)
{
    switch (s->kind) {
    case Slice_kind:
        if (s->v.Slice.lower)
            VISIT(st, expr, s->v.Slice.lower)
        if (s->v.Slice.upper)
            VISIT(st, expr, s->v.Slice.upper)
        if (s->v.Slice.step)
            VISIT(st, expr, s->v.Slice.step)
        break;
    case ExtSlice_kind:
        VISIT_SEQ(st, slice, s->v.ExtSlice.dims)
        break;
    case Index_kind:
        VISIT(st, expr, s->v.Index.value)
        break;
    }
    return 1;
}

static int
symtable_handle_comprehension(struct symtable *st, expr_ty e,
                              identifier scope_name, asdl_seq *generators,
                              expr_ty elt, expr_ty value)
{
    int is_generator = (e->kind == GeneratorExp_kind);
    int needs_tmp = !is_generator;
    comprehension_ty outermost = ((comprehension_ty)
                                    asdl_seq_GET(generators, 0));
    /* Outermost iterator is evaluated in current scope */
    VISIT(st, expr, outermost->iter);
    /* Create comprehension scope for the rest */
    if (!scope_name ||
        !symtable_enter_block(st, scope_name, FunctionBlock, (void *)e,
                              e->lineno, e->col_offset)) {
        return 0;
    }
    st->st_cur->ste_generator = is_generator;
    if (outermost->is_async) {
        st->st_cur->ste_coroutine = 1;
    }
    /* Outermost iter is received as an argument */
    if (!symtable_implicit_arg(st, 0)) {
        symtable_exit_block(st, (void *)e);
        return 0;
    }
    /* Allocate temporary name if needed */
    if (needs_tmp && !symtable_new_tmpname(st)) {
        symtable_exit_block(st, (void *)e);
        return 0;
    }
    VISIT(st, expr, outermost->target);
    VISIT_SEQ(st, expr, outermost->ifs);
    VISIT_SEQ_TAIL(st, comprehension, generators, 1);
    if (value)
        VISIT(st, expr, value);
    VISIT(st, expr, elt);
    return symtable_exit_block(st, (void *)e);
}

static int
symtable_visit_genexp(struct symtable *st, expr_ty e)
{
    return symtable_handle_comprehension(st, e, GET_IDENTIFIER(genexpr),
                                         e->v.GeneratorExp.generators,
                                         e->v.GeneratorExp.elt, NULL);
}

static int
symtable_visit_listcomp(struct symtable *st, expr_ty e)
{
    return symtable_handle_comprehension(st, e, GET_IDENTIFIER(listcomp),
                                         e->v.ListComp.generators,
                                         e->v.ListComp.elt, NULL);
}

static int
symtable_visit_setcomp(struct symtable *st, expr_ty e)
{
    return symtable_handle_comprehension(st, e, GET_IDENTIFIER(setcomp),
                                         e->v.SetComp.generators,
                                         e->v.SetComp.elt, NULL);
}

static int
symtable_visit_dictcomp(struct symtable *st, expr_ty e)
{
    return symtable_handle_comprehension(st, e, GET_IDENTIFIER(dictcomp),
                                         e->v.DictComp.generators,
                                         e->v.DictComp.key,
                                         e->v.DictComp.value);
}
