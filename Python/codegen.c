/*
 * This file implements the compiler's code generation stage, which
 * produces a sequence of pseudo-instructions from an AST.
 *
 * The primary entry point is _PyCodegen_Module() for modules, and
 * _PyCodegen_Expression() for expressions.
 *
 * CAUTION: The VISIT_* macros abort the current function when they
 * encounter a problem. So don't invoke them when there is memory
 * which needs to be released. Code blocks are OK, as the compiler
 * structure takes care of releasing those.  Use the arena to manage
 * objects.
 */

#include "Python.h"
#include "opcode.h"
#include "pycore_ast.h"           // _PyAST_GetDocString()
#define NEED_OPCODE_TABLES
#include "pycore_opcode_utils.h"
#undef NEED_OPCODE_TABLES
#include "pycore_c_array.h"       // _Py_c_array_t
#include "pycore_code.h"          // COMPARISON_LESS_THAN
#include "pycore_compile.h"
#include "pycore_instruction_sequence.h" // _PyInstructionSequence_NewLabel()
#include "pycore_intrinsics.h"
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_object.h"        // _Py_ANNOTATE_FORMAT_VALUE_WITH_FAKE_GLOBALS
#include "pycore_pystate.h"       // _Py_GetConfig()
#include "pycore_symtable.h"      // PySTEntryObject
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString
#include "pycore_ceval.h"         // SPECIAL___ENTER__
#include "pycore_template.h"      // _PyTemplate_Type

#define NEED_OPCODE_METADATA
#include "pycore_opcode_metadata.h" // _PyOpcode_opcode_metadata, _PyOpcode_num_popped/pushed
#undef NEED_OPCODE_METADATA

#include <stdbool.h>

#define COMP_GENEXP   0
#define COMP_LISTCOMP 1
#define COMP_SETCOMP  2
#define COMP_DICTCOMP 3

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

#define RETURN_IF_ERROR_IN_SCOPE(C, CALL)   \
    do {                                    \
        if ((CALL) < 0) {                   \
            _PyCompile_ExitScope((C));      \
            return ERROR;                   \
        }                                   \
    } while (0)

struct _PyCompiler;
typedef struct _PyCompiler compiler;

#define INSTR_SEQUENCE(C) _PyCompile_InstrSequence(C)
#define FUTURE_FEATURES(C) _PyCompile_FutureFeatures(C)
#define SYMTABLE(C) _PyCompile_Symtable(C)
#define SYMTABLE_ENTRY(C) _PyCompile_SymtableEntry(C)
#define OPTIMIZATION_LEVEL(C) _PyCompile_OptimizationLevel(C)
#define IS_INTERACTIVE_TOP_LEVEL(C) _PyCompile_IsInteractiveTopLevel(C)
#define SCOPE_TYPE(C) _PyCompile_ScopeType(C)
#define QUALNAME(C) _PyCompile_Qualname(C)
#define METADATA(C) _PyCompile_Metadata(C)

typedef _PyInstruction instruction;
typedef _PyInstructionSequence instr_sequence;
typedef _Py_SourceLocation location;
typedef _PyJumpTargetLabel jump_target_label;

typedef _PyCompile_FBlockInfo fblockinfo;

#define LOCATION(LNO, END_LNO, COL, END_COL) \
    ((const _Py_SourceLocation){(LNO), (END_LNO), (COL), (END_COL)})

#define LOC(x) SRC_LOCATION_FROM_AST(x)

#define NEW_JUMP_TARGET_LABEL(C, NAME) \
    jump_target_label NAME = _PyInstructionSequence_NewLabel(INSTR_SEQUENCE(C)); \
    if (!IS_JUMP_TARGET_LABEL(NAME)) { \
        return ERROR; \
    }

#define USE_LABEL(C, LBL) \
    RETURN_IF_ERROR(_PyInstructionSequence_UseLabel(INSTR_SEQUENCE(C), (LBL).id))

static const int compare_masks[] = {
    [Py_LT] = COMPARISON_LESS_THAN,
    [Py_LE] = COMPARISON_LESS_THAN | COMPARISON_EQUALS,
    [Py_EQ] = COMPARISON_EQUALS,
    [Py_NE] = COMPARISON_NOT_EQUALS,
    [Py_GT] = COMPARISON_GREATER_THAN,
    [Py_GE] = COMPARISON_GREATER_THAN | COMPARISON_EQUALS,
};


int
_Py_CArray_Init(_Py_c_array_t* array, int item_size, int initial_num_entries) {
    memset(array, 0, sizeof(_Py_c_array_t));
    array->item_size = item_size;
    array->initial_num_entries = initial_num_entries;
    return 0;
}

void
_Py_CArray_Fini(_Py_c_array_t* array)
{
    if (array->array) {
        PyMem_Free(array->array);
        array->allocated_entries = 0;
    }
}

int
_Py_CArray_EnsureCapacity(_Py_c_array_t *c_array, int idx)
{
    void *arr = c_array->array;
    int alloc = c_array->allocated_entries;
    if (arr == NULL) {
        int new_alloc = c_array->initial_num_entries;
        if (idx >= new_alloc) {
            new_alloc = idx + c_array->initial_num_entries;
        }
        arr = PyMem_Calloc(new_alloc, c_array->item_size);
        if (arr == NULL) {
            PyErr_NoMemory();
            return ERROR;
        }
        alloc = new_alloc;
    }
    else if (idx >= alloc) {
        size_t oldsize = alloc * c_array->item_size;
        int new_alloc = alloc << 1;
        if (idx >= new_alloc) {
            new_alloc = idx + c_array->initial_num_entries;
        }
        size_t newsize = new_alloc * c_array->item_size;

        if (oldsize > (SIZE_MAX >> 1)) {
            PyErr_NoMemory();
            return ERROR;
        }

        assert(newsize > 0);
        void *tmp = PyMem_Realloc(arr, newsize);
        if (tmp == NULL) {
            PyErr_NoMemory();
            return ERROR;
        }
        alloc = new_alloc;
        arr = tmp;
        memset((char *)arr + oldsize, 0, newsize - oldsize);
    }

    c_array->array = arr;
    c_array->allocated_entries = alloc;
    return SUCCESS;
}


typedef struct {
    // A list of strings corresponding to name captures. It is used to track:
    // - Repeated name assignments in the same pattern.
    // - Different name assignments in alternatives.
    // - The order of name assignments in alternatives.
    PyObject *stores;
    // If 0, any name captures against our subject will raise.
    int allow_irrefutable;
    // An array of blocks to jump to on failure. Jumping to fail_pop[i] will pop
    // i items off of the stack. The end result looks like this (with each block
    // falling through to the next):
    // fail_pop[4]: POP_TOP
    // fail_pop[3]: POP_TOP
    // fail_pop[2]: POP_TOP
    // fail_pop[1]: POP_TOP
    // fail_pop[0]: NOP
    jump_target_label *fail_pop;
    // The current length of fail_pop.
    Py_ssize_t fail_pop_size;
    // The number of items on top of the stack that need to *stay* on top of the
    // stack. Variable captures go beneath these. All of them will be popped on
    // failure.
    Py_ssize_t on_top;
} pattern_context;

static int codegen_nameop(compiler *, location, identifier, expr_context_ty);

static int codegen_visit_stmt(compiler *, stmt_ty);
static int codegen_visit_keyword(compiler *, keyword_ty);
static int codegen_visit_expr(compiler *, expr_ty);
static int codegen_augassign(compiler *, stmt_ty);
static int codegen_annassign(compiler *, stmt_ty);
static int codegen_subscript(compiler *, expr_ty);
static int codegen_slice_two_parts(compiler *, expr_ty);
static int codegen_slice(compiler *, expr_ty);

static int codegen_body(compiler *, location, asdl_stmt_seq *, bool);
static int codegen_with(compiler *, stmt_ty);
static int codegen_async_with(compiler *, stmt_ty);
static int codegen_with_inner(compiler *, stmt_ty, int);
static int codegen_async_with_inner(compiler *, stmt_ty, int);
static int codegen_async_for(compiler *, stmt_ty);
static int codegen_call_simple_kw_helper(compiler *c,
                                         location loc,
                                         asdl_keyword_seq *keywords,
                                         Py_ssize_t nkwelts);
static int codegen_call_helper_impl(compiler *c, location loc,
                                    int n, /* Args already pushed */
                                    asdl_expr_seq *args,
                                    PyObject *injected_arg,
                                    asdl_keyword_seq *keywords);
static int codegen_call_helper(compiler *c, location loc,
                               int n, asdl_expr_seq *args,
                               asdl_keyword_seq *keywords);
static int codegen_try_except(compiler *, stmt_ty);
static int codegen_try_star_except(compiler *, stmt_ty);

static int codegen_sync_comprehension_generator(
                                      compiler *c, location loc,
                                      asdl_comprehension_seq *generators, int gen_index,
                                      int depth,
                                      expr_ty elt, expr_ty val, int type,
                                      int iter_on_stack);

static int codegen_async_comprehension_generator(
                                      compiler *c, location loc,
                                      asdl_comprehension_seq *generators, int gen_index,
                                      int depth,
                                      expr_ty elt, expr_ty val, int type,
                                      int iter_on_stack);

static int codegen_pattern(compiler *, pattern_ty, pattern_context *);
static int codegen_match(compiler *, stmt_ty);
static int codegen_pattern_subpattern(compiler *,
                                      pattern_ty, pattern_context *);
static int codegen_make_closure(compiler *c, location loc,
                                PyCodeObject *co, Py_ssize_t flags);


/* Add an opcode with an integer argument */
static int
codegen_addop_i(instr_sequence *seq, int opcode, Py_ssize_t oparg, location loc)
{
    /* oparg value is unsigned, but a signed C int is usually used to store
       it in the C code (like Python/ceval.c).

       Limit to 32-bit signed C int (rather than INT_MAX) for portability.

       The argument of a concrete bytecode instruction is limited to 8-bit.
       EXTENDED_ARG is used for 16, 24, and 32-bit arguments. */

    int oparg_ = Py_SAFE_DOWNCAST(oparg, Py_ssize_t, int);
    assert(!IS_ASSEMBLER_OPCODE(opcode));
    return _PyInstructionSequence_Addop(seq, opcode, oparg_, loc);
}

#define ADDOP_I(C, LOC, OP, O) \
    RETURN_IF_ERROR(codegen_addop_i(INSTR_SEQUENCE(C), (OP), (O), (LOC)))

#define ADDOP_I_IN_SCOPE(C, LOC, OP, O) \
    RETURN_IF_ERROR_IN_SCOPE(C, codegen_addop_i(INSTR_SEQUENCE(C), (OP), (O), (LOC)))

static int
codegen_addop_noarg(instr_sequence *seq, int opcode, location loc)
{
    assert(!OPCODE_HAS_ARG(opcode));
    assert(!IS_ASSEMBLER_OPCODE(opcode));
    return _PyInstructionSequence_Addop(seq, opcode, 0, loc);
}

#define ADDOP(C, LOC, OP) \
    RETURN_IF_ERROR(codegen_addop_noarg(INSTR_SEQUENCE(C), (OP), (LOC)))

#define ADDOP_IN_SCOPE(C, LOC, OP) \
    RETURN_IF_ERROR_IN_SCOPE((C), codegen_addop_noarg(INSTR_SEQUENCE(C), (OP), (LOC)))

static int
codegen_addop_load_const(compiler *c, location loc, PyObject *o)
{
    Py_ssize_t arg = _PyCompile_AddConst(c, o);
    if (arg < 0) {
        return ERROR;
    }
    ADDOP_I(c, loc, LOAD_CONST, arg);
    return SUCCESS;
}

#define ADDOP_LOAD_CONST(C, LOC, O) \
    RETURN_IF_ERROR(codegen_addop_load_const((C), (LOC), (O)))

#define ADDOP_LOAD_CONST_IN_SCOPE(C, LOC, O) \
    RETURN_IF_ERROR_IN_SCOPE((C), codegen_addop_load_const((C), (LOC), (O)))

/* Same as ADDOP_LOAD_CONST, but steals a reference. */
#define ADDOP_LOAD_CONST_NEW(C, LOC, O)                                 \
    do {                                                                \
        PyObject *__new_const = (O);                                    \
        if (__new_const == NULL) {                                      \
            return ERROR;                                               \
        }                                                               \
        if (codegen_addop_load_const((C), (LOC), __new_const) < 0) {    \
            Py_DECREF(__new_const);                                     \
            return ERROR;                                               \
        }                                                               \
        Py_DECREF(__new_const);                                         \
    } while (0)

static int
codegen_addop_o(compiler *c, location loc,
                int opcode, PyObject *dict, PyObject *o)
{
    Py_ssize_t arg = _PyCompile_DictAddObj(dict, o);
    RETURN_IF_ERROR(arg);
    ADDOP_I(c, loc, opcode, arg);
    return SUCCESS;
}

#define ADDOP_N(C, LOC, OP, O, TYPE)                                    \
    do {                                                                \
        assert(!OPCODE_HAS_CONST(OP)); /* use ADDOP_LOAD_CONST_NEW */   \
        int ret = codegen_addop_o((C), (LOC), (OP),                     \
                                  METADATA(C)->u_ ## TYPE, (O));        \
        Py_DECREF((O));                                                 \
        RETURN_IF_ERROR(ret);                                           \
    } while (0)

#define ADDOP_N_IN_SCOPE(C, LOC, OP, O, TYPE)                           \
    do {                                                                \
        assert(!OPCODE_HAS_CONST(OP)); /* use ADDOP_LOAD_CONST_NEW */   \
        int ret = codegen_addop_o((C), (LOC), (OP),                     \
                                  METADATA(C)->u_ ## TYPE, (O));        \
        Py_DECREF((O));                                                 \
        RETURN_IF_ERROR_IN_SCOPE((C), ret);                             \
    } while (0)

#define LOAD_METHOD -1
#define LOAD_SUPER_METHOD -2
#define LOAD_ZERO_SUPER_ATTR -3
#define LOAD_ZERO_SUPER_METHOD -4

static int
codegen_addop_name(compiler *c, location loc,
                   int opcode, PyObject *dict, PyObject *o)
{
    PyObject *mangled = _PyCompile_MaybeMangle(c, o);
    if (!mangled) {
        return ERROR;
    }
    Py_ssize_t arg = _PyCompile_DictAddObj(dict, mangled);
    Py_DECREF(mangled);
    if (arg < 0) {
        return ERROR;
    }
    if (opcode == LOAD_ATTR) {
        arg <<= 1;
    }
    if (opcode == LOAD_METHOD) {
        opcode = LOAD_ATTR;
        arg <<= 1;
        arg |= 1;
    }
    if (opcode == LOAD_SUPER_ATTR) {
        arg <<= 2;
        arg |= 2;
    }
    if (opcode == LOAD_SUPER_METHOD) {
        opcode = LOAD_SUPER_ATTR;
        arg <<= 2;
        arg |= 3;
    }
    if (opcode == LOAD_ZERO_SUPER_ATTR) {
        opcode = LOAD_SUPER_ATTR;
        arg <<= 2;
    }
    if (opcode == LOAD_ZERO_SUPER_METHOD) {
        opcode = LOAD_SUPER_ATTR;
        arg <<= 2;
        arg |= 1;
    }
    ADDOP_I(c, loc, opcode, arg);
    return SUCCESS;
}

#define ADDOP_NAME(C, LOC, OP, O, TYPE) \
    RETURN_IF_ERROR(codegen_addop_name((C), (LOC), (OP), METADATA(C)->u_ ## TYPE, (O)))

static int
codegen_addop_j(instr_sequence *seq, location loc,
                int opcode, jump_target_label target)
{
    assert(IS_JUMP_TARGET_LABEL(target));
    assert(HAS_TARGET(opcode));
    assert(!IS_ASSEMBLER_OPCODE(opcode));
    return _PyInstructionSequence_Addop(seq, opcode, target.id, loc);
}

#define ADDOP_JUMP(C, LOC, OP, O) \
    RETURN_IF_ERROR(codegen_addop_j(INSTR_SEQUENCE(C), (LOC), (OP), (O)))

#define ADDOP_COMPARE(C, LOC, CMP) \
    RETURN_IF_ERROR(codegen_addcompare((C), (LOC), (cmpop_ty)(CMP)))

#define ADDOP_BINARY(C, LOC, BINOP) \
    RETURN_IF_ERROR(addop_binary((C), (LOC), (BINOP), false))

#define ADDOP_INPLACE(C, LOC, BINOP) \
    RETURN_IF_ERROR(addop_binary((C), (LOC), (BINOP), true))

#define ADD_YIELD_FROM(C, LOC, await) \
    RETURN_IF_ERROR(codegen_add_yield_from((C), (LOC), (await)))

#define POP_EXCEPT_AND_RERAISE(C, LOC) \
    RETURN_IF_ERROR(codegen_pop_except_and_reraise((C), (LOC)))

#define ADDOP_YIELD(C, LOC) \
    RETURN_IF_ERROR(codegen_addop_yield((C), (LOC)))

/* VISIT and VISIT_SEQ takes an ASDL type as their second argument.  They use
   the ASDL name to synthesize the name of the C type and the visit function.
*/

#define VISIT(C, TYPE, V) \
    RETURN_IF_ERROR(codegen_visit_ ## TYPE((C), (V)))

#define VISIT_IN_SCOPE(C, TYPE, V) \
    RETURN_IF_ERROR_IN_SCOPE((C), codegen_visit_ ## TYPE((C), (V)))

#define VISIT_SEQ(C, TYPE, SEQ)                                             \
    do {                                                                    \
        asdl_ ## TYPE ## _seq *seq = (SEQ); /* avoid variable capture */    \
        for (int _i = 0; _i < asdl_seq_LEN(seq); _i++) {                    \
            TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, _i);           \
            RETURN_IF_ERROR(codegen_visit_ ## TYPE((C), elt));              \
        }                                                                   \
    } while (0)

#define VISIT_SEQ_IN_SCOPE(C, TYPE, SEQ)                                    \
    do {                                                                    \
        asdl_ ## TYPE ## _seq *seq = (SEQ); /* avoid variable capture */    \
        for (int _i = 0; _i < asdl_seq_LEN(seq); _i++) {                    \
            TYPE ## _ty elt = (TYPE ## _ty)asdl_seq_GET(seq, _i);           \
            if (codegen_visit_ ## TYPE((C), elt) < 0) {                     \
                _PyCompile_ExitScope(C);                                    \
                return ERROR;                                               \
            }                                                               \
        }                                                                   \
    } while (0)

static int
codegen_call_exit_with_nones(compiler *c, location loc)
{
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADDOP_I(c, loc, CALL, 3);
    return SUCCESS;
}

static int
codegen_add_yield_from(compiler *c, location loc, int await)
{
    NEW_JUMP_TARGET_LABEL(c, send);
    NEW_JUMP_TARGET_LABEL(c, fail);
    NEW_JUMP_TARGET_LABEL(c, exit);

    USE_LABEL(c, send);
    ADDOP_JUMP(c, loc, SEND, exit);
    // Set up a virtual try/except to handle when StopIteration is raised during
    // a close or throw call. The only way YIELD_VALUE raises if they do!
    ADDOP_JUMP(c, loc, SETUP_FINALLY, fail);
    ADDOP_I(c, loc, YIELD_VALUE, 1);
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    ADDOP_I(c, loc, RESUME, await ? RESUME_AFTER_AWAIT : RESUME_AFTER_YIELD_FROM);
    ADDOP_JUMP(c, loc, JUMP_NO_INTERRUPT, send);

    USE_LABEL(c, fail);
    ADDOP(c, loc, CLEANUP_THROW);

    USE_LABEL(c, exit);
    ADDOP(c, loc, END_SEND);
    return SUCCESS;
}

static int
codegen_pop_except_and_reraise(compiler *c, location loc)
{
    /* Stack contents
     * [exc_info, lasti, exc]            COPY        3
     * [exc_info, lasti, exc, exc_info]  POP_EXCEPT
     * [exc_info, lasti, exc]            RERAISE      1
     * (exception_unwind clears the stack)
     */

    ADDOP_I(c, loc, COPY, 3);
    ADDOP(c, loc, POP_EXCEPT);
    ADDOP_I(c, loc, RERAISE, 1);
    return SUCCESS;
}

/* Unwind a frame block.  If preserve_tos is true, the TOS before
 * popping the blocks will be restored afterwards, unless another
 * return, break or continue is found. In which case, the TOS will
 * be popped.
 */
static int
codegen_unwind_fblock(compiler *c, location *ploc,
                      fblockinfo *info, int preserve_tos)
{
    switch (info->fb_type) {
        case COMPILE_FBLOCK_WHILE_LOOP:
        case COMPILE_FBLOCK_EXCEPTION_HANDLER:
        case COMPILE_FBLOCK_EXCEPTION_GROUP_HANDLER:
        case COMPILE_FBLOCK_ASYNC_COMPREHENSION_GENERATOR:
        case COMPILE_FBLOCK_STOP_ITERATION:
            return SUCCESS;

        case COMPILE_FBLOCK_FOR_LOOP:
            /* Pop the iterator */
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 3);
            }
            ADDOP(c, *ploc, POP_TOP);
            ADDOP(c, *ploc, POP_TOP);
            return SUCCESS;

        case COMPILE_FBLOCK_ASYNC_FOR_LOOP:
            /* Pop the iterator */
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            ADDOP(c, *ploc, POP_TOP);
            return SUCCESS;

        case COMPILE_FBLOCK_TRY_EXCEPT:
            ADDOP(c, *ploc, POP_BLOCK);
            return SUCCESS;

        case COMPILE_FBLOCK_FINALLY_TRY:
            /* This POP_BLOCK gets the line number of the unwinding statement */
            ADDOP(c, *ploc, POP_BLOCK);
            if (preserve_tos) {
                RETURN_IF_ERROR(
                    _PyCompile_PushFBlock(c, *ploc, COMPILE_FBLOCK_POP_VALUE,
                                          NO_LABEL, NO_LABEL, NULL));
            }
            /* Emit the finally block */
            VISIT_SEQ(c, stmt, info->fb_datum);
            if (preserve_tos) {
                _PyCompile_PopFBlock(c, COMPILE_FBLOCK_POP_VALUE, NO_LABEL);
            }
            /* The finally block should appear to execute after the
             * statement causing the unwinding, so make the unwinding
             * instruction artificial */
            *ploc = NO_LOCATION;
            return SUCCESS;

        case COMPILE_FBLOCK_FINALLY_END:
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            ADDOP(c, *ploc, POP_TOP); /* exc_value */
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            ADDOP(c, *ploc, POP_BLOCK);
            ADDOP(c, *ploc, POP_EXCEPT);
            return SUCCESS;

        case COMPILE_FBLOCK_WITH:
        case COMPILE_FBLOCK_ASYNC_WITH:
            *ploc = info->fb_loc;
            ADDOP(c, *ploc, POP_BLOCK);
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 3);
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            RETURN_IF_ERROR(codegen_call_exit_with_nones(c, *ploc));
            if (info->fb_type == COMPILE_FBLOCK_ASYNC_WITH) {
                ADDOP_I(c, *ploc, GET_AWAITABLE, 2);
                ADDOP_LOAD_CONST(c, *ploc, Py_None);
                ADD_YIELD_FROM(c, *ploc, 1);
            }
            ADDOP(c, *ploc, POP_TOP);
            /* The exit block should appear to execute after the
             * statement causing the unwinding, so make the unwinding
             * instruction artificial */
            *ploc = NO_LOCATION;
            return SUCCESS;

        case COMPILE_FBLOCK_HANDLER_CLEANUP: {
            if (info->fb_datum) {
                ADDOP(c, *ploc, POP_BLOCK);
            }
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            ADDOP(c, *ploc, POP_BLOCK);
            ADDOP(c, *ploc, POP_EXCEPT);
            if (info->fb_datum) {
                ADDOP_LOAD_CONST(c, *ploc, Py_None);
                RETURN_IF_ERROR(codegen_nameop(c, *ploc, info->fb_datum, Store));
                RETURN_IF_ERROR(codegen_nameop(c, *ploc, info->fb_datum, Del));
            }
            return SUCCESS;
        }
        case COMPILE_FBLOCK_POP_VALUE: {
            if (preserve_tos) {
                ADDOP_I(c, *ploc, SWAP, 2);
            }
            ADDOP(c, *ploc, POP_TOP);
            return SUCCESS;
        }
    }
    Py_UNREACHABLE();
}

/** Unwind block stack. If loop is not NULL, then stop when the first loop is encountered. */
static int
codegen_unwind_fblock_stack(compiler *c, location *ploc,
                            int preserve_tos, fblockinfo **loop)
{
    fblockinfo *top = _PyCompile_TopFBlock(c);
    if (top == NULL) {
        return SUCCESS;
    }
    if (top->fb_type == COMPILE_FBLOCK_EXCEPTION_GROUP_HANDLER) {
        return _PyCompile_Error(
            c, *ploc, "'break', 'continue' and 'return' cannot appear in an except* block");
    }
    if (loop != NULL && (top->fb_type == COMPILE_FBLOCK_WHILE_LOOP ||
                         top->fb_type == COMPILE_FBLOCK_FOR_LOOP ||
                         top->fb_type == COMPILE_FBLOCK_ASYNC_FOR_LOOP)) {
        *loop = top;
        return SUCCESS;
    }
    fblockinfo copy = *top;
    _PyCompile_PopFBlock(c, top->fb_type, top->fb_block);
    RETURN_IF_ERROR(codegen_unwind_fblock(c, ploc, &copy, preserve_tos));
    RETURN_IF_ERROR(codegen_unwind_fblock_stack(c, ploc, preserve_tos, loop));
    _PyCompile_PushFBlock(c, copy.fb_loc, copy.fb_type, copy.fb_block,
                          copy.fb_exit, copy.fb_datum);
    return SUCCESS;
}

static int
codegen_enter_scope(compiler *c, identifier name, int scope_type,
                    void *key, int lineno, PyObject *private,
                    _PyCompile_CodeUnitMetadata *umd)
{
    RETURN_IF_ERROR(
        _PyCompile_EnterScope(c, name, scope_type, key, lineno, private, umd));
    location loc = LOCATION(lineno, lineno, 0, 0);
    if (scope_type == COMPILE_SCOPE_MODULE) {
        loc.lineno = 0;
    }
    ADDOP_I(c, loc, RESUME, RESUME_AT_FUNC_START);
    if (scope_type == COMPILE_SCOPE_MODULE) {
        ADDOP(c, loc, ANNOTATIONS_PLACEHOLDER);
    }
    return SUCCESS;
}

static int
codegen_setup_annotations_scope(compiler *c, location loc,
                                void *key, PyObject *name)
{
    _PyCompile_CodeUnitMetadata umd = {
        .u_posonlyargcount = 1,
    };
    RETURN_IF_ERROR(
        codegen_enter_scope(c, name, COMPILE_SCOPE_ANNOTATIONS,
                            key, loc.lineno, NULL, &umd));

    // if .format > VALUE_WITH_FAKE_GLOBALS: raise NotImplementedError
    PyObject *value_with_fake_globals = PyLong_FromLong(_Py_ANNOTATE_FORMAT_VALUE_WITH_FAKE_GLOBALS);
    assert(!SYMTABLE_ENTRY(c)->ste_has_docstring);
    _Py_DECLARE_STR(format, ".format");
    ADDOP_I(c, loc, LOAD_FAST, 0);
    ADDOP_LOAD_CONST(c, loc, value_with_fake_globals);
    ADDOP_I(c, loc, COMPARE_OP, (Py_GT << 5) | compare_masks[Py_GT]);
    NEW_JUMP_TARGET_LABEL(c, body);
    ADDOP_JUMP(c, loc, POP_JUMP_IF_FALSE, body);
    ADDOP_I(c, loc, LOAD_COMMON_CONSTANT, CONSTANT_NOTIMPLEMENTEDERROR);
    ADDOP_I(c, loc, RAISE_VARARGS, 1);
    USE_LABEL(c, body);
    return SUCCESS;
}

static int
codegen_leave_annotations_scope(compiler *c, location loc)
{
    ADDOP_IN_SCOPE(c, loc, RETURN_VALUE);
    PyCodeObject *co = _PyCompile_OptimizeAndAssemble(c, 1);
    if (co == NULL) {
        return ERROR;
    }

    // We want the parameter to __annotate__ to be named "format" in the
    // signature  shown by inspect.signature(), but we need to use a
    // different name (.format) in the symtable; if the name
    // "format" appears in the annotations, it doesn't get clobbered
    // by this name.  This code is essentially:
    // co->co_localsplusnames = ("format", *co->co_localsplusnames[1:])
    const Py_ssize_t size = PyObject_Size(co->co_localsplusnames);
    if (size == -1) {
        Py_DECREF(co);
        return ERROR;
    }
    PyObject *new_names = PyTuple_New(size);
    if (new_names == NULL) {
        Py_DECREF(co);
        return ERROR;
    }
    PyTuple_SET_ITEM(new_names, 0, Py_NewRef(&_Py_ID(format)));
    for (int i = 1; i < size; i++) {
        PyObject *item = PyTuple_GetItem(co->co_localsplusnames, i);
        if (item == NULL) {
            Py_DECREF(co);
            Py_DECREF(new_names);
            return ERROR;
        }
        Py_INCREF(item);
        PyTuple_SET_ITEM(new_names, i, item);
    }
    Py_SETREF(co->co_localsplusnames, new_names);

    _PyCompile_ExitScope(c);
    int ret = codegen_make_closure(c, loc, co, 0);
    Py_DECREF(co);
    RETURN_IF_ERROR(ret);
    return SUCCESS;
}

static int
codegen_deferred_annotations_body(compiler *c, location loc,
    PyObject *deferred_anno, PyObject *conditional_annotation_indices, int scope_type)
{
    Py_ssize_t annotations_len = PyList_GET_SIZE(deferred_anno);

    assert(PyList_CheckExact(conditional_annotation_indices));
    assert(annotations_len == PyList_Size(conditional_annotation_indices));

    ADDOP_I(c, loc, BUILD_MAP, 0); // stack now contains <annos>

    for (Py_ssize_t i = 0; i < annotations_len; i++) {
        PyObject *ptr = PyList_GET_ITEM(deferred_anno, i);
        stmt_ty st = (stmt_ty)PyLong_AsVoidPtr(ptr);
        if (st == NULL) {
            return ERROR;
        }
        PyObject *mangled = _PyCompile_Mangle(c, st->v.AnnAssign.target->v.Name.id);
        if (!mangled) {
            return ERROR;
        }
        PyObject *cond_index = PyList_GET_ITEM(conditional_annotation_indices, i);
        assert(PyLong_CheckExact(cond_index));
        long idx = PyLong_AS_LONG(cond_index);
        NEW_JUMP_TARGET_LABEL(c, not_set);

        if (idx != -1) {
            ADDOP_LOAD_CONST(c, LOC(st), cond_index);
            if (scope_type == COMPILE_SCOPE_CLASS) {
                ADDOP_NAME(
                    c, LOC(st), LOAD_DEREF, &_Py_ID(__conditional_annotations__), freevars);
            }
            else {
                ADDOP_NAME(
                    c, LOC(st), LOAD_GLOBAL, &_Py_ID(__conditional_annotations__), names);
            }

            ADDOP_I(c, LOC(st), CONTAINS_OP, 0);
            ADDOP_JUMP(c, LOC(st), POP_JUMP_IF_FALSE, not_set);
        }

        VISIT(c, expr, st->v.AnnAssign.annotation);
        ADDOP_I(c, LOC(st), COPY, 2);
        ADDOP_LOAD_CONST_NEW(c, LOC(st), mangled);
        // stack now contains <annos> <name> <annos> <value>
        ADDOP(c, loc, STORE_SUBSCR);
        // stack now contains <annos>

        USE_LABEL(c, not_set);
    }
    return SUCCESS;
}

static int
codegen_process_deferred_annotations(compiler *c, location loc)
{
    PyObject *deferred_anno = NULL;
    PyObject *conditional_annotation_indices = NULL;
    _PyCompile_DeferredAnnotations(c, &deferred_anno, &conditional_annotation_indices);
    if (deferred_anno == NULL) {
        assert(conditional_annotation_indices == NULL);
        return SUCCESS;
    }

    int scope_type = SCOPE_TYPE(c);
    bool need_separate_block = scope_type == COMPILE_SCOPE_MODULE;
    if (need_separate_block) {
        if (_PyCompile_StartAnnotationSetup(c) == ERROR) {
            goto error;
        }
    }

    // It's possible that ste_annotations_block is set but
    // u_deferred_annotations is not, because the former is still
    // set if there are only non-simple annotations (i.e., annotations
    // for attributes, subscripts, or parenthesized names). However, the
    // reverse should not be possible.
    PySTEntryObject *ste = SYMTABLE_ENTRY(c);
    assert(ste->ste_annotation_block != NULL);
    void *key = (void *)((uintptr_t)ste->ste_id + 1);
    if (codegen_setup_annotations_scope(c, loc, key,
                                        ste->ste_annotation_block->ste_name) < 0) {
        goto error;
    }
    if (codegen_deferred_annotations_body(c, loc, deferred_anno,
                                          conditional_annotation_indices, scope_type) < 0) {
        _PyCompile_ExitScope(c);
        goto error;
    }

    Py_DECREF(deferred_anno);
    Py_DECREF(conditional_annotation_indices);

    RETURN_IF_ERROR(codegen_leave_annotations_scope(c, loc));
    RETURN_IF_ERROR(codegen_nameop(
        c, loc,
        ste->ste_type == ClassBlock ? &_Py_ID(__annotate_func__) : &_Py_ID(__annotate__),
        Store));

    if (need_separate_block) {
        RETURN_IF_ERROR(_PyCompile_EndAnnotationSetup(c));
    }

    return SUCCESS;
error:
    Py_XDECREF(deferred_anno);
    Py_XDECREF(conditional_annotation_indices);
    return ERROR;
}

/* Compile an expression */
int
_PyCodegen_Expression(compiler *c, expr_ty e)
{
    VISIT(c, expr, e);
    return SUCCESS;
}

/* Compile a sequence of statements, checking for a docstring
   and for annotations. */

int
_PyCodegen_Module(compiler *c, location loc, asdl_stmt_seq *stmts, bool is_interactive)
{
    if (SYMTABLE_ENTRY(c)->ste_has_conditional_annotations) {
        ADDOP_I(c, loc, BUILD_SET, 0);
        ADDOP_N(c, loc, STORE_NAME, &_Py_ID(__conditional_annotations__), names);
    }
    return codegen_body(c, loc, stmts, is_interactive);
}

int
codegen_body(compiler *c, location loc, asdl_stmt_seq *stmts, bool is_interactive)
{
    /* If from __future__ import annotations is active,
     * every annotated class and module should have __annotations__.
     * Else __annotate__ is created when necessary. */
    PySTEntryObject *ste = SYMTABLE_ENTRY(c);
    if ((FUTURE_FEATURES(c) & CO_FUTURE_ANNOTATIONS) && ste->ste_annotations_used) {
        ADDOP(c, loc, SETUP_ANNOTATIONS);
    }
    if (!asdl_seq_LEN(stmts)) {
        return SUCCESS;
    }
    Py_ssize_t first_instr = 0;
    if (!is_interactive) { /* A string literal on REPL prompt is not a docstring */
        if (ste->ste_has_docstring) {
            PyObject *docstring = _PyAST_GetDocString(stmts);
            assert(docstring);
            first_instr = 1;
            /* set docstring */
            assert(OPTIMIZATION_LEVEL(c) < 2);
            PyObject *cleandoc = _PyCompile_CleanDoc(docstring);
            if (cleandoc == NULL) {
                return ERROR;
            }
            stmt_ty st = (stmt_ty)asdl_seq_GET(stmts, 0);
            assert(st->kind == Expr_kind);
            location loc = LOC(st->v.Expr.value);
            ADDOP_LOAD_CONST(c, loc, cleandoc);
            Py_DECREF(cleandoc);
            RETURN_IF_ERROR(codegen_nameop(c, NO_LOCATION, &_Py_ID(__doc__), Store));
        }
    }
    for (Py_ssize_t i = first_instr; i < asdl_seq_LEN(stmts); i++) {
        VISIT(c, stmt, (stmt_ty)asdl_seq_GET(stmts, i));
    }
    // If there are annotations and the future import is not on, we
    // collect the annotations in a separate pass and generate an
    // __annotate__ function. See PEP 649.
    if (!(FUTURE_FEATURES(c) & CO_FUTURE_ANNOTATIONS)) {
        RETURN_IF_ERROR(codegen_process_deferred_annotations(c, loc));
    }
    return SUCCESS;
}

int
_PyCodegen_EnterAnonymousScope(compiler* c, mod_ty mod)
{
    _Py_DECLARE_STR(anon_module, "<module>");
    RETURN_IF_ERROR(
        codegen_enter_scope(c, &_Py_STR(anon_module), COMPILE_SCOPE_MODULE,
                            mod, 1, NULL, NULL));
    return SUCCESS;
}

static int
codegen_make_closure(compiler *c, location loc,
                     PyCodeObject *co, Py_ssize_t flags)
{
    if (co->co_nfreevars) {
        int i = PyUnstable_Code_GetFirstFree(co);
        for (; i < co->co_nlocalsplus; ++i) {
            /* Bypass com_addop_varname because it will generate
               LOAD_DEREF but LOAD_CLOSURE is needed.
            */
            PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, i);
            int arg = _PyCompile_LookupArg(c, co, name);
            RETURN_IF_ERROR(arg);
            ADDOP_I(c, loc, LOAD_CLOSURE, arg);
        }
        flags |= MAKE_FUNCTION_CLOSURE;
        ADDOP_I(c, loc, BUILD_TUPLE, co->co_nfreevars);
    }
    ADDOP_LOAD_CONST(c, loc, (PyObject*)co);

    ADDOP(c, loc, MAKE_FUNCTION);

    if (flags & MAKE_FUNCTION_CLOSURE) {
        ADDOP_I(c, loc, SET_FUNCTION_ATTRIBUTE, MAKE_FUNCTION_CLOSURE);
    }
    if (flags & MAKE_FUNCTION_ANNOTATIONS) {
        ADDOP_I(c, loc, SET_FUNCTION_ATTRIBUTE, MAKE_FUNCTION_ANNOTATIONS);
    }
    if (flags & MAKE_FUNCTION_ANNOTATE) {
        ADDOP_I(c, loc, SET_FUNCTION_ATTRIBUTE, MAKE_FUNCTION_ANNOTATE);
    }
    if (flags & MAKE_FUNCTION_KWDEFAULTS) {
        ADDOP_I(c, loc, SET_FUNCTION_ATTRIBUTE, MAKE_FUNCTION_KWDEFAULTS);
    }
    if (flags & MAKE_FUNCTION_DEFAULTS) {
        ADDOP_I(c, loc, SET_FUNCTION_ATTRIBUTE, MAKE_FUNCTION_DEFAULTS);
    }
    return SUCCESS;
}

static int
codegen_decorators(compiler *c, asdl_expr_seq* decos)
{
    if (!decos) {
        return SUCCESS;
    }

    for (Py_ssize_t i = 0; i < asdl_seq_LEN(decos); i++) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(decos, i));
    }
    return SUCCESS;
}

