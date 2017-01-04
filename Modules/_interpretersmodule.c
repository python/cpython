
/* interpreters module */
/* low-level access to interpreter primitives */

#include "Python.h"
#include "frameobject.h"


static PyInterpreterState *
_get_current(void)
{
    PyThreadState *tstate;

    tstate = PyThreadState_Get();
    if (tstate == NULL)
        return NULL;
    return tstate->interp;
}

static PyInterpreterState *
_look_up_int64(PY_INT64_T requested_id)
{
    if (requested_id < 0)
        goto error;

    PyInterpreterState *interp = PyInterpreterState_Head();
    while (interp != NULL) {
        PY_INT64_T id = PyInterpreterState_GetID(interp);
        if (id < 0)
            return NULL;
        if (requested_id == id)
            return interp;
        interp = PyInterpreterState_Next(interp);
    }

error:
    PyErr_Format(PyExc_RuntimeError,
                 "unrecognized interpreter ID %lld", requested_id);
    return NULL;
}

static PyInterpreterState *
_look_up(PyObject *requested_id)
{
    long long id = PyLong_AsLongLong(requested_id);
    if (id == -1 && PyErr_Occurred() != NULL)
        return NULL;
    assert(id <= INT64_MAX);
    return _look_up_int64(id);
}

static PyObject *
_get_id(PyInterpreterState *interp)
{
    PY_INT64_T id = PyInterpreterState_GetID(interp);
    if (id < 0)
        return NULL;
    return PyLong_FromLongLong(id);
}

static int
_is_running(PyInterpreterState *interp)
{
    PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
    if (PyThreadState_Next(tstate) != NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "interpreter has more than one thread");
        return -1;
    }
    PyFrameObject *frame = _PyThreadState_GetFrame(tstate);
    if (frame == NULL) {
        if (PyErr_Occurred() != NULL)
            return -1;
        return 0;
    }
    return (int)(frame->f_executing);
}

static int
_ensure_not_running(PyInterpreterState *interp)
{
    int is_running = _is_running(interp);
    if (is_running < 0)
        return -1;
    if (is_running) {
        PyErr_Format(PyExc_RuntimeError, "interpreter already running");
        return -1;
    }
    return 0;
}

static PyObject *
_copy_module(PyObject *m, PyObject *updates)
{
    PyObject *orig = PyModule_GetDict(m);  // borrowed
    if (orig == NULL) {
        return NULL;
    }

    PyObject *copy = PyModule_New("__main__");
    if (copy == NULL) {
        return NULL;
    }
    PyObject *ns = PyModule_GetDict(copy);  // borrowed
    if (ns == NULL)
        goto error;

    if (PyDict_Merge(ns, orig, 1) < 0)
        goto error;
    if (updates != NULL && PyDict_Merge(ns, updates, 0) < 0)
        goto error;
    return copy;

error:
    Py_DECREF(copy);
    return NULL;
}

static int
_run_string(PyInterpreterState *interp, const char *codestr, PyObject *ns)
{
    // Switch to interpreter.
    PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
    PyThreadState *save_tstate = PyThreadState_Swap(tstate);

    // Run the string (see PyRun_SimpleStringFlags).
    PyObject *exc = NULL, *value = NULL, *tb = NULL;
    PyObject *result = PyRun_StringFlags(codestr, Py_file_input, ns, ns, NULL);
    if (result == NULL) {
        // Get the exception from the subinterpreter.
        PyErr_Fetch(&exc, &value, &tb);
    } else {
        Py_DECREF(result);  // We throw away the result.
    }

    // Switch back.
    if (save_tstate != NULL)
        PyThreadState_Swap(save_tstate);

    // Propagate any exception out to the caller.
    PyErr_Restore(exc, value, tb);

    return result == NULL ? -1 : 0;
}

static PyObject *
_run_string_in_main(PyInterpreterState *interp, const char *codestr,
                    PyObject *updates)
{
    if (_ensure_not_running(interp) < 0)
        return NULL;

    // Get the namespace in which to execute.  This involves creating
    // a new module, updating it from the __main__ module and the given
    // updates (if any), replacing the __main__ with the new module in
    // sys.modules, and then using the new module's __dict__.  At the
    // end we restore the original __main__ module.
    PyObject *main_mod = PyMapping_GetItemString(interp->modules, "__main__");
    if (main_mod == NULL)
        return NULL;
    PyObject *m = _copy_module(main_mod, updates);
    if (m == NULL) {
        Py_DECREF(main_mod);
        return NULL;
    }
    if (PyMapping_SetItemString(interp->modules, "__main__", m) < 0) {
        Py_DECREF(main_mod);
        Py_DECREF(m);
        return NULL;
    }
    PyObject *ns = PyModule_GetDict(m);  // borrowed
    Py_INCREF(ns);
    Py_DECREF(m);

    // Run the string.
    PyObject *result = ns;
    if (_run_string(interp, codestr, ns) < 0) {
        result = NULL;
        Py_DECREF(ns);
    }

    // Restore __main__.
    if (PyMapping_SetItemString(interp->modules, "__main__", main_mod) < 0) {
        // XXX Chain exceptions...
        //PyErr_Restore(exc, value, tb);
    }
    Py_DECREF(main_mod);

    return result;
}


