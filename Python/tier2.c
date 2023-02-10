#include "Python.h"
#include "stdlib.h"
#include "pycore_code.h"
#include "pycore_frame.h"

#include "opcode.h"

static _Py_CODEUNIT *
_PyCode_Tier2Initialize(_PyInterpreterFrame *, _Py_CODEUNIT *);

// Tier 2 warmup counter
_Py_CODEUNIT *
_PyCode_Tier2Warmup(_PyInterpreterFrame *frame, _Py_CODEUNIT *next_instr)
{
    PyCodeObject *code = frame->f_code;
    if (code->_tier2_warmup != 0) {
        code->_tier2_warmup++;
        if (code->_tier2_warmup == 0) {
            // If it fails, due to lack of memory or whatever,
            // just fall back to the tier 1 interpreter.
            _Py_CODEUNIT *next = _PyCode_Tier2Initialize(frame, next_instr);
            return next_instr;
            if (next != NULL) {
                return next;
            }
        }
    }
    return next_instr;
}

// Gets end of the bytecode for a code object.
_Py_CODEUNIT *
_PyCode_GetEnd(PyCodeObject *co)
{
    return (_Py_CODEUNIT *)(co->co_code_adaptive + _PyCode_NBYTES(co));
}

// Creates the overallocated array for the BBs.
static _PyTier2BBSpace *
_PyTier2_CreateBBSpace(Py_ssize_t space_to_alloc)
{
    _PyTier2BBSpace *bb_space = PyMem_Malloc(space_to_alloc);
    if (bb_space == NULL) {
        return NULL;
    }
    bb_space->next = NULL;
    bb_space->water_level = 0;
    assert((int)(space_to_alloc - sizeof(_PyTier2BB)) == (space_to_alloc - sizeof(_PyTier2BB)));
    bb_space->max_capacity = (int)(space_to_alloc - sizeof(_PyTier2BB));
    return bb_space;
}

/* Init a BB in BB space without any checks for waterlevel. */
static _PyTier2BB *
_PyTier2_InitBBNoCheck(_PyTier2BBSpace *bb_space, _Py_CODEUNIT *tier1_end,
    _Py_CODEUNIT *instr_start, _Py_CODEUNIT *instr_end)
{
    Py_ssize_t nbytes = (instr_end - instr_start) * sizeof(_Py_CODEUNIT);
    _PyTier2BB *bb_ptr = &bb_space->bbs[bb_space->water_level];
    bb_ptr->tier1_end = tier1_end;
    memcpy(bb_ptr->u_code, (const void *)instr_start, nbytes);
    assert(bb_space->water_level + nbytes == (int)nbytes);
    bb_space->water_level += (int)nbytes;
    return bb_ptr;
}


/* Opcode detection functions. Keep in sync with compile.c and dis! */

// dis.hasjabs
static inline int
IS_JABS_OPCODE(int opcode)
{
    return 0;
}

// dis.hasjrel
static inline int
IS_JREL_OPCODE(int opcode)
{
    switch (opcode) {
    case FOR_ITER:
    case JUMP_FORWARD:
    case JUMP_IF_FALSE_OR_POP:
    case JUMP_IF_TRUE_OR_POP:
    case POP_JUMP_IF_FALSE:
    case POP_JUMP_IF_TRUE:
    case SEND:
    case POP_JUMP_IF_NOT_NONE:
    case POP_JUMP_IF_NONE:
    case JUMP_BACKWARD_NO_INTERRUPT:
    case JUMP_BACKWARD:
        return 1;
    default:
        return 0;

    }
}

// dis.hasjrel || dis.hasjabs
static inline int
IS_JUMP_OPCODE(int opcode)
{
    return IS_JREL_OPCODE(opcode) || IS_JABS_OPCODE(opcode);
}


static inline int
IS_SCOPE_EXIT_OPCODE(int opcode)
{
    switch (opcode) {
    case RETURN_VALUE:
    case RAISE_VARARGS:
    case RERAISE:
        return 1;
    default:
        return 0;
    }
}

// KEEP IN SYNC WITH COMPILE.c!!!!
static int
IS_TERMINATOR_OPCODE(int opcode)
{
    return IS_JUMP_OPCODE(opcode) || IS_SCOPE_EXIT_OPCODE(opcode);
}

static int
compare_ints(const void *a, const void *b)
{
    const int a_num = *(int *)a;
    const int b_num = *(int *)b;
    return a_num - b_num;
}

// Returns 1 on error, 0 on success. Populates the jump target offset
// array for a code object.
static int
_PyCode_Tier2FillJumpTargets(PyCodeObject *co)
{
    // Remove all the RESUME instructions.
    // Count all the jump targets.
    Py_ssize_t jump_target_count = 0;
    for (Py_ssize_t i = 0; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT *instr_ptr = _PyCode_CODE(co) + i;
        _Py_CODEUNIT instr = *instr_ptr;
        switch (_Py_OPCODE(instr)) {
        case RESUME:
            _py_set_opcode(instr_ptr, RESUME_QUICK);
            break;
        default:
            jump_target_count += IS_JUMP_OPCODE(_Py_OPCODE(instr));
        }
    }

    // Find all the jump target instructions
    _Py_CODEUNIT *end = _PyCode_GetEnd(co);
    _Py_CODEUNIT *start = _PyCode_CODE(co);
    _Py_CODEUNIT *curr = start;

    // Impossibly big.
    if (jump_target_count != (int)jump_target_count) {
        return 1;
    }
    // Impossibly big
    if (end - start != (int)(end - start)) {
        return 1;
    }
    co->_jump_target_count = (int)jump_target_count;
    int *jump_targets = PyMem_Malloc(jump_target_count * sizeof(int));
    if (jump_targets == NULL) {
        return 1;
    }
    int curr_i = 0;
    while (curr <= end) {
        _Py_CODEUNIT instr = *curr;
        if (IS_JUMP_OPCODE(_Py_OPCODE(instr))) {
            _Py_CODEUNIT *target = curr + _Py_OPARG(instr);
            // (in terms of offset from start of co_code_adaptive)
            jump_targets[curr_i] = (int)(target - start);
            curr_i++;
        }
        curr++;
    }
    qsort(jump_targets, jump_target_count, sizeof(int), compare_ints);
    co->_jump_targets = jump_targets;
    return 0;
}

