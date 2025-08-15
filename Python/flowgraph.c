#include "Python.h"
#include "pycore_flowgraph.h"
#include "pycore_compile.h"
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()

#include "pycore_opcode_utils.h"
#include "pycore_opcode_metadata.h" // OPCODE_HAS_ARG, etc

#include <stdbool.h>


#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    if ((X) == -1) {        \
        return ERROR;       \
    }

#define DEFAULT_BLOCK_SIZE 16

typedef _Py_SourceLocation location;
typedef _PyJumpTargetLabel jump_target_label;

typedef struct _PyCfgInstruction {
    int i_opcode;
    int i_oparg;
    _Py_SourceLocation i_loc;
    struct _PyCfgBasicblock *i_target; /* target block (if jump instruction) */
    struct _PyCfgBasicblock *i_except; /* target block when exception is raised */
} cfg_instr;

typedef struct _PyCfgBasicblock {
    /* Each basicblock in a compilation unit is linked via b_list in the
       reverse order that the block are allocated.  b_list points to the next
       block in this list, not to be confused with b_next, which is next by
       control flow. */
    struct _PyCfgBasicblock *b_list;
    /* The label of this block if it is a jump target, -1 otherwise */
    _PyJumpTargetLabel b_label;
    /* Exception stack at start of block, used by assembler to create the exception handling table */
    struct _PyCfgExceptStack *b_exceptstack;
    /* pointer to an array of instructions, initially NULL */
    cfg_instr *b_instr;
    /* If b_next is non-NULL, it is a pointer to the next
       block reached by normal control flow. */
    struct _PyCfgBasicblock *b_next;
    /* number of instructions used */
    int b_iused;
    /* length of instruction array (b_instr) */
    int b_ialloc;
    /* Used by add_checks_for_loads_of_unknown_variables */
    uint64_t b_unsafe_locals_mask;
    /* Number of predecessors that a block has. */
    int b_predecessors;
    /* depth of stack upon entry of block, computed by stackdepth() */
    int b_startdepth;
    /* Basic block is an exception handler that preserves lasti */
    unsigned b_preserve_lasti : 1;
    /* Used by compiler passes to mark whether they have visited a basic block. */
    unsigned b_visited : 1;
    /* b_except_handler is used by the cold-detection algorithm to mark exception targets */
    unsigned b_except_handler : 1;
    /* b_cold is true if this block is not perf critical (like an exception handler) */
    unsigned b_cold : 1;
    /* b_warm is used by the cold-detection algorithm to mark blocks which are definitely not cold */
    unsigned b_warm : 1;
} basicblock;


struct _PyCfgBuilder {
    /* The entryblock, at which control flow begins. All blocks of the
       CFG are reachable through the b_next links */
    struct _PyCfgBasicblock *g_entryblock;
    /* Pointer to the most recently allocated block.  By following
       b_list links, you can reach all allocated blocks. */
    struct _PyCfgBasicblock *g_block_list;
    /* pointer to the block currently being constructed */
    struct _PyCfgBasicblock *g_curblock;
    /* label for the next instruction to be placed */
    _PyJumpTargetLabel g_current_label;
};

typedef struct _PyCfgBuilder cfg_builder;

static const jump_target_label NO_LABEL = {-1};

#define SAME_LABEL(L1, L2) ((L1).id == (L2).id)
#define IS_LABEL(L) (!SAME_LABEL((L), (NO_LABEL)))

#define LOCATION(LNO, END_LNO, COL, END_COL) \
    ((const _Py_SourceLocation){(LNO), (END_LNO), (COL), (END_COL)})

static inline int
is_block_push(cfg_instr *i)
{
    assert(OPCODE_HAS_ARG(i->i_opcode) || !IS_BLOCK_PUSH_OPCODE(i->i_opcode));
    return IS_BLOCK_PUSH_OPCODE(i->i_opcode);
}

static inline int
is_jump(cfg_instr *i)
{
    return OPCODE_HAS_JUMP(i->i_opcode);
}

/* One arg*/
#define INSTR_SET_OP1(I, OP, ARG) \
    do { \
        assert(OPCODE_HAS_ARG(OP)); \
        cfg_instr *_instr__ptr_ = (I); \
        _instr__ptr_->i_opcode = (OP); \
        _instr__ptr_->i_oparg = (ARG); \
    } while (0);

/* No args*/
#define INSTR_SET_OP0(I, OP) \
    do { \
        assert(!OPCODE_HAS_ARG(OP)); \
        cfg_instr *_instr__ptr_ = (I); \
        _instr__ptr_->i_opcode = (OP); \
        _instr__ptr_->i_oparg = 0; \
    } while (0);

/***** Blocks *****/

/* Returns the offset of the next instruction in the current block's
   b_instr array.  Resizes the b_instr as necessary.
   Returns -1 on failure.
*/
static int
basicblock_next_instr(basicblock *b)
{
    assert(b != NULL);
    RETURN_IF_ERROR(
        _PyCompile_EnsureArrayLargeEnough(
            b->b_iused + 1,
            (void**)&b->b_instr,
            &b->b_ialloc,
            DEFAULT_BLOCK_SIZE,
            sizeof(cfg_instr)));
    return b->b_iused++;
}

static cfg_instr *
basicblock_last_instr(const basicblock *b) {
    assert(b->b_iused >= 0);
    if (b->b_iused > 0) {
        assert(b->b_instr != NULL);
        return &b->b_instr[b->b_iused - 1];
    }
    return NULL;
}

/* Allocate a new block and return a pointer to it.
   Returns NULL on error.
*/

