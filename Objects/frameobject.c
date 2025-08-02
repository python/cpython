/* Frame object implementation */

#include "Python.h"
#include "pycore_cell.h"          // PyCell_GetRef()
#include "pycore_ceval.h"         // _PyEval_SetOpcodeTrace()
#include "pycore_code.h"          // CO_FAST_LOCAL
#include "pycore_dict.h"          // _PyDict_LoadBuiltinsFromGlobals()
#include "pycore_frame.h"         // PyFrameObject
#include "pycore_function.h"      // _PyFunction_FromConstructor()
#include "pycore_genobject.h"     // _PyGen_GetGeneratorFromFrame()
#include "pycore_interpframe.h"   // _PyFrame_GetLocalsArray()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_optimizer.h"     // _Py_Executors_InvalidateDependency()
#include "pycore_unicodeobject.h" // _PyUnicode_Equal()

#include "frameobject.h"          // PyFrameLocalsProxyObject
#include "opcode.h"               // EXTENDED_ARG

#include "clinic/frameobject.c.h"


#define PyFrameObject_CAST(op)  \
    (assert(PyObject_TypeCheck((op), &PyFrame_Type)), (PyFrameObject *)(op))

#define PyFrameLocalsProxyObject_CAST(op)                           \
    (                                                               \
        assert(PyObject_TypeCheck((op), &PyFrameLocalsProxy_Type)), \
        (PyFrameLocalsProxyObject *)(op)                            \
    )

#define OFF(x) offsetof(PyFrameObject, x)

/*[clinic input]
class frame "PyFrameObject *" "&PyFrame_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2d1dbf2e06cf351f]*/


// Returns new reference or NULL
static PyObject *
framelocalsproxy_getval(_PyInterpreterFrame *frame, PyCodeObject *co, int i)
{
    _PyStackRef *fast = _PyFrame_GetLocalsArray(frame);
    _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);

    PyObject *value = PyStackRef_AsPyObjectBorrow(fast[i]);
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
        value = PyCell_GetRef((PyCellObject *)cell);
    }
    else {
        Py_XINCREF(value);
    }

    if (value == NULL) {
        return NULL;
    }

    return value;
}

static bool
framelocalsproxy_hasval(_PyInterpreterFrame *frame, PyCodeObject *co, int i)
{
    PyObject *value = framelocalsproxy_getval(frame, co, i);
    if (value == NULL) {
        return false;
    }
    Py_DECREF(value);
    return true;
}