static int
codegen_apply_decorators(compiler *c, asdl_expr_seq* decos)
{
    if (!decos) {
        return SUCCESS;
    }

    for (Py_ssize_t i = asdl_seq_LEN(decos) - 1; i > -1; i--) {
        location loc = LOC((expr_ty)asdl_seq_GET(decos, i));
        ADDOP_I(c, loc, CALL, 0);
    }
    return SUCCESS;
}

static int
codegen_kwonlydefaults(compiler *c, location loc,
                       asdl_arg_seq *kwonlyargs, asdl_expr_seq *kw_defaults)
{
    /* Push a dict of keyword-only default values.

       Return -1 on error, 0 if no dict pushed, 1 if a dict is pushed.
       */
    int default_count = 0;
    for (int i = 0; i < asdl_seq_LEN(kwonlyargs); i++) {
        arg_ty arg = asdl_seq_GET(kwonlyargs, i);
        expr_ty default_ = asdl_seq_GET(kw_defaults, i);
        if (default_) {
            default_count++;
            PyObject *mangled = _PyCompile_MaybeMangle(c, arg->arg);
            if (!mangled) {
                return ERROR;
            }
            ADDOP_LOAD_CONST_NEW(c, loc, mangled);
            VISIT(c, expr, default_);
        }
    }
    if (default_count) {
        ADDOP_I(c, loc, BUILD_MAP, default_count);
        return 1;
    }
    else {
        return 0;
    }
}

static int
codegen_visit_annexpr(compiler *c, expr_ty annotation)
{
    location loc = LOC(annotation);
    ADDOP_LOAD_CONST_NEW(c, loc, _PyAST_ExprAsUnicode(annotation));
    return SUCCESS;
}

static int
codegen_argannotation(compiler *c, identifier id,
    expr_ty annotation, Py_ssize_t *annotations_len, location loc)
{
    if (!annotation) {
        return SUCCESS;
    }
    PyObject *mangled = _PyCompile_MaybeMangle(c, id);
    if (!mangled) {
        return ERROR;
    }
    ADDOP_LOAD_CONST(c, loc, mangled);
    Py_DECREF(mangled);

    if (FUTURE_FEATURES(c) & CO_FUTURE_ANNOTATIONS) {
        VISIT(c, annexpr, annotation);
    }
    else {
        if (annotation->kind == Starred_kind) {
            // *args: *Ts (where Ts is a TypeVarTuple).
            // Do [annotation_value] = [*Ts].
            // (Note that in theory we could end up here even for an argument
            // other than *args, but in practice the grammar doesn't allow it.)
            VISIT(c, expr, annotation->v.Starred.value);
            ADDOP_I(c, loc, UNPACK_SEQUENCE, (Py_ssize_t) 1);
        }
        else {
            VISIT(c, expr, annotation);
        }
    }
    *annotations_len += 1;
    return SUCCESS;
}

static int
codegen_argannotations(compiler *c, asdl_arg_seq* args,
                       Py_ssize_t *annotations_len, location loc)
{
    int i;
    for (i = 0; i < asdl_seq_LEN(args); i++) {
        arg_ty arg = (arg_ty)asdl_seq_GET(args, i);
        RETURN_IF_ERROR(
            codegen_argannotation(
                        c,
                        arg->arg,
                        arg->annotation,
                        annotations_len,
                        loc));
    }
    return SUCCESS;
}

static int
codegen_annotations_in_scope(compiler *c, location loc,
                             arguments_ty args, expr_ty returns,
                             Py_ssize_t *annotations_len)
{
    RETURN_IF_ERROR(
        codegen_argannotations(c, args->args, annotations_len, loc));

    RETURN_IF_ERROR(
        codegen_argannotations(c, args->posonlyargs, annotations_len, loc));

    if (args->vararg && args->vararg->annotation) {
        RETURN_IF_ERROR(
            codegen_argannotation(c, args->vararg->arg,
                                     args->vararg->annotation, annotations_len, loc));
    }

    RETURN_IF_ERROR(
        codegen_argannotations(c, args->kwonlyargs, annotations_len, loc));

    if (args->kwarg && args->kwarg->annotation) {
        RETURN_IF_ERROR(
            codegen_argannotation(c, args->kwarg->arg,
                                     args->kwarg->annotation, annotations_len, loc));
    }

    RETURN_IF_ERROR(
        codegen_argannotation(c, &_Py_ID(return), returns, annotations_len, loc));

    return 0;
}

static int
codegen_function_annotations(compiler *c, location loc,
                             arguments_ty args, expr_ty returns)
{
    /* Push arg annotation names and values.
       The expressions are evaluated separately from the rest of the source code.

       Return -1 on error, or a combination of flags to add to the function.
       */
    Py_ssize_t annotations_len = 0;

    PySTEntryObject *ste;
    RETURN_IF_ERROR(_PySymtable_LookupOptional(SYMTABLE(c), args, &ste));
    assert(ste != NULL);

    if (ste->ste_annotations_used) {
        int err = codegen_setup_annotations_scope(c, loc, (void *)args, ste->ste_name);
        Py_DECREF(ste);
        RETURN_IF_ERROR(err);
        RETURN_IF_ERROR_IN_SCOPE(
            c, codegen_annotations_in_scope(c, loc, args, returns, &annotations_len)
        );
        ADDOP_I(c, loc, BUILD_MAP, annotations_len);
        RETURN_IF_ERROR(codegen_leave_annotations_scope(c, loc));
        return MAKE_FUNCTION_ANNOTATE;
    }
    else {
        Py_DECREF(ste);
    }

    return 0;
}

static int
codegen_defaults(compiler *c, arguments_ty args,
                        location loc)
{
    VISIT_SEQ(c, expr, args->defaults);
    ADDOP_I(c, loc, BUILD_TUPLE, asdl_seq_LEN(args->defaults));
    return SUCCESS;
}

static Py_ssize_t
codegen_default_arguments(compiler *c, location loc,
                          arguments_ty args)
{
    Py_ssize_t funcflags = 0;
    if (args->defaults && asdl_seq_LEN(args->defaults) > 0) {
        RETURN_IF_ERROR(codegen_defaults(c, args, loc));
        funcflags |= MAKE_FUNCTION_DEFAULTS;
    }
    if (args->kwonlyargs) {
        int res = codegen_kwonlydefaults(c, loc,
                                         args->kwonlyargs,
                                         args->kw_defaults);
        RETURN_IF_ERROR(res);
        if (res > 0) {
            funcflags |= MAKE_FUNCTION_KWDEFAULTS;
        }
    }
    return funcflags;
}

static int
codegen_wrap_in_stopiteration_handler(compiler *c)
{
    NEW_JUMP_TARGET_LABEL(c, handler);

    /* Insert SETUP_CLEANUP at start */
    RETURN_IF_ERROR(
        _PyInstructionSequence_InsertInstruction(
            INSTR_SEQUENCE(c), 0,
            SETUP_CLEANUP, handler.id, NO_LOCATION));

    ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
    ADDOP(c, NO_LOCATION, RETURN_VALUE);
    USE_LABEL(c, handler);
    ADDOP_I(c, NO_LOCATION, CALL_INTRINSIC_1, INTRINSIC_STOPITERATION_ERROR);
    ADDOP_I(c, NO_LOCATION, RERAISE, 1);
    return SUCCESS;
}

static int
codegen_type_param_bound_or_default(compiler *c, expr_ty e,
                                    identifier name, void *key,
                                    bool allow_starred)
{
    PyObject *defaults = PyTuple_Pack(1, _PyLong_GetOne());
    ADDOP_LOAD_CONST_NEW(c, LOC(e), defaults);
    RETURN_IF_ERROR(codegen_setup_annotations_scope(c, LOC(e), key, name));
    if (allow_starred && e->kind == Starred_kind) {
        VISIT(c, expr, e->v.Starred.value);
        ADDOP_I(c, LOC(e), UNPACK_SEQUENCE, (Py_ssize_t)1);
    }
    else {
        VISIT(c, expr, e);
    }
    ADDOP_IN_SCOPE(c, LOC(e), RETURN_VALUE);
    PyCodeObject *co = _PyCompile_OptimizeAndAssemble(c, 1);
    _PyCompile_ExitScope(c);
    if (co == NULL) {
        return ERROR;
    }
    int ret = codegen_make_closure(c, LOC(e), co, MAKE_FUNCTION_DEFAULTS);
    Py_DECREF(co);
    RETURN_IF_ERROR(ret);
    return SUCCESS;
}

static int
codegen_type_params(compiler *c, asdl_type_param_seq *type_params)
{
    if (!type_params) {
        return SUCCESS;
    }
    Py_ssize_t n = asdl_seq_LEN(type_params);
    bool seen_default = false;

    for (Py_ssize_t i = 0; i < n; i++) {
        type_param_ty typeparam = asdl_seq_GET(type_params, i);
        location loc = LOC(typeparam);
        switch(typeparam->kind) {
        case TypeVar_kind:
            ADDOP_LOAD_CONST(c, loc, typeparam->v.TypeVar.name);
            if (typeparam->v.TypeVar.bound) {
                expr_ty bound = typeparam->v.TypeVar.bound;
                RETURN_IF_ERROR(
                    codegen_type_param_bound_or_default(c, bound, typeparam->v.TypeVar.name,
                                                        (void *)typeparam, false));

                int intrinsic = bound->kind == Tuple_kind
                    ? INTRINSIC_TYPEVAR_WITH_CONSTRAINTS
                    : INTRINSIC_TYPEVAR_WITH_BOUND;
                ADDOP_I(c, loc, CALL_INTRINSIC_2, intrinsic);
            }
            else {
                ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_TYPEVAR);
            }
            if (typeparam->v.TypeVar.default_value) {
                seen_default = true;
                expr_ty default_ = typeparam->v.TypeVar.default_value;
                RETURN_IF_ERROR(
                    codegen_type_param_bound_or_default(c, default_, typeparam->v.TypeVar.name,
                                                        (void *)((uintptr_t)typeparam + 1), false));
                ADDOP_I(c, loc, CALL_INTRINSIC_2, INTRINSIC_SET_TYPEPARAM_DEFAULT);
            }
            else if (seen_default) {
                return _PyCompile_Error(c, loc, "non-default type parameter '%U' "
                                        "follows default type parameter",
                                        typeparam->v.TypeVar.name);
            }
            ADDOP_I(c, loc, COPY, 1);
            RETURN_IF_ERROR(codegen_nameop(c, loc, typeparam->v.TypeVar.name, Store));
            break;
        case TypeVarTuple_kind:
            ADDOP_LOAD_CONST(c, loc, typeparam->v.TypeVarTuple.name);
            ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_TYPEVARTUPLE);
            if (typeparam->v.TypeVarTuple.default_value) {
                expr_ty default_ = typeparam->v.TypeVarTuple.default_value;
                RETURN_IF_ERROR(
                    codegen_type_param_bound_or_default(c, default_, typeparam->v.TypeVarTuple.name,
                                                        (void *)typeparam, true));
                ADDOP_I(c, loc, CALL_INTRINSIC_2, INTRINSIC_SET_TYPEPARAM_DEFAULT);
                seen_default = true;
            }
            else if (seen_default) {
                return _PyCompile_Error(c, loc, "non-default type parameter '%U' "
                                        "follows default type parameter",
                                        typeparam->v.TypeVarTuple.name);
            }
            ADDOP_I(c, loc, COPY, 1);
            RETURN_IF_ERROR(codegen_nameop(c, loc, typeparam->v.TypeVarTuple.name, Store));
            break;
        case ParamSpec_kind:
            ADDOP_LOAD_CONST(c, loc, typeparam->v.ParamSpec.name);
            ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_PARAMSPEC);
            if (typeparam->v.ParamSpec.default_value) {
                expr_ty default_ = typeparam->v.ParamSpec.default_value;
                RETURN_IF_ERROR(
                    codegen_type_param_bound_or_default(c, default_, typeparam->v.ParamSpec.name,
                                                        (void *)typeparam, false));
                ADDOP_I(c, loc, CALL_INTRINSIC_2, INTRINSIC_SET_TYPEPARAM_DEFAULT);
                seen_default = true;
            }
            else if (seen_default) {
                return _PyCompile_Error(c, loc, "non-default type parameter '%U' "
                                        "follows default type parameter",
                                        typeparam->v.ParamSpec.name);
            }
            ADDOP_I(c, loc, COPY, 1);
            RETURN_IF_ERROR(codegen_nameop(c, loc, typeparam->v.ParamSpec.name, Store));
            break;
        }
    }
    ADDOP_I(c, LOC(asdl_seq_GET(type_params, 0)), BUILD_TUPLE, n);
    return SUCCESS;
}

static int
codegen_function_body(compiler *c, stmt_ty s, int is_async, Py_ssize_t funcflags,
                      int firstlineno)
{
    arguments_ty args;
    identifier name;
    asdl_stmt_seq *body;
    int scope_type;

    if (is_async) {
        assert(s->kind == AsyncFunctionDef_kind);

        args = s->v.AsyncFunctionDef.args;
        name = s->v.AsyncFunctionDef.name;
        body = s->v.AsyncFunctionDef.body;

        scope_type = COMPILE_SCOPE_ASYNC_FUNCTION;
    } else {
        assert(s->kind == FunctionDef_kind);

        args = s->v.FunctionDef.args;
        name = s->v.FunctionDef.name;
        body = s->v.FunctionDef.body;

        scope_type = COMPILE_SCOPE_FUNCTION;
    }

    _PyCompile_CodeUnitMetadata umd = {
        .u_argcount = asdl_seq_LEN(args->args),
        .u_posonlyargcount = asdl_seq_LEN(args->posonlyargs),
        .u_kwonlyargcount = asdl_seq_LEN(args->kwonlyargs),
    };
    RETURN_IF_ERROR(
        codegen_enter_scope(c, name, scope_type, (void *)s, firstlineno, NULL, &umd));

    PySTEntryObject *ste = SYMTABLE_ENTRY(c);
    Py_ssize_t first_instr = 0;
    if (ste->ste_has_docstring) {
        PyObject *docstring = _PyAST_GetDocString(body);
        assert(docstring);
        first_instr = 1;
        docstring = _PyCompile_CleanDoc(docstring);
        if (docstring == NULL) {
            _PyCompile_ExitScope(c);
            return ERROR;
        }
        Py_ssize_t idx = _PyCompile_AddConst(c, docstring);
        Py_DECREF(docstring);
        RETURN_IF_ERROR_IN_SCOPE(c, idx < 0 ? ERROR : SUCCESS);
    }

    NEW_JUMP_TARGET_LABEL(c, start);
    USE_LABEL(c, start);
    bool add_stopiteration_handler = ste->ste_coroutine || ste->ste_generator;
    if (add_stopiteration_handler) {
        /* codegen_wrap_in_stopiteration_handler will push a block, so we need to account for that */
        RETURN_IF_ERROR(
            _PyCompile_PushFBlock(c, NO_LOCATION, COMPILE_FBLOCK_STOP_ITERATION,
                                  start, NO_LABEL, NULL));
    }

    for (Py_ssize_t i = first_instr; i < asdl_seq_LEN(body); i++) {
        VISIT_IN_SCOPE(c, stmt, (stmt_ty)asdl_seq_GET(body, i));
    }
    if (add_stopiteration_handler) {
        RETURN_IF_ERROR_IN_SCOPE(c, codegen_wrap_in_stopiteration_handler(c));
        _PyCompile_PopFBlock(c, COMPILE_FBLOCK_STOP_ITERATION, start);
    }
    PyCodeObject *co = _PyCompile_OptimizeAndAssemble(c, 1);
    _PyCompile_ExitScope(c);
    if (co == NULL) {
        Py_XDECREF(co);
        return ERROR;
    }
    int ret = codegen_make_closure(c, LOC(s), co, funcflags);
    Py_DECREF(co);
    return ret;
}