static basicblock *
cfg_builder_new_block(cfg_builder *g)
{
    basicblock *b = (basicblock *)PyMem_Calloc(1, sizeof(basicblock));
    if (b == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    /* Extend the singly linked list of blocks with new block. */
    b->b_list = g->g_block_list;
    g->g_block_list = b;
    b->b_label = NO_LABEL;
    return b;
}

static int
basicblock_addop(basicblock *b, int opcode, int oparg, location loc)
{
    assert(IS_WITHIN_OPCODE_RANGE(opcode));
    assert(!IS_ASSEMBLER_OPCODE(opcode));
    assert(OPCODE_HAS_ARG(opcode) || HAS_TARGET(opcode) || oparg == 0);
    assert(0 <= oparg && oparg < (1 << 30));

    int off = basicblock_next_instr(b);
    if (off < 0) {
        return ERROR;
    }
    cfg_instr *i = &b->b_instr[off];
    i->i_opcode = opcode;
    i->i_oparg = oparg;
    i->i_target = NULL;
    i->i_loc = loc;

    return SUCCESS;
}

static int
basicblock_add_jump(basicblock *b, int opcode, basicblock *target, location loc)
{
    cfg_instr *last = basicblock_last_instr(b);
    if (last && is_jump(last)) {
        return ERROR;
    }

    RETURN_IF_ERROR(
        basicblock_addop(b, opcode, target->b_label.id, loc));
    last = basicblock_last_instr(b);
    assert(last && last->i_opcode == opcode);
    last->i_target = target;
    return SUCCESS;
}

static inline int
basicblock_append_instructions(basicblock *to, basicblock *from)
{
    for (int i = 0; i < from->b_iused; i++) {
        int n = basicblock_next_instr(to);
        if (n < 0) {
            return ERROR;
        }
        to->b_instr[n] = from->b_instr[i];
    }
    return SUCCESS;
}

static inline int
basicblock_nofallthrough(const basicblock *b) {
    cfg_instr *last = basicblock_last_instr(b);
    return (last &&
            (IS_SCOPE_EXIT_OPCODE(last->i_opcode) ||
             IS_UNCONDITIONAL_JUMP_OPCODE(last->i_opcode)));
}

#define BB_NO_FALLTHROUGH(B) (basicblock_nofallthrough(B))
#define BB_HAS_FALLTHROUGH(B) (!basicblock_nofallthrough(B))

static basicblock *
copy_basicblock(cfg_builder *g, basicblock *block)
{
    /* Cannot copy a block if it has a fallthrough, since
     * a block can only have one fallthrough predecessor.
     */
    assert(BB_NO_FALLTHROUGH(block));
    basicblock *result = cfg_builder_new_block(g);
    if (result == NULL) {
        return NULL;
    }
    if (basicblock_append_instructions(result, block) < 0) {
        return NULL;
    }
    return result;
}

static int
basicblock_insert_instruction(basicblock *block, int pos, cfg_instr *instr) {
    RETURN_IF_ERROR(basicblock_next_instr(block));
    for (int i = block->b_iused - 1; i > pos; i--) {
        block->b_instr[i] = block->b_instr[i-1];
    }
    block->b_instr[pos] = *instr;
    return SUCCESS;
}

/* For debugging purposes only */
#if 0
static void
dump_instr(cfg_instr *i)
{
    const char *jump = is_jump(i) ? "jump " : "";

    char arg[128];

    *arg = '\0';
    if (OPCODE_HAS_ARG(i->i_opcode)) {
        sprintf(arg, "arg: %d ", i->i_oparg);
    }
    if (HAS_TARGET(i->i_opcode)) {
        sprintf(arg, "target: %p [%d] ", i->i_target, i->i_oparg);
    }
    fprintf(stderr, "line: %d, %s (%d)  %s%s\n",
                    i->i_loc.lineno, _PyOpcode_OpName[i->i_opcode], i->i_opcode, arg, jump);
}

static inline int
basicblock_returns(const basicblock *b) {
    cfg_instr *last = basicblock_last_instr(b);
    return last && (last->i_opcode == RETURN_VALUE || last->i_opcode == RETURN_CONST);
}

static void
dump_basicblock(const basicblock *b)
{
    const char *b_return = basicblock_returns(b) ? "return " : "";
    fprintf(stderr, "%d: [EH=%d CLD=%d WRM=%d NO_FT=%d %p] used: %d, depth: %d, preds: %d %s\n",
        b->b_label.id, b->b_except_handler, b->b_cold, b->b_warm, BB_NO_FALLTHROUGH(b), b, b->b_iused,
        b->b_startdepth, b->b_predecessors, b_return);
    if (b->b_instr) {
        int i;
        for (i = 0; i < b->b_iused; i++) {
            fprintf(stderr, "  [%02d] ", i);
            dump_instr(b->b_instr + i);
        }
    }
}

void
_PyCfgBuilder_DumpGraph(const basicblock *entryblock)
{
    for (const basicblock *b = entryblock; b != NULL; b = b->b_next) {
        dump_basicblock(b);
    }
}

#endif


/***** CFG construction and modification *****/

static basicblock *
cfg_builder_use_next_block(cfg_builder *g, basicblock *block)
{
    assert(block != NULL);
    g->g_curblock->b_next = block;
    g->g_curblock = block;
    return block;
}

static inline int
basicblock_exits_scope(const basicblock *b) {
    cfg_instr *last = basicblock_last_instr(b);
    return last && IS_SCOPE_EXIT_OPCODE(last->i_opcode);
}

static inline int
basicblock_has_eval_break(const basicblock *b) {
    for (int i = 0; i < b->b_iused; i++) {
        if (OPCODE_HAS_EVAL_BREAK(b->b_instr[i].i_opcode)) {
            return true;
        }
    }
    return false;
}

static bool
cfg_builder_current_block_is_terminated(cfg_builder *g)
{
    cfg_instr *last = basicblock_last_instr(g->g_curblock);
    if (last && IS_TERMINATOR_OPCODE(last->i_opcode)) {
        return true;
    }
    if (IS_LABEL(g->g_current_label)) {
        if (last || IS_LABEL(g->g_curblock->b_label)) {
            return true;
        }
        else {
            /* current block is empty, label it */
            g->g_curblock->b_label = g->g_current_label;
            g->g_current_label = NO_LABEL;
        }
    }
    return false;
}

static int
cfg_builder_maybe_start_new_block(cfg_builder *g)
{
    if (cfg_builder_current_block_is_terminated(g)) {
        basicblock *b = cfg_builder_new_block(g);
        if (b == NULL) {
            return ERROR;
        }
        b->b_label = g->g_current_label;
        g->g_current_label = NO_LABEL;
        cfg_builder_use_next_block(g, b);
    }
    return SUCCESS;
}

#ifndef NDEBUG
static bool
cfg_builder_check(cfg_builder *g)
{
    assert(g->g_entryblock->b_iused > 0);
    for (basicblock *block = g->g_block_list; block != NULL; block = block->b_list) {
        assert(!_PyMem_IsPtrFreed(block));
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
    return true;
}
#endif

static int
init_cfg_builder(cfg_builder *g)
{
    g->g_block_list = NULL;
    basicblock *block = cfg_builder_new_block(g);
    if (block == NULL) {
        return ERROR;
    }
    g->g_curblock = g->g_entryblock = block;
    g->g_current_label = NO_LABEL;
    return SUCCESS;
}

cfg_builder *
_PyCfgBuilder_New(void)
{
    cfg_builder *g = PyMem_Malloc(sizeof(cfg_builder));
    if (g == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(g, 0, sizeof(cfg_builder));
    if (init_cfg_builder(g) < 0) {
        PyMem_Free(g);
        return NULL;
    }
    return g;
}

void
_PyCfgBuilder_Free(cfg_builder *g)
{
    if (g == NULL) {
        return;
    }
    assert(cfg_builder_check(g));
    basicblock *b = g->g_block_list;
    while (b != NULL) {
        if (b->b_instr) {
            PyMem_Free((void *)b->b_instr);
        }
        basicblock *next = b->b_list;
        PyMem_Free((void *)b);
        b = next;
    }
    PyMem_Free(g);
}

int
_PyCfgBuilder_CheckSize(cfg_builder *g)
{
    int nblocks = 0;
    for (basicblock *b = g->g_block_list; b != NULL; b = b->b_list) {
        nblocks++;
    }
    if ((size_t)nblocks > SIZE_MAX / sizeof(basicblock *)) {
        PyErr_NoMemory();
        return ERROR;
    }
    return SUCCESS;
}

int
_PyCfgBuilder_UseLabel(cfg_builder *g, jump_target_label lbl)
{
    g->g_current_label = lbl;
    return cfg_builder_maybe_start_new_block(g);
}

int
_PyCfgBuilder_Addop(cfg_builder *g, int opcode, int oparg, location loc)
{
    RETURN_IF_ERROR(cfg_builder_maybe_start_new_block(g));
    return basicblock_addop(g->g_curblock, opcode, oparg, loc);
}


static basicblock *
next_nonempty_block(basicblock *b)
{
    while (b && b->b_iused == 0) {
        b = b->b_next;
    }
    return b;
}

/***** debugging helpers *****/

#ifndef NDEBUG
static int remove_redundant_nops(cfg_builder *g);

static bool
no_redundant_nops(cfg_builder *g) {
    if (remove_redundant_nops(g) != 0) {
        return false;
    }
    return true;
}

static bool
no_redundant_jumps(cfg_builder *g) {
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        cfg_instr *last = basicblock_last_instr(b);
        if (last != NULL) {
            if (IS_UNCONDITIONAL_JUMP_OPCODE(last->i_opcode)) {
                basicblock *next = next_nonempty_block(b->b_next);
                basicblock *jump_target = next_nonempty_block(last->i_target);
                if (jump_target == next) {
                    assert(next);
                    if (last->i_loc.lineno == next->b_instr[0].i_loc.lineno) {
                        assert(0);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

#endif

/***** CFG preprocessing (jump targets and exceptions) *****/

static int
normalize_jumps_in_block(cfg_builder *g, basicblock *b) {
    cfg_instr *last = basicblock_last_instr(b);
    if (last == NULL || !is_jump(last) ||
        IS_UNCONDITIONAL_JUMP_OPCODE(last->i_opcode)) {
        return SUCCESS;
    }
    assert(!IS_ASSEMBLER_OPCODE(last->i_opcode));

    bool is_forward = last->i_target->b_visited == 0;
    if (is_forward) {
        return SUCCESS;
    }

    int reversed_opcode = 0;
    switch(last->i_opcode) {
        case POP_JUMP_IF_NOT_NONE:
            reversed_opcode = POP_JUMP_IF_NONE;
            break;
        case POP_JUMP_IF_NONE:
            reversed_opcode = POP_JUMP_IF_NOT_NONE;
            break;
        case POP_JUMP_IF_FALSE:
            reversed_opcode = POP_JUMP_IF_TRUE;
            break;
        case POP_JUMP_IF_TRUE:
            reversed_opcode = POP_JUMP_IF_FALSE;
            break;
    }
    /* transform 'conditional jump T' to
     * 'reversed_jump b_next' followed by 'jump_backwards T'
     */

    basicblock *target = last->i_target;
    basicblock *backwards_jump = cfg_builder_new_block(g);
    if (backwards_jump == NULL) {
        return ERROR;
    }
    RETURN_IF_ERROR(
        basicblock_add_jump(backwards_jump, JUMP, target, last->i_loc));
    last->i_opcode = reversed_opcode;
    last->i_target = b->b_next;

    backwards_jump->b_cold = b->b_cold;
    backwards_jump->b_next = b->b_next;
    b->b_next = backwards_jump;
    return SUCCESS;
}


static int
normalize_jumps(cfg_builder *g)
{
    basicblock *entryblock = g->g_entryblock;
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        b->b_visited = 0;
    }
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        b->b_visited = 1;
        RETURN_IF_ERROR(normalize_jumps_in_block(g, b));
    }
    return SUCCESS;
}

static int
check_cfg(cfg_builder *g) {
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        /* Raise SystemError if jump or exit is not last instruction in the block. */
        for (int i = 0; i < b->b_iused; i++) {
            int opcode = b->b_instr[i].i_opcode;
            assert(!IS_ASSEMBLER_OPCODE(opcode));
            if (IS_TERMINATOR_OPCODE(opcode)) {
                if (i != b->b_iused - 1) {
                    PyErr_SetString(PyExc_SystemError, "malformed control flow graph.");
                    return ERROR;
                }
            }
        }
    }
    return SUCCESS;
}

static int
get_max_label(basicblock *entryblock)
{
    int lbl = -1;
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        if (b->b_label.id > lbl) {
            lbl = b->b_label.id;
        }
    }
    return lbl;
}

/* Calculate the actual jump target from the target_label */
static int
translate_jump_labels_to_targets(basicblock *entryblock)
{
    int max_label = get_max_label(entryblock);
    size_t mapsize = sizeof(basicblock *) * (max_label + 1);
    basicblock **label2block = (basicblock **)PyMem_Malloc(mapsize);
    if (!label2block) {
        PyErr_NoMemory();
        return ERROR;
    }
    memset(label2block, 0, mapsize);
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        if (b->b_label.id >= 0) {
            label2block[b->b_label.id] = b;
        }
    }
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            assert(instr->i_target == NULL);
            if (HAS_TARGET(instr->i_opcode)) {
                int lbl = instr->i_oparg;
                assert(lbl >= 0 && lbl <= max_label);
                instr->i_target = label2block[lbl];
                assert(instr->i_target != NULL);
                assert(instr->i_target->b_label.id == lbl);
            }
        }
    }
    PyMem_Free(label2block);
    return SUCCESS;
}

static int
mark_except_handlers(basicblock *entryblock) {
#ifndef NDEBUG
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        assert(!b->b_except_handler);
    }
#endif
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        for (int i=0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            if (is_block_push(instr)) {
                instr->i_target->b_except_handler = 1;
            }
        }
    }
    return SUCCESS;
}


struct _PyCfgExceptStack {
    basicblock *handlers[CO_MAXBLOCKS+2];
    int depth;
};


static basicblock *
push_except_block(struct _PyCfgExceptStack *stack, cfg_instr *setup) {
    assert(is_block_push(setup));
    int opcode = setup->i_opcode;
    basicblock * target = setup->i_target;
    if (opcode == SETUP_WITH || opcode == SETUP_CLEANUP) {
        target->b_preserve_lasti = 1;
    }
    assert(stack->depth <= CO_MAXBLOCKS);
    stack->handlers[++stack->depth] = target;
    return target;
}

static basicblock *
pop_except_block(struct _PyCfgExceptStack *stack) {
    assert(stack->depth > 0);
    return stack->handlers[--stack->depth];
}

static basicblock *
except_stack_top(struct _PyCfgExceptStack *stack) {
    return stack->handlers[stack->depth];
}

