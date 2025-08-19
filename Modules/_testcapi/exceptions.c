// clinic/exceptions.c.h uses internal pycore_modsupport.h API
#define PYTESTCAPI_NEED_INTERNAL_API

#include "parts.h"
#include "util.h"

#include "clinic/exceptions.c.h"


/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/

/*[clinic input]
_testcapi.err_set_raised
    exception as exc: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_err_set_raised(PyObject *module, PyObject *exc)
/*[clinic end generated code: output=0a0c7743961fcae5 input=c5f7331864a94df9]*/
{
    Py_INCREF(exc);
    PyErr_SetRaisedException(exc);
    assert(PyErr_Occurred());
    return NULL;
}

static PyObject *
err_restore(PyObject *self, PyObject *args) {
    PyObject *type = NULL, *value = NULL, *traceback = NULL;
    switch(PyTuple_Size(args)) {
        case 3:
            traceback = PyTuple_GetItem(args, 2);
            Py_INCREF(traceback);
            _Py_FALLTHROUGH;
        case 2:
            value = PyTuple_GetItem(args, 1);
            Py_INCREF(value);
            _Py_FALLTHROUGH;
        case 1:
            type = PyTuple_GetItem(args, 0);
            Py_INCREF(type);
            break;
        default:
            PyErr_SetString(PyExc_TypeError,
                        "wrong number of arguments");
            return NULL;
    }
    PyErr_Restore(type, value, traceback);
    assert(PyErr_Occurred());
    return NULL;
}

/*[clinic input]
_testcapi.exception_print
    exception as exc: object
    legacy: bool = False
    /

To test the format of exceptions as printed out.
[clinic start generated code]*/

static PyObject *
_testcapi_exception_print_impl(PyObject *module, PyObject *exc, int legacy)
/*[clinic end generated code: output=3f04fe0c18412ae0 input=c76f42cb94136dbf]*/
{
    if (legacy) {
        PyObject *tb = NULL;
        if (PyExceptionInstance_Check(exc)) {
            tb = PyException_GetTraceback(exc);
        }
        PyErr_Display((PyObject *) Py_TYPE(exc), exc, tb);
        Py_XDECREF(tb);
    }
    else {
        PyErr_DisplayException(exc);
    }
    Py_RETURN_NONE;
}

/*[clinic input]
@permit_long_summary
_testcapi.make_exception_with_doc
    name: str
    doc: str = NULL
    base: object = NULL
    dict: object = NULL

Test PyErr_NewExceptionWithDoc (also exercise PyErr_NewException). Run via Lib/test/test_exceptions.py
[clinic start generated code]*/

static PyObject *
_testcapi_make_exception_with_doc_impl(PyObject *module, const char *name,
                                       const char *doc, PyObject *base,
                                       PyObject *dict)
/*[clinic end generated code: output=439f0d963c1ce2c4 input=508b420b7f9253ed]*/
{
    return PyErr_NewExceptionWithDoc(name, doc, base, dict);
}

/*[clinic input]
_testcapi.exc_set_object
    exception as exc: object
    obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_exc_set_object_impl(PyObject *module, PyObject *exc, PyObject *obj)
/*[clinic end generated code: output=34c8c7c83e5c8463 input=fc530aafb1b0a360]*/
{
    PyErr_SetObject(exc, obj);
    return NULL;
}

/*[clinic input]
_testcapi.exc_set_object_fetch = _testcapi.exc_set_object
[clinic start generated code]*/

static PyObject *
_testcapi_exc_set_object_fetch_impl(PyObject *module, PyObject *exc,
                                    PyObject *obj)
/*[clinic end generated code: output=7a5ff5f6d3cf687f input=77ec686f1f95fa38]*/
{
    PyObject *type = UNINITIALIZED_PTR;
    PyObject *value = UNINITIALIZED_PTR;
    PyObject *tb = UNINITIALIZED_PTR;

    PyErr_SetObject(exc, obj);
    PyErr_Fetch(&type, &value, &tb);
    assert(type != UNINITIALIZED_PTR);
    assert(value != UNINITIALIZED_PTR);
    assert(tb != UNINITIALIZED_PTR);
    Py_XDECREF(type);
    Py_XDECREF(tb);
    return value;
}

/*[clinic input]
_testcapi.err_setstring
    exc: object
    value: str(zeroes=True, accept={robuffer, str, NoneType})
    /
[clinic start generated code]*/

static PyObject *
_testcapi_err_setstring_impl(PyObject *module, PyObject *exc,
                             const char *value, Py_ssize_t value_length)
