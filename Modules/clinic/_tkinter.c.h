/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_tkinter_tkapp_eval__doc__,
"eval($self, script, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EVAL_METHODDEF    \
    {"eval", (PyCFunction)_tkinter_tkapp_eval, METH_O, _tkinter_tkapp_eval__doc__},

static PyObject *
_tkinter_tkapp_eval_impl(TkappObject *self, const char *script);

static PyObject *
_tkinter_tkapp_eval(TkappObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *script;

    if (!PyArg_Parse(arg, "s:eval", &script)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_eval_impl(self, script);

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
_tkinter_tkapp_evalfile(TkappObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *fileName;

    if (!PyArg_Parse(arg, "s:evalfile", &fileName)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_evalfile_impl(self, fileName);

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
_tkinter_tkapp_record(TkappObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *script;

    if (!PyArg_Parse(arg, "s:record", &script)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_record_impl(self, script);

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
_tkinter_tkapp_adderrorinfo(TkappObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *msg;

    if (!PyArg_Parse(arg, "s:adderrorinfo", &msg)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_adderrorinfo_impl(self, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_getint__doc__,
"getint($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETINT_METHODDEF    \
    {"getint", (PyCFunction)_tkinter_tkapp_getint, METH_O, _tkinter_tkapp_getint__doc__},

PyDoc_STRVAR(_tkinter_tkapp_getdouble__doc__,
"getdouble($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETDOUBLE_METHODDEF    \
    {"getdouble", (PyCFunction)_tkinter_tkapp_getdouble, METH_O, _tkinter_tkapp_getdouble__doc__},

PyDoc_STRVAR(_tkinter_tkapp_getboolean__doc__,
"getboolean($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_GETBOOLEAN_METHODDEF    \
    {"getboolean", (PyCFunction)_tkinter_tkapp_getboolean, METH_O, _tkinter_tkapp_getboolean__doc__},

PyDoc_STRVAR(_tkinter_tkapp_exprstring__doc__,
"exprstring($self, s, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_EXPRSTRING_METHODDEF    \
    {"exprstring", (PyCFunction)_tkinter_tkapp_exprstring, METH_O, _tkinter_tkapp_exprstring__doc__},

static PyObject *
_tkinter_tkapp_exprstring_impl(TkappObject *self, const char *s);

static PyObject *
_tkinter_tkapp_exprstring(TkappObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *s;

    if (!PyArg_Parse(arg, "s:exprstring", &s)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_exprstring_impl(self, s);

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
_tkinter_tkapp_exprlong(TkappObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *s;

    if (!PyArg_Parse(arg, "s:exprlong", &s)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_exprlong_impl(self, s);

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
_tkinter_tkapp_exprdouble(TkappObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *s;

    if (!PyArg_Parse(arg, "s:exprdouble", &s)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_exprdouble_impl(self, s);

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
_tkinter_tkapp_exprboolean(TkappObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *s;

    if (!PyArg_Parse(arg, "s:exprboolean", &s)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_exprboolean_impl(self, s);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_splitlist__doc__,
"splitlist($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_SPLITLIST_METHODDEF    \
    {"splitlist", (PyCFunction)_tkinter_tkapp_splitlist, METH_O, _tkinter_tkapp_splitlist__doc__},

PyDoc_STRVAR(_tkinter_tkapp_split__doc__,
"split($self, arg, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_SPLIT_METHODDEF    \
    {"split", (PyCFunction)_tkinter_tkapp_split, METH_O, _tkinter_tkapp_split__doc__},

PyDoc_STRVAR(_tkinter_tkapp_createcommand__doc__,
"createcommand($self, name, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATECOMMAND_METHODDEF    \
    {"createcommand", (PyCFunction)_tkinter_tkapp_createcommand, METH_FASTCALL, _tkinter_tkapp_createcommand__doc__},

static PyObject *
_tkinter_tkapp_createcommand_impl(TkappObject *self, const char *name,
                                  PyObject *func);

static PyObject *
_tkinter_tkapp_createcommand(TkappObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *name;
    PyObject *func;

    if (!_PyArg_ParseStack(args, nargs, "sO:createcommand",
        &name, &func)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_createcommand_impl(self, name, func);

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
_tkinter_tkapp_deletecommand(TkappObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *name;

    if (!PyArg_Parse(arg, "s:deletecommand", &name)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_deletecommand_impl(self, name);

exit:
    return return_value;
}

#if defined(HAVE_CREATEFILEHANDLER)

PyDoc_STRVAR(_tkinter_tkapp_createfilehandler__doc__,
"createfilehandler($self, file, mask, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATEFILEHANDLER_METHODDEF    \
    {"createfilehandler", (PyCFunction)_tkinter_tkapp_createfilehandler, METH_FASTCALL, _tkinter_tkapp_createfilehandler__doc__},

static PyObject *
_tkinter_tkapp_createfilehandler_impl(TkappObject *self, PyObject *file,
                                      int mask, PyObject *func);

static PyObject *
_tkinter_tkapp_createfilehandler(TkappObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *file;
    int mask;
    PyObject *func;

    if (!_PyArg_ParseStack(args, nargs, "OiO:createfilehandler",
        &file, &mask, &func)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_createfilehandler_impl(self, file, mask, func);

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
_tkinter_tktimertoken_deletetimerhandler(TkttObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tktimertoken_deletetimerhandler_impl(self);
}

PyDoc_STRVAR(_tkinter_tkapp_createtimerhandler__doc__,
"createtimerhandler($self, milliseconds, func, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_CREATETIMERHANDLER_METHODDEF    \
    {"createtimerhandler", (PyCFunction)_tkinter_tkapp_createtimerhandler, METH_FASTCALL, _tkinter_tkapp_createtimerhandler__doc__},

static PyObject *
_tkinter_tkapp_createtimerhandler_impl(TkappObject *self, int milliseconds,
                                       PyObject *func);

static PyObject *
_tkinter_tkapp_createtimerhandler(TkappObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int milliseconds;
    PyObject *func;

    if (!_PyArg_ParseStack(args, nargs, "iO:createtimerhandler",
        &milliseconds, &func)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_createtimerhandler_impl(self, milliseconds, func);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_mainloop__doc__,
"mainloop($self, threshold=0, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_MAINLOOP_METHODDEF    \
    {"mainloop", (PyCFunction)_tkinter_tkapp_mainloop, METH_FASTCALL, _tkinter_tkapp_mainloop__doc__},

static PyObject *
_tkinter_tkapp_mainloop_impl(TkappObject *self, int threshold);

static PyObject *
_tkinter_tkapp_mainloop(TkappObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int threshold = 0;

    if (!_PyArg_ParseStack(args, nargs, "|i:mainloop",
        &threshold)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_mainloop_impl(self, threshold);

exit:
    return return_value;
}

PyDoc_STRVAR(_tkinter_tkapp_dooneevent__doc__,
"dooneevent($self, flags=0, /)\n"
"--\n"
"\n");

#define _TKINTER_TKAPP_DOONEEVENT_METHODDEF    \
    {"dooneevent", (PyCFunction)_tkinter_tkapp_dooneevent, METH_FASTCALL, _tkinter_tkapp_dooneevent__doc__},

static PyObject *
_tkinter_tkapp_dooneevent_impl(TkappObject *self, int flags);

static PyObject *
_tkinter_tkapp_dooneevent(TkappObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int flags = 0;

    if (!_PyArg_ParseStack(args, nargs, "|i:dooneevent",
        &flags)) {
        goto exit;
    }
    return_value = _tkinter_tkapp_dooneevent_impl(self, flags);

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
_tkinter_tkapp_quit(TkappObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_quit_impl(self);
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
_tkinter_tkapp_interpaddr(TkappObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_interpaddr_impl(self);
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
_tkinter_tkapp_loadtk(TkappObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_loadtk_impl(self);
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
_tkinter_tkapp_willdispatch(TkappObject *self, PyObject *Py_UNUSED(ignored))
{
    return _tkinter_tkapp_willdispatch_impl(self);
}

PyDoc_STRVAR(_tkinter__flatten__doc__,
"_flatten($module, item, /)\n"
"--\n"
"\n");

#define _TKINTER__FLATTEN_METHODDEF    \
    {"_flatten", (PyCFunction)_tkinter__flatten, METH_O, _tkinter__flatten__doc__},

PyDoc_STRVAR(_tkinter_create__doc__,
"create($module, screenName=None, baseName=None, className=\'Tk\',\n"
"       interactive=False, wantobjects=False, wantTk=True, sync=False,\n"
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
    {"create", (PyCFunction)_tkinter_create, METH_FASTCALL, _tkinter_create__doc__},

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
    const char *baseName = NULL;
    const char *className = "Tk";
    int interactive = 0;
    int wantobjects = 0;
    int wantTk = 1;
    int sync = 0;
    const char *use = NULL;

    if (!_PyArg_ParseStack(args, nargs, "|zssiiiiz:create",
        &screenName, &baseName, &className, &interactive, &wantobjects, &wantTk, &sync, &use)) {
        goto exit;
    }
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

    if (!PyArg_Parse(arg, "i:setbusywaitinterval", &new_val)) {
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
/*[clinic end generated code: output=eb3202e07db3dbb1 input=a9049054013a1b77]*/
