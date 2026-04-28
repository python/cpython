#define Py_LIMITED_API 0x030f0000

#include "parts.h"

PyABIInfo_VAR(abi_info);

/* Define a bunch of (mostly nonsensical) functions to put in slots, so
 * Lib/test/test_capi/test_slots.py can verify they've been assigned to
 * the right slots.

 * This module is full of "magic constants" which simply need to match
 * between the C and Python part of the tests.
 */

// getbufferproc: export buffer; increment a counter
static int
demo_getbuffer(PyObject *exporter, Py_buffer *view, int flags)
{
    Py_INCREF(exporter);
    // PyObject_GetTypeData & Py_TYPE: safe on non-subclassable type
    int *data = PyObject_GetTypeData(exporter, Py_TYPE(exporter));
    if (!data) {
        return -1;
    }
    (*data)++;
    return PyBuffer_FillInfo(view, exporter, "buf", 4, 1, flags);
}

// releasebufferproc: release buffer; decrement a counter
static void
demo_releasebuffer(PyObject *exporter, Py_buffer *view)
{
    Py_DECREF(exporter);
    // PyObject_GetTypeData & Py_TYPE: safe on non-subclassable type
    int *data = PyObject_GetTypeData(exporter, Py_TYPE(exporter));
    if (!data) {
        PyErr_WriteUnraisable(exporter);
        return;
    }
    (*data)--;
    return;
}

// objobjargproc: raise KeyError
static int
demo_ass_subscript(PyObject *o, PyObject *key, PyObject *v)
{
    PyErr_Format(PyExc_KeyError, "I don't like that key");
    return -1;
}

// lenfunc: report 456
static Py_ssize_t
demo_length(PyObject *o)
{
    return (Py_ssize_t)456;
}

// binaryfunc; return constant value
static PyObject *binop_123(PyObject* a, PyObject *b) { return PyLong_FromLong(123); }
static PyObject *binop_234(PyObject* a, PyObject *b) { return PyLong_FromLong(234); }
static PyObject *binop_345(PyObject* a, PyObject *b) { return PyLong_FromLong(345); }
static PyObject *binop_456(PyObject* a, PyObject *b) { return PyLong_FromLong(456); }
static PyObject *binop_567(PyObject* a, PyObject *b) { return PyLong_FromLong(567); }
static PyObject *binop_678(PyObject* a, PyObject *b) { return PyLong_FromLong(678); }

