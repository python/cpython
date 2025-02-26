/* Frame object implementation */

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_BuiltinsFromGlobals()
#include "pycore_code.h"          // CO_FAST_LOCAL, etc.
#include "pycore_function.h"      // _PyFunction_FromConstructor()
#include "pycore_moduleobject.h"  // _PyModule_GetDict()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_opcode_metadata.h" // _PyOpcode_Deopt, _PyOpcode_Caches


#include "frameobject.h"          // PyFrameObject
#include "pycore_frame.h"
#include "opcode.h"               // EXTENDED_ARG


#define OFF(x) offsetof(PyFrameObject, x)


// Returns borrowed reference or NULL
static PyObject *
framelocalsproxy_getval(_PyInterpreterFrame *frame, PyCodeObject *co, int i)
{
    PyObject **fast = _PyFrame_GetLocalsArray(frame);
    _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);

    PyObject *value = fast[i];
    PyObject *cell = NULL;

    if (value == NULL) {
        return NULL;
    }

    if (kind == CO_FAST_FREE || kind & CO_FAST_CELL) {
        // The cell was set when the frame was created from
        // the function's closure.
        // GH-128396: With PEP 709, it's possible to have a fast variable in
        // an inlined comprehension that has the same name as the cell variable
        // in the frame, where the `kind` obtained from frame can not guarantee
        // that the variable is a cell.
        // If the variable is not a cell, we are okay with it and we can simply
        // return the value.
        if (PyCell_Check(value)) {
            cell = value;
        }
    }

    if (cell != NULL) {
        value = PyCell_GET(cell);
    }

    if (value == NULL) {
        return NULL;
    }

    return value;
}

static int
framelocalsproxy_getkeyindex(PyFrameObject *frame, PyObject* key, bool read)
{
    /*
     * Returns -2 (!) if an error occurred; exception will be set.
     * Returns the fast locals index of the key on success:
     *   - if read == true, returns the index if the value is not NULL
     *   - if read == false, returns the index if the value is not hidden
     * Otherwise returns -1.
     */

    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);

    // Ensure that the key is hashable.
    Py_hash_t key_hash = PyObject_Hash(key);
    if (key_hash == -1) {
        return -2;
    }
    bool found = false;

    // We do 2 loops here because it's highly possible the key is interned
    // and we can do a pointer comparison.
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, i);
        if (name == key) {
            if (read) {
                if (framelocalsproxy_getval(frame->f_frame, co, i) != NULL) {
                    return i;
                }
            } else {
                if (!(_PyLocals_GetKind(co->co_localspluskinds, i) & CO_FAST_HIDDEN)) {
                    return i;
                }
            }
            found = true;
        }
    }
    if (found) {
        // This is an attempt to read an unset local variable or
        // write to a variable that is hidden from regular write operations
        return -1;
    }
    // This is unlikely, but we need to make sure. This means the key
    // is not interned.
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, i);
        Py_hash_t name_hash = PyObject_Hash(name);
        assert(name_hash != -1);  // keys are exact unicode
        if (name_hash != key_hash) {
            continue;
        }
        int same = PyObject_RichCompareBool(name, key, Py_EQ);
        if (same < 0) {
            return -2;
        }
        if (same) {
            if (read) {
                if (framelocalsproxy_getval(frame->f_frame, co, i) != NULL) {
                    return i;
                }
            } else {
                if (!(_PyLocals_GetKind(co->co_localspluskinds, i) & CO_FAST_HIDDEN)) {
                    return i;
                }
            }
        }
    }

    return -1;
}

static PyObject *
framelocalsproxy_getitem(PyObject *self, PyObject *key)
{
    PyFrameObject* frame = ((PyFrameLocalsProxyObject*)self)->frame;
    PyCodeObject* co = _PyFrame_GetCode(frame->f_frame);

    int i = framelocalsproxy_getkeyindex(frame, key, true);
    if (i == -2) {
        return NULL;
    }
    if (i >= 0) {
        PyObject *value = framelocalsproxy_getval(frame->f_frame, co, i);
        assert(value != NULL);
        return Py_NewRef(value);
    }

    // Okay not in the fast locals, try extra locals

    PyObject *extra = frame->f_extra_locals;
    if (extra != NULL) {
        PyObject *value = PyDict_GetItem(extra, key);
        if (value != NULL) {
            return Py_NewRef(value);
        }
    }

    PyErr_Format(PyExc_KeyError, "local variable '%R' is not defined", key);
    return NULL;
}

static int
framelocalsproxy_setitem(PyObject *self, PyObject *key, PyObject *value)
{
    /* Merge locals into fast locals */
    PyFrameObject* frame = ((PyFrameLocalsProxyObject*)self)->frame;
    PyObject** fast = _PyFrame_GetLocalsArray(frame->f_frame);
    PyCodeObject* co = _PyFrame_GetCode(frame->f_frame);

    int i = framelocalsproxy_getkeyindex(frame, key, false);
    if (i == -2) {
        return -1;
    }
    if (i >= 0) {
        if (value == NULL) {
            PyErr_SetString(PyExc_ValueError, "cannot remove local variables from FrameLocalsProxy");
            return -1;
        }

        _Py_Executors_InvalidateDependency(PyInterpreterState_Get(), co, 1);

        _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);
        PyObject *oldvalue = fast[i];
        PyObject *cell = NULL;
        if (kind == CO_FAST_FREE) {
            // The cell was set when the frame was created from
            // the function's closure.
            assert(oldvalue != NULL && PyCell_Check(oldvalue));
            cell = oldvalue;
        } else if (kind & CO_FAST_CELL && oldvalue != NULL) {
            if (PyCell_Check(oldvalue)) {
                cell = oldvalue;
            }
        }
        if (cell != NULL) {
            oldvalue = PyCell_GET(cell);
            if (value != oldvalue) {
                PyCell_SET(cell, Py_XNewRef(value));
                Py_XDECREF(oldvalue);
            }
        } else if (value != oldvalue) {
            Py_XSETREF(fast[i], Py_NewRef(value));
        }
        return 0;
    }

    // Okay not in the fast locals, try extra locals

    PyObject *extra = frame->f_extra_locals;

    if (extra == NULL) {
        if (value == NULL) {
            _PyErr_SetKeyError(key);
            return -1;
        }
        extra = PyDict_New();
        if (extra == NULL) {
            return -1;
        }
        frame->f_extra_locals = extra;
    }

    assert(PyDict_Check(extra));

    if (value == NULL) {
        return PyDict_DelItem(extra, key);
    } else {
        return PyDict_SetItem(extra, key, value);
    }
}

static int
framelocalsproxy_merge(PyObject* self, PyObject* other)
{
    if (!PyDict_Check(other) && !PyFrameLocalsProxy_Check(other)) {
        return -1;
    }

    PyObject *keys = PyMapping_Keys(other);
    if (keys == NULL) {
        return -1;
    }

    PyObject *iter = PyObject_GetIter(keys);
    Py_DECREF(keys);
    if (iter == NULL) {
        return -1;
    }

    PyObject *key = NULL;
    PyObject *value = NULL;

    while ((key = PyIter_Next(iter)) != NULL) {
        value = PyObject_GetItem(other, key);
        if (value == NULL) {
            Py_DECREF(key);
            Py_DECREF(iter);
            return -1;
        }

        if (framelocalsproxy_setitem(self, key, value) < 0) {
            Py_DECREF(key);
            Py_DECREF(value);
            Py_DECREF(iter);
            return -1;
        }

        Py_DECREF(key);
        Py_DECREF(value);
    }

    Py_DECREF(iter);

    if (PyErr_Occurred()) {
        return -1;
    }

    return 0;
}

static PyObject *
framelocalsproxy_keys(PyObject *self, void *Py_UNUSED(ignored))
{
    PyFrameObject *frame = ((PyFrameLocalsProxyObject*)self)->frame;
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);
    PyObject *names = PyList_New(0);
    if (names == NULL) {
        return NULL;
    }

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *val = framelocalsproxy_getval(frame->f_frame, co, i);
        if (val) {
            PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, i);
            if (PyList_Append(names, name) < 0) {
                Py_DECREF(names);
                return NULL;
            }
        }
    }

    // Iterate through the extra locals
    if (frame->f_extra_locals) {
        assert(PyDict_Check(frame->f_extra_locals));

        Py_ssize_t i = 0;
        PyObject *key = NULL;
        PyObject *value = NULL;

        while (PyDict_Next(frame->f_extra_locals, &i, &key, &value)) {
            if (PyList_Append(names, key) < 0) {
                Py_DECREF(names);
                return NULL;
            }
        }
    }

    return names;
}

