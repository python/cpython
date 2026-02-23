/*
 * This file compiles an abstract syntax tree (AST) into Python bytecode.
 *
 * The primary entry point is _PyAST_Compile(), which returns a
 * PyCodeObject.  The compiler makes several passes to build the code
 * object:
 *   1. Checks for future statements.  See future.c
 *   2. Builds a symbol table.  See symtable.c.
 *   3. Generate an instruction sequence. See compiler_mod() in this file, which
 *      calls functions from codegen.c.
 *   4. Generate a control flow graph and run optimizations on it.  See flowgraph.c.
 *   5. Assemble the basic blocks into final code.  See optimize_and_assemble() in
 *      this file, and assembler.c.
 *
 */

#include "Python.h"
#include "pycore_ast.h"           // PyAST_Check()
#include "pycore_code.h"
#include "pycore_compile.h"
#include "pycore_flowgraph.h"     // _PyCfg_FromInstructionSequence()
#include "pycore_pystate.h"       // _Py_GetConfig()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_setobject.h"     // _PySet_NextEntry()
#include "pycore_stats.h"
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()

#include "cpython/code.h"

#include <stdbool.h>


#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    do {                    \
        if ((X) == -1) {    \
            return ERROR;   \
        }                   \
    } while (0)

typedef _Py_SourceLocation location;
typedef _PyJumpTargetLabel jump_target_label;
typedef _PyInstructionSequence instr_sequence;
typedef struct _PyCfgBuilder cfg_builder;
typedef _PyCompile_FBlockInfo fblockinfo;
typedef enum _PyCompile_FBlockType fblocktype;

/* The following items change on entry and exit of code blocks.
   They must be saved and restored when returning to a block.
*/
struct compiler_unit {
    PySTEntryObject *u_ste;

    int u_scope_type;

    PyObject *u_private;            /* for private name mangling */
    PyObject *u_static_attributes;  /* for class: attributes accessed via self.X */
    PyObject *u_deferred_annotations; /* AnnAssign nodes deferred to the end of compilation */
    PyObject *u_conditional_annotation_indices;  /* indices of annotations that are conditionally executed (or -1 for unconditional annotations) */
    long u_next_conditional_annotation_index;  /* index of the next conditional annotation */

    instr_sequence *u_instr_sequence; /* codegen output */
    instr_sequence *u_stashed_instr_sequence; /* temporarily stashed parent instruction sequence */

    int u_nfblocks;
    int u_in_inlined_comp;
    int u_in_conditional_block;

    _PyCompile_FBlockInfo u_fblock[CO_MAXBLOCKS];

    _PyCompile_CodeUnitMetadata u_metadata;
};

/* This struct captures the global state of a compilation.

The u pointer points to the current compilation unit, while units
for enclosing blocks are stored in c_stack.     The u and c_stack are
managed by _PyCompile_EnterScope() and _PyCompile_ExitScope().

Note that we don't track recursion levels during compilation - the
task of detecting and rejecting excessive levels of nesting is
handled by the symbol analysis pass.

*/

typedef struct _PyCompiler {
    PyObject *c_filename;
    struct symtable *c_st;
    _PyFutureFeatures c_future;  /* module's __future__ */
    PyCompilerFlags c_flags;

    int c_optimize;              /* optimization level */
    int c_interactive;           /* true if in interactive mode */
    PyObject *c_const_cache;     /* Python dict holding all constants,
                                    including names tuple */
    struct compiler_unit *u;     /* compiler state for current block */
    PyObject *c_stack;           /* Python list holding compiler_unit ptrs */

    bool c_save_nested_seqs;     /* if true, construct recursive instruction sequences
                                  * (including instructions for nested code objects)
                                  */
    int c_disable_warning;
    PyObject *c_module;
} compiler;

static int
compiler_setup(compiler *c, mod_ty mod, PyObject *filename,
               PyCompilerFlags *flags, int optimize, PyArena *arena,
               PyObject *module)
{
    PyCompilerFlags local_flags = _PyCompilerFlags_INIT;

    c->c_const_cache = PyDict_New();
    if (!c->c_const_cache) {
        return ERROR;
    }

    c->c_stack = PyList_New(0);
    if (!c->c_stack) {
        return ERROR;
    }

    c->c_filename = Py_NewRef(filename);
    if (!_PyFuture_FromAST(mod, filename, &c->c_future)) {
        return ERROR;
    }
    c->c_module = Py_XNewRef(module);
    if (!flags) {
        flags = &local_flags;
    }
    int merged = c->c_future.ff_features | flags->cf_flags;
    c->c_future.ff_features = merged;
    flags->cf_flags = merged;
    c->c_flags = *flags;
    c->c_optimize = (optimize == -1) ? _Py_GetConfig()->optimization_level : optimize;
    c->c_save_nested_seqs = false;

    if (!_PyAST_Preprocess(mod, arena, filename, c->c_optimize, merged,
                           0, 1, module))
    {
        return ERROR;
    }
    c->c_st = _PySymtable_Build(mod, filename, &c->c_future);
    if (c->c_st == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError, "no symtable");
        }
        return ERROR;
    }
    return SUCCESS;
}

static void
compiler_free(compiler *c)
{
    if (c->c_st) {
        _PySymtable_Free(c->c_st);
    }
    Py_XDECREF(c->c_filename);
    Py_XDECREF(c->c_module);
    Py_XDECREF(c->c_const_cache);
    Py_XDECREF(c->c_stack);
    PyMem_Free(c);
}

static compiler*
new_compiler(mod_ty mod, PyObject *filename, PyCompilerFlags *pflags,
             int optimize, PyArena *arena, PyObject *module)
{
    compiler *c = PyMem_Calloc(1, sizeof(compiler));
    if (c == NULL) {
        return NULL;
    }
    if (compiler_setup(c, mod, filename, pflags, optimize, arena, module) < 0) {
        compiler_free(c);
        return NULL;
    }
    return c;
}

static void
compiler_unit_free(struct compiler_unit *u)
{
    Py_CLEAR(u->u_instr_sequence);
    Py_CLEAR(u->u_stashed_instr_sequence);
    Py_CLEAR(u->u_ste);
    Py_CLEAR(u->u_metadata.u_name);
    Py_CLEAR(u->u_metadata.u_qualname);
    Py_CLEAR(u->u_metadata.u_consts);
    Py_CLEAR(u->u_metadata.u_names);
    Py_CLEAR(u->u_metadata.u_varnames);
    Py_CLEAR(u->u_metadata.u_freevars);
    Py_CLEAR(u->u_metadata.u_cellvars);
    Py_CLEAR(u->u_metadata.u_fasthidden);
    Py_CLEAR(u->u_private);
    Py_CLEAR(u->u_static_attributes);
    Py_CLEAR(u->u_deferred_annotations);
    Py_CLEAR(u->u_conditional_annotation_indices);
    PyMem_Free(u);
}

#define CAPSULE_NAME "compile.c compiler unit"

