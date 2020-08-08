/*
 * This file compiles an abstract syntax tree (AST) into Python bytecode.
 *
 * The primary entry point is PyAST_Compile(), which returns a
 * PyCodeObject.  The compiler makes several passes to build the code
 * object:
 *   1. Checks for future statements.  See future.c
 *   2. Builds a symbol table.  See symtable.c.
 *   3. Generate code for basic blocks.  See compiler_mod() in this file.
 *   4. Assemble the basic blocks into final code.  See assemble() in
 *      this file.
 *   5. Optimize the byte code (peephole optimizations).
 *
 * Note that compiler_mod() suggests module, but the module ast type
 * (mod_ty) has cases for expressions and interactive statements.
 *
 * CAUTION: The VISIT_* macros abort the current function when they
 * encounter a problem. So don't invoke them when there is memory
 * which needs to be released. Code blocks are OK, as the compiler
 * structure takes care of releasing those.  Use the arena to manage
 * objects.
 */

#include "Python.h"

#include "Python-ast.h"
#include "ast.h"
#include "code.h"
#include "symtable.h"
#define NEED_OPCODE_JUMP_TABLES
#include "opcode.h"
#include "wordcode_helpers.h"

#define DEFAULT_BLOCK_SIZE 16
#define DEFAULT_BLOCKS 8
#define DEFAULT_CODE_SIZE 128
#define DEFAULT_LNOTAB_SIZE 16

#define COMP_GENEXP   0
#define COMP_LISTCOMP 1
#define COMP_SETCOMP  2
#define COMP_DICTCOMP 3

#define IS_TOP_LEVEL_AWAIT(c) ( \
        (c->c_flags->cf_flags & PyCF_ALLOW_TOP_LEVEL_AWAIT) \
        && (c->u->u_ste->ste_type == ModuleBlock))

struct instr {
    unsigned char i_opcode;
    int i_oparg;
    struct basicblock_ *i_target; /* target block (if jump instruction) */
    int i_lineno;
};

#define LOG_BITS_PER_INT 5
#define MASK_LOW_LOG_BITS 31

static inline int
is_bit_set_in_table(uint32_t *table, int bitindex) {
    /* Is the relevant bit set in the relevant word? */
    /* 256 bits fit into 8 32-bits words.
     * Word is indexed by (bitindex>>ln(size of int in bits)).
     * Bit within word is the low bits of bitindex.
     */
    uint32_t word = table[bitindex >> LOG_BITS_PER_INT];
    return (word >> (bitindex & MASK_LOW_LOG_BITS)) & 1;
}

static inline int
is_relative_jump(struct instr *i)
{
    return is_bit_set_in_table(_PyOpcode_RelativeJump, i->i_opcode);
}

static inline int
is_jump(struct instr *i)
{
    return is_bit_set_in_table(_PyOpcode_Jump, i->i_opcode);
}

typedef struct basicblock_ {
    /* Each basicblock in a compilation unit is linked via b_list in the
       reverse order that the block are allocated.  b_list points to the next
       block, not to be confused with b_next, which is next by control flow. */
    struct basicblock_ *b_list;
    /* number of instructions used */
    int b_iused;
    /* length of instruction array (b_instr) */
    int b_ialloc;
    /* pointer to an array of instructions, initially NULL */
    struct instr *b_instr;
    /* If b_next is non-NULL, it is a pointer to the next
       block reached by normal control flow. */
    struct basicblock_ *b_next;
    /* b_return is true if a RETURN_VALUE opcode is inserted. */
    unsigned b_return : 1;
    unsigned b_reachable : 1;
    /* depth of stack upon entry of block, computed by stackdepth() */
    int b_startdepth;
    /* instruction offset for block, computed by assemble_jump_offsets() */
    int b_offset;
} basicblock;

/* fblockinfo tracks the current frame block.

A frame block is used to handle loops, try/except, and try/finally.
It's called a frame block to distinguish it from a basic block in the
compiler IR.
*/

enum fblocktype { WHILE_LOOP, FOR_LOOP, EXCEPT, FINALLY_TRY, FINALLY_END,
                  WITH, ASYNC_WITH, HANDLER_CLEANUP, POP_VALUE };

struct fblockinfo {
    enum fblocktype fb_type;
    basicblock *fb_block;
    /* (optional) type-specific exit or cleanup block */
    basicblock *fb_exit;
    /* (optional) additional information required for unwinding */
    void *fb_datum;
};

enum {
    COMPILER_SCOPE_MODULE,
    COMPILER_SCOPE_CLASS,
    COMPILER_SCOPE_FUNCTION,
    COMPILER_SCOPE_ASYNC_FUNCTION,
    COMPILER_SCOPE_LAMBDA,
    COMPILER_SCOPE_COMPREHENSION,
};

/* The following items change on entry and exit of code blocks.
   They must be saved and restored when returning to a block.
*/
struct compiler_unit {
    PySTEntryObject *u_ste;

    PyObject *u_name;
    PyObject *u_qualname;  /* dot-separated qualified name (lazy) */
    int u_scope_type;

    /* The following fields are dicts that map objects to
       the index of them in co_XXX.      The index is used as
       the argument for opcodes that refer to those collections.
    */
    PyObject *u_consts;    /* all constants */
    PyObject *u_names;     /* all names */
    PyObject *u_varnames;  /* local variables */
    PyObject *u_cellvars;  /* cell variables */
    PyObject *u_freevars;  /* free variables */

    PyObject *u_private;        /* for private name mangling */

    Py_ssize_t u_argcount;        /* number of arguments for block */
    Py_ssize_t u_posonlyargcount;        /* number of positional only arguments for block */
    Py_ssize_t u_kwonlyargcount; /* number of keyword only arguments for block */
    /* Pointer to the most recently allocated block.  By following b_list
       members, you can reach all early allocated blocks. */
    basicblock *u_blocks;
    basicblock *u_curblock; /* pointer to current block */

    int u_nfblocks;
    struct fblockinfo u_fblock[CO_MAXBLOCKS];

    int u_firstlineno; /* the first lineno of the block */
    int u_lineno;          /* the lineno for the current stmt */
    int u_col_offset;      /* the offset of the current stmt */
};

/* This struct captures the global state of a compilation.

The u pointer points to the current compilation unit, while units
for enclosing blocks are stored in c_stack.     The u and c_stack are
managed by compiler_enter_scope() and compiler_exit_scope().

Note that we don't track recursion levels during compilation - the
task of detecting and rejecting excessive levels of nesting is
handled by the symbol analysis pass.

*/

struct compiler {
    PyObject *c_filename;
    struct symtable *c_st;
    PyFutureFeatures *c_future; /* pointer to module's __future__ */
    PyCompilerFlags *c_flags;

    int c_optimize;              /* optimization level */
    int c_interactive;           /* true if in interactive mode */
    int c_nestlevel;
    int c_do_not_emit_bytecode;  /* The compiler won't emit any bytecode
                                    if this value is different from zero.
                                    This can be used to temporarily visit
                                    nodes without emitting bytecode to
                                    check only errors. */

    PyObject *c_const_cache;     /* Python dict holding all constants,
                                    including names tuple */
    struct compiler_unit *u; /* compiler state for current block */
    PyObject *c_stack;           /* Python list holding compiler_unit ptrs */
    PyArena *c_arena;            /* pointer to memory allocation arena */
};

static int compiler_enter_scope(struct compiler *, identifier, int, void *, int);
static void compiler_free(struct compiler *);
static basicblock *compiler_new_block(struct compiler *);
static int compiler_next_instr(basicblock *);
static int compiler_addop(struct compiler *, int);
static int compiler_addop_i(struct compiler *, int, Py_ssize_t);
static int compiler_addop_j(struct compiler *, int, basicblock *);
static int compiler_error(struct compiler *, const char *);
static int compiler_warn(struct compiler *, const char *, ...);
static int compiler_nameop(struct compiler *, identifier, expr_context_ty);

static PyCodeObject *compiler_mod(struct compiler *, mod_ty);
static int compiler_visit_stmt(struct compiler *, stmt_ty);
static int compiler_visit_keyword(struct compiler *, keyword_ty);
static int compiler_visit_expr(struct compiler *, expr_ty);
static int compiler_augassign(struct compiler *, stmt_ty);
static int compiler_annassign(struct compiler *, stmt_ty);
static int compiler_subscript(struct compiler *, expr_ty);
static int compiler_slice(struct compiler *, expr_ty);

static int inplace_binop(operator_ty);
static int are_all_items_const(asdl_seq *, Py_ssize_t, Py_ssize_t);
static int expr_constant(expr_ty);

static int compiler_with(struct compiler *, stmt_ty, int);
static int compiler_async_with(struct compiler *, stmt_ty, int);
static int compiler_async_for(struct compiler *, stmt_ty);
static int compiler_call_helper(struct compiler *c, int n,
                                asdl_seq *args,
                                asdl_seq *keywords);
static int compiler_try_except(struct compiler *, stmt_ty);
static int compiler_set_qualname(struct compiler *);

static int compiler_sync_comprehension_generator(
                                      struct compiler *c,
                                      asdl_seq *generators, int gen_index,
                                      int depth,
                                      expr_ty elt, expr_ty val, int type);

static int compiler_async_comprehension_generator(
                                      struct compiler *c,
                                      asdl_seq *generators, int gen_index,
                                      int depth,
                                      expr_ty elt, expr_ty val, int type);

static PyCodeObject *assemble(struct compiler *, int addNone);
static PyObject *__doc__, *__annotations__;

#define CAPSULE_NAME "compile.c compiler unit"

PyObject *
_Py_Mangle(PyObject *privateobj, PyObject *ident)
{
    /* Name mangling: __private becomes _classname__private.
       This is independent from how the name is used. */
    PyObject *result;
    size_t nlen, plen, ipriv;
    Py_UCS4 maxchar;
    if (privateobj == NULL || !PyUnicode_Check(privateobj) ||
        PyUnicode_READ_CHAR(ident, 0) != '_' ||
        PyUnicode_READ_CHAR(ident, 1) != '_') {
        Py_INCREF(ident);
        return ident;
    }
    nlen = PyUnicode_GET_LENGTH(ident);
    plen = PyUnicode_GET_LENGTH(privateobj);
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
        Py_INCREF(ident);
        return ident; /* Don't mangle __whatever__ */
    }
    /* Strip leading underscores from class name */
    ipriv = 0;
    while (PyUnicode_READ_CHAR(privateobj, ipriv) == '_')
        ipriv++;
    if (ipriv == plen) {
        Py_INCREF(ident);
        return ident; /* Don't mangle if class is just underscores */
    }
    plen -= ipriv;

    if (plen + nlen >= PY_SSIZE_T_MAX - 1) {
        PyErr_SetString(PyExc_OverflowError,
                        "private identifier too large to be mangled");
        return NULL;
    }

    maxchar = PyUnicode_MAX_CHAR_VALUE(ident);
    if (PyUnicode_MAX_CHAR_VALUE(privateobj) > maxchar)
        maxchar = PyUnicode_MAX_CHAR_VALUE(privateobj);

    result = PyUnicode_New(1 + nlen + plen, maxchar);
    if (!result)
        return 0;
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

static int
compiler_init(struct compiler *c)
{
    memset(c, 0, sizeof(struct compiler));

    c->c_const_cache = PyDict_New();
    if (!c->c_const_cache) {
        return 0;
    }

    c->c_stack = PyList_New(0);
    if (!c->c_stack) {
        Py_CLEAR(c->c_const_cache);
        return 0;
    }

    return 1;
}

PyCodeObject *
PyAST_CompileObject(mod_ty mod, PyObject *filename, PyCompilerFlags *flags,
                   int optimize, PyArena *arena)
{
    struct compiler c;
    PyCodeObject *co = NULL;
    PyCompilerFlags local_flags = _PyCompilerFlags_INIT;
    int merged;

    if (!__doc__) {
        __doc__ = PyUnicode_InternFromString("__doc__");
        if (!__doc__)
            return NULL;
    }
    if (!__annotations__) {
        __annotations__ = PyUnicode_InternFromString("__annotations__");
        if (!__annotations__)
            return NULL;
    }
    if (!compiler_init(&c))
        return NULL;
    Py_INCREF(filename);
    c.c_filename = filename;
    c.c_arena = arena;
    c.c_future = PyFuture_FromASTObject(mod, filename);
    if (c.c_future == NULL)
        goto finally;
    if (!flags) {
        flags = &local_flags;
    }
    merged = c.c_future->ff_features | flags->cf_flags;
    c.c_future->ff_features = merged;
    flags->cf_flags = merged;
    c.c_flags = flags;
    c.c_optimize = (optimize == -1) ? _Py_GetConfig()->optimization_level : optimize;
    c.c_nestlevel = 0;
    c.c_do_not_emit_bytecode = 0;

    _PyASTOptimizeState state;
    state.optimize = c.c_optimize;
    state.ff_features = merged;

    if (!_PyAST_Optimize(mod, arena, &state)) {
        goto finally;
    }

    c.c_st = PySymtable_BuildObject(mod, filename, c.c_future);
    if (c.c_st == NULL) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_SystemError, "no symtable");
        goto finally;
    }

    co = compiler_mod(&c, mod);

 finally:
    compiler_free(&c);
    assert(co || PyErr_Occurred());
    return co;
}

PyCodeObject *
PyAST_CompileEx(mod_ty mod, const char *filename_str, PyCompilerFlags *flags,
                int optimize, PyArena *arena)
{
    PyObject *filename;
    PyCodeObject *co;
    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    co = PyAST_CompileObject(mod, filename, flags, optimize, arena);
    Py_DECREF(filename);
    return co;

}