static int
framelocalsproxy_getkeyindex(PyFrameObject *frame, PyObject *key, bool read, PyObject **value_ptr)
{
    /*
     * Returns -2 (!) if an error occurred; exception will be set.
     * Returns the fast locals index of the key on success:
     *   - if read == true, returns the index if the value is not NULL
     *   - if read == false, returns the index if the value is not hidden
     * Otherwise returns -1.
     *
     * If read == true and value_ptr is not NULL, *value_ptr is set to
     * the value of the key if it is found (with a new reference).
     */

    // value_ptr should only be given if we are reading the value
    assert(read || value_ptr == NULL);

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
                PyObject *value = framelocalsproxy_getval(frame->f_frame, co, i);
                if (value != NULL) {
                    if (value_ptr != NULL) {
                        *value_ptr = value;
                    }
                    else {
                        Py_DECREF(value);
                    }
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
                PyObject *value = framelocalsproxy_getval(frame->f_frame, co, i);
                if (value != NULL) {
                    if (value_ptr != NULL) {
                        *value_ptr = value;
                    }
                    else {
                        Py_DECREF(value);
                    }
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
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
    PyObject *value = NULL;

    int i = framelocalsproxy_getkeyindex(frame, key, true, &value);
    if (i == -2) {
        return NULL;
    }
    if (i >= 0) {
        assert(value != NULL);
        return value;
    }
    assert(value == NULL);

    // Okay not in the fast locals, try extra locals

    PyObject *extra = frame->f_extra_locals;
    if (extra != NULL) {
        if (PyDict_GetItemRef(extra, key, &value) < 0) {
            return NULL;
        }
        if (value != NULL) {
            return value;
        }
    }

    PyErr_Format(PyExc_KeyError, "local variable '%R' is not defined", key);
    return NULL;
}

static int
add_overwritten_fast_local(PyFrameObject *frame, PyObject *obj)
{
    Py_ssize_t new_size;
    if (frame->f_overwritten_fast_locals == NULL) {
        new_size = 1;
    }
    else {
        Py_ssize_t size = PyTuple_Size(frame->f_overwritten_fast_locals);
        if (size == -1) {
            return -1;
        }
        new_size = size + 1;
    }
    PyObject *new_tuple = PyTuple_New(new_size);
    if (new_tuple == NULL) {
        return -1;
    }
    for (Py_ssize_t i = 0; i < new_size - 1; i++) {
        PyObject *o = PyTuple_GET_ITEM(frame->f_overwritten_fast_locals, i);
        PyTuple_SET_ITEM(new_tuple, i, Py_NewRef(o));
    }
    PyTuple_SET_ITEM(new_tuple, new_size - 1, Py_NewRef(obj));
    Py_XSETREF(frame->f_overwritten_fast_locals, new_tuple);
    return 0;
}

static int
framelocalsproxy_setitem(PyObject *self, PyObject *key, PyObject *value)
{
    /* Merge locals into fast locals */
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
    _PyStackRef *fast = _PyFrame_GetLocalsArray(frame->f_frame);
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);

    int i = framelocalsproxy_getkeyindex(frame, key, false, NULL);
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
        _PyStackRef oldvalue = fast[i];
        PyObject *cell = NULL;
        if (kind == CO_FAST_FREE) {
            // The cell was set when the frame was created from
            // the function's closure.
            assert(!PyStackRef_IsNull(oldvalue) && PyCell_Check(PyStackRef_AsPyObjectBorrow(oldvalue)));
            cell = PyStackRef_AsPyObjectBorrow(oldvalue);
        } else if (kind & CO_FAST_CELL && !PyStackRef_IsNull(oldvalue)) {
            PyObject *as_obj = PyStackRef_AsPyObjectBorrow(oldvalue);
            if (PyCell_Check(as_obj)) {
                cell = as_obj;
            }
        }
        if (cell != NULL) {
            Py_XINCREF(value);
            PyCell_SetTakeRef((PyCellObject *)cell, value);
        } else if (value != PyStackRef_AsPyObjectBorrow(oldvalue)) {
            PyObject *old_obj = PyStackRef_AsPyObjectBorrow(fast[i]);
            if (old_obj != NULL && !_Py_IsImmortal(old_obj)) {
                if (add_overwritten_fast_local(frame, old_obj) < 0) {
                    return -1;
                }
                PyStackRef_CLOSE(fast[i]);
            }
            fast[i] = PyStackRef_FromPyObjectNew(value);
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
framelocalsproxy_keys(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);
    PyObject *names = PyList_New(0);
    if (names == NULL) {
        return NULL;
    }

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        if (framelocalsproxy_hasval(frame->f_frame, co, i)) {
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
    PyFrameLocalsProxyObject *proxy = PyFrameLocalsProxyObject_CAST(self);
    PyObject_GC_UnTrack(self);
    Py_CLEAR(proxy->frame);
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
    PyFrameLocalsProxyObject *proxy = PyFrameLocalsProxyObject_CAST(self);
    Py_CLEAR(proxy->frame);
    return 0;
}

static int
framelocalsproxy_visit(PyObject *self, visitproc visit, void *arg)
{
    PyFrameLocalsProxyObject *proxy = PyFrameLocalsProxyObject_CAST(self);
    Py_VISIT(proxy->frame);
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
framelocalsproxy_richcompare(PyObject *lhs, PyObject *rhs, int op)
{
    PyFrameLocalsProxyObject *self = PyFrameLocalsProxyObject_CAST(lhs);
    if (PyFrameLocalsProxy_Check(rhs)) {
        PyFrameLocalsProxyObject *other = (PyFrameLocalsProxyObject *)rhs;
        bool result = self->frame == other->frame;
        if (op == Py_EQ) {
            return PyBool_FromLong(result);
        } else if (op == Py_NE) {
            return PyBool_FromLong(!result);
        }
    } else if (PyDict_Check(rhs)) {
        PyObject *dct = PyDict_New();
        if (dct == NULL) {
            return NULL;
        }

        if (PyDict_Update(dct, lhs) < 0) {
            Py_DECREF(dct);
            return NULL;
        }

        PyObject *result = PyObject_RichCompare(dct, rhs, op);
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

static PyObject *
framelocalsproxy_values(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
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
                Py_DECREF(value);
                return NULL;
            }
            Py_DECREF(value);
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
framelocalsproxy_items(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
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
                Py_DECREF(value);
                return NULL;
            }

            if (PyList_Append(items, pair) < 0) {
                Py_DECREF(items);
                Py_DECREF(pair);
                Py_DECREF(value);
                return NULL;
            }

            Py_DECREF(pair);
            Py_DECREF(value);
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
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);
    Py_ssize_t size = 0;

    if (frame->f_extra_locals != NULL) {
        assert(PyDict_Check(frame->f_extra_locals));
        size += PyDict_Size(frame->f_extra_locals);
    }

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        if (framelocalsproxy_hasval(frame->f_frame, co, i)) {
            size++;
        }
    }
    return size;
}

static int
framelocalsproxy_contains(PyObject *self, PyObject *key)
{
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;

    int i = framelocalsproxy_getkeyindex(frame, key, true, NULL);
    if (i == -2) {
        return -1;
    }
    if (i >= 0) {
        return 1;
    }

    PyObject *extra = frame->f_extra_locals;
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

    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;

    int i = framelocalsproxy_getkeyindex(frame, key, false, NULL);
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
framelocalsproxy_reversed(PyObject *self, PyObject *Py_UNUSED(ignored))
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
    .mp_length = framelocalsproxy_length,
    .mp_subscript = framelocalsproxy_getitem,
    .mp_ass_subscript = framelocalsproxy_setitem,
};

static PyMethodDef framelocalsproxy_methods[] = {
    {"__contains__", framelocalsproxy___contains__, METH_O | METH_COEXIST, NULL},
    {"__getitem__", framelocalsproxy_getitem, METH_O | METH_COEXIST, NULL},
    {"update", framelocalsproxy_update, METH_O, NULL},
    {"__reversed__", framelocalsproxy_reversed, METH_NOARGS, NULL},
    {"copy", framelocalsproxy_copy, METH_NOARGS, NULL},
    {"keys", framelocalsproxy_keys, METH_NOARGS, NULL},
    {"values", framelocalsproxy_values, METH_NOARGS, NULL},
    {"items", _PyCFunction_CAST(framelocalsproxy_items), METH_NOARGS, NULL},
    {"get", _PyCFunction_CAST(framelocalsproxy_get), METH_FASTCALL, NULL},
    {"pop", _PyCFunction_CAST(framelocalsproxy_pop), METH_FASTCALL, NULL},
    {
        "setdefault",
        _PyCFunction_CAST(framelocalsproxy_setdefault),
        METH_FASTCALL,
        NULL
    },
    {NULL, NULL}   /* sentinel */
};

PyDoc_STRVAR(framelocalsproxy_doc,
"FrameLocalsProxy($frame)\n"
"--\n"
"\n"
"Create a write-through view of the locals dictionary for a frame.\n"
"\n"
"  frame\n"
"    the frame object to wrap.");

PyTypeObject PyFrameLocalsProxy_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "FrameLocalsProxy",
    .tp_basicsize = sizeof(PyFrameLocalsProxyObject),
    .tp_dealloc = framelocalsproxy_dealloc,
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
    .tp_doc = framelocalsproxy_doc,
};

PyObject *
_PyFrameLocalsProxy_New(PyFrameObject *frame)
{
    PyObject* args = PyTuple_Pack(1, frame);
    if (args == NULL) {
        return NULL;
    }

    PyObject* proxy = framelocalsproxy_new(&PyFrameLocalsProxy_Type, args, NULL);
    Py_DECREF(args);
    return proxy;
}

static PyMemberDef frame_memberlist[] = {
    {"f_trace_lines",   Py_T_BOOL,         OFF(f_trace_lines), 0},
    {NULL}      /* Sentinel */
};

/*[clinic input]
@critical_section
@getter
frame.f_locals as frame_locals

Return the mapping used by the frame to look up local variables.
[clinic start generated code]*/

static PyObject *
frame_locals_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=b4ace8bb4cae71f4 input=7bd444d0dc8ddf44]*/
{
    assert(!_PyFrame_IsIncomplete(self->f_frame));

    PyCodeObject *co = _PyFrame_GetCode(self->f_frame);

    if (!(co->co_flags & CO_OPTIMIZED) && !_PyFrame_HasHiddenLocals(self->f_frame)) {
        if (self->f_frame->f_locals == NULL) {
            // We found cases when f_locals is NULL for non-optimized code.
            // We fill the f_locals with an empty dict to avoid crash until
            // we find the root cause.
            self->f_frame->f_locals = PyDict_New();
            if (self->f_frame->f_locals == NULL) {
                return NULL;
            }
        }
        return Py_NewRef(self->f_frame->f_locals);
    }

    return _PyFrameLocalsProxy_New(self);
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

/*[clinic input]
@critical_section
@getter
frame.f_lineno as frame_lineno

Return the current line number in the frame.
[clinic start generated code]*/

static PyObject *
frame_lineno_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=70f35de5ac7ad630 input=87b9ec648b742936]*/
{
    int lineno = PyFrame_GetLineNumber(self);
    if (lineno < 0) {
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(lineno);
}

/*[clinic input]
@critical_section
@getter
frame.f_lasti as frame_lasti

Return the index of the last attempted instruction in the frame.
[clinic start generated code]*/

static PyObject *
frame_lasti_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=03275b4f0327d1a2 input=0225ed49cb1fbeeb]*/
{
    int lasti = _PyInterpreterFrame_LASTI(self->f_frame);
    if (lasti < 0) {
        return PyLong_FromLong(-1);
    }
    return PyLong_FromLong(lasti * sizeof(_Py_CODEUNIT));
}

/*[clinic input]
@critical_section
@getter
frame.f_globals as frame_globals

Return the global variables in the frame.
[clinic start generated code]*/

static PyObject *
frame_globals_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=7758788c32885528 input=7fff7241357d314d]*/
{
    PyObject *globals = self->f_frame->f_globals;
    if (globals == NULL) {
        globals = Py_None;
    }
    return Py_NewRef(globals);
}

