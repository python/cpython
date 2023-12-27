/* interpreters module */
/* low-level access to interpreter primitives */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_crossinterp.h"   // struct _xid
#include "pycore_interp.h"        // _PyInterpreterState_IDIncref()
#include "pycore_initconfig.h"    // _PyErr_SetFromPyStatus()
#include "pycore_long.h"          // _PyLong_IsNegative()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_pybuffer.h"      // _PyBuffer_ReleaseInInterpreterAndRawFree()
#include "pycore_pyerrors.h"      // _Py_excinfo
#include "pycore_pystate.h"       // _PyInterpreterState_SetRunningMain()

#include "interpreteridobject.h"
#include "marshal.h"              // PyMarshal_ReadObjectFromString()


#define MODULE_NAME "_xxsubinterpreters"


static PyInterpreterState *
_get_current_interp(void)
{
    // PyInterpreterState_Get() aborts if lookup fails, so don't need
    // to check the result for NULL.
    return PyInterpreterState_Get();
}

static int64_t
pylong_to_interpid(PyObject *idobj)
{
    assert(PyLong_CheckExact(idobj));

    if (_PyLong_IsNegative((PyLongObject *)idobj)) {
        PyErr_Format(PyExc_ValueError,
                     "interpreter ID must be a non-negative int, got %R",
                     idobj);
        return -1;
    }

    int overflow;
    long long id = PyLong_AsLongLongAndOverflow(idobj, &overflow);
    if (id == -1) {
        if (!overflow) {
            assert(PyErr_Occurred());
            return -1;
        }
        assert(!PyErr_Occurred());
        // For now, we don't worry about if LLONG_MAX < INT64_MAX.
        goto bad_id;
    }
#if LLONG_MAX > INT64_MAX
    if (id > INT64_MAX) {
        goto bad_id;
    }
#endif
    return (int64_t)id;

bad_id:
    PyErr_Format(PyExc_RuntimeError,
                 "unrecognized interpreter ID %O", idobj);
    return -1;
}

static int64_t
convert_interpid_obj(PyObject *arg)
{
    int64_t id = -1;
    if (_PyIndex_Check(arg)) {
        PyObject *idobj = PyNumber_Long(arg);
        if (idobj == NULL) {
            return -1;
        }
        id = pylong_to_interpid(idobj);
        Py_DECREF(idobj);
        if (id < 0) {
            return -1;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "interpreter ID must be an int, got %.100s",
                     Py_TYPE(arg)->tp_name);
        return -1;
    }
    return id;
}

static PyInterpreterState *
look_up_interp(PyObject *arg)
{
    int64_t id = convert_interpid_obj(arg);
    if (id < 0) {
        return NULL;
    }
    return _PyInterpreterState_LookUpID(id);
}


static PyObject *
interpid_to_pylong(int64_t id)
{
    assert(id < LLONG_MAX);
    return PyLong_FromLongLong(id);
}

static PyObject *
get_interpid_obj(PyInterpreterState *interp)
{
    if (_PyInterpreterState_IDInitref(interp) != 0) {
        return NULL;
    };
    int64_t id = PyInterpreterState_GetID(interp);
    if (id < 0) {
        return NULL;
    }
    return interpid_to_pylong(id);
}

static PyObject *
_get_current_module(void)
{
    PyObject *name = PyUnicode_FromString(MODULE_NAME);
    if (name == NULL) {
        return NULL;
    }
    PyObject *mod = PyImport_GetModule(name);
    Py_DECREF(name);
    if (mod == NULL) {
        return NULL;
    }
    assert(mod != Py_None);
    return mod;
}


/* Cross-interpreter Buffer Views *******************************************/

// XXX Release when the original interpreter is destroyed.

typedef struct {
    PyObject_HEAD
    Py_buffer *view;
    int64_t interpid;
} XIBufferViewObject;

