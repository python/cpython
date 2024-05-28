/* Test PEP 688 - Buffers */

#include "parts.h"


#include <stddef.h>                 // offsetof

typedef struct {
    PyObject_HEAD
    PyObject *obj;
    Py_ssize_t references;
} testBufObject;

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
testbuf_traverse(testBufObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->obj);
    return 0;
}

static int
testbuf_clear(testBufObject *self)
{
    Py_CLEAR(self->obj);
    return 0;
}

static void
testbuf_dealloc(testBufObject *self)
{
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->obj);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
testbuf_getbuf(testBufObject *self, Py_buffer *view, int flags)
{
    int buf = PyObject_GetBuffer(self->obj, view, flags);
    if (buf == 0) {
        Py_SETREF(view->obj, Py_NewRef(self));
        self->references++;
    }
    return buf;
}

static void
testbuf_releasebuf(testBufObject *self, Py_buffer *view)
{
    self->references--;
    assert(self->references >= 0);
}

static PyBufferProcs testbuf_as_buffer = {
    .bf_getbuffer = (getbufferproc) testbuf_getbuf,
    .bf_releasebuffer = (releasebufferproc) testbuf_releasebuf,
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
    .tp_dealloc = (destructor) testbuf_dealloc,
    .tp_traverse = (traverseproc) testbuf_traverse,
    .tp_clear = (inquiry) testbuf_clear,
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