static void
framelocalsproxy_dealloc(PyObject *self)
{
    PyObject_GC_UnTrack(self);
    Py_CLEAR(((PyFrameLocalsProxyObject*)self)->frame);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
framelocalsproxy_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (PyTuple_GET_SIZE(args) != 1) {
        PyErr_Format(PyExc_TypeError,
                     "FrameLocalsProxy expected 1 argument, got %zd",
                     PyTuple_GET_SIZE(args));
        return NULL;
    }
    PyObject *item = PyTuple_GET_ITEM(args, 0);

    if (!PyFrame_Check(item)) {
        PyErr_Format(PyExc_TypeError, "expect frame, not %T", item);
        return NULL;
    }
    PyFrameObject *frame = (PyFrameObject*)item;

    if (kwds != NULL && PyDict_Size(kwds) != 0) {
        PyErr_SetString(PyExc_TypeError,
                        "FrameLocalsProxy takes no keyword arguments");
        return 0;
    }

    PyFrameLocalsProxyObject *self = (PyFrameLocalsProxyObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    ((PyFrameLocalsProxyObject*)self)->frame = (PyFrameObject*)Py_NewRef(frame);

    return (PyObject *)self;
}

static int
framelocalsproxy_tp_clear(PyObject *self)
{
    Py_CLEAR(((PyFrameLocalsProxyObject*)self)->frame);
    return 0;
}

static int
framelocalsproxy_visit(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(((PyFrameLocalsProxyObject*)self)->frame);
    return 0;
}

static PyObject *
framelocalsproxy_iter(PyObject *self)
{
    PyObject* keys = framelocalsproxy_keys(self, NULL);
    if (keys == NULL) {
        return NULL;
    }

    PyObject* iter = PyObject_GetIter(keys);
    Py_XDECREF(keys);

    return iter;
}

static PyObject *
framelocalsproxy_richcompare(PyObject *self, PyObject *other, int op)
{
    if (PyFrameLocalsProxy_Check(other)) {
        bool result = ((PyFrameLocalsProxyObject*)self)->frame == ((PyFrameLocalsProxyObject*)other)->frame;
        if (op == Py_EQ) {
            return PyBool_FromLong(result);
        } else if (op == Py_NE) {
            return PyBool_FromLong(!result);
        }
    } else if (PyDict_Check(other)) {
        PyObject *dct = PyDict_New();
        if (dct == NULL) {
            return NULL;
        }

        if (PyDict_Update(dct, self) < 0) {
            Py_DECREF(dct);
            return NULL;
        }

        PyObject *result = PyObject_RichCompare(dct, other, op);
        Py_DECREF(dct);
        return result;
    }

    Py_RETURN_NOTIMPLEMENTED;
}

static PyObject *
framelocalsproxy_repr(PyObject *self)
{
    int i = Py_ReprEnter(self);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("{...}") : NULL;
    }

    PyObject *dct = PyDict_New();
    if (dct == NULL) {
        Py_ReprLeave(self);
        return NULL;
    }

    if (PyDict_Update(dct, self) < 0) {
        Py_DECREF(dct);
        Py_ReprLeave(self);
        return NULL;
    }

    PyObject *repr = PyObject_Repr(dct);
    Py_DECREF(dct);

    Py_ReprLeave(self);

    return repr;
}

static PyObject*
framelocalsproxy_or(PyObject *self, PyObject *other)
{
    if (!PyDict_Check(other) && !PyFrameLocalsProxy_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    PyObject *result = PyDict_New();
    if (result == NULL) {
        return NULL;
    }

    if (PyDict_Update(result, self) < 0) {
        Py_DECREF(result);
        return NULL;
    }

    if (PyDict_Update(result, other) < 0) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

static PyObject*
framelocalsproxy_inplace_or(PyObject *self, PyObject *other)
{
    if (!PyDict_Check(other) && !PyFrameLocalsProxy_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (framelocalsproxy_merge(self, other) < 0) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    return Py_NewRef(self);
}

static PyObject*
framelocalsproxy_values(PyObject *self, void *Py_UNUSED(ignored))
{
    PyFrameObject *frame = ((PyFrameLocalsProxyObject*)self)->frame;
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);
    PyObject *values = PyList_New(0);
    if (values == NULL) {
        return NULL;
    }

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *value = framelocalsproxy_getval(frame->f_frame, co, i);
        if (value) {
            if (PyList_Append(values, value) < 0) {
                Py_DECREF(values);
                return NULL;
            }
        }
    }

    // Iterate through the extra locals
    if (frame->f_extra_locals) {
        Py_ssize_t j = 0;
        PyObject *key = NULL;
        PyObject *value = NULL;
        while (PyDict_Next(frame->f_extra_locals, &j, &key, &value)) {
            if (PyList_Append(values, value) < 0) {
                Py_DECREF(values);
                return NULL;
            }
        }
    }

    return values;
}

static PyObject *
framelocalsproxy_items(PyObject *self, void *Py_UNUSED(ignored))
{
    PyFrameObject *frame = ((PyFrameLocalsProxyObject*)self)->frame;
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);
    PyObject *items = PyList_New(0);
    if (items == NULL) {
        return NULL;
    }

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, i);
        PyObject *value = framelocalsproxy_getval(frame->f_frame, co, i);

        if (value) {
            PyObject *pair = PyTuple_Pack(2, name, value);
            if (pair == NULL) {
                Py_DECREF(items);
                return NULL;
            }

            if (PyList_Append(items, pair) < 0) {
                Py_DECREF(items);
                Py_DECREF(pair);
                return NULL;
            }

            Py_DECREF(pair);
        }
    }

    // Iterate through the extra locals
    if (frame->f_extra_locals) {
        Py_ssize_t j = 0;
        PyObject *key = NULL;
        PyObject *value = NULL;
        while (PyDict_Next(frame->f_extra_locals, &j, &key, &value)) {
            PyObject *pair = PyTuple_Pack(2, key, value);
            if (pair == NULL) {
                Py_DECREF(items);
                return NULL;
            }

            if (PyList_Append(items, pair) < 0) {
                Py_DECREF(items);
                Py_DECREF(pair);
                return NULL;
            }

            Py_DECREF(pair);
        }
    }

    return items;
}

static Py_ssize_t
framelocalsproxy_length(PyObject *self)
{
    PyFrameObject *frame = ((PyFrameLocalsProxyObject*)self)->frame;
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);
    Py_ssize_t size = 0;

    if (frame->f_extra_locals != NULL) {
        assert(PyDict_Check(frame->f_extra_locals));
        size += PyDict_Size(frame->f_extra_locals);
    }

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        if (framelocalsproxy_getval(frame->f_frame, co, i) != NULL) {
            size++;
        }
    }
    return size;
}

static int
framelocalsproxy_contains(PyObject *self, PyObject *key)
{
    PyFrameObject *frame = ((PyFrameLocalsProxyObject*)self)->frame;

    int i = framelocalsproxy_getkeyindex(frame, key, true);
    if (i == -2) {
        return -1;
    }
    if (i >= 0) {
        return 1;
    }

    PyObject *extra = ((PyFrameObject*)frame)->f_extra_locals;
    if (extra != NULL) {
        return PyDict_Contains(extra, key);
    }

    return 0;
}

static PyObject* framelocalsproxy___contains__(PyObject *self, PyObject *key)
{
    int result = framelocalsproxy_contains(self, key);
    if (result < 0) {
        return NULL;
    }
    return PyBool_FromLong(result);
}