int
_PyCompile_MaybeAddStaticAttributeToClass(compiler *c, expr_ty e)
{
    assert(e->kind == Attribute_kind);
    expr_ty attr_value = e->v.Attribute.value;
    if (attr_value->kind != Name_kind ||
        e->v.Attribute.ctx != Store ||
        !_PyUnicode_EqualToASCIIString(attr_value->v.Name.id, "self"))
    {
        return SUCCESS;
    }
    Py_ssize_t stack_size = PyList_GET_SIZE(c->c_stack);
    for (Py_ssize_t i = stack_size - 1; i >= 0; i--) {
        PyObject *capsule = PyList_GET_ITEM(c->c_stack, i);
        struct compiler_unit *u = (struct compiler_unit *)PyCapsule_GetPointer(
                                                              capsule, CAPSULE_NAME);
        assert(u);
        if (u->u_scope_type == COMPILE_SCOPE_CLASS) {
            assert(u->u_static_attributes);
            RETURN_IF_ERROR(PySet_Add(u->u_static_attributes, e->v.Attribute.attr));
            break;
        }
    }
    return SUCCESS;
}

static int
compiler_set_qualname(compiler *c)
{
    Py_ssize_t stack_size;
    struct compiler_unit *u = c->u;
    PyObject *name, *base;

    base = NULL;
    stack_size = PyList_GET_SIZE(c->c_stack);
    assert(stack_size >= 1);
    if (stack_size > 1) {
        int scope, force_global = 0;
        struct compiler_unit *parent;
        PyObject *mangled, *capsule;

        capsule = PyList_GET_ITEM(c->c_stack, stack_size - 1);
        parent = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
        assert(parent);
        if (parent->u_scope_type == COMPILE_SCOPE_ANNOTATIONS) {
            /* The parent is an annotation scope, so we need to
               look at the grandparent. */
            if (stack_size == 2) {
                // If we're immediately within the module, we can skip
                // the rest and just set the qualname to be the same as name.
                u->u_metadata.u_qualname = Py_NewRef(u->u_metadata.u_name);
                return SUCCESS;
            }
            capsule = PyList_GET_ITEM(c->c_stack, stack_size - 2);
            parent = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
            assert(parent);
        }

        if (u->u_scope_type == COMPILE_SCOPE_FUNCTION
            || u->u_scope_type == COMPILE_SCOPE_ASYNC_FUNCTION
            || u->u_scope_type == COMPILE_SCOPE_CLASS) {
            assert(u->u_metadata.u_name);
            mangled = _Py_Mangle(parent->u_private, u->u_metadata.u_name);
            if (!mangled) {
                return ERROR;
            }

            scope = _PyST_GetScope(parent->u_ste, mangled);
            Py_DECREF(mangled);
            RETURN_IF_ERROR(scope);
            assert(scope != GLOBAL_IMPLICIT);
            if (scope == GLOBAL_EXPLICIT)
                force_global = 1;
        }

        if (!force_global) {
            if (parent->u_scope_type == COMPILE_SCOPE_FUNCTION
                || parent->u_scope_type == COMPILE_SCOPE_ASYNC_FUNCTION
                || parent->u_scope_type == COMPILE_SCOPE_LAMBDA)
            {
                _Py_DECLARE_STR(dot_locals, ".<locals>");
                base = PyUnicode_Concat(parent->u_metadata.u_qualname,
                                        &_Py_STR(dot_locals));
                if (base == NULL) {
                    return ERROR;
                }
            }
            else {
                base = Py_NewRef(parent->u_metadata.u_qualname);
            }
        }
    }

    if (base != NULL) {
        name = PyUnicode_Concat(base, _Py_LATIN1_CHR('.'));
        Py_DECREF(base);
        if (name == NULL) {
            return ERROR;
        }
        PyUnicode_Append(&name, u->u_metadata.u_name);
        if (name == NULL) {
            return ERROR;
        }
    }
    else {
        name = Py_NewRef(u->u_metadata.u_name);
    }
    u->u_metadata.u_qualname = name;

    return SUCCESS;
}

/* Merge const *o* and return constant key object.
 * If recursive, insert all elements if o is a tuple or frozen set.
 */
static PyObject*
const_cache_insert(PyObject *const_cache, PyObject *o, bool recursive)
{
    assert(PyDict_CheckExact(const_cache));
    // None and Ellipsis are immortal objects, and key is the singleton.
    // No need to merge object and key.
    if (o == Py_None || o == Py_Ellipsis) {
        return o;
    }

    PyObject *key = _PyCode_ConstantKey(o);
    if (key == NULL) {
        return NULL;
    }

    PyObject *t;
    int res = PyDict_SetDefaultRef(const_cache, key, key, &t);
    if (res != 0) {
        // o was not inserted into const_cache. t is either the existing value
        // or NULL (on error).
        Py_DECREF(key);
        return t;
    }
    Py_DECREF(t);

    if (!recursive) {
        return key;
    }

    // We registered o in const_cache.
    // When o is a tuple or frozenset, we want to merge its
    // items too.
    if (PyTuple_CheckExact(o)) {
        Py_ssize_t len = PyTuple_GET_SIZE(o);
        for (Py_ssize_t i = 0; i < len; i++) {
            PyObject *item = PyTuple_GET_ITEM(o, i);
            PyObject *u = const_cache_insert(const_cache, item, recursive);
            if (u == NULL) {
                Py_DECREF(key);
                return NULL;
            }

            // See _PyCode_ConstantKey()
            PyObject *v;  // borrowed
            if (PyTuple_CheckExact(u)) {
                v = PyTuple_GET_ITEM(u, 1);
            }
            else {
                v = u;
            }
            if (v != item) {
                PyTuple_SET_ITEM(o, i, Py_NewRef(v));
                Py_DECREF(item);
            }

            Py_DECREF(u);
        }
    }
    else if (PyFrozenSet_CheckExact(o)) {
        // *key* is tuple. And its first item is frozenset of
        // constant keys.
        // See _PyCode_ConstantKey() for detail.
        assert(PyTuple_CheckExact(key));
        assert(PyTuple_GET_SIZE(key) == 2);

        Py_ssize_t len = PySet_GET_SIZE(o);
        if (len == 0) {  // empty frozenset should not be re-created.
            return key;
        }
        PyObject *tuple = PyTuple_New(len);
        if (tuple == NULL) {
            Py_DECREF(key);
            return NULL;
        }
        Py_ssize_t i = 0, pos = 0;
        PyObject *item;
        Py_hash_t hash;
        while (_PySet_NextEntry(o, &pos, &item, &hash)) {
            PyObject *k = const_cache_insert(const_cache, item, recursive);
            if (k == NULL) {
                Py_DECREF(tuple);
                Py_DECREF(key);
                return NULL;
            }
            PyObject *u;
            if (PyTuple_CheckExact(k)) {
                u = Py_NewRef(PyTuple_GET_ITEM(k, 1));
                Py_DECREF(k);
            }
            else {
                u = k;
            }
            PyTuple_SET_ITEM(tuple, i, u);  // Steals reference of u.
            i++;
        }

        // Instead of rewriting o, we create new frozenset and embed in the
        // key tuple.  Caller should get merged frozenset from the key tuple.
        PyObject *new = PyFrozenSet_New(tuple);
        Py_DECREF(tuple);
        if (new == NULL) {
            Py_DECREF(key);
            return NULL;
        }
        assert(PyTuple_GET_ITEM(key, 1) == o);
        Py_DECREF(o);
        PyTuple_SET_ITEM(key, 1, new);
    }

    return key;
}