static PyObject *
type_from_slots(PyObject* module, PyObject *args)
{
    char *case_name;
    if (!PyArg_ParseTuple(args, "s", &case_name)) {
        return NULL;
    }
#define CASE(NAME)                                                          \
    if (strcmp(case_name, NAME) == 0) {                                     \
        return PyType_FromSlots((PySlot[]) {                                \
            PySlot_DATA(Py_tp_name, "_testlimitedcapi.MyType"),             \
            PySlot_DATA(Py_tp_module, module),                              \
    /////////////////////////////////////////////////////////////////////////
#define ENDCASE()                                                           \
            PySlot_END                                                      \
        });                                                                 \
    }                                                                       \
    /////////////////////////////////////////////////////////////////////////

    CASE("basic")
    ENDCASE()
    CASE("foreign_slot")
        PySlot_DATA(Py_mod_name, "this is not a module"),
    ENDCASE()
    CASE("basicsize")
        PySlot_SIZE(Py_tp_basicsize, 256),
    ENDCASE()
    CASE("extra_basicsize")
        PySlot_SIZE(Py_tp_extra_basicsize, 256),
    ENDCASE()
    CASE("itemsize")
        PySlot_SIZE(Py_tp_itemsize, 16),
    ENDCASE()
    CASE("flags")
        PySlot_UINT64(Py_tp_flags,
                      Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_BASETYPE),
    ENDCASE()
    CASE("matmul_123")
        PySlot_FUNC(Py_nb_matrix_multiply, binop_123),
    ENDCASE()
    CASE("optional_end")
        {.sl_flags=PySlot_OPTIONAL},
    ENDCASE()
    CASE("invalid")
        {.sl_id=Py_slot_invalid},
    ENDCASE()
    CASE("invalid_fbad")
        {.sl_id=0xfbad},
    ENDCASE()
    CASE("optional_invalid")
        {.sl_id=Py_slot_invalid, .sl_flags=PySlot_OPTIONAL},
        PySlot_SIZE(Py_tp_extra_basicsize, 256),
    ENDCASE()
    CASE("optional_invalid_fbad")
        {.sl_id=0xfbad, .sl_flags=PySlot_OPTIONAL},
        PySlot_SIZE(Py_tp_extra_basicsize, 256),
    ENDCASE()
    CASE("old_slot_numbers")
        PySlot_FUNC(1, demo_getbuffer),
        PySlot_FUNC(2, demo_releasebuffer),
        PySlot_FUNC(3, demo_ass_subscript),
        PySlot_FUNC(4, demo_length),
        PySlot_SIZE(Py_tp_extra_basicsize, sizeof(int)),
        PySlot_STATIC_DATA(Py_tp_members, ((PyMemberDef[]) {
            {"buf_counter", Py_T_INT, 0, Py_READONLY | Py_RELATIVE_OFFSET},
            {NULL},
        })),
    ENDCASE()
    CASE("new_slot_numbers")
        PySlot_FUNC(88, demo_getbuffer),
        PySlot_FUNC(89, demo_releasebuffer),
        PySlot_FUNC(90, demo_ass_subscript),
        PySlot_FUNC(91, demo_length),
        PySlot_SIZE(Py_tp_extra_basicsize, sizeof(int)),
        PySlot_STATIC_DATA(Py_tp_members, ((PyMemberDef[]) {
            {"buf_counter", Py_T_INT, 0, Py_READONLY | Py_RELATIVE_OFFSET},
            {NULL},
        })),
    ENDCASE()
    CASE("nonstatic_tp_members")
        PySlot_SIZE(Py_tp_extra_basicsize, sizeof(int)),
        PySlot_DATA(Py_tp_members, ((PyMemberDef[]) {
            {"buf_counter", Py_T_INT, 0, Py_READONLY | Py_RELATIVE_OFFSET},
            {NULL},
        })),
    ENDCASE()
    CASE("intptr_flags_macro")
        PySlot_PTR(Py_tp_flags, (void*)(intptr_t)Py_TPFLAGS_IMMUTABLETYPE),
    ENDCASE()
    CASE("intptr_flags_struct")
        {.sl_id=Py_tp_flags,
         .sl_flags=PySlot_INTPTR,
         .sl_ptr=(void*)(intptr_t)Py_TPFLAGS_IMMUTABLETYPE,
         },
    ENDCASE()
    CASE("intptr_static")
        PySlot_SIZE(Py_tp_extra_basicsize, sizeof(int)),
        PySlot_PTR_STATIC(Py_tp_members, ((PyMemberDef[]) {
            {"attribute", Py_T_INT, 0, Py_READONLY | Py_RELATIVE_OFFSET},
            {NULL},
        })),
    ENDCASE()
    CASE("nested")
        PySlot_FUNC(Py_nb_add, binop_123),
        PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
            PySlot_FUNC(Py_nb_subtract, binop_234),
            PySlot_END,
        })),
    ENDCASE()
    CASE("nested_max")
        PySlot_FUNC(Py_nb_add, binop_123),
        PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
            PySlot_FUNC(Py_nb_subtract, binop_234),
            PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                PySlot_FUNC(Py_nb_multiply, binop_345),
                PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                    PySlot_FUNC(Py_nb_true_divide, binop_456),
                    PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                        PySlot_FUNC(Py_nb_remainder, binop_567),
                        PySlot_END
                    })),
                    PySlot_END,
                })),
                PySlot_END,
            })),
            PySlot_END,
        })),
    ENDCASE()
    CASE("nested_over_limit")
        PySlot_FUNC(Py_nb_add, binop_123),
        PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
            PySlot_FUNC(Py_nb_subtract, binop_234),
            PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                PySlot_FUNC(Py_nb_multiply, binop_345),
                PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                    PySlot_FUNC(Py_nb_true_divide, binop_456),
                    PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                        PySlot_FUNC(Py_nb_remainder, binop_567),
                        PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                            PySlot_FUNC(Py_nb_xor, binop_678),
                            PySlot_END
                        })),
                        PySlot_END
                    })),
                    PySlot_END,
                })),
                PySlot_END,
            })),
            PySlot_END,
        })),
    ENDCASE()
    CASE("nested_old")
        PySlot_FUNC(Py_nb_add, binop_123),
        PySlot_DATA(Py_tp_slots, ((PyType_Slot[]) {
            {Py_nb_subtract, binop_234},
            {0},
        })),
    ENDCASE()
    CASE("nested_old_max")
        PySlot_FUNC(Py_nb_add, binop_123),
        PySlot_DATA(Py_tp_slots, ((PyType_Slot[]) {
            {Py_nb_subtract, binop_234},
            {Py_tp_slots, ((PyType_Slot[]) {
                {Py_nb_multiply, binop_345},
                {Py_tp_slots, ((PyType_Slot[]) {
                    {Py_nb_true_divide, binop_456},
                    {Py_tp_slots, ((PyType_Slot[]) {
                        {Py_nb_remainder, binop_567},
                        {0},
                    })},
                    {0},
                })},
                {0},
            })},
            {0},
        })),
    ENDCASE()
    CASE("nested_old_over_limit")
        PySlot_FUNC(Py_nb_add, binop_123),
        PySlot_DATA(Py_tp_slots, ((PyType_Slot[]) {
            {Py_nb_subtract, binop_234},
            {Py_tp_slots, ((PyType_Slot[]) {
                {Py_nb_multiply, binop_345},
                {Py_tp_slots, ((PyType_Slot[]) {
                    {Py_nb_true_divide, binop_456},
                    {Py_tp_slots, ((PyType_Slot[]) {
                        {Py_nb_remainder, binop_567},
                        {Py_tp_slots, ((PyType_Slot[]) {
                            {Py_nb_xor, binop_678},
                            {0},
                        })},
                        {0},
                    })},
                    {0},
                })},
                {0},
            })},
            {0},
        })),
    ENDCASE()
    CASE("nested_pingpong")
        PySlot_FUNC(Py_nb_add, binop_123),
        PySlot_DATA(Py_tp_slots, ((PyType_Slot[]) {
            {Py_nb_subtract, binop_234},
            {Py_slot_subslots, ((PySlot[]) {
                PySlot_FUNC(Py_nb_multiply, binop_345),
                PySlot_DATA(Py_tp_slots, ((PyType_Slot[]) {
                    {Py_nb_true_divide, binop_456},
                    {Py_slot_subslots, ((PySlot[]) {
                        PySlot_FUNC(Py_nb_remainder, binop_567),
                        PySlot_END
                    })},
                    {0},
                })),
                PySlot_END,
            })},
            {0},
        })),
    ENDCASE()
    CASE("repeat_add")
        PySlot_FUNC(Py_nb_add, binop_123),
        PySlot_FUNC(Py_nb_add, binop_456),
    ENDCASE()
    CASE("repeat_module")
        PySlot_DATA(Py_tp_module, Py_True),
        PySlot_DATA(Py_tp_module, Py_False),
    ENDCASE()