static PyObject*
framelocalsproxy_update(PyObject *self, PyObject *other)
{
    if (framelocalsproxy_merge(self, other) < 0) {
        PyErr_SetString(PyExc_TypeError, "update() argument must be dict or another FrameLocalsProxy");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject*
framelocalsproxy_get(PyObject* self, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs < 1 || nargs > 2) {
        PyErr_SetString(PyExc_TypeError, "get expected 1 or 2 arguments");
        return NULL;
    }

    PyObject *key = args[0];
    PyObject *default_value = Py_None;

    if (nargs == 2) {
        default_value = args[1];
    }

    PyObject *result = framelocalsproxy_getitem(self, key);

    if (result == NULL) {
        if (PyErr_ExceptionMatches(PyExc_KeyError)) {
            PyErr_Clear();
            return Py_XNewRef(default_value);
        }
        return NULL;
    }

    return result;
}

static PyObject*
framelocalsproxy_setdefault(PyObject* self, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs < 1 || nargs > 2) {
        PyErr_SetString(PyExc_TypeError, "setdefault expected 1 or 2 arguments");
        return NULL;
    }

    PyObject *key = args[0];
    PyObject *default_value = Py_None;

    if (nargs == 2) {
        default_value = args[1];
    }

    PyObject *result = framelocalsproxy_getitem(self, key);

    if (result == NULL) {
        if (PyErr_ExceptionMatches(PyExc_KeyError)) {
            PyErr_Clear();
            if (framelocalsproxy_setitem(self, key, default_value) < 0) {
                return NULL;
            }
            return Py_XNewRef(default_value);
        }
        return NULL;
    }

    return result;
}

static PyObject*
framelocalsproxy_pop(PyObject* self, PyObject *const *args, Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("pop", nargs, 1, 2)) {
        return NULL;
    }

    PyObject *key = args[0];
    PyObject *default_value = NULL;

    if (nargs == 2) {
        default_value = args[1];
    }

    PyFrameObject *frame = ((PyFrameLocalsProxyObject*)self)->frame;

    int i = framelocalsproxy_getkeyindex(frame, key, false);
    if (i == -2) {
        return NULL;
    }

    if (i >= 0) {
        PyErr_SetString(PyExc_ValueError, "cannot remove local variables from FrameLocalsProxy");
        return NULL;
    }

    PyObject *result = NULL;

    if (frame->f_extra_locals == NULL) {
        if (default_value != NULL) {
            return Py_XNewRef(default_value);
        } else {
            _PyErr_SetKeyError(key);
            return NULL;
        }
    }

    if (PyDict_Pop(frame->f_extra_locals, key, &result) < 0) {
        return NULL;
    }

    if (result == NULL) {
        if (default_value != NULL) {
            return Py_XNewRef(default_value);
        } else {
            _PyErr_SetKeyError(key);
            return NULL;
        }
    }

    return result;
}

static PyObject*
framelocalsproxy_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject* result = PyDict_New();

    if (result == NULL) {
        return NULL;
    }

    if (PyDict_Update(result, self) < 0) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

static PyObject*
framelocalsproxy_reversed(PyObject *self, void *Py_UNUSED(ignored))
{
    PyObject *result = framelocalsproxy_keys(self, NULL);

    if (result == NULL) {
        return NULL;
    }

    if (PyList_Reverse(result) < 0) {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}

static PyNumberMethods framelocalsproxy_as_number = {
    .nb_or = framelocalsproxy_or,
    .nb_inplace_or = framelocalsproxy_inplace_or,
};

static PySequenceMethods framelocalsproxy_as_sequence = {
    .sq_contains = framelocalsproxy_contains,
};

static PyMappingMethods framelocalsproxy_as_mapping = {
    framelocalsproxy_length, // mp_length
    framelocalsproxy_getitem, // mp_subscript
    framelocalsproxy_setitem, // mp_ass_subscript
};

static PyMethodDef framelocalsproxy_methods[] = {
    {"__contains__",  framelocalsproxy___contains__,      METH_O | METH_COEXIST,
     NULL},
    {"__getitem__",   framelocalsproxy_getitem,           METH_O | METH_COEXIST,
     NULL},
    {"update",        framelocalsproxy_update,            METH_O,
     NULL},
    {"__reversed__", _PyCFunction_CAST(framelocalsproxy_reversed),       METH_NOARGS,
     NULL},
    {"copy",         _PyCFunction_CAST(framelocalsproxy_copy),           METH_NOARGS,
     NULL},
    {"keys",         _PyCFunction_CAST(framelocalsproxy_keys),           METH_NOARGS,
     NULL},
    {"values",       _PyCFunction_CAST(framelocalsproxy_values),         METH_NOARGS,
     NULL},
    {"items",        _PyCFunction_CAST(framelocalsproxy_items),          METH_NOARGS,
     NULL},
    {"get",          _PyCFunction_CAST(framelocalsproxy_get),            METH_FASTCALL,
     NULL},
    {"pop",          _PyCFunction_CAST(framelocalsproxy_pop),            METH_FASTCALL,
     NULL},
    {"setdefault",   _PyCFunction_CAST(framelocalsproxy_setdefault),     METH_FASTCALL,
     NULL},
    {NULL,            NULL}   /* sentinel */
};

PyTypeObject PyFrameLocalsProxy_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "FrameLocalsProxy",
    .tp_basicsize = sizeof(PyFrameLocalsProxyObject),
    .tp_dealloc = (destructor)framelocalsproxy_dealloc,
    .tp_repr = &framelocalsproxy_repr,
    .tp_as_number = &framelocalsproxy_as_number,
    .tp_as_sequence = &framelocalsproxy_as_sequence,
    .tp_as_mapping = &framelocalsproxy_as_mapping,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_setattro = PyObject_GenericSetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_MAPPING,
    .tp_traverse = framelocalsproxy_visit,
    .tp_clear = framelocalsproxy_tp_clear,
    .tp_richcompare = framelocalsproxy_richcompare,
    .tp_iter = framelocalsproxy_iter,
    .tp_methods = framelocalsproxy_methods,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = framelocalsproxy_new,
    .tp_free = PyObject_GC_Del,
};

PyObject *
_PyFrameLocalsProxy_New(PyFrameObject *frame)
{
    PyObject* args = PyTuple_Pack(1, frame);
    if (args == NULL) {
        return NULL;
    }

    PyObject* proxy = (PyObject*)framelocalsproxy_new(&PyFrameLocalsProxy_Type, args, NULL);
    Py_DECREF(args);
    return proxy;
}

static PyMemberDef frame_memberlist[] = {
    {"f_trace_lines",   Py_T_BOOL,         OFF(f_trace_lines), 0},
    {NULL}      /* Sentinel */
};

static PyObject *
frame_getlocals(PyFrameObject *f, void *closure)
{
    if (f == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }
    assert(!_PyFrame_IsIncomplete(f->f_frame));

    PyCodeObject *co = _PyFrame_GetCode(f->f_frame);

    if (!(co->co_flags & CO_OPTIMIZED) && !_PyFrame_HasHiddenLocals(f->f_frame)) {
        if (f->f_frame->f_locals == NULL) {
            // We found cases when f_locals is NULL for non-optimized code.
            // We fill the f_locals with an empty dict to avoid crash until
            // we find the root cause.
            f->f_frame->f_locals = PyDict_New();
            if (f->f_frame->f_locals == NULL) {
                return NULL;
            }
        }
        return Py_NewRef(f->f_frame->f_locals);
    }

    return _PyFrameLocalsProxy_New(f);
}

int
PyFrame_GetLineNumber(PyFrameObject *f)
{
    assert(f != NULL);
    if (f->f_lineno == -1) {
        // We should calculate it once. If we can't get the line number,
        // set f->f_lineno to 0.
        f->f_lineno = PyUnstable_InterpreterFrame_GetLine(f->f_frame);
        if (f->f_lineno < 0) {
            f->f_lineno = 0;
            return -1;
        }
    }

    if (f->f_lineno > 0) {
        return f->f_lineno;
    }
    return PyUnstable_InterpreterFrame_GetLine(f->f_frame);
}

static PyObject *
frame_getlineno(PyFrameObject *f, void *closure)
{
    int lineno = PyFrame_GetLineNumber(f);
    if (lineno < 0) {
        Py_RETURN_NONE;
    }
    else {
        return PyLong_FromLong(lineno);
    }
}

static PyObject *
frame_getlasti(PyFrameObject *f, void *closure)
{
    int lasti = _PyInterpreterFrame_LASTI(f->f_frame);
    if (lasti < 0) {
        return PyLong_FromLong(-1);
    }
    return PyLong_FromLong(lasti * sizeof(_Py_CODEUNIT));
}

