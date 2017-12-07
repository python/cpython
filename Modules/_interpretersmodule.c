
/* interpreters module */
/* low-level access to interpreter primitives */

#include "Python.h"
#include "frameobject.h"
#include "internal/pystate.h"


static PyInterpreterState *
_get_current(void)
{
    PyThreadState *tstate;

    tstate = PyThreadState_Get();
    if (tstate == NULL)
        return NULL;
    return tstate->interp;
}

/* sharing-specific functions */

static int
_PyObject_CheckShareable(PyObject *obj)
{
    if (PyBytes_CheckExact(obj))
        return 0;
    PyErr_SetString(PyExc_ValueError,
                    "obj is not a cross-interpreter shareable type");
    return 1;
}

static PyObject *
_new_bytes_object(_PyCrossInterpreterData *data)
{
    return PyBytes_FromString((char *)(data->data));
}

static int
_bytes_shared(PyObject *obj, _PyCrossInterpreterData *data)
{
    data->data = (void *)(PyBytes_AS_STRING(obj));
    data->new_object = _new_bytes_object;
    data->free = NULL;
    return 0;
}

static int
_PyObject_GetCrossInterpreterData(PyObject *obj, _PyCrossInterpreterData *data)
{
    Py_INCREF(obj);

    if (_PyObject_CheckShareable(obj) != 0) {
        Py_DECREF(obj);
        return 1;
    }

    data->interp = _get_current();
    data->object = obj;

    if (PyBytes_CheckExact(obj)) {
        if (_bytes_shared(obj, data) != 0) {
            Py_DECREF(obj);
            return 1;
        }
    }

    return 0;
};

static void
_PyCrossInterpreterData_Release(_PyCrossInterpreterData *data)
{
    PyThreadState *save_tstate = NULL;
    if (data->interp != NULL) {
        // Switch to the original interpreter.
        PyThreadState *tstate = PyInterpreterState_ThreadHead(data->interp);
        save_tstate = PyThreadState_Swap(tstate);
    }

    if (data->free != NULL) {
        data->free(data->data);
    }
    Py_XDECREF(data->object);

    // Switch back.
    if (save_tstate != NULL)
        PyThreadState_Swap(save_tstate);
}

static PyObject *
_PyCrossInterpreterData_NewObject(_PyCrossInterpreterData *data)
{
    return data->new_object(data);
}

/* interpreter-specific functions */

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
_copy_module(PyObject *m, const char *name)
{
    PyObject *orig = PyModule_GetDict(m);  // borrowed
    if (orig == NULL) {
        return NULL;
    }

    PyObject *copy = PyModule_New(name);
    if (copy == NULL) {
        return NULL;
    }
    PyObject *ns = PyModule_GetDict(copy);  // borrowed
    if (ns == NULL)
        goto error;

    if (PyDict_Merge(ns, orig, 1) < 0)
        goto error;
    return copy;

error:
    Py_DECREF(copy);
    return NULL;
}

