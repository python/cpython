/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_NoKeywords()

PyDoc_STRVAR(inst_seq_new__doc__,
"InstructionSequenceType()\n"
"--\n"
"\n"
"Create a new InstructionSequence object.");

static PyObject *
inst_seq_new_impl(PyTypeObject *type);

static PyObject *
inst_seq_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &_PyInstructionSequence_Type;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositional("InstructionSequenceType", args)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("InstructionSequenceType", kwargs)) {
        goto exit;
    }
    return_value = inst_seq_new_impl(type);

exit:
    return return_value;
}

PyDoc_STRVAR(InstructionSequenceType_use_label__doc__,
"use_label($self, /, label)\n"
"--\n"
"\n"
"Place label at current location.");

#define INSTRUCTIONSEQUENCETYPE_USE_LABEL_METHODDEF    \
    {"use_label", _PyCFunction_CAST(InstructionSequenceType_use_label), METH_FASTCALL|METH_KEYWORDS, InstructionSequenceType_use_label__doc__},

static PyObject *
InstructionSequenceType_use_label_impl(_PyInstructionSequence *self,
                                       int label);

static PyObject *
InstructionSequenceType_use_label(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(label), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"label", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "use_label",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int label;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    label = PyLong_AsInt(args[0]);
    if (label == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = InstructionSequenceType_use_label_impl((_PyInstructionSequence *)self, label);

exit:
    return return_value;
}

PyDoc_STRVAR(InstructionSequenceType_addop__doc__,
"addop($self, /, opcode, oparg, lineno, col_offset, end_lineno,\n"
"      end_col_offset)\n"
"--\n"
"\n"
"Append an instruction.");

#define INSTRUCTIONSEQUENCETYPE_ADDOP_METHODDEF    \
    {"addop", _PyCFunction_CAST(InstructionSequenceType_addop), METH_FASTCALL|METH_KEYWORDS, InstructionSequenceType_addop__doc__},

static PyObject *
InstructionSequenceType_addop_impl(_PyInstructionSequence *self, int opcode,
                                   int oparg, int lineno, int col_offset,
                                   int end_lineno, int end_col_offset);

static PyObject *
InstructionSequenceType_addop(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(opcode), &_Py_ID(oparg), &_Py_ID(lineno), &_Py_ID(col_offset), &_Py_ID(end_lineno), &_Py_ID(end_col_offset), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", "oparg", "lineno", "col_offset", "end_lineno", "end_col_offset", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "addop",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    int opcode;
    int oparg;
    int lineno;
    int col_offset;
    int end_lineno;
    int end_col_offset;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 6, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    opcode = PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    oparg = PyLong_AsInt(args[1]);
    if (oparg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    lineno = PyLong_AsInt(args[2]);
    if (lineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    col_offset = PyLong_AsInt(args[3]);
    if (col_offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    end_lineno = PyLong_AsInt(args[4]);
    if (end_lineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    end_col_offset = PyLong_AsInt(args[5]);
    if (end_col_offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = InstructionSequenceType_addop_impl((_PyInstructionSequence *)self, opcode, oparg, lineno, col_offset, end_lineno, end_col_offset);

exit:
    return return_value;
}

PyDoc_STRVAR(InstructionSequenceType_new_label__doc__,
"new_label($self, /)\n"
"--\n"
"\n"
"Return a new label.");

#define INSTRUCTIONSEQUENCETYPE_NEW_LABEL_METHODDEF    \
    {"new_label", (PyCFunction)InstructionSequenceType_new_label, METH_NOARGS, InstructionSequenceType_new_label__doc__},

static int
InstructionSequenceType_new_label_impl(_PyInstructionSequence *self);

static PyObject *
InstructionSequenceType_new_label(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = InstructionSequenceType_new_label_impl((_PyInstructionSequence *)self);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(InstructionSequenceType_add_nested__doc__,
"add_nested($self, /, nested)\n"
"--\n"
"\n"
"Add a nested sequence.");

#define INSTRUCTIONSEQUENCETYPE_ADD_NESTED_METHODDEF    \
    {"add_nested", _PyCFunction_CAST(InstructionSequenceType_add_nested), METH_FASTCALL|METH_KEYWORDS, InstructionSequenceType_add_nested__doc__},

static PyObject *
InstructionSequenceType_add_nested_impl(_PyInstructionSequence *self,
                                        PyObject *nested);

static PyObject *
InstructionSequenceType_add_nested(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(nested), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"nested", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "add_nested",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *nested;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    nested = args[0];
    return_value = InstructionSequenceType_add_nested_impl((_PyInstructionSequence *)self, nested);

exit:
    return return_value;
}

PyDoc_STRVAR(InstructionSequenceType_get_nested__doc__,
"get_nested($self, /)\n"
"--\n"
"\n"
"Add a nested sequence.");

#define INSTRUCTIONSEQUENCETYPE_GET_NESTED_METHODDEF    \
    {"get_nested", (PyCFunction)InstructionSequenceType_get_nested, METH_NOARGS, InstructionSequenceType_get_nested__doc__},

static PyObject *
InstructionSequenceType_get_nested_impl(_PyInstructionSequence *self);

static PyObject *
InstructionSequenceType_get_nested(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return InstructionSequenceType_get_nested_impl((_PyInstructionSequence *)self);
}

PyDoc_STRVAR(InstructionSequenceType_get_instructions__doc__,
"get_instructions($self, /)\n"
"--\n"
"\n"
"Return the instructions as a list of tuples or labels.");

#define INSTRUCTIONSEQUENCETYPE_GET_INSTRUCTIONS_METHODDEF    \
    {"get_instructions", (PyCFunction)InstructionSequenceType_get_instructions, METH_NOARGS, InstructionSequenceType_get_instructions__doc__},

static PyObject *
InstructionSequenceType_get_instructions_impl(_PyInstructionSequence *self);

static PyObject *
InstructionSequenceType_get_instructions(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return InstructionSequenceType_get_instructions_impl((_PyInstructionSequence *)self);
}
/*[clinic end generated code: output=c80501a59a1a1103 input=a9049054013a1b77]*/