static PyObject*
merge_consts_recursive(PyObject *const_cache, PyObject *o)
{
    return const_cache_insert(const_cache, o, true);
}

Py_ssize_t
_PyCompile_DictAddObj(PyObject *dict, PyObject *o)
{
    PyObject *v;
    Py_ssize_t arg;

    if (PyDict_GetItemRef(dict, o, &v) < 0) {
        return ERROR;
    }
    if (!v) {
        arg = PyDict_GET_SIZE(dict);
        v = PyLong_FromSsize_t(arg);
        if (!v) {
            return ERROR;
        }
        if (PyDict_SetItem(dict, o, v) < 0) {
            Py_DECREF(v);
            return ERROR;
        }
    }
    else
        arg = PyLong_AsLong(v);
    Py_DECREF(v);
    return arg;
}

Py_ssize_t
_PyCompile_AddConst(compiler *c, PyObject *o)
{
    PyObject *key = merge_consts_recursive(c->c_const_cache, o);
    if (key == NULL) {
        return ERROR;
    }

    Py_ssize_t arg = _PyCompile_DictAddObj(c->u->u_metadata.u_consts, key);
    Py_DECREF(key);
    return arg;
}

static PyObject *
list2dict(PyObject *list)
{
    Py_ssize_t i, n;
    PyObject *v, *k;
    PyObject *dict = PyDict_New();
    if (!dict) return NULL;

    n = PyList_Size(list);
    for (i = 0; i < n; i++) {
        v = PyLong_FromSsize_t(i);
        if (!v) {
            Py_DECREF(dict);
            return NULL;
        }
        k = PyList_GET_ITEM(list, i);
        if (PyDict_SetItem(dict, k, v) < 0) {
            Py_DECREF(v);
            Py_DECREF(dict);
            return NULL;
        }
        Py_DECREF(v);
    }
    return dict;
}

/* Return new dict containing names from src that match scope(s).

src is a symbol table dictionary.  If the scope of a name matches
either scope_type or flag is set, insert it into the new dict.  The
values are integers, starting at offset and increasing by one for
each key.
*/

static PyObject *
dictbytype(PyObject *src, int scope_type, int flag, Py_ssize_t offset)
{
    Py_ssize_t i = offset, num_keys, key_i;
    PyObject *k, *v, *dest = PyDict_New();
    PyObject *sorted_keys;

    assert(offset >= 0);
    if (dest == NULL)
        return NULL;

    /* Sort the keys so that we have a deterministic order on the indexes
       saved in the returned dictionary.  These indexes are used as indexes
       into the free and cell var storage.  Therefore if they aren't
       deterministic, then the generated bytecode is not deterministic.
    */
    sorted_keys = PyDict_Keys(src);
    if (sorted_keys == NULL) {
        Py_DECREF(dest);
        return NULL;
    }
    if (PyList_Sort(sorted_keys) != 0) {
        Py_DECREF(sorted_keys);
        Py_DECREF(dest);
        return NULL;
    }
    num_keys = PyList_GET_SIZE(sorted_keys);

    for (key_i = 0; key_i < num_keys; key_i++) {
        k = PyList_GET_ITEM(sorted_keys, key_i);
        v = PyDict_GetItemWithError(src, k);
        if (!v) {
            if (!PyErr_Occurred()) {
                PyErr_SetObject(PyExc_KeyError, k);
            }
            Py_DECREF(sorted_keys);
            Py_DECREF(dest);
            return NULL;
        }
        long vi = PyLong_AsLong(v);
        if (vi == -1 && PyErr_Occurred()) {
            Py_DECREF(sorted_keys);
            Py_DECREF(dest);
            return NULL;
        }
        if (SYMBOL_TO_SCOPE(vi) == scope_type || vi & flag) {
            PyObject *item = PyLong_FromSsize_t(i);
            if (item == NULL) {
                Py_DECREF(sorted_keys);
                Py_DECREF(dest);
                return NULL;
            }
            i++;
            if (PyDict_SetItem(dest, k, item) < 0) {
                Py_DECREF(sorted_keys);
                Py_DECREF(item);
                Py_DECREF(dest);
                return NULL;
            }
            Py_DECREF(item);
        }
    }
    Py_DECREF(sorted_keys);
    return dest;
}

int
_PyCompile_EnterScope(compiler *c, identifier name, int scope_type,
                       void *key, int lineno, PyObject *private,
                      _PyCompile_CodeUnitMetadata *umd)
{
    struct compiler_unit *u;
    u = (struct compiler_unit *)PyMem_Calloc(1, sizeof(struct compiler_unit));
    if (!u) {
        PyErr_NoMemory();
        return ERROR;
    }
    u->u_scope_type = scope_type;
    if (umd != NULL) {
        u->u_metadata = *umd;
    }
    else {
        u->u_metadata.u_argcount = 0;
        u->u_metadata.u_posonlyargcount = 0;
        u->u_metadata.u_kwonlyargcount = 0;
    }
    u->u_ste = _PySymtable_Lookup(c->c_st, key);
    if (!u->u_ste) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_name = Py_NewRef(name);
    u->u_metadata.u_varnames = list2dict(u->u_ste->ste_varnames);
    if (!u->u_metadata.u_varnames) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_cellvars = dictbytype(u->u_ste->ste_symbols, CELL, DEF_COMP_CELL, 0);
    if (!u->u_metadata.u_cellvars) {
        compiler_unit_free(u);
        return ERROR;
    }
    if (u->u_ste->ste_needs_class_closure) {
        /* Cook up an implicit __class__ cell. */
        Py_ssize_t res;
        assert(u->u_scope_type == COMPILE_SCOPE_CLASS);
        res = _PyCompile_DictAddObj(u->u_metadata.u_cellvars, &_Py_ID(__class__));
        if (res < 0) {
            compiler_unit_free(u);
            return ERROR;
        }
    }
    if (u->u_ste->ste_needs_classdict) {
        /* Cook up an implicit __classdict__ cell. */
        Py_ssize_t res;
        assert(u->u_scope_type == COMPILE_SCOPE_CLASS);
        res = _PyCompile_DictAddObj(u->u_metadata.u_cellvars, &_Py_ID(__classdict__));
        if (res < 0) {
            compiler_unit_free(u);
            return ERROR;
        }
    }
    if (u->u_ste->ste_has_conditional_annotations) {
        /* Cook up an implicit __conditional_annotations__ cell */
        Py_ssize_t res;
        assert(u->u_scope_type == COMPILE_SCOPE_CLASS || u->u_scope_type == COMPILE_SCOPE_MODULE);
        res = _PyCompile_DictAddObj(u->u_metadata.u_cellvars, &_Py_ID(__conditional_annotations__));
        if (res < 0) {
            compiler_unit_free(u);
            return ERROR;
        }
    }

    u->u_metadata.u_freevars = dictbytype(u->u_ste->ste_symbols, FREE, DEF_FREE_CLASS,
                               PyDict_GET_SIZE(u->u_metadata.u_cellvars));
    if (!u->u_metadata.u_freevars) {
        compiler_unit_free(u);
        return ERROR;
    }

    u->u_metadata.u_fasthidden = PyDict_New();
    if (!u->u_metadata.u_fasthidden) {
        compiler_unit_free(u);
        return ERROR;
    }

    u->u_nfblocks = 0;
    u->u_in_inlined_comp = 0;
    u->u_metadata.u_firstlineno = lineno;
    u->u_metadata.u_consts = PyDict_New();
    if (!u->u_metadata.u_consts) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_names = PyDict_New();
    if (!u->u_metadata.u_names) {
        compiler_unit_free(u);
        return ERROR;
    }

    u->u_deferred_annotations = NULL;
    u->u_conditional_annotation_indices = NULL;
    u->u_next_conditional_annotation_index = 0;
    if (scope_type == COMPILE_SCOPE_CLASS) {
        u->u_static_attributes = PySet_New(0);
        if (!u->u_static_attributes) {
            compiler_unit_free(u);
            return ERROR;
        }
    }
    else {
        u->u_static_attributes = NULL;
    }

    u->u_instr_sequence = (instr_sequence*)_PyInstructionSequence_New();
    if (!u->u_instr_sequence) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_stashed_instr_sequence = NULL;

    /* Push the old compiler_unit on the stack. */
    if (c->u) {
        PyObject *capsule = PyCapsule_New(c->u, CAPSULE_NAME, NULL);
        if (!capsule || PyList_Append(c->c_stack, capsule) < 0) {
            Py_XDECREF(capsule);
            compiler_unit_free(u);
            return ERROR;
        }
        Py_DECREF(capsule);
        if (private == NULL) {
            private = c->u->u_private;
        }
    }

    u->u_private = Py_XNewRef(private);

    c->u = u;
    if (scope_type != COMPILE_SCOPE_MODULE) {
        RETURN_IF_ERROR(compiler_set_qualname(c));
    }
    return SUCCESS;
}

