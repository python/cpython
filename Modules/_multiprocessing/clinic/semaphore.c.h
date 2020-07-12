/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_SemLock_acquire__doc__,
"acquire($self, /, block=True, timeout=None)\n"
"--\n"
"\n"
"Acquire the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK_ACQUIRE_METHODDEF    \
    {"acquire", (PyCFunction)(void(*)(void))_multiprocessing_SemLock_acquire, METH_FASTCALL|METH_KEYWORDS, _multiprocessing_SemLock_acquire__doc__},

static PyObject *
_multiprocessing_SemLock_acquire_impl(SemLockObject *self, int blocking,
                                      PyObject *timeout_obj);

static PyObject *
_multiprocessing_SemLock_acquire(SemLockObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"block", "timeout", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "acquire", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int blocking = 1;
    PyObject *timeout_obj = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        blocking = _PyLong_AsInt(args[0]);
        if (blocking == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    timeout_obj = args[1];
skip_optional_pos:
    return_value = _multiprocessing_SemLock_acquire_impl(self, blocking, timeout_obj);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_SemLock_release__doc__,
"release($self, /)\n"
"--\n"
"\n"
"Release the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK_RELEASE_METHODDEF    \
    {"release", (PyCFunction)_multiprocessing_SemLock_release, METH_NOARGS, _multiprocessing_SemLock_release__doc__},

static PyObject *
_multiprocessing_SemLock_release_impl(SemLockObject *self);

static PyObject *
_multiprocessing_SemLock_release(SemLockObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multiprocessing_SemLock_release_impl(self);
}

#endif /* defined(MS_WINDOWS) */

#if !defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_SemLock_acquire__doc__,
"acquire($self, /, block=True, timeout=None)\n"
"--\n"
"\n"
"Acquire the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK_ACQUIRE_METHODDEF    \
    {"acquire", (PyCFunction)(void(*)(void))_multiprocessing_SemLock_acquire, METH_FASTCALL|METH_KEYWORDS, _multiprocessing_SemLock_acquire__doc__},

static PyObject *
_multiprocessing_SemLock_acquire_impl(SemLockObject *self, int blocking,
                                      PyObject *timeout_obj);

static PyObject *
_multiprocessing_SemLock_acquire(SemLockObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"block", "timeout", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "acquire", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int blocking = 1;
    PyObject *timeout_obj = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        blocking = _PyLong_AsInt(args[0]);
        if (blocking == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    timeout_obj = args[1];
skip_optional_pos:
    return_value = _multiprocessing_SemLock_acquire_impl(self, blocking, timeout_obj);

exit:
    return return_value;
}

#endif /* !defined(MS_WINDOWS) */

#if !defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_SemLock_release__doc__,
"release($self, /)\n"
"--\n"
"\n"
"Release the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK_RELEASE_METHODDEF    \
    {"release", (PyCFunction)_multiprocessing_SemLock_release, METH_NOARGS, _multiprocessing_SemLock_release__doc__},

static PyObject *
_multiprocessing_SemLock_release_impl(SemLockObject *self);

static PyObject *
_multiprocessing_SemLock_release(SemLockObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multiprocessing_SemLock_release_impl(self);
}

#endif /* !defined(MS_WINDOWS) */

static PyObject *
_multiprocessing_SemLock_impl(PyTypeObject *type, int kind, int value,
                              int maxvalue, const char *name, int unlink);

static PyObject *
_multiprocessing_SemLock(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"kind", "value", "maxvalue", "name", "unlink", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "SemLock", 0};
    PyObject *argsbuf[5];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    int kind;
    int value;
    int maxvalue;
    const char *name;
    int unlink;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 5, 5, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    kind = _PyLong_AsInt(fastargs[0]);
    if (kind == -1 && PyErr_Occurred()) {
        goto exit;
    }
    value = _PyLong_AsInt(fastargs[1]);
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
    maxvalue = _PyLong_AsInt(fastargs[2]);
    if (maxvalue == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[3])) {
        _PyArg_BadArgument("SemLock", "argument 'name'", "str", fastargs[3]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(fastargs[3], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    unlink = _PyLong_AsInt(fastargs[4]);
    if (unlink == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _multiprocessing_SemLock_impl(type, kind, value, maxvalue, name, unlink);

exit:
    return return_value;
}

PyDoc_STRVAR(_multiprocessing_SemLock__rebuild__doc__,
"_rebuild($type, handle, kind, maxvalue, name, /)\n"
"--\n"
"\n");

#define _MULTIPROCESSING_SEMLOCK__REBUILD_METHODDEF    \
    {"_rebuild", (PyCFunction)(void(*)(void))_multiprocessing_SemLock__rebuild, METH_FASTCALL|METH_CLASS, _multiprocessing_SemLock__rebuild__doc__},

static PyObject *
_multiprocessing_SemLock__rebuild_impl(PyTypeObject *type, SEM_HANDLE handle,
                                       int kind, int maxvalue,
                                       const char *name);

static PyObject *
_multiprocessing_SemLock__rebuild(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    SEM_HANDLE handle;
    int kind;
    int maxvalue;
    const char *name;

    if (!_PyArg_ParseStack(args, nargs, ""F_SEM_HANDLE"iiz:_rebuild",
        &handle, &kind, &maxvalue, &name)) {
        goto exit;
    }
    return_value = _multiprocessing_SemLock__rebuild_impl(type, handle, kind, maxvalue, name);

exit:
    return return_value;
}

PyDoc_STRVAR(_multiprocessing_SemLock__count__doc__,
"_count($self, /)\n"
"--\n"
"\n"
"Num of `acquire()`s minus num of `release()`s for this process.");

#define _MULTIPROCESSING_SEMLOCK__COUNT_METHODDEF    \
    {"_count", (PyCFunction)_multiprocessing_SemLock__count, METH_NOARGS, _multiprocessing_SemLock__count__doc__},

static PyObject *
_multiprocessing_SemLock__count_impl(SemLockObject *self);

static PyObject *
_multiprocessing_SemLock__count(SemLockObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multiprocessing_SemLock__count_impl(self);
}

PyDoc_STRVAR(_multiprocessing_SemLock__is_mine__doc__,
"_is_mine($self, /)\n"
"--\n"
"\n"
"Whether the lock is owned by this thread.");

#define _MULTIPROCESSING_SEMLOCK__IS_MINE_METHODDEF    \
    {"_is_mine", (PyCFunction)_multiprocessing_SemLock__is_mine, METH_NOARGS, _multiprocessing_SemLock__is_mine__doc__},

static PyObject *
_multiprocessing_SemLock__is_mine_impl(SemLockObject *self);

static PyObject *
_multiprocessing_SemLock__is_mine(SemLockObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multiprocessing_SemLock__is_mine_impl(self);
}

PyDoc_STRVAR(_multiprocessing_SemLock__get_value__doc__,
"_get_value($self, /)\n"
"--\n"
"\n"
"Get the value of the semaphore.");

#define _MULTIPROCESSING_SEMLOCK__GET_VALUE_METHODDEF    \
    {"_get_value", (PyCFunction)_multiprocessing_SemLock__get_value, METH_NOARGS, _multiprocessing_SemLock__get_value__doc__},

static PyObject *
_multiprocessing_SemLock__get_value_impl(SemLockObject *self);

static PyObject *
_multiprocessing_SemLock__get_value(SemLockObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multiprocessing_SemLock__get_value_impl(self);
}

PyDoc_STRVAR(_multiprocessing_SemLock__is_zero__doc__,
"_is_zero($self, /)\n"
"--\n"
"\n"
"Return whether semaphore has value zero.");

#define _MULTIPROCESSING_SEMLOCK__IS_ZERO_METHODDEF    \
    {"_is_zero", (PyCFunction)_multiprocessing_SemLock__is_zero, METH_NOARGS, _multiprocessing_SemLock__is_zero__doc__},

static PyObject *
_multiprocessing_SemLock__is_zero_impl(SemLockObject *self);

static PyObject *
_multiprocessing_SemLock__is_zero(SemLockObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multiprocessing_SemLock__is_zero_impl(self);
}

PyDoc_STRVAR(_multiprocessing_SemLock__after_fork__doc__,
"_after_fork($self, /)\n"
"--\n"
"\n"
"Rezero the net acquisition count after fork().");

#define _MULTIPROCESSING_SEMLOCK__AFTER_FORK_METHODDEF    \
    {"_after_fork", (PyCFunction)_multiprocessing_SemLock__after_fork, METH_NOARGS, _multiprocessing_SemLock__after_fork__doc__},

static PyObject *
_multiprocessing_SemLock__after_fork_impl(SemLockObject *self);

static PyObject *
_multiprocessing_SemLock__after_fork(SemLockObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multiprocessing_SemLock__after_fork_impl(self);
}

PyDoc_STRVAR(_multiprocessing_SemLock___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Enter the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK___ENTER___METHODDEF    \
    {"__enter__", (PyCFunction)_multiprocessing_SemLock___enter__, METH_NOARGS, _multiprocessing_SemLock___enter____doc__},

static PyObject *
_multiprocessing_SemLock___enter___impl(SemLockObject *self);

static PyObject *
_multiprocessing_SemLock___enter__(SemLockObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multiprocessing_SemLock___enter___impl(self);
}

PyDoc_STRVAR(_multiprocessing_SemLock___exit____doc__,
"__exit__($self, exc_type=None, exc_value=None, exc_tb=None, /)\n"
"--\n"
"\n"
"Exit the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK___EXIT___METHODDEF    \
    {"__exit__", (PyCFunction)(void(*)(void))_multiprocessing_SemLock___exit__, METH_FASTCALL, _multiprocessing_SemLock___exit____doc__},

static PyObject *
_multiprocessing_SemLock___exit___impl(SemLockObject *self,
                                       PyObject *exc_type,
                                       PyObject *exc_value, PyObject *exc_tb);

static PyObject *
_multiprocessing_SemLock___exit__(SemLockObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc_type = Py_None;
    PyObject *exc_value = Py_None;
    PyObject *exc_tb = Py_None;

    if (!_PyArg_CheckPositional("__exit__", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    exc_type = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    exc_value = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    exc_tb = args[2];
skip_optional:
    return_value = _multiprocessing_SemLock___exit___impl(self, exc_type, exc_value, exc_tb);

exit:
    return return_value;
}

#ifndef _MULTIPROCESSING_SEMLOCK_ACQUIRE_METHODDEF
    #define _MULTIPROCESSING_SEMLOCK_ACQUIRE_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK_ACQUIRE_METHODDEF) */

#ifndef _MULTIPROCESSING_SEMLOCK_RELEASE_METHODDEF
    #define _MULTIPROCESSING_SEMLOCK_RELEASE_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK_RELEASE_METHODDEF) */
/*[clinic end generated code: output=e7fd938150601fe5 input=a9049054013a1b77]*/