static PyObject *
frame_getglobals(PyFrameObject *f, void *closure)
{
    PyObject *globals = f->f_frame->f_globals;
    if (globals == NULL) {
        globals = Py_None;
    }
    return Py_NewRef(globals);
}

static PyObject *
frame_getbuiltins(PyFrameObject *f, void *closure)
{
    PyObject *builtins = f->f_frame->f_builtins;
    if (builtins == NULL) {
        builtins = Py_None;
    }
    return Py_NewRef(builtins);
}

static PyObject *
frame_getcode(PyFrameObject *f, void *closure)
{
    if (PySys_Audit("object.__getattr__", "Os", f, "f_code") < 0) {
        return NULL;
    }
    return (PyObject *)PyFrame_GetCode(f);
}

static PyObject *
frame_getback(PyFrameObject *f, void *closure)
{
    PyObject *res = (PyObject *)PyFrame_GetBack(f);
    if (res == NULL) {
        Py_RETURN_NONE;
    }
    return res;
}

static PyObject *
frame_gettrace_opcodes(PyFrameObject *f, void *closure)
{
    PyObject *result = f->f_trace_opcodes ? Py_True : Py_False;
    return Py_NewRef(result);
}

static int
frame_settrace_opcodes(PyFrameObject *f, PyObject* value, void *Py_UNUSED(ignored))
{
    if (!PyBool_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute value type must be bool");
        return -1;
    }
    if (value == Py_True) {
        f->f_trace_opcodes = 1;
        if (f->f_trace) {
            return _PyEval_SetOpcodeTrace(f, true);
        }
    }
    else {
        f->f_trace_opcodes = 0;
        return _PyEval_SetOpcodeTrace(f, false);
    }
    return 0;
}

/* Model the evaluation stack, to determine which jumps
 * are safe and how many values needs to be popped.
 * The stack is modelled by a 64 integer, treating any
 * stack that can't fit into 64 bits as "overflowed".
 */

typedef enum kind {
    Iterator = 1,
    Except = 2,
    Object = 3,
    Null = 4,
    Lasti = 5,
} Kind;

static int
compatible_kind(Kind from, Kind to) {
    if (to == 0) {
        return 0;
    }
    if (to == Object) {
        return from != Null;
    }
    if (to == Null) {
        return 1;
    }
    return from == to;
}

#define BITS_PER_BLOCK 3

#define UNINITIALIZED -2
#define OVERFLOWED -1

#define MAX_STACK_ENTRIES (63/BITS_PER_BLOCK)
#define WILL_OVERFLOW (1ULL<<((MAX_STACK_ENTRIES-1)*BITS_PER_BLOCK))

#define EMPTY_STACK 0

static inline int64_t
push_value(int64_t stack, Kind kind)
{
    if (((uint64_t)stack) >= WILL_OVERFLOW) {
        return OVERFLOWED;
    }
    else {
        return (stack << BITS_PER_BLOCK) | kind;
    }
}

static inline int64_t
pop_value(int64_t stack)
{
    return Py_ARITHMETIC_RIGHT_SHIFT(int64_t, stack, BITS_PER_BLOCK);
}

#define MASK ((1<<BITS_PER_BLOCK)-1)

static inline Kind
top_of_stack(int64_t stack)
{
    return stack & MASK;
}

static inline Kind
peek(int64_t stack, int n)
{
    assert(n >= 1);
    return (stack>>(BITS_PER_BLOCK*(n-1))) & MASK;
}

static Kind
stack_swap(int64_t stack, int n)
{
    assert(n >= 1);
    Kind to_swap = peek(stack, n);
    Kind top = top_of_stack(stack);
    int shift = BITS_PER_BLOCK*(n-1);
    int64_t replaced_low = (stack & ~(MASK << shift)) | (top << shift);
    int64_t replaced_top = (replaced_low & ~MASK) | to_swap;
    return replaced_top;
}

static int64_t
pop_to_level(int64_t stack, int level) {
    if (level == 0) {
        return EMPTY_STACK;
    }
    int64_t max_item = (1<<BITS_PER_BLOCK) - 1;
    int64_t level_max_stack = max_item << ((level-1) * BITS_PER_BLOCK);
    while (stack > level_max_stack) {
        stack = pop_value(stack);
    }
    return stack;
}

#if 0
/* These functions are useful for debugging the stack marking code */

static char
tos_char(int64_t stack) {
    switch(top_of_stack(stack)) {
        case Iterator:
            return 'I';
        case Except:
            return 'E';
        case Object:
            return 'O';
        case Lasti:
            return 'L';
        case Null:
            return 'N';
    }
    return '?';
}

static void
print_stack(int64_t stack) {
    if (stack < 0) {
        if (stack == UNINITIALIZED) {
            printf("---");
        }
        else if (stack == OVERFLOWED) {
            printf("OVERFLOWED");
        }
        else {
            printf("??");
        }
        return;
    }
    while (stack) {
        printf("%c", tos_char(stack));
        stack = pop_value(stack);
    }
}

static void
print_stacks(int64_t *stacks, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d: ", i);
        print_stack(stacks[i]);
        printf("\n");
    }
}

#endif