void
_PyCompile_ExitScope(compiler *c)
{
    // Don't call PySequence_DelItem() with an exception raised
    PyObject *exc = PyErr_GetRaisedException();

    instr_sequence *nested_seq = NULL;
    if (c->c_save_nested_seqs) {
        nested_seq = c->u->u_instr_sequence;
        Py_INCREF(nested_seq);
    }
    compiler_unit_free(c->u);
    /* Restore c->u to the parent unit. */
    Py_ssize_t n = PyList_GET_SIZE(c->c_stack) - 1;
    if (n >= 0) {
        PyObject *capsule = PyList_GET_ITEM(c->c_stack, n);
        c->u = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
        assert(c->u);
        /* we are deleting from a list so this really shouldn't fail */
        if (PySequence_DelItem(c->c_stack, n) < 0) {
            PyErr_FormatUnraisable("Exception ignored while removing "
                                   "the last compiler stack item");
        }
        if (nested_seq != NULL) {
            if (_PyInstructionSequence_AddNested(c->u->u_instr_sequence, nested_seq) < 0) {
                PyErr_FormatUnraisable("Exception ignored while appending "
                                       "nested instruction sequence");
            }
        }
    }
    else {
        c->u = NULL;
    }
    Py_XDECREF(nested_seq);

    PyErr_SetRaisedException(exc);
}

/*
 * Frame block handling functions
 */

int
_PyCompile_PushFBlock(compiler *c, location loc,
                     fblocktype t, jump_target_label block_label,
                     jump_target_label exit, void *datum)
{
    fblockinfo *f;
    if (c->u->u_nfblocks >= CO_MAXBLOCKS) {
        return _PyCompile_Error(c, loc, "too many statically nested blocks");
    }
    f = &c->u->u_fblock[c->u->u_nfblocks++];
    f->fb_type = t;
    f->fb_block = block_label;
    f->fb_loc = loc;
    f->fb_exit = exit;
    f->fb_datum = datum;
    if (t == COMPILE_FBLOCK_FINALLY_END) {
        c->c_disable_warning++;
    }
    return SUCCESS;
}

void
_PyCompile_PopFBlock(compiler *c, fblocktype t, jump_target_label block_label)
{
    struct compiler_unit *u = c->u;
    assert(u->u_nfblocks > 0);
    u->u_nfblocks--;
    assert(u->u_fblock[u->u_nfblocks].fb_type == t);
    assert(SAME_JUMP_TARGET_LABEL(u->u_fblock[u->u_nfblocks].fb_block, block_label));
    if (t == COMPILE_FBLOCK_FINALLY_END) {
        c->c_disable_warning--;
    }
}

fblockinfo *
_PyCompile_TopFBlock(compiler *c)
{
    if (c->u->u_nfblocks == 0) {
        return NULL;
    }
    return &c->u->u_fblock[c->u->u_nfblocks - 1];
}

bool
_PyCompile_InExceptionHandler(compiler *c)
{
    for (Py_ssize_t i = 0; i < c->u->u_nfblocks; i++) {
        fblockinfo *block = &c->u->u_fblock[i];
        switch (block->fb_type) {
            case COMPILE_FBLOCK_TRY_EXCEPT:
            case COMPILE_FBLOCK_FINALLY_TRY:
            case COMPILE_FBLOCK_FINALLY_END:
            case COMPILE_FBLOCK_EXCEPTION_HANDLER:
            case COMPILE_FBLOCK_EXCEPTION_GROUP_HANDLER:
            case COMPILE_FBLOCK_HANDLER_CLEANUP:
                return true;
            default:
                break;
        }
    }
    return false;
}

void
_PyCompile_DeferredAnnotations(compiler *c,
                               PyObject **deferred_annotations,
                               PyObject **conditional_annotation_indices)
{
    *deferred_annotations = Py_XNewRef(c->u->u_deferred_annotations);
    *conditional_annotation_indices = Py_XNewRef(c->u->u_conditional_annotation_indices);
}

static location
start_location(asdl_stmt_seq *stmts)
{
    if (asdl_seq_LEN(stmts) > 0) {
        /* Set current line number to the line number of first statement.
         * This way line number for SETUP_ANNOTATIONS will always
         * coincide with the line number of first "real" statement in module.
         * If body is empty, then lineno will be set later in the assembly stage.
         */
        stmt_ty st = (stmt_ty)asdl_seq_GET(stmts, 0);
        return SRC_LOCATION_FROM_AST(st);
    }
    return (const _Py_SourceLocation){1, 1, 0, 0};
}

static int
compiler_codegen(compiler *c, mod_ty mod)
{
    RETURN_IF_ERROR(_PyCodegen_EnterAnonymousScope(c, mod));
    assert(c->u->u_scope_type == COMPILE_SCOPE_MODULE);
    switch (mod->kind) {
    case Module_kind: {
        asdl_stmt_seq *stmts = mod->v.Module.body;
        RETURN_IF_ERROR(_PyCodegen_Module(c, start_location(stmts), stmts, false));
        break;
    }
    case Interactive_kind: {
        c->c_interactive = 1;
        asdl_stmt_seq *stmts = mod->v.Interactive.body;
        RETURN_IF_ERROR(_PyCodegen_Module(c, start_location(stmts), stmts, true));
        break;
    }
    case Expression_kind: {
        RETURN_IF_ERROR(_PyCodegen_Expression(c, mod->v.Expression.body));
        break;
    }
    default: {
        PyErr_Format(PyExc_SystemError,
                     "module kind %d should not be possible",
                     mod->kind);
        return ERROR;
    }}
    return SUCCESS;
}