static int
codegen_function(compiler *c, stmt_ty s, int is_async)
{
    arguments_ty args;
    expr_ty returns;
    identifier name;
    asdl_expr_seq *decos;
    asdl_type_param_seq *type_params;
    Py_ssize_t funcflags;
    int firstlineno;

    if (is_async) {
        assert(s->kind == AsyncFunctionDef_kind);

        args = s->v.AsyncFunctionDef.args;
        returns = s->v.AsyncFunctionDef.returns;
        decos = s->v.AsyncFunctionDef.decorator_list;
        name = s->v.AsyncFunctionDef.name;
        type_params = s->v.AsyncFunctionDef.type_params;
    } else {
        assert(s->kind == FunctionDef_kind);

        args = s->v.FunctionDef.args;
        returns = s->v.FunctionDef.returns;
        decos = s->v.FunctionDef.decorator_list;
        name = s->v.FunctionDef.name;
        type_params = s->v.FunctionDef.type_params;
    }

    RETURN_IF_ERROR(codegen_decorators(c, decos));

    firstlineno = s->lineno;
    if (asdl_seq_LEN(decos)) {
        firstlineno = ((expr_ty)asdl_seq_GET(decos, 0))->lineno;
    }

    location loc = LOC(s);

    int is_generic = asdl_seq_LEN(type_params) > 0;

    funcflags = codegen_default_arguments(c, loc, args);
    RETURN_IF_ERROR(funcflags);

    int num_typeparam_args = 0;

    if (is_generic) {
        if (funcflags & MAKE_FUNCTION_DEFAULTS) {
            num_typeparam_args += 1;
        }
        if (funcflags & MAKE_FUNCTION_KWDEFAULTS) {
            num_typeparam_args += 1;
        }
        if (num_typeparam_args == 2) {
            ADDOP_I(c, loc, SWAP, 2);
        }
        PyObject *type_params_name = PyUnicode_FromFormat("<generic parameters of %U>", name);
        if (!type_params_name) {
            return ERROR;
        }
        _PyCompile_CodeUnitMetadata umd = {
            .u_argcount = num_typeparam_args,
        };
        int ret = codegen_enter_scope(c, type_params_name, COMPILE_SCOPE_ANNOTATIONS,
                                      (void *)type_params, firstlineno, NULL, &umd);
        Py_DECREF(type_params_name);
        RETURN_IF_ERROR(ret);
        RETURN_IF_ERROR_IN_SCOPE(c, codegen_type_params(c, type_params));
        for (int i = 0; i < num_typeparam_args; i++) {
            ADDOP_I_IN_SCOPE(c, loc, LOAD_FAST, i);
        }
    }

    int annotations_flag = codegen_function_annotations(c, loc, args, returns);
    if (annotations_flag < 0) {
        if (is_generic) {
            _PyCompile_ExitScope(c);
        }
        return ERROR;
    }
    funcflags |= annotations_flag;

    int ret = codegen_function_body(c, s, is_async, funcflags, firstlineno);
    if (is_generic) {
        RETURN_IF_ERROR_IN_SCOPE(c, ret);
    }
    else {
        RETURN_IF_ERROR(ret);
    }

    if (is_generic) {
        ADDOP_I_IN_SCOPE(c, loc, SWAP, 2);
        ADDOP_I_IN_SCOPE(c, loc, CALL_INTRINSIC_2, INTRINSIC_SET_FUNCTION_TYPE_PARAMS);

        PyCodeObject *co = _PyCompile_OptimizeAndAssemble(c, 0);
        _PyCompile_ExitScope(c);
        if (co == NULL) {
            return ERROR;
        }
        int ret = codegen_make_closure(c, loc, co, 0);
        Py_DECREF(co);
        RETURN_IF_ERROR(ret);
        if (num_typeparam_args > 0) {
            ADDOP_I(c, loc, SWAP, num_typeparam_args + 1);
            ADDOP_I(c, loc, CALL, num_typeparam_args - 1);
        }
        else {
            ADDOP(c, loc, PUSH_NULL);
            ADDOP_I(c, loc, CALL, 0);
        }
    }

    RETURN_IF_ERROR(codegen_apply_decorators(c, decos));
    return codegen_nameop(c, loc, name, Store);
}

static int
codegen_set_type_params_in_class(compiler *c, location loc)
{
    _Py_DECLARE_STR(type_params, ".type_params");
    RETURN_IF_ERROR(codegen_nameop(c, loc, &_Py_STR(type_params), Load));
    RETURN_IF_ERROR(codegen_nameop(c, loc, &_Py_ID(__type_params__), Store));
    return SUCCESS;
}


static int
codegen_class_body(compiler *c, stmt_ty s, int firstlineno)
{
    /* ultimately generate code for:
         <name> = __build_class__(<func>, <name>, *<bases>, **<keywords>)
       where:
         <func> is a zero arg function/closure created from the class body.
            It mutates its locals to build the class namespace.
         <name> is the class name
         <bases> is the positional arguments and *varargs argument
         <keywords> is the keyword arguments and **kwds argument
       This borrows from codegen_call.
    */

    /* 1. compile the class body into a code object */
    RETURN_IF_ERROR(
        codegen_enter_scope(c, s->v.ClassDef.name, COMPILE_SCOPE_CLASS,
                            (void *)s, firstlineno, s->v.ClassDef.name, NULL));

    location loc = LOCATION(firstlineno, firstlineno, 0, 0);
    /* load (global) __name__ ... */
    RETURN_IF_ERROR_IN_SCOPE(c, codegen_nameop(c, loc, &_Py_ID(__name__), Load));
    /* ... and store it as __module__ */
    RETURN_IF_ERROR_IN_SCOPE(c, codegen_nameop(c, loc, &_Py_ID(__module__), Store));
    ADDOP_LOAD_CONST(c, loc, QUALNAME(c));
    RETURN_IF_ERROR_IN_SCOPE(c, codegen_nameop(c, loc, &_Py_ID(__qualname__), Store));
    ADDOP_LOAD_CONST_NEW(c, loc, PyLong_FromLong(METADATA(c)->u_firstlineno));
    RETURN_IF_ERROR_IN_SCOPE(c, codegen_nameop(c, loc, &_Py_ID(__firstlineno__), Store));
    asdl_type_param_seq *type_params = s->v.ClassDef.type_params;
    if (asdl_seq_LEN(type_params) > 0) {
        RETURN_IF_ERROR_IN_SCOPE(c, codegen_set_type_params_in_class(c, loc));
    }
    if (SYMTABLE_ENTRY(c)->ste_needs_classdict) {
        ADDOP(c, loc, LOAD_LOCALS);

        // We can't use codegen_nameop here because we need to generate a
        // STORE_DEREF in a class namespace, and codegen_nameop() won't do
        // that by default.
        ADDOP_N_IN_SCOPE(c, loc, STORE_DEREF, &_Py_ID(__classdict__), cellvars);
    }
    if (SYMTABLE_ENTRY(c)->ste_has_conditional_annotations) {
        ADDOP_I(c, loc, BUILD_SET, 0);
        ADDOP_N_IN_SCOPE(c, loc, STORE_DEREF, &_Py_ID(__conditional_annotations__), cellvars);
    }
    /* compile the body proper */
    RETURN_IF_ERROR_IN_SCOPE(c, codegen_body(c, loc, s->v.ClassDef.body, false));
    PyObject *static_attributes = _PyCompile_StaticAttributesAsTuple(c);
    if (static_attributes == NULL) {
        _PyCompile_ExitScope(c);
        return ERROR;
    }
    ADDOP_LOAD_CONST(c, NO_LOCATION, static_attributes);
    Py_CLEAR(static_attributes);
    RETURN_IF_ERROR_IN_SCOPE(
        c, codegen_nameop(c, NO_LOCATION, &_Py_ID(__static_attributes__), Store));
    /* The following code is artificial */
    /* Set __classdictcell__ if necessary */
    if (SYMTABLE_ENTRY(c)->ste_needs_classdict) {
        /* Store __classdictcell__ into class namespace */
        int i = _PyCompile_LookupCellvar(c, &_Py_ID(__classdict__));
        RETURN_IF_ERROR_IN_SCOPE(c, i);
        ADDOP_I(c, NO_LOCATION, LOAD_CLOSURE, i);
        RETURN_IF_ERROR_IN_SCOPE(
            c, codegen_nameop(c, NO_LOCATION, &_Py_ID(__classdictcell__), Store));
    }
    /* Return __classcell__ if it is referenced, otherwise return None */
    if (SYMTABLE_ENTRY(c)->ste_needs_class_closure) {
        /* Store __classcell__ into class namespace & return it */
        int i = _PyCompile_LookupCellvar(c, &_Py_ID(__class__));
        RETURN_IF_ERROR_IN_SCOPE(c, i);
        ADDOP_I(c, NO_LOCATION, LOAD_CLOSURE, i);
        ADDOP_I(c, NO_LOCATION, COPY, 1);
        RETURN_IF_ERROR_IN_SCOPE(
            c, codegen_nameop(c, NO_LOCATION, &_Py_ID(__classcell__), Store));
    }
    else {
        /* No methods referenced __class__, so just return None */
        ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
    }
    ADDOP_IN_SCOPE(c, NO_LOCATION, RETURN_VALUE);
    /* create the code object */
    PyCodeObject *co = _PyCompile_OptimizeAndAssemble(c, 1);

    /* leave the new scope */
    _PyCompile_ExitScope(c);
    if (co == NULL) {
        return ERROR;
    }

    /* 2. load the 'build_class' function */

    // these instructions should be attributed to the class line,
    // not a decorator line
    loc = LOC(s);
    ADDOP(c, loc, LOAD_BUILD_CLASS);
    ADDOP(c, loc, PUSH_NULL);

    /* 3. load a function (or closure) made from the code object */
    int ret = codegen_make_closure(c, loc, co, 0);
    Py_DECREF(co);
    RETURN_IF_ERROR(ret);

    /* 4. load class name */
    ADDOP_LOAD_CONST(c, loc, s->v.ClassDef.name);

    return SUCCESS;
}

static int
codegen_class(compiler *c, stmt_ty s)
{
    asdl_expr_seq *decos = s->v.ClassDef.decorator_list;

    RETURN_IF_ERROR(codegen_decorators(c, decos));

    int firstlineno = s->lineno;
    if (asdl_seq_LEN(decos)) {
        firstlineno = ((expr_ty)asdl_seq_GET(decos, 0))->lineno;
    }
    location loc = LOC(s);

    asdl_type_param_seq *type_params = s->v.ClassDef.type_params;
    int is_generic = asdl_seq_LEN(type_params) > 0;
    if (is_generic) {
        PyObject *type_params_name = PyUnicode_FromFormat("<generic parameters of %U>",
                                                         s->v.ClassDef.name);
        if (!type_params_name) {
            return ERROR;
        }
        int ret = codegen_enter_scope(c, type_params_name, COMPILE_SCOPE_ANNOTATIONS,
                                      (void *)type_params, firstlineno, s->v.ClassDef.name, NULL);
        Py_DECREF(type_params_name);
        RETURN_IF_ERROR(ret);
        RETURN_IF_ERROR_IN_SCOPE(c, codegen_type_params(c, type_params));
        _Py_DECLARE_STR(type_params, ".type_params");
        RETURN_IF_ERROR_IN_SCOPE(c, codegen_nameop(c, loc, &_Py_STR(type_params), Store));
    }

    int ret = codegen_class_body(c, s, firstlineno);
    if (is_generic) {
        RETURN_IF_ERROR_IN_SCOPE(c, ret);
    }
    else {
        RETURN_IF_ERROR(ret);
    }

    /* generate the rest of the code for the call */

    if (is_generic) {
        _Py_DECLARE_STR(type_params, ".type_params");
        _Py_DECLARE_STR(generic_base, ".generic_base");
        RETURN_IF_ERROR_IN_SCOPE(c, codegen_nameop(c, loc, &_Py_STR(type_params), Load));
        ADDOP_I_IN_SCOPE(c, loc, CALL_INTRINSIC_1, INTRINSIC_SUBSCRIPT_GENERIC);
        RETURN_IF_ERROR_IN_SCOPE(c, codegen_nameop(c, loc, &_Py_STR(generic_base), Store));

        RETURN_IF_ERROR_IN_SCOPE(c, codegen_call_helper_impl(c, loc, 2,
                                                             s->v.ClassDef.bases,
                                                             &_Py_STR(generic_base),
                                                             s->v.ClassDef.keywords));

        PyCodeObject *co = _PyCompile_OptimizeAndAssemble(c, 0);

        _PyCompile_ExitScope(c);
        if (co == NULL) {
            return ERROR;
        }
        int ret = codegen_make_closure(c, loc, co, 0);
        Py_DECREF(co);
        RETURN_IF_ERROR(ret);
        ADDOP(c, loc, PUSH_NULL);
        ADDOP_I(c, loc, CALL, 0);
    } else {
        RETURN_IF_ERROR(codegen_call_helper(c, loc, 2,
                                            s->v.ClassDef.bases,
                                            s->v.ClassDef.keywords));
    }

    /* 6. apply decorators */
    RETURN_IF_ERROR(codegen_apply_decorators(c, decos));

    /* 7. store into <name> */
    RETURN_IF_ERROR(codegen_nameop(c, loc, s->v.ClassDef.name, Store));
    return SUCCESS;
}

static int
codegen_typealias_body(compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    PyObject *name = s->v.TypeAlias.name->v.Name.id;
    PyObject *defaults = PyTuple_Pack(1, _PyLong_GetOne());
    ADDOP_LOAD_CONST_NEW(c, loc, defaults);
    RETURN_IF_ERROR(
        codegen_setup_annotations_scope(c, LOC(s), s, name));

    assert(!SYMTABLE_ENTRY(c)->ste_has_docstring);
    VISIT_IN_SCOPE(c, expr, s->v.TypeAlias.value);
    ADDOP_IN_SCOPE(c, loc, RETURN_VALUE);
    PyCodeObject *co = _PyCompile_OptimizeAndAssemble(c, 0);
    _PyCompile_ExitScope(c);
    if (co == NULL) {
        return ERROR;
    }
    int ret = codegen_make_closure(c, loc, co, MAKE_FUNCTION_DEFAULTS);
    Py_DECREF(co);
    RETURN_IF_ERROR(ret);

    ADDOP_I(c, loc, BUILD_TUPLE, 3);
    ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_TYPEALIAS);
    return SUCCESS;
}

static int
codegen_typealias(compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    asdl_type_param_seq *type_params = s->v.TypeAlias.type_params;
    int is_generic = asdl_seq_LEN(type_params) > 0;
    PyObject *name = s->v.TypeAlias.name->v.Name.id;
    if (is_generic) {
        PyObject *type_params_name = PyUnicode_FromFormat("<generic parameters of %U>",
                                                         name);
        if (!type_params_name) {
            return ERROR;
        }
        int ret = codegen_enter_scope(c, type_params_name, COMPILE_SCOPE_ANNOTATIONS,
                                      (void *)type_params, loc.lineno, NULL, NULL);
        Py_DECREF(type_params_name);
        RETURN_IF_ERROR(ret);
        ADDOP_LOAD_CONST_IN_SCOPE(c, loc, name);
        RETURN_IF_ERROR_IN_SCOPE(c, codegen_type_params(c, type_params));
    }
    else {
        ADDOP_LOAD_CONST(c, loc, name);
        ADDOP_LOAD_CONST(c, loc, Py_None);
    }

    int ret = codegen_typealias_body(c, s);
    if (is_generic) {
        RETURN_IF_ERROR_IN_SCOPE(c, ret);
    }
    else {
        RETURN_IF_ERROR(ret);
    }

    if (is_generic) {
        PyCodeObject *co = _PyCompile_OptimizeAndAssemble(c, 0);
        _PyCompile_ExitScope(c);
        if (co == NULL) {
            return ERROR;
        }
        int ret = codegen_make_closure(c, loc, co, 0);
        Py_DECREF(co);
        RETURN_IF_ERROR(ret);
        ADDOP(c, loc, PUSH_NULL);
        ADDOP_I(c, loc, CALL, 0);
    }
    RETURN_IF_ERROR(codegen_nameop(c, loc, name, Store));
    return SUCCESS;
}

static bool
is_const_tuple(asdl_expr_seq *elts)
{
    for (Py_ssize_t i = 0; i < asdl_seq_LEN(elts); i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(elts, i);
        if (e->kind != Constant_kind) {
            return false;
        }
    }
    return true;
}

/* Return false if the expression is a constant value except named singletons.
   Return true otherwise. */
static bool
check_is_arg(expr_ty e)
{
    if (e->kind == Tuple_kind) {
        return !is_const_tuple(e->v.Tuple.elts);
    }
    if (e->kind != Constant_kind) {
        return true;
    }
    PyObject *value = e->v.Constant.value;
    return (value == Py_None
         || value == Py_False
         || value == Py_True
         || value == Py_Ellipsis);
}

static PyTypeObject * infer_type(expr_ty e);

/* Check operands of identity checks ("is" and "is not").
   Emit a warning if any operand is a constant except named singletons.
 */
static int
codegen_check_compare(compiler *c, expr_ty e)
{
    Py_ssize_t i, n;
    bool left = check_is_arg(e->v.Compare.left);
    expr_ty left_expr = e->v.Compare.left;
    n = asdl_seq_LEN(e->v.Compare.ops);
    for (i = 0; i < n; i++) {
        cmpop_ty op = (cmpop_ty)asdl_seq_GET(e->v.Compare.ops, i);
        expr_ty right_expr = (expr_ty)asdl_seq_GET(e->v.Compare.comparators, i);
        bool right = check_is_arg(right_expr);
        if (op == Is || op == IsNot) {
            if (!right || !left) {
                const char *msg = (op == Is)
                        ? "\"is\" with '%.200s' literal. Did you mean \"==\"?"
                        : "\"is not\" with '%.200s' literal. Did you mean \"!=\"?";
                expr_ty literal = !left ? left_expr : right_expr;
                return _PyCompile_Warn(
                    c, LOC(e), msg, infer_type(literal)->tp_name
                );
            }
        }
        left = right;
        left_expr = right_expr;
    }
    return SUCCESS;
}

static int
codegen_addcompare(compiler *c, location loc, cmpop_ty op)
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
        ADDOP_I(c, loc, IS_OP, 0);
        return SUCCESS;
    case IsNot:
        ADDOP_I(c, loc, IS_OP, 1);
        return SUCCESS;
    case In:
        ADDOP_I(c, loc, CONTAINS_OP, 0);
        return SUCCESS;
    case NotIn:
        ADDOP_I(c, loc, CONTAINS_OP, 1);
        return SUCCESS;
    default:
        Py_UNREACHABLE();
    }
    // cmp goes in top three bits of the oparg, while the low four bits are used
    // by quickened versions of this opcode to store the comparison mask. The
    // fifth-lowest bit indicates whether the result should be converted to bool
    // and is set later):
    ADDOP_I(c, loc, COMPARE_OP, (cmp << 5) | compare_masks[cmp]);
    return SUCCESS;
}

static int
codegen_jump_if(compiler *c, location loc,
                expr_ty e, jump_target_label next, int cond)
{
    switch (e->kind) {
    case UnaryOp_kind:
        if (e->v.UnaryOp.op == Not) {
            return codegen_jump_if(c, loc, e->v.UnaryOp.operand, next, !cond);
        }
        /* fallback to general implementation */
        break;
    case BoolOp_kind: {
        asdl_expr_seq *s = e->v.BoolOp.values;
        Py_ssize_t i, n = asdl_seq_LEN(s) - 1;
        assert(n >= 0);
        int cond2 = e->v.BoolOp.op == Or;
        jump_target_label next2 = next;
        if (!cond2 != !cond) {
            NEW_JUMP_TARGET_LABEL(c, new_next2);
            next2 = new_next2;
        }
        for (i = 0; i < n; ++i) {
            RETURN_IF_ERROR(
                codegen_jump_if(c, loc, (expr_ty)asdl_seq_GET(s, i), next2, cond2));
        }
        RETURN_IF_ERROR(
            codegen_jump_if(c, loc, (expr_ty)asdl_seq_GET(s, n), next, cond));
        if (!SAME_JUMP_TARGET_LABEL(next2, next)) {
            USE_LABEL(c, next2);
        }
        return SUCCESS;
    }
    case IfExp_kind: {
        NEW_JUMP_TARGET_LABEL(c, end);
        NEW_JUMP_TARGET_LABEL(c, next2);
        RETURN_IF_ERROR(
            codegen_jump_if(c, loc, e->v.IfExp.test, next2, 0));
        RETURN_IF_ERROR(
            codegen_jump_if(c, loc, e->v.IfExp.body, next, cond));
        ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, end);

        USE_LABEL(c, next2);
        RETURN_IF_ERROR(
            codegen_jump_if(c, loc, e->v.IfExp.orelse, next, cond));

        USE_LABEL(c, end);
        return SUCCESS;
    }
    case Compare_kind: {
        Py_ssize_t n = asdl_seq_LEN(e->v.Compare.ops) - 1;
        if (n > 0) {
            RETURN_IF_ERROR(codegen_check_compare(c, e));
            NEW_JUMP_TARGET_LABEL(c, cleanup);
            VISIT(c, expr, e->v.Compare.left);
            for (Py_ssize_t i = 0; i < n; i++) {
                VISIT(c, expr,
                    (expr_ty)asdl_seq_GET(e->v.Compare.comparators, i));
                ADDOP_I(c, LOC(e), SWAP, 2);
                ADDOP_I(c, LOC(e), COPY, 2);
                ADDOP_COMPARE(c, LOC(e), asdl_seq_GET(e->v.Compare.ops, i));
                ADDOP(c, LOC(e), TO_BOOL);
                ADDOP_JUMP(c, LOC(e), POP_JUMP_IF_FALSE, cleanup);
            }
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Compare.comparators, n));
            ADDOP_COMPARE(c, LOC(e), asdl_seq_GET(e->v.Compare.ops, n));
            ADDOP(c, LOC(e), TO_BOOL);
            ADDOP_JUMP(c, LOC(e), cond ? POP_JUMP_IF_TRUE : POP_JUMP_IF_FALSE, next);
            NEW_JUMP_TARGET_LABEL(c, end);
            ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, end);

            USE_LABEL(c, cleanup);
            ADDOP(c, LOC(e), POP_TOP);
            if (!cond) {
                ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, next);
            }

            USE_LABEL(c, end);
            return SUCCESS;
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
    ADDOP(c, LOC(e), TO_BOOL);
    ADDOP_JUMP(c, LOC(e), cond ? POP_JUMP_IF_TRUE : POP_JUMP_IF_FALSE, next);
    return SUCCESS;
}

static int
codegen_ifexp(compiler *c, expr_ty e)
{
    assert(e->kind == IfExp_kind);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, next);

    RETURN_IF_ERROR(
        codegen_jump_if(c, LOC(e), e->v.IfExp.test, next, 0));

    VISIT(c, expr, e->v.IfExp.body);
    ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, end);

    USE_LABEL(c, next);
    VISIT(c, expr, e->v.IfExp.orelse);

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
codegen_lambda(compiler *c, expr_ty e)
{
    PyCodeObject *co;
    Py_ssize_t funcflags;
    arguments_ty args = e->v.Lambda.args;
    assert(e->kind == Lambda_kind);

    location loc = LOC(e);
    funcflags = codegen_default_arguments(c, loc, args);
    RETURN_IF_ERROR(funcflags);

    _PyCompile_CodeUnitMetadata umd = {
        .u_argcount = asdl_seq_LEN(args->args),
        .u_posonlyargcount = asdl_seq_LEN(args->posonlyargs),
        .u_kwonlyargcount = asdl_seq_LEN(args->kwonlyargs),
    };
    _Py_DECLARE_STR(anon_lambda, "<lambda>");
    RETURN_IF_ERROR(
        codegen_enter_scope(c, &_Py_STR(anon_lambda), COMPILE_SCOPE_LAMBDA,
                            (void *)e, e->lineno, NULL, &umd));

    assert(!SYMTABLE_ENTRY(c)->ste_has_docstring);

    VISIT_IN_SCOPE(c, expr, e->v.Lambda.body);
    if (SYMTABLE_ENTRY(c)->ste_generator) {
        co = _PyCompile_OptimizeAndAssemble(c, 0);
    }
    else {
        location loc = LOC(e->v.Lambda.body);
        ADDOP_IN_SCOPE(c, loc, RETURN_VALUE);
        co = _PyCompile_OptimizeAndAssemble(c, 1);
    }
    _PyCompile_ExitScope(c);
    if (co == NULL) {
        return ERROR;
    }

    int ret = codegen_make_closure(c, loc, co, funcflags);
    Py_DECREF(co);
    RETURN_IF_ERROR(ret);
    return SUCCESS;
}

static int
codegen_if(compiler *c, stmt_ty s)
{
    jump_target_label next;
    assert(s->kind == If_kind);
    NEW_JUMP_TARGET_LABEL(c, end);
    if (asdl_seq_LEN(s->v.If.orelse)) {
        NEW_JUMP_TARGET_LABEL(c, orelse);
        next = orelse;
    }
    else {
        next = end;
    }
    RETURN_IF_ERROR(
        codegen_jump_if(c, LOC(s), s->v.If.test, next, 0));

    VISIT_SEQ(c, stmt, s->v.If.body);
    if (asdl_seq_LEN(s->v.If.orelse)) {
        ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, end);

        USE_LABEL(c, next);
        VISIT_SEQ(c, stmt, s->v.If.orelse);
    }

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
codegen_for(compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    NEW_JUMP_TARGET_LABEL(c, start);
    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, cleanup);
    NEW_JUMP_TARGET_LABEL(c, end);

    RETURN_IF_ERROR(_PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_FOR_LOOP, start, end, NULL));

    VISIT(c, expr, s->v.For.iter);

    loc = LOC(s->v.For.iter);
    ADDOP(c, loc, GET_ITER);

    USE_LABEL(c, start);
    ADDOP_JUMP(c, loc, FOR_ITER, cleanup);

    /* Add NOP to ensure correct line tracing of multiline for statements.
     * It will be removed later if redundant.
     */
    ADDOP(c, LOC(s->v.For.target), NOP);

    USE_LABEL(c, body);
    VISIT(c, expr, s->v.For.target);
    VISIT_SEQ(c, stmt, s->v.For.body);
    /* Mark jump as artificial */
    ADDOP_JUMP(c, NO_LOCATION, JUMP, start);

    USE_LABEL(c, cleanup);
    /* It is important for instrumentation that the `END_FOR` comes first.
    * Iteration over a generator will jump to the first of these instructions,
    * but a non-generator will jump to a later instruction.
    */
    ADDOP(c, NO_LOCATION, END_FOR);
    ADDOP(c, NO_LOCATION, POP_ITER);

    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_FOR_LOOP, start);

    VISIT_SEQ(c, stmt, s->v.For.orelse);

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
codegen_async_for(compiler *c, stmt_ty s)
{
    location loc = LOC(s);

    NEW_JUMP_TARGET_LABEL(c, start);
    NEW_JUMP_TARGET_LABEL(c, send);
    NEW_JUMP_TARGET_LABEL(c, except);
    NEW_JUMP_TARGET_LABEL(c, end);

    VISIT(c, expr, s->v.AsyncFor.iter);
    ADDOP(c, LOC(s->v.AsyncFor.iter), GET_AITER);

    USE_LABEL(c, start);
    RETURN_IF_ERROR(_PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_ASYNC_FOR_LOOP, start, end, NULL));

    /* SETUP_FINALLY to guard the __anext__ call */
    ADDOP_JUMP(c, loc, SETUP_FINALLY, except);
    ADDOP(c, loc, GET_ANEXT);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    USE_LABEL(c, send);
    ADD_YIELD_FROM(c, loc, 1);
    ADDOP(c, loc, POP_BLOCK);  /* for SETUP_FINALLY */
    ADDOP(c, loc, NOT_TAKEN);

    /* Success block for __anext__ */
    VISIT(c, expr, s->v.AsyncFor.target);
    VISIT_SEQ(c, stmt, s->v.AsyncFor.body);
    /* Mark jump as artificial */
    ADDOP_JUMP(c, NO_LOCATION, JUMP, start);

    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_ASYNC_FOR_LOOP, start);

    /* Except block for __anext__ */
    USE_LABEL(c, except);

    /* Use same line number as the iterator,
     * as the END_ASYNC_FOR succeeds the `for`, not the body. */
    loc = LOC(s->v.AsyncFor.iter);
    ADDOP_JUMP(c, loc, END_ASYNC_FOR, send);

    /* `else` block */
    VISIT_SEQ(c, stmt, s->v.AsyncFor.orelse);

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
codegen_while(compiler *c, stmt_ty s)
{
    NEW_JUMP_TARGET_LABEL(c, loop);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, anchor);

    USE_LABEL(c, loop);

    RETURN_IF_ERROR(_PyCompile_PushFBlock(c, LOC(s), COMPILE_FBLOCK_WHILE_LOOP, loop, end, NULL));
    RETURN_IF_ERROR(codegen_jump_if(c, LOC(s), s->v.While.test, anchor, 0));

    VISIT_SEQ(c, stmt, s->v.While.body);
    ADDOP_JUMP(c, NO_LOCATION, JUMP, loop);

    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_WHILE_LOOP, loop);

    USE_LABEL(c, anchor);
    if (s->v.While.orelse) {
        VISIT_SEQ(c, stmt, s->v.While.orelse);
    }

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
codegen_return(compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    int preserve_tos = ((s->v.Return.value != NULL) &&
                        (s->v.Return.value->kind != Constant_kind));

    PySTEntryObject *ste = SYMTABLE_ENTRY(c);
    if (!_PyST_IsFunctionLike(ste)) {
        return _PyCompile_Error(c, loc, "'return' outside function");
    }
    if (s->v.Return.value != NULL && ste->ste_coroutine && ste->ste_generator) {
        return _PyCompile_Error(c, loc, "'return' with value in async generator");
    }

    if (preserve_tos) {
        VISIT(c, expr, s->v.Return.value);
    } else {
        /* Emit instruction with line number for return value */
        if (s->v.Return.value != NULL) {
            loc = LOC(s->v.Return.value);
            ADDOP(c, loc, NOP);
        }
    }
    if (s->v.Return.value == NULL || s->v.Return.value->lineno != s->lineno) {
        loc = LOC(s);
        ADDOP(c, loc, NOP);
    }

    RETURN_IF_ERROR(codegen_unwind_fblock_stack(c, &loc, preserve_tos, NULL));
    if (s->v.Return.value == NULL) {
        ADDOP_LOAD_CONST(c, loc, Py_None);
    }
    else if (!preserve_tos) {
        ADDOP_LOAD_CONST(c, loc, s->v.Return.value->v.Constant.value);
    }
    ADDOP(c, loc, RETURN_VALUE);

    return SUCCESS;
}