static PyObject *
xibufferview_from_xid(PyTypeObject *cls, _PyCrossInterpreterData *data)
{
    assert(data->data != NULL);
    assert(data->obj == NULL);
    assert(data->interpid >= 0);
    XIBufferViewObject *self = PyObject_Malloc(sizeof(XIBufferViewObject));
    if (self == NULL) {
        return NULL;
    }
    PyObject_Init((PyObject *)self, cls);
    self->view = (Py_buffer *)data->data;
    self->interpid = data->interpid;
    return (PyObject *)self;
}

static void
xibufferview_dealloc(XIBufferViewObject *self)
{
    PyInterpreterState *interp = _PyInterpreterState_LookUpID(self->interpid);
    /* If the interpreter is no longer alive then we have problems,
       since other objects may be using the buffer still. */
    assert(interp != NULL);

    if (_PyBuffer_ReleaseInInterpreterAndRawFree(interp, self->view) < 0) {
        // XXX Emit a warning?
        PyErr_Clear();
    }

    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    /* "Instances of heap-allocated types hold a reference to their type."
     * See: https://docs.python.org/3.11/howto/isolating-extensions.html#garbage-collection-protocol
     * See: https://docs.python.org/3.11/c-api/typeobj.html#c.PyTypeObject.tp_traverse
    */
    // XXX Why don't we implement Py_TPFLAGS_HAVE_GC, e.g. Py_tp_traverse,
    // like we do for _abc._abc_data?
    Py_DECREF(tp);
}

static int
xibufferview_getbuf(XIBufferViewObject *self, Py_buffer *view, int flags)
{
    /* Only PyMemoryView_FromObject() should ever call this,
       via _memoryview_from_xid() below. */
    *view = *self->view;
    view->obj = (PyObject *)self;
    // XXX Should we leave it alone?
    view->internal = NULL;
    return 0;
}

static PyType_Slot XIBufferViewType_slots[] = {
    {Py_tp_dealloc, (destructor)xibufferview_dealloc},
    {Py_bf_getbuffer, (getbufferproc)xibufferview_getbuf},
    // We don't bother with Py_bf_releasebuffer since we don't need it.
    {0, NULL},
};

