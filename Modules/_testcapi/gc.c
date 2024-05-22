#include "parts.h"

static PyObject*
test_gc_control(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    int orig_enabled = PyGC_IsEnabled();
    const char* msg = "ok";
    int old_state;

    old_state = PyGC_Enable();
    msg = "Enable(1)";
    if (old_state != orig_enabled) {
        goto failed;
    }
    msg = "IsEnabled(1)";
    if (!PyGC_IsEnabled()) {
        goto failed;
    }

    old_state = PyGC_Disable();
    msg = "disable(2)";
    if (!old_state) {
        goto failed;
    }
    msg = "IsEnabled(2)";
    if (PyGC_IsEnabled()) {
        goto failed;
    }

    old_state = PyGC_Enable();
    msg = "enable(3)";
    if (old_state) {
        goto failed;
    }
    msg = "IsEnabled(3)";
    if (!PyGC_IsEnabled()) {
        goto failed;
    }

    if (!orig_enabled) {
        old_state = PyGC_Disable();
        msg = "disable(4)";
        if (old_state) {
            goto failed;
        }
        msg = "IsEnabled(4)";
        if (PyGC_IsEnabled()) {
            goto failed;
        }
    }

    Py_RETURN_NONE;

failed:
    /* Try to clean up if we can. */
    if (orig_enabled) {
        PyGC_Enable();
    } else {
        PyGC_Disable();
    }
    PyErr_Format(PyExc_ValueError, "GC control failed in %s", msg);
    return NULL;
}

static PyObject *
without_gc(PyObject *Py_UNUSED(self), PyObject *obj)
{
    PyTypeObject *tp = (PyTypeObject*)obj;
    if (!PyType_Check(obj) || !PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE)) {
        return PyErr_Format(PyExc_TypeError, "heap type expected, got %R", obj);
    }
    if (PyType_IS_GC(tp)) {
        // Don't try this at home, kids:
        tp->tp_flags -= Py_TPFLAGS_HAVE_GC;
        tp->tp_free = PyObject_Del;
        tp->tp_traverse = NULL;
        tp->tp_clear = NULL;
    }
    assert(!PyType_IS_GC(tp));
    return Py_NewRef(obj);
}

static void
slot_tp_del(PyObject *self)
{
    PyObject *del, *res;

    /* Temporarily resurrect the object. */
    assert(Py_REFCNT(self) == 0);
    Py_SET_REFCNT(self, 1);

    /* Save the current exception, if any. */
    PyObject *exc = PyErr_GetRaisedException();

    PyObject *tp_del = PyUnicode_InternFromString("__tp_del__");
    if (tp_del == NULL) {
        PyErr_WriteUnraisable(NULL);
        PyErr_SetRaisedException(exc);
        return;
    }
    /* Execute __del__ method, if any. */
    del = _PyType_Lookup(Py_TYPE(self), tp_del);
    Py_DECREF(tp_del);
    if (del != NULL) {
        res = PyObject_CallOneArg(del, self);
        if (res == NULL)
            PyErr_WriteUnraisable(del);
        else
            Py_DECREF(res);
    }

    /* Restore the saved exception. */
    PyErr_SetRaisedException(exc);

    /* Undo the temporary resurrection; can't use DECREF here, it would
     * cause a recursive call.
     */
    assert(Py_REFCNT(self) > 0);
    Py_SET_REFCNT(self, Py_REFCNT(self) - 1);
    if (Py_REFCNT(self) == 0) {
        /* this is the normal path out */
        return;
    }

    /* __del__ resurrected it!  Make it look like the original Py_DECREF
     * never happened.
     */
    {
        _Py_ResurrectReference(self);
    }
    assert(!PyType_IS_GC(Py_TYPE(self)) || PyObject_GC_IsTracked(self));
}

static PyObject *
with_tp_del(PyObject *self, PyObject *args)
{
    PyObject *obj;
    PyTypeObject *tp;

    if (!PyArg_ParseTuple(args, "O:with_tp_del", &obj))
        return NULL;
    tp = (PyTypeObject *) obj;
    if (!PyType_Check(obj) || !PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE)) {
        PyErr_Format(PyExc_TypeError,
                     "heap type expected, got %R", obj);
        return NULL;
    }
    tp->tp_del = slot_tp_del;
    return Py_NewRef(obj);
}


struct gc_visit_state_basic {
    PyObject *target;
    int found;
};

static int
gc_visit_callback_basic(PyObject *obj, void *arg)
{
    struct gc_visit_state_basic *state = (struct gc_visit_state_basic *)arg;
    if (obj == state->target) {
        state->found = 1;
        return 0;
    }
    return 1;
}