static PyObject *
_copy_module_ns(PyInterpreterState *interp,
                const char *name, const char *tempname)
{
    // Get the namespace in which to execute.  This involves creating
    // a new module, updating it from the __main__ module and the given
    // updates (if any), replacing the __main__ with the new module in
    // sys.modules, and then using the new module's __dict__.  At the
    // end we restore the original __main__ module.
    PyObject *main_mod = PyMapping_GetItemString(interp->modules, name);
    if (main_mod == NULL)
        return NULL;
    PyObject *m = _copy_module(main_mod, name);
    if (m == NULL) {
        Py_DECREF(main_mod);
        return NULL;
    }
    if (tempname != NULL) {
        if (PyMapping_SetItemString(interp->modules, tempname, main_mod) < 0) {
            Py_DECREF(main_mod);
            Py_DECREF(m);
            return NULL;
        }
    }
    Py_DECREF(main_mod);
    if (PyMapping_SetItemString(interp->modules, name, m) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    PyObject *ns = PyModule_GetDict(m);  // borrowed
    Py_INCREF(ns);
    Py_DECREF(m);
    return ns;
}

static int
_restore_module(PyInterpreterState *interp,
                const char *name, const char *tempname)
{
    PyObject *main_mod = PyMapping_GetItemString(interp->modules, tempname);
    if (main_mod == NULL)
        return -1;
    if (PyMapping_SetItemString(interp->modules, name, main_mod) < 0) {
        Py_DECREF(main_mod);
        return -1;
    }
    Py_DECREF(main_mod);
    return 0;
}

struct _shareditem {
    Py_UNICODE *name;
    Py_ssize_t namelen;
    _PyCrossInterpreterData data;
};

void
_sharedns_clear(struct _shareditem *shared)
{
    for (struct _shareditem *item=shared; item->name != NULL; item += 1) {
        _PyCrossInterpreterData_Release(&item->data);
    }
}

static struct _shareditem *
_get_shared_ns(PyObject *shareable)
{
    if (shareable == NULL || shareable == Py_None)
        return NULL;
    Py_ssize_t len = PyDict_Size(shareable);
    if (len == 0)
        return NULL;

    struct _shareditem *shared = PyMem_NEW(struct _shareditem, len+1);
    Py_ssize_t pos = 0;
    for (Py_ssize_t i=0; i < len; i++) {
        PyObject *key, *value;
        if (PyDict_Next(shareable, &pos, &key, &value) == 0) {
            break;
        }
        struct _shareditem *item = shared + i;

        if (_PyObject_GetCrossInterpreterData(value, &item->data) != 0)
            break;
        item->name = PyUnicode_AsUnicodeAndSize(key, &item->namelen);
        if (item->name == NULL) {
            _PyCrossInterpreterData_Release(&item->data);
            break;
        }
        (item + 1)->name = NULL;  // Mark the next one as the last.
    }
    if (PyErr_Occurred()) {
        _sharedns_clear(shared);
        PyMem_Free(shared);
        return NULL;
    }
    return shared;
}

static int
_shareditem_apply(struct _shareditem *item, PyObject *ns)
{
    PyObject *name = PyUnicode_FromUnicode(item->name, item->namelen);
    if (name == NULL) {
        return 1;
    }
    PyObject *value = _PyCrossInterpreterData_NewObject(&item->data);
    if (value == NULL) {
        Py_DECREF(name);
        return 1;
    }
    int res = PyDict_SetItem(ns, name, value);
    Py_DECREF(name);
    Py_DECREF(value);
    return res;
}

// XXX This cannot use PyObject fields.

struct _shared_exception {
    PyObject *exc;
    PyObject *value;
    PyObject *tb;
};

static struct _shared_exception *
_get_shared_exception(void)
{
    struct _shared_exception *exc = PyMem_NEW(struct _shared_exception, 1);
    // XXX Fatal if NULL?
    PyErr_Fetch(&exc->exc, &exc->value, &exc->tb);
    return exc;
}

static void
_apply_shared_exception(struct _shared_exception *exc)
{
    if (PyErr_Occurred()) {
        _PyErr_ChainExceptions(exc->exc, exc->value, exc->tb);
    } else {
        PyErr_Restore(exc->exc, exc->value, exc->tb);
    }

}

// XXX Return int instead.

static PyObject *
_run_script(PyInterpreterState *interp, const char *codestr,
            struct _shareditem *shared, struct _shared_exception **exc)
{
    // XXX Do not copy.

    // Get a copy of the __main__ module.
    //
    // This involves creating a new module, updating it from the
    // __main__ module and the given updates (if any), replacing the
    // __main__ with the new module in sys.modules, and then using the
    // new module's __dict__.  At the end we restore the original
    // __main__ module.
    PyObject *ns = _copy_module_ns(interp, "__main__", "_orig___main___");
    if (ns == NULL) {
        return NULL;
    }

    // Apply the cross-interpreter data.
    if (shared != NULL) {
        for (struct _shareditem *item=shared; item->name != NULL; item += 1) {
            if (_shareditem_apply(shared, ns) != 0) {
                Py_DECREF(ns);
                ns = NULL;
                goto done;
            }
        }
    }

    // Run the string (see PyRun_SimpleStringFlags).
    PyObject *result = PyRun_StringFlags(codestr, Py_file_input, ns, ns, NULL);
    if (result == NULL) {
        // Get the exception from the subinterpreter.
        *exc = _get_shared_exception();
        // XXX Clear the exception?
    } else {
        Py_DECREF(result);  // We throw away the result.
    }

done:
    // Restore __main__.
    if (_restore_module(interp, "__main__", "_orig___main___") != 0) {
        // XXX How to propagate this exception...
        //_PyErr_ChainExceptions(exc, value, tb);
    }
    return ns;
}

static PyObject *
_run_script_in_interpreter(PyInterpreterState *interp, const char *codestr,
                           PyObject *shareable)
{
    // XXX lock?
    if (_ensure_not_running(interp) < 0)
        return NULL;

    struct _shareditem *shared = _get_shared_ns(shareable);
    if (shared == NULL && PyErr_Occurred())
        return NULL;

    // Switch to interpreter.
    PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
    PyThreadState *save_tstate = PyThreadState_Swap(tstate);

    // Run the script.
    struct _shared_exception *exc = NULL;
    PyObject *result = _run_script(interp, codestr, shared, &exc);
    // XXX What to do if result is NULL?

    // Switch back.
    if (save_tstate != NULL)
        PyThreadState_Swap(save_tstate);

    // Propagate any exception out to the caller.
    if (exc != NULL) {
        _apply_shared_exception(exc);
    }

    if (shared != NULL) {
        _sharedns_clear(shared);
        PyMem_Free(shared);
    }

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
    PyObject *shared = NULL;
    if (!PyArg_UnpackTuple(args, "run_string", 2, 3, &id, &code, &shared))
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
    PyObject *ns = _run_script_in_interpreter(interp, codestr, shared);
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


/* XXX Drop run_string_unrestricted(). */

static PyObject *
interp_run_string_unrestricted(PyObject *self, PyObject *args)
{
    PyObject *id, *code, *shared = NULL;
    if (!PyArg_UnpackTuple(args, "run_string_unrestricted", 2, 3,
                           &id, &code, &shared))
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
    if (shared == Py_None)
        shared = NULL;

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
    return _run_script_in_interpreter(interp, codestr, shared);
}

PyDoc_STRVAR(run_string_unrestricted_doc,
"run_string_unrestricted(ID, sourcetext, ns=None) -> main module ns\n\
\n\
Execute the provided string in the identified interpreter.  Return the\n\
dict in which the code executed.  If the ns arg is provided then it is\n\
merged into the execution namespace before the code is executed.\n\
\n\
See PyRun_SimpleStrings.");


static PyObject *
object_is_shareable(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_UnpackTuple(args, "is_shareable", 1, 1, &obj))
        return NULL;
    if (_PyObject_CheckShareable(obj) == 0)
        Py_RETURN_TRUE;
    PyErr_Clear();
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(is_shareable_doc,
"is_shareable(obj) -> bool\n\
\n\
Return True if the object's data may be shared between interpreters and\n\
False otherwise.");


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

    {"is_shareable",            (PyCFunction)object_is_shareable,
     METH_VARARGS, is_shareable_doc},

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