static int64_t *
mark_stacks(PyCodeObject *code_obj, int len)
{
    PyObject *co_code = _PyCode_GetCode(code_obj);
    if (co_code == NULL) {
        return NULL;
    }
    _Py_CODEUNIT *code = (_Py_CODEUNIT *)PyBytes_AS_STRING(co_code);
    int64_t *stacks = PyMem_New(int64_t, len+1);
    int i, j, opcode;

    if (stacks == NULL) {
        PyErr_NoMemory();
        Py_DECREF(co_code);
        return NULL;
    }
    for (int i = 1; i <= len; i++) {
        stacks[i] = UNINITIALIZED;
    }
    stacks[0] = EMPTY_STACK;
    int todo = 1;
    while (todo) {
        todo = 0;
        /* Scan instructions */
        for (i = 0; i < len;) {
            int64_t next_stack = stacks[i];
            opcode = _Py_GetBaseOpcode(code_obj, i);
            int oparg = 0;
            while (opcode == EXTENDED_ARG) {
                oparg = (oparg << 8) | code[i].op.arg;
                i++;
                opcode = _Py_GetBaseOpcode(code_obj, i);
                stacks[i] = next_stack;
            }
            int next_i = i + _PyOpcode_Caches[opcode] + 1;
            if (next_stack == UNINITIALIZED) {
                i = next_i;
                continue;
            }
            oparg = (oparg << 8) | code[i].op.arg;
            switch (opcode) {
                case POP_JUMP_IF_FALSE:
                case POP_JUMP_IF_TRUE:
                case POP_JUMP_IF_NONE:
                case POP_JUMP_IF_NOT_NONE:
                {
                    int64_t target_stack;
                    int j = next_i + oparg;
                    assert(j < len);
                    next_stack = pop_value(next_stack);
                    target_stack = next_stack;
                    assert(stacks[j] == UNINITIALIZED || stacks[j] == target_stack);
                    stacks[j] = target_stack;
                    stacks[next_i] = next_stack;
                    break;
                }
                case SEND:
                    j = oparg + i + INLINE_CACHE_ENTRIES_SEND + 1;
                    assert(j < len);
                    assert(stacks[j] == UNINITIALIZED || stacks[j] == next_stack);
                    stacks[j] = next_stack;
                    stacks[next_i] = next_stack;
                    break;
                case JUMP_FORWARD:
                    j = oparg + i + 1;
                    assert(j < len);
                    assert(stacks[j] == UNINITIALIZED || stacks[j] == next_stack);
                    stacks[j] = next_stack;
                    break;
                case JUMP_BACKWARD:
                case JUMP_BACKWARD_NO_INTERRUPT:
                    j = next_i - oparg;
                    assert(j >= 0);
                    assert(j < len);
                    if (stacks[j] == UNINITIALIZED && j < i) {
                        todo = 1;
                    }
                    assert(stacks[j] == UNINITIALIZED || stacks[j] == next_stack);
                    stacks[j] = next_stack;
                    break;
                case GET_ITER:
                case GET_AITER:
                    next_stack = push_value(pop_value(next_stack), Iterator);
                    stacks[next_i] = next_stack;
                    break;
                case FOR_ITER:
                {
                    int64_t target_stack = push_value(next_stack, Object);
                    stacks[next_i] = target_stack;
                    j = oparg + 1 + INLINE_CACHE_ENTRIES_FOR_ITER + i;
                    assert(j < len);
                    assert(stacks[j] == UNINITIALIZED || stacks[j] == target_stack);
                    stacks[j] = target_stack;
                    break;
                }
                case END_ASYNC_FOR:
                    next_stack = pop_value(pop_value(next_stack));
                    stacks[next_i] = next_stack;
                    break;
                case PUSH_EXC_INFO:
                    next_stack = push_value(next_stack, Except);
                    stacks[next_i] = next_stack;
                    break;
                case POP_EXCEPT:
                    assert(top_of_stack(next_stack) == Except);
                    next_stack = pop_value(next_stack);
                    stacks[next_i] = next_stack;
                    break;
                case RETURN_VALUE:
                    assert(pop_value(next_stack) == EMPTY_STACK);
                    assert(top_of_stack(next_stack) == Object);
                    break;
                case RETURN_CONST:
                    break;
                case RAISE_VARARGS:
                    break;
                case RERAISE:
                    assert(top_of_stack(next_stack) == Except);
                    /* End of block */
                    break;
                case PUSH_NULL:
                    next_stack = push_value(next_stack, Null);
                    stacks[next_i] = next_stack;
                    break;
                case LOAD_GLOBAL:
                {
                    int j = oparg;
                    next_stack = push_value(next_stack, Object);
                    if (j & 1) {
                        next_stack = push_value(next_stack, Null);
                    }
                    stacks[next_i] = next_stack;
                    break;
                }
                case LOAD_ATTR:
                {
                    assert(top_of_stack(next_stack) == Object);
                    int j = oparg;
                    if (j & 1) {
                        next_stack = pop_value(next_stack);
                        next_stack = push_value(next_stack, Object);
                        next_stack = push_value(next_stack, Null);
                    }
                    stacks[next_i] = next_stack;
                    break;
                }
                case SWAP:
                {
                    int n = oparg;
                    next_stack = stack_swap(next_stack, n);
                    stacks[next_i] = next_stack;
                    break;
                }
                case COPY:
                {
                    int n = oparg;
                    next_stack = push_value(next_stack, peek(next_stack, n));
                    stacks[next_i] = next_stack;
                    break;
                }
                case CACHE:
                case RESERVED:
                {
                    assert(0);
                }
                default:
                {
                    int delta = PyCompile_OpcodeStackEffect(opcode, oparg);
                    assert(delta != PY_INVALID_STACK_EFFECT);
                    while (delta < 0) {
                        next_stack = pop_value(next_stack);
                        delta++;
                    }
                    while (delta > 0) {
                        next_stack = push_value(next_stack, Object);
                        delta--;
                    }
                    stacks[next_i] = next_stack;
                }
            }
            i = next_i;
        }
        /* Scan exception table */
        unsigned char *start = (unsigned char *)PyBytes_AS_STRING(code_obj->co_exceptiontable);
        unsigned char *end = start + PyBytes_GET_SIZE(code_obj->co_exceptiontable);
        unsigned char *scan = start;
        while (scan < end) {
            int start_offset, size, handler;
            scan = parse_varint(scan, &start_offset);
            assert(start_offset >= 0 && start_offset < len);
            scan = parse_varint(scan, &size);
            assert(size >= 0 && start_offset+size <= len);
            scan = parse_varint(scan, &handler);
            assert(handler >= 0 && handler < len);
            int depth_and_lasti;
            scan = parse_varint(scan, &depth_and_lasti);
            int level = depth_and_lasti >> 1;
            int lasti = depth_and_lasti & 1;
            if (stacks[start_offset] != UNINITIALIZED) {
                if (stacks[handler] == UNINITIALIZED) {
                    todo = 1;
                    uint64_t target_stack = pop_to_level(stacks[start_offset], level);
                    if (lasti) {
                        target_stack = push_value(target_stack, Lasti);
                    }
                    target_stack = push_value(target_stack, Except);
                    stacks[handler] = target_stack;
                }
            }
        }
    }
    Py_DECREF(co_code);
    return stacks;
}

static int
compatible_stack(int64_t from_stack, int64_t to_stack)
{
    if (from_stack < 0 || to_stack < 0) {
        return 0;
    }
    while(from_stack > to_stack) {
        from_stack = pop_value(from_stack);
    }
    while(from_stack) {
        Kind from_top = top_of_stack(from_stack);
        Kind to_top = top_of_stack(to_stack);
        if (!compatible_kind(from_top, to_top)) {
            return 0;
        }
        from_stack = pop_value(from_stack);
        to_stack = pop_value(to_stack);
    }
    return to_stack == 0;
}

static const char *
explain_incompatible_stack(int64_t to_stack)
{
    assert(to_stack != 0);
    if (to_stack == OVERFLOWED) {
        return "stack is too deep to analyze";
    }
    if (to_stack == UNINITIALIZED) {
        return "can't jump into an exception handler, or code may be unreachable";
    }
    Kind target_kind = top_of_stack(to_stack);
    switch(target_kind) {
        case Except:
            return "can't jump into an 'except' block as there's no exception";
        case Lasti:
            return "can't jump into a re-raising block as there's no location";
        case Object:
        case Null:
            return "incompatible stacks";
        case Iterator:
            return "can't jump into the body of a for loop";
        default:
            Py_UNREACHABLE();
    }
}

static int *
marklines(PyCodeObject *code, int len)
{
    PyCodeAddressRange bounds;
    _PyCode_InitAddressRange(code, &bounds);
    assert (bounds.ar_end == 0);
    int last_line = -1;

    int *linestarts = PyMem_New(int, len);
    if (linestarts == NULL) {
        return NULL;
    }
    for (int i = 0; i < len; i++) {
        linestarts[i] = -1;
    }

    while (_PyLineTable_NextAddressRange(&bounds)) {
        assert(bounds.ar_start / (int)sizeof(_Py_CODEUNIT) < len);
        if (bounds.ar_line != last_line && bounds.ar_line != -1) {
            linestarts[bounds.ar_start / sizeof(_Py_CODEUNIT)] = bounds.ar_line;
            last_line = bounds.ar_line;
        }
    }
    return linestarts;
}

static int
first_line_not_before(int *lines, int len, int line)
{
    int result = INT_MAX;
    for (int i = 0; i < len; i++) {
        if (lines[i] < result && lines[i] >= line) {
            result = lines[i];
        }
    }
    if (result == INT_MAX) {
        return -1;
    }
    return result;
}

static bool frame_is_suspended(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    if (frame->f_frame->owner == FRAME_OWNED_BY_GENERATOR) {
        PyGenObject *gen = _PyFrame_GetGenerator(frame->f_frame);
        return FRAME_STATE_SUSPENDED(gen->gi_frame_state);
    }
    return false;
}

/* Setter for f_lineno - you can set f_lineno from within a trace function in
 * order to jump to a given line of code, subject to some restrictions.  Most
 * lines are OK to jump to because they don't make any assumptions about the
 * state of the stack (obvious because you could remove the line and the code
 * would still work without any stack errors), but there are some constructs
 * that limit jumping:
 *
 *  o Any exception handlers.
 *  o 'for' and 'async for' loops can't be jumped into because the
 *    iterator needs to be on the stack.
 *  o Jumps cannot be made from within a trace function invoked with a
 *    'return' or 'exception' event since the eval loop has been exited at
 *    that time.
 */
