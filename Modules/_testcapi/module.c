#include "parts.h"
#include "util.h"

// Test PyModule_* API

/* unittest Cases that use these functions are in:
 * Lib/test/test_capi/test_module.py
 */

static PyObject *
module_from_slots_empty(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_null(PyObject *self, PyObject *spec)
{
    return PyModule_FromSlotsAndSpec(NULL, spec);
}

static PyObject *
module_from_slots_name(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_name, "currently ignored..."},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_doc(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_doc, "the docstring"},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_size(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_state_size, (void*)123},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return NULL;
    }
    return mod;
}

static PyObject *
a_method(PyObject *self, PyObject *arg)
{
    return PyTuple_Pack(2, self, arg);
}

static PyMethodDef a_methoddef_array[] = {
    {"a_method", a_method, METH_O},
    {0},
};

static PyObject *
module_from_slots_methods(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_methods, a_methoddef_array},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static int noop_traverse(PyObject *self, visitproc visit, void *arg) {
    return 0;
}
static int noop_clear(PyObject *self) { return 0; }
static void noop_free(void *self) { }

static PyObject *
module_from_slots_gc(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_state_traverse, noop_traverse},
        {Py_mod_state_clear, noop_clear},
        {Py_mod_state_free, noop_free},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return NULL;
    }
    if (PyModule_Add(mod, "traverse", PyLong_FromVoidPtr(&noop_traverse)) < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    if (PyModule_Add(mod, "clear", PyLong_FromVoidPtr(&noop_clear)) < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    if (PyModule_Add(mod, "free", PyLong_FromVoidPtr(&noop_free)) < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    return mod;
}

static const char test_token;

static PyObject *
module_from_slots_token(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_token, (void*)&test_token},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return NULL;
    }
    void *got_token;
    if (PyModule_GetToken(mod, &got_token) < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    assert(got_token == &test_token);
    return mod;
}

static int
simple_exec(PyObject *module)
{
    return PyModule_AddIntConstant(module, "a_number", 456);
}

static PyObject *
module_from_slots_exec(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_exec, simple_exec},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return NULL;
    }
    int res = PyObject_HasAttrStringWithError(mod, "a_number");
    if (res < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    assert(res == 0);
    if (PyModule_Exec(mod) < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    return mod;
}

static PyObject *
create_attr_from_spec(PyObject *spec, PyObject *def)
{
    assert(!def);
    return PyObject_GetAttrString(spec, "_gimme_this");
}

static PyObject *
module_from_slots_create(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_create, create_attr_from_spec},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}


static int
slot_from_object(PyObject *obj)
{
    PyObject *slot_id_obj = PyObject_GetAttrString(obj, "_test_slot_id");
    if (slot_id_obj == NULL) {
        return -1;
    }
    int slot_id = PyLong_AsInt(slot_id_obj);
    if (PyErr_Occurred()) {
        return -1;
    }
    return slot_id;
}

