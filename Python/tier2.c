#include "Python.h"
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
            if (next != NULL) {
                return next;
            }
        }
    }
    return next_instr;
}

static _Py_CODEUNIT *
_PyCode_Tier2Initialize(_PyInterpreterFrame *frame, _Py_CODEUNIT *next_instr)
{
    int curr = _Py_OPCODE(*(next_instr - 1));
    assert(curr == RESUME || curr == JUMP_BACKWARD);
    PyCodeObject *co = frame->f_code;
    assert(co->_bb_space == NULL);
    // 1. Initialize basic blocks space.
    // 2. Copy over instructions to basic blocks.
    // (For now, just have one basic block = on code object)
    // @TODO split up code object into basic blocks.
    // 3. Set the instruction pointer to correct one.
    fprintf(stderr, "INITIALIZING %ld\n", Py_SIZE(co));
    _PyTier2BBSpace *bb_space = PyMem_Malloc((sizeof(_PyTier2BB) + _PyCode_NBYTES(co)) * 2);
    if (bb_space == NULL) {
        return NULL;
    }
    bb_space->next = NULL;
    bb_space->water_level = 0;
    co->_bb_space = bb_space;

    _PyTier2BB *bb_ptr = bb_space->bbs;
    bb_ptr->tier1_start = _PyCode_CODE(co);
    memcpy(bb_ptr->u_code, _PyCode_CODE(co), _PyCode_NBYTES(co));
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
    co->_first_instr = bb_ptr->u_code;

    // Set the instruction pointer to the next one in the bb
    Py_ssize_t offset_from_start = (frame->prev_instr - _PyCode_CODE(co));
    assert(offset_from_start >= -1);
    frame->prev_instr = bb_ptr->u_code + offset_from_start;
    // test to see we are working
    // _py_set_opcode(next_instr, CACHE);
    return bb_ptr->u_code + (next_instr - _PyCode_CODE(co));
}