static void
compiler_free(struct compiler *c)
{
    if (c->c_st)
        PySymtable_Free(c->c_st);
    if (c->c_future)
        PyObject_Free(c->c_future);
    Py_XDECREF(c->c_filename);
    Py_DECREF(c->c_const_cache);
    Py_DECREF(c->c_stack);
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
    Py_ssize_t i = offset, scope, num_keys, key_i;
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
    if (sorted_keys == NULL)
        return NULL;
    if (PyList_Sort(sorted_keys) != 0) {
        Py_DECREF(sorted_keys);
        return NULL;
    }
    num_keys = PyList_GET_SIZE(sorted_keys);

    for (key_i = 0; key_i < num_keys; key_i++) {
        /* XXX this should probably be a macro in symtable.h */
        long vi;
        k = PyList_GET_ITEM(sorted_keys, key_i);
        v = PyDict_GetItem(src, k);
        assert(PyLong_Check(v));
        vi = PyLong_AS_LONG(v);
        scope = (vi >> SCOPE_OFFSET) & SCOPE_MASK;

        if (scope == scope_type || vi & flag) {
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

static void
compiler_unit_check(struct compiler_unit *u)
{
    basicblock *block;
    for (block = u->u_blocks; block != NULL; block = block->b_list) {
        assert((uintptr_t)block != 0xcbcbcbcbU);
        assert((uintptr_t)block != 0xfbfbfbfbU);
        assert((uintptr_t)block != 0xdbdbdbdbU);
        if (block->b_instr != NULL) {
            assert(block->b_ialloc > 0);
            assert(block->b_iused >= 0);
            assert(block->b_ialloc >= block->b_iused);
        }
        else {
            assert (block->b_iused == 0);
            assert (block->b_ialloc == 0);
        }
    }
}

static void
compiler_unit_free(struct compiler_unit *u)
{
    basicblock *b, *next;

    compiler_unit_check(u);
    b = u->u_blocks;
    while (b != NULL) {
        if (b->b_instr)
            PyObject_Free((void *)b->b_instr);
        next = b->b_list;
        PyObject_Free((void *)b);
        b = next;
    }
    Py_CLEAR(u->u_ste);
    Py_CLEAR(u->u_name);
    Py_CLEAR(u->u_qualname);
    Py_CLEAR(u->u_consts);
    Py_CLEAR(u->u_names);
    Py_CLEAR(u->u_varnames);
    Py_CLEAR(u->u_freevars);
    Py_CLEAR(u->u_cellvars);
    Py_CLEAR(u->u_private);
    PyObject_Free(u);
}

static int
compiler_enter_scope(struct compiler *c, identifier name,
                     int scope_type, void *key, int lineno)
{
    struct compiler_unit *u;
    basicblock *block;

    u = (struct compiler_unit *)PyObject_Calloc(1, sizeof(
                                            struct compiler_unit));
    if (!u) {
        PyErr_NoMemory();
        return 0;
    }
    u->u_scope_type = scope_type;
    u->u_argcount = 0;
    u->u_posonlyargcount = 0;
    u->u_kwonlyargcount = 0;
    u->u_ste = PySymtable_Lookup(c->c_st, key);
    if (!u->u_ste) {
        compiler_unit_free(u);
        return 0;
    }
    Py_INCREF(name);
    u->u_name = name;
    u->u_varnames = list2dict(u->u_ste->ste_varnames);
    u->u_cellvars = dictbytype(u->u_ste->ste_symbols, CELL, 0, 0);
    if (!u->u_varnames || !u->u_cellvars) {
        compiler_unit_free(u);
        return 0;
    }
    if (u->u_ste->ste_needs_class_closure) {
        /* Cook up an implicit __class__ cell. */
        _Py_IDENTIFIER(__class__);
        PyObject *name;
        int res;
        assert(u->u_scope_type == COMPILER_SCOPE_CLASS);
        assert(PyDict_GET_SIZE(u->u_cellvars) == 0);
        name = _PyUnicode_FromId(&PyId___class__);
        if (!name) {
            compiler_unit_free(u);
            return 0;
        }
        res = PyDict_SetItem(u->u_cellvars, name, _PyLong_Zero);
        if (res < 0) {
            compiler_unit_free(u);
            return 0;
        }
    }

    u->u_freevars = dictbytype(u->u_ste->ste_symbols, FREE, DEF_FREE_CLASS,
                               PyDict_GET_SIZE(u->u_cellvars));
    if (!u->u_freevars) {
        compiler_unit_free(u);
        return 0;
    }

    u->u_blocks = NULL;
    u->u_nfblocks = 0;
    u->u_firstlineno = lineno;
    u->u_lineno = 0;
    u->u_col_offset = 0;
    u->u_consts = PyDict_New();
    if (!u->u_consts) {
        compiler_unit_free(u);
        return 0;
    }
    u->u_names = PyDict_New();
    if (!u->u_names) {
        compiler_unit_free(u);
        return 0;
    }

    u->u_private = NULL;

    /* Push the old compiler_unit on the stack. */
    if (c->u) {
        PyObject *capsule = PyCapsule_New(c->u, CAPSULE_NAME, NULL);
        if (!capsule || PyList_Append(c->c_stack, capsule) < 0) {
            Py_XDECREF(capsule);
            compiler_unit_free(u);
            return 0;
        }
        Py_DECREF(capsule);
        u->u_private = c->u->u_private;
        Py_XINCREF(u->u_private);
    }
    c->u = u;

    c->c_nestlevel++;

    block = compiler_new_block(c);
    if (block == NULL)
        return 0;
    c->u->u_curblock = block;

    if (u->u_scope_type != COMPILER_SCOPE_MODULE) {
        if (!compiler_set_qualname(c))
            return 0;
    }

    return 1;
}

static void
compiler_exit_scope(struct compiler *c)
{
    Py_ssize_t n;
    PyObject *capsule;

    c->c_nestlevel--;
    compiler_unit_free(c->u);
    /* Restore c->u to the parent unit. */
    n = PyList_GET_SIZE(c->c_stack) - 1;
    if (n >= 0) {
        capsule = PyList_GET_ITEM(c->c_stack, n);
        c->u = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
        assert(c->u);
        /* we are deleting from a list so this really shouldn't fail */
        if (PySequence_DelItem(c->c_stack, n) < 0)
            Py_FatalError("compiler_exit_scope()");
        compiler_unit_check(c->u);
    }
    else
        c->u = NULL;

}

static int
compiler_set_qualname(struct compiler *c)
{
    _Py_static_string(dot, ".");
    _Py_static_string(dot_locals, ".<locals>");
    Py_ssize_t stack_size;
    struct compiler_unit *u = c->u;
    PyObject *name, *base, *dot_str, *dot_locals_str;

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

        if (u->u_scope_type == COMPILER_SCOPE_FUNCTION
            || u->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION
            || u->u_scope_type == COMPILER_SCOPE_CLASS) {
            assert(u->u_name);
            mangled = _Py_Mangle(parent->u_private, u->u_name);
            if (!mangled)
                return 0;
            scope = PyST_GetScope(parent->u_ste, mangled);
            Py_DECREF(mangled);
            assert(scope != GLOBAL_IMPLICIT);
            if (scope == GLOBAL_EXPLICIT)
                force_global = 1;
        }

        if (!force_global) {
            if (parent->u_scope_type == COMPILER_SCOPE_FUNCTION
                || parent->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION
                || parent->u_scope_type == COMPILER_SCOPE_LAMBDA) {
                dot_locals_str = _PyUnicode_FromId(&dot_locals);
                if (dot_locals_str == NULL)
                    return 0;
                base = PyUnicode_Concat(parent->u_qualname, dot_locals_str);
                if (base == NULL)
                    return 0;
            }
            else {
                Py_INCREF(parent->u_qualname);
                base = parent->u_qualname;
            }
        }
    }

    if (base != NULL) {
        dot_str = _PyUnicode_FromId(&dot);
        if (dot_str == NULL) {
            Py_DECREF(base);
            return 0;
        }
        name = PyUnicode_Concat(base, dot_str);
        Py_DECREF(base);
        if (name == NULL)
            return 0;
        PyUnicode_Append(&name, u->u_name);
        if (name == NULL)
            return 0;
    }
    else {
        Py_INCREF(u->u_name);
        name = u->u_name;
    }
    u->u_qualname = name;

    return 1;
}


/* Allocate a new block and return a pointer to it.
   Returns NULL on error.
*/

static basicblock *
compiler_new_block(struct compiler *c)
{
    basicblock *b;
    struct compiler_unit *u;

    u = c->u;
    b = (basicblock *)PyObject_Calloc(1, sizeof(basicblock));
    if (b == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    /* Extend the singly linked list of blocks with new block. */
    b->b_list = u->u_blocks;
    u->u_blocks = b;
    return b;
}

static basicblock *
compiler_next_block(struct compiler *c)
{
    basicblock *block = compiler_new_block(c);
    if (block == NULL)
        return NULL;
    c->u->u_curblock->b_next = block;
    c->u->u_curblock = block;
    return block;
}

static basicblock *
compiler_use_next_block(struct compiler *c, basicblock *block)
{
    assert(block != NULL);
    c->u->u_curblock->b_next = block;
    c->u->u_curblock = block;
    return block;
}

/* Returns the offset of the next instruction in the current block's
   b_instr array.  Resizes the b_instr as necessary.
   Returns -1 on failure.
*/

static int
compiler_next_instr(basicblock *b)
{
    assert(b != NULL);
    if (b->b_instr == NULL) {
        b->b_instr = (struct instr *)PyObject_Calloc(
                         DEFAULT_BLOCK_SIZE, sizeof(struct instr));
        if (b->b_instr == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        b->b_ialloc = DEFAULT_BLOCK_SIZE;
    }
    else if (b->b_iused == b->b_ialloc) {
        struct instr *tmp;
        size_t oldsize, newsize;
        oldsize = b->b_ialloc * sizeof(struct instr);
        newsize = oldsize << 1;

        if (oldsize > (SIZE_MAX >> 1)) {
            PyErr_NoMemory();
            return -1;
        }

        if (newsize == 0) {
            PyErr_NoMemory();
            return -1;
        }
        b->b_ialloc <<= 1;
        tmp = (struct instr *)PyObject_Realloc(
                                        (void *)b->b_instr, newsize);
        if (tmp == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        b->b_instr = tmp;
        memset((char *)b->b_instr + oldsize, 0, newsize - oldsize);
    }
    return b->b_iused++;
}

/* Set the line number and column offset for the following instructions.

   The line number is reset in the following cases:
   - when entering a new scope
   - on each statement
   - on each expression and sub-expression
   - before the "except" and "finally" clauses
*/

#define SET_LOC(c, x)                           \
    (c)->u->u_lineno = (x)->lineno;             \
    (c)->u->u_col_offset = (x)->col_offset;

/* Return the stack effect of opcode with argument oparg.

   Some opcodes have different stack effect when jump to the target and
   when not jump. The 'jump' parameter specifies the case:

   * 0 -- when not jump
   * 1 -- when jump
   * -1 -- maximal
 */
/* XXX Make the stack effect of WITH_CLEANUP_START and
   WITH_CLEANUP_FINISH deterministic. */
static int
stack_effect(int opcode, int oparg, int jump)
{
    switch (opcode) {
        case NOP:
        case EXTENDED_ARG:
            return 0;

        /* Stack manipulation */
        case POP_TOP:
            return -1;
        case ROT_TWO:
        case ROT_THREE:
        case ROT_FOUR:
            return 0;
        case DUP_TOP:
            return 1;
        case DUP_TOP_TWO:
            return 2;

        /* Unary operators */
        case UNARY_POSITIVE:
        case UNARY_NEGATIVE:
        case UNARY_NOT:
        case UNARY_INVERT:
            return 0;

        case SET_ADD:
        case LIST_APPEND:
            return -1;
        case MAP_ADD:
            return -2;

        /* Binary operators */
        case BINARY_POWER:
        case BINARY_MULTIPLY:
        case BINARY_MATRIX_MULTIPLY:
        case BINARY_MODULO:
        case BINARY_ADD:
        case BINARY_SUBTRACT:
        case BINARY_SUBSCR:
        case BINARY_FLOOR_DIVIDE:
        case BINARY_TRUE_DIVIDE:
            return -1;
        case INPLACE_FLOOR_DIVIDE:
        case INPLACE_TRUE_DIVIDE:
            return -1;

        case INPLACE_ADD:
        case INPLACE_SUBTRACT:
        case INPLACE_MULTIPLY:
        case INPLACE_MATRIX_MULTIPLY:
        case INPLACE_MODULO:
            return -1;
        case STORE_SUBSCR:
            return -3;
        case DELETE_SUBSCR:
            return -2;

        case BINARY_LSHIFT:
        case BINARY_RSHIFT:
        case BINARY_AND:
        case BINARY_XOR:
        case BINARY_OR:
            return -1;
        case INPLACE_POWER:
            return -1;
        case GET_ITER:
            return 0;

        case PRINT_EXPR:
            return -1;
        case LOAD_BUILD_CLASS:
            return 1;
        case INPLACE_LSHIFT:
        case INPLACE_RSHIFT:
        case INPLACE_AND:
        case INPLACE_XOR:
        case INPLACE_OR:
            return -1;

        case SETUP_WITH:
            /* 1 in the normal flow.
             * Restore the stack position and push 6 values before jumping to
             * the handler if an exception be raised. */
            return jump ? 6 : 1;
        case RETURN_VALUE:
            return -1;
        case IMPORT_STAR:
            return -1;
        case SETUP_ANNOTATIONS:
            return 0;
        case YIELD_VALUE:
            return 0;
        case YIELD_FROM:
            return -1;
        case POP_BLOCK:
            return 0;
        case POP_EXCEPT:
            return -3;

        case STORE_NAME:
            return -1;
        case DELETE_NAME:
            return 0;
        case UNPACK_SEQUENCE:
            return oparg-1;
        case UNPACK_EX:
            return (oparg&0xFF) + (oparg>>8);
        case FOR_ITER:
            /* -1 at end of iterator, 1 if continue iterating. */
            return jump > 0 ? -1 : 1;

        case STORE_ATTR:
            return -2;
        case DELETE_ATTR:
            return -1;
        case STORE_GLOBAL:
            return -1;
        case DELETE_GLOBAL:
            return 0;
        case LOAD_CONST:
            return 1;
        case LOAD_NAME:
            return 1;
        case BUILD_TUPLE:
        case BUILD_LIST:
        case BUILD_SET:
        case BUILD_STRING:
            return 1-oparg;
        case BUILD_MAP:
            return 1 - 2*oparg;
        case BUILD_CONST_KEY_MAP:
            return -oparg;
        case LOAD_ATTR:
            return 0;
        case COMPARE_OP:
        case IS_OP:
        case CONTAINS_OP:
            return -1;
        case JUMP_IF_NOT_EXC_MATCH:
            return -2;
        case IMPORT_NAME:
            return -1;
        case IMPORT_FROM:
            return 1;

        /* Jumps */
        case JUMP_FORWARD:
        case JUMP_ABSOLUTE:
            return 0;

        case JUMP_IF_TRUE_OR_POP:
        case JUMP_IF_FALSE_OR_POP:
            return jump ? 0 : -1;

        case POP_JUMP_IF_FALSE:
        case POP_JUMP_IF_TRUE:
            return -1;

        case LOAD_GLOBAL:
            return 1;

        /* Exception handling */
        case SETUP_FINALLY:
            /* 0 in the normal flow.
             * Restore the stack position and push 6 values before jumping to
             * the handler if an exception be raised. */
            return jump ? 6 : 0;
        case RERAISE:
            return -3;

        case WITH_EXCEPT_START:
            return 1;

        case LOAD_FAST:
            return 1;
        case STORE_FAST:
            return -1;
        case DELETE_FAST:
            return 0;

        case RAISE_VARARGS:
            return -oparg;

        /* Functions and calls */
        case CALL_FUNCTION:
            return -oparg;
        case CALL_METHOD:
            return -oparg-1;
        case CALL_FUNCTION_KW:
            return -oparg-1;
        case CALL_FUNCTION_EX:
            return -1 - ((oparg & 0x01) != 0);
        case MAKE_FUNCTION:
            return -1 - ((oparg & 0x01) != 0) - ((oparg & 0x02) != 0) -
                ((oparg & 0x04) != 0) - ((oparg & 0x08) != 0);
        case BUILD_SLICE:
            if (oparg == 3)
                return -2;
            else
                return -1;

        /* Closures */
        case LOAD_CLOSURE:
            return 1;
        case LOAD_DEREF:
        case LOAD_CLASSDEREF:
            return 1;
        case STORE_DEREF:
            return -1;
        case DELETE_DEREF:
            return 0;

        /* Iterators and generators */
        case GET_AWAITABLE:
            return 0;
        case SETUP_ASYNC_WITH:
            /* 0 in the normal flow.
             * Restore the stack position to the position before the result
             * of __aenter__ and push 6 values before jumping to the handler
             * if an exception be raised. */
            return jump ? -1 + 6 : 0;
        case BEFORE_ASYNC_WITH:
            return 1;
        case GET_AITER:
            return 0;
        case GET_ANEXT:
            return 1;
        case GET_YIELD_FROM_ITER:
            return 0;
        case END_ASYNC_FOR:
            return -7;
        case FORMAT_VALUE:
            /* If there's a fmt_spec on the stack, we go from 2->1,
               else 1->1. */
            return (oparg & FVS_MASK) == FVS_HAVE_SPEC ? -1 : 0;
        case LOAD_METHOD:
            return 1;
        case LOAD_ASSERTION_ERROR:
            return 1;
        case LIST_TO_TUPLE:
            return 0;
        case LIST_EXTEND:
        case SET_UPDATE:
        case DICT_MERGE:
        case DICT_UPDATE:
            return -1;
        default:
            return PY_INVALID_STACK_EFFECT;
    }
    return PY_INVALID_STACK_EFFECT; /* not reachable */
}

int
PyCompile_OpcodeStackEffectWithJump(int opcode, int oparg, int jump)
{
    return stack_effect(opcode, oparg, jump);
}

int
PyCompile_OpcodeStackEffect(int opcode, int oparg)
{
    return stack_effect(opcode, oparg, -1);
}

/* Add an opcode with no argument.
   Returns 0 on failure, 1 on success.
*/

static int
compiler_addop(struct compiler *c, int opcode)
{
    basicblock *b;
    struct instr *i;
    int off;
    assert(!HAS_ARG(opcode));
    if (c->c_do_not_emit_bytecode) {
        return 1;
    }
    off = compiler_next_instr(c->u->u_curblock);
    if (off < 0)
        return 0;
    b = c->u->u_curblock;
    i = &b->b_instr[off];
    i->i_opcode = opcode;
    i->i_oparg = 0;
    if (opcode == RETURN_VALUE)
        b->b_return = 1;
    i->i_lineno = c->u->u_lineno;
    return 1;
}

static Py_ssize_t
compiler_add_o(PyObject *dict, PyObject *o)
{
    PyObject *v;
    Py_ssize_t arg;

    v = PyDict_GetItemWithError(dict, o);
    if (!v) {
        if (PyErr_Occurred()) {
            return -1;
        }
        arg = PyDict_GET_SIZE(dict);
        v = PyLong_FromSsize_t(arg);
        if (!v) {
            return -1;
        }
        if (PyDict_SetItem(dict, o, v) < 0) {
            Py_DECREF(v);
            return -1;
        }
        Py_DECREF(v);
    }
    else
        arg = PyLong_AsLong(v);
    return arg;
}

// Merge const *o* recursively and return constant key object.
static PyObject*
merge_consts_recursive(struct compiler *c, PyObject *o)
{
    // None and Ellipsis are singleton, and key is the singleton.
    // No need to merge object and key.
    if (o == Py_None || o == Py_Ellipsis) {
        Py_INCREF(o);
        return o;
    }

    PyObject *key = _PyCode_ConstantKey(o);
    if (key == NULL) {
        return NULL;
    }

    // t is borrowed reference
    PyObject *t = PyDict_SetDefault(c->c_const_cache, key, key);
    if (t != key) {
        // o is registered in c_const_cache.  Just use it.
        Py_XINCREF(t);
        Py_DECREF(key);
        return t;
    }

    // We registered o in c_const_cache.
    // When o is a tuple or frozenset, we want to merge its
    // items too.
    if (PyTuple_CheckExact(o)) {
        Py_ssize_t len = PyTuple_GET_SIZE(o);
        for (Py_ssize_t i = 0; i < len; i++) {
            PyObject *item = PyTuple_GET_ITEM(o, i);
            PyObject *u = merge_consts_recursive(c, item);
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
                Py_INCREF(v);
                PyTuple_SET_ITEM(o, i, v);
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
            PyObject *k = merge_consts_recursive(c, item);
            if (k == NULL) {
                Py_DECREF(tuple);
                Py_DECREF(key);
                return NULL;
            }
            PyObject *u;
            if (PyTuple_CheckExact(k)) {
                u = PyTuple_GET_ITEM(k, 1);
                Py_INCREF(u);
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

static Py_ssize_t
compiler_add_const(struct compiler *c, PyObject *o)
{
    if (c->c_do_not_emit_bytecode) {
        return 0;
    }

    PyObject *key = merge_consts_recursive(c, o);
    if (key == NULL) {
        return -1;
    }

    Py_ssize_t arg = compiler_add_o(c->u->u_consts, key);
    Py_DECREF(key);
    return arg;
}

static int
compiler_addop_load_const(struct compiler *c, PyObject *o)
{
    if (c->c_do_not_emit_bytecode) {
        return 1;
    }

    Py_ssize_t arg = compiler_add_const(c, o);
    if (arg < 0)
        return 0;
    return compiler_addop_i(c, LOAD_CONST, arg);
}

static int
compiler_addop_o(struct compiler *c, int opcode, PyObject *dict,
                     PyObject *o)
{
    if (c->c_do_not_emit_bytecode) {
        return 1;
    }

    Py_ssize_t arg = compiler_add_o(dict, o);
    if (arg < 0)
        return 0;
    return compiler_addop_i(c, opcode, arg);
}

static int
compiler_addop_name(struct compiler *c, int opcode, PyObject *dict,
                    PyObject *o)
{
    Py_ssize_t arg;

    if (c->c_do_not_emit_bytecode) {
        return 1;
    }

    PyObject *mangled = _Py_Mangle(c->u->u_private, o);
    if (!mangled)
        return 0;
    arg = compiler_add_o(dict, mangled);
    Py_DECREF(mangled);
    if (arg < 0)
        return 0;
    return compiler_addop_i(c, opcode, arg);
}

/* Add an opcode with an integer argument.
   Returns 0 on failure, 1 on success.
*/

static int
compiler_addop_i(struct compiler *c, int opcode, Py_ssize_t oparg)
{
    struct instr *i;
    int off;

    if (c->c_do_not_emit_bytecode) {
        return 1;
    }

    /* oparg value is unsigned, but a signed C int is usually used to store
       it in the C code (like Python/ceval.c).

       Limit to 32-bit signed C int (rather than INT_MAX) for portability.

       The argument of a concrete bytecode instruction is limited to 8-bit.
       EXTENDED_ARG is used for 16, 24, and 32-bit arguments. */
    assert(HAS_ARG(opcode));
    assert(0 <= oparg && oparg <= 2147483647);

    off = compiler_next_instr(c->u->u_curblock);
    if (off < 0)
        return 0;
    i = &c->u->u_curblock->b_instr[off];
    i->i_opcode = opcode;
    i->i_oparg = Py_SAFE_DOWNCAST(oparg, Py_ssize_t, int);
    i->i_lineno = c->u->u_lineno;
    return 1;
}

static int
compiler_addop_j(struct compiler *c, int opcode, basicblock *b)
{
    struct instr *i;
    int off;

    if (c->c_do_not_emit_bytecode) {
        return 1;
    }

    assert(HAS_ARG(opcode));
    assert(b != NULL);
    off = compiler_next_instr(c->u->u_curblock);
    if (off < 0)
        return 0;
    i = &c->u->u_curblock->b_instr[off];
    i->i_opcode = opcode;
    i->i_target = b;
    i->i_lineno = c->u->u_lineno;
    return 1;
}

/* NEXT_BLOCK() creates an implicit jump from the current block
   to the new block.

   The returns inside this macro make it impossible to decref objects
   created in the local function. Local objects should use the arena.
*/
#define NEXT_BLOCK(C) { \
    if (compiler_next_block((C)) == NULL) \
        return 0; \
}

#define ADDOP(C, OP) { \
    if (!compiler_addop((C), (OP))) \
        return 0; \
}

#define ADDOP_IN_SCOPE(C, OP) { \
    if (!compiler_addop((C), (OP))) { \
        compiler_exit_scope(c); \
        return 0; \
    } \
}

#define ADDOP_LOAD_CONST(C, O) { \
    if (!compiler_addop_load_const((C), (O))) \
        return 0; \
}

/* Same as ADDOP_LOAD_CONST, but steals a reference. */
#define ADDOP_LOAD_CONST_NEW(C, O) { \
    PyObject *__new_const = (O); \
    if (__new_const == NULL) { \
        return 0; \
    } \
    if (!compiler_addop_load_const((C), __new_const)) { \
        Py_DECREF(__new_const); \
        return 0; \
    } \
    Py_DECREF(__new_const); \
}

#define ADDOP_O(C, OP, O, TYPE) { \
    if (!compiler_addop_o((C), (OP), (C)->u->u_ ## TYPE, (O))) \
        return 0; \
}

/* Same as ADDOP_O, but steals a reference. */
#define ADDOP_N(C, OP, O, TYPE) { \
    if (!compiler_addop_o((C), (OP), (C)->u->u_ ## TYPE, (O))) { \
        Py_DECREF((O)); \
        return 0; \
    } \
    Py_DECREF((O)); \
}

#define ADDOP_NAME(C, OP, O, TYPE) { \
    if (!compiler_addop_name((C), (OP), (C)->u->u_ ## TYPE, (O))) \
        return 0; \
}

#define ADDOP_I(C, OP, O) { \
    if (!compiler_addop_i((C), (OP), (O))) \
        return 0; \
}

#define ADDOP_JUMP(C, OP, O) { \
    if (!compiler_addop_j((C), (OP), (O))) \
        return 0; \
}

#define ADDOP_COMPARE(C, CMP) { \
    if (!compiler_addcompare((C), (cmpop_ty)(CMP))) \
        return 0; \
}

/* VISIT and VISIT_SEQ takes an ASDL type as their second argument.  They use
   the ASDL name to synthesize the name of the C type and the visit function.
*/

#define VISIT(C, TYPE, V) {\
    if (!compiler_visit_ ## TYPE((C), (V))) \
        return 0; \
}

#define VISIT_IN_SCOPE(C, TYPE, V) {\
    if (!compiler_visit_ ## TYPE((C), (V))) { \
        compiler_exit_scope(c); \
        return 0; \
    } \
}

#define VISIT_SLICE(C, V, CTX) {\
    if (!compiler_visit_slice((C), (V), (CTX))) \
        return 0; \
}

#define VISIT_SEQ(C, TYPE, SEQ) { \
    int _i; \
    asdl_seq *seq = (SEQ); /* avoid variable capture */ \
    for (_i = 0; _i < asdl_seq_LEN(seq); _i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, _i); \
        if (!compiler_visit_ ## TYPE((C), elt)) \
            return 0; \
    } \
}

#define VISIT_SEQ_IN_SCOPE(C, TYPE, SEQ) { \
    int _i; \
    asdl_seq *seq = (SEQ); /* avoid variable capture */ \
    for (_i = 0; _i < asdl_seq_LEN(seq); _i++) { \
        TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, _i); \
        if (!compiler_visit_ ## TYPE((C), elt)) { \
            compiler_exit_scope(c); \
            return 0; \
        } \
    } \
}

/* These macros allows to check only for errors and not emmit bytecode
 * while visiting nodes.
*/

#define BEGIN_DO_NOT_EMIT_BYTECODE { \
    c->c_do_not_emit_bytecode++;

#define END_DO_NOT_EMIT_BYTECODE \
    c->c_do_not_emit_bytecode--; \
}

/* Search if variable annotations are present statically in a block. */

static int
find_ann(asdl_seq *stmts)
{
    int i, j, res = 0;
    stmt_ty st;

    for (i = 0; i < asdl_seq_LEN(stmts); i++) {
        st = (stmt_ty)asdl_seq_GET(stmts, i);
        switch (st->kind) {
        case AnnAssign_kind:
            return 1;
        case For_kind:
            res = find_ann(st->v.For.body) ||
                  find_ann(st->v.For.orelse);
            break;
        case AsyncFor_kind:
            res = find_ann(st->v.AsyncFor.body) ||
                  find_ann(st->v.AsyncFor.orelse);
            break;
        case While_kind:
            res = find_ann(st->v.While.body) ||
                  find_ann(st->v.While.orelse);
            break;
        case If_kind:
            res = find_ann(st->v.If.body) ||
                  find_ann(st->v.If.orelse);
            break;
        case With_kind:
            res = find_ann(st->v.With.body);
            break;
        case AsyncWith_kind:
            res = find_ann(st->v.AsyncWith.body);
            break;
        case Try_kind:
            for (j = 0; j < asdl_seq_LEN(st->v.Try.handlers); j++) {
                excepthandler_ty handler = (excepthandler_ty)asdl_seq_GET(
                    st->v.Try.handlers, j);
                if (find_ann(handler->v.ExceptHandler.body)) {
                    return 1;
                }
            }
            res = find_ann(st->v.Try.body) ||
                  find_ann(st->v.Try.finalbody) ||
                  find_ann(st->v.Try.orelse);
            break;
        default:
            res = 0;
        }
        if (res) {
            break;
        }
    }
    return res;
}

/*
 * Frame block handling functions
 */

static int
compiler_push_fblock(struct compiler *c, enum fblocktype t, basicblock *b,
                     basicblock *exit, void *datum)
{
    struct fblockinfo *f;
    if (c->u->u_nfblocks >= CO_MAXBLOCKS) {
        PyErr_SetString(PyExc_SyntaxError,
                        "too many statically nested blocks");
        return 0;
    }
    f = &c->u->u_fblock[c->u->u_nfblocks++];
    f->fb_type = t;
    f->fb_block = b;
    f->fb_exit = exit;
    f->fb_datum = datum;
    return 1;
}

static void
compiler_pop_fblock(struct compiler *c, enum fblocktype t, basicblock *b)
{
    struct compiler_unit *u = c->u;
    assert(u->u_nfblocks > 0);
    u->u_nfblocks--;
    assert(u->u_fblock[u->u_nfblocks].fb_type == t);
    assert(u->u_fblock[u->u_nfblocks].fb_block == b);
}

static int
compiler_call_exit_with_nones(struct compiler *c) {
    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    ADDOP(c, DUP_TOP);
    ADDOP(c, DUP_TOP);
    ADDOP_I(c, CALL_FUNCTION, 3);
    return 1;
}

/* Unwind a frame block.  If preserve_tos is true, the TOS before
 * popping the blocks will be restored afterwards, unless another
 * return, break or continue is found. In which case, the TOS will
 * be popped.
 */
static int
compiler_unwind_fblock(struct compiler *c, struct fblockinfo *info,
                       int preserve_tos)
{
    switch (info->fb_type) {
        case WHILE_LOOP:
            return 1;

        case FOR_LOOP:
            /* Pop the iterator */
            if (preserve_tos) {
                ADDOP(c, ROT_TWO);
            }
            ADDOP(c, POP_TOP);
            return 1;

        case EXCEPT:
            ADDOP(c, POP_BLOCK);
            return 1;

        case FINALLY_TRY:
            ADDOP(c, POP_BLOCK);
            if (preserve_tos) {
                if (!compiler_push_fblock(c, POP_VALUE, NULL, NULL, NULL)) {
                    return 0;
                }
            }
            /* Emit the finally block, restoring the line number when done */
            int saved_lineno = c->u->u_lineno;
            VISIT_SEQ(c, stmt, info->fb_datum);
            c->u->u_lineno = saved_lineno;
            if (preserve_tos) {
                compiler_pop_fblock(c, POP_VALUE, NULL);
            }
            return 1;

        case FINALLY_END:
            if (preserve_tos) {
                ADDOP(c, ROT_FOUR);
            }
            ADDOP(c, POP_TOP);
            ADDOP(c, POP_TOP);
            ADDOP(c, POP_TOP);
            if (preserve_tos) {
                ADDOP(c, ROT_FOUR);
            }
            ADDOP(c, POP_EXCEPT);
            return 1;

        case WITH:
        case ASYNC_WITH:
            ADDOP(c, POP_BLOCK);
            if (preserve_tos) {
                ADDOP(c, ROT_TWO);
            }
            if(!compiler_call_exit_with_nones(c)) {
                return 0;
            }
            if (info->fb_type == ASYNC_WITH) {
                ADDOP(c, GET_AWAITABLE);
                ADDOP_LOAD_CONST(c, Py_None);
                ADDOP(c, YIELD_FROM);
            }
            ADDOP(c, POP_TOP);
            return 1;

        case HANDLER_CLEANUP:
            if (info->fb_datum) {
                ADDOP(c, POP_BLOCK);
            }
            if (preserve_tos) {
                ADDOP(c, ROT_FOUR);
            }
            ADDOP(c, POP_EXCEPT);
            if (info->fb_datum) {
                ADDOP_LOAD_CONST(c, Py_None);
                compiler_nameop(c, info->fb_datum, Store);
                compiler_nameop(c, info->fb_datum, Del);
            }
            return 1;

        case POP_VALUE:
            if (preserve_tos) {
                ADDOP(c, ROT_TWO);
            }
            ADDOP(c, POP_TOP);
            return 1;
    }
    Py_UNREACHABLE();
}

