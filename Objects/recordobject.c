
/* Record object implementation */

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_gc.h"            // _PyObject_GC_IS_TRACKED()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "recordobject.h"

PyAPI_FUNC(PyObject *) PyRecord_New(Py_ssize_t size)
{
    return NULL;
}

static void
record_dealloc(PyRecordObject *op)
{
    // TODO
}

static PyObject *
record_repr(PyRecordObject *v)
{
    // TODO
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