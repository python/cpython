/* InterpreterID object */

#include "Python.h"
#include "internal/pycore_pystate.h"
#include "interpreteridobject.h"


int64_t
_Py_CoerceID(PyObject *orig)
{
    PyObject *pyid = PyNumber_Long(orig);
    if (pyid == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Format(PyExc_TypeError,
                         "'id' must be a non-negative int, got %R", orig);
        }
        else {
            PyErr_Format(PyExc_ValueError,
                         "'id' must be a non-negative int, got %R", orig);
        }
        return -1;
    }
    int64_t id = PyLong_AsLongLong(pyid);
    Py_DECREF(pyid);
    if (id == -1 && PyErr_Occurred() != NULL) {
        if (!PyErr_ExceptionMatches(PyExc_OverflowError)) {
            PyErr_Format(PyExc_ValueError,
                         "'id' must be a non-negative int, got %R", orig);
        }
        return -1;
    }
    if (id < 0) {
        PyErr_Format(PyExc_ValueError,
                     "'id' must be a non-negative int, got %R", orig);
        return -1;
    }
    return id;
}

typedef struct interpid {
    PyObject_HEAD
    int64_t id;
} interpid;

static interpid *
newinterpid(PyTypeObject *cls, int64_t id, int force)
{
    PyInterpreterState *interp = _PyInterpreterState_LookUpID(id);
    if (interp == NULL) {
        if (force) {
            PyErr_Clear();
        }
        else {
            return NULL;
        }
    }

    interpid *self = PyObject_New(interpid, cls);
    if (self == NULL) {
        return NULL;
    }
    self->id = id;

    if (interp != NULL) {
        _PyInterpreterState_IDIncref(interp);
    }
    return self;
}

static PyObject *
interpid_new(PyTypeObject *cls, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "force", NULL};
    PyObject *idobj;
    int force = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O|$p:InterpreterID.__init__", kwlist,
                                     &idobj, &force)) {
        return NULL;
    }

    // Coerce and check the ID.
    int64_t id;
    if (PyObject_TypeCheck(idobj, &_PyInterpreterID_Type)) {
        id = ((interpid *)idobj)->id;
    }
    else {
        id = _Py_CoerceID(idobj);
        if (id < 0) {
            return NULL;
        }
    }

    return (PyObject *)newinterpid(cls, id, force);
}

static void
interpid_dealloc(PyObject *v)
{
    int64_t id = ((interpid *)v)->id;
    PyInterpreterState *interp = _PyInterpreterState_LookUpID(id);
    if (interp != NULL) {
        _PyInterpreterState_IDDecref(interp);
    }
    else {
        // already deleted
        PyErr_Clear();
    }
    Py_TYPE(v)->tp_free(v);
}

static PyObject *
interpid_repr(PyObject *self)
{
    PyTypeObject *type = Py_TYPE(self);
    const char *name = _PyType_Name(type);
    interpid *id = (interpid *)self;
    return PyUnicode_FromFormat("%s(%" PRId64 ")", name, id->id);
}

static PyObject *
interpid_str(PyObject *self)
{
    interpid *id = (interpid *)self;
    return PyUnicode_FromFormat("%" PRId64 "", id->id);
}

static PyObject *
interpid_int(PyObject *self)
{
    interpid *id = (interpid *)self;
    return PyLong_FromLongLong(id->id);
}