/** Unwind block stack. If loop is not NULL, then stop when the first loop is encountered. */
static int
compiler_unwind_fblock_stack(struct compiler *c, int preserve_tos, struct fblockinfo **loop) {
    if (c->u->u_nfblocks == 0) {
        return 1;
    }
    struct fblockinfo *top = &c->u->u_fblock[c->u->u_nfblocks-1];
    if (loop != NULL && (top->fb_type == WHILE_LOOP || top->fb_type == FOR_LOOP)) {
        *loop = top;
        return 1;
    }
    struct fblockinfo copy = *top;
    c->u->u_nfblocks--;
    if (!compiler_unwind_fblock(c, &copy, preserve_tos)) {
        return 0;
    }
    if (!compiler_unwind_fblock_stack(c, preserve_tos, loop)) {
        return 0;
    }
    c->u->u_fblock[c->u->u_nfblocks] = copy;
    c->u->u_nfblocks++;
    return 1;
}

/* Compile a sequence of statements, checking for a docstring
   and for annotations. */

static int
compiler_body(struct compiler *c, asdl_seq *stmts)
{
    int i = 0;
    stmt_ty st;
    PyObject *docstring;

    /* Set current line number to the line number of first statement.
       This way line number for SETUP_ANNOTATIONS will always
       coincide with the line number of first "real" statement in module.
       If body is empty, then lineno will be set later in assemble. */
    if (c->u->u_scope_type == COMPILER_SCOPE_MODULE && asdl_seq_LEN(stmts)) {
        st = (stmt_ty)asdl_seq_GET(stmts, 0);
        SET_LOC(c, st);
    }
    /* Every annotated class and module should have __annotations__. */
    if (find_ann(stmts)) {
        ADDOP(c, SETUP_ANNOTATIONS);
    }
    if (!asdl_seq_LEN(stmts))
        return 1;
    /* if not -OO mode, set docstring */
    if (c->c_optimize < 2) {
        docstring = _PyAST_GetDocString(stmts);
        if (docstring) {
            i = 1;
            st = (stmt_ty)asdl_seq_GET(stmts, 0);
            assert(st->kind == Expr_kind);
            VISIT(c, expr, st->v.Expr.value);
            if (!compiler_nameop(c, __doc__, Store))
                return 0;
        }
    }
    for (; i < asdl_seq_LEN(stmts); i++)
        VISIT(c, stmt, (stmt_ty)asdl_seq_GET(stmts, i));
    return 1;
}

static PyCodeObject *
compiler_mod(struct compiler *c, mod_ty mod)
{
    PyCodeObject *co;
    int addNone = 1;
    static PyObject *module;
    if (!module) {
        module = PyUnicode_InternFromString("<module>");
        if (!module)
            return NULL;
    }
    /* Use 0 for firstlineno initially, will fixup in assemble(). */
    if (!compiler_enter_scope(c, module, COMPILER_SCOPE_MODULE, mod, 0))
        return NULL;
    switch (mod->kind) {
    case Module_kind:
        if (!compiler_body(c, mod->v.Module.body)) {
            compiler_exit_scope(c);
            return 0;
        }
        break;
    case Interactive_kind:
        if (find_ann(mod->v.Interactive.body)) {
            ADDOP(c, SETUP_ANNOTATIONS);
        }
        c->c_interactive = 1;
        VISIT_SEQ_IN_SCOPE(c, stmt,
                                mod->v.Interactive.body);
        break;
    case Expression_kind:
        VISIT_IN_SCOPE(c, expr, mod->v.Expression.body);
        addNone = 0;
        break;
    default:
        PyErr_Format(PyExc_SystemError,
                     "module kind %d should not be possible",
                     mod->kind);
        return 0;
    }
    co = assemble(c, addNone);
    compiler_exit_scope(c);
    return co;
}

/* The test for LOCAL must come before the test for FREE in order to
   handle classes where name is both local and free.  The local var is
   a method and the free var is a free var referenced within a method.
*/

static int
get_ref_type(struct compiler *c, PyObject *name)
{
    int scope;
    if (c->u->u_scope_type == COMPILER_SCOPE_CLASS &&
        _PyUnicode_EqualToASCIIString(name, "__class__"))
        return CELL;
    scope = PyST_GetScope(c->u->u_ste, name);
    if (scope == 0) {
        _Py_FatalErrorFormat(__func__,
           "unknown scope for %.100s in %.100s(%s)\n"
           "symbols: %s\nlocals: %s\nglobals: %s",
           PyUnicode_AsUTF8(name),
           PyUnicode_AsUTF8(c->u->u_name),
           PyUnicode_AsUTF8(PyObject_Repr(c->u->u_ste->ste_id)),
           PyUnicode_AsUTF8(PyObject_Repr(c->u->u_ste->ste_symbols)),
           PyUnicode_AsUTF8(PyObject_Repr(c->u->u_varnames)),
           PyUnicode_AsUTF8(PyObject_Repr(c->u->u_names)));
    }

    return scope;
}

static int
compiler_lookup_arg(PyObject *dict, PyObject *name)
{
    PyObject *v;
    v = PyDict_GetItem(dict, name);
    if (v == NULL)
        return -1;
    return PyLong_AS_LONG(v);
}

static int
compiler_make_closure(struct compiler *c, PyCodeObject *co, Py_ssize_t flags, PyObject *qualname)
{
    Py_ssize_t i, free = PyCode_GetNumFree(co);
    if (qualname == NULL)
        qualname = co->co_name;

    if (free) {
        for (i = 0; i < free; ++i) {
            /* Bypass com_addop_varname because it will generate
               LOAD_DEREF but LOAD_CLOSURE is needed.
            */
            PyObject *name = PyTuple_GET_ITEM(co->co_freevars, i);
            int arg, reftype;

            /* Special case: If a class contains a method with a
               free variable that has the same name as a method,
               the name will be considered free *and* local in the
               class.  It should be handled by the closure, as
               well as by the normal name lookup logic.
            */
            reftype = get_ref_type(c, name);
            if (reftype == CELL)
                arg = compiler_lookup_arg(c->u->u_cellvars, name);
            else /* (reftype == FREE) */
                arg = compiler_lookup_arg(c->u->u_freevars, name);
            if (arg == -1) {
                _Py_FatalErrorFormat(__func__,
                    "lookup %s in %s %d %d\n"
                    "freevars of %s: %s\n",
                    PyUnicode_AsUTF8(PyObject_Repr(name)),
                    PyUnicode_AsUTF8(c->u->u_name),
                    reftype, arg,
                    PyUnicode_AsUTF8(co->co_name),
                    PyUnicode_AsUTF8(PyObject_Repr(co->co_freevars)));
            }
            ADDOP_I(c, LOAD_CLOSURE, arg);
        }
        flags |= 0x08;
        ADDOP_I(c, BUILD_TUPLE, free);
    }
    ADDOP_LOAD_CONST(c, (PyObject*)co);
    ADDOP_LOAD_CONST(c, qualname);
    ADDOP_I(c, MAKE_FUNCTION, flags);
    return 1;
}

static int
compiler_decorators(struct compiler *c, asdl_seq* decos)
{
    int i;

    if (!decos)
        return 1;

    for (i = 0; i < asdl_seq_LEN(decos); i++) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(decos, i));
    }
    return 1;
}

static int
compiler_visit_kwonlydefaults(struct compiler *c, asdl_seq *kwonlyargs,
                              asdl_seq *kw_defaults)
{
    /* Push a dict of keyword-only default values.

       Return 0 on error, -1 if no dict pushed, 1 if a dict is pushed.
       */
    int i;
    PyObject *keys = NULL;

    for (i = 0; i < asdl_seq_LEN(kwonlyargs); i++) {
        arg_ty arg = asdl_seq_GET(kwonlyargs, i);
        expr_ty default_ = asdl_seq_GET(kw_defaults, i);
        if (default_) {
            PyObject *mangled = _Py_Mangle(c->u->u_private, arg->arg);
            if (!mangled) {
                goto error;
            }
            if (keys == NULL) {
                keys = PyList_New(1);
                if (keys == NULL) {
                    Py_DECREF(mangled);
                    return 0;
                }
                PyList_SET_ITEM(keys, 0, mangled);
            }
            else {
                int res = PyList_Append(keys, mangled);
                Py_DECREF(mangled);
                if (res == -1) {
                    goto error;
                }
            }
            if (!compiler_visit_expr(c, default_)) {
                goto error;
            }
        }
    }
    if (keys != NULL) {
        Py_ssize_t default_count = PyList_GET_SIZE(keys);
        PyObject *keys_tuple = PyList_AsTuple(keys);
        Py_DECREF(keys);
        ADDOP_LOAD_CONST_NEW(c, keys_tuple);
        ADDOP_I(c, BUILD_CONST_KEY_MAP, default_count);
        assert(default_count > 0);
        return 1;
    }
    else {
        return -1;
    }

error:
    Py_XDECREF(keys);
    return 0;
}

static int
compiler_visit_annexpr(struct compiler *c, expr_ty annotation)
{
    ADDOP_LOAD_CONST_NEW(c, _PyAST_ExprAsUnicode(annotation));
    return 1;
}

static int
compiler_visit_argannotation(struct compiler *c, identifier id,
    expr_ty annotation, PyObject *names)
{
    if (annotation) {
        PyObject *mangled;
        if (c->c_future->ff_features & CO_FUTURE_ANNOTATIONS) {
            VISIT(c, annexpr, annotation)
        }
        else {
            VISIT(c, expr, annotation);
        }
        mangled = _Py_Mangle(c->u->u_private, id);
        if (!mangled)
            return 0;
        if (PyList_Append(names, mangled) < 0) {
            Py_DECREF(mangled);
            return 0;
        }
        Py_DECREF(mangled);
    }
    return 1;
}

static int
compiler_visit_argannotations(struct compiler *c, asdl_seq* args,
                              PyObject *names)
{
    int i;
    for (i = 0; i < asdl_seq_LEN(args); i++) {
        arg_ty arg = (arg_ty)asdl_seq_GET(args, i);
        if (!compiler_visit_argannotation(
                        c,
                        arg->arg,
                        arg->annotation,
                        names))
            return 0;
    }
    return 1;
}

static int
compiler_visit_annotations(struct compiler *c, arguments_ty args,
                           expr_ty returns)
{
    /* Push arg annotation dict.
       The expressions are evaluated out-of-order wrt the source code.

       Return 0 on error, -1 if no dict pushed, 1 if a dict is pushed.
       */
    static identifier return_str;
    PyObject *names;
    Py_ssize_t len;
    names = PyList_New(0);
    if (!names)
        return 0;

    if (!compiler_visit_argannotations(c, args->args, names))
        goto error;
    if (!compiler_visit_argannotations(c, args->posonlyargs, names))
        goto error;
    if (args->vararg && args->vararg->annotation &&
        !compiler_visit_argannotation(c, args->vararg->arg,
                                     args->vararg->annotation, names))
        goto error;
    if (!compiler_visit_argannotations(c, args->kwonlyargs, names))
        goto error;
    if (args->kwarg && args->kwarg->annotation &&
        !compiler_visit_argannotation(c, args->kwarg->arg,
                                     args->kwarg->annotation, names))
        goto error;

    if (!return_str) {
        return_str = PyUnicode_InternFromString("return");
        if (!return_str)
            goto error;
    }
    if (!compiler_visit_argannotation(c, return_str, returns, names)) {
        goto error;
    }

    len = PyList_GET_SIZE(names);
    if (len) {
        PyObject *keytuple = PyList_AsTuple(names);
        Py_DECREF(names);
        ADDOP_LOAD_CONST_NEW(c, keytuple);
        ADDOP_I(c, BUILD_CONST_KEY_MAP, len);
        return 1;
    }
    else {
        Py_DECREF(names);
        return -1;
    }

error:
    Py_DECREF(names);
    return 0;
}

static int
compiler_visit_defaults(struct compiler *c, arguments_ty args)
{
    VISIT_SEQ(c, expr, args->defaults);
    ADDOP_I(c, BUILD_TUPLE, asdl_seq_LEN(args->defaults));
    return 1;
}

static Py_ssize_t
compiler_default_arguments(struct compiler *c, arguments_ty args)
{
    Py_ssize_t funcflags = 0;
    if (args->defaults && asdl_seq_LEN(args->defaults) > 0) {
        if (!compiler_visit_defaults(c, args))
            return -1;
        funcflags |= 0x01;
    }
    if (args->kwonlyargs) {
        int res = compiler_visit_kwonlydefaults(c, args->kwonlyargs,
                                                args->kw_defaults);
        if (res == 0) {
            return -1;
        }
        else if (res > 0) {
            funcflags |= 0x02;
        }
    }
    return funcflags;
}

static int
forbidden_name(struct compiler *c, identifier name, expr_context_ty ctx)
{

    if (ctx == Store && _PyUnicode_EqualToASCIIString(name, "__debug__")) {
        compiler_error(c, "cannot assign to __debug__");
        return 1;
    }
    return 0;
}

static int
compiler_check_debug_one_arg(struct compiler *c, arg_ty arg)
{
    if (arg != NULL) {
        if (forbidden_name(c, arg->arg, Store))
            return 0;
    }
    return 1;
}

static int
compiler_check_debug_args_seq(struct compiler *c, asdl_seq *args)
{
    if (args != NULL) {
        for (Py_ssize_t i = 0, n = asdl_seq_LEN(args); i < n; i++) {
            if (!compiler_check_debug_one_arg(c, asdl_seq_GET(args, i)))
                return 0;
        }
    }
    return 1;
}

static int
compiler_check_debug_args(struct compiler *c, arguments_ty args)
{
    if (!compiler_check_debug_args_seq(c, args->posonlyargs))
        return 0;
    if (!compiler_check_debug_args_seq(c, args->args))
        return 0;
    if (!compiler_check_debug_one_arg(c, args->vararg))
        return 0;
    if (!compiler_check_debug_args_seq(c, args->kwonlyargs))
        return 0;
    if (!compiler_check_debug_one_arg(c, args->kwarg))
        return 0;
    return 1;
}

static int
compiler_function(struct compiler *c, stmt_ty s, int is_async)
{
    PyCodeObject *co;
    PyObject *qualname, *docstring = NULL;
    arguments_ty args;
    expr_ty returns;
    identifier name;
    asdl_seq* decos;
    asdl_seq *body;
    Py_ssize_t i, funcflags;
    int annotations;
    int scope_type;
    int firstlineno;

    if (is_async) {
        assert(s->kind == AsyncFunctionDef_kind);

        args = s->v.AsyncFunctionDef.args;
        returns = s->v.AsyncFunctionDef.returns;
        decos = s->v.AsyncFunctionDef.decorator_list;
        name = s->v.AsyncFunctionDef.name;
        body = s->v.AsyncFunctionDef.body;

        scope_type = COMPILER_SCOPE_ASYNC_FUNCTION;
    } else {
        assert(s->kind == FunctionDef_kind);

        args = s->v.FunctionDef.args;
        returns = s->v.FunctionDef.returns;
        decos = s->v.FunctionDef.decorator_list;
        name = s->v.FunctionDef.name;
        body = s->v.FunctionDef.body;

        scope_type = COMPILER_SCOPE_FUNCTION;
    }

    if (!compiler_check_debug_args(c, args))
        return 0;

    if (!compiler_decorators(c, decos))
        return 0;

    firstlineno = s->lineno;
    if (asdl_seq_LEN(decos)) {
        firstlineno = ((expr_ty)asdl_seq_GET(decos, 0))->lineno;
    }

    funcflags = compiler_default_arguments(c, args);
    if (funcflags == -1) {
        return 0;
    }

    annotations = compiler_visit_annotations(c, args, returns);
    if (annotations == 0) {
        return 0;
    }
    else if (annotations > 0) {
        funcflags |= 0x04;
    }

    if (!compiler_enter_scope(c, name, scope_type, (void *)s, firstlineno)) {
        return 0;
    }

    /* if not -OO mode, add docstring */
    if (c->c_optimize < 2) {
        docstring = _PyAST_GetDocString(body);
    }
    if (compiler_add_const(c, docstring ? docstring : Py_None) < 0) {
        compiler_exit_scope(c);
        return 0;
    }

    c->u->u_argcount = asdl_seq_LEN(args->args);
    c->u->u_posonlyargcount = asdl_seq_LEN(args->posonlyargs);
    c->u->u_kwonlyargcount = asdl_seq_LEN(args->kwonlyargs);
    VISIT_SEQ_IN_SCOPE(c, stmt, body);
    co = assemble(c, 1);
    qualname = c->u->u_qualname;
    Py_INCREF(qualname);
    compiler_exit_scope(c);
    if (co == NULL) {
        Py_XDECREF(qualname);
        Py_XDECREF(co);
        return 0;
    }

    compiler_make_closure(c, co, funcflags, qualname);
    Py_DECREF(qualname);
    Py_DECREF(co);

    /* decorators */
    for (i = 0; i < asdl_seq_LEN(decos); i++) {
        ADDOP_I(c, CALL_FUNCTION, 1);
    }

    return compiler_nameop(c, name, Store);
}

static int
compiler_class(struct compiler *c, stmt_ty s)
{
    PyCodeObject *co;
    PyObject *str;
    int i, firstlineno;
    asdl_seq* decos = s->v.ClassDef.decorator_list;

    if (!compiler_decorators(c, decos))
        return 0;

    firstlineno = s->lineno;
    if (asdl_seq_LEN(decos)) {
        firstlineno = ((expr_ty)asdl_seq_GET(decos, 0))->lineno;
    }

    /* ultimately generate code for:
         <name> = __build_class__(<func>, <name>, *<bases>, **<keywords>)
       where:
         <func> is a function/closure created from the class body;
            it has a single argument (__locals__) where the dict
            (or MutableSequence) representing the locals is passed
         <name> is the class name
         <bases> is the positional arguments and *varargs argument
         <keywords> is the keyword arguments and **kwds argument
       This borrows from compiler_call.
    */

    /* 1. compile the class body into a code object */
    if (!compiler_enter_scope(c, s->v.ClassDef.name,
                              COMPILER_SCOPE_CLASS, (void *)s, firstlineno))
        return 0;
    /* this block represents what we do in the new scope */
    {
        /* use the class name for name mangling */
        Py_INCREF(s->v.ClassDef.name);
        Py_XSETREF(c->u->u_private, s->v.ClassDef.name);
        /* load (global) __name__ ... */
        str = PyUnicode_InternFromString("__name__");
        if (!str || !compiler_nameop(c, str, Load)) {
            Py_XDECREF(str);
            compiler_exit_scope(c);
            return 0;
        }
        Py_DECREF(str);
        /* ... and store it as __module__ */
        str = PyUnicode_InternFromString("__module__");
        if (!str || !compiler_nameop(c, str, Store)) {
            Py_XDECREF(str);
            compiler_exit_scope(c);
            return 0;
        }
        Py_DECREF(str);
        assert(c->u->u_qualname);
        ADDOP_LOAD_CONST(c, c->u->u_qualname);
        str = PyUnicode_InternFromString("__qualname__");
        if (!str || !compiler_nameop(c, str, Store)) {
            Py_XDECREF(str);
            compiler_exit_scope(c);
            return 0;
        }
        Py_DECREF(str);
        /* compile the body proper */
        if (!compiler_body(c, s->v.ClassDef.body)) {
            compiler_exit_scope(c);
            return 0;
        }
        /* Return __classcell__ if it is referenced, otherwise return None */
        if (c->u->u_ste->ste_needs_class_closure) {
            /* Store __classcell__ into class namespace & return it */
            str = PyUnicode_InternFromString("__class__");
            if (str == NULL) {
                compiler_exit_scope(c);
                return 0;
            }
            i = compiler_lookup_arg(c->u->u_cellvars, str);
            Py_DECREF(str);
            if (i < 0) {
                compiler_exit_scope(c);
                return 0;
            }
            assert(i == 0);

            ADDOP_I(c, LOAD_CLOSURE, i);
            ADDOP(c, DUP_TOP);
            str = PyUnicode_InternFromString("__classcell__");
            if (!str || !compiler_nameop(c, str, Store)) {
                Py_XDECREF(str);
                compiler_exit_scope(c);
                return 0;
            }
            Py_DECREF(str);
        }
        else {
            /* No methods referenced __class__, so just return None */
            assert(PyDict_GET_SIZE(c->u->u_cellvars) == 0);
            ADDOP_LOAD_CONST(c, Py_None);
        }
        ADDOP_IN_SCOPE(c, RETURN_VALUE);
        /* create the code object */
        co = assemble(c, 1);
    }
    /* leave the new scope */
    compiler_exit_scope(c);
    if (co == NULL)
        return 0;

    /* 2. load the 'build_class' function */
    ADDOP(c, LOAD_BUILD_CLASS);

    /* 3. load a function (or closure) made from the code object */
    compiler_make_closure(c, co, 0, NULL);
    Py_DECREF(co);

    /* 4. load class name */
    ADDOP_LOAD_CONST(c, s->v.ClassDef.name);

    /* 5. generate the rest of the code for the call */
    if (!compiler_call_helper(c, 2,
                              s->v.ClassDef.bases,
                              s->v.ClassDef.keywords))
        return 0;

    /* 6. apply decorators */
    for (i = 0; i < asdl_seq_LEN(decos); i++) {
        ADDOP_I(c, CALL_FUNCTION, 1);
    }

    /* 7. store into <name> */
    if (!compiler_nameop(c, s->v.ClassDef.name, Store))
        return 0;
    return 1;
}

/* Return 0 if the expression is a constant value except named singletons.
   Return 1 otherwise. */
static int
check_is_arg(expr_ty e)
{
    if (e->kind != Constant_kind) {
        return 1;
    }
    PyObject *value = e->v.Constant.value;
    return (value == Py_None
         || value == Py_False
         || value == Py_True
         || value == Py_Ellipsis);
}

/* Check operands of identity chacks ("is" and "is not").
   Emit a warning if any operand is a constant except named singletons.
   Return 0 on error.
 */
static int
check_compare(struct compiler *c, expr_ty e)
{
    Py_ssize_t i, n;
    int left = check_is_arg(e->v.Compare.left);
    n = asdl_seq_LEN(e->v.Compare.ops);
    for (i = 0; i < n; i++) {
        cmpop_ty op = (cmpop_ty)asdl_seq_GET(e->v.Compare.ops, i);
        int right = check_is_arg((expr_ty)asdl_seq_GET(e->v.Compare.comparators, i));
        if (op == Is || op == IsNot) {
            if (!right || !left) {
                const char *msg = (op == Is)
                        ? "\"is\" with a literal. Did you mean \"==\"?"
                        : "\"is not\" with a literal. Did you mean \"!=\"?";
                return compiler_warn(c, msg);
            }
        }
        left = right;
    }
    return 1;
}

static int compiler_addcompare(struct compiler *c, cmpop_ty op)
{
    int cmp;
    switch (op) {
    case Eq:
        cmp = Py_EQ;
        break;
    case NotEq:
        cmp = Py_NE;
        break;
    case Lt:
        cmp = Py_LT;
        break;
    case LtE:
        cmp = Py_LE;
        break;
    case Gt:
        cmp = Py_GT;
        break;
    case GtE:
        cmp = Py_GE;
        break;
    case Is:
        ADDOP_I(c, IS_OP, 0);
        return 1;
    case IsNot:
        ADDOP_I(c, IS_OP, 1);
        return 1;
    case In:
        ADDOP_I(c, CONTAINS_OP, 0);
        return 1;
    case NotIn:
        ADDOP_I(c, CONTAINS_OP, 1);
        return 1;
    default:
        Py_UNREACHABLE();
    }
    ADDOP_I(c, COMPARE_OP, cmp);
    return 1;
}