static int
frame_setlineno(PyFrameObject *f, PyObject* p_new_lineno, void *Py_UNUSED(ignored))
{
    PyCodeObject *code = _PyFrame_GetCode(f->f_frame);
    if (p_new_lineno == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }
    /* f_lineno must be an integer. */
    if (!PyLong_CheckExact(p_new_lineno)) {
        PyErr_SetString(PyExc_ValueError,
                        "lineno must be an integer");
        return -1;
    }

    bool is_suspended = frame_is_suspended(f);
    /*
     * This code preserves the historical restrictions on
     * setting the line number of a frame.
     * Jumps are forbidden on a 'return' trace event (except after a yield).
     * Jumps from 'call' trace events are also forbidden.
     * In addition, jumps are forbidden when not tracing,
     * as this is a debugging feature.
     */
    int what_event = PyThreadState_GET()->what_event;
    if (what_event < 0) {
        PyErr_Format(PyExc_ValueError,
                    "f_lineno can only be set in a trace function");
        return -1;
    }
    switch (what_event) {
        case PY_MONITORING_EVENT_PY_RESUME:
        case PY_MONITORING_EVENT_JUMP:
        case PY_MONITORING_EVENT_BRANCH:
        case PY_MONITORING_EVENT_LINE:
        case PY_MONITORING_EVENT_PY_YIELD:
            /* Setting f_lineno is allowed for the above events */
            break;
        case PY_MONITORING_EVENT_PY_START:
            PyErr_Format(PyExc_ValueError,
                     "can't jump from the 'call' trace event of a new frame");
            return -1;
        case PY_MONITORING_EVENT_CALL:
        case PY_MONITORING_EVENT_C_RETURN:
            PyErr_SetString(PyExc_ValueError,
                "can't jump during a call");
            return -1;
        case PY_MONITORING_EVENT_PY_RETURN:
        case PY_MONITORING_EVENT_PY_UNWIND:
        case PY_MONITORING_EVENT_PY_THROW:
        case PY_MONITORING_EVENT_RAISE:
        case PY_MONITORING_EVENT_C_RAISE:
        case PY_MONITORING_EVENT_INSTRUCTION:
        case PY_MONITORING_EVENT_EXCEPTION_HANDLED:
            PyErr_Format(PyExc_ValueError,
                "can only jump from a 'line' trace event");
            return -1;
        default:
            PyErr_SetString(PyExc_SystemError,
                "unexpected event type");
            return -1;
    }

    int new_lineno;

    /* Fail if the line falls outside the code block and
        select first line with actual code. */
    int overflow;
    long l_new_lineno = PyLong_AsLongAndOverflow(p_new_lineno, &overflow);
    if (overflow
#if SIZEOF_LONG > SIZEOF_INT
        || l_new_lineno > INT_MAX
        || l_new_lineno < INT_MIN
#endif
    ) {
        PyErr_SetString(PyExc_ValueError,
                        "lineno out of range");
        return -1;
    }
    new_lineno = (int)l_new_lineno;

    if (new_lineno < code->co_firstlineno) {
        PyErr_Format(PyExc_ValueError,
                    "line %d comes before the current code block",
                    new_lineno);
        return -1;
    }

    /* PyCode_NewWithPosOnlyArgs limits co_code to be under INT_MAX so this
     * should never overflow. */
    int len = (int)Py_SIZE(code);
    int *lines = marklines(code, len);
    if (lines == NULL) {
        return -1;
    }

    new_lineno = first_line_not_before(lines, len, new_lineno);
    if (new_lineno < 0) {
        PyErr_Format(PyExc_ValueError,
                    "line %d comes after the current code block",
                    (int)l_new_lineno);
        PyMem_Free(lines);
        return -1;
    }

    int64_t *stacks = mark_stacks(code, len);
    if (stacks == NULL) {
        PyMem_Free(lines);
        return -1;
    }

    int64_t best_stack = OVERFLOWED;
    int best_addr = -1;
    int64_t start_stack = stacks[_PyInterpreterFrame_LASTI(f->f_frame)];
    int err = -1;
    const char *msg = "cannot find bytecode for specified line";
    for (int i = 0; i < len; i++) {
        if (lines[i] == new_lineno) {
            int64_t target_stack = stacks[i];
            if (compatible_stack(start_stack, target_stack)) {
                err = 0;
                if (target_stack > best_stack) {
                    best_stack = target_stack;
                    best_addr = i;
                }
            }
            else if (err < 0) {
                if (start_stack == OVERFLOWED) {
                    msg = "stack to deep to analyze";
                }
                else if (start_stack == UNINITIALIZED) {
                    msg = "can't jump from unreachable code";
                }
                else {
                    msg = explain_incompatible_stack(target_stack);
                    err = 1;
                }
            }
        }
    }
    PyMem_Free(stacks);
    PyMem_Free(lines);
    if (err) {
        PyErr_SetString(PyExc_ValueError, msg);
        return -1;
    }
    // Populate any NULL locals that the compiler might have "proven" to exist
    // in the new location. Rather than crashing or changing co_code, just bind
    // None instead:
    int unbound = 0;
    for (int i = 0; i < code->co_nlocalsplus; i++) {
        // Counting every unbound local is overly-cautious, but a full flow
        // analysis (like we do in the compiler) is probably too expensive:
        unbound += f->f_frame->localsplus[i] == NULL;
    }
    if (unbound) {
        const char *e = "assigning None to %d unbound local%s";
        const char *s = (unbound == 1) ? "" : "s";
        if (PyErr_WarnFormat(PyExc_RuntimeWarning, 0, e, unbound, s)) {
            return -1;
        }
        // Do this in a second pass to avoid writing a bunch of Nones when
        // warnings are being treated as errors and the previous bit raises:
        for (int i = 0; i < code->co_nlocalsplus; i++) {
            if (f->f_frame->localsplus[i] == NULL) {
                f->f_frame->localsplus[i] = Py_NewRef(Py_None);
                unbound--;
            }
        }
        assert(unbound == 0);
    }
    if (is_suspended) {
        /* Account for value popped by yield */
        start_stack = pop_value(start_stack);
    }
    while (start_stack > best_stack) {
        if (top_of_stack(start_stack) == Except) {
            /* Pop exception stack as well as the evaluation stack */
            PyObject *exc = _PyFrame_StackPop(f->f_frame);
            assert(PyExceptionInstance_Check(exc) || exc == Py_None);
            PyThreadState *tstate = _PyThreadState_GET();
            Py_XSETREF(tstate->exc_info->exc_value, exc == Py_None ? NULL : exc);
        }
        else {
            PyObject *v = _PyFrame_StackPop(f->f_frame);
            Py_XDECREF(v);
        }
        start_stack = pop_value(start_stack);
    }
    /* Finally set the new lasti and return OK. */
    f->f_lineno = 0;
    f->f_frame->instr_ptr = _PyCode_CODE(code) + best_addr;
    return 0;
}

static PyObject *
frame_gettrace(PyFrameObject *f, void *closure)
{
    PyObject* trace = f->f_trace;
    if (trace == NULL)
        trace = Py_None;
    return Py_NewRef(trace);
}

static int
frame_settrace(PyFrameObject *f, PyObject* v, void *closure)
{
    if (v == Py_None) {
        v = NULL;
    }
    if (v != f->f_trace) {
        Py_XSETREF(f->f_trace, Py_XNewRef(v));
        if (v != NULL && f->f_trace_opcodes) {
            return _PyEval_SetOpcodeTrace(f, true);
        }
    }
    return 0;
}


static PyGetSetDef frame_getsetlist[] = {
    {"f_back",          (getter)frame_getback, NULL, NULL},
    {"f_locals",        (getter)frame_getlocals, NULL, NULL},
    {"f_lineno",        (getter)frame_getlineno,
                    (setter)frame_setlineno, NULL},
    {"f_trace",         (getter)frame_gettrace, (setter)frame_settrace, NULL},
    {"f_lasti",         (getter)frame_getlasti, NULL, NULL},
    {"f_globals",       (getter)frame_getglobals, NULL, NULL},
    {"f_builtins",      (getter)frame_getbuiltins, NULL, NULL},
    {"f_code",          (getter)frame_getcode, NULL, NULL},
    {"f_trace_opcodes", (getter)frame_gettrace_opcodes, (setter)frame_settrace_opcodes, NULL},
    {0}
};

static void
frame_dealloc(PyFrameObject *f)
{
    /* It is the responsibility of the owning generator/coroutine
     * to have cleared the generator pointer */

    if (_PyObject_GC_IS_TRACKED(f)) {
        _PyObject_GC_UNTRACK(f);
    }

    Py_TRASHCAN_BEGIN(f, frame_dealloc);
    PyObject *co = NULL;

    /* GH-106092: If f->f_frame was on the stack and we reached the maximum
     * nesting depth for deallocations, the trashcan may have delayed this
     * deallocation until after f->f_frame is freed. Avoid dereferencing
     * f->f_frame unless we know it still points to valid memory. */
    _PyInterpreterFrame *frame = (_PyInterpreterFrame *)f->_f_frame_data;

    /* Kill all local variables including specials, if we own them */
    if (f->f_frame == frame && frame->owner == FRAME_OWNED_BY_FRAME_OBJECT) {
        /* Don't clear code object until the end */
        co = frame->f_executable;
        frame->f_executable = NULL;
        Py_CLEAR(frame->f_funcobj);
        Py_CLEAR(frame->f_locals);
        PyObject **locals = _PyFrame_GetLocalsArray(frame);
        for (int i = 0; i < frame->stacktop; i++) {
            Py_CLEAR(locals[i]);
        }
    }
    Py_CLEAR(f->f_back);
    Py_CLEAR(f->f_trace);
    Py_CLEAR(f->f_extra_locals);
    Py_CLEAR(f->f_locals_cache);
    PyObject_GC_Del(f);
    Py_XDECREF(co);
    Py_TRASHCAN_END;
}