static PyCodeObject *
compiler_mod(compiler *c, mod_ty mod)
{
    PyCodeObject *co = NULL;
    int addNone = mod->kind != Expression_kind;
    if (compiler_codegen(c, mod) < 0) {
        goto finally;
    }
    co = _PyCompile_OptimizeAndAssemble(c, addNone);
finally:
    _PyCompile_ExitScope(c);
    return co;
}

int
_PyCompile_GetRefType(compiler *c, PyObject *name)
{
    if (c->u->u_scope_type == COMPILE_SCOPE_CLASS &&
        (_PyUnicode_EqualToASCIIString(name, "__class__") ||
         _PyUnicode_EqualToASCIIString(name, "__classdict__") ||
         _PyUnicode_EqualToASCIIString(name, "__conditional_annotations__"))) {
        return CELL;
    }
    PySTEntryObject *ste = c->u->u_ste;
    int scope = _PyST_GetScope(ste, name);
    if (scope == 0) {
        PyErr_Format(PyExc_SystemError,
                     "_PyST_GetScope(name=%R) failed: "
                     "unknown scope in unit %S (%R); "
                     "symbols: %R; locals: %R; "
                     "globals: %R",
                     name,
                     c->u->u_metadata.u_name, ste->ste_id,
                     ste->ste_symbols, c->u->u_metadata.u_varnames,
                     c->u->u_metadata.u_names);
        return ERROR;
    }
    return scope;
}

static int
dict_lookup_arg(PyObject *dict, PyObject *name)
{
    PyObject *v = PyDict_GetItemWithError(dict, name);
    if (v == NULL) {
        return ERROR;
    }
    return PyLong_AsLong(v);
}

int
_PyCompile_LookupCellvar(compiler *c, PyObject *name)
{
    assert(c->u->u_metadata.u_cellvars);
    return dict_lookup_arg(c->u->u_metadata.u_cellvars, name);
}

int
_PyCompile_LookupArg(compiler *c, PyCodeObject *co, PyObject *name)
{
    /* Special case: If a class contains a method with a
     * free variable that has the same name as a method,
     * the name will be considered free *and* local in the
     * class.  It should be handled by the closure, as
     * well as by the normal name lookup logic.
     */
    int reftype = _PyCompile_GetRefType(c, name);
    if (reftype == -1) {
        return ERROR;
    }
    int arg;
    if (reftype == CELL) {
        arg = dict_lookup_arg(c->u->u_metadata.u_cellvars, name);
    }
    else {
        arg = dict_lookup_arg(c->u->u_metadata.u_freevars, name);
    }
    if (arg == -1 && !PyErr_Occurred()) {
        PyObject *freevars = _PyCode_GetFreevars(co);
        if (freevars == NULL) {
            PyErr_Clear();
        }
        PyErr_Format(PyExc_SystemError,
            "compiler_lookup_arg(name=%R) with reftype=%d failed in %S; "
            "freevars of code %S: %R",
            name,
            reftype,
            c->u->u_metadata.u_name,
            co->co_name,
            freevars);
        Py_XDECREF(freevars);
        return ERROR;
    }
    return arg;
}

PyObject *
_PyCompile_StaticAttributesAsTuple(compiler *c)
{
    assert(c->u->u_static_attributes);
    PyObject *static_attributes_unsorted = PySequence_List(c->u->u_static_attributes);
    if (static_attributes_unsorted == NULL) {
        return NULL;
    }
    if (PyList_Sort(static_attributes_unsorted) != 0) {
        Py_DECREF(static_attributes_unsorted);
        return NULL;
    }
    PyObject *static_attributes = PySequence_Tuple(static_attributes_unsorted);
    Py_DECREF(static_attributes_unsorted);
    return static_attributes;
}

int
_PyCompile_ResolveNameop(compiler *c, PyObject *mangled, int scope,
                          _PyCompile_optype *optype, Py_ssize_t *arg)
{
    PyObject *dict = c->u->u_metadata.u_names;
    *optype = COMPILE_OP_NAME;

    assert(scope >= 0);
    switch (scope) {
    case FREE:
        dict = c->u->u_metadata.u_freevars;
        *optype = COMPILE_OP_DEREF;
        break;
    case CELL:
        dict = c->u->u_metadata.u_cellvars;
        *optype = COMPILE_OP_DEREF;
        break;
    case LOCAL:
        if (_PyST_IsFunctionLike(c->u->u_ste)) {
            *optype = COMPILE_OP_FAST;
        }
        else {
            PyObject *item;
            RETURN_IF_ERROR(PyDict_GetItemRef(c->u->u_metadata.u_fasthidden, mangled,
                                              &item));
            if (item == Py_True) {
                *optype = COMPILE_OP_FAST;
            }
            Py_XDECREF(item);
        }
        break;
    case GLOBAL_IMPLICIT:
        if (_PyST_IsFunctionLike(c->u->u_ste)) {
            *optype = COMPILE_OP_GLOBAL;
        }
        break;
    case GLOBAL_EXPLICIT:
        *optype = COMPILE_OP_GLOBAL;
        break;
    default:
        /* scope can be 0 */
        break;
    }
    if (*optype != COMPILE_OP_FAST) {
        *arg = _PyCompile_DictAddObj(dict, mangled);
        RETURN_IF_ERROR(*arg);
    }
    return SUCCESS;
}

int
_PyCompile_TweakInlinedComprehensionScopes(compiler *c, location loc,
                                            PySTEntryObject *entry,
                                            _PyCompile_InlinedComprehensionState *state)
{
    int in_class_block = (c->u->u_ste->ste_type == ClassBlock) && !c->u->u_in_inlined_comp;
    c->u->u_in_inlined_comp++;

    PyObject *k, *v;
    Py_ssize_t pos = 0;
    while (PyDict_Next(entry->ste_symbols, &pos, &k, &v)) {
        long symbol = PyLong_AsLong(v);
        assert(symbol >= 0 || PyErr_Occurred());
        RETURN_IF_ERROR(symbol);
        long scope = SYMBOL_TO_SCOPE(symbol);

        long outsymbol = _PyST_GetSymbol(c->u->u_ste, k);
        RETURN_IF_ERROR(outsymbol);
        long outsc = SYMBOL_TO_SCOPE(outsymbol);

        // If a name has different scope inside than outside the comprehension,
        // we need to temporarily handle it with the right scope while
        // compiling the comprehension. If it's free in the comprehension
        // scope, no special handling; it should be handled the same as the
        // enclosing scope. (If it's free in outer scope and cell in inner
        // scope, we can't treat it as both cell and free in the same function,
        // but treating it as free throughout is fine; it's *_DEREF
        // either way.)
        if ((scope != outsc && scope != FREE && !(scope == CELL && outsc == FREE))
                || in_class_block) {
            if (state->temp_symbols == NULL) {
                state->temp_symbols = PyDict_New();
                if (state->temp_symbols == NULL) {
                    return ERROR;
                }
            }
            // update the symbol to the in-comprehension version and save
            // the outer version; we'll restore it after running the
            // comprehension
            if (PyDict_SetItem(c->u->u_ste->ste_symbols, k, v) < 0) {
                return ERROR;
            }
            PyObject *outv = PyLong_FromLong(outsymbol);
            if (outv == NULL) {
                return ERROR;
            }
            int res = PyDict_SetItem(state->temp_symbols, k, outv);
            Py_DECREF(outv);
            RETURN_IF_ERROR(res);
        }
        // locals handling for names bound in comprehension (DEF_LOCAL |
        // DEF_NONLOCAL occurs in assignment expression to nonlocal)
        if ((symbol & DEF_LOCAL && !(symbol & DEF_NONLOCAL)) || in_class_block) {
            if (!_PyST_IsFunctionLike(c->u->u_ste)) {
                // non-function scope: override this name to use fast locals
                PyObject *orig;
                if (PyDict_GetItemRef(c->u->u_metadata.u_fasthidden, k, &orig) < 0) {
                    return ERROR;
                }
                assert(orig == NULL || orig == Py_True || orig == Py_False);
                if (orig != Py_True) {
                    if (PyDict_SetItem(c->u->u_metadata.u_fasthidden, k, Py_True) < 0) {
                        return ERROR;
                    }
                    if (state->fast_hidden == NULL) {
                        state->fast_hidden = PySet_New(NULL);
                        if (state->fast_hidden == NULL) {
                            return ERROR;
                        }
                    }
                    if (PySet_Add(state->fast_hidden, k) < 0) {
                        return ERROR;
                    }
                }
            }
        }
    }
    return SUCCESS;
}