static struct _PyCfgExceptStack *
make_except_stack(void) {
    struct _PyCfgExceptStack *new = PyMem_Malloc(sizeof(struct _PyCfgExceptStack));
    if (new == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    new->depth = 0;
    new->handlers[0] = NULL;
    return new;
}

static struct _PyCfgExceptStack *
copy_except_stack(struct _PyCfgExceptStack *stack) {
    struct _PyCfgExceptStack *copy = PyMem_Malloc(sizeof(struct _PyCfgExceptStack));
    if (copy == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(copy, stack, sizeof(struct _PyCfgExceptStack));
    return copy;
}

static basicblock**
make_cfg_traversal_stack(basicblock *entryblock) {
    int nblocks = 0;
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        b->b_visited = 0;
        nblocks++;
    }
    basicblock **stack = (basicblock **)PyMem_Malloc(sizeof(basicblock *) * nblocks);
    if (!stack) {
        PyErr_NoMemory();
    }
    return stack;
}

Py_LOCAL_INLINE(int)
stackdepth_push(basicblock ***sp, basicblock *b, int depth)
{
    if (!(b->b_startdepth < 0 || b->b_startdepth == depth)) {
        PyErr_Format(PyExc_ValueError, "Invalid CFG, inconsistent stackdepth");
        return ERROR;
    }
    if (b->b_startdepth < depth && b->b_startdepth < 100) {
        assert(b->b_startdepth < 0);
        b->b_startdepth = depth;
        *(*sp)++ = b;
    }
    return SUCCESS;
}

/* Find the flow path that needs the largest stack.  We assume that
 * cycles in the flow graph have no net effect on the stack depth.
 */
static int
calculate_stackdepth(cfg_builder *g)
{
    basicblock *entryblock = g->g_entryblock;
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        b->b_startdepth = INT_MIN;
    }
    basicblock **stack = make_cfg_traversal_stack(entryblock);
    if (!stack) {
        return ERROR;
    }


    int stackdepth = -1;
    int maxdepth = 0;
    basicblock **sp = stack;
    if (stackdepth_push(&sp, entryblock, 0) < 0) {
        goto error;
    }
    while (sp != stack) {
        basicblock *b = *--sp;
        int depth = b->b_startdepth;
        assert(depth >= 0);
        basicblock *next = b->b_next;
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            int effect = PyCompile_OpcodeStackEffectWithJump(
                             instr->i_opcode, instr->i_oparg, 0);
            if (effect == PY_INVALID_STACK_EFFECT) {
                PyErr_Format(PyExc_SystemError,
                             "Invalid stack effect for opcode=%d, arg=%i",
                             instr->i_opcode, instr->i_oparg);
                goto error;
            }
            int new_depth = depth + effect;
            if (new_depth < 0) {
               PyErr_Format(PyExc_ValueError,
                            "Invalid CFG, stack underflow");
               goto error;
            }
            if (new_depth > maxdepth) {
                maxdepth = new_depth;
            }
            if (HAS_TARGET(instr->i_opcode)) {
                effect = PyCompile_OpcodeStackEffectWithJump(
                             instr->i_opcode, instr->i_oparg, 1);
                if (effect == PY_INVALID_STACK_EFFECT) {
                    PyErr_Format(PyExc_SystemError,
                                 "Invalid stack effect for opcode=%d, arg=%i",
                                 instr->i_opcode, instr->i_oparg);
                    goto error;
                }
                int target_depth = depth + effect;
                assert(target_depth >= 0); /* invalid code or bug in stackdepth() */
                if (target_depth > maxdepth) {
                    maxdepth = target_depth;
                }
                if (stackdepth_push(&sp, instr->i_target, target_depth) < 0) {
                    goto error;
                }
            }
            depth = new_depth;
            assert(!IS_ASSEMBLER_OPCODE(instr->i_opcode));
            if (IS_UNCONDITIONAL_JUMP_OPCODE(instr->i_opcode) ||
                IS_SCOPE_EXIT_OPCODE(instr->i_opcode))
            {
                /* remaining code is dead */
                next = NULL;
                break;
            }
        }
        if (next != NULL) {
            assert(BB_HAS_FALLTHROUGH(b));
            if (stackdepth_push(&sp, next, depth) < 0) {
                goto error;
            }
        }
    }
    stackdepth = maxdepth;
error:
    PyMem_Free(stack);
    return stackdepth;
}

static int
label_exception_targets(basicblock *entryblock) {
    basicblock **todo_stack = make_cfg_traversal_stack(entryblock);
    if (todo_stack == NULL) {
        return ERROR;
    }
    struct _PyCfgExceptStack *except_stack = make_except_stack();
    if (except_stack == NULL) {
        PyMem_Free(todo_stack);
        PyErr_NoMemory();
        return ERROR;
    }
    except_stack->depth = 0;
    todo_stack[0] = entryblock;
    entryblock->b_visited = 1;
    entryblock->b_exceptstack = except_stack;
    basicblock **todo = &todo_stack[1];
    basicblock *handler = NULL;
    while (todo > todo_stack) {
        todo--;
        basicblock *b = todo[0];
        assert(b->b_visited == 1);
        except_stack = b->b_exceptstack;
        assert(except_stack != NULL);
        b->b_exceptstack = NULL;
        handler = except_stack_top(except_stack);
        int last_yield_except_depth = -1;
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            if (is_block_push(instr)) {
                if (!instr->i_target->b_visited) {
                    struct _PyCfgExceptStack *copy = copy_except_stack(except_stack);
                    if (copy == NULL) {
                        goto error;
                    }
                    instr->i_target->b_exceptstack = copy;
                    todo[0] = instr->i_target;
                    instr->i_target->b_visited = 1;
                    todo++;
                }
                handler = push_except_block(except_stack, instr);
            }
            else if (instr->i_opcode == POP_BLOCK) {
                handler = pop_except_block(except_stack);
                INSTR_SET_OP0(instr, NOP);
            }
            else if (is_jump(instr)) {
                instr->i_except = handler;
                assert(i == b->b_iused -1);
                if (!instr->i_target->b_visited) {
                    if (BB_HAS_FALLTHROUGH(b)) {
                        struct _PyCfgExceptStack *copy = copy_except_stack(except_stack);
                        if (copy == NULL) {
                            goto error;
                        }
                        instr->i_target->b_exceptstack = copy;
                    }
                    else {
                        instr->i_target->b_exceptstack = except_stack;
                        except_stack = NULL;
                    }
                    todo[0] = instr->i_target;
                    instr->i_target->b_visited = 1;
                    todo++;
                }
            }
            else if (instr->i_opcode == YIELD_VALUE) {
                instr->i_except = handler;
                last_yield_except_depth = except_stack->depth;
            }
            else if (instr->i_opcode == RESUME) {
                instr->i_except = handler;
                if (instr->i_oparg != RESUME_AT_FUNC_START) {
                    assert(last_yield_except_depth >= 0);
                    if (last_yield_except_depth == 1) {
                        instr->i_oparg |= RESUME_OPARG_DEPTH1_MASK;
                    }
                    last_yield_except_depth = -1;
                }
            }
            else {
                instr->i_except = handler;
            }
        }
        if (BB_HAS_FALLTHROUGH(b) && !b->b_next->b_visited) {
            assert(except_stack != NULL);
            b->b_next->b_exceptstack = except_stack;
            todo[0] = b->b_next;
            b->b_next->b_visited = 1;
            todo++;
        }
        else if (except_stack != NULL) {
           PyMem_Free(except_stack);
        }
    }
#ifdef Py_DEBUG
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        assert(b->b_exceptstack == NULL);
    }
#endif
    PyMem_Free(todo_stack);
    return SUCCESS;
error:
    PyMem_Free(todo_stack);
    PyMem_Free(except_stack);
    return ERROR;
}

/***** CFG optimizations *****/

static int
remove_unreachable(basicblock *entryblock) {
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        b->b_predecessors = 0;
    }
    basicblock **stack = make_cfg_traversal_stack(entryblock);
    if (stack == NULL) {
        return ERROR;
    }
    basicblock **sp = stack;
    entryblock->b_predecessors = 1;
    *sp++ = entryblock;
    entryblock->b_visited = 1;
    while (sp > stack) {
        basicblock *b = *(--sp);
        if (b->b_next && BB_HAS_FALLTHROUGH(b)) {
            if (!b->b_next->b_visited) {
                assert(b->b_next->b_predecessors == 0);
                *sp++ = b->b_next;
                b->b_next->b_visited = 1;
            }
            b->b_next->b_predecessors++;
        }
        for (int i = 0; i < b->b_iused; i++) {
            basicblock *target;
            cfg_instr *instr = &b->b_instr[i];
            if (is_jump(instr) || is_block_push(instr)) {
                target = instr->i_target;
                if (!target->b_visited) {
                    *sp++ = target;
                    target->b_visited = 1;
                }
                target->b_predecessors++;
            }
        }
    }
    PyMem_Free(stack);

    /* Delete unreachable instructions */
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
       if (b->b_predecessors == 0) {
            b->b_iused = 0;
            b->b_except_handler = 0;
       }
    }
    return SUCCESS;
}

static int
basicblock_remove_redundant_nops(basicblock *bb) {
    /* Remove NOPs when legal to do so. */
    int dest = 0;
    int prev_lineno = -1;
    for (int src = 0; src < bb->b_iused; src++) {
        int lineno = bb->b_instr[src].i_loc.lineno;
        if (bb->b_instr[src].i_opcode == NOP) {
            /* Eliminate no-op if it doesn't have a line number */
            if (lineno < 0) {
                continue;
            }
            /* or, if the previous instruction had the same line number. */
            if (prev_lineno == lineno) {
                continue;
            }
            /* or, if the next instruction has same line number or no line number */
            if (src < bb->b_iused - 1) {
                int next_lineno = bb->b_instr[src+1].i_loc.lineno;
                if (next_lineno == lineno) {
                    continue;
                }
                if (next_lineno < 0) {
                    bb->b_instr[src+1].i_loc = bb->b_instr[src].i_loc;
                    continue;
                }
            }
            else {
                basicblock *next = next_nonempty_block(bb->b_next);
                /* or if last instruction in BB and next BB has same line number */
                if (next) {
                    location next_loc = NO_LOCATION;
                    for (int next_i=0; next_i < next->b_iused; next_i++) {
                        cfg_instr *instr = &next->b_instr[next_i];
                        if (instr->i_opcode == NOP && instr->i_loc.lineno == NO_LOCATION.lineno) {
                            /* Skip over NOPs without location, they will be removed */
                            continue;
                        }
                        next_loc = instr->i_loc;
                        break;
                    }
                    if (lineno == next_loc.lineno) {
                        continue;
                    }
                }
            }

        }
        if (dest != src) {
            bb->b_instr[dest] = bb->b_instr[src];
        }
        dest++;
        prev_lineno = lineno;
    }
    assert(dest <= bb->b_iused);
    int num_removed = bb->b_iused - dest;
    bb->b_iused = dest;
    return num_removed;
}

static int
remove_redundant_nops(cfg_builder *g) {
    int changes = 0;
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        int change = basicblock_remove_redundant_nops(b);
        RETURN_IF_ERROR(change);
        changes += change;
    }
    return changes;
}

static int
remove_redundant_nops_and_pairs(basicblock *entryblock)
{
    bool done = false;

    while (! done) {
        done = true;
        cfg_instr *prev_instr = NULL;
        cfg_instr *instr = NULL;
        for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
            RETURN_IF_ERROR(basicblock_remove_redundant_nops(b));
            if (IS_LABEL(b->b_label)) {
                /* this block is a jump target, forget instr */
                instr = NULL;
            }
            for (int i = 0; i < b->b_iused; i++) {
                prev_instr = instr;
                instr = &b->b_instr[i];
                int prev_opcode = prev_instr ? prev_instr->i_opcode : 0;
                int prev_oparg = prev_instr ? prev_instr->i_oparg : 0;
                int opcode = instr->i_opcode;
                bool is_redundant_pair = false;
                if (opcode == POP_TOP) {
                   if (prev_opcode == LOAD_CONST) {
                       is_redundant_pair = true;
                   }
                   else if (prev_opcode == COPY && prev_oparg == 1) {
                       is_redundant_pair = true;
                   }
                }
                if (is_redundant_pair) {
                    INSTR_SET_OP0(prev_instr, NOP);
                    INSTR_SET_OP0(instr, NOP);
                    done = false;
                }
            }
            if ((instr && is_jump(instr)) || !BB_HAS_FALLTHROUGH(b)) {
                instr = NULL;
            }
        }
    }
    return SUCCESS;
}

static int
remove_redundant_jumps(cfg_builder *g) {
    /* If a non-empty block ends with a jump instruction, check if the next
     * non-empty block reached through normal flow control is the target
     * of that jump. If it is, then the jump instruction is redundant and
     * can be deleted.
     *
     * Return the number of changes applied, or -1 on error.
     */

    int changes = 0;
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        cfg_instr *last = basicblock_last_instr(b);
        if (last == NULL) {
            continue;
        }
        assert(!IS_ASSEMBLER_OPCODE(last->i_opcode));
        if (IS_UNCONDITIONAL_JUMP_OPCODE(last->i_opcode)) {
            basicblock* jump_target = next_nonempty_block(last->i_target);
            if (jump_target == NULL) {
                PyErr_SetString(PyExc_SystemError, "jump with NULL target");
                return ERROR;
            }
            basicblock *next = next_nonempty_block(b->b_next);
            if (jump_target == next) {
                changes++;
                INSTR_SET_OP0(last, NOP);
            }
        }
    }

    return changes;
}

