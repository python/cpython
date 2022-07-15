/*
 * This file compiles an abstract syntax tree (AST) into Python bytecode.
 *
 * The primary entry point is _PyAST_Compile(), which returns a
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
 */

#include <stdbool.h>

#include "Python.h"
#include "pycore_ast.h"
#include "pycore_compile.h"       // _PyFuture_FromAST()
#include "pycore_symtable.h"      // PySTEntryObject

#define DEBUGGING 0

#if DEBUGGING
#define NEED_STRING_FOR_ENUM
#endif

#include "opcode.h"

#define CHECK(X) if(!(X)) return 0

static inline int
intern_static_identifier(identifier *identifier, const char *str) {
    return *identifier || (*identifier = PyUnicode_InternFromString(str));
}

/* A soft limit for tmp register use, to avoid excessive
 * memory use for large constants, etc.
 *
 * The value 30 is plucked out of thin air.
 * Code that could use more stack than this is
 * rare, so the exact value is unimportant.
*/
#define TMP_USE_GUIDELINE 30

/* If we exceed this limit, it should
 * be considered a compiler bug.
 * Currently it should be impossible
 * to exceed TMP_USE_GUIDELINE * 100,
 * as 100 is the maximum parse depth.
 * For performance reasons we will
 * want to reduce this to a
 * few hundred in the future.
 *
 * NOTE: Whatever MAX_ALLOWED_TMP_USE is
 * set to, it should never restrict what Python
 * we can write, just how we compile it.
*/
#define MAX_ALLOWED_TMP_USE (TMP_USE_GUIDELINE * 100)

/* Not all NOP can be deleted since we need to keep the line number,
 * The deletable NOP is set to DELETED_OP after optimization,
 * they emit nothing in the bytecode emit phase. */
static const Opcode DELETED_OP = _INVALID_OPCODE_255;

enum OpargType {
    OpargX, OpargImm, OpargConst, OpargFast, OpargTmp
};

typedef size_t Oparg;
#define TYPED_OPARG(T, V) ((T) | ((V) << 3))
#define OPARG_EQ(A, B) ((A) == (B))
#define OPARG_TYPE(A) ((enum OpargType) ((A) & 0b111))
#define OPARG_VALUE(A) ((A) >> 3)
/* OPARG_FINAL_VALUE should be able to return a l-value */
#define OPARG_FINAL_VALUE(A) (A)

static inline Oparg
make_oparg(enum OpargType type, Py_ssize_t value) {
    assert(value >= 0);
    Oparg oparg = TYPED_OPARG(type, value);
    assert((Py_ssize_t) OPARG_VALUE(oparg) == value);
    return oparg;
}

static inline Oparg
imm_oparg(Py_ssize_t value) {
    return make_oparg(OpargImm, value);
}

static inline Oparg
tmp_oparg(Py_ssize_t value) {
    return make_oparg(OpargTmp, value);
}

static inline Oparg
oparg_by_offset(Oparg base, Py_ssize_t offset) {
    return make_oparg(OPARG_TYPE(base), OPARG_VALUE(base) + offset);
}

static const Oparg RELOC_OPARG = TYPED_OPARG(OpargX, 0);
static const Oparg UNSED_OPARG = TYPED_OPARG(OpargX, 1);
static const Oparg NEG_ADDR_OPARG = TYPED_OPARG(OpargX, 2);
static const Oparg ADDR_OPARG = TYPED_OPARG(OpargX, 3);

struct instr {
    int i_lineno;
    Opcode i_opcode;
    Oparg i_opargs[3];
};

/* Minimum number of code units necessary to encode instruction with
   EXTENDED_ARGs */
static int
instrsize(struct instr *i) {
    size_t arg_or_bits = \
            OPARG_FINAL_VALUE(i->i_opargs[0]) | \
            OPARG_FINAL_VALUE(i->i_opargs[1]) | \
            OPARG_FINAL_VALUE(i->i_opargs[2]);
    int len = 1;
    while (arg_or_bits >>= 8) {
        len++;
    }
    return len;
}

static inline void
set_to_nop(struct instr *i) {
    i->i_opcode = NOP;
    i->i_opargs[0] = i->i_opargs[1] = i->i_opargs[2] = UNSED_OPARG;
}

typedef struct basicblock_ {
    /* Each basicblock in a compilation unit is linked via b_prev_allocated
       in the reverse order that the block are allocated. */
    struct basicblock_ *b_prev_allocated;
    /* The block it falls through, if non-NULL and not b_nofallthrough */
    struct basicblock_ *b_fall;
    /* The block it jumps to, if non-NULL */
    struct basicblock_ *b_jump;
    /* Some fields are never used at the same time. */
    union {
        /* For control flow analysis */
        struct {
            /* The block ends with return or raise */
            bool b_exit;
            /* b_exit is true, or it ends with JUMP_ALWAYS */
            bool b_nofallthrough;
            /* the jump is not a real jump, but a exception handler */
            bool b_exc_jump;
            /* Number of predecessors that a block has. */
            int b_predecessors;
        };
        /* For bytecode generation */
        struct {
            /* The offset of its first instruction (for assemble) */
            int b_offset;
            /* The total size of its instructions (for assemble) */
            int b_size;
        };
    };
    /* number of instructions used */
    int b_iused;
    /* number of instructions allocated (b_instr) */
    int b_ialloc;
    /* The array of instructions */
    struct instr b_instr[];
} basicblock;


/* For debugging purposes only */
#if DEBUGGING

static void
dump_block(basicblock *b, bool finalized) {
    printf(">>> [block %p] fall=%p jump=%p", b, b->b_fall, b->b_jump);
    if (!finalized) {
        printf(" predecessors=%d", b->b_predecessors);
    }
    for (int i = 0; i < b->b_iused; ++i) {
        struct instr *this_i = &b->b_instr[i];
        printf("\n    %d\t %s", this_i->i_lineno, opcode_names[this_i->i_opcode]);
        for (int j = 0; j < 3; j++) {
            Oparg arg = this_i->i_opargs[j];
            if (finalized) {
                printf("  %zd", OPARG_FINAL_VALUE(arg));
            } else {
                printf("  %d.%zd", (int) OPARG_TYPE(arg), OPARG_VALUE(arg));
            }
        }
    }
    printf("\n");
}

static void
dump_blocks(basicblock *entry, bool finalized) {
    for (int i = 0; i < 80; ++i) {
        printf("=");
    }
    printf("\n");
    for (basicblock *b = entry; b; b = b->b_fall) {
        dump_block(b, finalized);
    }
    for (int i = 0; i < 80; ++i) {
        printf("=");
    }
    printf("\n\n");
}

#endif

/* fblockinfo tracks the current frame block.

A frame block is used to handle loops, try/except, and try/finally.
It's called a frame block to distinguish it from a basic block in the
compiler IR.
*/

enum fblocktype {
    FB_LOOP,
    FB_FINALLY,
    FB_EXCEPT
};

struct fblockinfo {
    enum fblocktype fb_type;
    struct fblockinfo *fb_prev;
    const void *fb_datum;
};

enum scope_type {
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
    struct compiler_unit *u_parent; /* The parent compiler_unit or NULL */

    PyObject *u_name;
    PyObject *u_qualname;  /* dot-separated qualified name (lazy) */
    enum scope_type u_scope_type;

    /* The following fields are dicts that map objects to
       the index of them in co_XXX.      The index is used as
       the argument for opcodes that refer to those collections.
    */
    PyObject *u_consts;    /* all constants */
    PyObject *u_names;     /* all names */
    PyObject *u_varnames;  /* local variables */
    PyObject *u_cellvars;  /* cell variables */
    PyObject *u_freevars;  /* free variables */

    PyObject *u_private;        /* for private name mangling, borrowed reference */

    Py_ssize_t u_argcount;        /* number of arguments for block */
    Py_ssize_t u_posonlyargcount;        /* number of positional only arguments for block */
    Py_ssize_t u_kwonlyargcount; /* number of keyword only arguments for block */
    /* Pointer to the most recently allocated block.  By following b_list
       members, you can reach all early allocated blocks. */
    basicblock *u_blocks;
    basicblock *u_curblock; /* pointer to current block */

    struct fblockinfo *u_fblocks;
    int u_block_num;

    Py_ssize_t u_tmp;
    Py_ssize_t u_tmp_max;
    Oparg *u_out_arg;

    int u_annotations;

    int u_firstlineno; /* the first lineno of the block */
    int u_lineno;          /* the lineno for the current stmt */
    int u_col_offset;      /* the offset of the current stmt */
    int u_end_lineno;      /* the end line of the current stmt */
    int u_end_col_offset;  /* the end offset of the current stmt */
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
    bool c_interactive;           /* true if in interactive mode */
    int c_optimize;              /* optimization level */
    PyObject *c_const_cache;     /* Python dict holding all constants,
                                    including names tuple */
    struct compiler_unit *u;     /* compiler state for current block */
};

/* Raises a SyntaxError and returns 0.
   If something goes wrong, a different exception may be raised.
*/
static int
compiler_error(struct compiler *c, const char *format, ...) {
    va_list vargs;
#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
    PyObject *msg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (!msg) {
        return 0;
    }
    PyObject *loc = PyErr_ProgramTextObject(c->c_filename, c->u->u_lineno);
    if (!loc) {
        Py_INCREF(Py_None);
        loc = Py_None;
    }
    PyObject *args = Py_BuildValue("O(OiiOii)", msg, c->c_filename,
                                   c->u->u_lineno, c->u->u_col_offset + 1, loc,
                                   c->u->u_end_lineno, c->u->u_end_col_offset + 1);
    Py_DECREF(msg);
    if (!args) {
        goto exit;
    }
    PyErr_SetObject(PyExc_SyntaxError, args);
exit:
    Py_DECREF(loc);
    Py_XDECREF(args);
    return 0;
}

/* Emits a SyntaxWarning and returns 1 on success.
   If a SyntaxWarning raised as error, replaces it with a SyntaxError
   and returns 0.
*/
static int
compiler_warn(struct compiler *c, const char *format, ...) {
    va_list vargs;
#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
    PyObject *msg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (!msg) {
        return 0;
    }
    if (PyErr_WarnExplicitObject(PyExc_SyntaxWarning, msg, c->c_filename,
                                 c->u->u_lineno, NULL, NULL) < 0) {
        if (PyErr_ExceptionMatches(PyExc_SyntaxWarning)) {
            /* Replace the SyntaxWarning exception with a SyntaxError
               to get a more accurate error report */
            PyErr_Clear();
            assert(PyUnicode_AsUTF8(msg));
            compiler_error(c, PyUnicode_AsUTF8(msg));
        }
        Py_DECREF(msg);
        return 0;
    }
    Py_DECREF(msg);
    return 1;
}

/*
 * Basic block allocation and manipulation
*/
const int DEFAULT_INSTR_PER_BLOCK = 32;
const int MAX_INSTR_PER_BLOCK = 512;

static basicblock *
compiler_new_block_with_size(struct compiler *c, int instr_num) {
    basicblock *b = PyObject_Malloc(sizeof(basicblock) + sizeof(struct instr) * instr_num);
    if (!b) {
        PyErr_NoMemory();
        return NULL;
    }
    b->b_prev_allocated = c->u->u_blocks;
    b->b_fall = b->b_jump = NULL;
    b->b_exit = b->b_nofallthrough = false;
    b->b_predecessors = 0;
    b->b_iused = 0;
    b->b_ialloc = instr_num;
    return c->u->u_blocks = b;
}

static inline basicblock *
compiler_new_block(struct compiler *c) {
    return compiler_new_block_with_size(c, DEFAULT_INSTR_PER_BLOCK);
}