int
_PyCompile_RevertInlinedComprehensionScopes(compiler *c, location loc,
                                             _PyCompile_InlinedComprehensionState *state)
{
    c->u->u_in_inlined_comp--;
    if (state->temp_symbols) {
        PyObject *k, *v;
        Py_ssize_t pos = 0;
        while (PyDict_Next(state->temp_symbols, &pos, &k, &v)) {
            if (PyDict_SetItem(c->u->u_ste->ste_symbols, k, v)) {
                return ERROR;
            }
        }
        Py_CLEAR(state->temp_symbols);
    }
    if (state->fast_hidden) {
        while (PySet_Size(state->fast_hidden) > 0) {
            PyObject *k = PySet_Pop(state->fast_hidden);
            if (k == NULL) {
                return ERROR;
            }
            // we set to False instead of clearing, so we can track which names
            // were temporarily fast-locals and should use CO_FAST_HIDDEN
            if (PyDict_SetItem(c->u->u_metadata.u_fasthidden, k, Py_False)) {
                Py_DECREF(k);
                return ERROR;
            }
            Py_DECREF(k);
        }
        Py_CLEAR(state->fast_hidden);
    }
    return SUCCESS;
}

void
_PyCompile_EnterConditionalBlock(struct _PyCompiler *c)
{
    c->u->u_in_conditional_block++;
}

void
_PyCompile_LeaveConditionalBlock(struct _PyCompiler *c)
{
    assert(c->u->u_in_conditional_block > 0);
    c->u->u_in_conditional_block--;
}

int
_PyCompile_AddDeferredAnnotation(compiler *c, stmt_ty s,
                                 PyObject **conditional_annotation_index)
{
    if (c->u->u_deferred_annotations == NULL) {
        c->u->u_deferred_annotations = PyList_New(0);
        if (c->u->u_deferred_annotations == NULL) {
            return ERROR;
        }
    }
    if (c->u->u_conditional_annotation_indices == NULL) {
        c->u->u_conditional_annotation_indices = PyList_New(0);
        if (c->u->u_conditional_annotation_indices == NULL) {
            return ERROR;
        }
    }
    PyObject *ptr = PyLong_FromVoidPtr((void *)s);
    if (ptr == NULL) {
        return ERROR;
    }
    if (PyList_Append(c->u->u_deferred_annotations, ptr) < 0) {
        Py_DECREF(ptr);
        return ERROR;
    }
    Py_DECREF(ptr);
    PyObject *index;
    if (c->u->u_scope_type == COMPILE_SCOPE_MODULE || c->u->u_in_conditional_block) {
        index = PyLong_FromLong(c->u->u_next_conditional_annotation_index);
        if (index == NULL) {
            return ERROR;
        }
        *conditional_annotation_index = Py_NewRef(index);
        c->u->u_next_conditional_annotation_index++;
    }
    else {
        index = PyLong_FromLong(-1);
        if (index == NULL) {
            return ERROR;
        }
    }
    int rc = PyList_Append(c->u->u_conditional_annotation_indices, index);
    Py_DECREF(index);
    RETURN_IF_ERROR(rc);
    return SUCCESS;
}

/* Raises a SyntaxError and returns ERROR.
 * If something goes wrong, a different exception may be raised.
 */
int
_PyCompile_Error(compiler *c, location loc, const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    PyObject *msg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (msg == NULL) {
        return ERROR;
    }
    _PyErr_RaiseSyntaxError(msg, c->c_filename, loc.lineno, loc.col_offset + 1,
                            loc.end_lineno, loc.end_col_offset + 1);
    Py_DECREF(msg);
    return ERROR;
}

/* Emits a SyntaxWarning and returns 0 on success.
   If a SyntaxWarning raised as error, replaces it with a SyntaxError
   and returns -1.
*/
int
_PyCompile_Warn(compiler *c, location loc, const char *format, ...)
{
    if (c->c_disable_warning) {
        return 0;
    }
    va_list vargs;
    va_start(vargs, format);
    PyObject *msg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (msg == NULL) {
        return ERROR;
    }
    int ret = _PyErr_EmitSyntaxWarning(msg, c->c_filename, loc.lineno, loc.col_offset + 1,
                                       loc.end_lineno, loc.end_col_offset + 1,
                                       c->c_module);
    Py_DECREF(msg);
    return ret;
}

PyObject *
_PyCompile_Mangle(compiler *c, PyObject *name)
{
    return _Py_Mangle(c->u->u_private, name);
}

PyObject *
_PyCompile_MaybeMangle(compiler *c, PyObject *name)
{
    return _Py_MaybeMangle(c->u->u_private, c->u->u_ste, name);
}

instr_sequence *
_PyCompile_InstrSequence(compiler *c)
{
    return c->u->u_instr_sequence;
}

int
_PyCompile_StartAnnotationSetup(struct _PyCompiler *c)
{
    instr_sequence *new_seq = (instr_sequence *)_PyInstructionSequence_New();
    if (new_seq == NULL) {
        return ERROR;
    }
    assert(c->u->u_stashed_instr_sequence == NULL);
    c->u->u_stashed_instr_sequence = c->u->u_instr_sequence;
    c->u->u_instr_sequence = new_seq;
    return SUCCESS;
}

int
_PyCompile_EndAnnotationSetup(struct _PyCompiler *c)
{
    assert(c->u->u_stashed_instr_sequence != NULL);
    instr_sequence *parent_seq = c->u->u_stashed_instr_sequence;
    instr_sequence *anno_seq = c->u->u_instr_sequence;
    c->u->u_stashed_instr_sequence = NULL;
    c->u->u_instr_sequence = parent_seq;
    if (_PyInstructionSequence_SetAnnotationsCode(parent_seq, anno_seq) == ERROR) {
        Py_DECREF(anno_seq);
        return ERROR;
    }
    return SUCCESS;
}