// Detects a BB from the current instruction start to the end of the code object.
_Py_CODEUNIT *
_PyTier2_Code_DetectBB(PyCodeObject *co, _Py_CODEUNIT *start)
{
    // There are only two cases that a BB ends.
    // 1. If there's a branch instruction / scope exit.
    // 2. If the instruction is a jump target.
    _Py_CODEUNIT *instr_end = _PyCode_GetEnd(co);
    _Py_CODEUNIT *curr = start;
    int *jump_target_offsets = co->_jump_targets;
    int jump_target_count = co->_jump_target_count;
    int curr_jump = 0;
    while (curr < instr_end) {
        _Py_CODEUNIT instr = *curr;
        int opcode = _Py_OPCODE(instr);
        if (IS_JUMP_OPCODE(opcode) || IS_SCOPE_EXIT_OPCODE(opcode)) {
            return curr;
        }
        while (_PyCode_CODE(co) + jump_target_offsets[curr_jump] < curr) {
            curr_jump++;
        }
        if (_PyCode_CODE(co) + jump_target_offsets[curr_jump] == curr) {
            return curr;
        }
        curr++;
    }
    return instr_end;
}

static _Py_CODEUNIT *
_PyCode_Tier2Initialize(_PyInterpreterFrame *frame, _Py_CODEUNIT *next_instr)
{
    int curr = _Py_OPCODE(*(next_instr - 1));
    assert(curr == RESUME);
    PyCodeObject *co = frame->f_code;
    assert(co->_bb_space == NULL);
    // 0. Single pass: mark jump targets
    // 1. Initialize basic blocks space.
    // 2. Create the entry BB (if it is, else the current BB).
    // 3. Jump into that BB.
    fprintf(stderr, "INITIALIZING %ld\n", Py_SIZE(co));

    Py_ssize_t space_to_alloc = (sizeof(_PyTier2BB) + _PyCode_NBYTES(co)) * 2;

    _PyTier2BBSpace *bb_space = _PyTier2_CreateBBSpace(space_to_alloc);
    if (bb_space == NULL) {
        return NULL;
    }

    co->_bb_space = bb_space;

    //_Py_CODEUNIT *entry_bb_end = _PyTier2_Code_DetectBB(co, _PyCode_CODE(co));
    _Py_CODEUNIT *entry_bb_end = _PyCode_GetEnd(co);

    _PyTier2BB *bb_ptr = _PyTier2_InitBBNoCheck(bb_space,
        entry_bb_end,
        _PyCode_CODE(co),
        entry_bb_end);

    if (_PyCode_Tier2FillJumpTargets(co)) {
        goto cleanup;
    }

    
    co->_entry_bb = bb_ptr;

    // Set the instruction pointer to the next one in the bb
    Py_ssize_t offset_from_start = (frame->prev_instr - _PyCode_CODE(co));
    assert(offset_from_start >= -1);
    frame->prev_instr = bb_ptr->u_code + offset_from_start;
    // _py_set_opcode(next_instr, CACHE);
    return bb_ptr->u_code + (next_instr - _PyCode_CODE(co));

cleanup:
    PyMem_Free(bb_space);
    return NULL;
}

/* Allocates and initializes a new basic block. If there's not enough space in
   the overallocated array, create a new array.

   Make sure to call _PyCode_Tier2Initialize before this!
*/
static _PyTier2BB *
_PyCode_Tier2BBNew(PyCodeObject *co, _Py_CODEUNIT *instr_start, _Py_CODEUNIT *instr_end)
{
    assert(co->_bb_space != NULL);

    _PyTier2BBSpace *bb_space = co->_bb_space;
    Py_ssize_t amount_to_alloc = (instr_start - instr_end) * sizeof(_Py_CODEUNIT *) + sizeof(_PyTier2BB);
    assert(bb_space->water_level + amount_to_alloc == (int)(bb_space->water_level + amount_to_alloc));

    // Need to allocate a new array.
    if (bb_space->water_level + amount_to_alloc > bb_space->max_capacity) {
        _PyTier2BBSpace *next_bb_space = _PyTier2_CreateBBSpace(bb_space->max_capacity + amount_to_alloc);
        if (next_bb_space == NULL) {
            return NULL;
        }
        next_bb_space->next = bb_space;
        // We want to make our bb_space point to the most recent one to get O(1) BB allocations.
        co->_bb_space = next_bb_space;
        bb_space = next_bb_space;
    }
    return _PyTier2_InitBBNoCheck(bb_space, _PyCode_GetEnd(co), instr_start, instr_end);
}