static inline bool
basicblock_has_no_lineno(basicblock *b) {
    for (int i = 0; i < b->b_iused; i++) {
        if (b->b_instr[i].i_loc.lineno >= 0) {
            return false;
        }
    }
    return true;
}

/* Maximum size of basic block that should be copied in optimizer */
#define MAX_COPY_SIZE 4

/* If this block ends with an unconditional jump to a small exit block or
 * a block that has no line numbers (and no fallthrough), then
 * remove the jump and extend this block with the target.
 * Returns 1 if extended, 0 if no change, and -1 on error.
 */
static int
basicblock_inline_small_or_no_lineno_blocks(basicblock *bb) {
    cfg_instr *last = basicblock_last_instr(bb);
    if (last == NULL) {
        return 0;
    }
    if (!IS_UNCONDITIONAL_JUMP_OPCODE(last->i_opcode)) {
        return 0;
    }
    basicblock *target = last->i_target;
    bool small_exit_block = (basicblock_exits_scope(target) &&
                             target->b_iused <= MAX_COPY_SIZE);
    bool no_lineno_no_fallthrough = (basicblock_has_no_lineno(target) &&
                                     !BB_HAS_FALLTHROUGH(target));
    if (small_exit_block || no_lineno_no_fallthrough) {
        assert(is_jump(last));
        int removed_jump_opcode = last->i_opcode;
        INSTR_SET_OP0(last, NOP);
        RETURN_IF_ERROR(basicblock_append_instructions(bb, target));
        if (no_lineno_no_fallthrough) {
            last = basicblock_last_instr(bb);
            if (IS_UNCONDITIONAL_JUMP_OPCODE(last->i_opcode) &&
                removed_jump_opcode == JUMP)
            {
                /* Make sure we don't lose eval breaker checks */
                last->i_opcode = JUMP;
            }
        }
        target->b_predecessors--;
        return 1;
    }
    return 0;
}

static int
inline_small_or_no_lineno_blocks(basicblock *entryblock) {
    bool changes;
    do {
        changes = false;
        for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
            int res = basicblock_inline_small_or_no_lineno_blocks(b);
            RETURN_IF_ERROR(res);
            if (res) {
                changes = true;
            }
        }
    } while(changes); /* every change removes a jump, ensuring convergence */
    return changes;
}

// Attempt to eliminate jumps to jumps by updating inst to jump to
// target->i_target using the provided opcode. Return whether or not the
// optimization was successful.
static bool
jump_thread(basicblock *bb, cfg_instr *inst, cfg_instr *target, int opcode)
{
    assert(is_jump(inst));
    assert(is_jump(target));
    assert(inst == basicblock_last_instr(bb));
    // bpo-45773: If inst->i_target == target->i_target, then nothing actually
    // changes (and we fall into an infinite loop):
    if (inst->i_target != target->i_target) {
        /* Change inst to NOP and append a jump to target->i_target. The
         * NOP will be removed later if it's not needed for the lineno.
         */
        INSTR_SET_OP0(inst, NOP);

        RETURN_IF_ERROR(
            basicblock_add_jump(
                bb, opcode, target->i_target, target->i_loc));

        return true;
    }
    return false;
}

static PyObject*
get_const_value(int opcode, int oparg, PyObject *co_consts)
{
    PyObject *constant = NULL;
    assert(OPCODE_HAS_CONST(opcode));
    if (opcode == LOAD_CONST) {
        constant = PyList_GET_ITEM(co_consts, oparg);
    }

    if (constant == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "Internal error: failed to get value of a constant");
        return NULL;
    }
    return Py_NewRef(constant);
}

// Steals a reference to newconst.
static int
add_const(PyObject *newconst, PyObject *consts, PyObject *const_cache)
{
    if (_PyCompile_ConstCacheMergeOne(const_cache, &newconst) < 0) {
        Py_DECREF(newconst);
        return -1;
    }

    Py_ssize_t index;
    for (index = 0; index < PyList_GET_SIZE(consts); index++) {
        if (PyList_GET_ITEM(consts, index) == newconst) {
            break;
        }
    }
    if (index == PyList_GET_SIZE(consts)) {
        if ((size_t)index >= (size_t)INT_MAX - 1) {
            PyErr_SetString(PyExc_OverflowError, "too many constants");
            Py_DECREF(newconst);
            return -1;
        }
        if (PyList_Append(consts, newconst)) {
            Py_DECREF(newconst);
            return -1;
        }
    }
    Py_DECREF(newconst);
    return (int)index;
}

/* Replace LOAD_CONST c1, LOAD_CONST c2 ... LOAD_CONST cn, BUILD_TUPLE n
   with    LOAD_CONST (c1, c2, ... cn).
   The consts table must still be in list form so that the
   new constant (c1, c2, ... cn) can be appended.
   Called with codestr pointing to the first LOAD_CONST.
*/
static int
fold_tuple_on_constants(PyObject *const_cache,
                        cfg_instr *inst,
                        int n, PyObject *consts)
{
    /* Pre-conditions */
    assert(PyDict_CheckExact(const_cache));
    assert(PyList_CheckExact(consts));
    assert(inst[n].i_opcode == BUILD_TUPLE);
    assert(inst[n].i_oparg == n);

    for (int i = 0; i < n; i++) {
        if (!OPCODE_HAS_CONST(inst[i].i_opcode)) {
            return SUCCESS;
        }
    }

    /* Buildup new tuple of constants */
    PyObject *newconst = PyTuple_New(n);
    if (newconst == NULL) {
        return ERROR;
    }
    for (int i = 0; i < n; i++) {
        int op = inst[i].i_opcode;
        int arg = inst[i].i_oparg;
        PyObject *constant = get_const_value(op, arg, consts);
        if (constant == NULL) {
            return ERROR;
        }
        PyTuple_SET_ITEM(newconst, i, constant);
    }
    int index = add_const(newconst, consts, const_cache);
    if (index < 0) {
        return ERROR;
    }
    for (int i = 0; i < n; i++) {
        INSTR_SET_OP0(&inst[i], NOP);
    }
    INSTR_SET_OP1(&inst[n], LOAD_CONST, index);
    return SUCCESS;
}

#define VISITED (-1)

