#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "context.h"
#include "module.h"

#define clinic_state() (pysqlite_get_state_by_type(type))
#include "clinic/context.c.h"
#undef clinic_state

/*[clinic input]
module _sqlite3
class _sqlite3._CallbackContext "pysqlite_CallbackContext *" "clinic_state()->CallbackContextType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f06c18b7c3666e05]*/

/*[clinic input]
@classmethod
_sqlite3._CallbackContext.__new__ as callback_context_new

    callable: object
    /

[clinic start generated code]*/

static PyObject *
callback_context_new_impl(PyTypeObject *type, PyObject *callable)
/*[clinic end generated code: output=46b4f355475d88cc input=ae33656b48f65a6d]*/
{
    PyObject *module = PyType_GetModuleByDef(type, &_sqlite3module);
    assert(module != NULL);
    pysqlite_state *state = pysqlite_get_state_by_type(type);
    assert(state != NULL);

    pysqlite_CallbackContext *ctx = PyObject_GC_New(
        pysqlite_CallbackContext,
        state->CallbackContextType
    );
    if (ctx == NULL) {
        return NULL;
    }

    // TODO(picnixz): check that 'callable' is effectively callable
    // instead of relying on a generic TypeError when attempting to
    // call it.
    ctx->callable = Py_NewRef(callable);
    ctx->module = Py_NewRef(module);
    ctx->state = state;
    PyObject_GC_Track(ctx);
    return (PyObject *)ctx;

}

static int
callback_context_clear(PyObject *op)
{
    pysqlite_CallbackContext *ctx = pysqlite_CallbackContext_CAST(op);
    Py_CLEAR(ctx->callable);
    Py_CLEAR(ctx->module);
    return 0;
}

static void
callback_context_dealloc(PyObject *op)
{
    PyTypeObject *type = Py_TYPE(op);
    PyObject_GC_UnTrack(op);
    (void)type->tp_clear(op);
    type->tp_free(op);
    Py_DECREF(type);
}

static int
callback_context_traverse(PyObject *op, visitproc visit, void *arg)
{
    pysqlite_CallbackContext *ctx = pysqlite_CallbackContext_CAST(op);
    Py_VISIT(Py_TYPE(op));
    Py_VISIT(ctx->callable);
    Py_VISIT(ctx->module);
    return 0;
}

static PyType_Slot callback_context_slots[] = {
    {Py_tp_new, callback_context_new},
    {Py_tp_clear, callback_context_clear},
    {Py_tp_dealloc, callback_context_dealloc},
    {Py_tp_traverse, callback_context_traverse},
    {0, NULL},
};

static PyType_Spec callback_context_spec = {
    .name = MODULE_NAME "._CallbackContext",
    .basicsize = sizeof(pysqlite_CallbackContext),
    .flags = (
        Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_DISALLOW_INSTANTIATION
        | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_HAVE_GC
    ),
    .slots = callback_context_slots,
};

PyObject *
pysqlite_create_callback_context(pysqlite_state *state, PyObject *callable)
{
    PyTypeObject *type = state->CallbackContextType;
    return callback_context_new_impl(type, callable);
}

int
pysqlite_context_setup_types(PyObject *module)
{
    PyObject *type = PyType_FromModuleAndSpec(module, &callback_context_spec, NULL);
    if (type == NULL) {
        return -1;
    }
    pysqlite_state *state = pysqlite_get_state(module);
    state->CallbackContextType = (PyTypeObject *)type;
    return 0;
}
