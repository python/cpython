#include <Python.h>

static PyTypeObject noddy_NoddyType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
} noddy_NoddyObject;

static PyObject*
noddy_new_noddy(PyObject* self, PyObject* args)
{
    noddy_NoddyObject* noddy;

    noddy = PyObject_New(noddy_NoddyObject, &noddy_NoddyType);

    /* Initialize type-specific fields here. */

    return (PyObject*)noddy;
}

static void
noddy_noddy_dealloc(PyObject* self)
{
    /* Free any external resources here;
     * if the instance owns references to any Python
     * objects, call Py_DECREF() on them here.
     */
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
    {"new_noddy", noddy_new_noddy, METH_NOARGS,
     "Create a new Noddy object."},

    {NULL}  /* Sentinel */
};

PyMODINIT_FUNC
initnoddy(void) 
{
    noddy_NoddyType.ob_type = &PyType_Type;
    if (PyType_Ready(&noddy_NoddyType))
        return;

    Py_InitModule3("noddy", noddy_methods
                   "Example module that creates an extension type.");
}
