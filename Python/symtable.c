#include "Python.h"
#include "pycore_ast.h"           // stmt_ty
#include "pycore_parser.h"        // _PyParser_ASTFromString()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_symtable.h"      // PySTEntryObject

// Set this to 1 to dump all symtables to stdout for debugging
#define _PY_DUMP_SYMTABLE 0

/* error strings used for warnings */
#define GLOBAL_PARAM \
"name '%U' is parameter and global"

#define NONLOCAL_PARAM \
"name '%U' is parameter and nonlocal"

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

#define NAMED_EXPR_COMP_IN_CLASS \
"assignment expression within a comprehension cannot be used in a class body"

#define NAMED_EXPR_COMP_IN_TYPEVAR_BOUND \
"assignment expression within a comprehension cannot be used in a TypeVar bound"

#define NAMED_EXPR_COMP_IN_TYPEALIAS \
"assignment expression within a comprehension cannot be used in a type alias"

#define NAMED_EXPR_COMP_IN_TYPEPARAM \
"assignment expression within a comprehension cannot be used within the definition of a generic"

#define NAMED_EXPR_COMP_CONFLICT \
"assignment expression cannot rebind comprehension iteration variable '%U'"

#define NAMED_EXPR_COMP_INNER_LOOP_CONFLICT \
"comprehension inner loop cannot rebind assignment expression target '%U'"

#define NAMED_EXPR_COMP_ITER_EXPR \
"assignment expression cannot be used in a comprehension iterable expression"

#define ANNOTATION_NOT_ALLOWED \
"%s cannot be used within an annotation"

#define TYPEVAR_BOUND_NOT_ALLOWED \
"%s cannot be used within a TypeVar bound"

#define TYPEALIAS_NOT_ALLOWED \
"%s cannot be used within a type alias"

#define TYPEPARAM_NOT_ALLOWED \
"%s cannot be used within the definition of a generic"

#define DUPLICATE_TYPE_PARAM \
"duplicate type parameter '%U'"


#define LOCATION(x) \
 (x)->lineno, (x)->col_offset, (x)->end_lineno, (x)->end_col_offset

#define ST_LOCATION(x) \
 (x)->ste_lineno, (x)->ste_col_offset, (x)->ste_end_lineno, (x)->ste_end_col_offset

static PySTEntryObject *
ste_new(struct symtable *st, identifier name, _Py_block_ty block,
        void *key, int lineno, int col_offset,
        int end_lineno, int end_col_offset)
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

    ste->ste_name = Py_NewRef(name);

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
    ste->ste_lineno = lineno;
    ste->ste_col_offset = col_offset;
    ste->ste_end_lineno = end_lineno;
    ste->ste_end_col_offset = end_col_offset;

    if (st->st_cur != NULL &&
        (st->st_cur->ste_nested ||
         _PyST_IsFunctionLike(st->st_cur)))
        ste->ste_nested = 1;
    ste->ste_child_free = 0;
    ste->ste_generator = 0;
    ste->ste_coroutine = 0;
    ste->ste_comprehension = NoComprehension;
    ste->ste_returns_value = 0;
    ste->ste_needs_class_closure = 0;
    ste->ste_comp_inlined = 0;
    ste->ste_comp_iter_target = 0;
    ste->ste_can_see_class_scope = 0;
    ste->ste_comp_iter_expr = 0;
    ste->ste_needs_classdict = 0;

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
    return PyUnicode_FromFormat("<symtable entry %U(%R), line %d>",
                                ste->ste_name, ste->ste_id, ste->ste_lineno);
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
    PyObject_Free(ste);
}

#define OFF(x) offsetof(PySTEntryObject, x)

static PyMemberDef ste_memberlist[] = {
    {"id",       _Py_T_OBJECT, OFF(ste_id), Py_READONLY},
    {"name",     _Py_T_OBJECT, OFF(ste_name), Py_READONLY},
    {"symbols",  _Py_T_OBJECT, OFF(ste_symbols), Py_READONLY},
    {"varnames", _Py_T_OBJECT, OFF(ste_varnames), Py_READONLY},
    {"children", _Py_T_OBJECT, OFF(ste_children), Py_READONLY},
    {"nested",   Py_T_INT,    OFF(ste_nested), Py_READONLY},
    {"type",     Py_T_INT,    OFF(ste_type), Py_READONLY},
    {"lineno",   Py_T_INT,    OFF(ste_lineno), Py_READONLY},
    {NULL}
};

PyTypeObject PySTEntry_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "symtable entry",
    sizeof(PySTEntryObject),
    0,
    (destructor)ste_dealloc,                /* tp_dealloc */
    0,                                      /* tp_vectorcall_offset */
    0,                                         /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
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
                                _Py_block_ty block, void *ast,
                                int lineno, int col_offset,
                                int end_lineno, int end_col_offset);
static int symtable_exit_block(struct symtable *st);
static int symtable_visit_stmt(struct symtable *st, stmt_ty s);
static int symtable_visit_expr(struct symtable *st, expr_ty s);
static int symtable_visit_type_param(struct symtable *st, type_param_ty s);
static int symtable_visit_genexp(struct symtable *st, expr_ty s);
static int symtable_visit_listcomp(struct symtable *st, expr_ty s);
static int symtable_visit_setcomp(struct symtable *st, expr_ty s);
static int symtable_visit_dictcomp(struct symtable *st, expr_ty s);
static int symtable_visit_arguments(struct symtable *st, arguments_ty);
static int symtable_visit_excepthandler(struct symtable *st, excepthandler_ty);
static int symtable_visit_alias(struct symtable *st, alias_ty);
static int symtable_visit_comprehension(struct symtable *st, comprehension_ty);
static int symtable_visit_keyword(struct symtable *st, keyword_ty);
static int symtable_visit_params(struct symtable *st, asdl_arg_seq *args);
static int symtable_visit_annotation(struct symtable *st, expr_ty annotation);
static int symtable_visit_argannotations(struct symtable *st, asdl_arg_seq *args);
static int symtable_implicit_arg(struct symtable *st, int pos);
static int symtable_visit_annotations(struct symtable *st, stmt_ty, arguments_ty, expr_ty);
static int symtable_visit_withitem(struct symtable *st, withitem_ty item);
static int symtable_visit_match_case(struct symtable *st, match_case_ty m);
static int symtable_visit_pattern(struct symtable *st, pattern_ty s);
static int symtable_raise_if_annotation_block(struct symtable *st, const char *, expr_ty);
static int symtable_raise_if_comprehension_block(struct symtable *st, expr_ty);

/* For debugging purposes only */
#if _PY_DUMP_SYMTABLE
static void _dump_symtable(PySTEntryObject* ste, PyObject* prefix)
{
    const char *blocktype = "";
    switch (ste->ste_type) {
        case FunctionBlock: blocktype = "FunctionBlock"; break;
        case ClassBlock: blocktype = "ClassBlock"; break;
        case ModuleBlock: blocktype = "ModuleBlock"; break;
        case AnnotationBlock: blocktype = "AnnotationBlock"; break;
        case TypeVarBoundBlock: blocktype = "TypeVarBoundBlock"; break;
        case TypeAliasBlock: blocktype = "TypeAliasBlock"; break;
        case TypeParamBlock: blocktype = "TypeParamBlock"; break;
    }
    const char *comptype = "";
    switch (ste->ste_comprehension) {
        case ListComprehension: comptype = " ListComprehension"; break;
        case DictComprehension: comptype = " DictComprehension"; break;
        case SetComprehension: comptype = " SetComprehension"; break;
        case GeneratorExpression: comptype = " GeneratorExpression"; break;
        case NoComprehension: break;
    }
    PyObject* msg = PyUnicode_FromFormat(
        (
            "%U=== Symtable for %U ===\n"
            "%U%s%s\n"
            "%U%s%s%s%s%s%s%s%s%s%s%s%s%s\n"
            "%Ulineno: %d col_offset: %d\n"
            "%U--- Symbols ---\n"
        ),
        prefix,
        ste->ste_name,
        prefix,
        blocktype,
        comptype,
        prefix,
        ste->ste_nested ? " nested" : "",
        ste->ste_free ? " free" : "",
        ste->ste_child_free ? " child_free" : "",
        ste->ste_generator ? " generator" : "",
        ste->ste_coroutine ? " coroutine" : "",
        ste->ste_varargs ? " varargs" : "",
        ste->ste_varkeywords ? " varkeywords" : "",
        ste->ste_returns_value ? " returns_value" : "",
        ste->ste_needs_class_closure ? " needs_class_closure" : "",
        ste->ste_needs_classdict ? " needs_classdict" : "",
        ste->ste_comp_inlined ? " comp_inlined" : "",
        ste->ste_comp_iter_target ? " comp_iter_target" : "",
        ste->ste_can_see_class_scope ? " can_see_class_scope" : "",
        prefix,
        ste->ste_lineno,
        ste->ste_col_offset,
        prefix
    );
    assert(msg != NULL);
    printf("%s", PyUnicode_AsUTF8(msg));
    Py_DECREF(msg);
    PyObject *name, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(ste->ste_symbols, &pos, &name, &value)) {
        int scope = _PyST_GetScope(ste, name);
        long flags = _PyST_GetSymbol(ste, name);
        printf("%s  %s: ", PyUnicode_AsUTF8(prefix), PyUnicode_AsUTF8(name));
        if (flags & DEF_GLOBAL) printf(" DEF_GLOBAL");
        if (flags & DEF_LOCAL) printf(" DEF_LOCAL");
        if (flags & DEF_PARAM) printf(" DEF_PARAM");
        if (flags & DEF_NONLOCAL) printf(" DEF_NONLOCAL");
        if (flags & USE) printf(" USE");
        if (flags & DEF_FREE) printf(" DEF_FREE");
        if (flags & DEF_FREE_CLASS) printf(" DEF_FREE_CLASS");
        if (flags & DEF_IMPORT) printf(" DEF_IMPORT");
        if (flags & DEF_ANNOT) printf(" DEF_ANNOT");
        if (flags & DEF_COMP_ITER) printf(" DEF_COMP_ITER");
        if (flags & DEF_TYPE_PARAM) printf(" DEF_TYPE_PARAM");
        if (flags & DEF_COMP_CELL) printf(" DEF_COMP_CELL");
        switch (scope) {
            case LOCAL: printf(" LOCAL"); break;
            case GLOBAL_EXPLICIT: printf(" GLOBAL_EXPLICIT"); break;
            case GLOBAL_IMPLICIT: printf(" GLOBAL_IMPLICIT"); break;
            case FREE: printf(" FREE"); break;
            case CELL: printf(" CELL"); break;
        }
        printf("\n");
    }
    printf("%s--- Children ---\n", PyUnicode_AsUTF8(prefix));
    PyObject *new_prefix = PyUnicode_FromFormat("  %U", prefix);
    assert(new_prefix != NULL);
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(ste->ste_children); i++) {
        PyObject *child = PyList_GetItem(ste->ste_children, i);
        assert(child != NULL && PySTEntry_Check(child));
        _dump_symtable((PySTEntryObject *)child, new_prefix);
    }
    Py_DECREF(new_prefix);
}