static int
codegen_break(compiler *c, location loc)
{
    fblockinfo *loop = NULL;
    location origin_loc = loc;
    /* Emit instruction with line number */
    ADDOP(c, loc, NOP);
    RETURN_IF_ERROR(codegen_unwind_fblock_stack(c, &loc, 0, &loop));
    if (loop == NULL) {
        return _PyCompile_Error(c, origin_loc, "'break' outside loop");
    }
    RETURN_IF_ERROR(codegen_unwind_fblock(c, &loc, loop, 0));
    ADDOP_JUMP(c, loc, JUMP, loop->fb_exit);
    return SUCCESS;
}

static int
codegen_continue(compiler *c, location loc)
{
    fblockinfo *loop = NULL;
    location origin_loc = loc;
    /* Emit instruction with line number */
    ADDOP(c, loc, NOP);
    RETURN_IF_ERROR(codegen_unwind_fblock_stack(c, &loc, 0, &loop));
    if (loop == NULL) {
        return _PyCompile_Error(c, origin_loc, "'continue' not properly in loop");
    }
    ADDOP_JUMP(c, loc, JUMP, loop->fb_block);
    return SUCCESS;
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
codegen_try_finally(compiler *c, stmt_ty s)
{
    location loc = LOC(s);

    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, exit);
    NEW_JUMP_TARGET_LABEL(c, cleanup);

    /* `try` block */
    ADDOP_JUMP(c, loc, SETUP_FINALLY, end);

    USE_LABEL(c, body);
    RETURN_IF_ERROR(
        _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_FINALLY_TRY, body, end,
                              s->v.Try.finalbody));

    if (s->v.Try.handlers && asdl_seq_LEN(s->v.Try.handlers)) {
        RETURN_IF_ERROR(codegen_try_except(c, s));
    }
    else {
        VISIT_SEQ(c, stmt, s->v.Try.body);
    }
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_FINALLY_TRY, body);
    VISIT_SEQ(c, stmt, s->v.Try.finalbody);

    ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, exit);
    /* `finally` block */

    USE_LABEL(c, end);

    loc = NO_LOCATION;
    ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup);
    ADDOP(c, loc, PUSH_EXC_INFO);
    RETURN_IF_ERROR(
        _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_FINALLY_END, end, NO_LABEL, NULL));
    VISIT_SEQ(c, stmt, s->v.Try.finalbody);
    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_FINALLY_END, end);

    loc = NO_LOCATION;
    ADDOP_I(c, loc, RERAISE, 0);

    USE_LABEL(c, cleanup);
    POP_EXCEPT_AND_RERAISE(c, loc);

    USE_LABEL(c, exit);
    return SUCCESS;
}

static int
codegen_try_star_finally(compiler *c, stmt_ty s)
{
    location loc = LOC(s);

    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, exit);
    NEW_JUMP_TARGET_LABEL(c, cleanup);
    /* `try` block */
    ADDOP_JUMP(c, loc, SETUP_FINALLY, end);

    USE_LABEL(c, body);
    RETURN_IF_ERROR(
        _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_FINALLY_TRY, body, end,
                              s->v.TryStar.finalbody));

    if (s->v.TryStar.handlers && asdl_seq_LEN(s->v.TryStar.handlers)) {
        RETURN_IF_ERROR(codegen_try_star_except(c, s));
    }
    else {
        VISIT_SEQ(c, stmt, s->v.TryStar.body);
    }
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_FINALLY_TRY, body);
    VISIT_SEQ(c, stmt, s->v.TryStar.finalbody);

    ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, exit);

    /* `finally` block */
    USE_LABEL(c, end);

    loc = NO_LOCATION;
    ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup);
    ADDOP(c, loc, PUSH_EXC_INFO);
    RETURN_IF_ERROR(
        _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_FINALLY_END, end, NO_LABEL, NULL));

    VISIT_SEQ(c, stmt, s->v.TryStar.finalbody);

    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_FINALLY_END, end);
    loc = NO_LOCATION;
    ADDOP_I(c, loc, RERAISE, 0);

    USE_LABEL(c, cleanup);
    POP_EXCEPT_AND_RERAISE(c, loc);

    USE_LABEL(c, exit);
    return SUCCESS;
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
   []                           JUMP            L0

   [exc]                L1:     <evaluate E1>           )
   [exc, E1]                    CHECK_EXC_MATCH         )
   [exc, bool]                  POP_JUMP_IF_FALSE L2    ) only if E1
   [exc]                        <assign to V1>  (or POP if no V1)
   []                           <code for S1>
                                JUMP            L0

   [exc]                L2:     <evaluate E2>
   .............................etc.......................

   [exc]                Ln+1:   RERAISE     # re-raise exception

   []                   L0:     <next statement>

   Of course, parts are not generated if Vi or Ei is not present.
*/
static int
codegen_try_except(compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    Py_ssize_t i, n;

    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, except);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, cleanup);

    ADDOP_JUMP(c, loc, SETUP_FINALLY, except);

    USE_LABEL(c, body);
    RETURN_IF_ERROR(
        _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_TRY_EXCEPT, body, NO_LABEL, NULL));
    VISIT_SEQ(c, stmt, s->v.Try.body);
    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_TRY_EXCEPT, body);
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    if (s->v.Try.orelse && asdl_seq_LEN(s->v.Try.orelse)) {
        VISIT_SEQ(c, stmt, s->v.Try.orelse);
    }
    ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, end);
    n = asdl_seq_LEN(s->v.Try.handlers);

    USE_LABEL(c, except);

    ADDOP_JUMP(c, NO_LOCATION, SETUP_CLEANUP, cleanup);
    ADDOP(c, NO_LOCATION, PUSH_EXC_INFO);

    /* Runtime will push a block here, so we need to account for that */
    RETURN_IF_ERROR(
        _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_EXCEPTION_HANDLER,
                              NO_LABEL, NO_LABEL, NULL));

    for (i = 0; i < n; i++) {
        excepthandler_ty handler = (excepthandler_ty)asdl_seq_GET(
            s->v.Try.handlers, i);
        location loc = LOC(handler);
        if (!handler->v.ExceptHandler.type && i < n-1) {
            return _PyCompile_Error(c, loc, "default 'except:' must be last");
        }
        NEW_JUMP_TARGET_LABEL(c, next_except);
        except = next_except;
        if (handler->v.ExceptHandler.type) {
            VISIT(c, expr, handler->v.ExceptHandler.type);
            ADDOP(c, loc, CHECK_EXC_MATCH);
            ADDOP_JUMP(c, loc, POP_JUMP_IF_FALSE, except);
        }
        if (handler->v.ExceptHandler.name) {
            NEW_JUMP_TARGET_LABEL(c, cleanup_end);
            NEW_JUMP_TARGET_LABEL(c, cleanup_body);

            RETURN_IF_ERROR(
                codegen_nameop(c, loc, handler->v.ExceptHandler.name, Store));

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
            ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup_end);

            USE_LABEL(c, cleanup_body);
            RETURN_IF_ERROR(
                _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_HANDLER_CLEANUP, cleanup_body,
                                      NO_LABEL, handler->v.ExceptHandler.name));

            /* second # body */
            VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
            _PyCompile_PopFBlock(c, COMPILE_FBLOCK_HANDLER_CLEANUP, cleanup_body);
            /* name = None; del name; # Mark as artificial */
            ADDOP(c, NO_LOCATION, POP_BLOCK);
            ADDOP(c, NO_LOCATION, POP_BLOCK);
            ADDOP(c, NO_LOCATION, POP_EXCEPT);
            ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
            RETURN_IF_ERROR(
                codegen_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Store));
            RETURN_IF_ERROR(
                codegen_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Del));
            ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, end);

            /* except: */
            USE_LABEL(c, cleanup_end);

            /* name = None; del name; # artificial */
            ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
            RETURN_IF_ERROR(
                codegen_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Store));
            RETURN_IF_ERROR(
                codegen_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Del));

            ADDOP_I(c, NO_LOCATION, RERAISE, 1);
        }
        else {
            NEW_JUMP_TARGET_LABEL(c, cleanup_body);

            ADDOP(c, loc, POP_TOP); /* exc_value */

            USE_LABEL(c, cleanup_body);
            RETURN_IF_ERROR(
                _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_HANDLER_CLEANUP, cleanup_body,
                                      NO_LABEL, NULL));

            VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
            _PyCompile_PopFBlock(c, COMPILE_FBLOCK_HANDLER_CLEANUP, cleanup_body);
            ADDOP(c, NO_LOCATION, POP_BLOCK);
            ADDOP(c, NO_LOCATION, POP_EXCEPT);
            ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, end);
        }

        USE_LABEL(c, except);
    }
    /* artificial */
    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_EXCEPTION_HANDLER, NO_LABEL);
    ADDOP_I(c, NO_LOCATION, RERAISE, 0);

    USE_LABEL(c, cleanup);
    POP_EXCEPT_AND_RERAISE(c, NO_LOCATION);

    USE_LABEL(c, end);
    return SUCCESS;
}

/*
   Code generated for "try: S except* E1 as V1: S1 except* E2 as V2: S2 ...":
   (The contents of the value stack is shown in [], with the top
   at the right; 'tb' is trace-back info, 'val' the exception instance,
   and 'typ' the exception's type.)

   Value stack                   Label         Instruction     Argument
   []                                         SETUP_FINALLY         L1
   []                                         <code for S>
   []                                         POP_BLOCK
   []                                         JUMP                  L0

   [exc]                            L1:       BUILD_LIST   )  list for raised/reraised excs ("result")
   [orig, res]                                COPY 2       )  make a copy of the original EG

   [orig, res, exc]                           <evaluate E1>
   [orig, res, exc, E1]                       CHECK_EG_MATCH
   [orig, res, rest/exc, match?]              COPY 1
   [orig, res, rest/exc, match?, match?]      POP_JUMP_IF_NONE      C1

   [orig, res, rest, match]                   <assign to V1>  (or POP if no V1)

   [orig, res, rest]                          SETUP_FINALLY         R1
   [orig, res, rest]                          <code for S1>
   [orig, res, rest]                          JUMP                  L2

   [orig, res, rest, i, v]          R1:       LIST_APPEND   3 ) exc raised in except* body - add to res
   [orig, res, rest, i]                       POP
   [orig, res, rest]                          JUMP                  LE2

   [orig, res, rest]                L2:       NOP  ) for lineno
   [orig, res, rest]                          JUMP                  LE2

   [orig, res, rest/exc, None]      C1:       POP

   [orig, res, rest]               LE2:       <evaluate E2>
   .............................etc.......................

   [orig, res, rest]                Ln+1:     LIST_APPEND 1  ) add unhandled exc to res (could be None)

   [orig, res]                                CALL_INTRINSIC_2 PREP_RERAISE_STAR
   [exc]                                      COPY 1
   [exc, exc]                                 POP_JUMP_IF_NOT_NONE  RER
   [exc]                                      POP_TOP
   []                                         JUMP                  L0

   [exc]                            RER:      SWAP 2
   [exc, prev_exc_info]                       POP_EXCEPT
   [exc]                                      RERAISE               0

   []                               L0:       <next statement>
*/
static int
codegen_try_star_except(compiler *c, stmt_ty s)
{
    location loc = LOC(s);

    NEW_JUMP_TARGET_LABEL(c, body);
    NEW_JUMP_TARGET_LABEL(c, except);
    NEW_JUMP_TARGET_LABEL(c, orelse);
    NEW_JUMP_TARGET_LABEL(c, end);
    NEW_JUMP_TARGET_LABEL(c, cleanup);
    NEW_JUMP_TARGET_LABEL(c, reraise_star);

    ADDOP_JUMP(c, loc, SETUP_FINALLY, except);

    USE_LABEL(c, body);
    RETURN_IF_ERROR(
        _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_TRY_EXCEPT, body, NO_LABEL, NULL));
    VISIT_SEQ(c, stmt, s->v.TryStar.body);
    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_TRY_EXCEPT, body);
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, orelse);
    Py_ssize_t n = asdl_seq_LEN(s->v.TryStar.handlers);

    USE_LABEL(c, except);

    ADDOP_JUMP(c, NO_LOCATION, SETUP_CLEANUP, cleanup);
    ADDOP(c, NO_LOCATION, PUSH_EXC_INFO);

    /* Runtime will push a block here, so we need to account for that */
    RETURN_IF_ERROR(
        _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_EXCEPTION_GROUP_HANDLER,
                              NO_LABEL, NO_LABEL, "except handler"));

    for (Py_ssize_t i = 0; i < n; i++) {
        excepthandler_ty handler = (excepthandler_ty)asdl_seq_GET(
            s->v.TryStar.handlers, i);
        location loc = LOC(handler);
        NEW_JUMP_TARGET_LABEL(c, next_except);
        except = next_except;
        NEW_JUMP_TARGET_LABEL(c, except_with_error);
        NEW_JUMP_TARGET_LABEL(c, no_match);
        if (i == 0) {
            /* create empty list for exceptions raised/reraise in the except* blocks */
            /*
               [orig]       BUILD_LIST
            */
            /* Create a copy of the original EG */
            /*
               [orig, []]   COPY 2
               [orig, [], exc]
            */
            ADDOP_I(c, loc, BUILD_LIST, 0);
            ADDOP_I(c, loc, COPY, 2);
        }
        if (handler->v.ExceptHandler.type) {
            VISIT(c, expr, handler->v.ExceptHandler.type);
            ADDOP(c, loc, CHECK_EG_MATCH);
            ADDOP_I(c, loc, COPY, 1);
            ADDOP_JUMP(c, loc, POP_JUMP_IF_NONE, no_match);
        }

        NEW_JUMP_TARGET_LABEL(c, cleanup_end);
        NEW_JUMP_TARGET_LABEL(c, cleanup_body);

        if (handler->v.ExceptHandler.name) {
            RETURN_IF_ERROR(
                codegen_nameop(c, loc, handler->v.ExceptHandler.name, Store));
        }
        else {
            ADDOP(c, loc, POP_TOP);  // match
        }

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
        ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup_end);

        USE_LABEL(c, cleanup_body);
        RETURN_IF_ERROR(
            _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_HANDLER_CLEANUP, cleanup_body,
                                  NO_LABEL, handler->v.ExceptHandler.name));

        /* second # body */
        VISIT_SEQ(c, stmt, handler->v.ExceptHandler.body);
        _PyCompile_PopFBlock(c, COMPILE_FBLOCK_HANDLER_CLEANUP, cleanup_body);
        /* name = None; del name; # artificial */
        ADDOP(c, NO_LOCATION, POP_BLOCK);
        if (handler->v.ExceptHandler.name) {
            ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
            RETURN_IF_ERROR(
                codegen_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Store));
            RETURN_IF_ERROR(
                codegen_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Del));
        }
        ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, except);

        /* except: */
        USE_LABEL(c, cleanup_end);

        /* name = None; del name; # artificial */
        if (handler->v.ExceptHandler.name) {
            ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
            RETURN_IF_ERROR(
                codegen_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Store));
            RETURN_IF_ERROR(
                codegen_nameop(c, NO_LOCATION, handler->v.ExceptHandler.name, Del));
        }

        /* add exception raised to the res list */
        ADDOP_I(c, NO_LOCATION, LIST_APPEND, 3); // exc
        ADDOP(c, NO_LOCATION, POP_TOP); // lasti
        ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, except_with_error);

        USE_LABEL(c, except);
        ADDOP(c, NO_LOCATION, NOP);  // to hold a propagated location info
        ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, except_with_error);

        USE_LABEL(c, no_match);
        ADDOP(c, loc, POP_TOP);  // match (None)

        USE_LABEL(c, except_with_error);

        if (i == n - 1) {
            /* Add exc to the list (if not None it's the unhandled part of the EG) */
            ADDOP_I(c, NO_LOCATION, LIST_APPEND, 1);
            ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, reraise_star);
        }
    }
    /* artificial */
    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_EXCEPTION_GROUP_HANDLER, NO_LABEL);
    NEW_JUMP_TARGET_LABEL(c, reraise);

    USE_LABEL(c, reraise_star);
    ADDOP_I(c, NO_LOCATION, CALL_INTRINSIC_2, INTRINSIC_PREP_RERAISE_STAR);
    ADDOP_I(c, NO_LOCATION, COPY, 1);
    ADDOP_JUMP(c, NO_LOCATION, POP_JUMP_IF_NOT_NONE, reraise);

    /* Nothing to reraise */
    ADDOP(c, NO_LOCATION, POP_TOP);
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    ADDOP(c, NO_LOCATION, POP_EXCEPT);
    ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, end);

    USE_LABEL(c, reraise);
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    ADDOP_I(c, NO_LOCATION, SWAP, 2);
    ADDOP(c, NO_LOCATION, POP_EXCEPT);
    ADDOP_I(c, NO_LOCATION, RERAISE, 0);

    USE_LABEL(c, cleanup);
    POP_EXCEPT_AND_RERAISE(c, NO_LOCATION);

    USE_LABEL(c, orelse);
    VISIT_SEQ(c, stmt, s->v.TryStar.orelse);

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
codegen_try(compiler *c, stmt_ty s) {
    if (s->v.Try.finalbody && asdl_seq_LEN(s->v.Try.finalbody))
        return codegen_try_finally(c, s);
    else
        return codegen_try_except(c, s);
}

static int
codegen_try_star(compiler *c, stmt_ty s)
{
    if (s->v.TryStar.finalbody && asdl_seq_LEN(s->v.TryStar.finalbody)) {
        return codegen_try_star_finally(c, s);
    }
    else {
        return codegen_try_star_except(c, s);
    }
}

static int
codegen_import_as(compiler *c, location loc,
                  identifier name, identifier asname)
{
    /* The IMPORT_NAME opcode was already generated.  This function
       merely needs to bind the result to a name.

       If there is a dot in name, we need to split it and emit a
       IMPORT_FROM for each name.
    */
    Py_ssize_t len = PyUnicode_GET_LENGTH(name);
    Py_ssize_t dot = PyUnicode_FindChar(name, '.', 0, len, 1);
    if (dot == -2) {
        return ERROR;
    }
    if (dot != -1) {
        /* Consume the base module name to get the first attribute */
        while (1) {
            Py_ssize_t pos = dot + 1;
            PyObject *attr;
            dot = PyUnicode_FindChar(name, '.', pos, len, 1);
            if (dot == -2) {
                return ERROR;
            }
            attr = PyUnicode_Substring(name, pos, (dot != -1) ? dot : len);
            if (!attr) {
                return ERROR;
            }
            ADDOP_N(c, loc, IMPORT_FROM, attr, names);
            if (dot == -1) {
                break;
            }
            ADDOP_I(c, loc, SWAP, 2);
            ADDOP(c, loc, POP_TOP);
        }
        RETURN_IF_ERROR(codegen_nameop(c, loc, asname, Store));
        ADDOP(c, loc, POP_TOP);
        return SUCCESS;
    }
    return codegen_nameop(c, loc, asname, Store);
}

static int
codegen_import(compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    /* The Import node stores a module name like a.b.c as a single
       string.  This is convenient for all cases except
         import a.b.c as d
       where we need to parse that string to extract the individual
       module names.
       XXX Perhaps change the representation to make this case simpler?
     */
    Py_ssize_t i, n = asdl_seq_LEN(s->v.Import.names);

    PyObject *zero = _PyLong_GetZero();  // borrowed reference
    for (i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.Import.names, i);
        int r;

        ADDOP_LOAD_CONST(c, loc, zero);
        ADDOP_LOAD_CONST(c, loc, Py_None);
        ADDOP_NAME(c, loc, IMPORT_NAME, alias->name, names);

        if (alias->asname) {
            r = codegen_import_as(c, loc, alias->name, alias->asname);
            RETURN_IF_ERROR(r);
        }
        else {
            identifier tmp = alias->name;
            Py_ssize_t dot = PyUnicode_FindChar(
                alias->name, '.', 0, PyUnicode_GET_LENGTH(alias->name), 1);
            if (dot != -1) {
                tmp = PyUnicode_Substring(alias->name, 0, dot);
                if (tmp == NULL) {
                    return ERROR;
                }
            }
            r = codegen_nameop(c, loc, tmp, Store);
            if (dot != -1) {
                Py_DECREF(tmp);
            }
            RETURN_IF_ERROR(r);
        }
    }
    return SUCCESS;
}

static int
codegen_from_import(compiler *c, stmt_ty s)
{
    Py_ssize_t n = asdl_seq_LEN(s->v.ImportFrom.names);

    ADDOP_LOAD_CONST_NEW(c, LOC(s), PyLong_FromLong(s->v.ImportFrom.level));

    PyObject *names = PyTuple_New(n);
    if (!names) {
        return ERROR;
    }

    /* build up the names */
    for (Py_ssize_t i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.ImportFrom.names, i);
        PyTuple_SET_ITEM(names, i, Py_NewRef(alias->name));
    }

    ADDOP_LOAD_CONST_NEW(c, LOC(s), names);

    if (s->v.ImportFrom.module) {
        ADDOP_NAME(c, LOC(s), IMPORT_NAME, s->v.ImportFrom.module, names);
    }
    else {
        _Py_DECLARE_STR(empty, "");
        ADDOP_NAME(c, LOC(s), IMPORT_NAME, &_Py_STR(empty), names);
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        alias_ty alias = (alias_ty)asdl_seq_GET(s->v.ImportFrom.names, i);
        identifier store_name;

        if (i == 0 && PyUnicode_READ_CHAR(alias->name, 0) == '*') {
            assert(n == 1);
            ADDOP_I(c, LOC(s), CALL_INTRINSIC_1, INTRINSIC_IMPORT_STAR);
            ADDOP(c, NO_LOCATION, POP_TOP);
            return SUCCESS;
        }

        ADDOP_NAME(c, LOC(s), IMPORT_FROM, alias->name, names);
        store_name = alias->name;
        if (alias->asname) {
            store_name = alias->asname;
        }

        RETURN_IF_ERROR(codegen_nameop(c, LOC(s), store_name, Store));
    }
    /* remove imported module */
    ADDOP(c, LOC(s), POP_TOP);
    return SUCCESS;
}

static int
codegen_assert(compiler *c, stmt_ty s)
{
    /* Always emit a warning if the test is a non-zero length tuple */
    if ((s->v.Assert.test->kind == Tuple_kind &&
        asdl_seq_LEN(s->v.Assert.test->v.Tuple.elts) > 0) ||
        (s->v.Assert.test->kind == Constant_kind &&
         PyTuple_Check(s->v.Assert.test->v.Constant.value) &&
         PyTuple_Size(s->v.Assert.test->v.Constant.value) > 0))
    {
        RETURN_IF_ERROR(
            _PyCompile_Warn(c, LOC(s), "assertion is always true, "
                                       "perhaps remove parentheses?"));
    }
    if (OPTIMIZATION_LEVEL(c)) {
        return SUCCESS;
    }
    NEW_JUMP_TARGET_LABEL(c, end);
    RETURN_IF_ERROR(codegen_jump_if(c, LOC(s), s->v.Assert.test, end, 1));
    ADDOP_I(c, LOC(s), LOAD_COMMON_CONSTANT, CONSTANT_ASSERTIONERROR);
    if (s->v.Assert.msg) {
        VISIT(c, expr, s->v.Assert.msg);
        ADDOP_I(c, LOC(s), CALL, 0);
    }
    ADDOP_I(c, LOC(s->v.Assert.test), RAISE_VARARGS, 1);

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
codegen_stmt_expr(compiler *c, location loc, expr_ty value)
{
    if (IS_INTERACTIVE_TOP_LEVEL(c)) {
        VISIT(c, expr, value);
        ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_PRINT);
        ADDOP(c, NO_LOCATION, POP_TOP);
        return SUCCESS;
    }

    if (value->kind == Constant_kind) {
        /* ignore constant statement */
        ADDOP(c, loc, NOP);
        return SUCCESS;
    }

    VISIT(c, expr, value);
    ADDOP(c, NO_LOCATION, POP_TOP); /* artificial */
    return SUCCESS;
}

#define CODEGEN_COND_BLOCK(FUNC, C, S) \
    do { \
        _PyCompile_EnterConditionalBlock((C)); \
        int result = FUNC((C), (S)); \
        _PyCompile_LeaveConditionalBlock((C)); \
        return result; \
    } while(0)

static int
codegen_visit_stmt(compiler *c, stmt_ty s)
{

    switch (s->kind) {
    case FunctionDef_kind:
        return codegen_function(c, s, 0);
    case ClassDef_kind:
        return codegen_class(c, s);
    case TypeAlias_kind:
        return codegen_typealias(c, s);
    case Return_kind:
        return codegen_return(c, s);
    case Delete_kind:
        VISIT_SEQ(c, expr, s->v.Delete.targets);
        break;
    case Assign_kind:
    {
        Py_ssize_t n = asdl_seq_LEN(s->v.Assign.targets);
        VISIT(c, expr, s->v.Assign.value);
        for (Py_ssize_t i = 0; i < n; i++) {
            if (i < n - 1) {
                ADDOP_I(c, LOC(s), COPY, 1);
            }
            VISIT(c, expr,
                  (expr_ty)asdl_seq_GET(s->v.Assign.targets, i));
        }
        break;
    }
    case AugAssign_kind:
        return codegen_augassign(c, s);
    case AnnAssign_kind:
        return codegen_annassign(c, s);
    case For_kind:
        CODEGEN_COND_BLOCK(codegen_for, c, s);
        break;
    case While_kind:
        CODEGEN_COND_BLOCK(codegen_while, c, s);
        break;
    case If_kind:
        CODEGEN_COND_BLOCK(codegen_if, c, s);
        break;
    case Match_kind:
        CODEGEN_COND_BLOCK(codegen_match, c, s);
        break;
    case Raise_kind:
    {
        Py_ssize_t n = 0;
        if (s->v.Raise.exc) {
            VISIT(c, expr, s->v.Raise.exc);
            n++;
            if (s->v.Raise.cause) {
                VISIT(c, expr, s->v.Raise.cause);
                n++;
            }
        }
        ADDOP_I(c, LOC(s), RAISE_VARARGS, (int)n);
        break;
    }
    case Try_kind:
        CODEGEN_COND_BLOCK(codegen_try, c, s);
        break;
    case TryStar_kind:
        CODEGEN_COND_BLOCK(codegen_try_star, c, s);
        break;
    case Assert_kind:
        return codegen_assert(c, s);
    case Import_kind:
        return codegen_import(c, s);
    case ImportFrom_kind:
        return codegen_from_import(c, s);
    case Global_kind:
    case Nonlocal_kind:
        break;
    case Expr_kind:
    {
        return codegen_stmt_expr(c, LOC(s), s->v.Expr.value);
    }
    case Pass_kind:
    {
        ADDOP(c, LOC(s), NOP);
        break;
    }
    case Break_kind:
    {
        return codegen_break(c, LOC(s));
    }
    case Continue_kind:
    {
        return codegen_continue(c, LOC(s));
    }
    case With_kind:
        CODEGEN_COND_BLOCK(codegen_with, c, s);
        break;
    case AsyncFunctionDef_kind:
        return codegen_function(c, s, 1);
    case AsyncWith_kind:
        CODEGEN_COND_BLOCK(codegen_async_with, c, s);
        break;
    case AsyncFor_kind:
        CODEGEN_COND_BLOCK(codegen_async_for, c, s);
        break;
    }

    return SUCCESS;
}

static int
unaryop(unaryop_ty op)
{
    switch (op) {
    case Invert:
        return UNARY_INVERT;
    case USub:
        return UNARY_NEGATIVE;
    default:
        PyErr_Format(PyExc_SystemError,
            "unary op %d should not be possible", op);
        return 0;
    }
}