static PyObject *
module_from_slots_repeat_slot(PyObject *self, PyObject *spec)
{
    int slot_id = slot_from_object(spec);
    if (slot_id < 0) {
        return NULL;
    }
    PyModuleDef_Slot slots[] = {
        {slot_id, "anything"},
        {slot_id, "anything else"},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_null_slot(PyObject *self, PyObject *spec)
{
    int slot_id = slot_from_object(spec);
    if (slot_id < 0) {
        return NULL;
    }
    PyModuleDef_Slot slots[] = {
        {slot_id, NULL},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_def_slot(PyObject *self, PyObject *spec)
{
    int slot_id = slot_from_object(spec);
    if (slot_id < 0) {
        return NULL;
    }
    PyModuleDef_Slot slots[] = {
        {slot_id, "anything"},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    PyModuleDef def = {
        PyModuleDef_HEAD_INIT,
        .m_name = "currently ignored",
        .m_slots = slots,
    };
    // PyModuleDef is normally static; the real requirement is that it
    // must outlive its module.
    // Here, module creation fails, so it's fine on the stack.
    PyObject *result = PyModule_FromDefAndSpec(&def, spec);
    assert(result == NULL);
    return result;
}

static int
another_exec(PyObject *module)
{
    /* Make sure simple_exec was called */
    assert(PyObject_HasAttrString(module, "a_number"));

    /* Add or negate a global called 'another_number'  */
    PyObject *another_number;
    if (PyObject_GetOptionalAttrString(module, "another_number",
                                       &another_number) < 0) {
        return -1;
    }
    if (!another_number) {
        return PyModule_AddIntConstant(module, "another_number", 789);
    }
    PyObject *neg_number = PyNumber_Negative(another_number);
    Py_DECREF(another_number);
    if (!neg_number) {
        return -1;
    }
    int result = PyObject_SetAttrString(module, "another_number",
                                        neg_number);
    Py_DECREF(neg_number);
    return result;
}

static PyObject *
module_from_def_multiple_exec(PyObject *self, PyObject *spec)
{
    static PyModuleDef_Slot slots[] = {
        {Py_mod_exec, simple_exec},
        {Py_mod_exec, another_exec},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    static PyModuleDef def = {
        PyModuleDef_HEAD_INIT,
        .m_name = "currently ignored",
        .m_slots = slots,
    };
    return PyModule_FromDefAndSpec(&def, spec);
}

static PyObject *
pymodule_exec(PyObject *self, PyObject *module)
{
    if (PyModule_Exec(module) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
pymodule_get_token(PyObject *self, PyObject *module)
{
    void *token;
    if (PyModule_GetToken(module, &token) < 0) {
        return NULL;
    }
    return PyLong_FromVoidPtr(token);
}

static PyObject *
pymodule_get_def(PyObject *self, PyObject *module)
{
    PyModuleDef *def = PyModule_GetDef(module);
    if (!def && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromVoidPtr(def);
}

static PyObject *
pymodule_get_state_size(PyObject *self, PyObject *module)
{
    Py_ssize_t size;
    if (PyModule_GetStateSize(module, &size) < 0) {
        return NULL;
    }
    return PyLong_FromSsize_t(size);
}

static PyMethodDef test_methods[] = {
    {"module_from_slots_empty", module_from_slots_empty, METH_O},
    {"module_from_slots_null", module_from_slots_null, METH_O},
    {"module_from_slots_name", module_from_slots_name, METH_O},
    {"module_from_slots_doc", module_from_slots_doc, METH_O},
    {"module_from_slots_size", module_from_slots_size, METH_O},
    {"module_from_slots_methods", module_from_slots_methods, METH_O},
    {"module_from_slots_gc", module_from_slots_gc, METH_O},
    {"module_from_slots_token", module_from_slots_token, METH_O},
    {"module_from_slots_exec", module_from_slots_exec, METH_O},
    {"module_from_slots_create", module_from_slots_create, METH_O},
    {"module_from_slots_repeat_slot", module_from_slots_repeat_slot, METH_O},
    {"module_from_slots_null_slot", module_from_slots_null_slot, METH_O},
    {"module_from_def_multiple_exec", module_from_def_multiple_exec, METH_O},
    {"module_from_def_slot", module_from_def_slot, METH_O},
    {"pymodule_get_token", pymodule_get_token, METH_O},
    {"pymodule_get_def", pymodule_get_def, METH_O},
    {"pymodule_get_state_size", pymodule_get_state_size, METH_O},
    {"pymodule_exec", pymodule_exec, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Module(PyObject *m)
{
#define ADD_INT_MACRO(C) if (PyModule_AddIntConstant(m, #C, C) < 0) return -1;
    ADD_INT_MACRO(Py_mod_create);
    ADD_INT_MACRO(Py_mod_exec);
    ADD_INT_MACRO(Py_mod_multiple_interpreters);
    ADD_INT_MACRO(Py_mod_gil);
    ADD_INT_MACRO(Py_mod_name);
    ADD_INT_MACRO(Py_mod_doc);
    ADD_INT_MACRO(Py_mod_state_size);
    ADD_INT_MACRO(Py_mod_methods);
    ADD_INT_MACRO(Py_mod_state_traverse);
    ADD_INT_MACRO(Py_mod_state_clear);
    ADD_INT_MACRO(Py_mod_state_free);
    ADD_INT_MACRO(Py_mod_token);
#undef ADD_INT_MACRO
    if (PyModule_Add(m, "module_test_token",
                     PyLong_FromVoidPtr((void*)&test_token)) < 0)
    {
        return -1;
    }
    return PyModule_AddFunctions(m, test_methods);
}