static int
compiler_jump_if(struct compiler *c, expr_ty e, basicblock *next, int cond)
{
    switch (e->kind) {
    case UnaryOp_kind:
        if (e->v.UnaryOp.op == Not)
            return compiler_jump_if(c, e->v.UnaryOp.operand, next, !cond);
        /* fallback to general implementation */
        break;
    case BoolOp_kind: {
        asdl_seq *s = e->v.BoolOp.values;
        Py_ssize_t i, n = asdl_seq_LEN(s) - 1;
        assert(n >= 0);
        int cond2 = e->v.BoolOp.op == Or;
        basicblock *next2 = next;
        if (!cond2 != !cond) {
            next2 = compiler_new_block(c);
            if (next2 == NULL)
                return 0;
        }
        for (i = 0; i < n; ++i) {
            if (!compiler_jump_if(c, (expr_ty)asdl_seq_GET(s, i), next2, cond2))
                return 0;
        }
        if (!compiler_jump_if(c, (expr_ty)asdl_seq_GET(s, n), next, cond))
            return 0;
        if (next2 != next)
            compiler_use_next_block(c, next2);
        return 1;
    }
    case IfExp_kind: {
        basicblock *end, *next2;
        end = compiler_new_block(c);
        if (end == NULL)
            return 0;
        next2 = compiler_new_block(c);
        if (next2 == NULL)
            return 0;
        if (!compiler_jump_if(c, e->v.IfExp.test, next2, 0))
            return 0;
        if (!compiler_jump_if(c, e->v.IfExp.body, next, cond))
            return 0;
        ADDOP_JUMP(c, JUMP_FORWARD, end);
        compiler_use_next_block(c, next2);
        if (!compiler_jump_if(c, e->v.IfExp.orelse, next, cond))
            return 0;
        compiler_use_next_block(c, end);
        return 1;
    }
    case Compare_kind: {
        Py_ssize_t i, n = asdl_seq_LEN(e->v.Compare.ops) - 1;
        if (n > 0) {
            if (!check_compare(c, e)) {
                return 0;
            }
            basicblock *cleanup = compiler_new_block(c);
            if (cleanup == NULL)
                return 0;
            VISIT(c, expr, e->v.Compare.left);
            for (i = 0; i < n; i++) {
                VISIT(c, expr,
                    (expr_ty)asdl_seq_GET(e->v.Compare.comparators, i));
                ADDOP(c, DUP_TOP);
                ADDOP(c, ROT_THREE);
                ADDOP_COMPARE(c, asdl_seq_GET(e->v.Compare.ops, i));
                ADDOP_JUMP(c, POP_JUMP_IF_FALSE, cleanup);
                NEXT_BLOCK(c);
            }
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Compare.comparators, n));
            ADDOP_COMPARE(c, asdl_seq_GET(e->v.Compare.ops, n));
            ADDOP_JUMP(c, cond ? POP_JUMP_IF_TRUE : POP_JUMP_IF_FALSE, next);
            basicblock *end = compiler_new_block(c);
            if (end == NULL)
                return 0;
            ADDOP_JUMP(c, JUMP_FORWARD, end);
            compiler_use_next_block(c, cleanup);
            ADDOP(c, POP_TOP);
            if (!cond) {
                ADDOP_JUMP(c, JUMP_FORWARD, next);
            }
            compiler_use_next_block(c, end);
            return 1;
        }
        /* fallback to general implementation */
        break;
    }
    default:
        /* fallback to general implementation */
        break;
    }

    /* general implementation */
    VISIT(c, expr, e);
    ADDOP_JUMP(c, cond ? POP_JUMP_IF_TRUE : POP_JUMP_IF_FALSE, next);
    return 1;
}

static int
compiler_ifexp(struct compiler *c, expr_ty e)
{
    basicblock *end, *next;

    assert(e->kind == IfExp_kind);
    end = compiler_new_block(c);
    if (end == NULL)
        return 0;
    next = compiler_new_block(c);
    if (next == NULL)
        return 0;
    if (!compiler_jump_if(c, e->v.IfExp.test, next, 0))
        return 0;
    VISIT(c, expr, e->v.IfExp.body);
    ADDOP_JUMP(c, JUMP_FORWARD, end);
    compiler_use_next_block(c, next);
    VISIT(c, expr, e->v.IfExp.orelse);
    compiler_use_next_block(c, end);
    return 1;
}

static int
compiler_lambda(struct compiler *c, expr_ty e)
{
    PyCodeObject *co;
    PyObject *qualname;
    static identifier name;
    Py_ssize_t funcflags;
    arguments_ty args = e->v.Lambda.args;
    assert(e->kind == Lambda_kind);

    if (!compiler_check_debug_args(c, args))
        return 0;

    if (!name) {
        name = PyUnicode_InternFromString("<lambda>");
        if (!name)
            return 0;
    }

    funcflags = compiler_default_arguments(c, args);
    if (funcflags == -1) {
        return 0;
    }

    if (!compiler_enter_scope(c, name, COMPILER_SCOPE_LAMBDA,
                              (void *)e, e->lineno))
        return 0;

    /* Make None the first constant, so the lambda can't have a
       docstring. */
    if (compiler_add_const(c, Py_None) < 0)
        return 0;

    c->u->u_argcount = asdl_seq_LEN(args->args);
    c->u->u_posonlyargcount = asdl_seq_LEN(args->posonlyargs);
    c->u->u_kwonlyargcount = asdl_seq_LEN(args->kwonlyargs);
    VISIT_IN_SCOPE(c, expr, e->v.Lambda.body);
    if (c->u->u_ste->ste_generator) {
        co = assemble(c, 0);
    }
    else {
        ADDOP_IN_SCOPE(c, RETURN_VALUE);
        co = assemble(c, 1);
    }
    qualname = c->u->u_qualname;
    Py_INCREF(qualname);
    compiler_exit_scope(c);
    if (co == NULL)
        return 0;

    compiler_make_closure(c, co, funcflags, qualname);
    Py_DECREF(qualname);
    Py_DECREF(co);

    return 1;
}

static int
compiler_if(struct compiler *c, stmt_ty s)
{
    basicblock *end, *next;
    int constant;
    assert(s->kind == If_kind);
    end = compiler_new_block(c);
    if (end == NULL)
        return 0;

    constant = expr_constant(s->v.If.test);
    /* constant = 0: "if 0"
     * constant = 1: "if 1", "if 2", ...
     * constant = -1: rest */
    if (constant == 0) {
        BEGIN_DO_NOT_EMIT_BYTECODE
        VISIT_SEQ(c, stmt, s->v.If.body);
        END_DO_NOT_EMIT_BYTECODE
        if (s->v.If.orelse) {
            VISIT_SEQ(c, stmt, s->v.If.orelse);
        }
    } else if (constant == 1) {
        VISIT_SEQ(c, stmt, s->v.If.body);
        if (s->v.If.orelse) {
            BEGIN_DO_NOT_EMIT_BYTECODE
            VISIT_SEQ(c, stmt, s->v.If.orelse);
            END_DO_NOT_EMIT_BYTECODE
        }
    } else {
        if (asdl_seq_LEN(s->v.If.orelse)) {
            next = compiler_new_block(c);
            if (next == NULL)
                return 0;
        }
        else {
            next = end;
        }
        if (!compiler_jump_if(c, s->v.If.test, next, 0)) {
            return 0;
        }
        VISIT_SEQ(c, stmt, s->v.If.body);
        if (asdl_seq_LEN(s->v.If.orelse)) {
            ADDOP_JUMP(c, JUMP_FORWARD, end);
            compiler_use_next_block(c, next);
            VISIT_SEQ(c, stmt, s->v.If.orelse);
        }
    }
    compiler_use_next_block(c, end);
    return 1;
}

static int
compiler_for(struct compiler *c, stmt_ty s)
{
    basicblock *start, *cleanup, *end;

    start = compiler_new_block(c);
    cleanup = compiler_new_block(c);
    end = compiler_new_block(c);
    if (start == NULL || end == NULL || cleanup == NULL) {
        return 0;
    }
    if (!compiler_push_fblock(c, FOR_LOOP, start, end, NULL)) {
        return 0;
    }
    VISIT(c, expr, s->v.For.iter);
    ADDOP(c, GET_ITER);
    compiler_use_next_block(c, start);
    ADDOP_JUMP(c, FOR_ITER, cleanup);
    VISIT(c, expr, s->v.For.target);
    VISIT_SEQ(c, stmt, s->v.For.body);
    ADDOP_JUMP(c, JUMP_ABSOLUTE, start);
    compiler_use_next_block(c, cleanup);

    compiler_pop_fblock(c, FOR_LOOP, start);

    VISIT_SEQ(c, stmt, s->v.For.orelse);
    compiler_use_next_block(c, end);
    return 1;
}


static int
compiler_async_for(struct compiler *c, stmt_ty s)
{
    basicblock *start, *except, *end;
    if (IS_TOP_LEVEL_AWAIT(c)){
        c->u->u_ste->ste_coroutine = 1;
    } else if (c->u->u_scope_type != COMPILER_SCOPE_ASYNC_FUNCTION) {
        return compiler_error(c, "'async for' outside async function");
    }

    start = compiler_new_block(c);
    except = compiler_new_block(c);
    end = compiler_new_block(c);

    if (start == NULL || except == NULL || end == NULL) {
        return 0;
    }
    VISIT(c, expr, s->v.AsyncFor.iter);
    ADDOP(c, GET_AITER);

    compiler_use_next_block(c, start);
    if (!compiler_push_fblock(c, FOR_LOOP, start, end, NULL)) {
        return 0;
    }
    /* SETUP_FINALLY to guard the __anext__ call */
    ADDOP_JUMP(c, SETUP_FINALLY, except);
    ADDOP(c, GET_ANEXT);
    ADDOP_LOAD_CONST(c, Py_None);
    ADDOP(c, YIELD_FROM);
    ADDOP(c, POP_BLOCK);  /* for SETUP_FINALLY */

    /* Success block for __anext__ */
    VISIT(c, expr, s->v.AsyncFor.target);
    VISIT_SEQ(c, stmt, s->v.AsyncFor.body);
    ADDOP_JUMP(c, JUMP_ABSOLUTE, start);

    compiler_pop_fblock(c, FOR_LOOP, start);

    /* Except block for __anext__ */
    compiler_use_next_block(c, except);
    ADDOP(c, END_ASYNC_FOR);

    /* `else` block */
    VISIT_SEQ(c, stmt, s->v.For.orelse);

    compiler_use_next_block(c, end);

    return 1;
}

static int
compiler_while(struct compiler *c, stmt_ty s)
{
    basicblock *loop, *orelse, *end, *anchor = NULL;
    int constant = expr_constant(s->v.While.test);

    if (constant == 0) {
        BEGIN_DO_NOT_EMIT_BYTECODE
        // Push a dummy block so the VISIT_SEQ knows that we are
        // inside a while loop so it can correctly evaluate syntax
        // errors.
        if (!compiler_push_fblock(c, WHILE_LOOP, NULL, NULL, NULL)) {
            return 0;
        }
        VISIT_SEQ(c, stmt, s->v.While.body);
        // Remove the dummy block now that is not needed.
        compiler_pop_fblock(c, WHILE_LOOP, NULL);
        END_DO_NOT_EMIT_BYTECODE
        if (s->v.While.orelse) {
            VISIT_SEQ(c, stmt, s->v.While.orelse);
        }
        return 1;
    }
    loop = compiler_new_block(c);
    end = compiler_new_block(c);
    if (constant == -1) {
        anchor = compiler_new_block(c);
        if (anchor == NULL)
            return 0;
    }
    if (loop == NULL || end == NULL)
        return 0;
    if (s->v.While.orelse) {
        orelse = compiler_new_block(c);
        if (orelse == NULL)
            return 0;
    }
    else
        orelse = NULL;

    compiler_use_next_block(c, loop);
    if (!compiler_push_fblock(c, WHILE_LOOP, loop, end, NULL))
        return 0;
    if (constant == -1) {
        if (!compiler_jump_if(c, s->v.While.test, anchor, 0))
            return 0;
    }
    VISIT_SEQ(c, stmt, s->v.While.body);
    ADDOP_JUMP(c, JUMP_ABSOLUTE, loop);

    /* XXX should the two POP instructions be in a separate block
       if there is no else clause ?
    */

    if (constant == -1)
        compiler_use_next_block(c, anchor);
    compiler_pop_fblock(c, WHILE_LOOP, loop);

    if (orelse != NULL) /* what if orelse is just pass? */
        VISIT_SEQ(c, stmt, s->v.While.orelse);
    compiler_use_next_block(c, end);

    return 1;
}

static int
compiler_return(struct compiler *c, stmt_ty s)
{
    int preserve_tos = ((s->v.Return.value != NULL) &&
                        (s->v.Return.value->kind != Constant_kind));
    if (c->u->u_ste->ste_type != FunctionBlock)
        return compiler_error(c, "'return' outside function");
    if (s->v.Return.value != NULL &&
        c->u->u_ste->ste_coroutine && c->u->u_ste->ste_generator)
    {
            return compiler_error(
                c, "'return' with value in async generator");
    }
    if (preserve_tos) {
        VISIT(c, expr, s->v.Return.value);
    }
    if (!compiler_unwind_fblock_stack(c, preserve_tos, NULL))
        return 0;
    if (s->v.Return.value == NULL) {
        ADDOP_LOAD_CONST(c, Py_None);
    }
    else if (!preserve_tos) {
        VISIT(c, expr, s->v.Return.value);
    }
    ADDOP(c, RETURN_VALUE);

    return 1;
}

static int
compiler_break(struct compiler *c)
{
    struct fblockinfo *loop = NULL;
    if (!compiler_unwind_fblock_stack(c, 0, &loop)) {
        return 0;
    }
    if (loop == NULL) {
        return compiler_error(c, "'break' outside loop");
    }
    if (!compiler_unwind_fblock(c, loop, 0)) {
        return 0;
    }
    ADDOP_JUMP(c, JUMP_ABSOLUTE, loop->fb_exit);
    return 1;
}

static int
compiler_continue(struct compiler *c)
{
    struct fblockinfo *loop = NULL;
    if (!compiler_unwind_fblock_stack(c, 0, &loop)) {
        return 0;
    }
    if (loop == NULL) {
        return compiler_error(c, "'continue' not properly in loop");
    }
    ADDOP_JUMP(c, JUMP_ABSOLUTE, loop->fb_block);
    return 1;
}


/* Code generated for "try: <body> finally: <finalbody>" is as follows:

        SETUP_FINALLY           L
        <code for body>
        POP_BLOCK
        <code for finalbody>
        JUMP E
    L:
        <code for finalbody>
    E:

   The special instructions use the block stack.  Each block
   stack entry contains the instruction that created it (here
   SETUP_FINALLY), the level of the value stack at the time the
   block stack entry was created, and a label (here L).

   SETUP_FINALLY:
    Pushes the current value stack level and the label
    onto the block stack.
   POP_BLOCK:
    Pops en entry from the block stack.

   The block stack is unwound when an exception is raised:
   when a SETUP_FINALLY entry is found, the raised and the caught
   exceptions are pushed onto the value stack (and the exception
   condition is cleared), and the interpreter jumps to the label
   gotten from the block stack.
*/

static int
compiler_try_finally(struct compiler *c, stmt_ty s)
{
    basicblock *body, *end, *exit;

    body = compiler_new_block(c);
    end = compiler_new_block(c);
    exit = compiler_new_block(c);
    if (body == NULL || end == NULL || exit == NULL)
        return 0;

    /* `try` block */
    ADDOP_JUMP(c, SETUP_FINALLY, end);
    compiler_use_next_block(c, body);
    if (!compiler_push_fblock(c, FINALLY_TRY, body, end, s->v.Try.finalbody))
        return 0;
    if (s->v.Try.handlers && asdl_seq_LEN(s->v.Try.handlers)) {
        if (!compiler_try_except(c, s))
            return 0;
    }
    else {
        VISIT_SEQ(c, stmt, s->v.Try.body);
    }
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, FINALLY_TRY, body);
    VISIT_SEQ(c, stmt, s->v.Try.finalbody);
    ADDOP_JUMP(c, JUMP_FORWARD, exit);
    /* `finally` block */
    compiler_use_next_block(c, end);
    if (!compiler_push_fblock(c, FINALLY_END, end, NULL, NULL))
        return 0;
    VISIT_SEQ(c, stmt, s->v.Try.finalbody);
    compiler_pop_fblock(c, FINALLY_END, end);
    ADDOP(c, RERAISE);
    compiler_use_next_block(c, exit);
    return 1;
}

/*
   Code generated for "try: S except E1 as V1: S1 except E2 as V2: S2 ...":
   (The contents of the value stack is shown in [], with the top
   at the right; 'tb' is trace-back info, 'val' the exception's
   associated value, and 'exc' the exception.)

   Value stack          Label   Instruction     Argument
   []                           SETUP_FINALLY   L1
   []                           <code for S>
   []                           POP_BLOCK
   []                           JUMP_FORWARD    L0

   [tb, val, exc]       L1:     DUP                             )
   [tb, val, exc, exc]          <evaluate E1>                   )
   [tb, val, exc, exc, E1]      JUMP_IF_NOT_EXC_MATCH L2        ) only if E1
   [tb, val, exc]               POP
   [tb, val]                    <assign to V1>  (or POP if no V1)
   [tb]                         POP
   []                           <code for S1>
                                JUMP_FORWARD    L0

   [tb, val, exc]       L2:     DUP
   .............................etc.......................

   [tb, val, exc]       Ln+1:   RERAISE     # re-raise exception

   []                   L0:     <next statement>

   Of course, parts are not generated if Vi or Ei is not present.
*/
static int
compiler_try_except(struct compiler *c, stmt_ty s)
{
    basicblock *body, *orelse, *except, *end;
    Py_ssize_t i, n;

    body = compiler_new_block(c);
    except = compiler_new_block(c);
    orelse = compiler_new_block(c);
    end = compiler_new_block(c);
    if (body == NULL || except == NULL || orelse == NULL || end == NULL)
        return 0;
    ADDOP_JUMP(c, SETUP_FINALLY, except);
    compiler_use_next_block(c, body);
    if (!compiler_push_fblock(c, EXCEPT, body, NULL, NULL))
        return 0;
    VISIT_SEQ(c, stmt, s->v.Try.body);
    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, EXCEPT, body);
    ADDOP_JUMP(c, JUMP_FORWARD, orelse);
    n = asdl_seq_LEN(s->v.Try.handlers);
    compiler_use_next_block(c, except);
    for (i = 0; i < n; i++) {
        excepthandler_ty handler = (excepthandler_ty)asdl_seq_GET(
            s->v.Try.handlers, i);
        if (!handler->v.ExceptHandler.type && i < n-1)
            return compiler_error(c, "default 'except:' must be last");
        SET_LOC(c, handler);
        except = compiler_new_block(c);
        if (except == NULL)
            return 0;
        if (handler->v.ExceptHandler.type) {
            ADDOP(c, DUP_TOP);
            VISIT(c, expr, handler->v.ExceptHandler.type);
            ADDOP_JUMP(c, JUMP_IF_NOT_EXC_MATCH, except);
        }
        ADDOP(c, POP_TOP);
        if (handler->v.ExceptHandler.name) {
            basicblock *cleanup_end, *cleanup_body;

            cleanup_end = compiler_new_block(c);
            cleanup_body = compiler_new_block(c);
            if (cleanup_end == NULL || cleanup_body == NULL) {
                return 0;
            }

            compiler_nameop(c, handler->v.ExceptHandler.name, Store);
            ADDOP(c, POP_TOP);

            /*
              try:
                  # body
              except type as name:
                  try:
                      # body
                  finally:
                      name = None # in case body contains "del name"
                      del name
            */

            /* second try: */
            ADDOP_JUMP(c, SETUP_FINALLY, cleanup_end);
            compiler_use_next_block(c, cleanup_body);
            if (!compiler_push_fblock(c, HANDLER_CLEANUP, cleanup_body, NULL, handler->v.ExceptHandler.name))
                return 0;

            /* second # body */
            VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
            compiler_pop_fblock(c, HANDLER_CLEANUP, cleanup_body);
            ADDOP(c, POP_BLOCK);
            ADDOP(c, POP_EXCEPT);
            /* name = None; del name */
            ADDOP_LOAD_CONST(c, Py_None);
            compiler_nameop(c, handler->v.ExceptHandler.name, Store);
            compiler_nameop(c, handler->v.ExceptHandler.name, Del);
            ADDOP_JUMP(c, JUMP_FORWARD, end);

            /* except: */
            compiler_use_next_block(c, cleanup_end);

            /* name = None; del name */
            ADDOP_LOAD_CONST(c, Py_None);
            compiler_nameop(c, handler->v.ExceptHandler.name, Store);
            compiler_nameop(c, handler->v.ExceptHandler.name, Del);

            ADDOP(c, RERAISE);
        }
        else {
            basicblock *cleanup_body;

            cleanup_body = compiler_new_block(c);
            if (!cleanup_body)
                return 0;

            ADDOP(c, POP_TOP);
            ADDOP(c, POP_TOP);
            compiler_use_next_block(c, cleanup_body);
            if (!compiler_push_fblock(c, HANDLER_CLEANUP, cleanup_body, NULL, NULL))
                return 0;
            VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
            compiler_pop_fblock(c, HANDLER_CLEANUP, cleanup_body);
            ADDOP(c, POP_EXCEPT);
            ADDOP_JUMP(c, JUMP_FORWARD, end);
        }
        compiler_use_next_block(c, except);
    }
    ADDOP(c, RERAISE);
    compiler_use_next_block(c, orelse);
    VISIT_SEQ(c, stmt, s->v.Try.orelse);
    compiler_use_next_block(c, end);
    return 1;
}

static int
compiler_try(struct compiler *c, stmt_ty s) {
    if (s->v.Try.finalbody && asdl_seq_LEN(s->v.Try.finalbody))
        return compiler_try_finally(c, s);
    else
        return compiler_try_except(c, s);
}


static int
compiler_import_as(struct compiler *c, identifier name, identifier asname)
{
    /* The IMPORT_NAME opcode was already generated.  This function
       merely needs to bind the result to a name.

       If there is a dot in name, we need to split it and emit a
       IMPORT_FROM for each name.
    */
    Py_ssize_t len = PyUnicode_GET_LENGTH(name);
    Py_ssize_t dot = PyUnicode_FindChar(name, '.', 0, len, 1);
    if (dot == -2)
        return 0;
    if (dot != -1) {
        /* Consume the base module name to get the first attribute */
        while (1) {
            Py_ssize_t pos = dot + 1;
            PyObject *attr;
            dot = PyUnicode_FindChar(name, '.', pos, len, 1);
            if (dot == -2)
                return 0;
            attr = PyUnicode_Substring(name, pos, (dot != -1) ? dot : len);
            if (!attr)
                return 0;
            ADDOP_N(c, IMPORT_FROM, attr, names);
            if (dot == -1) {
                break;
            }
            ADDOP(c, ROT_TWO);
            ADDOP(c, POP_TOP);
        }
        if (!compiler_nameop(c, asname, Store)) {
            return 0;
        }
        ADDOP(c, POP_TOP);
        return 1;
    }
    return compiler_nameop(c, asname, Store);
}

static int
compiler_import(struct compiler *c, stmt_ty s)
{
    /* The Import node stores a module name like a.b.c as a single
       string.  This is convenient for all cases except
         import a.b.c as d
       where we need to parse that string to extract the individual
       module names.
       XXX Perhaps change the representation to make this case simpler?
     */
    Py_ssize_t i, n = asdl_seq_LEN(s->v.Import.names);

    for (i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.Import.names, i);
        int r;

        ADDOP_LOAD_CONST(c, _PyLong_Zero);
        ADDOP_LOAD_CONST(c, Py_None);
        ADDOP_NAME(c, IMPORT_NAME, alias->name, names);

        if (alias->asname) {
            r = compiler_import_as(c, alias->name, alias->asname);
            if (!r)
                return r;
        }
        else {
            identifier tmp = alias->name;
            Py_ssize_t dot = PyUnicode_FindChar(
                alias->name, '.', 0, PyUnicode_GET_LENGTH(alias->name), 1);
            if (dot != -1) {
                tmp = PyUnicode_Substring(alias->name, 0, dot);
                if (tmp == NULL)
                    return 0;
            }
            r = compiler_nameop(c, tmp, Store);
            if (dot != -1) {
                Py_DECREF(tmp);
            }
            if (!r)
                return r;
        }
    }
    return 1;
}