static void dump_symtable(PySTEntryObject* ste)
{
    PyObject *empty = PyUnicode_FromString("");
    assert(empty != NULL);
    _dump_symtable(ste, empty);
    Py_DECREF(empty);
}
#endif

#define DUPLICATE_ARGUMENT \
"duplicate argument '%U' in function definition"

static struct symtable *
symtable_new(void)
{
    struct symtable *st;

    st = (struct symtable *)PyMem_Malloc(sizeof(struct symtable));
    if (st == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

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
    _PySymtable_Free(st);
    return NULL;
}

/* Using a scaling factor means this should automatically adjust when
   the recursion limit is adjusted for small or large C stack allocations.
*/
#define COMPILER_STACK_FRAME_SCALE 2

struct symtable *
_PySymtable_Build(mod_ty mod, PyObject *filename, PyFutureFeatures *future)
{
    struct symtable *st = symtable_new();
    asdl_stmt_seq *seq;
    int i;
    PyThreadState *tstate;
    int starting_recursion_depth;

    if (st == NULL)
        return NULL;
    if (filename == NULL) {
        _PySymtable_Free(st);
        return NULL;
    }
    st->st_filename = Py_NewRef(filename);
    st->st_future = future;

    /* Setup recursion depth check counters */
    tstate = _PyThreadState_GET();
    if (!tstate) {
        _PySymtable_Free(st);
        return NULL;
    }
    /* Be careful here to prevent overflow. */
    int recursion_depth = Py_C_RECURSION_LIMIT - tstate->c_recursion_remaining;
    starting_recursion_depth = recursion_depth * COMPILER_STACK_FRAME_SCALE;
    st->recursion_depth = starting_recursion_depth;
    st->recursion_limit = Py_C_RECURSION_LIMIT * COMPILER_STACK_FRAME_SCALE;

    /* Make the initial symbol information gathering pass */
    if (!symtable_enter_block(st, &_Py_ID(top), ModuleBlock, (void *)mod, 0, 0, 0, 0)) {
        _PySymtable_Free(st);
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
    case FunctionType_kind:
        PyErr_SetString(PyExc_RuntimeError,
                        "this compiler does not handle FunctionTypes");
        goto error;
    }
    if (!symtable_exit_block(st)) {
        _PySymtable_Free(st);
        return NULL;
    }
    /* Check that the recursion depth counting balanced correctly */
    if (st->recursion_depth != starting_recursion_depth) {
        PyErr_Format(PyExc_SystemError,
            "symtable analysis recursion depth mismatch (before=%d, after=%d)",
            starting_recursion_depth, st->recursion_depth);
        _PySymtable_Free(st);
        return NULL;
    }
    /* Make the second symbol analysis pass */
    if (symtable_analyze(st)) {
#if _PY_DUMP_SYMTABLE
        dump_symtable(st->st_top);
#endif
        return st;
    }
    _PySymtable_Free(st);
    return NULL;
 error:
    (void) symtable_exit_block(st);
    _PySymtable_Free(st);
    return NULL;
}


void
_PySymtable_Free(struct symtable *st)
{
    Py_XDECREF(st->st_filename);
    Py_XDECREF(st->st_blocks);
    Py_XDECREF(st->st_stack);
    PyMem_Free((void *)st);
}

PySTEntryObject *
_PySymtable_Lookup(struct symtable *st, void *key)
{
    PyObject *k, *v;

    k = PyLong_FromVoidPtr(key);
    if (k == NULL)
        return NULL;
    if (PyDict_GetItemRef(st->st_blocks, k, &v) == 0) {
        PyErr_SetString(PyExc_KeyError,
                        "unknown symbol table entry");
    }
    Py_DECREF(k);

    assert(v == NULL || PySTEntry_Check(v));
    return (PySTEntryObject *)v;
}

long
_PyST_GetSymbol(PySTEntryObject *ste, PyObject *name)
{
    PyObject *v = PyDict_GetItemWithError(ste->ste_symbols, name);
    if (!v)
        return 0;
    assert(PyLong_Check(v));
    return PyLong_AS_LONG(v);
}

int
_PyST_GetScope(PySTEntryObject *ste, PyObject *name)
{
    long symbol = _PyST_GetSymbol(ste, name);
    return (symbol >> SCOPE_OFFSET) & SCOPE_MASK;
}

int
_PyST_IsFunctionLike(PySTEntryObject *ste)
{
    return ste->ste_type == FunctionBlock
        || ste->ste_type == TypeVarBoundBlock
        || ste->ste_type == TypeAliasBlock
        || ste->ste_type == TypeParamBlock;
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
            PyErr_RangedSyntaxLocationObject(ste->ste_table->st_filename,
                                             PyLong_AsLong(PyTuple_GET_ITEM(data, 1)),
                                             PyLong_AsLong(PyTuple_GET_ITEM(data, 2)) + 1,
                                             PyLong_AsLong(PyTuple_GET_ITEM(data, 3)),
                                             PyLong_AsLong(PyTuple_GET_ITEM(data, 4)) + 1);

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
             PyObject *global, PyObject *type_params, PySTEntryObject *class_entry)
{
    int contains;
    if (flags & DEF_GLOBAL) {
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
        if (!bound) {
            PyErr_Format(PyExc_SyntaxError,
                         "nonlocal declaration not allowed at module level");
            return error_at_directive(ste, name);
        }
        contains = PySet_Contains(bound, name);
        if (contains < 0) {
            return 0;
        }
        if (!contains) {
            PyErr_Format(PyExc_SyntaxError,
                         "no binding for nonlocal '%U' found",
                         name);

            return error_at_directive(ste, name);
        }
        contains = PySet_Contains(type_params, name);
        if (contains < 0) {
            return 0;
        }
        if (contains) {
            PyErr_Format(PyExc_SyntaxError,
                         "nonlocal binding not allowed for type parameter '%U'",
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
        if (flags & DEF_TYPE_PARAM) {
            if (PySet_Add(type_params, name) < 0)
                return 0;
        }
        else {
            if (PySet_Discard(type_params, name) < 0)
                return 0;
        }
        return 1;
    }
    // If we were passed class_entry (i.e., we're in an ste_can_see_class_scope scope)
    // and the bound name is in that set, then the name is potentially bound both by
    // the immediately enclosing class namespace, and also by an outer function namespace.
    // In that case, we want the runtime name resolution to look at only the class
    // namespace and the globals (not the namespace providing the bound).
    // Similarly, if the name is explicitly global in the class namespace (through the
    // global statement), we want to also treat it as a global in this scope.
    if (class_entry != NULL) {
        long class_flags = _PyST_GetSymbol(class_entry, name);
        if (class_flags & DEF_GLOBAL) {
            SET_SCOPE(scopes, name, GLOBAL_EXPLICIT);
            return 1;
        }
        else if (class_flags & DEF_BOUND && !(class_flags & DEF_NONLOCAL)) {
            SET_SCOPE(scopes, name, GLOBAL_IMPLICIT);
            return 1;
        }
    }
    /* If an enclosing block has a binding for this name, it
       is a free variable rather than a global variable.
       Note that having a non-NULL bound implies that the block
       is nested.
    */
    if (bound) {
        contains = PySet_Contains(bound, name);
        if (contains < 0) {
            return 0;
        }
        if (contains) {
            SET_SCOPE(scopes, name, FREE);
            ste->ste_free = 1;
            return PySet_Add(free, name) >= 0;
        }
    }
    /* If a parent has a global statement, then call it global
       explicit?  It could also be global implicit.
     */
    if (global) {
        contains = PySet_Contains(global, name);
        if (contains < 0) {
            return 0;
        }
        if (contains) {
            SET_SCOPE(scopes, name, GLOBAL_IMPLICIT);
            return 1;
        }
    }
    if (ste->ste_nested)
        ste->ste_free = 1;
    SET_SCOPE(scopes, name, GLOBAL_IMPLICIT);
    return 1;
}

static int
is_free_in_any_child(PySTEntryObject *entry, PyObject *key)
{
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(entry->ste_children); i++) {
        PySTEntryObject *child_ste = (PySTEntryObject *)PyList_GET_ITEM(
            entry->ste_children, i);
        long scope = _PyST_GetScope(child_ste, key);
        if (scope == FREE) {
            return 1;
        }
    }
    return 0;
}

static int
inline_comprehension(PySTEntryObject *ste, PySTEntryObject *comp,
                     PyObject *scopes, PyObject *comp_free,
                     PyObject *inlined_cells)
{
    PyObject *k, *v;
    Py_ssize_t pos = 0;
    while (PyDict_Next(comp->ste_symbols, &pos, &k, &v)) {
        // skip comprehension parameter
        long comp_flags = PyLong_AS_LONG(v);
        if (comp_flags & DEF_PARAM) {
            assert(_PyUnicode_EqualToASCIIString(k, ".0"));
            continue;
        }
        int scope = (comp_flags >> SCOPE_OFFSET) & SCOPE_MASK;
        int only_flags = comp_flags & ((1 << SCOPE_OFFSET) - 1);
        if (scope == CELL || only_flags & DEF_COMP_CELL) {
            if (PySet_Add(inlined_cells, k) < 0) {
                return 0;
            }
        }
        PyObject *existing = PyDict_GetItemWithError(ste->ste_symbols, k);
        if (existing == NULL && PyErr_Occurred()) {
            return 0;
        }
        if (!existing) {
            // name does not exist in scope, copy from comprehension
            assert(scope != FREE || PySet_Contains(comp_free, k) == 1);
            PyObject *v_flags = PyLong_FromLong(only_flags);
            if (v_flags == NULL) {
                return 0;
            }
            int ok = PyDict_SetItem(ste->ste_symbols, k, v_flags);
            Py_DECREF(v_flags);
            if (ok < 0) {
                return 0;
            }
            SET_SCOPE(scopes, k, scope);
        }
        else {
            if (PyLong_AsLong(existing) & DEF_BOUND) {
                // free vars in comprehension that are locals in outer scope can
                // now simply be locals, unless they are free in comp children,
                // or if the outer scope is a class block
                if (!is_free_in_any_child(comp, k) && ste->ste_type != ClassBlock) {
                    if (PySet_Discard(comp_free, k) < 0) {
                        return 0;
                    }
                }
            }
        }
    }
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
analyze_cells(PyObject *scopes, PyObject *free, PyObject *inlined_cells)
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
        int contains = PySet_Contains(free, name);
        if (contains < 0) {
            goto error;
        }
        if (!contains) {
            contains = PySet_Contains(inlined_cells, name);
            if (contains < 0) {
                goto error;
            }
            if (!contains) {
                continue;
            }
        }
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
    res = PySet_Discard(free, &_Py_ID(__class__));
    if (res < 0)
        return 0;
    if (res)
        ste->ste_needs_class_closure = 1;
    res = PySet_Discard(free, &_Py_ID(__classdict__));
    if (res < 0)
        return 0;
    if (res)
        ste->ste_needs_classdict = 1;
    return 1;
}

/* Enter the final scope information into the ste_symbols dict.
 *
 * All arguments are dicts.  Modifies symbols, others are read-only.
*/
static int
update_symbols(PyObject *symbols, PyObject *scopes,
               PyObject *bound, PyObject *free,
               PyObject *inlined_cells, int classflag)
{
    PyObject *name = NULL, *itr = NULL;
    PyObject *v = NULL, *v_scope = NULL, *v_new = NULL, *v_free = NULL;
    Py_ssize_t pos = 0;

    /* Update scope information for all symbols in this scope */
    while (PyDict_Next(symbols, &pos, &name, &v)) {
        long scope, flags;
        assert(PyLong_Check(v));
        flags = PyLong_AS_LONG(v);
        int contains = PySet_Contains(inlined_cells, name);
        if (contains < 0) {
            return 0;
        }
        if (contains) {
            flags |= DEF_COMP_CELL;
        }
        v_scope = PyDict_GetItemWithError(scopes, name);
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
    if (itr == NULL) {
        Py_DECREF(v_free);
        return 0;
    }

    while ((name = PyIter_Next(itr))) {
        v = PyDict_GetItemWithError(symbols, name);

        /* Handle symbol that already exists in this scope */
        if (v) {
            /* Handle a free variable in a method of
               the class that has the same name as a local
               or global in the class scope.
            */
            if  (classflag) {
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
        else if (PyErr_Occurred()) {
            goto error;
        }
        /* Handle global symbol */
        if (bound) {
            int contains = PySet_Contains(bound, name);
            if (contains < 0) {
                goto error;
            }
            if (!contains) {
                Py_DECREF(name);
                continue;       /* it's a global */
            }
        }
        /* Propagate new free symbol up the lexical stack */
        if (PyDict_SetItem(symbols, name, v_free) < 0) {
            goto error;
        }
        Py_DECREF(name);
    }

    /* Check if loop ended because of exception in PyIter_Next */
    if (PyErr_Occurred()) {
        goto error;
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
                    PyObject *global, PyObject *type_params,
                    PySTEntryObject *class_entry, PyObject **child_free);

static int
analyze_block(PySTEntryObject *ste, PyObject *bound, PyObject *free,
              PyObject *global, PyObject *type_params,
              PySTEntryObject *class_entry)
{
    PyObject *name, *v, *local = NULL, *scopes = NULL, *newbound = NULL;
    PyObject *newglobal = NULL, *newfree = NULL, *inlined_cells = NULL;
    PyObject *temp;
    int success = 0;
    Py_ssize_t i, pos = 0;

    local = PySet_New(NULL);  /* collect new names bound in block */
    if (!local)
        goto error;
    scopes = PyDict_New();  /* collect scopes defined for each name */
    if (!scopes)
        goto error;

    /* Allocate new global, bound and free variable sets.  These
       sets hold the names visible in nested blocks.  For
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
    inlined_cells = PySet_New(NULL);
    if (!inlined_cells)
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
                          bound, local, free, global, type_params, class_entry))
            goto error;
    }

    /* Populate global and bound sets to be passed to children. */
    if (ste->ste_type != ClassBlock) {
        /* Add function locals to bound set */
        if (_PyST_IsFunctionLike(ste)) {
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
        /* Special-case __class__ and __classdict__ */
        if (PySet_Add(newbound, &_Py_ID(__class__)) < 0)
            goto error;
        if (PySet_Add(newbound, &_Py_ID(__classdict__)) < 0)
            goto error;
    }

    /* Recursively call analyze_child_block() on each child block.

       newbound, newglobal now contain the names visible in
       nested blocks.  The free variables in the children will
       be added to newfree.
    */
    for (i = 0; i < PyList_GET_SIZE(ste->ste_children); ++i) {
        PyObject *child_free = NULL;
        PyObject *c = PyList_GET_ITEM(ste->ste_children, i);
        PySTEntryObject* entry;
        assert(c && PySTEntry_Check(c));
        entry = (PySTEntryObject*)c;

        PySTEntryObject *new_class_entry = NULL;
        if (entry->ste_can_see_class_scope) {
            if (ste->ste_type == ClassBlock) {
                new_class_entry = ste;
            }
            else if (class_entry) {
                new_class_entry = class_entry;
            }
        }

        // we inline all non-generator-expression comprehensions
        int inline_comp =
            entry->ste_comprehension &&
            !entry->ste_generator;

        if (!analyze_child_block(entry, newbound, newfree, newglobal,
                                 type_params, new_class_entry, &child_free))
        {
            goto error;
        }
        if (inline_comp) {
            if (!inline_comprehension(ste, entry, scopes, child_free, inlined_cells)) {
                Py_DECREF(child_free);
                goto error;
            }
            entry->ste_comp_inlined = 1;
        }
        temp = PyNumber_InPlaceOr(newfree, child_free);
        Py_DECREF(child_free);
        if (!temp)
            goto error;
        Py_DECREF(temp);
        /* Check if any children have free variables */
        if (entry->ste_free || entry->ste_child_free)
            ste->ste_child_free = 1;
    }

    /* Splice children of inlined comprehensions into our children list */
    for (i = PyList_GET_SIZE(ste->ste_children) - 1; i >= 0; --i) {
        PyObject* c = PyList_GET_ITEM(ste->ste_children, i);
        PySTEntryObject* entry;
        assert(c && PySTEntry_Check(c));
        entry = (PySTEntryObject*)c;
        if (entry->ste_comp_inlined &&
            PyList_SetSlice(ste->ste_children, i, i + 1,
                            entry->ste_children) < 0)
        {
            goto error;
        }
    }

    /* Check if any local variables must be converted to cell variables */
    if (_PyST_IsFunctionLike(ste) && !analyze_cells(scopes, newfree, inlined_cells))
        goto error;
    else if (ste->ste_type == ClassBlock && !drop_class_free(ste, newfree))
        goto error;
    /* Records the results of the analysis in the symbol table entry */
    if (!update_symbols(ste->ste_symbols, scopes, bound, newfree, inlined_cells,
                        (ste->ste_type == ClassBlock) || ste->ste_can_see_class_scope))
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
    Py_XDECREF(inlined_cells);
    if (!success)
        assert(PyErr_Occurred());
    return success;
}

static int
analyze_child_block(PySTEntryObject *entry, PyObject *bound, PyObject *free,
                    PyObject *global, PyObject *type_params,
                    PySTEntryObject *class_entry, PyObject** child_free)
{
    PyObject *temp_bound = NULL, *temp_global = NULL, *temp_free = NULL;
    PyObject *temp_type_params = NULL;

    /* Copy the bound/global/free sets.

       These sets are used by all blocks enclosed by the
       current block.  The analyze_block() call modifies these
       sets.

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
    temp_type_params = PySet_New(type_params);
    if (!temp_type_params)
        goto error;

    if (!analyze_block(entry, temp_bound, temp_free, temp_global,
                       temp_type_params, class_entry))
        goto error;
    *child_free = temp_free;
    Py_DECREF(temp_bound);
    Py_DECREF(temp_global);
    Py_DECREF(temp_type_params);
    return 1;
 error:
    Py_XDECREF(temp_bound);
    Py_XDECREF(temp_free);
    Py_XDECREF(temp_global);
    Py_XDECREF(temp_type_params);
    return 0;
}

static int
symtable_analyze(struct symtable *st)
{
    PyObject *free, *global, *type_params;
    int r;

    free = PySet_New(NULL);
    if (!free)
        return 0;
    global = PySet_New(NULL);
    if (!global) {
        Py_DECREF(free);
        return 0;
    }
    type_params = PySet_New(NULL);
    if (!type_params) {
        Py_DECREF(free);
        Py_DECREF(global);
        return 0;
    }
    r = analyze_block(st->st_top, NULL, free, global, type_params, NULL);
    Py_DECREF(free);
    Py_DECREF(global);
    Py_DECREF(type_params);
    return r;
}

/* symtable_enter_block() gets a reference via ste_new.
   This reference is released when the block is exited, via the DECREF
   in symtable_exit_block().
*/

static int
symtable_exit_block(struct symtable *st)
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
                     void *ast, int lineno, int col_offset,
                     int end_lineno, int end_col_offset)
{
    PySTEntryObject *prev = NULL, *ste;

    ste = ste_new(st, name, block, ast, lineno, col_offset, end_lineno, end_col_offset);
    if (ste == NULL)
        return 0;
    if (PyList_Append(st->st_stack, (PyObject *)ste) < 0) {
        Py_DECREF(ste);
        return 0;
    }
    prev = st->st_cur;
    /* bpo-37757: For now, disallow *all* assignment expressions in the
     * outermost iterator expression of a comprehension, even those inside
     * a nested comprehension or a lambda expression.
     */
    if (prev) {
        ste->ste_comp_iter_expr = prev->ste_comp_iter_expr;
    }
    /* The entry is owned by the stack. Borrow it for st_cur. */
    Py_DECREF(ste);
    st->st_cur = ste;

    /* Annotation blocks shouldn't have any affect on the symbol table since in
     * the compilation stage, they will all be transformed to strings. They are
     * only created if future 'annotations' feature is activated. */
    if (block == AnnotationBlock) {
        return 1;
    }

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
    PyObject *mangled = _Py_Mangle(st->st_private, name);
    if (!mangled)
        return 0;
    long ret = _PyST_GetSymbol(st->st_cur, mangled);
    Py_DECREF(mangled);
    return ret;
}

static int
symtable_add_def_helper(struct symtable *st, PyObject *name, int flag, struct _symtable_entry *ste,
                        int lineno, int col_offset, int end_lineno, int end_col_offset)
{
    PyObject *o;
    PyObject *dict;
    long val;
    PyObject *mangled = _Py_Mangle(st->st_private, name);


    if (!mangled)
        return 0;
    dict = ste->ste_symbols;
    if ((o = PyDict_GetItemWithError(dict, mangled))) {
        val = PyLong_AS_LONG(o);
        if ((flag & DEF_PARAM) && (val & DEF_PARAM)) {
            /* Is it better to use 'mangled' or 'name' here? */
            PyErr_Format(PyExc_SyntaxError, DUPLICATE_ARGUMENT, name);
            PyErr_RangedSyntaxLocationObject(st->st_filename,
                                             lineno, col_offset + 1,
                                             end_lineno, end_col_offset + 1);
            goto error;
        }
        if ((flag & DEF_TYPE_PARAM) && (val & DEF_TYPE_PARAM)) {
            PyErr_Format(PyExc_SyntaxError, DUPLICATE_TYPE_PARAM, name);
            PyErr_RangedSyntaxLocationObject(st->st_filename,
                                             lineno, col_offset + 1,
                                             end_lineno, end_col_offset + 1);
            goto error;
        }
        val |= flag;
    }
    else if (PyErr_Occurred()) {
        goto error;
    }
    else {
        val = flag;
    }
    if (ste->ste_comp_iter_target) {
        /* This name is an iteration variable in a comprehension,
         * so check for a binding conflict with any named expressions.
         * Otherwise, mark it as an iteration variable so subsequent
         * named expressions can check for conflicts.
         */
        if (val & (DEF_GLOBAL | DEF_NONLOCAL)) {
            PyErr_Format(PyExc_SyntaxError,
                NAMED_EXPR_COMP_INNER_LOOP_CONFLICT, name);
            PyErr_RangedSyntaxLocationObject(st->st_filename,
                                             lineno, col_offset + 1,
                                             end_lineno, end_col_offset + 1);
            goto error;
        }
        val |= DEF_COMP_ITER;
    }
    o = PyLong_FromLong(val);
    if (o == NULL)
        goto error;
    if (PyDict_SetItem(dict, mangled, o) < 0) {
        Py_DECREF(o);
        goto error;
    }
    Py_DECREF(o);

    if (flag & DEF_PARAM) {
        if (PyList_Append(ste->ste_varnames, mangled) < 0)
            goto error;
    } else      if (flag & DEF_GLOBAL) {
        /* XXX need to update DEF_GLOBAL for other flags too;
           perhaps only DEF_FREE_GLOBAL */
        val = flag;
        if ((o = PyDict_GetItemWithError(st->st_global, mangled))) {
            val |= PyLong_AS_LONG(o);
        }
        else if (PyErr_Occurred()) {
            goto error;
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

static int
symtable_add_def(struct symtable *st, PyObject *name, int flag,
                 int lineno, int col_offset, int end_lineno, int end_col_offset)
{
    return symtable_add_def_helper(st, name, flag, st->st_cur,
                        lineno, col_offset, end_lineno, end_col_offset);
}

static int
symtable_enter_type_param_block(struct symtable *st, identifier name,
                               void *ast, int has_defaults, int has_kwdefaults,
                               enum _stmt_kind kind,
                               int lineno, int col_offset,
                               int end_lineno, int end_col_offset)
{
    _Py_block_ty current_type = st->st_cur->ste_type;
    if(!symtable_enter_block(st, name, TypeParamBlock, ast, lineno,
                             col_offset, end_lineno, end_col_offset)) {
        return 0;
    }
    if (current_type == ClassBlock) {
        st->st_cur->ste_can_see_class_scope = 1;
        if (!symtable_add_def(st, &_Py_ID(__classdict__), USE, lineno, col_offset, end_lineno, end_col_offset)) {
            return 0;
        }
    }
    if (kind == ClassDef_kind) {
        _Py_DECLARE_STR(type_params, ".type_params");
        // It gets "set" when we create the type params tuple and
        // "used" when we build up the bases.
        if (!symtable_add_def(st, &_Py_STR(type_params), DEF_LOCAL,
                              lineno, col_offset, end_lineno, end_col_offset)) {
            return 0;
        }
        if (!symtable_add_def(st, &_Py_STR(type_params), USE,
                              lineno, col_offset, end_lineno, end_col_offset)) {
            return 0;
        }
        st->st_private = name;
        // This is used for setting the generic base
        _Py_DECLARE_STR(generic_base, ".generic_base");
        if (!symtable_add_def(st, &_Py_STR(generic_base), DEF_LOCAL,
                              lineno, col_offset, end_lineno, end_col_offset)) {
            return 0;
        }
        if (!symtable_add_def(st, &_Py_STR(generic_base), USE,
                              lineno, col_offset, end_lineno, end_col_offset)) {
            return 0;
        }
    }
    if (has_defaults) {
        _Py_DECLARE_STR(defaults, ".defaults");
        if (!symtable_add_def(st, &_Py_STR(defaults), DEF_PARAM,
                              lineno, col_offset, end_lineno, end_col_offset)) {
            return 0;
        }
    }
    if (has_kwdefaults) {
        _Py_DECLARE_STR(kwdefaults, ".kwdefaults");
        if (!symtable_add_def(st, &_Py_STR(kwdefaults), DEF_PARAM,
                              lineno, col_offset, end_lineno, end_col_offset)) {
            return 0;
        }
    }
    return 1;
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
    asdl_ ## TYPE ## _seq *seq = (SEQ); /* avoid variable capture */ \
    for (i = 0; i < asdl_seq_LEN(seq); i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, i); \
        if (!symtable_visit_ ## TYPE((ST), elt)) \
            VISIT_QUIT((ST), 0);                 \
    } \
}

#define VISIT_SEQ_TAIL(ST, TYPE, SEQ, START) { \
    int i; \
    asdl_ ## TYPE ## _seq *seq = (SEQ); /* avoid variable capture */ \
    for (i = (START); i < asdl_seq_LEN(seq); i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, i); \
        if (!symtable_visit_ ## TYPE((ST), elt)) \
            VISIT_QUIT((ST), 0);                 \
    } \
}

#define VISIT_SEQ_WITH_NULL(ST, TYPE, SEQ) {     \
    int i = 0; \
    asdl_ ## TYPE ## _seq *seq = (SEQ); /* avoid variable capture */ \
    for (i = 0; i < asdl_seq_LEN(seq); i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, i); \
        if (!elt) continue; /* can be NULL */ \
        if (!symtable_visit_ ## TYPE((ST), elt)) \
            VISIT_QUIT((ST), 0);             \
    } \
}

static int
symtable_record_directive(struct symtable *st, identifier name, int lineno,
                          int col_offset, int end_lineno, int end_col_offset)
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
    data = Py_BuildValue("(Niiii)", mangled, lineno, col_offset, end_lineno, end_col_offset);
    if (!data)
        return 0;
    res = PyList_Append(st->st_cur->ste_directives, data);
    Py_DECREF(data);
    return res == 0;
}

static int
has_kwonlydefaults(asdl_arg_seq *kwonlyargs, asdl_expr_seq *kw_defaults)
{
    for (int i = 0; i < asdl_seq_LEN(kwonlyargs); i++) {
        expr_ty default_ = asdl_seq_GET(kw_defaults, i);
        if (default_) {
            return 1;
        }
    }
    return 0;
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
        if (!symtable_add_def(st, s->v.FunctionDef.name, DEF_LOCAL, LOCATION(s)))
            VISIT_QUIT(st, 0);
        if (s->v.FunctionDef.args->defaults)
            VISIT_SEQ(st, expr, s->v.FunctionDef.args->defaults);
        if (s->v.FunctionDef.args->kw_defaults)
            VISIT_SEQ_WITH_NULL(st, expr, s->v.FunctionDef.args->kw_defaults);
        if (s->v.FunctionDef.decorator_list)
            VISIT_SEQ(st, expr, s->v.FunctionDef.decorator_list);
        if (asdl_seq_LEN(s->v.FunctionDef.type_params) > 0) {
            if (!symtable_enter_type_param_block(
                    st, s->v.FunctionDef.name,
                    (void *)s->v.FunctionDef.type_params,
                    s->v.FunctionDef.args->defaults != NULL,
                    has_kwonlydefaults(s->v.FunctionDef.args->kwonlyargs,
                                       s->v.FunctionDef.args->kw_defaults),
                    s->kind,
                    LOCATION(s))) {
                VISIT_QUIT(st, 0);
            }
            VISIT_SEQ(st, type_param, s->v.FunctionDef.type_params);
        }
        if (!symtable_visit_annotations(st, s, s->v.FunctionDef.args,
                                        s->v.FunctionDef.returns))
            VISIT_QUIT(st, 0);
        if (!symtable_enter_block(st, s->v.FunctionDef.name,
                                  FunctionBlock, (void *)s,
                                  LOCATION(s)))
            VISIT_QUIT(st, 0);
        VISIT(st, arguments, s->v.FunctionDef.args);
        VISIT_SEQ(st, stmt, s->v.FunctionDef.body);
        if (!symtable_exit_block(st))
            VISIT_QUIT(st, 0);
        if (asdl_seq_LEN(s->v.FunctionDef.type_params) > 0) {
            if (!symtable_exit_block(st))
                VISIT_QUIT(st, 0);
        }
        break;
    case ClassDef_kind: {
        PyObject *tmp;
        if (!symtable_add_def(st, s->v.ClassDef.name, DEF_LOCAL, LOCATION(s)))
            VISIT_QUIT(st, 0);
        if (s->v.ClassDef.decorator_list)
            VISIT_SEQ(st, expr, s->v.ClassDef.decorator_list);
        if (asdl_seq_LEN(s->v.ClassDef.type_params) > 0) {
            if (!symtable_enter_type_param_block(st, s->v.ClassDef.name,
                                                (void *)s->v.ClassDef.type_params,
                                                false, false, s->kind,
                                                LOCATION(s))) {
                VISIT_QUIT(st, 0);
            }
            VISIT_SEQ(st, type_param, s->v.ClassDef.type_params);
        }
        VISIT_SEQ(st, expr, s->v.ClassDef.bases);
        VISIT_SEQ(st, keyword, s->v.ClassDef.keywords);
        if (!symtable_enter_block(st, s->v.ClassDef.name, ClassBlock,
                                  (void *)s, s->lineno, s->col_offset,
                                  s->end_lineno, s->end_col_offset))
            VISIT_QUIT(st, 0);
        tmp = st->st_private;
        st->st_private = s->v.ClassDef.name;
        if (asdl_seq_LEN(s->v.ClassDef.type_params) > 0) {
            if (!symtable_add_def(st, &_Py_ID(__type_params__),
                                  DEF_LOCAL, LOCATION(s))) {
                VISIT_QUIT(st, 0);
            }
            _Py_DECLARE_STR(type_params, ".type_params");
            if (!symtable_add_def(st, &_Py_STR(type_params),
                                  USE, LOCATION(s))) {
                VISIT_QUIT(st, 0);
            }
        }
        VISIT_SEQ(st, stmt, s->v.ClassDef.body);
        st->st_private = tmp;
        if (!symtable_exit_block(st))
            VISIT_QUIT(st, 0);
        if (asdl_seq_LEN(s->v.ClassDef.type_params) > 0) {
            if (!symtable_exit_block(st))
                VISIT_QUIT(st, 0);
        }
        break;
    }
    case TypeAlias_kind: {
        VISIT(st, expr, s->v.TypeAlias.name);
        assert(s->v.TypeAlias.name->kind == Name_kind);
        PyObject *name = s->v.TypeAlias.name->v.Name.id;
        int is_in_class = st->st_cur->ste_type == ClassBlock;
        int is_generic = asdl_seq_LEN(s->v.TypeAlias.type_params) > 0;
        if (is_generic) {
            if (!symtable_enter_type_param_block(
                    st, name,
                    (void *)s->v.TypeAlias.type_params,
                    false, false, s->kind,
                    LOCATION(s))) {
                VISIT_QUIT(st, 0);
            }
            VISIT_SEQ(st, type_param, s->v.TypeAlias.type_params);
        }
        if (!symtable_enter_block(st, name, TypeAliasBlock,
                                  (void *)s, LOCATION(s)))
            VISIT_QUIT(st, 0);
        st->st_cur->ste_can_see_class_scope = is_in_class;
        if (is_in_class && !symtable_add_def(st, &_Py_ID(__classdict__), USE, LOCATION(s->v.TypeAlias.value))) {
            VISIT_QUIT(st, 0);
        }
        VISIT(st, expr, s->v.TypeAlias.value);
        if (!symtable_exit_block(st))
            VISIT_QUIT(st, 0);
        if (is_generic) {
            if (!symtable_exit_block(st))
                VISIT_QUIT(st, 0);
        }
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
                && (st->st_cur->ste_symbols != st->st_global)
                && s->v.AnnAssign.simple) {
                PyErr_Format(PyExc_SyntaxError,
                             cur & DEF_GLOBAL ? GLOBAL_ANNOT : NONLOCAL_ANNOT,
                             e_name->v.Name.id);
                PyErr_RangedSyntaxLocationObject(st->st_filename,
                                                 s->lineno,
                                                 s->col_offset + 1,
                                                 s->end_lineno,
                                                 s->end_col_offset + 1);
                VISIT_QUIT(st, 0);
            }
            if (s->v.AnnAssign.simple &&
                !symtable_add_def(st, e_name->v.Name.id,
                                  DEF_ANNOT | DEF_LOCAL, LOCATION(e_name))) {
                VISIT_QUIT(st, 0);
            }
            else {
                if (s->v.AnnAssign.value
                    && !symtable_add_def(st, e_name->v.Name.id, DEF_LOCAL, LOCATION(e_name))) {
                    VISIT_QUIT(st, 0);
                }
            }
        }
        else {
            VISIT(st, expr, s->v.AnnAssign.target);
        }
        if (!symtable_visit_annotation(st, s->v.AnnAssign.annotation)) {
            VISIT_QUIT(st, 0);
        }

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
    case Match_kind:
        VISIT(st, expr, s->v.Match.subject);
        VISIT_SEQ(st, match_case, s->v.Match.cases);
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
        VISIT_SEQ(st, excepthandler, s->v.Try.handlers);
        VISIT_SEQ(st, stmt, s->v.Try.orelse);
        VISIT_SEQ(st, stmt, s->v.Try.finalbody);
        break;
    case TryStar_kind:
        VISIT_SEQ(st, stmt, s->v.TryStar.body);
        VISIT_SEQ(st, excepthandler, s->v.TryStar.handlers);
        VISIT_SEQ(st, stmt, s->v.TryStar.orelse);
        VISIT_SEQ(st, stmt, s->v.TryStar.finalbody);
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
        asdl_identifier_seq *seq = s->v.Global.names;
        for (i = 0; i < asdl_seq_LEN(seq); i++) {
            identifier name = (identifier)asdl_seq_GET(seq, i);
            long cur = symtable_lookup(st, name);
            if (cur < 0)
                VISIT_QUIT(st, 0);
            if (cur & (DEF_PARAM | DEF_LOCAL | USE | DEF_ANNOT)) {
                const char* msg;
                if (cur & DEF_PARAM) {
                    msg = GLOBAL_PARAM;
                } else if (cur & USE) {
                    msg = GLOBAL_AFTER_USE;
                } else if (cur & DEF_ANNOT) {
                    msg = GLOBAL_ANNOT;
                } else {  /* DEF_LOCAL */
                    msg = GLOBAL_AFTER_ASSIGN;
                }
                PyErr_Format(PyExc_SyntaxError,
                             msg, name);
                PyErr_RangedSyntaxLocationObject(st->st_filename,
                                                 s->lineno,
                                                 s->col_offset + 1,
                                                 s->end_lineno,
                                                 s->end_col_offset + 1);
                VISIT_QUIT(st, 0);
            }
            if (!symtable_add_def(st, name, DEF_GLOBAL, LOCATION(s)))
                VISIT_QUIT(st, 0);
            if (!symtable_record_directive(st, name, s->lineno, s->col_offset,
                                           s->end_lineno, s->end_col_offset))
                VISIT_QUIT(st, 0);
        }
        break;
    }
    case Nonlocal_kind: {
        int i;
        asdl_identifier_seq *seq = s->v.Nonlocal.names;
        for (i = 0; i < asdl_seq_LEN(seq); i++) {
            identifier name = (identifier)asdl_seq_GET(seq, i);
            long cur = symtable_lookup(st, name);
            if (cur < 0)
                VISIT_QUIT(st, 0);
            if (cur & (DEF_PARAM | DEF_LOCAL | USE | DEF_ANNOT)) {
                const char* msg;
                if (cur & DEF_PARAM) {
                    msg = NONLOCAL_PARAM;
                } else if (cur & USE) {
                    msg = NONLOCAL_AFTER_USE;
                } else if (cur & DEF_ANNOT) {
                    msg = NONLOCAL_ANNOT;
                } else {  /* DEF_LOCAL */
                    msg = NONLOCAL_AFTER_ASSIGN;
                }
                PyErr_Format(PyExc_SyntaxError, msg, name);
                PyErr_RangedSyntaxLocationObject(st->st_filename,
                                                 s->lineno,
                                                 s->col_offset + 1,
                                                 s->end_lineno,
                                                 s->end_col_offset + 1);
                VISIT_QUIT(st, 0);
            }
            if (!symtable_add_def(st, name, DEF_NONLOCAL, LOCATION(s)))
                VISIT_QUIT(st, 0);
            if (!symtable_record_directive(st, name, s->lineno, s->col_offset,
                                           s->end_lineno, s->end_col_offset))
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
        if (!symtable_add_def(st, s->v.AsyncFunctionDef.name, DEF_LOCAL, LOCATION(s)))
            VISIT_QUIT(st, 0);
        if (s->v.AsyncFunctionDef.args->defaults)
            VISIT_SEQ(st, expr, s->v.AsyncFunctionDef.args->defaults);
        if (s->v.AsyncFunctionDef.args->kw_defaults)
            VISIT_SEQ_WITH_NULL(st, expr,
                                s->v.AsyncFunctionDef.args->kw_defaults);
        if (s->v.AsyncFunctionDef.decorator_list)
            VISIT_SEQ(st, expr, s->v.AsyncFunctionDef.decorator_list);
        if (asdl_seq_LEN(s->v.AsyncFunctionDef.type_params) > 0) {
            if (!symtable_enter_type_param_block(
                    st, s->v.AsyncFunctionDef.name,
                    (void *)s->v.AsyncFunctionDef.type_params,
                    s->v.AsyncFunctionDef.args->defaults != NULL,
                    has_kwonlydefaults(s->v.AsyncFunctionDef.args->kwonlyargs,
                                       s->v.AsyncFunctionDef.args->kw_defaults),
                    s->kind,
                    LOCATION(s))) {
                VISIT_QUIT(st, 0);
            }
            VISIT_SEQ(st, type_param, s->v.AsyncFunctionDef.type_params);
        }
        if (!symtable_visit_annotations(st, s, s->v.AsyncFunctionDef.args,
                                        s->v.AsyncFunctionDef.returns))
            VISIT_QUIT(st, 0);
        if (!symtable_enter_block(st, s->v.AsyncFunctionDef.name,
                                  FunctionBlock, (void *)s,
                                  s->lineno, s->col_offset,
                                  s->end_lineno, s->end_col_offset))
            VISIT_QUIT(st, 0);
        st->st_cur->ste_coroutine = 1;
        VISIT(st, arguments, s->v.AsyncFunctionDef.args);
        VISIT_SEQ(st, stmt, s->v.AsyncFunctionDef.body);
        if (!symtable_exit_block(st))
            VISIT_QUIT(st, 0);
        if (asdl_seq_LEN(s->v.AsyncFunctionDef.type_params) > 0) {
            if (!symtable_exit_block(st))
                VISIT_QUIT(st, 0);
        }
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
symtable_extend_namedexpr_scope(struct symtable *st, expr_ty e)
{
    assert(st->st_stack);
    assert(e->kind == Name_kind);

    PyObject *target_name = e->v.Name.id;
    Py_ssize_t i, size;
    struct _symtable_entry *ste;
    size = PyList_GET_SIZE(st->st_stack);
    assert(size);

    /* Iterate over the stack in reverse and add to the nearest adequate scope */
    for (i = size - 1; i >= 0; i--) {
        ste = (struct _symtable_entry *) PyList_GET_ITEM(st->st_stack, i);

        /* If we find a comprehension scope, check for a target
         * binding conflict with iteration variables, otherwise skip it
         */
        if (ste->ste_comprehension) {
            long target_in_scope = _PyST_GetSymbol(ste, target_name);
            if ((target_in_scope & DEF_COMP_ITER) &&
                (target_in_scope & DEF_LOCAL)) {
                PyErr_Format(PyExc_SyntaxError, NAMED_EXPR_COMP_CONFLICT, target_name);
                PyErr_RangedSyntaxLocationObject(st->st_filename,
                                                  e->lineno,
                                                  e->col_offset + 1,
                                                  e->end_lineno,
                                                  e->end_col_offset + 1);
                VISIT_QUIT(st, 0);
            }
            continue;
        }

        /* If we find a FunctionBlock entry, add as GLOBAL/LOCAL or NONLOCAL/LOCAL */
        if (ste->ste_type == FunctionBlock) {
            long target_in_scope = _PyST_GetSymbol(ste, target_name);
            if (target_in_scope & DEF_GLOBAL) {
                if (!symtable_add_def(st, target_name, DEF_GLOBAL, LOCATION(e)))
                    VISIT_QUIT(st, 0);
            } else {
                if (!symtable_add_def(st, target_name, DEF_NONLOCAL, LOCATION(e)))
                    VISIT_QUIT(st, 0);
            }
            if (!symtable_record_directive(st, target_name, LOCATION(e)))
                VISIT_QUIT(st, 0);

            return symtable_add_def_helper(st, target_name, DEF_LOCAL, ste, LOCATION(e));
        }
        /* If we find a ModuleBlock entry, add as GLOBAL */
        if (ste->ste_type == ModuleBlock) {
            if (!symtable_add_def(st, target_name, DEF_GLOBAL, LOCATION(e)))
                VISIT_QUIT(st, 0);
            if (!symtable_record_directive(st, target_name, LOCATION(e)))
                VISIT_QUIT(st, 0);

            return symtable_add_def_helper(st, target_name, DEF_GLOBAL, ste, LOCATION(e));
        }
        /* Disallow usage in ClassBlock and type scopes */
        if (ste->ste_type == ClassBlock ||
            ste->ste_type == TypeParamBlock ||
            ste->ste_type == TypeAliasBlock ||
            ste->ste_type == TypeVarBoundBlock) {
            switch (ste->ste_type) {
                case ClassBlock:
                    PyErr_Format(PyExc_SyntaxError, NAMED_EXPR_COMP_IN_CLASS);
                    break;
                case TypeParamBlock:
                    PyErr_Format(PyExc_SyntaxError, NAMED_EXPR_COMP_IN_TYPEPARAM);
                    break;
                case TypeAliasBlock:
                    PyErr_Format(PyExc_SyntaxError, NAMED_EXPR_COMP_IN_TYPEALIAS);
                    break;
                case TypeVarBoundBlock:
                    PyErr_Format(PyExc_SyntaxError, NAMED_EXPR_COMP_IN_TYPEVAR_BOUND);
                    break;
                default:
                    Py_UNREACHABLE();
            }
            PyErr_RangedSyntaxLocationObject(st->st_filename,
                                              e->lineno,
                                              e->col_offset + 1,
                                              e->end_lineno,
                                              e->end_col_offset + 1);
            VISIT_QUIT(st, 0);
        }
    }

    /* We should always find either a function-like block, ModuleBlock or ClassBlock
       and should never fall to this case
    */
    Py_UNREACHABLE();
    return 0;
}

static int
symtable_handle_namedexpr(struct symtable *st, expr_ty e)
{
    if (st->st_cur->ste_comp_iter_expr > 0) {
        /* Assignment isn't allowed in a comprehension iterable expression */
        PyErr_Format(PyExc_SyntaxError, NAMED_EXPR_COMP_ITER_EXPR);
        PyErr_RangedSyntaxLocationObject(st->st_filename,
                                          e->lineno,
                                          e->col_offset + 1,
                                          e->end_lineno,
                                          e->end_col_offset + 1);
        return 0;
    }
    if (st->st_cur->ste_comprehension) {
        /* Inside a comprehension body, so find the right target scope */
        if (!symtable_extend_namedexpr_scope(st, e->v.NamedExpr.target))
            return 0;
    }
    VISIT(st, expr, e->v.NamedExpr.value);
    VISIT(st, expr, e->v.NamedExpr.target);
    return 1;
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
    case NamedExpr_kind:
        if (!symtable_raise_if_annotation_block(st, "named expression", e)) {
            VISIT_QUIT(st, 0);
        }
        if(!symtable_handle_namedexpr(st, e))
            VISIT_QUIT(st, 0);
        break;
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
        if (st->st_cur->ste_can_see_class_scope) {
            // gh-109118
            PyErr_Format(PyExc_SyntaxError,
                         "Cannot use lambda in annotation scope within class scope");
            PyErr_RangedSyntaxLocationObject(st->st_filename,
                                              e->lineno,
                                              e->col_offset + 1,
                                              e->end_lineno,
                                              e->end_col_offset + 1);
            VISIT_QUIT(st, 0);
        }
        if (e->v.Lambda.args->defaults)
            VISIT_SEQ(st, expr, e->v.Lambda.args->defaults);
        if (e->v.Lambda.args->kw_defaults)
            VISIT_SEQ_WITH_NULL(st, expr, e->v.Lambda.args->kw_defaults);
        if (!symtable_enter_block(st, &_Py_ID(lambda),
                                  FunctionBlock, (void *)e,
                                  e->lineno, e->col_offset,
                                  e->end_lineno, e->end_col_offset))
            VISIT_QUIT(st, 0);
        VISIT(st, arguments, e->v.Lambda.args);
        VISIT(st, expr, e->v.Lambda.body);
        if (!symtable_exit_block(st))
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
        if (!symtable_raise_if_annotation_block(st, "yield expression", e)) {
            VISIT_QUIT(st, 0);
        }
        if (e->v.Yield.value)
            VISIT(st, expr, e->v.Yield.value);
        st->st_cur->ste_generator = 1;
        if (st->st_cur->ste_comprehension) {
            return symtable_raise_if_comprehension_block(st, e);
        }
        break;
    case YieldFrom_kind:
        if (!symtable_raise_if_annotation_block(st, "yield expression", e)) {
            VISIT_QUIT(st, 0);
        }
        VISIT(st, expr, e->v.YieldFrom.value);
        st->st_cur->ste_generator = 1;
        if (st->st_cur->ste_comprehension) {
            return symtable_raise_if_comprehension_block(st, e);
        }
        break;
    case Await_kind:
        if (!symtable_raise_if_annotation_block(st, "await expression", e)) {
            VISIT_QUIT(st, 0);
        }
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
        /* Nothing to do here. */
        break;
    /* The following exprs can be assignment targets. */
    case Attribute_kind:
        VISIT(st, expr, e->v.Attribute.value);
        break;
    case Subscript_kind:
        VISIT(st, expr, e->v.Subscript.value);
        VISIT(st, expr, e->v.Subscript.slice);
        break;
    case Starred_kind:
        VISIT(st, expr, e->v.Starred.value);
        break;
    case Slice_kind:
        if (e->v.Slice.lower)
            VISIT(st, expr, e->v.Slice.lower)
        if (e->v.Slice.upper)
            VISIT(st, expr, e->v.Slice.upper)
        if (e->v.Slice.step)
            VISIT(st, expr, e->v.Slice.step)
        break;
    case Name_kind:
        if (!symtable_add_def(st, e->v.Name.id,
                              e->v.Name.ctx == Load ? USE : DEF_LOCAL, LOCATION(e)))
            VISIT_QUIT(st, 0);
        /* Special-case super: it counts as a use of __class__ */
        if (e->v.Name.ctx == Load &&
            _PyST_IsFunctionLike(st->st_cur) &&
            _PyUnicode_EqualToASCIIString(e->v.Name.id, "super")) {
            if (!symtable_add_def(st, &_Py_ID(__class__), USE, LOCATION(e)))
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
symtable_visit_type_param(struct symtable *st, type_param_ty tp)
{
    if (++st->recursion_depth > st->recursion_limit) {
        PyErr_SetString(PyExc_RecursionError,
                        "maximum recursion depth exceeded during compilation");
        VISIT_QUIT(st, 0);
    }
    switch(tp->kind) {
    case TypeVar_kind:
        if (!symtable_add_def(st, tp->v.TypeVar.name, DEF_TYPE_PARAM | DEF_LOCAL, LOCATION(tp)))
            VISIT_QUIT(st, 0);
        if (tp->v.TypeVar.bound) {
            int is_in_class = st->st_cur->ste_can_see_class_scope;
            if (!symtable_enter_block(st, tp->v.TypeVar.name,
                                      TypeVarBoundBlock, (void *)tp,
                                      LOCATION(tp)))
                VISIT_QUIT(st, 0);
            st->st_cur->ste_can_see_class_scope = is_in_class;
            if (is_in_class && !symtable_add_def(st, &_Py_ID(__classdict__), USE, LOCATION(tp->v.TypeVar.bound))) {
                VISIT_QUIT(st, 0);
            }
            VISIT(st, expr, tp->v.TypeVar.bound);
            if (!symtable_exit_block(st))
                VISIT_QUIT(st, 0);
        }
        break;
    case TypeVarTuple_kind:
        if (!symtable_add_def(st, tp->v.TypeVarTuple.name, DEF_TYPE_PARAM | DEF_LOCAL, LOCATION(tp)))
            VISIT_QUIT(st, 0);
        break;
    case ParamSpec_kind:
        if (!symtable_add_def(st, tp->v.ParamSpec.name, DEF_TYPE_PARAM | DEF_LOCAL, LOCATION(tp)))
            VISIT_QUIT(st, 0);
        break;
    }
    VISIT_QUIT(st, 1);
}

static int
symtable_visit_pattern(struct symtable *st, pattern_ty p)
{
    if (++st->recursion_depth > st->recursion_limit) {
        PyErr_SetString(PyExc_RecursionError,
                        "maximum recursion depth exceeded during compilation");
        VISIT_QUIT(st, 0);
    }
    switch (p->kind) {
    case MatchValue_kind:
        VISIT(st, expr, p->v.MatchValue.value);
        break;
    case MatchSingleton_kind:
        /* Nothing to do here. */
        break;
    case MatchSequence_kind:
        VISIT_SEQ(st, pattern, p->v.MatchSequence.patterns);
        break;
    case MatchStar_kind:
        if (p->v.MatchStar.name) {
            symtable_add_def(st, p->v.MatchStar.name, DEF_LOCAL, LOCATION(p));
        }
        break;
    case MatchMapping_kind:
        VISIT_SEQ(st, expr, p->v.MatchMapping.keys);
        VISIT_SEQ(st, pattern, p->v.MatchMapping.patterns);
        if (p->v.MatchMapping.rest) {
            symtable_add_def(st, p->v.MatchMapping.rest, DEF_LOCAL, LOCATION(p));
        }
        break;
    case MatchClass_kind:
        VISIT(st, expr, p->v.MatchClass.cls);
        VISIT_SEQ(st, pattern, p->v.MatchClass.patterns);
        VISIT_SEQ(st, pattern, p->v.MatchClass.kwd_patterns);
        break;
    case MatchAs_kind:
        if (p->v.MatchAs.pattern) {
            VISIT(st, pattern, p->v.MatchAs.pattern);
        }
        if (p->v.MatchAs.name) {
            symtable_add_def(st, p->v.MatchAs.name, DEF_LOCAL, LOCATION(p));
        }
        break;
    case MatchOr_kind:
        VISIT_SEQ(st, pattern, p->v.MatchOr.patterns);
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
    if (!symtable_add_def(st, id, DEF_PARAM, ST_LOCATION(st->st_cur))) {
        Py_DECREF(id);
        return 0;
    }
    Py_DECREF(id);
    return 1;
}

static int
symtable_visit_params(struct symtable *st, asdl_arg_seq *args)
{
    int i;

    if (!args)
        return -1;

    for (i = 0; i < asdl_seq_LEN(args); i++) {
        arg_ty arg = (arg_ty)asdl_seq_GET(args, i);
        if (!symtable_add_def(st, arg->arg, DEF_PARAM, LOCATION(arg)))
            return 0;
    }

    return 1;
}

static int
symtable_visit_annotation(struct symtable *st, expr_ty annotation)
{
    int future_annotations = st->st_future->ff_features & CO_FUTURE_ANNOTATIONS;
    if (future_annotations &&
        !symtable_enter_block(st, &_Py_ID(_annotation), AnnotationBlock,
                              (void *)annotation, annotation->lineno,
                              annotation->col_offset, annotation->end_lineno,
                              annotation->end_col_offset)) {
        VISIT_QUIT(st, 0);
    }
    VISIT(st, expr, annotation);
    if (future_annotations && !symtable_exit_block(st)) {
        VISIT_QUIT(st, 0);
    }
    return 1;
}

static int
symtable_visit_argannotations(struct symtable *st, asdl_arg_seq *args)
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
symtable_visit_annotations(struct symtable *st, stmt_ty o, arguments_ty a, expr_ty returns)
{
    int future_annotations = st->st_future->ff_features & CO_FUTURE_ANNOTATIONS;
    if (future_annotations &&
        !symtable_enter_block(st, &_Py_ID(_annotation), AnnotationBlock,
                              (void *)o, o->lineno, o->col_offset, o->end_lineno,
                              o->end_col_offset)) {
        VISIT_QUIT(st, 0);
    }
    if (a->posonlyargs && !symtable_visit_argannotations(st, a->posonlyargs))
        return 0;
    if (a->args && !symtable_visit_argannotations(st, a->args))
        return 0;
    if (a->vararg && a->vararg->annotation)
        VISIT(st, expr, a->vararg->annotation);
    if (a->kwarg && a->kwarg->annotation)
        VISIT(st, expr, a->kwarg->annotation);
    if (a->kwonlyargs && !symtable_visit_argannotations(st, a->kwonlyargs))
        return 0;
    if (future_annotations && !symtable_exit_block(st)) {
        VISIT_QUIT(st, 0);
    }
    if (returns && !symtable_visit_annotation(st, returns)) {
        VISIT_QUIT(st, 0);
    }
    return 1;
}

static int
symtable_visit_arguments(struct symtable *st, arguments_ty a)
{
    /* skip default arguments inside function block
       XXX should ast be different?
    */
    if (a->posonlyargs && !symtable_visit_params(st, a->posonlyargs))
        return 0;
    if (a->args && !symtable_visit_params(st, a->args))
        return 0;
    if (a->kwonlyargs && !symtable_visit_params(st, a->kwonlyargs))
        return 0;
    if (a->vararg) {
        if (!symtable_add_def(st, a->vararg->arg, DEF_PARAM, LOCATION(a->vararg)))
            return 0;
        st->st_cur->ste_varargs = 1;
    }
    if (a->kwarg) {
        if (!symtable_add_def(st, a->kwarg->arg, DEF_PARAM, LOCATION(a->kwarg)))
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
        if (!symtable_add_def(st, eh->v.ExceptHandler.name, DEF_LOCAL, LOCATION(eh)))
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
symtable_visit_match_case(struct symtable *st, match_case_ty m)
{
    VISIT(st, pattern, m->pattern);
    if (m->guard) {
        VISIT(st, expr, m->guard);
    }
    VISIT_SEQ(st, stmt, m->body);
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
        store_name = Py_NewRef(name);
    }
    if (!_PyUnicode_EqualToASCIIString(name, "*")) {
        int r = symtable_add_def(st, store_name, DEF_IMPORT, LOCATION(a));
        Py_DECREF(store_name);
        return r;
    }
    else {
        if (st->st_cur->ste_type != ModuleBlock) {
            int lineno = a->lineno;
            int col_offset = a->col_offset;
            int end_lineno = a->end_lineno;
            int end_col_offset = a->end_col_offset;
            PyErr_SetString(PyExc_SyntaxError, IMPORT_STAR_WARNING);
            PyErr_RangedSyntaxLocationObject(st->st_filename,
                                             lineno, col_offset + 1,
                                             end_lineno, end_col_offset + 1);
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
    st->st_cur->ste_comp_iter_target = 1;
    VISIT(st, expr, lc->target);
    st->st_cur->ste_comp_iter_target = 0;
    st->st_cur->ste_comp_iter_expr++;
    VISIT(st, expr, lc->iter);
    st->st_cur->ste_comp_iter_expr--;
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
symtable_handle_comprehension(struct symtable *st, expr_ty e,
                              identifier scope_name, asdl_comprehension_seq *generators,
                              expr_ty elt, expr_ty value)
{
    if (st->st_cur->ste_can_see_class_scope) {
        // gh-109118
        PyErr_Format(PyExc_SyntaxError,
                     "Cannot use comprehension in annotation scope within class scope");
        PyErr_RangedSyntaxLocationObject(st->st_filename,
                                         e->lineno,
                                         e->col_offset + 1,
                                         e->end_lineno,
                                         e->end_col_offset + 1);
        VISIT_QUIT(st, 0);
    }

    int is_generator = (e->kind == GeneratorExp_kind);
    comprehension_ty outermost = ((comprehension_ty)
                                    asdl_seq_GET(generators, 0));
    /* Outermost iterator is evaluated in current scope */
    st->st_cur->ste_comp_iter_expr++;
    VISIT(st, expr, outermost->iter);
    st->st_cur->ste_comp_iter_expr--;
    /* Create comprehension scope for the rest */
    if (!scope_name ||
        !symtable_enter_block(st, scope_name, FunctionBlock, (void *)e,
                              e->lineno, e->col_offset,
                              e->end_lineno, e->end_col_offset)) {
        return 0;
    }
    switch(e->kind) {
        case ListComp_kind:
            st->st_cur->ste_comprehension = ListComprehension;
            break;
        case SetComp_kind:
            st->st_cur->ste_comprehension = SetComprehension;
            break;
        case DictComp_kind:
            st->st_cur->ste_comprehension = DictComprehension;
            break;
        default:
            st->st_cur->ste_comprehension = GeneratorExpression;
            break;
    }
    if (outermost->is_async) {
        st->st_cur->ste_coroutine = 1;
    }

    /* Outermost iter is received as an argument */
    if (!symtable_implicit_arg(st, 0)) {
        symtable_exit_block(st);
        return 0;
    }
    /* Visit iteration variable target, and mark them as such */
    st->st_cur->ste_comp_iter_target = 1;
    VISIT(st, expr, outermost->target);
    st->st_cur->ste_comp_iter_target = 0;
    /* Visit the rest of the comprehension body */
    VISIT_SEQ(st, expr, outermost->ifs);
    VISIT_SEQ_TAIL(st, comprehension, generators, 1);
    if (value)
        VISIT(st, expr, value);
    VISIT(st, expr, elt);
    st->st_cur->ste_generator = is_generator;
    int is_async = st->st_cur->ste_coroutine && !is_generator;
    if (!symtable_exit_block(st)) {
        return 0;
    }
    if (is_async) {
        st->st_cur->ste_coroutine = 1;
    }
    return 1;
}

static int
symtable_visit_genexp(struct symtable *st, expr_ty e)
{
    return symtable_handle_comprehension(st, e, &_Py_ID(genexpr),
                                         e->v.GeneratorExp.generators,
                                         e->v.GeneratorExp.elt, NULL);
}

static int
symtable_visit_listcomp(struct symtable *st, expr_ty e)
{
    return symtable_handle_comprehension(st, e, &_Py_ID(listcomp),
                                         e->v.ListComp.generators,
                                         e->v.ListComp.elt, NULL);
}

static int
symtable_visit_setcomp(struct symtable *st, expr_ty e)
{
    return symtable_handle_comprehension(st, e, &_Py_ID(setcomp),
                                         e->v.SetComp.generators,
                                         e->v.SetComp.elt, NULL);
}

static int
symtable_visit_dictcomp(struct symtable *st, expr_ty e)
{
    return symtable_handle_comprehension(st, e, &_Py_ID(dictcomp),
                                         e->v.DictComp.generators,
                                         e->v.DictComp.key,
                                         e->v.DictComp.value);
}

static int
symtable_raise_if_annotation_block(struct symtable *st, const char *name, expr_ty e)
{
    enum _block_type type = st->st_cur->ste_type;
    if (type == AnnotationBlock)
        PyErr_Format(PyExc_SyntaxError, ANNOTATION_NOT_ALLOWED, name);
    else if (type == TypeVarBoundBlock)
        PyErr_Format(PyExc_SyntaxError, TYPEVAR_BOUND_NOT_ALLOWED, name);
    else if (type == TypeAliasBlock)
        PyErr_Format(PyExc_SyntaxError, TYPEALIAS_NOT_ALLOWED, name);
    else if (type == TypeParamBlock)
        PyErr_Format(PyExc_SyntaxError, TYPEPARAM_NOT_ALLOWED, name);
    else
        return 1;

    PyErr_RangedSyntaxLocationObject(st->st_filename,
                                     e->lineno,
                                     e->col_offset + 1,
                                     e->end_lineno,
                                     e->end_col_offset + 1);
    return 0;
}

static int
symtable_raise_if_comprehension_block(struct symtable *st, expr_ty e) {
    _Py_comprehension_ty type = st->st_cur->ste_comprehension;
    PyErr_SetString(PyExc_SyntaxError,
            (type == ListComprehension) ? "'yield' inside list comprehension" :
            (type == SetComprehension) ? "'yield' inside set comprehension" :
            (type == DictComprehension) ? "'yield' inside dict comprehension" :
            "'yield' inside generator expression");
    PyErr_RangedSyntaxLocationObject(st->st_filename,
                                     e->lineno, e->col_offset + 1,
                                     e->end_lineno, e->end_col_offset + 1);
    VISIT_QUIT(st, 0);
}

struct symtable *
_Py_SymtableStringObjectFlags(const char *str, PyObject *filename,
                              int start, PyCompilerFlags *flags)
{
    struct symtable *st;
    mod_ty mod;
    PyArena *arena;

    arena = _PyArena_New();
    if (arena == NULL)
        return NULL;

    mod = _PyParser_ASTFromString(str, filename, start, flags, arena);
    if (mod == NULL) {
        _PyArena_Free(arena);
        return NULL;
    }
    PyFutureFeatures future;
    if (!_PyFuture_FromAST(mod, filename, &future)) {
        _PyArena_Free(arena);
        return NULL;
    }
    future.ff_features |= flags->cf_flags;
    st = _PySymtable_Build(mod, filename, &future);
    _PyArena_Free(arena);
    return st;
}

PyObject *
_Py_Mangle(PyObject *privateobj, PyObject *ident)
{
    /* Name mangling: __private becomes _classname__private.
       This is independent from how the name is used. */
    if (privateobj == NULL || !PyUnicode_Check(privateobj) ||
        PyUnicode_READ_CHAR(ident, 0) != '_' ||
        PyUnicode_READ_CHAR(ident, 1) != '_') {
        return Py_NewRef(ident);
    }
    size_t nlen = PyUnicode_GET_LENGTH(ident);
    size_t plen = PyUnicode_GET_LENGTH(privateobj);
    /* Don't mangle __id__ or names with dots.

       The only time a name with a dot can occur is when
       we are compiling an import statement that has a
       package name.

       TODO(jhylton): Decide whether we want to support
       mangling of the module name, e.g. __M.X.
    */
    if ((PyUnicode_READ_CHAR(ident, nlen-1) == '_' &&
         PyUnicode_READ_CHAR(ident, nlen-2) == '_') ||
        PyUnicode_FindChar(ident, '.', 0, nlen, 1) != -1) {
        return Py_NewRef(ident); /* Don't mangle __whatever__ */
    }
    /* Strip leading underscores from class name */
    size_t ipriv = 0;
    while (PyUnicode_READ_CHAR(privateobj, ipriv) == '_') {
        ipriv++;
    }
    if (ipriv == plen) {
        return Py_NewRef(ident); /* Don't mangle if class is just underscores */
    }
    plen -= ipriv;

    if (plen + nlen >= PY_SSIZE_T_MAX - 1) {
        PyErr_SetString(PyExc_OverflowError,
                        "private identifier too large to be mangled");
        return NULL;
    }

    Py_UCS4 maxchar = PyUnicode_MAX_CHAR_VALUE(ident);
    if (PyUnicode_MAX_CHAR_VALUE(privateobj) > maxchar) {
        maxchar = PyUnicode_MAX_CHAR_VALUE(privateobj);
    }

    PyObject *result = PyUnicode_New(1 + nlen + plen, maxchar);
    if (!result) {
        return NULL;
    }
    /* ident = "_" + priv[ipriv:] + ident # i.e. 1+plen+nlen bytes */
    PyUnicode_WRITE(PyUnicode_KIND(result), PyUnicode_DATA(result), 0, '_');
    if (PyUnicode_CopyCharacters(result, 1, privateobj, ipriv, plen) < 0) {
        Py_DECREF(result);
        return NULL;
    }
    if (PyUnicode_CopyCharacters(result, plen+1, ident, 0, nlen) < 0) {
        Py_DECREF(result);
        return NULL;
    }
    assert(_PyUnicode_CheckConsistency(result, 1));
    return result;
}