// Replace an arbitrary run of SWAPs and NOPs with an optimal one that has the
// same effect.
static int
swaptimize(basicblock *block, int *ix)
{
    // NOTE: "./python -m test test_patma" serves as a good, quick stress test
    // for this function. Make sure to blow away cached *.pyc files first!
    assert(*ix < block->b_iused);
    cfg_instr *instructions = &block->b_instr[*ix];
    // Find the length of the current sequence of SWAPs and NOPs, and record the
    // maximum depth of the stack manipulations:
    assert(instructions[0].i_opcode == SWAP);
    int depth = instructions[0].i_oparg;
    int len = 0;
    int more = false;
    int limit = block->b_iused - *ix;
    while (++len < limit) {
        int opcode = instructions[len].i_opcode;
        if (opcode == SWAP) {
            depth = Py_MAX(depth, instructions[len].i_oparg);
            more = true;
        }
        else if (opcode != NOP) {
            break;
        }
    }
    // It's already optimal if there's only one SWAP:
    if (!more) {
        return SUCCESS;
    }
    // Create an array with elements {0, 1, 2, ..., depth - 1}:
    int *stack = PyMem_Malloc(depth * sizeof(int));
    if (stack == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    for (int i = 0; i < depth; i++) {
        stack[i] = i;
    }
    // Simulate the combined effect of these instructions by "running" them on
    // our "stack":
    for (int i = 0; i < len; i++) {
        if (instructions[i].i_opcode == SWAP) {
            int oparg = instructions[i].i_oparg;
            int top = stack[0];
            // SWAPs are 1-indexed:
            stack[0] = stack[oparg - 1];
            stack[oparg - 1] = top;
        }
    }
    // Now we can begin! Our approach here is based on a solution to a closely
    // related problem (https://cs.stackexchange.com/a/13938). It's easiest to
    // think of this algorithm as determining the steps needed to efficiently
    // "un-shuffle" our stack. By performing the moves in *reverse* order,
    // though, we can efficiently *shuffle* it! For this reason, we will be
    // replacing instructions starting from the *end* of the run. Since the
    // solution is optimal, we don't need to worry about running out of space:
    int current = len - 1;
    for (int i = 0; i < depth; i++) {
        // Skip items that have already been visited, or just happen to be in
        // the correct location:
        if (stack[i] == VISITED || stack[i] == i) {
            continue;
        }
        // Okay, we've found an item that hasn't been visited. It forms a cycle
        // with other items; traversing the cycle and swapping each item with
        // the next will put them all in the correct place. The weird
        // loop-and-a-half is necessary to insert 0 into every cycle, since we
        // can only swap from that position:
        int j = i;
        while (true) {
            // Skip the actual swap if our item is zero, since swapping the top
            // item with itself is pointless:
            if (j) {
                assert(0 <= current);
                // SWAPs are 1-indexed:
                instructions[current].i_opcode = SWAP;
                instructions[current--].i_oparg = j + 1;
            }
            if (stack[j] == VISITED) {
                // Completed the cycle:
                assert(j == i);
                break;
            }
            int next_j = stack[j];
            stack[j] = VISITED;
            j = next_j;
        }
    }
    // NOP out any unused instructions:
    while (0 <= current) {
        INSTR_SET_OP0(&instructions[current--], NOP);
    }
    PyMem_Free(stack);
    *ix += len - 1;
    return SUCCESS;
}


// This list is pretty small, since it's only okay to reorder opcodes that:
// - can't affect control flow (like jumping or raising exceptions)
// - can't invoke arbitrary code (besides finalizers)
// - only touch the TOS (and pop it when finished)
#define SWAPPABLE(opcode) \
    ((opcode) == STORE_FAST || \
     (opcode) == STORE_FAST_MAYBE_NULL || \
     (opcode) == POP_TOP)

#define STORES_TO(instr) \
    (((instr).i_opcode == STORE_FAST || \
      (instr).i_opcode == STORE_FAST_MAYBE_NULL) \
     ? (instr).i_oparg : -1)

static int
next_swappable_instruction(basicblock *block, int i, int lineno)
{
    while (++i < block->b_iused) {
        cfg_instr *instruction = &block->b_instr[i];
        if (0 <= lineno && instruction->i_loc.lineno != lineno) {
            // Optimizing across this instruction could cause user-visible
            // changes in the names bound between line tracing events!
            return -1;
        }
        if (instruction->i_opcode == NOP) {
            continue;
        }
        if (SWAPPABLE(instruction->i_opcode)) {
            return i;
        }
        return -1;
    }
    return -1;
}

// Attempt to apply SWAPs statically by swapping *instructions* rather than
// stack items. For example, we can replace SWAP(2), POP_TOP, STORE_FAST(42)
// with the more efficient NOP, STORE_FAST(42), POP_TOP.
static void
apply_static_swaps(basicblock *block, int i)
{
    // SWAPs are to our left, and potential swaperands are to our right:
    for (; 0 <= i; i--) {
        assert(i < block->b_iused);
        cfg_instr *swap = &block->b_instr[i];
        if (swap->i_opcode != SWAP) {
            if (swap->i_opcode == NOP || SWAPPABLE(swap->i_opcode)) {
                // Nope, but we know how to handle these. Keep looking:
                continue;
            }
            // We can't reason about what this instruction does. Bail:
            return;
        }
        int j = next_swappable_instruction(block, i, -1);
        if (j < 0) {
            return;
        }
        int k = j;
        int lineno = block->b_instr[j].i_loc.lineno;
        for (int count = swap->i_oparg - 1; 0 < count; count--) {
            k = next_swappable_instruction(block, k, lineno);
            if (k < 0) {
                return;
            }
        }
        // The reordering is not safe if the two instructions to be swapped
        // store to the same location, or if any intervening instruction stores
        // to the same location as either of them.
        int store_j = STORES_TO(block->b_instr[j]);
        int store_k = STORES_TO(block->b_instr[k]);
        if (store_j >= 0 || store_k >= 0) {
            if (store_j == store_k) {
                return;
            }
            for (int idx = j + 1; idx < k; idx++) {
                int store_idx = STORES_TO(block->b_instr[idx]);
                if (store_idx >= 0 && (store_idx == store_j || store_idx == store_k)) {
                    return;
                }
            }
        }

        // Success!
        INSTR_SET_OP0(swap, NOP);
        cfg_instr temp = block->b_instr[j];
        block->b_instr[j] = block->b_instr[k];
        block->b_instr[k] = temp;
    }
}

static int
basicblock_optimize_load_const(PyObject *const_cache, basicblock *bb, PyObject *consts)
{
    assert(PyDict_CheckExact(const_cache));
    assert(PyList_CheckExact(consts));
    int opcode = 0;
    int oparg = 0;
    for (int i = 0; i < bb->b_iused; i++) {
        cfg_instr *inst = &bb->b_instr[i];
        bool is_copy_of_load_const = (opcode == LOAD_CONST &&
                                      inst->i_opcode == COPY &&
                                      inst->i_oparg == 1);
        if (! is_copy_of_load_const) {
            opcode = inst->i_opcode;
            oparg = inst->i_oparg;
        }
        assert(!IS_ASSEMBLER_OPCODE(opcode));
        if (opcode != LOAD_CONST) {
            continue;
        }
        int nextop = i+1 < bb->b_iused ? bb->b_instr[i+1].i_opcode : 0;
        switch(nextop) {
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            {
                /* Remove LOAD_CONST const; conditional jump */
                PyObject* cnt = get_const_value(opcode, oparg, consts);
                if (cnt == NULL) {
                    return ERROR;
                }
                int is_true = PyObject_IsTrue(cnt);
                Py_DECREF(cnt);
                if (is_true == -1) {
                    return ERROR;
                }
                INSTR_SET_OP0(inst, NOP);
                int jump_if_true = nextop == POP_JUMP_IF_TRUE;
                if (is_true == jump_if_true) {
                    bb->b_instr[i+1].i_opcode = JUMP;
                }
                else {
                    INSTR_SET_OP0(&bb->b_instr[i + 1], NOP);
                }
                break;
            }
            case IS_OP:
            {
                // Fold to POP_JUMP_IF_NONE:
                // - LOAD_CONST(None) IS_OP(0) POP_JUMP_IF_TRUE
                // - LOAD_CONST(None) IS_OP(1) POP_JUMP_IF_FALSE
                // - LOAD_CONST(None) IS_OP(0) TO_BOOL POP_JUMP_IF_TRUE
                // - LOAD_CONST(None) IS_OP(1) TO_BOOL POP_JUMP_IF_FALSE
                // Fold to POP_JUMP_IF_NOT_NONE:
                // - LOAD_CONST(None) IS_OP(0) POP_JUMP_IF_FALSE
                // - LOAD_CONST(None) IS_OP(1) POP_JUMP_IF_TRUE
                // - LOAD_CONST(None) IS_OP(0) TO_BOOL POP_JUMP_IF_FALSE
                // - LOAD_CONST(None) IS_OP(1) TO_BOOL POP_JUMP_IF_TRUE
                PyObject *cnt = get_const_value(opcode, oparg, consts);
                if (cnt == NULL) {
                    return ERROR;
                }
                if (!Py_IsNone(cnt)) {
                    Py_DECREF(cnt);
                    break;
                }
                if (bb->b_iused <= i + 2) {
                    break;
                }
                cfg_instr *is_instr = &bb->b_instr[i + 1];
                cfg_instr *jump_instr = &bb->b_instr[i + 2];
                // Get rid of TO_BOOL regardless:
                if (jump_instr->i_opcode == TO_BOOL) {
                    INSTR_SET_OP0(jump_instr, NOP);
                    if (bb->b_iused <= i + 3) {
                        break;
                    }
                    jump_instr = &bb->b_instr[i + 3];
                }
                bool invert = is_instr->i_oparg;
                if (jump_instr->i_opcode == POP_JUMP_IF_FALSE) {
                    invert = !invert;
                }
                else if (jump_instr->i_opcode != POP_JUMP_IF_TRUE) {
                    break;
                }
                INSTR_SET_OP0(inst, NOP);
                INSTR_SET_OP0(is_instr, NOP);
                jump_instr->i_opcode = invert ? POP_JUMP_IF_NOT_NONE
                                              : POP_JUMP_IF_NONE;
                break;
            }
            case RETURN_VALUE:
            {
                INSTR_SET_OP0(inst, NOP);
                INSTR_SET_OP1(&bb->b_instr[++i], RETURN_CONST, oparg);
                break;
            }
            case TO_BOOL:
            {
                PyObject *cnt = get_const_value(opcode, oparg, consts);
                if (cnt == NULL) {
                    return ERROR;
                }
                int is_true = PyObject_IsTrue(cnt);
                Py_DECREF(cnt);
                if (is_true == -1) {
                    return ERROR;
                }
                cnt = PyBool_FromLong(is_true);
                int index = add_const(cnt, consts, const_cache);
                if (index < 0) {
                    return ERROR;
                }
                INSTR_SET_OP0(inst, NOP);
                INSTR_SET_OP1(&bb->b_instr[i + 1], LOAD_CONST, index);
                break;
            }
        }
    }
    return SUCCESS;
}

static int
optimize_load_const(PyObject *const_cache, cfg_builder *g, PyObject *consts) {
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        RETURN_IF_ERROR(basicblock_optimize_load_const(const_cache, b, consts));
    }
    return SUCCESS;
}

static int
optimize_basic_block(PyObject *const_cache, basicblock *bb, PyObject *consts)
{
    assert(PyDict_CheckExact(const_cache));
    assert(PyList_CheckExact(consts));
    cfg_instr nop;
    INSTR_SET_OP0(&nop, NOP);
    for (int i = 0; i < bb->b_iused; i++) {
        cfg_instr *inst = &bb->b_instr[i];
        cfg_instr *target;
        int opcode = inst->i_opcode;
        int oparg = inst->i_oparg;
        if (HAS_TARGET(opcode)) {
            assert(inst->i_target->b_iused > 0);
            target = &inst->i_target->b_instr[0];
            assert(!IS_ASSEMBLER_OPCODE(target->i_opcode));
        }
        else {
            target = &nop;
        }
        int nextop = i+1 < bb->b_iused ? bb->b_instr[i+1].i_opcode : 0;
        assert(!IS_ASSEMBLER_OPCODE(opcode));
        switch (opcode) {
            /* Try to fold tuples of constants.
               Skip over BUILD_TUPLE(1) UNPACK_SEQUENCE(1).
               Replace BUILD_TUPLE(2) UNPACK_SEQUENCE(2) with SWAP(2).
               Replace BUILD_TUPLE(3) UNPACK_SEQUENCE(3) with SWAP(3). */
            case BUILD_TUPLE:
                if (nextop == UNPACK_SEQUENCE && oparg == bb->b_instr[i+1].i_oparg) {
                    switch(oparg) {
                        case 1:
                            INSTR_SET_OP0(inst, NOP);
                            INSTR_SET_OP0(&bb->b_instr[i + 1], NOP);
                            continue;
                        case 2:
                        case 3:
                            INSTR_SET_OP0(inst, NOP);
                            bb->b_instr[i+1].i_opcode = SWAP;
                            continue;
                    }
                }
                if (i >= oparg) {
                    if (fold_tuple_on_constants(const_cache, inst-oparg, oparg, consts)) {
                        goto error;
                    }
                }
                break;
            case POP_JUMP_IF_NOT_NONE:
            case POP_JUMP_IF_NONE:
                switch (target->i_opcode) {
                    case JUMP:
                        i -= jump_thread(bb, inst, target, inst->i_opcode);
                }
                break;
            case POP_JUMP_IF_FALSE:
                switch (target->i_opcode) {
                    case JUMP:
                        i -= jump_thread(bb, inst, target, POP_JUMP_IF_FALSE);
                }
                break;
            case POP_JUMP_IF_TRUE:
                switch (target->i_opcode) {
                    case JUMP:
                        i -= jump_thread(bb, inst, target, POP_JUMP_IF_TRUE);
                }
                break;
            case JUMP:
            case JUMP_NO_INTERRUPT:
                switch (target->i_opcode) {
                    case JUMP:
                        i -= jump_thread(bb, inst, target, JUMP);
                        continue;
                    case JUMP_NO_INTERRUPT:
                        i -= jump_thread(bb, inst, target, opcode);
                        continue;
                }
                break;
            case FOR_ITER:
                if (target->i_opcode == JUMP) {
                    /* This will not work now because the jump (at target) could
                     * be forward or backward and FOR_ITER only jumps forward. We
                     * can re-enable this if ever we implement a backward version
                     * of FOR_ITER.
                     */
                    /*
                    i -= jump_thread(bb, inst, target, FOR_ITER);
                    */
                }
                break;
            case STORE_FAST:
                if (opcode == nextop &&
                    oparg == bb->b_instr[i+1].i_oparg &&
                    bb->b_instr[i].i_loc.lineno == bb->b_instr[i+1].i_loc.lineno) {
                    bb->b_instr[i].i_opcode = POP_TOP;
                    bb->b_instr[i].i_oparg = 0;
                }
                break;
            case SWAP:
                if (oparg == 1) {
                    INSTR_SET_OP0(inst, NOP);
                }
                break;
            case LOAD_GLOBAL:
                if (nextop == PUSH_NULL && (oparg & 1) == 0) {
                    INSTR_SET_OP1(inst, LOAD_GLOBAL, oparg | 1);
                    INSTR_SET_OP0(&bb->b_instr[i + 1], NOP);
                }
                break;
            case COMPARE_OP:
                if (nextop == TO_BOOL) {
                    INSTR_SET_OP0(inst, NOP);
                    INSTR_SET_OP1(&bb->b_instr[i + 1], COMPARE_OP, oparg | 16);
                    continue;
                }
                break;
            case CONTAINS_OP:
            case IS_OP:
                if (nextop == TO_BOOL) {
                    INSTR_SET_OP0(inst, NOP);
                    INSTR_SET_OP1(&bb->b_instr[i + 1], opcode, oparg);
                    continue;
                }
                break;
            case TO_BOOL:
                if (nextop == TO_BOOL) {
                    INSTR_SET_OP0(inst, NOP);
                    continue;
                }
                break;
            case UNARY_NOT:
                if (nextop == TO_BOOL) {
                    INSTR_SET_OP0(inst, NOP);
                    INSTR_SET_OP0(&bb->b_instr[i + 1], UNARY_NOT);
                    continue;
                }
                if (nextop == UNARY_NOT) {
                    INSTR_SET_OP0(inst, NOP);
                    INSTR_SET_OP0(&bb->b_instr[i + 1], NOP);
                    continue;
                }
                break;
        }
    }

    for (int i = 0; i < bb->b_iused; i++) {
        cfg_instr *inst = &bb->b_instr[i];
        if (inst->i_opcode == SWAP) {
            if (swaptimize(bb, &i) < 0) {
                goto error;
            }
            apply_static_swaps(bb, i);
        }
    }
    return SUCCESS;
error:
    return ERROR;
}