static int
compiler_from_import(struct compiler *c, stmt_ty s)
{
    Py_ssize_t i, n = asdl_seq_LEN(s->v.ImportFrom.names);
    PyObject *names;
    static PyObject *empty_string;

    if (!empty_string) {
        empty_string = PyUnicode_FromString("");
        if (!empty_string)
            return 0;
    }

    ADDOP_LOAD_CONST_NEW(c, PyLong_FromLong(s->v.ImportFrom.level));

    names = PyTuple_New(n);
    if (!names)
        return 0;

    /* build up the names */
    for (i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.ImportFrom.names, i);
        Py_INCREF(alias->name);
        PyTuple_SET_ITEM(names, i, alias->name);
    }

    if (s->lineno > c->c_future->ff_lineno && s->v.ImportFrom.module &&
        _PyUnicode_EqualToASCIIString(s->v.ImportFrom.module, "__future__")) {
        Py_DECREF(names);
        return compiler_error(c, "from __future__ imports must occur "
                              "at the beginning of the file");
    }
    ADDOP_LOAD_CONST_NEW(c, names);

    if (s->v.ImportFrom.module) {
        ADDOP_NAME(c, IMPORT_NAME, s->v.ImportFrom.module, names);
    }
    else {
        ADDOP_NAME(c, IMPORT_NAME, empty_string, names);
    }
    for (i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.ImportFrom.names, i);
        identifier store_name;

        if (i == 0 && PyUnicode_READ_CHAR(alias->name, 0) == '*') {
            assert(n == 1);
            ADDOP(c, IMPORT_STAR);
            return 1;
        }

        ADDOP_NAME(c, IMPORT_FROM, alias->name, names);
        store_name = alias->name;
        if (alias->asname)
            store_name = alias->asname;

        if (!compiler_nameop(c, store_name, Store)) {
            return 0;
        }
    }
    /* remove imported module */
    ADDOP(c, POP_TOP);
    return 1;
}

static int
compiler_assert(struct compiler *c, stmt_ty s)
{
    basicblock *end;

    if (c->c_optimize)
        return 1;
    if (s->v.Assert.test->kind == Tuple_kind &&
        asdl_seq_LEN(s->v.Assert.test->v.Tuple.elts) > 0)
    {
        if (!compiler_warn(c, "assertion is always true, "
                              "perhaps remove parentheses?"))
        {
            return 0;
        }
    }
    end = compiler_new_block(c);
    if (end == NULL)
        return 0;
    if (!compiler_jump_if(c, s->v.Assert.test, end, 1))
        return 0;
    ADDOP(c, LOAD_ASSERTION_ERROR);
    if (s->v.Assert.msg) {
        VISIT(c, expr, s->v.Assert.msg);
        ADDOP_I(c, CALL_FUNCTION, 1);
    }
    ADDOP_I(c, RAISE_VARARGS, 1);
    compiler_use_next_block(c, end);
    return 1;
}

static int
compiler_visit_stmt_expr(struct compiler *c, expr_ty value)
{
    if (c->c_interactive && c->c_nestlevel <= 1) {
        VISIT(c, expr, value);
        ADDOP(c, PRINT_EXPR);
        return 1;
    }

    if (value->kind == Constant_kind) {
        /* ignore constant statement */
        return 1;
    }

    VISIT(c, expr, value);
    ADDOP(c, POP_TOP);
    return 1;
}

static int
compiler_visit_stmt(struct compiler *c, stmt_ty s)
{
    Py_ssize_t i, n;

    /* Always assign a lineno to the next instruction for a stmt. */
    SET_LOC(c, s);

    switch (s->kind) {
    case FunctionDef_kind:
        return compiler_function(c, s, 0);
    case ClassDef_kind:
        return compiler_class(c, s);
    case Return_kind:
        return compiler_return(c, s);
    case Delete_kind:
        VISIT_SEQ(c, expr, s->v.Delete.targets)
        break;
    case Assign_kind:
        n = asdl_seq_LEN(s->v.Assign.targets);
        VISIT(c, expr, s->v.Assign.value);
        for (i = 0; i < n; i++) {
            if (i < n - 1)
                ADDOP(c, DUP_TOP);
            VISIT(c, expr,
                  (expr_ty)asdl_seq_GET(s->v.Assign.targets, i));
        }
        break;
    case AugAssign_kind:
        return compiler_augassign(c, s);
    case AnnAssign_kind:
        return compiler_annassign(c, s);
    case For_kind:
        return compiler_for(c, s);
    case While_kind:
        return compiler_while(c, s);
    case If_kind:
        return compiler_if(c, s);
    case Raise_kind:
        n = 0;
        if (s->v.Raise.exc) {
            VISIT(c, expr, s->v.Raise.exc);
            n++;
            if (s->v.Raise.cause) {
                VISIT(c, expr, s->v.Raise.cause);
                n++;
            }
        }
        ADDOP_I(c, RAISE_VARARGS, (int)n);
        break;
    case Try_kind:
        return compiler_try(c, s);
    case Assert_kind:
        return compiler_assert(c, s);
    case Import_kind:
        return compiler_import(c, s);
    case ImportFrom_kind:
        return compiler_from_import(c, s);
    case Global_kind:
    case Nonlocal_kind:
        break;
    case Expr_kind:
        return compiler_visit_stmt_expr(c, s->v.Expr.value);
    case Pass_kind:
        break;
    case Break_kind:
        return compiler_break(c);
    case Continue_kind:
        return compiler_continue(c);
    case With_kind:
        return compiler_with(c, s, 0);
    case AsyncFunctionDef_kind:
        return compiler_function(c, s, 1);
    case AsyncWith_kind:
        return compiler_async_with(c, s, 0);
    case AsyncFor_kind:
        return compiler_async_for(c, s);
    }

    return 1;
}

static int
unaryop(unaryop_ty op)
{
    switch (op) {
    case Invert:
        return UNARY_INVERT;
    case Not:
        return UNARY_NOT;
    case UAdd:
        return UNARY_POSITIVE;
    case USub:
        return UNARY_NEGATIVE;
    default:
        PyErr_Format(PyExc_SystemError,
            "unary op %d should not be possible", op);
        return 0;
    }
}

static int
binop(operator_ty op)
{
    switch (op) {
    case Add:
        return BINARY_ADD;
    case Sub:
        return BINARY_SUBTRACT;
    case Mult:
        return BINARY_MULTIPLY;
    case MatMult:
        return BINARY_MATRIX_MULTIPLY;
    case Div:
        return BINARY_TRUE_DIVIDE;
    case Mod:
        return BINARY_MODULO;
    case Pow:
        return BINARY_POWER;
    case LShift:
        return BINARY_LSHIFT;
    case RShift:
        return BINARY_RSHIFT;
    case BitOr:
        return BINARY_OR;
    case BitXor:
        return BINARY_XOR;
    case BitAnd:
        return BINARY_AND;
    case FloorDiv:
        return BINARY_FLOOR_DIVIDE;
    default:
        PyErr_Format(PyExc_SystemError,
            "binary op %d should not be possible", op);
        return 0;
    }
}

static int
inplace_binop(operator_ty op)
{
    switch (op) {
    case Add:
        return INPLACE_ADD;
    case Sub:
        return INPLACE_SUBTRACT;
    case Mult:
        return INPLACE_MULTIPLY;
    case MatMult:
        return INPLACE_MATRIX_MULTIPLY;
    case Div:
        return INPLACE_TRUE_DIVIDE;
    case Mod:
        return INPLACE_MODULO;
    case Pow:
        return INPLACE_POWER;
    case LShift:
        return INPLACE_LSHIFT;
    case RShift:
        return INPLACE_RSHIFT;
    case BitOr:
        return INPLACE_OR;
    case BitXor:
        return INPLACE_XOR;
    case BitAnd:
        return INPLACE_AND;
    case FloorDiv:
        return INPLACE_FLOOR_DIVIDE;
    default:
        PyErr_Format(PyExc_SystemError,
            "inplace binary op %d should not be possible", op);
        return 0;
    }
}

static int
compiler_nameop(struct compiler *c, identifier name, expr_context_ty ctx)
{
    int op, scope;
    Py_ssize_t arg;
    enum { OP_FAST, OP_GLOBAL, OP_DEREF, OP_NAME } optype;

    PyObject *dict = c->u->u_names;
    PyObject *mangled;

    assert(!_PyUnicode_EqualToASCIIString(name, "None") &&
           !_PyUnicode_EqualToASCIIString(name, "True") &&
           !_PyUnicode_EqualToASCIIString(name, "False"));

    if (forbidden_name(c, name, ctx))
        return 0;

    mangled = _Py_Mangle(c->u->u_private, name);
    if (!mangled)
        return 0;

    op = 0;
    optype = OP_NAME;
    scope = PyST_GetScope(c->u->u_ste, mangled);
    switch (scope) {
    case FREE:
        dict = c->u->u_freevars;
        optype = OP_DEREF;
        break;
    case CELL:
        dict = c->u->u_cellvars;
        optype = OP_DEREF;
        break;
    case LOCAL:
        if (c->u->u_ste->ste_type == FunctionBlock)
            optype = OP_FAST;
        break;
    case GLOBAL_IMPLICIT:
        if (c->u->u_ste->ste_type == FunctionBlock)
            optype = OP_GLOBAL;
        break;
    case GLOBAL_EXPLICIT:
        optype = OP_GLOBAL;
        break;
    default:
        /* scope can be 0 */
        break;
    }

    /* XXX Leave assert here, but handle __doc__ and the like better */
    assert(scope || PyUnicode_READ_CHAR(name, 0) == '_');

    switch (optype) {
    case OP_DEREF:
        switch (ctx) {
        case Load:
            op = (c->u->u_ste->ste_type == ClassBlock) ? LOAD_CLASSDEREF : LOAD_DEREF;
            break;
        case Store: op = STORE_DEREF; break;
        case Del: op = DELETE_DEREF; break;
        }
        break;
    case OP_FAST:
        switch (ctx) {
        case Load: op = LOAD_FAST; break;
        case Store: op = STORE_FAST; break;
        case Del: op = DELETE_FAST; break;
        }
        ADDOP_N(c, op, mangled, varnames);
        return 1;
    case OP_GLOBAL:
        switch (ctx) {
        case Load: op = LOAD_GLOBAL; break;
        case Store: op = STORE_GLOBAL; break;
        case Del: op = DELETE_GLOBAL; break;
        }
        break;
    case OP_NAME:
        switch (ctx) {
        case Load: op = LOAD_NAME; break;
        case Store: op = STORE_NAME; break;
        case Del: op = DELETE_NAME; break;
        }
        break;
    }

    assert(op);
    arg = compiler_add_o(dict, mangled);
    Py_DECREF(mangled);
    if (arg < 0)
        return 0;
    return compiler_addop_i(c, op, arg);
}

static int
compiler_boolop(struct compiler *c, expr_ty e)
{
    basicblock *end;
    int jumpi;
    Py_ssize_t i, n;
    asdl_seq *s;

    assert(e->kind == BoolOp_kind);
    if (e->v.BoolOp.op == And)
        jumpi = JUMP_IF_FALSE_OR_POP;
    else
        jumpi = JUMP_IF_TRUE_OR_POP;
    end = compiler_new_block(c);
    if (end == NULL)
        return 0;
    s = e->v.BoolOp.values;
    n = asdl_seq_LEN(s) - 1;
    assert(n >= 0);
    for (i = 0; i < n; ++i) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(s, i));
        ADDOP_JUMP(c, jumpi, end);
        basicblock *next = compiler_new_block(c);
        if (next == NULL) {
            return 0;
        }
        compiler_use_next_block(c, next);
    }
    VISIT(c, expr, (expr_ty)asdl_seq_GET(s, n));
    compiler_use_next_block(c, end);
    return 1;
}

static int
starunpack_helper(struct compiler *c, asdl_seq *elts, int pushed,
                  int build, int add, int extend, int tuple)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    Py_ssize_t i, seen_star = 0;
    if (n > 2 && are_all_items_const(elts, 0, n)) {
        PyObject *folded = PyTuple_New(n);
        if (folded == NULL) {
            return 0;
        }
        PyObject *val;
        for (i = 0; i < n; i++) {
            val = ((expr_ty)asdl_seq_GET(elts, i))->v.Constant.value;
            Py_INCREF(val);
            PyTuple_SET_ITEM(folded, i, val);
        }
        if (tuple) {
            ADDOP_LOAD_CONST_NEW(c, folded);
        } else {
            if (add == SET_ADD) {
                Py_SETREF(folded, PyFrozenSet_New(folded));
                if (folded == NULL) {
                    return 0;
                }
            }
            ADDOP_I(c, build, pushed);
            ADDOP_LOAD_CONST_NEW(c, folded);
            ADDOP_I(c, extend, 1);
        }
        return 1;
    }

    for (i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == Starred_kind) {
            seen_star = 1;
        }
    }
    if (seen_star) {
        seen_star = 0;
        for (i = 0; i < n; i++) {
            expr_ty elt = asdl_seq_GET(elts, i);
            if (elt->kind == Starred_kind) {
                if (seen_star == 0) {
                    ADDOP_I(c, build, i+pushed);
                    seen_star = 1;
                }
                VISIT(c, expr, elt->v.Starred.value);
                ADDOP_I(c, extend, 1);
            }
            else {
                VISIT(c, expr, elt);
                if (seen_star) {
                    ADDOP_I(c, add, 1);
                }
            }
        }
        assert(seen_star);
        if (tuple) {
            ADDOP(c, LIST_TO_TUPLE);
        }
    }
    else {
        for (i = 0; i < n; i++) {
            expr_ty elt = asdl_seq_GET(elts, i);
            VISIT(c, expr, elt);
        }
        if (tuple) {
            ADDOP_I(c, BUILD_TUPLE, n+pushed);
        } else {
            ADDOP_I(c, build, n+pushed);
        }
    }
    return 1;
}

static int
assignment_helper(struct compiler *c, asdl_seq *elts)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    Py_ssize_t i;
    int seen_star = 0;
    for (i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == Starred_kind && !seen_star) {
            if ((i >= (1 << 8)) ||
                (n-i-1 >= (INT_MAX >> 8)))
                return compiler_error(c,
                    "too many expressions in "
                    "star-unpacking assignment");
            ADDOP_I(c, UNPACK_EX, (i + ((n-i-1) << 8)));
            seen_star = 1;
        }
        else if (elt->kind == Starred_kind) {
            return compiler_error(c,
                "multiple starred expressions in assignment");
        }
    }
    if (!seen_star) {
        ADDOP_I(c, UNPACK_SEQUENCE, n);
    }
    for (i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        VISIT(c, expr, elt->kind != Starred_kind ? elt : elt->v.Starred.value);
    }
    return 1;
}

static int
compiler_list(struct compiler *c, expr_ty e)
{
    asdl_seq *elts = e->v.List.elts;
    if (e->v.List.ctx == Store) {
        return assignment_helper(c, elts);
    }
    else if (e->v.List.ctx == Load) {
        return starunpack_helper(c, elts, 0, BUILD_LIST,
                                 LIST_APPEND, LIST_EXTEND, 0);
    }
    else
        VISIT_SEQ(c, expr, elts);
    return 1;
}

static int
compiler_tuple(struct compiler *c, expr_ty e)
{
    asdl_seq *elts = e->v.Tuple.elts;
    if (e->v.Tuple.ctx == Store) {
        return assignment_helper(c, elts);
    }
    else if (e->v.Tuple.ctx == Load) {
        return starunpack_helper(c, elts, 0, BUILD_LIST,
                                 LIST_APPEND, LIST_EXTEND, 1);
    }
    else
        VISIT_SEQ(c, expr, elts);
    return 1;
}

static int
compiler_set(struct compiler *c, expr_ty e)
{
    return starunpack_helper(c, e->v.Set.elts, 0, BUILD_SET,
                             SET_ADD, SET_UPDATE, 0);
}

static int
are_all_items_const(asdl_seq *seq, Py_ssize_t begin, Py_ssize_t end)
{
    Py_ssize_t i;
    for (i = begin; i < end; i++) {
        expr_ty key = (expr_ty)asdl_seq_GET(seq, i);
        if (key == NULL || key->kind != Constant_kind)
            return 0;
    }
    return 1;
}

static int
compiler_subdict(struct compiler *c, expr_ty e, Py_ssize_t begin, Py_ssize_t end)
{
    Py_ssize_t i, n = end - begin;
    PyObject *keys, *key;
    if (n > 1 && are_all_items_const(e->v.Dict.keys, begin, end)) {
        for (i = begin; i < end; i++) {
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
        }
        keys = PyTuple_New(n);
        if (keys == NULL) {
            return 0;
        }
        for (i = begin; i < end; i++) {
            key = ((expr_ty)asdl_seq_GET(e->v.Dict.keys, i))->v.Constant.value;
            Py_INCREF(key);
            PyTuple_SET_ITEM(keys, i - begin, key);
        }
        ADDOP_LOAD_CONST_NEW(c, keys);
        ADDOP_I(c, BUILD_CONST_KEY_MAP, n);
    }
    else {
        for (i = begin; i < end; i++) {
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.keys, i));
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
        }
        ADDOP_I(c, BUILD_MAP, n);
    }
    return 1;
}

static int
compiler_dict(struct compiler *c, expr_ty e)
{
    Py_ssize_t i, n, elements;
    int have_dict;
    int is_unpacking = 0;
    n = asdl_seq_LEN(e->v.Dict.values);
    have_dict = 0;
    elements = 0;
    for (i = 0; i < n; i++) {
        is_unpacking = (expr_ty)asdl_seq_GET(e->v.Dict.keys, i) == NULL;
        if (is_unpacking) {
            if (elements) {
                if (!compiler_subdict(c, e, i - elements, i)) {
                    return 0;
                }
                if (have_dict) {
                    ADDOP_I(c, DICT_UPDATE, 1);
                }
                have_dict = 1;
                elements = 0;
            }
            if (have_dict == 0) {
                ADDOP_I(c, BUILD_MAP, 0);
                have_dict = 1;
            }
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
            ADDOP_I(c, DICT_UPDATE, 1);
        }
        else {
            if (elements == 0xFFFF) {
                if (!compiler_subdict(c, e, i - elements, i)) {
                    return 0;
                }
                if (have_dict) {
                    ADDOP_I(c, DICT_UPDATE, 1);
                }
                have_dict = 1;
                elements = 0;
            }
            else {
                elements++;
            }
        }
    }
    if (elements) {
        if (!compiler_subdict(c, e, n - elements, n)) {
            return 0;
        }
        if (have_dict) {
            ADDOP_I(c, DICT_UPDATE, 1);
        }
        have_dict = 1;
    }
    if (!have_dict) {
        ADDOP_I(c, BUILD_MAP, 0);
    }
    return 1;
}

static int
compiler_compare(struct compiler *c, expr_ty e)
{
    Py_ssize_t i, n;

    if (!check_compare(c, e)) {
        return 0;
    }
    VISIT(c, expr, e->v.Compare.left);
    assert(asdl_seq_LEN(e->v.Compare.ops) > 0);
    n = asdl_seq_LEN(e->v.Compare.ops) - 1;
    if (n == 0) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Compare.comparators, 0));
        ADDOP_COMPARE(c, asdl_seq_GET(e->v.Compare.ops, 0));
    }
    else {
        basicblock *cleanup = compiler_new_block(c);
        if (cleanup == NULL)
            return 0;
        for (i = 0; i < n; i++) {
            VISIT(c, expr,
                (expr_ty)asdl_seq_GET(e->v.Compare.comparators, i));
            ADDOP(c, DUP_TOP);
            ADDOP(c, ROT_THREE);
            ADDOP_COMPARE(c, asdl_seq_GET(e->v.Compare.ops, i));
            ADDOP_JUMP(c, JUMP_IF_FALSE_OR_POP, cleanup);
            NEXT_BLOCK(c);
        }
        VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Compare.comparators, n));
        ADDOP_COMPARE(c, asdl_seq_GET(e->v.Compare.ops, n));
        basicblock *end = compiler_new_block(c);
        if (end == NULL)
            return 0;
        ADDOP_JUMP(c, JUMP_FORWARD, end);
        compiler_use_next_block(c, cleanup);
        ADDOP(c, ROT_TWO);
        ADDOP(c, POP_TOP);
        compiler_use_next_block(c, end);
    }
    return 1;
}

static PyTypeObject *
infer_type(expr_ty e)
{
    switch (e->kind) {
    case Tuple_kind:
        return &PyTuple_Type;
    case List_kind:
    case ListComp_kind:
        return &PyList_Type;
    case Dict_kind:
    case DictComp_kind:
        return &PyDict_Type;
    case Set_kind:
    case SetComp_kind:
        return &PySet_Type;
    case GeneratorExp_kind:
        return &PyGen_Type;
    case Lambda_kind:
        return &PyFunction_Type;
    case JoinedStr_kind:
    case FormattedValue_kind:
        return &PyUnicode_Type;
    case Constant_kind:
        return Py_TYPE(e->v.Constant.value);
    default:
        return NULL;
    }
}

static int
check_caller(struct compiler *c, expr_ty e)
{
    switch (e->kind) {
    case Constant_kind:
    case Tuple_kind:
    case List_kind:
    case ListComp_kind:
    case Dict_kind:
    case DictComp_kind:
    case Set_kind:
    case SetComp_kind:
    case GeneratorExp_kind:
    case JoinedStr_kind:
    case FormattedValue_kind:
        return compiler_warn(c, "'%.200s' object is not callable; "
                                "perhaps you missed a comma?",
                                infer_type(e)->tp_name);
    default:
        return 1;
    }
}

static int
check_subscripter(struct compiler *c, expr_ty e)
{
    PyObject *v;

    switch (e->kind) {
    case Constant_kind:
        v = e->v.Constant.value;
        if (!(v == Py_None || v == Py_Ellipsis ||
              PyLong_Check(v) || PyFloat_Check(v) || PyComplex_Check(v) ||
              PyAnySet_Check(v)))
        {
            return 1;
        }
        /* fall through */
    case Set_kind:
    case SetComp_kind:
    case GeneratorExp_kind:
    case Lambda_kind:
        return compiler_warn(c, "'%.200s' object is not subscriptable; "
                                "perhaps you missed a comma?",
                                infer_type(e)->tp_name);
    default:
        return 1;
    }
}

static int
check_index(struct compiler *c, expr_ty e, expr_ty s)
{
    PyObject *v;

    PyTypeObject *index_type = infer_type(s);
    if (index_type == NULL
        || PyType_FastSubclass(index_type, Py_TPFLAGS_LONG_SUBCLASS)
        || index_type == &PySlice_Type) {
        return 1;
    }

    switch (e->kind) {
    case Constant_kind:
        v = e->v.Constant.value;
        if (!(PyUnicode_Check(v) || PyBytes_Check(v) || PyTuple_Check(v))) {
            return 1;
        }
        /* fall through */
    case Tuple_kind:
    case List_kind:
    case ListComp_kind:
    case JoinedStr_kind:
    case FormattedValue_kind:
        return compiler_warn(c, "%.200s indices must be integers or slices, "
                                "not %.200s; "
                                "perhaps you missed a comma?",
                                infer_type(e)->tp_name,
                                index_type->tp_name);
    default:
        return 1;
    }
}

