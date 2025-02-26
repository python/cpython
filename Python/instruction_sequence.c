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

#include "clinic/instruction_sequence.c.h"

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

int
_PyInstructionSequence_AddNested(instr_sequence *seq, instr_sequence *nested)
{
    if (seq->s_nested == NULL) {
        seq->s_nested = PyList_New(0);
        if (seq->s_nested == NULL) {
            return ERROR;
        }
    }
    if (PyList_Append(seq->s_nested, (PyObject*)nested) < 0) {
        return ERROR;
    }
    return SUCCESS;
}

void
PyInstructionSequence_Fini(instr_sequence *seq) {
    Py_XDECREF(seq->s_nested);

    PyMem_Free(seq->s_labelmap);
    seq->s_labelmap = NULL;

    PyMem_Free(seq->s_instrs);
    seq->s_instrs = NULL;
}

/*[clinic input]
class InstructionSequenceType "_PyInstructionSequence *" "&_PyInstructionSequence_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=589963e07480390f]*/

static _PyInstructionSequence*
inst_seq_create(void)
{
    _PyInstructionSequence *seq;
    seq = PyObject_GC_New(_PyInstructionSequence, &_PyInstructionSequence_Type);
    if (seq == NULL) {
        return NULL;
    }
    seq->s_instrs = NULL;
    seq->s_allocated = 0;
    seq->s_used = 0;
    seq->s_next_free_label = 0;
    seq->s_labelmap = NULL;
    seq->s_labelmap_size = 0;
    seq->s_nested = NULL;

    PyObject_GC_Track(seq);
    return seq;
}

PyObject*
_PyInstructionSequence_New(void)
{
    _PyInstructionSequence *seq = inst_seq_create();
    if (seq == NULL) {
        return NULL;
    }
    return (PyObject*)seq;
}

/*[clinic input]
@classmethod
InstructionSequenceType.__new__ as inst_seq_new

Create a new InstructionSequence object.
[clinic start generated code]*/

static PyObject *
inst_seq_new_impl(PyTypeObject *type)
/*[clinic end generated code: output=98881de92c8876f6 input=b393150146849c74]*/
{
    return (PyObject*)inst_seq_create();
}

/*[clinic input]
InstructionSequenceType.use_label

  label: int

Place label at current location.
[clinic start generated code]*/

static PyObject *
InstructionSequenceType_use_label_impl(_PyInstructionSequence *self,
                                       int label)
/*[clinic end generated code: output=4c06bbacb2854755 input=da55f49bb91841f3]*/