static int resolve_line_numbers(cfg_builder *g, int firstlineno);

static int
remove_redundant_nops_and_jumps(cfg_builder *g)
{
    int removed_nops, removed_jumps;
    do {
        /* Convergence is guaranteed because the number of
         * redundant jumps and nops only decreases.
         */
        removed_nops = remove_redundant_nops(g);
        RETURN_IF_ERROR(removed_nops);
        removed_jumps = remove_redundant_jumps(g);
        RETURN_IF_ERROR(removed_jumps);
    } while(removed_nops + removed_jumps > 0);
    return SUCCESS;
}

/* Perform optimizations on a control flow graph.
   The consts object should still be in list form to allow new constants
   to be appended.

   Code trasnformations that reduce code size initially fill the gaps with
   NOPs.  Later those NOPs are removed.
*/
static int
optimize_cfg(cfg_builder *g, PyObject *consts, PyObject *const_cache, int firstlineno)
{
    assert(PyDict_CheckExact(const_cache));
    RETURN_IF_ERROR(check_cfg(g));
    RETURN_IF_ERROR(inline_small_or_no_lineno_blocks(g->g_entryblock));
    RETURN_IF_ERROR(remove_unreachable(g->g_entryblock));
    RETURN_IF_ERROR(resolve_line_numbers(g, firstlineno));
    RETURN_IF_ERROR(optimize_load_const(const_cache, g, consts));
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        RETURN_IF_ERROR(optimize_basic_block(const_cache, b, consts));
    }
    RETURN_IF_ERROR(remove_redundant_nops_and_pairs(g->g_entryblock));
    RETURN_IF_ERROR(remove_unreachable(g->g_entryblock));
    RETURN_IF_ERROR(remove_redundant_nops_and_jumps(g));
    assert(no_redundant_jumps(g));
    return SUCCESS;
}

static void
make_super_instruction(cfg_instr *inst1, cfg_instr *inst2, int super_op)
{
    int32_t line1 = inst1->i_loc.lineno;
    int32_t line2 = inst2->i_loc.lineno;
    /* Skip if instructions are on different lines */
    if (line1 >= 0 && line2 >= 0 && line1 != line2) {
        return;
    }
    if (inst1->i_oparg >= 16 || inst2->i_oparg >= 16) {
        return;
    }
    INSTR_SET_OP1(inst1, super_op, (inst1->i_oparg << 4) | inst2->i_oparg);
    INSTR_SET_OP0(inst2, NOP);
}

static int
insert_superinstructions(cfg_builder *g)
{
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {

        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *inst = &b->b_instr[i];
            int nextop = i+1 < b->b_iused ? b->b_instr[i+1].i_opcode : 0;
            switch(inst->i_opcode) {
                case LOAD_FAST:
                    if (nextop == LOAD_FAST) {
                        make_super_instruction(inst, &b->b_instr[i + 1], LOAD_FAST_LOAD_FAST);
                    }
                    break;
                case STORE_FAST:
                    switch (nextop) {
                        case LOAD_FAST:
                            make_super_instruction(inst, &b->b_instr[i + 1], STORE_FAST_LOAD_FAST);
                            break;
                        case STORE_FAST:
                            make_super_instruction(inst, &b->b_instr[i + 1], STORE_FAST_STORE_FAST);
                            break;
                    }
                    break;
            }
        }
    }
    int res = remove_redundant_nops(g);
    assert(no_redundant_nops(g));
    return res;
}

// helper functions for add_checks_for_loads_of_unknown_variables
static inline void
maybe_push(basicblock *b, uint64_t unsafe_mask, basicblock ***sp)
{
    // Push b if the unsafe mask is giving us any new information.
    // To avoid overflowing the stack, only allow each block once.
    // Use b->b_visited=1 to mean that b is currently on the stack.
    uint64_t both = b->b_unsafe_locals_mask | unsafe_mask;
    if (b->b_unsafe_locals_mask != both) {
        b->b_unsafe_locals_mask = both;
        // More work left to do.
        if (!b->b_visited) {
            // not on the stack, so push it.
            *(*sp)++ = b;
            b->b_visited = 1;
        }
    }
}

static void
scan_block_for_locals(basicblock *b, basicblock ***sp)
{
    // bit i is set if local i is potentially uninitialized
    uint64_t unsafe_mask = b->b_unsafe_locals_mask;
    for (int i = 0; i < b->b_iused; i++) {
        cfg_instr *instr = &b->b_instr[i];
        assert(instr->i_opcode != EXTENDED_ARG);
        if (instr->i_except != NULL) {
            maybe_push(instr->i_except, unsafe_mask, sp);
        }
        if (instr->i_oparg >= 64) {
            continue;
        }
        assert(instr->i_oparg >= 0);
        uint64_t bit = (uint64_t)1 << instr->i_oparg;
        switch (instr->i_opcode) {
            case DELETE_FAST:
            case LOAD_FAST_AND_CLEAR:
            case STORE_FAST_MAYBE_NULL:
                unsafe_mask |= bit;
                break;
            case STORE_FAST:
                unsafe_mask &= ~bit;
                break;
            case LOAD_FAST_CHECK:
                // If this doesn't raise, then the local is defined.
                unsafe_mask &= ~bit;
                break;
            case LOAD_FAST:
                if (unsafe_mask & bit) {
                    instr->i_opcode = LOAD_FAST_CHECK;
                }
                unsafe_mask &= ~bit;
                break;
        }
    }
    if (b->b_next && BB_HAS_FALLTHROUGH(b)) {
        maybe_push(b->b_next, unsafe_mask, sp);
    }
    cfg_instr *last = basicblock_last_instr(b);
    if (last && is_jump(last)) {
        assert(last->i_target != NULL);
        maybe_push(last->i_target, unsafe_mask, sp);
    }
}

static int
fast_scan_many_locals(basicblock *entryblock, int nlocals)
{
    assert(nlocals > 64);
    Py_ssize_t *states = PyMem_Calloc(nlocals - 64, sizeof(Py_ssize_t));
    if (states == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    Py_ssize_t blocknum = 0;
    // state[i - 64] == blocknum if local i is guaranteed to
    // be initialized, i.e., if it has had a previous LOAD_FAST or
    // STORE_FAST within that basicblock (not followed by
    // DELETE_FAST/LOAD_FAST_AND_CLEAR/STORE_FAST_MAYBE_NULL).
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        blocknum++;
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            assert(instr->i_opcode != EXTENDED_ARG);
            int arg = instr->i_oparg;
            if (arg < 64) {
                continue;
            }
            assert(arg >= 0);
            switch (instr->i_opcode) {
                case DELETE_FAST:
                case LOAD_FAST_AND_CLEAR:
                case STORE_FAST_MAYBE_NULL:
                    states[arg - 64] = blocknum - 1;
                    break;
                case STORE_FAST:
                    states[arg - 64] = blocknum;
                    break;
                case LOAD_FAST:
                    if (states[arg - 64] != blocknum) {
                        instr->i_opcode = LOAD_FAST_CHECK;
                    }
                    states[arg - 64] = blocknum;
                    break;
                    Py_UNREACHABLE();
            }
        }
    }
    PyMem_Free(states);
    return SUCCESS;
}

static int
remove_unused_consts(basicblock *entryblock, PyObject *consts)
{
    assert(PyList_CheckExact(consts));
    Py_ssize_t nconsts = PyList_GET_SIZE(consts);
    if (nconsts == 0) {
        return SUCCESS;  /* nothing to do */
    }

    Py_ssize_t *index_map = NULL;
    Py_ssize_t *reverse_index_map = NULL;
    int err = ERROR;

    index_map = PyMem_Malloc(nconsts * sizeof(Py_ssize_t));
    if (index_map == NULL) {
        goto end;
    }
    for (Py_ssize_t i = 1; i < nconsts; i++) {
        index_map[i] = -1;
    }
    // The first constant may be docstring; keep it always.
    index_map[0] = 0;

    /* mark used consts */
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        for (int i = 0; i < b->b_iused; i++) {
            if (OPCODE_HAS_CONST(b->b_instr[i].i_opcode)) {
                int index = b->b_instr[i].i_oparg;
                index_map[index] = index;
            }
        }
    }
    /* now index_map[i] == i if consts[i] is used, -1 otherwise */
    /* condense consts */
    Py_ssize_t n_used_consts = 0;
    for (int i = 0; i < nconsts; i++) {
        if (index_map[i] != -1) {
            assert(index_map[i] == i);
            index_map[n_used_consts++] = index_map[i];
        }
    }
    if (n_used_consts == nconsts) {
        /* nothing to do */
        err = SUCCESS;
        goto end;
    }

    /* move all used consts to the beginning of the consts list */
    assert(n_used_consts < nconsts);
    for (Py_ssize_t i = 0; i < n_used_consts; i++) {
        Py_ssize_t old_index = index_map[i];
        assert(i <= old_index && old_index < nconsts);
        if (i != old_index) {
            PyObject *value = PyList_GET_ITEM(consts, index_map[i]);
            assert(value != NULL);
            PyList_SetItem(consts, i, Py_NewRef(value));
        }
    }

    /* truncate the consts list at its new size */
    if (PyList_SetSlice(consts, n_used_consts, nconsts, NULL) < 0) {
        goto end;
    }
    /* adjust const indices in the bytecode */
    reverse_index_map = PyMem_Malloc(nconsts * sizeof(Py_ssize_t));
    if (reverse_index_map == NULL) {
        goto end;
    }
    for (Py_ssize_t i = 0; i < nconsts; i++) {
        reverse_index_map[i] = -1;
    }
    for (Py_ssize_t i = 0; i < n_used_consts; i++) {
        assert(index_map[i] != -1);
        assert(reverse_index_map[index_map[i]] == -1);
        reverse_index_map[index_map[i]] = i;
    }

    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        for (int i = 0; i < b->b_iused; i++) {
            if (OPCODE_HAS_CONST(b->b_instr[i].i_opcode)) {
                int index = b->b_instr[i].i_oparg;
                assert(reverse_index_map[index] >= 0);
                assert(reverse_index_map[index] < n_used_consts);
                b->b_instr[i].i_oparg = (int)reverse_index_map[index];
            }
        }
    }

    err = SUCCESS;