static int
addop_binary(compiler *c, location loc, operator_ty binop,
             bool inplace)
{
    int oparg;
    switch (binop) {
        case Add:
            oparg = inplace ? NB_INPLACE_ADD : NB_ADD;
            break;
        case Sub:
            oparg = inplace ? NB_INPLACE_SUBTRACT : NB_SUBTRACT;
            break;
        case Mult:
            oparg = inplace ? NB_INPLACE_MULTIPLY : NB_MULTIPLY;
            break;
        case MatMult:
            oparg = inplace ? NB_INPLACE_MATRIX_MULTIPLY : NB_MATRIX_MULTIPLY;
            break;
        case Div:
            oparg = inplace ? NB_INPLACE_TRUE_DIVIDE : NB_TRUE_DIVIDE;
            break;
        case Mod:
            oparg = inplace ? NB_INPLACE_REMAINDER : NB_REMAINDER;
            break;
        case Pow:
            oparg = inplace ? NB_INPLACE_POWER : NB_POWER;
            break;
        case LShift:
            oparg = inplace ? NB_INPLACE_LSHIFT : NB_LSHIFT;
            break;
        case RShift:
            oparg = inplace ? NB_INPLACE_RSHIFT : NB_RSHIFT;
            break;
        case BitOr:
            oparg = inplace ? NB_INPLACE_OR : NB_OR;
            break;
        case BitXor:
            oparg = inplace ? NB_INPLACE_XOR : NB_XOR;
            break;
        case BitAnd:
            oparg = inplace ? NB_INPLACE_AND : NB_AND;
            break;
        case FloorDiv:
            oparg = inplace ? NB_INPLACE_FLOOR_DIVIDE : NB_FLOOR_DIVIDE;
            break;
        default:
            PyErr_Format(PyExc_SystemError, "%s op %d should not be possible",
                         inplace ? "inplace" : "binary", binop);
            return ERROR;
    }
    ADDOP_I(c, loc, BINARY_OP, oparg);
    return SUCCESS;
}


static int
codegen_addop_yield(compiler *c, location loc) {
    PySTEntryObject *ste = SYMTABLE_ENTRY(c);
    if (ste->ste_generator && ste->ste_coroutine) {
        ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_ASYNC_GEN_WRAP);
    }
    ADDOP_I(c, loc, YIELD_VALUE, 0);
    ADDOP_I(c, loc, RESUME, RESUME_AFTER_YIELD);
    return SUCCESS;
}

static int
codegen_load_classdict_freevar(compiler *c, location loc)
{
    ADDOP_N(c, loc, LOAD_DEREF, &_Py_ID(__classdict__), freevars);
    return SUCCESS;
}

static int
codegen_nameop(compiler *c, location loc,
               identifier name, expr_context_ty ctx)
{
    assert(!_PyUnicode_EqualToASCIIString(name, "None") &&
           !_PyUnicode_EqualToASCIIString(name, "True") &&
           !_PyUnicode_EqualToASCIIString(name, "False"));

    PyObject *mangled = _PyCompile_MaybeMangle(c, name);
    if (!mangled) {
        return ERROR;
    }

    int scope = _PyST_GetScope(SYMTABLE_ENTRY(c), mangled);
    RETURN_IF_ERROR(scope);
    _PyCompile_optype optype;
    Py_ssize_t arg = 0;
    if (_PyCompile_ResolveNameop(c, mangled, scope, &optype, &arg) < 0) {
        Py_DECREF(mangled);
        return ERROR;
    }

    /* XXX Leave assert here, but handle __doc__ and the like better */
    assert(scope || PyUnicode_READ_CHAR(name, 0) == '_');

    int op = 0;
    switch (optype) {
    case COMPILE_OP_DEREF:
        switch (ctx) {
        case Load:
            if (SYMTABLE_ENTRY(c)->ste_type == ClassBlock && !_PyCompile_IsInInlinedComp(c)) {
                op = LOAD_FROM_DICT_OR_DEREF;
                // First load the locals
                if (codegen_addop_noarg(INSTR_SEQUENCE(c), LOAD_LOCALS, loc) < 0) {
                    goto error;
                }
            }
            else if (SYMTABLE_ENTRY(c)->ste_can_see_class_scope) {
                op = LOAD_FROM_DICT_OR_DEREF;
                // First load the classdict
                if (codegen_load_classdict_freevar(c, loc) < 0) {
                    goto error;
                }
            }
            else {
                op = LOAD_DEREF;
            }
            break;
        case Store: op = STORE_DEREF; break;
        case Del: op = DELETE_DEREF; break;
        }
        break;
    case COMPILE_OP_FAST:
        switch (ctx) {
        case Load: op = LOAD_FAST; break;
        case Store: op = STORE_FAST; break;
        case Del: op = DELETE_FAST; break;
        }
        ADDOP_N(c, loc, op, mangled, varnames);
        return SUCCESS;
    case COMPILE_OP_GLOBAL:
        switch (ctx) {
        case Load:
            if (SYMTABLE_ENTRY(c)->ste_can_see_class_scope && scope == GLOBAL_IMPLICIT) {
                op = LOAD_FROM_DICT_OR_GLOBALS;
                // First load the classdict
                if (codegen_load_classdict_freevar(c, loc) < 0) {
                    goto error;
                }
            } else {
                op = LOAD_GLOBAL;
            }
            break;
        case Store: op = STORE_GLOBAL; break;
        case Del: op = DELETE_GLOBAL; break;
        }
        break;
    case COMPILE_OP_NAME:
        switch (ctx) {
        case Load:
            op = (SYMTABLE_ENTRY(c)->ste_type == ClassBlock
                    && _PyCompile_IsInInlinedComp(c))
                ? LOAD_GLOBAL
                : LOAD_NAME;
            break;
        case Store: op = STORE_NAME; break;
        case Del: op = DELETE_NAME; break;
        }
        break;
    }

    assert(op);
    Py_DECREF(mangled);
    if (op == LOAD_GLOBAL) {
        arg <<= 1;
    }
    ADDOP_I(c, loc, op, arg);
    return SUCCESS;

error:
    Py_DECREF(mangled);
    return ERROR;
}

static int
codegen_boolop(compiler *c, expr_ty e)
{
    int jumpi;
    Py_ssize_t i, n;
    asdl_expr_seq *s;

    location loc = LOC(e);
    assert(e->kind == BoolOp_kind);
    if (e->v.BoolOp.op == And)
        jumpi = JUMP_IF_FALSE;
    else
        jumpi = JUMP_IF_TRUE;
    NEW_JUMP_TARGET_LABEL(c, end);
    s = e->v.BoolOp.values;
    n = asdl_seq_LEN(s) - 1;
    assert(n >= 0);
    for (i = 0; i < n; ++i) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(s, i));
        ADDOP_JUMP(c, loc, jumpi, end);
        ADDOP(c, loc, POP_TOP);
    }
    VISIT(c, expr, (expr_ty)asdl_seq_GET(s, n));

    USE_LABEL(c, end);
    return SUCCESS;
}

static int
starunpack_helper_impl(compiler *c, location loc,
                       asdl_expr_seq *elts, PyObject *injected_arg, int pushed,
                       int build, int add, int extend, int tuple)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    int big = n + pushed + (injected_arg ? 1 : 0) > _PY_STACK_USE_GUIDELINE;
    int seen_star = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == Starred_kind) {
            seen_star = 1;
            break;
        }
    }
    if (!seen_star && !big) {
        for (Py_ssize_t i = 0; i < n; i++) {
            expr_ty elt = asdl_seq_GET(elts, i);
            VISIT(c, expr, elt);
        }
        if (injected_arg) {
            RETURN_IF_ERROR(codegen_nameop(c, loc, injected_arg, Load));
            n++;
        }
        if (tuple) {
            ADDOP_I(c, loc, BUILD_TUPLE, n+pushed);
        } else {
            ADDOP_I(c, loc, build, n+pushed);
        }
        return SUCCESS;
    }
    int sequence_built = 0;
    if (big) {
        ADDOP_I(c, loc, build, pushed);
        sequence_built = 1;
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == Starred_kind) {
            if (sequence_built == 0) {
                ADDOP_I(c, loc, build, i+pushed);
                sequence_built = 1;
            }
            VISIT(c, expr, elt->v.Starred.value);
            ADDOP_I(c, loc, extend, 1);
        }
        else {
            VISIT(c, expr, elt);
            if (sequence_built) {
                ADDOP_I(c, loc, add, 1);
            }
        }
    }
    assert(sequence_built);
    if (injected_arg) {
        RETURN_IF_ERROR(codegen_nameop(c, loc, injected_arg, Load));
        ADDOP_I(c, loc, add, 1);
    }
    if (tuple) {
        ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_LIST_TO_TUPLE);
    }
    return SUCCESS;
}

static int
starunpack_helper(compiler *c, location loc,
                  asdl_expr_seq *elts, int pushed,
                  int build, int add, int extend, int tuple)
{
    return starunpack_helper_impl(c, loc, elts, NULL, pushed,
                                  build, add, extend, tuple);
}

static int
unpack_helper(compiler *c, location loc, asdl_expr_seq *elts)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    int seen_star = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == Starred_kind && !seen_star) {
            if ((i >= (1 << 8)) ||
                (n-i-1 >= (INT_MAX >> 8))) {
                return _PyCompile_Error(c, loc,
                    "too many expressions in "
                    "star-unpacking assignment");
            }
            ADDOP_I(c, loc, UNPACK_EX, (i + ((n-i-1) << 8)));
            seen_star = 1;
        }
        else if (elt->kind == Starred_kind) {
            return _PyCompile_Error(c, loc,
                "multiple starred expressions in assignment");
        }
    }
    if (!seen_star) {
        ADDOP_I(c, loc, UNPACK_SEQUENCE, n);
    }
    return SUCCESS;
}

static int
assignment_helper(compiler *c, location loc, asdl_expr_seq *elts)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    RETURN_IF_ERROR(unpack_helper(c, loc, elts));
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty elt = asdl_seq_GET(elts, i);
        VISIT(c, expr, elt->kind != Starred_kind ? elt : elt->v.Starred.value);
    }
    return SUCCESS;
}

static int
codegen_list(compiler *c, expr_ty e)
{
    location loc = LOC(e);
    asdl_expr_seq *elts = e->v.List.elts;
    if (e->v.List.ctx == Store) {
        return assignment_helper(c, loc, elts);
    }
    else if (e->v.List.ctx == Load) {
        return starunpack_helper(c, loc, elts, 0,
                                 BUILD_LIST, LIST_APPEND, LIST_EXTEND, 0);
    }
    else {
        VISIT_SEQ(c, expr, elts);
    }
    return SUCCESS;
}

static int
codegen_tuple(compiler *c, expr_ty e)
{
    location loc = LOC(e);
    asdl_expr_seq *elts = e->v.Tuple.elts;
    if (e->v.Tuple.ctx == Store) {
        return assignment_helper(c, loc, elts);
    }
    else if (e->v.Tuple.ctx == Load) {
        return starunpack_helper(c, loc, elts, 0,
                                 BUILD_LIST, LIST_APPEND, LIST_EXTEND, 1);
    }
    else {
        VISIT_SEQ(c, expr, elts);
    }
    return SUCCESS;
}

static int
codegen_set(compiler *c, expr_ty e)
{
    location loc = LOC(e);
    return starunpack_helper(c, loc, e->v.Set.elts, 0,
                             BUILD_SET, SET_ADD, SET_UPDATE, 0);
}

static int
codegen_subdict(compiler *c, expr_ty e, Py_ssize_t begin, Py_ssize_t end)
{
    Py_ssize_t i, n = end - begin;
    int big = n*2 > _PY_STACK_USE_GUIDELINE;
    location loc = LOC(e);
    if (big) {
        ADDOP_I(c, loc, BUILD_MAP, 0);
    }
    for (i = begin; i < end; i++) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.keys, i));
        VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
        if (big) {
            ADDOP_I(c, loc, MAP_ADD, 1);
        }
    }
    if (!big) {
        ADDOP_I(c, loc, BUILD_MAP, n);
    }
    return SUCCESS;
}

static int
codegen_dict(compiler *c, expr_ty e)
{
    location loc = LOC(e);
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
                RETURN_IF_ERROR(codegen_subdict(c, e, i - elements, i));
                if (have_dict) {
                    ADDOP_I(c, loc, DICT_UPDATE, 1);
                }
                have_dict = 1;
                elements = 0;
            }
            if (have_dict == 0) {
                ADDOP_I(c, loc, BUILD_MAP, 0);
                have_dict = 1;
            }
            VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Dict.values, i));
            ADDOP_I(c, loc, DICT_UPDATE, 1);
        }
        else {
            if (elements*2 > _PY_STACK_USE_GUIDELINE) {
                RETURN_IF_ERROR(codegen_subdict(c, e, i - elements, i + 1));
                if (have_dict) {
                    ADDOP_I(c, loc, DICT_UPDATE, 1);
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
        RETURN_IF_ERROR(codegen_subdict(c, e, n - elements, n));
        if (have_dict) {
            ADDOP_I(c, loc, DICT_UPDATE, 1);
        }
        have_dict = 1;
    }
    if (!have_dict) {
        ADDOP_I(c, loc, BUILD_MAP, 0);
    }
    return SUCCESS;
}

static int
codegen_compare(compiler *c, expr_ty e)
{
    location loc = LOC(e);
    Py_ssize_t i, n;

    RETURN_IF_ERROR(codegen_check_compare(c, e));
    VISIT(c, expr, e->v.Compare.left);
    assert(asdl_seq_LEN(e->v.Compare.ops) > 0);
    n = asdl_seq_LEN(e->v.Compare.ops) - 1;
    if (n == 0) {
        VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Compare.comparators, 0));
        ADDOP_COMPARE(c, loc, asdl_seq_GET(e->v.Compare.ops, 0));
    }
    else {
        NEW_JUMP_TARGET_LABEL(c, cleanup);
        for (i = 0; i < n; i++) {
            VISIT(c, expr,
                (expr_ty)asdl_seq_GET(e->v.Compare.comparators, i));
            ADDOP_I(c, loc, SWAP, 2);
            ADDOP_I(c, loc, COPY, 2);
            ADDOP_COMPARE(c, loc, asdl_seq_GET(e->v.Compare.ops, i));
            ADDOP_I(c, loc, COPY, 1);
            ADDOP(c, loc, TO_BOOL);
            ADDOP_JUMP(c, loc, POP_JUMP_IF_FALSE, cleanup);
            ADDOP(c, loc, POP_TOP);
        }
        VISIT(c, expr, (expr_ty)asdl_seq_GET(e->v.Compare.comparators, n));
        ADDOP_COMPARE(c, loc, asdl_seq_GET(e->v.Compare.ops, n));
        NEW_JUMP_TARGET_LABEL(c, end);
        ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, end);

        USE_LABEL(c, cleanup);
        ADDOP_I(c, loc, SWAP, 2);
        ADDOP(c, loc, POP_TOP);

        USE_LABEL(c, end);
    }
    return SUCCESS;
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
    case TemplateStr_kind:
    case Interpolation_kind:
        return &_PyTemplate_Type;
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
check_caller(compiler *c, expr_ty e)
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
    case TemplateStr_kind:
    case FormattedValue_kind:
    case Interpolation_kind: {
        location loc = LOC(e);
        return _PyCompile_Warn(c, loc, "'%.200s' object is not callable; "
                                       "perhaps you missed a comma?",
                                       infer_type(e)->tp_name);
    }
    default:
        return SUCCESS;
    }
}

static int
check_subscripter(compiler *c, expr_ty e)
{
    PyObject *v;

    switch (e->kind) {
    case Constant_kind:
        v = e->v.Constant.value;
        if (!(v == Py_None || v == Py_Ellipsis ||
              PyLong_Check(v) || PyFloat_Check(v) || PyComplex_Check(v) ||
              PyAnySet_Check(v)))
        {
            return SUCCESS;
        }
        _Py_FALLTHROUGH;
    case Set_kind:
    case SetComp_kind:
    case GeneratorExp_kind:
    case TemplateStr_kind:
    case Interpolation_kind:
    case Lambda_kind: {
        location loc = LOC(e);
        return _PyCompile_Warn(c, loc, "'%.200s' object is not subscriptable; "
                                       "perhaps you missed a comma?",
                                       infer_type(e)->tp_name);
    }
    default:
        return SUCCESS;
    }
}

static int
check_index(compiler *c, expr_ty e, expr_ty s)
{
    PyObject *v;

    PyTypeObject *index_type = infer_type(s);
    if (index_type == NULL
        || PyType_FastSubclass(index_type, Py_TPFLAGS_LONG_SUBCLASS)
        || index_type == &PySlice_Type) {
        return SUCCESS;
    }

    switch (e->kind) {
    case Constant_kind:
        v = e->v.Constant.value;
        if (!(PyUnicode_Check(v) || PyBytes_Check(v) || PyTuple_Check(v))) {
            return SUCCESS;
        }
        _Py_FALLTHROUGH;
    case Tuple_kind:
    case List_kind:
    case ListComp_kind:
    case JoinedStr_kind:
    case FormattedValue_kind: {
        location loc = LOC(e);
        return _PyCompile_Warn(c, loc, "%.200s indices must be integers "
                                       "or slices, not %.200s; "
                                       "perhaps you missed a comma?",
                                       infer_type(e)->tp_name,
                                       index_type->tp_name);
    }
    default:
        return SUCCESS;
    }
}

static int
is_import_originated(compiler *c, expr_ty e)
{
    /* Check whether the global scope has an import named
     e, if it is a Name object. For not traversing all the
     scope stack every time this function is called, it will
     only check the global scope to determine whether something
     is imported or not. */

    if (e->kind != Name_kind) {
        return 0;
    }

    long flags = _PyST_GetSymbol(SYMTABLE(c)->st_top, e->v.Name.id);
    RETURN_IF_ERROR(flags);
    return flags & DEF_IMPORT;
}

static int
can_optimize_super_call(compiler *c, expr_ty attr)
{
    expr_ty e = attr->v.Attribute.value;
    if (e->kind != Call_kind ||
        e->v.Call.func->kind != Name_kind ||
        !_PyUnicode_EqualToASCIIString(e->v.Call.func->v.Name.id, "super") ||
        _PyUnicode_EqualToASCIIString(attr->v.Attribute.attr, "__class__") ||
        asdl_seq_LEN(e->v.Call.keywords) != 0) {
        return 0;
    }
    Py_ssize_t num_args = asdl_seq_LEN(e->v.Call.args);

    PyObject *super_name = e->v.Call.func->v.Name.id;
    // detect statically-visible shadowing of 'super' name
    int scope = _PyST_GetScope(SYMTABLE_ENTRY(c), super_name);
    RETURN_IF_ERROR(scope);
    if (scope != GLOBAL_IMPLICIT) {
        return 0;
    }
    scope = _PyST_GetScope(SYMTABLE(c)->st_top, super_name);
    RETURN_IF_ERROR(scope);
    if (scope != 0) {
        return 0;
    }

    if (num_args == 2) {
        for (Py_ssize_t i = 0; i < num_args; i++) {
            expr_ty elt = asdl_seq_GET(e->v.Call.args, i);
            if (elt->kind == Starred_kind) {
                return 0;
            }
        }
        // exactly two non-starred args; we can just load
        // the provided args
        return 1;
    }

    if (num_args != 0) {
        return 0;
    }
    // we need the following for zero-arg super():

    // enclosing function should have at least one argument
    if (METADATA(c)->u_argcount == 0 &&
        METADATA(c)->u_posonlyargcount == 0) {
        return 0;
    }
    // __class__ cell should be available
    if (_PyCompile_GetRefType(c, &_Py_ID(__class__)) == FREE) {
        return 1;
    }
    return 0;
}

static int
load_args_for_super(compiler *c, expr_ty e) {
    location loc = LOC(e);

    // load super() global
    PyObject *super_name = e->v.Call.func->v.Name.id;
    RETURN_IF_ERROR(codegen_nameop(c, LOC(e->v.Call.func), super_name, Load));

    if (asdl_seq_LEN(e->v.Call.args) == 2) {
        VISIT(c, expr, asdl_seq_GET(e->v.Call.args, 0));
        VISIT(c, expr, asdl_seq_GET(e->v.Call.args, 1));
        return SUCCESS;
    }

    // load __class__ cell
    PyObject *name = &_Py_ID(__class__);
    assert(_PyCompile_GetRefType(c, name) == FREE);
    RETURN_IF_ERROR(codegen_nameop(c, loc, name, Load));

    // load self (first argument)
    Py_ssize_t i = 0;
    PyObject *key, *value;
    if (!PyDict_Next(METADATA(c)->u_varnames, &i, &key, &value)) {
        return ERROR;
    }
    RETURN_IF_ERROR(codegen_nameop(c, loc, key, Load));

    return SUCCESS;
}

// If an attribute access spans multiple lines, update the current start
// location to point to the attribute name.
static location
update_start_location_to_match_attr(compiler *c, location loc,
                                    expr_ty attr)
{
    assert(attr->kind == Attribute_kind);
    if (loc.lineno != attr->end_lineno) {
        loc.lineno = attr->end_lineno;
        int len = (int)PyUnicode_GET_LENGTH(attr->v.Attribute.attr);
        if (len <= attr->end_col_offset) {
            loc.col_offset = attr->end_col_offset - len;
        }
        else {
            // GH-94694: Somebody's compiling weird ASTs. Just drop the columns:
            loc.col_offset = -1;
            loc.end_col_offset = -1;
        }
        // Make sure the end position still follows the start position, even for
        // weird ASTs:
        loc.end_lineno = Py_MAX(loc.lineno, loc.end_lineno);
        if (loc.lineno == loc.end_lineno) {
            loc.end_col_offset = Py_MAX(loc.col_offset, loc.end_col_offset);
        }
    }
    return loc;
}

static int
maybe_optimize_function_call(compiler *c, expr_ty e, jump_target_label end)
{
    asdl_expr_seq *args = e->v.Call.args;
    asdl_keyword_seq *kwds = e->v.Call.keywords;
    expr_ty func = e->v.Call.func;

    if (! (func->kind == Name_kind &&
           asdl_seq_LEN(args) == 1 &&
           asdl_seq_LEN(kwds) == 0 &&
           asdl_seq_GET(args, 0)->kind == GeneratorExp_kind))
    {
        return 0;
    }

    location loc = LOC(func);

    int optimized = 0;
    NEW_JUMP_TARGET_LABEL(c, skip_optimization);

    int const_oparg = -1;
    PyObject *initial_res = NULL;
    int continue_jump_opcode = -1;
    if (_PyUnicode_EqualToASCIIString(func->v.Name.id, "all")) {
        const_oparg = CONSTANT_BUILTIN_ALL;
        initial_res = Py_True;
        continue_jump_opcode = POP_JUMP_IF_TRUE;
    }
    else if (_PyUnicode_EqualToASCIIString(func->v.Name.id, "any")) {
        const_oparg = CONSTANT_BUILTIN_ANY;
        initial_res = Py_False;
        continue_jump_opcode = POP_JUMP_IF_FALSE;
    }
    else if (_PyUnicode_EqualToASCIIString(func->v.Name.id, "tuple")) {
        const_oparg = CONSTANT_BUILTIN_TUPLE;
    }
    if (const_oparg != -1) {
        ADDOP_I(c, loc, COPY, 1); // the function
        ADDOP_I(c, loc, LOAD_COMMON_CONSTANT, const_oparg);
        ADDOP_COMPARE(c, loc, Is);
        ADDOP_JUMP(c, loc, POP_JUMP_IF_FALSE, skip_optimization);
        ADDOP(c, loc, POP_TOP);

        if (const_oparg == CONSTANT_BUILTIN_TUPLE) {
            ADDOP_I(c, loc, BUILD_LIST, 0);
        }
        expr_ty generator_exp = asdl_seq_GET(args, 0);
        VISIT(c, expr, generator_exp);

        NEW_JUMP_TARGET_LABEL(c, loop);
        NEW_JUMP_TARGET_LABEL(c, cleanup);

        ADDOP(c, loc, PUSH_NULL); // Push NULL index for loop
        USE_LABEL(c, loop);
        ADDOP_JUMP(c, loc, FOR_ITER, cleanup);
        if (const_oparg == CONSTANT_BUILTIN_TUPLE) {
            ADDOP_I(c, loc, LIST_APPEND, 3);
            ADDOP_JUMP(c, loc, JUMP, loop);
        }
        else {
            ADDOP(c, loc, TO_BOOL);
            ADDOP_JUMP(c, loc, continue_jump_opcode, loop);
        }

        ADDOP(c, NO_LOCATION, POP_ITER);
        if (const_oparg != CONSTANT_BUILTIN_TUPLE) {
            ADDOP_LOAD_CONST(c, loc, initial_res == Py_True ? Py_False : Py_True);
        }
        ADDOP_JUMP(c, loc, JUMP, end);

        USE_LABEL(c, cleanup);
        ADDOP(c, NO_LOCATION, END_FOR);
        ADDOP(c, NO_LOCATION, POP_ITER);
        if (const_oparg == CONSTANT_BUILTIN_TUPLE) {
            ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_LIST_TO_TUPLE);
        }
        else {
            ADDOP_LOAD_CONST(c, loc, initial_res);
        }

        optimized = 1;
        ADDOP_JUMP(c, loc, JUMP, end);
    }
    USE_LABEL(c, skip_optimization);
    return optimized;
}

// Return 1 if the method call was optimized, 0 if not, and -1 on error.
static int
maybe_optimize_method_call(compiler *c, expr_ty e)
{
    Py_ssize_t argsl, i, kwdsl;
    expr_ty meth = e->v.Call.func;
    asdl_expr_seq *args = e->v.Call.args;
    asdl_keyword_seq *kwds = e->v.Call.keywords;

    /* Check that the call node is an attribute access */
    if (meth->kind != Attribute_kind || meth->v.Attribute.ctx != Load) {
        return 0;
    }

    /* Check that the base object is not something that is imported */
    int ret = is_import_originated(c, meth->v.Attribute.value);
    RETURN_IF_ERROR(ret);
    if (ret) {
        return 0;
    }

    /* Check that there aren't too many arguments */
    argsl = asdl_seq_LEN(args);
    kwdsl = asdl_seq_LEN(kwds);
    if (argsl + kwdsl + (kwdsl != 0) >= _PY_STACK_USE_GUIDELINE) {
        return 0;
    }
    /* Check that there are no *varargs types of arguments. */
    for (i = 0; i < argsl; i++) {
        expr_ty elt = asdl_seq_GET(args, i);
        if (elt->kind == Starred_kind) {
            return 0;
        }
    }

    for (i = 0; i < kwdsl; i++) {
        keyword_ty kw = asdl_seq_GET(kwds, i);
        if (kw->arg == NULL) {
            return 0;
        }
    }

    /* Alright, we can optimize the code. */
    location loc = LOC(meth);

    ret = can_optimize_super_call(c, meth);
    RETURN_IF_ERROR(ret);
    if (ret) {
        RETURN_IF_ERROR(load_args_for_super(c, meth->v.Attribute.value));
        int opcode = asdl_seq_LEN(meth->v.Attribute.value->v.Call.args) ?
            LOAD_SUPER_METHOD : LOAD_ZERO_SUPER_METHOD;
        ADDOP_NAME(c, loc, opcode, meth->v.Attribute.attr, names);
        loc = update_start_location_to_match_attr(c, loc, meth);
        ADDOP(c, loc, NOP);
    } else {
        VISIT(c, expr, meth->v.Attribute.value);
        loc = update_start_location_to_match_attr(c, loc, meth);
        ADDOP_NAME(c, loc, LOAD_METHOD, meth->v.Attribute.attr, names);
    }

    VISIT_SEQ(c, expr, e->v.Call.args);

    if (kwdsl) {
        VISIT_SEQ(c, keyword, kwds);
        RETURN_IF_ERROR(
            codegen_call_simple_kw_helper(c, loc, kwds, kwdsl));
        loc = update_start_location_to_match_attr(c, LOC(e), meth);
        ADDOP_I(c, loc, CALL_KW, argsl + kwdsl);
    }
    else {
        loc = update_start_location_to_match_attr(c, LOC(e), meth);
        ADDOP_I(c, loc, CALL, argsl);
    }
    return 1;
}

static int
codegen_validate_keywords(compiler *c, asdl_keyword_seq *keywords)
{
    Py_ssize_t nkeywords = asdl_seq_LEN(keywords);
    for (Py_ssize_t i = 0; i < nkeywords; i++) {
        keyword_ty key = ((keyword_ty)asdl_seq_GET(keywords, i));
        if (key->arg == NULL) {
            continue;
        }
        for (Py_ssize_t j = i + 1; j < nkeywords; j++) {
            keyword_ty other = ((keyword_ty)asdl_seq_GET(keywords, j));
            if (other->arg && !PyUnicode_Compare(key->arg, other->arg)) {
                return _PyCompile_Error(c, LOC(other), "keyword argument repeated: %U", key->arg);
            }
        }
    }
    return SUCCESS;
}