/*[clinic input]
@critical_section
@getter
frame.f_builtins as frame_builtins

Return the built-in variables in the frame.
[clinic start generated code]*/

static PyObject *
frame_builtins_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=45362faa6d42c702 input=27c696d6ffcad2c7]*/
{
    PyObject *builtins = self->f_frame->f_builtins;
    if (builtins == NULL) {
        builtins = Py_None;
    }
    return Py_NewRef(builtins);
}

/*[clinic input]
@getter
frame.f_code as frame_code

Return the code object being executed in this frame.
[clinic start generated code]*/

static PyObject *
frame_code_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=a5ed6207395a8cef input=e127e7098c124816]*/
{
    if (PySys_Audit("object.__getattr__", "Os", self, "f_code") < 0) {
        return NULL;
    }
    return (PyObject *)PyFrame_GetCode(self);
}

/*[clinic input]
@critical_section
@getter
frame.f_back as frame_back
[clinic start generated code]*/

static PyObject *
frame_back_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=3a84c22a55a63c79 input=9e528570d0e1f44a]*/
{
    PyObject *res = (PyObject *)PyFrame_GetBack(self);
    if (res == NULL) {
        Py_RETURN_NONE;
    }
    return res;
}

/*[clinic input]
@critical_section
@getter
frame.f_trace_opcodes as frame_trace_opcodes

Return True if opcode tracing is enabled, False otherwise.
[clinic start generated code]*/