static PyType_Spec XIBufferViewType_spec = {
    .name = MODULE_NAME ".CrossInterpreterBufferView",
    .basicsize = sizeof(XIBufferViewObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = XIBufferViewType_slots,
};


static PyTypeObject * _get_current_xibufferview_type(void);

static PyObject *
_memoryview_from_xid(_PyCrossInterpreterData *data)
{
    PyTypeObject *cls = _get_current_xibufferview_type();
    if (cls == NULL) {
        return NULL;
    }
    PyObject *obj = xibufferview_from_xid(cls, data);
    if (obj == NULL) {
        return NULL;
    }
    return PyMemoryView_FromObject(obj);
}

static int
_memoryview_shared(PyThreadState *tstate, PyObject *obj,
                   _PyCrossInterpreterData *data)
{
    Py_buffer *view = PyMem_RawMalloc(sizeof(Py_buffer));
    if (view == NULL) {
        return -1;
    }
    if (PyObject_GetBuffer(obj, view, PyBUF_FULL_RO) < 0) {
        PyMem_RawFree(view);
        return -1;
    }
    _PyCrossInterpreterData_Init(data, tstate->interp, view, NULL,
                                 _memoryview_from_xid);
    return 0;
}

static int
register_memoryview_xid(PyObject *mod, PyTypeObject **p_state)
{
    // XIBufferView
    assert(*p_state == NULL);
    PyTypeObject *cls = (PyTypeObject *)PyType_FromModuleAndSpec(
                mod, &XIBufferViewType_spec, NULL);
    if (cls == NULL) {
        return -1;
    }
    if (PyModule_AddType(mod, cls) < 0) {
        Py_DECREF(cls);
        return -1;
    }
    *p_state = cls;

    // Register XID for the builtin memoryview type.
    if (_PyCrossInterpreterData_RegisterClass(
                &PyMemoryView_Type, _memoryview_shared) < 0) {
        return -1;
    }
    // We don't ever bother un-registering memoryview.

    return 0;
}



/* module state *************************************************************/

typedef struct {
    int _notused;

    /* heap types */
    PyTypeObject *XIBufferViewType;
} module_state;

static inline module_state *
get_module_state(PyObject *mod)
{
    assert(mod != NULL);
    module_state *state = PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static module_state *
_get_current_module_state(void)
{
    PyObject *mod = _get_current_module();
    if (mod == NULL) {
        // XXX import it?
        PyErr_SetString(PyExc_RuntimeError,
                        MODULE_NAME " module not imported yet");
        return NULL;
    }
    module_state *state = get_module_state(mod);
    Py_DECREF(mod);
    return state;
}

static int
traverse_module_state(module_state *state, visitproc visit, void *arg)
{
    /* heap types */
    Py_VISIT(state->XIBufferViewType);

    return 0;
}

static int
clear_module_state(module_state *state)
{
    /* heap types */
    Py_CLEAR(state->XIBufferViewType);

    return 0;
}


static PyTypeObject *
_get_current_xibufferview_type(void)
{
    module_state *state = _get_current_module_state();
    if (state == NULL) {
        return NULL;
    }
    return state->XIBufferViewType;
}


/* Python code **************************************************************/

static const char *
check_code_str(PyUnicodeObject *text)
{
    assert(text != NULL);
    if (PyUnicode_GET_LENGTH(text) == 0) {
        return "too short";
    }

    // XXX Verify that it parses?

    return NULL;
}

static const char *
check_code_object(PyCodeObject *code)
{
    assert(code != NULL);
    if (code->co_argcount > 0
        || code->co_posonlyargcount > 0
        || code->co_kwonlyargcount > 0
        || code->co_flags & (CO_VARARGS | CO_VARKEYWORDS))
    {
        return "arguments not supported";
    }
    if (code->co_ncellvars > 0) {
        return "closures not supported";
    }
    // We trust that no code objects under co_consts have unbound cell vars.

    if (code->co_executors != NULL
        || code->_co_instrumentation_version > 0)
    {
        return "only basic functions are supported";
    }
    if (code->_co_monitoring != NULL) {
        return "only basic functions are supported";
    }
    if (code->co_extra != NULL) {
        return "only basic functions are supported";
    }

    return NULL;
}

#define RUN_TEXT 1
#define RUN_CODE 2

static const char *
get_code_str(PyObject *arg, Py_ssize_t *len_p, PyObject **bytes_p, int *flags_p)
{
    const char *codestr = NULL;
    Py_ssize_t len = -1;
    PyObject *bytes_obj = NULL;
    int flags = 0;

    if (PyUnicode_Check(arg)) {
        assert(PyUnicode_CheckExact(arg)
               && (check_code_str((PyUnicodeObject *)arg) == NULL));
        codestr = PyUnicode_AsUTF8AndSize(arg, &len);
        if (codestr == NULL) {
            return NULL;
        }
        if (strlen(codestr) != (size_t)len) {
            PyErr_SetString(PyExc_ValueError,
                            "source code string cannot contain null bytes");
            return NULL;
        }
        flags = RUN_TEXT;
    }
    else {
        assert(PyCode_Check(arg)
               && (check_code_object((PyCodeObject *)arg) == NULL));
        flags = RUN_CODE;

        // Serialize the code object.
        bytes_obj = PyMarshal_WriteObjectToString(arg, Py_MARSHAL_VERSION);
        if (bytes_obj == NULL) {
            return NULL;
        }
        codestr = PyBytes_AS_STRING(bytes_obj);
        len = PyBytes_GET_SIZE(bytes_obj);
    }

    *flags_p = flags;
    *bytes_p = bytes_obj;
    *len_p = len;
    return codestr;
}


/* interpreter-specific code ************************************************/

static int
_run_script(PyObject *ns, const char *codestr, Py_ssize_t codestrlen, int flags)
{
    PyObject *result = NULL;
    if (flags & RUN_TEXT) {
        result = PyRun_StringFlags(codestr, Py_file_input, ns, ns, NULL);
    }
    else if (flags & RUN_CODE) {
        PyObject *code = PyMarshal_ReadObjectFromString(codestr, codestrlen);
        if (code != NULL) {
            result = PyEval_EvalCode(code, ns, ns);
            Py_DECREF(code);
        }
    }
    else {
        Py_UNREACHABLE();
    }
    if (result == NULL) {
        return -1;
    }
    Py_DECREF(result);  // We throw away the result.
    return 0;
}

static int
_run_in_interpreter(PyInterpreterState *interp,
                    const char *codestr, Py_ssize_t codestrlen,
                    PyObject *shareables, int flags,
                    PyObject **p_excinfo)
{
    assert(!PyErr_Occurred());
    _PyXI_session session = {0};

    // Prep and switch interpreters.
    if (_PyXI_Enter(&session, interp, shareables) < 0) {
        assert(!PyErr_Occurred());
        PyObject *excinfo = _PyXI_ApplyError(session.error);
        if (excinfo != NULL) {
            *p_excinfo = excinfo;
        }
        assert(PyErr_Occurred());
        return -1;
    }

    // Run the script.
    int res = _run_script(session.main_ns, codestr, codestrlen, flags);

    // Clean up and switch back.
    _PyXI_Exit(&session);

    // Propagate any exception out to the caller.
    assert(!PyErr_Occurred());
    if (res < 0) {
        PyObject *excinfo = _PyXI_ApplyCapturedException(&session);
        if (excinfo != NULL) {
            *p_excinfo = excinfo;
        }
    }
    else {
        assert(!_PyXI_HasCapturedException(&session));
    }

    return res;
}


/* module level code ********************************************************/

static PyObject *
interp_create(PyObject *self, PyObject *args, PyObject *kwds)
{

    static char *kwlist[] = {"isolated", NULL};
    int isolated = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|$i:create", kwlist,
                                     &isolated)) {
        return NULL;
    }

    // Create and initialize the new interpreter.
    PyThreadState *save_tstate = PyThreadState_Get();
    assert(save_tstate != NULL);
    const PyInterpreterConfig config = isolated
        ? (PyInterpreterConfig)_PyInterpreterConfig_INIT
        : (PyInterpreterConfig)_PyInterpreterConfig_LEGACY_INIT;

    // XXX Possible GILState issues?
    PyThreadState *tstate = NULL;
    PyStatus status = Py_NewInterpreterFromConfig(&tstate, &config);
    PyThreadState_Swap(save_tstate);
    if (PyStatus_Exception(status)) {
        /* Since no new thread state was created, there is no exception to
           propagate; raise a fresh one after swapping in the old thread
           state. */
        _PyErr_SetFromPyStatus(status);
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_RuntimeError, "interpreter creation failed");
        _PyErr_ChainExceptions1(exc);
        return NULL;
    }
    assert(tstate != NULL);

    PyInterpreterState *interp = PyThreadState_GetInterpreter(tstate);
    PyObject *idobj = get_interpid_obj(interp);
    if (idobj == NULL) {
        // XXX Possible GILState issues?
        save_tstate = PyThreadState_Swap(tstate);
        Py_EndInterpreter(tstate);
        PyThreadState_Swap(save_tstate);
        return NULL;
    }

    PyThreadState_Swap(tstate);
    PyThreadState_Clear(tstate);
    PyThreadState_Swap(save_tstate);
    PyThreadState_Delete(tstate);

    _PyInterpreterState_RequireIDRef(interp, 1);
    return idobj;
}