static int
frame_traverse(PyFrameObject *f, visitproc visit, void *arg)
{
    Py_VISIT(f->f_back);
    Py_VISIT(f->f_trace);
    Py_VISIT(f->f_extra_locals);
    Py_VISIT(f->f_locals_cache);
    if (f->f_frame->owner != FRAME_OWNED_BY_FRAME_OBJECT) {
        return 0;
    }
    assert(f->f_frame->frame_obj == NULL);
    return _PyFrame_Traverse(f->f_frame, visit, arg);
}

static int
frame_tp_clear(PyFrameObject *f)
{
    Py_CLEAR(f->f_trace);
    Py_CLEAR(f->f_extra_locals);
    Py_CLEAR(f->f_locals_cache);

    /* locals and stack */
    PyObject **locals = _PyFrame_GetLocalsArray(f->f_frame);
    assert(f->f_frame->stacktop >= 0);
    for (int i = 0; i < f->f_frame->stacktop; i++) {
        Py_CLEAR(locals[i]);
    }
    f->f_frame->stacktop = 0;
    Py_CLEAR(f->f_frame->f_locals);
    return 0;
}

static PyObject *
frame_clear(PyFrameObject *f, PyObject *Py_UNUSED(ignored))
{
    if (f->f_frame->owner == FRAME_OWNED_BY_GENERATOR) {
        PyGenObject *gen = _PyFrame_GetGenerator(f->f_frame);
        if (gen->gi_frame_state == FRAME_EXECUTING) {
            goto running;
        }
        if (FRAME_STATE_SUSPENDED(gen->gi_frame_state)) {
            goto suspended;
        }
        _PyGen_Finalize((PyObject *)gen);
    }
    else if (f->f_frame->owner == FRAME_OWNED_BY_THREAD) {
        goto running;
    }
    else {
        assert(f->f_frame->owner == FRAME_OWNED_BY_FRAME_OBJECT);
        (void)frame_tp_clear(f);
    }
    Py_RETURN_NONE;
running:
    PyErr_SetString(PyExc_RuntimeError,
                    "cannot clear an executing frame");
    return NULL;
suspended:
    PyErr_SetString(PyExc_RuntimeError,
                    "cannot clear a suspended frame");
    return NULL;
}

PyDoc_STRVAR(clear__doc__,
"F.clear(): clear most references held by the frame");

static PyObject *
frame_sizeof(PyFrameObject *f, PyObject *Py_UNUSED(ignored))
{
    Py_ssize_t res;
    res = offsetof(PyFrameObject, _f_frame_data) + offsetof(_PyInterpreterFrame, localsplus);
    PyCodeObject *code = _PyFrame_GetCode(f->f_frame);
    res += _PyFrame_NumSlotsForCodeObject(code) * sizeof(PyObject *);
    return PyLong_FromSsize_t(res);
}

PyDoc_STRVAR(sizeof__doc__,
"F.__sizeof__() -> size of F in memory, in bytes");

static PyObject *
frame_repr(PyFrameObject *f)
{
    int lineno = PyFrame_GetLineNumber(f);
    PyCodeObject *code = _PyFrame_GetCode(f->f_frame);
    return PyUnicode_FromFormat(
        "<frame at %p, file %R, line %d, code %S>",
        f, code->co_filename, lineno, code->co_name);
}

static PyMethodDef frame_methods[] = {
    {"clear",           (PyCFunction)frame_clear,       METH_NOARGS,
     clear__doc__},
    {"__sizeof__",      (PyCFunction)frame_sizeof,      METH_NOARGS,
     sizeof__doc__},
    {NULL,              NULL}   /* sentinel */
};

PyTypeObject PyFrame_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "frame",
    offsetof(PyFrameObject, _f_frame_data) +
    offsetof(_PyInterpreterFrame, localsplus),
    sizeof(PyObject *),
    (destructor)frame_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)frame_repr,                       /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)frame_traverse,               /* tp_traverse */
    (inquiry)frame_tp_clear,                    /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    frame_methods,                              /* tp_methods */
    frame_memberlist,                           /* tp_members */
    frame_getsetlist,                           /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
};

static void
init_frame(_PyInterpreterFrame *frame, PyFunctionObject *func, PyObject *locals)
{
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    _PyFrame_Initialize(frame, (PyFunctionObject*)Py_NewRef(func),
                        Py_XNewRef(locals), code, 0);
    frame->previous = NULL;
}

PyFrameObject*
_PyFrame_New_NoTrack(PyCodeObject *code)
{
    CALL_STAT_INC(frame_objects_created);
    int slots = code->co_nlocalsplus + code->co_stacksize;
    PyFrameObject *f = PyObject_GC_NewVar(PyFrameObject, &PyFrame_Type, slots);
    if (f == NULL) {
        return NULL;
    }
    f->f_back = NULL;
    f->f_trace = NULL;
    f->f_trace_lines = 1;
    f->f_trace_opcodes = 0;
    f->f_lineno = 0;
    f->f_extra_locals = NULL;
    f->f_locals_cache = NULL;
    return f;
}

/* Legacy API */
PyFrameObject*
PyFrame_New(PyThreadState *tstate, PyCodeObject *code,
            PyObject *globals, PyObject *locals)
{
    PyObject *builtins = _PyEval_BuiltinsFromGlobals(tstate, globals); // borrowed ref
    if (builtins == NULL) {
        return NULL;
    }
    PyFrameConstructor desc = {
        .fc_globals = globals,
        .fc_builtins = builtins,
        .fc_name = code->co_name,
        .fc_qualname = code->co_name,
        .fc_code = (PyObject *)code,
        .fc_defaults = NULL,
        .fc_kwdefaults = NULL,
        .fc_closure = NULL
    };
    PyFunctionObject *func = _PyFunction_FromConstructor(&desc);
    if (func == NULL) {
        return NULL;
    }
    PyFrameObject *f = _PyFrame_New_NoTrack(code);
    if (f == NULL) {
        Py_DECREF(func);
        return NULL;
    }
    init_frame((_PyInterpreterFrame *)f->_f_frame_data, func, locals);
    f->f_frame = (_PyInterpreterFrame *)f->_f_frame_data;
    f->f_frame->owner = FRAME_OWNED_BY_FRAME_OBJECT;
    // This frame needs to be "complete", so pretend that the first RESUME ran:
    f->f_frame->instr_ptr = _PyCode_CODE(code) + code->_co_firsttraceable + 1;
    assert(!_PyFrame_IsIncomplete(f->f_frame));
    Py_DECREF(func);
    _PyObject_GC_TRACK(f);
    return f;
}

static int
_PyFrame_OpAlreadyRan(_PyInterpreterFrame *frame, int opcode, int oparg)
{
    // This only works when opcode is a non-quickened form:
    assert(_PyOpcode_Deopt[opcode] == opcode);
    int check_oparg = 0;
    for (_Py_CODEUNIT *instruction = _PyCode_CODE(_PyFrame_GetCode(frame));
         instruction < frame->instr_ptr; instruction++)
    {
        int check_opcode = _PyOpcode_Deopt[instruction->op.code];
        check_oparg |= instruction->op.arg;
        if (check_opcode == opcode && check_oparg == oparg) {
            return 1;
        }
        if (check_opcode == EXTENDED_ARG) {
            check_oparg <<= 8;
        }
        else {
            check_oparg = 0;
        }
        instruction += _PyOpcode_Caches[check_opcode];
    }
    return 0;
}