// Return 1 if the method call was optimized, -1 if not, and 0 on error.
static int
maybe_optimize_method_call(struct compiler *c, expr_ty e)
{
    Py_ssize_t argsl, i;
    expr_ty meth = e->v.Call.func;
    asdl_seq *args = e->v.Call.args;

    /* Check that the call node is an attribute access, and that
       the call doesn't have keyword parameters. */
    if (meth->kind != Attribute_kind || meth->v.Attribute.ctx != Load ||
            asdl_seq_LEN(e->v.Call.keywords))
        return -1;

    /* Check that there are no *varargs types of arguments. */
    argsl = asdl_seq_LEN(args);
    for (i = 0; i < argsl; i++) {
        expr_ty elt = asdl_seq_GET(args, i);
        if (elt->kind == Starred_kind) {
            return -1;
        }
    }

    /* Alright, we can optimize the code. */
    VISIT(c, expr, meth->v.Attribute.value);
    ADDOP_NAME(c, LOAD_METHOD, meth->v.Attribute.attr, names);
    VISIT_SEQ(c, expr, e->v.Call.args);
    ADDOP_I(c, CALL_METHOD, asdl_seq_LEN(e->v.Call.args));
    return 1;
}

static int
validate_keywords(struct compiler *c, asdl_seq *keywords)
{
    Py_ssize_t nkeywords = asdl_seq_LEN(keywords);
    for (Py_ssize_t i = 0; i < nkeywords; i++) {
        keyword_ty key = ((keyword_ty)asdl_seq_GET(keywords, i));
        if (key->arg == NULL) {
            continue;
        }
        if (forbidden_name(c, key->arg, Store)) {
            return -1;
        }
        for (Py_ssize_t j = i + 1; j < nkeywords; j++) {
            keyword_ty other = ((keyword_ty)asdl_seq_GET(keywords, j));
            if (other->arg && !PyUnicode_Compare(key->arg, other->arg)) {
                PyObject *msg = PyUnicode_FromFormat("keyword argument repeated: %U", key->arg);
                if (msg == NULL) {
                    return -1;
                }
                c->u->u_col_offset = other->col_offset;
                compiler_error(c, PyUnicode_AsUTF8(msg));
                Py_DECREF(msg);
                return -1;
            }
        }
    }
    return 0;
}

static int
compiler_call(struct compiler *c, expr_ty e)
{
    int ret = maybe_optimize_method_call(c, e);
    if (ret >= 0) {
        return ret;
    }
    if (!check_caller(c, e->v.Call.func)) {
        return 0;
    }
    VISIT(c, expr, e->v.Call.func);
    return compiler_call_helper(c, 0,
                                e->v.Call.args,
                                e->v.Call.keywords);
}

static int
compiler_joined_str(struct compiler *c, expr_ty e)
{
    VISIT_SEQ(c, expr, e->v.JoinedStr.values);
    if (asdl_seq_LEN(e->v.JoinedStr.values) != 1)
        ADDOP_I(c, BUILD_STRING, asdl_seq_LEN(e->v.JoinedStr.values));
    return 1;
}

/* Used to implement f-strings. Format a single value. */
static int
compiler_formatted_value(struct compiler *c, expr_ty e)
{
    /* Our oparg encodes 2 pieces of information: the conversion
       character, and whether or not a format_spec was provided.

       Convert the conversion char to 3 bits:
           : 000  0x0  FVC_NONE   The default if nothing specified.
       !s  : 001  0x1  FVC_STR
       !r  : 010  0x2  FVC_REPR
       !a  : 011  0x3  FVC_ASCII

       next bit is whether or not we have a format spec:
       yes : 100  0x4
       no  : 000  0x0
    */

    int conversion = e->v.FormattedValue.conversion;
    int oparg;

    /* The expression to be formatted. */
    VISIT(c, expr, e->v.FormattedValue.value);

    switch (conversion) {
    case 's': oparg = FVC_STR;   break;
    case 'r': oparg = FVC_REPR;  break;
    case 'a': oparg = FVC_ASCII; break;
    case -1:  oparg = FVC_NONE;  break;
    default:
        PyErr_Format(PyExc_SystemError,
                     "Unrecognized conversion character %d", conversion);
        return 0;
    }
    if (e->v.FormattedValue.format_spec) {
        /* Evaluate the format spec, and update our opcode arg. */
        VISIT(c, expr, e->v.FormattedValue.format_spec);
        oparg |= FVS_HAVE_SPEC;
    }

    /* And push our opcode and oparg */
    ADDOP_I(c, FORMAT_VALUE, oparg);

    return 1;
}

static int
compiler_subkwargs(struct compiler *c, asdl_seq *keywords, Py_ssize_t begin, Py_ssize_t end)
{
    Py_ssize_t i, n = end - begin;
    keyword_ty kw;
    PyObject *keys, *key;
    assert(n > 0);
    if (n > 1) {
        for (i = begin; i < end; i++) {
            kw = asdl_seq_GET(keywords, i);
            VISIT(c, expr, kw->value);
        }
        keys = PyTuple_New(n);
        if (keys == NULL) {
            return 0;
        }
        for (i = begin; i < end; i++) {
            key = ((keyword_ty) asdl_seq_GET(keywords, i))->arg;
            Py_INCREF(key);
            PyTuple_SET_ITEM(keys, i - begin, key);
        }
        ADDOP_LOAD_CONST_NEW(c, keys);
        ADDOP_I(c, BUILD_CONST_KEY_MAP, n);
    }
    else {
        /* a for loop only executes once */
        for (i = begin; i < end; i++) {
            kw = asdl_seq_GET(keywords, i);
            ADDOP_LOAD_CONST(c, kw->arg);
            VISIT(c, expr, kw->value);
        }
        ADDOP_I(c, BUILD_MAP, n);
    }
    return 1;
}

/* shared code between compiler_call and compiler_class */
static int
compiler_call_helper(struct compiler *c,
                     int n, /* Args already pushed */
                     asdl_seq *args,
                     asdl_seq *keywords)
{
    Py_ssize_t i, nseen, nelts, nkwelts;

    if (validate_keywords(c, keywords) == -1) {
        return 0;
    }

    nelts = asdl_seq_LEN(args);
    nkwelts = asdl_seq_LEN(keywords);

    for (i = 0; i < nelts; i++) {
        expr_ty elt = asdl_seq_GET(args, i);
        if (elt->kind == Starred_kind) {
            goto ex_call;
        }
    }
    for (i = 0; i < nkwelts; i++) {
        keyword_ty kw = asdl_seq_GET(keywords, i);
        if (kw->arg == NULL) {
            goto ex_call;
        }
    }

    /* No * or ** args, so can use faster calling sequence */
    for (i = 0; i < nelts; i++) {
        expr_ty elt = asdl_seq_GET(args, i);
        assert(elt->kind != Starred_kind);
        VISIT(c, expr, elt);
    }
    if (nkwelts) {
        PyObject *names;
        VISIT_SEQ(c, keyword, keywords);
        names = PyTuple_New(nkwelts);
        if (names == NULL) {
            return 0;
        }
        for (i = 0; i < nkwelts; i++) {
            keyword_ty kw = asdl_seq_GET(keywords, i);
            Py_INCREF(kw->arg);
            PyTuple_SET_ITEM(names, i, kw->arg);
        }
        ADDOP_LOAD_CONST_NEW(c, names);
        ADDOP_I(c, CALL_FUNCTION_KW, n + nelts + nkwelts);
        return 1;
    }
    else {
        ADDOP_I(c, CALL_FUNCTION, n + nelts);
        return 1;
    }

ex_call:

    /* Do positional arguments. */
    if (n ==0 && nelts == 1 && ((expr_ty)asdl_seq_GET(args, 0))->kind == Starred_kind) {
        VISIT(c, expr, ((expr_ty)asdl_seq_GET(args, 0))->v.Starred.value);
    }
    else if (starunpack_helper(c, args, n, BUILD_LIST,
                                 LIST_APPEND, LIST_EXTEND, 1) == 0) {
        return 0;
    }
    /* Then keyword arguments */
    if (nkwelts) {
        /* Has a new dict been pushed */
        int have_dict = 0;

        nseen = 0;  /* the number of keyword arguments on the stack following */
        for (i = 0; i < nkwelts; i++) {
            keyword_ty kw = asdl_seq_GET(keywords, i);
            if (kw->arg == NULL) {
                /* A keyword argument unpacking. */
                if (nseen) {
                    if (!compiler_subkwargs(c, keywords, i - nseen, i)) {
                        return 0;
                    }
                    if (have_dict) {
                        ADDOP_I(c, DICT_MERGE, 1);
                    }
                    have_dict = 1;
                    nseen = 0;
                }
                if (!have_dict) {
                    ADDOP_I(c, BUILD_MAP, 0);
                    have_dict = 1;
                }
                VISIT(c, expr, kw->value);
                ADDOP_I(c, DICT_MERGE, 1);
            }
            else {
                nseen++;
            }
        }
        if (nseen) {
            /* Pack up any trailing keyword arguments. */
            if (!compiler_subkwargs(c, keywords, nkwelts - nseen, nkwelts)) {
                return 0;
            }
            if (have_dict) {
                ADDOP_I(c, DICT_MERGE, 1);
            }
            have_dict = 1;
        }
        assert(have_dict);
    }
    ADDOP_I(c, CALL_FUNCTION_EX, nkwelts > 0);
    return 1;
}


/* List and set comprehensions and generator expressions work by creating a
  nested function to perform the actual iteration. This means that the
  iteration variables don't leak into the current scope.
  The defined function is called immediately following its definition, with the
  result of that call being the result of the expression.
  The LC/SC version returns the populated container, while the GE version is
  flagged in symtable.c as a generator, so it returns the generator object
  when the function is called.

  Possible cleanups:
    - iterate over the generator sequence instead of using recursion
*/


static int
compiler_comprehension_generator(struct compiler *c,
                                 asdl_seq *generators, int gen_index,
                                 int depth,
                                 expr_ty elt, expr_ty val, int type)
{
    comprehension_ty gen;
    gen = (comprehension_ty)asdl_seq_GET(generators, gen_index);
    if (gen->is_async) {
        return compiler_async_comprehension_generator(
            c, generators, gen_index, depth, elt, val, type);
    } else {
        return compiler_sync_comprehension_generator(
            c, generators, gen_index, depth, elt, val, type);
    }
}

static int
compiler_sync_comprehension_generator(struct compiler *c,
                                      asdl_seq *generators, int gen_index,
                                      int depth,
                                      expr_ty elt, expr_ty val, int type)
{
    /* generate code for the iterator, then each of the ifs,
       and then write to the element */

    comprehension_ty gen;
    basicblock *start, *anchor, *skip, *if_cleanup;
    Py_ssize_t i, n;

    start = compiler_new_block(c);
    skip = compiler_new_block(c);
    if_cleanup = compiler_new_block(c);
    anchor = compiler_new_block(c);

    if (start == NULL || skip == NULL || if_cleanup == NULL ||
        anchor == NULL)
        return 0;

    gen = (comprehension_ty)asdl_seq_GET(generators, gen_index);

    if (gen_index == 0) {
        /* Receive outermost iter as an implicit argument */
        c->u->u_argcount = 1;
        ADDOP_I(c, LOAD_FAST, 0);
    }
    else {
        /* Sub-iter - calculate on the fly */
        /* Fast path for the temporary variable assignment idiom:
             for y in [f(x)]
         */
        asdl_seq *elts;
        switch (gen->iter->kind) {
            case List_kind:
                elts = gen->iter->v.List.elts;
                break;
            case Tuple_kind:
                elts = gen->iter->v.Tuple.elts;
                break;
            default:
                elts = NULL;
        }
        if (asdl_seq_LEN(elts) == 1) {
            expr_ty elt = asdl_seq_GET(elts, 0);
            if (elt->kind != Starred_kind) {
                VISIT(c, expr, elt);
                start = NULL;
            }
        }
        if (start) {
            VISIT(c, expr, gen->iter);
            ADDOP(c, GET_ITER);
        }
    }
    if (start) {
        depth++;
        compiler_use_next_block(c, start);
        ADDOP_JUMP(c, FOR_ITER, anchor);
        NEXT_BLOCK(c);
    }
    VISIT(c, expr, gen->target);

    /* XXX this needs to be cleaned up...a lot! */
    n = asdl_seq_LEN(gen->ifs);
    for (i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(gen->ifs, i);
        if (!compiler_jump_if(c, e, if_cleanup, 0))
            return 0;
        NEXT_BLOCK(c);
    }

    if (++gen_index < asdl_seq_LEN(generators))
        if (!compiler_comprehension_generator(c,
                                              generators, gen_index, depth,
                                              elt, val, type))
        return 0;

    /* only append after the last for generator */
    if (gen_index >= asdl_seq_LEN(generators)) {
        /* comprehension specific code */
        switch (type) {
        case COMP_GENEXP:
            VISIT(c, expr, elt);
            ADDOP(c, YIELD_VALUE);
            ADDOP(c, POP_TOP);
            break;
        case COMP_LISTCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, LIST_APPEND, depth + 1);
            break;
        case COMP_SETCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, SET_ADD, depth + 1);
            break;
        case COMP_DICTCOMP:
            /* With '{k: v}', k is evaluated before v, so we do
               the same. */
            VISIT(c, expr, elt);
            VISIT(c, expr, val);
            ADDOP_I(c, MAP_ADD, depth + 1);
            break;
        default:
            return 0;
        }

        compiler_use_next_block(c, skip);
    }
    compiler_use_next_block(c, if_cleanup);
    if (start) {
        ADDOP_JUMP(c, JUMP_ABSOLUTE, start);
        compiler_use_next_block(c, anchor);
    }

    return 1;
}

static int
compiler_async_comprehension_generator(struct compiler *c,
                                      asdl_seq *generators, int gen_index,
                                      int depth,
                                      expr_ty elt, expr_ty val, int type)
{
    comprehension_ty gen;
    basicblock *start, *if_cleanup, *except;
    Py_ssize_t i, n;
    start = compiler_new_block(c);
    except = compiler_new_block(c);
    if_cleanup = compiler_new_block(c);

    if (start == NULL || if_cleanup == NULL || except == NULL) {
        return 0;
    }

    gen = (comprehension_ty)asdl_seq_GET(generators, gen_index);

    if (gen_index == 0) {
        /* Receive outermost iter as an implicit argument */
        c->u->u_argcount = 1;
        ADDOP_I(c, LOAD_FAST, 0);
    }
    else {
        /* Sub-iter - calculate on the fly */
        VISIT(c, expr, gen->iter);
        ADDOP(c, GET_AITER);
    }

    compiler_use_next_block(c, start);

    ADDOP_JUMP(c, SETUP_FINALLY, except);
    ADDOP(c, GET_ANEXT);
    ADDOP_LOAD_CONST(c, Py_None);
    ADDOP(c, YIELD_FROM);
    ADDOP(c, POP_BLOCK);
    VISIT(c, expr, gen->target);

    n = asdl_seq_LEN(gen->ifs);
    for (i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(gen->ifs, i);
        if (!compiler_jump_if(c, e, if_cleanup, 0))
            return 0;
        NEXT_BLOCK(c);
    }

    depth++;
    if (++gen_index < asdl_seq_LEN(generators))
        if (!compiler_comprehension_generator(c,
                                              generators, gen_index, depth,
                                              elt, val, type))
        return 0;

    /* only append after the last for generator */
    if (gen_index >= asdl_seq_LEN(generators)) {
        /* comprehension specific code */
        switch (type) {
        case COMP_GENEXP:
            VISIT(c, expr, elt);
            ADDOP(c, YIELD_VALUE);
            ADDOP(c, POP_TOP);
            break;
        case COMP_LISTCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, LIST_APPEND, depth + 1);
            break;
        case COMP_SETCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, SET_ADD, depth + 1);
            break;
        case COMP_DICTCOMP:
            /* With '{k: v}', k is evaluated before v, so we do
               the same. */
            VISIT(c, expr, elt);
            VISIT(c, expr, val);
            ADDOP_I(c, MAP_ADD, depth + 1);
            break;
        default:
            return 0;
        }
    }
    compiler_use_next_block(c, if_cleanup);
    ADDOP_JUMP(c, JUMP_ABSOLUTE, start);

    compiler_use_next_block(c, except);
    ADDOP(c, END_ASYNC_FOR);

    return 1;
}

static int
compiler_comprehension(struct compiler *c, expr_ty e, int type,
                       identifier name, asdl_seq *generators, expr_ty elt,
                       expr_ty val)
{
    PyCodeObject *co = NULL;
    comprehension_ty outermost;
    PyObject *qualname = NULL;
    int is_async_generator = 0;
    int top_level_await = IS_TOP_LEVEL_AWAIT(c);


    int is_async_function = c->u->u_ste->ste_coroutine;

    outermost = (comprehension_ty) asdl_seq_GET(generators, 0);
    if (!compiler_enter_scope(c, name, COMPILER_SCOPE_COMPREHENSION,
                              (void *)e, e->lineno))
    {
        goto error;
    }

    is_async_generator = c->u->u_ste->ste_coroutine;

    if (is_async_generator && !is_async_function && type != COMP_GENEXP  && !top_level_await) {
        compiler_error(c, "asynchronous comprehension outside of "
                          "an asynchronous function");
        goto error_in_scope;
    }

    if (type != COMP_GENEXP) {
        int op;
        switch (type) {
        case COMP_LISTCOMP:
            op = BUILD_LIST;
            break;
        case COMP_SETCOMP:
            op = BUILD_SET;
            break;
        case COMP_DICTCOMP:
            op = BUILD_MAP;
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "unknown comprehension type %d", type);
            goto error_in_scope;
        }

        ADDOP_I(c, op, 0);
    }

    if (!compiler_comprehension_generator(c, generators, 0, 0, elt,
                                          val, type))
        goto error_in_scope;

    if (type != COMP_GENEXP) {
        ADDOP(c, RETURN_VALUE);
    }

    co = assemble(c, 1);
    qualname = c->u->u_qualname;
    Py_INCREF(qualname);
    compiler_exit_scope(c);
    if (top_level_await && is_async_generator){
        c->u->u_ste->ste_coroutine = 1;
    }
    if (co == NULL)
        goto error;

    if (!compiler_make_closure(c, co, 0, qualname))
        goto error;
    Py_DECREF(qualname);
    Py_DECREF(co);

    VISIT(c, expr, outermost->iter);

    if (outermost->is_async) {
        ADDOP(c, GET_AITER);
    } else {
        ADDOP(c, GET_ITER);
    }

    ADDOP_I(c, CALL_FUNCTION, 1);

    if (is_async_generator && type != COMP_GENEXP) {
        ADDOP(c, GET_AWAITABLE);
        ADDOP_LOAD_CONST(c, Py_None);
        ADDOP(c, YIELD_FROM);
    }

    return 1;
error_in_scope:
    compiler_exit_scope(c);
error:
    Py_XDECREF(qualname);
    Py_XDECREF(co);
    return 0;
}

static int
compiler_genexp(struct compiler *c, expr_ty e)
{
    static identifier name;
    if (!name) {
        name = PyUnicode_InternFromString("<genexpr>");
        if (!name)
            return 0;
    }
    assert(e->kind == GeneratorExp_kind);
    return compiler_comprehension(c, e, COMP_GENEXP, name,
                                  e->v.GeneratorExp.generators,
                                  e->v.GeneratorExp.elt, NULL);
}

static int
compiler_listcomp(struct compiler *c, expr_ty e)
{
    static identifier name;
    if (!name) {
        name = PyUnicode_InternFromString("<listcomp>");
        if (!name)
            return 0;
    }
    assert(e->kind == ListComp_kind);
    return compiler_comprehension(c, e, COMP_LISTCOMP, name,
                                  e->v.ListComp.generators,
                                  e->v.ListComp.elt, NULL);
}

static int
compiler_setcomp(struct compiler *c, expr_ty e)
{
    static identifier name;
    if (!name) {
        name = PyUnicode_InternFromString("<setcomp>");
        if (!name)
            return 0;
    }
    assert(e->kind == SetComp_kind);
    return compiler_comprehension(c, e, COMP_SETCOMP, name,
                                  e->v.SetComp.generators,
                                  e->v.SetComp.elt, NULL);
}


static int
compiler_dictcomp(struct compiler *c, expr_ty e)
{
    static identifier name;
    if (!name) {
        name = PyUnicode_InternFromString("<dictcomp>");
        if (!name)
            return 0;
    }
    assert(e->kind == DictComp_kind);
    return compiler_comprehension(c, e, COMP_DICTCOMP, name,
                                  e->v.DictComp.generators,
                                  e->v.DictComp.key, e->v.DictComp.value);
}


static int
compiler_visit_keyword(struct compiler *c, keyword_ty k)
{
    VISIT(c, expr, k->value);
    return 1;
}

/* Test whether expression is constant.  For constants, report
   whether they are true or false.

   Return values: 1 for true, 0 for false, -1 for non-constant.
 */

static int
expr_constant(expr_ty e)
{
    if (e->kind == Constant_kind) {
        return PyObject_IsTrue(e->v.Constant.value);
    }
    return -1;
}

static int
compiler_with_except_finish(struct compiler *c) {
    basicblock *exit;
    exit = compiler_new_block(c);
    if (exit == NULL)
        return 0;
    ADDOP_JUMP(c, POP_JUMP_IF_TRUE, exit);
    ADDOP(c, RERAISE);
    compiler_use_next_block(c, exit);
    ADDOP(c, POP_TOP);
    ADDOP(c, POP_TOP);
    ADDOP(c, POP_TOP);
    ADDOP(c, POP_EXCEPT);
    ADDOP(c, POP_TOP);
    return 1;
}

/*
   Implements the async with statement.

   The semantics outlined in that PEP are as follows:

   async with EXPR as VAR:
       BLOCK

   It is implemented roughly as:

   context = EXPR
   exit = context.__aexit__  # not calling it
   value = await context.__aenter__()
   try:
       VAR = value  # if VAR present in the syntax
       BLOCK
   finally:
       if an exception was raised:
           exc = copy of (exception, instance, traceback)
       else:
           exc = (None, None, None)
       if not (await exit(*exc)):
           raise
 */
static int
compiler_async_with(struct compiler *c, stmt_ty s, int pos)
{
    basicblock *block, *final, *exit;
    withitem_ty item = asdl_seq_GET(s->v.AsyncWith.items, pos);

    assert(s->kind == AsyncWith_kind);
    if (IS_TOP_LEVEL_AWAIT(c)){
        c->u->u_ste->ste_coroutine = 1;
    } else if (c->u->u_scope_type != COMPILER_SCOPE_ASYNC_FUNCTION){
        return compiler_error(c, "'async with' outside async function");
    }

    block = compiler_new_block(c);
    final = compiler_new_block(c);
    exit = compiler_new_block(c);
    if (!block || !final || !exit)
        return 0;

    /* Evaluate EXPR */
    VISIT(c, expr, item->context_expr);

    ADDOP(c, BEFORE_ASYNC_WITH);
    ADDOP(c, GET_AWAITABLE);
    ADDOP_LOAD_CONST(c, Py_None);
    ADDOP(c, YIELD_FROM);

    ADDOP_JUMP(c, SETUP_ASYNC_WITH, final);

    /* SETUP_ASYNC_WITH pushes a finally block. */
    compiler_use_next_block(c, block);
    if (!compiler_push_fblock(c, ASYNC_WITH, block, final, NULL)) {
        return 0;
    }

    if (item->optional_vars) {
        VISIT(c, expr, item->optional_vars);
    }
    else {
    /* Discard result from context.__aenter__() */
        ADDOP(c, POP_TOP);
    }

    pos++;
    if (pos == asdl_seq_LEN(s->v.AsyncWith.items))
        /* BLOCK code */
        VISIT_SEQ(c, stmt, s->v.AsyncWith.body)
    else if (!compiler_async_with(c, s, pos))
            return 0;

    compiler_pop_fblock(c, ASYNC_WITH, block);
    ADDOP(c, POP_BLOCK);
    /* End of body; start the cleanup */

    /* For successful outcome:
     * call __exit__(None, None, None)
     */
    if(!compiler_call_exit_with_nones(c))
        return 0;
    ADDOP(c, GET_AWAITABLE);
    ADDOP_O(c, LOAD_CONST, Py_None, consts);
    ADDOP(c, YIELD_FROM);

    ADDOP(c, POP_TOP);

    ADDOP_JUMP(c, JUMP_ABSOLUTE, exit);

    /* For exceptional outcome: */
    compiler_use_next_block(c, final);

    ADDOP(c, WITH_EXCEPT_START);
    ADDOP(c, GET_AWAITABLE);
    ADDOP_LOAD_CONST(c, Py_None);
    ADDOP(c, YIELD_FROM);
    compiler_with_except_finish(c);

compiler_use_next_block(c, exit);
    return 1;
}