static int
codegen_call(compiler *c, expr_ty e)
{
    RETURN_IF_ERROR(codegen_validate_keywords(c, e->v.Call.keywords));
    int ret = maybe_optimize_method_call(c, e);
    if (ret < 0) {
        return ERROR;
    }
    if (ret == 1) {
        return SUCCESS;
    }
    NEW_JUMP_TARGET_LABEL(c, skip_normal_call);
    RETURN_IF_ERROR(check_caller(c, e->v.Call.func));
    VISIT(c, expr, e->v.Call.func);
    RETURN_IF_ERROR(maybe_optimize_function_call(c, e, skip_normal_call));
    location loc = LOC(e->v.Call.func);
    ADDOP(c, loc, PUSH_NULL);
    loc = LOC(e);
    ret = codegen_call_helper(c, loc, 0,
                              e->v.Call.args,
                              e->v.Call.keywords);
    USE_LABEL(c, skip_normal_call);
    return ret;
}

static int
codegen_template_str(compiler *c, expr_ty e)
{
    location loc = LOC(e);
    expr_ty value;

    Py_ssize_t value_count = asdl_seq_LEN(e->v.TemplateStr.values);
    int last_was_interpolation = 1;
    Py_ssize_t stringslen = 0;
    for (Py_ssize_t i = 0; i < value_count; i++) {
        value = asdl_seq_GET(e->v.TemplateStr.values, i);
        if (value->kind == Interpolation_kind) {
            if (last_was_interpolation) {
                ADDOP_LOAD_CONST(c, loc, Py_NewRef(&_Py_STR(empty)));
                stringslen++;
            }
            last_was_interpolation = 1;
        }
        else {
            VISIT(c, expr, value);
            stringslen++;
            last_was_interpolation = 0;
        }
    }
    if (last_was_interpolation) {
        ADDOP_LOAD_CONST(c, loc, Py_NewRef(&_Py_STR(empty)));
        stringslen++;
    }
    ADDOP_I(c, loc, BUILD_TUPLE, stringslen);

    Py_ssize_t interpolationslen = 0;
    for (Py_ssize_t i = 0; i < value_count; i++) {
        value = asdl_seq_GET(e->v.TemplateStr.values, i);
        if (value->kind == Interpolation_kind) {
            VISIT(c, expr, value);
            interpolationslen++;
        }
    }
    ADDOP_I(c, loc, BUILD_TUPLE, interpolationslen);
    ADDOP(c, loc, BUILD_TEMPLATE);
    return SUCCESS;
}

static int
codegen_joined_str(compiler *c, expr_ty e)
{
    location loc = LOC(e);
    Py_ssize_t value_count = asdl_seq_LEN(e->v.JoinedStr.values);
    if (value_count > _PY_STACK_USE_GUIDELINE) {
        _Py_DECLARE_STR(empty, "");
        ADDOP_LOAD_CONST_NEW(c, loc, Py_NewRef(&_Py_STR(empty)));
        ADDOP_NAME(c, loc, LOAD_METHOD, &_Py_ID(join), names);
        ADDOP_I(c, loc, BUILD_LIST, 0);
        for (Py_ssize_t i = 0; i < asdl_seq_LEN(e->v.JoinedStr.values); i++) {
            VISIT(c, expr, asdl_seq_GET(e->v.JoinedStr.values, i));
            ADDOP_I(c, loc, LIST_APPEND, 1);
        }
        ADDOP_I(c, loc, CALL, 1);
    }
    else {
        VISIT_SEQ(c, expr, e->v.JoinedStr.values);
        if (value_count > 1) {
            ADDOP_I(c, loc, BUILD_STRING, value_count);
        }
        else if (value_count == 0) {
            _Py_DECLARE_STR(empty, "");
            ADDOP_LOAD_CONST_NEW(c, loc, Py_NewRef(&_Py_STR(empty)));
        }
    }
    return SUCCESS;
}

static int
codegen_interpolation(compiler *c, expr_ty e)
{
    location loc = LOC(e);

    VISIT(c, expr, e->v.Interpolation.value);
    ADDOP_LOAD_CONST(c, loc, e->v.Interpolation.str);

    int oparg = 2;
    if (e->v.Interpolation.format_spec) {
        oparg++;
        VISIT(c, expr, e->v.Interpolation.format_spec);
    }

    int conversion = e->v.Interpolation.conversion;
    if (conversion != -1) {
        switch (conversion) {
        case 's': oparg |= FVC_STR << 2;   break;
        case 'r': oparg |= FVC_REPR << 2;  break;
        case 'a': oparg |= FVC_ASCII << 2; break;
        default:
            PyErr_Format(PyExc_SystemError,
                     "Unrecognized conversion character %d", conversion);
            return ERROR;
        }
    }

    ADDOP_I(c, loc, BUILD_INTERPOLATION, oparg);
    return SUCCESS;
}

/* Used to implement f-strings. Format a single value. */
static int
codegen_formatted_value(compiler *c, expr_ty e)
{
    int conversion = e->v.FormattedValue.conversion;
    int oparg;

    /* The expression to be formatted. */
    VISIT(c, expr, e->v.FormattedValue.value);

    location loc = LOC(e);
    if (conversion != -1) {
        switch (conversion) {
        case 's': oparg = FVC_STR;   break;
        case 'r': oparg = FVC_REPR;  break;
        case 'a': oparg = FVC_ASCII; break;
        default:
            PyErr_Format(PyExc_SystemError,
                     "Unrecognized conversion character %d", conversion);
            return ERROR;
        }
        ADDOP_I(c, loc, CONVERT_VALUE, oparg);
    }
    if (e->v.FormattedValue.format_spec) {
        /* Evaluate the format spec, and update our opcode arg. */
        VISIT(c, expr, e->v.FormattedValue.format_spec);
        ADDOP(c, loc, FORMAT_WITH_SPEC);
    } else {
        ADDOP(c, loc, FORMAT_SIMPLE);
    }
    return SUCCESS;
}

static int
codegen_subkwargs(compiler *c, location loc,
                  asdl_keyword_seq *keywords,
                  Py_ssize_t begin, Py_ssize_t end)
{
    Py_ssize_t i, n = end - begin;
    keyword_ty kw;
    assert(n > 0);
    int big = n*2 > _PY_STACK_USE_GUIDELINE;
    if (big) {
        ADDOP_I(c, NO_LOCATION, BUILD_MAP, 0);
    }
    for (i = begin; i < end; i++) {
        kw = asdl_seq_GET(keywords, i);
        ADDOP_LOAD_CONST(c, loc, kw->arg);
        VISIT(c, expr, kw->value);
        if (big) {
            ADDOP_I(c, NO_LOCATION, MAP_ADD, 1);
        }
    }
    if (!big) {
        ADDOP_I(c, loc, BUILD_MAP, n);
    }
    return SUCCESS;
}

/* Used by codegen_call_helper and maybe_optimize_method_call to emit
 * a tuple of keyword names before CALL.
 */
static int
codegen_call_simple_kw_helper(compiler *c, location loc,
                              asdl_keyword_seq *keywords, Py_ssize_t nkwelts)
{
    PyObject *names;
    names = PyTuple_New(nkwelts);
    if (names == NULL) {
        return ERROR;
    }
    for (Py_ssize_t i = 0; i < nkwelts; i++) {
        keyword_ty kw = asdl_seq_GET(keywords, i);
        PyTuple_SET_ITEM(names, i, Py_NewRef(kw->arg));
    }
    ADDOP_LOAD_CONST_NEW(c, loc, names);
    return SUCCESS;
}

/* shared code between codegen_call and codegen_class */
static int
codegen_call_helper_impl(compiler *c, location loc,
                         int n, /* Args already pushed */
                         asdl_expr_seq *args,
                         PyObject *injected_arg,
                         asdl_keyword_seq *keywords)
{
    Py_ssize_t i, nseen, nelts, nkwelts;

    RETURN_IF_ERROR(codegen_validate_keywords(c, keywords));

    nelts = asdl_seq_LEN(args);
    nkwelts = asdl_seq_LEN(keywords);

    if (nelts + nkwelts*2 > _PY_STACK_USE_GUIDELINE) {
         goto ex_call;
    }
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
    if (injected_arg) {
        RETURN_IF_ERROR(codegen_nameop(c, loc, injected_arg, Load));
        nelts++;
    }
    if (nkwelts) {
        VISIT_SEQ(c, keyword, keywords);
        RETURN_IF_ERROR(
            codegen_call_simple_kw_helper(c, loc, keywords, nkwelts));
        ADDOP_I(c, loc, CALL_KW, n + nelts + nkwelts);
    }
    else {
        ADDOP_I(c, loc, CALL, n + nelts);
    }
    return SUCCESS;

ex_call:

    /* Do positional arguments. */
    if (n == 0 && nelts == 1 && ((expr_ty)asdl_seq_GET(args, 0))->kind == Starred_kind) {
        VISIT(c, expr, ((expr_ty)asdl_seq_GET(args, 0))->v.Starred.value);
    }
    else {
        RETURN_IF_ERROR(starunpack_helper_impl(c, loc, args, injected_arg, n,
                                               BUILD_LIST, LIST_APPEND, LIST_EXTEND, 1));
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
                    RETURN_IF_ERROR(codegen_subkwargs(c, loc, keywords, i - nseen, i));
                    if (have_dict) {
                        ADDOP_I(c, loc, DICT_MERGE, 1);
                    }
                    have_dict = 1;
                    nseen = 0;
                }
                if (!have_dict) {
                    ADDOP_I(c, loc, BUILD_MAP, 0);
                    have_dict = 1;
                }
                VISIT(c, expr, kw->value);
                ADDOP_I(c, loc, DICT_MERGE, 1);
            }
            else {
                nseen++;
            }
        }
        if (nseen) {
            /* Pack up any trailing keyword arguments. */
            RETURN_IF_ERROR(codegen_subkwargs(c, loc, keywords, nkwelts - nseen, nkwelts));
            if (have_dict) {
                ADDOP_I(c, loc, DICT_MERGE, 1);
            }
            have_dict = 1;
        }
        assert(have_dict);
    }
    if (nkwelts == 0) {
        ADDOP(c, loc, PUSH_NULL);
    }
    ADDOP(c, loc, CALL_FUNCTION_EX);
    return SUCCESS;
}

static int
codegen_call_helper(compiler *c, location loc,
                    int n, /* Args already pushed */
                    asdl_expr_seq *args,
                    asdl_keyword_seq *keywords)
{
    return codegen_call_helper_impl(c, loc, n, args, NULL, keywords);
}

/* List and set comprehensions work by being inlined at the location where
  they are defined. The isolation of iteration variables is provided by
  pushing/popping clashing locals on the stack. Generator expressions work
  by creating a nested function to perform the actual iteration.
  This means that the iteration variables don't leak into the current scope.
  See https://peps.python.org/pep-0709/ for additional information.
  The defined function is called immediately following its definition, with the
  result of that call being the result of the expression.
  The LC/SC version returns the populated container, while the GE version is
  flagged in symtable.c as a generator, so it returns the generator object
  when the function is called.

  Possible cleanups:
    - iterate over the generator sequence instead of using recursion
*/


static int
codegen_comprehension_generator(compiler *c, location loc,
                                asdl_comprehension_seq *generators, int gen_index,
                                int depth,
                                expr_ty elt, expr_ty val, int type,
                                int iter_on_stack)
{
    comprehension_ty gen;
    gen = (comprehension_ty)asdl_seq_GET(generators, gen_index);
    if (gen->is_async) {
        return codegen_async_comprehension_generator(
            c, loc, generators, gen_index, depth, elt, val, type,
            iter_on_stack);
    } else {
        return codegen_sync_comprehension_generator(
            c, loc, generators, gen_index, depth, elt, val, type,
            iter_on_stack);
    }
}

static int
codegen_sync_comprehension_generator(compiler *c, location loc,
                                     asdl_comprehension_seq *generators,
                                     int gen_index, int depth,
                                     expr_ty elt, expr_ty val, int type,
                                     int iter_on_stack)
{
    /* generate code for the iterator, then each of the ifs,
       and then write to the element */

    NEW_JUMP_TARGET_LABEL(c, start);
    NEW_JUMP_TARGET_LABEL(c, if_cleanup);
    NEW_JUMP_TARGET_LABEL(c, anchor);

    comprehension_ty gen = (comprehension_ty)asdl_seq_GET(generators,
                                                          gen_index);

    if (!iter_on_stack) {
        if (gen_index == 0) {
            assert(METADATA(c)->u_argcount == 1);
            ADDOP_I(c, loc, LOAD_FAST, 0);
        }
        else {
            /* Sub-iter - calculate on the fly */
            /* Fast path for the temporary variable assignment idiom:
                for y in [f(x)]
            */
            asdl_expr_seq *elts;
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
                    start = NO_LABEL;
                }
            }
            if (IS_JUMP_TARGET_LABEL(start)) {
                VISIT(c, expr, gen->iter);
            }
        }
    }

    if (IS_JUMP_TARGET_LABEL(start)) {
        depth += 2;
        ADDOP(c, LOC(gen->iter), GET_ITER);
        USE_LABEL(c, start);
        ADDOP_JUMP(c, LOC(gen->iter), FOR_ITER, anchor);
    }
    VISIT(c, expr, gen->target);

    /* XXX this needs to be cleaned up...a lot! */
    Py_ssize_t n = asdl_seq_LEN(gen->ifs);
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(gen->ifs, i);
        RETURN_IF_ERROR(codegen_jump_if(c, loc, e, if_cleanup, 0));
    }

    if (++gen_index < asdl_seq_LEN(generators)) {
        RETURN_IF_ERROR(
            codegen_comprehension_generator(c, loc,
                                            generators, gen_index, depth,
                                            elt, val, type, 0));
    }

    location elt_loc = LOC(elt);

    /* only append after the last for generator */
    if (gen_index >= asdl_seq_LEN(generators)) {
        /* comprehension specific code */
        switch (type) {
        case COMP_GENEXP:
            VISIT(c, expr, elt);
            ADDOP_YIELD(c, elt_loc);
            ADDOP(c, elt_loc, POP_TOP);
            break;
        case COMP_LISTCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, elt_loc, LIST_APPEND, depth + 1);
            break;
        case COMP_SETCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, elt_loc, SET_ADD, depth + 1);
            break;
        case COMP_DICTCOMP:
            /* With '{k: v}', k is evaluated before v, so we do
               the same. */
            VISIT(c, expr, elt);
            VISIT(c, expr, val);
            elt_loc = LOCATION(elt->lineno,
                               val->end_lineno,
                               elt->col_offset,
                               val->end_col_offset);
            ADDOP_I(c, elt_loc, MAP_ADD, depth + 1);
            break;
        default:
            return ERROR;
        }
    }

    USE_LABEL(c, if_cleanup);
    if (IS_JUMP_TARGET_LABEL(start)) {
        ADDOP_JUMP(c, elt_loc, JUMP, start);

        USE_LABEL(c, anchor);
        /* It is important for instrumentation that the `END_FOR` comes first.
        * Iteration over a generator will jump to the first of these instructions,
        * but a non-generator will jump to a later instruction.
        */
        ADDOP(c, NO_LOCATION, END_FOR);
        ADDOP(c, NO_LOCATION, POP_ITER);
    }

    return SUCCESS;
}

static int
codegen_async_comprehension_generator(compiler *c, location loc,
                                      asdl_comprehension_seq *generators,
                                      int gen_index, int depth,
                                      expr_ty elt, expr_ty val, int type,
                                      int iter_on_stack)
{
    NEW_JUMP_TARGET_LABEL(c, start);
    NEW_JUMP_TARGET_LABEL(c, send);
    NEW_JUMP_TARGET_LABEL(c, except);
    NEW_JUMP_TARGET_LABEL(c, if_cleanup);

    comprehension_ty gen = (comprehension_ty)asdl_seq_GET(generators,
                                                          gen_index);

    if (!iter_on_stack) {
        if (gen_index == 0) {
            assert(METADATA(c)->u_argcount == 1);
            ADDOP_I(c, loc, LOAD_FAST, 0);
        }
        else {
            /* Sub-iter - calculate on the fly */
            VISIT(c, expr, gen->iter);
        }
    }
    ADDOP(c, LOC(gen->iter), GET_AITER);

    USE_LABEL(c, start);
    /* Runtime will push a block here, so we need to account for that */
    RETURN_IF_ERROR(
        _PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_ASYNC_COMPREHENSION_GENERATOR,
                              start, NO_LABEL, NULL));

    ADDOP_JUMP(c, loc, SETUP_FINALLY, except);
    ADDOP(c, loc, GET_ANEXT);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    USE_LABEL(c, send);
    ADD_YIELD_FROM(c, loc, 1);
    ADDOP(c, loc, POP_BLOCK);
    VISIT(c, expr, gen->target);

    Py_ssize_t n = asdl_seq_LEN(gen->ifs);
    for (Py_ssize_t i = 0; i < n; i++) {
        expr_ty e = (expr_ty)asdl_seq_GET(gen->ifs, i);
        RETURN_IF_ERROR(codegen_jump_if(c, loc, e, if_cleanup, 0));
    }

    depth++;
    if (++gen_index < asdl_seq_LEN(generators)) {
        RETURN_IF_ERROR(
            codegen_comprehension_generator(c, loc,
                                            generators, gen_index, depth,
                                            elt, val, type, 0));
    }

    location elt_loc = LOC(elt);
    /* only append after the last for generator */
    if (gen_index >= asdl_seq_LEN(generators)) {
        /* comprehension specific code */
        switch (type) {
        case COMP_GENEXP:
            VISIT(c, expr, elt);
            ADDOP_YIELD(c, elt_loc);
            ADDOP(c, elt_loc, POP_TOP);
            break;
        case COMP_LISTCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, elt_loc, LIST_APPEND, depth + 1);
            break;
        case COMP_SETCOMP:
            VISIT(c, expr, elt);
            ADDOP_I(c, elt_loc, SET_ADD, depth + 1);
            break;
        case COMP_DICTCOMP:
            /* With '{k: v}', k is evaluated before v, so we do
               the same. */
            VISIT(c, expr, elt);
            VISIT(c, expr, val);
            elt_loc = LOCATION(elt->lineno,
                               val->end_lineno,
                               elt->col_offset,
                               val->end_col_offset);
            ADDOP_I(c, elt_loc, MAP_ADD, depth + 1);
            break;
        default:
            return ERROR;
        }
    }

    USE_LABEL(c, if_cleanup);
    ADDOP_JUMP(c, elt_loc, JUMP, start);

    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_ASYNC_COMPREHENSION_GENERATOR, start);

    USE_LABEL(c, except);

    ADDOP_JUMP(c, loc, END_ASYNC_FOR, send);

    return SUCCESS;
}

static int
codegen_push_inlined_comprehension_locals(compiler *c, location loc,
                                          PySTEntryObject *comp,
                                          _PyCompile_InlinedComprehensionState *state)
{
    int in_class_block = (SYMTABLE_ENTRY(c)->ste_type == ClassBlock) &&
                          !_PyCompile_IsInInlinedComp(c);
    PySTEntryObject *outer = SYMTABLE_ENTRY(c);
    // iterate over names bound in the comprehension and ensure we isolate
    // them from the outer scope as needed
    PyObject *k, *v;
    Py_ssize_t pos = 0;
    while (PyDict_Next(comp->ste_symbols, &pos, &k, &v)) {
        long symbol = PyLong_AsLong(v);
        assert(symbol >= 0 || PyErr_Occurred());
        RETURN_IF_ERROR(symbol);
        long scope = SYMBOL_TO_SCOPE(symbol);

        long outsymbol = _PyST_GetSymbol(outer, k);
        RETURN_IF_ERROR(outsymbol);
        long outsc = SYMBOL_TO_SCOPE(outsymbol);

        if ((symbol & DEF_LOCAL && !(symbol & DEF_NONLOCAL)) || in_class_block) {
            // local names bound in comprehension must be isolated from
            // outer scope; push existing value (which may be NULL if
            // not defined) on stack
            if (state->pushed_locals == NULL) {
                state->pushed_locals = PyList_New(0);
                if (state->pushed_locals == NULL) {
                    return ERROR;
                }
            }
            // in the case of a cell, this will actually push the cell
            // itself to the stack, then we'll create a new one for the
            // comprehension and restore the original one after
            ADDOP_NAME(c, loc, LOAD_FAST_AND_CLEAR, k, varnames);
            if (scope == CELL) {
                if (outsc == FREE) {
                    ADDOP_NAME(c, loc, MAKE_CELL, k, freevars);
                } else {
                    ADDOP_NAME(c, loc, MAKE_CELL, k, cellvars);
                }
            }
            if (PyList_Append(state->pushed_locals, k) < 0) {
                return ERROR;
            }
        }
    }
    if (state->pushed_locals) {
        // Outermost iterable expression was already evaluated and is on the
        // stack, we need to swap it back to TOS. This also rotates the order of
        // `pushed_locals` on the stack, but this will be reversed when we swap
        // out the comprehension result in pop_inlined_comprehension_state
        ADDOP_I(c, loc, SWAP, PyList_GET_SIZE(state->pushed_locals) + 1);

        // Add our own cleanup handler to restore comprehension locals in case
        // of exception, so they have the correct values inside an exception
        // handler or finally block.
        NEW_JUMP_TARGET_LABEL(c, cleanup);
        state->cleanup = cleanup;

        // no need to push an fblock for this "virtual" try/finally; there can't
        // be return/continue/break inside a comprehension
        ADDOP_JUMP(c, loc, SETUP_FINALLY, cleanup);
    }
    return SUCCESS;
}

static int
push_inlined_comprehension_state(compiler *c, location loc,
                                 PySTEntryObject *comp,
                                 _PyCompile_InlinedComprehensionState *state)
{
    RETURN_IF_ERROR(
        _PyCompile_TweakInlinedComprehensionScopes(c, loc, comp, state));
    RETURN_IF_ERROR(
        codegen_push_inlined_comprehension_locals(c, loc, comp, state));
    return SUCCESS;
}

static int
restore_inlined_comprehension_locals(compiler *c, location loc,
                                     _PyCompile_InlinedComprehensionState *state)
{
    PyObject *k;
    // pop names we pushed to stack earlier
    Py_ssize_t npops = PyList_GET_SIZE(state->pushed_locals);
    // Preserve the comprehension result (or exception) as TOS. This
    // reverses the SWAP we did in push_inlined_comprehension_state
    // to get the outermost iterable to TOS, so we can still just iterate
    // pushed_locals in simple reverse order
    ADDOP_I(c, loc, SWAP, npops + 1);
    for (Py_ssize_t i = npops - 1; i >= 0; --i) {
        k = PyList_GetItem(state->pushed_locals, i);
        if (k == NULL) {
            return ERROR;
        }
        ADDOP_NAME(c, loc, STORE_FAST_MAYBE_NULL, k, varnames);
    }
    return SUCCESS;
}

static int
codegen_pop_inlined_comprehension_locals(compiler *c, location loc,
                                         _PyCompile_InlinedComprehensionState *state)
{
    if (state->pushed_locals) {
        ADDOP(c, NO_LOCATION, POP_BLOCK);

        NEW_JUMP_TARGET_LABEL(c, end);
        ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, end);

        // cleanup from an exception inside the comprehension
        USE_LABEL(c, state->cleanup);
        // discard incomplete comprehension result (beneath exc on stack)
        ADDOP_I(c, NO_LOCATION, SWAP, 2);
        ADDOP(c, NO_LOCATION, POP_TOP);
        RETURN_IF_ERROR(restore_inlined_comprehension_locals(c, loc, state));
        ADDOP_I(c, NO_LOCATION, RERAISE, 0);

        USE_LABEL(c, end);
        RETURN_IF_ERROR(restore_inlined_comprehension_locals(c, loc, state));
        Py_CLEAR(state->pushed_locals);
    }
    return SUCCESS;
}

static int
pop_inlined_comprehension_state(compiler *c, location loc,
                                _PyCompile_InlinedComprehensionState *state)
{
    RETURN_IF_ERROR(codegen_pop_inlined_comprehension_locals(c, loc, state));
    RETURN_IF_ERROR(_PyCompile_RevertInlinedComprehensionScopes(c, loc, state));
    return SUCCESS;
}

static int
codegen_comprehension(compiler *c, expr_ty e, int type,
                      identifier name, asdl_comprehension_seq *generators, expr_ty elt,
                      expr_ty val)
{
    PyCodeObject *co = NULL;
    _PyCompile_InlinedComprehensionState inline_state = {NULL, NULL, NULL, NO_LABEL};
    comprehension_ty outermost;
    PySTEntryObject *entry = _PySymtable_Lookup(SYMTABLE(c), (void *)e);
    if (entry == NULL) {
        goto error;
    }
    int is_inlined = entry->ste_comp_inlined;
    int is_async_comprehension = entry->ste_coroutine;

    location loc = LOC(e);

    outermost = (comprehension_ty) asdl_seq_GET(generators, 0);
    if (is_inlined) {
        VISIT(c, expr, outermost->iter);
        if (push_inlined_comprehension_state(c, loc, entry, &inline_state)) {
            goto error;
        }
    }
    else {
        /* Receive outermost iter as an implicit argument */
        _PyCompile_CodeUnitMetadata umd = {
            .u_argcount = 1,
        };
        if (codegen_enter_scope(c, name, COMPILE_SCOPE_COMPREHENSION,
                                (void *)e, e->lineno, NULL, &umd) < 0) {
            goto error;
        }
    }
    Py_CLEAR(entry);

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

        ADDOP_I(c, loc, op, 0);
        if (is_inlined) {
            ADDOP_I(c, loc, SWAP, 2);
        }
    }

    if (codegen_comprehension_generator(c, loc, generators, 0, 0,
                                        elt, val, type, is_inlined) < 0) {
        goto error_in_scope;
    }

    if (is_inlined) {
        if (pop_inlined_comprehension_state(c, loc, &inline_state)) {
            goto error;
        }
        return SUCCESS;
    }

    if (type != COMP_GENEXP) {
        ADDOP(c, LOC(e), RETURN_VALUE);
    }
    if (type == COMP_GENEXP) {
        if (codegen_wrap_in_stopiteration_handler(c) < 0) {
            goto error_in_scope;
        }
    }

    co = _PyCompile_OptimizeAndAssemble(c, 1);
    _PyCompile_ExitScope(c);
    if (co == NULL) {
        goto error;
    }

    loc = LOC(e);
    if (codegen_make_closure(c, loc, co, 0) < 0) {
        goto error;
    }
    Py_CLEAR(co);

    VISIT(c, expr, outermost->iter);
    ADDOP_I(c, loc, CALL, 0);

    if (is_async_comprehension && type != COMP_GENEXP) {
        ADDOP_I(c, loc, GET_AWAITABLE, 0);
        ADDOP_LOAD_CONST(c, loc, Py_None);
        ADD_YIELD_FROM(c, loc, 1);
    }

    return SUCCESS;
error_in_scope:
    if (!is_inlined) {
        _PyCompile_ExitScope(c);
    }
error:
    Py_XDECREF(co);
    Py_XDECREF(entry);
    Py_XDECREF(inline_state.pushed_locals);
    Py_XDECREF(inline_state.temp_symbols);
    Py_XDECREF(inline_state.fast_hidden);
    return ERROR;
}

static int
codegen_genexp(compiler *c, expr_ty e)
{
    assert(e->kind == GeneratorExp_kind);
    _Py_DECLARE_STR(anon_genexpr, "<genexpr>");
    return codegen_comprehension(c, e, COMP_GENEXP, &_Py_STR(anon_genexpr),
                                 e->v.GeneratorExp.generators,
                                 e->v.GeneratorExp.elt, NULL);
}

static int
codegen_listcomp(compiler *c, expr_ty e)
{
    assert(e->kind == ListComp_kind);
    _Py_DECLARE_STR(anon_listcomp, "<listcomp>");
    return codegen_comprehension(c, e, COMP_LISTCOMP, &_Py_STR(anon_listcomp),
                                 e->v.ListComp.generators,
                                 e->v.ListComp.elt, NULL);
}

static int
codegen_setcomp(compiler *c, expr_ty e)
{
    assert(e->kind == SetComp_kind);
    _Py_DECLARE_STR(anon_setcomp, "<setcomp>");
    return codegen_comprehension(c, e, COMP_SETCOMP, &_Py_STR(anon_setcomp),
                                 e->v.SetComp.generators,
                                 e->v.SetComp.elt, NULL);
}