/*[clinic end generated code: output=fba8705e5703dd3f input=e8a95fad66d9004b]*/
{
    NULLABLE(exc);
    PyErr_SetString(exc, value);
    return NULL;
}

/*[clinic input]
_testcapi.err_setfromerrnowithfilename
    error: int
    exc: object
    value: str(zeroes=True, accept={robuffer, str, NoneType})
    /
[clinic start generated code]*/

static PyObject *
_testcapi_err_setfromerrnowithfilename_impl(PyObject *module, int error,
                                            PyObject *exc, const char *value,
                                            Py_ssize_t value_length)
/*[clinic end generated code: output=d02df5749a01850e input=ff7c384234bf097f]*/
{
    NULLABLE(exc);
    errno = error;
    PyErr_SetFromErrnoWithFilename(exc, value);
    return NULL;
}

/*[clinic input]
_testcapi.raise_exception
    exception as exc: object
    num_args: int
    /
[clinic start generated code]*/

static PyObject *
_testcapi_raise_exception_impl(PyObject *module, PyObject *exc, int num_args)
/*[clinic end generated code: output=eb0a9c5d69e0542d input=83d6262c3829d088]*/
{
    PyObject *exc_args = PyTuple_New(num_args);
    if (exc_args == NULL) {
        return NULL;
    }
    for (int i = 0; i < num_args; ++i) {
        PyObject *v = PyLong_FromLong(i);
        if (v == NULL) {
            Py_DECREF(exc_args);
            return NULL;
        }
        PyTuple_SET_ITEM(exc_args, i, v);
    }
    PyErr_SetObject(exc, exc_args);
    Py_DECREF(exc_args);
    return NULL;
}

/*[clinic input]
_testcapi.raise_memoryerror
[clinic start generated code]*/

static PyObject *
_testcapi_raise_memoryerror_impl(PyObject *module)
/*[clinic end generated code: output=dd057803fb0131e6 input=6ca521bd07fb73cb]*/
{
    return PyErr_NoMemory();
}

/*[clinic input]
_testcapi.fatal_error
    message: str(accept={robuffer})
    release_gil: bool = False
    /
[clinic start generated code]*/

static PyObject *
_testcapi_fatal_error_impl(PyObject *module, const char *message,
                           int release_gil)
/*[clinic end generated code: output=9c3237116e6a03e8 input=1be357a2ccb04c8c]*/
{
    if (release_gil) {
        Py_BEGIN_ALLOW_THREADS
        Py_FatalError(message);
        Py_END_ALLOW_THREADS
    }
    else {
        Py_FatalError(message);
    }
    // Py_FatalError() does not return, but exits the process.
    Py_RETURN_NONE;
}

/*[clinic input]
_testcapi.set_exc_info
    new_type: object
    new_value: object
    new_tb: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_set_exc_info_impl(PyObject *module, PyObject *new_type,
                            PyObject *new_value, PyObject *new_tb)
/*[clinic end generated code: output=b55fa35dec31300e input=ea9f19e0f55fe5b3]*/
{
    PyObject *type = UNINITIALIZED_PTR, *value = UNINITIALIZED_PTR, *tb = UNINITIALIZED_PTR;
    PyErr_GetExcInfo(&type, &value, &tb);

    Py_INCREF(new_type);
    Py_INCREF(new_value);
    Py_INCREF(new_tb);
    PyErr_SetExcInfo(new_type, new_value, new_tb);

    PyObject *orig_exc = PyTuple_Pack(3,
            type  ? type  : Py_None,
            value ? value : Py_None,
            tb    ? tb    : Py_None);
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(tb);
    return orig_exc;
}

/*[clinic input]
_testcapi.set_exception
    new_exc: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_set_exception(PyObject *module, PyObject *new_exc)
/*[clinic end generated code: output=8b969b35d029e96d input=c89d4ca966c69738]*/
{
    PyObject *exc = PyErr_GetHandledException();
    assert(PyExceptionInstance_Check(exc) || exc == NULL);
    PyErr_SetHandledException(new_exc);
    return exc;
}

/*[clinic input]
_testcapi.traceback_print
    traceback: object
    file: object
    /
To test the format of tracebacks as printed out.
[clinic start generated code]*/

static PyObject *
_testcapi_traceback_print_impl(PyObject *module, PyObject *traceback,
                               PyObject *file)
