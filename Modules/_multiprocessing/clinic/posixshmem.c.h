/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(HAVE_SHM_OPEN)

PyDoc_STRVAR(_posixshmem_shm_open__doc__,
"shm_open($module, /, path, flags, mode=511)\n"
"--\n"
"\n"
"Open a shared memory object.  Returns a file descriptor (integer).");

#define _POSIXSHMEM_SHM_OPEN_METHODDEF    \
    {"shm_open", _PyCFunction_CAST(_posixshmem_shm_open), METH_FASTCALL|METH_KEYWORDS, _posixshmem_shm_open__doc__},

static int
_posixshmem_shm_open_impl(PyObject *module, PyObject *path, int flags,
                          int mode);

static PyObject *
_posixshmem_shm_open(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "flags", "mode", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "shm_open", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *path;
    int flags;
    int mode = 511;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("shm_open", "argument 'path'", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    path = args[0];
    flags = _PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    mode = _PyLong_AsInt(args[2]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    _return_value = _posixshmem_shm_open_impl(module, path, flags, mode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SHM_OPEN) */

#if defined(HAVE_SHM_UNLINK)

PyDoc_STRVAR(_posixshmem_shm_unlink__doc__,
"shm_unlink($module, /, path)\n"
"--\n"
"\n"
"Remove a shared memory object (similar to unlink()).\n"
"\n"
"Remove a shared memory object name, and, once all processes  have  unmapped\n"
"the object, de-allocates and destroys the contents of the associated memory\n"
"region.");

#define _POSIXSHMEM_SHM_UNLINK_METHODDEF    \
    {"shm_unlink", _PyCFunction_CAST(_posixshmem_shm_unlink), METH_FASTCALL|METH_KEYWORDS, _posixshmem_shm_unlink__doc__},

static PyObject *
_posixshmem_shm_unlink_impl(PyObject *module, PyObject *path);

static PyObject *
_posixshmem_shm_unlink(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "shm_unlink", 0};
    PyObject *argsbuf[1];
    PyObject *path;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("shm_unlink", "argument 'path'", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    path = args[0];
    return_value = _posixshmem_shm_unlink_impl(module, path);

exit:
    return return_value;
}

#endif /* defined(HAVE_SHM_UNLINK) */

#ifndef _POSIXSHMEM_SHM_OPEN_METHODDEF
    #define _POSIXSHMEM_SHM_OPEN_METHODDEF
#endif /* !defined(_POSIXSHMEM_SHM_OPEN_METHODDEF) */

#ifndef _POSIXSHMEM_SHM_UNLINK_METHODDEF
    #define _POSIXSHMEM_SHM_UNLINK_METHODDEF
#endif /* !defined(_POSIXSHMEM_SHM_UNLINK_METHODDEF) */
/*[clinic end generated code: output=a6db931a47d36e1b input=a9049054013a1b77]*/