static int
codegen_dictcomp(compiler *c, expr_ty e)
{
    assert(e->kind == DictComp_kind);
    _Py_DECLARE_STR(anon_dictcomp, "<dictcomp>");
    return codegen_comprehension(c, e, COMP_DICTCOMP, &_Py_STR(anon_dictcomp),
                                 e->v.DictComp.generators,
                                 e->v.DictComp.key, e->v.DictComp.value);
}


static int
codegen_visit_keyword(compiler *c, keyword_ty k)
{
    VISIT(c, expr, k->value);
    return SUCCESS;
}


static int
codegen_with_except_finish(compiler *c, jump_target_label cleanup) {
    NEW_JUMP_TARGET_LABEL(c, suppress);
    ADDOP(c, NO_LOCATION, TO_BOOL);
    ADDOP_JUMP(c, NO_LOCATION, POP_JUMP_IF_TRUE, suppress);
    ADDOP_I(c, NO_LOCATION, RERAISE, 2);

    USE_LABEL(c, suppress);
    ADDOP(c, NO_LOCATION, POP_TOP); /* exc_value */
    ADDOP(c, NO_LOCATION, POP_BLOCK);
    ADDOP(c, NO_LOCATION, POP_EXCEPT);
    ADDOP(c, NO_LOCATION, POP_TOP);
    ADDOP(c, NO_LOCATION, POP_TOP);
    ADDOP(c, NO_LOCATION, POP_TOP);
    NEW_JUMP_TARGET_LABEL(c, exit);
    ADDOP_JUMP(c, NO_LOCATION, JUMP_NO_INTERRUPT, exit);

    USE_LABEL(c, cleanup);
    POP_EXCEPT_AND_RERAISE(c, NO_LOCATION);

    USE_LABEL(c, exit);
    return SUCCESS;
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
codegen_async_with_inner(compiler *c, stmt_ty s, int pos)
{
    location loc = LOC(s);
    withitem_ty item = asdl_seq_GET(s->v.AsyncWith.items, pos);

    assert(s->kind == AsyncWith_kind);

    NEW_JUMP_TARGET_LABEL(c, block);
    NEW_JUMP_TARGET_LABEL(c, final);
    NEW_JUMP_TARGET_LABEL(c, exit);
    NEW_JUMP_TARGET_LABEL(c, cleanup);

    /* Evaluate EXPR */
    VISIT(c, expr, item->context_expr);
    loc = LOC(item->context_expr);
    ADDOP_I(c, loc, COPY, 1);
    ADDOP_I(c, loc, LOAD_SPECIAL, SPECIAL___AEXIT__);
    ADDOP_I(c, loc, SWAP, 2);
    ADDOP_I(c, loc, SWAP, 3);
    ADDOP_I(c, loc, LOAD_SPECIAL, SPECIAL___AENTER__);
    ADDOP_I(c, loc, CALL, 0);
    ADDOP_I(c, loc, GET_AWAITABLE, 1);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADD_YIELD_FROM(c, loc, 1);

    ADDOP_JUMP(c, loc, SETUP_WITH, final);

    /* SETUP_WITH pushes a finally block. */
    USE_LABEL(c, block);
    RETURN_IF_ERROR(_PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_ASYNC_WITH, block, final, s));

    if (item->optional_vars) {
        VISIT(c, expr, item->optional_vars);
    }
    else {
        /* Discard result from context.__aenter__() */
        ADDOP(c, loc, POP_TOP);
    }

    pos++;
    if (pos == asdl_seq_LEN(s->v.AsyncWith.items)) {
        /* BLOCK code */
        VISIT_SEQ(c, stmt, s->v.AsyncWith.body);
    }
    else {
        RETURN_IF_ERROR(codegen_async_with_inner(c, s, pos));
    }

    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_ASYNC_WITH, block);

    ADDOP(c, loc, POP_BLOCK);
    /* End of body; start the cleanup */

    /* For successful outcome:
     * call __exit__(None, None, None)
     */
    RETURN_IF_ERROR(codegen_call_exit_with_nones(c, loc));
    ADDOP_I(c, loc, GET_AWAITABLE, 2);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADD_YIELD_FROM(c, loc, 1);

    ADDOP(c, loc, POP_TOP);

    ADDOP_JUMP(c, loc, JUMP, exit);

    /* For exceptional outcome: */
    USE_LABEL(c, final);

    ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup);
    ADDOP(c, loc, PUSH_EXC_INFO);
    ADDOP(c, loc, WITH_EXCEPT_START);
    ADDOP_I(c, loc, GET_AWAITABLE, 2);
    ADDOP_LOAD_CONST(c, loc, Py_None);
    ADD_YIELD_FROM(c, loc, 1);
    RETURN_IF_ERROR(codegen_with_except_finish(c, cleanup));

    USE_LABEL(c, exit);
    return SUCCESS;
}

static int
codegen_async_with(compiler *c, stmt_ty s)
{
    return codegen_async_with_inner(c, s, 0);
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
        JUMP  EXIT
    E:  WITH_EXCEPT_START (calls EXPR.__exit__)
        POP_JUMP_IF_TRUE T:
        RERAISE
    T:  POP_TOP (remove exception from stack)
        POP_EXCEPT
        POP_TOP
    EXIT:
 */

static int
codegen_with_inner(compiler *c, stmt_ty s, int pos)
{
    withitem_ty item = asdl_seq_GET(s->v.With.items, pos);

    assert(s->kind == With_kind);

    NEW_JUMP_TARGET_LABEL(c, block);
    NEW_JUMP_TARGET_LABEL(c, final);
    NEW_JUMP_TARGET_LABEL(c, exit);
    NEW_JUMP_TARGET_LABEL(c, cleanup);

    /* Evaluate EXPR */
    VISIT(c, expr, item->context_expr);
    /* Will push bound __exit__ */
    location loc = LOC(item->context_expr);
    ADDOP_I(c, loc, COPY, 1);
    ADDOP_I(c, loc, LOAD_SPECIAL, SPECIAL___EXIT__);
    ADDOP_I(c, loc, SWAP, 2);
    ADDOP_I(c, loc, SWAP, 3);
    ADDOP_I(c, loc, LOAD_SPECIAL, SPECIAL___ENTER__);
    ADDOP_I(c, loc, CALL, 0);
    ADDOP_JUMP(c, loc, SETUP_WITH, final);

    /* SETUP_WITH pushes a finally block. */
    USE_LABEL(c, block);
    RETURN_IF_ERROR(_PyCompile_PushFBlock(c, loc, COMPILE_FBLOCK_WITH, block, final, s));

    if (item->optional_vars) {
        VISIT(c, expr, item->optional_vars);
    }
    else {
    /* Discard result from context.__enter__() */
        ADDOP(c, loc, POP_TOP);
    }

    pos++;
    if (pos == asdl_seq_LEN(s->v.With.items)) {
        /* BLOCK code */
        VISIT_SEQ(c, stmt, s->v.With.body);
    }
    else {
        RETURN_IF_ERROR(codegen_with_inner(c, s, pos));
    }

    ADDOP(c, NO_LOCATION, POP_BLOCK);
    _PyCompile_PopFBlock(c, COMPILE_FBLOCK_WITH, block);

    /* End of body; start the cleanup. */

    /* For successful outcome:
     * call __exit__(None, None, None)
     */
    RETURN_IF_ERROR(codegen_call_exit_with_nones(c, loc));
    ADDOP(c, loc, POP_TOP);
    ADDOP_JUMP(c, loc, JUMP, exit);

    /* For exceptional outcome: */
    USE_LABEL(c, final);

    ADDOP_JUMP(c, loc, SETUP_CLEANUP, cleanup);
    ADDOP(c, loc, PUSH_EXC_INFO);
    ADDOP(c, loc, WITH_EXCEPT_START);
    RETURN_IF_ERROR(codegen_with_except_finish(c, cleanup));

    USE_LABEL(c, exit);
    return SUCCESS;
}

static int
codegen_with(compiler *c, stmt_ty s)
{
    return codegen_with_inner(c, s, 0);
}

static int
codegen_visit_expr(compiler *c, expr_ty e)
{
    if (Py_EnterRecursiveCall(" during compilation")) {
        return ERROR;
    }
    location loc = LOC(e);
    switch (e->kind) {
    case NamedExpr_kind:
        VISIT(c, expr, e->v.NamedExpr.value);
        ADDOP_I(c, loc, COPY, 1);
        VISIT(c, expr, e->v.NamedExpr.target);
        break;
    case BoolOp_kind:
        return codegen_boolop(c, e);
    case BinOp_kind:
        VISIT(c, expr, e->v.BinOp.left);
        VISIT(c, expr, e->v.BinOp.right);
        ADDOP_BINARY(c, loc, e->v.BinOp.op);
        break;
    case UnaryOp_kind:
        VISIT(c, expr, e->v.UnaryOp.operand);
        if (e->v.UnaryOp.op == UAdd) {
            ADDOP_I(c, loc, CALL_INTRINSIC_1, INTRINSIC_UNARY_POSITIVE);
        }
        else if (e->v.UnaryOp.op == Not) {
            ADDOP(c, loc, TO_BOOL);
            ADDOP(c, loc, UNARY_NOT);
        }
        else {
            ADDOP(c, loc, unaryop(e->v.UnaryOp.op));
        }
        break;
    case Lambda_kind:
        return codegen_lambda(c, e);
    case IfExp_kind:
        return codegen_ifexp(c, e);
    case Dict_kind:
        return codegen_dict(c, e);
    case Set_kind:
        return codegen_set(c, e);
    case GeneratorExp_kind:
        return codegen_genexp(c, e);
    case ListComp_kind:
        return codegen_listcomp(c, e);
    case SetComp_kind:
        return codegen_setcomp(c, e);
    case DictComp_kind:
        return codegen_dictcomp(c, e);
    case Yield_kind:
        if (!_PyST_IsFunctionLike(SYMTABLE_ENTRY(c))) {
            return _PyCompile_Error(c, loc, "'yield' outside function");
        }
        if (e->v.Yield.value) {
            VISIT(c, expr, e->v.Yield.value);
        }
        else {
            ADDOP_LOAD_CONST(c, loc, Py_None);
        }
        ADDOP_YIELD(c, loc);
        break;
    case YieldFrom_kind:
        if (!_PyST_IsFunctionLike(SYMTABLE_ENTRY(c))) {
            return _PyCompile_Error(c, loc, "'yield from' outside function");
        }
        if (SCOPE_TYPE(c) == COMPILE_SCOPE_ASYNC_FUNCTION) {
            return _PyCompile_Error(c, loc, "'yield from' inside async function");
        }
        VISIT(c, expr, e->v.YieldFrom.value);
        ADDOP(c, loc, GET_YIELD_FROM_ITER);
        ADDOP_LOAD_CONST(c, loc, Py_None);
        ADD_YIELD_FROM(c, loc, 0);
        break;
    case Await_kind:
        VISIT(c, expr, e->v.Await.value);
        ADDOP_I(c, loc, GET_AWAITABLE, 0);
        ADDOP_LOAD_CONST(c, loc, Py_None);
        ADD_YIELD_FROM(c, loc, 1);
        break;
    case Compare_kind:
        return codegen_compare(c, e);
    case Call_kind:
        return codegen_call(c, e);
    case Constant_kind:
        ADDOP_LOAD_CONST(c, loc, e->v.Constant.value);
        break;
    case JoinedStr_kind:
        return codegen_joined_str(c, e);
    case TemplateStr_kind:
        return codegen_template_str(c, e);
    case FormattedValue_kind:
        return codegen_formatted_value(c, e);
    case Interpolation_kind:
        return codegen_interpolation(c, e);
    /* The following exprs can be assignment targets. */
    case Attribute_kind:
        if (e->v.Attribute.ctx == Load) {
            int ret = can_optimize_super_call(c, e);
            RETURN_IF_ERROR(ret);
            if (ret) {
                RETURN_IF_ERROR(load_args_for_super(c, e->v.Attribute.value));
                int opcode = asdl_seq_LEN(e->v.Attribute.value->v.Call.args) ?
                    LOAD_SUPER_ATTR : LOAD_ZERO_SUPER_ATTR;
                ADDOP_NAME(c, loc, opcode, e->v.Attribute.attr, names);
                loc = update_start_location_to_match_attr(c, loc, e);
                ADDOP(c, loc, NOP);
                return SUCCESS;
            }
        }
        RETURN_IF_ERROR(_PyCompile_MaybeAddStaticAttributeToClass(c, e));
        VISIT(c, expr, e->v.Attribute.value);
        loc = LOC(e);
        loc = update_start_location_to_match_attr(c, loc, e);
        switch (e->v.Attribute.ctx) {
        case Load:
            ADDOP_NAME(c, loc, LOAD_ATTR, e->v.Attribute.attr, names);
            break;
        case Store:
            ADDOP_NAME(c, loc, STORE_ATTR, e->v.Attribute.attr, names);
            break;
        case Del:
            ADDOP_NAME(c, loc, DELETE_ATTR, e->v.Attribute.attr, names);
            break;
        }
        break;
    case Subscript_kind:
        return codegen_subscript(c, e);
    case Starred_kind:
        switch (e->v.Starred.ctx) {
        case Store:
            /* In all legitimate cases, the Starred node was already replaced
             * by codegen_list/codegen_tuple. XXX: is that okay? */
            return _PyCompile_Error(c, loc,
                "starred assignment target must be in a list or tuple");
        default:
            return _PyCompile_Error(c, loc,
                "can't use starred expression here");
        }
        break;
    case Slice_kind:
        RETURN_IF_ERROR(codegen_slice(c, e));
        break;
    case Name_kind:
        return codegen_nameop(c, loc, e->v.Name.id, e->v.Name.ctx);
    /* child nodes of List and Tuple will have expr_context set */
    case List_kind:
        return codegen_list(c, e);
    case Tuple_kind:
        return codegen_tuple(c, e);
    }
    return SUCCESS;
}

static bool
is_constant_slice(expr_ty s)
{
    return s->kind == Slice_kind &&
        (s->v.Slice.lower == NULL ||
         s->v.Slice.lower->kind == Constant_kind) &&
        (s->v.Slice.upper == NULL ||
         s->v.Slice.upper->kind == Constant_kind) &&
        (s->v.Slice.step == NULL ||
         s->v.Slice.step->kind == Constant_kind);
}

static bool
should_apply_two_element_slice_optimization(expr_ty s)
{
    return !is_constant_slice(s) &&
           s->kind == Slice_kind &&
           s->v.Slice.step == NULL;
}

static int
codegen_augassign(compiler *c, stmt_ty s)
{
    assert(s->kind == AugAssign_kind);
    expr_ty e = s->v.AugAssign.target;

    location loc = LOC(e);

    switch (e->kind) {
    case Attribute_kind:
        VISIT(c, expr, e->v.Attribute.value);
        ADDOP_I(c, loc, COPY, 1);
        loc = update_start_location_to_match_attr(c, loc, e);
        ADDOP_NAME(c, loc, LOAD_ATTR, e->v.Attribute.attr, names);
        break;
    case Subscript_kind:
        VISIT(c, expr, e->v.Subscript.value);
        if (should_apply_two_element_slice_optimization(e->v.Subscript.slice)) {
            RETURN_IF_ERROR(codegen_slice_two_parts(c, e->v.Subscript.slice));
            ADDOP_I(c, loc, COPY, 3);
            ADDOP_I(c, loc, COPY, 3);
            ADDOP_I(c, loc, COPY, 3);
            ADDOP(c, loc, BINARY_SLICE);
        }
        else {
            VISIT(c, expr, e->v.Subscript.slice);
            ADDOP_I(c, loc, COPY, 2);
            ADDOP_I(c, loc, COPY, 2);
            ADDOP_I(c, loc, BINARY_OP, NB_SUBSCR);
        }
        break;
    case Name_kind:
        RETURN_IF_ERROR(codegen_nameop(c, loc, e->v.Name.id, Load));
        break;
    default:
        PyErr_Format(PyExc_SystemError,
            "invalid node type (%d) for augmented assignment",
            e->kind);
        return ERROR;
    }

    loc = LOC(s);

    VISIT(c, expr, s->v.AugAssign.value);
    ADDOP_INPLACE(c, loc, s->v.AugAssign.op);

    loc = LOC(e);

    switch (e->kind) {
    case Attribute_kind:
        loc = update_start_location_to_match_attr(c, loc, e);
        ADDOP_I(c, loc, SWAP, 2);
        ADDOP_NAME(c, loc, STORE_ATTR, e->v.Attribute.attr, names);
        break;
    case Subscript_kind:
        if (should_apply_two_element_slice_optimization(e->v.Subscript.slice)) {
            ADDOP_I(c, loc, SWAP, 4);
            ADDOP_I(c, loc, SWAP, 3);
            ADDOP_I(c, loc, SWAP, 2);
            ADDOP(c, loc, STORE_SLICE);
        }
        else {
            ADDOP_I(c, loc, SWAP, 3);
            ADDOP_I(c, loc, SWAP, 2);
            ADDOP(c, loc, STORE_SUBSCR);
        }
        break;
    case Name_kind:
        return codegen_nameop(c, loc, e->v.Name.id, Store);
    default:
        Py_UNREACHABLE();
    }
    return SUCCESS;
}

static int
codegen_check_ann_expr(compiler *c, expr_ty e)
{
    VISIT(c, expr, e);
    ADDOP(c, LOC(e), POP_TOP);
    return SUCCESS;
}

static int
codegen_check_ann_subscr(compiler *c, expr_ty e)
{
    /* We check that everything in a subscript is defined at runtime. */
    switch (e->kind) {
    case Slice_kind:
        if (e->v.Slice.lower && codegen_check_ann_expr(c, e->v.Slice.lower) < 0) {
            return ERROR;
        }
        if (e->v.Slice.upper && codegen_check_ann_expr(c, e->v.Slice.upper) < 0) {
            return ERROR;
        }
        if (e->v.Slice.step && codegen_check_ann_expr(c, e->v.Slice.step) < 0) {
            return ERROR;
        }
        return SUCCESS;
    case Tuple_kind: {
        /* extended slice */
        asdl_expr_seq *elts = e->v.Tuple.elts;
        Py_ssize_t i, n = asdl_seq_LEN(elts);
        for (i = 0; i < n; i++) {
            RETURN_IF_ERROR(codegen_check_ann_subscr(c, asdl_seq_GET(elts, i)));
        }
        return SUCCESS;
    }
    default:
        return codegen_check_ann_expr(c, e);
    }
}

static int
codegen_annassign(compiler *c, stmt_ty s)
{
    location loc = LOC(s);
    expr_ty targ = s->v.AnnAssign.target;
    bool future_annotations = FUTURE_FEATURES(c) & CO_FUTURE_ANNOTATIONS;
    PyObject *mangled;

    assert(s->kind == AnnAssign_kind);

    /* We perform the actual assignment first. */
    if (s->v.AnnAssign.value) {
        VISIT(c, expr, s->v.AnnAssign.value);
        VISIT(c, expr, targ);
    }
    switch (targ->kind) {
    case Name_kind:
        /* If we have a simple name in a module or class, store annotation. */
        if (s->v.AnnAssign.simple &&
            (SCOPE_TYPE(c) == COMPILE_SCOPE_MODULE ||
             SCOPE_TYPE(c) == COMPILE_SCOPE_CLASS)) {
            if (future_annotations) {
                VISIT(c, annexpr, s->v.AnnAssign.annotation);
                ADDOP_NAME(c, loc, LOAD_NAME, &_Py_ID(__annotations__), names);
                mangled = _PyCompile_MaybeMangle(c, targ->v.Name.id);
                ADDOP_LOAD_CONST_NEW(c, loc, mangled);
                ADDOP(c, loc, STORE_SUBSCR);
            }
            else {
                PyObject *conditional_annotation_index = NULL;
                RETURN_IF_ERROR(_PyCompile_AddDeferredAnnotation(
                    c, s, &conditional_annotation_index));
                if (conditional_annotation_index != NULL) {
                    if (SCOPE_TYPE(c) == COMPILE_SCOPE_CLASS) {
                        ADDOP_NAME(c, loc, LOAD_DEREF, &_Py_ID(__conditional_annotations__), cellvars);
                    }
                    else {
                        ADDOP_NAME(c, loc, LOAD_NAME, &_Py_ID(__conditional_annotations__), names);
                    }
                    ADDOP_LOAD_CONST_NEW(c, loc, conditional_annotation_index);
                    ADDOP_I(c, loc, SET_ADD, 1);
                    ADDOP(c, loc, POP_TOP);
                }
            }
        }
        break;
    case Attribute_kind:
        if (!s->v.AnnAssign.value &&
            codegen_check_ann_expr(c, targ->v.Attribute.value) < 0) {
            return ERROR;
        }
        break;
    case Subscript_kind:
        if (!s->v.AnnAssign.value &&
            (codegen_check_ann_expr(c, targ->v.Subscript.value) < 0 ||
             codegen_check_ann_subscr(c, targ->v.Subscript.slice) < 0)) {
                return ERROR;
        }
        break;
    default:
        PyErr_Format(PyExc_SystemError,
                     "invalid node type (%d) for annotated assignment",
                     targ->kind);
        return ERROR;
    }
    return SUCCESS;
}

static int
codegen_subscript(compiler *c, expr_ty e)
{
    location loc = LOC(e);
    expr_context_ty ctx = e->v.Subscript.ctx;

    if (ctx == Load) {
        RETURN_IF_ERROR(check_subscripter(c, e->v.Subscript.value));
        RETURN_IF_ERROR(check_index(c, e->v.Subscript.value, e->v.Subscript.slice));
    }

    VISIT(c, expr, e->v.Subscript.value);
    if (should_apply_two_element_slice_optimization(e->v.Subscript.slice) &&
        ctx != Del
    ) {
        RETURN_IF_ERROR(codegen_slice_two_parts(c, e->v.Subscript.slice));
        if (ctx == Load) {
            ADDOP(c, loc, BINARY_SLICE);
        }
        else {
            assert(ctx == Store);
            ADDOP(c, loc, STORE_SLICE);
        }
    }
    else {
        VISIT(c, expr, e->v.Subscript.slice);
        switch (ctx) {
            case Load:
                ADDOP_I(c, loc, BINARY_OP, NB_SUBSCR);
                break;
            case Store:
                ADDOP(c, loc, STORE_SUBSCR);
                break;
            case Del:
                ADDOP(c, loc, DELETE_SUBSCR);
                break;
        }
    }
    return SUCCESS;
}

static int
codegen_slice_two_parts(compiler *c, expr_ty s)
{
    if (s->v.Slice.lower) {
        VISIT(c, expr, s->v.Slice.lower);
    }
    else {
        ADDOP_LOAD_CONST(c, LOC(s), Py_None);
    }

    if (s->v.Slice.upper) {
        VISIT(c, expr, s->v.Slice.upper);
    }
    else {
        ADDOP_LOAD_CONST(c, LOC(s), Py_None);
    }

    return 0;
}

static int
codegen_slice(compiler *c, expr_ty s)
{
    int n = 2;
    assert(s->kind == Slice_kind);

    if (is_constant_slice(s)) {
        PyObject *start = NULL;
        if (s->v.Slice.lower) {
            start = s->v.Slice.lower->v.Constant.value;
        }
        PyObject *stop = NULL;
        if (s->v.Slice.upper) {
            stop = s->v.Slice.upper->v.Constant.value;
        }
        PyObject *step = NULL;
        if (s->v.Slice.step) {
            step = s->v.Slice.step->v.Constant.value;
        }
        PyObject *slice = PySlice_New(start, stop, step);
        if (slice == NULL) {
            return ERROR;
        }
        ADDOP_LOAD_CONST_NEW(c, LOC(s), slice);
        return SUCCESS;
    }

    RETURN_IF_ERROR(codegen_slice_two_parts(c, s));

    if (s->v.Slice.step) {
        n++;
        VISIT(c, expr, s->v.Slice.step);
    }

    ADDOP_I(c, LOC(s), BUILD_SLICE, n);
    return SUCCESS;
}


// PEP 634: Structural Pattern Matching

// To keep things simple, all codegen_pattern_* routines follow the convention
// of consuming TOS (the subject for the given pattern) and calling
// jump_to_fail_pop on failure (no match).

// When calling into these routines, it's important that pc->on_top be kept
// updated to reflect the current number of items that we are using on the top
// of the stack: they will be popped on failure, and any name captures will be
// stored *underneath* them on success. This lets us defer all names stores
// until the *entire* pattern matches.

#define WILDCARD_CHECK(N) \
    ((N)->kind == MatchAs_kind && !(N)->v.MatchAs.name)

#define WILDCARD_STAR_CHECK(N) \
    ((N)->kind == MatchStar_kind && !(N)->v.MatchStar.name)

// Limit permitted subexpressions, even if the parser & AST validator let them through
#define MATCH_VALUE_EXPR(N) \
    ((N)->kind == Constant_kind || (N)->kind == Attribute_kind)

// Allocate or resize pc->fail_pop to allow for n items to be popped on failure.
static int
ensure_fail_pop(compiler *c, pattern_context *pc, Py_ssize_t n)
{
    Py_ssize_t size = n + 1;
    if (size <= pc->fail_pop_size) {
        return SUCCESS;
    }
    Py_ssize_t needed = sizeof(jump_target_label) * size;
    jump_target_label *resized = PyMem_Realloc(pc->fail_pop, needed);
    if (resized == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    pc->fail_pop = resized;
    while (pc->fail_pop_size < size) {
        NEW_JUMP_TARGET_LABEL(c, new_block);
        pc->fail_pop[pc->fail_pop_size++] = new_block;
    }
    return SUCCESS;
}

// Use op to jump to the correct fail_pop block.
static int
jump_to_fail_pop(compiler *c, location loc,
                 pattern_context *pc, int op)
{
    // Pop any items on the top of the stack, plus any objects we were going to
    // capture on success:
    Py_ssize_t pops = pc->on_top + PyList_GET_SIZE(pc->stores);
    RETURN_IF_ERROR(ensure_fail_pop(c, pc, pops));
    ADDOP_JUMP(c, loc, op, pc->fail_pop[pops]);
    return SUCCESS;
}

// Build all of the fail_pop blocks and reset fail_pop.
static int
emit_and_reset_fail_pop(compiler *c, location loc,
                        pattern_context *pc)
{
    if (!pc->fail_pop_size) {
        assert(pc->fail_pop == NULL);
        return SUCCESS;
    }
    while (--pc->fail_pop_size) {
        USE_LABEL(c, pc->fail_pop[pc->fail_pop_size]);
        if (codegen_addop_noarg(INSTR_SEQUENCE(c), POP_TOP, loc) < 0) {
            pc->fail_pop_size = 0;
            PyMem_Free(pc->fail_pop);
            pc->fail_pop = NULL;
            return ERROR;
        }
    }
    USE_LABEL(c, pc->fail_pop[0]);
    PyMem_Free(pc->fail_pop);
    pc->fail_pop = NULL;
    return SUCCESS;
}

static int
codegen_error_duplicate_store(compiler *c, location loc, identifier n)
{
    return _PyCompile_Error(c, loc,
        "multiple assignments to name %R in pattern", n);
}

// Duplicate the effect of 3.10's ROT_* instructions using SWAPs.
static int
codegen_pattern_helper_rotate(compiler *c, location loc, Py_ssize_t count)
{
    while (1 < count) {
        ADDOP_I(c, loc, SWAP, count--);
    }
    return SUCCESS;
}

static int
codegen_pattern_helper_store_name(compiler *c, location loc,
                                  identifier n, pattern_context *pc)
{
    if (n == NULL) {
        ADDOP(c, loc, POP_TOP);
        return SUCCESS;
    }
    // Can't assign to the same name twice:
    int duplicate = PySequence_Contains(pc->stores, n);
    RETURN_IF_ERROR(duplicate);
    if (duplicate) {
        return codegen_error_duplicate_store(c, loc, n);
    }
    // Rotate this object underneath any items we need to preserve:
    Py_ssize_t rotations = pc->on_top + PyList_GET_SIZE(pc->stores) + 1;
    RETURN_IF_ERROR(codegen_pattern_helper_rotate(c, loc, rotations));
    RETURN_IF_ERROR(PyList_Append(pc->stores, n));
    return SUCCESS;
}


static int
codegen_pattern_unpack_helper(compiler *c, location loc,
                              asdl_pattern_seq *elts)
{
    Py_ssize_t n = asdl_seq_LEN(elts);
    int seen_star = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        pattern_ty elt = asdl_seq_GET(elts, i);
        if (elt->kind == MatchStar_kind && !seen_star) {
            if ((i >= (1 << 8)) ||
                (n-i-1 >= (INT_MAX >> 8))) {
                return _PyCompile_Error(c, loc,
                    "too many expressions in "
                    "star-unpacking sequence pattern");
            }
            ADDOP_I(c, loc, UNPACK_EX, (i + ((n-i-1) << 8)));
            seen_star = 1;
        }
        else if (elt->kind == MatchStar_kind) {
            return _PyCompile_Error(c, loc,
                "multiple starred expressions in sequence pattern");
        }
    }
    if (!seen_star) {
        ADDOP_I(c, loc, UNPACK_SEQUENCE, n);
    }
    return SUCCESS;
}

static int
pattern_helper_sequence_unpack(compiler *c, location loc,
                               asdl_pattern_seq *patterns, Py_ssize_t star,
                               pattern_context *pc)
{
    RETURN_IF_ERROR(codegen_pattern_unpack_helper(c, loc, patterns));
    Py_ssize_t size = asdl_seq_LEN(patterns);
    // We've now got a bunch of new subjects on the stack. They need to remain
    // there after each subpattern match:
    pc->on_top += size;
    for (Py_ssize_t i = 0; i < size; i++) {
        // One less item to keep track of each time we loop through:
        pc->on_top--;
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        RETURN_IF_ERROR(codegen_pattern_subpattern(c, pattern, pc));
    }
    return SUCCESS;
}

// Like pattern_helper_sequence_unpack, but uses BINARY_OP/NB_SUBSCR instead of
// UNPACK_SEQUENCE / UNPACK_EX. This is more efficient for patterns with a
// starred wildcard like [first, *_] / [first, *_, last] / [*_, last] / etc.
static int
pattern_helper_sequence_subscr(compiler *c, location loc,
                               asdl_pattern_seq *patterns, Py_ssize_t star,
                               pattern_context *pc)
{
    // We need to keep the subject around for extracting elements:
    pc->on_top++;
    Py_ssize_t size = asdl_seq_LEN(patterns);
    for (Py_ssize_t i = 0; i < size; i++) {
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        if (WILDCARD_CHECK(pattern)) {
            continue;
        }
        if (i == star) {
            assert(WILDCARD_STAR_CHECK(pattern));
            continue;
        }
        ADDOP_I(c, loc, COPY, 1);
        if (i < star) {
            ADDOP_LOAD_CONST_NEW(c, loc, PyLong_FromSsize_t(i));
        }
        else {
            // The subject may not support negative indexing! Compute a
            // nonnegative index:
            ADDOP(c, loc, GET_LEN);
            ADDOP_LOAD_CONST_NEW(c, loc, PyLong_FromSsize_t(size - i));
            ADDOP_BINARY(c, loc, Sub);
        }
        ADDOP_I(c, loc, BINARY_OP, NB_SUBSCR);
        RETURN_IF_ERROR(codegen_pattern_subpattern(c, pattern, pc));
    }
    // Pop the subject, we're done with it:
    pc->on_top--;
    ADDOP(c, loc, POP_TOP);
    return SUCCESS;
}

// Like codegen_pattern, but turn off checks for irrefutability.
static int
codegen_pattern_subpattern(compiler *c,
                            pattern_ty p, pattern_context *pc)
{
    int allow_irrefutable = pc->allow_irrefutable;
    pc->allow_irrefutable = 1;
    RETURN_IF_ERROR(codegen_pattern(c, p, pc));
    pc->allow_irrefutable = allow_irrefutable;
    return SUCCESS;
}

static int
codegen_pattern_as(compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchAs_kind);
    if (p->v.MatchAs.pattern == NULL) {
        // An irrefutable match:
        if (!pc->allow_irrefutable) {
            if (p->v.MatchAs.name) {
                const char *e = "name capture %R makes remaining patterns unreachable";
                return _PyCompile_Error(c, LOC(p), e, p->v.MatchAs.name);
            }
            const char *e = "wildcard makes remaining patterns unreachable";
            return _PyCompile_Error(c, LOC(p), e);
        }
        return codegen_pattern_helper_store_name(c, LOC(p), p->v.MatchAs.name, pc);
    }
    // Need to make a copy for (possibly) storing later:
    pc->on_top++;
    ADDOP_I(c, LOC(p), COPY, 1);
    RETURN_IF_ERROR(codegen_pattern(c, p->v.MatchAs.pattern, pc));
    // Success! Store it:
    pc->on_top--;
    RETURN_IF_ERROR(codegen_pattern_helper_store_name(c, LOC(p), p->v.MatchAs.name, pc));
    return SUCCESS;
}