PyDoc_STRVAR(create_doc,
"create() -> ID\n\
\n\
Create a new interpreter and return a unique generated ID.\n\
\n\
The caller is responsible for destroying the interpreter before exiting.");


static PyObject *
interp_destroy(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", NULL};
    PyObject *id;
    // XXX Use "L" for id?
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O:destroy", kwlist, &id)) {
        return NULL;
    }

    // Look up the interpreter.
    PyInterpreterState *interp = look_up_interp(id);
    if (interp == NULL) {
        return NULL;
    }

    // Ensure we don't try to destroy the current interpreter.
    PyInterpreterState *current = _get_current_interp();
    if (current == NULL) {
        return NULL;
    }
    if (interp == current) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot destroy the current interpreter");
        return NULL;
    }

    // Ensure the interpreter isn't running.
    /* XXX We *could* support destroying a running interpreter but
       aren't going to worry about it for now. */
    if (_PyInterpreterState_IsRunningMain(interp)) {
        PyErr_Format(PyExc_RuntimeError, "interpreter running");
        return NULL;
    }

    // Destroy the interpreter.
    PyThreadState *tstate = PyThreadState_New(interp);
    tstate->_whence = _PyThreadState_WHENCE_INTERP;
    // XXX Possible GILState issues?
    PyThreadState *save_tstate = PyThreadState_Swap(tstate);
    Py_EndInterpreter(tstate);
    PyThreadState_Swap(save_tstate);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(destroy_doc,
"destroy(id)\n\
\n\
Destroy the identified interpreter.\n\
\n\
Attempting to destroy the current interpreter results in a RuntimeError.\n\
So does an unrecognized ID.");


