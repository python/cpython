/*
 * This file implements a data structure representing a sequence of
 * instructions, which is used by different parts of the compilation
 * pipeline.
 */


#include <stdbool.h>

#include "Python.h"

#include "pycore_compile.h" // _PyCompile_EnsureArrayLargeEnough
#include "pycore_opcode_utils.h"
#include "pycore_opcode_metadata.h" // OPCODE_HAS_ARG, etc

typedef _PyInstruction instruction;
typedef _PyInstructionSequence instr_sequence;
typedef _Py_SourceLocation location;

#define INITIAL_INSTR_SEQUENCE_SIZE 100
#define INITIAL_INSTR_SEQUENCE_LABELS_MAP_SIZE 10

#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    if ((X) == -1) {        \
        return ERROR;       \
    }

static int
instr_sequence_next_inst(instr_sequence *seq) {
    assert(seq->s_instrs != NULL || seq->s_used == 0);

    RETURN_IF_ERROR(
        _PyCompile_EnsureArrayLargeEnough(seq->s_used + 1,
                                          (void**)&seq->s_instrs,
                                          &seq->s_allocated,
                                          INITIAL_INSTR_SEQUENCE_SIZE,
                                          sizeof(instruction)));
    assert(seq->s_allocated >= 0);
    assert(seq->s_used < seq->s_allocated);
    return seq->s_used++;
}

_PyJumpTargetLabel
_PyInstructionSequence_NewLabel(instr_sequence *seq)
{
    _PyJumpTargetLabel lbl = {++seq->s_next_free_label};
    return lbl;
}

int
_PyInstructionSequence_UseLabel(instr_sequence *seq, int lbl)
{
    int old_size = seq->s_labelmap_size;
    RETURN_IF_ERROR(
        _PyCompile_EnsureArrayLargeEnough(lbl,
                                          (void**)&seq->s_labelmap,
                                           &seq->s_labelmap_size,
                                           INITIAL_INSTR_SEQUENCE_LABELS_MAP_SIZE,
                                           sizeof(int)));

    for(int i = old_size; i < seq->s_labelmap_size; i++) {
        seq->s_labelmap[i] = -111;  /* something weird, for debugging */
    }
    seq->s_labelmap[lbl] = seq->s_used; /* label refers to the next instruction */
    return SUCCESS;
}

int
_PyInstructionSequence_ApplyLabelMap(instr_sequence *instrs)
{
    if (instrs->s_labelmap == NULL) {
        /* Already applied - nothing to do */
        return SUCCESS;
    }
    /* Replace labels by offsets in the code */
    for (int i=0; i < instrs->s_used; i++) {
        instruction *instr = &instrs->s_instrs[i];
        if (HAS_TARGET(instr->i_opcode)) {
            assert(instr->i_oparg < instrs->s_labelmap_size);
            instr->i_oparg = instrs->s_labelmap[instr->i_oparg];
        }
        _PyExceptHandlerInfo *hi = &instr->i_except_handler_info;
        if (hi->h_label >= 0) {
            assert(hi->h_label < instrs->s_labelmap_size);
            hi->h_label = instrs->s_labelmap[hi->h_label];
        }
    }
    /* Clear label map so it's never used again */
    PyMem_Free(instrs->s_labelmap);
    instrs->s_labelmap = NULL;
    instrs->s_labelmap_size = 0;
    return SUCCESS;
}

#define MAX_OPCODE 511

int
_PyInstructionSequence_Addop(instr_sequence *seq, int opcode, int oparg,
                             location loc)
{
    assert(0 <= opcode && opcode <= MAX_OPCODE);
    assert(IS_WITHIN_OPCODE_RANGE(opcode));
    assert(OPCODE_HAS_ARG(opcode) || HAS_TARGET(opcode) || oparg == 0);
    assert(0 <= oparg && oparg < (1 << 30));

    int idx = instr_sequence_next_inst(seq);
    RETURN_IF_ERROR(idx);
    instruction *ci = &seq->s_instrs[idx];
    ci->i_opcode = opcode;
    ci->i_oparg = oparg;
    ci->i_loc = loc;
    return SUCCESS;
}

int
_PyInstructionSequence_InsertInstruction(instr_sequence *seq, int pos,
                                         int opcode, int oparg, location loc)
{
    assert(pos >= 0 && pos <= seq->s_used);
    int last_idx = instr_sequence_next_inst(seq);
    RETURN_IF_ERROR(last_idx);
    for (int i=last_idx-1; i >= pos; i--) {
        seq->s_instrs[i+1] = seq->s_instrs[i];
    }
    instruction *ci = &seq->s_instrs[pos];
    ci->i_opcode = opcode;
    ci->i_oparg = oparg;
    ci->i_loc = loc;

    /* fix the labels map */
    for(int lbl=0; lbl < seq->s_labelmap_size; lbl++) {
        if (seq->s_labelmap[lbl] >= pos) {
            seq->s_labelmap[lbl]++;
        }
    }
    return SUCCESS;
}

void
PyInstructionSequence_Fini(instr_sequence *seq) {
    PyMem_Free(seq->s_labelmap);
    seq->s_labelmap = NULL;

    PyMem_Free(seq->s_instrs);
    seq->s_instrs = NULL;
}