static int
codegen_pattern_star(compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchStar_kind);
    RETURN_IF_ERROR(
        codegen_pattern_helper_store_name(c, LOC(p), p->v.MatchStar.name, pc));
    return SUCCESS;
}

static int
validate_kwd_attrs(compiler *c, asdl_identifier_seq *attrs, asdl_pattern_seq* patterns)
{
    // Any errors will point to the pattern rather than the arg name as the
    // parser is only supplying identifiers rather than Name or keyword nodes
    Py_ssize_t nattrs = asdl_seq_LEN(attrs);
    for (Py_ssize_t i = 0; i < nattrs; i++) {
        identifier attr = ((identifier)asdl_seq_GET(attrs, i));
        for (Py_ssize_t j = i + 1; j < nattrs; j++) {
            identifier other = ((identifier)asdl_seq_GET(attrs, j));
            if (!PyUnicode_Compare(attr, other)) {
                location loc = LOC((pattern_ty) asdl_seq_GET(patterns, j));
                return _PyCompile_Error(c, loc, "attribute name repeated "
                                                "in class pattern: %U", attr);
            }
        }
    }
    return SUCCESS;
}

static int
codegen_pattern_class(compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchClass_kind);
    asdl_pattern_seq *patterns = p->v.MatchClass.patterns;
    asdl_identifier_seq *kwd_attrs = p->v.MatchClass.kwd_attrs;
    asdl_pattern_seq *kwd_patterns = p->v.MatchClass.kwd_patterns;
    Py_ssize_t nargs = asdl_seq_LEN(patterns);
    Py_ssize_t nattrs = asdl_seq_LEN(kwd_attrs);
    Py_ssize_t nkwd_patterns = asdl_seq_LEN(kwd_patterns);
    if (nattrs != nkwd_patterns) {
        // AST validator shouldn't let this happen, but if it does,
        // just fail, don't crash out of the interpreter
        const char * e = "kwd_attrs (%d) / kwd_patterns (%d) length mismatch in class pattern";
        return _PyCompile_Error(c, LOC(p), e, nattrs, nkwd_patterns);
    }
    if (INT_MAX < nargs || INT_MAX < nargs + nattrs - 1) {
        const char *e = "too many sub-patterns in class pattern %R";
        return _PyCompile_Error(c, LOC(p), e, p->v.MatchClass.cls);
    }
    if (nattrs) {
        RETURN_IF_ERROR(validate_kwd_attrs(c, kwd_attrs, kwd_patterns));
    }
    VISIT(c, expr, p->v.MatchClass.cls);
    PyObject *attr_names = PyTuple_New(nattrs);
    if (attr_names == NULL) {
        return ERROR;
    }
    Py_ssize_t i;
    for (i = 0; i < nattrs; i++) {
        PyObject *name = asdl_seq_GET(kwd_attrs, i);
        PyTuple_SET_ITEM(attr_names, i, Py_NewRef(name));
    }
    ADDOP_LOAD_CONST_NEW(c, LOC(p), attr_names);
    ADDOP_I(c, LOC(p), MATCH_CLASS, nargs);
    ADDOP_I(c, LOC(p), COPY, 1);
    ADDOP_LOAD_CONST(c, LOC(p), Py_None);
    ADDOP_I(c, LOC(p), IS_OP, 1);
    // TOS is now a tuple of (nargs + nattrs) attributes (or None):
    pc->on_top++;
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    ADDOP_I(c, LOC(p), UNPACK_SEQUENCE, nargs + nattrs);
    pc->on_top += nargs + nattrs - 1;
    for (i = 0; i < nargs + nattrs; i++) {
        pc->on_top--;
        pattern_ty pattern;
        if (i < nargs) {
            // Positional:
            pattern = asdl_seq_GET(patterns, i);
        }
        else {
            // Keyword:
            pattern = asdl_seq_GET(kwd_patterns, i - nargs);
        }
        if (WILDCARD_CHECK(pattern)) {
            ADDOP(c, LOC(p), POP_TOP);
            continue;
        }
        RETURN_IF_ERROR(codegen_pattern_subpattern(c, pattern, pc));
    }
    // Success! Pop the tuple of attributes:
    return SUCCESS;
}

static int
codegen_pattern_mapping_key(compiler *c, PyObject *seen, pattern_ty p, Py_ssize_t i)
{
    asdl_expr_seq *keys = p->v.MatchMapping.keys;
    asdl_pattern_seq *patterns = p->v.MatchMapping.patterns;
    expr_ty key = asdl_seq_GET(keys, i);
    if (key == NULL) {
        const char *e = "can't use NULL keys in MatchMapping "
                        "(set 'rest' parameter instead)";
        location loc = LOC((pattern_ty) asdl_seq_GET(patterns, i));
        return _PyCompile_Error(c, loc, e);
    }

    if (key->kind == Constant_kind) {
        int in_seen = PySet_Contains(seen, key->v.Constant.value);
        RETURN_IF_ERROR(in_seen);
        if (in_seen) {
            const char *e = "mapping pattern checks duplicate key (%R)";
            return _PyCompile_Error(c, LOC(p), e, key->v.Constant.value);
        }
        RETURN_IF_ERROR(PySet_Add(seen, key->v.Constant.value));
    }
    else if (key->kind != Attribute_kind) {
        const char *e = "mapping pattern keys may only match literals and attribute lookups";
        return _PyCompile_Error(c, LOC(p), e);
    }
    VISIT(c, expr, key);
    return SUCCESS;
}

static int
codegen_pattern_mapping(compiler *c, pattern_ty p,
                        pattern_context *pc)
{
    assert(p->kind == MatchMapping_kind);
    asdl_expr_seq *keys = p->v.MatchMapping.keys;
    asdl_pattern_seq *patterns = p->v.MatchMapping.patterns;
    Py_ssize_t size = asdl_seq_LEN(keys);
    Py_ssize_t npatterns = asdl_seq_LEN(patterns);
    if (size != npatterns) {
        // AST validator shouldn't let this happen, but if it does,
        // just fail, don't crash out of the interpreter
        const char * e = "keys (%d) / patterns (%d) length mismatch in mapping pattern";
        return _PyCompile_Error(c, LOC(p), e, size, npatterns);
    }
    // We have a double-star target if "rest" is set
    PyObject *star_target = p->v.MatchMapping.rest;
    // We need to keep the subject on top during the mapping and length checks:
    pc->on_top++;
    ADDOP(c, LOC(p), MATCH_MAPPING);
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    if (!size && !star_target) {
        // If the pattern is just "{}", we're done! Pop the subject:
        pc->on_top--;
        ADDOP(c, LOC(p), POP_TOP);
        return SUCCESS;
    }
    if (size) {
        // If the pattern has any keys in it, perform a length check:
        ADDOP(c, LOC(p), GET_LEN);
        ADDOP_LOAD_CONST_NEW(c, LOC(p), PyLong_FromSsize_t(size));
        ADDOP_COMPARE(c, LOC(p), GtE);
        RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    }
    if (INT_MAX < size - 1) {
        return _PyCompile_Error(c, LOC(p), "too many sub-patterns in mapping pattern");
    }
    // Collect all of the keys into a tuple for MATCH_KEYS and
    // **rest. They can either be dotted names or literals:

    // Maintaining a set of Constant_kind kind keys allows us to raise a
    // SyntaxError in the case of duplicates.
    PyObject *seen = PySet_New(NULL);
    if (seen == NULL) {
        return ERROR;
    }
    for (Py_ssize_t i = 0; i < size; i++) {
        if (codegen_pattern_mapping_key(c, seen, p, i) < 0) {
            Py_DECREF(seen);
            return ERROR;
        }
    }
    Py_DECREF(seen);

    // all keys have been checked; there are no duplicates

    ADDOP_I(c, LOC(p), BUILD_TUPLE, size);
    ADDOP(c, LOC(p), MATCH_KEYS);
    // There's now a tuple of keys and a tuple of values on top of the subject:
    pc->on_top += 2;
    ADDOP_I(c, LOC(p), COPY, 1);
    ADDOP_LOAD_CONST(c, LOC(p), Py_None);
    ADDOP_I(c, LOC(p), IS_OP, 1);
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    // So far so good. Use that tuple of values on the stack to match
    // sub-patterns against:
    ADDOP_I(c, LOC(p), UNPACK_SEQUENCE, size);
    pc->on_top += size - 1;
    for (Py_ssize_t i = 0; i < size; i++) {
        pc->on_top--;
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        RETURN_IF_ERROR(codegen_pattern_subpattern(c, pattern, pc));
    }
    // If we get this far, it's a match! Whatever happens next should consume
    // the tuple of keys and the subject:
    pc->on_top -= 2;
    if (star_target) {
        // If we have a starred name, bind a dict of remaining items to it (this may
        // seem a bit inefficient, but keys is rarely big enough to actually impact
        // runtime):
        // rest = dict(TOS1)
        // for key in TOS:
        //     del rest[key]
        ADDOP_I(c, LOC(p), BUILD_MAP, 0);           // [subject, keys, empty]
        ADDOP_I(c, LOC(p), SWAP, 3);                // [empty, keys, subject]
        ADDOP_I(c, LOC(p), DICT_UPDATE, 2);         // [copy, keys]
        ADDOP_I(c, LOC(p), UNPACK_SEQUENCE, size);  // [copy, keys...]
        while (size) {
            ADDOP_I(c, LOC(p), COPY, 1 + size--);   // [copy, keys..., copy]
            ADDOP_I(c, LOC(p), SWAP, 2);            // [copy, keys..., copy, key]
            ADDOP(c, LOC(p), DELETE_SUBSCR);        // [copy, keys...]
        }
        RETURN_IF_ERROR(codegen_pattern_helper_store_name(c, LOC(p), star_target, pc));
    }
    else {
        ADDOP(c, LOC(p), POP_TOP);  // Tuple of keys.
        ADDOP(c, LOC(p), POP_TOP);  // Subject.
    }
    return SUCCESS;
}

static int
codegen_pattern_or(compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchOr_kind);
    NEW_JUMP_TARGET_LABEL(c, end);
    Py_ssize_t size = asdl_seq_LEN(p->v.MatchOr.patterns);
    assert(size > 1);
    // We're going to be messing with pc. Keep the original info handy:
    pattern_context old_pc = *pc;
    Py_INCREF(pc->stores);
    // control is the list of names bound by the first alternative. It is used
    // for checking different name bindings in alternatives, and for correcting
    // the order in which extracted elements are placed on the stack.
    PyObject *control = NULL;
    // NOTE: We can't use returning macros anymore! goto error on error.
    for (Py_ssize_t i = 0; i < size; i++) {
        pattern_ty alt = asdl_seq_GET(p->v.MatchOr.patterns, i);
        PyObject *pc_stores = PyList_New(0);
        if (pc_stores == NULL) {
            goto error;
        }
        Py_SETREF(pc->stores, pc_stores);
        // An irrefutable sub-pattern must be last, if it is allowed at all:
        pc->allow_irrefutable = (i == size - 1) && old_pc.allow_irrefutable;
        pc->fail_pop = NULL;
        pc->fail_pop_size = 0;
        pc->on_top = 0;
        if (codegen_addop_i(INSTR_SEQUENCE(c), COPY, 1, LOC(alt)) < 0 ||
            codegen_pattern(c, alt, pc) < 0) {
            goto error;
        }
        // Success!
        Py_ssize_t nstores = PyList_GET_SIZE(pc->stores);
        if (!i) {
            // This is the first alternative, so save its stores as a "control"
            // for the others (they can't bind a different set of names, and
            // might need to be reordered):
            assert(control == NULL);
            control = Py_NewRef(pc->stores);
        }
        else if (nstores != PyList_GET_SIZE(control)) {
            goto diff;
        }
        else if (nstores) {
            // There were captures. Check to see if we differ from control:
            Py_ssize_t icontrol = nstores;
            while (icontrol--) {
                PyObject *name = PyList_GET_ITEM(control, icontrol);
                Py_ssize_t istores = PySequence_Index(pc->stores, name);
                if (istores < 0) {
                    PyErr_Clear();
                    goto diff;
                }
                if (icontrol != istores) {
                    // Reorder the names on the stack to match the order of the
                    // names in control. There's probably a better way of doing
                    // this; the current solution is potentially very
                    // inefficient when each alternative subpattern binds lots
                    // of names in different orders. It's fine for reasonable
                    // cases, though, and the peephole optimizer will ensure
                    // that the final code is as efficient as possible.
                    assert(istores < icontrol);
                    Py_ssize_t rotations = istores + 1;
                    // Perform the same rotation on pc->stores:
                    PyObject *rotated = PyList_GetSlice(pc->stores, 0,
                                                        rotations);
                    if (rotated == NULL ||
                        PyList_SetSlice(pc->stores, 0, rotations, NULL) ||
                        PyList_SetSlice(pc->stores, icontrol - istores,
                                        icontrol - istores, rotated))
                    {
                        Py_XDECREF(rotated);
                        goto error;
                    }
                    Py_DECREF(rotated);
                    // That just did:
                    // rotated = pc_stores[:rotations]
                    // del pc_stores[:rotations]
                    // pc_stores[icontrol-istores:icontrol-istores] = rotated
                    // Do the same thing to the stack, using several
                    // rotations:
                    while (rotations--) {
                        if (codegen_pattern_helper_rotate(c, LOC(alt), icontrol + 1) < 0) {
                            goto error;
                        }
                    }
                }
            }
        }
        assert(control);
        if (codegen_addop_j(INSTR_SEQUENCE(c), LOC(alt), JUMP, end) < 0 ||
            emit_and_reset_fail_pop(c, LOC(alt), pc) < 0)
        {
            goto error;
        }
    }
    Py_DECREF(pc->stores);
    *pc = old_pc;
    Py_INCREF(pc->stores);
    // Need to NULL this for the PyMem_Free call in the error block.
    old_pc.fail_pop = NULL;
    // No match. Pop the remaining copy of the subject and fail:
    if (codegen_addop_noarg(INSTR_SEQUENCE(c), POP_TOP, LOC(p)) < 0 ||
        jump_to_fail_pop(c, LOC(p), pc, JUMP) < 0) {
        goto error;
    }

    USE_LABEL(c, end);
    Py_ssize_t nstores = PyList_GET_SIZE(control);
    // There's a bunch of stuff on the stack between where the new stores
    // are and where they need to be:
    // - The other stores.
    // - A copy of the subject.
    // - Anything else that may be on top of the stack.
    // - Any previous stores we've already stashed away on the stack.
    Py_ssize_t nrots = nstores + 1 + pc->on_top + PyList_GET_SIZE(pc->stores);
    for (Py_ssize_t i = 0; i < nstores; i++) {
        // Rotate this capture to its proper place on the stack:
        if (codegen_pattern_helper_rotate(c, LOC(p), nrots) < 0) {
            goto error;
        }
        // Update the list of previous stores with this new name, checking for
        // duplicates:
        PyObject *name = PyList_GET_ITEM(control, i);
        int dupe = PySequence_Contains(pc->stores, name);
        if (dupe < 0) {
            goto error;
        }
        if (dupe) {
            codegen_error_duplicate_store(c, LOC(p), name);
            goto error;
        }
        if (PyList_Append(pc->stores, name)) {
            goto error;
        }
    }
    Py_DECREF(old_pc.stores);
    Py_DECREF(control);
    // NOTE: Returning macros are safe again.
    // Pop the copy of the subject:
    ADDOP(c, LOC(p), POP_TOP);
    return SUCCESS;
diff:
    _PyCompile_Error(c, LOC(p), "alternative patterns bind different names");
error:
    PyMem_Free(old_pc.fail_pop);
    Py_DECREF(old_pc.stores);
    Py_XDECREF(control);
    return ERROR;
}


static int
codegen_pattern_sequence(compiler *c, pattern_ty p,
                         pattern_context *pc)
{
    assert(p->kind == MatchSequence_kind);
    asdl_pattern_seq *patterns = p->v.MatchSequence.patterns;
    Py_ssize_t size = asdl_seq_LEN(patterns);
    Py_ssize_t star = -1;
    int only_wildcard = 1;
    int star_wildcard = 0;
    // Find a starred name, if it exists. There may be at most one:
    for (Py_ssize_t i = 0; i < size; i++) {
        pattern_ty pattern = asdl_seq_GET(patterns, i);
        if (pattern->kind == MatchStar_kind) {
            if (star >= 0) {
                const char *e = "multiple starred names in sequence pattern";
                return _PyCompile_Error(c, LOC(p), e);
            }
            star_wildcard = WILDCARD_STAR_CHECK(pattern);
            only_wildcard &= star_wildcard;
            star = i;
            continue;
        }
        only_wildcard &= WILDCARD_CHECK(pattern);
    }
    // We need to keep the subject on top during the sequence and length checks:
    pc->on_top++;
    ADDOP(c, LOC(p), MATCH_SEQUENCE);
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    if (star < 0) {
        // No star: len(subject) == size
        ADDOP(c, LOC(p), GET_LEN);
        ADDOP_LOAD_CONST_NEW(c, LOC(p), PyLong_FromSsize_t(size));
        ADDOP_COMPARE(c, LOC(p), Eq);
        RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    }
    else if (size > 1) {
        // Star: len(subject) >= size - 1
        ADDOP(c, LOC(p), GET_LEN);
        ADDOP_LOAD_CONST_NEW(c, LOC(p), PyLong_FromSsize_t(size - 1));
        ADDOP_COMPARE(c, LOC(p), GtE);
        RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    }
    // Whatever comes next should consume the subject:
    pc->on_top--;
    if (only_wildcard) {
        // Patterns like: [] / [_] / [_, _] / [*_] / [_, *_] / [_, _, *_] / etc.
        ADDOP(c, LOC(p), POP_TOP);
    }
    else if (star_wildcard) {
        RETURN_IF_ERROR(pattern_helper_sequence_subscr(c, LOC(p), patterns, star, pc));
    }
    else {
        RETURN_IF_ERROR(pattern_helper_sequence_unpack(c, LOC(p), patterns, star, pc));
    }
    return SUCCESS;
}

static int
codegen_pattern_value(compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchValue_kind);
    expr_ty value = p->v.MatchValue.value;
    if (!MATCH_VALUE_EXPR(value)) {
        const char *e = "patterns may only match literals and attribute lookups";
        return _PyCompile_Error(c, LOC(p), e);
    }
    VISIT(c, expr, value);
    ADDOP_COMPARE(c, LOC(p), Eq);
    ADDOP(c, LOC(p), TO_BOOL);
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    return SUCCESS;
}

static int
codegen_pattern_singleton(compiler *c, pattern_ty p, pattern_context *pc)
{
    assert(p->kind == MatchSingleton_kind);
    ADDOP_LOAD_CONST(c, LOC(p), p->v.MatchSingleton.value);
    ADDOP_COMPARE(c, LOC(p), Is);
    RETURN_IF_ERROR(jump_to_fail_pop(c, LOC(p), pc, POP_JUMP_IF_FALSE));
    return SUCCESS;
}

static int
codegen_pattern(compiler *c, pattern_ty p, pattern_context *pc)
{
    switch (p->kind) {
        case MatchValue_kind:
            return codegen_pattern_value(c, p, pc);
        case MatchSingleton_kind:
            return codegen_pattern_singleton(c, p, pc);
        case MatchSequence_kind:
            return codegen_pattern_sequence(c, p, pc);
        case MatchMapping_kind:
            return codegen_pattern_mapping(c, p, pc);
        case MatchClass_kind:
            return codegen_pattern_class(c, p, pc);
        case MatchStar_kind:
            return codegen_pattern_star(c, p, pc);
        case MatchAs_kind:
            return codegen_pattern_as(c, p, pc);
        case MatchOr_kind:
            return codegen_pattern_or(c, p, pc);
    }
    // AST validator shouldn't let this happen, but if it does,
    // just fail, don't crash out of the interpreter
    const char *e = "invalid match pattern node in AST (kind=%d)";
    return _PyCompile_Error(c, LOC(p), e, p->kind);
}

static int
codegen_match_inner(compiler *c, stmt_ty s, pattern_context *pc)
{
    VISIT(c, expr, s->v.Match.subject);
    NEW_JUMP_TARGET_LABEL(c, end);
    Py_ssize_t cases = asdl_seq_LEN(s->v.Match.cases);
    assert(cases > 0);
    match_case_ty m = asdl_seq_GET(s->v.Match.cases, cases - 1);
    int has_default = WILDCARD_CHECK(m->pattern) && 1 < cases;
    for (Py_ssize_t i = 0; i < cases - has_default; i++) {
        m = asdl_seq_GET(s->v.Match.cases, i);
        // Only copy the subject if we're *not* on the last case:
        if (i != cases - has_default - 1) {
            ADDOP_I(c, LOC(m->pattern), COPY, 1);
        }
        pc->stores = PyList_New(0);
        if (pc->stores == NULL) {
            return ERROR;
        }
        // Irrefutable cases must be either guarded, last, or both:
        pc->allow_irrefutable = m->guard != NULL || i == cases - 1;
        pc->fail_pop = NULL;
        pc->fail_pop_size = 0;
        pc->on_top = 0;
        // NOTE: Can't use returning macros here (they'll leak pc->stores)!
        if (codegen_pattern(c, m->pattern, pc) < 0) {
            Py_DECREF(pc->stores);
            return ERROR;
        }
        assert(!pc->on_top);
        // It's a match! Store all of the captured names (they're on the stack).
        Py_ssize_t nstores = PyList_GET_SIZE(pc->stores);
        for (Py_ssize_t n = 0; n < nstores; n++) {
            PyObject *name = PyList_GET_ITEM(pc->stores, n);
            if (codegen_nameop(c, LOC(m->pattern), name, Store) < 0) {
                Py_DECREF(pc->stores);
                return ERROR;
            }
        }
        Py_DECREF(pc->stores);
        // NOTE: Returning macros are safe again.
        if (m->guard) {
            RETURN_IF_ERROR(ensure_fail_pop(c, pc, 0));
            RETURN_IF_ERROR(codegen_jump_if(c, LOC(m->pattern), m->guard, pc->fail_pop[0], 0));
        }
        // Success! Pop the subject off, we're done with it:
        if (i != cases - has_default - 1) {
            /* Use the next location to give better locations for branch events */
            ADDOP(c, NEXT_LOCATION, POP_TOP);
        }
        VISIT_SEQ(c, stmt, m->body);
        ADDOP_JUMP(c, NO_LOCATION, JUMP, end);
        // If the pattern fails to match, we want the line number of the
        // cleanup to be associated with the failed pattern, not the last line
        // of the body
        RETURN_IF_ERROR(emit_and_reset_fail_pop(c, LOC(m->pattern), pc));
    }
    if (has_default) {
        // A trailing "case _" is common, and lets us save a bit of redundant
        // pushing and popping in the loop above:
        m = asdl_seq_GET(s->v.Match.cases, cases - 1);
        if (cases == 1) {
            // No matches. Done with the subject:
            ADDOP(c, LOC(m->pattern), POP_TOP);
        }
        else {
            // Show line coverage for default case (it doesn't create bytecode)
            ADDOP(c, LOC(m->pattern), NOP);
        }
        if (m->guard) {
            RETURN_IF_ERROR(codegen_jump_if(c, LOC(m->pattern), m->guard, end, 0));
        }
        VISIT_SEQ(c, stmt, m->body);
    }
    USE_LABEL(c, end);
    return SUCCESS;
}

static int
codegen_match(compiler *c, stmt_ty s)
{
    pattern_context pc;
    pc.fail_pop = NULL;
    int result = codegen_match_inner(c, s, &pc);
    PyMem_Free(pc.fail_pop);
    return result;
}

#undef WILDCARD_CHECK
#undef WILDCARD_STAR_CHECK


int
_PyCodegen_AddReturnAtEnd(compiler *c, int addNone)
{
    /* Make sure every instruction stream that falls off the end returns None.
     * This also ensures that no jump target offsets are out of bounds.
     */
    if (addNone) {
        ADDOP_LOAD_CONST(c, NO_LOCATION, Py_None);
    }
    ADDOP(c, NO_LOCATION, RETURN_VALUE);
    return SUCCESS;
}