#undef CASE
#undef ENDCASE
    PyErr_Format(PyExc_AssertionError, "bad case: %s", case_name);
    return NULL;
}

static PyObject *
type_from_null_slot(PyObject* module, PyObject *args)
{
    long slot_number;
    if (!PyArg_ParseTuple(args, "l", &slot_number)) {
        return NULL;
    }
    return PyType_FromSlots((PySlot[]) {
        PySlot_DATA(Py_tp_name, "_testlimitedcapi.MyType"),
        PySlot_DATA(Py_tp_module, module),
        PySlot_PTR_STATIC((uint16_t)slot_number, NULL),
        PySlot_END
    });
}

static PyObject *
type_from_null_spec_slot(PyObject* Py_UNUSED(module), PyObject *args)
{
    long slot_number;
    if (!PyArg_ParseTuple(args, "l", &slot_number)) {
        return NULL;
    }
    return PyType_FromSpec(&(PyType_Spec) {
        .name = "_testlimitedcapi.MyType",
        .slots = (PyType_Slot[]) {
            {slot_number, NULL},
            {0},
        },
    });
}

static PyObject *
demo_create(PyObject *spec, PyModuleDef *def)
{
    assert(def == NULL);
    return Py_NewRef(spec);
}

static int
demo_exec(PyObject *mod)
{
    return PyModule_AddStringConstant(mod, "exec_done", "yes");
}

static PyMethodDef *TestMethods;