static PyObject *
frame_trace_opcodes_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=53ff41d09cc32e87 input=4eb91dc88e04677a]*/
{
    return self->f_trace_opcodes ? Py_True : Py_False;
}

/*[clinic input]
@critical_section
@setter
frame.f_trace_opcodes as frame_trace_opcodes
[clinic start generated code]*/

static int
frame_trace_opcodes_set_impl(PyFrameObject *self, PyObject *value)
/*[clinic end generated code: output=92619da2bfccd449 input=7e286eea3c0333ff]*/
{
    if (!PyBool_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute value type must be bool");
        return -1;
    }
    if (value == Py_True) {
        self->f_trace_opcodes = 1;
        if (self->f_trace) {
            return _PyEval_SetOpcodeTrace(self, true);
        }
    }
    else {
        self->f_trace_opcodes = 0;
        return _PyEval_SetOpcodeTrace(self, false);
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
    int64_t *stacks = PyMem_New(int64_t, len+1);

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
        for (int i = 0; i < len;) {
            int j;
            int64_t next_stack = stacks[i];
            _Py_CODEUNIT inst = _Py_GetBaseCodeUnit(code_obj, i);
            int opcode = inst.op.code;
            int oparg = 0;
            while (opcode == EXTENDED_ARG) {
                oparg = (oparg << 8) | inst.op.arg;
                i++;
                inst = _Py_GetBaseCodeUnit(code_obj, i);
                opcode = inst.op.code;
                stacks[i] = next_stack;
            }
            oparg = (oparg << 8) | inst.op.arg;
            int next_i = i + _PyOpcode_Caches[opcode] + 1;
            if (next_stack == UNINITIALIZED) {
                i = next_i;
                continue;
            }
            switch (opcode) {
                case POP_JUMP_IF_FALSE:
                case POP_JUMP_IF_TRUE:
                case POP_JUMP_IF_NONE:
                case POP_JUMP_IF_NOT_NONE:
                {
                    int64_t target_stack;
                    j = next_i + oparg;
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
                    next_stack = push_value(pop_value(next_stack), Iterator);
                    next_stack = push_value(next_stack, Iterator);
                    stacks[next_i] = next_stack;
                    break;
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
        PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame->f_frame);
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
/*[clinic input]
@critical_section
@setter
frame.f_lineno as frame_lineno
[clinic start generated code]*/

static int
frame_lineno_set_impl(PyFrameObject *self, PyObject *value)
/*[clinic end generated code: output=e64c86ff6be64292 input=36ed3c896b27fb91]*/
{
    PyCodeObject *code = _PyFrame_GetCode(self->f_frame);
    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }
    /* f_lineno must be an integer. */
    if (!PyLong_CheckExact(value)) {
        PyErr_SetString(PyExc_ValueError,
                        "lineno must be an integer");
        return -1;
    }

    bool is_suspended = frame_is_suspended(self);
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
        case PY_MONITORING_EVENT_BRANCH_LEFT:
        case PY_MONITORING_EVENT_BRANCH_RIGHT:
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
    long l_new_lineno = PyLong_AsLongAndOverflow(value, &overflow);
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
    int64_t start_stack = stacks[_PyInterpreterFrame_LASTI(self->f_frame)];
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
        unbound += PyStackRef_IsNull(self->f_frame->localsplus[i]);
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
            if (PyStackRef_IsNull(self->f_frame->localsplus[i])) {
                self->f_frame->localsplus[i] = PyStackRef_None;
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
        _PyStackRef popped = _PyFrame_StackPop(self->f_frame);
        if (top_of_stack(start_stack) == Except) {
            /* Pop exception stack as well as the evaluation stack */
            PyObject *exc = PyStackRef_AsPyObjectBorrow(popped);
            assert(PyExceptionInstance_Check(exc) || exc == Py_None);
            PyThreadState *tstate = _PyThreadState_GET();
            Py_XSETREF(tstate->exc_info->exc_value, exc == Py_None ? NULL : exc);
        }
        else {
            PyStackRef_XCLOSE(popped);
        }
        start_stack = pop_value(start_stack);
    }
    /* Finally set the new lasti and return OK. */
    self->f_lineno = 0;
    self->f_frame->instr_ptr = _PyFrame_GetBytecode(self->f_frame) + best_addr;
    return 0;
}

/*[clinic input]
@critical_section
@getter
frame.f_trace as frame_trace

Return the trace function for this frame, or None if no trace function is set.
[clinic start generated code]*/

static PyObject *
frame_trace_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=5475cbfce07826cd input=f382612525829773]*/
{
    PyObject* trace = self->f_trace;
    if (trace == NULL) {
        trace = Py_None;
    }
    return Py_NewRef(trace);
}

/*[clinic input]
@critical_section
@setter
frame.f_trace as frame_trace
[clinic start generated code]*/

static int
frame_trace_set_impl(PyFrameObject *self, PyObject *value)
/*[clinic end generated code: output=d6fe08335cf76ae4 input=d96a18bda085707f]*/
{
    if (value == Py_None) {
        value = NULL;
    }
    if (value != self->f_trace) {
        Py_XSETREF(self->f_trace, Py_XNewRef(value));
        if (value != NULL && self->f_trace_opcodes) {
            return _PyEval_SetOpcodeTrace(self, true);
        }
    }
    return 0;
}

/*[clinic input]
@critical_section
@getter
frame.f_generator as frame_generator

Return the generator or coroutine associated with this frame, or None.
[clinic start generated code]*/

static PyObject *
frame_generator_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=97aeb2392562e55b input=00a2bd008b239ab0]*/
{
    if (self->f_frame->owner == FRAME_OWNED_BY_GENERATOR) {
        PyObject *gen = (PyObject *)_PyGen_GetGeneratorFromFrame(self->f_frame);
        return Py_NewRef(gen);
    }
    Py_RETURN_NONE;
}


static PyGetSetDef frame_getsetlist[] = {
    FRAME_BACK_GETSETDEF
    FRAME_LOCALS_GETSETDEF
    FRAME_LINENO_GETSETDEF
    FRAME_TRACE_GETSETDEF
    FRAME_LASTI_GETSETDEF
    FRAME_GLOBALS_GETSETDEF
    FRAME_BUILTINS_GETSETDEF
    FRAME_CODE_GETSETDEF
    FRAME_TRACE_OPCODES_GETSETDEF
    FRAME_GENERATOR_GETSETDEF
    {0}
};

static void
frame_dealloc(PyObject *op)
{
    /* It is the responsibility of the owning generator/coroutine
     * to have cleared the generator pointer */
    PyFrameObject *f = PyFrameObject_CAST(op);
    if (_PyObject_GC_IS_TRACKED(f)) {
        _PyObject_GC_UNTRACK(f);
    }

    /* GH-106092: If f->f_frame was on the stack and we reached the maximum
     * nesting depth for deallocations, the trashcan may have delayed this
     * deallocation until after f->f_frame is freed. Avoid dereferencing
     * f->f_frame unless we know it still points to valid memory. */
    _PyInterpreterFrame *frame = (_PyInterpreterFrame *)f->_f_frame_data;

    /* Kill all local variables including specials, if we own them */
    if (f->f_frame == frame && frame->owner == FRAME_OWNED_BY_FRAME_OBJECT) {
        PyStackRef_CLEAR(frame->f_executable);
        PyStackRef_CLEAR(frame->f_funcobj);
        Py_CLEAR(frame->f_locals);
        _PyStackRef *locals = _PyFrame_GetLocalsArray(frame);
        _PyStackRef *sp = frame->stackpointer;
        while (sp > locals) {
            sp--;
            PyStackRef_CLEAR(*sp);
        }
    }
    Py_CLEAR(f->f_back);
    Py_CLEAR(f->f_trace);
    Py_CLEAR(f->f_extra_locals);
    Py_CLEAR(f->f_locals_cache);
    Py_CLEAR(f->f_overwritten_fast_locals);
    PyObject_GC_Del(f);
}

static int
frame_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyFrameObject *f = PyFrameObject_CAST(op);
    Py_VISIT(f->f_back);
    Py_VISIT(f->f_trace);
    Py_VISIT(f->f_extra_locals);
    Py_VISIT(f->f_locals_cache);
    Py_VISIT(f->f_overwritten_fast_locals);
    if (f->f_frame->owner != FRAME_OWNED_BY_FRAME_OBJECT) {
        return 0;
    }
    assert(f->f_frame->frame_obj == NULL);
    return _PyFrame_Traverse(f->f_frame, visit, arg);
}

