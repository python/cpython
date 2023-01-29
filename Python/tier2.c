#include "Python.h"
#include "pycore_code.h"
#include "pycore_frame.h"

#include "opcode.h"

static int
_PyCode_Tier2Initialize(_PyInterpreterFrame *, _Py_CODEUNIT **);

// Tier 2 warmup counter
void
_PyCode_Tier2Warmup(_PyInterpreterFrame *frame, _Py_CODEUNIT **next_instr)
{
    PyCodeObject *code = frame->f_code;
    if (code->_tier2_warmup != 0) {
        code->_tier2_warmup++;
        if (code->_tier2_warmup == 0) {
            // If it fails, due to lack of memory or whatever,
            // just fall back to the tier 1 interpreter.
            if (_PyCode_Tier2Initialize(frame, next_instr) < 0) {
                PyErr_Clear();
            }
        }
    }
}

static int
_PyCode_Tier2Initialize(_PyInterpreterFrame *frame, _Py_CODEUNIT **next_instr)
{
    PyCodeObject *co = frame->f_code;
    if (co->_bb_space != NULL) {
        return 0;
    }
    // 1. Initialize basic blocks space.
    // 2. Copy over instructions to basic blocks.
    // (For now, just have one basic block = on code object)
    // @TODO split up code object into basic blocks.
    // 3. Set the instruction pointer to correct one.
    fprintf(stderr, "INITIALIZING %ld\n", Py_SIZE(co));
    _PyTier2BBSpace *bb_space = PyMem_Malloc(Py_SIZE(co) * sizeof(_Py_CODEUNIT) * 2);
    if (bb_space == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    bb_space->next = NULL;
    bb_space->water_level = 0;
    co->_bb_space = bb_space;

    _PyTier2BB *bb_ptr = (_PyTier2BB *)bb_space->bbs;
    bb_ptr->tier1_start = _PyCode_CODE(co);
    memcpy(bb_ptr->u_code, _PyCode_CODE(co), Py_SIZE(co) * sizeof(_Py_CODEUNIT));

    co->_bb_next = bb_ptr;
    // test to see we are working
    _py_set_opcode(bb_ptr->u_code, CACHE);

    // Set the instruction pointer to the next one in the bb
    frame->prev_instr = bb_ptr->u_code + (frame->prev_instr - _PyCode_CODE(co));
    *next_instr = frame->prev_instr + 1;

    return 0;
}