static PyNumberMethods interpid_as_number = {
     0,                       /* nb_add */
     0,                       /* nb_subtract */
     0,                       /* nb_multiply */
     0,                       /* nb_remainder */
     0,                       /* nb_divmod */
     0,                       /* nb_power */
     0,                       /* nb_negative */
     0,                       /* nb_positive */
     0,                       /* nb_absolute */
     0,                       /* nb_bool */
     0,                       /* nb_invert */
     0,                       /* nb_lshift */
     0,                       /* nb_rshift */
     0,                       /* nb_and */
     0,                       /* nb_xor */
     0,                       /* nb_or */
     (unaryfunc)interpid_int, /* nb_int */
     0,                       /* nb_reserved */
     0,                       /* nb_float */

     0,                       /* nb_inplace_add */
     0,                       /* nb_inplace_subtract */
     0,                       /* nb_inplace_multiply */
     0,                       /* nb_inplace_remainder */
     0,                       /* nb_inplace_power */
     0,                       /* nb_inplace_lshift */
     0,                       /* nb_inplace_rshift */
     0,                       /* nb_inplace_and */
     0,                       /* nb_inplace_xor */
     0,                       /* nb_inplace_or */

     0,                       /* nb_floor_divide */
     0,                       /* nb_true_divide */
     0,                       /* nb_inplace_floor_divide */
     0,                       /* nb_inplace_true_divide */

     (unaryfunc)interpid_int, /* nb_index */
};

static Py_hash_t
interpid_hash(PyObject *self)
{
    interpid *id = (interpid *)self;
    PyObject *obj = PyLong_FromLongLong(id->id);
    if (obj == NULL) {
        return -1;
    }
    Py_hash_t hash = PyObject_Hash(obj);
    Py_DECREF(obj);
    return hash;
}

static PyObject *
interpid_richcompare(PyObject *self, PyObject *other, int op)
{
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (!PyObject_TypeCheck(self, &_PyInterpreterID_Type)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    interpid *id = (interpid *)self;
    int equal;
    if (PyObject_TypeCheck(other, &_PyInterpreterID_Type)) {
        interpid *otherid = (interpid *)other;
        equal = (id->id == otherid->id);
    }
    else {
        other = PyNumber_Long(other);
        if (other == NULL) {
            PyErr_Clear();
            Py_RETURN_NOTIMPLEMENTED;
        }
        int64_t otherid = PyLong_AsLongLong(other);
        Py_DECREF(other);
        if (otherid == -1 && PyErr_Occurred() != NULL) {
            return NULL;
        }
        if (otherid < 0) {
            equal = 0;
        }
        else {
            equal = (id->id == otherid);
        }
    }

    if ((op == Py_EQ && equal) || (op == Py_NE && !equal)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(interpid_doc,
"A interpreter ID identifies a interpreter and may be used as an int.");

PyTypeObject _PyInterpreterID_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "InterpreterID",   /* tp_name */
    sizeof(interpid),               /* tp_basicsize */
    0,                              /* tp_itemsize */
    (destructor)interpid_dealloc,   /* tp_dealloc */
    0,                              /* tp_vectorcall_offset */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_as_async */
    (reprfunc)interpid_repr,        /* tp_repr */
    &interpid_as_number,            /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    interpid_hash,                  /* tp_hash */
    0,                              /* tp_call */
    (reprfunc)interpid_str,         /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_LONG_SUBCLASS,   /* tp_flags */
    interpid_doc,                   /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    interpid_richcompare,           /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    0,                              /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    &PyLong_Type,                   /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    interpid_new,                   /* tp_new */
};

PyObject *_PyInterpreterID_New(int64_t id)
{
    return (PyObject *)newinterpid(&_PyInterpreterID_Type, id, 0);
}

PyObject *
_PyInterpreterState_GetIDObject(PyInterpreterState *interp)
{
    if (_PyInterpreterState_IDInitref(interp) != 0) {
        return NULL;
    };
    PY_INT64_T id = PyInterpreterState_GetID(interp);
    if (id < 0) {
        return NULL;
    }
    return (PyObject *)newinterpid(&_PyInterpreterID_Type, id, 0);
}

PyInterpreterState *
_PyInterpreterID_LookUp(PyObject *requested_id)
{
    int64_t id;
    if (PyObject_TypeCheck(requested_id, &_PyInterpreterID_Type)) {
        id = ((interpid *)requested_id)->id;
    }
    else {
        id = PyLong_AsLongLong(requested_id);
        if (id == -1 && PyErr_Occurred() != NULL) {
            return NULL;
        }
        assert(id <= INT64_MAX);
    }
    return _PyInterpreterState_LookUpID(id);
}