static PyObject *
interp_list_all(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *ids, *id;
    PyInterpreterState *interp;

    ids = PyList_New(0);
    if (ids == NULL) {
        return NULL;
    }

    interp = PyInterpreterState_Head();
    while (interp != NULL) {
        id = get_interpid_obj(interp);
        if (id == NULL) {
            Py_DECREF(ids);
            return NULL;
        }
        // insert at front of list
        int res = PyList_Insert(ids, 0, id);
        Py_DECREF(id);
        if (res < 0) {
            Py_DECREF(ids);
            return NULL;
        }

        interp = PyInterpreterState_Next(interp);
    }

    return ids;
}

PyDoc_STRVAR(list_all_doc,
"list_all() -> [ID]\n\
\n\
Return a list containing the ID of every existing interpreter.");


static PyObject *
interp_get_current(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyInterpreterState *interp =_get_current_interp();
    if (interp == NULL) {
        return NULL;
    }
    return get_interpid_obj(interp);
}

PyDoc_STRVAR(get_current_doc,
"get_current() -> ID\n\
\n\
Return the ID of current interpreter.");


static PyObject *
interp_get_main(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    // Currently, 0 is always the main interpreter.
    int64_t id = 0;
    return PyLong_FromLongLong(id);
}

PyDoc_STRVAR(get_main_doc,
"get_main() -> ID\n\
\n\
Return the ID of main interpreter.");

static PyObject *
interp_set___main___attrs(PyObject *self, PyObject *args)
{
    PyObject *id, *updates;
    if (!PyArg_ParseTuple(args, "OO:" MODULE_NAME ".set___main___attrs",
                          &id, &updates))
    {
        return NULL;
    }

    // Look up the interpreter.
    PyInterpreterState *interp = PyInterpreterID_LookUp(id);
    if (interp == NULL) {
        return NULL;
    }

    // Check the updates.
    if (updates != Py_None) {
        Py_ssize_t size = PyObject_Size(updates);
        if (size < 0) {
            return NULL;
        }
        if (size == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "arg 2 must be a non-empty mapping");
            return NULL;
        }
    }

    _PyXI_session session = {0};

    // Prep and switch interpreters, including apply the updates.
    if (_PyXI_Enter(&session, interp, updates) < 0) {
        if (!PyErr_Occurred()) {
            _PyXI_ApplyCapturedException(&session);
            assert(PyErr_Occurred());
        }
        else {
            assert(!_PyXI_HasCapturedException(&session));
        }
        return NULL;
    }

    // Clean up and switch back.
    _PyXI_Exit(&session);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(set___main___attrs_doc,
"set___main___attrs(id, ns)\n\
\n\
Bind the given attributes in the interpreter's __main__ module.");