int
_PyCompile_FutureFeatures(compiler *c)
{
    return c->c_future.ff_features;
}

struct symtable *
_PyCompile_Symtable(compiler *c)
{
    return c->c_st;
}

PySTEntryObject *
_PyCompile_SymtableEntry(compiler *c)
{
    return c->u->u_ste;
}

int
_PyCompile_OptimizationLevel(compiler *c)
{
    return c->c_optimize;
}

int
_PyCompile_IsInteractiveTopLevel(compiler *c)
{
    assert(c->c_stack != NULL);
    assert(PyList_CheckExact(c->c_stack));
    bool is_nested_scope = PyList_GET_SIZE(c->c_stack) > 0;
    return c->c_interactive && !is_nested_scope;
}

int
_PyCompile_ScopeType(compiler *c)
{
    return c->u->u_scope_type;
}

int
_PyCompile_IsInInlinedComp(compiler *c)
{
    return c->u->u_in_inlined_comp;
}

PyObject *
_PyCompile_Qualname(compiler *c)
{
    assert(c->u->u_metadata.u_qualname);
    return c->u->u_metadata.u_qualname;
}

_PyCompile_CodeUnitMetadata *
_PyCompile_Metadata(compiler *c)
{
    return &c->u->u_metadata;
}

// Merge *obj* with constant cache, without recursion.
int
_PyCompile_ConstCacheMergeOne(PyObject *const_cache, PyObject **obj)
{
    PyObject *key = const_cache_insert(const_cache, *obj, false);
    if (key == NULL) {
        return ERROR;
    }
    if (PyTuple_CheckExact(key)) {
        PyObject *item = PyTuple_GET_ITEM(key, 1);
        Py_SETREF(*obj, Py_NewRef(item));
        Py_DECREF(key);
    }
    else {
        Py_SETREF(*obj, key);
    }
    return SUCCESS;
}

static PyObject *
consts_dict_keys_inorder(PyObject *dict)
{
    PyObject *consts, *k, *v;
    Py_ssize_t i, pos = 0, size = PyDict_GET_SIZE(dict);

    consts = PyList_New(size);   /* PyCode_Optimize() requires a list */
    if (consts == NULL)
        return NULL;
    while (PyDict_Next(dict, &pos, &k, &v)) {
        assert(PyLong_CheckExact(v));
        i = PyLong_AsLong(v);
        /* The keys of the dictionary can be tuples wrapping a constant.
         * (see _PyCompile_DictAddObj and _PyCode_ConstantKey). In that case
         * the object we want is always second. */
        if (PyTuple_CheckExact(k)) {
            k = PyTuple_GET_ITEM(k, 1);
        }
        assert(i < size);
        assert(i >= 0);
        PyList_SET_ITEM(consts, i, Py_NewRef(k));
    }
    return consts;
}

static int
compute_code_flags(compiler *c)
{
    PySTEntryObject *ste = c->u->u_ste;
    int flags = 0;
    if (_PyST_IsFunctionLike(ste)) {
        flags |= CO_NEWLOCALS | CO_OPTIMIZED;
        if (ste->ste_nested)
            flags |= CO_NESTED;
        if (ste->ste_generator && !ste->ste_coroutine)
            flags |= CO_GENERATOR;
        if (ste->ste_generator && ste->ste_coroutine)
            flags |= CO_ASYNC_GENERATOR;
        if (ste->ste_varargs)
            flags |= CO_VARARGS;
        if (ste->ste_varkeywords)
            flags |= CO_VARKEYWORDS;
        if (ste->ste_has_docstring)
            flags |= CO_HAS_DOCSTRING;
        if (ste->ste_method)
            flags |= CO_METHOD;
    }

    if (ste->ste_coroutine && !ste->ste_generator) {
        flags |= CO_COROUTINE;
    }

    /* (Only) inherit compilerflags in PyCF_MASK */
    flags |= (c->c_flags.cf_flags & PyCF_MASK);

    return flags;
}

static PyCodeObject *
optimize_and_assemble_code_unit(struct compiler_unit *u, PyObject *const_cache,
                                int code_flags, PyObject *filename)
{
    cfg_builder *g = NULL;
    instr_sequence optimized_instrs;
    memset(&optimized_instrs, 0, sizeof(instr_sequence));

    PyCodeObject *co = NULL;
    PyObject *consts = consts_dict_keys_inorder(u->u_metadata.u_consts);
    if (consts == NULL) {
        goto error;
    }
    g = _PyCfg_FromInstructionSequence(u->u_instr_sequence);
    if (g == NULL) {
        goto error;
    }
    int nlocals = (int)PyDict_GET_SIZE(u->u_metadata.u_varnames);
    int nparams = (int)PyList_GET_SIZE(u->u_ste->ste_varnames);
    assert(u->u_metadata.u_firstlineno);

    if (_PyCfg_OptimizeCodeUnit(g, consts, const_cache, nlocals,
                                nparams, u->u_metadata.u_firstlineno) < 0) {
        goto error;
    }

    int stackdepth;
    int nlocalsplus;
    if (_PyCfg_OptimizedCfgToInstructionSequence(g, &u->u_metadata,
                                                 &stackdepth, &nlocalsplus,
                                                 &optimized_instrs) < 0) {
        goto error;
    }

    /** Assembly **/
    co = _PyAssemble_MakeCodeObject(&u->u_metadata, const_cache, consts,
                                    stackdepth, &optimized_instrs, nlocalsplus,
                                    code_flags, filename);

error:
    Py_XDECREF(consts);
    PyInstructionSequence_Fini(&optimized_instrs);
    _PyCfgBuilder_Free(g);
    return co;
}


PyCodeObject *
_PyCompile_OptimizeAndAssemble(compiler *c, int addNone)
{
    struct compiler_unit *u = c->u;
    PyObject *const_cache = c->c_const_cache;
    PyObject *filename = c->c_filename;

    int code_flags = compute_code_flags(c);
    if (code_flags < 0) {
        return NULL;
    }

    if (_PyCodegen_AddReturnAtEnd(c, addNone) < 0) {
        return NULL;
    }

    return optimize_and_assemble_code_unit(u, const_cache, code_flags, filename);
}

PyCodeObject *
_PyAST_Compile(mod_ty mod, PyObject *filename, PyCompilerFlags *pflags,
               int optimize, PyArena *arena, PyObject *module)
{
    assert(!PyErr_Occurred());
    compiler *c = new_compiler(mod, filename, pflags, optimize, arena, module);
    if (c == NULL) {
        return NULL;
    }

    PyCodeObject *co = compiler_mod(c, mod);
    compiler_free(c);
    assert(co || PyErr_Occurred());
    return co;
}

