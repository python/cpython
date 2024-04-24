#ifndef Py_INTERNAL_INSTRUCTION_SEQUENCE_H
#define Py_INTERNAL_INSTRUCTION_SEQUENCE_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_symtable.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    int h_label;
    int h_startdepth;
    int h_preserve_lasti;
} _PyExceptHandlerInfo;

typedef struct {
    int i_opcode;
    int i_oparg;
    _Py_SourceLocation i_loc;
    _PyExceptHandlerInfo i_except_handler_info;

    /* Temporary fields, used by the assembler and in instr_sequence_to_cfg */
    int i_target;
    int i_offset;
} _PyInstruction;

typedef struct instruction_sequence {
    PyObject_HEAD
    _PyInstruction *s_instrs;
    int s_allocated;
    int s_used;

    int s_next_free_label; /* next free label id */

    /* Map of a label id to instruction offset (index into s_instrs).
     * If s_labelmap is NULL, then each label id is the offset itself.
     */
    int *s_labelmap;
    int s_labelmap_size;

    /* PyList of instruction sequences of nested functions */
    PyObject *s_nested;
} _PyInstructionSequence;

typedef struct {
    int id;
} _PyJumpTargetLabel;

PyAPI_FUNC(PyObject*)_PyInstructionSequence_New(void);

int _PyInstructionSequence_UseLabel(_PyInstructionSequence *seq, int lbl);
int _PyInstructionSequence_Addop(_PyInstructionSequence *seq,
                                 int opcode, int oparg,
                                 _Py_SourceLocation loc);
_PyJumpTargetLabel _PyInstructionSequence_NewLabel(_PyInstructionSequence *seq);
int _PyInstructionSequence_ApplyLabelMap(_PyInstructionSequence *seq);
int _PyInstructionSequence_InsertInstruction(_PyInstructionSequence *seq, int pos,
                                             int opcode, int oparg, _Py_SourceLocation loc);
int _PyInstructionSequence_AddNested(_PyInstructionSequence *seq, _PyInstructionSequence *nested);
void PyInstructionSequence_Fini(_PyInstructionSequence *seq);

extern PyTypeObject _PyInstructionSequence_Type;
#define _PyInstructionSequence_Check(v) Py_IS_TYPE((v), &_PyInstructionSequence_Type)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_INSTRUCTION_SEQUENCE_H */