static PyUnicodeObject *
convert_script_arg(PyObject *arg, const char *fname, const char *displayname,
                   const char *expected)
{
    PyUnicodeObject *str = NULL;
    if (PyUnicode_CheckExact(arg)) {
        str = (PyUnicodeObject *)Py_NewRef(arg);
    }
    else if (PyUnicode_Check(arg)) {
        // XXX str = PyUnicode_FromObject(arg);
        str = (PyUnicodeObject *)Py_NewRef(arg);
    }
    else {
        _PyArg_BadArgument(fname, displayname, expected, arg);
        return NULL;
    }

    const char *err = check_code_str(str);
    if (err != NULL) {
        Py_DECREF(str);
        PyErr_Format(PyExc_ValueError,
                     "%.200s(): bad script text (%s)", fname, err);
        return NULL;
    }

    return str;
}

static PyCodeObject *
convert_code_arg(PyObject *arg, const char *fname, const char *displayname,
                 const char *expected)
{
    const char *kind = NULL;
    PyCodeObject *code = NULL;
    if (PyFunction_Check(arg)) {
        if (PyFunction_GetClosure(arg) != NULL) {
            PyErr_Format(PyExc_ValueError,
                         "%.200s(): closures not supported", fname);
            return NULL;
        }
        code = (PyCodeObject *)PyFunction_GetCode(arg);
        if (code == NULL) {
            if (PyErr_Occurred()) {
                // This chains.
                PyErr_Format(PyExc_ValueError,
                             "%.200s(): bad func", fname);
            }
            else {
                PyErr_Format(PyExc_ValueError,
                             "%.200s(): func.__code__ missing", fname);
            }
            return NULL;
        }
        Py_INCREF(code);
        kind = "func";
    }
    else if (PyCode_Check(arg)) {
        code = (PyCodeObject *)Py_NewRef(arg);
        kind = "code object";
    }
    else {
        _PyArg_BadArgument(fname, displayname, expected, arg);
        return NULL;
    }

    const char *err = check_code_object(code);
    if (err != NULL) {
        Py_DECREF(code);
        PyErr_Format(PyExc_ValueError,
                     "%.200s(): bad %s (%s)", fname, kind, err);
        return NULL;
    }

    return code;
}

static int
_interp_exec(PyObject *self,
             PyObject *id_arg, PyObject *code_arg, PyObject *shared_arg,
             PyObject **p_excinfo)
{
    // Look up the interpreter.
    PyInterpreterState *interp = look_up_interp(id_arg);
    if (interp == NULL) {
        return -1;
    }

    // Extract code.
    Py_ssize_t codestrlen = -1;
    PyObject *bytes_obj = NULL;
    int flags = 0;
    const char *codestr = get_code_str(code_arg,
                                       &codestrlen, &bytes_obj, &flags);
    if (codestr == NULL) {
        return -1;
    }

    // Run the code in the interpreter.
    int res = _run_in_interpreter(interp, codestr, codestrlen,
                                  shared_arg, flags, p_excinfo);
    Py_XDECREF(bytes_obj);
    if (res < 0) {
        return -1;
    }

    return 0;
}