int
_PyCompile_AstPreprocess(mod_ty mod, PyObject *filename, PyCompilerFlags *cf,
                         int optimize, PyArena *arena, int no_const_folding,
                         PyObject *module)
{
    _PyFutureFeatures future;
    if (!_PyFuture_FromAST(mod, filename, &future)) {
        return -1;
    }
    int flags = future.ff_features | cf->cf_flags;
    if (optimize == -1) {
        optimize = _Py_GetConfig()->optimization_level;
    }
    if (!_PyAST_Preprocess(mod, arena, filename, optimize, flags,
                           no_const_folding, 0, module))
    {
        return -1;
    }
    return 0;
}

// C implementation of inspect.cleandoc()
//
// Difference from inspect.cleandoc():
// - Do not remove leading and trailing blank lines to keep lineno.
PyObject *
_PyCompile_CleanDoc(PyObject *doc)
{
    doc = PyObject_CallMethod(doc, "expandtabs", NULL);
    if (doc == NULL) {
        return NULL;
    }

    Py_ssize_t doc_size;
    const char *doc_utf8 = PyUnicode_AsUTF8AndSize(doc, &doc_size);
    if (doc_utf8 == NULL) {
        Py_DECREF(doc);
        return NULL;
    }
    const char *p = doc_utf8;
    const char *pend = p + doc_size;

    // First pass: find minimum indentation of any non-blank lines
    // after first line.
    while (p < pend && *p++ != '\n') {
    }

    Py_ssize_t margin = PY_SSIZE_T_MAX;
    while (p < pend) {
        const char *s = p;
        while (*p == ' ') p++;
        if (p < pend && *p != '\n') {
            margin = Py_MIN(margin, p - s);
        }
        while (p < pend && *p++ != '\n') {
        }
    }
    if (margin == PY_SSIZE_T_MAX) {
        margin = 0;
    }

    // Second pass: write cleandoc into buff.

    // copy first line without leading spaces.
    p = doc_utf8;
    while (*p == ' ') {
        p++;
    }
    if (p == doc_utf8 && margin == 0 ) {
        // doc is already clean.
        return doc;
    }

    char *buff = PyMem_Malloc(doc_size);
    if (buff == NULL){
        Py_DECREF(doc);
        PyErr_NoMemory();
        return NULL;
    }

    char *w = buff;

    while (p < pend) {
        int ch = *w++ = *p++;
        if (ch == '\n') {
            break;
        }
    }

    // copy subsequent lines without margin.
    while (p < pend) {
        for (Py_ssize_t i = 0; i < margin; i++, p++) {
            if (*p != ' ') {
                assert(*p == '\n' || *p == '\0');
                break;
            }
        }
        while (p < pend) {
            int ch = *w++ = *p++;
            if (ch == '\n') {
                break;
            }
        }
    }

    Py_DECREF(doc);
    PyObject *res = PyUnicode_FromStringAndSize(buff, w - buff);
    PyMem_Free(buff);
    return res;
}

/* Access to compiler optimizations for unit tests.
 *
 * _PyCompile_CodeGen takes an AST, applies code-gen and
 * returns the unoptimized CFG as an instruction list.
 *
 */
PyObject *
_PyCompile_CodeGen(PyObject *ast, PyObject *filename, PyCompilerFlags *pflags,
                   int optimize, int compile_mode)
{
    PyObject *res = NULL;
    PyObject *metadata = NULL;

    if (!PyAST_Check(ast)) {
        PyErr_SetString(PyExc_TypeError, "expected an AST");
        return NULL;
    }

    PyArena *arena = _PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    mod_ty mod = PyAST_obj2mod(ast, arena, compile_mode);
    if (mod == NULL || !_PyAST_Validate(mod)) {
        _PyArena_Free(arena);
        return NULL;
    }

    compiler *c = new_compiler(mod, filename, pflags, optimize, arena, NULL);
    if (c == NULL) {
        _PyArena_Free(arena);
        return NULL;
    }
    c->c_save_nested_seqs = true;

    metadata = PyDict_New();
    if (metadata == NULL) {
        return NULL;
    }

    if (compiler_codegen(c, mod) < 0) {
        goto finally;
    }

    _PyCompile_CodeUnitMetadata *umd = &c->u->u_metadata;

#define SET_METADATA_INT(key, value) do { \
        PyObject *v = PyLong_FromLong((long)value); \
        if (v == NULL) goto finally; \
        int res = PyDict_SetItemString(metadata, key, v); \
        Py_XDECREF(v); \
        if (res < 0) goto finally; \
    } while (0);

    SET_METADATA_INT("argcount", umd->u_argcount);
    SET_METADATA_INT("posonlyargcount", umd->u_posonlyargcount);
    SET_METADATA_INT("kwonlyargcount", umd->u_kwonlyargcount);
#undef SET_METADATA_INT

    int addNone = mod->kind != Expression_kind;
    if (_PyCodegen_AddReturnAtEnd(c, addNone) < 0) {
        goto finally;
    }

    if (_PyInstructionSequence_ApplyLabelMap(_PyCompile_InstrSequence(c)) < 0) {
        return NULL;
    }
    /* Allocate a copy of the instruction sequence on the heap */
    res = PyTuple_Pack(2, _PyCompile_InstrSequence(c), metadata);

finally:
    Py_XDECREF(metadata);
    _PyCompile_ExitScope(c);
    compiler_free(c);
    _PyArena_Free(arena);
    return res;
}

int _PyCfg_JumpLabelsToTargets(cfg_builder *g);

PyCodeObject *
_PyCompile_Assemble(_PyCompile_CodeUnitMetadata *umd, PyObject *filename,
                    PyObject *seq)
{
    if (!_PyInstructionSequence_Check(seq)) {
        PyErr_SetString(PyExc_TypeError, "expected an instruction sequence");
        return NULL;
    }
    cfg_builder *g = NULL;
    PyCodeObject *co = NULL;
    instr_sequence optimized_instrs;
    memset(&optimized_instrs, 0, sizeof(instr_sequence));

    PyObject *const_cache = PyDict_New();
    if (const_cache == NULL) {
        return NULL;
    }

    g = _PyCfg_FromInstructionSequence((instr_sequence*)seq);
    if (g == NULL) {
        goto error;
    }

    if (_PyCfg_JumpLabelsToTargets(g) < 0) {
        goto error;
    }

    int code_flags = 0;
    int stackdepth, nlocalsplus;
    if (_PyCfg_OptimizedCfgToInstructionSequence(g, umd,
                                                 &stackdepth, &nlocalsplus,
                                                 &optimized_instrs) < 0) {
        goto error;
    }

    PyObject *consts = consts_dict_keys_inorder(umd->u_consts);
    if (consts == NULL) {
        goto error;
    }
    co = _PyAssemble_MakeCodeObject(umd, const_cache,
                                    consts, stackdepth, &optimized_instrs,
                                    nlocalsplus, code_flags, filename);
    Py_DECREF(consts);

error:
    Py_DECREF(const_cache);
    _PyCfgBuilder_Free(g);
    PyInstructionSequence_Fini(&optimized_instrs);
    return co;
}

/* Retained for API compatibility.
 * Optimization is now done in _PyCfg_OptimizeCodeUnit */

PyObject *
PyCode_Optimize(PyObject *code, PyObject* Py_UNUSED(consts),
                PyObject *Py_UNUSED(names), PyObject *Py_UNUSED(lnotab_obj))
{
    return Py_NewRef(code);
}