end:
    PyMem_Free(index_map);
    PyMem_Free(reverse_index_map);
    return err;
}



static int
add_checks_for_loads_of_uninitialized_variables(basicblock *entryblock,
                                                int nlocals,
                                                int nparams)
{
    if (nlocals == 0) {
        return SUCCESS;
    }
    if (nlocals > 64) {
        // To avoid O(nlocals**2) compilation, locals beyond the first
        // 64 are only analyzed one basicblock at a time: initialization
        // info is not passed between basicblocks.
        if (fast_scan_many_locals(entryblock, nlocals) < 0) {
            return ERROR;
        }
        nlocals = 64;
    }
    basicblock **stack = make_cfg_traversal_stack(entryblock);
    if (stack == NULL) {
        return ERROR;
    }
    basicblock **sp = stack;

    // First origin of being uninitialized:
    // The non-parameter locals in the entry block.
    uint64_t start_mask = 0;
    for (int i = nparams; i < nlocals; i++) {
        start_mask |= (uint64_t)1 << i;
    }
    maybe_push(entryblock, start_mask, &sp);

    // Second origin of being uninitialized:
    // There could be DELETE_FAST somewhere, so
    // be sure to scan each basicblock at least once.
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        scan_block_for_locals(b, &sp);
    }
    // Now propagate the uncertainty from the origins we found: Use
    // LOAD_FAST_CHECK for any LOAD_FAST where the local could be undefined.
    while (sp > stack) {
        basicblock *b = *--sp;
        // mark as no longer on stack
        b->b_visited = 0;
        scan_block_for_locals(b, &sp);
    }
    PyMem_Free(stack);
    return SUCCESS;
}


static int
mark_warm(basicblock *entryblock) {
    basicblock **stack = make_cfg_traversal_stack(entryblock);
    if (stack == NULL) {
        return ERROR;
    }
    basicblock **sp = stack;

    *sp++ = entryblock;
    entryblock->b_visited = 1;
    while (sp > stack) {
        basicblock *b = *(--sp);
        assert(!b->b_except_handler);
        b->b_warm = 1;
        basicblock *next = b->b_next;
        if (next && BB_HAS_FALLTHROUGH(b) && !next->b_visited) {
            *sp++ = next;
            next->b_visited = 1;
        }
        for (int i=0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            if (is_jump(instr) && !instr->i_target->b_visited) {
                *sp++ = instr->i_target;
                instr->i_target->b_visited = 1;
            }
        }
    }
    PyMem_Free(stack);
    return SUCCESS;
}

static int
mark_cold(basicblock *entryblock) {
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        assert(!b->b_cold && !b->b_warm);
    }
    if (mark_warm(entryblock) < 0) {
        return ERROR;
    }

    basicblock **stack = make_cfg_traversal_stack(entryblock);
    if (stack == NULL) {
        return ERROR;
    }

    basicblock **sp = stack;
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        if (b->b_except_handler) {
            assert(!b->b_warm);
            *sp++ = b;
            b->b_visited = 1;
        }
    }

    while (sp > stack) {
        basicblock *b = *(--sp);
        b->b_cold = 1;
        basicblock *next = b->b_next;
        if (next && BB_HAS_FALLTHROUGH(b)) {
            if (!next->b_warm && !next->b_visited) {
                *sp++ = next;
                next->b_visited = 1;
            }
        }
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            if (is_jump(instr)) {
                assert(i == b->b_iused - 1);
                basicblock *target = b->b_instr[i].i_target;
                if (!target->b_warm && !target->b_visited) {
                    *sp++ = target;
                    target->b_visited = 1;
                }
            }
        }
    }
    PyMem_Free(stack);
    return SUCCESS;
}


static int
push_cold_blocks_to_end(cfg_builder *g) {
    basicblock *entryblock = g->g_entryblock;
    if (entryblock->b_next == NULL) {
        /* single basicblock, no need to reorder */
        return SUCCESS;
    }
    RETURN_IF_ERROR(mark_cold(entryblock));

    int next_lbl = get_max_label(g->g_entryblock) + 1;

    /* If we have a cold block with fallthrough to a warm block, add */
    /* an explicit jump instead of fallthrough */
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        if (b->b_cold && BB_HAS_FALLTHROUGH(b) && b->b_next && b->b_next->b_warm) {
            basicblock *explicit_jump = cfg_builder_new_block(g);
            if (explicit_jump == NULL) {
                return ERROR;
            }
            if (!IS_LABEL(b->b_next->b_label)) {
                b->b_next->b_label.id = next_lbl++;
            }
            basicblock_addop(explicit_jump, JUMP_NO_INTERRUPT, b->b_next->b_label.id,
                             NO_LOCATION);
            explicit_jump->b_cold = 1;
            explicit_jump->b_next = b->b_next;
            explicit_jump->b_predecessors = 1;
            b->b_next = explicit_jump;

            /* set target */
            cfg_instr *last = basicblock_last_instr(explicit_jump);
            last->i_target = explicit_jump->b_next;
        }
    }

    assert(!entryblock->b_cold);  /* First block can't be cold */
    basicblock *cold_blocks = NULL;
    basicblock *cold_blocks_tail = NULL;

    basicblock *b = entryblock;
    while(b->b_next) {
        assert(!b->b_cold);
        while (b->b_next && !b->b_next->b_cold) {
            b = b->b_next;
        }
        if (b->b_next == NULL) {
            /* no more cold blocks */
            break;
        }

        /* b->b_next is the beginning of a cold streak */
        assert(!b->b_cold && b->b_next->b_cold);

        basicblock *b_end = b->b_next;
        while (b_end->b_next && b_end->b_next->b_cold) {
            b_end = b_end->b_next;
        }

        /* b_end is the end of the cold streak */
        assert(b_end && b_end->b_cold);
        assert(b_end->b_next == NULL || !b_end->b_next->b_cold);

        if (cold_blocks == NULL) {
            cold_blocks = b->b_next;
        }
        else {
            cold_blocks_tail->b_next = b->b_next;
        }
        cold_blocks_tail = b_end;
        b->b_next = b_end->b_next;
        b_end->b_next = NULL;
    }
    assert(b != NULL && b->b_next == NULL);
    b->b_next = cold_blocks;

    if (cold_blocks != NULL) {
        RETURN_IF_ERROR(remove_redundant_nops_and_jumps(g));
    }
    return SUCCESS;
}

static int
convert_pseudo_ops(cfg_builder *g)
{
    basicblock *entryblock = g->g_entryblock;
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            if (is_block_push(instr)) {
                INSTR_SET_OP0(instr, NOP);
            }
            else if (instr->i_opcode == LOAD_CLOSURE) {
                assert(is_pseudo_target(LOAD_CLOSURE, LOAD_FAST));
                instr->i_opcode = LOAD_FAST;
            }
            else if (instr->i_opcode == STORE_FAST_MAYBE_NULL) {
                assert(is_pseudo_target(STORE_FAST_MAYBE_NULL, STORE_FAST));
                instr->i_opcode = STORE_FAST;
            }
        }
    }
    return remove_redundant_nops_and_jumps(g);
}

static inline bool
is_exit_or_eval_check_without_lineno(basicblock *b) {
    if (basicblock_exits_scope(b) || basicblock_has_eval_break(b)) {
        return basicblock_has_no_lineno(b);
    }
    else {
        return false;
    }
}


/* PEP 626 mandates that the f_lineno of a frame is correct
 * after a frame terminates. It would be prohibitively expensive
 * to continuously update the f_lineno field at runtime,
 * so we make sure that all exiting instruction (raises and returns)
 * have a valid line number, allowing us to compute f_lineno lazily.
 * We can do this by duplicating the exit blocks without line number
 * so that none have more than one predecessor. We can then safely
 * copy the line number from the sole predecessor block.
 */
static int
duplicate_exits_without_lineno(cfg_builder *g)
{
    int next_lbl = get_max_label(g->g_entryblock) + 1;

    /* Copy all exit blocks without line number that are targets of a jump.
     */
    basicblock *entryblock = g->g_entryblock;
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        cfg_instr *last = basicblock_last_instr(b);
        if (last == NULL) {
            continue;
        }
        if (is_jump(last)) {
            basicblock *target = next_nonempty_block(last->i_target);
            if (is_exit_or_eval_check_without_lineno(target) && target->b_predecessors > 1) {
                basicblock *new_target = copy_basicblock(g, target);
                if (new_target == NULL) {
                    return ERROR;
                }
                new_target->b_instr[0].i_loc = last->i_loc;
                last->i_target = new_target;
                target->b_predecessors--;
                new_target->b_predecessors = 1;
                new_target->b_next = target->b_next;
                new_target->b_label.id = next_lbl++;
                target->b_next = new_target;
            }
        }
    }

    /* Any remaining reachable exit blocks without line number can only be reached by
     * fall through, and thus can only have a single predecessor */
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        if (BB_HAS_FALLTHROUGH(b) && b->b_next && b->b_iused > 0) {
            if (is_exit_or_eval_check_without_lineno(b->b_next)) {
                cfg_instr *last = basicblock_last_instr(b);
                assert(last != NULL);
                b->b_next->b_instr[0].i_loc = last->i_loc;
            }
        }
    }
    return SUCCESS;
}


/* If an instruction has no line number, but it's predecessor in the BB does,
 * then copy the line number. If a successor block has no line number, and only
 * one predecessor, then inherit the line number.
 * This ensures that all exit blocks (with one predecessor) receive a line number.
 * Also reduces the size of the line number table,
 * but has no impact on the generated line number events.
 */
static void
propagate_line_numbers(basicblock *entryblock) {
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        cfg_instr *last = basicblock_last_instr(b);
        if (last == NULL) {
            continue;
        }

        location prev_location = NO_LOCATION;
        for (int i = 0; i < b->b_iused; i++) {
            if (b->b_instr[i].i_loc.lineno < 0) {
                b->b_instr[i].i_loc = prev_location;
            }
            else {
                prev_location = b->b_instr[i].i_loc;
            }
        }
        if (BB_HAS_FALLTHROUGH(b) && b->b_next->b_predecessors == 1) {
            if (b->b_next->b_iused > 0) {
                if (b->b_next->b_instr[0].i_loc.lineno < 0) {
                    b->b_next->b_instr[0].i_loc = prev_location;
                }
            }
        }
        if (is_jump(last)) {
            basicblock *target = last->i_target;
            if (target->b_predecessors == 1) {
                if (target->b_instr[0].i_loc.lineno < 0) {
                    target->b_instr[0].i_loc = prev_location;
                }
            }
        }
    }
}

static int
resolve_line_numbers(cfg_builder *g, int firstlineno)
{
    RETURN_IF_ERROR(duplicate_exits_without_lineno(g));
    propagate_line_numbers(g->g_entryblock);
    return SUCCESS;
}

