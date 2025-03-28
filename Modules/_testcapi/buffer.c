/* Test PEP 688 - Buffers */

#include "parts.h"


#include <stddef.h>                 // offsetof

typedef struct {
    PyObject_HEAD
    PyObject *obj;
    Py_ssize_t references;
} testBufObject;

#define testBufObject_CAST(op)  ((testBufObject *)(op))

static PyObject *
testbuf_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *obj = PyBytes_FromString("test");
    if (obj == NULL) {
        return NULL;
    }
    testBufObject *self = (testBufObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        Py_DECREF(obj);
        return NULL;
    }
    self->obj = obj;
    self->references = 0;
    return (PyObject *)self;
}

static int
testbuf_traverse(PyObject *op, visitproc visit, void *arg)
{
    testBufObject *self = testBufObject_CAST(op);
    Py_VISIT(self->obj);
    return 0;
}

static int
testbuf_clear(PyObject *op)
{
    testBufObject *self = testBufObject_CAST(op);
    Py_CLEAR(self->obj);
    return 0;
}

static void
testbuf_dealloc(PyObject *op)
{
    testBufObject *self = testBufObject_CAST(op);
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->obj);
    Py_TYPE(self)->tp_free(self);
}

static int
testbuf_getbuf(PyObject *op, Py_buffer *view, int flags)
{
    testBufObject *self = testBufObject_CAST(op);
    int buf = PyObject_GetBuffer(self->obj, view, flags);
    if (buf == 0) {
        Py_SETREF(view->obj, Py_NewRef(self));
        self->references++;
    }
    return buf;
}

static void
testbuf_releasebuf(PyObject *op, Py_buffer *Py_UNUSED(view))
{
    testBufObject *self = testBufObject_CAST(op);
    self->references--;
    assert(self->references >= 0);
}

static PyBufferProcs testbuf_as_buffer = {
    .bf_getbuffer = testbuf_getbuf,
    .bf_releasebuffer = testbuf_releasebuf,
};

static struct PyMemberDef testbuf_members[] = {
    {"references", Py_T_PYSSIZET, offsetof(testBufObject, references), Py_READONLY},
    {NULL},
};

static PyTypeObject testBufType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "testBufType",
    .tp_basicsize = sizeof(testBufObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_new = testbuf_new,
    .tp_dealloc = testbuf_dealloc,
    .tp_traverse = testbuf_traverse,
    .tp_clear = testbuf_clear,
    .tp_as_buffer = &testbuf_as_buffer,
    .tp_members = testbuf_members
};

int
_PyTestCapi_Init_Buffer(PyObject *m) {
    if (PyType_Ready(&testBufType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "testBuf", (PyObject *)&testBufType)) {
        return -1;
    }

    return 0;
}
