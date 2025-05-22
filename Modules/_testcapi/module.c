#include "parts.h"
#include "util.h"

// Test PyModule_* API

static PyObject *
module_from_slots_empty(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_name(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_name, "currently ignored..."},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_doc(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_doc, "the docstring"},
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static PyObject *
module_from_slots_size(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_size, (void*)123},
        {0},
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return NULL;
    }
    Py_ssize_t size;
    if (PyModule_GetSize(mod, &size) < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    if (PyModule_AddIntConstant(mod, "size", size) < 0) {
        Py_DECREF(mod);
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
        {0},
    };
    return PyModule_FromSlotsAndSpec(slots, spec);
}

static int trivial_traverse(PyObject *self, visitproc visit, void *arg) {
    return 0;
}
static int trivial_clear(PyObject *self) { return 0; }
static void trivial_free(void *self) { }

static PyObject *
module_from_slots_gc(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_traverse, trivial_traverse},
        {Py_mod_clear, trivial_clear},
        {Py_mod_free, trivial_free},
        {0},
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return NULL;
    }
    traverseproc traverse;
    inquiry clear;
    freefunc free;
    if (_PyModule_GetGCHooks(mod, &traverse, &clear, &free) < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    assert(traverse == &trivial_traverse);
    assert(clear == &trivial_clear);
    assert(free == &trivial_free);
    return mod;
}

static char test_token;

static PyObject *
module_from_slots_token(PyObject *self, PyObject *spec)
{
    PyModuleDef_Slot slots[] = {
        {Py_mod_token, &test_token},
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
    int slot_id = PyLong_AsLong(slot_id_obj);
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
        {0},
    };
    PyModuleDef def = {
        PyModuleDef_HEAD_INIT,
        .m_name = "currently ignored",
        .m_slots = slots,
    };
    return PyModule_FromDefAndSpec(&def, spec);
}

static PyMethodDef test_methods[] = {
    {"module_from_slots_empty", module_from_slots_empty, METH_O},
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
    {"module_from_def_slot", module_from_def_slot, METH_O},
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
    ADD_INT_MACRO(Py_mod_size);
    ADD_INT_MACRO(Py_mod_methods);
    ADD_INT_MACRO(Py_mod_traverse);
    ADD_INT_MACRO(Py_mod_clear);
    ADD_INT_MACRO(Py_mod_free);
    ADD_INT_MACRO(Py_mod_token);
#undef ADD_INT_MACRO
    return PyModule_AddFunctions(m, test_methods);
}