static int
frame_tp_clear(PyObject *op)
{
    PyFrameObject *f = PyFrameObject_CAST(op);
    Py_CLEAR(f->f_trace);
    Py_CLEAR(f->f_extra_locals);
    Py_CLEAR(f->f_locals_cache);
    Py_CLEAR(f->f_overwritten_fast_locals);

    /* locals and stack */
    _PyStackRef *locals = _PyFrame_GetLocalsArray(f->f_frame);
    _PyStackRef *sp = f->f_frame->stackpointer;
    assert(sp >= locals);
    while (sp > locals) {
        sp--;
        PyStackRef_CLEAR(*sp);
    }
    f->f_frame->stackpointer = locals;
    Py_CLEAR(f->f_frame->f_locals);
    return 0;
}

/*[clinic input]
@critical_section
frame.clear

Clear all references held by the frame.
[clinic start generated code]*/

static PyObject *
frame_clear_impl(PyFrameObject *self)
/*[clinic end generated code: output=864c662f16e9bfcc input=c358f9cff5f9b681]*/
{
    if (self->f_frame->owner == FRAME_OWNED_BY_GENERATOR) {
        PyGenObject *gen = _PyGen_GetGeneratorFromFrame(self->f_frame);
        if (gen->gi_frame_state == FRAME_EXECUTING) {
            goto running;
        }
        if (FRAME_STATE_SUSPENDED(gen->gi_frame_state)) {
            goto suspended;
        }
        _PyGen_Finalize((PyObject *)gen);
    }
    else if (self->f_frame->owner == FRAME_OWNED_BY_THREAD) {
        goto running;
    }
    else {
        assert(self->f_frame->owner == FRAME_OWNED_BY_FRAME_OBJECT);
        (void)frame_tp_clear((PyObject *)self);
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

/*[clinic input]
@critical_section
frame.__sizeof__

Return the size of the frame in memory, in bytes.
[clinic start generated code]*/

static PyObject *
frame___sizeof___impl(PyFrameObject *self)
/*[clinic end generated code: output=82948688e81078e2 input=908f90a83e73131d]*/
{
    Py_ssize_t res;
    res = offsetof(PyFrameObject, _f_frame_data) + offsetof(_PyInterpreterFrame, localsplus);
    PyCodeObject *code = _PyFrame_GetCode(self->f_frame);
    res += _PyFrame_NumSlotsForCodeObject(code) * sizeof(PyObject *);
    return PyLong_FromSsize_t(res);
}

static PyObject *
frame_repr(PyObject *op)
{
    PyFrameObject *f = PyFrameObject_CAST(op);
    int lineno = PyFrame_GetLineNumber(f);
    PyCodeObject *code = _PyFrame_GetCode(f->f_frame);
    return PyUnicode_FromFormat(
        "<frame at %p, file %R, line %d, code %S>",
        f, code->co_filename, lineno, code->co_name);
}

static PyMethodDef frame_methods[] = {
    FRAME_CLEAR_METHODDEF
    FRAME___SIZEOF___METHODDEF
    {NULL, NULL}  /* sentinel */
};

PyTypeObject PyFrame_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "frame",
    offsetof(PyFrameObject, _f_frame_data) +
    offsetof(_PyInterpreterFrame, localsplus),
    sizeof(PyObject *),
    frame_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    frame_repr,                                 /* tp_repr */
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
    frame_traverse,                             /* tp_traverse */
    frame_tp_clear,                             /* tp_clear */
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
init_frame(PyThreadState *tstate, _PyInterpreterFrame *frame,
           PyFunctionObject *func, PyObject *locals)
{
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    _PyFrame_Initialize(tstate, frame, PyStackRef_FromPyObjectNew(func),
                        Py_XNewRef(locals), code, 0, NULL);
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
    f->f_overwritten_fast_locals = NULL;
    return f;
}

/* Legacy API */
PyFrameObject*
PyFrame_New(PyThreadState *tstate, PyCodeObject *code,
            PyObject *globals, PyObject *locals)
{
    PyObject *builtins = _PyDict_LoadBuiltinsFromGlobals(globals);
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
    _Py_DECREF_BUILTINS(builtins);
    if (func == NULL) {
        return NULL;
    }
    PyFrameObject *f = _PyFrame_New_NoTrack(code);
    if (f == NULL) {
        Py_DECREF(func);
        return NULL;
    }
    init_frame(tstate, (_PyInterpreterFrame *)f->_f_frame_data, func, locals);
    f->f_frame = (_PyInterpreterFrame *)f->_f_frame_data;
    f->f_frame->owner = FRAME_OWNED_BY_FRAME_OBJECT;
    // This frame needs to be "complete", so pretend that the first RESUME ran:
    f->f_frame->instr_ptr = _PyCode_CODE(code) + code->_co_firsttraceable + 1;
    assert(!_PyFrame_IsIncomplete(f->f_frame));
    Py_DECREF(func);
    _PyObject_GC_TRACK(f);
    return f;
}

// Initialize frame free variables if needed
static void
frame_init_get_vars(_PyInterpreterFrame *frame)
{
    // COPY_FREE_VARS has no quickened forms, so no need to use _PyOpcode_Deopt
    // here:
    PyCodeObject *co = _PyFrame_GetCode(frame);
    int lasti = _PyInterpreterFrame_LASTI(frame);
    if (!(lasti < 0
          && _PyFrame_GetBytecode(frame)->op.code == COPY_FREE_VARS
          && PyStackRef_FunctionCheck(frame->f_funcobj)))
    {
        /* Free vars are initialized */
        return;
    }

    /* Free vars have not been initialized -- Do that */
    PyFunctionObject *func = _PyFrame_GetFunction(frame);
    PyObject *closure = func->func_closure;
    int offset = PyUnstable_Code_GetFirstFree(co);
    for (int i = 0; i < co->co_nfreevars; ++i) {
        PyObject *o = PyTuple_GET_ITEM(closure, i);
        frame->localsplus[offset + i] = PyStackRef_FromPyObjectNew(o);
    }
    // COPY_FREE_VARS doesn't have inline CACHEs, either:
    frame->instr_ptr = _PyFrame_GetBytecode(frame);
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

    PyObject *value = NULL;
    if (frame->stackpointer == NULL || frame->stackpointer > frame->localsplus + i) {
        value = PyStackRef_AsPyObjectBorrow(frame->localsplus[i]);
        if (kind & CO_FAST_FREE) {
            // The cell was set by COPY_FREE_VARS.
            assert(value != NULL && PyCell_Check(value));
            value = PyCell_GetRef((PyCellObject *)value);
        }
        else if (kind & CO_FAST_CELL) {
            if (value != NULL) {
                if (PyCell_Check(value)) {
                    assert(!_PyFrame_IsIncomplete(frame));
                    value = PyCell_GetRef((PyCellObject *)value);
                }
                else {
                    // (likely) Otherwise it is an arg (kind & CO_FAST_LOCAL),
                    // with the initial value set when the frame was created...
                    // (unlikely) ...or it was set via the f_locals proxy.
                    Py_INCREF(value);
                }
            }
        }
        else {
            Py_XINCREF(value);
        }
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
            if (framelocalsproxy_hasval(frame, co, i)) {
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

        PyObject *value;
        if (!frame_get_var(frame, co, i, &value)) {
            break;
        }
        if (value == NULL) {
            break;
        }
        return value;
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
    return f->previous && f->previous->owner == FRAME_OWNED_BY_INTERPRETER;
}

PyCodeObject *
PyFrame_GetCode(PyFrameObject *frame)
{
    assert(frame != NULL);
    PyObject *code;
    Py_BEGIN_CRITICAL_SECTION(frame);
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    code = Py_NewRef(_PyFrame_GetCode(frame->f_frame));
    Py_END_CRITICAL_SECTION();
    return (PyCodeObject *)code;
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
    return frame_locals_get((PyObject *)frame, NULL);
}

PyObject*
PyFrame_GetGlobals(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    return frame_globals_get((PyObject *)frame, NULL);
}

PyObject*
PyFrame_GetBuiltins(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    return frame_builtins_get((PyObject *)frame, NULL);
}

int
PyFrame_GetLasti(PyFrameObject *frame)
{
    int ret;
    Py_BEGIN_CRITICAL_SECTION(frame);
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    int lasti = _PyInterpreterFrame_LASTI(frame->f_frame);
    ret = lasti < 0 ? -1 : lasti * (int)sizeof(_Py_CODEUNIT);
    Py_END_CRITICAL_SECTION();
    return ret;
}

PyObject *
PyFrame_GetGenerator(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    return frame_generator_get((PyObject *)frame, NULL);
}
