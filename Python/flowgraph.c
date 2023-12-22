
#include <stdbool.h>

#include "Python.h"
#include "pycore_flowgraph.h"
#include "pycore_compile.h"
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()

#include "pycore_opcode_utils.h"
#define NEED_OPCODE_METADATA
#include "opcode_metadata.h"      // _PyOpcode_opcode_metadata, _PyOpcode_num_popped/pushed
#undef NEED_OPCODE_METADATA


#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    if ((X) == -1) {        \
        return ERROR;       \
    }

#define DEFAULT_BLOCK_SIZE 16

typedef _PyCompilerSrcLocation location;
typedef _PyCfgJumpTargetLabel jump_target_label;
typedef _PyCfgBasicblock basicblock;
typedef _PyCfgBuilder cfg_builder;
typedef _PyCfgInstruction cfg_instr;

static const jump_target_label NO_LABEL = {-1};

#define SAME_LABEL(L1, L2) ((L1).id == (L2).id)
#define IS_LABEL(L) (!SAME_LABEL((L), (NO_LABEL)))


static inline int
is_block_push(cfg_instr *i)
{
    return IS_BLOCK_PUSH_OPCODE(i->i_opcode);
}

static inline int
is_jump(cfg_instr *i)
{
    return IS_JUMP_OPCODE(i->i_opcode);
}

/* One arg*/
#define INSTR_SET_OP1(I, OP, ARG) \
    do { \
        assert(HAS_ARG(OP)); \
        _PyCfgInstruction *_instr__ptr_ = (I); \
        _instr__ptr_->i_opcode = (OP); \
        _instr__ptr_->i_oparg = (ARG); \
    } while (0);