static PyObject *
interp_exec(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "code", "shared", NULL};
    PyObject *id, *code;
    PyObject *shared = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "OO|O:" MODULE_NAME ".exec", kwlist,
                                     &id, &code, &shared)) {
        return NULL;
    }

    const char *expected = "a string, a function, or a code object";
    if (PyUnicode_Check(code)) {
         code = (PyObject *)convert_script_arg(code, MODULE_NAME ".exec",
                                               "argument 2", expected);
    }
    else {
         code = (PyObject *)convert_code_arg(code, MODULE_NAME ".exec",
                                             "argument 2", expected);
    }
    if (code == NULL) {
        return NULL;
    }

    PyObject *excinfo = NULL;
    int res = _interp_exec(self, id, code, shared, &excinfo);
    Py_DECREF(code);
    if (res < 0) {
        assert((excinfo == NULL) != (PyErr_Occurred() == NULL));
        return excinfo;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(exec_doc,
"exec(id, code, shared=None)\n\
\n\
Execute the provided code in the identified interpreter.\n\
This is equivalent to running the builtin exec() under the target\n\
interpreter, using the __dict__ of its __main__ module as both\n\
globals and locals.\n\
\n\
\"code\" may be a string containing the text of a Python script.\n\
\n\
Functions (and code objects) are also supported, with some restrictions.\n\
The code/function must not take any arguments or be a closure\n\
(i.e. have cell vars).  Methods and other callables are not supported.\n\
\n\
If a function is provided, its code object is used and all its state\n\
is ignored, including its __globals__ dict.");

static PyObject *
interp_run_string(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "script", "shared", NULL};
    PyObject *id, *script;
    PyObject *shared = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "OU|O:" MODULE_NAME ".run_string", kwlist,
                                     &id, &script, &shared)) {
        return NULL;
    }

    script = (PyObject *)convert_script_arg(script, MODULE_NAME ".exec",
                                            "argument 2", "a string");
    if (script == NULL) {
        return NULL;
    }

    PyObject *excinfo = NULL;
    int res = _interp_exec(self, id, script, shared, &excinfo);
    Py_DECREF(script);
    if (res < 0) {
        assert((excinfo == NULL) != (PyErr_Occurred() == NULL));
        return excinfo;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(run_string_doc,
"run_string(id, script, shared=None)\n\
\n\
Execute the provided string in the identified interpreter.\n\
\n\
(See " MODULE_NAME ".exec().");

static PyObject *
interp_run_func(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "func", "shared", NULL};
    PyObject *id, *func;
    PyObject *shared = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "OO|O:" MODULE_NAME ".run_func", kwlist,
                                     &id, &func, &shared)) {
        return NULL;
    }

    PyCodeObject *code = convert_code_arg(func, MODULE_NAME ".exec",
                                          "argument 2",
                                          "a function or a code object");
    if (code == NULL) {
        return NULL;
    }

    PyObject *excinfo = NULL;
    int res = _interp_exec(self, id, (PyObject *)code, shared, &excinfo);
    Py_DECREF(code);
    if (res < 0) {
        assert((excinfo == NULL) != (PyErr_Occurred() == NULL));
        return excinfo;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(run_func_doc,
"run_func(id, func, shared=None)\n\
\n\
Execute the body of the provided function in the identified interpreter.\n\
Code objects are also supported.  In both cases, closures and args\n\
are not supported.  Methods and other callables are not supported either.\n\
\n\
(See " MODULE_NAME ".exec().");


static PyObject *
object_is_shareable(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"obj", NULL};
    PyObject *obj;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O:is_shareable", kwlist, &obj)) {
        return NULL;
    }

    if (_PyObject_CheckCrossInterpreterData(obj) == 0) {
        Py_RETURN_TRUE;
    }
    PyErr_Clear();
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(is_shareable_doc,
"is_shareable(obj) -> bool\n\
\n\
Return True if the object's data may be shared between interpreters and\n\
False otherwise.");