/*[clinic end generated code: output=17074ecf9d95cf30 input=9423f2857b008ca8]*/
{
    if (PyTraceBack_Print(traceback, file) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
err_writeunraisable(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *exc, *obj;
    if (!PyArg_ParseTuple(args, "OO", &exc, &obj)) {
        return NULL;
    }
    NULLABLE(exc);
    NULLABLE(obj);
    if (exc) {
        PyErr_SetRaisedException(Py_NewRef(exc));
    }
    PyErr_WriteUnraisable(obj);
    Py_RETURN_NONE;
}

static PyObject *
err_formatunraisable(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *exc;
    const char *fmt;
    Py_ssize_t fmtlen;
    PyObject *objs[10] = {NULL};

    if (!PyArg_ParseTuple(args, "Oz#|OOOOOOOOOO", &exc, &fmt, &fmtlen,
            &objs[0], &objs[1], &objs[2], &objs[3], &objs[4],
            &objs[5], &objs[6], &objs[7], &objs[8], &objs[9]))
    {
        return NULL;
    }
    NULLABLE(exc);
    if (exc) {
        PyErr_SetRaisedException(Py_NewRef(exc));
    }
    PyErr_FormatUnraisable(fmt,
            objs[0], objs[1], objs[2], objs[3], objs[4],
            objs[5], objs[6], objs[7], objs[8], objs[9]);
    Py_RETURN_NONE;
}

/*[clinic input]
_testcapi.unstable_exc_prep_reraise_star
    orig: object
    excs: object
    /
To test PyUnstable_Exc_PrepReraiseStar.
[clinic start generated code]*/

static PyObject *
_testcapi_unstable_exc_prep_reraise_star_impl(PyObject *module,
                                              PyObject *orig, PyObject *excs)
/*[clinic end generated code: output=850cf008e0563c77 input=27fbcda2203eb301]*/
{
    return PyUnstable_Exc_PrepReraiseStar(orig, excs);
}

/* Test PyUnicodeEncodeError_GetStart */
static PyObject *
unicode_encode_get_start(PyObject *Py_UNUSED(module), PyObject *arg)
{
    Py_ssize_t start;
    if (PyUnicodeEncodeError_GetStart(arg, &start) < 0) {
        return NULL;
    }
    RETURN_SIZE(start);
}

/* Test PyUnicodeDecodeError_GetStart */
static PyObject *
unicode_decode_get_start(PyObject *Py_UNUSED(module), PyObject *arg)
{
    Py_ssize_t start;
    if (PyUnicodeDecodeError_GetStart(arg, &start) < 0) {
        return NULL;
    }
    RETURN_SIZE(start);
}

/* Test PyUnicodeTranslateError_GetStart */
static PyObject *
unicode_translate_get_start(PyObject *Py_UNUSED(module), PyObject *arg)
{
    Py_ssize_t start;
    if (PyUnicodeTranslateError_GetStart(arg, &start) < 0) {
        return NULL;
    }
    RETURN_SIZE(start);
}

/* Test PyUnicodeEncodeError_SetStart */
static PyObject *
unicode_encode_set_start(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *exc;
    Py_ssize_t start;
    if (PyArg_ParseTuple(args, "On", &exc, &start) < 0) {
        return NULL;
    }
    if (PyUnicodeEncodeError_SetStart(exc, start) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/* Test PyUnicodeDecodeError_SetStart */
static PyObject *
unicode_decode_set_start(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *exc;
    Py_ssize_t start;
    if (PyArg_ParseTuple(args, "On", &exc, &start) < 0) {
        return NULL;
    }
    if (PyUnicodeDecodeError_SetStart(exc, start) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/* Test PyUnicodeTranslateError_SetStart */
static PyObject *
unicode_translate_set_start(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *exc;
    Py_ssize_t start;
    if (PyArg_ParseTuple(args, "On", &exc, &start) < 0) {
        return NULL;
    }
    if (PyUnicodeTranslateError_SetStart(exc, start) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/* Test PyUnicodeEncodeError_GetEnd */
static PyObject *
unicode_encode_get_end(PyObject *Py_UNUSED(module), PyObject *arg)
{
    Py_ssize_t end;
    if (PyUnicodeEncodeError_GetEnd(arg, &end) < 0) {
        return NULL;
    }
    RETURN_SIZE(end);
}

/* Test PyUnicodeDecodeError_GetEnd */
static PyObject *
unicode_decode_get_end(PyObject *Py_UNUSED(module), PyObject *arg)
{
    Py_ssize_t end;
    if (PyUnicodeDecodeError_GetEnd(arg, &end) < 0) {
        return NULL;
    }
    RETURN_SIZE(end);
}

/* Test PyUnicodeTranslateError_GetEnd */
static PyObject *
unicode_translate_get_end(PyObject *Py_UNUSED(module), PyObject *arg)
{
    Py_ssize_t end;
    if (PyUnicodeTranslateError_GetEnd(arg, &end) < 0) {
        return NULL;
    }
    RETURN_SIZE(end);
}

/* Test PyUnicodeEncodeError_SetEnd */
static PyObject *
unicode_encode_set_end(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *exc;
    Py_ssize_t end;
    if (PyArg_ParseTuple(args, "On", &exc, &end) < 0) {
        return NULL;
    }
    if (PyUnicodeEncodeError_SetEnd(exc, end) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/* Test PyUnicodeDecodeError_SetEnd */
static PyObject *
unicode_decode_set_end(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *exc;
    Py_ssize_t end;
    if (PyArg_ParseTuple(args, "On", &exc, &end) < 0) {
        return NULL;
    }
    if (PyUnicodeDecodeError_SetEnd(exc, end) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/* Test PyUnicodeTranslateError_SetEnd */
static PyObject *
unicode_translate_set_end(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *exc;
    Py_ssize_t end;
    if (PyArg_ParseTuple(args, "On", &exc, &end) < 0) {
        return NULL;
    }
    if (PyUnicodeTranslateError_SetEnd(exc, end) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*
 * Define the PyRecurdingInfinitelyError_Type
 */

static PyTypeObject PyRecursingInfinitelyError_Type;

static int
recurse_infinitely_error_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *type = (PyObject *)&PyRecursingInfinitelyError_Type;

    /* Instantiating this exception starts infinite recursion. */
    Py_INCREF(type);
    PyErr_SetObject(type, NULL);
    return -1;
}

static PyTypeObject PyRecursingInfinitelyError_Type = {
    .tp_name = "RecursingInfinitelyError",
    .tp_basicsize = sizeof(PyBaseExceptionObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("Instantiating this exception starts infinite recursion."),
    .tp_init = recurse_infinitely_error_init,
};

static PyMethodDef test_methods[] = {
    {"err_restore",             err_restore,                     METH_VARARGS},
    {"err_writeunraisable",     err_writeunraisable,             METH_VARARGS},
    {"err_formatunraisable",    err_formatunraisable,            METH_VARARGS},
    _TESTCAPI_ERR_SET_RAISED_METHODDEF
    _TESTCAPI_EXCEPTION_PRINT_METHODDEF
    _TESTCAPI_FATAL_ERROR_METHODDEF
    _TESTCAPI_MAKE_EXCEPTION_WITH_DOC_METHODDEF
    _TESTCAPI_EXC_SET_OBJECT_METHODDEF
    _TESTCAPI_EXC_SET_OBJECT_FETCH_METHODDEF
    _TESTCAPI_ERR_SETSTRING_METHODDEF
    _TESTCAPI_ERR_SETFROMERRNOWITHFILENAME_METHODDEF
    _TESTCAPI_RAISE_EXCEPTION_METHODDEF
    _TESTCAPI_RAISE_MEMORYERROR_METHODDEF
    _TESTCAPI_SET_EXC_INFO_METHODDEF
    _TESTCAPI_SET_EXCEPTION_METHODDEF
    _TESTCAPI_TRACEBACK_PRINT_METHODDEF
    _TESTCAPI_UNSTABLE_EXC_PREP_RERAISE_STAR_METHODDEF
    {"unicode_encode_get_start", unicode_encode_get_start,       METH_O},
    {"unicode_decode_get_start", unicode_decode_get_start,       METH_O},
    {"unicode_translate_get_start", unicode_translate_get_start, METH_O},
    {"unicode_encode_set_start", unicode_encode_set_start,       METH_VARARGS},
    {"unicode_decode_set_start", unicode_decode_set_start,       METH_VARARGS},
    {"unicode_translate_set_start", unicode_translate_set_start, METH_VARARGS},
    {"unicode_encode_get_end", unicode_encode_get_end,           METH_O},
    {"unicode_decode_get_end", unicode_decode_get_end,           METH_O},
    {"unicode_translate_get_end", unicode_translate_get_end,     METH_O},
    {"unicode_encode_set_end", unicode_encode_set_end,           METH_VARARGS},
    {"unicode_decode_set_end", unicode_decode_set_end,           METH_VARARGS},
    {"unicode_translate_set_end", unicode_translate_set_end,     METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Exceptions(PyObject *mod)
{
    PyRecursingInfinitelyError_Type.tp_base = (PyTypeObject *)PyExc_Exception;
    if (PyType_Ready(&PyRecursingInfinitelyError_Type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(mod, "RecursingInfinitelyError",
                              (PyObject *)&PyRecursingInfinitelyError_Type) < 0)
    {
        return -1;
    }

    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