// Initialize frame free variables if needed
static void
frame_init_get_vars(_PyInterpreterFrame *frame)
{
    // COPY_FREE_VARS has no quickened forms, so no need to use _PyOpcode_Deopt
    // here:
    PyCodeObject *co = _PyFrame_GetCode(frame);
    int lasti = _PyInterpreterFrame_LASTI(frame);
    if (!(lasti < 0 && _PyCode_CODE(co)->op.code == COPY_FREE_VARS
          && PyFunction_Check(frame->f_funcobj)))
    {
        /* Free vars are initialized */
        return;
    }

    /* Free vars have not been initialized -- Do that */
    PyObject *closure = ((PyFunctionObject *)frame->f_funcobj)->func_closure;
    int offset = PyUnstable_Code_GetFirstFree(co);
    for (int i = 0; i < co->co_nfreevars; ++i) {
        PyObject *o = PyTuple_GET_ITEM(closure, i);
        frame->localsplus[offset + i] = Py_NewRef(o);
    }
    // COPY_FREE_VARS doesn't have inline CACHEs, either:
    frame->instr_ptr = _PyCode_CODE(_PyFrame_GetCode(frame));
}


static int
frame_get_var(_PyInterpreterFrame *frame, PyCodeObject *co, int i,
              PyObject **pvalue)
{
    _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);

    /* If the namespace is unoptimized, then one of the
       following cases applies:
       1. It does not contain free variables, because it
          uses import * or is a top-level namespace.
       2. It is a class namespace.
       We don't want to accidentally copy free variables
       into the locals dict used by the class.
    */
    if (kind & CO_FAST_FREE && !(co->co_flags & CO_OPTIMIZED)) {
        return 0;
    }

    PyObject *value = frame->localsplus[i];
    if (frame->stacktop) {
        if (kind & CO_FAST_FREE) {
            // The cell was set by COPY_FREE_VARS.
            assert(value != NULL && PyCell_Check(value));
            value = PyCell_GET(value);
        }
        else if (kind & CO_FAST_CELL) {
            // Note that no *_DEREF ops can happen before MAKE_CELL
            // executes.  So there's no need to duplicate the work
            // that MAKE_CELL would otherwise do later, if it hasn't
            // run yet.
            if (value != NULL) {
                if (PyCell_Check(value) &&
                        _PyFrame_OpAlreadyRan(frame, MAKE_CELL, i)) {
                    // (likely) MAKE_CELL must have executed already.
                    value = PyCell_GET(value);
                }
                // (likely) Otherwise it is an arg (kind & CO_FAST_LOCAL),
                // with the initial value set when the frame was created...
                // (unlikely) ...or it was set via the f_locals proxy.
            }
        }
    }
    else {
        assert(value == NULL);
    }
    *pvalue = value;
    return 1;
}


bool
_PyFrame_HasHiddenLocals(_PyInterpreterFrame *frame)
{
    /*
     * This function returns if there are hidden locals introduced by PEP 709,
     * which are the isolated fast locals for inline comprehensions
     */
    PyCodeObject* co = _PyFrame_GetCode(frame);

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);

        if (kind & CO_FAST_HIDDEN) {
            PyObject* value = framelocalsproxy_getval(frame, co, i);

            if (value != NULL) {
                return true;
            }
        }
    }

    return false;
}


PyObject *
_PyFrame_GetLocals(_PyInterpreterFrame *frame)
{
    // We should try to avoid creating the FrameObject if possible.
    // So we check if the frame is a module or class level scope
    PyCodeObject *co = _PyFrame_GetCode(frame);

    if (!(co->co_flags & CO_OPTIMIZED) && !_PyFrame_HasHiddenLocals(frame)) {
        if (frame->f_locals == NULL) {
            // We found cases when f_locals is NULL for non-optimized code.
            // We fill the f_locals with an empty dict to avoid crash until
            // we find the root cause.
            frame->f_locals = PyDict_New();
            if (frame->f_locals == NULL) {
                return NULL;
            }
        }
        return Py_NewRef(frame->f_locals);
    }

    PyFrameObject* f = _PyFrame_GetFrameObject(frame);

    return _PyFrameLocalsProxy_New(f);
}


PyObject *
PyFrame_GetVar(PyFrameObject *frame_obj, PyObject *name)
{
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError, "name must be str, not %s",
                     Py_TYPE(name)->tp_name);
        return NULL;
    }

    _PyInterpreterFrame *frame = frame_obj->f_frame;
    frame_init_get_vars(frame);

    PyCodeObject *co = _PyFrame_GetCode(frame);
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *var_name = PyTuple_GET_ITEM(co->co_localsplusnames, i);
        if (!_PyUnicode_Equal(var_name, name)) {
            continue;
        }

        PyObject *value;  // borrowed reference
        if (!frame_get_var(frame, co, i, &value)) {
            break;
        }
        if (value == NULL) {
            break;
        }
        return Py_NewRef(value);
    }

    PyErr_Format(PyExc_NameError, "variable %R does not exist", name);
    return NULL;
}


PyObject *
PyFrame_GetVarString(PyFrameObject *frame, const char *name)
{
    PyObject *name_obj = PyUnicode_FromString(name);
    if (name_obj == NULL) {
        return NULL;
    }
    PyObject *value = PyFrame_GetVar(frame, name_obj);
    Py_DECREF(name_obj);
    return value;
}


int
PyFrame_FastToLocalsWithError(PyFrameObject *f)
{
    // Nothing to do here, as f_locals is now a write-through proxy in
    // optimized frames. Soft-deprecated, since there's no maintenance hassle.
    return 0;
}

void
PyFrame_FastToLocals(PyFrameObject *f)
{
    // Nothing to do here, as f_locals is now a write-through proxy in
    // optimized frames. Soft-deprecated, since there's no maintenance hassle.
    return;
}

void
PyFrame_LocalsToFast(PyFrameObject *f, int clear)
{
    // Nothing to do here, as f_locals is now a write-through proxy in
    // optimized frames. Soft-deprecated, since there's no maintenance hassle.
    return;
}

int
_PyFrame_IsEntryFrame(PyFrameObject *frame)
{
    assert(frame != NULL);
    _PyInterpreterFrame *f = frame->f_frame;
    assert(!_PyFrame_IsIncomplete(f));
    return f->previous && f->previous->owner == FRAME_OWNED_BY_CSTACK;
}

PyCodeObject *
PyFrame_GetCode(PyFrameObject *frame)
{
    assert(frame != NULL);
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    PyCodeObject *code = _PyFrame_GetCode(frame->f_frame);
    assert(code != NULL);
    return (PyCodeObject*)Py_NewRef(code);
}


PyFrameObject*
PyFrame_GetBack(PyFrameObject *frame)
{
    assert(frame != NULL);
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    PyFrameObject *back = frame->f_back;
    if (back == NULL) {
        _PyInterpreterFrame *prev = frame->f_frame->previous;
        prev = _PyFrame_GetFirstComplete(prev);
        if (prev) {
            back = _PyFrame_GetFrameObject(prev);
        }
    }
    return (PyFrameObject*)Py_XNewRef(back);
}

PyObject*
PyFrame_GetLocals(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    return frame_getlocals(frame, NULL);
}

PyObject*
PyFrame_GetGlobals(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    return frame_getglobals(frame, NULL);
}

PyObject*
PyFrame_GetBuiltins(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    return frame_getbuiltins(frame, NULL);
}

int
PyFrame_GetLasti(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    int lasti = _PyInterpreterFrame_LASTI(frame->f_frame);
    if (lasti < 0) {
        return -1;
    }
    return lasti * sizeof(_Py_CODEUNIT);
}

PyObject *
PyFrame_GetGenerator(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    if (frame->f_frame->owner != FRAME_OWNED_BY_GENERATOR) {
        return NULL;
    }
    PyGenObject *gen = _PyFrame_GetGenerator(frame->f_frame);
    return Py_NewRef(gen);
}

PyObject*
_PyEval_BuiltinsFromGlobals(PyThreadState *tstate, PyObject *globals)
{
    PyObject *builtins = PyDict_GetItemWithError(globals, &_Py_ID(__builtins__));
    if (builtins) {
        if (PyModule_Check(builtins)) {
            builtins = _PyModule_GetDict(builtins);
            assert(builtins != NULL);
        }
        return builtins;
    }
    if (PyErr_Occurred()) {
        return NULL;
    }

    return _PyEval_GetBuiltins(tstate);
}