/* No args*/
#define INSTR_SET_OP0(I, OP) \
    do { \
        assert(!HAS_ARG(OP)); \
        _PyCfgInstruction *_instr__ptr_ = (I); \
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

/* Allocate a new block and return a pointer to it.
   Returns NULL on error.
*/

static basicblock *
cfg_builder_new_block(cfg_builder *g)
{
    basicblock *b = (basicblock *)PyObject_Calloc(1, sizeof(basicblock));
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
    assert(HAS_ARG(opcode) || HAS_TARGET(opcode) || oparg == 0);
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

static inline int
basicblock_append_instructions(basicblock *target, basicblock *source)
{
    for (int i = 0; i < source->b_iused; i++) {
        int n = basicblock_next_instr(target);
        if (n < 0) {
            return ERROR;
        }
        target->b_instr[n] = source->b_instr[i];
    }
    return SUCCESS;
}

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

int
_PyBasicblock_InsertInstruction(basicblock *block, int pos, cfg_instr *instr) {
    RETURN_IF_ERROR(basicblock_next_instr(block));
    for (int i = block->b_iused - 1; i > pos; i--) {
        block->b_instr[i] = block->b_instr[i-1];
    }
    block->b_instr[pos] = *instr;
    return SUCCESS;
}

static int
instr_size(cfg_instr *instruction)
{
    return _PyCompile_InstrSize(instruction->i_opcode, instruction->i_oparg);
}

static int
blocksize(basicblock *b)
{
    int size = 0;
    for (int i = 0; i < b->b_iused; i++) {
        size += instr_size(&b->b_instr[i]);
    }
    return size;
}

/* For debugging purposes only */
#if 0
static void
dump_instr(cfg_instr *i)
{
    const char *jump = is_jump(i) ? "jump " : "";

    char arg[128];

    *arg = '\0';
    if (HAS_ARG(i->i_opcode)) {
        sprintf(arg, "arg: %d ", i->i_oparg);
    }
    if (HAS_TARGET(i->i_opcode)) {
        sprintf(arg, "target: %p [%d] ", i->i_target, i->i_oparg);
    }
    fprintf(stderr, "line: %d, opcode: %d %s%s\n",
                    i->i_loc.lineno, i->i_opcode, arg, jump);
}

static inline int
basicblock_returns(const basicblock *b) {
    cfg_instr *last = _PyCfg_BasicblockLastInstr(b);
    return last && (last->i_opcode == RETURN_VALUE || last->i_opcode == RETURN_CONST);
}

static void
dump_basicblock(const basicblock *b)
{
    const char *b_return = basicblock_returns(b) ? "return " : "";
    fprintf(stderr, "%d: [EH=%d CLD=%d WRM=%d NO_FT=%d %p] used: %d, depth: %d, offset: %d %s\n",
        b->b_label.id, b->b_except_handler, b->b_cold, b->b_warm, BB_NO_FALLTHROUGH(b), b, b->b_iused,
        b->b_startdepth, b->b_offset, b_return);
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

cfg_instr *
_PyCfg_BasicblockLastInstr(const basicblock *b) {
    assert(b->b_iused >= 0);
    if (b->b_iused > 0) {
        assert(b->b_instr != NULL);
        return &b->b_instr[b->b_iused - 1];
    }
    return NULL;
}

static inline int
basicblock_exits_scope(const basicblock *b) {
    cfg_instr *last = _PyCfg_BasicblockLastInstr(b);
    return last && IS_SCOPE_EXIT_OPCODE(last->i_opcode);
}

static bool
cfg_builder_current_block_is_terminated(cfg_builder *g)
{
    cfg_instr *last = _PyCfg_BasicblockLastInstr(g->g_curblock);
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

int
_PyCfgBuilder_Init(cfg_builder *g)
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

void
_PyCfgBuilder_Fini(cfg_builder* g)
{
    assert(cfg_builder_check(g));
    basicblock *b = g->g_block_list;
    while (b != NULL) {
        if (b->b_instr) {
            PyObject_Free((void *)b->b_instr);
        }
        basicblock *next = b->b_list;
        PyObject_Free((void *)b);
        b = next;
    }
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


/***** debugging helpers *****/

#ifndef NDEBUG
static int remove_redundant_nops(basicblock *bb);

/*
static bool
no_redundant_nops(cfg_builder *g) {
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        if (remove_redundant_nops(b) != 0) {
            return false;
        }
    }
    return true;
}
*/

static bool
no_empty_basic_blocks(cfg_builder *g) {
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        if (b->b_iused == 0) {
            return false;
        }
    }
    return true;
}

static bool
no_redundant_jumps(cfg_builder *g) {
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        cfg_instr *last = _PyCfg_BasicblockLastInstr(b);
        if (last != NULL) {
            if (IS_UNCONDITIONAL_JUMP_OPCODE(last->i_opcode)) {
                assert(last->i_target != b->b_next);
                if (last->i_target == b->b_next) {
                    return false;
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
    cfg_instr *last = _PyCfg_BasicblockLastInstr(b);
    if (last == NULL || !is_jump(last)) {
        return SUCCESS;
    }
    assert(!IS_ASSEMBLER_OPCODE(last->i_opcode));
    bool is_forward = last->i_target->b_visited == 0;
    switch(last->i_opcode) {
        case JUMP:
            last->i_opcode = is_forward ? JUMP_FORWARD : JUMP_BACKWARD;
            return SUCCESS;
        case JUMP_NO_INTERRUPT:
            last->i_opcode = is_forward ?
                JUMP_FORWARD : JUMP_BACKWARD_NO_INTERRUPT;
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
    if (is_forward) {
        return SUCCESS;
    }
    /* transform 'conditional jump T' to
     * 'reversed_jump b_next' followed by 'jump_backwards T'
     */

    basicblock *target = last->i_target;
    basicblock *backwards_jump = cfg_builder_new_block(g);
    if (backwards_jump == NULL) {
        return ERROR;
    }
    basicblock_addop(backwards_jump, JUMP, target->b_label.id, last->i_loc);
    backwards_jump->b_instr[0].i_target = target;
    last->i_opcode = reversed_opcode;
    last->i_target = b->b_next;

    backwards_jump->b_cold = b->b_cold;
    backwards_jump->b_next = b->b_next;
    b->b_next = backwards_jump;
    return SUCCESS;
}


static int
normalize_jumps(_PyCfgBuilder *g)
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

static void
resolve_jump_offsets(basicblock *entryblock)
{
    int bsize, totsize, extended_arg_recompile;

    /* Compute the size of each block and fixup jump args.
       Replace block pointer with position in bytecode. */
    do {
        totsize = 0;
        for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
            bsize = blocksize(b);
            b->b_offset = totsize;
            totsize += bsize;
        }
        extended_arg_recompile = 0;
        for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
            bsize = b->b_offset;
            for (int i = 0; i < b->b_iused; i++) {
                cfg_instr *instr = &b->b_instr[i];
                int isize = instr_size(instr);
                /* jump offsets are computed relative to
                 * the instruction pointer after fetching
                 * the jump instruction.
                 */
                bsize += isize;
                if (is_jump(instr)) {
                    instr->i_oparg = instr->i_target->b_offset;
                    if (instr->i_oparg < bsize) {
                        assert(IS_BACKWARDS_JUMP_OPCODE(instr->i_opcode));
                        instr->i_oparg = bsize - instr->i_oparg;
                    }
                    else {
                        assert(!IS_BACKWARDS_JUMP_OPCODE(instr->i_opcode));
                        instr->i_oparg -= bsize;
                    }
                    if (instr_size(instr) != isize) {
                        extended_arg_recompile = 1;
                    }
                }
            }
        }

    /* XXX: This is an awful hack that could hurt performance, but
        on the bright side it should work until we come up
        with a better solution.

        The issue is that in the first loop blocksize() is called
        which calls instr_size() which requires i_oparg be set
        appropriately. There is a bootstrap problem because
        i_oparg is calculated in the second loop above.

        So we loop until we stop seeing new EXTENDED_ARGs.
        The only EXTENDED_ARGs that could be popping up are
        ones in jump instructions.  So this should converge
        fairly quickly.
    */
    } while (extended_arg_recompile);
}

int
_PyCfg_ResolveJumps(_PyCfgBuilder *g)
{
    RETURN_IF_ERROR(normalize_jumps(g));
    assert(no_redundant_jumps(g));
    resolve_jump_offsets(g->g_entryblock);
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

int
_PyCfg_JumpLabelsToTargets(basicblock *entryblock)
{
    return translate_jump_labels_to_targets(entryblock);
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


typedef _PyCfgExceptStack ExceptStack;

static basicblock *
push_except_block(ExceptStack *stack, cfg_instr *setup) {
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
pop_except_block(ExceptStack *stack) {
    assert(stack->depth > 0);
    return stack->handlers[--stack->depth];
}

static basicblock *
except_stack_top(ExceptStack *stack) {
    return stack->handlers[stack->depth];
}

static ExceptStack *
make_except_stack(void) {
    ExceptStack *new = PyMem_Malloc(sizeof(ExceptStack));
    if (new == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    new->depth = 0;
    new->handlers[0] = NULL;
    return new;
}

static ExceptStack *
copy_except_stack(ExceptStack *stack) {
    ExceptStack *copy = PyMem_Malloc(sizeof(ExceptStack));
    if (copy == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(copy, stack, sizeof(ExceptStack));
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
int
_PyCfg_Stackdepth(basicblock *entryblock, int code_flags)
{
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        b->b_startdepth = INT_MIN;
    }
    basicblock **stack = make_cfg_traversal_stack(entryblock);
    if (!stack) {
        return ERROR;
    }

    int maxdepth = 0;
    basicblock **sp = stack;
    if (code_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) {
        stackdepth_push(&sp, entryblock, 1);
    } else {
        stackdepth_push(&sp, entryblock, 0);
    }

    while (sp != stack) {
        basicblock *b = *--sp;
        int depth = b->b_startdepth;
        assert(depth >= 0);
        basicblock *next = b->b_next;
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            int effect = PyCompile_OpcodeStackEffectWithJump(instr->i_opcode, instr->i_oparg, 0);
            if (effect == PY_INVALID_STACK_EFFECT) {
                PyErr_Format(PyExc_SystemError,
                             "compiler PyCompile_OpcodeStackEffectWithJump(opcode=%d, arg=%i) failed",
                             instr->i_opcode, instr->i_oparg);
                return ERROR;
            }
            int new_depth = depth + effect;
            assert(new_depth >= 0); /* invalid code or bug in stackdepth() */
            if (new_depth > maxdepth) {
                maxdepth = new_depth;
            }
            if (HAS_TARGET(instr->i_opcode)) {
                effect = PyCompile_OpcodeStackEffectWithJump(instr->i_opcode, instr->i_oparg, 1);
                assert(effect != PY_INVALID_STACK_EFFECT);
                int target_depth = depth + effect;
                assert(target_depth >= 0); /* invalid code or bug in stackdepth() */
                if (target_depth > maxdepth) {
                    maxdepth = target_depth;
                }
                stackdepth_push(&sp, instr->i_target, target_depth);
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
            stackdepth_push(&sp, next, depth);
        }
    }
    PyMem_Free(stack);
    return maxdepth;
}

static int
label_exception_targets(basicblock *entryblock) {
    basicblock **todo_stack = make_cfg_traversal_stack(entryblock);
    if (todo_stack == NULL) {
        return ERROR;
    }
    ExceptStack *except_stack = make_except_stack();
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
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            if (is_block_push(instr)) {
                if (!instr->i_target->b_visited) {
                    ExceptStack *copy = copy_except_stack(except_stack);
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
            }
            else if (is_jump(instr)) {
                instr->i_except = handler;
                assert(i == b->b_iused -1);
                if (!instr->i_target->b_visited) {
                    if (BB_HAS_FALLTHROUGH(b)) {
                        ExceptStack *copy = copy_except_stack(except_stack);
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
            else {
                if (instr->i_opcode == YIELD_VALUE) {
                    instr->i_oparg = except_stack->depth;
                }
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
mark_reachable(basicblock *entryblock) {
    basicblock **stack = make_cfg_traversal_stack(entryblock);
    if (stack == NULL) {
        return ERROR;
    }
    basicblock **sp = stack;
    entryblock->b_predecessors = 1;
    *sp++ = entryblock;
    while (sp > stack) {
        basicblock *b = *(--sp);
        b->b_visited = 1;
        if (b->b_next && BB_HAS_FALLTHROUGH(b)) {
            if (!b->b_next->b_visited) {
                assert(b->b_next->b_predecessors == 0);
                *sp++ = b->b_next;
            }
            b->b_next->b_predecessors++;
        }
        for (int i = 0; i < b->b_iused; i++) {
            basicblock *target;
            cfg_instr *instr = &b->b_instr[i];
            if (is_jump(instr) || is_block_push(instr)) {
                target = instr->i_target;
                if (!target->b_visited) {
                    assert(target->b_predecessors == 0 || target == b->b_next);
                    *sp++ = target;
                }
                target->b_predecessors++;
            }
        }
    }
    PyMem_Free(stack);
    return SUCCESS;
}

static void
eliminate_empty_basic_blocks(cfg_builder *g) {
    /* Eliminate empty blocks */
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        basicblock *next = b->b_next;
        while (next && next->b_iused == 0) {
            next = next->b_next;
        }
        b->b_next = next;
    }
    while(g->g_entryblock && g->g_entryblock->b_iused == 0) {
        g->g_entryblock = g->g_entryblock->b_next;
    }
    int next_lbl = get_max_label(g->g_entryblock) + 1;
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        assert(b->b_iused > 0);
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            if (HAS_TARGET(instr->i_opcode)) {
                basicblock *target = instr->i_target;
                while (target->b_iused == 0) {
                    target = target->b_next;
                }
                if (instr->i_target != target) {
                    if (!IS_LABEL(target->b_label)) {
                        target->b_label.id = next_lbl++;
                    }
                    instr->i_target = target;
                    instr->i_oparg = target->b_label.id;
                }
                assert(instr->i_target && instr->i_target->b_iused > 0);
            }
        }
    }
}

static int
remove_redundant_nops(basicblock *bb) {
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
                basicblock* next = bb->b_next;
                while (next && next->b_iused == 0) {
                    next = next->b_next;
                }
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
remove_redundant_nops_and_pairs(basicblock *entryblock)
{
    bool done = false;

    while (! done) {
        done = true;
        cfg_instr *prev_instr = NULL;
        cfg_instr *instr = NULL;
        for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
            remove_redundant_nops(b);
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
     */
    assert(no_empty_basic_blocks(g));
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        cfg_instr *last = _PyCfg_BasicblockLastInstr(b);
        assert(last != NULL);
        assert(!IS_ASSEMBLER_OPCODE(last->i_opcode));
        if (IS_UNCONDITIONAL_JUMP_OPCODE(last->i_opcode)) {
            if (last->i_target == NULL) {
                PyErr_SetString(PyExc_SystemError, "jump with NULL target");
                return ERROR;
            }
            if (last->i_target == b->b_next) {
                assert(b->b_next->b_iused);
                INSTR_SET_OP0(last, NOP);
            }
        }
    }
    return SUCCESS;
}

/* Maximum size of basic block that should be copied in optimizer */
#define MAX_COPY_SIZE 4

/* If this block ends with an unconditional jump to a small exit block, then
 * remove the jump and extend this block with the target.
 * Returns 1 if extended, 0 if no change, and -1 on error.
 */
static int
inline_small_exit_blocks(basicblock *bb) {
    cfg_instr *last = _PyCfg_BasicblockLastInstr(bb);
    if (last == NULL) {
        return 0;
    }
    if (!IS_UNCONDITIONAL_JUMP_OPCODE(last->i_opcode)) {
        return 0;
    }
    basicblock *target = last->i_target;
    if (basicblock_exits_scope(target) && target->b_iused <= MAX_COPY_SIZE) {
        INSTR_SET_OP0(last, NOP);
        RETURN_IF_ERROR(basicblock_append_instructions(bb, target));
        return 1;
    }
    return 0;
}

// Attempt to eliminate jumps to jumps by updating inst to jump to
// target->i_target using the provided opcode. Return whether or not the
// optimization was successful.
static bool
jump_thread(cfg_instr *inst, cfg_instr *target, int opcode)
{
    assert(is_jump(inst));
    assert(is_jump(target));
    // bpo-45773: If inst->i_target == target->i_target, then nothing actually
    // changes (and we fall into an infinite loop):
    if ((inst->i_loc.lineno == target->i_loc.lineno || target->i_loc.lineno == -1) &&
        inst->i_target != target->i_target)
    {
        inst->i_target = target->i_target;
        inst->i_opcode = opcode;
        return true;
    }
    return false;
}

static PyObject*
get_const_value(int opcode, int oparg, PyObject *co_consts)
{
    PyObject *constant = NULL;
    assert(HAS_CONST(opcode));
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
        if (!HAS_CONST(inst[i].i_opcode)) {
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
    if (_PyCompile_ConstCacheMergeOne(const_cache, &newconst) < 0) {
        Py_DECREF(newconst);
        return ERROR;
    }

    Py_ssize_t index;
    for (index = 0; index < PyList_GET_SIZE(consts); index++) {
        if (PyList_GET_ITEM(consts, index) == newconst) {
            break;
        }
    }
    if (index == PyList_GET_SIZE(consts)) {
        if ((size_t)index >= (size_t)INT_MAX - 1) {
            Py_DECREF(newconst);
            PyErr_SetString(PyExc_OverflowError, "too many constants");
            return ERROR;
        }
        if (PyList_Append(consts, newconst)) {
            Py_DECREF(newconst);
            return ERROR;
        }
    }
    Py_DECREF(newconst);
    for (int i = 0; i < n; i++) {
        INSTR_SET_OP0(&inst[i], NOP);
    }
    INSTR_SET_OP1(&inst[n], LOAD_CONST, (int)index);
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
optimize_basic_block(PyObject *const_cache, basicblock *bb, PyObject *consts)
{
    assert(PyDict_CheckExact(const_cache));
    assert(PyList_CheckExact(consts));
    cfg_instr nop;
    INSTR_SET_OP0(&nop, NOP);
    cfg_instr *target = &nop;
    int opcode = 0;
    int oparg = 0;
    int nextop = 0;
    for (int i = 0; i < bb->b_iused; i++) {
        cfg_instr *inst = &bb->b_instr[i];
        bool is_copy_of_load_const = (opcode == LOAD_CONST &&
                                      inst->i_opcode == COPY &&
                                      inst->i_oparg == 1);
        if (! is_copy_of_load_const) {
            opcode = inst->i_opcode;
            oparg = inst->i_oparg;
            if (HAS_TARGET(opcode)) {
                assert(inst->i_target->b_iused > 0);
                target = &inst->i_target->b_instr[0];
                assert(!IS_ASSEMBLER_OPCODE(target->i_opcode));
            }
            else {
                target = &nop;
            }
        }
        nextop = i+1 < bb->b_iused ? bb->b_instr[i+1].i_opcode : 0;
        assert(!IS_ASSEMBLER_OPCODE(opcode));
        switch (opcode) {
            /* Remove LOAD_CONST const; conditional jump */
            case LOAD_CONST:
            {
                PyObject* cnt;
                int is_true;
                int jump_if_true;
                switch(nextop) {
                    case POP_JUMP_IF_FALSE:
                    case POP_JUMP_IF_TRUE:
                        cnt = get_const_value(opcode, oparg, consts);
                        if (cnt == NULL) {
                            goto error;
                        }
                        is_true = PyObject_IsTrue(cnt);
                        Py_DECREF(cnt);
                        if (is_true == -1) {
                            goto error;
                        }
                        INSTR_SET_OP0(inst, NOP);
                        jump_if_true = nextop == POP_JUMP_IF_TRUE;
                        if (is_true == jump_if_true) {
                            bb->b_instr[i+1].i_opcode = JUMP;
                        }
                        else {
                            INSTR_SET_OP0(&bb->b_instr[i + 1], NOP);
                        }
                        break;
                    case IS_OP:
                        cnt = get_const_value(opcode, oparg, consts);
                        if (cnt == NULL) {
                            goto error;
                        }
                        int jump_op = i+2 < bb->b_iused ? bb->b_instr[i+2].i_opcode : 0;
                        if (Py_IsNone(cnt) && (jump_op == POP_JUMP_IF_FALSE || jump_op == POP_JUMP_IF_TRUE)) {
                            unsigned char nextarg = bb->b_instr[i+1].i_oparg;
                            INSTR_SET_OP0(inst, NOP);
                            INSTR_SET_OP0(&bb->b_instr[i + 1], NOP);
                            bb->b_instr[i+2].i_opcode = nextarg ^ (jump_op == POP_JUMP_IF_FALSE) ?
                                    POP_JUMP_IF_NOT_NONE : POP_JUMP_IF_NONE;
                        }
                        Py_DECREF(cnt);
                        break;
                    case RETURN_VALUE:
                        INSTR_SET_OP0(inst, NOP);
                        INSTR_SET_OP1(&bb->b_instr[++i], RETURN_CONST, oparg);
                        break;
                }
                break;
            }
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
                        i -= jump_thread(inst, target, inst->i_opcode);
                }
                break;
            case POP_JUMP_IF_FALSE:
                switch (target->i_opcode) {
                    case JUMP:
                        i -= jump_thread(inst, target, POP_JUMP_IF_FALSE);
                }
                break;
            case POP_JUMP_IF_TRUE:
                switch (target->i_opcode) {
                    case JUMP:
                        i -= jump_thread(inst, target, POP_JUMP_IF_TRUE);
                }
                break;
            case JUMP:
                switch (target->i_opcode) {
                    case JUMP:
                        i -= jump_thread(inst, target, JUMP);
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
                    i -= jump_thread(inst, target, FOR_ITER);
                    */
                }
                break;
            case SWAP:
                if (oparg == 1) {
                    INSTR_SET_OP0(inst, NOP);
                    break;
                }
                if (swaptimize(bb, &i) < 0) {
                    goto error;
                }
                apply_static_swaps(bb, i);
                break;
            case KW_NAMES:
                break;
            case PUSH_NULL:
                if (nextop == LOAD_GLOBAL && (bb->b_instr[i+1].i_oparg & 1) == 0) {
                    INSTR_SET_OP0(inst, NOP);
                    bb->b_instr[i+1].i_oparg |= 1;
                }
                break;
            default:
                /* All HAS_CONST opcodes should be handled with LOAD_CONST */
                assert (!HAS_CONST(inst->i_opcode));
        }
    }
    return SUCCESS;
error:
    return ERROR;
}


/* Perform optimizations on a control flow graph.
   The consts object should still be in list form to allow new constants
   to be appended.

   Code trasnformations that reduce code size initially fill the gaps with
   NOPs.  Later those NOPs are removed.
*/
static int
optimize_cfg(cfg_builder *g, PyObject *consts, PyObject *const_cache)
{
    assert(PyDict_CheckExact(const_cache));
    RETURN_IF_ERROR(check_cfg(g));
    eliminate_empty_basic_blocks(g);
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        RETURN_IF_ERROR(inline_small_exit_blocks(b));
    }
    assert(no_empty_basic_blocks(g));
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        RETURN_IF_ERROR(optimize_basic_block(const_cache, b, consts));
        assert(b->b_predecessors == 0);
    }
    RETURN_IF_ERROR(remove_redundant_nops_and_pairs(g->g_entryblock));
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        RETURN_IF_ERROR(inline_small_exit_blocks(b));
    }
    RETURN_IF_ERROR(mark_reachable(g->g_entryblock));

    /* Delete unreachable instructions */
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
       if (b->b_predecessors == 0) {
            b->b_iused = 0;
       }
    }
    for (basicblock *b = g->g_entryblock; b != NULL; b = b->b_next) {
        remove_redundant_nops(b);
    }
    eliminate_empty_basic_blocks(g);
    /* This assertion fails in an edge case (See gh-109889).
     * Remove it for the release (it's just one more NOP in the
     * bytecode for unlikely code).
     */
    // assert(no_redundant_nops(g));
    RETURN_IF_ERROR(remove_redundant_jumps(g));
    return SUCCESS;
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
        assert(!IS_SUPERINSTRUCTION_OPCODE(instr->i_opcode));
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
    cfg_instr *last = _PyCfg_BasicblockLastInstr(b);
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
            assert(!IS_SUPERINSTRUCTION_OPCODE(instr->i_opcode));
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
            if (HAS_CONST(b->b_instr[i].i_opcode)) {
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
            if (HAS_CONST(b->b_instr[i].i_opcode)) {
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
push_cold_blocks_to_end(cfg_builder *g, int code_flags) {
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
            basicblock_addop(explicit_jump, JUMP, b->b_next->b_label.id, NO_LOCATION);
            explicit_jump->b_cold = 1;
            explicit_jump->b_next = b->b_next;
            b->b_next = explicit_jump;

            /* set target */
            cfg_instr *last = _PyCfg_BasicblockLastInstr(explicit_jump);
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
        RETURN_IF_ERROR(remove_redundant_jumps(g));
    }
    return SUCCESS;
}

void
_PyCfg_ConvertPseudoOps(basicblock *entryblock)
{
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        for (int i = 0; i < b->b_iused; i++) {
            cfg_instr *instr = &b->b_instr[i];
            if (is_block_push(instr) || instr->i_opcode == POP_BLOCK) {
                INSTR_SET_OP0(instr, NOP);
            }
            else if (instr->i_opcode == STORE_FAST_MAYBE_NULL) {
                instr->i_opcode = STORE_FAST;
            }
        }
    }
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        remove_redundant_nops(b);
    }
}

static inline bool
is_exit_without_lineno(basicblock *b) {
    if (!basicblock_exits_scope(b)) {
        return false;
    }
    for (int i = 0; i < b->b_iused; i++) {
        if (b->b_instr[i].i_loc.lineno >= 0) {
            return false;
        }
    }
    return true;
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
    assert(no_empty_basic_blocks(g));

    int next_lbl = get_max_label(g->g_entryblock) + 1;

    /* Copy all exit blocks without line number that are targets of a jump.
     */
    basicblock *entryblock = g->g_entryblock;
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        cfg_instr *last = _PyCfg_BasicblockLastInstr(b);
        assert(last != NULL);
        if (is_jump(last)) {
            basicblock *target = last->i_target;
            if (is_exit_without_lineno(target) && target->b_predecessors > 1) {
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
            if (is_exit_without_lineno(b->b_next)) {
                cfg_instr *last = _PyCfg_BasicblockLastInstr(b);
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
        cfg_instr *last = _PyCfg_BasicblockLastInstr(b);
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
            assert(b->b_next->b_iused);
            if (b->b_next->b_instr[0].i_loc.lineno < 0) {
                b->b_next->b_instr[0].i_loc = prev_location;
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

/* Make sure that all returns have a line number, even if early passes
 * have failed to propagate a correct line number.
 * The resulting line number may not be correct according to PEP 626,
 * but should be "good enough", and no worse than in older versions. */
static void
guarantee_lineno_for_exits(basicblock *entryblock, int firstlineno) {
    int lineno = firstlineno;
    assert(lineno > 0);
    for (basicblock *b = entryblock; b != NULL; b = b->b_next) {
        cfg_instr *last = _PyCfg_BasicblockLastInstr(b);
        if (last == NULL) {
            continue;
        }
        if (last->i_loc.lineno < 0) {
            if (last->i_opcode == RETURN_VALUE) {
                for (int i = 0; i < b->b_iused; i++) {
                    assert(b->b_instr[i].i_loc.lineno < 0);

                    b->b_instr[i].i_loc.lineno = lineno;
                }
            }
        }
        else {
            lineno = last->i_loc.lineno;
        }
    }
}

static int
resolve_line_numbers(cfg_builder *g, int firstlineno)
{
    RETURN_IF_ERROR(duplicate_exits_without_lineno(g));
    propagate_line_numbers(g->g_entryblock);
    guarantee_lineno_for_exits(g->g_entryblock, firstlineno);
    return SUCCESS;
}

int
_PyCfg_OptimizeCodeUnit(cfg_builder *g, PyObject *consts, PyObject *const_cache,
                       int code_flags, int nlocals, int nparams, int firstlineno)
{
    assert(cfg_builder_check(g));
    /** Preprocessing **/
    /* Map labels to targets and mark exception handlers */
    RETURN_IF_ERROR(translate_jump_labels_to_targets(g->g_entryblock));
    RETURN_IF_ERROR(mark_except_handlers(g->g_entryblock));
    RETURN_IF_ERROR(label_exception_targets(g->g_entryblock));

    /** Optimization **/
    RETURN_IF_ERROR(optimize_cfg(g, consts, const_cache));
    RETURN_IF_ERROR(remove_unused_consts(g->g_entryblock, consts));
    RETURN_IF_ERROR(
        add_checks_for_loads_of_uninitialized_variables(
            g->g_entryblock, nlocals, nparams));

    RETURN_IF_ERROR(push_cold_blocks_to_end(g, code_flags));
    RETURN_IF_ERROR(resolve_line_numbers(g, firstlineno));
    return SUCCESS;
}
