#include <Python.h>

staticforward PyTypeObject noddy_NoddyType;

typedef struct {
    PyObject_HEAD
} noddy_NoddyObject;

static PyObject*
noddy_new_noddy(PyObject* self, PyObject* args)
{
    noddy_NoddyObject* noddy;

    if (!PyArg_ParseTuple(args,":new_noddy")) 
        return NULL;

    noddy = PyObject_New(noddy_NoddyObject, &noddy_NoddyType);

    return (PyObject*)noddy;
}

static void
noddy_noddy_dealloc(PyObject* self)
{
    PyObject_Del(self);
}

static PyTypeObject noddy_NoddyType = {
    PyObject_HEAD_INIT(NULL)
    0,
    "Noddy",
    sizeof(noddy_NoddyObject),
    0,
    noddy_noddy_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */
};

static PyMethodDef noddy_methods[] = {
    {"new_noddy", noddy_new_noddy, METH_VARARGS,
     "Create a new Noddy object."},
    {NULL, NULL, 0, NULL}
};

DL_EXPORT(void)
initnoddy(void) 
{
    noddy_NoddyType.ob_type = &PyType_Type;

    Py_InitModule("noddy", noddy_methods);
}