static PyObject *
module_from_slots(PyObject* Py_UNUSED(module), PyObject *args)
{
    PyObject *spec;
    char *case_name;
    if (!PyArg_ParseTuple(args, "sO", &case_name, &spec)) {
        return NULL;
    }
    PyObject *mod = NULL;
#define CASE(NAME)                                                          \
    if (strcmp(case_name, NAME) == 0) {                                     \
        mod = PyModule_FromSlotsAndSpec((PySlot[]) {                        \
            PySlot_DATA(Py_mod_abi, &abi_info),                             \
            PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),                   \
    /////////////////////////////////////////////////////////////////////////
#define ENDCASE()                                                           \
            PySlot_END                                                      \
        }, spec);                                                           \
    }                                                                       \
    /////////////////////////////////////////////////////////////////////////

    CASE("basic")
    ENDCASE()
    CASE("foreign_slot")
        PySlot_DATA(Py_tp_name, "this is not a type"),
    ENDCASE()
    CASE("state_size")
        PySlot_SIZE(Py_mod_state_size, 42),
    ENDCASE()
    CASE("multi_interp")
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
    ENDCASE()
    CASE("exec")
        PySlot_FUNC(Py_mod_exec, demo_exec),
    ENDCASE()
    CASE("optional_end")
        {.sl_flags=PySlot_OPTIONAL},
    ENDCASE()
    CASE("invalid")
        {.sl_id=Py_slot_invalid},
    ENDCASE()
    CASE("invalid_fbad")
        {.sl_id=0xfbad},
    ENDCASE()
    CASE("optional_invalid")
        {.sl_id=Py_slot_invalid, .sl_flags=PySlot_OPTIONAL},
        PySlot_SIZE(Py_mod_exec, demo_exec),
    ENDCASE()
    CASE("optional_invalid_fbad")
        {.sl_id=0xfbad, .sl_flags=PySlot_OPTIONAL},
        PySlot_SIZE(Py_mod_exec, demo_exec),
    ENDCASE()
    CASE("old_slot_numbers")
        // 1: see old_slot_number_create case
        PySlot_FUNC(2, demo_exec),
        PySlot_DATA(3, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        // 4: see module_from_gil_slot function
    ENDCASE()
    CASE("new_slot_numbers")
        // 84: see new_slot_number_create case
        PySlot_FUNC(85, demo_exec),
        PySlot_FUNC(86, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        // 87: see module_from_gil_slot function
    ENDCASE()
    CASE("old_slot_number_create")
        PySlot_FUNC(1, demo_create),
    ENDCASE()
    CASE("new_slot_number_create")
        PySlot_FUNC(84, demo_create),
    ENDCASE()
    CASE("nonstatic_mod_methods")
        PySlot_DATA(Py_mod_methods, TestMethods),
    ENDCASE()
    CASE("intptr_methods")
        PySlot_PTR_STATIC(Py_mod_methods, TestMethods),
    ENDCASE()
    CASE("nested")
        PySlot_FUNC(Py_mod_exec, demo_exec),
        PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
            PySlot_DATA(Py_mod_doc, "doc"),
            PySlot_END,
        })),
    ENDCASE()
    CASE("nested_max")
        PySlot_FUNC(Py_mod_exec, demo_exec),
        PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
            PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                    PySlot_SIZE(Py_mod_state_size, 53),
                    PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                        PySlot_DATA(Py_mod_doc, "doc"),
                        PySlot_END
                    })),
                    PySlot_END,
                })),
                PySlot_END,
            })),
            PySlot_END,
        })),
    ENDCASE()
    CASE("nested_over_limit")
        PySlot_FUNC(Py_mod_exec, demo_exec),
        PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
            PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                    PySlot_SIZE(Py_mod_state_size, 53),
                    PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                        PySlot_DATA(Py_mod_doc, "doc"),
                        PySlot_DATA(Py_slot_subslots, ((PySlot[]) {
                            PySlot_END
                        })),
                        PySlot_END
                    })),
                    PySlot_END,
                })),
                PySlot_END,
            })),
            PySlot_END,
        })),
    ENDCASE()
    CASE("nested_old")
        PySlot_FUNC(Py_mod_exec, demo_exec),
        PySlot_DATA(Py_mod_slots, ((PyModuleDef_Slot[]) {
            {Py_mod_doc, "doc"},
            {0},
        })),
    ENDCASE()
    CASE("nested_old_max")
        PySlot_FUNC(Py_mod_exec, demo_exec),
        PySlot_DATA(Py_mod_slots, ((PyModuleDef_Slot[]) {
            {Py_mod_slots, ((PyModuleDef_Slot[]) {
                {Py_mod_state_size, (void*)(intptr_t)53},
                {Py_mod_slots, ((PyModuleDef_Slot[]) {
                    {Py_mod_slots, ((PyModuleDef_Slot[]) {
                        {Py_mod_doc, "doc"},
                        {0},
                    })},
                    {0},
                })},
                {0},
            })},
            {0},
        })),
    ENDCASE()
    CASE("nested_old_over_limit")
        PySlot_FUNC(Py_mod_exec, demo_exec),
        PySlot_DATA(Py_mod_slots, ((PyModuleDef_Slot[]) {
            {Py_mod_slots, ((PyModuleDef_Slot[]) {
                {Py_mod_state_size, (void*)(intptr_t)53},
                {Py_mod_slots, ((PyModuleDef_Slot[]) {
                    {Py_mod_slots, ((PyModuleDef_Slot[]) {
                        {Py_mod_slots, ((PyModuleDef_Slot[]) {
                            {Py_mod_doc, "doc"},
                            {0},
                        })},
                        {0},
                    })},
                    {0},
                })},
                {0},
            })},
            {0},
        })),
    ENDCASE()
    CASE("nested_pingpong")
        PySlot_FUNC(Py_mod_exec, demo_exec),
        PySlot_DATA(Py_mod_slots, ((PyModuleDef_Slot[]) {
            {Py_slot_subslots, ((PySlot[]) {
                PySlot_DATA(Py_mod_slots, ((PyModuleDef_Slot[]) {
                    {Py_mod_state_size, (void*)(intptr_t)53},
                    {Py_slot_subslots, ((PySlot[]) {
                        PySlot_DATA(Py_mod_doc, "doc"),
                        PySlot_END
                    })},
                    {0},
                })),
                PySlot_END,
            })},
            {0},
        })),
    ENDCASE()
    CASE("repeat_create")
        PySlot_DATA(Py_mod_create, demo_create),
        PySlot_DATA(Py_mod_create, demo_create),
        PySlot_DATA(Py_mod_create, demo_create),
    ENDCASE()
    CASE("repeat_gil")
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
    ENDCASE()
    CASE("repeat_exec")
        PySlot_FUNC(Py_mod_exec, demo_exec),
        PySlot_FUNC(Py_mod_exec, demo_exec),
    ENDCASE()

