
/* interpreters module */
/* low-level access to interpreter primitives */

#include "Python.h"


static PyObject *
_get_id(PyInterpreterState *interp)
{
    unsigned long id = PyInterpreterState_GetID(interp);
    if (id == 0 && PyErr_Occurred() != NULL)
        return NULL;
    return PyLong_FromUnsignedLong(id);
}

static PyInterpreterState *
_look_up(PyObject *requested_id)
{
    PyObject * id;
    PyInterpreterState *interp;

    interp = PyInterpreterState_Head();
    while (interp != NULL) {
        id = _get_id(interp);
        if (id == NULL)
            return NULL;
        if (requested_id == id)
            return interp;
        interp = PyInterpreterState_Next(interp);
    }

    PyErr_Format(PyExc_RuntimeError,
                 "unrecognized interpreter ID %R", requested_id);
    return NULL;
}

static PyObject *
_get_current(void)
{
    PyThreadState *tstate;
    PyInterpreterState *interp;

    tstate = PyThreadState_Get();
    if (tstate == NULL)
        return NULL;
    interp = tstate->interp;

    // get ID
    return _get_id(interp);
}


/* module level code ********************************************************/

// XXX track count?

static PyObject *
interp_create(PyObject *self, PyObject *args)
{
    if (!PyArg_UnpackTuple(args, "create", 0, 0))
        return NULL;

    // Create and initialize the new interpreter.
    PyThreadState *tstate, *save_tstate;
    save_tstate = PyThreadState_Swap(NULL);
    tstate = Py_NewInterpreter();
    PyThreadState_Swap(save_tstate);
    if (tstate == NULL) {
        /* Since no new thread state was created, there is no exception to
           propagate; raise a fresh one after swapping in the old thread
           state. */
        PyErr_SetString(PyExc_RuntimeError, "interpreter creation failed");
        return NULL;
    }
    return _get_id(tstate->interp);
}

PyDoc_STRVAR(create_doc,
"create() -> ID\n\
\n\
Create a new interpreter and return a unique generated ID.");


static PyObject *
interp_destroy(PyObject *self, PyObject *args)
{
    PyObject *id;
    if (!PyArg_UnpackTuple(args, "destroy", 1, 1, &id))
        return NULL;
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "ID must be an int");
        return NULL;
    }

    // Ensure the ID is not the current interpreter.
    PyObject *current = _get_current();
    if (current == NULL)
        return NULL;
    if (PyObject_RichCompareBool(id, current, Py_EQ) != 0) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot destroy the current interpreter");
        return NULL;
    }

    // Look up the interpreter.
    PyInterpreterState *interp = _look_up(id);
    if (interp == NULL)
        return NULL;

    // Destroy the interpreter.
    //PyInterpreterState_Delete(interp);
    PyThreadState *tstate, *save_tstate;
    tstate = PyInterpreterState_ThreadHead(interp);  // XXX Is this the right one?
    save_tstate = PyThreadState_Swap(tstate);
    // XXX Stop current execution?
    Py_EndInterpreter(tstate);  // XXX Handle possible errors?
    PyThreadState_Swap(save_tstate);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(destroy_doc,
"destroy(ID)\n\
\n\
Destroy the identified interpreter.\n\
\n\
Attempting to destroy the current interpreter results in a RuntimeError.\n\
So does an unrecognized ID.");


static PyObject *
interp_enumerate(PyObject *self)
{
    PyObject *ids, *id;
    PyInterpreterState *interp;

    // XXX Handle multiple main interpreters.

    ids = PyList_New(0);
    if (ids == NULL)
        return NULL;

    interp = PyInterpreterState_Head();
    while (interp != NULL) {
        id = _get_id(interp);
        if (id == NULL)
            return NULL;
        // insert at front of list
        if (PyList_Insert(ids, 0, id) < 0)
            return NULL;

        interp = PyInterpreterState_Next(interp);
    }

    return ids;
}

PyDoc_STRVAR(enumerate_doc,
"enumerate() -> [ID]\n\
\n\
Return a list containing the ID of every existing interpreter.");


static PyMethodDef module_functions[] = {
    {"create",      (PyCFunction)interp_create,
     METH_VARARGS, create_doc},
    {"destroy",     (PyCFunction)interp_destroy,
     METH_VARARGS, destroy_doc},

    {"_enumerate",   (PyCFunction)interp_enumerate,
     METH_NOARGS, enumerate_doc},

    {NULL,          NULL}           /* sentinel */
};


/* initialization function */

PyDoc_STRVAR(module_doc,
"This module provides primitive operations to manage Python interpreters.\n\
The 'interpreters' module provides a more convenient interface.");

static struct PyModuleDef interpretersmodule = {
    PyModuleDef_HEAD_INIT,
    "_interpreters",
    module_doc,
    -1,
    module_functions,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit__interpreters(void)
{
    PyObject *module;

    module = PyModule_Create(&interpretersmodule);
    if (module == NULL)
        return NULL;


    return module;
}