static PyObject *
test_gc_visit_objects_basic(PyObject *Py_UNUSED(self),
                            PyObject *Py_UNUSED(ignored))
{
    PyObject *obj;
    struct gc_visit_state_basic state;

    obj = PyList_New(0);
    if (obj == NULL) {
        return NULL;
    }
    state.target = obj;
    state.found = 0;

    PyUnstable_GC_VisitObjects(gc_visit_callback_basic, &state);
    Py_DECREF(obj);
    if (!state.found) {
        PyErr_SetString(
             PyExc_AssertionError,
             "test_gc_visit_objects_basic: Didn't find live list");
         return NULL;
    }
    Py_RETURN_NONE;
}

static int
gc_visit_callback_exit_early(PyObject *obj, void *arg)
 {
    int *visited_i = (int *)arg;
    (*visited_i)++;
    if (*visited_i == 2) {
        return 0;
    }
    return 1;
}

static PyObject *
test_gc_visit_objects_exit_early(PyObject *Py_UNUSED(self),
                                 PyObject *Py_UNUSED(ignored))
{
    int visited_i = 0;
    PyUnstable_GC_VisitObjects(gc_visit_callback_exit_early, &visited_i);
    if (visited_i != 2) {
        PyErr_SetString(
            PyExc_AssertionError,
            "test_gc_visit_objects_exit_early: did not exit when expected");
    }
    Py_RETURN_NONE;
}

typedef struct {
    PyObject_HEAD
} ObjExtraData;

static PyObject *
obj_extra_data_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    size_t extra_size = sizeof(PyObject *);
    PyObject *obj = PyUnstable_Object_GC_NewWithExtraData(type, extra_size);
    if (obj == NULL) {
        return PyErr_NoMemory();
    }
    PyObject_GC_Track(obj);
    return obj;
}

static PyObject **
obj_extra_data_get_extra_storage(PyObject *self)
{
    return (PyObject **)((char *)self + Py_TYPE(self)->tp_basicsize);
}

static PyObject *
obj_extra_data_get(PyObject *self, void *Py_UNUSED(ignored))
{
    PyObject **extra_storage = obj_extra_data_get_extra_storage(self);
    PyObject *value = *extra_storage;
    if (!value) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(value);
}

static int
obj_extra_data_set(PyObject *self, PyObject *newval, void *Py_UNUSED(ignored))
{
    PyObject **extra_storage = obj_extra_data_get_extra_storage(self);
    Py_CLEAR(*extra_storage);
    if (newval) {
        *extra_storage = Py_NewRef(newval);
    }
    return 0;
}

static PyGetSetDef obj_extra_data_getset[] = {
    {"extra", (getter)obj_extra_data_get, (setter)obj_extra_data_set, NULL},
    {NULL}
};

static int
obj_extra_data_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyObject **extra_storage = obj_extra_data_get_extra_storage(self);
    PyObject *value = *extra_storage;
    Py_VISIT(value);
    return 0;
}

static int
obj_extra_data_clear(PyObject *self)
{
    PyObject **extra_storage = obj_extra_data_get_extra_storage(self);
    Py_CLEAR(*extra_storage);
    return 0;
}

static void
obj_extra_data_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    obj_extra_data_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyType_Slot ObjExtraData_Slots[] = {
    {Py_tp_getset, obj_extra_data_getset},
    {Py_tp_dealloc, obj_extra_data_dealloc},
    {Py_tp_traverse, obj_extra_data_traverse},
    {Py_tp_clear, obj_extra_data_clear},
    {Py_tp_new, obj_extra_data_new},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec ObjExtraData_TypeSpec = {
    .name = "_testcapi.ObjExtraData",
    .basicsize = sizeof(ObjExtraData),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = ObjExtraData_Slots,
};

static PyMethodDef test_methods[] = {
    {"test_gc_control", test_gc_control, METH_NOARGS},
    {"test_gc_visit_objects_basic", test_gc_visit_objects_basic, METH_NOARGS, NULL},
    {"test_gc_visit_objects_exit_early", test_gc_visit_objects_exit_early, METH_NOARGS, NULL},
    {"without_gc", without_gc, METH_O, NULL},
    {"with_tp_del", with_tp_del, METH_VARARGS, NULL},
    {NULL}
};

int _PyTestCapi_Init_GC(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    PyObject *ObjExtraData_Type = PyType_FromModuleAndSpec(
        mod, &ObjExtraData_TypeSpec, NULL);
    if (ObjExtraData_Type == 0) {
        return -1;
    }
    int ret = PyModule_AddType(mod, (PyTypeObject*)ObjExtraData_Type);
    Py_DECREF(ObjExtraData_Type);
    if (ret < 0) {
        return ret;
    }

    return 0;
}
