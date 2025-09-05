/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_BadArgument()

PyDoc_STRVAR(_tkinter_tkapp_eval__doc__,
"eval($self, script, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EVAL_METHODDEF    \
    {"eval", (PyCFunction)_tkinter_tkapp_eval, METH_O, _tkinter_tkapp_eval__doc__},

static PyObject *
_tkinter_tkapp_eval_impl(TkappObject *self, const char *script);

static PyObject *
_tkinter_tkapp_eval(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *script;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("eval", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t script_length;
    script = PyUnicode_AsUTF8AndSize(arg, &script_length);
    if (script == NULL) {
        goto exit;
    }
    if (strlen(script) != (size_t)script_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _tkinter_tkapp_eval_impl((TkappObject *)self, script);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_evalfile__doc__,
"evalfile($self, fileName, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EVALFILE_METHODDEF    \
    {"evalfile", (PyCFunction)_tkinter_tkapp_evalfile, METH_O, _tkinter_tkapp_evalfile__doc__},

static PyObject *
_tkinter_tkapp_evalfile_impl(TkappObject *self, const char *fileName);

static PyObject *
_tkinter_tkapp_evalfile(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *fileName;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("evalfile", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t fileName_length;
    fileName = PyUnicode_AsUTF8AndSize(arg, &fileName_length);
    if (fileName == NULL) {
        goto exit;
    }
    if (strlen(fileName) != (size_t)fileName_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _tkinter_tkapp_evalfile_impl((TkappObject *)self, fileName);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_record__doc__,
"record($self, script, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_RECORD_METHODDEF    \
    {"record", (PyCFunction)_tkinter_tkapp_record, METH_O, _tkinter_tkapp_record__doc__},

static PyObject *
_tkinter_tkapp_record_impl(TkappObject *self, const char *script);

static PyObject *
_tkinter_tkapp_record(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *script;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("record", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t script_length;
    script = PyUnicode_AsUTF8AndSize(arg, &script_length);
    if (script == NULL) {
        goto exit;
    }
    if (strlen(script) != (size_t)script_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _tkinter_tkapp_record_impl((TkappObject *)self, script);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_adderrorinfo__doc__,
"adderrorinfo($self, msg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_ADDERRORINFO_METHODDEF    \
    {"adderrorinfo", (PyCFunction)_tkinter_tkapp_adderrorinfo, METH_O, _tkinter_tkapp_adderrorinfo__doc__},

static PyObject *
_tkinter_tkapp_adderrorinfo_impl(TkappObject *self, const char *msg);

static PyObject *
_tkinter_tkapp_adderrorinfo(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *msg;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("adderrorinfo", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t msg_length;
    msg = PyUnicode_AsUTF8AndSize(arg, &msg_length);
    if (msg == NULL) {
        goto exit;
    }
    if (strlen(msg) != (size_t)msg_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _tkinter_tkapp_adderrorinfo_impl((TkappObject *)self, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_getint__doc__,
"getint($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETINT_METHODDEF    \
    {"getint", (PyCFunction)_tkinter_tkapp_getint, METH_O, _tkinter_tkapp_getint__doc__},

static PyObject *
_tkinter_tkapp_getint_impl(TkappObject *self, PyObject *arg);

static PyObject *
_tkinter_tkapp_getint(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;

    return_value = _tkinter_tkapp_getint_impl((TkappObject *)self, arg);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_getdouble__doc__,
"getdouble($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETDOUBLE_METHODDEF    \
    {"getdouble", (PyCFunction)_tkinter_tkapp_getdouble, METH_O, _tkinter_tkapp_getdouble__doc__},

static PyObject *
_tkinter_tkapp_getdouble_impl(TkappObject *self, PyObject *arg);

static PyObject *
_tkinter_tkapp_getdouble(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;

    return_value = _tkinter_tkapp_getdouble_impl((TkappObject *)self, arg);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_getboolean__doc__,
"getboolean($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETBOOLEAN_METHODDEF    \
    {"getboolean", (PyCFunction)_tkinter_tkapp_getboolean, METH_O, _tkinter_tkapp_getboolean__doc__},

static PyObject *
_tkinter_tkapp_getboolean_impl(TkappObject *self, PyObject *arg);

static PyObject *
_tkinter_tkapp_getboolean(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;

    return_value = _tkinter_tkapp_getboolean_impl((TkappObject *)self, arg);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprstring__doc__,
"exprstring($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRSTRING_METHODDEF    \
    {"exprstring", (PyCFunction)_tkinter_tkapp_exprstring, METH_O, _tkinter_tkapp_exprstring__doc__},

static PyObject *
_tkinter_tkapp_exprstring_impl(TkappObject *self, const char *s);

static PyObject *
_tkinter_tkapp_exprstring(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *s;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("exprstring", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t s_length;
    s = PyUnicode_AsUTF8AndSize(arg, &s_length);
    if (s == NULL) {
        goto exit;
    }
    if (strlen(s) != (size_t)s_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _tkinter_tkapp_exprstring_impl((TkappObject *)self, s);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprlong__doc__,
"exprlong($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRLONG_METHODDEF    \
    {"exprlong", (PyCFunction)_tkinter_tkapp_exprlong, METH_O, _tkinter_tkapp_exprlong__doc__},

static PyObject *
_tkinter_tkapp_exprlong_impl(TkappObject *self, const char *s);

static PyObject *
_tkinter_tkapp_exprlong(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *s;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("exprlong", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t s_length;
    s = PyUnicode_AsUTF8AndSize(arg, &s_length);
    if (s == NULL) {
        goto exit;
    }
    if (strlen(s) != (size_t)s_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _tkinter_tkapp_exprlong_impl((TkappObject *)self, s);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprdouble__doc__,
"exprdouble($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRDOUBLE_METHODDEF    \
    {"exprdouble", (PyCFunction)_tkinter_tkapp_exprdouble, METH_O, _tkinter_tkapp_exprdouble__doc__},

static PyObject *
_tkinter_tkapp_exprdouble_impl(TkappObject *self, const char *s);

static PyObject *
_tkinter_tkapp_exprdouble(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *s;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("exprdouble", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t s_length;
    s = PyUnicode_AsUTF8AndSize(arg, &s_length);
    if (s == NULL) {
        goto exit;
    }
    if (strlen(s) != (size_t)s_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _tkinter_tkapp_exprdouble_impl((TkappObject *)self, s);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_exprboolean__doc__,
"exprboolean($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRBOOLEAN_METHODDEF    \
    {"exprboolean", (PyCFunction)_tkinter_tkapp_exprboolean, METH_O, _tkinter_tkapp_exprboolean__doc__},

static PyObject *
_tkinter_tkapp_exprboolean_impl(TkappObject *self, const char *s);

static PyObject *
_tkinter_tkapp_exprboolean(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *s;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("exprboolean", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t s_length;
    s = PyUnicode_AsUTF8AndSize(arg, &s_length);
    if (s == NULL) {
        goto exit;
    }
    if (strlen(s) != (size_t)s_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _tkinter_tkapp_exprboolean_impl((TkappObject *)self, s);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_splitlist__doc__,
"splitlist($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_SPLITLIST_METHODDEF    \
    {"splitlist", (PyCFunction)_tkinter_tkapp_splitlist, METH_O, _tkinter_tkapp_splitlist__doc__},

static PyObject *
_tkinter_tkapp_splitlist_impl(TkappObject *self, PyObject *arg);

static PyObject *
_tkinter_tkapp_splitlist(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;

    return_value = _tkinter_tkapp_splitlist_impl((TkappObject *)self, arg);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_createcommand__doc__,
"createcommand($self, name, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATECOMMAND_METHODDEF    \
    {"createcommand", _PyCFunction_CAST(_tkinter_tkapp_createcommand), METH_FASTCALL, _tkinter_tkapp_createcommand__doc__},

static PyObject *
_tkinter_tkapp_createcommand_impl(TkappObject *self, const char *name,
                                  PyObject *func);

static PyObject *
_tkinter_tkapp_createcommand(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *name;
    PyObject *func;

    if (!_PyArg_CheckPositional("createcommand", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("createcommand", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(args[0], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    func = args[1];
    return_value = _tkinter_tkapp_createcommand_impl((TkappObject *)self, name, func);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_deletecommand__doc__,
"deletecommand($self, name, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_DELETECOMMAND_METHODDEF    \
    {"deletecommand", (PyCFunction)_tkinter_tkapp_deletecommand, METH_O, _tkinter_tkapp_deletecommand__doc__},

static PyObject *
_tkinter_tkapp_deletecommand_impl(TkappObject *self, const char *name);

static PyObject *
_tkinter_tkapp_deletecommand(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *name;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("deletecommand", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(arg, &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _tkinter_tkapp_deletecommand_impl((TkappObject *)self, name);

exit:
    return return_value;
}

#if defined(HAVE_CREATEFILEHANDLER)

PyDoc_STRVAR(_tkinter_tkapp_createfilehandler__doc__,
"createfilehandler($self, file, mask, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF    \
    {"createfilehandler", _PyCFunction_CAST(_tkinter_tkapp_createfilehandler), METH_FASTCALL, _tkinter_tkapp_createfilehandler__doc__},

static PyObject *
_tkinter_tkapp_createfilehandler_impl(TkappObject *self, PyObject *file,
                                      int mask, PyObject *func);

static PyObject *
_tkinter_tkapp_createfilehandler(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *file;
    int mask;
    PyObject *func;

    if (!_PyArg_CheckPositional("createfilehandler", nargs, 3, 3)) {
        goto exit;
    }
    file = args[0];
    mask = PyLong_AsInt(args[1]);
    if (mask == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = args[2];
    return_value = _tkinter_tkapp_createfilehandler_impl((TkappObject *)self, file, mask, func);

exit:
    return return_value;
}

#endif /* defined(HAVE_CREATEFILEHANDLER) */

#if defined(HAVE_CREATEFILEHANDLER)

PyDoc_STRVAR(_tkinter_tkapp_deletefilehandler__doc__,
"deletefilehandler($self, file, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF    \
    {"deletefilehandler", (PyCFunction)_tkinter_tkapp_deletefilehandler, METH_O, _tkinter_tkapp_deletefilehandler__doc__},

static PyObject *
_tkinter_tkapp_deletefilehandler_impl(TkappObject *self, PyObject *file);

static PyObject *
_tkinter_tkapp_deletefilehandler(PyObject *self, PyObject *file)
{
    PyObject *return_value = NULL;

    return_value = _tkinter_tkapp_deletefilehandler_impl((TkappObject *)self, file);

    return return_value;
}

#endif /* defined(HAVE_CREATEFILEHANDLER) */

PyDoc_STRVAR(_tkinter_tktimertoken_deletetimerhandler__doc__,
"deletetimerhandler($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKTIMERTOKEN_DELETETIMERHANDLER_METHODDEF    \
    {"deletetimerhandler", (PyCFunction)_tkinter_tktimertoken_deletetimerhandler, METH_NOARGS, _tkinter_tktimertoken_deletetimerhandler__doc__},

static PyObject *
_tkinter_tktimertoken_deletetimerhandler_impl(TkttObject *self);

static PyObject *
_tkinter_tktimertoken_deletetimerhandler(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tktimertoken_deletetimerhandler_impl((TkttObject *)self);
}

PyDoc_STRVAR(_tkinter_tkapp_createtimerhandler__doc__,
"createtimerhandler($self, milliseconds, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATETIMERHANDLER_METHODDEF    \
    {"createtimerhandler", _PyCFunction_CAST(_tkinter_tkapp_createtimerhandler), METH_FASTCALL, _tkinter_tkapp_createtimerhandler__doc__},

static PyObject *
_tkinter_tkapp_createtimerhandler_impl(TkappObject *self, int milliseconds,
                                       PyObject *func);

static PyObject *
_tkinter_tkapp_createtimerhandler(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int milliseconds;
    PyObject *func;

    if (!_PyArg_CheckPositional("createtimerhandler", nargs, 2, 2)) {
        goto exit;
    }
    milliseconds = PyLong_AsInt(args[0]);
    if (milliseconds == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = args[1];
    return_value = _tkinter_tkapp_createtimerhandler_impl((TkappObject *)self, milliseconds, func);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_mainloop__doc__,
"mainloop($self, threshold=0, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_MAINLOOP_METHODDEF    \
    {"mainloop", _PyCFunction_CAST(_tkinter_tkapp_mainloop), METH_FASTCALL, _tkinter_tkapp_mainloop__doc__},

static PyObject *
_tkinter_tkapp_mainloop_impl(TkappObject *self, int threshold);

static PyObject *
_tkinter_tkapp_mainloop(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int threshold = 0;

    if (!_PyArg_CheckPositional("mainloop", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    threshold = PyLong_AsInt(args[0]);
    if (threshold == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _tkinter_tkapp_mainloop_impl((TkappObject *)self, threshold);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_dooneevent__doc__,
"dooneevent($self, flags=0, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_DOONEEVENT_METHODDEF    \
    {"dooneevent", _PyCFunction_CAST(_tkinter_tkapp_dooneevent), METH_FASTCALL, _tkinter_tkapp_dooneevent__doc__},

static PyObject *
_tkinter_tkapp_dooneevent_impl(TkappObject *self, int flags);

static PyObject *
_tkinter_tkapp_dooneevent(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int flags = 0;

    if (!_PyArg_CheckPositional("dooneevent", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flags = PyLong_AsInt(args[0]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _tkinter_tkapp_dooneevent_impl((TkappObject *)self, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_quit__doc__,
"quit($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_QUIT_METHODDEF    \
    {"quit", (PyCFunction)_tkinter_tkapp_quit, METH_NOARGS, _tkinter_tkapp_quit__doc__},

static PyObject *
_tkinter_tkapp_quit_impl(TkappObject *self);

static PyObject *
_tkinter_tkapp_quit(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_quit_impl((TkappObject *)self);
}

PyDoc_STRVAR(_tkinter_tkapp_interpaddr__doc__,
"interpaddr($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_INTERPADDR_METHODDEF    \
    {"interpaddr", (PyCFunction)_tkinter_tkapp_interpaddr, METH_NOARGS, _tkinter_tkapp_interpaddr__doc__},

static PyObject *
_tkinter_tkapp_interpaddr_impl(TkappObject *self);

static PyObject *
_tkinter_tkapp_interpaddr(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_interpaddr_impl((TkappObject *)self);
}

PyDoc_STRVAR(_tkinter_tkapp_loadtk__doc__,
"loadtk($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_LOADTK_METHODDEF    \
    {"loadtk", (PyCFunction)_tkinter_tkapp_loadtk, METH_NOARGS, _tkinter_tkapp_loadtk__doc__},

static PyObject *
_tkinter_tkapp_loadtk_impl(TkappObject *self);

static PyObject *
_tkinter_tkapp_loadtk(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_loadtk_impl((TkappObject *)self);
}

PyDoc_STRVAR(_tkinter_tkapp_settrace__doc__,
"settrace($self, func, /)\n"
"--\n"
"\n"
"Set the tracing function.");

#define _TKINTER_TKAPP_SETTRACE_METHODDEF    \
    {"settrace", (PyCFunction)_tkinter_tkapp_settrace, METH_O, _tkinter_tkapp_settrace__doc__},

static PyObject *
_tkinter_tkapp_settrace_impl(TkappObject *self, PyObject *func);

static PyObject *
_tkinter_tkapp_settrace(PyObject *self, PyObject *func)
{
    PyObject *return_value = NULL;

    return_value = _tkinter_tkapp_settrace_impl((TkappObject *)self, func);

    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_gettrace__doc__,
"gettrace($self, /)\n"
"--\n"
"\n"
"Get the tracing function.");

#define _TKINTER_TKAPP_GETTRACE_METHODDEF    \
    {"gettrace", (PyCFunction)_tkinter_tkapp_gettrace, METH_NOARGS, _tkinter_tkapp_gettrace__doc__},

static PyObject *
_tkinter_tkapp_gettrace_impl(TkappObject *self);

static PyObject *
_tkinter_tkapp_gettrace(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_gettrace_impl((TkappObject *)self);
}

PyDoc_STRVAR(_tkinter_tkapp_willdispatch__doc__,
"willdispatch($self, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_WILLDISPATCH_METHODDEF    \
    {"willdispatch", (PyCFunction)_tkinter_tkapp_willdispatch, METH_NOARGS, _tkinter_tkapp_willdispatch__doc__},

static PyObject *
_tkinter_tkapp_willdispatch_impl(TkappObject *self);

static PyObject *
_tkinter_tkapp_willdispatch(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_willdispatch_impl((TkappObject *)self);
}

PyDoc_STRVAR(_tkinter__flatten__doc__,
"_flatten($module, item, /)\n"
"--\n"
"\n");

#define _TKINTER__FLATTEN_METHODDEF    \
    {"_flatten", (PyCFunction)_tkinter__flatten, METH_O, _tkinter__flatten__doc__},

PyDoc_STRVAR(_tkinter_create__doc__,
"create($module, screenName=None, baseName=\'\', className=\'Tk\',\n"
"       interactive=False, wantobjects=0, wantTk=True, sync=False,\n"
"       use=None, /)\n"
"--\n"
"\n"
"\n"
"\n"
"  wantTk\n"
"    if false, then Tk_Init() doesn\'t get called\n"
"  sync\n"
"    if true, then pass -sync to wish\n"
"  use\n"
"    if not None, then pass -use to wish");

#define _TKINTER_CREATE_METHODDEF    \
    {"create", _PyCFunction_CAST(_tkinter_create), METH_FASTCALL, _tkinter_create__doc__},

static PyObject *
_tkinter_create_impl(PyObject *module, const char *screenName,
                     const char *baseName, const char *className,
                     int interactive, int wantobjects, int wantTk, int sync,
                     const char *use);

static PyObject *
_tkinter_create(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *screenName = NULL;
    const char *baseName = "";
    const char *className = "Tk";
    int interactive = 0;
    int wantobjects = 0;
    int wantTk = 1;
    int sync = 0;
    const char *use = NULL;

    if (!_PyArg_CheckPositional("create", nargs, 0, 8)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (args[0] == Py_None) {
        screenName = NULL;
    }
    else if (PyUnicode_Check(args[0])) {
        Py_ssize_t screenName_length;
        screenName = PyUnicode_AsUTF8AndSize(args[0], &screenName_length);
        if (screenName == NULL) {
            goto exit;
        }
        if (strlen(screenName) != (size_t)screenName_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("create", "argument 1", "str or None", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("create", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t baseName_length;
    baseName = PyUnicode_AsUTF8AndSize(args[1], &baseName_length);
    if (baseName == NULL) {
        goto exit;
    }
    if (strlen(baseName) != (size_t)baseName_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("create", "argument 3", "str", args[2]);
        goto exit;
    }
    Py_ssize_t className_length;
    className = PyUnicode_AsUTF8AndSize(args[2], &className_length);
    if (className == NULL) {
        goto exit;
    }
    if (strlen(className) != (size_t)className_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    interactive = PyObject_IsTrue(args[3]);
    if (interactive < 0) {
        goto exit;
    }
    if (nargs < 5) {
        goto skip_optional;
    }
    wantobjects = PyLong_AsInt(args[4]);
    if (wantobjects == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 6) {
        goto skip_optional;
    }
    wantTk = PyObject_IsTrue(args[5]);
    if (wantTk < 0) {
        goto exit;
    }
    if (nargs < 7) {
        goto skip_optional;
    }
    sync = PyObject_IsTrue(args[6]);
    if (sync < 0) {
        goto exit;
    }
    if (nargs < 8) {
        goto skip_optional;
    }
    if (args[7] == Py_None) {
        use = NULL;
    }
    else if (PyUnicode_Check(args[7])) {
        Py_ssize_t use_length;
        use = PyUnicode_AsUTF8AndSize(args[7], &use_length);
        if (use == NULL) {
            goto exit;
        }
        if (strlen(use) != (size_t)use_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("create", "argument 8", "str or None", args[7]);
        goto exit;
    }
skip_optional:
    return_value = _tkinter_create_impl(module, screenName, baseName, className, interactive, wantobjects, wantTk, sync, use);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_setbusywaitinterval__doc__,
"setbusywaitinterval($module, new_val, /)\n"
"--\n"
"\n"
"Set the busy-wait interval in milliseconds between successive calls to Tcl_DoOneEvent in a threaded Python interpreter.\n"
"\n"
"It should be set to a divisor of the maximum time between frames in an animation.");

#define _TKINTER_SETBUSYWAITINTERVAL_METHODDEF    \
    {"setbusywaitinterval", (PyCFunction)_tkinter_setbusywaitinterval, METH_O, _tkinter_setbusywaitinterval__doc__},

static PyObject *
_tkinter_setbusywaitinterval_impl(PyObject *module, int new_val);

static PyObject *
_tkinter_setbusywaitinterval(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int new_val;

    new_val = PyLong_AsInt(arg);
    if (new_val == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _tkinter_setbusywaitinterval_impl(module, new_val);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_getbusywaitinterval__doc__,
"getbusywaitinterval($module, /)\n"
"--\n"
"\n"
"Return the current busy-wait interval between successive calls to Tcl_DoOneEvent in a threaded Python interpreter.");

#define _TKINTER_GETBUSYWAITINTERVAL_METHODDEF    \
    {"getbusywaitinterval", (PyCFunction)_tkinter_getbusywaitinterval, METH_NOARGS, _tkinter_getbusywaitinterval__doc__},

static int
_tkinter_getbusywaitinterval_impl(PyObject *module);

static PyObject *
_tkinter_getbusywaitinterval(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _tkinter_getbusywaitinterval_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#ifndef _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF
    #define _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF
#endif /* !defined(_TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF) */

#ifndef _TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF
    #define _TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF
#endif /* !defined(_TKINTER_TKAPP_DELETEFILEHANDLER_METHODDEF) */
/*[clinic end generated code: output=052c067aa69237be input=a9049054013a1b77]*/