static PyObject *
interp_is_running(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", NULL};
    PyObject *id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O:is_running", kwlist, &id)) {
        return NULL;
    }

    PyInterpreterState *interp = look_up_interp(id);
    if (interp == NULL) {
        return NULL;
    }
    if (_PyInterpreterState_IsRunningMain(interp)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(is_running_doc,
"is_running(id) -> bool\n\
\n\
Return whether or not the identified interpreter is running.");


static PyObject *
interp_incref(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", NULL};
    PyObject *id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O:_incref", kwlist, &id)) {
        return NULL;
    }

    PyInterpreterState *interp = look_up_interp(id);
    if (interp == NULL) {
        return NULL;
    }
    if (_PyInterpreterState_IDInitref(interp) < 0) {
        return NULL;
    }
    _PyInterpreterState_IDIncref(interp);

    Py_RETURN_NONE;
}


static PyObject *
interp_decref(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", NULL};
    PyObject *id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O:_incref", kwlist, &id)) {
        return NULL;
    }

    PyInterpreterState *interp = look_up_interp(id);
    if (interp == NULL) {
        return NULL;
    }
    _PyInterpreterState_IDDecref(interp);

    Py_RETURN_NONE;
}


static PyMethodDef module_functions[] = {
    {"create",                    _PyCFunction_CAST(interp_create),
     METH_VARARGS | METH_KEYWORDS, create_doc},
    {"destroy",                   _PyCFunction_CAST(interp_destroy),
     METH_VARARGS | METH_KEYWORDS, destroy_doc},
    {"list_all",                  interp_list_all,
     METH_NOARGS, list_all_doc},
    {"get_current",               interp_get_current,
     METH_NOARGS, get_current_doc},
    {"get_main",                  interp_get_main,
     METH_NOARGS, get_main_doc},

    {"is_running",                _PyCFunction_CAST(interp_is_running),
     METH_VARARGS | METH_KEYWORDS, is_running_doc},
    {"exec",                      _PyCFunction_CAST(interp_exec),
     METH_VARARGS | METH_KEYWORDS, exec_doc},
    {"run_string",                _PyCFunction_CAST(interp_run_string),
     METH_VARARGS | METH_KEYWORDS, run_string_doc},
    {"run_func",                  _PyCFunction_CAST(interp_run_func),
     METH_VARARGS | METH_KEYWORDS, run_func_doc},

    {"set___main___attrs",        _PyCFunction_CAST(interp_set___main___attrs),
     METH_VARARGS, set___main___attrs_doc},
    {"is_shareable",              _PyCFunction_CAST(object_is_shareable),
     METH_VARARGS | METH_KEYWORDS, is_shareable_doc},

    {"_incref",                   _PyCFunction_CAST(interp_incref),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"_decref",                   _PyCFunction_CAST(interp_decref),
     METH_VARARGS | METH_KEYWORDS, NULL},

    {NULL,                        NULL}           /* sentinel */
};


/* initialization function */

PyDoc_STRVAR(module_doc,
"This module provides primitive operations to manage Python interpreters.\n\
The 'interpreters' module provides a more convenient interface.");

static int
module_exec(PyObject *mod)
{
    module_state *state = get_module_state(mod);

    // exceptions
    if (PyModule_AddType(mod, (PyTypeObject *)PyExc_InterpreterError) < 0) {
        goto error;
    }
    if (PyModule_AddType(mod, (PyTypeObject *)PyExc_InterpreterNotFoundError) < 0) {
        goto error;
    }

    if (register_memoryview_xid(mod, &state->XIBufferViewType) < 0) {
        goto error;
    }

    return 0;

error:
    return -1;
}

static struct PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL},
};

static int
module_traverse(PyObject *mod, visitproc visit, void *arg)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);
    traverse_module_state(state, visit, arg);
    return 0;
}

static int
module_clear(PyObject *mod)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);
    clear_module_state(state);
    return 0;
}

static void
module_free(void *mod)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);
    clear_module_state(state);
}

static struct PyModuleDef moduledef = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME,
    .m_doc = module_doc,
    .m_size = sizeof(module_state),
    .m_methods = module_functions,
    .m_slots = module_slots,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_free = (freefunc)module_free,
};

PyMODINIT_FUNC
PyInit__xxsubinterpreters(void)
{
    return PyModuleDef_Init(&moduledef);
}
