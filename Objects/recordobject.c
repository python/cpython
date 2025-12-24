
/* Record object implementation */

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_gc.h"            // _PyObject_GC_IS_TRACKED()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "recordobject.h"

static inline void
record_gc_track(PyRecordObject *op)
{
    _PyObject_GC_TRACK(op);
}

static PyRecordObject *
record_alloc(Py_ssize_t size)
{
    PyRecordObject *op;
    if (size < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }

    {
        /* Check for overflow */
        if ((size_t)size > ((size_t)PY_SSIZE_T_MAX - (sizeof(PyRecordObject) -
                    sizeof(PyObject *))) / sizeof(PyObject *)) {
            return (PyRecordObject *)PyErr_NoMemory();
        }
        op = PyObject_GC_NewVar(PyRecordObject, &PyRecord_Type, size);
        if (op == NULL)
            return NULL;
    }
    return op;
}

PyAPI_FUNC(PyObject *) PyRecord_New(Py_ssize_t size)
{
    PyRecordObject *op;
    op = record_alloc(size);
    if (op == NULL) {
        return NULL;
    }
    op->names = NULL;
    for (Py_ssize_t i = 0; i < size; i++) {
        op->ob_item[i] = NULL;
    }
    record_gc_track(op);
    return (PyObject *) op;
}

static void
record_dealloc(PyRecordObject *op)
{
    Py_ssize_t len =  Py_SIZE(op);
    PyObject_GC_UnTrack(op);
    Py_TRASHCAN_BEGIN(op, record_dealloc)
    Py_XDECREF(op->names);
    if (len > 0) {
        Py_ssize_t i = len;
        while (--i >= 0) {
            Py_XDECREF(op->ob_item[i]);
        }
    }
    Py_TYPE(op)->tp_free((PyObject *)op);
    Py_TRASHCAN_END
}

static PyObject *
record_repr(PyRecordObject *v)
{
    Py_ssize_t i, n;
    _PyUnicodeWriter writer;

    n = Py_SIZE(v);
    if (n == 0)
        return PyUnicode_FromString("record()");

    /* While not mutable, it is still possible to end up with a cycle in a
       record through an object that stores itself within a record (and thus
       infinitely asks for the repr of itself). This should only be
       possible within a type. */
    i = Py_ReprEnter((PyObject *)v);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("record(...)") : NULL;
    }

    _PyUnicodeWriter_Init(&writer);
    writer.overallocate = 1;
    if (Py_SIZE(v) > 1) {
        /* "(" + "1" + ", 2" * (len - 1) + ")" */
        writer.min_length = 1 + 1 + (2 + 1) * (Py_SIZE(v) - 1) + 1;
    }
    else {
        /* "(1,)" */
        writer.min_length = 4;
    }

    if (_PyUnicodeWriter_WriteASCIIString(&writer, "record(", 7) < 0)
        goto error;

    /* Do repr() on each element. */
    for (i = 0; i < n; ++i) {
        PyObject *s;
        PyObject *name;

        if (i > 0) {
            if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0)
                goto error;
        }

        name = PyTuple_GET_ITEM(v->names, i);
        s = PyObject_Repr(v->ob_item[i]);
        if (s == NULL)
            goto error;

        if (_PyUnicodeWriter_WriteStr(&writer, name) < 0) {
            Py_DECREF(s);
            goto error;
        }
        if (_PyUnicodeWriter_WriteChar(&writer, '=') < 0) {
            Py_DECREF(s);
            goto error;
        }
        if (_PyUnicodeWriter_WriteStr(&writer, s) < 0) {
            Py_DECREF(s);
            goto error;
        }
        Py_DECREF(s);
    }

    writer.overallocate = 0;
    if (_PyUnicodeWriter_WriteChar(&writer, ')') < 0)
        goto error;

    Py_ReprLeave((PyObject *)v);
    return _PyUnicodeWriter_Finish(&writer);

error:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_ReprLeave((PyObject *)v);
    return NULL;
}

static Py_ssize_t
record_length(PyRecordObject *a)
{
    return Py_SIZE(a);
}

static PyObject *
recorditem(PyRecordObject *a, Py_ssize_t i)
{
    if (i < 0 || i >= Py_SIZE(a)) {
        PyErr_SetString(PyExc_IndexError, "record index out of range");
        return NULL;
    }
    Py_INCREF(a->ob_item[i]);
    return a->ob_item[i];
}


static PySequenceMethods record_as_sequence = {
    (lenfunc)record_length,                      /* sq_length */
    0,                                           /* sq_concat */
    0,                                           /* sq_repeat */
    (ssizeargfunc)recorditem,                    /* sq_item */
    0,                                           /* sq_slice */
    0,                                           /* sq_ass_item */
    0,                                           /* sq_ass_slice */
    0,                                           /* sq_contains */
};

static Py_hash_t
record_hash(PyRecordObject *v)
{
    return -1;
}

PyObject *
record_getattro(PyObject *obj, PyObject *name)
{
    // TODO
    return NULL;
}

static PyObject *
record_rich_compare(PyObject *v, PyObject *w, int op)
{
    return NULL;
}

static PyObject *
record_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return NULL;
}

PyTypeObject PyRecord_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "record",
    sizeof(PyRecordObject) - sizeof(PyObject *),
    sizeof(PyObject *),
    (destructor)record_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)record_repr,                       /* tp_repr */
    0,                                          /* tp_as_number */
    &record_as_sequence,                        /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    (hashfunc)record_hash,                       /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    record_getattro,                            /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    record_rich_compare,                        /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    record_new,                                 /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
    0,
};