int
_PyCfg_OptimizeCodeUnit(cfg_builder *g, PyObject *consts, PyObject *const_cache,
                        int nlocals, int nparams, int firstlineno)
{
    assert(cfg_builder_check(g));
    /** Preprocessing **/
    /* Map labels to targets and mark exception handlers */
    RETURN_IF_ERROR(translate_jump_labels_to_targets(g->g_entryblock));
    RETURN_IF_ERROR(mark_except_handlers(g->g_entryblock));
    RETURN_IF_ERROR(label_exception_targets(g->g_entryblock));

    /** Optimization **/
    RETURN_IF_ERROR(optimize_cfg(g, consts, const_cache, firstlineno));
    RETURN_IF_ERROR(remove_unused_consts(g->g_entryblock, consts));
    RETURN_IF_ERROR(
        add_checks_for_loads_of_uninitialized_variables(
            g->g_entryblock, nlocals, nparams));
    RETURN_IF_ERROR(insert_superinstructions(g));

    RETURN_IF_ERROR(push_cold_blocks_to_end(g));
    RETURN_IF_ERROR(resolve_line_numbers(g, firstlineno));
    return SUCCESS;
}

static int *
build_cellfixedoffsets(_PyCompile_CodeUnitMetadata *umd)
{
    int nlocals = (int)PyDict_GET_SIZE(umd->u_varnames);
    int ncellvars = (int)PyDict_GET_SIZE(umd->u_cellvars);
    int nfreevars = (int)PyDict_GET_SIZE(umd->u_freevars);

    int noffsets = ncellvars + nfreevars;
    int *fixed = PyMem_New(int, noffsets);
    if (fixed == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    for (int i = 0; i < noffsets; i++) {
        fixed[i] = nlocals + i;
    }

    PyObject *varname, *cellindex;
    Py_ssize_t pos = 0;
    while (PyDict_Next(umd->u_cellvars, &pos, &varname, &cellindex)) {
        PyObject *varindex;
        if (PyDict_GetItemRef(umd->u_varnames, varname, &varindex) < 0) {
            goto error;
        }
        if (varindex == NULL) {
            continue;
        }

        int argoffset = PyLong_AsInt(varindex);
        Py_DECREF(varindex);
        if (argoffset == -1 && PyErr_Occurred()) {
            goto error;
        }

        int oldindex = PyLong_AsInt(cellindex);
        if (oldindex == -1 && PyErr_Occurred()) {
            goto error;
        }
        fixed[oldindex] = argoffset;
    }
    return fixed;

error:
    PyMem_Free(fixed);
    return NULL;
}

#define IS_GENERATOR(CF) \
    ((CF) & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR))

static int
insert_prefix_instructions(_PyCompile_CodeUnitMetadata *umd, basicblock *entryblock,
                           int *fixed, int nfreevars, int code_flags)
{
    assert(umd->u_firstlineno > 0);

    /* Add the generator prefix instructions. */
    if (IS_GENERATOR(code_flags)) {
        /* Note that RETURN_GENERATOR + POP_TOP have a net stack effect
         * of 0. This is because RETURN_GENERATOR pushes an element
         * with _PyFrame_StackPush before switching stacks.
         */

        location loc = LOCATION(umd->u_firstlineno, umd->u_firstlineno, -1, -1);
        cfg_instr make_gen = {
            .i_opcode = RETURN_GENERATOR,
            .i_oparg = 0,
            .i_loc = loc,
            .i_target = NULL,
        };
        RETURN_IF_ERROR(basicblock_insert_instruction(entryblock, 0, &make_gen));
        cfg_instr pop_top = {
            .i_opcode = POP_TOP,
            .i_oparg = 0,
            .i_loc = loc,
            .i_target = NULL,
        };
        RETURN_IF_ERROR(basicblock_insert_instruction(entryblock, 1, &pop_top));
    }

    /* Set up cells for any variable that escapes, to be put in a closure. */
    const int ncellvars = (int)PyDict_GET_SIZE(umd->u_cellvars);
    if (ncellvars) {
        // umd->u_cellvars has the cells out of order so we sort them
        // before adding the MAKE_CELL instructions.  Note that we
        // adjust for arg cells, which come first.
        const int nvars = ncellvars + (int)PyDict_GET_SIZE(umd->u_varnames);
        int *sorted = PyMem_RawCalloc(nvars, sizeof(int));
        if (sorted == NULL) {
            PyErr_NoMemory();
            return ERROR;
        }
        for (int i = 0; i < ncellvars; i++) {
            sorted[fixed[i]] = i + 1;
        }
        for (int i = 0, ncellsused = 0; ncellsused < ncellvars; i++) {
            int oldindex = sorted[i] - 1;
            if (oldindex == -1) {
                continue;
            }
            cfg_instr make_cell = {
                .i_opcode = MAKE_CELL,
                // This will get fixed in offset_derefs().
                .i_oparg = oldindex,
                .i_loc = NO_LOCATION,
                .i_target = NULL,
            };
            if (basicblock_insert_instruction(entryblock, ncellsused, &make_cell) < 0) {
                PyMem_RawFree(sorted);
                return ERROR;
            }
            ncellsused += 1;
        }
        PyMem_RawFree(sorted);
    }

    if (nfreevars) {
        cfg_instr copy_frees = {
            .i_opcode = COPY_FREE_VARS,
            .i_oparg = nfreevars,
            .i_loc = NO_LOCATION,
            .i_target = NULL,
        };
        RETURN_IF_ERROR(basicblock_insert_instruction(entryblock, 0, &copy_frees));
    }

    return SUCCESS;
}

static int
fix_cell_offsets(_PyCompile_CodeUnitMetadata *umd, basicblock *entryblock, int *fixedmap)
{
    int nlocals = (int)PyDict_GET_SIZE(umd->u_varnames);
    int ncellvars = (int)PyDict_GET_SIZE(umd->u_cellvars);
    int nfreevars = (int)PyDict_GET_SIZE(umd->u_freevars);
    int noffsets = ncellvars + nfreevars;

    // First deal with duplicates (arg cells).
    int numdropped = 0;
    for (int i = 0; i < noffsets ; i++) {
        if (fixedmap[i] == i + nlocals) {
            fixedmap[i] -= numdropped;
        }
        else {
            // It was a duplicate (cell/arg).
            numdropped += 1;
        }
    }

    // Then update offsets, either relative to locals or by cell2arg.
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *inst = &b->b_instr[i];
            // This is called before extended args are generated.
            assert(inst->i_opcode != EXTENDED_ARG);
            int oldoffset = inst->i_oparg;
            switch(inst->i_opcode) {
                case MAKE_CELL:
                case LOAD_CLOSURE:
                case LOAD_DEREF:
                case STORE_DEREF:
                case DELETE_DEREF:
                case LOAD_FROM_DICT_OR_DEREF:
                    assert(oldoffset >= 0);
                    assert(oldoffset < noffsets);
                    assert(fixedmap[oldoffset] >= 0);
                    inst->i_oparg = fixedmap[oldoffset];
            }
        }
    }

    return numdropped;
}

static int
prepare_localsplus(_PyCompile_CodeUnitMetadata *umd, cfg_builder *g, int code_flags)
{
    assert(PyDict_GET_SIZE(umd->u_varnames) < INT_MAX);
    assert(PyDict_GET_SIZE(umd->u_cellvars) < INT_MAX);
    assert(PyDict_GET_SIZE(umd->u_freevars) < INT_MAX);
    int nlocals = (int)PyDict_GET_SIZE(umd->u_varnames);
    int ncellvars = (int)PyDict_GET_SIZE(umd->u_cellvars);
    int nfreevars = (int)PyDict_GET_SIZE(umd->u_freevars);
    assert(INT_MAX - nlocals - ncellvars > 0);
    assert(INT_MAX - nlocals - ncellvars - nfreevars > 0);
    int nlocalsplus = nlocals + ncellvars + nfreevars;
    int* cellfixedoffsets = build_cellfixedoffsets(umd);
    if (cellfixedoffsets == NULL) {
        return ERROR;
    }

    // This must be called before fix_cell_offsets().
    if (insert_prefix_instructions(umd, g->g_entryblock, cellfixedoffsets, nfreevars, code_flags)) {
        PyMem_Free(cellfixedoffsets);
        return ERROR;
    }

    int numdropped = fix_cell_offsets(umd, g->g_entryblock, cellfixedoffsets);
    PyMem_Free(cellfixedoffsets);  // At this point we're done with it.
    cellfixedoffsets = NULL;
    if (numdropped < 0) {
        return ERROR;
    }

    nlocalsplus -= numdropped;
    return nlocalsplus;
}

int
_PyCfg_ToInstructionSequence(cfg_builder *g, _PyInstructionSequence *seq)
{
    int lbl = 0;
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        b->b_label = (jump_target_label){lbl};
        lbl += 1;
    }
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        RETURN_IF_ERROR(_PyInstructionSequence_UseLabel(seq, b->b_label.id));
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            if (HAS_TARGET(instr->i_opcode)) {
                /* Set oparg to the label id (it will later be mapped to an offset) */
                instr->i_oparg = instr->i_target->b_label.id;
            }
            RETURN_IF_ERROR(
                _PyInstructionSequence_Addop(
                    seq, instr->i_opcode, instr->i_oparg, instr->i_loc));

            _PyExceptHandlerInfo *hi = &seq->s_instrs[seq->s_used-1].i_except_handler_info;
            if (instr->i_except != NULL) {
                hi->h_label = instr->i_except->b_label.id;
                hi->h_startdepth = instr->i_except->b_startdepth;
                hi->h_preserve_lasti = instr->i_except->b_preserve_lasti;
            }
            else {
                hi->h_label = -1;
            }
        }
    }
    return SUCCESS;
}


int
_PyCfg_OptimizedCfgToInstructionSequence(cfg_builder *g,
                                     _PyCompile_CodeUnitMetadata *umd, int code_flags,
                                     int *stackdepth, int *nlocalsplus,
                                     _PyInstructionSequence *seq)
{
    *stackdepth = calculate_stackdepth(g);
    if (*stackdepth < 0) {
        return ERROR;
    }

    /* prepare_localsplus adds instructions for generators that push
     * and pop an item on the stack. This assertion makes sure there
     * is space on the stack for that.
     * It should always be true, because a generator must have at
     * least one expression or call to INTRINSIC_STOPITERATION_ERROR,
     * which requires stackspace.
     */
    assert(!(IS_GENERATOR(code_flags) && *stackdepth == 0));

    *nlocalsplus = prepare_localsplus(umd, g, code_flags);
    if (*nlocalsplus < 0) {
        return ERROR;
    }

    RETURN_IF_ERROR(convert_pseudo_ops(g));

    /* Order of basic blocks must have been determined by now */

    RETURN_IF_ERROR(normalize_jumps(g));
    assert(no_redundant_jumps(g));

    /* Can't modify the bytecode after computing jump offsets. */
    if (_PyCfg_ToInstructionSequence(g, seq) < 0) {
        return ERROR;
    }

    return SUCCESS;
}

/* This is used by _PyCompile_Assemble to fill in the jump and exception
 * targets in a synthetic CFG (which is not the ouptut of the builtin compiler).
 */
int
_PyCfg_JumpLabelsToTargets(cfg_builder *g)
{
    RETURN_IF_ERROR(translate_jump_labels_to_targets(g->g_entryblock));
    RETURN_IF_ERROR(label_exception_targets(g->g_entryblock));
    return SUCCESS;
}