/*
   Implements the with statement from PEP 343.
   with EXPR as VAR:
       BLOCK
   is implemented as:
        <code for EXPR>
        SETUP_WITH  E
        <code to store to VAR> or POP_TOP
        <code for BLOCK>
        LOAD_CONST (None, None, None)
        CALL_FUNCTION_EX 0
        JUMP_FORWARD  EXIT
    E:  WITH_EXCEPT_START (calls EXPR.__exit__)
        POP_JUMP_IF_TRUE T:
        RERAISE
    T:  POP_TOP * 3 (remove exception from stack)
        POP_EXCEPT
        POP_TOP
    EXIT:
 */

static int
compiler_with(struct compiler *c, stmt_ty s, int pos)
{
    basicblock *block, *final, *exit;
    withitem_ty item = asdl_seq_GET(s->v.With.items, pos);

    assert(s->kind == With_kind);

    block = compiler_new_block(c);
    final = compiler_new_block(c);
    exit = compiler_new_block(c);
    if (!block || !final || !exit)
        return 0;

    /* Evaluate EXPR */
    VISIT(c, expr, item->context_expr);
    /* Will push bound __exit__ */
    ADDOP_JUMP(c, SETUP_WITH, final);

    /* SETUP_WITH pushes a finally block. */
    compiler_use_next_block(c, block);
    if (!compiler_push_fblock(c, WITH, block, final, NULL)) {
        return 0;
    }

    if (item->optional_vars) {
        VISIT(c, expr, item->optional_vars);
    }
    else {
    /* Discard result from context.__enter__() */
        ADDOP(c, POP_TOP);
    }

    pos++;
    if (pos == asdl_seq_LEN(s->v.With.items))
        /* BLOCK code */
        VISIT_SEQ(c, stmt, s->v.With.body)
    else if (!compiler_with(c, s, pos))
            return 0;

    ADDOP(c, POP_BLOCK);
    compiler_pop_fblock(c, WITH, block);

    /* End of body; start the cleanup. */

    /* For successful outcome:
     * call __exit__(None, None, None)
     */
    if (!compiler_call_exit_with_nones(c))
        return 0;
    ADDOP(c, POP_TOP);
    ADDOP_JUMP(c, JUMP_FORWARD, exit);

    /* For exceptional outcome: */
    compiler_use_next_block(c, final);

    ADDOP(c, WITH_EXCEPT_START);
    compiler_with_except_finish(c);

    compiler_use_next_block(c, exit);
    return 1;
}

static int
compiler_visit_expr1(struct compiler *c, expr_ty e)
{
    switch (e->kind) {
    case NamedExpr_kind:
        VISIT(c, expr, e->v.NamedExpr.value);
        ADDOP(c, DUP_TOP);
        VISIT(c, expr, e->v.NamedExpr.target);
        break;
    case BoolOp_kind:
        return compiler_boolop(c, e);
    case BinOp_kind:
        VISIT(c, expr, e->v.BinOp.left);
        VISIT(c, expr, e->v.BinOp.right);
        ADDOP(c, binop(e->v.BinOp.op));
        break;
    case UnaryOp_kind:
        VISIT(c, expr, e->v.UnaryOp.operand);
        ADDOP(c, unaryop(e->v.UnaryOp.op));
        break;
    case Lambda_kind:
        return compiler_lambda(c, e);
    case IfExp_kind:
        return compiler_ifexp(c, e);
    case Dict_kind:
        return compiler_dict(c, e);
    case Set_kind:
        return compiler_set(c, e);
    case GeneratorExp_kind:
        return compiler_genexp(c, e);
    case ListComp_kind:
        return compiler_listcomp(c, e);
    case SetComp_kind:
        return compiler_setcomp(c, e);
    case DictComp_kind:
        return compiler_dictcomp(c, e);
    case Yield_kind:
        if (c->u->u_ste->ste_type != FunctionBlock)
            return compiler_error(c, "'yield' outside function");
        if (e->v.Yield.value) {
            VISIT(c, expr, e->v.Yield.value);
        }
        else {
            ADDOP_LOAD_CONST(c, Py_None);
        }
        ADDOP(c, YIELD_VALUE);
        break;
    case YieldFrom_kind:
        if (c->u->u_ste->ste_type != FunctionBlock)
            return compiler_error(c, "'yield' outside function");

        if (c->u->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION)
            return compiler_error(c, "'yield from' inside async function");

        VISIT(c, expr, e->v.YieldFrom.value);
        ADDOP(c, GET_YIELD_FROM_ITER);
        ADDOP_LOAD_CONST(c, Py_None);
        ADDOP(c, YIELD_FROM);
        break;
    case Await_kind:
        if (!IS_TOP_LEVEL_AWAIT(c)){
            if (c->u->u_ste->ste_type != FunctionBlock){
                return compiler_error(c, "'await' outside function");
            }

            if (c->u->u_scope_type != COMPILER_SCOPE_ASYNC_FUNCTION &&
                    c->u->u_scope_type != COMPILER_SCOPE_COMPREHENSION){
                return compiler_error(c, "'await' outside async function");
            }
        }

        VISIT(c, expr, e->v.Await.value);
        ADDOP(c, GET_AWAITABLE);
        ADDOP_LOAD_CONST(c, Py_None);
        ADDOP(c, YIELD_FROM);
        break;
    case Compare_kind:
        return compiler_compare(c, e);
    case Call_kind:
        return compiler_call(c, e);
    case Constant_kind:
        ADDOP_LOAD_CONST(c, e->v.Constant.value);
        break;
    case JoinedStr_kind:
        return compiler_joined_str(c, e);
    case FormattedValue_kind:
        return compiler_formatted_value(c, e);
    /* The following exprs can be assignment targets. */
    case Attribute_kind:
        VISIT(c, expr, e->v.Attribute.value);
        switch (e->v.Attribute.ctx) {
        case Load:
            ADDOP_NAME(c, LOAD_ATTR, e->v.Attribute.attr, names);
            break;
        case Store:
            if (forbidden_name(c, e->v.Attribute.attr, e->v.Attribute.ctx))
                return 0;
            ADDOP_NAME(c, STORE_ATTR, e->v.Attribute.attr, names);
            break;
        case Del:
            ADDOP_NAME(c, DELETE_ATTR, e->v.Attribute.attr, names);
            break;
        }
        break;
    case Subscript_kind:
        return compiler_subscript(c, e);
    case Starred_kind:
        switch (e->v.Starred.ctx) {
        case Store:
            /* In all legitimate cases, the Starred node was already replaced
             * by compiler_list/compiler_tuple. XXX: is that okay? */
            return compiler_error(c,
                "starred assignment target must be in a list or tuple");
        default:
            return compiler_error(c,
                "can't use starred expression here");
        }
        break;
    case Slice_kind:
        return compiler_slice(c, e);
    case Name_kind:
        return compiler_nameop(c, e->v.Name.id, e->v.Name.ctx);
    /* child nodes of List and Tuple will have expr_context set */
    case List_kind:
        return compiler_list(c, e);
    case Tuple_kind:
        return compiler_tuple(c, e);
    }
    return 1;
}

static int
compiler_visit_expr(struct compiler *c, expr_ty e)
{
    int old_lineno = c->u->u_lineno;
    int old_col_offset = c->u->u_col_offset;
    SET_LOC(c, e);
    int res = compiler_visit_expr1(c, e);
    c->u->u_lineno = old_lineno;
    c->u->u_col_offset = old_col_offset;
    return res;
}

static int
compiler_augassign(struct compiler *c, stmt_ty s)
{
    assert(s->kind == AugAssign_kind);
    expr_ty e = s->v.AugAssign.target;

    int old_lineno = c->u->u_lineno;
    int old_col_offset = c->u->u_col_offset;
    SET_LOC(c, e);

    switch (e->kind) {
    case Attribute_kind:
        VISIT(c, expr, e->v.Attribute.value);
        ADDOP(c, DUP_TOP);
        ADDOP_NAME(c, LOAD_ATTR, e->v.Attribute.attr, names);
        break;
    case Subscript_kind:
        VISIT(c, expr, e->v.Subscript.value);
        VISIT(c, expr, e->v.Subscript.slice);
        ADDOP(c, DUP_TOP_TWO);
        ADDOP(c, BINARY_SUBSCR);
        break;
    case Name_kind:
        if (!compiler_nameop(c, e->v.Name.id, Load))
            return 0;
        break;
    default:
        PyErr_Format(PyExc_SystemError,
            "invalid node type (%d) for augmented assignment",
            e->kind);
        return 0;
    }

    c->u->u_lineno = old_lineno;
    c->u->u_col_offset = old_col_offset;

    VISIT(c, expr, s->v.AugAssign.value);
    ADDOP(c, inplace_binop(s->v.AugAssign.op));

    SET_LOC(c, e);

    switch (e->kind) {
    case Attribute_kind:
        ADDOP(c, ROT_TWO);
        ADDOP_NAME(c, STORE_ATTR, e->v.Attribute.attr, names);
        break;
    case Subscript_kind:
        ADDOP(c, ROT_THREE);
        ADDOP(c, STORE_SUBSCR);
        break;
    case Name_kind:
        return compiler_nameop(c, e->v.Name.id, Store);
    default:
        Py_UNREACHABLE();
    }
    return 1;
}

static int
check_ann_expr(struct compiler *c, expr_ty e)
{
    VISIT(c, expr, e);
    ADDOP(c, POP_TOP);
    return 1;
}

static int
check_annotation(struct compiler *c, stmt_ty s)
{
    /* Annotations are only evaluated in a module or class. */
    if (c->u->u_scope_type == COMPILER_SCOPE_MODULE ||
        c->u->u_scope_type == COMPILER_SCOPE_CLASS) {
        return check_ann_expr(c, s->v.AnnAssign.annotation);
    }
    return 1;
}

static int
check_ann_subscr(struct compiler *c, expr_ty e)
{
    /* We check that everything in a subscript is defined at runtime. */
    switch (e->kind) {
    case Slice_kind:
        if (e->v.Slice.lower && !check_ann_expr(c, e->v.Slice.lower)) {
            return 0;
        }
        if (e->v.Slice.upper && !check_ann_expr(c, e->v.Slice.upper)) {
            return 0;
        }
        if (e->v.Slice.step && !check_ann_expr(c, e->v.Slice.step)) {
            return 0;
        }
        return 1;
    case Tuple_kind: {
        /* extended slice */
        asdl_seq *elts = e->v.Tuple.elts;
        Py_ssize_t i, n = asdl_seq_LEN(elts);
        for (i = 0; i < n; i++) {
            if (!check_ann_subscr(c, asdl_seq_GET(elts, i))) {
                return 0;
            }
        }
        return 1;
    }
    default:
        return check_ann_expr(c, e);
    }
}

static int
compiler_annassign(struct compiler *c, stmt_ty s)
{
    expr_ty targ = s->v.AnnAssign.target;
    PyObject* mangled;

    assert(s->kind == AnnAssign_kind);

    /* We perform the actual assignment first. */
    if (s->v.AnnAssign.value) {
        VISIT(c, expr, s->v.AnnAssign.value);
        VISIT(c, expr, targ);
    }
    switch (targ->kind) {
    case Name_kind:
        if (forbidden_name(c, targ->v.Name.id, Store))
            return 0;
        /* If we have a simple name in a module or class, store annotation. */
        if (s->v.AnnAssign.simple &&
            (c->u->u_scope_type == COMPILER_SCOPE_MODULE ||
             c->u->u_scope_type == COMPILER_SCOPE_CLASS)) {
            if (c->c_future->ff_features & CO_FUTURE_ANNOTATIONS) {
                VISIT(c, annexpr, s->v.AnnAssign.annotation)
            }
            else {
                VISIT(c, expr, s->v.AnnAssign.annotation);
            }
            ADDOP_NAME(c, LOAD_NAME, __annotations__, names);
            mangled = _Py_Mangle(c->u->u_private, targ->v.Name.id);
            ADDOP_LOAD_CONST_NEW(c, mangled);
            ADDOP(c, STORE_SUBSCR);
        }
        break;
    case Attribute_kind:
        if (forbidden_name(c, targ->v.Attribute.attr, Store))
            return 0;
        if (!s->v.AnnAssign.value &&
            !check_ann_expr(c, targ->v.Attribute.value)) {
            return 0;
        }
        break;
    case Subscript_kind:
        if (!s->v.AnnAssign.value &&
            (!check_ann_expr(c, targ->v.Subscript.value) ||
             !check_ann_subscr(c, targ->v.Subscript.slice))) {
                return 0;
        }
        break;
    default:
        PyErr_Format(PyExc_SystemError,
                     "invalid node type (%d) for annotated assignment",
                     targ->kind);
            return 0;
    }
    /* Annotation is evaluated last. */
    if (!s->v.AnnAssign.simple && !check_annotation(c, s)) {
        return 0;
    }
    return 1;
}

/* Raises a SyntaxError and returns 0.
   If something goes wrong, a different exception may be raised.
*/

static int
compiler_error(struct compiler *c, const char *errstr)
{
    PyObject *loc;
    PyObject *u = NULL, *v = NULL;

    loc = PyErr_ProgramTextObject(c->c_filename, c->u->u_lineno);
    if (!loc) {
        Py_INCREF(Py_None);
        loc = Py_None;
    }
    u = Py_BuildValue("(OiiO)", c->c_filename, c->u->u_lineno,
                      c->u->u_col_offset + 1, loc);
    if (!u)
        goto exit;
    v = Py_BuildValue("(zO)", errstr, u);
    if (!v)
        goto exit;
    PyErr_SetObject(PyExc_SyntaxError, v);
 exit:
    Py_DECREF(loc);
    Py_XDECREF(u);
    Py_XDECREF(v);
    return 0;
}

/* Emits a SyntaxWarning and returns 1 on success.
   If a SyntaxWarning raised as error, replaces it with a SyntaxError
   and returns 0.
*/
static int
compiler_warn(struct compiler *c, const char *format, ...)
{
    va_list vargs;
#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
    PyObject *msg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (msg == NULL) {
        return 0;
    }
    if (PyErr_WarnExplicitObject(PyExc_SyntaxWarning, msg, c->c_filename,
                                 c->u->u_lineno, NULL, NULL) < 0)
    {
        if (PyErr_ExceptionMatches(PyExc_SyntaxWarning)) {
            /* Replace the SyntaxWarning exception with a SyntaxError
               to get a more accurate error report */
            PyErr_Clear();
            assert(PyUnicode_AsUTF8(msg) != NULL);
            compiler_error(c, PyUnicode_AsUTF8(msg));
        }
        Py_DECREF(msg);
        return 0;
    }
    Py_DECREF(msg);
    return 1;
}

static int
compiler_subscript(struct compiler *c, expr_ty e)
{
    expr_context_ty ctx = e->v.Subscript.ctx;
    int op = 0;

    if (ctx == Load) {
        if (!check_subscripter(c, e->v.Subscript.value)) {
            return 0;
        }
        if (!check_index(c, e->v.Subscript.value, e->v.Subscript.slice)) {
            return 0;
        }
    }

    switch (ctx) {
        case Load:    op = BINARY_SUBSCR; break;
        case Store:   op = STORE_SUBSCR; break;
        case Del:     op = DELETE_SUBSCR; break;
    }
    assert(op);
    VISIT(c, expr, e->v.Subscript.value);
    VISIT(c, expr, e->v.Subscript.slice);
    ADDOP(c, op);
    return 1;
}

static int
compiler_slice(struct compiler *c, expr_ty s)
{
    int n = 2;
    assert(s->kind == Slice_kind);

    /* only handles the cases where BUILD_SLICE is emitted */
    if (s->v.Slice.lower) {
        VISIT(c, expr, s->v.Slice.lower);
    }
    else {
        ADDOP_LOAD_CONST(c, Py_None);
    }

    if (s->v.Slice.upper) {
        VISIT(c, expr, s->v.Slice.upper);
    }
    else {
        ADDOP_LOAD_CONST(c, Py_None);
    }

    if (s->v.Slice.step) {
        n++;
        VISIT(c, expr, s->v.Slice.step);
    }
    ADDOP_I(c, BUILD_SLICE, n);
    return 1;
}

/* End of the compiler section, beginning of the assembler section */

/* do depth-first search of basic block graph, starting with block.
   post records the block indices in post-order.

   XXX must handle implicit jumps from one block to next
*/

struct assembler {
    PyObject *a_bytecode;  /* string containing bytecode */
    int a_offset;              /* offset into bytecode */
    int a_nblocks;             /* number of reachable blocks */
    basicblock **a_reverse_postorder; /* list of blocks in dfs postorder */
    PyObject *a_lnotab;    /* string containing lnotab */
    int a_lnotab_off;      /* offset into lnotab */
    int a_lineno;              /* last lineno of emitted instruction */
    int a_lineno_off;      /* bytecode offset of last lineno */
};

static void
dfs(struct compiler *c, basicblock *b, struct assembler *a, int end)
{

    /* There is no real depth-first-search to do here because all the
     * blocks are emitted in topological order already, so we just need to
     * follow the b_next pointers and place them in a->a_reverse_postorder in
     * reverse order and make sure that the first one starts at 0. */

    for (a->a_nblocks = 0; b != NULL; b = b->b_next) {
        a->a_reverse_postorder[a->a_nblocks++] = b;
    }
}

Py_LOCAL_INLINE(void)
stackdepth_push(basicblock ***sp, basicblock *b, int depth)
{
    assert(b->b_startdepth < 0 || b->b_startdepth == depth);
    if (b->b_startdepth < depth && b->b_startdepth < 100) {
        assert(b->b_startdepth < 0);
        b->b_startdepth = depth;
        *(*sp)++ = b;
    }
}

/* Find the flow path that needs the largest stack.  We assume that
 * cycles in the flow graph have no net effect on the stack depth.
 */
static int
stackdepth(struct compiler *c)
{
    basicblock *b, *entryblock = NULL;
    basicblock **stack, **sp;
    int nblocks = 0, maxdepth = 0;
    for (b = c->u->u_blocks; b != NULL; b = b->b_list) {
        b->b_startdepth = INT_MIN;
        entryblock = b;
        nblocks++;
    }
    if (!entryblock)
        return 0;
    stack = (basicblock **)PyObject_Malloc(sizeof(basicblock *) * nblocks);
    if (!stack) {
        PyErr_NoMemory();
        return -1;
    }

    sp = stack;
    stackdepth_push(&sp, entryblock, 0);
    while (sp != stack) {
        b = *--sp;
        int depth = b->b_startdepth;
        assert(depth >= 0);
        basicblock *next = b->b_next;
        for (int i = 0; i < b->b_iused; i++) {
            struct instr *instr = &b->b_instr[i];
            int effect = stack_effect(instr->i_opcode, instr->i_oparg, 0);
            if (effect == PY_INVALID_STACK_EFFECT) {
                _Py_FatalErrorFormat(__func__,
                    "opcode = %d", instr->i_opcode);
            }
            int new_depth = depth + effect;
            if (new_depth > maxdepth) {
                maxdepth = new_depth;
            }
            assert(depth >= 0); /* invalid code or bug in stackdepth() */
            if (is_jump(instr)) {
                effect = stack_effect(instr->i_opcode, instr->i_oparg, 1);
                assert(effect != PY_INVALID_STACK_EFFECT);
                int target_depth = depth + effect;
                if (target_depth > maxdepth) {
                    maxdepth = target_depth;
                }
                assert(target_depth >= 0); /* invalid code or bug in stackdepth() */
                stackdepth_push(&sp, instr->i_target, target_depth);
            }
            depth = new_depth;
            if (instr->i_opcode == JUMP_ABSOLUTE ||
                instr->i_opcode == JUMP_FORWARD ||
                instr->i_opcode == RETURN_VALUE ||
                instr->i_opcode == RAISE_VARARGS ||
                instr->i_opcode == RERAISE)
            {
                /* remaining code is dead */
                next = NULL;
                break;
            }
        }
        if (next != NULL) {
            stackdepth_push(&sp, next, depth);
        }
    }
    PyObject_Free(stack);
    return maxdepth;
}

static int
assemble_init(struct assembler *a, int nblocks, int firstlineno)
{
    memset(a, 0, sizeof(struct assembler));
    a->a_lineno = firstlineno;
    a->a_bytecode = PyBytes_FromStringAndSize(NULL, DEFAULT_CODE_SIZE);
    if (!a->a_bytecode)
        return 0;
    a->a_lnotab = PyBytes_FromStringAndSize(NULL, DEFAULT_LNOTAB_SIZE);
    if (!a->a_lnotab)
        return 0;
    if ((size_t)nblocks > SIZE_MAX / sizeof(basicblock *)) {
        PyErr_NoMemory();
        return 0;
    }
    a->a_reverse_postorder = (basicblock **)PyObject_Malloc(
                                        sizeof(basicblock *) * nblocks);
    if (!a->a_reverse_postorder) {
        PyErr_NoMemory();
        return 0;
    }
    return 1;
}

static void
assemble_free(struct assembler *a)
{
    Py_XDECREF(a->a_bytecode);
    Py_XDECREF(a->a_lnotab);
    if (a->a_reverse_postorder)
        PyObject_Free(a->a_reverse_postorder);
}

static int
blocksize(basicblock *b)
{
    int i;
    int size = 0;

    for (i = 0; i < b->b_iused; i++)
        size += instrsize(b->b_instr[i].i_oparg);
    return size;
}

/* Appends a pair to the end of the line number table, a_lnotab, representing
   the instruction's bytecode offset and line number.  See
   Objects/lnotab_notes.txt for the description of the line number table. */

static int
assemble_lnotab(struct assembler *a, struct instr *i)
{
    int d_bytecode, d_lineno;
    Py_ssize_t len;
    unsigned char *lnotab;

    d_lineno = i->i_lineno - a->a_lineno;
    if (d_lineno == 0) {
        return 1;
    }

    d_bytecode = (a->a_offset - a->a_lineno_off) * sizeof(_Py_CODEUNIT);
    assert(d_bytecode >= 0);

    if (d_bytecode > 255) {
        int j, nbytes, ncodes = d_bytecode / 255;
        nbytes = a->a_lnotab_off + 2 * ncodes;
        len = PyBytes_GET_SIZE(a->a_lnotab);
        if (nbytes >= len) {
            if ((len <= INT_MAX / 2) && (len * 2 < nbytes))
                len = nbytes;
            else if (len <= INT_MAX / 2)
                len *= 2;
            else {
                PyErr_NoMemory();
                return 0;
            }
            if (_PyBytes_Resize(&a->a_lnotab, len) < 0)
                return 0;
        }
        lnotab = (unsigned char *)
                   PyBytes_AS_STRING(a->a_lnotab) + a->a_lnotab_off;
        for (j = 0; j < ncodes; j++) {
            *lnotab++ = 255;
            *lnotab++ = 0;
        }
        d_bytecode -= ncodes * 255;
        a->a_lnotab_off += ncodes * 2;
    }
    assert(0 <= d_bytecode && d_bytecode <= 255);

    if (d_lineno < -128 || 127 < d_lineno) {
        int j, nbytes, ncodes, k;
        if (d_lineno < 0) {
            k = -128;
            /* use division on positive numbers */
            ncodes = (-d_lineno) / 128;
        }
        else {
            k = 127;
            ncodes = d_lineno / 127;
        }
        d_lineno -= ncodes * k;
        assert(ncodes >= 1);
        nbytes = a->a_lnotab_off + 2 * ncodes;
        len = PyBytes_GET_SIZE(a->a_lnotab);
        if (nbytes >= len) {
            if ((len <= INT_MAX / 2) && len * 2 < nbytes)
                len = nbytes;
            else if (len <= INT_MAX / 2)
                len *= 2;
            else {
                PyErr_NoMemory();
                return 0;
            }
            if (_PyBytes_Resize(&a->a_lnotab, len) < 0)
                return 0;
        }
        lnotab = (unsigned char *)
                   PyBytes_AS_STRING(a->a_lnotab) + a->a_lnotab_off;
        *lnotab++ = d_bytecode;
        *lnotab++ = k;
        d_bytecode = 0;
        for (j = 1; j < ncodes; j++) {
            *lnotab++ = 0;
            *lnotab++ = k;
        }
        a->a_lnotab_off += ncodes * 2;
    }
    assert(-128 <= d_lineno && d_lineno <= 127);

    len = PyBytes_GET_SIZE(a->a_lnotab);
    if (a->a_lnotab_off + 2 >= len) {
        if (_PyBytes_Resize(&a->a_lnotab, len * 2) < 0)
            return 0;
    }
    lnotab = (unsigned char *)
                    PyBytes_AS_STRING(a->a_lnotab) + a->a_lnotab_off;

    a->a_lnotab_off += 2;
    if (d_bytecode) {
        *lnotab++ = d_bytecode;
        *lnotab++ = d_lineno;
    }
    else {      /* First line of a block; def stmt, etc. */
        *lnotab++ = 0;
        *lnotab++ = d_lineno;
    }
    a->a_lineno = i->i_lineno;
    a->a_lineno_off = a->a_offset;
    return 1;
}

