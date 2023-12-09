/* InterpreterID object */

#include "Python.h"
#include "pycore_abstract.h"   // _PyIndex_Check()
#include "pycore_interp.h"     // _PyInterpreterState_LookUpID()
#include "interpreteridobject.h"


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

    if (interp != NULL) {
        if (_PyInterpreterState_IDIncref(interp) < 0) {
            return NULL;
        }
    }

    interpid *self = PyObject_New(interpid, cls);
    if (self == NULL) {
        if (interp != NULL) {
            _PyInterpreterState_IDDecref(interp);
        }
        return NULL;
    }
    self->id = id;

    return self;
}

static int
interp_id_converter(PyObject *arg, void *ptr)
{
    int64_t id;
    if (PyObject_TypeCheck(arg, &_PyInterpreterID_Type)) {
        id = ((interpid *)arg)->id;
    }
    else if (_PyIndex_Check(arg)) {
        id = PyLong_AsLongLong(arg);
        if (id == -1 && PyErr_Occurred()) {
            return 0;
        }
        if (id < 0) {
            PyErr_Format(PyExc_ValueError,
                         "interpreter ID must be a non-negative int, got %R", arg);
            return 0;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "interpreter ID must be an int, got %.100s",
                     Py_TYPE(arg)->tp_name);
        return 0;
    }
    *(int64_t *)ptr = id;
    return 1;
}

static PyObject *
interpid_new(PyTypeObject *cls, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "force", NULL};
    int64_t id;
    int force = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&|$p:InterpreterID.__init__", kwlist,
                                     interp_id_converter, &id, &force)) {
        return NULL;
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
    else if (PyLong_CheckExact(other)) {
        /* Fast path */
        int overflow;
        long long otherid = PyLong_AsLongLongAndOverflow(other, &overflow);
        if (otherid == -1 && PyErr_Occurred()) {
            return NULL;
        }
        equal = !overflow && (otherid >= 0) && (id->id == otherid);
    }
    else if (PyNumber_Check(other)) {
        PyObject *pyid = PyLong_FromLongLong(id->id);
        if (pyid == NULL) {
            return NULL;
        }
        PyObject *res = PyObject_RichCompare(pyid, other, op);
        Py_DECREF(pyid);
        return res;
    }
    else {
        Py_RETURN_NOTIMPLEMENTED;
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
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
    0,                              /* tp_base */
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
    int64_t id = PyInterpreterState_GetID(interp);
    if (id < 0) {
        return NULL;
    }
    return (PyObject *)newinterpid(&_PyInterpreterID_Type, id, 0);
}

PyInterpreterState *
_PyInterpreterID_LookUp(PyObject *requested_id)
{
    int64_t id;
    if (!interp_id_converter(requested_id, &id)) {
        return NULL;
    }
    return _PyInterpreterState_LookUpID(id);
}