static basicblock *
compiler_copy_block(struct compiler *c, basicblock *target, basicblock *behind) {
    /* Cannot copy a block if it has a fallthrough, since
     * a block can only have one fallthrough predecessor. */
    assert(target->b_nofallthrough);
    size_t mem_size = sizeof(basicblock) + sizeof(struct instr) * target->b_iused;
    basicblock *copied = PyObject_Malloc(mem_size);
    if (!copied) {
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(copied, target, mem_size);
    target->b_predecessors--;
    copied->b_ialloc = target->b_iused;
    copied->b_prev_allocated = c->u->u_blocks;
    copied->b_predecessors = 1;
    copied->b_fall = behind->b_fall;
    behind->b_fall = copied;
    return c->u->u_blocks = copied;;
}

static inline basicblock *
compiler_current_block(struct compiler *c) {
    return c->u->u_curblock;
}

static void
compiler_use_block(struct compiler *c, basicblock *block) {
    assert(block);
    basicblock *old_block = c->u->u_curblock;
    c->u->u_curblock = old_block->b_fall = block;
}

/*
 * Frame block handling functions
*/
static int
compiler_push_fblock(struct compiler *c, struct fblockinfo *fb) {
    fb->fb_prev = c->u->u_fblocks;
    c->u->u_fblocks = fb;
    if (++c->u->u_block_num >= CO_MAXBLOCKS) {
        return compiler_error(c, "too many statically nested blocks");
    }
    return 1;
}

static inline void
compiler_pop_fblock(struct compiler *c) {
    c->u->u_block_num--;
    c->u->u_fblocks = c->u->u_fblocks->fb_prev;
}

/* Returns the pointer to the next instruction in the current block.
   Returns NULL on failure.
*/
static struct instr *
compiler_append_instr(struct compiler *c, bool last_one) {
    basicblock *b = c->u->u_curblock;
    if (b->b_ialloc == b->b_iused) {
        int new_size = 1;
        if (!last_one) {
            new_size = b->b_ialloc * 2;
            new_size = new_size < MAX_INSTR_PER_BLOCK ? new_size : MAX_INSTR_PER_BLOCK;
        }

        b = compiler_new_block_with_size(c, new_size);
        if (!b) {
            return NULL;
        }
        compiler_use_block(c, b);
    }
    return &b->b_instr[b->b_iused++];
}

/*
 * Add an instruction to the current block
*/

static int
addop_line(struct compiler *c, int opcode, Oparg oparg1, Oparg oparg2, Oparg oparg3, int lineno) {
    struct instr *i = compiler_append_instr(c, false);
    CHECK(i);
    i->i_opcode = opcode;
    i->i_opargs[0] = oparg1;
    i->i_opargs[1] = oparg2;
    i->i_opargs[2] = oparg3;
    i->i_lineno = lineno;
    c->u->u_out_arg = &i->i_opargs[2];
    return 1;
}

static inline int
addop(struct compiler *c, int opcode, Oparg oparg1, Oparg oparg2, Oparg oparg3) {
    return addop_line(c, opcode, oparg1, oparg2, oparg3, c->u->u_lineno);
}

static inline int
addop_noline(struct compiler *c, int opcode, Oparg oparg1, Oparg oparg2, Oparg oparg3) {
    return addop_line(c, opcode, oparg1, oparg2, oparg3, -1);
}

static int
addop_jump_line(struct compiler *c, int opcode, Oparg oparg1, Oparg oparg2,
                basicblock *target, basicblock *fallthrough, int lineno) {
    struct instr *i = compiler_append_instr(c, true);
    CHECK(i);
    i->i_opcode = opcode;
    i->i_opargs[0] = oparg1;
    i->i_opargs[1] = oparg2;
    i->i_opargs[2] = ADDR_OPARG;
    i->i_lineno = lineno;
    c->u->u_curblock->b_jump = target;
    c->u->u_out_arg = &i->i_opargs[2];
    c->u->u_curblock->b_nofallthrough = opcode == JUMP_ALWAYS;
    c->u->u_curblock->b_exc_jump = opcode == START_TRY || opcode == ENTER_WITH;
    if (!fallthrough) {
        CHECK(fallthrough = compiler_new_block(c));
    }
    compiler_use_block(c, fallthrough);
    return 1;
}

static inline int
addop_jump(struct compiler *c, int opcode, Oparg oparg1, Oparg oparg2,
           basicblock *target, basicblock *fallthrough) {
    return addop_jump_line(c, opcode, oparg1, oparg2, target, fallthrough, c->u->u_lineno);
}

static inline int
addop_jump_noline(struct compiler *c, int opcode, Oparg oparg1, Oparg oparg2,
                  basicblock *target, basicblock *fallthrough) {
    return addop_jump_line(c, opcode, oparg1, oparg2, target, fallthrough, -1);
}

static int
addop_exit_line(struct compiler *c, int opcode, Oparg oparg1, Oparg oparg2, Oparg oparg3,
                basicblock *fallthrough, int lineno) {
    struct instr *i = compiler_append_instr(c, true);
    CHECK(i);
    i->i_opcode = opcode;
    i->i_opargs[0] = oparg1;
    i->i_opargs[1] = oparg2;
    i->i_opargs[2] = oparg3;
    i->i_lineno = lineno;
    c->u->u_out_arg = NULL;
    c->u->u_curblock->b_nofallthrough = true;
    c->u->u_curblock->b_exit = true;
    if (fallthrough) {
        compiler_use_block(c, fallthrough);
    }
    return 1;
}

static inline int
addop_exit(struct compiler *c, int opcode, Oparg oparg1, Oparg oparg2, Oparg oparg3,
           basicblock *fallthrough) {
    return addop_exit_line(c, opcode, oparg1, oparg2, oparg3, fallthrough, c->u->u_lineno);
}

static inline int
addop_exit_noline(struct compiler *c, int opcode, Oparg oparg1, Oparg oparg2, Oparg oparg3,
                  basicblock *fallthrough) {
    return addop_exit_line(c, opcode, oparg1, oparg2, oparg3, fallthrough, -1);
}

/*
 * Enter and exit scope.
*/

static int
compiler_enter_scope(struct compiler *c, identifier name,
                     int scope_type, void *key, int lineno) {
    struct compiler_unit *u = PyObject_Calloc(1, sizeof(struct compiler_unit));
    if (!u) {
        PyErr_NoMemory();
        return 0;
    }

    u->u_parent = c->u;
    c->u = u;

    u->u_private = u->u_parent ? u->u_parent->u_private : NULL;
    u->u_scope_type = scope_type;
    u->u_firstlineno = lineno;
    CHECK(u->u_ste = PySymtable_Lookup(c->c_st, key));

    CHECK(u->u_consts = PyDict_New());
    CHECK(u->u_names = PyDict_New());

    /* prepare u->u_varnames */
    PyObject *dict;
    CHECK(dict = u->u_varnames = PyDict_New());
    Py_ssize_t size = PyList_Size(u->u_ste->ste_varnames);
    while (size--) {
        PyObject *v = PyLong_FromSsize_t(size);
        CHECK(v);
        PyObject *k = PyList_GET_ITEM(u->u_ste->ste_varnames, size);
        int res = PyDict_SetItem(dict, k, v);
        Py_DECREF(v);
        CHECK(res >= 0);
    }

    /* Sort the keys so that we have a deterministic order on the indexes
       saved in the returned dictionary.  These indexes are used as indexes
       into the free and cell var storage.  Therefore if they aren't
       deterministic, then the generated bytecode is not deterministic.
    */
    PyObject *sorted_keys = PyDict_Keys(u->u_ste->ste_symbols);
    CHECK(sorted_keys);
    if (PyList_Sort(sorted_keys) != 0) {
        Py_DECREF(sorted_keys);
        return 0;
    }
    Py_ssize_t num_keys = PyList_GET_SIZE(sorted_keys);
    size = 0;

    /* prepare u->u_cellvars */
    CHECK(dict = u->u_cellvars = PyDict_New());
    if (u->u_ste->ste_needs_class_closure) {
        assert(u->u_scope_type == COMPILER_SCOPE_CLASS);
        PyObject *item = PyLong_FromSsize_t(size++);
        if (!item || PyDict_SetItemString(dict, "__class__", item) < 0) {
            Py_XDECREF(item);
            return 0;
        }
        Py_DECREF(item);
    }
    for (Py_ssize_t i = 0; i < num_keys; i++) {
        PyObject *k = PyList_GET_ITEM(sorted_keys, i);
        PyObject *v_o = PyDict_GetItemWithError(u->u_ste->ste_symbols, k);
        long v = PyLong_AS_LONG(v_o);
        if ((v >> SCOPE_OFFSET & SCOPE_MASK) == CELL) {
            PyObject *item = PyLong_FromSsize_t(size++);
            if (!item || PyDict_SetItem(dict, k, item) < 0) {
                Py_DECREF(sorted_keys);
                Py_XDECREF(item);
                return 0;
            }
            Py_DECREF(item);
        }
    }

    /* prepare u->u_freevars */
    CHECK(dict = u->u_freevars = PyDict_New());
    for (Py_ssize_t i = 0; i < num_keys; i++) {
        PyObject *k = PyList_GET_ITEM(sorted_keys, i);
        PyObject *v_o = PyDict_GetItemWithError(u->u_ste->ste_symbols, k);
        long v = PyLong_AS_LONG(v_o);
        if ((v >> SCOPE_OFFSET & SCOPE_MASK) == FREE || v & DEF_FREE_CLASS) {
            PyObject *item = PyLong_FromSsize_t(size++);
            if (!item || PyDict_SetItem(dict, k, item) < 0) {
                Py_DECREF(sorted_keys);
                Py_XDECREF(item);
                return 0;
            }
            Py_DECREF(item);
        }
    }
    Py_DECREF(sorted_keys);

    /* prepare the name and qualname */
    Py_INCREF(name);
    u->u_name = name;
    if (scope_type != COMPILER_SCOPE_MODULE) {
        struct compiler_unit *parent = u->u_parent;
        assert(parent);

        PyObject *qualname = NULL;
        if (parent->u_parent) {
            bool force_global = false;
            if (u->u_scope_type == COMPILER_SCOPE_FUNCTION
                || u->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION
                || u->u_scope_type == COMPILER_SCOPE_CLASS) {
                assert(u->u_name);
                PyObject *mangled = _Py_Mangle(parent->u_private, u->u_name);
                CHECK(mangled);
                int scope = _PyST_GetScope(parent->u_ste, mangled);
                Py_DECREF(mangled);
                assert(scope != GLOBAL_IMPLICIT);
                force_global = scope == GLOBAL_EXPLICIT;
            }

            if (!force_global) {
                if (parent->u_scope_type == COMPILER_SCOPE_FUNCTION
                    || parent->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION
                    || parent->u_scope_type == COMPILER_SCOPE_LAMBDA) {
                    static PyObject *dot_locals;
                    CHECK(intern_static_identifier(&dot_locals, ".<locals>."));
                    qualname = dot_locals;
                } else {
                    static PyObject *dot;
                    CHECK(intern_static_identifier(&dot, "."));
                    qualname = dot;
                }
                CHECK(qualname = PyUnicode_Concat(parent->u_qualname, qualname));
            }
        }

        if (qualname) {
            PyUnicode_Append(&qualname, u->u_name);
            CHECK(u->u_qualname = qualname);
        } else {
            Py_INCREF(u->u_name);
            u->u_qualname = u->u_name;
        }
    }

    if (scope_type == COMPILER_SCOPE_MODULE || scope_type == COMPILER_SCOPE_CLASS) {
        u->u_annotations = 1;
    }

    CHECK(u->u_curblock = compiler_new_block(c));
    /* Pre-insert instructions for GEN_START and SETUP_ANNOTATIONS,
     * they will be revised after compilation.
     * These instructions are written consecutively. */
    assert(u->u_curblock->b_iused == 0 && u->u_curblock->b_ialloc >= 2);
    CHECK(addop_noline(c, GEN_START, RELOC_OPARG, UNSED_OPARG, UNSED_OPARG));
    CHECK(addop_noline(c, SETUP_ANNOTATIONS, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));
    return 1;
}

static void
compiler_exit_scope(struct compiler *c) {
    // Don't call PySequence_DelItem() with an exception raised
    PyObject *exc_type, *exc_val, *exc_tb;
    PyErr_Fetch(&exc_type, &exc_val, &exc_tb);

    /* Restore c->u to the parent unit. */
    struct compiler_unit *u = c->u;
    c->u = u->u_parent;

    for (basicblock *b = u->u_blocks; b;) {
        basicblock *next = b->b_prev_allocated;
        PyObject_Free(b);
        b = next;
    }
    Py_CLEAR(u->u_ste);
    Py_CLEAR(u->u_name);
    Py_CLEAR(u->u_qualname);
    Py_CLEAR(u->u_consts);
    Py_CLEAR(u->u_names);
    Py_CLEAR(u->u_varnames);
    Py_CLEAR(u->u_cellvars);
    Py_CLEAR(u->u_freevars);
    PyObject_Free(u);

    PyErr_Restore(exc_type, exc_val, exc_tb);
}

static int
compiler_safe_exit_scope(struct compiler *c, struct compiler_unit *old_u, int res) {
    struct compiler_unit *new_u = c->u;
    assert(old_u == new_u || (new_u->u_parent == old_u && !res));
    if (new_u != old_u) {
        compiler_exit_scope(c);
    }
    return res;
}

static inline bool
is_top_level_await(struct compiler *c) {
    return c->c_flags->cf_flags & PyCF_ALLOW_TOP_LEVEL_AWAIT
           && c->u->u_ste->ste_type == ModuleBlock;
}

typedef struct {
    /* the temporary register of the subject. */
    Oparg subject;
    /* the fallback basicblock for a failed match. */
    basicblock *fallback;
    /* A tuple of strings of the names to store. */
    PyObject *stores;
    /* temporary registers for the names to store */
    Oparg tmps;
    /* name captures is allowed only when it is true */
    bool allow_capture;
} pattern_context;

static int compiler_access_var_name(struct compiler *, identifier, expr_context_ty);

static int compiler_visit_stmts(struct compiler *, asdl_stmt_seq *);

static int compiler_visit_expr(struct compiler *, expr_ty);

static int compiler_visit_expr_as_tmp(struct compiler *, expr_ty);

static int compiler_visit_ann_expr(struct compiler *, expr_ty);

static int compiler_call_helper(struct compiler *, bool, int, asdl_expr_seq *, asdl_keyword_seq *);

static int compiler_try_except(struct compiler *, stmt_ty);

static int compiler_pattern(struct compiler *, pattern_ty, pattern_context *);

static int compiler_match(struct compiler *, stmt_ty);

static int compiler_pattern_subpattern(struct compiler *, pattern_ty, Oparg, pattern_context *);

static int sequence_building_helper(struct compiler *, asdl_expr_seq *, Opcode, Opcode, Py_ssize_t);

static int assemble(struct compiler *, PyCodeObject **);

/* Set the line number and column offset for the following instructions.

   The line number is reset in the following cases:
   - when entering a new scope
   - on each statement
   - on each expression and sub-expression
   - before the "except" and "finally" clauses
*/
#define SET_LOC(c, x)                           \
    (c)->u->u_lineno = (x)->lineno;             \
    (c)->u->u_col_offset = (x)->col_offset;     \
    (c)->u->u_end_lineno = (x)->end_lineno;     \
    (c)->u->u_end_col_offset = (x)->end_col_offset;

/*
 * Register allocation.
*/

static inline Oparg *
cur_out(struct compiler *c) {
    return c->u->u_out_arg;
}

static inline void
set_cur_out(struct compiler *c, Oparg *arg) {
    c->u->u_out_arg = arg;
}

static inline void
adjust_cur_out(struct compiler *c, int which) {
    set_cur_out(c, c->u->u_out_arg - 3 + which);
}

static inline Py_ssize_t
cur_tmp(struct compiler *c) {
    return c->u->u_tmp;
}

static inline void
set_cur_tmp(struct compiler *c, Py_ssize_t where) {
    c->u->u_tmp = where;
}

static inline void
free_cur_tmp(struct compiler *c, Py_ssize_t how_many) {
    c->u->u_tmp -= how_many;
}

static inline Oparg
cur_tmp_arg(struct compiler *c) {
    return tmp_oparg(c->u->u_tmp);
}

static Oparg
new_tmp_arg_n(struct compiler *c, Py_ssize_t n) {
    struct compiler_unit *u = c->u;
    Oparg arg = tmp_oparg(u->u_tmp);
    u->u_tmp += n;
    if (u->u_tmp > u->u_tmp_max) {
        u->u_tmp_max = u->u_tmp;
    }
    return arg;
}

static inline Oparg
new_tmp_arg(struct compiler *c) {
    return new_tmp_arg_n(c, 1);
}

static inline Oparg
alloc_arg_auto(struct compiler *c) {
    Oparg *arg = cur_out(c);
    if (OPARG_EQ(*arg, RELOC_OPARG)) {
        *arg = new_tmp_arg(c);
    }
    return *arg;
}

static inline Oparg
alloc_arg_reuse(struct compiler *c, Oparg *reuse) {
    Oparg *arg = cur_out(c);
    if (OPARG_EQ(*arg, RELOC_OPARG)) {
        if (OPARG_EQ(*reuse, RELOC_OPARG)) {
            *reuse = new_tmp_arg(c);
        }
        return *arg = *reuse;
    }
    return *arg;
}

static inline int
alloc_arg_bind(struct compiler *c, Oparg dest) {
    assert(!OPARG_EQ(dest, RELOC_OPARG));
    Oparg *src = cur_out(c);
    if (OPARG_EQ(*src, RELOC_OPARG)) {
        *src = dest;
    } else {
        CHECK(addop(c, MOVE_FAST, *src, UNSED_OPARG, dest));
    }
    return 1;
}

static inline int
alloc_arg_tmp(struct compiler *c) {
    return alloc_arg_bind(c, new_tmp_arg(c));
}

// Merge const *o* recursively and return constant key object.
static PyObject *
merge_consts_recursive(struct compiler *c, PyObject *o) {
    // None and Ellipsis are singleton, and key is the singleton.
    // No need to merge object and key.
    if (o == Py_None || o == Py_Ellipsis) {
        Py_INCREF(o);
        return o;
    }

    PyObject *key = _PyCode_ConstantKey(o);
    if (!key) {
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
            if (!u) {
                Py_DECREF(key);
                return NULL;
            }

            // See _PyCode_ConstantKey()
            PyObject *v;  // borrowed
            if (PyTuple_CheckExact(u)) {
                v = PyTuple_GET_ITEM(u, 1);
            } else {
                v = u;
            }
            if (v != item) {
                Py_INCREF(v);
                PyTuple_SET_ITEM(o, i, v);
                Py_DECREF(item);
            }

            Py_DECREF(u);
        }
    } else if (PyFrozenSet_CheckExact(o)) {
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
        if (!tuple) {
            Py_DECREF(key);
            return NULL;
        }
        Py_ssize_t i = 0, pos = 0;
        PyObject *item;
        Py_hash_t hash;
        while (_PySet_NextEntry(o, &pos, &item, &hash)) {
            PyObject *k = merge_consts_recursive(c, item);
            if (!k) {
                Py_DECREF(tuple);
                Py_DECREF(key);
                return NULL;
            }
            PyObject *u;
            if (PyTuple_CheckExact(k)) {
                u = PyTuple_GET_ITEM(k, 1);
                Py_INCREF(u);
                Py_DECREF(k);
            } else {
                u = k;
            }
            PyTuple_SET_ITEM(tuple, i, u);  // Steals reference of u.
            i++;
        }

        // Instead of rewriting o, we create new frozenset and embed in the
        // key tuple.  Caller should get merged frozenset from the key tuple.
        PyObject *new = PyFrozenSet_New(tuple);
        Py_DECREF(tuple);
        if (!new) {
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
indexify_obj(PyObject *dict, PyObject *obj) {
    Py_ssize_t index = -1;
    PyObject *index_o = PyDict_GetItemWithError(dict, obj);
    if (index_o) {
        index = PyLong_AsSsize_t(index_o);
    } else if (!PyErr_Occurred()) {
        Py_ssize_t size = PyDict_GET_SIZE(dict);
        index_o = PyLong_FromSsize_t(size);
        if (index_o) {
            if (PyDict_SetItem(dict, obj, index_o) >= 0) {
                index = size;
            }
            Py_DECREF(index_o);
        }
    }
    return index;
}

static int
compiler_load_const(struct compiler *c, PyObject *o) {
    PyObject *key = merge_consts_recursive(c, o);
    CHECK(key);
    Py_ssize_t index = indexify_obj(c->u->u_consts, key);
    Py_DECREF(key);
    CHECK(index >= 0);
    Oparg oparg = make_oparg(OpargConst, index);
    return addop(c, MOVE_FAST, oparg, UNSED_OPARG, oparg);
}

static inline int
compiler_load_const_new(struct compiler *c, PyObject *o) {
    int r = compiler_load_const(c, o);
    Py_DECREF(o);
    return r;
}

static inline int
compiler_load_name(struct compiler *c, PyObject *o, Oparg *arg) {
    PyObject *mangled = _Py_Mangle(c->u->u_private, o);
    if (mangled) {
        Py_ssize_t index = indexify_obj(c->u->u_names, mangled);
        Py_DECREF(mangled);
        if (index >= 0) {
            *arg = imm_oparg(index);
            return 1;
        }
    }
    return 0;
}

static int
verify_var_name(struct compiler *c, identifier name, expr_context_ty ctx) {
    if (ctx == Store && _PyUnicode_EqualToASCIIString(name, "__debug__")) {
        return compiler_error(c, "cannot assign to __debug__");
    }
    if (ctx == Del && _PyUnicode_EqualToASCIIString(name, "__debug__")) {
        return compiler_error(c, "cannot delete __debug__");
    }
    return 1;
}

static int
compiler_access_var_name(struct compiler *c, identifier name, expr_context_ty ctx) {
    CHECK(verify_var_name(c, name, ctx));

    PyObject *mangled = _Py_Mangle(c->u->u_private, name);
    CHECK(mangled);

    PyObject *dict = c->u->u_names;
    enum {
        Fast, Deref, ClassDeref, Global, Name, _NS_NUMS
    } namespace;
    namespace = Name;
    int scope = _PyST_GetScope(c->u->u_ste, mangled);
    switch (scope) {
        case LOCAL:
            if (c->u->u_ste->ste_type == FunctionBlock) {
                dict = c->u->u_varnames;
                namespace = Fast;
            }
            break;
        case FREE:
            dict = c->u->u_freevars;
            namespace = c->u->u_ste->ste_type == ClassBlock ? ClassDeref : Deref;
            break;
        case CELL:
            dict = c->u->u_cellvars;
            namespace = Deref;
            break;
        case GLOBAL_IMPLICIT:
            if (c->u->u_ste->ste_type == FunctionBlock) {
                namespace = Global;
            }
            break;
        case GLOBAL_EXPLICIT:
            namespace = Global;
            break;
        default:
            /* scope can be 0 */
            break;
    }
    /* XXX Leave assert here, but handle __doc__ and the like better */
    assert(scope || PyUnicode_READ_CHAR(name, 0) == '_');

    Py_ssize_t index = indexify_obj(dict, mangled);
    Py_DECREF(mangled);
    CHECK(index >= 0);

    Oparg oparg1 = make_oparg(namespace == Fast ? OpargFast : OpargImm, index);
    Oparg oparg2 = ctx == Del ? imm_oparg(0) : UNSED_OPARG;
    Oparg oparg3 = ctx == Del ? UNSED_OPARG : (namespace == Fast ? oparg1 : RELOC_OPARG);

    static const Opcode opcodes[][_NS_NUMS] = {
            [Load][Fast] = MOVE_FAST,
            [Store][Fast] = MOVE_FAST,
            [Del][Fast] = DELETE_FAST,
            [Load][Deref] = LOAD_DEREF,
            [Store][Deref] = STORE_DEREF,
            [Del][Deref] = DELETE_DEREF,
            [Load][ClassDeref] = LOAD_CLASSDEREF,
            [Store][ClassDeref] = STORE_DEREF,
            [Del][ClassDeref] = DELETE_DEREF,
            [Load][Global] = LOAD_GLOBAL,
            [Store][Global] = STORE_GLOBAL,
            [Del][Global] = DELETE_GLOBAL,
            [Load][Name] = LOAD_NAME,
            [Store][Name] = STORE_NAME,
            [Del][Name] = DELETE_NAME
    };
    return addop(c, opcodes[ctx][namespace], oparg1, oparg2, oparg3);
}

static int
compiler_bind_to_var_name(struct compiler *c, PyObject *name) {
    Oparg *value = cur_out(c);
    CHECK(compiler_access_var_name(c, name, Store));
    Oparg *dest = cur_out(c);
    if (OPARG_EQ(*value, RELOC_OPARG)) {
        if (OPARG_EQ(*dest, RELOC_OPARG)) {
            *value = *dest = new_tmp_arg(c);
            free_cur_tmp(c, 1);
        } else {
            *value = *dest;
        }
    } else {
        if (OPARG_EQ(*dest, RELOC_OPARG)) {
            *dest = *value;
        } else {
            return addop(c, MOVE_FAST, *value, UNSED_OPARG, *dest);
        }
    }
    return 1;
}

static int
compiler_bind_to_target(struct compiler *c, expr_ty target) {
    if (target->kind == Name_kind) {
        int old_lineno = c->u->u_lineno;
        c->u->u_lineno = target->lineno;
        assert(target->v.Name.ctx == Store);
        compiler_bind_to_var_name(c, target->v.Name.id);
        c->u->u_lineno = old_lineno;
    } else {
        Py_ssize_t old_tmp = cur_tmp(c);
        Oparg value_arg = alloc_arg_auto(c);
        CHECK(compiler_visit_expr(c, target));
        assert(OPARG_EQ(*cur_out(c), RELOC_OPARG));
        *cur_out(c) = value_arg;
        set_cur_tmp(c, old_tmp);
    }
    return 1;
}

static Opcode
get_unary_opcode(unaryop_ty op) {
    static const Opcode opcodes[] = {
            [Invert]=UNARY_INVERT,
            [Not]=UNARY_NOT,
            [UAdd]=UNARY_POSITIVE,
            [USub]=UNARY_NEGATIVE
    };
    return opcodes[op];
}

static Opcode
get_binary_opcode(operator_ty op, bool inplace) {
    static const Opcode opcodes[][2] = {
            [Add]={BINARY_ADD, INPLACE_ADD},
            [Sub]={BINARY_SUBTRACT, INPLACE_SUBTRACT},
            [Mult]={BINARY_MULTIPLY, INPLACE_MULTIPLY},
            [MatMult]={BINARY_MATRIX_MULTIPLY, INPLACE_MATRIX_MULTIPLY},
            [Div]={BINARY_TRUE_DIVIDE, INPLACE_TRUE_DIVIDE},
            [Mod]={BINARY_MODULO, INPLACE_MODULO},
            [Pow]={BINARY_POWER, INPLACE_POWER},
            [LShift]={BINARY_LSHIFT, INPLACE_LSHIFT},
            [RShift]={BINARY_RSHIFT, INPLACE_RSHIFT},
            [BitOr]={BINARY_OR, INPLACE_OR},
            [BitXor]={BINARY_XOR, INPLACE_XOR},
            [BitAnd]={BINARY_AND, INPLACE_AND},
            [FloorDiv]={BINARY_FLOOR_DIVIDE, INPLACE_FLOOR_DIVIDE}
    };
    return opcodes[op][inplace];
}

static Opcode
get_compare_opcode(cmpop_ty op) {
    static const Opcode opcodes[] = {
            [Lt]=COMPARE_LT, [LtE]=COMPARE_LE,
            [Eq]=COMPARE_EQ, [NotEq]=COMPARE_NE,
            [Gt]=COMPARE_GT, [GtE]=COMPARE_GE,
            [Is]=IS_OP, [IsNot]=IS_OP,
            [In]=CONTAINS_OP, [NotIn]=CONTAINS_OP
    };
    return opcodes[op];
}

static int
compiler_get_closures(struct compiler *c, PyCodeObject *co, bool allow_empty) {
    Py_ssize_t num = PyCode_GetNumFree(co);
    if (num || allow_empty) {
        PyObject *tuple = PyTuple_New(num);
        CHECK(tuple);
        for (Py_ssize_t i = 0; i < num; i++) {
            PyObject *name = PyTuple_GET_ITEM(co->co_freevars, i);
            int reftype;
            if (c->u->u_scope_type == COMPILER_SCOPE_CLASS &&
                _PyUnicode_EqualToASCIIString(name, "__class__")) {
                reftype = CELL;
            } else if (!(reftype = _PyST_GetScope(c->u->u_ste, name))) {
                PyErr_Format(PyExc_SystemError,
                             "_PyST_GetScope(name=%R) failed: "
                             "unknown scope in unit %S (%R); "
                             "symbols: %R; locals: %R; globals: %R",
                             name,
                             c->u->u_name, c->u->u_ste->ste_id,
                             c->u->u_ste->ste_symbols, c->u->u_varnames, c->u->u_names);
                Py_DECREF(tuple);
                return 0;
            }
            PyObject *dict = reftype == CELL ? c->u->u_cellvars : c->u->u_freevars;
            PyObject *index = PyDict_GetItemWithError(dict, name);
            if (!index) {
                if (!PyErr_Occurred()) {
                    PyErr_Format(PyExc_SystemError,
                                 "PyDict_GetItemWithError(name=%R) with reftype=%d failed in %S; "
                                 "freevars of code %S: %R",
                                 name, reftype,
                                 c->u->u_name, co->co_name, co->co_freevars);
                }
                Py_DECREF(tuple);
                return 0;
            }
            Py_INCREF(index);
            PyTuple_SET_ITEM(tuple, i, index);
        }
        CHECK(compiler_load_const_new(c, tuple));
        if (num) {
            Oparg flag = imm_oparg(0);
            return addop(c, GET_CLOSURES, flag, *cur_out(c), RELOC_OPARG);
        }
        return 1;
    }
    return -1;
}

static int
compiler_apply_decorators(struct compiler *c, Py_ssize_t location, Py_ssize_t num) {
    if (num) {
        CHECK(alloc_arg_bind(c, tmp_oparg(location + num)));
        Oparg call_flag = imm_oparg(1);
        while (num--) {
            Oparg deco_func = tmp_oparg(location + num);
            Oparg out_func = num ? deco_func : RELOC_OPARG;
            CHECK(addop(c, CALL_FUNCTION, call_flag, deco_func, out_func));
        }
    }
    return 1;
}

static int
compiler_add_docstring(struct compiler *c, asdl_stmt_seq *stmts, bool to_name) {
    expr_ty doc_expr = NULL;
    if (c->c_optimize < 2 && asdl_seq_LEN(stmts)) {
        stmt_ty stmt = asdl_seq_GET(stmts, 0);
        if (stmt->kind == Expr_kind) {
            expr_ty e = stmt->v.Expr.value;
            if (e->kind == Constant_kind && PyUnicode_CheckExact(e->v.Constant.value)) {
                doc_expr = e;
            }
        }
    }
    if (!to_name) {
        PyObject *docstring = doc_expr ? doc_expr->v.Constant.value : Py_None;
        PyObject *key = merge_consts_recursive(c, docstring);
        CHECK(key);
        Py_ssize_t index = indexify_obj(c->u->u_consts, key);
        Py_DECREF(key);
        CHECK(index >= 0);
        assert(index == 0);
    } else if (doc_expr) {
        SET_LOC(c, doc_expr);
        CHECK(compiler_visit_expr(c, doc_expr));
        static PyObject *__doc__;
        CHECK(intern_static_identifier(&__doc__, "__doc__"));
        CHECK(compiler_bind_to_var_name(c, __doc__));
    }
    return 1;
}

/* Make a dict of keyword-only default values.
   Return -1 if no default values, 0 on error, 1 on success. */
static int
default_kwargs_building_helper(struct compiler *c, asdl_arg_seq *args, asdl_expr_seq *values) {
    Py_ssize_t stack = cur_tmp(c);
    Oparg stack_arg = tmp_oparg(stack);
    Oparg values_arg = stack_arg;

    Py_ssize_t nargs = asdl_seq_LEN(args);
    bool with_existed = false;
    if (!nargs) {
        return -1;
    }

    for (Py_ssize_t i = 0;;) {
        PyObject *keys = NULL;
        Py_ssize_t nvalues = 0;
        Py_ssize_t j = i;
        for (; j < nargs; j++) {
            if (asdl_seq_GET(values, j)) {
                PyObject *arg = _Py_Mangle(c->u->u_private, asdl_seq_GET(args, j)->arg);
                if (!arg) {
                    Py_XDECREF(keys);
                    return 0;
                }
                if (keys) {
                    int res = PyList_Append(keys, arg);
                    Py_DECREF(arg);
                    if (res == -1) {
                        Py_DECREF(keys);
                        return 0;
                    }
                } else {
                    if ((keys = PyList_New(1)) == NULL) {
                        Py_DECREF(arg);
                        return 0;
                    }
                    PyList_SET_ITEM(keys, 0, arg);
                }
                if (++nvalues == TMP_USE_GUIDELINE) {
                    break;
                }
            }
        }

        if (!keys) {
            return with_existed ? 1 : -1;
        }

        PyObject *keys_tuple = PyList_AsTuple(keys);
        Py_DECREF(keys);
        CHECK(keys_tuple);
        CHECK(compiler_load_const_new(c, keys_tuple));
        Oparg keys_arg = *cur_out(c);

        if (with_existed) {
            CHECK(alloc_arg_tmp(c));
        }
        values_arg = cur_tmp_arg(c);
        for (; i < j; i++) {
            expr_ty value = asdl_seq_GET(values, i);
            if (value) {
                CHECK(compiler_visit_expr_as_tmp(c, value));
            }
        }

        if (with_existed) {
            CHECK(addop(c, DICT_WITH_CONST_KEYS, keys_arg, values_arg, values_arg));
            CHECK(addop(c, DICT_UPDATE, values_arg, UNSED_OPARG, stack_arg));
            CHECK(addop(c, MOVE_FAST, stack_arg, UNSED_OPARG, RELOC_OPARG));
        } else {
            CHECK(addop(c, DICT_WITH_CONST_KEYS, keys_arg, values_arg, RELOC_OPARG));
        }
        set_cur_tmp(c, stack);
        with_existed = true;
    }
}

static int
compiler_visit_argannotation(struct compiler *c, identifier id,
                             expr_ty annotation) {
    if (!annotation) {
        return 1;
    }
    PyObject *mangled = _Py_Mangle(c->u->u_private, id);
    CHECK(mangled);
    CHECK(compiler_load_const_new(c, mangled));
    CHECK(alloc_arg_tmp(c));
    CHECK(compiler_visit_ann_expr(c, annotation));
    CHECK(alloc_arg_tmp(c));
    return 1;
}

static inline int
compiler_visit_argannotations(struct compiler *c, asdl_arg_seq *args) {
    Py_ssize_t n = asdl_seq_LEN(args);
    for (Py_ssize_t i = 0; i < n; i++) {
        arg_ty arg = asdl_seq_GET(args, i);
        CHECK(compiler_visit_argannotation(c, arg->arg, arg->annotation));
    }
    return 1;
}

/* Build a tuple for arg annotation names and values.
   The expressions are evaluated out-of-order wrt the source code.
   Return 0 on error, -1 if no annotations, 1 if on success .
   Unfortunately, TMP_USE_GUIDELINE is not supported here.
   */
static int
compiler_visit_annotations(struct compiler *c, arguments_ty args, expr_ty returns) {
    Py_ssize_t stack = cur_tmp(c);
    CHECK(compiler_visit_argannotations(c, args->args));
    CHECK(compiler_visit_argannotations(c, args->posonlyargs));
    CHECK(!args->vararg || compiler_visit_argannotation(c, args->vararg->arg, args->vararg->annotation));
    CHECK(compiler_visit_argannotations(c, args->kwonlyargs));
    CHECK(!args->kwarg || compiler_visit_argannotation(c, args->kwarg->arg, args->kwarg->annotation));
    if (returns) {
        static identifier return_str;
        CHECK(intern_static_identifier(&return_str, "return"));
        CHECK(compiler_visit_argannotation(c, return_str, returns));
    }

    Py_ssize_t len = cur_tmp(c) - stack;
    if (len) {
        Oparg flag_arg = imm_oparg(len);
        Oparg stack_arg = tmp_oparg(stack);
        CHECK(addop(c, TUPLE_BUILD, flag_arg, stack_arg, RELOC_OPARG));
        set_cur_tmp(c, stack);
        return 1;
    }
    return -1;
}

static int
compiler_scope_to_function(struct compiler *c, arguments_ty args, expr_ty returns) {
    c->u->u_argcount = asdl_seq_LEN(args->args);
    c->u->u_posonlyargcount = asdl_seq_LEN(args->posonlyargs);
    c->u->u_kwonlyargcount = asdl_seq_LEN(args->kwonlyargs);
    PyCodeObject *co;
    CHECK(assemble(c, &co));
    PyObject *name = c->u->u_qualname;
    Py_INCREF(name);
    compiler_exit_scope(c);

    Oparg input_arg = cur_tmp_arg(c);
    if (!compiler_load_const_new(c, name) || !alloc_arg_tmp(c)) {
        Py_DECREF(co);
        return 0;
    }
    if (!compiler_load_const(c, (PyObject *) co) || !alloc_arg_tmp(c)) {
        Py_DECREF(co);
        return 0;
    }

    Py_ssize_t func_flags = 0;
    int res = compiler_get_closures(c, co, false);
    Py_DECREF(co);
    CHECK(res);
    if (res > 0) {
        func_flags |= 1;
        CHECK(alloc_arg_tmp(c));
    }
    if (asdl_seq_LEN(args->defaults)) {
        sequence_building_helper(c, args->defaults, TUPLE_BUILD, LIST_UPDATE, 0);
        func_flags |= 0b10;
        CHECK(alloc_arg_tmp(c));
    }
    CHECK(res = default_kwargs_building_helper(c, args->kwonlyargs, args->kw_defaults));
    if (res > 0) {
        func_flags |= 0b100;
        CHECK(alloc_arg_tmp(c));
    }
    CHECK(res = compiler_visit_annotations(c, args, returns));
    if (res > 0) {
        func_flags |= 0b1000;
        CHECK(alloc_arg_tmp(c));
    }

    return addop(c, MAKE_FUNCTION, imm_oparg(func_flags), input_arg, RELOC_OPARG);
}

static int
compiler_check_debug_args_seq(struct compiler *c, asdl_arg_seq *args) {
    if (args) {
        for (Py_ssize_t i = 0, n = asdl_seq_LEN(args); i < n; i++) {
            arg_ty arg = asdl_seq_GET(args, i);
            CHECK(!arg || verify_var_name(c, arg->arg, Store));
        }
    }
    return 1;
}

static int
compiler_check_debug_args(struct compiler *c, arguments_ty args) {
    CHECK(compiler_check_debug_args_seq(c, args->posonlyargs));
    CHECK(compiler_check_debug_args_seq(c, args->args));
    CHECK(!args->vararg || verify_var_name(c, args->vararg->arg, Store));
    CHECK(compiler_check_debug_args_seq(c, args->kwonlyargs));
    CHECK(!args->kwarg || verify_var_name(c, args->kwarg->arg, Store));
    return 1;
}

/* Return 0 if the expression is a constant value except named singletons.
   Return 1 otherwise. */
static int
check_is_arg(expr_ty e) {
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
check_compare(struct compiler *c, expr_ty e) {
    Py_ssize_t i, n;
    int left = check_is_arg(e->v.Compare.left);
    n = asdl_seq_LEN(e->v.Compare.ops);
    for (i = 0; i < n; i++) {
        cmpop_ty op = asdl_seq_GET(e->v.Compare.ops, i);
        int right = check_is_arg(asdl_seq_GET(e->v.Compare.comparators, i));
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

static int
compiler_logical_expr(struct compiler *c, expr_ty e, basicblock *b_jump, bool cond) {
    static const Opcode jump_opcodes[] = {JUMP_IF_FALSE, JUMP_IF_TRUE};

    switch (e->kind) {
        case UnaryOp_kind: {
            if (e->v.UnaryOp.op == Not) {
                return compiler_logical_expr(c, e->v.UnaryOp.operand, b_jump, !cond);
            }
            CHECK(compiler_visit_expr(c, e->v.UnaryOp.operand));
            Oparg output = new_tmp_arg(c);
            Oparg operand = alloc_arg_reuse(c, &output);
            CHECK(addop(c, get_unary_opcode(e->v.UnaryOp.op), operand, UNSED_OPARG, output));
            CHECK(addop_jump(c, jump_opcodes[cond], output, NEG_ADDR_OPARG, b_jump, NULL));
            free_cur_tmp(c, 1);
            return 1;
        }

        case BoolOp_kind: {
            asdl_expr_seq *s = e->v.BoolOp.values;
            Py_ssize_t n = asdl_seq_LEN(s) - 1;
            bool is_or = e->v.BoolOp.op == Or;
            basicblock *b_end = compiler_new_block(c);
            CHECK(b_end);
            basicblock *b_shortcut = is_or == cond ? b_jump : b_end;
            for (Py_ssize_t i = 0; i < n; i++) {
                CHECK(compiler_logical_expr(c, asdl_seq_GET(s, i), b_shortcut, is_or));
            }
            CHECK(compiler_logical_expr(c, asdl_seq_GET(s, n), b_jump, cond));
            compiler_use_block(c, b_end);
            return 1;
        }

        case IfExp_kind: {
            basicblock *b_end = compiler_new_block(c);
            basicblock *b_else = compiler_new_block(c);
            CHECK(b_end && b_else);
            CHECK(compiler_logical_expr(c, e->v.IfExp.test, b_else, false));
            CHECK(compiler_logical_expr(c, e->v.IfExp.body, b_jump, cond));
            CHECK(addop_jump_noline(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, b_end, b_else));
            CHECK(compiler_logical_expr(c, e->v.IfExp.orelse, b_jump, cond));
            compiler_use_block(c, b_end);
            return 1;
        }

        case Compare_kind: {
            CHECK(check_compare(c, e));
            Py_ssize_t n = asdl_seq_LEN(e->v.Compare.ops) - 1;
            basicblock *b_end = compiler_new_block(c);
            CHECK(b_end);
            basicblock *b_shortcut = cond ? b_end : b_jump;

            Py_ssize_t old_tmp = cur_tmp(c);
            Oparg reuse_args[2] = {RELOC_OPARG, RELOC_OPARG};
            int flip = 0;
            CHECK(compiler_visit_expr(c, e->v.Compare.left));
            Oparg left = alloc_arg_reuse(c, &reuse_args[flip]);
            for (Py_ssize_t i = 0;; i++) {
                CHECK(compiler_visit_expr(c, asdl_seq_GET(e->v.Compare.comparators, i)));
                Oparg right = alloc_arg_reuse(c, &reuse_args[!flip]);
                cmpop_ty cmp_op = asdl_seq_GET(e->v.Compare.ops, i);
                CHECK(addop(c, get_compare_opcode(cmp_op), left, right, RELOC_OPARG));
                Oparg output = alloc_arg_reuse(c, &reuse_args[flip]);
                bool inverted = cmp_op == IsNot || cmp_op == NotIn;
                if (i == n) {
                    CHECK(addop_jump(c, jump_opcodes[cond ^ inverted], output, NEG_ADDR_OPARG, b_jump, b_end));
                    set_cur_tmp(c, old_tmp);
                    return 1;
                }
                CHECK(addop_jump(c, jump_opcodes[inverted], output, NEG_ADDR_OPARG, b_shortcut, NULL));
                left = right;
                flip = !flip;
            }
        }

        default:
            assert(b_jump);
            /* For literal constant, simplify to unconditional jump */
            if (e->kind == Constant_kind) {
                int is_true = PyObject_IsTrue(e->v.Constant.value);
                CHECK(is_true >= 0);
                if (is_true == cond) {
                    return addop_jump(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, b_jump, NULL);
                }
                /* NOP to keep the line information */
                return addop(c, NOP, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG);
            }
            CHECK(compiler_visit_expr(c, e));
            Py_ssize_t old_tmp = cur_tmp(c);
            CHECK(addop_jump(c, jump_opcodes[cond], alloc_arg_auto(c), NEG_ADDR_OPARG, b_jump, NULL));
            set_cur_tmp(c, old_tmp);
            return 1;
    }
}

static int
sequence_building_helper(struct compiler *c, asdl_expr_seq *seq,
                         Opcode build_op, Opcode update_op, Py_ssize_t npushed) {
    Py_ssize_t len = asdl_seq_LEN(seq);
    Py_ssize_t nconsts = 0;
    Py_ssize_t stack = cur_tmp(c) - npushed;
    Oparg stack_oparg = tmp_oparg(stack);

    while (nconsts < len && asdl_seq_GET(seq, nconsts)->kind == Constant_kind) {
        nconsts++;
    }
    if (nconsts == len && (build_op == TUPLE_BUILD ? !npushed : len > 4)) {
        PyObject *folded = PyTuple_New(nconsts);
        CHECK(folded);
        for (Py_ssize_t i = 0; i < nconsts; i++) {
            PyObject *v = asdl_seq_GET(seq, i)->v.Constant.value;
            Py_INCREF(v);
            PyTuple_SET_ITEM(folded, i, v);
        }

        if (build_op == TUPLE_BUILD) {
            set_cur_tmp(c, stack);
            return compiler_load_const_new(c, folded);
        } else {
            if (build_op == SET_BUILD) {
                PyObject *folded_set = PyFrozenSet_New(folded);
                Py_DECREF(folded);
                CHECK(folded_set);
                folded = folded_set;
            }
            CHECK(addop(c, build_op, imm_oparg(npushed), stack_oparg, stack_oparg));
            CHECK(compiler_load_const_new(c, folded));
            CHECK(addop(c, update_op, imm_oparg(1), *cur_out(c), stack_oparg));
            set_cur_tmp(c, stack);
            return addop(c, MOVE_FAST, stack_oparg, UNSED_OPARG, RELOC_OPARG);
        }
    }

    bool big = len + npushed > TMP_USE_GUIDELINE;
    bool seen_star = false;
    for (Py_ssize_t i = 0; i < len; i++) {
        if (asdl_seq_GET(seq, i)->kind == Starred_kind) {
            seen_star = true;
            break;
        }
    }
    if (!seen_star && !big) {
        for (Py_ssize_t i = 0; i < len; i++) {
            CHECK(compiler_visit_expr_as_tmp(c, asdl_seq_GET(seq, i)));
        }
        set_cur_tmp(c, stack);
        return addop(c, build_op, imm_oparg(len + npushed), stack_oparg, RELOC_OPARG);
    }

    Opcode tmp_build_op = build_op == TUPLE_BUILD ? LIST_BUILD : build_op;
    bool sequence_built = false;
    if (big) {
        CHECK(addop(c, tmp_build_op, imm_oparg(npushed), stack_oparg, stack_oparg));
        sequence_built = true;
        set_cur_tmp(c, stack + 1);
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        expr_ty elt = asdl_seq_GET(seq, i);
        if (elt->kind == Starred_kind) {
            if (!sequence_built) {
                CHECK(addop(c, tmp_build_op, imm_oparg(npushed + i), stack_oparg, stack_oparg));
                sequence_built = true;
                set_cur_tmp(c, stack + 1);
            }
            CHECK(compiler_visit_expr(c, elt->v.Starred.value));
            CHECK(addop(c, update_op, imm_oparg(1), alloc_arg_auto(c), stack_oparg));
            set_cur_tmp(c, stack + 1);
        } else {
            CHECK(compiler_visit_expr_as_tmp(c, elt));
            if (sequence_built) {
                CHECK(addop(c, update_op, imm_oparg(0), alloc_arg_auto(c), stack_oparg));
                set_cur_tmp(c, stack + 1);
            }
        }
    }

    assert(sequence_built);
    set_cur_tmp(c, stack);
    if (build_op == TUPLE_BUILD) {
        return addop(c, TUPLE_FROM_LIST, stack_oparg, UNSED_OPARG, RELOC_OPARG);
    }
    return addop(c, MOVE_FAST, stack_oparg, UNSED_OPARG, RELOC_OPARG);
}

static int
dict_building_helper(struct compiler *c, expr_ty e) {
    asdl_expr_seq *keys = e->v.Dict.keys;
    asdl_expr_seq *values = e->v.Dict.values;
    Py_ssize_t stack = cur_tmp(c);
    Oparg stack_oparg = tmp_oparg(stack);
    Py_ssize_t len = asdl_seq_LEN(keys);

    if (!len) {
        return addop(c, DICT_BUILD, imm_oparg(0), stack_oparg, RELOC_OPARG);
    }

    int lineno = c->u->u_lineno;
    c->u->u_lineno = -1;

    for (Py_ssize_t i = 0; i < len;) {
        assert(i < len);
        expr_ty key = asdl_seq_GET(keys, i);
        if (key) {
            Py_ssize_t n_normal = 0;
            Py_ssize_t n_const = 0;
            for (Py_ssize_t i_end = i; i_end < len && n_normal < TMP_USE_GUIDELINE / 2;) {
                key = asdl_seq_GET(keys, i_end);
                if (!key) {
                    break;
                }
                if (key->kind != Constant_kind) {
                    i_end++;
                    n_normal++;
                } else {
                    while (i_end < len && n_const < TMP_USE_GUIDELINE - 1) {
                        key = asdl_seq_GET(keys, i_end);
                        if (!key || key->kind != Constant_kind) {
                            break;
                        }
                        i_end++;
                        n_const++;
                    }
                    if (n_const <= 4) {
                        n_normal += n_const;
                        n_const = 0;
                        if (n_normal >= TMP_USE_GUIDELINE / 2) {
                            n_normal = TMP_USE_GUIDELINE / 2;
                            break;
                        }
                    } else {
                        break;
                    }
                }
            }

            if (n_normal) {
                if (i == 0) {
                    for (Py_ssize_t j = 0; j < n_normal; j++) {
                        CHECK(compiler_visit_expr_as_tmp(c, asdl_seq_GET(keys, i + j)));
                        CHECK(compiler_visit_expr_as_tmp(c, asdl_seq_GET(values, i + j)));
                    }
                    CHECK(addop_line(c, DICT_BUILD, imm_oparg(n_normal), stack_oparg, RELOC_OPARG, lineno));
                } else {
                    CHECK(alloc_arg_tmp(c));
                    for (Py_ssize_t j = 0; j < n_normal; j++) {
                        CHECK(compiler_visit_expr(c, asdl_seq_GET(keys, i + j)));
                        Oparg key_oparg = alloc_arg_auto(c);
                        CHECK(compiler_visit_expr(c, asdl_seq_GET(values, i + j)));
                        Oparg value_oparg = alloc_arg_auto(c);
                        CHECK(addop_line(c, DICT_INSERT, key_oparg, value_oparg, stack_oparg, lineno));
                        set_cur_tmp(c, stack + 1);
                    }
                    CHECK(addop(c, MOVE_FAST, stack_oparg, UNSED_OPARG, RELOC_OPARG));
                }
                i += n_normal;
                set_cur_tmp(c, stack);
            }
            if (n_const) {
                if (i != 0) {
                    CHECK(alloc_arg_tmp(c));
                }
                PyObject *const_keys = PyTuple_New(n_const);
                CHECK(const_keys);
                for (Py_ssize_t j = 0; j < n_const; j++) {
                    PyObject *k = asdl_seq_GET(keys, i + j)->v.Constant.value;
                    Py_INCREF(k);
                    PyTuple_SET_ITEM(const_keys, j, k);
                }
                CHECK(compiler_load_const_new(c, const_keys));
                Oparg keys_oparg = *cur_out(c);
                for (Py_ssize_t j = 0; j < n_const; j++) {
                    CHECK(compiler_visit_expr_as_tmp(c, asdl_seq_GET(values, i + j)));
                }
                if (i == 0) {
                    CHECK(addop_line(c, DICT_WITH_CONST_KEYS, keys_oparg, stack_oparg, RELOC_OPARG, lineno));
                } else {
                    Oparg value_arg = oparg_by_offset(stack_oparg, 1);
                    CHECK(addop_line(c, DICT_WITH_CONST_KEYS, keys_oparg, value_arg, value_arg, lineno));
                    CHECK(addop_line(c, DICT_UPDATE, value_arg, UNSED_OPARG, stack_oparg, lineno));
                    CHECK(addop(c, MOVE_FAST, stack_oparg, UNSED_OPARG, RELOC_OPARG));
                }
                i += n_const;
                set_cur_tmp(c, stack);
            }
        } else {
            if (i == 0) {
                CHECK(addop_line(c, DICT_BUILD, imm_oparg(0), stack_oparg, RELOC_OPARG, lineno));
            }
            CHECK(alloc_arg_tmp(c));
            CHECK(compiler_visit_expr(c, asdl_seq_GET(values, i)));
            CHECK(addop_line(c, DICT_UPDATE, alloc_arg_auto(c), UNSED_OPARG, stack_oparg, lineno));
            CHECK(addop(c, MOVE_FAST, stack_oparg, UNSED_OPARG, RELOC_OPARG));
            i++;
            set_cur_tmp(c, stack);
        }
    }
    return 1;
}

static int
kwargs_building_helper(struct compiler *c, asdl_keyword_seq *seq) {
    Py_ssize_t stack = cur_tmp(c);
    Oparg stack_oparg = tmp_oparg(stack);
    Py_ssize_t len = asdl_seq_LEN(seq);

    if (!len) {
        return addop(c, DICT_BUILD, imm_oparg(0), stack_oparg, RELOC_OPARG);
    }

    int lineno = c->u->u_lineno;
    c->u->u_lineno = -1;

    for (Py_ssize_t i = 0; i < len;) {
        assert(i < len);
        keyword_ty kwarg = asdl_seq_GET(seq, i);
        if (kwarg->arg) {
            Py_ssize_t n_const = 0;
            for (Py_ssize_t i_end = i; i_end < len && n_const < TMP_USE_GUIDELINE - 1;) {
                kwarg = asdl_seq_GET(seq, i_end);
                if (!kwarg->arg) {
                    break;
                }
                i_end++;
                n_const++;
            }

            if (n_const) {
                if (i != 0) {
                    CHECK(alloc_arg_tmp(c));
                }
                PyObject *const_keys = PyTuple_New(n_const);
                CHECK(const_keys);
                for (Py_ssize_t j = 0; j < n_const; j++) {
                    PyObject *k = asdl_seq_GET(seq, i + j)->arg;
                    Py_INCREF(k);
                    PyTuple_SET_ITEM(const_keys, j, k);
                }
                CHECK(compiler_load_const_new(c, const_keys));
                Oparg keys_oparg = *cur_out(c);
                for (Py_ssize_t j = 0; j < n_const; j++) {
                    CHECK(compiler_visit_expr_as_tmp(c, asdl_seq_GET(seq, i + j)->value));
                }
                if (i == 0) {
                    CHECK(addop(c, DICT_WITH_CONST_KEYS, keys_oparg, stack_oparg, RELOC_OPARG));
                } else {
                    Oparg value_arg = oparg_by_offset(stack_oparg, 1);
                    CHECK(addop(c, DICT_WITH_CONST_KEYS, keys_oparg, value_arg, value_arg));
                    CHECK(addop(c, DICT_MERGE, value_arg, UNSED_OPARG, stack_oparg));
                    CHECK(addop(c, MOVE_FAST, stack_oparg, UNSED_OPARG, RELOC_OPARG));
                }
                i += n_const;
                set_cur_tmp(c, stack);
            }
        } else {
            if (i == 0) {
                CHECK(addop(c, DICT_BUILD, imm_oparg(0), stack_oparg, RELOC_OPARG));
            }
            CHECK(alloc_arg_tmp(c));
            CHECK(compiler_visit_expr(c, asdl_seq_GET(seq, i)->value));
            CHECK(addop(c, DICT_MERGE, alloc_arg_auto(c), UNSED_OPARG, stack_oparg));
            CHECK(addop(c, MOVE_FAST, stack_oparg, UNSED_OPARG, RELOC_OPARG));
            i++;
            set_cur_tmp(c, stack);
        }
    }
    c->u->u_lineno = lineno;
    return 1;
}

static int
unpack_star_helper(struct compiler *c, Py_ssize_t i_star, Py_ssize_t size,
                   Oparg input, Oparg outputs) {
    Opcode opcode;
    Py_ssize_t flag;
    if (i_star == size) {
        opcode = UNPACK_SEQUENCE;
        flag = size;
    } else {
        opcode = UNPACK_EX;
        /* assert is enough, not necessary to call compiler_error.
         * Who will unpack 65535 values? (supposing _Py_OPARG has 32 bits) */
        assert(size < ((Py_ssize_t) 1 << (sizeof(_Py_OPARG) * CHAR_BIT / 2)));
        Py_ssize_t before = i_star;
        Py_ssize_t after = size - i_star - 1;
        flag = 0;
        do {
            flag = (flag << 4) | (after & 0b1111);
            after >>= 4;
            flag = (flag << 4) | (before & 0b1111);
            before >>= 4;
        } while (before | after);
    }

    CHECK(addop(c, opcode, imm_oparg(flag), input, outputs));
    return 1;
}

static Py_ssize_t
find_unpack_star(asdl_expr_seq *elts) {
    Py_ssize_t size = asdl_seq_LEN(elts);
    Py_ssize_t i_star = 0;
    for (; i_star < size; i_star++) {
        if (asdl_seq_GET(elts, i_star)->kind == Starred_kind) {
            break;
        }
    }
    return i_star;
}

static PyTypeObject *
infer_type(expr_ty e) {
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
check_caller(struct compiler *c, expr_ty e) {
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
check_subscripter(struct compiler *c, expr_ty e) {
    PyObject *v;

    switch (e->kind) {
        case Constant_kind:
            v = e->v.Constant.value;
            if (!(v == Py_None || v == Py_Ellipsis ||
                  PyLong_Check(v) || PyFloat_Check(v) || PyComplex_Check(v) ||
                  PyAnySet_Check(v))) {
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
check_index(struct compiler *c, expr_ty e, expr_ty s) {
    PyObject *v;

    PyTypeObject *index_type = infer_type(s);
    if (!index_type
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

static int
check_keywords(struct compiler *c, asdl_keyword_seq *keywords) {
    Py_ssize_t nkeywords = asdl_seq_LEN(keywords);
    for (Py_ssize_t i = 0; i < nkeywords; i++) {
        keyword_ty key = asdl_seq_GET(keywords, i);
        if (!key->arg) {
            continue;
        }
        if (!verify_var_name(c, key->arg, Store)) {
            return -1;
        }
        for (Py_ssize_t j = i + 1; j < nkeywords; j++) {
            keyword_ty other = asdl_seq_GET(keywords, j);
            if (other->arg && !PyUnicode_Compare(key->arg, other->arg)) {
                SET_LOC(c, other);
                compiler_error(c, "keyword argument repeated: %U", key->arg);
                return -1;
            }
        }
    }
    return 0;
}

static int
check_ann_expr(struct compiler *c, expr_ty e) {
    CHECK(compiler_visit_expr_as_tmp(c, e));
    free_cur_tmp(c, 1);
    return 1;
}

static int
check_ann_subscr(struct compiler *c, expr_ty e) {
    /* We check that everything in a subscript is defined at runtime. */
    switch (e->kind) {
        case Slice_kind:
            CHECK(!e->v.Slice.lower || check_ann_expr(c, e->v.Slice.lower));
            CHECK(!e->v.Slice.upper || check_ann_expr(c, e->v.Slice.upper));
            CHECK(!e->v.Slice.step || check_ann_expr(c, e->v.Slice.step));
            return 1;
        case Tuple_kind: {
            /* extended slice */
            asdl_expr_seq *elts = e->v.Tuple.elts;
            Py_ssize_t n = asdl_seq_LEN(elts);
            for (Py_ssize_t i = 0; i < n; i++) {
                CHECK(check_ann_subscr(c, asdl_seq_GET(elts, i)));
            }
            return 1;
        }
        default:
            return check_ann_expr(c, e);
    }
}

static bool
need_ex_call(asdl_expr_seq *args, asdl_keyword_seq *kwargs) {
    Py_ssize_t nargs = asdl_seq_LEN(args);
    Py_ssize_t nkwargs = asdl_seq_LEN(kwargs);

    if (nargs + nkwargs > TMP_USE_GUIDELINE) {
        return true;
    }
    for (Py_ssize_t i = 0; i < nargs; i++) {
        if (asdl_seq_GET(args, i)->kind == Starred_kind) {

            return true;
        }
    }
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        if (!asdl_seq_GET(kwargs, i)->arg) {
            return true;
        }
    }
    return false;
}

static int
compiler_call_helper(struct compiler *c, bool method_call,
                     int implicit_args, asdl_expr_seq *args, asdl_keyword_seq *kwargs) {
    CHECK(check_keywords(c, kwargs) != -1);

    Py_ssize_t nargs = asdl_seq_LEN(args);
    Py_ssize_t nkwargs = asdl_seq_LEN(kwargs);
    Oparg stack = oparg_by_offset(cur_tmp_arg(c), -(1 + implicit_args + method_call));

    if (need_ex_call(args, kwargs)) {
        assert(!method_call);
        CHECK(sequence_building_helper(c, args, TUPLE_BUILD, LIST_UPDATE, implicit_args));
        CHECK(alloc_arg_tmp(c));
        if (kwargs) {
            CHECK(kwargs_building_helper(c, kwargs));
            CHECK(alloc_arg_tmp(c));
        }
        return addop(c, CALL_FUNCTION_EX, imm_oparg(nkwargs > 0), stack, RELOC_OPARG);
    }
    for (Py_ssize_t i = 0; i < asdl_seq_LEN(args); i++) {
        CHECK(compiler_visit_expr_as_tmp(c, asdl_seq_GET(args, i)));
    }
    if (nkwargs) {
        PyObject *names = PyTuple_New(nkwargs);
        CHECK(names);
        for (Py_ssize_t i = 0; i < nkwargs; i++) {
            keyword_ty kwarg = asdl_seq_GET(kwargs, i);
            Py_INCREF(kwarg->arg);
            PyTuple_SET_ITEM(names, i, kwarg->arg);
            if (!compiler_visit_expr_as_tmp(c, kwarg->value)) {
                Py_DECREF(names);
            }
        }
        CHECK(compiler_load_const_new(c, names));
        CHECK(alloc_arg_tmp(c));
    }
    static const Opcode call_opcodes[2][2] = {
            {CALL_FUNCTION,    CALL_METHOD},
            {CALL_FUNCTION_KW, CALL_METHOD_KW},
    };
    Oparg flag = imm_oparg((implicit_args + nargs + nkwargs));
    return addop(c, call_opcodes[nkwargs > 0][method_call], flag, stack, RELOC_OPARG);
}

/*
 * Compile expressions
*/

static int
compiler_attribute(struct compiler *c, expr_ty e) {
    CHECK(verify_var_name(c, e->v.Attribute.attr, e->v.Attribute.ctx));

    CHECK(compiler_visit_expr(c, e->v.Attribute.value));
    Oparg obj = alloc_arg_auto(c);
    Oparg attr;
    CHECK(compiler_load_name(c, e->v.Attribute.attr, &attr));

    static const Opcode opcodes[] = {[Load]=LOAD_ATTR, [Store]=STORE_ATTR, [Del]=DELETE_ATTR};
    Oparg output = e->v.Attribute.ctx == Del ? UNSED_OPARG : RELOC_OPARG;
    return addop_line(c, opcodes[e->v.Attribute.ctx], obj, attr, output, e->end_lineno);
}

static int
compiler_subscript(struct compiler *c, expr_ty e) {
    if (e->v.Subscript.ctx == Load) {
        CHECK(check_subscripter(c, e->v.Subscript.value));
        CHECK(check_index(c, e->v.Subscript.value, e->v.Subscript.slice));
    }

    CHECK(compiler_visit_expr(c, e->v.Subscript.value));
    Oparg value = alloc_arg_auto(c);
    CHECK(compiler_visit_expr(c, e->v.Subscript.slice));
    Oparg slice = alloc_arg_auto(c);
    static const Opcode opcodes[] = {[Load]=LOAD_SUBSCR, [Store]=STORE_SUBSCR, [Del]=DELETE_SUBSCR};
    Oparg output = e->v.Attribute.ctx == Del ? UNSED_OPARG : RELOC_OPARG;
    return addop(c, opcodes[e->v.Attribute.ctx], value, slice, output);
}

static int
compiler_named_expr(struct compiler *c, expr_ty e) {
    CHECK(compiler_visit_expr(c, e->v.NamedExpr.value));
    CHECK(compiler_bind_to_var_name(c, e->v.NamedExpr.target->v.Name.id));
    Oparg v = *cur_out(c);
    if (OPARG_TYPE(v) == OpargTmp) {
        /* because tmp register cannot be served as out reg */
        return addop(c, MOVE_FAST, v, UNSED_OPARG, RELOC_OPARG);
    }
    return 1;
}

static int
compiler_boolop(struct compiler *c, expr_ty e) {
    bool is_or = e->v.BoolOp.op == Or;
    Opcode jump_opcode = is_or ? JUMP_IF_TRUE : JUMP_IF_FALSE;
    basicblock *b_end = compiler_new_block(c);
    CHECK(b_end);

    Oparg output = new_tmp_arg(c);
    free_cur_tmp(c, 1);

    Py_ssize_t n = asdl_seq_LEN(e->v.BoolOp.values) - 1;
    for (Py_ssize_t i = 0;; i++) {
        expr_ty value = asdl_seq_GET(e->v.BoolOp.values, i);
        if (value->kind == Constant_kind) {
            int is_true = PyObject_IsTrue(value->v.Constant.value);
            CHECK(is_true >= 0);
            compiler_load_const(c, value->v.Constant.value);
            if (is_true == is_or || i == n) {
                CHECK(alloc_arg_bind(c, output));
                break;
            } else {
                continue;
            }
        }
        CHECK(compiler_visit_expr(c, value));
        CHECK(alloc_arg_bind(c, output));
        if (i == n) {
            break;
        }
        CHECK(addop_jump(c, jump_opcode, output, NEG_ADDR_OPARG, b_end, NULL));
    }
    compiler_use_block(c, b_end);
    CHECK(addop_noline(c, MOVE_FAST, output, UNSED_OPARG, RELOC_OPARG));
    return 1;
}

static int
compiler_compare(struct compiler *c, expr_ty e) {
    CHECK(check_compare(c, e));
    Py_ssize_t n = asdl_seq_LEN(e->v.Compare.ops) - 1;
    if (n == 0) {
        Py_ssize_t old_tmp = cur_tmp(c);
        CHECK(compiler_visit_expr(c, e->v.Compare.left));
        Oparg left = alloc_arg_auto(c);
        CHECK(compiler_visit_expr(c, asdl_seq_GET(e->v.Compare.comparators, 0)));
        Oparg right = alloc_arg_auto(c);
        cmpop_ty cmp_op = asdl_seq_GET(e->v.Compare.ops, 0);
        CHECK(addop(c, get_compare_opcode(cmp_op), left, right, RELOC_OPARG));
        set_cur_tmp(c, old_tmp);
        if (cmp_op == IsNot || cmp_op == NotIn) {
            CHECK(addop(c, UNARY_NOT, alloc_arg_auto(c), UNSED_OPARG, RELOC_OPARG));
        }
        return 1;
    }

    basicblock *b_end = compiler_new_block(c);
    CHECK(b_end);
    Oparg output = new_tmp_arg(c);
    Oparg reuse_args[2] = {RELOC_OPARG, RELOC_OPARG};
    int flip = 0;

    CHECK(compiler_visit_expr(c, e->v.Compare.left));
    Oparg left = alloc_arg_reuse(c, &reuse_args[flip]);
    for (Py_ssize_t i = 0;; i++) {
        CHECK(compiler_visit_expr(c, asdl_seq_GET(e->v.Compare.comparators, i)));
        Oparg right = alloc_arg_reuse(c, &reuse_args[!flip]);
        cmpop_ty cmp_op = asdl_seq_GET(e->v.Compare.ops, i);
        CHECK(addop(c, get_compare_opcode(cmp_op), left, right, output));
        if (cmp_op == IsNot || cmp_op == NotIn) {
            CHECK(addop(c, UNARY_NOT, output, UNSED_OPARG, output));
        }
        if (i == n) {
            break;
        }
        CHECK(addop_jump(c, JUMP_IF_FALSE, output, NEG_ADDR_OPARG, b_end, NULL));
        left = right;
        flip = !flip;
    }

    compiler_use_block(c, b_end);
    CHECK(addop_noline(c, MOVE_FAST, output, UNSED_OPARG, RELOC_OPARG));
    return 1;
}

static int
compiler_lambda(struct compiler *c, expr_ty e) {
    static identifier name;
    CHECK(intern_static_identifier(&name, "<lambda>"));

    CHECK(compiler_check_debug_args(c, e->v.Lambda.args));
    CHECK(compiler_enter_scope(c, name, COMPILER_SCOPE_LAMBDA, e, e->lineno));
    SET_LOC(c, e);

    /* This makes None the first constant, so the lambda have no docstring */
    CHECK(compiler_add_docstring(c, NULL, false));

    CHECK(compiler_visit_expr(c, e->v.Lambda.body));
    CHECK(addop_exit(c, RETURN_VALUE, alloc_arg_auto(c), UNSED_OPARG, UNSED_OPARG, NULL));
    return compiler_scope_to_function(c, e->v.Lambda.args, NULL);
}

static int
compiler_call(struct compiler *c, expr_ty e) {
    expr_ty callable = e->v.Call.func;
    asdl_expr_seq *args = e->v.Call.args;
    asdl_keyword_seq *kwargs = e->v.Call.keywords;
    bool method_call = callable->kind == Attribute_kind;

    if (method_call && !need_ex_call(args, kwargs)) {
        assert(callable->v.Attribute.ctx == Load);
        Py_ssize_t old_tmp = cur_tmp(c);
        CHECK(compiler_visit_expr(c, callable->v.Attribute.value));
        Oparg obj = alloc_arg_auto(c);
        set_cur_tmp(c, old_tmp);
        Oparg field;
        CHECK(compiler_load_name(c, callable->v.Attribute.attr, &field));
        Oparg stack = new_tmp_arg_n(c, 2);
        int old_lineno = c->u->u_lineno;
        c->u->u_lineno = callable->end_lineno;
        CHECK(addop(c, LOAD_METHOD, obj, field, stack));
        CHECK(compiler_call_helper(c, true, 0, args, kwargs));
        c->u->u_lineno = old_lineno;
        return 1;
    }

    CHECK(check_caller(c, e->v.Call.func));
    CHECK(compiler_visit_expr(c, e->v.Call.func));
    if (!(asdl_seq_LEN(args) || asdl_seq_LEN(kwargs))) {
        /* Eliminate call stack for f() */
        Oparg flag = imm_oparg(0);
        return addop(c, CALL_FUNCTION, flag, alloc_arg_auto(c), RELOC_OPARG);
    }
    CHECK(alloc_arg_tmp(c));
    CHECK(compiler_call_helper(c, false, 0, args, kwargs));
    return 1;
}

static int
compiler_joined_str(struct compiler *c, expr_ty e) {
    Py_ssize_t num = asdl_seq_LEN(e->v.JoinedStr.values);
    /* num may be 0 */
    if (num == 1) {
        return compiler_visit_expr(c, asdl_seq_GET(e->v.JoinedStr.values, 0));
    } else if (num > 0 && num <= TMP_USE_GUIDELINE) {
        Oparg stack = cur_tmp_arg(c);
        for (Py_ssize_t i = 0; i < num; i++) {
            CHECK(compiler_visit_expr_as_tmp(c, asdl_seq_GET(e->v.JoinedStr.values, i)));
        }
        CHECK(addop(c, BUILD_STRING, imm_oparg(num), stack, RELOC_OPARG));
        return 1;
    } else {
        CHECK(sequence_building_helper(c, e->v.JoinedStr.values, TUPLE_BUILD, LIST_UPDATE, 0));
        Oparg tuple = alloc_arg_auto(c);
        CHECK(addop(c, BUILD_STRING, imm_oparg(0), tuple, RELOC_OPARG));
        return 1;
    }
}

/* Used to implement f-strings. Format a single value. */
static int
compiler_formatted_value(struct compiler *c, expr_ty e) {
    /* Our oparg1 encodes 2 pieces of information: the conversion
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
    /* The expression to be formatted. */
    Oparg reuse = RELOC_OPARG;

    CHECK(compiler_visit_expr(c, e->v.FormattedValue.value));

    int flag;
    int conversion = e->v.FormattedValue.conversion;
    switch (conversion) {
        case 's':
            flag = 0;
            break;
        case 'r':
            flag = 1;
            break;
        case 'a':
            flag = 2;
            break;
        case -1:
            flag = -1;
            break;
        default:
            PyErr_Format(PyExc_SystemError, "unrecognized conversion character %d", conversion);
            return 0;
    }
    if (flag >= 0) {
        Oparg value_arg = alloc_arg_reuse(c, &reuse);
        Oparg flag_arg = imm_oparg(flag);
        CHECK(addop(c, STRINGIFY_VALUE, flag_arg, value_arg, RELOC_OPARG));
    }

    Oparg value_arg = alloc_arg_reuse(c, &reuse);
    if (e->v.FormattedValue.format_spec) {
        CHECK(compiler_visit_expr(c, e->v.FormattedValue.format_spec));
    } else {
        static PyObject *empty_str;
        CHECK(intern_static_identifier(&empty_str, ""));
        CHECK(compiler_load_const(c, empty_str));
    }
    Oparg spec_arg = alloc_arg_auto(c);
    return addop(c, FORMAT_VALUE, value_arg, spec_arg, RELOC_OPARG);
}

static int
compiler_slice(struct compiler *c, expr_ty e) {
    expr_ty const slices[3] = {e->v.Slice.lower, e->v.Slice.upper, e->v.Slice.step};
    int num = 0;
    int flag = 0;
    for (int i = 0; i < 3; ++i) {
        int has_value = slices[i] != NULL;
        num += has_value;
        flag |= has_value << i;
    }
    Oparg flag_arg = imm_oparg(flag);

    Oparg slice_arg = cur_tmp_arg(c);
    if (num == 1) {
        for (int i = 0; i < 3; ++i) {
            if (slices[i]) {
                CHECK(compiler_visit_expr(c, slices[i]));
                slice_arg = alloc_arg_auto(c);
                break;
            }
        }
    } else if (num) {
        for (int i = 0; i < 3; ++i) {
            if (slices[i]) {
                CHECK(compiler_visit_expr_as_tmp(c, slices[i]));
            }
        }
    }
    return addop(c, BUILD_SLICE, flag_arg, slice_arg, RELOC_OPARG);
}

static int
compiler_if_expr(struct compiler *c, expr_ty e) {
    basicblock *b_end = compiler_new_block(c);
    basicblock *b_else = compiler_new_block(c);
    CHECK(b_end && b_else);
    CHECK(compiler_logical_expr(c, e->v.IfExp.test, b_else, false));

    /* Don't be surprised, we free this space */
    Oparg output = new_tmp_arg(c);
    free_cur_tmp(c, 1);

    CHECK(compiler_visit_expr(c, e->v.IfExp.body));
    CHECK(alloc_arg_bind(c, output));

    CHECK(addop_jump_noline(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, b_end, b_else));

    CHECK(compiler_visit_expr(c, e->v.IfExp.orelse));
    CHECK(alloc_arg_bind(c, output));

    /* Ensure there is only one output arg,
     * generally, this instruction will be optimized out. */
    compiler_use_block(c, b_end);
    CHECK(addop_noline(c, MOVE_FAST, output, UNSED_OPARG, RELOC_OPARG));
    return 1;
}

static int
compiler_tuple_or_list(struct compiler *c, expr_ty e) {
    asdl_expr_seq *elts = e->v.Tuple.elts;
    switch (e->v.Tuple.ctx) {
        case Load: {
            if (e->kind == List_kind) {
                return sequence_building_helper(c, elts, LIST_BUILD, LIST_UPDATE, 0);
            } else {
                return sequence_building_helper(c, elts, TUPLE_BUILD, LIST_UPDATE, 0);
            }
        }
        case Del: {
            Py_ssize_t n = asdl_seq_LEN(elts);
            for (Py_ssize_t i = 0; i < n; i++) {
                CHECK(compiler_visit_expr(c, asdl_seq_GET(elts, i)));
            }
            return 1;
        }
        case Store: {
            Py_ssize_t size = asdl_seq_LEN(elts);
            Py_ssize_t i_star = find_unpack_star(elts);
            for (Py_ssize_t j = i_star + 1; j < size; j++) {
                if (asdl_seq_GET(elts, j)->kind == Starred_kind) {
                    compiler_error(c, "multiple starred expressions in assignment");
                    return 0;
                }
            }

            Oparg tmps = new_tmp_arg_n(c, size);
            unpack_star_helper(c, i_star, size, RELOC_OPARG, tmps);
            adjust_cur_out(c, 2);
            Oparg *to_unpack = cur_out(c);

            for (Py_ssize_t i = 0; i < size; i++) {
                Oparg tmp = oparg_by_offset(tmps, i);
                set_cur_out(c, &tmp);
                expr_ty elt = asdl_seq_GET(elts, i);
                CHECK(compiler_bind_to_target(c, i == i_star ? elt->v.Starred.value : elt));
            }
            set_cur_out(c, to_unpack);
            return 1;
        }
    }
    Py_UNREACHABLE();
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
compiler_comprehension_recursively(struct compiler *c,
                                   asdl_comprehension_seq *generators, int gen_index,
                                   Opcode opcode, Oparg container, expr_ty elt, expr_ty val) {
    comprehension_ty gen = asdl_seq_GET(generators, gen_index);
    bool is_async = gen->is_async;
    Py_ssize_t nifs = asdl_seq_LEN(gen->ifs);
    bool need_iteration = true;
    Oparg iterator;

    if (gen_index && !is_async) {
        asdl_expr_seq *seq_elts;
        switch (gen->iter->kind) {
            case Tuple_kind:
                seq_elts = gen->iter->v.Tuple.elts;
                break;
            case List_kind:
                seq_elts = gen->iter->v.List.elts;
                break;
            case Set_kind:
                seq_elts = gen->iter->v.Set.elts;
                break;
            default:
                seq_elts = NULL;
        }
        if (asdl_seq_LEN(seq_elts) == 1) {
            expr_ty the_only_expr = asdl_seq_GET(seq_elts, 0);
            if (the_only_expr->kind != Starred_kind) {
                CHECK(compiler_visit_expr(c, the_only_expr));
                CHECK(compiler_bind_to_target(c, gen->target));
                need_iteration = false;
            }
        }
    }

    if (need_iteration) {
        if (gen_index == 0) {
            iterator = make_oparg(OpargFast, 0);
        } else {
            iterator = RELOC_OPARG;
            CHECK(compiler_visit_expr(c, gen->iter));
            Opcode get_iter_op = gen->is_async ? GET_ASYNC_ITER : GET_ITER;
            CHECK(addop(c, get_iter_op, alloc_arg_reuse(c, &iterator), UNSED_OPARG, RELOC_OPARG));
            alloc_arg_reuse(c, &iterator);
        }
    }

    basicblock *b_iter = NULL;
    basicblock *b_end = NULL;
    if (need_iteration) {
        CHECK(b_iter = compiler_new_block(c));
        CHECK(b_end = compiler_new_block(c));
        compiler_use_block(c, b_iter);
        if (is_async) {
            Oparg value = new_tmp_arg_n(c, 2);
            CHECK(addop_jump(c, START_TRY, value, UNSED_OPARG, b_end, NULL));
            struct fblockinfo fb = {.fb_type = FB_FINALLY, .fb_datum=&value};
            CHECK(compiler_push_fblock(c, &fb));
            CHECK(addop(c, GET_ASYNC_NEXT, iterator, UNSED_OPARG, value));
            CHECK(addop(c, YIELD_FROM, value, UNSED_OPARG, RELOC_OPARG));
            Oparg *async_result = cur_out(c);
            CHECK(addop(c, REVOKE_EXCEPT, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));
            free_cur_tmp(c, 2);
            set_cur_out(c, async_result);
            compiler_pop_fblock(c);
        } else {
            CHECK(addop_jump(c, FOR_ITER, iterator, RELOC_OPARG, b_end, NULL));
            adjust_cur_out(c, 2);
        }
        CHECK(compiler_bind_to_target(c, gen->target));
    } else if (nifs) {
        CHECK(b_iter = b_end = compiler_new_block(c));
    }

    for (Py_ssize_t i = 0; i < nifs; i++) {
        CHECK(compiler_logical_expr(c, asdl_seq_GET(gen->ifs, i), b_iter, false));
    }

    if (++gen_index < asdl_seq_LEN(generators)) {
        CHECK(compiler_comprehension_recursively(c, generators, gen_index, opcode, container, elt, val));
    } else {
        CHECK(compiler_visit_expr(c, elt));
        Py_ssize_t old_tmp = cur_tmp(c);
        if (opcode == YIELD_VALUE) {
            Oparg arg = new_tmp_arg(c);
            CHECK(addop(c, YIELD_VALUE, alloc_arg_reuse(c, &arg), UNSED_OPARG, arg));
        } else {
            if (opcode == DICT_INSERT) {
                Oparg key = alloc_arg_auto(c);
                CHECK(compiler_visit_expr(c, val));
                Oparg value = alloc_arg_auto(c);
                CHECK(addop(c, opcode, key, value, container));
            } else {
                CHECK(addop(c, opcode, imm_oparg(0), alloc_arg_auto(c), container));
            }
        }
        set_cur_tmp(c, old_tmp);
    }

    if (need_iteration) {
        CHECK(addop_jump(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, b_iter, b_end));
        if (is_async) {
            new_tmp_arg_n(c, 6);
            CHECK(addop(c, END_ASYNC_FOR, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));
            free_cur_tmp(c, 6);
        }
    } else if (b_end) {
        compiler_use_block(c, b_end);
    }
    return 1;
}

static int
compiler_comprehension(struct compiler *c, expr_ty e) {
    Opcode build_op, update_op;
    asdl_comprehension_seq *generators;
    expr_ty elt;
    expr_ty val;
    identifier name;
    switch (e->kind) {
        case GeneratorExp_kind:
            build_op = NOP;
            update_op = YIELD_VALUE;
            generators = e->v.GeneratorExp.generators;
            elt = e->v.GeneratorExp.elt;
            val = NULL;
            static identifier name_genexpr;
            CHECK(intern_static_identifier(&name_genexpr, "<genexpr>"));
            name = name_genexpr;
            break;
        case ListComp_kind:
            build_op = LIST_BUILD;
            update_op = LIST_UPDATE;
            generators = e->v.ListComp.generators;
            elt = e->v.ListComp.elt;
            val = NULL;
            static identifier name_listcomp;
            CHECK(intern_static_identifier(&name_listcomp, "<listcomp>"));
            name = name_listcomp;
            break;
        case SetComp_kind:
            build_op = SET_BUILD;
            update_op = SET_UPDATE;
            generators = e->v.SetComp.generators;
            elt = e->v.SetComp.elt;
            val = NULL;
            static identifier name_setcomp;
            CHECK(intern_static_identifier(&name_setcomp, "<setcomp>"));
            name = name_setcomp;
            break;
        case DictComp_kind:
            build_op = DICT_BUILD;
            update_op = DICT_INSERT;
            generators = e->v.DictComp.generators;
            elt = e->v.DictComp.key;
            val = e->v.DictComp.value;
            static identifier name_dictcomp;
            CHECK(intern_static_identifier(&name_dictcomp, "<dictcomp>"));
            name = name_dictcomp;
            break;
        default:
            Py_UNREACHABLE();
    }

    bool top_await = is_top_level_await(c);
    bool is_out_coro = c->u->u_ste->ste_coroutine;

    CHECK(compiler_enter_scope(c, name, COMPILER_SCOPE_COMPREHENSION, e, e->lineno));
    c->u->u_argcount = 1;
    SET_LOC(c, e);
    bool is_inner_coro = c->u->u_ste->ste_coroutine;
    if (is_inner_coro && !(is_out_coro || build_op == NOP || top_await)) {
        compiler_error(c, "asynchronous comprehension outside of an asynchronous function");
        return 0;
    }

    Oparg container;
    if (build_op == NOP) {
        // set container to None for RETURN_VALUE
        CHECK(compiler_load_const(c, Py_None));
        container = *cur_out(c);
    } else {
        container = new_tmp_arg(c);
        CHECK(addop(c, build_op, imm_oparg(0), container, container));
    }
    CHECK(compiler_comprehension_recursively(c, generators, 0, update_op, container, elt, val));
    CHECK(addop_exit(c, RETURN_VALUE, container, UNSED_OPARG, UNSED_OPARG, NULL));
    PyCodeObject *co;
    CHECK(assemble(c, &co));
    PyObject *qualname = c->u->u_qualname;
    Py_INCREF(qualname);
    compiler_exit_scope(c);

    if (top_await && is_inner_coro) {
        c->u->u_ste->ste_coroutine = 1;
    }

    Py_ssize_t old_tmp = cur_tmp(c);
    Oparg value = tmp_oparg(old_tmp);
    if (!compiler_load_const_new(c, qualname) || !alloc_arg_tmp(c)) {
        Py_DECREF(co);
        return 0;
    }
    if (!compiler_load_const(c, (PyObject *) co) || !alloc_arg_tmp(c)) {
        Py_DECREF(co);
        return 0;
    }
    int res = compiler_get_closures(c, co, false);
    Py_DECREF(co);
    CHECK(res);
    if (res > 0) {
        CHECK(alloc_arg_tmp(c));
    }
    CHECK(addop(c, MAKE_FUNCTION, imm_oparg(res > 0), value, value));
    set_cur_tmp(c, old_tmp + 1);

    comprehension_ty gen = asdl_seq_GET(generators, 0);
    Oparg iter = RELOC_OPARG;
    CHECK(compiler_visit_expr(c, gen->iter));
    Opcode get_iter_op = gen->is_async ? GET_ASYNC_ITER : GET_ITER;
    CHECK(addop(c, get_iter_op, alloc_arg_reuse(c, &iter), UNSED_OPARG, RELOC_OPARG));
    alloc_arg_reuse(c, &iter);
    CHECK(addop(c, CALL_FUNCTION, imm_oparg(1), value, RELOC_OPARG));

    if (is_inner_coro && build_op != NOP) {
        alloc_arg_bind(c, value);
        CHECK(addop(c, GET_AWAITABLE, value, UNSED_OPARG, value));
        CHECK(addop(c, YIELD_FROM, value, UNSED_OPARG, RELOC_OPARG));
    }
    return 1;
}

static int
compiler_yield(struct compiler *c, expr_ty e) {
    if (c->u->u_ste->ste_type != FunctionBlock) {
        return compiler_error(c, "'yield' outside function");
    }
    if (e->v.Yield.value) {
        CHECK(compiler_visit_expr(c, e->v.Yield.value));
    } else {
        CHECK(compiler_load_const(c, Py_None));
    }
    return addop(c, YIELD_VALUE, alloc_arg_auto(c), UNSED_OPARG, RELOC_OPARG);
}

static int
compiler_yield_from(struct compiler *c, expr_ty e) {
    if (c->u->u_ste->ste_type != FunctionBlock) {
        return compiler_error(c, "'yield' outside function");
    }
    if (c->u->u_scope_type == COMPILER_SCOPE_ASYNC_FUNCTION) {
        return compiler_error(c, "'yield from' inside async function");
    }
    CHECK(compiler_visit_expr(c, e->v.YieldFrom.value));
    Oparg yield_from = new_tmp_arg_n(c, 2);
    Oparg value = alloc_arg_reuse(c, &yield_from);
    CHECK(addop(c, GET_YIELD_FROM_ITER, value, UNSED_OPARG, yield_from));
    return addop(c, YIELD_FROM, yield_from, UNSED_OPARG, RELOC_OPARG);
}

static int
compiler_await(struct compiler *c, expr_ty e) {
    if (!is_top_level_await(c) &&
        c->u->u_scope_type != COMPILER_SCOPE_ASYNC_FUNCTION &&
        c->u->u_scope_type != COMPILER_SCOPE_COMPREHENSION) {
        return compiler_error(c, "'await' outside async function");
    }
    CHECK(compiler_visit_expr(c, e->v.Await.value));
    Oparg yield_from = new_tmp_arg_n(c, 2);
    Oparg value = alloc_arg_reuse(c, &yield_from);
    CHECK(addop(c, GET_AWAITABLE, value, UNSED_OPARG, yield_from));
    return addop(c, YIELD_FROM, yield_from, UNSED_OPARG, RELOC_OPARG);
}

static int
compiler_visit_expr_body(struct compiler *c, expr_ty e) {
    switch (e->kind) {
        case Constant_kind:
            return compiler_load_const(c, e->v.Constant.value);
        case Name_kind:
            return compiler_access_var_name(c, e->v.Name.id, e->v.Name.ctx);
        case Attribute_kind:
            return compiler_attribute(c, e);
        case Subscript_kind:
            return compiler_subscript(c, e);
        case NamedExpr_kind:
            return compiler_named_expr(c, e);
        case BoolOp_kind:
            return compiler_boolop(c, e);
        case Compare_kind:
            return compiler_compare(c, e);
        case BinOp_kind: {
            CHECK(compiler_visit_expr(c, e->v.BinOp.left));
            Oparg l = alloc_arg_auto(c);
            CHECK(compiler_visit_expr(c, e->v.BinOp.right));
            Oparg r = alloc_arg_auto(c);
            return addop(c, get_binary_opcode(e->v.BinOp.op, false), l, r, RELOC_OPARG);
        }
        case UnaryOp_kind: {
            CHECK(compiler_visit_expr(c, e->v.UnaryOp.operand));
            Oparg v = alloc_arg_auto(c);
            return addop(c, get_unary_opcode(e->v.UnaryOp.op), v, UNSED_OPARG, RELOC_OPARG);
        }
        case Lambda_kind: {
            struct compiler_unit *u = c->u;
            return compiler_safe_exit_scope(c, u, compiler_lambda(c, e));
        }
        case Call_kind:
            return compiler_call(c, e);
        case JoinedStr_kind:
            return compiler_joined_str(c, e);
        case FormattedValue_kind:
            return compiler_formatted_value(c, e);
        case Slice_kind:
            return compiler_slice(c, e);
        case IfExp_kind:
            return compiler_if_expr(c, e);
        case Tuple_kind:
        case List_kind:
            return compiler_tuple_or_list(c, e);
        case Set_kind:
            return sequence_building_helper(c, e->v.Set.elts, SET_BUILD, SET_UPDATE, 0);
        case Dict_kind:
            return dict_building_helper(c, e);
        case GeneratorExp_kind:
        case ListComp_kind:
        case SetComp_kind:
        case DictComp_kind: {
            struct compiler_unit *u = c->u;
            return compiler_safe_exit_scope(c, u, compiler_comprehension(c, e));
        }
        case Yield_kind:
            return compiler_yield(c, e);
        case YieldFrom_kind:
            return compiler_yield_from(c, e);
        case Await_kind:
            return compiler_await(c, e);
        case Starred_kind:
            switch (e->v.Starred.ctx) {
                case Store:
                    /* In all legitimate cases, the Starred node was already replaced
                     * by compiler_list/compiler_tuple. XXX: is that okay? */
                    return compiler_error(c, "starred assignment target must be in a list or tuple");
                default:
                    return compiler_error(c, "can't use starred expression here");
            }
        default:
            Py_UNREACHABLE();
    }
}

static int
compiler_visit_expr(struct compiler *c, expr_ty e) {
    int old_lineno = c->u->u_lineno;
    int old_end_lineno = c->u->u_end_lineno;
    int old_col_offset = c->u->u_col_offset;
    int old_end_col_offset = c->u->u_end_col_offset;
    SET_LOC(c, e);
    Py_ssize_t old_tmp = cur_tmp(c);
    int res = compiler_visit_expr_body(c, e);
    set_cur_tmp(c, old_tmp);
    c->u->u_lineno = old_lineno;
    c->u->u_end_lineno = old_end_lineno;
    c->u->u_col_offset = old_col_offset;
    c->u->u_end_col_offset = old_end_col_offset;
    return res;
}

static int
compiler_visit_expr_as_tmp(struct compiler *c, expr_ty e) {
    int old_lineno = c->u->u_lineno;
    int old_end_lineno = c->u->u_end_lineno;
    int old_col_offset = c->u->u_col_offset;
    int old_end_col_offset = c->u->u_end_col_offset;
    SET_LOC(c, e);
    Py_ssize_t old_tmp = cur_tmp(c);
    int res = compiler_visit_expr_body(c, e);
    set_cur_tmp(c, old_tmp);
    res = res && alloc_arg_tmp(c);
    c->u->u_lineno = old_lineno;
    c->u->u_end_lineno = old_end_lineno;
    c->u->u_col_offset = old_col_offset;
    c->u->u_end_col_offset = old_end_col_offset;
    return res;
}

static int
compiler_visit_ann_expr(struct compiler *c, expr_ty e) {
    if (c->c_future->ff_features & CO_FUTURE_ANNOTATIONS) {
        PyObject *str = _PyAST_ExprAsUnicode(e);
        return str && compiler_load_const_new(c, str);
    } else {
        return compiler_visit_expr(c, e);
    }
}

/*
 * Compile statements
*/

static int
compiler_expr_stmt(struct compiler *c, expr_ty value) {
    if (c->c_interactive && !c->u->u_parent) {
        CHECK(compiler_visit_expr(c, value));
        CHECK(addop(c, PRINT_EXPR, alloc_arg_auto(c), UNSED_OPARG, UNSED_OPARG));
        return 1;
    }

    if (value->kind == Constant_kind) {
        /* ignore constant statement */
        CHECK(addop(c, NOP, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));
    } else {
        CHECK(compiler_visit_expr(c, value));
        alloc_arg_auto(c);
    }
    return 1;
}

static int
compiler_function(struct compiler *c, stmt_ty s) {
    arguments_ty args;
    expr_ty returns;
    identifier name;
    asdl_expr_seq *decos;
    asdl_stmt_seq *body;
    int scope_type;
    if (s->kind == AsyncFunctionDef_kind) {
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

    CHECK(compiler_check_debug_args(c, args));

    Py_ssize_t decos_num = asdl_seq_LEN(decos);
    Py_ssize_t deco_location = cur_tmp(c);
    int firstlineno;
    if (decos_num) {
        firstlineno = asdl_seq_GET(decos, 0)->lineno;
        for (int i = 0; i < decos_num; i++) {
            CHECK(compiler_visit_expr_as_tmp(c, asdl_seq_GET(decos, i)));
        }
    } else {
        firstlineno = s->lineno;
    }
    c->u->u_lineno = s->lineno;

    CHECK(compiler_enter_scope(c, name, scope_type, s, firstlineno));
    CHECK(compiler_add_docstring(c, body, false));
    CHECK(compiler_visit_stmts(c, body));
    if (!compiler_current_block(c)->b_exit) {
        c->u->u_lineno = -1;
        CHECK(compiler_load_const(c, Py_None));
        CHECK(addop_exit(c, RETURN_VALUE, *cur_out(c), UNSED_OPARG, UNSED_OPARG, NULL));
    }
    CHECK(compiler_scope_to_function(c, args, returns));

    /* decorators */
    CHECK(compiler_apply_decorators(c, deco_location, decos_num));
    set_cur_tmp(c, deco_location);
    return compiler_bind_to_var_name(c, name);
}

static int
compiler_class(struct compiler *c, stmt_ty s) {
    asdl_expr_seq *decos = s->v.ClassDef.decorator_list;
    Py_ssize_t decos_num = asdl_seq_LEN(decos);
    Py_ssize_t deco_location = cur_tmp(c);
    int firstlineno;
    if (decos_num) {
        firstlineno = asdl_seq_GET(decos, 0)->lineno;
        for (int i = 0; i < decos_num; i++) {
            CHECK(compiler_visit_expr_as_tmp(c, asdl_seq_GET(decos, i)));
        }
    } else {
        firstlineno = s->lineno;
    }

    /* compile the class body into a code object */
    CHECK(compiler_enter_scope(c, s->v.ClassDef.name, COMPILER_SCOPE_CLASS, s, firstlineno));
    /* use the class name for name mangling */
    c->u->u_private = s->v.ClassDef.name;

    /* load (global) __name__ ... */
    static PyObject *__name__;
    CHECK(intern_static_identifier(&__name__, "__name__"));
    CHECK(compiler_access_var_name(c, __name__, Load));
    /* ... and store it as __module__ */
    static PyObject *__module__;
    CHECK(intern_static_identifier(&__module__, "__module__"));
    CHECK(compiler_bind_to_var_name(c, __module__));

    CHECK(compiler_load_const(c, c->u->u_qualname));
    static PyObject *__qualname__;
    CHECK(intern_static_identifier(&__qualname__, "__qualname__"));
    CHECK(compiler_bind_to_var_name(c, __qualname__));

    /* compile the body proper */
    CHECK(compiler_add_docstring(c, s->v.ClassDef.body, true));
    CHECK(compiler_visit_stmts(c, s->v.ClassDef.body));

    /* The following code is artificial */
    c->u->u_lineno = -1;
    /* Return __classcell__ if it is referenced, otherwise return None */
    if (c->u->u_ste->ste_needs_class_closure) {
        /* Store __classcell__ into class namespace and return it */
        CHECK(addop(c, GET_CLOSURES, imm_oparg(1), imm_oparg(0), RELOC_OPARG));
        static PyObject *__classcell__;
        CHECK(intern_static_identifier(&__classcell__, "__classcell__"));
        CHECK(compiler_bind_to_var_name(c, __classcell__));
    } else {
        /* No methods referenced __class__, so just return None */
        CHECK(compiler_load_const(c, Py_None));
    }
    CHECK(addop_exit(c, RETURN_VALUE, *cur_out(c), UNSED_OPARG, UNSED_OPARG, NULL));
    /* create the code object */
    PyCodeObject *co;
    CHECK(assemble(c, &co));
    /* leave the new scope */
    compiler_exit_scope(c);

    /* SETUP_CLASS has 2 inputs operands:
         the code object of the class body
         the closures tuple
       and it will generate 3 objects:
         <__build_class__> is loaded from builtin namespace
         <func> is built from the code object along with closures
         <name> is qualified and retrieved from the code object
       Then, *<bases>, **<keywords> arguments will be loaded in sequence.
       A function will be called to build the class:
         <output> = <__build_class__>(<func>, <name>, *<bases>, **<keywords>)
       Finally, decorated <output> is saved to the class name.
    */
    Oparg arg_output = new_tmp_arg_n(c, 3);

    if (!compiler_get_closures(c, co, true)) {
        Py_DECREF(co);
        return 0;
    }
    Oparg arg_closures = alloc_arg_reuse(c, &arg_output);
    CHECK(compiler_load_const_new(c, (PyObject *) co));
    Oparg arg_co = *cur_out(c);
    CHECK(addop(c, SETUP_CLASS, arg_closures, arg_co, arg_output));

    CHECK(compiler_call_helper(c, false, 2, s->v.ClassDef.bases, s->v.ClassDef.keywords));
    CHECK(compiler_apply_decorators(c, deco_location, decos_num));
    set_cur_tmp(c, deco_location);
    return compiler_bind_to_var_name(c, s->v.ClassDef.name);
}

static int
compiler_return(struct compiler *c, stmt_ty s) {
    if (c->u->u_ste->ste_type != FunctionBlock) {
        return compiler_error(c, "'return' outside function");
    }
    if (s->v.Return.value && c->u->u_ste->ste_coroutine && c->u->u_ste->ste_generator) {
        return compiler_error(c, "'return' with value in async generator");
    }
    if (s->v.Return.value) {
        CHECK(compiler_visit_expr(c, s->v.Return.value));
    } else {
        CHECK(compiler_load_const(c, Py_None));
    }
    Oparg arg = alloc_arg_auto(c);
    bool return_const = OPARG_TYPE(arg) == OpargConst;

    for (struct fblockinfo *fb = c->u->u_fblocks; fb; fb = fb->fb_prev) {
        if (fb->fb_type == FB_FINALLY) {
            /* For constants, no need to backup the return value */
            if (!return_const) {
                /* there are 6 registers guaranteed to be available */
                Oparg valid_tmp_space = ((const Oparg *) fb->fb_datum)[0];
                CHECK(addop(c, MOVE_FAST, arg, UNSED_OPARG, valid_tmp_space));
                arg = valid_tmp_space;
            }
            basicblock *b_next = compiler_new_block(c);
            CHECK(b_next);
            CHECK(addop_jump(c, CALL_FINALLY, UNSED_OPARG, UNSED_OPARG, b_next, b_next));
        } else if (fb->fb_type == FB_EXCEPT) {
            CHECK(addop(c, REVOKE_EXCEPT, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));
        }
    }
    /* A new block is used for removing the dead code after return */
    basicblock *b_dead = compiler_new_block(c);
    CHECK(b_dead);
    /* treat the dead block as a exit block */
    b_dead->b_exit = b_dead->b_nofallthrough = true;
    CHECK(addop_exit(c, RETURN_VALUE, arg, UNSED_OPARG, UNSED_OPARG, b_dead));
    return 1;
}

static int
compiler_assign(struct compiler *c, stmt_ty s) {
    expr_ty value = s->v.Assign.value;
    asdl_expr_seq *targets = s->v.Assign.targets;
    Py_ssize_t ntargets = asdl_seq_LEN(targets);

    asdl_expr_seq *velts = value->kind == Tuple_kind ? value->v.Tuple.elts :
                           (value->kind == List_kind ? value->v.List.elts : NULL);
    Py_ssize_t vlen = asdl_seq_LEN(velts);
    if (vlen && vlen < TMP_USE_GUIDELINE && find_unpack_star(velts) == vlen) {
        bool quick_unpack = true;
        for (Py_ssize_t t = 0; t < ntargets; t++) {
            expr_ty target = asdl_seq_GET(targets, t);
            asdl_expr_seq *telts = target->kind == Tuple_kind ? target->v.Tuple.elts :
                                   (target->kind == List_kind ? target->v.List.elts : NULL);
            if (asdl_seq_LEN(telts) != vlen || find_unpack_star(telts) != vlen) {
                quick_unpack = false;
                break;
            }
        }
        /* assignment without unnecessary packing and unpacking */
        if (quick_unpack) {
            Py_ssize_t unpack_at = cur_tmp(c);
            for (Py_ssize_t i = 0; i < vlen; i++) {
                CHECK(compiler_visit_expr_as_tmp(c, asdl_seq_GET(velts, i)));
            }
            for (Py_ssize_t t = 0; t < ntargets; t++) {
                expr_ty target = asdl_seq_GET(targets, t);
                asdl_expr_seq *telts = target->kind == Tuple_kind ? target->v.Tuple.elts :
                                       (target->kind == List_kind ? target->v.List.elts : NULL);
                for (Py_ssize_t i = 0; i < vlen; i++) {
                    Oparg v = tmp_oparg(unpack_at + i);
                    set_cur_out(c, &v);
                    CHECK(compiler_bind_to_target(c, asdl_seq_GET(telts, i)));
                }
            }
            return 1;
        }
    }

    /* do normal assignment */
    CHECK(compiler_visit_expr(c, value));
    Oparg *value_arg = cur_out(c);
    CHECK(compiler_bind_to_target(c, asdl_seq_GET(targets, 0)));
    if (ntargets > 1) {
        if (OPARG_TYPE(*value_arg) == OpargTmp) {
            /* preserve the tmp register */
            new_tmp_arg(c);
        }
        for (Py_ssize_t i = 1; i < ntargets; i++) {
            CHECK(compiler_bind_to_target(c, asdl_seq_GET(targets, i)));
        }
    }
    return 1;
}

static int
compiler_augassign(struct compiler *c, stmt_ty s) {
    expr_ty target = s->v.AugAssign.target;
    SET_LOC(c, target);

    Oparg obj = RELOC_OPARG, field = RELOC_OPARG;
    Oparg left;

    switch (target->kind) {
        case Attribute_kind:
            CHECK(compiler_visit_expr(c, target->v.Attribute.value));
            obj = alloc_arg_auto(c);
            CHECK(compiler_load_name(c, target->v.Attribute.attr, &field));
            left = new_tmp_arg(c);
            CHECK(addop_line(c, LOAD_ATTR, obj, field, left, target->end_lineno));
            break;
        case Subscript_kind:
            CHECK(compiler_visit_expr(c, target->v.Subscript.value));
            obj = alloc_arg_auto(c);
            CHECK(compiler_visit_expr(c, target->v.Subscript.slice));
            field = alloc_arg_auto(c);
            left = new_tmp_arg(c);
            CHECK(addop(c, LOAD_SUBSCR, obj, field, left));
            break;
        case Name_kind:
            CHECK(compiler_access_var_name(c, target->v.Name.id, Load));
            left = alloc_arg_auto(c);
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "invalid node type (%d) for augmented assignment",
                         target->kind);
            return 0;
    }

    SET_LOC(c, s);
    CHECK(compiler_visit_expr(c, s->v.AugAssign.value));
    Oparg right = alloc_arg_auto(c);
    CHECK(addop(c, get_binary_opcode(s->v.AugAssign.op, true), left, right, UNSED_OPARG));
    SET_LOC(c, target);

    switch (target->kind) {
        case Attribute_kind:
            return addop_line(c, STORE_ATTR, obj, field, left, target->end_lineno);
        case Subscript_kind:
            return addop(c, STORE_SUBSCR, obj, field, left);
        case Name_kind:
            if (OPARG_TYPE(left) == OpargTmp) {
                set_cur_out(c, &left);
                return compiler_bind_to_var_name(c, target->v.Name.id);
            }
            return 1;
        default:
            Py_UNREACHABLE();
    }
}

static int
compiler_annassign(struct compiler *c, stmt_ty s) {
    expr_ty target = s->v.AnnAssign.target;

    /* We perform the actual assignment first. */
    if (s->v.AnnAssign.value) {
        CHECK(compiler_visit_expr(c, s->v.AnnAssign.value));
        CHECK(compiler_bind_to_target(c, target));
    }
    switch (target->kind) {
        case Name_kind: {
            CHECK(verify_var_name(c, target->v.Name.id, Store));
            /* If we have a simple name in a module or class, store annotation. */
            if (s->v.AnnAssign.simple && c->u->u_annotations) {
                c->u->u_annotations = 2;
                CHECK(compiler_visit_ann_expr(c, s->v.AnnAssign.annotation));
                Oparg annotation = alloc_arg_auto(c);

                static PyObject *__annotations__;
                CHECK(intern_static_identifier(&__annotations__, "__annotations__"));
                CHECK(compiler_access_var_name(c, __annotations__, Load));
                Oparg obj = alloc_arg_auto(c);

                PyObject *mangled = _Py_Mangle(c->u->u_private, target->v.Name.id);
                CHECK(mangled);
                CHECK(compiler_load_const_new(c, mangled));
                return addop(c, STORE_SUBSCR, obj, *cur_out(c), annotation);
            }
            return 1;
        }
        case Attribute_kind:
            CHECK(verify_var_name(c, target->v.Attribute.attr, Store));
            if (!s->v.AnnAssign.value) {
                CHECK(check_ann_expr(c, target->v.Attribute.value));
            }
            break;
        case Subscript_kind:
            if (!s->v.AnnAssign.value) {
                CHECK(check_ann_expr(c, target->v.Subscript.value));
                CHECK(check_ann_subscr(c, target->v.Subscript.slice));
            }
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "invalid node type (%d) for annotated assignment",
                         target->kind);
            return 0;
    }

    assert(!s->v.AnnAssign.simple);
    /* Annotations of complex targets does not produce anything under annotations future
       Annotations are only evaluated in a module or class. */
    return c->c_future->ff_features & CO_FUTURE_ANNOTATIONS || \
        !c->u->u_annotations || \
        check_ann_expr(c, s->v.AnnAssign.annotation);
}

static int
compiler_for(struct compiler *c, stmt_ty s) {
    basicblock *b_iter = compiler_new_block(c);
    basicblock *b_body = compiler_new_block(c);
    basicblock *b_else = compiler_new_block(c);
    basicblock *b_end = s->v.While.orelse ? compiler_new_block(c) : b_else;
    CHECK(b_iter && b_body && b_else && b_end);

    CHECK(compiler_visit_expr(c, s->v.For.iter));
    Oparg iterable = new_tmp_arg(c);
    CHECK(addop(c, GET_ITER, alloc_arg_reuse(c, &iterable), UNSED_OPARG, iterable));
    compiler_use_block(c, b_iter);
    CHECK(addop_jump(c, FOR_ITER, iterable, RELOC_OPARG, b_else, b_body));
    adjust_cur_out(c, 2);
    CHECK(compiler_bind_to_target(c, s->v.For.target));

    struct fblockinfo fb = {.fb_type = FB_LOOP, .fb_datum=(basicblock *[2]) {b_iter, b_end}};
    CHECK(compiler_push_fblock(c, &fb));
    CHECK(compiler_visit_stmts(c, s->v.For.body));
    compiler_pop_fblock(c);

    /* Mark jump as artificial */
    CHECK(addop_jump_noline(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, b_iter, b_else));

    if (b_else != b_end) {
        CHECK(compiler_visit_stmts(c, s->v.For.orelse));
        compiler_use_block(c, b_end);
    }
    return 1;
}

static int
compiler_async_for(struct compiler *c, stmt_ty s) {
    if (is_top_level_await(c)) {
        c->u->u_ste->ste_coroutine = 1;
    } else if (c->u->u_scope_type != COMPILER_SCOPE_ASYNC_FUNCTION) {
        return compiler_error(c, "'async for' outside async function");
    }
    basicblock *b_iter = compiler_new_block(c);
    basicblock *b_except = compiler_new_block(c);
    basicblock *b_end = compiler_new_block(c);
    CHECK(b_iter && b_except && b_end);

    CHECK(compiler_visit_expr(c, s->v.AsyncFor.iter));
    Oparg iterable = new_tmp_arg(c);
    CHECK(addop(c, GET_ASYNC_ITER, alloc_arg_reuse(c, &iterable), UNSED_OPARG, iterable));

    Oparg value = new_tmp_arg_n(c, 2);
    compiler_use_block(c, b_iter);
    /* START_TRY to guard the __anext__ call */
    CHECK(addop_jump(c, START_TRY, iterable, UNSED_OPARG, b_except, NULL));
    struct fblockinfo fb_finally = {.fb_type = FB_FINALLY, .fb_datum=&iterable};
    CHECK(compiler_push_fblock(c, &fb_finally));
    CHECK(addop(c, GET_ASYNC_NEXT, iterable, UNSED_OPARG, value));
    CHECK(addop(c, YIELD_FROM, value, UNSED_OPARG, RELOC_OPARG));
    Oparg *async_result = cur_out(c);
    CHECK(addop(c, REVOKE_EXCEPT, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));
    set_cur_out(c, async_result);
    compiler_pop_fblock(c);
    free_cur_tmp(c, 2);

    /* Success block for __anext__ */
    CHECK(compiler_bind_to_target(c, s->v.AsyncFor.target));

    struct fblockinfo fb_loop = {.fb_type = FB_LOOP, .fb_datum=(basicblock *[2]) {b_iter, b_end}};
    CHECK(compiler_push_fblock(c, &fb_loop));
    CHECK(compiler_visit_stmts(c, s->v.AsyncFor.body));
    compiler_pop_fblock(c);
    CHECK(addop_jump(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, b_iter, b_except));

    /* Use same line number as the iterator,
     * as the END_ASYNC_FOR succeeds the `for`, not the body. */
    compiler_use_block(c, b_except);
    SET_LOC(c, s->v.AsyncFor.iter);
    /* 6 registers are needed, but *iterable* is reused for sextuple). */
    new_tmp_arg_n(c, 6 - 1);
    CHECK(addop(c, END_ASYNC_FOR, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));

    /* `else` block */
    CHECK(compiler_visit_stmts(c, s->v.For.orelse));
    compiler_use_block(c, b_end);
    return 1;
}

static int
compiler_while(struct compiler *c, stmt_ty s) {
    basicblock *b_test = compiler_new_block(c);
    basicblock *b_else = compiler_new_block(c);
    basicblock *b_end = s->v.While.orelse ? compiler_new_block(c) : b_else;
    CHECK(b_test && b_else && b_end);

    compiler_use_block(c, b_test);
    CHECK(compiler_logical_expr(c, s->v.While.test, b_else, false));
    basicblock *b_body = compiler_current_block(c);

    struct fblockinfo fb = {.fb_type = FB_LOOP, .fb_datum=(basicblock *[2]) {b_test, b_end}};
    CHECK(compiler_push_fblock(c, &fb));
    CHECK(compiler_visit_stmts(c, s->v.While.body));
    compiler_pop_fblock(c);

    SET_LOC(c, s);
    CHECK(compiler_logical_expr(c, s->v.While.test, b_body, true));
    compiler_use_block(c, b_else);

    if (b_else != b_end) {
        CHECK(compiler_visit_stmts(c, s->v.While.orelse));
        compiler_use_block(c, b_end);
    }
    return 1;
}

static int
compiler_continue_break(struct compiler *c, bool is_break) {
    for (struct fblockinfo *fb = c->u->u_fblocks; fb; fb = fb->fb_prev) {
        if (fb->fb_type == FB_FINALLY) {
            basicblock *b_next = compiler_new_block(c);
            CHECK(b_next);
            CHECK(addop_jump(c, CALL_FINALLY, UNSED_OPARG, UNSED_OPARG, b_next, b_next));
        } else if (fb->fb_type == FB_EXCEPT) {
            CHECK(addop(c, REVOKE_EXCEPT, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));
        } else if (fb->fb_type == FB_LOOP) {
            basicblock *to_block = ((basicblock *const *) fb->fb_datum)[is_break];
            return addop_jump(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, to_block, NULL);
        }
    }
    return compiler_error(c, "'%s' outside loop", is_break ? "break" : "continue");
}

static int
compiler_if(struct compiler *c, stmt_ty s) {
    basicblock *end = compiler_new_block(c);
    CHECK(end);
    basicblock *branch = asdl_seq_LEN(s->v.If.orelse) ? compiler_new_block(c) : end;
    CHECK(branch);
    CHECK(compiler_logical_expr(c, s->v.If.test, branch, false));
    CHECK(compiler_visit_stmts(c, s->v.If.body));
    if (branch != end) {
        CHECK(addop_jump_noline(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, end, branch));
        CHECK(compiler_visit_stmts(c, s->v.If.orelse));
    }
    compiler_use_block(c, end);
    return 1;
}

static int
compiler_import(struct compiler *c, stmt_ty s) {
    /* The Import node stores a module name like a.b.c as a single
       string.  This is convenient for all cases except
         import a.b.c as d
       where we need to parse that string to extract the individual
       module names.
       XXX Perhaps change the representation to make this case simpler?
     */
    Py_ssize_t n = asdl_seq_LEN(s->v.Import.names);

    for (Py_ssize_t i = 0; i < n; i++) {
        alias_ty alias = asdl_seq_GET(s->v.Import.names, i);

        Oparg name_arg;
        CHECK(compiler_load_name(c, alias->name, &name_arg));
        CHECK(compiler_load_const(c, Py_None));
        CHECK(addop(c, IMPORT_NAME, name_arg, *cur_out(c), RELOC_OPARG));

        Py_ssize_t len = PyUnicode_GET_LENGTH(alias->name);
        Py_ssize_t dot = PyUnicode_FindChar(alias->name, '.', 0, len, 1);
        CHECK(dot != -2);

        if (dot == -1) {
            CHECK(compiler_bind_to_var_name(c, alias->asname ? alias->asname : alias->name));
            continue;
        }

        if (!alias->asname) {
            identifier topname = PyUnicode_Substring(alias->name, 0, dot);
            CHECK(topname);
            int r = compiler_bind_to_var_name(c, topname);
            Py_DECREF(topname);
            CHECK(r);
            continue;
        }

        Oparg arg = alloc_arg_auto(c);
        while (dot != -1) {
            Py_ssize_t dot2 = PyUnicode_FindChar(alias->name, '.', ++dot, len, 1);
            CHECK(dot2 != -2);
            PyObject *attr = PyUnicode_Substring(alias->name, dot, dot2 == -1 ? len : dot2);
            CHECK(attr);
            Oparg attr_arg;
            CHECK(compiler_load_name(c, attr, &attr_arg));
            Py_DECREF(attr);
            CHECK(addop(c, IMPORT_FROM, arg, attr_arg, dot2 == -1 ? RELOC_OPARG : arg));
            dot = dot2;
        }
        CHECK(compiler_bind_to_var_name(c, alias->asname));
    }
    return 1;
}

static int
compiler_from_import(struct compiler *c, stmt_ty s) {
    if (s->lineno > c->c_future->ff_lineno && s->v.ImportFrom.module &&
        _PyUnicode_EqualToASCIIString(s->v.ImportFrom.module, "__future__")) {
        return compiler_error(c, "from __future__ imports must occur "
                                 "at the beginning of the file");
    }

    Oparg module_arg;
    int level = s->v.ImportFrom.level;
    if (level) {
        PyObject *mod = s->v.ImportFrom.module;
        size_t mod_len;
        Py_UCS4 maxchar;
        if (mod) {
            maxchar = PyUnicode_MAX_CHAR_VALUE(mod);
            maxchar = maxchar > '.' ? maxchar : '.';
            mod_len = PyUnicode_GET_LENGTH(mod);
        } else {
            mod_len = 0;
            maxchar = '.';
        }
        PyObject *dotted = PyUnicode_New(level + mod_len, maxchar);
        CHECK(dotted);
        for (int i = 0; i < level; i++) {
            PyUnicode_WRITE(PyUnicode_KIND(dotted), PyUnicode_DATA(dotted), i, '.');
        }
        if (mod_len && PyUnicode_CopyCharacters(dotted, level, mod, 0, mod_len) < 0) {
            Py_DECREF(dotted);
            return 0;
        }
        int res = compiler_load_name(c, dotted, &module_arg);
        Py_DECREF(dotted);
        CHECK(res);
    } else {
        CHECK(compiler_load_name(c, s->v.ImportFrom.module, &module_arg));
    }

    /* build up the names */
    Py_ssize_t name = asdl_seq_LEN(s->v.ImportFrom.names);
    PyObject *names = PyTuple_New(name);
    CHECK(names);
    for (Py_ssize_t i = 0; i < name; i++) {
        alias_ty alias = asdl_seq_GET(s->v.ImportFrom.names, i);
        Py_INCREF(alias->name);
        PyTuple_SET_ITEM(names, i, alias->name);
    }
    CHECK(compiler_load_const(c, names));
    CHECK(addop(c, IMPORT_NAME, module_arg, *cur_out(c), RELOC_OPARG));
    Oparg mod_out_arg = alloc_arg_auto(c);

    if (PyUnicode_READ_CHAR(asdl_seq_GET(s->v.ImportFrom.names, 0)->name, 0) == '*') {
        return addop(c, IMPORT_STAR, mod_out_arg, UNSED_OPARG, UNSED_OPARG);
    }

    for (Py_ssize_t i = 0; i < name; i++) {
        alias_ty alias = asdl_seq_GET(s->v.ImportFrom.names, i);
        Oparg attr_arg;
        CHECK(compiler_load_name(c, alias->name, &attr_arg));
        CHECK(addop(c, IMPORT_FROM, mod_out_arg, attr_arg, RELOC_OPARG));
        CHECK(compiler_bind_to_var_name(c, alias->asname ? alias->asname : alias->name));
    }
    return 1;
}

static int
compiler_raise_exception(struct compiler *c, stmt_ty s) {
    bool is_assert = s->kind == Assert_kind;
    int flag = is_assert;
    expr_ty exc_expr;
    expr_ty cause_expr;
    basicblock *b_end = compiler_new_block(c);
    CHECK(b_end);
    if (is_assert) {
        /* Always emit a warning if the test is a non-zero length tuple */
        expr_ty test = s->v.Assert.test;
        bool warn = test->kind == Tuple_kind && asdl_seq_LEN(test->v.Tuple.elts);
        if (!warn && test->kind == Constant_kind) {
            PyObject *value = test->v.Constant.value;
            warn = PyTuple_Check(value) && PyTuple_Size(value);
        }
        CHECK(!warn || compiler_warn(c, "assertion is always true, perhaps remove parentheses?"));

        if (c->c_optimize) {
            return 1;
        }

        CHECK(b_end);
        CHECK(compiler_logical_expr(c, s->v.Assert.test, b_end, true));
        exc_expr = s->v.Assert.msg;
        cause_expr = NULL;
    } else {
        exc_expr = s->v.Raise.exc;
        cause_expr = s->v.Raise.cause;
    }
    Oparg exc = cur_tmp_arg(c);
    Oparg cause = exc;
    if (exc_expr) {
        CHECK(compiler_visit_expr(c, exc_expr));
        exc = alloc_arg_auto(c);
        flag |= 0b010;
    }
    if (cause_expr) {
        assert(s->v.Raise.exc);
        CHECK(compiler_visit_expr(c, cause_expr));
        cause = alloc_arg_auto(c);
        flag |= 0b100;
    }
    CHECK(addop_exit(c, RAISE_EXCEPTION, imm_oparg(flag), exc, cause, b_end));
    return 1;
}

static int
compiler_try_finally(struct compiler *c, stmt_ty s) {
    basicblock *b_finally = compiler_new_block(c);
    basicblock *b_end = compiler_new_block(c);
    CHECK(b_finally && b_end);

    Oparg sextuple = cur_tmp_arg(c);
    CHECK(addop_jump(c, START_TRY, sextuple, UNSED_OPARG, b_finally, NULL));

    struct fblockinfo fb = {.fb_type = FB_FINALLY, .fb_datum=&sextuple};
    CHECK(compiler_push_fblock(c, &fb));
    if (asdl_seq_LEN(s->v.Try.handlers)) {
        CHECK(compiler_try_except(c, s));
    } else {
        CHECK(compiler_visit_stmts(c, s->v.Try.body));
    }
    compiler_pop_fblock(c);

    CHECK(addop_jump_noline(c, CALL_FINALLY, UNSED_OPARG, UNSED_OPARG, b_end, b_finally));

    new_tmp_arg_n(c, 6);
    /* The finally body may call return/continue/break,
     * we need to push a FB_EXCEPT fbblock here to ensure that
     * such statements call REVOKE_FINALLY correctly. */
    fb.fb_type = FB_EXCEPT;
    CHECK(compiler_push_fblock(c, &fb));
    CHECK(compiler_visit_stmts(c, s->v.Try.finalbody));
    compiler_pop_fblock(c);
    CHECK(addop_exit_noline(c, END_FINALLY, imm_oparg(0), UNSED_OPARG, UNSED_OPARG, b_end));
    return 1;
}

static int
compiler_try_except(struct compiler *c, stmt_ty s) {
    basicblock *b_except = compiler_new_block(c);
    basicblock *b_else = compiler_new_block(c);
    basicblock *b_end = compiler_new_block(c);
    CHECK(b_except && b_else && b_end);

    Oparg sextuple = cur_tmp_arg(c);
    CHECK(addop_jump(c, START_TRY, sextuple, UNSED_OPARG, b_except, NULL));

    struct fblockinfo fb = {.fb_type = FB_EXCEPT};
    CHECK(compiler_push_fblock(c, &fb));
    CHECK(compiler_visit_stmts(c, s->v.Try.body));
    CHECK(addop_noline(c, REVOKE_EXCEPT, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));
    CHECK(addop_jump_noline(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, b_else, b_except));
    new_tmp_arg_n(c, 6);

    Py_ssize_t n = asdl_seq_LEN(s->v.Try.handlers);
    for (Py_ssize_t i = 0; i < n; i++) {
        excepthandler_ty handler = asdl_seq_GET(s->v.Try.handlers, i);
        SET_LOC(c, handler);
        if (!handler->v.ExceptHandler.type && i < n - 1) {
            return compiler_error(c, "default 'except:' must be last");
        }
        CHECK(b_except = compiler_new_block(c));
        if (handler->v.ExceptHandler.type) {
            Oparg raised_exc = oparg_by_offset(sextuple, 3);
            CHECK(compiler_visit_expr(c, handler->v.ExceptHandler.type));
            Py_ssize_t old_tmp = cur_tmp(c);
            Oparg to_match = alloc_arg_auto(c);
            set_cur_tmp(c, old_tmp);
            CHECK(addop_jump(c, JUMP_IF_NOT_EXC_MATCH, raised_exc, to_match, b_except, NULL));
        }
        if (handler->v.ExceptHandler.name) {
            basicblock *eb_finally = compiler_new_block(c);
            basicblock *eb_end = compiler_new_block(c);
            CHECK(eb_finally && eb_end);

            Oparg raised_value = oparg_by_offset(sextuple, 4);
            set_cur_out(c, &raised_value);
            CHECK(compiler_bind_to_var_name(c, handler->v.ExceptHandler.name));

            Oparg inner_sextuple = cur_tmp_arg(c);
            CHECK(addop_jump(c, START_TRY, inner_sextuple, UNSED_OPARG, eb_finally, NULL));
            struct fblockinfo inner_fb = {.fb_type = FB_FINALLY, .fb_datum=&inner_sextuple};
            CHECK(compiler_push_fblock(c, &inner_fb));
            CHECK(compiler_visit_stmts(c, handler->v.ExceptHandler.body));
            compiler_pop_fblock(c);

            c->u->u_lineno = -1;
            CHECK(addop_jump(c, CALL_FINALLY, UNSED_OPARG, UNSED_OPARG, eb_end, eb_finally));

            new_tmp_arg_n(c, 6);
            CHECK(compiler_access_var_name(c, handler->v.ExceptHandler.name, Del));
            adjust_cur_out(c, 2);
            /* allow undefined names when delete */
            *cur_out(c) = imm_oparg(1);
            CHECK(addop_exit(c, END_FINALLY, imm_oparg(1), UNSED_OPARG, UNSED_OPARG, eb_end));
            free_cur_tmp(c, 6);
        } else {
            CHECK(compiler_visit_stmts(c, handler->v.ExceptHandler.body));
        }
        CHECK(addop_noline(c, REVOKE_EXCEPT, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));
        CHECK(addop_jump_noline(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, b_end, b_except));
    }
    compiler_pop_fblock(c);
    CHECK(addop_exit_noline(c, END_FINALLY, imm_oparg(0), UNSED_OPARG, UNSED_OPARG, b_else));
    free_cur_tmp(c, 6);

    CHECK(compiler_visit_stmts(c, s->v.Try.orelse));
    compiler_use_block(c, b_end);
    return 1;
}

static int
compiler_try(struct compiler *c, stmt_ty s) {
    if (asdl_seq_LEN(s->v.Try.finalbody)) {
        return compiler_try_finally(c, s);
    } else {
        return compiler_try_except(c, s);
    }
}

static int
compiler_with(struct compiler *c, stmt_ty s, int pos) {
    basicblock *b_finally = compiler_new_block(c);
    CHECK(b_finally);

    /* Evaluate the expression */
    withitem_ty item = asdl_seq_GET(s->v.With.items, pos);
    CHECK(compiler_visit_expr(c, item->context_expr));
    /* the six exception slots must follow the saved __exit__ method */
    Oparg saved_exit = new_tmp_arg(c);
    Oparg sextuple = cur_tmp_arg(c);
    CHECK(alloc_arg_bind(c, saved_exit));

    /* Create a new finally block with address at oparg3 and sextuple at [oparg1+1].
       Call oparg1.__start__() and save the result to oparg2.
       Store oparg1.__exit__ to [oparg1]. */
    CHECK(addop_jump(c, ENTER_WITH, saved_exit, RELOC_OPARG, b_finally, NULL));
    adjust_cur_out(c, 2);

    struct fblockinfo fb = {.fb_type = FB_FINALLY, .fb_datum=&sextuple};
    CHECK(compiler_push_fblock(c, &fb));
    if (item->optional_vars) {
        CHECK(compiler_bind_to_target(c, item->optional_vars));
    } else {
        /* the result is overridden by __exit__ since unused */
        CHECK(alloc_arg_bind(c, saved_exit));
    }
    /* compile the body or do a recursive call */
    if (++pos == asdl_seq_LEN(s->v.With.items)) {
        CHECK(compiler_visit_stmts(c, s->v.With.body));
    } else {
        CHECK(compiler_with(c, s, pos));
    }
    compiler_pop_fblock(c);

    new_tmp_arg_n(c, 6);
    compiler_use_block(c, b_finally);
    c->u->u_lineno = s->lineno;
    CHECK(addop(c, EXIT_WITH, saved_exit, UNSED_OPARG, UNSED_OPARG));
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
compiler_async_with(struct compiler *c, stmt_ty s, int pos) {
    if (is_top_level_await(c)) {
        c->u->u_ste->ste_coroutine = 1;
    } else if (c->u->u_scope_type != COMPILER_SCOPE_ASYNC_FUNCTION) {
        return compiler_error(c, "'async with' outside async function");
    }
    basicblock *b_finally = compiler_new_block(c);
    basicblock *b_exit_true = compiler_new_block(c);
    basicblock *b_end = compiler_new_block(c);
    CHECK(b_finally && b_exit_true && b_end);

    withitem_ty item = asdl_seq_GET(s->v.AsyncWith.items, pos);
    CHECK(compiler_visit_expr(c, item->context_expr));
    Oparg saved_exit = new_tmp_arg(c);
    Oparg saved_enter = new_tmp_arg_n(c, 2);
    Oparg context = alloc_arg_reuse(c, &saved_exit);
    CHECK(addop(c, ENTER_ASYNC_WITH, context, UNSED_OPARG, saved_exit));
    CHECK(addop(c, GET_AWAITABLE, saved_enter, UNSED_OPARG, saved_enter));
    CHECK(addop(c, YIELD_FROM, saved_enter, UNSED_OPARG, RELOC_OPARG));
    Oparg *async_result = cur_out(c);
    free_cur_tmp(c, 2);
    Oparg sextuple = oparg_by_offset(saved_exit, 2);
    CHECK(addop_jump(c, START_TRY, sextuple, UNSED_OPARG, b_finally, NULL));

    struct fblockinfo fb = {.fb_type = FB_FINALLY, .fb_datum=&sextuple};
    CHECK(compiler_push_fblock(c, &fb));
    if (item->optional_vars) {
        set_cur_out(c, async_result);
        CHECK(compiler_bind_to_target(c, item->optional_vars));
    } else {
        *async_result = saved_enter;
    }
    /* compile the body or do a recursive call */
    if (++pos == asdl_seq_LEN(s->v.AsyncWith.items)) {
        CHECK(compiler_visit_stmts(c, s->v.AsyncWith.body));
    } else {
        CHECK(compiler_async_with(c, s, pos));
    }
    CHECK(addop_jump(c, CALL_FINALLY, UNSED_OPARG, UNSED_OPARG, b_end, b_finally));
    compiler_pop_fblock(c);

    new_tmp_arg_n(c, 1 + 6);
    c->u->u_lineno = s->lineno;

    CHECK(addop(c, EXIT_ASYNC_WITH, saved_exit, UNSED_OPARG, UNSED_OPARG));
    CHECK(addop(c, GET_AWAITABLE, saved_exit, UNSED_OPARG, saved_exit));
    CHECK(addop(c, YIELD_FROM, saved_exit, UNSED_OPARG, saved_exit));
    CHECK(addop_jump(c, JUMP_IF_TRUE, saved_exit, NEG_ADDR_OPARG, b_exit_true, NULL));
    CHECK(addop_exit(c, END_FINALLY, imm_oparg(1), UNSED_OPARG, UNSED_OPARG, b_exit_true));
    CHECK(addop(c, REVOKE_EXCEPT, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG));
    compiler_use_block(c, b_end);
    return 1;
}

/* PEP 634: Structural Pattern Matching */

static inline bool
is_wildcard_capture(pattern_ty pattern) {
    return pattern->kind == MatchAs_kind && !pattern->v.MatchAs.name;
}

static int
pattern_helper_store_to_tmps(struct compiler *c, identifier name, pattern_context *pc) {
    if (!name) {
        return addop(c, NOP, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG);
    }
    Py_ssize_t offset = PySequence_Index(pc->stores, name);
    /* because we have already verified the legality of name capturing. */
    assert(offset >= 0);
    Oparg tmp = oparg_by_offset(pc->tmps, offset);
    return alloc_arg_bind(c, tmp);
}

static int
pattern_helper_store_from_tmps(struct compiler *c, pattern_context *pc) {
    Py_ssize_t nstores = PyTuple_GET_SIZE(pc->stores);
    for (Py_ssize_t i = 0; i < nstores; ++i) {
        Oparg tmp = oparg_by_offset(pc->tmps, i);
        PyObject *name = PyTuple_GET_ITEM(pc->stores, i);
        set_cur_out(c, &tmp);
        CHECK(compiler_bind_to_var_name(c, name));
    }
    return 1;
}

static int
compiler_pattern_subpattern(struct compiler *c, pattern_ty p, Oparg new_subject, pattern_context *pc) {
    pattern_context new_pc = {
            .subject = new_subject,
            .stores = pc->stores,
            .tmps = pc->tmps,
            .fallback = pc->fallback,
            .allow_capture = 1
    };
    CHECK(compiler_pattern(c, p, &new_pc));
    return 1;
}

static int
compiler_pattern_as(struct compiler *c, pattern_ty p, identifier name, pattern_context *pc) {
    set_cur_out(c, &pc->subject);
    CHECK(pattern_helper_store_to_tmps(c, name, pc));
    if (p) {
        CHECK(compiler_pattern(c, p, pc));
    } else if (!pc->allow_capture) {
        const char *e = "makes remaining patterns unreachable";
        return name ?
               compiler_error(c, "name capture %R %s", name, e) :
               compiler_error(c, "wildcard %s", e);
    }
    return 1;
}

static int
compiler_pattern_mapping(struct compiler *c, pattern_ty p, pattern_context *pc) {
    assert(p->kind == MatchMapping_kind);
    asdl_expr_seq *keys = p->v.MatchMapping.keys;
    asdl_pattern_seq *patterns = p->v.MatchMapping.patterns;

    Py_ssize_t size = asdl_seq_LEN(keys);
    if (size != asdl_seq_LEN(patterns)) {
        /* In fact, AST validator shouldn't let this happen */
        return compiler_error(c,
                              "keys (%z) / patterns (%z) length mismatch in mapping pattern",
                              size, asdl_seq_LEN(patterns));
    }

    /* Duplicated const keys should raise a compiler error */
    if (size) {
        PyObject *appeared_keys = PySet_New(NULL);
        for (Py_ssize_t i = 0; i < size; i++) {
            expr_ty key = asdl_seq_GET(keys, i);
            if (key->kind == Attribute_kind) {
                continue;
            }
            if (key->kind != Constant_kind) {
                /* In fact, AST validator shouldn't let this happen */
                compiler_error(c, "mapping pattern keys may only match literals and attribute lookups");
                Py_DECREF(appeared_keys);
                return 0;
            }
            int duplicated = PySet_Contains(appeared_keys, key->v.Constant.value);
            if (duplicated != 0) {
                if (duplicated > 0) {
                    compiler_error(c, "mapping pattern checks duplicate key (%R)", key->v.Constant.value);
                }
                Py_DECREF(appeared_keys);
                return 0;
            }
            CHECK(PySet_Add(appeared_keys, key->v.Constant.value) == 0);
        }
        Py_DECREF(appeared_keys);
    }

    /* Match the keys and fetch the corresponding values */
    CHECK(sequence_building_helper(c, keys, TUPLE_BUILD, LIST_UPDATE, 0));
    Oparg matching_keys = alloc_arg_auto(c);
    Oparg matched_values = new_tmp_arg(c);
    CHECK(addop(c, MATCH_DICT, matching_keys, pc->subject, matched_values));
    CHECK(addop_jump(c, JUMP_IF_FALSE, matched_values, NEG_ADDR_OPARG, pc->fallback, NULL));

    /* Apply the subpattern for matched values */
    Oparg tmp = new_tmp_arg(c);
    for (Py_ssize_t i = 0; i < size; i++) {
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        if (is_wildcard_capture(pattern)) {
            continue;
        }
        PyObject *i_o = PyLong_FromSsize_t(i);
        CHECK(i_o);
        CHECK(compiler_load_const_new(c, i_o));
        CHECK(addop(c, LOAD_SUBSCR, matched_values, *cur_out(c), tmp));
        CHECK(compiler_pattern_subpattern(c, pattern, tmp, pc));
    }
    free_cur_tmp(c, 1);

    /* For the starred name, bind a dict of remaining items to it */
    if (p->v.MatchMapping.rest) {
        CHECK(addop(c, COPY_DICT_WITHOUT_KEYS, matching_keys, pc->subject, RELOC_OPARG));
        CHECK(pattern_helper_store_to_tmps(c, p->v.MatchMapping.rest, pc));
    }
    return 1;
}

static int
compiler_pattern_class(struct compiler *c, pattern_ty p, pattern_context *pc) {
    assert(p->kind == MatchClass_kind);
    asdl_pattern_seq *patterns = p->v.MatchClass.patterns;
    asdl_identifier_seq *kwd_attrs = p->v.MatchClass.kwd_attrs;
    asdl_pattern_seq *kwd_patterns = p->v.MatchClass.kwd_patterns;
    Py_ssize_t nargs = asdl_seq_LEN(patterns);
    Py_ssize_t nkwargs = asdl_seq_LEN(kwd_attrs);

    if (nkwargs != asdl_seq_LEN(kwd_patterns)) {
        /* In fact, AST validator shouldn't let this happen */
        return compiler_error(c,
                              "kwd_attrs (%z) / kwd_patterns (%z) length mismatch in class pattern",
                              nkwargs, asdl_seq_LEN(kwd_patterns));
    }

    CHECK(compiler_visit_expr_as_tmp(c, p->v.MatchClass.cls));
    Oparg matched = *cur_out(c);

    if (nkwargs) {
        PyObject *attr_names = PyTuple_New(nkwargs);
        CHECK(attr_names);
        /* Any errors should point to the pattern */
        for (Py_ssize_t i = 0; i < nkwargs; i++) {
            identifier attr = asdl_seq_GET(kwd_attrs, i);
            if (!verify_var_name(c, attr, Store)) {
                SET_LOC(c, asdl_seq_GET(kwd_patterns, i));
                Py_DECREF(attr_names);
                return 0;
            }
            for (Py_ssize_t j = i + 1; j < nkwargs; j++) {
                identifier other = asdl_seq_GET(kwd_attrs, j);
                if (!PyUnicode_Compare(attr, other)) {
                    SET_LOC(c, asdl_seq_GET(kwd_patterns, j));
                    Py_DECREF(attr_names);
                    return compiler_error(c, "attribute name repeated in class pattern: %U", attr);
                }
            }
            Py_INCREF(attr);
            PyTuple_SET_ITEM(attr_names, i, attr);
        }
        SET_LOC(c, p);
        CHECK(compiler_load_const_new(c, attr_names));
        CHECK(alloc_arg_tmp(c));
    }

    Oparg flag = imm_oparg(nargs << 1 | (nkwargs > 0));
    CHECK(addop(c, MATCH_CLASS, flag, pc->subject, matched));
    CHECK(addop_jump(c, JUMP_IF_FALSE, matched, NEG_ADDR_OPARG, pc->fallback, NULL));
    free_cur_tmp(c, nkwargs > 0);

    /* Apply the subpattern for matched values */
    Oparg tmp = new_tmp_arg(c);
    for (Py_ssize_t i = 0; i < nargs + nkwargs; i++) {
        pattern_ty pattern = i < nargs ? asdl_seq_GET(patterns, i) : asdl_seq_GET(kwd_patterns, i - nargs);
        PyObject *i_o = PyLong_FromSsize_t(i);
        CHECK(i_o);
        CHECK(compiler_load_const_new(c, i_o));
        CHECK(addop(c, LOAD_SUBSCR, matched, *cur_out(c), tmp));
        CHECK(compiler_pattern_subpattern(c, pattern, tmp, pc));
    }

    return 1;
}

static int
compiler_pattern_or(struct compiler *c, asdl_pattern_seq *patterns, pattern_context *pc) {
    pattern_context new_pc = {
            .subject = pc->subject,
            .tmps = pc->tmps,
            .stores = pc->stores
    };
    Py_ssize_t size = asdl_seq_LEN(patterns);
    assert(size >= 2);
    basicblock *b_end = compiler_new_block(c);
    CHECK(b_end);

    for (Py_ssize_t i = 0;; i++) {
        new_pc.fallback = i == size - 1 ? pc->fallback : compiler_new_block(c);
        CHECK(new_pc.fallback);
        /* Capture is only allowed for the last alternative */
        new_pc.allow_capture = i == size - 1 && pc->allow_capture;

        CHECK(compiler_pattern(c, asdl_seq_GET(patterns, i), &new_pc));
        if (i == size - 1) {
            // Py_DECREF(prev_stores);
            compiler_use_block(c, b_end);
            return 1;
        }
        CHECK(addop_jump(c, JUMP_ALWAYS, UNSED_OPARG, ADDR_OPARG, b_end, NULL));
        compiler_use_block(c, new_pc.fallback);
    }
}

static int
compiler_pattern_sequence(struct compiler *c, asdl_pattern_seq *patterns, pattern_context *pc) {
    Py_ssize_t size = asdl_seq_LEN(patterns);
    bool only_wildcard = true;
    Py_ssize_t i_star = 0;

    /* Save the index of starred name to i_star if it exists.
     * Raise a compiler error for multiple starred names. */
    for (; i_star < size; i_star++) {
        pattern_ty pattern = asdl_seq_GET(patterns, i_star);
        if (pattern->kind == MatchStar_kind) {
            only_wildcard &= !pattern->v.MatchStar.name;
            break;
        }
        only_wildcard &= is_wildcard_capture(pattern);
    }
    for (Py_ssize_t i = i_star + 1; i < size; i++) {
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        if (pattern->kind == MatchStar_kind) {
            return compiler_error(c, "multiple starred names in sequence pattern");
        }
        only_wildcard &= is_wildcard_capture(pattern);
    }

    Py_ssize_t has_star = i_star < size;
    Py_ssize_t target_size = size - has_star;
    Py_ssize_t check_size = !(size == 1 && only_wildcard);
    Oparg flag = imm_oparg(target_size << 2 | has_star << 1 | check_size);
    Oparg match_res = new_tmp_arg(c);
    CHECK(addop(c, MATCH_SEQUENCE, flag, pc->subject, match_res));
    /* If check_size and has_star is true,
     * match_res will be int(max(0, real_size - target_size + 1)).
     * Otherwise, match_res is a bool value of the match result.
     * In both cases, bool(match_res) always indicates the success of matching. */
    CHECK(addop_jump(c, JUMP_IF_FALSE, match_res, NEG_ADDR_OPARG, pc->fallback, NULL));

    if (only_wildcard) {
        /* Patterns like: [] / [_] / [_, _] / [*_] / [_, *_] / [_, _, *_] / etc. */
        return 1;
    } else if (!has_star || asdl_seq_GET(patterns, i_star)->v.MatchStar.name) {
        /* match_res is not used, free its space. */
        free_cur_tmp(c, 1);
        Oparg tmps = new_tmp_arg_n(c, size);
        unpack_star_helper(c, i_star, size, pc->subject, tmps);
        for (Py_ssize_t i = 0; i < size; i++) {
            Oparg tmp = oparg_by_offset(tmps, i);
            pattern_ty pattern = asdl_seq_GET(patterns, i);
            CHECK(compiler_pattern_subpattern(c, pattern, tmp, pc));
        }
    } else {
        /* Unpack the sequence with LOAD_SUBSCR instead of sequence unpaack.
         * This is more efficient for patterns with a starred wildcard
         * like [first, *_] / [first, *_, last] / [*_, last] / etc. */
        Oparg tmp = new_tmp_arg(c);
        for (Py_ssize_t i = 0; i < size; i++) {
            pattern_ty pattern = asdl_seq_GET(patterns, i);
            if (is_wildcard_capture(pattern) || i == i_star) {
                continue;
            }
            Oparg index;
            if (i < i_star) {
                CHECK(compiler_load_const_new(c, PyLong_FromSsize_t(i)));
                index = *cur_out(c);
            } else {
                /*    match_res + (i - 2)
                 * == (real_size - target_size + 1) + (i - 2)
                 * == (real_size - (size - 1) + 1) + (i - 2)
                 * == real_size - (size - i) */
                PyObject *i_2 = PyLong_FromSsize_t(i - 2);
                CHECK(i_2);
                CHECK(compiler_load_const_new(c, i_2));
                CHECK(addop(c, BINARY_ADD, match_res, *cur_out(c), tmp));
                index = tmp;
            }
            CHECK(addop(c, LOAD_SUBSCR, pc->subject, index, tmp));
            CHECK(compiler_pattern_subpattern(c, pattern, tmp, pc));
        }
    }
    return 1;
}

static int
compiler_pattern_value(struct compiler *c, expr_ty value, pattern_context *pc) {
    if (value->kind != Constant_kind && value->kind != Attribute_kind) {
        return compiler_error(c, "patterns may only match literals and attribute lookups");
    }
    CHECK(compiler_visit_expr(c, value));
    Oparg res = new_tmp_arg(c);
    Oparg object = alloc_arg_reuse(c, &res);
    CHECK(addop(c, COMPARE_EQ, pc->subject, object, res));
    CHECK(addop_jump(c, JUMP_IF_FALSE, res, NEG_ADDR_OPARG, pc->fallback, NULL));
    return 1;
}

static int
compiler_pattern_singleton(struct compiler *c, PyObject *value, pattern_context *pc) {
    CHECK(compiler_load_const(c, value));
    Oparg object = *cur_out(c);
    Oparg res = new_tmp_arg(c);
    CHECK(addop(c, IS_OP, pc->subject, object, res));
    CHECK(addop_jump(c, JUMP_IF_FALSE, res, NEG_ADDR_OPARG, pc->fallback, NULL));
    return 1;
}

static int
compiler_pattern(struct compiler *c, pattern_ty p, pattern_context *pc) {
    SET_LOC(c, p);
    Py_ssize_t old_tmp = cur_tmp(c);
    int res;
    switch (p->kind) {
        case MatchValue_kind:
            res = compiler_pattern_value(c, p->v.MatchValue.value, pc);
            break;
        case MatchSingleton_kind:
            res = compiler_pattern_singleton(c, p->v.MatchSingleton.value, pc);
            break;
        case MatchSequence_kind:
            res = compiler_pattern_sequence(c, p->v.MatchSequence.patterns, pc);
            break;
        case MatchMapping_kind:
            res = compiler_pattern_mapping(c, p, pc);
            break;
        case MatchClass_kind:
            res = compiler_pattern_class(c, p, pc);
            break;
        case MatchStar_kind:
            set_cur_out(c, &pc->subject);
            res = pattern_helper_store_to_tmps(c, p->v.MatchStar.name, pc);
            break;
        case MatchAs_kind:
            res = compiler_pattern_as(c, p->v.MatchAs.pattern, p->v.MatchAs.name, pc);
            break;
        case MatchOr_kind:
            res = compiler_pattern_or(c, p->v.MatchOr.patterns, pc);
            break;
        default:
            /* In fact, AST validator shouldn't let this happen */
            return compiler_error(c, "invalid match pattern node in AST (kind=%z)", p->kind);
    }
    set_cur_tmp(c, old_tmp);
    return res;
}

/* Verify the legality of name capturing. */
static int
compiler_pattern_verify_names(struct compiler *c, pattern_ty p, PyObject *names) {
    PyObject *name;
    switch (p->kind) {
        case MatchSequence_kind: {
            asdl_pattern_seq *patterns = p->v.MatchSequence.patterns;
            Py_ssize_t size = asdl_seq_LEN(patterns);
            for (Py_ssize_t i = 0; i < size; i++) {
                CHECK(compiler_pattern_verify_names(c, asdl_seq_GET(patterns, i), names));
            }
            return 1;
        }
        case MatchMapping_kind: {
            asdl_pattern_seq *patterns = p->v.MatchMapping.patterns;
            Py_ssize_t size = asdl_seq_LEN(patterns);
            for (Py_ssize_t i = 0; i < size; i++) {
                CHECK(compiler_pattern_verify_names(c, asdl_seq_GET(patterns, i), names));
            }
            name = p->v.MatchMapping.rest;
            break;
        }
        case MatchClass_kind: {
            asdl_pattern_seq *patterns = p->v.MatchClass.patterns;
            asdl_pattern_seq *kwd_patterns = p->v.MatchClass.kwd_patterns;
            Py_ssize_t size = asdl_seq_LEN(patterns);
            for (Py_ssize_t i = 0; i < size; i++) {
                CHECK(compiler_pattern_verify_names(c, asdl_seq_GET(patterns, i), names));
            }
            Py_ssize_t kwd_size = asdl_seq_LEN(kwd_patterns);
            for (Py_ssize_t i = 0; i < kwd_size; i++) {
                CHECK(compiler_pattern_verify_names(c, asdl_seq_GET(kwd_patterns, i), names));
            }
            return 1;
        }
        case MatchStar_kind:
            name = p->v.MatchStar.name;
            break;
        case MatchAs_kind:
            if (p->v.MatchAs.pattern) {
                CHECK(compiler_pattern_verify_names(c, p->v.MatchAs.pattern, names));
            }
            name = p->v.MatchAs.name;
            break;
        case MatchOr_kind: {
            asdl_pattern_seq *patterns = p->v.MatchOr.patterns;
            Py_ssize_t size = asdl_seq_LEN(patterns);
            assert(size >= 2);
            PyObject *existed_names = PySet_New(names);
            CHECK(existed_names);
            if (!compiler_pattern_verify_names(c, asdl_seq_GET(patterns, 0), names)) {
                Py_DECREF(existed_names);
                return 0;
            }
            PyObject *control_names = PyNumber_Subtract(names, existed_names);
            Py_DECREF(existed_names);
            CHECK(control_names);
            bool ok = true;
            for (Py_ssize_t i = 1; ok && i < size; i++) {
                ok = false;
                PyObject *alt_names = PySet_New(NULL);
                if (alt_names) {
                    if (compiler_pattern_verify_names(c, asdl_seq_GET(patterns, i), alt_names)) {
                        int set_eq = PyObject_RichCompareBool(control_names, alt_names, Py_EQ);
                        ok = set_eq > 0;
                        if (set_eq == 0) {
                            SET_LOC(c, asdl_seq_GET(patterns, i));
                            compiler_error(c, "alternative patterns bind different names");
                        }
                    }
                    Py_DECREF(alt_names);
                }
            }
            Py_DECREF(control_names);
            return ok;
        }
        default:
            return 1;
    }
    if (!name) {
        return 1;
    }
    int duplicated = PySet_Contains(names, name);
    if (duplicated) {
        if (duplicated != -1) {
            SET_LOC(c, p);
            compiler_error(c, "multiple assignments to name %R in pattern", name);
        }
        return 0;
    }
    return PySet_Add(names, name) != -1;
}

static int
compiler_match(struct compiler *c, stmt_ty s) {
    /* Always store the subject to an temporary register,
     * because local variables may be modified during pattern matching */
    CHECK(compiler_visit_expr_as_tmp(c, s->v.Match.subject));

    pattern_context pc = {.subject = *cur_out(c)};
    basicblock *b_end = compiler_new_block(c);
    CHECK(b_end);

    Py_ssize_t ncases = asdl_seq_LEN(s->v.Match.cases);
    for (Py_ssize_t i = 0; i < ncases; i++) {
        match_case_ty m = asdl_seq_GET(s->v.Match.cases, i);

        pc.fallback = i == ncases - 1 ? b_end : compiler_new_block(c);
        CHECK(pc.fallback);
        /* Capture is allowed only for the last case or guarded case
         * Otherwise it makes the subsequent cases unreachable */
        pc.allow_capture = m->guard || i == ncases - 1;

        pc.stores = NULL;
        PyObject *names = PySet_New(NULL);
        CHECK(names);
        if (compiler_pattern_verify_names(c, m->pattern, names)) {
            pc.stores = PySequence_Tuple(names);
        }
        Py_DECREF(names);
        CHECK(pc.stores);

        Py_ssize_t nstores = PyTuple_GET_SIZE(pc.stores);
        pc.tmps = new_tmp_arg_n(c, nstores);
        int res = compiler_pattern(c, m->pattern, &pc) && pattern_helper_store_from_tmps(c, &pc);
        Py_DECREF(pc.stores);
        CHECK(res);
        free_cur_tmp(c, nstores);

        if (m->guard) {
            CHECK(compiler_logical_expr(c, m->guard, pc.fallback, false));
        }
        CHECK(compiler_visit_stmts(c, m->body));
        CHECK(addop_jump_noline(c, JUMP_ALWAYS, UNSED_OPARG, NEG_ADDR_OPARG, b_end, pc.fallback));
    }
    return 1;
}

static int
compiler_visit_stmt(struct compiler *c, stmt_ty s) {
    switch (s->kind) {
        case Global_kind:
        case Nonlocal_kind:
            return 1;
        case Pass_kind:
            return addop(c, NOP, UNSED_OPARG, UNSED_OPARG, UNSED_OPARG);
        case Expr_kind:
            return compiler_expr_stmt(c, s->v.Expr.value);
        case FunctionDef_kind:
        case AsyncFunctionDef_kind: {
            struct compiler_unit *u = c->u;
            return compiler_safe_exit_scope(c, u, compiler_function(c, s));
        }
        case ClassDef_kind: {
            struct compiler_unit *u = c->u;
            return compiler_safe_exit_scope(c, u, compiler_class(c, s));
        }
        case Return_kind:
            return compiler_return(c, s);
        case Delete_kind: {
            Py_ssize_t n = asdl_seq_LEN(s->v.Delete.targets);
            for (Py_ssize_t i = 0; i < n; i++) {
                CHECK(compiler_visit_expr(c, asdl_seq_GET(s->v.Delete.targets, i)));
            }
            return 1;
        }
        case Assign_kind:
            return compiler_assign(c, s);
        case AugAssign_kind:
            return compiler_augassign(c, s);
        case AnnAssign_kind:
            return compiler_annassign(c, s);
        case For_kind:
            return compiler_for(c, s);
        case AsyncFor_kind:
            return compiler_async_for(c, s);
        case While_kind:
            return compiler_while(c, s);
        case Continue_kind:
        case Break_kind:
            return compiler_continue_break(c, s->kind == Break_kind);
        case If_kind:
            return compiler_if(c, s);
        case Import_kind:
            return compiler_import(c, s);
        case ImportFrom_kind:
            return compiler_from_import(c, s);
        case Raise_kind:
        case Assert_kind:
            return compiler_raise_exception(c, s);
        case Try_kind:
            return compiler_try(c, s);
        case With_kind:
            return compiler_with(c, s, 0);
        case AsyncWith_kind:
            return compiler_async_with(c, s, 0);
        case Match_kind:
            return compiler_match(c, s);
        default:
            Py_UNREACHABLE();
    }
}

static int
compiler_visit_stmts(struct compiler *c, asdl_stmt_seq *seq) {
    for (Py_ssize_t i = 0; i < asdl_seq_LEN(seq); i++) {
        stmt_ty stmt = asdl_seq_GET(seq, i);
        SET_LOC(c, stmt);
        Py_ssize_t old_tmp = cur_tmp(c);
        CHECK(compiler_visit_stmt(c, stmt));
        set_cur_tmp(c, old_tmp);
    }
    return 1;
}

/* End of the compiler section, beginning of the assembler section */

struct assembler {
    PyObject *a_bytecode;  /* string containing bytecode */
    int a_offset;              /* offset into bytecode */
    PyObject *a_lnotab;    /* string containing lnotab */
    int a_lnotab_off;      /* offset into lnotab */
    int a_prevlineno;     /* lineno of last emitted line in line table */
    int a_lineno;          /* lineno of last emitted instruction */
    int a_lineno_start;    /* bytecode start offset of current lineno */
    basicblock *a_entry;
};

static int
assemble_emit_linetable_pair(struct assembler *a, int bdelta, int ldelta) {
    Py_ssize_t len = PyBytes_GET_SIZE(a->a_lnotab);
    if (a->a_lnotab_off + 2 >= len) {
        if (_PyBytes_Resize(&a->a_lnotab, len * 2) < 0)
            return 0;
    }
    unsigned char *lnotab = (unsigned char *) PyBytes_AS_STRING(a->a_lnotab);
    lnotab += a->a_lnotab_off;
    a->a_lnotab_off += 2;
    *lnotab++ = bdelta;
    *lnotab++ = ldelta;
    return 1;
}

/* Appends a range to the end of the line number table. See
 *  Objects/lnotab_notes.txt for the description of the line number table. */

static int
assemble_line_range(struct assembler *a) {
    int ldelta, bdelta;
    bdelta = (a->a_offset - a->a_lineno_start) * sizeof(_Py_CODEUNIT);
    if (bdelta == 0) {
        return 1;
    }
    if (a->a_lineno < 0) {
        ldelta = -128;
    } else {
        ldelta = a->a_lineno - a->a_prevlineno;
        a->a_prevlineno = a->a_lineno;
        while (ldelta > 127) {
            if (!assemble_emit_linetable_pair(a, 0, 127)) {
                return 0;
            }
            ldelta -= 127;
        }
        while (ldelta < -127) {
            if (!assemble_emit_linetable_pair(a, 0, -127)) {
                return 0;
            }
            ldelta += 127;
        }
    }
    assert(-128 <= ldelta && ldelta < 128);
    while (bdelta > 254) {
        if (!assemble_emit_linetable_pair(a, 254, ldelta)) {
            return 0;
        }
        ldelta = a->a_lineno < 0 ? -128 : 0;
        bdelta -= 254;
    }
    if (!assemble_emit_linetable_pair(a, bdelta, ldelta)) {
        return 0;
    }
    a->a_lineno_start = a->a_offset;
    return 1;
}

/* assemble_emit()
   Extend the bytecode with a new instruction.
   Update lnotab if necessary.
*/
static int
assemble_emit(struct assembler *a) {
    for (basicblock *b = a->a_entry; b; b = b->b_fall) {
        struct instr *i_end = &b->b_instr[b->b_iused];
        _Py_CODEUNIT *bytecode = (_Py_CODEUNIT *) PyBytes_AS_STRING(a->a_bytecode);
        for (struct instr *i = b->b_instr; i != i_end; i++) {
            if (i->i_opcode == DELETED_OP) {
                continue;
            }

            if (i->i_lineno && i->i_lineno != a->a_lineno) {
                CHECK(assemble_line_range(a));
                a->a_lineno = i->i_lineno;
            }
            int size = instrsize(i);
            assert(size >= 1);
            _Py_CODEUNIT *bc = &bytecode[a->a_offset];
            a->a_offset += size;

            size_t oparg1 = OPARG_FINAL_VALUE(i->i_opargs[0]);
            size_t oparg2 = OPARG_FINAL_VALUE(i->i_opargs[1]);
            size_t oparg3 = OPARG_FINAL_VALUE(i->i_opargs[2]);
            bc[--size] = PACKOPARG(i->i_opcode, oparg1, oparg2, oparg3);
            while (size) {
                oparg1 >>= 8;
                oparg2 >>= 8;
                oparg3 >>= 8;
                bc[--size] = PACKOPARG(EXTENDED_ARG, oparg1, oparg2, oparg3);
            }
        }
    }
    CHECK(assemble_line_range(a));
    return 1;
}

static int
finalize_instructions(struct assembler *a, struct compiler *c) {
    int total_size = 0;
    size_t tmp_offset = PyDict_GET_SIZE(c->u->u_varnames);
    size_t const_offset = tmp_offset + c->u->u_tmp_max;
    for (basicblock *b = a->a_entry; b; b = b->b_fall) {
        b->b_offset = total_size;
        for (int i = 0; i < b->b_iused; i++) {
            struct instr *instr = &b->b_instr[i];
            if (instr->i_opcode == DELETED_OP) {
                continue;
            }
            for (int j = 0; j < 3; ++j) {
                Oparg *oparg = &instr->i_opargs[j];
                size_t value = OPARG_VALUE(*oparg);
                switch (OPARG_TYPE(*oparg)) {
                    case OpargX:
                        assert(!OPARG_EQ(*oparg, RELOC_OPARG));
                        value = 0;
                        break;
                    case OpargImm:
                    case OpargFast:
                        break;
                    case OpargTmp:
                        value += tmp_offset;
                        break;
                    case OpargConst:
                        value += const_offset;
                        break;
                    default:
                        Py_UNREACHABLE();
                }
                assert((size_t) (_Py_OPARG) value == value);
                OPARG_FINAL_VALUE(*oparg) = value;
            }
            total_size += instrsize(instr);
        }
        b->b_size = total_size - b->b_offset;
    }

    /* The oparg of a jump instruction is a relative address.
     * The relative address is determined by the block size.
     * The block size is size sum of its instructions.
     * The size of instruction depends on the oparg.
     * ...
     * An endless story, a bootstrap problem.
     *
     * We use fixed point algorithm to solve it.
     * The oparg for jump is initially set to 0,
     * and we have the initial offset and size for each block.
     * For each iteration,
     * Then we fix the jump oparg with current values.
     * The the block's size is re-calculate if necessary.
     * The iteration stops if no size is changed.
     * Otherwise we update the offset with new size and continue. */
    do {
        bool size_changed = 0;
        for (basicblock *b = a->a_entry; b; b = b->b_fall) {
            if (!b->b_jump) {
                continue;
            }
            struct instr *last_i = &b->b_instr[b->b_iused - 1];
            int old_size = instrsize(last_i);
            /* Jumps always use relative addresses,
             * but backward jumps are only allowed for a few opcodes. */
            int jump_offset = b->b_jump->b_offset - (b->b_offset + b->b_size);
            bool backwards = jump_offset < 0;
            Opcode op = last_i->i_opcode;
            bool can_backwards = op == JUMP_ALWAYS || op == JUMP_IF_FALSE || op == JUMP_IF_TRUE;
            if (backwards) {
                assert(can_backwards);
                OPARG_FINAL_VALUE(last_i->i_opargs[1]) = -jump_offset;
                OPARG_FINAL_VALUE(last_i->i_opargs[2]) = 0;
            } else {
                if (can_backwards) {
                    OPARG_FINAL_VALUE(last_i->i_opargs[1]) = 0;
                }
                OPARG_FINAL_VALUE(last_i->i_opargs[2]) = jump_offset;
            }
            int new_size = instrsize(last_i);
            size_changed |= new_size != old_size;
            b->b_size += new_size - old_size;
        }

        if (!size_changed) {
            return total_size;
        }

        total_size = 0;
        for (basicblock *b = a->a_entry; b; b = b->b_fall) {
            b->b_offset = total_size;
            total_size += b->b_size;
        }
    } while (1);
}

static PyObject *
dict_keys_inorder(PyObject *dict, bool for_consts, Py_ssize_t offset) {
    Py_ssize_t size = PyDict_GET_SIZE(dict);
    PyObject *tuple = PyTuple_New(size);
    if (!tuple) {
        return NULL;
    }
    PyObject *k, *v;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict, &pos, &k, &v)) {
        Py_ssize_t i = PyLong_AS_LONG(v) - offset;
        if (for_consts && PyTuple_CheckExact(k)) {
            k = PyTuple_GET_ITEM(k, 1);
        }
        Py_INCREF(k);
        assert(i >= 0 && i < size);
        PyTuple_SET_ITEM(tuple, i, k);
    }
    return tuple;
}

// Merge *obj* with constant cache.
// Unlike merge_consts_recursive(), this function doesn't work recursively.
static int
merge_const_one(struct compiler *c, PyObject **obj) {
    PyObject *key = _PyCode_ConstantKey(*obj);
    CHECK(key);

    // t is borrowed reference
    PyObject *t = PyDict_SetDefault(c->c_const_cache, key, key);
    Py_DECREF(key);
    CHECK(t);
    if (t == key) {  // obj is new constant.
        return 1;
    }

    if (PyTuple_CheckExact(t)) {
        // t is still borrowed reference
        t = PyTuple_GET_ITEM(t, 1);
    }

    Py_INCREF(t);
    Py_DECREF(*obj);
    *obj = t;
    return 1;
}

static int
normalize_basic_blocks(struct compiler *c, struct assembler *a) {
    /* Find the entry and skip all empty blocks */
    int nblocks = 0;
    for (basicblock *b = c->u->u_blocks; b; b = b->b_prev_allocated) {
        a->a_entry = b;
        if (b->b_iused) {
            nblocks++;
            while (b->b_fall && !b->b_fall->b_iused) {
                b->b_fall = b->b_fall->b_fall;
            }
            while (b->b_jump && !b->b_jump->b_iused) {
                b->b_jump = b->b_jump->b_fall;
            }
        }
    }

    /* Revise the pre-inserted instructions  */
    assert(a->a_entry->b_iused >= 2);
    int kind = c->u->u_ste->ste_generator | (c->u->u_ste->ste_coroutine << 1);
    if (kind) {
        a->a_entry->b_instr[0].i_opargs[0] = imm_oparg(kind);
    } else {
        set_to_nop(&a->a_entry->b_instr[0]);
    }
    if (c->u->u_annotations < 2) {
        set_to_nop(&a->a_entry->b_instr[1]);
    }

    /* Calculate the real predecessor number by deep first search. */
    basicblock **stack = PyObject_Malloc(sizeof(basicblock *) * nblocks);
    if (!stack) {
        PyErr_NoMemory();
        return 0;
    }
    (stack[0] = a->a_entry)->b_predecessors = 1;
    for (basicblock **sp = &stack[1]; sp > stack;) {
        basicblock *b = *(--sp);
        if (b->b_fall && !b->b_nofallthrough && !b->b_fall->b_predecessors++) {
            *sp++ = b->b_fall;
        }
        if (b->b_jump && !b->b_jump->b_predecessors++) {
            *sp++ = b->b_jump;
        }
    }
    PyObject_Free(stack);

    /* Remove unreachable block */
    for (basicblock *b = a->a_entry; b; b = b->b_fall) {
        while (b->b_fall && !b->b_fall->b_predecessors) {
            b->b_fall = b->b_fall->b_fall;
        }
    }
    return 1;
}

static int
fix_lineno_of_basic_blocks(struct compiler *c, struct assembler *a) {
    /* Do an intra-block level lineno propagation.
     * After this this step, each block may only:
     * - have meaningful lineno for all instructions;
     * - lineno == -1 for all instructions. */
    for (basicblock *b = a->a_entry; b; b = b->b_fall) {
        int lineno = -1;
        int index;
        for (index = 0; index < b->b_iused; index++) {
            if ((lineno = b->b_instr[index].i_lineno) >= 0) {
                break;
            }
        }
        /* backward propagation */
        for (int i = 0; i < index; i++) {
            b->b_instr[i].i_lineno = lineno;
        }
        /* forward propagation */
        for (int i = index; i < b->b_iused; i++) {
            int *this_lineno = &b->b_instr[i].i_lineno;
            if (*this_lineno < 0) {
                *this_lineno = lineno;
            } else {
                lineno = *this_lineno;
            }
        }
    }

    /* Copy exit block if it has no lineno and multiple b_predecessors,
     * so inter-block lineno propagation will be valid for it.
     * Blocks end with unconditional jump are also copied. */
    for (basicblock *b = a->a_entry; b; b = b->b_fall) {
        basicblock *jump_from = b;
        basicblock *jump_to = jump_from->b_jump;
        /* jump_to->b_nofallthrough implies !jump_from->b_exc_jump */
        while (jump_to
               && jump_to->b_predecessors > 1
               && jump_to->b_nofallthrough
               && jump_to->b_instr[0].i_lineno < 0) {
            CHECK(jump_from->b_jump = compiler_copy_block(c, jump_to, jump_to));
            jump_from = jump_from->b_jump;
            jump_to = jump_from->b_jump;
            if (jump_to) {
                jump_to->b_predecessors++;
            }
        }
    }

    /* inter-block lineno propagation */
    int secure_lineno = -1;
    int insecure_lineno = c->u->u_firstlineno;
    for (basicblock *b = a->a_entry; b; b = b->b_fall) {
        int this_lineno = b->b_instr[b->b_iused - 1].i_lineno;
        if (this_lineno < 0 && b->b_predecessors == 1) {
            if (b->b_exit) {
                for (int i = 0; i < b->b_iused; i++) {
                    b->b_instr[i].i_lineno = insecure_lineno;
                }
            } else if (secure_lineno > 0) {
                this_lineno = secure_lineno;
                for (int i = 0; i < b->b_iused; i++) {
                    b->b_instr[i].i_lineno = this_lineno;
                }
            }
        }
        if (this_lineno > 0
            && b->b_jump && !b->b_exc_jump
            && b->b_jump->b_predecessors == 1
            && b->b_jump->b_instr[0].i_lineno < 0) {
            for (int i = 0; i < b->b_jump->b_iused; i++) {
                b->b_jump->b_instr[i].i_lineno = this_lineno;
            }
        }
        secure_lineno = b->b_nofallthrough ? -1 : this_lineno;
        insecure_lineno = this_lineno < 0 ? insecure_lineno : this_lineno;
    }

    /* lineno of GEN_START must be -1 */
    a->a_entry->b_instr[0].i_lineno = -1;
    return 1;
}

static int
optimize_basic_blocks(struct compiler *c, struct assembler *a) {
    /* Optimize unconditional jumps. */
    int moved_blocks = 0;
    for (basicblock *b = a->a_entry; b; b = b->b_fall) {
        struct instr *last_i = &b->b_instr[b->b_iused - 1];
        if (last_i->i_opcode != JUMP_ALWAYS) {
            continue;
        }
        basicblock *target = b->b_jump;
        if (target == b->b_fall) {
            b->b_jump = NULL;
            target->b_predecessors--;
            set_to_nop(last_i);
            continue;
        }

        /* If jumping to an exit block whose b_predecessors is 1,
         * Moving this block is more efficient than copying it.
         * N.B., if target->b_exit is true, target->b_jump is always NULL,
         * but we make use of this filed to store where it moves to. */
        if (target->b_exit && target->b_predecessors == 1) {
            set_to_nop(last_i);
            b->b_jump = NULL;
            b->b_nofallthrough = false;
            moved_blocks++;
            target->b_jump = b;
            continue;
        }

        const int MAX_COPY_SIZE = 4;
        if (target->b_exit && target->b_iused < MAX_COPY_SIZE) {
            basicblock *copied = compiler_copy_block(c, target, b);
            CHECK(copied);
            set_to_nop(last_i);
            b->b_jump = NULL;
            b->b_nofallthrough = false;
        }
    }
    for (basicblock *b = a->a_entry; moved_blocks && b; b = b->b_fall) {
        basicblock *target = b->b_fall;
        while (target && target->b_exit && target->b_jump) {
            moved_blocks--;
            basicblock *append_to = target->b_jump;
            target->b_jump = NULL;
            b->b_fall = target->b_fall;
            target->b_fall = append_to->b_fall;
            append_to->b_fall = target;
            target = b->b_fall;
        }
    }

    /* Set "MOVE_FAST x . x" to NOP */
    for (basicblock *b = a->a_entry; b; b = b->b_fall) {
        for (int i = 0; i < b->b_iused; i++) {
            struct instr *this_i = &b->b_instr[i];
            if (this_i->i_opcode == MOVE_FAST
                && OPARG_EQ(this_i->i_opargs[0], this_i->i_opargs[2])) {
                set_to_nop(this_i);
            }
        }
    }

    /* Remove NOP if possible */
    for (basicblock *b = a->a_entry; b; b = b->b_fall) {
        /* Inter-block NOP removing (except the last instruction) */
        struct instr *this_i = &b->b_instr[0];
        struct instr *last_i = &b->b_instr[b->b_iused - 1];
        int last_lineno = -1;
        while (this_i < last_i) {
            int lineno = this_i->i_lineno;
            if (this_i->i_opcode == NOP && (lineno == last_lineno || lineno == this_i[1].i_lineno)) {
                this_i->i_opcode = DELETED_OP;
            } else {
                last_lineno = lineno;
            }
            this_i++;
        }
        /* Inter-block NOP removing (the last instruction) */
        if (last_i->i_opcode == NOP && last_i->i_lineno == last_lineno) {
            last_i->i_opcode = DELETED_OP;
        } else {
            last_lineno = last_i->i_lineno;
        }

        /* Inter-block NOP removing */
        basicblock *successors[2] = {b->b_fall, b->b_jump};
        for (int i = 0; i < 2; ++i) {
            basicblock *s1 = successors[i];
            basicblock *s2 = successors[!i];
            if (s1) {
                struct instr *next_i = &s1->b_instr[0];
                struct instr *next_last_i = next_i + s1->b_iused - 1;
                while (next_i < next_last_i && next_i->i_opcode == DELETED_OP) {
                    next_i++;
                }
                if (next_i->i_lineno == last_lineno) {
                    if (last_i->i_opcode == NOP && !s2) {
                        last_i->i_opcode = DELETED_OP;
                    } else if (next_i->i_opcode == NOP && s1->b_predecessors == 1) {
                        next_i->i_opcode = DELETED_OP;
                    }
                }
            }
        }
    }
    return 1;
}

/* Try to combine two MOVE_FAST opcodes into one opcode.
 * reverse control the move direction (LOAD or STORE).
 * Try must_tmp=true first, it makes the compiled code more human readable. */
static bool
try_to_combine_double_moves(struct instr *first, struct instr *second,
                            bool reverse, bool must_tmp) {
    int i = reverse ? 2 : 0;
    Oparg s1 = first->i_opargs[i];
    Oparg d1 = first->i_opargs[2 - i];
    Oparg s2 = second->i_opargs[i];
    Oparg d2 = second->i_opargs[2 - i];
    if (OPARG_TYPE(d1) == OPARG_TYPE(d2) && (!must_tmp || OPARG_TYPE(d1) == OpargTmp)) {
        Py_ssize_t delta = OPARG_VALUE(d2) - OPARG_VALUE(d1);
        if (delta == 1 || (delta == -1 && !OPARG_EQ(first->i_opargs[2], second->i_opargs[0]))) {
            first->i_opcode = DELETED_OP;
            second->i_opcode = reverse ? STORE_TWO_FAST : LOAD_TWO_FAST;
            if (delta == 1) {
                second->i_opargs[0] = s1;
                second->i_opargs[1] = s2;
                second->i_opargs[2] = d1;
            } else {
                second->i_opargs[0] = s2;
                second->i_opargs[1] = s1;
                second->i_opargs[2] = d2;
            }
            return true;
        }
    }
    return false;
}

static int
optimize_double_moves(struct compiler *c, struct assembler *a) {
    struct instr *first = NULL;
    struct instr *second = &a->a_entry->b_instr[0];
    struct instr *end_instr = &a->a_entry->b_instr[a->a_entry->b_iused];
    for (basicblock *b = a->a_entry;;) {
        while (second < end_instr) {
            if (second->i_opcode == MOVE_FAST) {
                if (first && first->i_lineno == second->i_lineno) {
                    break;
                } else {
                    first = second;
                }
            } else if (second->i_opcode != DELETED_OP) {
                first = NULL;
            }
            second++;
        }

        if (second == end_instr) {
            if (!b->b_fall) {
                break;
            }
            if (b->b_jump || b->b_fall->b_predecessors != 1) {
                first = NULL;
            }
            b = b->b_fall;
            second = &b->b_instr[0];
            end_instr = &b->b_instr[b->b_iused];
            continue;
        }
        assert(first && first != second);

        if (try_to_combine_double_moves(first, second, false, true) ||
            try_to_combine_double_moves(first, second, true, true) ||
            try_to_combine_double_moves(first, second, false, false) ||
            try_to_combine_double_moves(first, second, true, false)) {
            first = NULL;
            continue;
        }
        first = second++;
    }
    return 1;
}

static PyCodeObject *
makecode(struct compiler *c, struct assembler *a) {
    int flags = c->c_flags->cf_flags & PyCF_MASK;
    PySTEntryObject *ste = c->u->u_ste;
    if (is_top_level_await(c) && ste->ste_coroutine && !ste->ste_generator) {
        flags |= CO_COROUTINE;
    }
    if (ste->ste_type == FunctionBlock) {
        flags |= CO_NEWLOCALS | CO_OPTIMIZED;
        flags |= ste->ste_nested ? CO_NESTED : 0;
        flags |= ste->ste_varargs ? CO_VARARGS : 0;
        flags |= ste->ste_varkeywords ? CO_VARKEYWORDS : 0;
        flags |= ste->ste_generator ?
                 (ste->ste_coroutine ? CO_ASYNC_GENERATOR : CO_GENERATOR) :
                 (ste->ste_coroutine ? CO_COROUTINE : 0);
    }

    PyCodeObject *co = NULL;
    PyObject *names = NULL;
    PyObject *varnames = NULL;
    PyObject *name = NULL;
    PyObject *freevars = NULL;
    PyObject *cellvars = NULL;
    PyObject *consts = NULL;

    names = dict_keys_inorder(c->u->u_names, false, 0);
    if (!names || !merge_const_one(c, &names)) {
        goto error;
    }
    varnames = dict_keys_inorder(c->u->u_varnames, false, 0);
    if (!varnames || !merge_const_one(c, &varnames)) {
        goto error;
    }
    cellvars = dict_keys_inorder(c->u->u_cellvars, false, 0);
    if (!cellvars || !merge_const_one(c, &cellvars)) {
        goto error;
    }
    freevars = dict_keys_inorder(c->u->u_freevars, false, PyTuple_GET_SIZE(cellvars));
    if (!freevars || !merge_const_one(c, &freevars)) {
        goto error;
    }
    int nlocals_int = Py_SAFE_DOWNCAST(PyDict_GET_SIZE(c->u->u_varnames), Py_ssize_t, int);

    consts = dict_keys_inorder(c->u->u_consts, true, 0);
    if (!consts || !merge_const_one(c, &consts)) {
        goto error;
    }

    int posonlyargcount = Py_SAFE_DOWNCAST(c->u->u_posonlyargcount, Py_ssize_t, int);
    int posorkeywordargcount = Py_SAFE_DOWNCAST(c->u->u_argcount, Py_ssize_t, int);
    int kwonlyargcount = Py_SAFE_DOWNCAST(c->u->u_kwonlyargcount, Py_ssize_t, int);
    int maxdepth = c->u->u_tmp_max;
    if (maxdepth > MAX_ALLOWED_TMP_USE) {
        PyErr_Format(PyExc_SystemError,
                     "excessive stack use: stack is %d deep",
                     maxdepth);
        goto error;
    }
    co = PyCode_NewWithPosOnlyArgs(posonlyargcount + posorkeywordargcount,
                                   posonlyargcount, kwonlyargcount, nlocals_int,
                                   maxdepth, flags, a->a_bytecode, consts, names,
                                   varnames, freevars, cellvars, c->c_filename,
                                   c->u->u_name, c->u->u_firstlineno, a->a_lnotab);
error:
    Py_XDECREF(names);
    Py_XDECREF(varnames);
    Py_XDECREF(name);
    Py_XDECREF(freevars);
    Py_XDECREF(cellvars);
    Py_XDECREF(consts);
    return co;
}

static int
assemble(struct compiler *c, PyCodeObject **co) {
    *co = NULL;
    struct assembler a;
    memset(&a, 0, sizeof(struct assembler));

    /* The firstlineno should always be set with our implementation. */
    assert(c->u->u_firstlineno > 0);
    a.a_prevlineno = a.a_lineno = c->u->u_firstlineno;

    CHECK(normalize_basic_blocks(c, &a));
    CHECK(fix_lineno_of_basic_blocks(c, &a));
    CHECK(optimize_basic_blocks(c, &a));
    CHECK(optimize_double_moves(c, &a));

    int instr_num = finalize_instructions(&a, c);
    CHECK(a.a_bytecode = PyBytes_FromStringAndSize(NULL, instr_num * sizeof(_Py_CODEUNIT)));
    const int DEFAULT_LNOTAB_SIZE = 16;
    a.a_lnotab = PyBytes_FromStringAndSize(NULL, DEFAULT_LNOTAB_SIZE);
    if (!a.a_lnotab) {
        Py_DECREF(a.a_bytecode);
        return 0;
    }

    if (!assemble_emit(&a)) {
        goto error;
    }

    if (_PyBytes_Resize(&a.a_lnotab, a.a_lnotab_off) < 0) {
        goto error;
    }
    if (!merge_const_one(c, &a.a_lnotab)) {
        goto error;
    }
    if (!merge_const_one(c, &a.a_bytecode)) {
        goto error;
    }

    *co = makecode(c, &a);
error:
    Py_DECREF(a.a_bytecode);
    Py_DECREF(a.a_lnotab);
    return *co != NULL;
}

static int
compiler_mod(struct compiler *c, mod_ty mod, PyCodeObject **co) {
    static PyObject *module;
    CHECK(intern_static_identifier(&module, "<module>"));
    CHECK(compiler_enter_scope(c, module, COMPILER_SCOPE_MODULE, mod, 1));

    bool add_return = true;
    switch (mod->kind) {
        case Module_kind:
            CHECK(compiler_add_docstring(c, mod->v.Module.body, true));
            CHECK(compiler_visit_stmts(c, mod->v.Module.body));
            break;
        case Interactive_kind:
            c->c_interactive = true;
            CHECK(compiler_visit_stmts(c, mod->v.Interactive.body));
            break;
        case Expression_kind:
            add_return = false;
            CHECK(compiler_visit_expr(c, mod->v.Expression.body));
            CHECK(addop_exit_noline(c, RETURN_VALUE, alloc_arg_auto(c), UNSED_OPARG, UNSED_OPARG, NULL));
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "module kind %d should not be possible",
                         mod->kind);
            return 0;
    }
    if (add_return) {
        c->u->u_lineno = -1;
        CHECK(compiler_load_const(c, Py_None));
        CHECK(addop_exit(c, RETURN_VALUE, *cur_out(c), UNSED_OPARG, UNSED_OPARG, NULL));
    }
    CHECK(assemble(c, co));
    compiler_exit_scope(c);
    return 1;
}

/* Non-static functions */

PyObject *
_Py_Mangle(PyObject *privateobj, PyObject *ident) {
    /* Name mangling: __private becomes _classname__private.
       This is independent from how the name is used. */
    PyObject *result;
    size_t nlen, plen, ipriv;
    Py_UCS4 maxchar;
    if (!privateobj || !PyUnicode_Check(privateobj) ||
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
    if ((PyUnicode_READ_CHAR(ident, nlen - 1) == '_' &&
         PyUnicode_READ_CHAR(ident, nlen - 2) == '_') ||
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
    if (PyUnicode_CopyCharacters(result, plen + 1, ident, 0, nlen) < 0) {
        Py_DECREF(result);
        return NULL;
    }
    assert(_PyUnicode_CheckConsistency(result, 1));
    return result;
}

PyCodeObject *
_PyAST_Compile(mod_ty mod, PyObject *filename, PyCompilerFlags *flags,
               int optimize, PyArena *arena) {
    struct compiler c;
    PyCodeObject *co = NULL;
    PyCompilerFlags local_flags = _PyCompilerFlags_INIT;
    int merged;

    memset(&c, 0, sizeof(struct compiler));
    if (!(c.c_const_cache = PyDict_New())) {
        return NULL;
    }
    Py_INCREF(filename);
    c.c_filename = filename;
    c.c_future = _PyFuture_FromAST(mod, filename);
    if (!c.c_future)
        goto finally;
    if (!flags) {
        flags = &local_flags;
    }
    merged = c.c_future->ff_features | flags->cf_flags;
    c.c_future->ff_features = merged;
    flags->cf_flags = merged;
    c.c_flags = flags;
    c.c_optimize = (optimize == -1) ? _Py_GetConfig()->optimization_level : optimize;

    _PyASTOptimizeState state;
    state.optimize = c.c_optimize;
    state.ff_features = merged;

    if (!_PyAST_Optimize(mod, arena, &state)) {
        goto finally;
    }

    c.c_st = _PySymtable_Build(mod, filename, c.c_future);
    if (!c.c_st) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_SystemError, "no symtable");
        goto finally;
    }

    compiler_safe_exit_scope(&c, NULL, compiler_mod(&c, mod, &co));

finally:
    assert(!c.u);
    if (c.c_st) {
        _PySymtable_Free(c.c_st);
    }
    if (c.c_future) {
        PyObject_Free(c.c_future);
    }
    Py_XDECREF(c.c_filename);
    Py_DECREF(c.c_const_cache);
    assert(co || PyErr_Occurred());
    return co;
}