/* assemble_emit()
   Extend the bytecode with a new instruction.
   Update lnotab if necessary.
*/

static int
assemble_emit(struct assembler *a, struct instr *i)
{
    int size, arg = 0;
    Py_ssize_t len = PyBytes_GET_SIZE(a->a_bytecode);
    _Py_CODEUNIT *code;

    arg = i->i_oparg;
    size = instrsize(arg);
    if (i->i_lineno && !assemble_lnotab(a, i))
        return 0;
    if (a->a_offset + size >= len / (int)sizeof(_Py_CODEUNIT)) {
        if (len > PY_SSIZE_T_MAX / 2)
            return 0;
        if (_PyBytes_Resize(&a->a_bytecode, len * 2) < 0)
            return 0;
    }
    code = (_Py_CODEUNIT *)PyBytes_AS_STRING(a->a_bytecode) + a->a_offset;
    a->a_offset += size;
    write_op_arg(code, i->i_opcode, arg, size);
    return 1;
}

static void
assemble_jump_offsets(struct assembler *a, struct compiler *c)
{
    basicblock *b;
    int bsize, totsize, extended_arg_recompile;
    int i;

    /* Compute the size of each block and fixup jump args.
       Replace block pointer with position in bytecode. */
    do {
        totsize = 0;
        for (i = 0; i < a->a_nblocks; i++) {
            b = a->a_reverse_postorder[i];
            bsize = blocksize(b);
            b->b_offset = totsize;
            totsize += bsize;
        }
        extended_arg_recompile = 0;
        for (b = c->u->u_blocks; b != NULL; b = b->b_list) {
            bsize = b->b_offset;
            for (i = 0; i < b->b_iused; i++) {
                struct instr *instr = &b->b_instr[i];
                int isize = instrsize(instr->i_oparg);
                /* Relative jumps are computed relative to
                   the instruction pointer after fetching
                   the jump instruction.
                */
                bsize += isize;
                if (is_jump(instr)) {
                    instr->i_oparg = instr->i_target->b_offset;
                    if (is_relative_jump(instr)) {
                        instr->i_oparg -= bsize;
                    }
                    instr->i_oparg *= sizeof(_Py_CODEUNIT);
                    if (instrsize(instr->i_oparg) != isize) {
                        extended_arg_recompile = 1;
                    }
                }
            }
        }

    /* XXX: This is an awful hack that could hurt performance, but
        on the bright side it should work until we come up
        with a better solution.

        The issue is that in the first loop blocksize() is called
        which calls instrsize() which requires i_oparg be set
        appropriately. There is a bootstrap problem because
        i_oparg is calculated in the second loop above.

        So we loop until we stop seeing new EXTENDED_ARGs.
        The only EXTENDED_ARGs that could be popping up are
        ones in jump instructions.  So this should converge
        fairly quickly.
    */
    } while (extended_arg_recompile);
}

static PyObject *
dict_keys_inorder(PyObject *dict, Py_ssize_t offset)
{
    PyObject *tuple, *k, *v;
    Py_ssize_t i, pos = 0, size = PyDict_GET_SIZE(dict);

    tuple = PyTuple_New(size);
    if (tuple == NULL)
        return NULL;
    while (PyDict_Next(dict, &pos, &k, &v)) {
        i = PyLong_AS_LONG(v);
        Py_INCREF(k);
        assert((i - offset) < size);
        assert((i - offset) >= 0);
        PyTuple_SET_ITEM(tuple, i - offset, k);
    }
    return tuple;
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
        i = PyLong_AS_LONG(v);
        /* The keys of the dictionary can be tuples wrapping a contant.
         * (see compiler_add_o and _PyCode_ConstantKey). In that case
         * the object we want is always second. */
        if (PyTuple_CheckExact(k)) {
            k = PyTuple_GET_ITEM(k, 1);
        }
        Py_INCREF(k);
        assert(i < size);
        assert(i >= 0);
        PyList_SET_ITEM(consts, i, k);
    }
    return consts;
}

static int
compute_code_flags(struct compiler *c)
{
    PySTEntryObject *ste = c->u->u_ste;
    int flags = 0;
    if (ste->ste_type == FunctionBlock) {
        flags |= CO_NEWLOCALS | CO_OPTIMIZED;
        if (ste->ste_nested)
            flags |= CO_NESTED;
        if (ste->ste_generator && !ste->ste_coroutine)
            flags |= CO_GENERATOR;
        if (!ste->ste_generator && ste->ste_coroutine)
            flags |= CO_COROUTINE;
        if (ste->ste_generator && ste->ste_coroutine)
            flags |= CO_ASYNC_GENERATOR;
        if (ste->ste_varargs)
            flags |= CO_VARARGS;
        if (ste->ste_varkeywords)
            flags |= CO_VARKEYWORDS;
    }

    /* (Only) inherit compilerflags in PyCF_MASK */
    flags |= (c->c_flags->cf_flags & PyCF_MASK);

    if ((IS_TOP_LEVEL_AWAIT(c)) &&
         ste->ste_coroutine &&
         !ste->ste_generator) {
        flags |= CO_COROUTINE;
    }

    return flags;
}

// Merge *tuple* with constant cache.
// Unlike merge_consts_recursive(), this function doesn't work recursively.
static int
merge_const_tuple(struct compiler *c, PyObject **tuple)
{
    assert(PyTuple_CheckExact(*tuple));

    PyObject *key = _PyCode_ConstantKey(*tuple);
    if (key == NULL) {
        return 0;
    }

    // t is borrowed reference
    PyObject *t = PyDict_SetDefault(c->c_const_cache, key, key);
    Py_DECREF(key);
    if (t == NULL) {
        return 0;
    }
    if (t == key) {  // tuple is new constant.
        return 1;
    }

    PyObject *u = PyTuple_GET_ITEM(t, 1);
    Py_INCREF(u);
    Py_DECREF(*tuple);
    *tuple = u;
    return 1;
}

static PyCodeObject *
makecode(struct compiler *c, struct assembler *a, PyObject *consts)
{
    PyCodeObject *co = NULL;
    PyObject *names = NULL;
    PyObject *varnames = NULL;
    PyObject *name = NULL;
    PyObject *freevars = NULL;
    PyObject *cellvars = NULL;
    Py_ssize_t nlocals;
    int nlocals_int;
    int flags;
    int posorkeywordargcount, posonlyargcount, kwonlyargcount, maxdepth;

    names = dict_keys_inorder(c->u->u_names, 0);
    varnames = dict_keys_inorder(c->u->u_varnames, 0);
    if (!names || !varnames) {
        goto error;
    }
    cellvars = dict_keys_inorder(c->u->u_cellvars, 0);
    if (!cellvars)
        goto error;
    freevars = dict_keys_inorder(c->u->u_freevars, PyTuple_GET_SIZE(cellvars));
    if (!freevars)
        goto error;

    if (!merge_const_tuple(c, &names) ||
            !merge_const_tuple(c, &varnames) ||
            !merge_const_tuple(c, &cellvars) ||
            !merge_const_tuple(c, &freevars))
    {
        goto error;
    }

    nlocals = PyDict_GET_SIZE(c->u->u_varnames);
    assert(nlocals < INT_MAX);
    nlocals_int = Py_SAFE_DOWNCAST(nlocals, Py_ssize_t, int);

    flags = compute_code_flags(c);
    if (flags < 0)
        goto error;

    consts = PyList_AsTuple(consts); /* PyCode_New requires a tuple */
    if (consts == NULL) {
        goto error;
    }
    if (!merge_const_tuple(c, &consts)) {
        Py_DECREF(consts);
        goto error;
    }

    posonlyargcount = Py_SAFE_DOWNCAST(c->u->u_posonlyargcount, Py_ssize_t, int);
    posorkeywordargcount = Py_SAFE_DOWNCAST(c->u->u_argcount, Py_ssize_t, int);
    kwonlyargcount = Py_SAFE_DOWNCAST(c->u->u_kwonlyargcount, Py_ssize_t, int);
    maxdepth = stackdepth(c);
    if (maxdepth < 0) {
        Py_DECREF(consts);
        goto error;
    }
    co = PyCode_NewWithPosOnlyArgs(posonlyargcount+posorkeywordargcount,
                                   posonlyargcount, kwonlyargcount, nlocals_int,
                                   maxdepth, flags, a->a_bytecode, consts, names,
                                   varnames, freevars, cellvars, c->c_filename,
                                   c->u->u_name, c->u->u_firstlineno, a->a_lnotab);
    Py_DECREF(consts);
 error:
    Py_XDECREF(names);
    Py_XDECREF(varnames);
    Py_XDECREF(name);
    Py_XDECREF(freevars);
    Py_XDECREF(cellvars);
    return co;
}


/* For debugging purposes only */
#if 0
static void
dump_instr(const struct instr *i)
{
    const char *jrel = (is_relative_jump(instr)) ? "jrel " : "";
    const char *jabs = (is_jump(instr) && !is_relative_jump(instr))? "jabs " : "";
    char arg[128];

    *arg = '\0';
    if (HAS_ARG(i->i_opcode)) {
        sprintf(arg, "arg: %d ", i->i_oparg);
    }
    fprintf(stderr, "line: %d, opcode: %d %s%s%s\n",
                    i->i_lineno, i->i_opcode, arg, jabs, jrel);
}

static void
dump_basicblock(const basicblock *b)
{
    const char *b_return = b->b_return ? "return " : "";
    fprintf(stderr, "used: %d, depth: %d, offset: %d %s\n",
        b->b_iused, b->b_startdepth, b->b_offset, b_return);
    if (b->b_instr) {
        int i;
        for (i = 0; i < b->b_iused; i++) {
            fprintf(stderr, "  [%02d] ", i);
            dump_instr(b->b_instr + i);
        }
    }
}
#endif

static int
optimize_cfg(struct assembler *a, PyObject *consts);

static PyCodeObject *
assemble(struct compiler *c, int addNone)
{
    basicblock *b, *entryblock;
    struct assembler a;
    int i, j, nblocks;
    PyCodeObject *co = NULL;
    PyObject *consts = NULL;

    /* Make sure every block that falls off the end returns None.
       XXX NEXT_BLOCK() isn't quite right, because if the last
       block ends with a jump or return b_next shouldn't set.
     */
    if (!c->u->u_curblock->b_return) {
        NEXT_BLOCK(c);
        if (addNone)
            ADDOP_LOAD_CONST(c, Py_None);
        ADDOP(c, RETURN_VALUE);
    }

    nblocks = 0;
    entryblock = NULL;
    for (b = c->u->u_blocks; b != NULL; b = b->b_list) {
        nblocks++;
        entryblock = b;
    }

    /* Set firstlineno if it wasn't explicitly set. */
    if (!c->u->u_firstlineno) {
        if (entryblock && entryblock->b_instr && entryblock->b_instr->i_lineno)
            c->u->u_firstlineno = entryblock->b_instr->i_lineno;
        else
            c->u->u_firstlineno = 1;
    }
    if (!assemble_init(&a, nblocks, c->u->u_firstlineno))
        goto error;
    dfs(c, entryblock, &a, nblocks);

    consts = consts_dict_keys_inorder(c->u->u_consts);
    if (consts == NULL) {
        goto error;
    }
    if (optimize_cfg(&a, consts)) {
        goto error;
    }

    /* Can't modify the bytecode after computing jump offsets. */
    assemble_jump_offsets(&a, c);

    /* Emit code in reverse postorder from dfs. */
    for (i = 0; i < a.a_nblocks; i++) {
        b = a.a_reverse_postorder[i];
        for (j = 0; j < b->b_iused; j++)
            if (!assemble_emit(&a, &b->b_instr[j]))
                goto error;
    }

    if (_PyBytes_Resize(&a.a_lnotab, a.a_lnotab_off) < 0)
        goto error;
    if (_PyBytes_Resize(&a.a_bytecode, a.a_offset * sizeof(_Py_CODEUNIT)) < 0)
        goto error;

    co = makecode(c, &a, consts);
 error:
    Py_XDECREF(consts);
    assemble_free(&a);
    return co;
}

#undef PyAST_Compile
PyCodeObject *
PyAST_Compile(mod_ty mod, const char *filename, PyCompilerFlags *flags,
              PyArena *arena)
{
    return PyAST_CompileEx(mod, filename, flags, -1, arena);
}


/* Replace LOAD_CONST c1, LOAD_CONST c2 ... LOAD_CONST cn, BUILD_TUPLE n
   with    LOAD_CONST (c1, c2, ... cn).
   The consts table must still be in list form so that the
   new constant (c1, c2, ... cn) can be appended.
   Called with codestr pointing to the first LOAD_CONST.
*/
static int
fold_tuple_on_constants(struct instr *inst,
                        int n, PyObject *consts)
{
    /* Pre-conditions */
    assert(PyList_CheckExact(consts));
    assert(inst[n].i_opcode == BUILD_TUPLE);
    assert(inst[n].i_oparg == n);

    for (int i = 0; i < n; i++) {
        if (inst[i].i_opcode != LOAD_CONST) {
            return 0;
        }
    }

    /* Buildup new tuple of constants */
    PyObject *newconst = PyTuple_New(n);
    if (newconst == NULL) {
        return -1;
    }
    for (int i = 0; i < n; i++) {
        int arg = inst[i].i_oparg;
        PyObject *constant = PyList_GET_ITEM(consts, arg);
        Py_INCREF(constant);
        PyTuple_SET_ITEM(newconst, i, constant);
    }
    Py_ssize_t index = PyList_GET_SIZE(consts);
#if SIZEOF_SIZE_T > SIZEOF_INT
    if ((size_t)index >= UINT_MAX - 1) {
        Py_DECREF(newconst);
        PyErr_SetString(PyExc_OverflowError, "too many constants");
        return -1;
    }
#endif
    if (PyList_Append(consts, newconst)) {
        Py_DECREF(newconst);
        return -1;
    }
    Py_DECREF(newconst);
    for (int i = 0; i < n; i++) {
        inst[i].i_opcode = NOP;
    }
    inst[n].i_opcode = LOAD_CONST;
    inst[n].i_oparg = index;
    return 0;
}


/* Optimization */
static int
optimize_basic_block(basicblock *bb, PyObject *consts)
{
    assert(PyList_CheckExact(consts));
    struct instr nop;
    nop.i_opcode = NOP;
    struct instr *target;
    int lineno;
    for (int i = 0; i < bb->b_iused; i++) {
        struct instr *inst = &bb->b_instr[i];
        int oparg = inst->i_oparg;
        int nextop = i+1 < bb->b_iused ? bb->b_instr[i+1].i_opcode : 0;
        if (is_jump(inst)) {
            /* Skip over empty basic blocks. */
            while (inst->i_target->b_iused == 0) {
                inst->i_target = inst->i_target->b_next;
            }
            target = &inst->i_target->b_instr[0];
        }
        else {
            target = &nop;
        }
        switch (inst->i_opcode) {
            /* Skip over LOAD_CONST trueconst
                   POP_JUMP_IF_FALSE xx.  This improves
                   "while 1" performance.  */
            case LOAD_CONST:
                if (nextop != POP_JUMP_IF_FALSE) {
                    break;
                }
                PyObject* cnt = PyList_GET_ITEM(consts, oparg);
                int is_true = PyObject_IsTrue(cnt);
                if (is_true == -1) {
                    goto error;
                }
                if (is_true == 1) {
                    inst->i_opcode = NOP;
                    bb->b_instr[i+1].i_opcode = NOP;
                }
                break;

                /* Try to fold tuples of constants.
                   Skip over BUILD_SEQN 1 UNPACK_SEQN 1.
                   Replace BUILD_SEQN 2 UNPACK_SEQN 2 with ROT2.
                   Replace BUILD_SEQN 3 UNPACK_SEQN 3 with ROT3 ROT2. */
            case BUILD_TUPLE:
                if (nextop == UNPACK_SEQUENCE && oparg == bb->b_instr[i+1].i_oparg) {
                    switch(oparg) {
                        case 1:
                            inst->i_opcode = NOP;
                            bb->b_instr[i+1].i_opcode = NOP;
                            break;
                        case 2:
                            inst->i_opcode = ROT_TWO;
                            bb->b_instr[i+1].i_opcode = NOP;
                            break;
                        case 3:
                            inst->i_opcode = ROT_THREE;
                            bb->b_instr[i+1].i_opcode = ROT_TWO;
                    }
                    break;
                }
                if (i >= oparg) {
                    if (fold_tuple_on_constants(inst-oparg, oparg, consts)) {
                        goto error;
                    }
                }
                break;

                /* Simplify conditional jump to conditional jump where the
                   result of the first test implies the success of a similar
                   test or the failure of the opposite test.
                   Arises in code like:
                   "a and b or c"
                   "(a and b) and c"
                   "(a or b) or c"
                   "(a or b) and c"
                   x:JUMP_IF_FALSE_OR_POP y   y:JUMP_IF_FALSE_OR_POP z
                      -->  x:JUMP_IF_FALSE_OR_POP z
                   x:JUMP_IF_FALSE_OR_POP y   y:JUMP_IF_TRUE_OR_POP z
                      -->  x:POP_JUMP_IF_FALSE y+1
                   where y+1 is the instruction following the second test.
                */
            case JUMP_IF_FALSE_OR_POP:
                switch(target->i_opcode) {
                    case POP_JUMP_IF_FALSE:
                        *inst = *target;
                        break;
                    case JUMP_ABSOLUTE:
                    case JUMP_FORWARD:
                    case JUMP_IF_FALSE_OR_POP:
                        inst->i_target = target->i_target;
                        break;
                    case JUMP_IF_TRUE_OR_POP:
                        assert (inst->i_target->b_iused == 1);
                        inst->i_opcode = POP_JUMP_IF_FALSE;
                        inst->i_target = inst->i_target->b_next;
                        break;
                }
                break;

            case JUMP_IF_TRUE_OR_POP:
                switch(target->i_opcode) {
                    case POP_JUMP_IF_TRUE:
                        *inst = *target;
                        break;
                    case JUMP_ABSOLUTE:
                    case JUMP_FORWARD:
                    case JUMP_IF_TRUE_OR_POP:
                        inst->i_target = target->i_target;
                        break;
                    case JUMP_IF_FALSE_OR_POP:
                        assert (inst->i_target->b_iused == 1);
                        inst->i_opcode = POP_JUMP_IF_TRUE;
                        inst->i_target = inst->i_target->b_next;
                        break;
                }
                break;

            case POP_JUMP_IF_FALSE:
                switch(target->i_opcode) {
                    case JUMP_ABSOLUTE:
                    case JUMP_FORWARD:
                        inst->i_target = target->i_target;
                        break;
                }
                break;

            case POP_JUMP_IF_TRUE:
                switch(target->i_opcode) {
                    case JUMP_ABSOLUTE:
                    case JUMP_FORWARD:
                        inst->i_target = target->i_target;
                        break;
                }
                break;

            case JUMP_ABSOLUTE:
            case JUMP_FORWARD:
                switch(target->i_opcode) {
                    case JUMP_FORWARD:
                        inst->i_target = target->i_target;
                        break;
                    case JUMP_ABSOLUTE:
                    case RETURN_VALUE:
                    case RERAISE:
                    case RAISE_VARARGS:
                        lineno = inst->i_lineno;
                        *inst = *target;
                        inst->i_lineno = lineno;
                        break;
                }
                break;
        }
    }
    return 0;
error:
    return -1;
}


static void
clean_basic_block(basicblock *bb) {
    /* Remove NOPs and any code following a return or re-raise. */
    int dest = 0;
    for (int src = 0; src < bb->b_iused; src++) {
        switch(bb->b_instr[src].i_opcode) {
            case NOP:
                /* skip */
                break;
            case RETURN_VALUE:
            case RERAISE:
                bb->b_next = NULL;
                bb->b_instr[dest] = bb->b_instr[src];
                dest++;
                goto end;
            default:
                if (dest != src) {
                    bb->b_instr[dest] = bb->b_instr[src];
                }
                dest++;
                break;
        }
    }
end:
    assert(dest <= bb->b_iused);
    bb->b_iused = dest;
}

static int
mark_reachable(struct assembler *a) {
    basicblock **stack, **sp;
    sp = stack = (basicblock **)PyObject_Malloc(sizeof(basicblock *) * a->a_nblocks);
    if (stack == NULL) {
        return -1;
    }
    basicblock *entry = a->a_reverse_postorder[0];
    entry->b_reachable = 1;
    *sp++ = entry;
    while (sp > stack) {
        basicblock *b = *(--sp);
        if (b->b_next && b->b_next->b_reachable == 0) {
            b->b_next->b_reachable = 1;
            *sp++ = b->b_next;
        }
        for (int i = 0; i < b->b_iused; i++) {
            basicblock *target;
            if (is_jump(&b->b_instr[i])) {
                target = b->b_instr[i].i_target;
                if (target->b_reachable == 0) {
                    target->b_reachable = 1;
                    *sp++ = target;
                }
            }
        }
    }
    PyObject_Free(stack);
    return 0;
}


/* Perform basic peephole optimizations on a control flow graph.
   The consts object should still be in list form to allow new constants
   to be appended.

   All transformations keep the code size the same or smaller.
   For those that reduce size, the gaps are initially filled with
   NOPs.  Later those NOPs are removed.
*/

static int
optimize_cfg(struct assembler *a, PyObject *consts)
{
    for (int i = 0; i < a->a_nblocks; i++) {
        if (optimize_basic_block(a->a_reverse_postorder[i], consts)) {
            return -1;
        }
        clean_basic_block(a->a_reverse_postorder[i]);
        assert(a->a_reverse_postorder[i]->b_reachable == 0);
    }
    if (mark_reachable(a)) {
        return -1;
    }
    /* Delete unreachable instructions */
    for (int i = 0; i < a->a_nblocks; i++) {
       if (a->a_reverse_postorder[i]->b_reachable == 0) {
            a->a_reverse_postorder[i]->b_iused = 0;
       }
    }
    return 0;
}

/* Retained for API compatibility.
 * Optimization is now done in optimize_cfg */

PyObject *
PyCode_Optimize(PyObject *code, PyObject* Py_UNUSED(consts),
                PyObject *Py_UNUSED(names), PyObject *Py_UNUSED(lnotab_obj))
{
    Py_INCREF(code);
    return code;
}