{
    if (_PyInstructionSequence_UseLabel(self, label) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
InstructionSequenceType.addop

  opcode: int
  oparg: int
  lineno: int
  col_offset: int
  end_lineno: int
  end_col_offset: int

Append an instruction.
[clinic start generated code]*/

static PyObject *
InstructionSequenceType_addop_impl(_PyInstructionSequence *self, int opcode,
                                   int oparg, int lineno, int col_offset,
                                   int end_lineno, int end_col_offset)
/*[clinic end generated code: output=af0cc22c048dfbf3 input=012762ac88198713]*/
{
    _Py_SourceLocation loc = {lineno, col_offset, end_lineno, end_col_offset};
    if (_PyInstructionSequence_Addop(self, opcode, oparg, loc) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
InstructionSequenceType.new_label -> int

Return a new label.
[clinic start generated code]*/

static int
InstructionSequenceType_new_label_impl(_PyInstructionSequence *self)
/*[clinic end generated code: output=dcb0589e4f5bf4bd input=c66040b9897bc327]*/
{
    _PyJumpTargetLabel lbl = _PyInstructionSequence_NewLabel(self);
    return lbl.id;
}

/*[clinic input]
InstructionSequenceType.add_nested

  nested: object

Add a nested sequence.
[clinic start generated code]*/

static PyObject *
InstructionSequenceType_add_nested_impl(_PyInstructionSequence *self,
                                        PyObject *nested)
/*[clinic end generated code: output=14540fad459f7971 input=f2c482568b3b3c0f]*/
{
    if (!_PyInstructionSequence_Check(nested)) {
        PyErr_Format(PyExc_TypeError,
                     "expected an instruction sequence, not %T",
                     Py_TYPE(nested));
        return NULL;
    }
    if (_PyInstructionSequence_AddNested(self, (_PyInstructionSequence*)nested) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
InstructionSequenceType.get_nested

Add a nested sequence.
[clinic start generated code]*/

static PyObject *
InstructionSequenceType_get_nested_impl(_PyInstructionSequence *self)
/*[clinic end generated code: output=f415112c292630cb input=e429e474c57b95b4]*/
{
    if (self->s_nested == NULL) {
        return PyList_New(0);
    }
    return Py_NewRef(self->s_nested);
}

/*[clinic input]
InstructionSequenceType.get_instructions

Return the instructions as a list of tuples or labels.
[clinic start generated code]*/

static PyObject *
InstructionSequenceType_get_instructions_impl(_PyInstructionSequence *self)
/*[clinic end generated code: output=23f4f3f894c301b3 input=fbadb5dadb611291]*/
{
    if (_PyInstructionSequence_ApplyLabelMap(self) < 0) {
        return NULL;
    }
    PyObject *instructions = PyList_New(0);
    if (instructions == NULL) {
        return NULL;
    }
    for (int i = 0; i < self->s_used; i++) {
        instruction *instr = &self->s_instrs[i];
        location loc = instr->i_loc;
        PyObject *inst_tuple;

        if (OPCODE_HAS_ARG(instr->i_opcode)) {
            inst_tuple = Py_BuildValue(
                "(iiiiii)", instr->i_opcode, instr->i_oparg,
                loc.lineno, loc.end_lineno,
                loc.col_offset, loc.end_col_offset);
        }
        else {
            inst_tuple = Py_BuildValue(
                "(iOiiii)", instr->i_opcode, Py_None,
                loc.lineno, loc.end_lineno,
                loc.col_offset, loc.end_col_offset);
        }
        if (inst_tuple == NULL) {
            goto error;
        }

        int res = PyList_Append(instructions, inst_tuple);
        Py_DECREF(inst_tuple);
        if (res != 0) {
            goto error;
        }
    }
    return instructions;
error:
    Py_XDECREF(instructions);
    return NULL;
}

static PyMethodDef inst_seq_methods[] = {
   INSTRUCTIONSEQUENCETYPE_ADDOP_METHODDEF
   INSTRUCTIONSEQUENCETYPE_NEW_LABEL_METHODDEF
   INSTRUCTIONSEQUENCETYPE_USE_LABEL_METHODDEF
   INSTRUCTIONSEQUENCETYPE_ADD_NESTED_METHODDEF
   INSTRUCTIONSEQUENCETYPE_GET_NESTED_METHODDEF
   INSTRUCTIONSEQUENCETYPE_GET_INSTRUCTIONS_METHODDEF
   {NULL, NULL, 0, NULL},
};

static PyMemberDef inst_seq_memberlist[] = {
    {NULL}      /* Sentinel */
};

static PyGetSetDef inst_seq_getsetters[] = {
    {NULL}      /* Sentinel */
};

static void
inst_seq_dealloc(PyObject *op)
{
    _PyInstructionSequence *seq = (_PyInstructionSequence *)op;
    PyObject_GC_UnTrack(seq);
    Py_TRASHCAN_BEGIN(seq, inst_seq_dealloc)
    PyInstructionSequence_Fini(seq);
    PyObject_GC_Del(seq);
    Py_TRASHCAN_END
}

static int
inst_seq_traverse(PyObject *op, visitproc visit, void *arg)
{
    _PyInstructionSequence *seq = (_PyInstructionSequence *)op;
    Py_VISIT(seq->s_nested);
    return 0;
}

static int
inst_seq_clear(PyObject *op)
{
    _PyInstructionSequence *seq = (_PyInstructionSequence *)op;
    Py_CLEAR(seq->s_nested);
    return 0;
}

PyTypeObject _PyInstructionSequence_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "InstructionSequence",
    sizeof(_PyInstructionSequence),
    0,
    inst_seq_dealloc,   /*tp_dealloc*/
    0,                  /*tp_vectorcall_offset*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_as_async*/
    0,                  /*tp_repr*/
    0,                  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /* tp_hash */
    0,                  /* tp_call */
    0,                  /* tp_str */
    PyObject_GenericGetAttr,  /* tp_getattro */
    0,                  /* tp_setattro */
    0,                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    inst_seq_new__doc__,                    /* tp_doc */
    inst_seq_traverse,                      /* tp_traverse */
    inst_seq_clear,                         /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    0,                                      /* tp_iter */
    0,                                      /* tp_iternext */
    inst_seq_methods,                       /* tp_methods */
    inst_seq_memberlist,                    /* tp_members */
    inst_seq_getsetters,                    /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    0,                                      /* tp_init */
    0,                                      /* tp_alloc */
    inst_seq_new,                           /* tp_new */
};
