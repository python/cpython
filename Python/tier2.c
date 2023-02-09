#include "Python.h"
#include "pycore_code.h"
#include "pycore_frame.h"
#include "pycore_interp.h"

#include "opcode.h"
#include "pystate.h"
#include "pytypedefs.h"

static _Py_CODEUNIT *
_PyCode_Tier2Initialize(_PyInterpreterFrame *, _Py_CODEUNIT *);

// Tier 2 warmup counter
int
_PyCode_Tier2Warmup(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag, _Py_CODEUNIT *next_instr, PyObject **stack_pointer, PyObject **retval)
{
    if (tstate->interp->eval_frame != NULL) {
        return 0;
    }
    PyCodeObject *code = frame->f_code;
    if (code->_tier2_warmup != 0) {
        code->_tier2_warmup++;
        if (code->_tier2_warmup == 0) {
            // If it fails, due to lack of memory or whatever,
            // just fall back to the tier 1 interpreter.
            _Py_CODEUNIT *next = _PyCode_Tier2Initialize(frame, next_instr);
            // Swap out the profiler to use the profiling eval loop.
            frame->prev_instr = next_instr - 1;
            _PyFrame_SetStackPointer(frame, stack_pointer);
            // Do something with entry_frame here, maybe set the current frame to an entry
            // frame and check for that in RETURN_VALUE?
            *retval = _PyEval_EvalFrameTier2Profile(tstate, frame, throwflag);
            PyObject_Print(*retval, stderr, Py_PRINT_RAW);
            return 1;
            
        }
    }
    return 0;
}



static _PyTier2BBSpace *
_PyTier2_CreateBBSpace(Py_ssize_t space_to_alloc)
{
    _PyTier2BBSpace *bb_space = PyMem_Malloc(space_to_alloc);
    if (bb_space == NULL) {
        return NULL;
    }
    bb_space->next = NULL;
    bb_space->water_level = 0;
    bb_space->max_capacity = space_to_alloc - sizeof(_PyTier2BB);
    return bb_space;
}

/* Init a BB in BB space without any checks for waterlevel. */
static _PyTier2BB *
_PyTier2_InitBBNoCheck(_PyTier2BBSpace *bb_space, _Py_CODEUNIT *tier1_start,
    const void *instr_bytes_src, Py_ssize_t instr_nbytes)
{
    _PyTier2BB *bb_ptr = &bb_space->bbs[bb_space->water_level];
    bb_ptr->tier1_start = tier1_start;
    memcpy(bb_ptr->u_code, instr_bytes_src, instr_nbytes);
    assert(bb_space->water_level + instr_nbytes == (int)(bb_space->water_level + instr_nbytes));
    bb_space->water_level += instr_nbytes;
    return bb_ptr;
}

static _Py_CODEUNIT *
_PyCode_Tier2Initialize(_PyInterpreterFrame *frame, _Py_CODEUNIT *next_instr)
{
    int curr = _Py_OPCODE(*(next_instr - 1));
    PyCodeObject *co = frame->f_code;
    assert(co->_bb_space == NULL);
    // 1. Initialize basic blocks space.
    // 2. Copy over instructions to basic blocks.
    // (For now, just have one basic block = on code object)
    // @TODO split up code object into basic blocks.
    // 3. Set the instruction pointer to correct one.
    fprintf(stderr, "INITIALIZING %ld\n", Py_SIZE(co));
    Py_ssize_t space_to_alloc = (sizeof(_PyTier2BB) + _PyCode_NBYTES(co)) * 2;

    _PyTier2BBSpace *bb_space = _PyTier2_CreateBBSpace(space_to_alloc);
    if (bb_space == NULL) {
        return NULL;
    }

    co->_bb_space = bb_space;

    _PyTier2BB *bb_ptr = _PyTier2_InitBBNoCheck(bb_space, _PyCode_CODE(co),
        _PyCode_CODE(co), _PyCode_NBYTES(co));

    // Remove all the RESUME and JUMP_BACKWARDS instructions
    for (Py_ssize_t i = 0; i < Py_SIZE(co); i++) {
        _Py_CODEUNIT instr = bb_ptr->u_code[i];
        switch (_Py_OPCODE(instr)) {
        case RESUME:
            _py_set_opcode(&(bb_ptr->u_code[i]), RESUME_QUICK);
            break;
        case JUMP_BACKWARD:
            _py_set_opcode(&(bb_ptr->u_code[i]), JUMP_BACKWARD);
            break;
        }

    }


    co->_bb_next = bb_ptr;

    // Set the instruction pointer to the next one in the bb
    Py_ssize_t offset_from_start = (frame->prev_instr - _PyCode_CODE(co));
    assert(offset_from_start >= -1);
    frame->prev_instr = bb_ptr->u_code + offset_from_start;
    // test to see we are working
    // _py_set_opcode(next_instr, CACHE);
    return bb_ptr->u_code + (next_instr - _PyCode_CODE(co));
}

/* Allocates and initializes a new basic block. If not enough space in
   the overallocated array, create a new array.

   Make sure to call _PyCode_Tier2Initialize before this!
*/
static _PyTier2BB *
_PyCode_Tier2BBNew(PyCodeObject *co, _Py_CODEUNIT *tier1_start, _Py_CODEUNIT *instr, Py_ssize_t code_size)
{
    assert(co->_bb_space != NULL);

    _PyTier2BBSpace *bb_space = co->_bb_space;
    Py_ssize_t amount_to_alloc = code_size + sizeof(_PyTier2BB);
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
    return _PyTier2_InitBBNoCheck(bb_space, tier1_start, instr, code_size * sizeof(_Py_CODEUNIT));
}