/* module level code ********************************************************/

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

    // Look up the interpreter.
    PyInterpreterState *interp = _look_up(id);
    if (interp == NULL)
        return NULL;

    // Ensure we don't try to destroy the current interpreter.
    PyInterpreterState *current = _get_current();
    if (current == NULL)
        return NULL;
    if (interp == current) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot destroy the current interpreter");
        return NULL;
    }

    // Ensure the interpreter isn't running.
    /* XXX We *could* support destroying a running interpreter but
       aren't going to worry about it for now. */
    if (_ensure_not_running(interp) < 0)
        return NULL;

    // Destroy the interpreter.
    //PyInterpreterState_Delete(interp);
    PyThreadState *tstate, *save_tstate;
    tstate = PyInterpreterState_ThreadHead(interp);
    save_tstate = PyThreadState_Swap(tstate);
    Py_EndInterpreter(tstate);
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

static PyObject *
interp_run_string(PyObject *self, PyObject *args)
{
    PyObject *id, *code;
    if (!PyArg_UnpackTuple(args, "run_string", 2, 2, &id, &code))
        return NULL;
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "first arg (ID) must be an int");
        return NULL;
    }
    if (!PyUnicode_Check(code)) {
        PyErr_SetString(PyExc_TypeError,
                        "second arg (code) must be a string");
        return NULL;
    }

    // Look up the interpreter.
    PyInterpreterState *interp = _look_up(id);
    if (interp == NULL)
        return NULL;

    // Extract code.
    Py_ssize_t size;
    const char *codestr = PyUnicode_AsUTF8AndSize(code, &size);
    if (codestr == NULL)
        return NULL;
    if (strlen(codestr) != (size_t)size) {
        PyErr_SetString(PyExc_ValueError,
                        "source code string cannot contain null bytes");
        return NULL;
    }

    // Run the code in the interpreter.
    PyObject *ns = _run_string_in_main(interp, codestr, NULL);
    if (ns == NULL)
        return NULL;
    else
        Py_RETURN_NONE;
}

PyDoc_STRVAR(run_string_doc,
"run_string(ID, sourcetext)\n\
\n\
Execute the provided string in the identified interpreter.\n\
\n\
See PyRun_SimpleStrings.");


static PyObject *
interp_run_string_unrestricted(PyObject *self, PyObject *args)
{
    PyObject *id, *code, *ns = NULL;
    if (!PyArg_UnpackTuple(args, "run_string_unrestricted", 2, 3,
                           &id, &code, &ns))
        return NULL;
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "first arg (ID) must be an int");
        return NULL;
    }
    if (!PyUnicode_Check(code)) {
        PyErr_SetString(PyExc_TypeError,
                        "second arg (code) must be a string");
        return NULL;
    }
    if (ns == Py_None)
        ns = NULL;

    // Look up the interpreter.
    PyInterpreterState *interp = _look_up(id);
    if (interp == NULL)
        return NULL;

    // Extract code.
    Py_ssize_t size;
    const char *codestr = PyUnicode_AsUTF8AndSize(code, &size);
    if (codestr == NULL)
        return NULL;
    if (strlen(codestr) != (size_t)size) {
        PyErr_SetString(PyExc_ValueError,
                        "source code string cannot contain null bytes");
        return NULL;
    }

    // Run the code in the interpreter.
    return _run_string_in_main(interp, codestr, ns);
}

PyDoc_STRVAR(run_string_unrestricted_doc,
"run_string_unrestricted(ID, sourcetext, ns=None) -> main module ns\n\
\n\
Execute the provided string in the identified interpreter.  Return the\n\
dict in which the code executed.  If the ns arg is provided then it is\n\
merged into the execution namespace before the code is executed.\n\
\n\
See PyRun_SimpleStrings.");


static PyMethodDef module_functions[] = {
    {"create",                  (PyCFunction)interp_create,
     METH_VARARGS, create_doc},
    {"destroy",                 (PyCFunction)interp_destroy,
     METH_VARARGS, destroy_doc},

    {"_enumerate",              (PyCFunction)interp_enumerate,
     METH_NOARGS, NULL},

    {"run_string",              (PyCFunction)interp_run_string,
     METH_VARARGS, run_string_doc},
    {"run_string_unrestricted", (PyCFunction)interp_run_string_unrestricted,
     METH_VARARGS, run_string_unrestricted_doc},

    {NULL,                      NULL}           /* sentinel */
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