#undef CASE
#undef ENDCASE
    if (!mod) {
        if (!PyErr_Occurred()) {
            PyErr_Format(PyExc_AssertionError, "bad case: %s", case_name);
            return NULL;
        }
        return NULL;
    }
    if (PyModule_Check(mod)) {
        Py_ssize_t size;
        if (PyModule_GetStateSize(mod, &size) < 0) {
            Py_DECREF(mod);
            return NULL;
        }
        if (PyModule_AddIntConstant(mod, "state_size", (long)size) < 0) {
            Py_DECREF(mod);
            return NULL;
        }
        if (PyModule_Exec(mod) < 0) {
            return NULL;
        }
    }
    return mod;
}

static PyObject *
module_from_gil_slot(PyObject* Py_UNUSED(module), PyObject *args)
{
    long slot_number;
    PyObject *spec;
    if (!PyArg_ParseTuple(args, "lO", &slot_number, &spec)) {
        return NULL;
    }
    return PyModule_FromSlotsAndSpec((PySlot[]) {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_PTR_STATIC((uint16_t)slot_number, Py_MOD_GIL_NOT_USED),
        PySlot_END
    }, spec);
}

static PyObject *
module_from_null_slot(PyObject* Py_UNUSED(module), PyObject *args)
{
    long slot_number;
    PyObject *spec;
    if (!PyArg_ParseTuple(args, "lO", &slot_number, &spec)) {
        return NULL;
    }
    uint16_t maybe_gil_slot = Py_mod_gil;
    if ((slot_number == 4) || (slot_number == 87)) {
        // Do not repeat the GIL slot
        maybe_gil_slot = Py_slot_invalid;
    }
    return PyModule_FromSlotsAndSpec((PySlot[]) {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_DATA(Py_mod_name, "mymod"),
        PySlot_PTR_STATIC((uint16_t)slot_number, NULL),
        {
            .sl_id=maybe_gil_slot,
            .sl_flags=PySlot_OPTIONAL,
            .sl_ptr=Py_MOD_GIL_NOT_USED,
        },
        PySlot_END
    }, spec);
}

static PyMethodDef _TestMethods[] = {
    {"type_from_slots", type_from_slots, METH_VARARGS},
    {"module_from_gil_slot", module_from_gil_slot, METH_VARARGS},
    {"type_from_null_slot", type_from_null_slot, METH_VARARGS},
    {"type_from_null_spec_slot", type_from_null_spec_slot, METH_VARARGS},
    {"module_from_slots", module_from_slots, METH_VARARGS},
    {"module_from_null_slot", module_from_null_slot, METH_VARARGS},
    {NULL},
};
static PyMethodDef *TestMethods = _TestMethods;

int
_PyTestLimitedCAPI_Init_Slots(PyObject *m)
{
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    return 0;
}
